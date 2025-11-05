#pragma once
#include "BarrierHelper.h"
namespace JEngine {

class Context;
class Image2D
{
  public:
    Image2D(Context& ctx);

    ComPtr<ID3D12Resource>& GetBuffer();
    ID3D12Resource* GetResourcePtr();
    void SetResource(ComPtr<ID3D12Resource>& res);

    DXGI_FORMAT GetFormat() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetView();

    void CreateRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);
    void CreateBackBufferRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);

    void CreateDepthStencil(UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);

    void Reset();
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);
  private:
    Context& context_;
    ComPtr<ID3D12Resource> resource_ = nullptr;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
    D3D12_CPU_DESCRIPTOR_HANDLE viewHandle_{};

    BarrierHelper barrierHelper_;
};
} // namespace JEngine