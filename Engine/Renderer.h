#pragma once
#include "Camera.h"

namespace JEngine {

struct MeshConst
{
    DirectX::XMFLOAT4X4 worldViewProj =
        DirectX::XMFLOAT4X4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
};

class Timer;
class GPUBuffer;
class Texture;
class UploadBuffer;
class Context;

class Renderer
{
  public:
    Renderer(Context& ctx);
    ~Renderer();

    void Initialize();
    void Update(const Timer& timer);
    void Draw(ID3D12GraphicsCommandList* cmdList, Texture& backBuffer);
    void Resize();

    void CreateConstantBuffer();
    void CreateRootSignature();
    void SetInputLayout();
    ComPtr<ID3DBlob> CompileShader(const std::wstring& filePath, const std::string& entryPoint,
                                   const std::string& target);
    void BuildShaders();
    void InitBox(ID3D12GraphicsCommandList* cmdList);
    void CreatePSO(DXGI_FORMAT backFormat);

    DirectX::XMFLOAT4X4 Identity4x4() {
        static DirectX::XMFLOAT4X4 I(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                     1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        return I;
    }

    ID3D12PipelineState* GetPSO() const {
        return mPSO.Get();
    }

  private:
    Context& context_;
    std::unique_ptr<Texture> depthStencil_;

    std::unique_ptr<UploadBuffer> constantBuffer_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
    ComPtr<ID3D12RootSignature> rootSignature_;

    ComPtr<ID3DBlob> vertexShader_;
    ComPtr<ID3DBlob> pixelShader_;

    std::unique_ptr<GPUBuffer> vertexBufferGPU_;
    std::unique_ptr<GPUBuffer> indexBufferGPU_;

    // ✅ 초기화 중에만 사용되는 업로드 버퍼 (GPU 작업 완료 후 해제)
    std::unique_ptr<UploadBuffer> vertexUploadBuffer_;
    std::unique_ptr<UploadBuffer> indexUploadBuffer_;

    // View 캐싱용 멤버 변수
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    UINT indexCount_ = 0;
    UINT vertexByteStride_ = 0;
    DXGI_FORMAT indexFormat_ = DXGI_FORMAT_R16_UINT;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    std::wstring assetsPath_ = L"../../Assets/";
    DirectX::XMFLOAT4X4 world_;
    MeshConst meshConst_;

    Camera camera_;
};

} // namespace JEngine