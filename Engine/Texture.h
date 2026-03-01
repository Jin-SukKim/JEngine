#pragma once
#include "Resource.h"

namespace JEngine {

enum class TextureType { TEXTURE2D, RENDER_TARGET, DEPTH_STENCIL };

enum class TextureUsage { RTV, DSV, CBV_SRV_UAV };

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
    void CreateRenderTarget(DXGI_FORMAT format, UINT width, UINT height);
    // SwapChain의 BackBuffer를 Resource로 Wrapping해 GPU에서 사용
    void WrapBackBuffer(DXGI_FORMAT format);

    void CreateDepthStencil(UINT width, UINT height);

    void CreateDDSFromFile(const std::wstring& filename);
    void CreateTextureFromFile(const std::wstring& filename, ID3D12GraphicsCommandList* cmdList);
    void CreateSRV();

    void Reset() override;

    // Resource Class의 GetCPUHandle, GetGPUHandle 함수 삭제
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t index) const = delete;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t index) const = delete;

    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUHandle() const;
  protected:
    void CreateTexture2D(TextureType type, UINT width, UINT height,
                         D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
                         DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, 
                         UINT16 depthOrArraySize = 1,
                         UINT16 mipLevels = 1,
                         D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                         D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ,
                         D3D12_CLEAR_VALUE* clearValue = nullptr);

  private:
    UINT width_;
    UINT height_;
    TextureType type_;
};

} // namespace JEngine
