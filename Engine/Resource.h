#pragma once
#include "BarrierHelper.h"

namespace JEngine {

class Context;
class Resource
{
  public:
    Resource(Context& ctx);
    virtual ~Resource();

    // 리소스 복사 방지
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    Resource(Resource&& other) noexcept;
    Resource& operator=(Resource&& other) noexcept;

    virtual void Reset();
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

    // Getter
    ComPtr<ID3D12Resource>& GetResource();
    const ComPtr<ID3D12Resource>& GetResource() const;
    ID3D12Resource* GetResourcePtr() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
    D3D12_RESOURCE_STATES GetCurrentState() const;
    D3D12_RESOURCE_DESC GetDesc() const;
    DXGI_FORMAT GetFormat() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetViewHandle() const;

    // Setter
    void SetResource(ComPtr<ID3D12Resource>& res);

  protected:
    // 하위 클래스에서 사용 가능한 Setter 함수
    void SetViewHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void SetFormat(DXGI_FORMAT format);

    // 리소스 생성 헬퍼 함수들
    D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE type) const;
    D3D12_RESOURCE_DESC
    CreateResourceDesc(D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height = 1,
                       UINT16 depthOrArraySize = 1, UINT16 mipLevels = 1,
                       DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
                       D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                       D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void CreateCommittedResource(const D3D12_HEAP_PROPERTIES& heapProps, D3D12_HEAP_FLAGS heapFlags,
                                 const D3D12_RESOURCE_DESC& desc,
                                 D3D12_RESOURCE_STATES initialState,
                                 const D3D12_CLEAR_VALUE* optimizedClearValue = nullptr);

  protected:
    Context& context_;
    ComPtr<ID3D12Resource> resource_ = nullptr;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
    D3D12_CPU_DESCRIPTOR_HANDLE viewHandle_{};

    BarrierHelper barrierHelper_;
};
} // namespace JEngine