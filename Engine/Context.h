#pragma once

#include "pch.h" // ComPtr 등이 정의된 pch.h 포함
#include "DescriptorPool.h"
#include "CommandBuffer.h"

namespace JEngine {

class Window;
/// Direct3D 12 렌더링 컨텍스트 관리 클래스
/// Device, Command Objects, Swap Chain, Descriptor Heaps 등을 관리
class Context
{
  public:
    Context(Window& window);
    ~Context();
    void Initialize();

    void createDevice();              // D3D12 Device 및 Factory 생성
    void createCommandObjects();      // Command Queue, Allocator, List 생성

    void ExecuteCommands(ID3D12GraphicsCommandList* cmd);

    //  Getter 함수들 
    IDXGIFactory6* GetDXGIFactory() const;

    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;
    Window& GetWindow();
    DescriptorPool* GetDescriptorPool();

    std::vector<CommandBuffer> CreateGraphicsCommandBuffers(uint32_t numBuffers);
    CommandBuffer CreateGraphicsCommandBuffer();
  private:
    Window& window_;
    //  Core D3D12 Objects 
    ComPtr<IDXGIFactory6> dxgiFactory_;         // DXGI Factory (Adapter, Swap Chain 생성용)
    ComPtr<ID3D12Device> device_;               // D3D12 Device (리소스 생성 및 관리)

    //  Command Objects (명령 기록 및 실행) 
    ComPtr<ID3D12CommandQueue> commandQueue_;         // GPU에 명령 제출용 큐

    std::unique_ptr<DescriptorPool> descriptorPool_;
};

} // namespace JEngine