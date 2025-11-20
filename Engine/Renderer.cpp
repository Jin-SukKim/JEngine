#include "pch.h"
#include "Renderer.h"
#include "Context.h"
#include "Resource.h"
#include "Window.h"
#include "DescriptorHeap.h"
#include "Vertex.h"
#include "Timer.h"

namespace JEngine {
Renderer::Renderer(Context& ctx) : context_(ctx), camera_(Camera::CameraType::LOOK_AT) {
}

void Renderer::Initialize() {
    // Depth Stencil 이미지 객체 생성
    depthStencil_ = std::make_unique<Resource>(context_);
    // Depth Stencil 버퍼 생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight(),
                                      context_.GetDescriptorPool()->AllocateDSV());

    world_ = Identity4x4();
}

void Renderer::Update(const Timer& timer) {
    using namespace DirectX;

    camera_.UpdateViewMatrix();

    // ⭐ World Matrix - 박스를 제자리에서 회전시킴
    // 경과 시간에 따라 회전 각도 계산 (라디안 단위)
    float rotationAngle = timer.TotalTime() * 0.5f; // 0.5는 회전 속도 (조절 가능)

    // Y축(수직 축)을 중심으로 회전
    XMMATRIX rotationY = XMMatrixRotationY(rotationAngle);

    // Z축을 중심으로 약간의 회전 추가 (더 흥미로운 효과)
    XMMATRIX rotationZ = XMMatrixRotationZ(rotationAngle * 0.3f);

    // 두 회전을 결합
    XMMATRIX world = rotationZ * rotationY;
    XMMATRIX worldViewProj = world * camera_.GetViewProjMatrix();

    XMStoreFloat4x4(&meshConst_.worldViewProj, XMMatrixTranspose(worldViewProj));
    constantBuffer_->UpdateData(0, meshConst_);
}

void Renderer::Draw(ID3D12GraphicsCommandList* cmdList, Resource& backBuffer) {
    // Back Buffer를 PRESENT → RENDER_TARGET 상태로 전환
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Render Target 및 Depth Stencil 설정
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBuffer.GetView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencil_->GetView();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 화면 클리어 (파란색 배경)
    cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   1.0f, 0, 0, nullptr);

    ID3D12DescriptorHeap* cbvHeap =
        context_.GetDescriptorPool()->Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap();
    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap};
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    //cmdList->SetPipelineState(mPSO.Get()); // Command List에서 Reset할때 설정해주고 있음
    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    // Store the views in variables before taking their address
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = 
        vertexBufferGPU_->VertexBufferView(vertexByteStride_, vertexBufferByteSize_);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = 
        indexBufferGPU_->IndexBufferView(indexFormat_, indexBufferByteSize_);

    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmdList->IASetIndexBuffer(&indexBufferView);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());
    cmdList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);

    // Back Buffer를 RENDER_TARGET → PRESENT 상태로 전환
    backBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PRESENT);
}

void Renderer::Resize() {
    depthStencil_->Reset();
    context_.GetDescriptorPool()->ResetDSV();
    // Depth Stencil 버퍼 재생성
    depthStencil_->CreateDepthStencil(context_.GetWindow().GetWidth(),
                                      context_.GetWindow().GetHeight(),
                                      context_.GetDescriptorPool()->AllocateDSV());

    camera_.SetPerspective(45.f, context_.GetWindow().GetAspectRatio(), 0.1f, 100.0f);
}

void Renderer::CreateConstantBuffer() {
    constantBuffer_ = std::make_unique<UploadBuffer>(context_);
    constantBuffer_->CreateBufferView(
        1, sizeof(MeshConst), true,
        context_.GetDescriptorPool()->AllocateCBV()); // 상수 버퍼로 생성

    LogInfo("Constant Buffer created successfully.");
}

void Renderer::SetInputLayout() {
    inputLayout_ = {
        // 위치 (Position) 속성
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        // 색상 (Color) 속성
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}
ComPtr<ID3DBlob> Renderer::CompileShader(const std::wstring& filePath,
                                         const std::string& entryPoint, const std::string& target) {
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> byteCode;
    ComPtr<ID3DBlob> errors;
    ThrowIfFailed(D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     entryPoint.c_str(), target.c_str(), compileFlags, 0, &byteCode,
                                     &errors));

    return byteCode;
}
void Renderer::BuildShaders() {
    // 예시: 정점 셰이더와 픽셀 셰이더 컴파일
    vertexShader_ = CompileShader(L"C:\\Study\\Project\\JEngine\\Assets\\Shaders\\Color.hlsl",
                                  "VSMain", "vs_5_0");
    pixelShader_ = CompileShader(L"C:\\Study\\Project\\JEngine\\Assets\\Shaders\\Color.hlsl",
                                 "PSMain", "ps_5_0");
    LogInfo("Shaders compiled successfully.");
}
void Renderer::InitBox(ID3D12GraphicsCommandList* cmdList) {
    using namespace DirectX;
    std::array<Vertex, 8> vertices = {
        Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
        Vertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
        Vertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
        Vertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
        Vertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),
        Vertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}),
        Vertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),
        Vertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)})};

    std::array<std::uint16_t, 36> indices = {// front face
                                             0, 1, 2, 0, 2, 3,
                                             // back face
                                             4, 6, 5, 4, 7, 6,
                                             // left face
                                             4, 5, 1, 4, 1, 0,
                                             // right face
                                             3, 2, 6, 3, 6, 7,
                                             // top face
                                             1, 5, 6, 1, 6, 2,
                                             // bottom face
                                             4, 0, 3, 4, 3, 7};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, vertexBufferCPU_.GetAddressOf()));
    memcpy(vertexBufferCPU_->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, indexBufferCPU_.GetAddressOf()));
    memcpy(indexBufferCPU_->GetBufferPointer(), indices.data(), ibByteSize);

    vertexBufferGPU_ = std::make_unique<Resource>(context_);
    vertexUploadBuffer_ = std::make_unique<UploadBuffer>(context_);
    vertexBufferGPU_->CreateBuffer(vbByteSize);
    // 수정: 8개의 정점이므로 elementCount를 8로 설정
    vertexUploadBuffer_->CreateBufferView(vertices.size(), sizeof(Vertex), false, {});
    vertexUploadBuffer_->CopySubresourceData(cmdList, vertices.data(), vbByteSize, vbByteSize,
                                             *vertexBufferGPU_);

    indexBufferGPU_ = std::make_unique<Resource>(context_);
    indexUploadBuffer_ = std::make_unique<UploadBuffer>(context_);
    indexBufferGPU_->CreateBuffer(ibByteSize);
    // 수정: 36개의 인덱스이므로 elementCount를 36으로 설정
    indexUploadBuffer_->CreateBufferView(indices.size(), sizeof(std::uint16_t), false, {});
    indexUploadBuffer_->CopySubresourceData(cmdList, indices.data(), ibByteSize, ibByteSize,
                                            *indexBufferGPU_);

    vertexByteStride_ = sizeof(Vertex);
    vertexBufferByteSize_ = vbByteSize;
    indexFormat_ = DXGI_FORMAT_R16_UINT;
    indexBufferByteSize_ = ibByteSize;
    indexCount_ = (UINT)indices.size();
    startIndexLocation_ = 0;
    baseVertexLocation_ = 0;
}

void Renderer::CreatePSO(DXGI_FORMAT backFormat) {
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL};
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS};
    depthStencilDesc.FrontFace = defaultStencilOp;
    depthStencilDesc.BackFace = defaultStencilOp;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {inputLayout_.data(), (UINT)inputLayout_.size()};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = {reinterpret_cast<BYTE*>(vertexShader_->GetBufferPointer()),
                  vertexShader_->GetBufferSize()};
    psoDesc.PS = {reinterpret_cast<BYTE*>(pixelShader_->GetBufferPointer()),
                  pixelShader_->GetBufferSize()};
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = backFormat;
    // MSAA 비활성화 (백 버퍼와 일치)
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = depthStencil_->GetFormat();
    ThrowIfFailed(context_.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
    
    LogInfo("Pipeline State Object created successfully.");
}

void Renderer::CreateRootSignature() {
    // Root Signature는 GPU와 CPU가 데이터를 주고받는 규칙(계약서)를 만드는 것

    // --- 1. 셰이더(GPU)가 받을 '슬롯' 정의 (Descriptor Range) ---
    // "셰이더가 데이터를 받을 슬롯(레지스터)은 이런 규칙을 가질 거야"
    D3D12_DESCRIPTOR_RANGE descriptorRange = {};

    // RangeType: "이 슬롯으로는 어떤 종류의 뷰(View)가 들어오지?"
    // -> "CBV (상수 버퍼 뷰) 타입이 들어올 거야."
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

    // NumDescriptors: "그런 뷰가 몇 개 필요하지?"
    // -> "1개만 필요해."
    descriptorRange.NumDescriptors = 1;

    // BaseShaderRegister: "그 뷰를 셰이더의 어떤 레지스터에 연결할까?"
    // -> "셰이더 코드(HLSL)의 0번 레지스터, 즉 'b0'에 연결할 거야."
    descriptorRange.BaseShaderRegister = 0; // b0

    // RegisterSpace: "레지스터 '공간' ID는? (특별한 경우 아니면 0)"
    descriptorRange.RegisterSpace = 0;

    // OffsetInDescriptorsFromTableStart: "테이블 내에서의 순서. 'APPEND'는 '그냥 순서대로'"
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // --- 2. C++(CPU)가 데이터를 '전달할 방식' 정의 (Root Parameter) ---
    // "C++ 코드는 이 '방식'을 통해 위에서 정의한 '슬롯'에 데이터를 전달할 거야"
    D3D12_ROOT_PARAMETER rootParameter = {};

    // ParameterType: "어떤 방식으로 데이터를 전달할까?"
    // -> "가장 표준적인 'Descriptor Table' 방식으로 전달할게."
    //    (의미: "내가 Descriptor Heap의 '시작 주소'를 알려줄게. 거기서 가져가.")
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    // ShaderVisibility: "이 데이터는 누가 볼 수 있지?"
    // -> "모든 셰이더 단계(버텍스, 픽셀 등)에서 다 볼 수 있어."
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // DescriptorTable: "어떤 '슬롯' 규칙(Range)을 이 방식(Parameter)에 연결할까?"
    // -> "위에서 만든 'descriptorRange' (b0에 CBV 1개) 규칙을 연결할게."
    rootParameter.DescriptorTable.NumDescriptorRanges = 1;
    rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRange;

    // --- 3. 위 모든 규칙을 모아 '최종 계약서' 완성 (Root Signature Description) ---
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

    // NumParameters: "이 계약서에는 총 몇 개의 '전달 방식(Parameter)'이 있지?"
    // -> "딱 1개 (위에서 만든 rootParameter)"
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParameter; // 그 1개가 바로 이것.

    // NumStaticSamplers/pStaticSamplers: "고정된 샘플러 설정. (지금은 안 씀)"
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;

    // Flags: "기타 설정. 'Input Assembler' (정점 버퍼) 접근은 허용해 줘."
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // --- 4. 계약서를 GPU가 읽을 수 있는 '기계어'로 변환 (Serialize) ---
    ComPtr<ID3DBlob> signature; // 변환된 '기계어(바이너리)'가 저장될 곳
    ComPtr<ID3DBlob> error;     // 만약 계약서에 오류가 있다면 여기에 저장될 곳

    // D3D12SerializeRootSignature: "C++ 구조체(rootSignatureDesc)를 -> 바이너리(signature)로!"
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              signature.GetAddressOf(), error.GetAddressOf()));

    // --- 5. 변환된 '기계어'를 GPU에 제출하여 실제 '객체' 생성 ---
    // CreateRootSignature: "이 바이너리 데이터로 실제 RootSignature 객체를 만들어줘."
    ThrowIfFailed(context_.GetDevice()->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_))); // 만들어진 객체는 여기에 저장

    LogInfo("Root Signature created successfully."); // "계약서(RootSignature) 생성 완료!"
}
} // namespace JEngine
