#pragma once

namespace JEngine {

class ShaderManager;

// 파이프라인 구성 정보
// TODO: add parameters as needed
struct PipelineConfig
{
    std::string name;

    // 입력 레이아웃 (임시 고정)
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
        // 위치 (Position) 속성
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        // 색상 (Color) 속성
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // 래스터라이저
    D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    BOOL frontCounterClockwise = FALSE;

    // Depth
    BOOL depthEnable = TRUE;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;

    // Stencil
    BOOL stencilEnable = FALSE;

    // 블렌딩
    // ... (추가 구성 옵션 필요 시 여기에 추가)

    // 포맷
    DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 토폴로지
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
};

class Pipeline
{
  public:
    Pipeline(ID3D12Device& device, ShaderManager& shaderManager);

    void CreatePSO(const PipelineConfig& config, ID3D12RootSignature* rootSignature);
    ID3D12PipelineState* GetPSO() const;

  private:
    D3D12_RASTERIZER_DESC CreateRasterizerDesc() const;
    D3D12_BLEND_DESC CreateBlendDesc() const;
    D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc() const;

  private:
    ID3D12Device& device_;
    ShaderManager& shaderManager_;
    PipelineConfig config_;
    ComPtr<ID3D12PipelineState> pso_;
};
} // namespace JEngine