#pragma once

namespace JEngine {
class Timer;
class Resource;
class Context;

class Renderer
{
  public:
    Renderer(Context& ctx);

    void Initialize();
    void Update(const Timer& timer);
    void Draw(ID3D12GraphicsCommandList* cmdList, Resource& backBuffer);
    void Resize(ID3D12GraphicsCommandList* cmdList);
  private:
    Context& context_;
    std::unique_ptr<Resource> depthStencil_;
};

} // namespace JEngine