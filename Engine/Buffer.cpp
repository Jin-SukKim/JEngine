#include "pch.h"
#include "Buffer.h"

namespace JEngine {

Buffer::Buffer(Context& ctx) : Resource(ctx), type_(BufferType::CONSTANT) {
}

Buffer::~Buffer() {
    Reset();
}

Buffer::Buffer(Buffer&& other) noexcept
    : Resource(std::move(other)), 
      elementByteSize_(other.elementByteSize_), 
      elementCount_(other.elementCount_),  //  추가
      type_(other.type_) {
    other.elementByteSize_ = 0;
    other.elementCount_ = 0;  //  추가
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        Resource::operator=(std::move(other));
        elementByteSize_ = other.elementByteSize_;
        elementCount_ = other.elementCount_;  //  추가
        type_ = other.type_;

        other.Reset();
    }
    return *this;
}

size_t Buffer::GetSize() const {
    return elementByteSize_ * elementCount_;
}

BufferType Buffer::GetType() const {
    return type_;
}

void Buffer::Reset() {
    Resource::Reset();
    elementByteSize_ = 0;
    elementCount_ = 0;
}

void Buffer::CreateBuffer(BufferType type, size_t count, size_t sizeOf,
                          D3D12_HEAP_TYPE heapType,
                          D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState) {
    type_ = type;
    elementByteSize_ = sizeOf;
    elementCount_ = count;

    D3D12_HEAP_PROPERTIES heapProps = CreateHeapProperties(heapType);

    // Buffer는 기본적으로 1D, DXGI_FORMAT_UNKNOWN
    D3D12_RESOURCE_DESC desc =
        CreateResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, static_cast<UINT>(sizeOf * count),
                           1, /*height*/ 1, /*depth*/ 1, /*mipLevels*/
                           DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);

    CreateCommittedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, initialState);
    
    LogInfo("Buffer created: type={}, count={}, sizeOf={}, total={} bytes",
            static_cast<int>(type), count, sizeOf, count * sizeOf);
}

} // namespace JEngine