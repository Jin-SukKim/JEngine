#include "pch.h"
#include "GPUBuffer.h"
#include "UploadBuffer.h"

namespace JEngine {

GPUBuffer::GPUBuffer(Context& ctx) : Buffer(ctx) {
}

GPUBuffer::~GPUBuffer() {
    Reset();
}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept : Buffer(std::move(other)) {
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept {
    if (this != &other) {
        Reset();
        Buffer::operator=(std::move(other));
    }
    return *this;
}

void GPUBuffer::CreateVertexBuffer(size_t count, size_t sizeOf) {
    LogInfo("Creating vertex buffer: {} vertices x {} bytes", count, sizeOf);

    // ✅ DEFAULT 힙에 COMMON 상태로 버퍼만 생성
    CreateBuffer(BufferType::VERTEX, count, sizeOf, D3D12_HEAP_TYPE_DEFAULT,
                 D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    LogInfo("Vertex buffer created at GPU address: 0x{:X}", GetGPUAddress());
}

void GPUBuffer::CreateIndexBuffer(size_t count, size_t sizeOf) {
    LogInfo("Creating index buffer: {} indices x {} bytes", count, sizeOf);

    // DEFAULT 힙에 생성 (COMMON 상태로)
    CreateBuffer(BufferType::INDEX, count, sizeOf, D3D12_HEAP_TYPE_DEFAULT,
                 D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    LogInfo("Index buffer created at GPU address: 0x{:X}", GetGPUAddress());
}

D3D12_VERTEX_BUFFER_VIEW GPUBuffer::CreateVertexBufferView(UINT stride) {
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = resource_->GetGPUVirtualAddress();
    vbv.StrideInBytes = stride;
    vbv.SizeInBytes = static_cast<UINT>(elementCount_ * elementByteSize_);

    LogInfo("Created VBV: Location=0x{:X}, Stride={}, Size={}",
            vbv.BufferLocation, vbv.StrideInBytes, vbv.SizeInBytes);

    return vbv;
}

D3D12_INDEX_BUFFER_VIEW GPUBuffer::CreateIndexBufferView(DXGI_FORMAT format) {
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = resource_->GetGPUVirtualAddress();
    ibv.Format = format;
    ibv.SizeInBytes = static_cast<UINT>(elementCount_ * elementByteSize_);

    LogInfo("Created IBV: Location=0x{:X}, Format={}, Size={}",
            ibv.BufferLocation, static_cast<int>(ibv.Format), ibv.SizeInBytes);

    return ibv;
}

} // namespace JEngine