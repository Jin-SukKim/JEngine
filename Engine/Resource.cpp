#include "pch.h"
#include "Resource.h"
#include "Context.h"

namespace JEngine {
Resource::Resource(Context& ctx) : context_(ctx) {
}

ComPtr<ID3D12Resource>& Resource::GetBuffer() {
    return resource_;
}

ID3D12Resource* Resource::GetResourcePtr() {
    return resource_.Get();
}

void Resource::SetResource(ComPtr<ID3D12Resource>& res) {
    resource_ = res;
}

DXGI_FORMAT Resource::GetFormat() const {
    return format_;
}

D3D12_CPU_DESCRIPTOR_HANDLE Resource::GetView() {
    return viewHandle_;
}

void Resource::CreateBackBufferRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    CreateRTV(format, viewHandle);
    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_PRESENT);
}

void Resource::CreateRTV(DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    format_ = format;
    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, viewHandle);
    viewHandle_ = viewHandle;

    LogInfo("Created RTV with format {}", static_cast<int>(format));
}

void Resource::CreateDepthStencil(UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    // Resource Description 설정
    D3D12_RESOURCE_DESC depthStencilDesc = SetResourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, DXGI_FORMAT_R24G8_TYPELESS,
        D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    // Optimized Clear Value 설정 (성능 최적화)
    // - GPU가 Clear 작업을 빠르게 수행하도록 힌트 제공
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth(24비트) + Stencil(8비트)
    optClear.DepthStencil.Depth = 1.0f;              // 최대 깊이 (먼 거리)
    optClear.DepthStencil.Stencil = 0;

    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = SetHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    
    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Committed Resource 생성 (Resource + Heap 동시 생성)
    ThrowIfFailed(context_.GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
        barrierHelper_.GetState(), // 초기 상태 (아직 사용 전)
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

void Resource::CreateBuffer(UINT sizeInBytes) {
    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = SetHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC vertexBufferDesc =
        SetResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, sizeInBytes, 1, DXGI_FORMAT_UNKNOWN,
                        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);
    // Committed Resource 생성 (Resource + Heap 동시 생성)
    ThrowIfFailed(context_.GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
        barrierHelper_.GetState(), // 초기 상태 (아직 사용 전)
        nullptr, IID_PPV_ARGS(resource_.GetAddressOf())));

    LogInfo("Buffer created.");
}

void Resource::Reset() {
    resource_.Reset();
    format_ = DXGI_FORMAT_UNKNOWN;
    viewHandle_.ptr = 0;
    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_COMMON);
    LogInfo("Resource resource have been reset.");
}

void Resource::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
    // === Resource State Transition (리소스 상태 전환) ===
    barrierHelper_.Transition(cmdList, resource_.Get(), newState);
}

D3D12_HEAP_PROPERTIES Resource::SetHeapProperties(D3D12_HEAP_TYPE type) {
    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = type;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1; // Single GPU
    heapProps.VisibleNodeMask = 1;
    return heapProps;
}

D3D12_RESOURCE_DESC Resource::SetResourceDesc(D3D12_RESOURCE_DIMENSION dimension, UINT width,
                                              UINT height, DXGI_FORMAT format,
                                              D3D12_TEXTURE_LAYOUT layout,
                                              D3D12_RESOURCE_FLAGS flag) {
    format_ = format;

    // Resource Description 설정
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = dimension;
    desc.Alignment = 0; // 기본 정렬
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1; // 단일 텍스처
    desc.MipLevels = 1;        // Mipmap 없음
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = layout;
    desc.Flags = flag;
    return desc;
}

} // namespace JEngine