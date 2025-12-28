#include "pch.h"
#include "RootSignature.h"

namespace JEngine {
RootSignature::RootSignature(ID3D12Device* device) : device_(device), rootSignature_(nullptr) {
}

RootSignature::~RootSignature() {
}

void RootSignature::Create(const std::vector<RootSignatureConfig>& rootParamConfigs) {
    // 각 Root Parameter에 대한 Descriptor Range들을 저장할 벡터 (RootSignature가 생성될때까지 유효해야됨)
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges;
    descriptorRanges.reserve(rootParamConfigs.size());

    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    rootParameters.reserve(rootParamConfigs.size());

    for (const auto& config : rootParamConfigs) {
        // 현재 Parameter의 Descriptor Range(데이터 형식)들을 저장
        std::vector<D3D12_DESCRIPTOR_RANGE> rangesForCurConfig;

        // 현재 Parameter 생성
        D3D12_ROOT_PARAMETER param = createRootParameter(config, rangesForCurConfig);

        // Range가 있다면 저장 (Descriptor Table인 경우만 해당)
        if (!rangesForCurConfig.empty()) {
            descriptorRanges.emplace_back(std::move(rangesForCurConfig));

            // move된 벡터의 데이터를 가리키도록 설정
            param.DescriptorTable.pDescriptorRanges = descriptorRanges.back().data();
        }

        rootParameters.emplace_back(param);
    }

    createRootSignature(rootParameters);
}

void RootSignature::createRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& rootParameters) {
    // 위 모든 규칙을 모아 '최종 계약서' 완성 (Root Signature Description) ---
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

    // Pipeline에서 사용할 Root Parameter 개수와 데이터
    rootSignatureDesc.NumParameters = static_cast<UINT>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data();

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
    return rootSignature_.Get();
}

RootSignatureConfig RootSignature::CreateDescriptorTableConfig(
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT baseShaderRegister,
    UINT registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility) {
    DescriptorRangeConfig rangeConfig = CreateDescriptorRangeConfig(rangeType, numDescriptors, baseShaderRegister, registerSpace);
    
    RootSignatureConfig config = {};
    config.type = RootParameterType::DESCRIPTOR_TABLE;
    config.descriptorRanges.emplace_back(rangeConfig);
    config.shaderVisibility = shaderVisibility;
    return config;
}

RootSignatureConfig RootSignature::CreateDescriptorTableConfig(
    const std::vector<DescriptorRangeConfig>& descriptorRanges,
    D3D12_SHADER_VISIBILITY shaderVisibility) {
    RootSignatureConfig config = {};
    config.type = RootParameterType::DESCRIPTOR_TABLE;
    config.descriptorRanges = descriptorRanges;
    config.shaderVisibility = shaderVisibility;
    return config;
}

DescriptorRangeConfig
RootSignature::CreateDescriptorRangeConfig(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
                                           UINT numDescriptors, UINT baseShaderRegister,
                                           UINT registerSpace) {
    DescriptorRangeConfig config = {};
    config.rangeType = rangeType;
    config.numDescriptors = numDescriptors;
    config.baseShaderRegister = baseShaderRegister;
    config.registerSpace = registerSpace;
    return config;
}

D3D12_ROOT_PARAMETER
RootSignature::createRootParameter(const RootSignatureConfig& config,
                                   std::vector<D3D12_DESCRIPTOR_RANGE>& descriptorRange) {
    // C++(CPU)가 데이터를 '전달할 방식' 정의(Root Parameter)-- -
    D3D12_ROOT_PARAMETER rootParameter = {};
    switch (config.type) {
    case RootParameterType::DESCRIPTOR_TABLE:
        rootParameter = createDescriptorTableParameter(config, descriptorRange);
        break;
    case RootParameterType::ROOT_CONSTANTS:
        rootParameter = createRootConstantsParameter(config);
        break;
    case RootParameterType::ROOT_DESCRIPTOR:
        rootParameter = createRootDescriptorParameter(config);
        break;
    }

    // 접근 가능한 Shader 종류 설정
    rootParameter.ShaderVisibility = config.shaderVisibility;
    return rootParameter;
}

D3D12_DESCRIPTOR_RANGE RootSignature::createDescriptorRange(const DescriptorRangeConfig& config) {
    // 셰이더(GPU)가 받을 '슬롯' 정의 (Descriptor Range) ---
    // Shader가 데이터를 받을 Register가 뭔지 정의
    D3D12_DESCRIPTOR_RANGE descriptorRange = {};

    // Register로 받을 view 종류 (CBV, SRV, UAV, Sampler 등)
    descriptorRange.RangeType = config.rangeType;

    // Register 몇개를 쓸지 (같은 크기의 Descriptor를 여러개 쓸 때)
    descriptorRange.NumDescriptors = config.numDescriptors;
    // 몇 번 Register부터 시작할지 (t0, t2 등)
    descriptorRange.BaseShaderRegister = config.baseShaderRegister;

    // RegisterSpace: 특별한 경우 아니면 0
    descriptorRange.RegisterSpace = config.registerSpace;

    // OffsetInDescriptorsFromTableStart: 테이블 내에서의 순서. (APPEND = 순서대로)
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return descriptorRange;
}

D3D12_ROOT_PARAMETER RootSignature::createDescriptorTableParameter(
    const RootSignatureConfig& config, std::vector<D3D12_DESCRIPTOR_RANGE>& descriptorRanges) {
    D3D12_ROOT_PARAMETER param = {};

    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    for (const auto& rangeConfig : config.descriptorRanges) {
        D3D12_DESCRIPTOR_RANGE descriptorRange = createDescriptorRange(rangeConfig);
        descriptorRanges.emplace_back(descriptorRange);
    }

    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRanges.size());
    // 현재 pointer는 Create()에서 설정
    param.DescriptorTable.pDescriptorRanges = nullptr;
    //param.DescriptorTable.pDescriptorRanges = descriptorRanges.data();
    return param;
}

D3D12_ROOT_PARAMETER
RootSignature::createRootConstantsParameter(const RootSignatureConfig& config) {
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants.Num32BitValues = config.num32BitValues;
    param.Constants.ShaderRegister = config.shaderRegister;
    param.Constants.RegisterSpace = config.registerSpace;
    return param;
}

D3D12_ROOT_PARAMETER
RootSignature::createRootDescriptorParameter(const RootSignatureConfig& config) {
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // (참고: SRV, UAV 분기 필요시 수정)
    param.Descriptor.ShaderRegister = config.shaderRegister;
    param.Descriptor.RegisterSpace = config.registerSpace;
    // param.Descriptor.Flags = config.descriptorFlags; (D3D12_ROOT_DESCRIPTOR1 사용시 필요)
    return param;
}
} // namespace JEngine