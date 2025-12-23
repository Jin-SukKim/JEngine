#include "pch.h"
#include "Model.h"
#include "Mesh.h"
#include "UploadBuffer.h"

namespace JEngine {

Model::Model() : worldMatrix_(DirectX::XMFLOAT4X4()) {
}

Model::~Model() {
    ReleaseStagingBuffers();
    for (auto& cb : constantBuffers_) {
        cb.Reset();
    }
}

Model::Model(Model&& other) noexcept
    : meshes_(std::move(other.meshes_)), worldMatrix_(std::move(other.worldMatrix_)),
      constantBuffers_(std::move(other.constantBuffers_)) {
    other.meshes_.clear();
}

Model& Model::operator=(Model&& other) noexcept {
    if (this != &other) {
        meshes_ = std::move(other.meshes_);
        worldMatrix_ = std::move(other.worldMatrix_);
        constantBuffers_ = std::move(other.constantBuffers_);

        other.meshes_.clear();
    }
    return *this;
}

void Model::Update(size_t frameIdx) {
    for (size_t i = 0; i < meshes_.size(); ++i) {
        constantBuffers_[frameIdx].Update(i, worldMatrix_);
    }
}

void Model::CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList) {
    for (auto& mesh : meshes_)
        mesh.CreateBuffers(ctx, cmdList);

    // 각 메시에 대한 상수 버퍼 생성
    constantBuffers_.clear();
    constantBuffers_.reserve(MAX_FRAME_COUNT);
    for (size_t i = 0; i < MAX_FRAME_COUNT; ++i) {
        auto& cb = constantBuffers_.emplace_back(UploadBuffer(ctx));
        cb.CreateConstantBufferArray(meshes_.size(), sizeof(DirectX::XMFLOAT4X4));
    }
}

void Model::ReleaseStagingBuffers() {
    for (auto& mesh : meshes_)
        mesh.ReleaseStagingBuffers();
}

void Model::AddMesh(Context& ctx, ID3D12GraphicsCommandList* cmdList, const std::string& name,
                    const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {

    Mesh mesh;
    mesh.SetMesh(name, vertices, indices);
    meshes_.emplace_back(std::move(mesh));

    CreateBuffers(ctx, cmdList);
}

const std::vector<Mesh>& Model::GetMeshes() const {
    return meshes_;
}

std::vector<Mesh>& Model::GetMeshes() {
    return meshes_;
}

Mesh& Model::GetMesh(size_t index) {
    return meshes_.at(index);
}

D3D12_GPU_DESCRIPTOR_HANDLE Model::GetConstantGPUHandle(size_t frameIdx, size_t meshIdx) const {
    return constantBuffers_[frameIdx].GetGPUHandle(meshIdx);
}

void Model::UpdateWorldMatrix(const DirectX::XMMATRIX& matrix) {
    DirectX::XMStoreFloat4x4(&worldMatrix_, XMMatrixTranspose(matrix));
}

void Model::SetWorldMatrix(const DirectX::XMFLOAT4X4& worldMatrix) {
    worldMatrix_ = worldMatrix;
}

const DirectX::XMFLOAT4X4& Model::GetWorldMatrix() const {
    return worldMatrix_;
}

} // namespace JEngine