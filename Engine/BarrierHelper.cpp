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
    if (!resource || !cmdList) {
        return;
    }

    if (state_ != newState) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = state_;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        state_ = newState;
    }
}

D3D12_RESOURCE_STATES BarrierHelper::GetState() const {
    return state_;
}

void BarrierHelper::SetInitialState(D3D12_RESOURCE_STATES newState) {
    state_ = newState;
}

} // namespace JEngine