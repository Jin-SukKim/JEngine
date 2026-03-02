#pragma once
#include "Resource.h"
#include "Context.h"

namespace JEngine {

enum class BufferType { VERTEX, INDEX, CONSTANT, STAGING };

// Buffer 전용 클래스로 Vertex, Index, Constant Buffer 등을 관리
class Buffer : public Resource
{
  public:
    Buffer(Context& ctx);
    ~Buffer() override;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    size_t GetSize() const;
    BufferType GetType() const;

    virtual void Reset() override;

  protected:
    void CreateBuffer(BufferType type, size_t count, size_t sizeOf,
                      D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
                      D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                      D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ);

  protected:
    size_t elementCount_ = 0;
    size_t elementByteSize_ = 0;
    BufferType type_;
};
} // namespace JEngine
