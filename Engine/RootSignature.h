#pragma once

namespace JEngine {

struct RootParamConfig
{
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    UINT numDescriptors = 1;     // descriptor view 개수
    UINT baseShaderRegister = 0; // 셰이더 레지스터 번호

    // 어떤 Shader에서 접근 가능한지 설정 (기본값: ALL)
    D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
};

// Root Signature는 GPU와 CPU가 데이터를 주고받는 규칙(계약서)을 만드는 것
class RootSignature
{
  public:
    RootSignature(ID3D12Device* device);
    ~RootSignature();

    void Create(const std::vector<RootParamConfig>& rootParamConfigs);

    ID3D12RootSignature* GetSignature() const;
  private:
    D3D12_DESCRIPTOR_RANGE CreateDescriptorRange(const RootParamConfig& config);
    D3D12_ROOT_PARAMETER
    CreateRootParameter(const RootParamConfig& config, D3D12_DESCRIPTOR_RANGE& descriptorRange);
    void CreateRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& rootParameters);


  private:
    ID3D12Device* device_;
    ID3D12RootSignature* rootSignature_;
};
} // namespace JEngine