#include "pch.h"
#include "UploadBuffer.h"

namespace JEngine {

UploadBuffer::UploadBuffer(Context& ctx) : Resource(ctx) {
}

UploadBuffer::~UploadBuffer() {
    if (resource_ != nullptr) {
        resource_->Unmap(0, nullptr);
    }
    resource_ = nullptr;
}

void UploadBuffer::CreateBufferView(UINT elementCount, UINT sizeOf, bool isConstantBuffer,
                                              D3D12_CPU_DESCRIPTOR_HANDLE viewHanle) {
    elementByteSize_ = sizeOf;
    if (isConstantBuffer)
        elementByteSize_ = CalConstantBufferByteSize(sizeOf);

    D3D12_HEAP_PROPERTIES heapProps = SetHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

    D3D12_RESOURCE_DESC desc = SetResourceDesc(
        D3D12_RESOURCE_DIMENSION_BUFFER, elementByteSize_ * elementCount, 1, DXGI_FORMAT_UNKNOWN,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

    ThrowIfFailed(context_.GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(resource_.GetAddressOf())));

    if (isConstantBuffer) {
        // Buffer를 CPU가 접근할 수 있도록 매핑
        ThrowIfFailed(resource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_)));

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = resource_->GetGPUVirtualAddress();
        int offset = 0;
        gpuAddress += offset * elementByteSize_;

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = gpuAddress;
        cbvDesc.SizeInBytes = elementByteSize_;

        context_.GetDevice()->CreateConstantBufferView(&cbvDesc, viewHanle);
        viewHandle_ = viewHanle;
    }
}

UINT UploadBuffer::CalConstantBufferByteSize(UINT byteSize) {
    // 상수 버퍼는 256바이트 정렬이 필요
    return (byteSize + 255) & ~255;
}

void UploadBuffer::CopySubresourceData(ID3D12GraphicsCommandList* cmdList,
                                                 const void* initData, UINT rowPitch,
                                                 UINT slicePitch, Resource& resource) {
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = initData;
    subresourceData.RowPitch = rowPitch;
    subresourceData.SlicePitch = slicePitch;

    resource.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
    UINT numRows;
    UINT64 rowSizeInBytes;
    UINT64 totalBytes;

    D3D12_RESOURCE_DESC destDesc = resource.GetResourcePtr()->GetDesc();
    context_.GetDevice()->GetCopyableFootprints(&destDesc, 0, 1, 0, &layout, &numRows,
                                                &rowSizeInBytes, &totalBytes);

    // 중간 버퍼 데이터 복사
    BYTE* pData;
    ThrowIfFailed(resource_->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

    for (UINT z = 0; z < layout.Footprint.Depth; ++z) {
        BYTE* destSlice = pData + layout.Offset + z * layout.Footprint.RowPitch * numRows;
        const BYTE* srcSlice = reinterpret_cast<const BYTE*>(initData) + z * slicePitch;

        for (UINT y = 0; y < numRows; ++y) {
            memcpy(destSlice + y * layout.Footprint.RowPitch, srcSlice + y * rowPitch,
                   rowSizeInBytes);
        }
    }

    resource_->Unmap(0, nullptr);

    // GPU로 복사
    cmdList->CopyBufferRegion(resource.GetResourcePtr(), 0, resource_.Get(), 0, totalBytes);

    resource.TransitionTo(cmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
}
}