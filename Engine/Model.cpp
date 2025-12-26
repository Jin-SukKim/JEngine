#include "pch.h"
#include "Model.h"
#include "Mesh.h"
#include "UploadBuffer.h"

namespace JEngine {

Model::Model() : meshConst_() {
}

Model::~Model() {
    ReleaseStagingBuffers();
    for (auto& cb : constantBuffers_) {
        cb.Reset();
    }
}

Model::Model(Model&& other) noexcept
    : meshes_(std::move(other.meshes_)), meshConst_(std::move(other.meshConst_)),
      constantBuffers_(std::move(other.constantBuffers_)) {
    other.meshes_.clear();
}

Model& Model::operator=(Model&& other) noexcept {
    if (this != &other) {
        meshes_ = std::move(other.meshes_);
        meshConst_ = std::move(other.meshConst_);
        constantBuffers_ = std::move(other.constantBuffers_);

        other.meshes_.clear();
    }
    return *this;
}

void Model::Update(size_t frameIdx) {
    for (size_t i = 0; i < meshes_.size(); ++i) {
        constantBuffers_[frameIdx].Update(meshConst_, i);
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
        cb.CreateConstantBufferArray(meshes_.size(), sizeof(MeshConstants));
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
    using namespace DirectX;
    // Shader에서 사용하기 위해 Transpose된 행렬로 저장
    XMStoreFloat4x4(&meshConst_.world, XMMatrixTranspose(matrix));
    XMStoreFloat4x4(&meshConst_.invWorld, XMMatrixTranspose(XMMatrixInverse(nullptr, matrix)));
}

void Model::SetWorldMatrix(const DirectX::XMFLOAT4X4& worldMatrix) {
    DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&worldMatrix);
    UpdateWorldMatrix(worldMat);
}

const DirectX::XMFLOAT4X4 Model::GetWorldMatrix() const {
    using namespace DirectX;
    DirectX::XMMATRIX mat = XMLoadFloat4x4(&meshConst_.world);
    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, XMMatrixTranspose(mat));
    return world;
}

} // namespace JEngine