#pragma once
#include "Context.h"

namespace JEngine {
class Fence
{
  public:
    Fence(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    ~Fence();

    // Disable copy semantics
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    
    Fence(Fence&& other) noexcept; // C++ 컨테이너에서 크기를 바꿀때 copy말고 move가 사용되도록 noexcept 설정
    Fence& operator=(Fence&& other) noexcept = delete; // Referece member 변수가 있기 때문에 비활성화

    void Signal();
    void WaitForGPU();

  private:
    ID3D12CommandQueue* commandQueue_;
    ComPtr<ID3D12Fence> fence_ = nullptr; // CPU-GPU 동기화용 Fence
    UINT64 fenceValue_ = 0;               // 현재 Fence 값 추적
    HANDLE fenceEvent_ = nullptr;         // Fence 이벤트 핸들
};
} // namespace JEngine