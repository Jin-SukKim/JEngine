#include "pch.h"
#include "DescriptorPool.h"

namespace JEngine {

DescriptorPool::DescriptorPool(ID3D12Device* device) : device_(device) {
    // TODO: 일단 고정된 개수로 초기화
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] =
        std::make_unique<DescriptorHeap>(device_, 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] =
        std::make_unique<DescriptorHeap>(device_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
        std::make_unique<DescriptorHeap>(device_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE); // Shader에서 접근 가능하도록 설정
    LogInfo("Descriptor Pool created.");
}

auto DescriptorPool::AllocateRTV() -> DescriptorHandle {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->AllocateView();
}

auto DescriptorPool::AllocateDSV() -> DescriptorHandle {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->AllocateView();
}

auto DescriptorPool::AllocateCBV() -> DescriptorHandle {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocateView();
}

auto DescriptorPool::AllocateSRV() -> DescriptorHandle {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocateView();
}

auto DescriptorPool::AllocateUAV() -> DescriptorHandle {
    return heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->AllocateView();
}

void DescriptorPool::ResetRTV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Reset();
}

void DescriptorPool::ResetDSV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->Reset();
}

void DescriptorPool::ResetCBV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Reset();
}

void DescriptorPool::ResetSRV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Reset();
}

void DescriptorPool::ResetUAV() {
    heapAllocator_[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Reset();
}

auto DescriptorPool::Get(D3D12_DESCRIPTOR_HEAP_TYPE type) -> DescriptorHeap* {
    return heapAllocator_[type].get();
}

} // namespace JEngine