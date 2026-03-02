#include "pch.h"
#include "Pipeline.h"
#include "ShaderManager.h"

namespace JEngine {
Pipeline::Pipeline(ID3D12Device* device, ShaderManager& shaderManager)
    : device_(device), shaderManager_(shaderManager) {
}

void Pipeline::CreatePSO(const PipelineConfig& config, ID3D12RootSignature* rootSignature) {
    if (!shaderManager_.IsPipelineShadersExist(config.name)) {
        LogError("Pipeline shaders '{}' do not exist.", config.name);
        return;
    }
    config_ = config;
    const PipelineShaders& pipelineShaders = shaderManager_.GetPipelineShaders(config.name);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {config.inputLayout.data(), (UINT)config.inputLayout.size()};
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = pipelineShaders.vs_->GetShader();
    psoDesc.PS = pipelineShaders.ps_->GetShader();
    psoDesc.RasterizerState = CreateRasterizerDesc();
    psoDesc.BlendState = CreateBlendDesc();
    psoDesc.DepthStencilState = CreateDepthStencilDesc();
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = config_.topologyType;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = config.rtvFormat;
    // MSAA 비활성화 (백 버퍼와 일치)
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = config.dsvFormat;
    ThrowIfFailed(device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_)));

    LogInfo("{} is created.", config.name);
}

ID3D12PipelineState* Pipeline::GetPSO() const {
    return pso_.Get();
}

D3D12_RASTERIZER_DESC Pipeline::CreateRasterizerDesc() const {
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode = config_.fillMode;
    rasterizerDesc.CullMode = config_.cullMode;
    rasterizerDesc.FrontCounterClockwise = config_.frontCounterClockwise;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return rasterizerDesc;
}

D3D12_BLEND_DESC Pipeline::CreateBlendDesc() const {
    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL};
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }
    return blendDesc;
}

D3D12_DEPTH_STENCIL_DESC Pipeline::CreateDepthStencilDesc() const {
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
    depthStencilDesc.DepthEnable = config_.depthEnable;
    depthStencilDesc.DepthWriteMask = config_.depthWriteMask;
    depthStencilDesc.DepthFunc = config_.depthFunc;
    depthStencilDesc.StencilEnable = config_.stencilEnable;
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS};
    depthStencilDesc.FrontFace = defaultStencilOp;
    depthStencilDesc.BackFace = defaultStencilOp;
    return depthStencilDesc;
}

} // namespace JEngine