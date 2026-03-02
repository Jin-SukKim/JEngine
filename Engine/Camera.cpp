#include "pch.h"
#include "Camera.h"

namespace JEngine {

Camera::Camera(CameraType type) : type_(type) {
    // Identity matrix로 초기화
    DirectX::XMStoreFloat4x4(&matrices.view_, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&matrices.perspective_, DirectX::XMMatrixIdentity());

    LogInfo("Camera Created.");

    // 임시 Camera 위치
    static float theta = 1.5f * DirectX::XM_PI;
    static float phi = DirectX::XM_PIDIV4;
    static float radius = 5.0f;

    float x = radius * std::sinf(phi) * std::cosf(theta);
    float y = radius * std::sinf(phi) * std::sinf(theta);
    float z = radius * std::cosf(phi);

    position_ = DirectX::XMFLOAT3(x, y, z);
}

Camera::~Camera() {
}

void Camera::Update() {
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position_);
    pos = DirectX::XMVectorSetW(pos, 1.0f);
    DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&up_);

    // TODO: 임시 Target
    DirectX::XMVECTOR target = DirectX::XMVectorZero();

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
    DirectX::XMStoreFloat4x4(&matrices.view_, view);
}

void Camera::UpdateSceneConstants(SceneConstants& sceneConst) {
    DirectX::XMMATRIX view =
        DirectX::XMMatrixTranspose(GetViewMatrix());
    DirectX::XMMATRIX proj = DirectX::XMMatrixTranspose(GetProjMatrix());
    DirectX::XMMATRIX viewProj = proj * view;

    DirectX::XMStoreFloat4x4(&sceneConst.view, view);
    DirectX::XMStoreFloat4x4(&sceneConst.invView, DirectX::XMMatrixInverse(nullptr, view));
    DirectX::XMStoreFloat4x4(&sceneConst.proj, proj);
    DirectX::XMStoreFloat4x4(&sceneConst.invProj, DirectX::XMMatrixInverse(nullptr, proj));
    DirectX::XMStoreFloat4x4(&sceneConst.viewProj, viewProj);
    DirectX::XMStoreFloat4x4(&sceneConst.invViewProj, DirectX::XMMatrixInverse(nullptr, viewProj));
    sceneConst.eyeWorld = position_;
}

void Camera::SetType(CameraType type) {
    type_ = type;
}

void Camera::SetPosition(const DirectX::XMFLOAT3& position) {
    position_ = position;
}

void Camera::SetRotation(const DirectX::XMFLOAT3& rotation) {
    SetRotation(rotation.x, rotation.y, rotation.z);
}

void Camera::SetRotation(float pitch, float yaw, float roll) {
    // 피치 각도를 -90도에서 +90도 사이로 제한
    static float maxPitch = DirectX::XM_PIDIV2 - 0.01f;
    rotation_.x = std::clamp(pitch, -maxPitch, maxPitch);
    rotation_.y = yaw;
    rotation_.z = roll;
}

void Camera::SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ) {
    parameters.fovY_ = fovY;
    parameters.nearZ_ = nearZ;
    parameters.farZ_ = farZ;

    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(fovY), aspectRatio, nearZ, farZ);
    DirectX::XMStoreFloat4x4(&matrices.perspective_, proj);
}

DirectX::XMMATRIX Camera::GetViewMatrix() const {
    return DirectX::XMLoadFloat4x4(&matrices.view_);
}

DirectX::XMMATRIX Camera::GetProjMatrix() const {
    return DirectX::XMLoadFloat4x4(&matrices.perspective_);
}

DirectX::XMMATRIX Camera::GetViewProjMatrix() const {
    return GetViewMatrix() * GetProjMatrix();
}

} // namespace JEngine