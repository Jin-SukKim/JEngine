#include "pch.h"
#include "Fence.h"

namespace JEngine {
Fence::Fence(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
    : commandQueue_(commandQueue) {

    // Fence 생성 (CPU-GPU 동기화용)
    // - GPU 작업 완료를 CPU에서 확인하기 위한 동기화 객체
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));

    fenceEvent_ = ::CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    LogInfo("Fence created for CPU-GPU synchronization.");
}

Fence::~Fence() {
    WaitForGPU();
    if (fenceEvent_) {
        ::CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
    LogInfo("Fence destroyed.");
}

Fence::Fence(Fence&& other) noexcept
    : commandQueue_(other.commandQueue_), fence_(std::move(other.fence_)),
      fenceValue_(other.fenceValue_), fenceEvent_(other.fenceEvent_) {
    other.fenceValue_ = 0;
    other.fenceEvent_ = nullptr;
}

void Fence::Signal() {
    ThrowIfFailed(commandQueue_->Signal(fence_.Get(), ++fenceValue_));
}

void Fence::WaitForGPU() {
    if (fenceValue_ == 0)
        return; // 아직 Signal이 한번도 호출되지 않음

    // GPU가 해당 Fence 값에 도달할 때까지 대기
    if (fence_->GetCompletedValue() < fenceValue_) {
        // Fence 값이 도달할 때 이벤트 신호 발생
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValue_, fenceEvent_));
        // 이벤트 대기
        ::WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

} // namespace JEngine