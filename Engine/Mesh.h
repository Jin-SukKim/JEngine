#pragma once

namespace JEngine {

class Vertex;
class GPUBuffer;
class UploadBuffer;
class Context;
class CommandBuffer;

class Mesh
{
  public:
    Mesh();
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList);
    void ReleaseStagingBuffers();

    void SetMesh(const std::string& name,
                 const std::vector<Vertex>& vertices,
                 const std::vector<uint32_t>& indices);
    void SetVertices(const std::vector<Vertex>& vertices);
    void SetIndices(const std::vector<uint32_t>& indices);

    D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();
    UINT GetIndexCount() const;
  private:
    std::string name_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    DXGI_FORMAT indexFormat_ = DXGI_FORMAT_R32_UINT;

    std::unique_ptr<GPUBuffer> vertexBuffer_;
    std::unique_ptr<GPUBuffer> indexBuffer_;

    // View 캐싱용 멤버 변수
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    // 초기화 중에만 사용되는 업로드 버퍼 (GPU 작업 완료 후 해제) - TODO: Mesh에서 임시로 생성해서 사용하는거 테스트 해보기
    std::unique_ptr<UploadBuffer> vertexUploadBuffer_;
    std::unique_ptr<UploadBuffer> indexUploadBuffer_;

};

} // namespace JEngine
