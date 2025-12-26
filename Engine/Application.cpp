#include "pch.h"
#include "Application.h"
#include "Vertex.h"
#include "GeometryGenerator.h"
#include "Mesh.h"
#include "Model.h"

namespace JEngine {
Application::Application(HINSTANCE hinstance, std::wstring name)
    : window_(hinstance, name), context_(window_), swapChain_(context_), renderer_(context_, swapChain_) {
}

Application::~Application() {
    LogInfo("Application destructor - GPU is idle, cleaning up resources.");
}

void Application::Initialize() {
    InitSubsystems();
    InitCommandBuffers();
    InitFences();

    OnResize();

    InitScene();
}

void Application::Update(size_t frameIdx) {
    model_->Update(frameIdx);
}

int Application::Run() {
    MSG msg = {0};

    timer_.Reset();

    LogInfo("=== Entering Main Message Loop ===");
    while (msg.message != WM_QUIT) {
        // 메시지 처리
        if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        } else {
            timer_.Tick();

            // 애플리케이션이 활성 상태일 때만 업데이트 및 렌더링
            if (!window_.IsPaused()) {
                int frameIdx = swapChain_.GetCurrentBackBufferIndex();

                frameFence_[frameIdx].WaitForGPU();

                auto& cmdBuffer = commandBuffers_[frameIdx];
                auto* cmdList = cmdBuffer.BeginRecording();

                context_.SetViewport(cmdList);

                // TODO: Constant Buffer를 업데이트 단위로 분리하면 Application의 update와
                // renderer의 Update의 순서가 바뀔 예정
                renderer_.Update(timer_, *model_, frameIdx); // World/View/Proj Matrix 업데이트
                Update(frameIdx);                  // Constant Buffer에 데이터 복사

                renderer_.Draw(cmdList, *model_, frameIdx);

                cmdBuffer.EndRecording();

                // Command Queue에 제출
                context_.ExecuteCommands(cmdList);

                // 화면에 표시 (Swap Chain Present)
                swapChain_.Present();

                frameFence_[frameIdx].Signal();

                // 다른 CPU 작업 수행 가능
            } else {
                Sleep(100); // 비활성 상태에서는 CPU 사용량 감소를 위해 잠시 대기
                // TODO: GUI 추가되면 GUI 사용
            }
        }
    }

    return (int)msg.wParam;
}

void Application::OnResize() {
    // Resource에 변화를 주기 전에 GPU가 모든 작업을 완료하도록 대기
    for (Fence& fence : frameFence_)
        fence.WaitForGPU();

    swapChain_.Resize();
    renderer_.Resize();

    // Viewport 및 Scissor Rect 재설정
    context_.SetViewportConfig();

    LogInfo("Resize complete.");
}

void Application::InitSubsystems() {
    window_.Initialize();
    context_.Initialize();
    swapChain_.Initialize();
    renderer_.Initialize();
}

void Application::InitCommandBuffers() {
    const uint32_t bufferCount = swapChain_.GetBufferCount();
    LogInfo("Creating {} command buffers...", bufferCount);

    commandBuffers_ = context_.CreateGraphicsCommandBuffers(bufferCount);
}

void Application::InitFences() {
    const uint32_t bufferCount = swapChain_.GetBufferCount();
    frameFence_.reserve(bufferCount);

    for (uint32_t i = 0; i < bufferCount; ++i) {
        frameFence_.emplace_back(Fence(context_.GetDevice(), context_.GetCommandQueue()));
    }
}

void Application::InitScene() {
    int frameIdx = swapChain_.GetCurrentBackBufferIndex();
    auto& cmdBuffer = commandBuffers_[frameIdx];
    auto* cmdList = cmdBuffer.BeginRecording();

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    GeometryGenerator::CreateBox(vertices, indices);

    model_ = std::make_unique<Model>();
    model_->AddMesh(context_, cmdList, "Box", vertices, indices);

    cmdBuffer.EndRecording();
    context_.ExecuteCommands(cmdList);

    frameFence_[frameIdx].Signal();
    frameFence_[frameIdx].WaitForGPU();

    model_->ReleaseStagingBuffers();
}

} // namespace JEngine