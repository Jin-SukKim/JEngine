#pragma once

namespace JEngine {
class DescriptorHeap
{
  public:
    DescriptorHeap(ComPtr<ID3D12Device>& device);

    void Initialize();

    void CreateRTVHeap(UINT numDescriptors);
    void CreateDSVHeap(UINT numDescriptors);

    auto AllocateRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto AllocateDSV() -> D3D12_CPU_DESCRIPTOR_HANDLE;
    void ResetRTVCount() {
        rtvCount_ = 0;
        LogInfo("RTV Descriptor count reset to 0.");
    }

    void ResetDSVCount() {
        dsvCount_ = 0;
        LogInfo("DSV Descriptor count reset to 0.");
    }
  private:
    ComPtr<ID3D12Device>& device_;
    // === Descriptor Heaps (View들을 담는 배열 컨테이너) ===
    // Descriptor Heap = View(Descriptor)들을 저장하는 "배열" 또는 "상자"
    // 예: rtvHeap_ = [ RTV_0 | RTV_1 | RTV_2 | ... ]
    //                   ↑       ↑       ↑
    //                 View    View    View
    //
    // - DirectX 11: View가 자동으로 관리됨 (개발자가 신경 쓸 필요 없음)
    // - DirectX 12: Heap을 명시적으로 생성하고 관리 (성능 최적화를 위해)
    //
    // GPU가 빠르게 접근할 수 있도록 연속된 메모리 공간에 View들을 저장
    // Vulkan의 Descriptor Pool과 유사하지만, D3D12는 배열처럼 직접 접근 가능
    ComPtr<ID3D12DescriptorHeap> rtvHeap_; // Render Target View들을 담는 Heap
    ComPtr<ID3D12DescriptorHeap> dsvHeap_; // Depth Stencil View들을 담는 Heap

    // === Descriptor Sizes (GPU 종속적) ===
    UINT rtvDescriptorSize_ = 0;       // Render Target View Descriptor 크기
    UINT dsvDescriptorSize_ = 0;       // Depth Stencil View Descriptor 크기
    UINT cbvSrvUavDescriptorSize_ = 0; // CBV/SRV/UAV Descriptor 크기

    // === View Counts ===
    UINT rtvCount_ = 0;
    UINT dsvCount_ = 0;
};
} // namespace JEngine
