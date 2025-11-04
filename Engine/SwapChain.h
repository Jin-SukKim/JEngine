#pragma once
#include "pch.h"
#include "Image2D.h"

namespace JEngine {
class SwapChain
{
  public:
    SwapChain(Context& context);
    ~SwapChain();
    void Initialize();

    Image2D& GetCurrentBackBuffer();
    int GetCurrentBackBufferIndex() const;
    void BufferReset();
    void Resize();
    void Present();

    uint32_t GetBufferCount() const;
  private:
    void create();
    void createRTV();

  private:
    Context& context_;
    ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
    const UINT bufferCount_ = 2; // Swap Chain Buffer 개수 (Double Buffering)

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::vector<Image2D> backBuffers_; // Swap Chain의 Back Buffer들
};
} // namespace JEngine