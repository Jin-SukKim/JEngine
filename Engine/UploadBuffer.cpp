#include "pch.h"
#include "UploadBuffer.h"

namespace JEngine {

UploadBuffer::UploadBuffer(Context& ctx) : Buffer(ctx) {
}

UploadBuffer::~UploadBuffer() {
    unmap();
    Reset();
}

UploadBuffer::UploadBuffer(UploadBuffer&& other) noexcept
    : Buffer(std::move(other)), mappedData_(other.mappedData_) {
    other.mappedData_ = nullptr;
}

UploadBuffer& UploadBuffer::operator=(UploadBuffer&& other) noexcept {
    if (this != &other) {
        // 기존 리소스 정리
        unmap();

        // 부모 클래스 이동
        Buffer::operator=(std::move(other));

        // mappedData_ 이동
        mappedData_ = other.mappedData_;
        other.mappedData_ = nullptr;
    }
    return *this;
}

void UploadBuffer::CreateStagingBuffer(size_t elementCount, size_t sizeOf) {
    // CPU-GPU 복사용으로 UPLOAD HEAP에 생성
    CreateBuffer(BufferType::STAGING, elementCount, sizeOf, D3D12_HEAP_TYPE_UPLOAD,
                 D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ);

    LogInfo("Staging buffer created and mapped: {} elements x {} bytes = {} total bytes",
            elementCount, sizeOf, elementCount * sizeOf);
}

void UploadBuffer::CreateConstantBuffer(size_t count, size_t sizeOf,
                                        D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
    // 상수 버퍼는 256byte 정렬이 필요
    sizeOf = calculateConstantBufferByteSize(sizeOf);

    CreateBuffer(BufferType::CONSTANT, count, sizeOf, D3D12_HEAP_TYPE_UPLOAD,
                 D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ);

    map();

    // CBV 생성
    // D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = GetGPUAddress();
    // int offset = 0;
    // gpuAddress += offset * sizeOf;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = GetGPUAddress();
    cbvDesc.SizeInBytes = static_cast<UINT>(sizeOf);

    context_.GetDevice()->CreateConstantBufferView(&cbvDesc, viewHandle);
    SetViewHandle(viewHandle);

    LogInfo("Constant buffer created: {} elements x {} bytes (aligned)", count, sizeOf);
}

void UploadBuffer::CopyDataToBuffer(ID3D12GraphicsCommandList* cmdList, Buffer& dstBuffer,
                                    const void* initData) {
    if (!initData) {
        LogError("Source data is null! Cannot copy data.");
        return;
    }

    D3D12_RESOURCE_STATES prevState = dstBuffer.GetCurrentState();
    const size_t srcSize = GetSize();
    const size_t dstSize = dstBuffer.GetSize();

    if (srcSize != dstSize) {
        LogError("Size mismatch: src={} bytes, dst={} bytes", srcSize, dstSize);
        return;
    }

    LogInfo("Starting buffer copy: {} bytes", srcSize);

    // 복사 직전에 매핑
    map();
    
    if (!mappedData_) {
        LogError("Failed to map staging buffer!");
        return;
    }

    // Destination Buffer 상태 전환
    dstBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    // CPU 메모리에서 Staging Buffer로 복사
    memcpy(mappedData_, initData, srcSize);


    // Staging Buffer → GPU Buffer 복사
    cmdList->CopyBufferRegion(dstBuffer.GetResourcePtr(), // 대상
                              0,                          // 대상 오프셋
                              resource_.Get(),            // 소스 (staging buffer)
                              0,                          // 소스 오프셋
                              srcSize);                   // 복사 크기

    // 사용 직후 unmap
    unmap();

    // 최종 상태로 전환
    dstBuffer.TransitionTo(cmdList, prevState);

    LogInfo("Buffer copy completed: {} bytes to state {}", srcSize, static_cast<int>(prevState));
}

void UploadBuffer::Reset() {
    unmap();
    Buffer::Reset();
    mappedData_ = nullptr;
}

void UploadBuffer::map() {
    if (resource_ && !mappedData_) {
        // GPU 리소스를 CPU가 접근할 수 있도록 매핑
        ThrowIfFailed(resource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_)));
        LogInfo("Buffer mapped: address=0x{:X}", reinterpret_cast<uintptr_t>(mappedData_));
    }
}

void UploadBuffer::unmap() {
    if (resource_ && mappedData_) {
        resource_->Unmap(0, nullptr);
        LogInfo("Buffer unmapped");
        mappedData_ = nullptr;
    }
}

size_t UploadBuffer::calculateConstantBufferByteSize(size_t byteSize) {
    // 상수 버퍼의 크기를 256byte 단위로 정렬
    /*
        아래 방법을 Bit 연산으로 최적화한 256byte 정렬
        if (byteSize % 256 == 0) return byteSize;
        return (byteSize / 256 + 1) * 256;
    */
    return (byteSize + 255) & ~255;
}

} // namespace JEngine