#pragma once

namespace JEngine {
class Camera
{
  public:
    enum CameraType { LOOK_AT, FIRST_PERSON };

  public:
    Camera(CameraType type = CameraType::FIRST_PERSON);
    ~Camera();

    void UpdateViewMatrix();

    void SetType(CameraType type);
    void SetPosition(const DirectX::XMFLOAT3& position);
    void SetRotation(const DirectX::XMFLOAT3& rotation);
    void SetRotation(float pitch, float yaw, float roll);
    void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjMatrix() const;
    DirectX::XMMATRIX GetViewProjMatrix() const;
  private:
    CameraType type_;
    DirectX::XMFLOAT3 position_ = {0.0f, 0.0f, -5.0f};
    DirectX::XMFLOAT3 rotation_ = {0.0f, 0.0f, 0.0f};

    DirectX::XMFLOAT3 forward_ = {0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 right_ = {1.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 up_ = {0.0f, 1.0f, 0.0f};

    struct
    {
        float fovY_ = 45.f;
        float nearZ_ = 0.1f;
        float farZ_ = 100.0f;
        float rotationSpeed_ = 1.0f;
        float movementSpeed_ = 1.0f;
    } parameters;

    struct
    {
        DirectX::XMFLOAT4X4 view_;
        DirectX::XMFLOAT4X4 perspective_;
    } matrices;
};
} // namespace JEngine