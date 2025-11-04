#include "pch.h"
#include "CommandBuffer.h"

namespace JEngine {

CommandBuffer::CommandBuffer(ComPtr<ID3D12Device>& device) {
    // Command Allocator 생성 (Command List의 메모리 관리자)
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  IID_PPV_ARGS(commandAllocator_.GetAddressOf())));
    LogInfo("Command Allocator created: {:p}", (void*)commandAllocator_.Get());

    // Command List 생성 (렌더링 명령 기록용)
    ThrowIfFailed(device->CreateCommandList(0, // Single GPU
                                             D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             commandAllocator_.Get(),
                                             nullptr, // 초기 Pipeline State 없음
                                             IID_PPV_ARGS(commandList_.GetAddressOf())));
    LogInfo("Graphics Command List created: {:p}", (void*)commandList_.Get());

    // Command List를 닫은 상태로 초기화
    commandList_->Close();
    LogInfo("Command List closed and ready for recording commands.");
}

CommandBuffer::~CommandBuffer() {
    if (commandList_) {
        LogInfo("CommandBuffer destructor - releasing CommandList: {:p}",
                (void*)commandList_.Get());
        commandList_.Reset();
    }
    if (commandAllocator_) {
        LogInfo("CommandBuffer destructor - releasing CommandAllocator: {:p}",
                (void*)commandAllocator_.Get());
        commandAllocator_.Reset();
    }
}

ID3D12GraphicsCommandList* CommandBuffer::BeginRecording() {
    ThrowIfFailed(commandAllocator_->Reset());
    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));
    return commandList_.Get();
}

void CommandBuffer::EndRecording() {
    ThrowIfFailed(commandList_->Close());
}

ID3D12GraphicsCommandList* CommandBuffer::GetCommandList() {
    return commandList_.Get();
}

} // namespace JEngine