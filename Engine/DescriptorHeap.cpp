#include "pch.h"
#include "DescriptorHeap.h"

namespace JEngine {
DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                               UINT maxDescriptorNum)
    : device_(device) {

    // Descriptor 크기 조회 (GPU마다 다를 수 있음)
    // - Descriptor Heap에서 다음 Descriptor로 이동할 때 필요한 오프셋 크기
    descriptorSize_ = device_->GetDescriptorHandleIncrementSize(type);

    // Descriptor Heap 생성
    createHeap(type, maxDescriptorNum);
}
auto DescriptorHeap::AllocateView() -> D3D12_CPU_DESCRIPTOR_HANDLE {
    if (viewIdx_ >= maxHeapSize_) {
        LogError("Descriptor Heap allocation failed: Exceeded maximum descriptors ({})",
                 maxHeapSize_);
    }
    // Heap의 시작 주소 가져오기
    auto handle = heap_->GetCPUDescriptorHandleForHeapStart();
    // - ptr = 시작 주소 + (인덱스 × Descriptor 크기)
    handle.ptr += descriptorSize_ * viewIdx_++;
    LogInfo("Get new Descriptor Handle - Current Index: {}, Max Size: {}", viewIdx_, maxHeapSize_);
    return handle;
}

void DescriptorHeap::Reset() {
    viewIdx_ = 0;
    LogInfo("Descriptor Heap reset - Current Index set to 0");
}

void DescriptorHeap::createHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxDescriptorNum) {
    maxHeapSize_ = maxDescriptorNum;


    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = maxDescriptorNum;
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU 전용 Heap
    desc.NodeMask = 0;                            // Single GPU
    ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_)));
    LogInfo("Descriptor Heap created - Type: {}, NumDescriptors: {}, DescriptorSize: {}",
            static_cast<UINT>(type), maxDescriptorNum, descriptorSize_);

}

} // namespace JEngine