#pragma once
#include "pch.h"
#include "Texture.h"

namespace JEngine {
class SwapChain
{
  public:
    SwapChain(Context& context);
    ~SwapChain();
    void Initialize();

    Texture& GetCurrentBackBuffer();
    int GetCurrentBackBufferIndex() const;
    void BufferReset();
    void Resize();
    void Present();

    uint32_t GetBufferCount() const;
    DXGI_FORMAT GetBackBufferFormat() const;
  private:
    void createSwapChain();
    void createRTV();

  private:
    Context& context_;
    ComPtr<IDXGISwapChain4> swapChain_ = nullptr;

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::vector<Texture> backBuffers_;  // ⭐ Resource → Texture
};
} // namespace JEngine