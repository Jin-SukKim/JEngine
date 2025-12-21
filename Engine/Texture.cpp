#include "pch.h"
#include "Texture.h"
#include "Context.h"

namespace JEngine {
Texture::Texture(Context& ctx) : Resource(ctx), width_(0), height_(0), type_(TextureType::TEXTURE2D) {
}

Texture::~Texture() {
    Reset();
}

Texture::Texture(Texture&& other) noexcept
    : Resource(std::move(other)), width_(other.width_), height_(other.height_), type_(other.type_) {
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Reset();
        Resource::operator=(std::move(other));
        width_ = other.width_;
        height_ = other.height_;
        type_ = other.type_;
    }
    return *this;
}

void Texture::CreateRenderTarget(DXGI_FORMAT format, UINT width, UINT height,
                                 DescriptorHandle handle) {
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = format;
    clearValue.Color[0] = 0.0f; // R
    clearValue.Color[1] = 0.0f; // G
    clearValue.Color[2] = 0.0f; // B
    clearValue.Color[3] = 1.0f; // A

    CreateTexture2D(TextureType::RENDER_TARGET, width, height, D3D12_HEAP_TYPE_DEFAULT, format,
                    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &clearValue);

    // Render Target View 생성
    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, handle.cpuHandle);
    SetDescriptorHandle(handle);
    LogInfo("Created RenderTarget with format {}", static_cast<int>(format));
}

void Texture::WrapBackBuffer(DXGI_FORMAT format, DescriptorHandle handle) {
    SetFormat(format);
    type_ = TextureType::RENDER_TARGET;

    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, handle.cpuHandle);
    SetDescriptorHandle(handle);

    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_PRESENT);
    LogInfo("Wrapped BackBuffer with format {}", static_cast<int>(format));
}

void Texture::CreateDepthStencil(UINT width, UINT height, DescriptorHandle handle) {
    // Optimized Clear Value 설정 (성능 최적화)
    // - GPU가 Clear 작업을 빠르게 수행하도록 힌트 제공
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth(24비트) + Stencil(8비트)
    optClear.DepthStencil.Depth = 1.0f;              // 최대 깊이 (먼 거리)
    optClear.DepthStencil.Stencil = 0;

    CreateTexture2D(TextureType::DEPTH_STENCIL, width, height, D3D12_HEAP_TYPE_DEFAULT,
                    DXGI_FORMAT_R24G8_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear);
    LogInfo("Depth Stencil Buffer created.");

    // DSV Description 설정
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D 텍스처
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;        // D24_UNORM_S8_UINT
    dsvDesc.Texture2D.MipSlice = 0;

    SetFormat(DXGI_FORMAT_D24_UNORM_S8_UINT);

    // Heap의 시작 위치에 DSV 생성
    context_.GetDevice()->CreateDepthStencilView(resource_.Get(), &dsvDesc, handle.cpuHandle);
    SetDescriptorHandle(handle);
    LogInfo("Depth Stencil View created.");
}

void Texture::Reset() {
    Resource::Reset();
    width_ = 0;
    height_ = 0;
}

void Texture::CreateTexture2D(TextureType type, UINT width, UINT height, D3D12_HEAP_TYPE heapType,
                              DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
                              D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue) {
    type_ = type;
    width_ = width;
    height_ = height;

    D3D12_HEAP_PROPERTIES heapProps = CreateHeapProperties(heapType);

    // Texture는 기본적으로 2D로 생성
    D3D12_RESOURCE_DESC desc =
        CreateResourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, 1 /*depth*/,
                           1 /*mipLevels*/, // 현재 Mipmal을 사용안하지만 추후 변경
                           format, D3D12_TEXTURE_LAYOUT_UNKNOWN, flags);

    CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, initialState, clearValue);
}

} // namespace JEngine