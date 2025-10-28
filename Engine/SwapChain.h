#pragma once
#include "pch.h"
#include "Image2D.h"

namespace JEngine {
class SwapChain
{
  public:
    SwapChain(Context& context);

    void Initialize();

    Image2D& GetCurrentBackBuffer();
    void BufferReset();
    void Resize();
    void Present();
  private:
    void create();
    void createRTV();

  private:
    Context& context_;
    ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
    const UINT bufferCount_ = 2; // Swap Chain Buffer 개수 (Double Buffering)
    int curBackBufferIdx_ = 0;             // 현재 사용 중인 Back Buffer 인덱스 (0 or 1)

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::vector<Image2D> backBuffers_; // Swap Chain의 Back Buffer들
};
} // namespace JEngine