#include "pch.h"
#include "Application.h"

namespace JEngine {
Application::Application(HINSTANCE hinstance, std::wstring name)
    : window_(hinstance, name), context_(window_), swapChain_(context_), renderer_(context_) {
}

Application::~Application() {
    context_.WaitForFence();
    LogInfo("Application destructor - GPU is idle, cleaning up resources.");
}

void Application::Initialize() {
    window_.Initialize();
    context_.Initialize();
    swapChain_.Initialize();
    renderer_.Initialize();

    commandBuffers_ = context_.CreateGraphicsCommandBuffers(swapChain_.GetBufferCount());

    OnResize();
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

                auto& cmdBuffer = commandBuffers_[frameIdx];
                auto* cmdList = cmdBuffer.BeginRecording();

                context_.SetViewport(cmdList);

                renderer_.Update(timer_);
                renderer_.Draw(cmdList, swapChain_.GetCurrentBackBuffer());

                cmdBuffer.EndRecording();

                // Command Queue에 제출
                context_.ExecuteCommands(cmdList);

                // 화면에 표시 (Swap Chain Present)
                swapChain_.Present();
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
    context_.WaitForFence();

    swapChain_.Resize();

    int frameIdx = swapChain_.GetCurrentBackBufferIndex();

    auto& cmdBuffer = commandBuffers_[frameIdx];
    auto* cmdList = cmdBuffer.BeginRecording();
    renderer_.Resize(cmdList);
    cmdBuffer.EndRecording();
    context_.ExecuteCommands(cmdList);

    // Viewport 및 Scissor Rect 재설정
    context_.SetViewportConfig();
    LogInfo("Viewport and Scissor Rect updated for new window size.");

    LogInfo("Resize handling complete.");
}
} // namespace JEngine