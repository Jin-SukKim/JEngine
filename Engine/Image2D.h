#pragma once
#include "BarrierHelper.h"
namespace JEngine {

class Context;
class Image2D
{
  public:
    Image2D(Context& ctx);

    ComPtr<ID3D12Resource>& GetBuffer() {
        return resource_;
    }
    ID3D12Resource* GetBufferPtr() {
        return resource_.Get();
    }

    DXGI_FORMAT GetFormat() const {
        return format_;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetView() {
        return viewHandle_;
    }

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