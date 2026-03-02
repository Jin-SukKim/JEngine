#pragma once

namespace JEngine {

class Mesh;
class UploadBuffer;
class Vertex;
class Context;

class Model
{
  public:
    Model();
    ~Model();

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    void Update(size_t frameIdx);

    void CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList);
    void ReleaseStagingBuffers();

    void UpdateWorldMatrix(const DirectX::XMMATRIX& matrix);

    void AddMesh(Context& ctx, ID3D12GraphicsCommandList* cmdList, const std::string& name,
                 const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    const std::vector<Mesh>& GetMeshes() const;
    std::vector<Mesh>& GetMeshes();
    Mesh& GetMesh(size_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetConstantGPUHandle(size_t frameIdx, size_t meshIdx = 0) const;

    void SetWorldMatrix(const DirectX::XMFLOAT4X4& worldMatrix);
    const DirectX::XMFLOAT4X4 GetWorldMatrix() const;

  private:
    // TODO: 하나의 Model의 모든 Mesh의 vertex, index buffer 등을 하나로 합치고 offset으로
    //       관리해 binding을 줄여서 buffer 전환 비용 최적화
    // 	     SubMesh 개념으로 IndexCount, StartIndexLocation, BaseVertexLocation 등 정보를 가지고 있으면 됨
    //       ex) unique_ptr<Mesh> mesh_; -> 하나의 Mesh에 모든 데이터
    //           std::vector<SubMesh> subMeshes_; -> 각 SubMesh의 Offset 정보들
    std::vector<Mesh> meshes_;

    // Model에 대한 상수 버퍼(world, 나중에 animation은 따로 skeleton class나 animation class를 통해 bone별 matrix로 관리)
    MeshConstants meshConst_;

    // Frame별 상수 버퍼
    std::vector<UploadBuffer> constantBuffers_;
};
} // namespace JEngine