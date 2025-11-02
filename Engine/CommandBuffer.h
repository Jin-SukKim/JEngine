#pragma once

namespace JEngine {

// TODO: Factory Pattern으로 CommandList의 종류가 달라지면 편리하게 확장 가능하도록 변경
class CommandBuffer
{
  public:
    CommandBuffer(ComPtr<ID3D12Device>& device);

    ~CommandBuffer();

    ID3D12GraphicsCommandList* BeginRecording();
    void EndRecording();
    ID3D12GraphicsCommandList* GetCommandList();
  private:
    ComPtr<ID3D12CommandAllocator> commandAllocator_; // Command Buffer의 메모리
    ComPtr<ID3D12GraphicsCommandList> commandList_;   // 렌더링 명령 기록용 List
};

} // namespace JEngine