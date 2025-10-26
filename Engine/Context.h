#pragma once

#include "pch.h" // ComPtr 등이 정의된 pch.h 포함

namespace JEngine {

/// <summary>
/// Direct3D 12 렌더링 컨텍스트 관리 클래스
/// Device, Command Objects, Swap Chain, Descriptor Heaps 등을 관리
/// </summary>
class Context
{
  public:
    // === 초기화 함수들 ===
    void createDevice();              // D3D12 Device 및 Factory 생성
    void createCommandObjects();      // Command Queue, Allocator, List 생성
    void createSwapChain();           // Swap Chain 생성 (화면 출력용)
    void createDescriptorHeaps();     // RTV, DSV Descriptor Heap 생성
    void createRenderTargetViews();   // Back Buffer용 Render Target View 생성
    void createDepthStencilView();    // Depth Stencil Buffer 및 View 생성

    void TransitionDepthStencilState(); // Depth Stencil 버퍼 상태 전환

    // === Getter 함수들 ===
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;  // 현재 Back Buffer의 RTV 핸들
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;       // Depth Stencil View 핸들
    ID3D12Resource* GetCurrentBackBuffer() const;                  // 현재 Back Buffer 리소스

    // === 렌더링 설정 ===
    void SetViewport();  // Viewport 및 Scissor Rect 설정

  private:
    // === Core D3D12 Objects ===
    ComPtr<IDXGIFactory6> dxgiFactory_;         // DXGI Factory (Adapter, Swap Chain 생성용)
    ComPtr<ID3D12Device> device_;               // D3D12 Device (리소스 생성 및 관리)
    ComPtr<IDXGISwapChain4> swapChain_;         // Swap Chain (화면 출력용 Back Buffer 관리)

    // === Synchronization ===
    ComPtr<ID3D12Fence> fence_;                 // CPU-GPU 동기화용 Fence

    // === Descriptor Sizes (GPU 종속적) ===
    UINT rtvDescriptorSize_ = 0;                // Render Target View Descriptor 크기
    UINT dsvDescriptorSize_ = 0;                // Depth Stencil View Descriptor 크기
    UINT cbvSrvUavDescriptorSize_ = 0;          // CBV/SRV/UAV Descriptor 크기

    // === Format 설정 ===
    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;      // Back Buffer 포맷 (8비트 RGBA)
    DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth(24비트) + Stencil(8비트)

    // === Buffer 관리 ===
    static constexpr int bufferCount_ = 2;      // Swap Chain Buffer 개수 (Double Buffering)
    int curBackBufferIdx_ = 0;                  // 현재 사용 중인 Back Buffer 인덱스 (0 or 1)

    // === Multi-Sampling Anti-Aliasing (MSAA) ===
    UINT m4xMsaaQuality_ = 1;                   // 4x MSAA 품질 수준 (0 = 미지원)

    // === Command Objects (명령 기록 및 실행) ===
    ComPtr<ID3D12CommandQueue> commandQueue_;         // GPU에 명령 제출용 큐
    ComPtr<ID3D12CommandAllocator> commandAllocator_; // Command List 메모리 할당자
    ComPtr<ID3D12GraphicsCommandList> commandList_;   // 렌더링 명령 기록용 리스트

    // === Window 설정 ===
    UINT screenWidth_ = 1280;                   // 화면 너비
    UINT screenHeight_ = 720;                   // 화면 높이
    HWND mainWnd_ = nullptr;                    // 윈도우 핸들 (나중에 설정)

    // === Viewport and Scissor Rect ===
    D3D12_VIEWPORT screenViewport_;             // 렌더링 영역 (화면 전체)
    D3D12_RECT scissorRect_;                    // 잘라낼 영역 (일반적으로 화면 전체)

    // === Descriptor Heaps (View들을 담는 배열 컨테이너) ===
    // Descriptor Heap = View(Descriptor)들을 저장하는 "배열" 또는 "상자"
    // 예: rtvHeap_ = [ RTV_0 | RTV_1 | RTV_2 | ... ]
    //                   ↑       ↑       ↑
    //                 View    View    View
    // 
    // - DirectX 11: View가 자동으로 관리됨 (개발자가 신경 쓸 필요 없음)
    // - DirectX 12: Heap을 명시적으로 생성하고 관리 (성능 최적화를 위해)
    // 
    // GPU가 빠르게 접근할 수 있도록 연속된 메모리 공간에 View들을 저장
    // Vulkan의 Descriptor Pool과 유사하지만, D3D12는 배열처럼 직접 접근 가능
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;      // Render Target View들을 담는 Heap
    ComPtr<ID3D12DescriptorHeap> dsvHeap_;      // Depth Stencil View들을 담는 Heap

    // === Resources (GPU 메모리 리소스) ===
    ComPtr<ID3D12Resource> backBuffers_[bufferCount_];  // Swap Chain의 Back Buffer들
    ComPtr<ID3D12Resource> depthStencilBuffer_;         // Depth Stencil Buffer (깊이 테스트용)
};

} // namespace JEngine