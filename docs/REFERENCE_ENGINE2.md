# extern/engine2 아키텍처 분석

> Vulkan 1.3 기반 실시간 3D 렌더링 엔진 — engine1의 확장 버전
> **engine1 대비 추가/개선된 기능**에 집중하여 분석한다.

---

## 1. 개요

### 프로젝트 정보

| 항목 | 내용 |
|------|------|
| 네임스페이스 | `hlab` |
| 그래픽스 API | Vulkan 1.3 |
| 기반 | engine1을 확장 (동일 네임스페이스, 동일 핵심 아키텍처) |
| 소스 규모 | 헤더 약 36개, 구현 약 40개 (engine1 대비 +5 신규 헤더) |

### 핵심 설계 철학 (engine1 계승 + 확장)

- **Resource 추상화**: 모든 GPU 리소스가 공통 기본 클래스(`Resource`)를 상속 → 다형적 디스크립터 바인딩
- **데이터 주도 파이프라인**: `PipelineConfig` 구조체로 파이프라인 설정을 코드에서 분리
- **Bindless 렌더링**: `TextureManager`로 텍스처 배열을 관리, 개별 디스크립터 세트 대신 인덱스 기반 접근
- **GPU 성능 프로파일링**: `GpuTimer`로 GPU 프레임 시간 측정
- **메모리 최적화**: 반정밀도(F16) Vertex로 대규모 메시의 메모리 사용량 32% 절감

---

## 2. engine1 대비 변경 요약

### 신규 추가 파일

| 파일 | 역할 |
|------|------|
| `Resource.h/cpp` | GPU 리소스 추상 기본 클래스 |
| `PipelineConfig.h/cpp` | 파이프라인 설정 데이터 구조체 + 팩토리 |
| `TextureManager.h/cpp` | Bindless 렌더링용 텍스처 배열 관리 |
| `GpuTimer.h/cpp` | GPU 타임스탬프 프로파일링 |
| `RenderGraph.h/cpp` | 렌더 패스 순서/리소스 정의 (JSON 직렬화) |

### 삭제/통합된 파일

| engine1 파일 | engine2 대체 방식 |
|--------------|-------------------|
| `UniformBuffer.h` | `MappedBuffer` + 템플릿 메서드로 통합 |
| `DepthStencil.h` | `Image2D::createDepthBuffer()` 등으로 통합 |
| `ShadowMap.h` | `Image2D::createShadow()` + `imageBuffers_["shadowMap"]` |
| `SkyTextures.h` | `imageBuffers_["prefilteredMap"]` 등으로 통합 |

### 주요 변경 파일

| 파일 | 변경 내용 |
|------|-----------|
| `Vertex.h` | 32비트 → 반정밀도(F16) 전환, 88B → ~60B |
| `Pipeline.h` | `PipelineConfig` 기반 생성, DescriptorSet 자동 관리 |
| `Renderer.h` | 리소스 맵 기반 관리, Bindless 렌더링, RenderGraph |
| `Application.h` | `GpuTimer` 추가, FPS 트래킹 이동 |
| `DescriptorSet.h` | `Resource` 다형성 기반 바인딩 |

### 전체 비교표

| 기능 영역 | engine1 | engine2 |
|-----------|---------|---------|
| 리소스 기본 클래스 | 없음 (독립 클래스) | `Resource` 추상 클래스 |
| 디스크립터 바인딩 | `ResourceBinding` 직접 참조 | `Resource::updateWrite()` 다형성 |
| 파이프라인 생성 | `createByName(string)` + 개별 함수 | `PipelineConfig` 데이터 구조체 |
| 텍스처 관리 | 모델별 개별 `Image2D` + `DescriptorSet` | `TextureManager` Bindless 배열 |
| Vertex 크기 | 88 bytes (F32) | ~60 bytes (F16 + F32 혼합) |
| GPU 프로파일링 | 없음 | `GpuTimer` |
| 렌더 그래프 | 없음 | `RenderGraph` (JSON 직렬화) |
| 유니폼 버퍼 | `UniformBuffer<T>` 템플릿 | `MappedBuffer` 직접 사용 |
| 리소스 저장 | 개별 멤버 변수 | `unordered_map<string, ...>` |
| 모델 컨테이너 | `vector<Model>` (값) | `vector<unique_ptr<Model>>` (포인터) |
| 깊이 버퍼 | 전용 `DepthStencil` 클래스 | `Image2D` 통합 |
| 그림자 맵 | 전용 `ShadowMap` 클래스 | `Image2D` 통합 |
| IBL 텍스처 | 전용 `SkyTextures` 클래스 | `imageBuffers_` 맵에 통합 |
| Bone 최대 수 | 256개 | 65개 (메모리 절감) |
| MSAA | Application에서 관리 | Renderer 내부 관리 |

---

## 3. 아키텍처 계층도

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application                             │
│        (메인 루프, 윈도우 이벤트, 전체 초기화)                      │
│                      + GpuTimer [NEW]                           │
└──────┬──────────┬──────────┬──────────┬────────────────┬────────┘
       │          │          │          │                │
   Window      Context   Swapchain  ShaderManager  GuiRenderer
  (GLFW)    (디바이스/큐)  (Present)  (셰이더 관리)   (ImGui)
                │                        │
         DescriptorPool             Shader × N
        (레이아웃 캐싱)          (SPIR-V + Reflect)
                │
  ┌─────────────┴──────────────────────────────────────┐
  │                    Renderer                         │
  │         (렌더링 전체 오케스트레이션)                    │
  │  + TextureManager [NEW]  + RenderGraph [NEW]        │
  │  + StorageBuffer (Bindless)  + PipelineConfig 기반   │
  └─────────────────────────────────────────────────────┘
       │              │               │
   Pipeline ×N   Image2D ×N     DescriptorSet
  (PipelineConfig  (텍스처/RT)   (Resource 다형성)
    기반 생성)        │
              ┌──────┴──────┐
         BarrierHelper  ResourceBinding
        (레이아웃 전환)  (디스크립터 링크)

  ┌──────────────── Resource [NEW] ────────────────┐
  │            (추상 기본 클래스)                      │
  │  ┌────────┬────────────┬──────────┬───────────┐│
  │  │Image2D │MappedBuffer│StorageBuf│TexManager ││
  │  │(Image) │ (Buffer)   │(Buffer)  │(Image[])  ││
  │  └────────┴────────────┴──────────┴───────────┘│
  └────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────┐
  │                  Model                    │
  │  ┌────────┬──────────┬─────────────────┐ │
  │  │  Mesh  │ Material │    ModelNode     │ │
  │  │(F16 정점)│(PBR UBO)│  (씬 그래프)    │ │
  │  └────────┴──────────┴─────────────────┘ │
  │      Vertex [CHANGED: F16]               │
  │              Animation                    │
  └──────────────────────────────────────────┘
```

---

## 4. 서브시스템별 분석

> engine1과 동일한 부분은 생략하고, **변경/추가된 부분**에 집중한다.

### 4.1 Resource 추상화 (신규)

#### Resource (`Resource.h/cpp`)

**역할:** 모든 GPU 리소스의 추상 기본 클래스. 이미지와 버퍼를 다형적으로 처리할 수 있게 한다.

```cpp
class Resource {
public:
    enum class Type { Image, Buffer };

    // 순수 가상 메서드 — 파생 클래스가 구현
    virtual void cleanup() = 0;
    virtual void updateWrite(VkDescriptorSetLayoutBinding expectedBinding,
                             VkWriteDescriptorSet& write) = 0;

    // 공통 인터페이스
    Type getType() const;
    bool isImage() const;
    bool isBuffer() const;
    BarrierHelper& barrierHelper();
    ResourceBinding& resourceBinding();

    // 이미지 전용 전환 헬퍼
    void transitionToColorAttachment(VkCommandBuffer cmd);
    void transitionToShaderRead(VkCommandBuffer cmd);
    void transitionToTransferSrc(VkCommandBuffer cmd);
    void transitionToTransferDst(VkCommandBuffer cmd);
    void transitionToDepthStencilAttachment(VkCommandBuffer cmd);
    void transitionToGeneral(VkCommandBuffer cmd, ...);
    void setSampler(VkSampler sampler);

    // 버퍼 전용
    void transitionBuffer(...);

protected:
    Context& ctx_;
    Type type_;
    BarrierHelper barrierHelper_;
    ResourceBinding resourceBinding_;

    void initializeImageResource(VkImage, VkFormat, uint32_t mipLevels, uint32_t arrayLayers);
    void initializeBufferResource(VkBuffer, VkDeviceSize);
    void updateResourceBinding();
};
```

**상속 계층:**

```
Resource (추상 기본 클래스)
  ├── Image2D         (Type::Image)    — 2D 텍스처/RT/깊이 버퍼
  ├── MappedBuffer    (Type::Buffer)   — CPU 접근 가능 버퍼
  ├── StorageBuffer   (Type::Buffer)   — SSBO
  └── TextureManager  (Type::Image)    — Bindless 텍스처 배열
```

**engine1과의 차이:**
- engine1: 각 리소스가 독립적으로 `ResourceBinding`과 `BarrierHelper`를 합성
- engine2: `Resource` 기본 클래스가 공통 기능을 제공하고, `updateWrite()` 가상 함수로 디스크립터 갱신을 다형적으로 처리

**설계 패턴:** Template Method 패턴 — `updateWrite()`는 파생 클래스가 구현하지만, 전환 헬퍼와 바인딩 관리는 기본 클래스가 제공

---

### 4.2 PipelineConfig (신규)

#### PipelineConfig (`PipelineConfig.h/cpp`)

**역할:** 파이프라인 생성 설정을 데이터로 분리. engine1의 `createByName()` + 개별 create 함수들을 대체한다.

```cpp
struct PipelineConfig {
    string name;
    enum class Type { Graphics, Compute } type = Type::Graphics;

    struct RequiredFormats {
        bool outColorFormat = false;
        bool depthFormat = false;
        bool msaaSamples = false;
    } requiredFormats;

    struct VertexInput {
        enum class Type { None, ImGui, Standard } type = Type::None;
    } vertexInput;

    struct DepthStencil {
        bool depthTest = false;
        bool depthWrite = false;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    } depthStencil;

    struct Rasterization {
        VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        bool depthClampEnable = false;
        bool depthBiasEnable = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
    } rasterization;

    struct ColorBlend {
        bool blendEnable = false;
        // ...
    } colorBlend;

    struct Multisample {
        enum class Type { Single, Variable } type;
    } multisample;

    struct DynamicState {
        vector<VkDynamicState> states;
    } dynamicState;

    struct SpecialConfig {
        bool isDepthOnly = false;           // ShadowMap: 컬러 어태치먼트 없음
        bool isScreenSpace = false;         // Post/Sky: 버텍스 버퍼 없음
        bool hasCustomVertexFormat = false;  // GUI: ImGui 포맷
    } specialConfig;

    // 팩토리 메서드
    static PipelineConfig createGui();
    static PipelineConfig createPbrForward();
    static PipelineConfig createPbrDeferred();
    static PipelineConfig createPost();
    static PipelineConfig createShadowMap();
    static PipelineConfig createSky();
    static PipelineConfig createCompute();
    static PipelineConfig createSsao();
    static PipelineConfig createDeferredLighting();
    static PipelineConfig createTriangle();
};
```

**engine1과의 차이:**

| 항목 | engine1 | engine2 |
|------|---------|---------|
| 파이프라인 정의 | `createByName()` → 개별 `createXxx()` 함수 | `PipelineConfig` 데이터 구조체 |
| 설정 위치 | 각 `PipelineXxx.cpp` 파일에 하드코딩 | `PipelineConfig`의 팩토리 메서드에 선언적 정의 |
| 새 파이프라인 추가 | 새 `.cpp` 파일 + `createByName()`에 분기 추가 | `PipelineConfig::createXxx()` 팩토리 메서드 추가 |

**설계 패턴:** Configuration Object 패턴 + Factory Method 패턴 — 설정을 데이터로 분리하여 Pipeline 생성 로직을 단순화

---

### 4.3 TextureManager & Bindless 렌더링 (신규)

#### TextureManager (`TextureManager.h/cpp`)

**역할:** Bindless 렌더링을 위한 텍스처 배열 관리

```cpp
class TextureManager : public Resource {
    friend class Model;
public:
    TextureManager(Context& ctx);

    void cleanup() override;
    void updateWrite(VkDescriptorSetLayoutBinding expectedBinding,
                     VkWriteDescriptorSet& write) override;

private:
    const uint32_t kMaxTextures_ = 512;           // 최대 512개
    vector<unique_ptr<Image2D>> textures_;
    vector<VkDescriptorImageInfo> imageInfos_;
};
```

**Bindless 렌더링 구조:**

```
전통적 방식 (engine1):
  각 Model → 개별 DescriptorSet → 텍스처 N개 바인딩
  드로우콜마다 DescriptorSet 전환 필요

Bindless 방식 (engine2):
  TextureManager → 하나의 큰 텍스처 배열 (최대 512개)
  PbrPushConstants::materialIndex → 셰이더에서 인덱스로 접근
  DescriptorSet 전환 없이 Push Constant만 변경
```

**관련 변경사항:**

```cpp
// engine2의 PbrPushConstants (Push Constants에 materialIndex 추가)
struct PbrPushConstants {
    glm::mat4 model = glm::mat4(1.0f);    // 64 bytes
    uint32_t materialIndex = 0;            // 4 bytes [NEW]
    float coeffs[15] = {0.0f};            // 60 bytes
};
static_assert(sizeof(PbrPushConstants) == 128);
```

**Renderer에서의 사용:**
- `TextureManager materialTextures_` — 모든 모델의 텍스처를 통합 관리
- `StorageBuffer materialBuffer_` — 모든 모델의 머티리얼 데이터를 하나의 SSBO로 관리
- 셰이더에서 `materialIndex`로 텍스처와 머티리얼을 동시에 인덱싱

---

### 4.4 GpuTimer (신규)

#### GpuTimer (`GpuTimer.h/cpp`)

**역할:** GPU 타임스탬프 쿼리를 통한 프레임 시간 측정

```cpp
class GpuTimer {
public:
    GpuTimer(Context& ctx, uint32_t maxFramesInFlight);
    ~GpuTimer();

    // 프레임 당 타이밍
    void beginFrame(VkCommandBuffer cmd, uint32_t frameIndex);
    void endFrame(VkCommandBuffer cmd, uint32_t frameIndex);
    void resetQueries(VkCommandBuffer cmd, uint32_t frameIndex);

    // 결과 조회
    float getGpuTimeMs(uint32_t frameIndex) const;
    bool isResultReady(uint32_t frameIndex) const;
    bool isTimestampSupported() const;
    bool hasAnyResultsReady() const;

private:
    std::vector<VkQueryPool> queryPools_;       // 프레임 인플라이트 수만큼
    mutable std::vector<float> gpuTimes_;
    mutable std::vector<bool> resultsReady_;
    float timestampPeriod_;                     // 나노초/틱 변환 계수
    bool timestampSupported_;
};
```

**Application에서의 사용:**

```cpp
// Application.h (engine2)
GpuTimer gpuTimer_;
float currentGpuTimeMs_{0.0f};
float gpuTimeUpdateTimer_{0.0f};
static constexpr float kGpuTimeUpdateInterval = 0.1f;  // 0.1초마다 갱신
```

**설계 포인트:**
- `maxFramesInFlight` 수만큼 쿼리 풀을 생성하여 인플라이트 프레임 간 충돌 방지
- `timestampPeriod`로 하드웨어별 나노초 변환 자동 처리
- 타임스탬프 미지원 GPU를 위한 `isTimestampSupported()` 가드

---

### 4.5 RenderGraph (신규)

#### RenderGraph (`RenderGraph.h/cpp`)

**역할:** 렌더 패스 순서와 리소스 연결을 선언적으로 정의하고 JSON으로 직렬화

```cpp
class RenderGraph {
    friend class Renderer;
public:
    struct RenderNode {
        vector<string> pipelineNames;       // 이 패스에서 사용할 파이프라인 이름들
        vector<string> colorAttachments;    // 컬러 어태치먼트 이름
        string depthAttachment;             // 깊이 어태치먼트 이름
        string stencilAttachment;           // 스텐실 어태치먼트 이름
    };

    void addRenderNode(RenderNode node);
    void writeToFile(const string& filename) const;   // JSON 직렬화
    bool readFromFile(const string& filename);         // JSON 역직렬화

private:
    vector<RenderNode> renderNodes_;
    // JSON 헬퍼
    string vectorToJsonArray(const vector<string>& vec) const;
    vector<string> jsonArrayToVector(const string& jsonArray) const;
};
```

**설계 포인트:**
- 현재는 **선언적 정의 + 직렬화** 수준이며, 자동 리소스 의존성 분석이나 배리어 삽입은 없음
- `Renderer`의 friend 클래스로, Renderer가 내부 데이터를 읽어 렌더 순서를 결정
- JSON 파일로 렌더링 구성을 외부화할 수 있는 기반 제공

---

### 4.6 Vertex 반정밀도 최적화 (변경)

#### Vertex (`Vertex.h`)

**engine1 (88 bytes, 모두 F32):**

```cpp
struct Vertex {
    alignas(4) vec3 position;       // 12 bytes
    alignas(4) vec3 normal;         // 12 bytes
    alignas(4) vec2 texCoord;       //  8 bytes
    alignas(4) vec3 tangent;        // 12 bytes
    alignas(4) vec3 bitangent;      // 12 bytes
    alignas(4) vec4 boneWeights;    // 16 bytes
    alignas(4) ivec4 boneIndices;   // 16 bytes
};  // 총 88 bytes
```

**engine2 (~60 bytes, F16 + F32 혼합):**

```cpp
// 반정밀도 타입 정의
using half = uint16_t;
struct alignas(2) hvec2 { half x, y; };
struct alignas(2) hvec3 { half x, y, z; };

struct Vertex {
    hvec3 position;       //  6 bytes (F16)
    hvec3 normal;         //  6 bytes (F16)
    hvec2 texCoord;       //  4 bytes (F16)
    hvec3 tangent;        //  6 bytes (F16)
    hvec3 bitangent;      //  6 bytes (F16)
    // 애니메이션 데이터는 정확도가 중요하므로 F32 유지
    alignas(4) vec4 boneWeights;    // 16 bytes (F32)
    alignas(4) ivec4 boneIndices;   // 16 bytes (F32)

    // F32 ↔ F16 변환 헬퍼
    static inline half packHalf(float v) { return glm::packHalf1x16(v); }
    static inline float unpackHalf(half v) { return glm::unpackHalf1x16(v); }

    // F32 접근자 (편의 메서드)
    vec3 getPosition() const;
    void setPosition(const vec3& pos);
    // ...
};

static_assert(sizeof(Vertex) <= 64, "Vertex should fit in 64 bytes");
```

**메모리 절감 분석:**

| 항목 | engine1 (F32) | engine2 (F16) | 절감 |
|------|---------------|---------------|------|
| 기하 속성 (position~bitangent) | 56 bytes | 28 bytes | 50% |
| 애니메이션 (boneWeights+Indices) | 32 bytes | 32 bytes | 0% |
| **합계** | **88 bytes** | **~60 bytes** | **~32%** |

**트레이드오프:**
- F16은 ±65504 범위, 소수점 약 3자리 정밀도 → 노멀, 텍스처 좌표에 충분
- 대규모 씬의 위치 데이터(>65504 단위)에서는 정밀도 손실 가능
- GLM의 `packHalf1x16`/`unpackHalf1x16`으로 변환 오버헤드 최소화

---

### 4.7 Pipeline 변경사항

#### Pipeline (`Pipeline.h/cpp`)

**engine2에서 추가된 기능:**

```cpp
class Pipeline {
    // PipelineConfig 기반 생성자 [NEW]
    Pipeline(Context& ctx, ShaderManager& shaderManager, const PipelineConfig& config,
             vector<VkFormat> outColorFormats = {},
             optional<VkFormat> depthFormat = nullopt,
             optional<VkSampleCountFlagBits> msaaSamples = nullopt);

    // DescriptorSet 자동 관리 [NEW]
    void setDescriptorSets(vector<vector<reference_wrapper<DescriptorSet>>>& descriptorSets);
    void bindDescriptorSets(const VkCommandBuffer& cmd, uint32_t frameIndex);
    void submitBarriers(const VkCommandBuffer& cmd, uint32_t frameIndex);

    // Compute 디스패치 내장 [NEW]
    void dispatch(const VkCommandBuffer& cmd, uint32_t frameIndex);

    // 셰이더 리플렉션에서 워크그룹 크기 자동 획득 [NEW]
    void initializeComputeLocalWorkgroupSize();
    void determineDimensionsFromFirstWriteOnlyBinding();

private:
    // 추가된 멤버
    VkPipelineBindPoint bindPoint_;     // Graphics or Compute
    vector<VkDescriptorSetLayout> layouts_;
    vector<vector<reference_wrapper<DescriptorSet>>> descriptorSets_;
    vector<vector<BindingInfo>> bindingInfos_;
    uint32_t width_, height_;
    array<uint32_t, 3> local_size_;     // Compute 워크그룹 크기
};
```

**engine1과의 차이:**

| 항목 | engine1 | engine2 |
|------|---------|---------|
| 생성 방식 | `createByName(string)` | `PipelineConfig` 기반 생성자 |
| DescriptorSet | 외부에서 수동 바인딩 | `setDescriptorSets()` + `bindDescriptorSets()` 자동화 |
| Compute | 주석 처리 | `dispatch()` + `local_size_` 자동 감지 |
| 배리어 | 외부에서 수동 관리 | `submitBarriers()` 자동 처리 |

---

### 4.8 Renderer 변경사항

#### Renderer (`Renderer.h/cpp`)

**리소스 관리 방식 변경:**

```cpp
// engine1: 개별 멤버 변수
Image2D msaaColorBuffer_;
DepthStencil depthStencil_;
ShadowMap shadowMap_;
SkyTextures skyTextures_;
vector<UniformBuffer<SceneUniform>> sceneUniforms_;

// engine2: 문자열 키 기반 맵
unordered_map<string, unique_ptr<Image2D>> imageBuffers_;
unordered_map<string, vector<unique_ptr<MappedBuffer>>> perFrameUniformBuffers_;
```

**engine2의 이미지 버퍼 키 예시:**
- `"shadowMap"`, `"prefilteredMap"`, `"irradianceMap"`, `"brdfLut"`
- `"floatColor1"`, `"floatColor2"`, `"depthBuffer"`
- `"ssaoOutput"`, `"ssaoBlurred"`

**Bindless 관련 추가 멤버:**

```cpp
TextureManager materialTextures_;     // 모든 텍스처 통합
StorageBuffer materialBuffer_;        // 모든 머티리얼 SSBO
```

**UBO 구조체 변경:**

```cpp
// BoneDataUniform — 본 수 축소
struct BoneDataUniform {
    glm::mat4 boneMatrices[65];     // engine1: 256 → engine2: 65
    glm::vec4 animationData;
};

// SSAO 옵션 분리 [NEW]
struct SsaoOptionsUBO {
    float ssaoRadius = 0.1f;
    float ssaoBias = 0.025f;
    int ssaoSampleCount = 16;
    float ssaoPower = 2.0f;
};
```

**모델 컨테이너 변경:**

```cpp
// engine1
vector<Model> models_;                    // 값 타입 (이동 전용이므로 복사 불가)

// engine2
vector<unique_ptr<Model>> models_;        // 포인터로 간접 소유
```

---

### 4.9 DescriptorSet 변경사항

#### DescriptorSet (`DescriptorSet.h/cpp`)

**engine1:**

```cpp
void create(Context& ctx,
            const vector<reference_wrapper<ResourceBinding>>& resourceBindings);
```

**engine2:**

```cpp
void create(Context& ctx,
            const VkDescriptorSetLayout& layout,
            const vector<reference_wrapper<Resource>>& resources);
```

**핵심 변경:** `ResourceBinding` 직접 참조 → `Resource` 다형성 참조. 어떤 리소스 타입이든 `Resource::updateWrite()`를 통해 자동으로 디스크립터를 갱신한다.

---

### 4.10 Application 변경사항

#### Application (`Application.h/cpp`)

**추가된 멤버:**

```cpp
// GPU 프로파일링 [NEW]
GpuTimer gpuTimer_;
float currentGpuTimeMs_{0.0f};
float gpuTimeUpdateTimer_{0.0f};
uint32_t gpuFramesSinceLastUpdate_{0};
static constexpr float kGpuTimeUpdateInterval = 0.1f;

// FPS 트래킹 (engine1에서는 다른 곳에 있었음)
float currentFPS_{0.0f};
float fpsUpdateTimer_{0.0f};
uint32_t framesSinceLastUpdate_{0};
```

**추가된 메서드:**

```cpp
void renderSSAOControlWindow();     // ImGui SSAO 제어 패널
```

**제거된 멤버:**

```cpp
// engine1에 있던 MSAA 직접 관리
VkSampleCountFlagBits msaaSamples_;  // → Renderer 내부로 이동
```

---

## 5. 설계 패턴 요약

> engine1의 패턴을 모두 계승하면서 추가된 패턴에 집중한다.

| 패턴 | 적용 클래스 | 목적 |
|------|-------------|------|
| **Template Method** | `Resource::updateWrite()` (순수 가상) | 파생 클래스별 디스크립터 갱신 방식을 다형적으로 처리 |
| **Configuration Object** | `PipelineConfig` | 파이프라인 설정을 데이터로 분리, 생성 로직과 설정 분리 |
| **Bindless Rendering** | `TextureManager` + `StorageBuffer` + `PbrPushConstants` | DescriptorSet 전환 없이 인덱스 기반 리소스 접근 |
| **Data-Driven Resource Management** | `Renderer`의 `unordered_map<string, ...>` | 문자열 키로 리소스를 동적으로 관리 |
| **추상 팩토리** | `PipelineConfig::createXxx()` | 파이프라인 종류별 사전 정의된 설정 생성 |
| **다형성 디스크립터 바인딩** | `DescriptorSet` + `Resource` 계층 | 리소스 타입과 무관한 통합 바인딩 인터페이스 |
| **GPU 프로파일링** | `GpuTimer` | 타임스탬프 쿼리로 GPU 병목 측정 |
| **선언적 렌더 그래프** | `RenderGraph` | 렌더 패스 순서를 JSON으로 외부화 |
| **RAII + 이동 전용** | engine1과 동일 | 계승 |
| **합성(Composition)** | engine1과 동일 (+ Resource 기본 클래스) | 계승 + 확장 |
| **Façade** | `Renderer`, `Application` | 계승 |
| **셰이더 리플렉션** | `Shader` + `ShaderManager` | 계승 |
| **싱글톤** | `Logger` | 계승 |

---

## 6. 클래스 의존성 다이어그램

```
                        Application
                       /    |    \     \        \         \
                      /     |     \     \        \         \
                 Window  Context  Swapchain  Camera  GuiRenderer  GpuTimer [NEW]
                          / |  \
                         /  |   \
              CmdBuffer  DPool  VulkanTools
                          |
                    ┌─────┴─────┐
             DescriptorSet    DescriptorSetLayout
                    |
              Resource (추상) [NEW]
              ┌─────┼──────────┬────────────┐
          Image2D  MappedBuffer  StorageBuf  TextureManager [NEW]
              |         |
         BarrierHelper  ResourceBinding

                       Renderer ──────────────────────────────────────┐
                      / | | | \    \        \                          |
                     /  | | |  \    \        \                         |
              Pipeline  | | | Image2D  TextureManager [NEW]    ViewFrustum
             (Config    | | |          StorageBuffer [NEW]
              기반)     | | |          RenderGraph [NEW]
                        | | |
                  MappedBuffer (맵 기반)
                        |
               PipelineConfig [NEW]

                        Model
                       / | | \
                      /  | |  \
                   Mesh  | |  ModelNode (트리)
                (F16     | |
                Vertex)  | |
                  Material Animation
                         |     |
                   MaterialUBO  AnimationClip + Bone

         ShaderManager ──→ Shader × N ──→ spirv-reflect
              |
              └──→ DescriptorPool (바인딩 정보)
```

**engine1 대비 의존성 변화:**

```
[새로운 의존성]
Application → GpuTimer
Renderer → TextureManager, StorageBuffer, RenderGraph
Pipeline → PipelineConfig
DescriptorSet → Resource (다형성)
Image2D, MappedBuffer, StorageBuffer, TextureManager → Resource (상속)

[제거된 의존성]
Renderer ✕→ ShadowMap (Image2D로 통합)
Renderer ✕→ SkyTextures (imageBuffers_로 통합)
Renderer ✕→ DepthStencil (Image2D로 통합)
Renderer ✕→ UniformBuffer<T> (MappedBuffer 직접 사용)
```

---

## 7. engine1 → engine2 구조적 개선 요약

### 7.1 리소스 통합 (Resource 기본 클래스)

```
engine1:                              engine2:
  Image2D (독립)                        Resource (추상)
  MappedBuffer (독립)         →           ├── Image2D
  StorageBuffer (독립)                    ├── MappedBuffer
  DepthStencil (독립)                     ├── StorageBuffer
  ShadowMap (독립)                        └── TextureManager
  SkyTextures (독립)
  UniformBuffer<T> (독립)
```

**장점:** 디스크립터 바인딩 코드의 통합, 새로운 리소스 타입 추가 시 `updateWrite()`만 구현하면 됨

### 7.2 파이프라인 설정 분리

```
engine1:                              engine2:
  Pipeline::createByName()              PipelineConfig (데이터)
    → createPbrForward()       →        Pipeline(ctx, shaderManager, config)
    → createShadowMap()
    → createPost()
    → ... (각각 하드코딩)
```

**장점:** 새 파이프라인 추가 시 코드 변경 최소화, 설정을 한 곳에서 관리

### 7.3 Bindless 렌더링

```
engine1:                              engine2:
  Model별 DescriptorSet                 TextureManager (전역 배열)
  → 드로우콜마다 Set 전환      →        → Push Constant로 인덱스만 변경
  → Set 수 = 모델 수 × 프레임 수        → Set 수 = 1 (전체 공유)
```

**장점:** CPU 오버헤드 감소 (DescriptorSet 전환 비용 제거), 대규모 씬에서 성능 향상

### 7.4 메모리 최적화 (F16 Vertex)

```
engine1:                              engine2:
  Vertex = 88 bytes (F32)              Vertex ≈ 60 bytes (F16+F32)
  1M 정점 = 84 MB           →          1M 정점 = 57 MB
                                       약 32% 메모리 절감
```

**장점:** GPU 메모리 대역폭 절감, 캐시 효율성 향상, 대규모 메시에서 성능 향상

### 7.5 리소스 맵 기반 관리

```
engine1:                              engine2:
  개별 멤버 변수:                        문자열 키 맵:
    Image2D msaaColorBuffer_              imageBuffers_["msaaColor"]
    DepthStencil depthStencil_   →        imageBuffers_["depthBuffer"]
    ShadowMap shadowMap_                  imageBuffers_["shadowMap"]
    SkyTextures skyTextures_              imageBuffers_["prefilteredMap"]
```

**장점:** 리소스 추가/제거가 코드 구조 변경 없이 가능, 동적 리소스 관리 용이

---

## 8. 외부 의존성

engine1과 동일하며, 추가된 의존성은 없다.

| 라이브러리 | 사용처 | 역할 |
|------------|--------|------|
| Vulkan 1.3 | 엔진 전반 | 그래픽스 API |
| GLFW | `Window` | 윈도우/입력 관리 |
| GLM | `Camera`, `Vertex`, `Animation`, `ViewFrustum` | SIMD 수학 (+ `packHalf1x16`) |
| Assimp | `ModelLoader`, `Animation` | 3D 모델 파일 로딩 |
| spirv-reflect | `Shader`, `ShaderManager` | 셰이더 리플렉션 |
| ImGui | `GuiRenderer` | 디버그 UI |
| stb_image | `Image2D` | 이미지 파일 로딩 |
| KTX2 | `Image2D` | KTX2 텍스처 로딩 |
