#pragma once
#include "Resource.h"

namespace JEngine {

enum class TextureType { TEXTURE2D, RENDER_TARGET, DEPTH_STENCIL };

class Texture : public Resource
{
  public:
    Texture(Context& ctx);
    ~Texture() override;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // RenderTarget 생성
    void CreateRenderTarget(DXGI_FORMAT format, UINT width, UINT height, DescriptorHandle handle);
    // SwapChain의 BackBuffer를 Resource로 Wrapping해 GPU에서 사용
    void WrapBackBuffer(DXGI_FORMAT format, DescriptorHandle handle);

    void CreateDepthStencil(UINT width, UINT height, DescriptorHandle handle);

    void Reset() override;

  protected:
    void CreateTexture2D(TextureType type, UINT width, UINT height,
                         D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
                         DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
                         D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                         D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ,
                         D3D12_CLEAR_VALUE* clearValue = nullptr);

  private:
    UINT width_;
    UINT height_;
    TextureType type_;
};

} // namespace JEngine
