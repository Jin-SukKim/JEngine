#pragma once

#include "Window.h"
#include "Context.h"
#include "Timer.h"
#include "SwapChain.h"
#include "Renderer.h"
#include "Fence.h"

namespace JEngine {
class Application
{
  public:
    Application(HINSTANCE hinstance, std::wstring name);
    ~Application();
    void Initialize();
    int Run();

    void OnResize();

  private:
    Window window_;
    Context context_;
    Timer timer_;
    SwapChain swapChain_;
    Renderer renderer_;

    std::vector<CommandBuffer> commandBuffers_;
    std::vector<Fence> frameFence_;
};
} // namespace JEngine