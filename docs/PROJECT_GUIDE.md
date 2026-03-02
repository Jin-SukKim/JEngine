# JEngine 프로젝트 종합 가이드

> 이 문서는 JEngine 코드베이스를 처음 접하는 개발자가 프로젝트 구조와 핵심 개념을 빠르게 파악할 수 있도록 작성되었습니다.

---

## 목차

1. [개요](#1-개요)
2. [아키텍처 & 기술 스택](#2-아키텍처--기술-스택)
3. [디렉토리 구조](#3-디렉토리-구조)
4. [핵심 컴포넌트](#4-핵심-컴포넌트)
   - [4.1 애플리케이션 계층](#41-애플리케이션-계층)
   - [4.2 D3D12 기초 인프라](#42-d3d12-기초-인프라)
   - [4.3 리소스 파이프라인](#43-리소스-파이프라인)
   - [4.4 디스크립터 관리](#44-디스크립터-관리)
   - [4.5 바인딩 & 셰이더](#45-바인딩--셰이더)
   - [4.6 장면 & 지오메트리](#46-장면--지오메트리)
   - [4.7 렌더링 오케스트레이션](#47-렌더링-오케스트레이션)
   - [4.8 전체 렌더링 루프 흐름도](#48-전체-렌더링-루프-흐름도)
5. [시작하기](#5-시작하기)

---

## 1. 개요

### JEngine이란?

JEngine은 **DirectX 12** 기반의 실시간 3D 렌더링 엔진입니다. **C++20**과 **Win32 API**를 활용하여 Windows 환경에서 동작하며, GPU 리소스 관리, 셰이더 파이프라인, 장면 렌더링 등 3D 그래픽스의 핵심 요소를 직접 구현합니다.

### 기술 스택 요약

| 항목 | 기술 |
|------|------|
| 언어 | C++20 |
| 그래픽 API | DirectX 12 |
| 윈도우 시스템 | Win32 API |
| 셰이더 | HLSL (Shader Model 5.1) |
| 패키지 관리 | vcpkg |
| 빌드 시스템 | Visual Studio 2022 (v143 toolset) |
| 수학 라이브러리 | DirectXMath (SIMD) |

### 현재 상태

- 기본 렌더링 파이프라인 구축 완료 (정점 색상 기반 렌더링)
- 멀티프레임 GPU 동기화 (트리플 버퍼링) 구현
- 프로시저럴 지오메트리 생성 (Box) 지원
- 카메라 시스템 (LOOK_AT / FIRST_PERSON 모드) 구현
- 텍스처 로딩 (DDS, 일반 이미지) 지원
- ImGui 디버그 UI 통합 준비 (vcpkg에 포함)

---

## 2. 아키텍처 & 기술 스택

### 3-Layer 아키텍처

```
┌──────────────────────────────────────────┐
│              Application                 │  ← 실행파일 (.exe)
│         (main.cpp, 진입점)               │     메인 루프, 윈도우 관리
├──────────────────────────────────────────┤
│               Engine                     │  ← 정적 라이브러리 (.lib)
│    (렌더링, 리소스, 셰이더, 장면)         │     모든 렌더링 로직
├──────────────────────────────────────────┤
│             DirectX 12                   │  ← 시스템 API
│       (GPU 드라이버, DXGI)               │     하드웨어 추상화
└──────────────────────────────────────────┘
```

Application은 Engine 정적 라이브러리에 링크하여, Engine이 제공하는 클래스들을 사용해 렌더링 루프를 실행합니다. Engine은 내부적으로 DirectX 12 API를 호출하여 GPU와 통신합니다.

### 외부 의존성

모든 외부 라이브러리는 `vcpkg.json`을 통해 관리됩니다.

| 패키지 | 용도 |
|--------|------|
| `assimp` | 3D 모델 파일 로딩 (FBX, OBJ 등) |
| `stb` | 이미지 파일 로딩 (PNG, JPG 등) |
| `directxtk12` | DirectX 12 유틸리티 (SimpleMath, DDSTextureLoader 등) |
| `directxmath` | SIMD 기반 벡터/행렬 수학 연산 |
| `directxmesh` | 메시 처리 유틸리티 |
| `directxtex` | 텍스처 로딩 (DDS, OpenEXR 포맷 지원) |
| `imgui` | 디버그 UI (DX12 + Win32 백엔드) |

### 코딩 컨벤션

| 규칙 | 예시 |
|------|------|
| 네임스페이스 | 모든 엔진 코드는 `namespace JEngine` 안에 위치 |
| Private 멤버 | 후행 언더스코어 사용: `device_`, `resource_` |
| COM 객체 | `ComPtr<T>`로 자동 수명 관리 |
| 힙 리소스 | `std::unique_ptr<T>`로 소유권 관리 |
| 복사 의미론 | 대부분 리소스 클래스는 복사 금지, 이동만 허용 |
| 포매팅 | `.clang-format`: LLVM 기반, 4칸 들여쓰기, 100칸 라인 제한 |
| 커밋 스타일 | Conventional Commits: `feat(Renderer): SceneConstant 추가` |

---

## 3. 디렉토리 구조

```
JEngine/
├── JEngine.sln                  # Visual Studio 솔루션 파일
├── vcpkg.json                   # 외부 의존성 선언
├── .clang-format                # 코드 포매팅 규칙
├── CLAUDE.md                    # AI 어시스턴트 설정
│
├── Application/                 # 실행파일 프로젝트
│   ├── Application.vcxproj      #   프로젝트 설정
│   └── main.cpp                 #   프로그램 진입점 (WinMain)
│
├── Engine/                      # 정적 라이브러리 프로젝트 (핵심 엔진)
│   ├── Engine.vcxproj           #   프로젝트 설정
│   ├── pch.h / pch.cpp          #   프리컴파일 헤더
│   │
│   │── Application.h / .cpp     #   메인 루프, 프레임 관리
│   │── Window.h / .cpp          #   Win32 윈도우 생성 및 메시지 처리
│   │── Timer.h / .cpp           #   프레임 타이밍 (DeltaTime, FPS)
│   │── Logger.h / .cpp          #   로깅 시스템 (std::format 기반)
│   │
│   │── Context.h / .cpp         #   D3D12 디바이스, 커맨드 큐, 디스크립터 풀
│   │── CommandBuffer.h / .cpp   #   커맨드 할당자 + 커맨드 리스트
│   │── Fence.h / .cpp           #   CPU-GPU 동기화
│   │── BarrierHelper.h / .cpp   #   리소스 상태 전이 헬퍼
│   │
│   │── Resource.h / .cpp        #   GPU 리소스 기본 클래스
│   │── Buffer.h / .cpp          #   버퍼 기본 클래스
│   │── GPUBuffer.h / .cpp       #   정점/인덱스 버퍼 (GPU 전용)
│   │── UploadBuffer.h / .cpp    #   상수/스테이징 버퍼 (CPU 접근 가능)
│   │── Texture.h / .cpp         #   렌더 타겟, 깊이 스텐실, 파일 텍스처
│   │── Sampler.h / .cpp         #   텍스처 샘플링 설정
│   │
│   │── DescriptorPool.h / .cpp  #   디스크립터 풀 (타입별 할당)
│   │── DescriptorHeap.h / .cpp  #   디스크립터 힙 (실제 메모리)
│   │
│   │── RootSignature.h / .cpp   #   GPU-CPU 바인딩 규약
│   │── Shader.h / .cpp          #   셰이더 컴파일 (HLSL → 바이트코드)
│   │── ShaderManager.h / .cpp   #   셰이더 관리 및 파이프라인 셰이더 조합
│   │── Pipeline.h / .cpp        #   PSO(Pipeline State Object) 생성
│   │
│   │── Model.h / .cpp           #   모델 (메시 컨테이너 + 월드 변환)
│   │── Mesh.h / .cpp            #   메시 (정점/인덱스 버퍼)
│   │── Vertex.h / .cpp          #   정점 구조체 정의
│   │── Camera.h / .cpp          #   카메라 (뷰/투영 행렬)
│   │── ConstantData.h           #   GPU 상수 버퍼 구조체
│   │── GeometryGenerator.h/.cpp #   프로시저럴 지오메트리 유틸리티
│   └── Utils.h / .cpp           #   기타 유틸리티
│
├── Assets/                      # 에셋 프로젝트
│   └── Shaders/
│       └── Color.hlsl           #   정점 색상 셰이더 (VS + PS)
│
└── extern/                      # 참고 코드
```

### 솔루션 프로젝트

| 프로젝트 | 타입 | 설명 |
|----------|------|------|
| `Engine` | 정적 라이브러리 (.lib) | 모든 렌더링 및 엔진 로직 |
| `Application` | 실행파일 (.exe) | 진입점, Engine에 의존 |
| `Assets` | 유틸리티 | 셰이더 파일 관리 |

---

## 4. 핵심 컴포넌트

JEngine의 클래스들은 7개의 서브시스템으로 분류됩니다.

### 4.1 애플리케이션 계층

프로그램의 생명주기를 관리하는 최상위 계층입니다.

#### Application (`Engine/Application.h`)

메인 루프를 실행하고, 모든 서브시스템을 초기화/조율하는 중심 클래스입니다.

```cpp
class Application {
public:
    Application(HINSTANCE hinstance, std::wstring name);
    void Initialize();              // 모든 서브시스템 초기화
    void Update(size_t frameIdx);   // 프레임별 업데이트
    int Run();                      // 메인 루프 실행

private:
    Window window_;
    Context context_;
    Timer timer_;
    SwapChain swapChain_;
    Renderer renderer_;
    std::vector<CommandBuffer> commandBuffers_;   // 프레임별 커맨드 버퍼
    std::vector<Fence> frameFence_;               // 프레임별 펜스
    std::unique_ptr<Model> model_;
};
```

**관계:** Window, Context, SwapChain, Renderer, CommandBuffer, Fence, Model을 소유하며 이들의 생명주기를 관리합니다.

#### Window (`Engine/Window.h`)

Win32 윈도우를 생성하고 메시지를 처리합니다. 정적 싱글톤 패턴으로 메시지 프로시저에서 접근 가능합니다.

```cpp
class Window {
public:
    void Initialize();              // RegisterClass + CreateWindowEx
    HWND GetHwnd() const;
    UINT GetWidth() const;          // 기본값: 1280
    UINT GetHeight() const;         // 기본값: 720
    float GetAspectRatio() const;
    bool IsPaused() const;
    static Window* GetWindow();     // 정적 싱글톤 접근

private:
    static Window* window_;         // 싱글톤 포인터
    HWND mainWnd_;
    UINT screenWidth_ = 1280;
    UINT screenHeight_ = 720;
};
```

#### Timer (`Engine/Timer.h`)

고정밀 타이머로 프레임 시간과 FPS를 계산합니다. `QueryPerformanceCounter` 기반입니다.

```cpp
class Timer {
public:
    float TotalTime() const;    // 총 경과 시간 (초)
    float DeltaTime() const;    // 프레임 간 시간 (초)
    float FrameRate() const;    // FPS
    void Reset();               // 메시지 루프 시작 전 호출
    void Tick();                // 매 프레임 호출
    void Start();               // 일시정지 해제 시
    void Stop();                // 일시정지 시
};
```

#### Logger (`Engine/Logger.h`)

C++20 `std::format` 기반의 로깅 시스템입니다. 싱글톤 패턴으로 어디서든 접근 가능합니다.

```cpp
// 사용 예시
LogInfo("프레임 레이트: {:.1f} FPS", timer.FrameRate());
LogError("디바이스 생성 실패: HRESULT {:#x}", hr);
LogWarning("셰이더 파일을 찾을 수 없습니다: {}", filename);

// DirectX HRESULT 에러 처리
ThrowIfFailed(device->CreateCommittedResource(...));
// 실패 시 std::source_location으로 파일명:줄번호 자동 포함
```

---

### 4.2 D3D12 기초 인프라

DirectX 12의 핵심 객체들을 관리하는 저수준 인프라입니다.

#### Context (`Engine/Context.h`)

D3D12 디바이스, 커맨드 큐, 디스크립터 풀 등 핵심 GPU 객체의 중앙 관리자입니다. 대부분의 클래스가 `Context&`를 참조합니다.

```cpp
class Context {
public:
    Context(Window& window);
    void Initialize();

    // 핵심 Getter
    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;
    IDXGIFactory6* GetDXGIFactory() const;
    DescriptorPool* GetDescriptorPool();

    // 커맨드 실행
    void ExecuteCommands(ID3D12GraphicsCommandList* cmd);
    std::vector<CommandBuffer> CreateGraphicsCommandBuffers(uint32_t numBuffers);

    // 뷰포트
    void SetViewportConfig();
    void SetViewport(ID3D12GraphicsCommandList* cmd);

private:
    void createDevice();            // DXGIFactory + D3D12Device 생성
    void createCommandObjects();    // CommandQueue 생성

    Window& window_;
    ComPtr<IDXGIFactory6> dxgiFactory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    D3D12_VIEWPORT screenViewport_;
    D3D12_RECT scissorRect_;
    std::unique_ptr<DescriptorPool> descriptorPool_;
};
```

**관계:** Window에 의존하며, DescriptorPool을 소유합니다. 거의 모든 GPU 관련 클래스가 Context를 참조합니다.

#### CommandBuffer (`Engine/CommandBuffer.h`)

GPU 커맨드 기록을 위한 커맨드 할당자(Allocator)와 커맨드 리스트(List)를 캡슐화합니다.

```cpp
class CommandBuffer {
public:
    CommandBuffer(ID3D12Device* device);

    // 커맨드 기록 시작: 할당자를 리셋하고 리스트를 열어 반환
    ID3D12GraphicsCommandList* BeginRecording(ID3D12PipelineState* pso = nullptr);

    // 커맨드 기록 종료: 리스트를 닫음
    void EndRecording();

private:
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
};
```

프레임마다 하나의 CommandBuffer를 사용하여, GPU가 이전 프레임의 커맨드를 실행하는 동안 다음 프레임의 커맨드를 기록할 수 있습니다.

#### Fence (`Engine/Fence.h`)

CPU와 GPU 간의 동기화를 담당합니다. 각 프레임마다 독립적인 Fence를 사용합니다.

```cpp
class Fence {
public:
    Fence(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

    void Signal();       // GPU에 신호를 보냄 (현재 프레임 완료 표시)
    void WaitForGPU();   // GPU가 특정 Fence 값에 도달할 때까지 대기

private:
    ComPtr<ID3D12Fence> fence_;
    UINT64 fenceValue_;
    HANDLE fenceEvent_;    // Win32 이벤트 핸들
};
```

**동기화 패턴:**
```
Frame N-1:  Signal()  ─────┐
                            │  (GPU가 N-1 프레임 작업 완료할 때까지)
Frame N:    WaitForGPU() ──┘
            BeginRecording()
            ... 렌더링 커맨드 ...
            EndRecording()
            ExecuteCommands()
            Present()
            Signal()
```

#### BarrierHelper (`Engine/BarrierHelper.h`)

GPU 리소스의 상태 전이(Transition)를 추적하고 배리어를 삽입합니다.

```cpp
class BarrierHelper {
public:
    void Transition(ID3D12GraphicsCommandList* cmdList,
                    ID3D12Resource* resource,
                    D3D12_RESOURCE_STATES newState);
    D3D12_RESOURCE_STATES GetState() const;
    void SetInitialState(D3D12_RESOURCE_STATES newState);

private:
    D3D12_RESOURCE_STATES state_ = D3D12_RESOURCE_STATE_COMMON;
};
```

예를 들어 백버퍼를 렌더 타겟으로 사용하려면 `PRESENT → RENDER_TARGET`으로, 화면에 표시하려면 `RENDER_TARGET → PRESENT`로 상태를 전이해야 합니다. BarrierHelper는 현재 상태를 기억하고 필요한 배리어를 자동으로 생성합니다.

---

### 4.3 리소스 파이프라인

GPU 리소스(버퍼, 텍스처)의 생성과 관리를 담당합니다.

#### 리소스 계층 구조

```
Resource (GPU 리소스 기본 클래스)
├── Buffer (버퍼 기본 클래스)
│   ├── GPUBuffer    ← 정점/인덱스 버퍼 (GPU 전용 메모리)
│   └── UploadBuffer ← 상수/스테이징 버퍼 (CPU 접근 가능)
└── Texture          ← 렌더 타겟, 깊이 스텐실, 파일 텍스처
```

#### Resource (`Engine/Resource.h`)

모든 GPU 리소스의 기본 클래스입니다. `ID3D12Resource`를 래핑하고, 디스크립터 핸들 및 상태 전이를 관리합니다.

```cpp
class Resource {
public:
    Resource(Context& ctx);
    virtual void Reset();

    // 리소스 상태 전이 (예: RENDER_TARGET → PRESENT)
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

    // Getter
    ID3D12Resource* GetResource();
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t index = 0) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t index = 0) const;

protected:
    Context& context_;
    ComPtr<ID3D12Resource> resource_;
    std::vector<DescriptorHandle> descriptorHandles_;
    BarrierHelper barrierHelper_;

    // 리소스 생성 헬퍼
    void CreateCommittedResource(...);
};
```

#### GPUBuffer (`Engine/GPUBuffer.h`)

GPU 전용 메모리에 위치하는 정점/인덱스 버퍼입니다. CPU에서 직접 접근할 수 없으며, UploadBuffer를 통해 데이터를 복사합니다.

```cpp
class GPUBuffer : public Buffer {
public:
    void CreateVertexBuffer(size_t count, size_t sizeOf);
    void CreateIndexBuffer(size_t count, size_t sizeOf);

    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(UINT stride);
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(DXGI_FORMAT format);
};
```

#### UploadBuffer (`Engine/UploadBuffer.h`)

CPU에서 접근 가능한 업로드 힙 버퍼입니다. 상수 버퍼(Constant Buffer) 용도로 주로 사용되며, 스테이징 버퍼로서 GPUBuffer에 데이터를 전송하는 역할도 합니다.

```cpp
class UploadBuffer : public Buffer {
public:
    void CreateConstantBuffer(size_t sizeOf);
    void CreateConstantBufferArray(size_t count, size_t sizeOf);
    void CreateStagingBuffer(size_t count, size_t sizeOf);

    // CPU → GPU 데이터 전송 (매 프레임 상수 업데이트에 사용)
    template <typename T>
    void Update(const T& data, size_t elementIndex = 0);

    // 스테이징 → GPU 복사
    void CopyDataToBuffer(ID3D12GraphicsCommandList* cmdList,
                          Buffer& dstBuffer, const void* initData);

private:
    BYTE* mappedData_;    // CPU 메모리에 매핑된 포인터
};
```

#### Texture (`Engine/Texture.h`)

렌더 타겟, 깊이 스텐실, 파일 기반 텍스처를 통합 관리합니다.

```cpp
class Texture : public Resource {
public:
    // 렌더 타겟 생성 (스왑체인 백버퍼 래핑 포함)
    void CreateRenderTarget(DXGI_FORMAT format, UINT width, UINT height);
    void WrapBackBuffer(DXGI_FORMAT format);

    // 깊이 스텐실 생성
    void CreateDepthStencil(UINT width, UINT height);

    // 파일에서 텍스처 로딩
    void CreateDDSFromFile(const std::wstring& filename);
    void CreateTextureFromFile(const std::wstring& filename,
                               ID3D12GraphicsCommandList* cmdList);

    // 디스크립터 핸들 접근
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

private:
    UINT width_, height_;
    TextureType type_;    // TEXTURE2D, RENDER_TARGET, DEPTH_STENCIL
};
```

#### Sampler (`Engine/Sampler.h`)

텍스처 샘플링 방식(필터링, 주소 모드 등)을 설정합니다.

```cpp
struct SamplerConfig {
    D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    D3D12_TEXTURE_ADDRESS_MODE addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE addressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    // ... 기타 설정
};

class Sampler {
public:
    void CreateSampler(const SamplerConfig& config = SamplerConfig());
    static std::array<D3D12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
};
```

---

### 4.4 디스크립터 관리

GPU가 리소스에 접근하기 위한 "핸들(디스크립터)"을 관리하는 시스템입니다.

> **디스크립터란?** GPU가 버퍼나 텍스처 같은 리소스에 접근하기 위한 일종의 포인터입니다. DirectX 12에서는 디스크립터를 힙(Heap)에 모아서 관리합니다.

#### DescriptorHandle

CPU와 GPU 양쪽의 핸들을 묶어둔 구조체입니다.

```cpp
struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};   // CPU 측 핸들
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};   // GPU 측 핸들
};
```

#### DescriptorPool (`Engine/DescriptorPool.h`)

타입별 디스크립터 힙을 관리하고, 용도에 맞는 디스크립터를 할당합니다.

```cpp
class DescriptorPool {
public:
    DescriptorPool(ID3D12Device* device);
    void Initialize();

    // 용도별 할당 메서드
    DescriptorHandle AllocateRTV();       // 렌더 타겟 뷰
    DescriptorHandle AllocateDSV();       // 깊이 스텐실 뷰
    DescriptorHandle AllocateCBV();       // 상수 버퍼 뷰
    DescriptorHandle AllocateSRV();       // 셰이더 리소스 뷰
    DescriptorHandle AllocateUAV();       // 비순서 접근 뷰
    DescriptorHandle AllocateSampler();   // 샘플러
    std::vector<DescriptorHandle> AllocateCBVArray(size_t count);

    DescriptorHeap* Get(D3D12_DESCRIPTOR_HEAP_TYPE type);

private:
    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE,
                       std::unique_ptr<DescriptorHeap>> heapAllocator_;
};
```

#### DescriptorHeap (`Engine/DescriptorHeap.h`)

실제 디스크립터 메모리를 관리하는 힙입니다. 선형 할당 방식으로 디스크립터를 순차적으로 배분합니다.

```cpp
class DescriptorHeap {
public:
    DescriptorHeap(ID3D12Device* device, UINT maxDescriptorNum,
                   D3D12_DESCRIPTOR_HEAP_TYPE type,
                   D3D12_DESCRIPTOR_HEAP_FLAGS flag);

    DescriptorHandle AllocateView();                          // 단일 할당
    std::vector<DescriptorHandle> AllocateViewArray(size_t count);  // 배열 할당
    void Reset();                       // 힙 초기화 (재사용)
    ID3D12DescriptorHeap* GetHeap();    // 원시 힙 포인터

private:
    UINT descriptorSize_;     // 디스크립터 하나의 크기
    UINT maxHeapSize_;        // 최대 할당 가능 수
    UINT viewIdx_;            // 현재 할당 인덱스
    bool isShaderVisible_;    // GPU에서 접근 가능 여부
};
```

**힙 타입별 역할:**

| 힙 타입 | 용도 | Shader Visible |
|---------|------|:--------------:|
| `D3D12_DESCRIPTOR_HEAP_TYPE_RTV` | 렌더 타겟 뷰 | No |
| `D3D12_DESCRIPTOR_HEAP_TYPE_DSV` | 깊이 스텐실 뷰 | No |
| `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV` | 상수/텍스처/UAV 뷰 | Yes |
| `D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER` | 샘플러 | Yes |

---

### 4.5 바인딩 & 셰이더

GPU가 셰이더를 실행할 때 어떤 데이터를 어떻게 전달할지 정의하는 시스템입니다.

#### RootSignature (`Engine/RootSignature.h`)

셰이더에 데이터를 전달하는 "계약서(Contract)"입니다. GPU-CPU 간 데이터 바인딩 규칙을 정의합니다.

```
Root Signature 개념도:
┌───────────────────────────────────────────────┐
│              Root Signature                   │
│                                               │
│  Parameter 0: Descriptor Table (CBV b0)       │  ← MeshConstants
│  Parameter 1: Descriptor Table (CBV b1)       │  ← SceneConstants
│  ...                                          │
│  Static Samplers: s0 ~ s5                     │  ← 텍스처 샘플러
└───────────────────────────────────────────────┘
```

```cpp
// Root Parameter 타입
enum class RootParameterType {
    DESCRIPTOR_TABLE,    // 디스크립터 테이블 (가장 일반적)
    ROOT_CONSTANTS,      // 32-bit 상수 직접 전달
    ROOT_DESCRIPTOR      // GPU 주소 직접 전달
};

// 구성 구조체
struct RootSignatureConfig {
    RootParameterType type;
    std::vector<DescriptorRangeConfig> descriptorRanges;
    D3D12_SHADER_VISIBILITY shaderVisibility;
    // ...
};

class RootSignature {
public:
    void Create(const std::vector<RootSignatureConfig>& config);
    ID3D12RootSignature* GetSignature() const;

    // 헬퍼 팩토리 메서드
    static RootSignatureConfig CreateDescriptorTableConfig(
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
        UINT numDescriptors,
        UINT baseShaderRegister,
        UINT registerSpace = 0,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
};
```

#### Shader / ShaderManager (`Engine/Shader.h`, `Engine/ShaderManager.h`)

HLSL 셰이더를 컴파일하고 파이프라인에 사용할 셰이더 조합을 관리합니다.

```cpp
// 개별 셰이더 컴파일
class Shader {
public:
    Shader(const std::string& name, const std::wstring& filePath,
           const std::string& entryPoint, const std::string& targetVersion);
    D3D12_SHADER_BYTECODE GetShader() const;
};

// 셰이더 매니저: 여러 셰이더를 로드하고 파이프라인 조합을 관리
class ShaderManager {
public:
    ShaderManager(const std::wstring& assetsPath);

    void LoadShader(const ShaderConfig& config);
    void CreatePipelineShaders(const std::string& pipelineName,
                               const PipelineShadersConfig& config);
    const PipelineShaders& GetPipelineShaders(const std::string& name) const;
};

// 파이프라인에 사용되는 셰이더 집합
struct PipelineShaders {
    Shader* vs_;    // Vertex Shader
    Shader* ps_;    // Pixel Shader
    Shader* hs_;    // Hull Shader (테셀레이션)
    Shader* ds_;    // Domain Shader (테셀레이션)
    Shader* gs_;    // Geometry Shader
    Shader* cs_;    // Compute Shader
};
```

#### Pipeline (`Engine/Pipeline.h`)

Root Signature + 셰이더 + 렌더 상태를 결합하여 PSO(Pipeline State Object)를 생성합니다.

```cpp
struct PipelineConfig {
    std::string name;

    // 입력 레이아웃 (정점 구조)
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, ...},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, ...},
    };

    // 래스터라이저 설정
    D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;

    // 깊이 테스트
    BOOL depthEnable = TRUE;
    D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;

    // 출력 형식
    DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

class Pipeline {
public:
    Pipeline(ID3D12Device* device, ShaderManager& shaderManager);
    void CreatePSO(const PipelineConfig& config, ID3D12RootSignature* rootSignature);
    ID3D12PipelineState* GetPSO() const;
};
```

**PSO 생성 과정:**
```
PipelineConfig ──┐
                 ├──→ Pipeline::CreatePSO() ──→ ID3D12PipelineState
RootSignature ───┤
ShaderManager ───┘
```

---

### 4.6 장면 & 지오메트리

3D 장면을 구성하는 모델, 메시, 카메라 등을 관리합니다.

#### Vertex (`Engine/Vertex.h`)

정점 데이터 구조체입니다. 셰이더의 입력 레이아웃과 일치해야 합니다.

```cpp
class Vertex {
public:
    DirectX::XMFLOAT3 pos_;     // 3D 위치 (x, y, z)
    DirectX::XMFLOAT4 color_;   // RGBA 색상
};
```

#### ConstantData (`Engine/ConstantData.h`)

셰이더에 전달되는 GPU 상수 버퍼의 구조체들입니다. HLSL의 `cbuffer`와 메모리 레이아웃이 정확히 일치해야 합니다.

```cpp
// 메시별 상수 (register b0)
struct MeshConstants {
    XMFLOAT4X4 world;       // 월드 변환 행렬
    XMFLOAT4X4 invWorld;    // 역 월드 변환 행렬
};

// 장면 전체 상수 (register b1)
struct SceneConstants {
    XMFLOAT4X4 view;           // 뷰 행렬
    XMFLOAT4X4 invView;        // 역 뷰 행렬
    XMFLOAT4X4 proj;           // 투영 행렬
    XMFLOAT4X4 invProj;        // 역 투영 행렬
    XMFLOAT4X4 viewProj;       // 뷰-투영 행렬
    XMFLOAT4X4 invViewProj;    // 역 뷰-투영 행렬
    XMFLOAT3 eyeWorld;         // 카메라 월드 좌표
    float padding1;
};

// 조명 상수 (향후 확장용)
struct LightConstants {
    XMFLOAT3 directionalLightDirection;
    float padding1;
    XMFLOAT3 directionalLightColor;
    float padding2;
    XMFLOAT4X4 lightViewProj;    // 그림자 매핑용
};
```

#### Mesh (`Engine/Mesh.h`)

하나의 메시(정점 + 인덱스 버퍼)를 나타냅니다.

```cpp
class Mesh {
public:
    void SetMesh(const std::string& name,
                 const std::vector<Vertex>& vertices,
                 const std::vector<uint32_t>& indices);

    // GPU 버퍼 생성 (정점 + 인덱스)
    void CreateBuffers(Context& ctx, ID3D12GraphicsCommandList* cmdList);

    // 스테이징 버퍼 해제 (초기 업로드 후)
    void ReleaseStagingBuffers();

    D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();
    UINT GetIndexCount() const;

private:
    std::string name_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::unique_ptr<GPUBuffer> vertexBuffer_;
    std::unique_ptr<GPUBuffer> indexBuffer_;
    std::unique_ptr<UploadBuffer> vertexUploadBuffer_;   // 스테이징
    std::unique_ptr<UploadBuffer> indexUploadBuffer_;     // 스테이징
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
};
```

#### Model (`Engine/Model.h`)

여러 Mesh를 포함하는 모델입니다. 월드 변환 행렬과 프레임별 상수 버퍼를 관리합니다.

```cpp
class Model {
public:
    void AddMesh(Context& ctx, ID3D12GraphicsCommandList* cmdList,
                 const std::string& name,
                 const std::vector<Vertex>& vertices,
                 const std::vector<uint32_t>& indices);

    void Update(size_t frameIdx);     // 상수 버퍼 업데이트
    void UpdateWorldMatrix(const DirectX::XMMATRIX& matrix);

    const std::vector<Mesh>& GetMeshes() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetConstantGPUHandle(size_t frameIdx,
                                                      size_t meshIdx = 0) const;

private:
    std::vector<Mesh> meshes_;
    MeshConstants meshConst_;                      // 월드 행렬
    std::vector<UploadBuffer> constantBuffers_;    // 프레임별 상수 버퍼
};
```

#### Camera (`Engine/Camera.h`)

뷰(View)와 투영(Projection) 행렬을 계산하는 카메라입니다. 두 가지 모드를 지원합니다.

```cpp
enum CameraType { LOOK_AT, FIRST_PERSON };

class Camera {
public:
    Camera(CameraType type = CameraType::FIRST_PERSON);
    void Update();                                        // 뷰 행렬 재계산
    void UpdateSceneConstants(SceneConstants& sceneConst); // 장면 상수에 반영

    // 설정
    void SetPosition(const XMFLOAT3& position);
    void SetRotation(float pitch, float yaw, float roll);
    void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);

    // 행렬 조회
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjMatrix() const;
    XMMATRIX GetViewProjMatrix() const;

private:
    CameraType type_;
    XMFLOAT3 position_ = {0.f, 0.f, -5.f};
    XMFLOAT3 rotation_ = {0.f, 0.f, 0.f};
    XMFLOAT3 forward_, right_, up_;

    struct { float fovY_, nearZ_, farZ_; } parameters;
    struct { XMFLOAT4X4 view_, perspective_; } matrices;
};
```

| 모드 | 설명 |
|------|------|
| `LOOK_AT` | 고정된 대상을 바라보는 카메라 (궤도 카메라) |
| `FIRST_PERSON` | 1인칭 시점 카메라 (자유 이동) |

#### GeometryGenerator (`Engine/GeometryGenerator.h`)

프로시저럴(절차적) 지오메트리를 생성하는 유틸리티입니다.

```cpp
class GeometryGenerator {
public:
    static void CreateBox(std::vector<Vertex>& outVertices,
                          std::vector<uint32_t>& outIndices,
                          float size = 1.f);
};
```

---

### 4.7 렌더링 오케스트레이션

위의 모든 서브시스템을 조율하여 실제 화면을 그리는 최상위 렌더링 계층입니다.

#### Renderer (`Engine/Renderer.h`)

카메라, 셰이더, Root Signature, PSO, 텍스처, 상수 버퍼 등을 총괄하는 고수준 렌더링 오케스트레이터입니다.

```cpp
class Renderer {
public:
    Renderer(Context& ctx, SwapChain& swapChain);
    void Initialize();
    void Update(const Timer& timer, Model& model, size_t frameIdx);
    void Draw(ID3D12GraphicsCommandList* cmdList, Model& model, size_t frameIdx);
    void Resize();

private:
    // 초기화 단계
    void InitResources();       // 깊이 스텐실 생성
    void InitShaders();         // 셰이더 로드
    void InitRootSignature();   // Root Signature 생성
    void InitPipeline();        // PSO 생성
    void InitSamplers();        // 샘플러 생성

    Context& context_;
    SwapChain& swapChain_;
    Camera camera_;
    std::unique_ptr<Texture> depthStencil_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<RootSignature> rootSignature_;
    std::unique_ptr<Pipeline> pipeline_;
    SceneConstants sceneConstants_;
    std::vector<UploadBuffer> sceneConstantBuffer_;   // 프레임별
};
```

**Renderer.Draw() 호출 시 일어나는 일:**
```
1. 백버퍼 상태 전이 → RENDER_TARGET
2. RTV / DSV 설정 및 Clear
3. 디스크립터 힙 바인딩
4. Root Signature 설정
5. PSO 설정
6. Scene Constant Buffer 바인딩 (b1)
7. 각 Mesh에 대해:
   ├─ Vertex Buffer 설정
   ├─ Index Buffer 설정
   ├─ Primitive Topology 설정 (Triangle List)
   ├─ Mesh Constant Buffer 바인딩 (b0)
   └─ DrawIndexedInstanced() 호출
8. 백버퍼 상태 전이 → PRESENT
```

#### SwapChain (`Engine/SwapChain.h`)

더블/트리플 버퍼링을 위한 스왑체인(백버퍼)을 관리합니다.

```cpp
class SwapChain {
public:
    SwapChain(Context& context);
    void Initialize();

    Texture& GetCurrentBackBuffer();
    int GetCurrentBackBufferIndex() const;
    void Present();
    void Resize();
    uint32_t GetBufferCount() const;
    DXGI_FORMAT GetBackBufferFormat() const;    // R8G8B8A8_UNORM

private:
    ComPtr<IDXGISwapChain4> swapChain_;
    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::vector<Texture> backBuffers_;    // 각 백버퍼를 Texture로 래핑
};
```

---

### 4.8 전체 렌더링 루프 흐름도

#### 초기화 단계

```
main()
│
├─ Application 생성자
│  ├─ Window(hInstance, "JEngine")
│  ├─ Context(window)
│  ├─ SwapChain(context)
│  └─ Renderer(context, swapChain)
│
└─ app.Initialize()
   │
   ├─ InitSubsystems()
   │  ├─ Window::Initialize()
   │  │  ├─ RegisterClass()            ← Win32 윈도우 클래스 등록
   │  │  └─ CreateWindowEx()           ← 실제 윈도우 생성 (1280x720)
   │  │
   │  └─ Context::Initialize()
   │     ├─ CreateDXGIFactory2()       ← DXGI 팩토리
   │     ├─ D3D12CreateDevice()        ← GPU 디바이스 (또는 WARP 폴백)
   │     ├─ CreateCommandQueue()       ← GPU 커맨드 큐
   │     └─ DescriptorPool 초기화      ← RTV, DSV, CBV/SRV/UAV, Sampler 힙
   │
   ├─ InitCommandBuffers()
   │  └─ 프레임 수만큼 CommandBuffer 생성 (할당자 + 리스트)
   │
   ├─ InitFences()
   │  └─ 프레임 수만큼 Fence 생성 (동기화 객체)
   │
   ├─ OnResize()
   │  ├─ SwapChain::Resize()
   │  │  ├─ CreateSwapChainForHwnd()   ← DXGI 스왑체인 생성
   │  │  └─ 각 백버퍼를 Texture로 래핑 + RTV 할당
   │  │
   │  ├─ Renderer::Resize()
   │  │  └─ 깊이 스텐실 텍스처 (재)생성
   │  │
   │  └─ Context::SetViewportConfig()  ← 뷰포트 + 시저 렉트 설정
   │
   └─ InitScene()
      ├─ Renderer::Initialize()
      │  ├─ InitShaders()              ← Color.hlsl에서 VS/PS 컴파일
      │  ├─ InitRootSignature()        ← CBV(b0) + CBV(b1) 구성
      │  └─ InitPipeline()             ← PSO 생성
      │
      └─ Model 생성
         ├─ GeometryGenerator::CreateBox()   ← 절차적 박스 지오메트리
         └─ Model::AddMesh()
            └─ Mesh::CreateBuffers()
               ├─ GPUBuffer (정점/인덱스)
               └─ UploadBuffer → GPUBuffer 데이터 복사
```

#### 메인 루프 (프레임별)

```
┌─────────────────────────────────────────────────────────────┐
│                        Run() 메인 루프                       │
│                                                             │
│  while (msg != WM_QUIT)                                     │
│  │                                                          │
│  ├─ Win32 메시지 처리 (PeekMessage)                          │
│  │                                                          │
│  └─ 렌더링 (앱이 활성 상태일 때)                               │
│     │                                                       │
│     ├─ 1. Timer::Tick()                                     │
│     │     deltaTime, FPS 계산                                │
│     │                                                       │
│     ├─ 2. Fence::WaitForGPU()                               │
│     │     이전 프레임의 GPU 작업 완료 대기                      │
│     │                                                       │
│     ├─ 3. CommandBuffer::BeginRecording()                   │
│     │     커맨드 할당자 리셋, 리스트 열기                       │
│     │                                                       │
│     ├─ 4. Context::SetViewport()                            │
│     │     뷰포트 및 시저 렉트 설정                             │
│     │                                                       │
│     ├─ 5. Renderer::Update()                                │
│     │     ├─ Camera::Update()             뷰 행렬 계산       │
│     │     ├─ SceneConstants 업데이트       행렬, 카메라 위치   │
│     │     └─ UploadBuffer::Update()       GPU 상수 전송      │
│     │                                                       │
│     ├─ 6. Model::Update()                                   │
│     │     └─ 각 메시의 MeshConstants 업데이트                  │
│     │                                                       │
│     ├─ 7. Renderer::Draw()                                  │
│     │     ├─ 백버퍼 → RENDER_TARGET 전이                     │
│     │     ├─ RTV/DSV Clear                                  │
│     │     ├─ Root Signature + PSO 바인딩                     │
│     │     ├─ Scene CBV 바인딩 (b1)                           │
│     │     ├─ 각 Mesh 드로우:                                 │
│     │     │   ├─ VB/IB 설정                                 │
│     │     │   ├─ Mesh CBV 바인딩 (b0)                        │
│     │     │   └─ DrawIndexedInstanced()                     │
│     │     └─ 백버퍼 → PRESENT 전이                           │
│     │                                                       │
│     ├─ 8. CommandBuffer::EndRecording()                     │
│     │                                                       │
│     ├─ 9. Context::ExecuteCommands()                        │
│     │     커맨드 리스트를 커맨드 큐에 제출                      │
│     │                                                       │
│     ├─ 10. SwapChain::Present()                             │
│     │      백버퍼를 화면에 표시                                │
│     │                                                       │
│     └─ 11. Fence::Signal()                                  │
│           GPU에 현재 프레임 완료 신호                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 클래스 의존성 다이어그램

```
                        ┌──────────┐
                        │   main   │
                        └────┬─────┘
                             │
                     ┌───────▼────────┐
                     │  Application   │
                     └───────┬────────┘
                             │ 소유
         ┌───────┬───────┬───┴───┬────────┬────────────┐
         │       │       │       │        │            │
    ┌────▼──┐ ┌──▼───┐ ┌─▼──┐ ┌─▼──────┐ │  ┌─────────▼──────────┐
    │Window │ │Timer │ │Model│ │Renderer│ │  │ CommandBuffer(N개) │
    └───────┘ └──────┘ └──┬──┘ └───┬────┘ │  └────────────────────┘
                          │        │      │
                     ┌────▼──┐     │  ┌───▼────┐
                     │ Mesh  │     │  │Fence(N)│
                     └───┬───┘     │  └────────┘
                         │         │
              ┌──────────┤    ┌────▼────────────────────────────────┐
              │          │    │  Context                            │
         ┌────▼────┐ ┌───▼──┐│  ├─ Device, CommandQueue             │
         │GPUBuffer│ │Upload││  └─ DescriptorPool                   │
         └────┬────┘ │Buffer││      ├─ DescriptorHeap (RTV)         │
              │      └──────┘│      ├─ DescriptorHeap (DSV)         │
              │              │      ├─ DescriptorHeap (CBV/SRV/UAV) │
              └──────────────┤      └─ DescriptorHeap (Sampler)     │
                             │                                      │
                        ┌────▼────┐                                 │
                        │Resource │ ← 기본 클래스                     │
                        └────┬────┘                                 │
                             │ 상속                                  │
                   ┌─────────┼──────────┐                           │
              ┌────▼──┐  ┌───▼───┐      │                           │
              │Buffer │  │Texture│      │                           │
              └───┬───┘  └───────┘      │                           │
           ┌──────┼──────┐              │                           │
      ┌────▼────┐ ┌──────▼────┐         │                           │
      │GPUBuffer│ │UploadBuffer│        │                           │
      └─────────┘ └───────────┘         │                           │
                                        │                           │
                         Renderer ──────┘                           │
                         ├─ Camera                                  │
                         ├─ SwapChain ──── Texture (백버퍼)          │
                         ├─ ShaderManager ── Shader                 │
                         ├─ RootSignature                           │
                         └─ Pipeline                                │
```

#### HLSL 셰이더와 CPU 데이터의 매핑

```
C++ (CPU)                          HLSL (GPU)
─────────────                      ─────────────
MeshConstants                      cbuffer cbPerObject : register(b0)
├─ world       ←──────────────────→   float4x4 world
└─ invWorld    ←──────────────────→   float4x4 worldInv

SceneConstants                     cbuffer cbPerScene : register(b1)
├─ view        ←──────────────────→   float4x4 view
├─ invView     ←──────────────────→   float4x4 invView
├─ proj        ←──────────────────→   float4x4 projection
├─ invProj     ←──────────────────→   float4x4 invProj
├─ viewProj    ←──────────────────→   float4x4 viewProj
├─ invViewProj ←──────────────────→   float4x4 invViewProj
├─ eyeWorld    ←──────────────────→   float3 eyeWorld
└─ padding1    ←──────────────────→   float padding1

Vertex                             struct VertexIn
├─ pos_        ←──────────────────→   float3 PosL : POSITION
└─ color_      ←──────────────────→   float4 Color : COLOR

Root Signature
├─ Parameter 0 (CBV) ─────────────→ register(b0)  MeshConstants
└─ Parameter 1 (CBV) ─────────────→ register(b1)  SceneConstants
```

---

## 5. 시작하기

### 필수 도구

| 도구 | 버전 | 용도 |
|------|------|------|
| Visual Studio 2022 | v143 toolset | C++ 빌드 및 디버깅 |
| Windows SDK | 10.0.19041.0+ | DirectX 12 헤더 및 라이브러리 |
| vcpkg | 최신 | 외부 패키지 관리 |
| Git | 최신 | 버전 관리 |

### 빌드 방법

#### Visual Studio에서 빌드

1. `JEngine.sln`을 Visual Studio 2022로 엽니다.
2. 상단 메뉴에서 구성을 선택합니다: `Debug` 또는 `Release`, 플랫폼은 `x64`.
3. `빌드 > 솔루션 빌드` (Ctrl+Shift+B)를 실행합니다.
4. `Application` 프로젝트를 시작 프로젝트로 설정하고 F5로 실행합니다.

#### CLI에서 빌드

```bash
# vcpkg 의존성 설치 (처음 한 번만)
vcpkg install

# MSBuild로 빌드
msbuild JEngine.sln /p:Configuration=Release /p:Platform=x64
```

### 실행 결과

빌드 후 실행하면 1280x720 크기의 윈도우가 열리고, 색상이 입혀진 3D 박스가 렌더링됩니다. 카메라는 기본적으로 (0, 0, -5) 위치에서 원점을 바라봅니다.

### 코드 수정 시 참고할 파일

| 하고 싶은 일 | 참고 파일 |
|-------------|----------|
| 새로운 3D 모델 추가 | `Engine/Model.h`, `Engine/Mesh.h`, `Engine/GeometryGenerator.h` |
| 셰이더 수정/추가 | `Assets/Shaders/Color.hlsl`, `Engine/ShaderManager.h` |
| 카메라 동작 변경 | `Engine/Camera.h` |
| 렌더링 파이프라인 수정 | `Engine/Renderer.h`, `Engine/Pipeline.h` |
| GPU 바인딩 변경 | `Engine/RootSignature.h`, `Engine/ConstantData.h` |
| 새 리소스 타입 추가 | `Engine/Resource.h`, `Engine/Buffer.h`, `Engine/Texture.h` |
| 윈도우/입력 처리 변경 | `Engine/Window.h`, `Engine/Application.h` |
| 외부 라이브러리 추가 | `vcpkg.json` |
