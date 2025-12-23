#pragma once
#include "Camera.h"

namespace JEngine {

class Timer;
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
    void Update(const Timer& timer, Model& model);
    void Draw(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx);
    void Resize();

    ID3D12PipelineState* GetPSO() const;

  private:
    void InitResources();
    void InitShaders();
    void InitRootSignature();
    void InitPipeline();

  private:
    Context& context_;
    SwapChain& swapChain_;
    Camera camera_;

    std::unique_ptr<Texture> depthStencil_;
    std::wstring assetsPath_ = L"..\\Assets\\";
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<RootSignature> rootSignature_;
    std::unique_ptr<Pipeline> pipeline_;
};

} // namespace JEngine