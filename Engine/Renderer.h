#pragma once

namespace JEngine {
class Timer;
class Image2D;
class Context;

class Renderer
{
  public:
    Renderer(Context& ctx);

    void Initialize();
    void Update(const Timer& timer);
    void Draw(ID3D12GraphicsCommandList* cmdList, Image2D& backBuffer);
    void Resize();
  private:
    Context& context_;
    std::unique_ptr<Image2D> depthStencil_;
};

} // namespace JEngine