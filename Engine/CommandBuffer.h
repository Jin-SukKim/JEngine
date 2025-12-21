#pragma once

namespace JEngine {

// TODO: Factory Pattern으로 CommandList의 종류가 달라지면 편리하게 확장 가능하도록 변경
class CommandBuffer
{
  public:
    CommandBuffer(ID3D12Device* device);
    ~CommandBuffer();

    // 복사 방지
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    // 이동 생성자 및 대입 연산자 추가
    CommandBuffer(CommandBuffer&& other) noexcept 
        : commandAllocator_(std::move(other.commandAllocator_)),
          commandList_(std::move(other.commandList_)) {}

    CommandBuffer& operator=(CommandBuffer&& other) noexcept {
        if (this != &other) {
            commandAllocator_ = std::move(other.commandAllocator_);
            commandList_ = std::move(other.commandList_);
        }
        return *this;
    }

    ID3D12GraphicsCommandList* BeginRecording(ID3D12PipelineState* pso = nullptr);
    void EndRecording();
  private:
    ComPtr<ID3D12CommandAllocator> commandAllocator_; // Command Buffer의 메모리
    ComPtr<ID3D12GraphicsCommandList> commandList_;   // 렌더링 명령 기록용 List
};

} // namespace JEngine