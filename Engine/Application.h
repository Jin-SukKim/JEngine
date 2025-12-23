#pragma once

#include "Window.h"
#include "Context.h"
#include "Timer.h"
#include "SwapChain.h"
#include "Renderer.h"
#include "Fence.h"

namespace JEngine {

class Model;
class ShaderManager;
class RootSignature;
class Pipeline;

class Application
{
  public:
    Application(HINSTANCE hinstance, std::wstring name);
    ~Application();
    void Initialize();
    void Update(size_t frameIdx);

    int Run();

    void OnResize();

  private:
    void InitSubsystems();
    void InitCommandBuffers();
    void InitFences();
    void InitScene();

  private:
    Window window_;
    Context context_;
    Timer timer_;
    SwapChain swapChain_;
    Renderer renderer_;

    std::vector<CommandBuffer> commandBuffers_;
    std::vector<Fence> frameFence_;

    std::unique_ptr<Model> model_;
};
} // namespace JEngine