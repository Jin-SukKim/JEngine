#pragma once
#include "Resource.h"
#include "Context.h"    

namespace JEngine {

class UploadBuffer : public Resource
{
  public:
    UploadBuffer(Context& ctx);
    ~UploadBuffer();

    void CreateBufferView(UINT elementCount, UINT sizeOf, bool isConstantBuffer,
                          D3D12_CPU_DESCRIPTOR_HANDLE viewHanle);

    template <typename T>
    void UpdateData(UINT elementIndex, const T& data);
    UINT CalConstantBufferByteSize(UINT byteSize);
    void CopySubresourceData(ID3D12GraphicsCommandList* cmdList, const void* initData,
                             UINT rowPitch, UINT slicePitch, Resource& resource);
  private:
    BYTE* mappedData_ = nullptr;
    UINT elementByteSize_ = 0;
    bool isConstantBuffer_ = false;
};

template <typename T>
inline void UploadBuffer::UpdateData(UINT elementIndex, const T& data) {
    memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
}

} // namespace JEngine