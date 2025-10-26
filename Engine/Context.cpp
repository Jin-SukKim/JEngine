#include "pch.h"
#include "Context.h"

namespace JEngine {

void Context::createDevice() {
    UINT dxgiFactoryFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    {
        // Enable the D3D12 debug layer.
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();

        // Enable DXGI debug layer
        dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        LogInfo("D3D12 Debug Layer enabled for Debug build.");
    }
#endif

    // CreateDXGIFactory2 권장 (DirectX 12 표준)
    ThrowIfFailed(::CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory_)));

    // Try to create hardware device first
    HRESULT hr = ::D3D12CreateDevice(
        nullptr,                    // default adapter (하드웨어 GPU)
        D3D_FEATURE_LEVEL_12_0,     // minimum feature level
        IID_PPV_ARGS(&device_));

    // Fallback to WARP device if hardware device creation failed
    if (FAILED(hr))
    {
        LogWarning("Hardware device creation failed. Falling back to WARP device (software rendering).");
        
        ComPtr<IDXGIAdapter> warpAdapter; // Display Adapter
        ThrowIfFailed(dxgiFactory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(::D3D12CreateDevice(
            warpAdapter.Get(),          // WARP adapter
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&device_)));
        
        LogInfo("WARP device created successfully.");
    }
    else
    {
        LogInfo("Hardware device created successfully.");
    }

    // CPU와 GPU 간 리소스 동기화를 위한 Fence 생성
    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
    LogInfo("Fence created for CPU-GPU synchronization.");

    // Descriptor size for CBV/SRV/UAV
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvSrvUavDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    LogInfo("Descriptor sizes - RTV: {}, DSV: {}, CBV/SRV/UAV: {}", 
        rtvDescriptorSize_, dsvDescriptorSize_, cbvSrvUavDescriptorSize_);

    // check 4x MSAA quality support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
    msaaQualityLevels.Format = backBufferFormat_;
    msaaQualityLevels.SampleCount = 4;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;  
    msaaQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(device_->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msaaQualityLevels, sizeof(msaaQualityLevels)));

    m4xMsaaQuality_ = msaaQualityLevels.NumQualityLevels;
    if (m4xMsaaQuality_ > 0) 
        ExitWithMessage("4x MSAA is not supported for the back buffer format!");
    LogInfo("4x MSAA Quality Levels supported: {}", m4xMsaaQuality_);
}

void Context::createCommandObjects() {
    // Create Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Direct Command Queue
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
    LogInfo("Command Queue created.");

    // Create Command Allocator
    ThrowIfFailed(device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, // Command Queue의 Type과 동일해야 함
        IID_PPV_ARGS(commandAllocator_.GetAddressOf())));
    LogInfo("Command Allocator created.");

    ThrowIfFailed(device_->CreateCommandList(
        0, // single GPU node
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(), // Command Allocator와 연결
        nullptr, // 초기 PSO 없음
        IID_PPV_ARGS(commandList_.GetAddressOf())));
    LogInfo("Graphics Command List created.");

    // Close Command List to prepare it for reset before recording commands
    commandList_->Close();
    LogInfo("Command List closed and ready for recording commands.");
}

void Context::createSwapChain() {
    // Release previous swap chain if exists
    swapChain_.Reset(); // 나중에 프로그램 실행 도중에도 Swap Chain 재생성 가능

    // DXGI 1.2+ Flip Model 사용 (CreateSwapChainForHwnd 권장)
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = screenWidth_;                        // Buffer width
    sd.Height = screenHeight_;                      // Buffer height
    sd.Format = backBufferFormat_;                  // Back buffer format
    sd.Stereo = FALSE;                              // 2D 모니터 설정
    // Multi-sampling 설정
    sd.SampleDesc.Count = m4xMsaaQuality_ > 0 ? 4 : 1;                      // 4x MSAA or no multi-sampling
    sd.SampleDesc.Quality = m4xMsaaQuality_ ? m4xMsaaQuality_ - 1 : 0;      // Quality level
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // Render Target으로 사용
    sd.BufferCount = bufferCount_;                      // Double buffering
    sd.Scaling = DXGI_SCALING_STRETCH;                  // 확대/축소 방식
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;      // Flip Model (DXGI 1.2+)
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;         // back Buffer Alpha 값 처리 방식
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;  // Allow full-screen mode switch

    // Fullscreen/Windowed 설정
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.RefreshRate.Numerator = 60;                      // 60 Hz
    fsDesc.RefreshRate.Denominator = 1;                     // 60/1 = 60 Hz
    fsDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // Scanline ordering (화면을 그리는 순서)
    fsDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;         // 스케일링 방식
    fsDesc.Windowed = TRUE;                                 // Windowed mode

    // CreateSwapChainForHwnd 사용 (DXGI 1.2+ 표준, IDXGISwapChain1 반환)
    ComPtr<IDXGISwapChain1> tempSwapChain;
    ThrowIfFailed(dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),            // Command Queue 연결
        mainWnd_,                       // Output window handle
        &sd,                            // DXGI_SWAP_CHAIN_DESC1
        &fsDesc,                        // Fullscreen/windowed desc
        nullptr, // 모든 모니터에 대해 허용
        tempSwapChain.GetAddressOf()));

    // QueryInterface로 IDXGISwapChain3로 업그레이드 (D3D12 표준)
    ThrowIfFailed(tempSwapChain.As(&swapChain_));
    LogInfo("Swap Chain created.");
}

void Context::createDescriptorHeaps() {
    // Swap Chain의 Buffer 수에 맞춰 Descriptor Heap 생성
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = bufferCount_; // Swap Chain Buffer 수
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // Render Target View 용
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //
    rtvHeapDesc.NodeMask = 0;                            // single GPU node
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));
    LogInfo("Render Target View Descriptor Heap created with {} descriptors.", bufferCount_);

    // Depth Stencil View Descriptor Heap 생성
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1; // Depth Stencil View 하나만 생성
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; //
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0; // single GPU node
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_)));    
    LogInfo("Depth Stencil View Descriptor Heap created.");
}

void Context::createRenderTargetViews() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < bufferCount_; ++i) {
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])));
        device_->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize_;
    }
    LogInfo("Render Target Views created for all back buffers.");
}

void Context::createDepthStencilView() {
    // Create Depth Stencil Buffer
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = screenWidth_;
    depthStencilDesc.Height = screenHeight_;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    // 나중에 이 Depth Stencil Buffer를 SRV, DSV로 둘 다 사용하기 위해 TYPELESS 포맷 사용
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = m4xMsaaQuality_ > 0 ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality_ > 0 ? m4xMsaaQuality_ - 1 : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Clear Value 설정
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = depthStencilFormat_;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    // Heap Properties 직접 설정 (CD3DX12_HEAP_PROPERTIES 대체)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;           // GPU 전용 메모리
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;                     // Single GPU
    heapProps.VisibleNodeMask = 1;                      // Single GPU

    // CreateCommittedResource 호출
    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProps,                         // Heap Properties 
        D3D12_HEAP_FLAG_NONE,               // Heap Flags
        &depthStencilDesc,                  // Resource Description
        D3D12_RESOURCE_STATE_COMMON,        // Initial State
        &optClear,                          // Optimized Clear Value
        IID_PPV_ARGS(depthStencilBuffer_.GetAddressOf())));
    
    LogInfo("Depth Stencil Buffer created.");

    // Create Depth Stencil View
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS; // Multi-sampled 2D texture
    dsvDesc.Format = depthStencilFormat_;
    dsvDesc.Texture2D.MipSlice = 0;
    device_->CreateDepthStencilView(
        depthStencilBuffer_.Get(),
        &dsvDesc,
        GetDepthStencilView());
    LogInfo("Depth Stencil View created.");

    // Transition the resource to be used as a depth buffer
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = depthStencilBuffer_.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    commandList_->ResourceBarrier(1, &barrier);
    LogInfo("Depth Stencil Buffer transitioned to DEPTH_WRITE state.");
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::GetCurrentBackBufferView() const {
    // Descriptor Handle을 통해 Descriptor 접근

    // Descriptor Heap의 시작 주소 가져오기
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    
    // 현재 Back Buffer의 Descriptor 위치 계산
    // ptr = 시작 주소 + (현재 버퍼 인덱스 × Descriptor 크기)
    handle.ptr += curBackBufferIdx_ * rtvDescriptorSize_;
    
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::GetDepthStencilView() const {
    return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* Context::GetCurrentBackBuffer() const {
    return nullptr;
}

void Context::SetViewport() {
    screenViewport_.TopLeftX = 0.0f;
    screenViewport_.TopLeftY = 0.0f;
    screenViewport_.Width = static_cast<float>(screenWidth_);
    screenViewport_.Height = static_cast<float>(screenHeight_);
    screenViewport_.MinDepth = 0.0f;
    screenViewport_.MaxDepth = 1.0f;

    scissorRect_ = {0, 0, static_cast<LONG>(screenWidth_), static_cast<LONG>(screenHeight_)};

    commandList_->RSSetViewports(1, &screenViewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
}




} // namespace JEngine