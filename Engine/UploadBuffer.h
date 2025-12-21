#pragma once
#include "Buffer.h"
#include "Context.h"

namespace JEngine {

// Upload 버퍼 클래스 (CPU에서 GPU로 데이터를 업로드하는 용도)
class UploadBuffer : public Buffer
{
  public:
    UploadBuffer(Context& ctx);
    ~UploadBuffer() override;

    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;

    UploadBuffer(UploadBuffer&& other) noexcept;
    UploadBuffer& operator=(UploadBuffer&& other) noexcept;

    // 1번 데이터 복사용으로 사용되는 Staging Buffer
    void CreateStagingBuffer(size_t count, size_t sizeOf);

    // 매 프레임 업데이트되는 Constant Buffer
    void CreateConstantBuffer(size_t count, size_t sizeOf, DescriptorHandle handle);

    template <typename T>
    void Update(UINT elementIndex, const T& data);

    // 1D 데이터 복사
    void CopyDataToBuffer(ID3D12GraphicsCommandList* cmdList, Buffer& dstBuffer,
                          const void* initData);

    void Reset() override;

  private:
    void map();
    void unmap();
    static size_t calculateConstantBufferByteSize(size_t byteSize);

  private:
    BYTE* mappedData_ = nullptr;
};

template <typename T>
inline void UploadBuffer::Update(UINT elementIndex, const T& data) {
    if (!mappedData_) {
        LogError("Unmapped buffer cannot be updated");
        return;
    }

    if (elementIndex >= elementCount_) {
        LogError("Element index {} is out of range (Total Count: {})", elementIndex, elementCount_);
        return;
    }

    memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
}

} // namespace JEngine