#include "pch.h"
#include "SwapChain.h"
#include "Context.h"
#include "Window.h"

namespace JEngine {

SwapChain::SwapChain(Context& context) : context_(context) {
}

SwapChain::~SwapChain() {
    BufferReset();
    if (swapChain_) {
        swapChain_.Reset();
    }
}

void SwapChain::Initialize() {
    // Back Buffer 이미지 객체들 초기화
    backBuffers_.reserve(MAX_FRAME_COUNT);
    for (UINT i = 0; i < MAX_FRAME_COUNT; ++i) 
        backBuffers_.emplace_back(context_);
    
    createSwapChain();
    createRTV();
}

Texture& SwapChain::GetCurrentBackBuffer() {
    return backBuffers_[GetCurrentBackBufferIndex()];
}

int SwapChain::GetCurrentBackBufferIndex() const {
    return swapChain_->GetCurrentBackBufferIndex();
}

void SwapChain::createSwapChain() {
    LogInfo(" Creating Swap Chain ");

    swapChain_.Reset();

    LogInfo("Configuring Swap Chain ({}x{}, Buffers: {})...", context_.GetWindow().GetWidth(),
            context_.GetWindow().GetHeight(), MAX_FRAME_COUNT);

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = context_.GetWindow().GetWidth();
    sd.Height = context_.GetWindow().GetHeight();
    sd.Format = backBufferFormat_;
    sd.Stereo = FALSE;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = MAX_FRAME_COUNT; // Double Buffering (2개)
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Flip Model (최고 성능)
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;    // 투명 윈도우 아님
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Fullscreen/Windowed 설정
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.RefreshRate.Numerator = 0; // 0으로 설정 시 기본값/최대값 사용
    fsDesc.RefreshRate.Denominator = 0; // Numerator = 60, Denominator = 1 → 60Hz
    fsDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    fsDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fsDesc.Windowed = TRUE;

    // CreateSwapChainForHwnd 사용 (DXGI 1.2+ 표준)
    // - Legacy CreateSwapChain()보다 Flip Model에 최적화
    ComPtr<IDXGISwapChain1> tempSwapChain;
    ThrowIfFailed(context_.GetDXGIFactory()->CreateSwapChainForHwnd(
        context_.GetCommandQueue(), context_.GetWindow().GetHwnd(), &sd, &fsDesc,
        nullptr, // Output 제한 없음 (모든 모니터 허용)
        tempSwapChain.GetAddressOf()));

    // IDXGISwapChain1 → IDXGISwapChain4로 업그레이드
    // - GetCurrentBackBufferIndex() 등 D3D12 필수 메서드 사용 가능
    ThrowIfFailed(tempSwapChain.As(&swapChain_));
    LogInfo("Swap Chain created successfully.");

    LogInfo(" Swap Chain Creation Complete \n");
}

void SwapChain::createRTV() {
    LogInfo(" Creating Render Target Views ");

    // 각 Back Buffer에 대한 RTV 생성
    for (UINT i = 0; i < MAX_FRAME_COUNT; ++i) {
        ComPtr<ID3D12Resource> buffer;  // 임시 변수 생성
        // Swap Chain으로부터 Back Buffer 리소스 가져오기
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&buffer)));
        
        //  SetResource → WrapBackBuffer 사용
        backBuffers_[i].SetResource(buffer);
        backBuffers_[i].WrapBackBuffer(backBufferFormat_);
    }
    LogInfo("Render Target Views created for all back buffers.");

    LogInfo(" Render Target Views Creation Complete \n");
}

void SwapChain::BufferReset() {
    for (UINT i = 0; i < MAX_FRAME_COUNT; ++i)
        backBuffers_[i].Reset();
}

void SwapChain::Resize() {
    LogInfo(" Resizing Swap Chain Buffers ");
    // 기존 Resource 해제
    BufferReset();

    // Swap Chain 크기 조정
    ThrowIfFailed(swapChain_->ResizeBuffers(MAX_FRAME_COUNT, context_.GetWindow().GetWidth(),
                                            context_.GetWindow().GetHeight(), backBufferFormat_,
                                            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    context_.GetDescriptorPool()->ResetRTV();

    // Render Target View 재생성
    createRTV();
}

void SwapChain::Present() {
    ThrowIfFailed(swapChain_->Present(0, 0));
}

uint32_t SwapChain::GetBufferCount() const {
    return MAX_FRAME_COUNT;
}

DXGI_FORMAT SwapChain::GetBackBufferFormat() const {
    return backBufferFormat_;
}

} // namespace JEngine