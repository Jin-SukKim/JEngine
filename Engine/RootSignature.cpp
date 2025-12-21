#include "pch.h"
#include "RootSignature.h"

namespace JEngine {
RootSignature::RootSignature(ID3D12Device* device)
    : device_(device), rootSignature_(nullptr) {
}

RootSignature::~RootSignature() {
}

void RootSignature::Create(const std::vector<RootParamConfig>& rootParamConfigs) {
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges(rootParamConfigs.size());

    for (size_t i = 0; i < rootParamConfigs.size(); ++i) {
        descriptorRanges[i] = CreateDescriptorRange(rootParamConfigs[i]);
    }

    std::vector<D3D12_ROOT_PARAMETER> rootParameters(rootParamConfigs.size());

    for (size_t i = 0; i < rootParamConfigs.size(); ++i) {
        rootParameters[i] = CreateRootParameter(rootParamConfigs[i], descriptorRanges[i]);
    }

    CreateRootSignature(rootParameters);
}

D3D12_DESCRIPTOR_RANGE RootSignature::CreateDescriptorRange(const RootParamConfig& config) {
    // --- 1. 셰이더(GPU)가 받을 '슬롯' 정의 (Descriptor Range) ---
    // Shader가 데이터를 받을 Register가 뭔지 정의
    D3D12_DESCRIPTOR_RANGE descriptorRange = {};

    // Register로 받을 view 종류 (CBV, SRV, UAV, Sampler 등)
    descriptorRange.RangeType = config.rangeType;

    // Register 몇개를 쓸지
    descriptorRange.NumDescriptors = config.numDescriptors;
    // 몇 번 Register부터 시작할지 (t0, t2 등)
    descriptorRange.BaseShaderRegister = config.baseShaderRegister;

    // RegisterSpace: 특별한 경우 아니면 0
    descriptorRange.RegisterSpace = 0;

    // OffsetInDescriptorsFromTableStart: 테이블 내에서의 순서. (APPEND = 순서대로)
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return descriptorRange;
}

D3D12_ROOT_PARAMETER RootSignature::CreateRootParameter(const RootParamConfig& config,
                                                        D3D12_DESCRIPTOR_RANGE& descriptorRange) {
    // --- 2. C++(CPU)가 데이터를 '전달할 방식' 정의 (Root Parameter) ---
    D3D12_ROOT_PARAMETER rootParameter = {};

    // 데이터 전달 방식 (Desctiptor Table이 가장 일반적)
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    // 접근 가능한 Shader 종류 설정
    rootParameter.ShaderVisibility = config.shaderVisibility;

    // 어떤 Descriptor Range를 쓸지 지정
    rootParameter.DescriptorTable.NumDescriptorRanges = 1;
    rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRange;

    return rootParameter;
}

void RootSignature::CreateRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& rootParameters) {
    // --- 3. 위 모든 규칙을 모아 '최종 계약서' 완성 (Root Signature Description) ---
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

    // Pipeline에서 사용할 Root Parameter 개수와 데이터
    rootSignatureDesc.NumParameters = static_cast<UINT>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data(); // 그 1개가 바로 이것.

    // 고정된 Sampler (현재는 사용 안하는 중)
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;

    // Root Signature의 다양한 옵션 설정
    // (ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : 정점 데이터를 Input Assembler가 읽을 수 있도록 허용)
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // --- 4. 계약서를 GPU가 읽을 수 있는 '기계어'로 변환 (Serialize) ---
    ComPtr<ID3DBlob> signature; // 변환된 '기계어(바이너리)'가 저장될 곳
    ComPtr<ID3DBlob> error;

    // D3D12_ROOT_SIGNATURE_DESC -> 바이너리(signature)
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              signature.GetAddressOf(), error.GetAddressOf()));

    // --- 5. 변환된 '기계어'를 GPU에 제출하여 실제 '객체' 생성 ---
    ThrowIfFailed(
        device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                     IID_PPV_ARGS(&rootSignature_))); // 만들어진 객체 저장

    LogInfo("Root Signature created successfully.");
}

ID3D12RootSignature* RootSignature::GetSignature() const {
    return rootSignature_;
}
} // namespace JEngine