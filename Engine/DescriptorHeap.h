#pragma once

namespace JEngine {
class DescriptorHeap
{
  public:
    DescriptorHeap(ID3D12Device* device, UINT maxDescriptorNum,
                   D3D12_DESCRIPTOR_HEAP_TYPE type,
                   D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    auto AllocateView() -> D3D12_CPU_DESCRIPTOR_HANDLE;
    void Reset();
    auto GetHeap() -> ID3D12DescriptorHeap*;
  private:
    void createHeap(UINT maxDescriptorNum, D3D12_DESCRIPTOR_HEAP_TYPE type,
                    D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
  private:
    // === Descriptor Heaps (View들을 담는 배열 컨테이너) ===
    // Descriptor Heap = View(Descriptor)들을 저장하는 "배열" 또는 "상자"
    // 예: rtvHeap_ = [ RTV_0 | RTV_1 | RTV_2 | ... ]
    //                   ↑       ↑       ↑
    //                 View    View    View
    //
    // - DirectX 12: Heap을 명시적으로 생성하고 관리 (성능 최적화를 위해)
    // GPU가 빠르게 접근할 수 있도록 연속된 메모리 공간에 View들을 저장
    ID3D12Device* device_;
    ComPtr<ID3D12DescriptorHeap> heap_; // Descriptor View들을 담는 Heap

    UINT descriptorSize_ = 0; // Descriptor 1개의 크기
    UINT maxHeapSize_ = 0;    // Heap이 담을 수 있는 최대 Descriptor 수
    UINT viewIdx_ = 0;        // 현재 할당된 Descriptor 인덱스
};
} // namespace JEngine
