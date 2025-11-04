#pragma once
#include "Context.h"

namespace JEngine {
class Fence
{
  public:
    Fence(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue>& commandQueue);
    ~Fence();

    // Disable copy semantics
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept = delete; // Referece member 변수가 있기 때문에 비활성화

    void Signal();
    void WaitForGPU();

  private:
    ComPtr<ID3D12CommandQueue>& commandQueue_;
    ComPtr<ID3D12Fence> fence_ = nullptr; // CPU-GPU 동기화용 Fence
    UINT64 fenceValue_ = 0;               // 현재 Fence 값 추적
    HANDLE fenceEvent_ = nullptr;         // Fence 이벤트 핸들
};
} // namespace JEngine