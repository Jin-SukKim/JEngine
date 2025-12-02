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

    void Update();

    void CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList);
    void ReleaseStagingBuffers();

    void AddMesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    const std::vector<Mesh>& GetMeshes() const;
    std::vector<Mesh>& GetMeshes();

    void UpdateWorldMatrix(const DirectX::XMMATRIX& matrix);
    void SetWorldMatrix(const DirectX::XMFLOAT4X4& worldMatrix);
    const DirectX::XMFLOAT4X4& GetWorldMatrix() const;

  private:
    std::vector<Mesh> meshes_;
    DirectX::XMFLOAT4X4 worldMatrix_;

    std::vector<UploadBuffer> constantBuffers_;
};
} // namespace JEngine