#pragma once

namespace JEngine {
/*
Root Parameter란 일종의 Page로 생각할 수 있음
DescriptorRange는 그 Page의 각 항목, 데이터
Root Signature는 이 Page들을 묶은 책과 같음

Draw 호출 시점에 RootSignature를 통해 어떤 Page(Root Parameter)를 갱신할지 결정
각 Page는 설정된 방식에 따라 데이터 형식이 맞춰 GPU에 전달됨

일반적으로 갱신 주기에 따라 Parameter를 나눠 설정 (예: Frame 단위, Object 단위 등)
그리고 갱신될 데이터 형식을 설정

성능상의 이유로 하나의 Root Signature에는 최대 64개의 DWORD(32-bit)만 설정 가능
*/

struct DescriptorRangeConfig
{
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    UINT numDescriptors = 1; // descriptor view 개수 (같은 크기의 Descriptor를 여러개 쓸 때)
    // 셰이더 레지스터 시작 번호 (시작 번호부터 numDescriptors 개수만큼 할당)
    UINT baseShaderRegister = 0;
    // Register Space (특별한 경우 아니면 0) - Shader Register를 지정하는 또 다른 차원
    /*
    ex) Texture2D diffuse : register(t0, space0);
        Texture2D specular : register(t0, space1);
         두 개의 Register가 같은 슬롯(ex: t0)에 겹쳐질 것 같지만, 
         각자 다른 공간에 있으므로 실제로는 서로 다른 레지스터
         배열을 사용할 경우 여러 개의 공간을 사용하는 것이 유용하고 크기를 미리 알 수 없는 배열을
    사용할 때도 유용
    */
    UINT registerSpace = 0; 
};

enum class RootParameterType {
    // Descriptor Heap 사용 (일반적) - Table당 DWORD 1개
    DESCRIPTOR_TABLE,
    // 직접 32-bit 상수 전달 - 32bit 상수당 DWORD 1개
    ROOT_CONSTANTS,
    // Descriptor 없이 GPU 주소 직접 전달 - DWORD 2개
    // - CBV나 자원 버퍼에 대한 SRV/UAV만 사용할 수 있는데 Texture에 대한 SRV는 불가능
    ROOT_DESCRIPTOR
};

struct RootSignatureConfig
{
    RootParameterType type = RootParameterType::DESCRIPTOR_TABLE;

    // type이 Descriptor Table일때 사용
    std::vector<DescriptorRangeConfig> descriptorRanges;

    // type이 Root Constants일때 사용
    UINT num32BitValues = 0; // 32-bit 상수들의 개수

    // type이 Root Constants, Root Descriptor일때 사용
    UINT shaderRegister = 0; // Shader Resgister 번호 (b0, t0 등)
    UINT registerSpace = 0;  // Register Space (특별한 경우 아니면 0)

    // type이 Root Descriptor일때 사용
    D3D12_ROOT_DESCRIPTOR_FLAGS descriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

    // 모든 Type에서 공통적으로 사용
    D3D12_SHADER_VISIBILITY shaderVisibility =
        D3D12_SHADER_VISIBILITY_ALL; // 어떤 Shader에서 접근 가능한지 설정 (기본값: ALL)
};

// Root Signature는 GPU와 CPU가 데이터를 주고받는 규칙(계약서)을 만드는 것
class RootSignature
{
  public:
    RootSignature(ID3D12Device* device);
    ~RootSignature();

    void Create(const std::vector<RootSignatureConfig>& config);

    ID3D12RootSignature* GetSignature() const;

    // RangeType 1개짜리 Descriptor Table 생성용
    static RootSignatureConfig CreateDescriptorTableConfig(
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT baseShaderRegister,
        UINT registerSpace = 0,
        D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);

    // RangeType 여러개짜리 Descriptor Table 생성용
    static RootSignatureConfig CreateDescriptorTableConfig(
        const std::vector<DescriptorRangeConfig>& descriptorRanges,
        D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
    // Range 설정
    static DescriptorRangeConfig CreateDescriptorRangeConfig(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
                                                             UINT numDescriptors,
                                                             UINT baseShaderRegister,
                                                             UINT registerSpace = 0);

  private:
    D3D12_ROOT_PARAMETER
    createRootParameter(const RootSignatureConfig& config,
                        std::vector<D3D12_DESCRIPTOR_RANGE>& descriptorRange);

    D3D12_DESCRIPTOR_RANGE createDescriptorRange(const DescriptorRangeConfig& config);
    D3D12_ROOT_PARAMETER
    createDescriptorTableParameter(const RootSignatureConfig& config,
                                   std::vector<D3D12_DESCRIPTOR_RANGE>& descriptorRanges);

    // TODO: 구현 필요
    D3D12_ROOT_PARAMETER createRootConstantsParameter(const RootSignatureConfig& config);
    D3D12_ROOT_PARAMETER createRootDescriptorParameter(const RootSignatureConfig& config);

    void createRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& rootParameters);

  private:
    ID3D12Device* device_;
    ComPtr<ID3D12RootSignature> rootSignature_;
};
} // namespace JEngine