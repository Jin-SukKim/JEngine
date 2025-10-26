#pragma once

#include "pch.h" // ComPtr 등이 정의된 pch.h 포함

namespace JEngine {

class Context
{
  public:
    void createDevice();
    void createCommandObjects();
    void createSwapChain();
    void createDescriptorHeaps();

    void createRenderTargetViews();
    void createDepthStencilView();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    ID3D12Resource* GetCurrentBackBuffer() const;

    void SetViewport();

  private:
    ComPtr<IDXGIFactory6> dxgiFactory_;
    ComPtr<ID3D12Device> device_;       // D3D12 하드웨어 디바이스
    ComPtr<IDXGISwapChain4> swapChain_;

    ComPtr<ID3D12Fence> fence_; // CPU-GPU 동기화용 Fence

    UINT rtvDescriptorSize_ = 0; // Render Target View Descriptor 크기
    UINT dsvDescriptorSize_ = 0; // Depth Stencil View Descriptor 크기
    UINT cbvSrvUavDescriptorSize_ = 0; // Constant Buffer View / Shader Resource View / Unordered Access View Descriptor 크기
    
    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static constexpr int bufferCount_ = 2; // 이중 버퍼링
    int curBackBufferIdx_ = 0; // 현재 Back Buffer Index

    UINT m4xMsaaQuality_ = 1; // 4x MSAA 품질 수준

    // Command Objects
    ComPtr<ID3D12CommandQueue> commandQueue_; // 명령 대기열
    ComPtr<ID3D12CommandAllocator> commandAllocator_; // Command List의 메모리 할당자
    ComPtr<ID3D12GraphicsCommandList> commandList_;   // 명령 리스트

   // Window
    UINT screenWidth_ = 1280;
    UINT screenHeight_ = 720;
    HWND mainWnd_ = nullptr;

    // Viewport and Scissor Rect
    D3D12_VIEWPORT screenViewport_;
    D3D12_RECT scissorRect_;

    // Descriptor Heaps
    ComPtr<ID3D12DescriptorHeap> rtvHeap_; // Render Target View Descriptor
    ComPtr<ID3D12DescriptorHeap> dsvHeap_; // Depth Stencil View Descriptor

    // Resources
    ComPtr<ID3D12Resource> backBuffers_[bufferCount_]; // Swap Chain의 Back Buffer들
    ComPtr<ID3D12Resource> depthStencilBuffer_;        // Depth Stencil Buffer
};

} // namespace JEngine