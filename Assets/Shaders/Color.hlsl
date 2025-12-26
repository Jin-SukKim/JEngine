//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
    float4x4 world;
    float4x4 worldInv;
};

cbuffer cbPerObject : register(b1)
{
    float4x4 view;
    float4x4 invView;
    float4x4 projection;
    float4x4 invProj;
    float4x4 viewProj;
    float4x4 invViewProj;
    float3 eyeWorld; // Cameara À§Ä¡
    float padding1;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

// Vertex Shader
VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    vin.PosL = mul(float4(vin.PosL, 1.0f), world);
    vout.PosH = mul(float4(vin.PosL, 1.0f), viewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

// Pixel Shader
float4 PSMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}


