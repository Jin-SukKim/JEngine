#pragma once
#include "Camera.h"

namespace JEngine {

class GPUBuffer;
class Texture;
class UploadBuffer;
class Context;
class Model;
class CommandBuffer;
class ShaderManager;
class RootSignature;
class Pipeline;
class SwapChain;

class Renderer
{
  public:
    Renderer(Context& ctx, SwapChain& swapChain);
    ~Renderer();

    void Initialize();
    void Update(size_t frameIdx);
    void Draw(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx);
    void Resize();

    void UpdateViewport();
    void ApplyViewport(ID3D12GraphicsCommandList* cmdList);

    ID3D12PipelineState* GetPSO() const;
    Camera& GetCamera();

  private:
    void InitResources();
    void InitShaders();
    void InitRootSignature();
    void InitPipeline();
    void InitSamplers();

    void BeginRenderPass(ID3D12GraphicsCommandList* cmdList);
    void BindPipeline(ID3D12GraphicsCommandList* cmdList, size_t frameIdx);
    void DrawModel(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx);
    void EndRenderPass(ID3D12GraphicsCommandList* cmdList);

  private:
    Context& context_;
    SwapChain& swapChain_;
    Camera camera_;

    std::unique_ptr<Texture> depthStencil_;
    std::wstring assetsPath_ = L"..\\Assets\\";
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<RootSignature> rootSignature_;
    std::unique_ptr<Pipeline> pipeline_;

    D3D12_VIEWPORT screenViewport_{};
    D3D12_RECT scissorRect_{};

    SceneConstants sceneConstants_;
    // frame마다 하나씩
    std::vector<UploadBuffer> sceneConstantBuffer_;
};

} // namespace JEngine