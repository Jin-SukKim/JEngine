#include "pch.h"
#include "Image2D.h"
#include "Context.h"

namespace JEngine {
Image2D::Image2D(Context& ctx) : context_(ctx) {
}

void Image2D::CreateBackBufferRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    format_ = format;
    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, viewHandle);
    viewHandle_ = viewHandle;
    LogInfo("Created Back Buffer with format {}", static_cast<int>(format));
}

void Image2D::CreateDepthStencil(UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    // Resource Description 설정
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D 텍스처
    depthStencilDesc.Alignment = 0;                                  // 기본 정렬
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1; // 단일 텍스처
    depthStencilDesc.MipLevels = 1;        // Mipmap 없음
    // TYPELESS 포맷 사용 = 나중에 DSV, SRV 등으로 다양하게 해석 가능
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // Depth Stencil 사용 플래그

    // Optimized Clear Value 설정 (성능 최적화)
    // - GPU가 Clear 작업을 빠르게 수행하도록 힌트 제공
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth(24비트) + Stencil(8비트)
    optClear.DepthStencil.Depth = 1.0f;              // 최대 깊이 (먼 거리)
    optClear.DepthStencil.Stencil = 0;

    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // GPU 전용 메모리 (가장 빠름)
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1; // Single GPU
    heapProps.VisibleNodeMask = 1;

    // Committed Resource 생성 (Resource + Heap 동시 생성)
    ThrowIfFailed(
        context_.GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
                                         D3D12_RESOURCE_STATE_COMMON, // 초기 상태 (아직 사용 전)
                                         &optClear, IID_PPV_ARGS(resource_.GetAddressOf())));

    LogInfo("Depth Stencil Buffer created.");

    // === Depth Stencil View 생성 ===

    // DSV Description 설정
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D 텍스처
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;        // D24_UNORM_S8_UINT
    dsvDesc.Texture2D.MipSlice = 0;

    // Heap의 시작 위치에 DSV 생성
    context_.GetDevice()->CreateDepthStencilView(resource_.Get(), &dsvDesc, viewHandle);
    viewHandle_ = viewHandle;
    LogInfo("Depth Stencil View created.");
}

void Image2D::Reset() {
    resource_.Reset();
    format_ = DXGI_FORMAT_UNKNOWN;
    viewHandle_.ptr = 0;
    LogInfo("Image2D resource have been reset.");

} // namespace JEngine

void Image2D::TransitionTo() {
    // === Resource State Transition (리소스 상태 전환) ===

    // COMMON → DEPTH_WRITE 상태로 전환
    // - D3D12에서는 리소스 사용 전에 명시적으로 상태 전환 필요
    // - Vulkan의 Image Layout Transition과 동일한 개념
    LogInfo("Transitioning Depth Stencil Buffer state (COMMON → DEPTH_WRITE)...");
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    context_.GetCommandList()->ResourceBarrier(1, &barrier);
    LogInfo("Depth Stencil Buffer transitioned to DEPTH_WRITE state.");
}
} // namespace JEngine