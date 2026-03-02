#include "pch.h"
#include "Renderer.h"
#include "Context.h"
#include "Resource.h"
#include "Window.h"
#include "DescriptorHeap.h"
#include "Vertex.h"
#include "UploadBuffer.h"
#include "GPUBuffer.h"
#include "Texture.h"
#include "GeometryGenerator.h"
#include "Model.h"
#include "Mesh.h"
#include "CommandBuffer.h"
#include "ShaderManager.h"
#include "RootSignature.h"
#include "Pipeline.h"
#include "SwapChain.h"

namespace JEngine {
Renderer::Renderer(Context& ctx, SwapChain& swapChain)
    : context_(ctx), swapChain_(swapChain), camera_(Camera::CameraType::LOOK_AT) {
}

Renderer::~Renderer() = default;

void Renderer::Initialize() {
    InitResources();
    InitShaders();
    InitRootSignature();
    InitPipeline();
}

void Renderer::Update(size_t frameIdx) {
    camera_.Update();

    // Scene Constant Buffer 업데이트
    camera_.UpdateSceneConstants(sceneConstants_);
    sceneConstantBuffer_[frameIdx].Update(sceneConstants_);
}

void Renderer::Draw(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx) {
    BeginRenderPass(cmdList);
    BindPipeline(cmdList, frameIdx);
    DrawModel(cmdList, model, frameIdx);
    EndRenderPass(cmdList);
}

void Renderer::BeginRenderPass(ID3D12GraphicsCommandList* cmdList) {
    Texture& backBuffer = swapChain_.GetCurrentBackBuffer();
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBuffer.GetRTVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetDSVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   1.0f, 0, 0, nullptr);
}

void Renderer::BindPipeline(ID3D12GraphicsCommandList* cmdList, size_t frameIdx) {
    ID3D12DescriptorHeap* cbvHeap =
        context_.GetDescriptorPool()->Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap();
    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap};
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootSignature(rootSignature_->GetSignature());
    cmdList->SetPipelineState(pipeline_->GetPSO());

    // Scene Constant Buffer Binding (Root Parameter 1)
    cmdList->SetGraphicsRootDescriptorTable(1, sceneConstantBuffer_[frameIdx].GetGPUHandle());
}

void Renderer::DrawModel(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx) {
    std::vector<Mesh>& meshes = model.GetMeshes();
    for (size_t i = 0; i < meshes.size(); ++i) {
        Mesh& mesh = meshes[i];
        cmdList->IASetVertexBuffers(0, 1, mesh.GetVertexBufferView());
        cmdList->IASetIndexBuffer(mesh.GetIndexBufferView());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->SetGraphicsRootDescriptorTable(0,
                                                model.GetConstantGPUHandle(frameIdx, i));
        cmdList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
    }
}

void Renderer::EndRenderPass(ID3D12GraphicsCommandList* cmdList) {
    Texture& backBuffer = swapChain_.GetCurrentBackBuffer();
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PRESENT);
}

void Renderer::Resize() {

    // Depth Stencil 리소스 정리
    depthStencil_->Reset();
    context_.GetDescriptorPool()->ResetDSV();
    // Depth Stencil 버퍼 재생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight());

    camera_.SetPerspective(45.f, context_.GetWindow().GetAspectRatio(), 0.1f, 100.0f);
}

ID3D12PipelineState* Renderer::GetPSO() const {
    return pipeline_->GetPSO();
}

Camera& Renderer::GetCamera() {
    return camera_;
}

void Renderer::UpdateViewport() {
    Window& window = context_.GetWindow();
    LogInfo("Setting Viewport and Scissor Rect ({}x{})...", window.GetWidth(), window.GetHeight());

    screenViewport_.TopLeftX = 0.0f;
    screenViewport_.TopLeftY = 0.0f;
    screenViewport_.Width = static_cast<float>(window.GetWidth());
    screenViewport_.Height = static_cast<float>(window.GetHeight());
    screenViewport_.MinDepth = 0.0f;
    screenViewport_.MaxDepth = 1.0f;

    scissorRect_ = {0, 0, static_cast<LONG>(window.GetWidth()),
                    static_cast<LONG>(window.GetHeight())};

    LogInfo("Viewport and Scissor Rect configured successfully.");
}

void Renderer::ApplyViewport(ID3D12GraphicsCommandList* cmdList) {
    cmdList->RSSetViewports(1, &screenViewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);
}

void Renderer::InitResources() {
    // Depth Stencil 이미지 객체 생성
    depthStencil_ = std::make_unique<Texture>(context_);
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight());

    // Frame마다 하나씩 Scene Constant Buffer 생성
    sceneConstantBuffer_.clear();
    sceneConstantBuffer_.reserve(MAX_FRAME_COUNT);
    for (size_t i = 0; i < MAX_FRAME_COUNT; ++i) {
        auto& cb = sceneConstantBuffer_.emplace_back(UploadBuffer(context_));
        cb.CreateConstantBuffer(sizeof(SceneConstants));
    }

}

void Renderer::InitShaders() {
    ShaderConfig vsConfig = {"BasicVS", L"Shaders\\Color.hlsl", "VSMain", "vs_5_0"};
    ShaderConfig psConfig = {"BasicPS", L"Shaders\\Color.hlsl", "PSMain", "ps_5_0"};

    shaderManager_ = std::make_unique<ShaderManager>(assetsPath_);
    shaderManager_->LoadShader(vsConfig);
    shaderManager_->LoadShader(psConfig);

    LogInfo("Shaders compiled successfully.");
}

void Renderer::InitRootSignature() {

    std::vector<RootSignatureConfig> configs;

    // Per-Object CBV Descriptor Table 설정
    configs.emplace_back(RootSignature::CreateDescriptorTableConfig(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0));

    // Per-Frame CBV Descriptor Table 설정
    configs.emplace_back(RootSignature::CreateDescriptorTableConfig(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1));

    // Sampler
    //configs.emplace_back(RootSignature::CreateDescriptorTableConfig(
    //    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL));

    // Example) Material (CBV + Textures)
    //{
    //    std::vector<DescriptorRangeConfig> ranges;

    //    DescriptorRangeConfig matCBVRange = RootSignature::CreateDescriptorRangeConfig(
    //        D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
    //    ranges.emplace_back(matCBVRange);

    //    DescriptorRangeConfig matTextureRange =
    //        RootSignature::CreateDescriptorRangeConfig(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);
    //    ranges.emplace_back(matTextureRange);

    //    configs.push_back(
    //        RootSignature::CreateDescriptorTableConfig(ranges, D3D12_SHADER_VISIBILITY_PIXEL));
    //}

    rootSignature_ = std::make_unique<RootSignature>(context_.GetDevice());
    rootSignature_->Create(configs);
}

void Renderer::InitPipeline() {
    PipelineShadersConfig pipelineConfig;
    pipelineConfig.vsName_ = "BasicVS";
    pipelineConfig.psName_ = "BasicPS";

    shaderManager_->CreatePipelineShaders("BasicPipeline", pipelineConfig);

    PipelineConfig config;
    config.name = "BasicPipeline";
    config.inputLayout = {
        // 위치 (Position) 속성
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        // 색상 (Color) 속성
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    config.rtvFormat = swapChain_.GetBackBufferFormat();
    config.dsvFormat = depthStencil_->GetFormat();

    pipeline_ = std::make_unique<Pipeline>(context_.GetDevice(), *shaderManager_);
    pipeline_->CreatePSO(config, rootSignature_->GetSignature());
}

void Renderer::InitSamplers() {
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Filter 설정
    // Address Mode 설정 (Wrap, Clamp, Mirror, Border, Mirror_Once)
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.0f; // Mipmap LOD의 bias 설정 (기본값 0.0f)
    samplerDesc.MinLOD = 0.0f; // 선택 가능한 최소 Mipmap Level
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

    // TODO: static sampler으로 Root Signature에 추가하는 방법 고려 (성능이 더 좋음) - RootSignature와 연계
    // https://gemini.google.com/app/d2f444dcda7b30dd
    D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
}

} // namespace JEngine
