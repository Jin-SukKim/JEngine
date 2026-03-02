#include "pch.h"
#include "Context.h"
#include "Application.h"
#include "DescriptorPool.h"

namespace JEngine {

Context::Context(Window& window)
    : window_(window), screenViewport_{}, scissorRect_{} {
}

Context::~Context() {
    //if (device_) 
    //    FlushCommandQueue();
}

void Context::Initialize() {
    LogInfo(" Initializing Context ");
    createDevice();
    createCommandObjects();
    descriptorPool_ = std::make_unique<DescriptorPool>(device_.Get());
    LogInfo(" Context Initialization Complete \n");
}



void Context::createDevice() {
    UINT dxgiFactoryFlags = 0;
    
    LogInfo(" Initializing Direct3D 12 Device ");

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
        D3D_FEATURE_LEVEL_12_2,     // Direct3D 12.0 기능 레벨 요구
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
            D3D_FEATURE_LEVEL_12_2,
            IID_PPV_ARGS(&device_)));
        
        LogInfo("WARP device created successfully.");
    }
    else
    {
        LogInfo("Hardware device created successfully.");
    }

    LogInfo(" Direct3D 12 Device Initialization Complete \n");
}

void Context::createCommandObjects() {
    LogInfo(" Creating Command Objects ");
    
    // Command Queue 생성 (GPU 명령 제출용 큐)
    // - Direct Queue = Graphics + Compute 명령 실행 가능
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));
    LogInfo("Command Queue created.");
    
    LogInfo(" Command Objects Creation Complete \n");
}

void Context::ExecuteCommands(ID3D12GraphicsCommandList* cmd) {
    ID3D12CommandList* cmdsLists[] = {cmd};
    commandQueue_->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

IDXGIFactory6* Context::GetDXGIFactory() const {
    return dxgiFactory_.Get();
}

ID3D12Device* Context::GetDevice() const {
    return device_.Get();
}

ID3D12CommandQueue* Context::GetCommandQueue() const {
    return commandQueue_.Get();
}

ID3D12Device* Context::GetDevice() {
    return device_.Get();
}

ID3D12CommandQueue* Context::GetCommandQueue() {
    return commandQueue_.Get();
}

Window& Context::GetWindow() {
    return window_;
}

DescriptorPool* Context::GetDescriptorPool() {
    return descriptorPool_.get();
}

ID3D12DescriptorHeap* Context::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return GetDescriptorPool()->Get(type)->GetHeap();
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

void Context::SetViewport(ID3D12GraphicsCommandList* cmd) {
    cmd->RSSetViewports(1, &screenViewport_);
    cmd->RSSetScissorRects(1, &scissorRect_);
}

std::vector<CommandBuffer> Context::CreateGraphicsCommandBuffers(uint32_t numBuffers) {
    std::vector<CommandBuffer> buffers;
    buffers.reserve(numBuffers);

    for (uint32_t i = 0; i < numBuffers; ++i) {
        buffers.emplace_back(CommandBuffer(device_.Get()));
    }

    return buffers;
}

CommandBuffer Context::CreateGraphicsCommandBuffer() {
    return CommandBuffer(device_.Get());
}

} // namespace JEngine