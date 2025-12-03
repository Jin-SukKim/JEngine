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
class Shader;

class Renderer
{
  public:
    Renderer(Context& ctx);
    ~Renderer();

    void Initialize();
    void Update(const Timer& timer, Model& model);
    void Draw(ID3D12GraphicsCommandList* cmdList, Texture& backBuffer, Model& model);
    void Resize();

    void CreateRootSignature();
    void SetInputLayout();
    ComPtr<ID3DBlob> CompileShader(const std::wstring& filePath, const std::string& entryPoint,
                                   const std::string& target);
    void BuildShaders();
    void CreatePSO(DXGI_FORMAT backFormat);

    ID3D12PipelineState* GetPSO() const {
        return mPSO.Get();
    }

  private:
    Context& context_;
    std::unique_ptr<Texture> depthStencil_;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
    ComPtr<ID3D12RootSignature> rootSignature_;

    std::unique_ptr<Shader> vertexShader_;
    std::unique_ptr<Shader> pixelShader_;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    std::wstring assetsPath_ = L"..\\Assets\\";

    Camera camera_;
};

} // namespace JEngine