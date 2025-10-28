#include "pch.h"
#include "Context.h"
#include "Application.h"

namespace JEngine {

Context::Context(Window& window) : window_(window), descriptorHeaps_(device_) {
    Initialize();
}

Context::~Context() {
    if (device_) 
        FlushCommandQueue();
}

void Context::Initialize() {
    LogInfo("=== Initializing Context ===");
    createDevice();
    createCommandObjects();
    descriptorHeaps_.Initialize();
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


void Context::ResetCommands() {
    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));
}

void Context::CloseCommands() {
    ThrowIfFailed(commandList_->Close());
}

void Context::ExecuteCommands() {
    ID3D12CommandList* cmdsLists[] = {commandList_.Get()};
    commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

ComPtr<IDXGIFactory6> Context::GetDXGIFactory() const {
    return dxgiFactory_;
}

ComPtr<ID3D12Device> Context::GetDevice() const {
    return device_;
}

ComPtr<ID3D12CommandQueue> Context::GetCommandQueue() const {
    return commandQueue_;
}

ComPtr<ID3D12GraphicsCommandList> Context::GetCommandList() const {
    return commandList_;
}

ComPtr<ID3D12CommandAllocator> Context::GetCommandAllocator() const {
    return commandAllocator_;
}

Window& Context::GetWindow() {
    return window_;
}

DescriptorHeap& Context::GetDescriptorHeaps() {
    return descriptorHeaps_;
}

void Context::SetViewportConfig() {
    LogInfo("Setting Viewport and Scissor Rect ({}x{})...", window_.GetWidth(), window_.GetHeight());

    // Viewport 설정 (렌더링 영역)
    // - NDC (Normalized Device Coordinates) → 화면 픽셀로 변환
    screenViewport_.TopLeftX = 0.0f;
    screenViewport_.TopLeftY = 0.0f;
    screenViewport_.Width = static_cast<float>(window_.GetWidth());
    screenViewport_.Height = static_cast<float>(window_.GetHeight());
    screenViewport_.MinDepth = 0.0f; // Near plane (가까운 면)
    screenViewport_.MaxDepth = 1.0f; // Far plane (먼 면)

    // Scissor Rect 설정 (잘라낼 영역)
    // - Viewport 밖의 픽셀은 폐기
    scissorRect_ = {0, 0, static_cast<LONG>(window_.GetWidth()), static_cast<LONG>(window_.GetHeight())};

    LogInfo("Viewport and Scissor Rect configured successfully.");
}

void Context::SetViewport() {
    commandList_->RSSetViewports(1, &screenViewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
}

} // namespace JEngine