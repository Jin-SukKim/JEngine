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
                                      context_.GetDescriptorPool()->AllocateDSV());
}

void Renderer::Update(const Timer& timer) {
}

void Renderer::Draw(ID3D12GraphicsCommandList* cmdList, Image2D& backBuffer) {
    // 1. Back Buffer를 PRESENT → RENDER_TARGET 상태로 전환
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 2. Render Target 및 Depth Stencil 설정
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBuffer.GetView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetView();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 3. 화면 클리어 (파란색 배경)
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f}; // RGBA: 진한 파란색
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       1.0f, 0, 0, nullptr);

    // 4. Back Buffer를 RENDER_TARGET → PRESENT 상태로 전환
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PRESENT);
}

void Renderer::Resize(ID3D12GraphicsCommandList* cmdList) {
    depthStencil_->Reset();
    context_.GetDescriptorPool()->ResetDSV();
    // Depth Stencil 버퍼 재생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight(),
                                      context_.GetDescriptorPool()->AllocateDSV());

    // Depth Stencil 버퍼 상태 전환
    depthStencil_->TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
} // namespace JEngine
