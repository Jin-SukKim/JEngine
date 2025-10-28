#include "pch.h"
#include "DescriptorHeap.h"

namespace JEngine {
DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device>& device) : device_(device) {

}

void DescriptorHeap::Initialize() {

    // Descriptor 크기 조회 (GPU마다 다를 수 있음)
    // - Descriptor Heap에서 다음 Descriptor로 이동할 때 필요한 오프셋 크기
    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvSrvUavDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    LogInfo("Descriptor sizes - RTV: {}, DSV: {}, CBV/SRV/UAV: {}", rtvDescriptorSize_,
            dsvDescriptorSize_, cbvSrvUavDescriptorSize_);

    LogInfo("=== Creating Descriptor Heaps ===");
    CreateRTVHeap(2); // TODO: 일단 2개로 고정 (Double Buffering)
    CreateDSVHeap(1); // TODO: 일단 1개로 고정
    LogInfo("=== Descriptor Heaps Creation Complete ===\n");
}

void DescriptorHeap::CreateRTVHeap(UINT numDescriptors) {
    // Render Target View Descriptor Heap 생성
    // - Back Buffer 수만큼 RTV 저장 공간 확보
    // - Heap = View들을 담는 "배열" 컨테이너
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = numDescriptors;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU 전용 Heap
    rtvHeapDesc.NodeMask = 0;                            // Single GPU
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));
    LogInfo("Render Target View Descriptor Heap created with {} descriptors.", numDescriptors);
}

void DescriptorHeap::CreateDSVHeap(UINT numDescriptors) {
    // Depth Stencil View Descriptor Heap 생성
    // - DSV는 1개만 필요 (모든 Back Buffer가 공유)
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = numDescriptors;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_)));
    LogInfo("Depth Stencil View Descriptor Heap created with {} descriptors.", numDescriptors);
}

auto DescriptorHeap::AllocateRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE {
    // Heap의 시작 주소 가져오기
    auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    // - ptr = 시작 주소 + (인덱스 × Descriptor 크기)
    handle.ptr += rtvDescriptorSize_ * rtvCount_++;
    LogInfo("Get new RTV Descriptor Handle");
    return handle;
}

auto DescriptorHeap::AllocateDSV() -> D3D12_CPU_DESCRIPTOR_HANDLE {
    auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += dsvDescriptorSize_ * dsvCount_++;
    LogInfo("Get new DSV Descriptor Handle");
    return handle;
}


} // namespace JEngine