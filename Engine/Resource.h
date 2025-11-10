#pragma once
#include "BarrierHelper.h"

namespace JEngine {

class Context;
class Resource
{
  public:
    Resource(Context& ctx);

    ComPtr<ID3D12Resource>& GetBuffer();
    ID3D12Resource* GetResourcePtr();
    void SetResource(ComPtr<ID3D12Resource>& res);

    DXGI_FORMAT GetFormat() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetView();

    void CreateRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);
    void CreateBackBufferRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);

    void CreateDepthStencil(UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle);
    void CreateBuffer(UINT sizeInBytes);

    void Reset();
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

    D3D12_HEAP_PROPERTIES SetHeapProperties(D3D12_HEAP_TYPE type);
    D3D12_RESOURCE_DESC
    SetResourceDesc(D3D12_RESOURCE_DIMENSION dimension, UINT width, UINT height = 1,
                    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
                    D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                    D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView(UINT vertexByteStride, UINT vertexBufferByteSize) const {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = resource_->GetGPUVirtualAddress();
        vbv.StrideInBytes = vertexByteStride;
        vbv.SizeInBytes = vertexBufferByteSize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView(DXGI_FORMAT indexFormat, UINT indexBufferByteSize) const {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = resource_->GetGPUVirtualAddress();
        ibv.Format = indexFormat;
        ibv.SizeInBytes = indexBufferByteSize;

        return ibv;
    }

  protected:
    Context& context_;
    ComPtr<ID3D12Resource> resource_ = nullptr;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
    D3D12_CPU_DESCRIPTOR_HANDLE viewHandle_{};

    BarrierHelper barrierHelper_;
};
} // namespace JEngine