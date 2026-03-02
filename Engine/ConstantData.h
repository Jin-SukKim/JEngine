#pragma once

namespace JEngine {
    using namespace DirectX;

struct MeshConstants
{
    XMFLOAT4X4 world = XMFLOAT4X4();
    XMFLOAT4X4 invWorld= XMFLOAT4X4();
};

// TODO: 여러 Type의 light 지원
// 현재는 Directional light 만 지원
struct LightConstants
{
    XMFLOAT3 directionalLightDirection = XMFLOAT3();
    float padding1 = 0.f;
    XMFLOAT3 directionalLightColor = XMFLOAT3();
    float padding2 = 0.f;
    XMFLOAT4X4 lightViewProj = XMFLOAT4X4(); // Shadow mapping 용
};

struct SceneConstants
{
    XMFLOAT4X4 view = XMFLOAT4X4();
    XMFLOAT4X4 invView = XMFLOAT4X4();
    XMFLOAT4X4 proj = XMFLOAT4X4();
    XMFLOAT4X4 invProj = XMFLOAT4X4();
    XMFLOAT4X4 viewProj = XMFLOAT4X4();
    XMFLOAT4X4 invViewProj = XMFLOAT4X4();
    XMFLOAT3 eyeWorld = XMFLOAT3(); // Cameara 위치
    float padding1 = 0.f;

    //LightConstants light = LightConstants();
};


} // namespace JEngine
