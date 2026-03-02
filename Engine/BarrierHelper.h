#pragma once

namespace JEngine {
class BarrierHelper
{
  public:
    BarrierHelper() = default;
    ~BarrierHelper() = default;

    // 이동 
    BarrierHelper(BarrierHelper&& other) noexcept;
    BarrierHelper& operator=(BarrierHelper&& other) noexcept;

    void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource,
                    D3D12_RESOURCE_STATES newState);

    D3D12_RESOURCE_STATES GetState() const;
    void SetInitialState(D3D12_RESOURCE_STATES newState);

  private:
    D3D12_RESOURCE_STATES state_ = D3D12_RESOURCE_STATE_COMMON;
};
} // namespace JEngine
