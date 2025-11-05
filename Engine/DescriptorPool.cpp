#include "pch.h"
#include "DescriptorPool.h"

namespace JEngine {

DescriptorPool::DescriptorPool(ComPtr<ID3D12Device>& device) : device_(device) {
    // TODO: 일단 고정된 개수로 초기화
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] =
        std::make_unique<DescriptorHeap>(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2);
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] =
        std::make_unique<DescriptorHeap>(device_, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
    LogInfo("Descriptor Pool created.");
}


auto DescriptorPool::AllocateRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->AllocateView();
}

auto DescriptorPool::AllocateDSV() -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->AllocateView();
}

void DescriptorPool::ResetRTV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Reset();
}

void DescriptorPool::ResetDSV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->Reset();
}

} // namespace JEngine