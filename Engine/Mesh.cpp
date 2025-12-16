#include "pch.h"
#include "Mesh.h"
#include "Context.h"
#include "GPUBuffer.h"
#include "UploadBuffer.h"
#include "Vertex.h"

namespace JEngine {

Mesh::Mesh() {
}
Mesh::~Mesh() {
}

Mesh::Mesh(Mesh&& other) noexcept
    : name_(std::move(other.name_)), vertices_(std::move(other.vertices_)),
      indices_(std::move(other.indices_)), vertexBuffer_(std::move(other.vertexBuffer_)),
      indexBuffer_(std::move(other.indexBuffer_)) {
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        vertexBuffer_ = std::move(other.vertexBuffer_);
        indexBuffer_ = std::move(other.indexBuffer_);

        other.name_.clear();
        other.vertices_.clear();
        other.indices_.clear();
        other.vertexBuffer_ = nullptr;
        other.indexBuffer_ = nullptr;
    }
    return *this;
}

void Mesh::CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList) {
    // GPU 버퍼 생성
    vertexBuffer_ = std::make_unique<GPUBuffer>(ctx);
    vertexBuffer_->CreateVertexBuffer(vertices_.size(), sizeof(Vertex));

    indexBuffer_ = std::make_unique<GPUBuffer>(ctx);
    indexBuffer_->CreateIndexBuffer(indices_.size(), sizeof(std::uint32_t));

    // Staging Buffer를 통한 데이터 복사
    vertexUploadBuffer_ = std::make_unique<UploadBuffer>(ctx);
    vertexUploadBuffer_->CreateStagingBuffer(vertices_.size(), sizeof(Vertex));
    vertexUploadBuffer_->CopyDataToBuffer(cmdList, *vertexBuffer_, vertices_.data());

    indexUploadBuffer_ = std::make_unique<UploadBuffer>(ctx);
    indexUploadBuffer_->CreateStagingBuffer(indices_.size(), sizeof(std::uint32_t));
    indexUploadBuffer_->CopyDataToBuffer(cmdList, *indexBuffer_, indices_.data());

    // View 캐싱
    vertexBufferView_ = vertexBuffer_->CreateVertexBufferView(sizeof(Vertex));
    indexBufferView_ = indexBuffer_->CreateIndexBufferView(indexFormat_);
}

void Mesh::ReleaseStagingBuffers() {
    vertexUploadBuffer_ = nullptr;
    indexUploadBuffer_ = nullptr;
}

void Mesh::SetMesh(const std::string& name, const std::vector<Vertex>& vertices,
                   const std::vector<uint32_t>& indices) {
    name_ = name;
    vertices_ = vertices;
    indices_ = indices;
}

void Mesh::SetVertices(const std::vector<Vertex>& vertices) {
    vertices_ = vertices;
}

void Mesh::SetIndices(const std::vector<uint32_t>& indices) {
    indices_ = indices;
}

D3D12_VERTEX_BUFFER_VIEW* Mesh::GetVertexBufferView() {
    return &vertexBufferView_;
}

D3D12_INDEX_BUFFER_VIEW* Mesh::GetIndexBufferView() {
    return &indexBufferView_;
}

UINT Mesh::GetIndexCount() const {
    return static_cast<UINT>(indices_.size());
}

} // namespace JEngine