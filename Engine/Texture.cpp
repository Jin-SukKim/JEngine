#include "pch.h"
#include "Texture.h"
#include "Context.h"
#include "UploadBuffer.h"

#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>

namespace JEngine {
Texture::Texture(Context& ctx)
    : Resource(ctx), width_(0), height_(0), type_(TextureType::TEXTURE2D) {
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

void Texture::CreateRenderTarget(DXGI_FORMAT format, UINT width, UINT height) {
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = format;
    clearValue.Color[0] = 0.0f; // R
    clearValue.Color[1] = 0.0f; // G
    clearValue.Color[2] = 0.0f; // B
    clearValue.Color[3] = 1.0f; // A

    CreateTexture2D(TextureType::RENDER_TARGET, width, height, D3D12_HEAP_TYPE_DEFAULT, format,
                    1, 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &clearValue);

    // Render Target View 생성
    DescriptorHandle handle = context_.GetDescriptorPool()->AllocateRTV();
    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, handle.cpuHandle);
    SetDescriptorHandle(static_cast<int>(TextureUsage::RTV), handle);
    LogInfo("Created RenderTarget with format {}", static_cast<int>(format));
}

void Texture::WrapBackBuffer(DXGI_FORMAT format) {
    type_ = TextureType::RENDER_TARGET;

    DescriptorHandle handle = context_.GetDescriptorPool()->AllocateRTV();
    context_.GetDevice()->CreateRenderTargetView(resource_.Get(), nullptr, handle.cpuHandle);
    SetDescriptorHandle(static_cast<int>(TextureUsage::RTV), handle);

    barrierHelper_.SetInitialState(D3D12_RESOURCE_STATE_PRESENT);
    LogInfo("Wrapped BackBuffer with format {}", static_cast<int>(format));
}

void Texture::CreateDepthStencil(UINT width, UINT height) {
    // Optimized Clear Value 설정 (성능 최적화)
    // - GPU가 Clear 작업을 빠르게 수행하도록 힌트 제공
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth(24비트) + Stencil(8비트)
    optClear.DepthStencil.Depth = 1.0f;              // 최대 깊이 (먼 거리)
    optClear.DepthStencil.Stencil = 0;

    CreateTexture2D(TextureType::DEPTH_STENCIL, width, height, D3D12_HEAP_TYPE_DEFAULT,
                    DXGI_FORMAT_R24G8_TYPELESS, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear);
    LogInfo("Depth Stencil Buffer created.");

    // DSV Description 설정
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D 텍스처
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;        // D24_UNORM_S8_UINT
    dsvDesc.Texture2D.MipSlice = 0;


    // Heap의 시작 위치에 DSV 생성
    DescriptorHandle handle = context_.GetDescriptorPool()->AllocateDSV();
    context_.GetDevice()->CreateDepthStencilView(resource_.Get(), &dsvDesc, handle.cpuHandle);
    SetDescriptorHandle(static_cast<int>(TextureUsage::DSV), handle);
    LogInfo("Depth Stencil View created.");
}

void Texture::CreateDDSFromFile(const std::wstring& filename) {
    DirectX::ResourceUploadBatch uploadBatch(context_.GetDevice());
    uploadBatch.Begin();

    ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(
        context_.GetDevice(), uploadBatch, filename.c_str(), 0, D3D12_RESOURCE_FLAG_NONE,
        DirectX::DDS_LOADER_DEFAULT, resource_.GetAddressOf()));

    uploadBatch.End(context_.GetCommandQueue());
    
    // 리소스 정보 설정
    auto desc = resource_->GetDesc();
    type_ = TextureType::TEXTURE2D;
    width_ = static_cast<UINT>(desc.Width);
    height_ = desc.Height;

    CreateSRV();
}

void Texture::CreateTextureFromFile(const std::wstring& filename,
                                    ID3D12GraphicsCommandList* cmdList) {
    // TODO: WIC를 사용하여 이미지 로드
}

void Texture::CreateSRV() {
    // TODO: Format과 MipLevels을 직접 설정해 사용하는 방식으로 수정 
    // (Resource나 Texture에 Config 구조체를 만들어서 사용하는 방식으로 Resource와 Texture 클래스 수정)

    // Resource config structure를 만들어서 사용하는 방식으로 Resource와 Texture 클래스 수정
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = GetDesc().Format; // Resource의 format
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    // Shader에서 Texture의 값을 가져올때 RGBA 순서를 지정
    srvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA로 접근하도록 설정
    srvDesc.Texture2D.MostDetailedMip = 0;        //  가장 상세한 Mipmap 레벨
    srvDesc.Texture2D.MipLevels = GetDesc().MipLevels; // 모든 Mipmap 레벨 사용
    // 특정 자원 형식에서는 이미지가 여러 개의 평면으로 구성될 수 있음
    srvDesc.Texture2D.PlaneSlice = 0; // 기본은 0
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 접근 가능한 최소 Mipmap Level

    DescriptorHandle handle = context_.GetDescriptorPool()->AllocateSRV();
    context_.GetDevice()->CreateShaderResourceView(resource_.Get(), &srvDesc, handle.cpuHandle);
    SetDescriptorHandle(static_cast<int>(TextureUsage::CBV_SRV_UAV), handle);
}

void Texture::Reset() {
    Resource::Reset();
    width_ = 0;
    height_ = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRTVHandle() const {
    if (type_ != TextureType::RENDER_TARGET)
        ExitWithMessage("Texture is not a Render Target!");
    return Resource::GetCPUHandle(static_cast<int>(TextureUsage::RTV));
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDSVHandle() const {
    if (type_ != TextureType::DEPTH_STENCIL)
        ExitWithMessage("Texture is not a Depth Stencil!");
    return Resource::GetCPUHandle(static_cast<int>(TextureUsage::DSV));
}

// TODO: error handling
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetSRVHandle() const {
    return Resource::GetCPUHandle(static_cast<int>(TextureUsage::CBV_SRV_UAV));
}
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetSRVGPUHandle() const {
    return Resource::GetGPUHandle(static_cast<int>(TextureUsage::CBV_SRV_UAV));
}
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetRTVGPUHandle() const {
    return Resource::GetGPUHandle(static_cast<int>(TextureUsage::RTV));
}

void Texture::CreateTexture2D(TextureType type, UINT width, UINT height, D3D12_HEAP_TYPE heapType,
                              DXGI_FORMAT format, UINT16 depthOrArraySize, UINT16 mipLevels,
                              D3D12_RESOURCE_FLAGS flags,
                              D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE* clearValue) {
    type_ = type;
    width_ = width;
    height_ = height;

    D3D12_HEAP_PROPERTIES heapProps = CreateHeapProperties(heapType);

    // Texture는 기본적으로 2D로 생성
    D3D12_RESOURCE_DESC desc =
        CreateResourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, depthOrArraySize /*depth*/,
                           mipLevels /*mipLevels*/, // 현재 Mipmal을 사용안하지만 추후 변경
                           format, D3D12_TEXTURE_LAYOUT_UNKNOWN, flags);

    CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, initialState, clearValue);
}

} // namespace JEngine