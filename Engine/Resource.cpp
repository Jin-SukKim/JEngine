#include "pch.h"
#include "Resource.h"
#include "Context.h"

namespace JEngine {
// Constructor
Resource::Resource(Context& ctx) : context_(ctx) {
}

Resource::~Resource() {
    Reset();
}

// Move constructor
Resource::Resource(Resource&& other) noexcept
    : context_(other.context_), resource_(std::move(other.resource_)), format_(other.format_),
      viewHandle_(other.viewHandle_), barrierHelper_(std::move(other.barrierHelper_)) {
    // 이동 후 other는 비어있는 상태로 만듦
    other.format_ = DXGI_FORMAT_UNKNOWN;
    other.viewHandle_.ptr = 0;
}

// Move assignment operator
Resource& Resource::operator=(Resource&& other) noexcept {
    if (this != &other) {
        // 기존 리소스 정리
        Reset();

        // context_는 참조이므로 재할당 불가 (이미 초기화됨)
        // 다른 멤버들만 이동
        resource_ = std::move(other.resource_);
        format_ = other.format_;
        viewHandle_ = other.viewHandle_;
        barrierHelper_ = std::move(other.barrierHelper_);

        // 이동 후 other는 비어있는 상태로 만듦
        other.Reset();
    }
    return *this;
}

void Resource::Reset() {
    // GPU Address 로깅 (디버깅용)
    if (resource_) {
        LogInfo("Resetting Resource: GPU Address=0x{:X}", GetGPUAddress());
    }
    
    // ComPtr 해제 (자동으로 ref count 감소)
    resource_.Reset();
    
    // 멤버 변수 초기화
    format_ = DXGI_FORMAT_UNKNOWN;
    viewHandle_.ptr = 0;
    
    // BarrierHelper 상태 초기화
    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_COMMON);
    
    LogInfo("Resource has been reset successfully.");
}

void Resource::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
    // === Resource State Transition (리소스 상태 전환) ===
    barrierHelper_.Transition(cmdList, resource_.Get(), newState);
}

// Getters
ComPtr<ID3D12Resource>& Resource::GetResource() {
    return resource_;
}

const ComPtr<ID3D12Resource>& Resource::GetResource() const {
    return resource_;
}

ID3D12Resource* Resource::GetResourcePtr() const {
    return resource_.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::GetGPUAddress() const {
    return resource_ ? resource_->GetGPUVirtualAddress() : 0;
}

D3D12_RESOURCE_STATES Resource::GetCurrentState() const {
    return barrierHelper_.GetState();
}

D3D12_RESOURCE_DESC Resource::GetDesc() const {
    return resource_ ? resource_->GetDesc() : D3D12_RESOURCE_DESC();
}

DXGI_FORMAT Resource::GetFormat() const {
    return format_;
}

D3D12_CPU_DESCRIPTOR_HANDLE Resource::GetViewHandle() const {
    return viewHandle_;
}

// Setter
void Resource::SetResource(ComPtr<ID3D12Resource>& res) {
    resource_ = res;
}

void Resource::SetViewHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    viewHandle_ = handle;
}

void Resource::SetFormat(DXGI_FORMAT format) {
    format_ = format;
}

// 리소스 생성 헬퍼 함수들
D3D12_HEAP_PROPERTIES Resource::CreateHeapProperties(D3D12_HEAP_TYPE type) const {
    // Heap Properties 설정 (CD3DX12_HEAP_PROPERTIES 헬퍼 없이)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = type;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1; // Single GPU
    heapProps.VisibleNodeMask = 1;
    return heapProps;
}

D3D12_RESOURCE_DESC
Resource::CreateResourceDesc(D3D12_RESOURCE_DIMENSION dimension, UINT64 width, UINT height,
                             UINT16 depthOrArraySize, UINT16 mipLevels, DXGI_FORMAT format,
                             D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flags) {
    SetFormat(format);

    // Resource Description 설정
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = dimension;
    desc.Alignment = 0; // 기본 정렬
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depthOrArraySize; // 1은 단일 텍스처
    desc.MipLevels = mipLevels;               // Mipmap 없음
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = layout;
    desc.Flags = flags;
    return desc;
}

void Resource::CreateCommittedResource(
    const D3D12_HEAP_PROPERTIES& heapProps,
    D3D12_HEAP_FLAGS heapFlags,
    const D3D12_RESOURCE_DESC& desc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* optimizedClearValue) {
    
    barrierHelper_.SetInitialState(initialState);
    
    ThrowIfFailed(context_.GetDevice()->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &desc,
        initialState,
        optimizedClearValue,
        IID_PPV_ARGS(&resource_)));
    
    LogInfo("D3D12 resource created: GPU Address=0x{:X}", GetGPUAddress());
}

} // namespace JEngine