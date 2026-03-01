#include "pch.h"
#include "Sampler.h"
#include "Context.h"

namespace JEngine {
Sampler::Sampler(Context& context) : context_(context) {
}

Sampler::~Sampler() {
}

void Sampler::CreateSampler(const SamplerConfig& config) {
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Filter 설정
    // Address Mode 설정 (Wrap, Clamp, Mirror, Border, Mirror_Once)
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.0f;          // Mipmap LOD의 bias 설정 (기본값 0.0f)
    samplerDesc.MinLOD = 0.0f;              // 선택 가능한 최소 Mipmap Level
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // 선택 가능한 최대 Mipmap Level
    samplerDesc.MaxAnisotropy =
        1; // 최대 Anisotropy 설정 ([1, 16] 범위) - filter가 Anisotropic type일 때만 적용
    // Border Color 설정 (Address Mode가 Border일 때 사용)
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS; // Shadow Map 등에 사용된 특화 옵션
    DescriptorHandle samplerHandle = context_.GetDescriptorPool()->AllocateSampler();
    context_.GetDevice()->CreateSampler(&samplerDesc, samplerHandle.cpuHandle);

    samplerHandles_.emplace_back(samplerHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE Sampler::GetCPUHandle(size_t index) const {
    if (index >= samplerHandles_.size()) {
        LogError("Sampler index out of range: {}", index);
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }
    return samplerHandles_[index].cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Sampler::GetGPUHandle(size_t index) const {
    if (index >= samplerHandles_.size()) {
        LogError("Sampler index out of range: {}", index);
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }
    return samplerHandles_[index].gpuHandle;
}

std::array<D3D12_STATIC_SAMPLER_DESC, 6> Sampler::GetStaticSamplers() {
    D3D12_STATIC_SAMPLER_DESC pointWrap = {};
    pointWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrap.MipLODBias = 0.f;
    pointWrap.MinLOD = 0.0f;
    pointWrap.MaxLOD = D3D12_FLOAT32_MAX;
    pointWrap.MaxAnisotropy = 1;
    pointWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointWrap.ShaderRegister = 0; // register(s0)
    pointWrap.RegisterSpace = 0;  // space0
    pointWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    pointWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC pointClamp = {};
    pointClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClamp.MipLODBias = 0.f;
    pointClamp.MinLOD = 0.0f;
    pointClamp.MaxLOD = D3D12_FLOAT32_MAX;
    pointClamp.MaxAnisotropy = 1;
    pointClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointClamp.ShaderRegister = 1; // register(s1)
    pointClamp.RegisterSpace = 0;  // space0
    pointClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    pointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC linearWrap = {};
    linearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrap.MipLODBias = 0.f;
    linearWrap.MinLOD = 0.0f;
    linearWrap.MaxLOD = D3D12_FLOAT32_MAX;
    linearWrap.MaxAnisotropy = 1;
    linearWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearWrap.ShaderRegister = 2; // register(s2)
    linearWrap.RegisterSpace = 0;  // space0
    linearWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    linearWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC linearClamp = {};
    linearClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClamp.MipLODBias = 0.f;
    linearClamp.MinLOD = 0.0f;
    linearClamp.MaxLOD = D3D12_FLOAT32_MAX;
    linearClamp.MaxAnisotropy = 1;
    linearClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearClamp.ShaderRegister = 3; // register(s3)
    linearClamp.RegisterSpace = 0;  // space0
    linearClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    linearClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC anisotropicWrap = {};
    anisotropicWrap.Filter = D3D12_FILTER_ANISOTROPIC;
    anisotropicWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicWrap.MipLODBias = 0.f;
    anisotropicWrap.MinLOD = 0.0f;
    anisotropicWrap.MaxLOD = D3D12_FLOAT32_MAX;
    anisotropicWrap.MaxAnisotropy = 8;
    anisotropicWrap.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    anisotropicWrap.ShaderRegister = 4; // register(s4)
    anisotropicWrap.RegisterSpace = 0;  // space0
    anisotropicWrap.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    anisotropicWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_STATIC_SAMPLER_DESC anisotropicClamp = {};
    anisotropicClamp.Filter = D3D12_FILTER_ANISOTROPIC;
    anisotropicClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    anisotropicClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    anisotropicClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    anisotropicClamp.MipLODBias = 0.f;
    anisotropicClamp.MinLOD = 0.0f;
    anisotropicClamp.MaxLOD = D3D12_FLOAT32_MAX;
    anisotropicClamp.MaxAnisotropy = 8;
    anisotropicClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    anisotropicClamp.ShaderRegister = 5; // register(s5)
    anisotropicClamp.RegisterSpace = 0;  // space0
    anisotropicClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    anisotropicClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    return {pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp};
}
} // namespace JEngine