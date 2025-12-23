#pragma once
#include "DescriptorHeap.h"

namespace JEngine {
class DescriptorPool
{
  public:
    DescriptorPool(ID3D12Device* device);
    void Initialize();

    auto AllocateRTV() -> DescriptorHandle;
    auto AllocateDSV() -> DescriptorHandle;
    auto AllocateCBV() -> DescriptorHandle;
    auto AllocateSRV() -> DescriptorHandle;
    auto AllocateUAV() -> DescriptorHandle;

    auto AllocateCBVArray(size_t count) -> std::vector<DescriptorHandle>;
    
    void ResetRTV();
    void ResetDSV();
    void ResetCBV();
    void ResetSRV();
    void ResetUAV();
    auto Get(D3D12_DESCRIPTOR_HEAP_TYPE type) -> DescriptorHeap*;

  private:
    ID3D12Device* device_;
    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<DescriptorHeap>> heapAllocator_;
};
} // namespace JEngine