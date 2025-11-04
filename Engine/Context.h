#pragma once

#include "pch.h" // ComPtr 등이 정의된 pch.h 포함
#include "DescriptorHeap.h"
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

    void WaitForFence();
    void ExecuteCommands(ID3D12GraphicsCommandList* cmd);

    // === Getter 함수들 ===
    ComPtr<IDXGIFactory6> GetDXGIFactory() const;
    ComPtr<ID3D12Device> GetDevice() const;
    ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
    Window& GetWindow();
    DescriptorHeap& GetDescriptorHeaps();

    // === 렌더링 설정 ===
    void SetViewportConfig();  // Viewport 및 Scissor Rect 설정
    void SetViewport(ID3D12GraphicsCommandList* cmd);

    std::vector<CommandBuffer> CreateGraphicsCommandBuffers(uint32_t numBuffers);
    CommandBuffer CreateGraphicsCommandBuffer();
  private:
    Window& window_;
    // === Core D3D12 Objects ===
    ComPtr<IDXGIFactory6> dxgiFactory_;         // DXGI Factory (Adapter, Swap Chain 생성용)
    ComPtr<ID3D12Device> device_;               // D3D12 Device (리소스 생성 및 관리)
   
    // === Synchronization ===
    ComPtr<ID3D12Fence> fence_;                 // CPU-GPU 동기화용 Fence
    UINT currentFence_ = 0;   // 현재 Fence 값 추적

    // === Command Objects (명령 기록 및 실행) ===
    ComPtr<ID3D12CommandQueue> commandQueue_;         // GPU에 명령 제출용 큐

    // === Viewport and Scissor Rect ===
    D3D12_VIEWPORT screenViewport_;             // 렌더링 영역 (화면 전체)
    D3D12_RECT scissorRect_;                    // 잘라낼 영역 (일반적으로 화면 전체)

    DescriptorHeap descriptorHeaps_;
};

} // namespace JEngine