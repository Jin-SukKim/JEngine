#pragma once
#include "DescriptorHeap.h"

namespace JEngine {
class DescriptorPool
{
  public:
    DescriptorPool(ComPtr<ID3D12Device>& device);
    void Initialize();

    auto AllocateRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto AllocateDSV() -> D3D12_CPU_DESCRIPTOR_HANDLE;
    void ResetRTV();
    void ResetDSV();

  private:
    ComPtr<ID3D12Device>& device_;
    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<DescriptorHeap>> heapAllocator_;
};
} // namespace JEngine