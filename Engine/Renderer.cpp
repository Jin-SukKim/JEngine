#include "pch.h"
#include "Renderer.h"
#include "Context.h"
#include "Image2D.h"
#include "Window.h"
#include "DescriptorHeap.h"

namespace JEngine {
Renderer::Renderer(Context& ctx) : context_(ctx) {
}

void Renderer::Initialize() {
    // Depth Stencil 이미지 객체 생성
    depthStencil_ = std::make_unique<Image2D>(context_);
    // Depth Stencil 버퍼 생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight(),
                                      context_.GetDescriptorHeaps().AllocateDSV());
}

void Renderer::Update(const Timer& timer) {
}

void Renderer::Draw(Image2D& backBuffer) {
    auto commandList = context_.GetCommandList().Get();

    // 1. Command List 및 Allocator 리셋
    ThrowIfFailed(context_.GetCommandAllocator()->Reset());
    ThrowIfFailed(commandList->Reset(context_.GetCommandAllocator().Get(), nullptr));

    // 2. Viewport 및 Scissor Rect 설정
    context_.SetViewport();

    // 3. Back Buffer를 PRESENT → RENDER_TARGET 상태로 전환
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer.GetBufferPtr();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // 4. Render Target 및 Depth Stencil 설정
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBuffer.GetView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetView();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 5. 화면 클리어 (파란색 배경)
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f}; // RGBA: 진한 파란색
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       1.0f, 0, 0, nullptr);

    // 6. Back Buffer를 RENDER_TARGET → PRESENT 상태로 전환
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    // 7. Command List 닫기
    ThrowIfFailed(commandList->Close());
}

void Renderer::Resize() {
    depthStencil_->Reset();
    context_.GetDescriptorHeaps().ResetDSVCount();
    // Depth Stencil 버퍼 재생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight(),
                                      context_.GetDescriptorHeaps().AllocateDSV());

    // Depth Stencil 버퍼 상태 전환
    depthStencil_->TransitionTo();
}
} // namespace JEngine
