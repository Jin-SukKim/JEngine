#include "pch.h"
#include "Context.h"


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return JEngine::Context::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

namespace JEngine {

Context* Context::app_ = nullptr;
Context* Context::GetApp() {
    return app_;
}
Context::Context(HINSTANCE hInstance) : appInst_(hInstance) {
    // 1개의 Context 인스턴스만 존재하도록 설정
    if (app_ != nullptr) {
        ExitWithMessage("Context instance already exists!");
    }
    app_ = this;
    LogInfo("Context instance created.");
}

Context::~Context() {
    if (device_) 
        FlushCommandQueue();
}

void Context::Initialize() {
    InitWindow();

    LogInfo("=== Initializing Context ===");
    createDevice();
    createCommandObjects();
    createSwapChain();
    createDescriptorHeaps();
    OnResize();
    LogInfo("=== Context Initialization Complete ===\n");
}


void Context::createDevice() {
    UINT dxgiFactoryFlags = 0;
    
    LogInfo("=== Initializing Direct3D 12 Device ===");

#if defined(DEBUG) || defined(_DEBUG)
    {
        // Debug 빌드에서 D3D12 디버그 레이어 활성화
        // - 런타임 에러 검출 및 경고 메시지 출력
        // - 성능 저하가 있으므로 Release 빌드에서는 비활성화
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();

        // DXGI 디버그 레이어도 함께 활성화
        dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        LogInfo("D3D12 Debug Layer enabled for Debug build.");
    }
#else
    LogInfo("Running in Release mode (Debug Layer disabled).");
#endif

    // DXGI Factory 생성 (DirectX 12 표준 방식)
    // - Adapter 열거, Swap Chain 생성 등에 사용
    LogInfo("Creating DXGI Factory...");
    ThrowIfFailed(::CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory_)));
    LogInfo("DXGI Factory created successfully.");

    // Hardware Device 생성 시도 (물리 GPU 사용)
    LogInfo("Attempting to create hardware device...");
    HRESULT hr = ::D3D12CreateDevice(
        nullptr,                    // nullptr = 기본 Adapter (주 GPU)
        D3D_FEATURE_LEVEL_12_0,     // Direct3D 12.0 기능 레벨 요구
        IID_PPV_ARGS(&device_));

    // Hardware Device 생성 실패 시 WARP Device로 폴백
    // WARP = Windows Advanced Rasterization Platform (소프트웨어 렌더러)
    if (FAILED(hr))
    {
        LogWarning("Hardware device creation failed (HRESULT: 0x{:X}). Falling back to WARP device.", static_cast<unsigned int>(hr));
        
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(dxgiFactory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(::D3D12CreateDevice(
            warpAdapter.Get(),      // WARP Adapter 사용
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&device_)));
        
        LogInfo("WARP device created successfully.");
    }
    else
    {
        LogInfo("Hardware device created successfully.");
    }

    // Fence 생성 (CPU-GPU 동기화용)
    // - GPU 작업 완료를 CPU에서 확인하기 위한 동기화 객체
    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
    LogInfo("Fence created for CPU-GPU synchronization.");

    // Descriptor 크기 조회 (GPU마다 다를 수 있음)
    // - Descriptor Heap에서 다음 Descriptor로 이동할 때 필요한 오프셋 크기
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvSrvUavDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    LogInfo("Descriptor sizes - RTV: {}, DSV: {}, CBV/SRV/UAV: {}", 
        rtvDescriptorSize_, dsvDescriptorSize_, cbvSrvUavDescriptorSize_);

    LogInfo("=== Direct3D 12 Device Initialization Complete ===\n");
}

void Context::createCommandObjects() {
    LogInfo("=== Creating Command Objects ===");
    
    // Command Queue 생성 (GPU 명령 제출용 큐)
    // - Direct Queue = Graphics + Compute 명령 실행 가능
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
    LogInfo("Command Queue created.");

    // Command Allocator 생성 (Command List의 메모리 관리자)
    // - Command List에 기록된 명령들을 저장하는 메모리 공간
    // - 프레임마다 Reset하여 재사용
    ThrowIfFailed(device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(commandAllocator_.GetAddressOf())));
    LogInfo("Command Allocator created.");

    // Command List 생성 (렌더링 명령 기록용)
    // - CPU에서 명령을 기록하고, GPU에서 실행
    // - Vulkan의 VkCommandBuffer와 유사
    ThrowIfFailed(device_->CreateCommandList(
        0,                              // Single GPU
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(),
        nullptr,                        // 초기 Pipeline State 없음
        IID_PPV_ARGS(commandList_.GetAddressOf())));
    LogInfo("Graphics Command List created.");

    // Command List를 닫은 상태로 초기화
    // - 명령 기록 전에 반드시 Reset() 호출 필요
    commandList_->Close();
    LogInfo("Command List closed and ready for recording commands.");
    
    LogInfo("=== Command Objects Creation Complete ===\n");
}

void Context::createSwapChain() {
    LogInfo("=== Creating Swap Chain ===");
    
    // 기존 Swap Chain 해제 (창 크기 변경 등으로 재생성 시 필요)
    swapChain_.Reset();

    // DXGI 1.2+ Flip Model 사용 (현대적 방식)
    // - Legacy BitBlt Model보다 성능 우수
    LogInfo("Configuring Swap Chain ({}x{}, Buffers: {})...", screenWidth_, screenHeight_, bufferCount_);
    
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = screenWidth_;
    sd.Height = screenHeight_;
    sd.Format = backBufferFormat_;
    sd.Stereo = FALSE;                              // VR/3D 안경 모드 비활성화
    // Multi-sampling 비활성화
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = bufferCount_;                  // Double Buffering (2개)
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // Flip Model (최고 성능)
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;     // 투명 윈도우 아님
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Fullscreen/Windowed 설정
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.RefreshRate.Numerator = 60;
    fsDesc.RefreshRate.Denominator = 1;             // 60 Hz
    fsDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    fsDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fsDesc.Windowed = TRUE;

    // CreateSwapChainForHwnd 사용 (DXGI 1.2+ 표준)
    // - Legacy CreateSwapChain()보다 Flip Model에 최적화
    ComPtr<IDXGISwapChain1> tempSwapChain;
    ThrowIfFailed(dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        mainWnd_,
        &sd,
        &fsDesc,
        nullptr,                        // Output 제한 없음 (모든 모니터 허용)
        tempSwapChain.GetAddressOf()));

    // IDXGISwapChain1 → IDXGISwapChain4로 업그레이드
    // - GetCurrentBackBufferIndex() 등 D3D12 필수 메서드 사용 가능
    ThrowIfFailed(tempSwapChain.As(&swapChain_));
    LogInfo("Swap Chain created successfully.");
    
    LogInfo("=== Swap Chain Creation Complete ===\n");
}

void Context::createDescriptorHeaps() {
    LogInfo("=== Creating Descriptor Heaps ===");
    
    // Render Target View Descriptor Heap 생성
    // - Back Buffer 수만큼 RTV 저장 공간 확보
    // - Heap = View들을 담는 "배열" 컨테이너
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = bufferCount_;      // Double Buffering이므로 2개
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // CPU 전용 Heap
    rtvHeapDesc.NodeMask = 0;                       // Single GPU
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));
    LogInfo("Render Target View Descriptor Heap created with {} descriptors.", bufferCount_);

    // Depth Stencil View Descriptor Heap 생성
    // - DSV는 1개만 필요 (모든 Back Buffer가 공유)
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_)));
    LogInfo("Depth Stencil View Descriptor Heap created.");
    
    LogInfo("=== Descriptor Heaps Creation Complete ===\n");
}

void Context::createRenderTargetViews() {
    LogInfo("=== Creating Render Target Views ===");
    
    // Descriptor Heap의 시작 위치 가져오기
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    
    // 각 Back Buffer에 대해 RTV 생성
    for (UINT i = 0; i < bufferCount_; ++i) {
        // Swap Chain에서 Back Buffer 리소스 가져오기
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])));
        
        // Heap의 현재 위치에 RTV 생성
        // - nullptr = 기본 설정 사용 (전체 리소스를 Render Target으로)
        device_->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, rtvHandle);
        
        // 다음 Descriptor 위치로 이동
        rtvHandle.ptr += rtvDescriptorSize_;
    }
    LogInfo("Render Target Views created for all back buffers.");
    
    LogInfo("=== Render Target Views Creation Complete ===\n");
}

void Context::createDepthStencilView() {
    LogInfo("=== Creating Depth Stencil Buffer & View ===");
    
    // === Depth Stencil Buffer 생성 ===
    
    // Resource Description 설정
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2D 텍스처
    depthStencilDesc.Alignment = 0;                                   // 기본 정렬
    depthStencilDesc.Width = screenWidth_;
    depthStencilDesc.Height = screenHeight_;
    depthStencilDesc.DepthOrArraySize = 1;                            // 단일 텍스처
    depthStencilDesc.MipLevels = 1;                                   // Mipmap 없음
    // TYPELESS 포맷 사용 = 나중에 DSV, SRV 등으로 다양하게 해석 가능
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // Depth Stencil 사용 플래그

    // Optimized Clear Value 설정 (성능 최적화)
    // - GPU가 Clear 작업을 빠르게 수행하도록 힌트 제공
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = depthStencilFormat_;          // DSV 포맷 (TYPELESS가 아님)
    optClear.DepthStencil.Depth = 1.0f;            // 최대 깊이 (먼 거리)
    optClear.DepthStencil.Stencil = 0;

    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;       // GPU 전용 메모리 (가장 빠름)
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;                 // Single GPU
    heapProps.VisibleNodeMask = 1;

    // Committed Resource 생성 (Resource + Heap 동시 생성)
    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,                // 초기 상태 (아직 사용 전)
        &optClear,
        IID_PPV_ARGS(depthStencilBuffer_.GetAddressOf())));
    
    LogInfo("Depth Stencil Buffer created.");

    // === Depth Stencil View 생성 ===
    
    // DSV Description 설정
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;  // 2D 텍스처
    dsvDesc.Format = depthStencilFormat_;                   // D24_UNORM_S8_UINT
    dsvDesc.Texture2D.MipSlice = 0;
    
    // Heap의 시작 위치에 DSV 생성
    device_->CreateDepthStencilView(
        depthStencilBuffer_.Get(),
        &dsvDesc,
        GetDepthStencilView());
    LogInfo("Depth Stencil View created.");
    LogInfo("=== Depth Stencil Buffer & View Creation Complete ===\n");
}

void Context::TransitionDepthStencilState() {
    // === Resource State Transition (리소스 상태 전환) ===

    // COMMON → DEPTH_WRITE 상태로 전환
    // - D3D12에서는 리소스 사용 전에 명시적으로 상태 전환 필요
    // - Vulkan의 Image Layout Transition과 동일한 개념
    LogInfo("Transitioning Depth Stencil Buffer state (COMMON → DEPTH_WRITE)...");
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

void Context::FlushCommandQueue() {
    // 새로운 Fence 지점 설정
    ++currentFence_;

    // 새 Fence 값으로 Command Queue에 Signal 전송
    ThrowIfFailed(commandQueue_->Signal(fence_.Get(), currentFence_));

    // GPU가 해당 Fence 값에 도달할 때까지 대기
    if (fence_->GetCompletedValue() < currentFence_) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        // Fence 값이 도달할 때 이벤트 신호 발생
        ThrowIfFailed(fence_->SetEventOnCompletion(currentFence_, eventHandle));

        // 이벤트 대기
        ::WaitForSingleObject(eventHandle, INFINITE);
        ::CloseHandle(eventHandle);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::GetCurrentBackBufferView() const {
    // Descriptor Heap에서 현재 Back Buffer의 RTV 핸들 계산
    
    // 1. Heap의 시작 주소 가져오기
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    
    // 2. 현재 Back Buffer의 위치로 오프셋 이동
    // - Heap은 배열 구조: [ RTV_0 | RTV_1 ]
    // - ptr = 시작 주소 + (인덱스 × Descriptor 크기)
    handle.ptr += curBackBufferIdx_ * rtvDescriptorSize_;
    
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::GetDepthStencilView() const {
    // DSV는 1개뿐이므로 Heap의 시작 위치 반환
    return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* Context::GetCurrentBackBuffer() const {
    // 현재 Back Buffer 리소스 반환 구현
    return backBuffers_[curBackBufferIdx_].Get();
}

void Context::SetViewportConfig() {
    LogInfo("Setting Viewport and Scissor Rect ({}x{})...", screenWidth_, screenHeight_);

    // Viewport 설정 (렌더링 영역)
    // - NDC (Normalized Device Coordinates) → 화면 픽셀로 변환
    screenViewport_.TopLeftX = 0.0f;
    screenViewport_.TopLeftY = 0.0f;
    screenViewport_.Width = static_cast<float>(screenWidth_);
    screenViewport_.Height = static_cast<float>(screenHeight_);
    screenViewport_.MinDepth = 0.0f; // Near plane (가까운 면)
    screenViewport_.MaxDepth = 1.0f; // Far plane (먼 면)

    // Scissor Rect 설정 (잘라낼 영역)
    // - Viewport 밖의 픽셀은 폐기
    scissorRect_ = {0, 0, static_cast<LONG>(screenWidth_), static_cast<LONG>(screenHeight_)};

    LogInfo("Viewport and Scissor Rect configured successfully.");
}

void Context::SetViewport() {
    // Command List에 Viewport와 Scissor Rect 설정
    commandList_->RSSetViewports(1, &screenViewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
}

void Context::OnResize() {
    if (!device_) {
        ExitWithMessage("Device is not initialized!");
        return;
    }
    if (!swapChain_) {
        ExitWithMessage("Swap Chain is not initialized!");
        return;
    }
    if (!commandAllocator_) {
        ExitWithMessage("Command Allocator is not initialized!");
        return;
    }

    // Resource에 변화를 주기 전에 GPU가 모든 작업을 완료하도록 대기
    FlushCommandQueue();

    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));
    LogInfo("Command List reset for resizing. Ready to record commands.");

    // 기존 Buffer 해제
    for (int i = 0; i < bufferCount_; ++i)
        backBuffers_[i].Reset();
    depthStencilBuffer_.Reset();

    // Swap Chain 크기 조정
    ThrowIfFailed(swapChain_->ResizeBuffers(
        bufferCount_,
        screenWidth_,
        screenHeight_,
        backBufferFormat_,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    curBackBufferIdx_ = 0;
    LogInfo("Swap Chain buffers resized to {}x{}.", screenWidth_, screenHeight_);

    // Render Target View 재생성
    createRenderTargetViews();

    // Depth Stencil View 재생성
    createDepthStencilView();
    // Depth Stencil 버퍼 상태 전환
    TransitionDepthStencilState();

    ThrowIfFailed(commandList_->Close());
    ID3D12CommandList* cmdsLists[] = {commandList_.Get()};
    commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    LogInfo("Command Record is closed and executed for resize operations.");

    // Viewport 및 Scissor Rect 재설정
    SetViewportConfig();
    LogInfo("Viewport and Scissor Rect updated for new window size.");

    FlushCommandQueue();
    LogInfo("Resize handling complete.");
    SetViewport();
    LogInfo("Viewport set after resize.");
}

void Context::Update(const Timer& timer) {
}

void Context::Draw() {
    // 1. Command List 및 Allocator 리셋
    ThrowIfFailed(commandAllocator_->Reset());
    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

    // 2. Viewport 및 Scissor Rect 설정
    SetViewport();

    // 3. Back Buffer를 PRESENT → RENDER_TARGET 상태로 전환
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = GetCurrentBackBuffer();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList_->ResourceBarrier(1, &barrier);

    // 4. Render Target 및 Depth Stencil 설정
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDepthStencilView();
    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 5. 화면 클리어 (파란색 배경)
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // RGBA: 진한 파란색
    commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, 
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
        1.0f, 0, 0, nullptr);

    // 6. Back Buffer를 RENDER_TARGET → PRESENT 상태로 전환
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);

    // 7. Command List 닫기
    ThrowIfFailed(commandList_->Close());

    // 8. Command Queue에 제출
    ID3D12CommandList* cmdsLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 9. 화면에 표시 (Swap Chain Present)
    ThrowIfFailed(swapChain_->Present(0, 0)); // VSync OFF (0), Flags: 0
    curBackBufferIdx_ = swapChain_->GetCurrentBackBufferIndex();

    // 10. GPU 작업 완료 대기
    FlushCommandQueue();
}

int Context::Run() {
    MSG msg = {0};

    timer.Reset();

    LogInfo("=== Entering Main Message Loop ===");
    while (msg.message != WM_QUIT) {
        // 메시지 처리
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            timer.Tick();

            // 애플리케이션이 활성 상태일 때만 업데이트 및 렌더링
            if (!appPaused_) {
                Update(timer);
                Draw();
            } else {
                Sleep(100); // 비활성 상태에서는 CPU 사용량 감소를 위해 잠시 대기
                // TODO: GUI 추가되면 GUI 사용
            }
        }
    }

    return (int)msg.wParam;
}

void Context::InitWindow() {
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ::MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = appInst_;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc)) {
        ExitWithMessage("RegisterClass Failed.");
        return;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = {0, 0, static_cast<LONG>(screenWidth_), static_cast<LONG>(screenHeight_)};
    ::AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mainWnd_ = ::CreateWindow(L"MainWnd", mainWndCaption_.c_str(), WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, appInst_, 0);
    if (!mainWnd_) {
        ExitWithMessage("CreateWindow Failed.");
        return;
    }

    ::ShowWindow(mainWnd_, SW_SHOW);
    ::UpdateWindow(mainWnd_);
}

LRESULT Context::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            appPaused_ = true;
            //mTimer.Stop();
        } else {
            appPaused_ = false;
            //mTimer.Start();
        }
        return 0;
    case WM_SIZE:
        screenWidth_ = LOWORD(lParam);
        screenHeight_ = HIWORD(lParam);
        if (device_) {
            if (wParam == SIZE_MINIMIZED) {
                appPaused_ = true;
                minimized_ = true;
                maximized_ = false;
            } else if (wParam == SIZE_MAXIMIZED) {
                appPaused_ = false;
                minimized_ = false;
                maximized_ = true;
                OnResize();
            } else if (wParam == SIZE_RESTORED) {

                // Restoring from minimized state?
                if (minimized_) {
                    appPaused_ = false;
                    minimized_ = false;
                    OnResize();
                }

                // Restoring from maximized state?
                else if (maximized_) {
                    appPaused_ = false;
                    maximized_ = false;
                    OnResize();
                } else if (resizing_) {
                    // Resizing by the user. Wait until the user is done resizing.
                    // WM_ENTERSIZEMOVE와 WM_EXITSIZEMOVE에서 처리.
                } else 
                {
                    // Window being resized, Start Graphics Resize
                    OnResize();
                }
            }
        }
        return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        appPaused_ = true;
        resizing_ = true;
        //mTimer.Stop();
        return 0;

    // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        appPaused_ = false;
        resizing_ = false;
        //mTimer.Start();
        OnResize();
        return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYUP:
        if (wParam == VK_ESCAPE) 
            PostQuitMessage(0);

        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

float Context::AspectRatio() const {
    return static_cast<float>(screenWidth_) / screenHeight_;
}

HWND Context::MainWnd() const {
    return mainWnd_;
}

} // namespace JEngine