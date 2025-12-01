#include "pch.h"
#include "BarrierHelper.h"

namespace JEngine {
BarrierHelper::BarrierHelper(BarrierHelper&& other) noexcept : state_(other.state_) {
    other.state_ = D3D12_RESOURCE_STATE_COMMON;
}

BarrierHelper& BarrierHelper::operator=(BarrierHelper&& other) noexcept {
    if (this != &other) {
        state_ = other.state_;
        other.state_ = D3D12_RESOURCE_STATE_COMMON;
    }
    return *this;
}

void BarrierHelper::Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource,
                               D3D12_RESOURCE_STATES newState) {
    if (state_ == newState) {
        //LogInfo("No transition needed. New state is the same as current state.");
        return;
    }

    // - D3D12에서는 리소스 사용 전에 명시적으로 상태 전환 필요
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = state_;
    barrier.Transition.StateAfter = newState;
    cmdList->ResourceBarrier(1, &barrier);

    state_ = newState;
    //LogInfo("Resource State Transition Command Recorded.");
}

D3D12_RESOURCE_STATES BarrierHelper::GetState() const {
    return state_;
}

void BarrierHelper::SetInitialState(D3D12_RESOURCE_STATES newState) {
    state_ = newState;
}

} // namespace JEngine