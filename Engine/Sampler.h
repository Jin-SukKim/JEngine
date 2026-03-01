#pragma once

namespace JEngine {

struct DescriptorHandle;
class Context;

struct SamplerConfig
{
    D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    D3D12_TEXTURE_ADDRESS_MODE addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE addressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    FLOAT mipLODBias = 0.0f;
    UINT maxAnisotropy = 1;
    D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_STATIC_BORDER_COLOR borderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    FLOAT minLOD = 0.0f;
    FLOAT maxLOD = D3D12_FLOAT32_MAX;
};

class Sampler
{
  public:
    Sampler(Context& context);
    ~Sampler();
    void CreateSampler(const SamplerConfig& config = SamplerConfig());

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t index) const;

    static std::array<D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
  private:
    Context& context_;
    std::vector<DescriptorHandle> samplerHandles_;
};
} // namespace JEngine
