#include "pch.h"
#include "Renderer.h"
#include "Context.h"
#include "Resource.h"
#include "Window.h"
#include "DescriptorHeap.h"
#include "Vertex.h"
#include "Timer.h"
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

    // 카메라 초기화
    static float theta = 1.5f * DirectX::XM_PI;
    static float phi = DirectX::XM_PIDIV4;
    static float radius = 5.0f;

    float x = radius * std::sinf(phi) * std::cosf(theta);
    float y = radius * std::sinf(phi) * std::sinf(theta);
    float z = radius * std::cosf(phi);

    camera_.SetPosition(DirectX::XMFLOAT3(x, y, z));
    camera_.SetPerspective(45.f, context_.GetWindow().GetAspectRatio(), 0.1f, 100.0f);
}

void Renderer::Update(const Timer& timer, Model& model, size_t frameIdx) {
    using namespace DirectX;

    camera_.UpdateViewMatrix();

    // Scene Constant Buffer 업데이트
    camera_.UpdateSceneConstants(sceneConstants_);
    sceneConstantBuffer_[frameIdx].Update(sceneConstants_);

    // ⭐ World Matrix - 박스를 제자리에서 회전시킴
    // 경과 시간에 따라 회전 각도 계산 (라디안 단위)
    float rotationAngle = timer.TotalTime() * 0.5f; // 0.5는 회전 속도 (조절 가능)
    XMMATRIX world = XMMatrixRotationZ(rotationAngle * 0.3f) *
                             XMMatrixRotationY(rotationAngle);

    model.UpdateWorldMatrix(world);
}

void Renderer::Draw(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx) {
    Texture& backBuffer = swapChain_.GetCurrentBackBuffer();
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBuffer.GetCPUHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetCPUHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   1.0f, 0, 0, nullptr);

    ID3D12DescriptorHeap* cbvHeap =
        context_.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap};
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootSignature(rootSignature_->GetSignature());
    cmdList->SetPipelineState(pipeline_->GetPSO());

    // Scene Constant Buffer Binding (Root Parameter 1)
    cmdList->SetGraphicsRootDescriptorTable(1, sceneConstantBuffer_[frameIdx].GetGPUHandle());

    std::vector<Mesh>& meshes = model.GetMeshes();
    for (size_t i = 0; i < meshes.size(); ++i) {
        Mesh& mesh = meshes[i];
        cmdList->IASetVertexBuffers(0, 1, mesh.GetVertexBufferView());
        cmdList->IASetIndexBuffer(mesh.GetIndexBufferView());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->SetGraphicsRootDescriptorTable(0, model.GetConstantGPUHandle(frameIdx, i)); // Constant Buffer Binding
        cmdList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
    }

    // Back Buffer를 RENDER_TARGET → PRESENT 상태로 전환
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
} // namespace JEngine
