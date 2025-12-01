#pragma once
#include "Buffer.h"

namespace JEngine {

// GPU 전용 버퍼 클래스
class GPUBuffer : public Buffer
{
  public:
    GPUBuffer(Context& ctx);
    ~GPUBuffer() override;

    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    GPUBuffer(GPUBuffer&& other) noexcept;
    GPUBuffer& operator=(GPUBuffer&& other) noexcept;

    void CreateVertexBuffer(size_t count, size_t sizeOf);
    void CreateIndexBuffer(size_t count, size_t sizeOf);

    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(UINT stride);
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(DXGI_FORMAT format);
};
} // namespace JEngine