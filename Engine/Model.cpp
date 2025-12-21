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
    other.worldMatrix_ = {};
    other.constantBuffers_.clear();
}

Model& Model::operator=(Model&& other) noexcept {
    if (this != &other) {
        meshes_ = std::move(other.meshes_);
        worldMatrix_ = std::move(other.worldMatrix_);
        constantBuffers_ = std::move(other.constantBuffers_);

        other.meshes_.clear();
        other.worldMatrix_ = {};
        other.constantBuffers_.clear();
    }
    return *this;
}
void Model::Update() {
    for (auto& cb : constantBuffers_) {
        cb.Update(0, worldMatrix_);
    }
}

void Model::CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList) {
    for (auto& mesh : meshes_)
        mesh.CreateBuffers(ctx, cmdList);

    // 각 메시에 대한 상수 버퍼 생성
    constantBuffers_.clear();
    constantBuffers_.reserve(meshes_.size());
    for (size_t i = 0; i < meshes_.size(); ++i) {
        // emplace_back으로 직접 생성
        auto& cb = constantBuffers_.emplace_back(ctx);
        cb.CreateConstantBuffer(1, sizeof(DirectX::XMFLOAT4X4),
                                ctx.GetDescriptorPool()->AllocateCBV());
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

D3D12_GPU_DESCRIPTOR_HANDLE Model::GetConstantGPUHandle(size_t index) const {
    return constantBuffers_.at(index).GetGPUHandle();
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