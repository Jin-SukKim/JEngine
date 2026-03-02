# extern/engine 아키텍처 분석

> Vulkan 1.3 기반 실시간 3D 렌더링 엔진 레퍼런스 분석 문서
> Vulkan API 사용법이 아닌 **엔진 구조와 설계 패턴**에 집중하여 분석한다.

---

## 1. 개요

### 프로젝트 정보

| 항목 | 내용 |
|------|------|
| 네임스페이스 | `hlab` |
| 그래픽스 API | Vulkan 1.3 |
| 윈도우 시스템 | GLFW |
| 셰이더 포맷 | SPIR-V (spirv-reflect로 런타임 리플렉션) |
| 수학 라이브러리 | GLM |
| 모델 로딩 | Assimp |
| UI | ImGui |
| 소스 규모 | 헤더 약 35개, 구현 약 40개 |

### 핵심 설계 철학

- **합성(Composition) 우선**: 깊은 상속 계층 없이 합성과 참조로 기능 조합
- **RAII + 이동 전용**: GPU 리소스를 소유하는 클래스는 복사 불가, 이동만 허용하여 핸들 이중 해제 방지
- **이름 기반 파이프라인**: 문자열 이름으로 파이프라인을 식별하고 생성/검색
- **셰이더 리플렉션 기반 자동 바인딩**: SPIR-V 리플렉션으로 디스크립터 레이아웃을 자동 수집
- **Façade 패턴**: `Renderer`가 렌더링 서브시스템 전체를 오케스트레이션

---

## 2. 아키텍처 계층도

```
┌─────────────────────────────────────────────────────────────┐
│                       Application                           │
│          (메인 루프, 윈도우 이벤트, 전체 초기화)                │
└──────┬──────────┬──────────┬──────────┬────────────────┬────┘
       │          │          │          │                │
   Window      Context   Swapchain  ShaderManager  GuiRenderer
  (GLFW)    (디바이스/큐)  (Present)  (셰이더 관리)   (ImGui)
                │                        │
         DescriptorPool             Shader × N
        (레이아웃 캐싱)          (SPIR-V + Reflect)
                │
  ┌─────────────┴──────────────────────────────────┐
  │                   Renderer                      │
  │        (렌더링 전체 오케스트레이션)                 │
  │  Pipeline · Image2D · ShadowMap · SkyTextures   │
  │  UniformBuffer · Sampler · ViewFrustum          │
  └─────────────────────────────────────────────────┘
       │              │               │
   Pipeline ×9    Image2D ×N     DescriptorSet
   (PSO 생성)   (텍스처/RT/DS)    (세트 빌더)
                     │
              ┌──────┴──────┐
         BarrierHelper  ResourceBinding
        (레이아웃 전환)  (디스크립터 링크)

  ┌──────────────────────────────────────────┐
  │                  Model                    │
  │  ┌────────┬──────────┬─────────────────┐ │
  │  │  Mesh  │ Material │    ModelNode     │ │
  │  │(버퍼)  │(PBR UBO) │  (씬 그래프)     │ │
  │  └────────┴──────────┴─────────────────┘ │
  │              Animation                    │
  │     (AnimationClip + Bone 트리)           │
  └──────────────────────────────────────────┘

  버퍼 계층:
  MappedBuffer ─→ UniformBuffer<T>  (템플릿 래퍼)
  StorageBuffer                     (SSBO, 독립)
```

---

## 3. 서브시스템별 분석

### 3.1 애플리케이션 라이프사이클

#### Application (`Application.h/cpp`)

**역할:** 메인 루프, 윈도우 이벤트 처리, 전체 Vulkan 리소스 초기화의 진입점

**핵심 클래스:**

```cpp
class Application {
    void run();                                    // 메인 루프
    void updateGui();                              // ImGui 업데이트
    void handleMouseMove(int32_t x, int32_t y);    // 마우스 입력

    // 소유 멤버
    Window window_;
    Context ctx_;
    Swapchain swapchain_;
    Camera camera_;
    vector<CommandBuffer> commandBuffers_;
    ShaderManager shaderManager_;
    vector<Model> models_;
    GuiRenderer guiRenderer_;
    Renderer renderer_;
};
```

**설정 구조체 (Application.h에 정의):**

| 구조체 | 역할 |
|--------|------|
| `ModelConfig` | 모델 파일 경로, 변환, 애니메이션 설정. 플루언트 인터페이스(`setName().setTransform()`) |
| `CameraConfig` | 위치, 회전, FOV. 프리셋 팩토리(`forBistro()`, `forHelmet()`) |
| `ApplicationConfig` | 모델 + 카메라 + 팩토리 메서드(`createDefault()`, `createGltfShowcase()`) |

**의존 관계:** 모든 핵심 시스템을 직접 소유하며 생명주기를 관리한다.

#### Window (`Window.h/cpp`)

**역할:** GLFW 윈도우 래퍼, 플랫폼 추상화

```cpp
class Window {
    auto createSurface(VkInstance) -> VkSurfaceKHR;
    auto getFramebufferSize() const -> VkExtent2D;
    auto getRequiredExtensions() -> vector<const char*>;
    bool isCloseRequested() const;
    void pollEvents();
    void setKeyCallback(GLFWkeyfun);
    void setMouseButtonCallback(...);
    void setCursorPosCallback(...);
};
```

**설계 포인트:** GLFW를 직접 노출하지 않고, Vulkan 서피스 생성과 입력 콜백을 위한 인터페이스만 제공한다.

---

### 3.2 디바이스/컨텍스트

#### Context (`Context.h/cpp`)

**역할:** Vulkan 디바이스, 큐, 커맨드 풀의 중앙 관리자. 엔진 전체의 GPU 접점.

```cpp
class Context {
    // 디바이스 접근
    auto device() -> VkDevice;
    auto physicalDevice() -> VkPhysicalDevice;
    auto instance() -> VkInstance;

    // 큐 접근 (그래픽스, 컴퓨트, 전송)
    auto graphicsQueue() const -> VkQueue;
    auto computeQueue() const -> VkQueue;
    auto transferQueue() const -> VkQueue;

    // 커맨드 버퍼 생성
    auto createGraphicsCommandBuffers(uint32_t numBuffers) -> vector<CommandBuffer>;
    auto createGraphicsCommandBuffer(VkCommandBufferLevel, bool begin) -> CommandBuffer;

    // 유틸리티
    auto getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags) -> uint32_t;
    auto descriptorPool() -> DescriptorPool&;
    auto getMaxUsableSampleCount() -> VkSampleCountFlagBits;
    void waitIdle();

    // 내부 구조체
    struct QueueFamilyIndices { uint32_t graphics, compute, transfer; };
};
```

**소유 멤버:**
- `VkInstance`, `VkPhysicalDevice`, `VkDevice`
- 큐 3종 (graphics, compute, transfer)과 커맨드 풀 3종
- `VkPipelineCache`
- `DescriptorPool` (직접 소유)

**의존 관계:** `CommandBuffer`, `DescriptorPool`, `VulkanTools`

**설계 포인트:** 엔진 내 거의 모든 클래스가 `Context&`를 생성자 인자로 받는다. Context가 GPU 리소스 접근의 단일 진입점 역할을 한다.

---

### 3.3 커맨드 버퍼 & 동기화

#### CommandBuffer (`CommandBuffer.h`)

**역할:** GPU 커맨드 버퍼의 RAII 래퍼

```cpp
class CommandBuffer {
    CommandBuffer(VkDevice&, VkCommandBuffer&, VkCommandPool&, VkQueue&, VkCommandBufferLevel);
    void submitAndWait();              // GPU 작업 동기 대기 (Fence 기반)
    auto handle() -> VkCommandBuffer&;
    auto queue() const -> VkQueue;
};
```

**설계 포인트:**
- **이동 전용 클래스** (복사 금지, 이동 생성자 구현)
- `submitAndWait()`는 Fence를 내부적으로 생성하고 대기 후 삭제하는 동기 방식
- 디바이스, 커맨드 풀, 큐 참조를 보유하여 자체적으로 제출 가능

#### BarrierHelper (`BarrierHelper.h`)

**역할:** 이미지 레이아웃 전환 자동화

```cpp
class BarrierHelper {
    void update(VkImage, VkFormat, uint32_t mipLevels, uint32_t arrayLayers);
    void transitionTo(VkCommandBuffer cmd, VkAccessFlags2 newAccess,
                      VkImageLayout newLayout, VkPipelineStageFlags2 newStage,
                      uint32_t baseMipLevel = 0, ...);
    auto prepareBarrier(VkImageLayout targetLayout, ...) -> VkImageMemoryBarrier2;
    auto currentLayout() -> VkImageLayout&;
};
```

**설계 포인트:**
- 현재 레이아웃 상태를 추적하여 **중복 전환을 자동으로 건너뜀**
- `Image2D`, `Swapchain`, `DepthStencil` 등에 합성으로 포함됨
- 이동 전용 클래스

---

### 3.4 스왑체인

#### Swapchain (`Swapchain.h/cpp`)

**역할:** 스왑체인 이미지/뷰 관리, Present 로직

```cpp
class Swapchain {
    Swapchain(Context&, VkSurfaceKHR, VkExtent2D& windowSize, bool vsync = false);
    auto acquireNextImage(VkSemaphore, uint32_t& imageIndex) -> VkResult;
    auto queuePresent(VkQueue, uint32_t imageIndex, VkSemaphore) -> VkResult;
    auto colorFormat() -> VkFormat;
    auto imageViews() -> vector<VkImageView>&;
    auto barrierHelper(uint32_t index) -> BarrierHelper&;
};
```

**의존 관계:** `Context`, `BarrierHelper`, `Logger`

**설계 포인트:** 각 스왑체인 이미지마다 `BarrierHelper`를 보유하여 레이아웃 전환을 자동 관리한다.

---

### 3.5 리소스 관리

> engine1에서는 리소스 기본 클래스 없이 각 리소스 타입이 독립적으로 존재한다.
> 공통 기능은 `ResourceBinding`과 `BarrierHelper`를 합성하여 공유한다.

#### Image2D (`Image2D.h/cpp`)

**역할:** 2D GPU 이미지 통합 관리 (텍스처, 렌더타겟, 스토리지 이미지, 깊이 버퍼)

```cpp
class Image2D {
    // 다양한 팩토리 스타일 생성 메서드
    void createFromPixelData(unsigned char* pixels, int w, int h, int c, bool sRGB);
    void createTextureFromKtx2(string filename, bool isCubemap);
    void createTextureFromImage(string filename, bool isCubemap, bool sRGB);
    void createRGBA32F(uint32_t width, uint32_t height);
    void createRGBA16F(uint16_t width, uint32_t height);
    void createMsaaColorBuffer(uint16_t width, uint32_t height, VkSampleCountFlagBits);
    void createGeneralStorage(uint16_t width, uint32_t height);

    // 레이아웃 전환 편의 메서드
    void transitionToColorAttachment(VkCommandBuffer cmd);
    void transitionToShaderRead(VkCommandBuffer cmd);
    void transitionToTransferSrc(VkCommandBuffer cmd);
    void transitionToTransferDst(VkCommandBuffer cmd);
    void transitionToGeneral(VkCommandBuffer cmd, ...);

    // 바인딩
    void setSampler(VkSampler sampler);
    auto resourceBinding() -> ResourceBinding&;
    auto barrierHelper() -> BarrierHelper&;
};
```

**설계 포인트:**
- 이동 전용 클래스
- `ResourceBinding`과 `BarrierHelper`를 합성(Composition)으로 소유
- 하나의 클래스가 텍스처, 렌더타겟, 깊이 버퍼, 스토리지 이미지를 모두 처리

#### MappedBuffer (`MappedBuffer.h/cpp`)

**역할:** CPU가 접근 가능한 GPU 버퍼 (UBO, Staging, Vertex, Index 용도 공용)

```cpp
class MappedBuffer {
    void create(VkBufferUsageFlags, VkMemoryPropertyFlags, VkDeviceSize size, void* data);
    void createVertexBuffer(VkDeviceSize size, void* data);
    void createIndexBuffer(VkDeviceSize size, void* data);
    void createStagingBuffer(VkDeviceSize size, void* data);
    void createUniformBuffer(VkDeviceSize size, void* data);
    void updateData(const void* data, VkDeviceSize size, VkDeviceSize offset);
    void flush() const;
    auto mapped() const -> void*;       // 영구 매핑된 CPU 포인터
    auto resourceBinding() -> ResourceBinding&;
};
```

**설계 포인트:**
- 이동 전용 클래스
- 영구 매핑(persistent mapping) 방식으로 CPU에서 직접 접근
- `ResourceBinding`을 합성으로 소유

#### UniformBuffer\<T\> (`UniformBuffer.h`)

**역할:** CPU 구조체를 GPU UBO에 동기화하는 템플릿 래퍼

```cpp
template <typename T_DATA>
class UniformBuffer {
    UniformBuffer(Context& ctx, T_DATA& cpuData);
    void updateData();                 // cpuData_를 GPU 버퍼에 memcpy
    auto cpuData() -> T_DATA&;
    auto resourceBinding() -> ResourceBinding&;
};
```

**설계 포인트:**
- `MappedBuffer`를 내부에 소유하고, `T_DATA&` 참조를 보유
- `updateData()` 호출 시 `T_DATA`의 현재 값을 자동으로 GPU에 업로드
- `static_assert(std::is_trivially_copyable_v<T_DATA>)` 제약

#### StorageBuffer (`StorageBuffer.h/cpp`)

**역할:** SSBO(Shader Storage Buffer Object) 관리

```cpp
class StorageBuffer {
    void create(VkDeviceSize size, VkBufferUsageFlags additionalUsage = 0);
    void* map();
    void unmap();
    void copyData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    auto getDescriptorInfo() const -> VkDescriptorBufferInfo;
};
```

#### ResourceBinding (`ResourceBinding.h`)

**역할:** 버퍼 또는 이미지를 디스크립터에 연결하는 데이터 컨테이너

```cpp
class ResourceBinding {
    void update();                     // descriptorType 자동 결정
    void setSampler(VkSampler sampler);
    auto barrierHelper() -> BarrierHelper&;

    // private 멤버 (friend 클래스들만 접근)
    VkImage image_;
    VkImageView imageView_;
    VkSampler sampler_;
    VkBuffer buffer_;
    VkDeviceSize bufferSize_;
    VkDescriptorType descriptorType_;
    BarrierHelper barrierHelper_;
};
```

**프렌드 클래스:** `DescriptorSet`, `Image2D`, `MappedBuffer`, `ShadowMap`

**설계 포인트:** 이미지와 버퍼를 동일한 인터페이스로 추상화하여 `DescriptorSet`에서 타입 구분 없이 처리할 수 있게 한다.

#### DepthStencil (`DepthStencil.h/cpp`)

**역할:** 깊이-스텐실 이미지 생성 및 관리

```cpp
class DepthStencil {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;            // 깊이+스텐실 뷰
    VkImageView samplerView;    // 셰이더 샘플링용 (깊이 전용)
    BarrierHelper barrierHelper_;
};
```

**설계 포인트:** public 멤버로 직접 접근을 허용하는 단순한 데이터 래퍼.

---

### 3.6 디스크립터 관리

#### DescriptorPool (`DescriptorPool.h/cpp`)

**역할:** 디스크립터 풀 생성, 레이아웃 캐싱, 온-디맨드 할당

```cpp
class DescriptorPool {
    void createFromScript();      // DescriptorPoolSize.txt에서 풀 크기 정의
    auto allocateDescriptorSet(const VkDescriptorSetLayout&) -> VkDescriptorSet;
    auto allocateDescriptorSets(const vector<VkDescriptorSetLayout>&) -> vector<VkDescriptorSet>;
    auto descriptorSetLayout(const vector<VkDescriptorSetLayoutBinding>&) -> const VkDescriptorSetLayout&;
    auto layoutsForPipeline(string pipelineName) -> vector<VkDescriptorSetLayout>;
    void printAllocatedStatistics() const;

    // 내부 구조체
    struct LayoutInfo {
        vector<VkDescriptorSetLayoutBinding> bindings_;
        vector<pair<string, uint32_t>> pipelineNamesAndSetNumbers_;
    };
};
```

**설계 포인트:**
- **레이아웃 캐싱**: 동일한 바인딩 벡터에 대해 `VkDescriptorSetLayout`을 재사용
- `Context`의 friend 클래스로, Context 생성 시 자동 초기화
- `ShaderManager`의 리플렉션 결과를 받아 파이프라인별 레이아웃을 자동 매핑

#### DescriptorSet (`DescriptorSet.h/cpp`)

**역할:** `ResourceBinding` 목록으로부터 디스크립터 세트를 빌드

```cpp
class DescriptorSet {
    void create(Context& ctx,
                const vector<reference_wrapper<ResourceBinding>>& resourceBindings);
    auto handle() const -> const VkDescriptorSet&;
};
```

**설계 포인트:** 빌더 패턴 — `ResourceBinding` 목록을 받아 레이아웃 검색 → 할당 → 갱신을 자동 처리한다.

---

### 3.7 셰이더 & 파이프라인

#### Shader (`Shader.h/cpp`)

**역할:** SPIR-V 셰이더 모듈 로딩 + `spirv-reflect` 기반 런타임 리플렉션

```cpp
class Shader {
    // private (ShaderManager에 friend로 노출)
    VkShaderModule shaderModule_;
    SpvReflectShaderModule reflectModule_;    // 바인딩/입력 정보 추출
    VkShaderStageFlagBits stage_;
};
```

**설계 포인트:**
- 이동 전용 클래스
- `ShaderManager`의 friend 클래스로, 외부에서는 직접 사용하지 않음
- 리플렉션 데이터로 디스크립터 레이아웃과 정점 입력을 자동 추출

#### ShaderManager (`ShaderManager.h/cpp`)

**역할:** 파이프라인 이름 → 셰이더 목록 매핑, 리플렉션 기반 레이아웃 수집

```cpp
class ShaderManager {
    ShaderManager(Context& ctx, string shaderPathPrefix,
                  const initializer_list<pair<string, vector<string>>>& pipelineShaders);

    auto createPipelineShaderStageCIs(string pipelineName) const
        -> vector<VkPipelineShaderStageCreateInfo>;
    auto pushConstantsRange(string pipelineName) -> VkPushConstantRange;
    auto createVertexInputAttrDesc(string pipelineName) const
        -> vector<VkVertexInputAttributeDescription>;
    auto collectPerPipelineBindings() const -> vector<VkDescriptorSetLayoutBinding>;
};
```

**핵심 멤버:** `unordered_map<string, vector<Shader>> pipelineShaders_`

**설계 포인트:**
- 파이프라인 이름과 셰이더 파일 목록을 `initializer_list`로 선언적 구성
- 셰이더 리플렉션으로 바인딩 정보를 자동 수집하여 `DescriptorPool`에 전달
- Push Constant 범위도 리플렉션으로 자동 결정

#### Pipeline (`Pipeline.h`, `Pipeline*.cpp`)

**역할:** PSO(Pipeline State Object) 생성 및 관리

```cpp
class Pipeline {
    Pipeline(Context& ctx, ShaderManager& shaderManager);

    // 이름 기반 생성 (전략 패턴)
    void createByName(string pipelineName, optional<VkFormat> colorFormat,
                      optional<VkFormat> depthFormat,
                      optional<VkSampleCountFlagBits> msaaSamples);

    // 개별 파이프라인 생성 (각 .cpp 파일에 분리 구현)
    void createPbrForward(VkFormat, VkFormat, VkSampleCountFlagBits);
    void createPbrDeferred();
    void createPost(VkFormat, VkFormat);
    void createGui(VkFormat);
    void createSky(VkFormat, VkFormat, VkSampleCountFlagBits);
    void createShadowMap();
    void createSsao();
    void createCompute();
    void createTriangle(VkFormat);

    auto pipeline() const -> VkPipeline;
    auto pipelineLayout() const -> VkPipelineLayout;
};
```

**파이프라인 구현 파일 분리:**

| 파일 | 파이프라인 종류 |
|------|----------------|
| `Pipeline.cpp` | 공통 로직 + `createByName()` 라우팅 |
| `PipelinePbrForward.cpp` | PBR Forward 렌더링 |
| `PipelinePbrDeferred.cpp` | PBR Deferred 렌더링 |
| `PipelinePost.cpp` | 포스트 프로세싱 |
| `PipelineGui.cpp` | ImGui UI |
| `PipelineSky.cpp` | 스카이박스/IBL |
| `PipelineShadowMap.cpp` | 그림자 맵 |
| `PipelineSsao.cpp` | SSAO |
| `PipelineCompute.cpp` | 컴퓨트 |
| `PipelineTriangle.cpp` | 기본 삼각형 (테스트) |

**설계 포인트:**
- 이동 전용 클래스
- `createByName()`이 문자열 이름으로 적절한 `createXxx()` 메서드를 라우팅 (전략 패턴)
- 파이프라인 종류별로 `.cpp` 파일을 분리하여 코드 관리 용이

---

### 3.8 씬 & 지오메트리

#### Model (`Model.h/cpp`)

**역할:** 3D 모델의 메시, 재질, 텍스처, 애니메이션 통합 컨테이너

```cpp
class Model {
    Model(Context& ctx);
    void loadFromModelFile(const string& modelFilename, bool readBistroObj);
    void createVulkanResources();
    void createDescriptorSets(Sampler& sampler, Image2D& dummyTexture);

    // 애니메이션 제어
    void updateAnimation(float deltaTime);
    void playAnimation() / pauseAnimation() / stopAnimation();
    void setAnimationIndex(uint32_t index);
    void setAnimationSpeed(float speed);
    const vector<mat4>& getBoneMatrices() const;

    // 접근자
    auto meshes() -> vector<Mesh>&;
    auto materials() -> vector<Material>&;
    auto modelMatrix() -> mat4&;
    auto visible() -> bool&;
    vec3 boundingBoxMin() const / boundingBoxMax() const;
};
```

**소유 리소스:**
- `vector<Mesh> meshes_`
- `vector<Material> materials_`
- `vector<Image2D> textures_`
- `unique_ptr<ModelNode> rootNode_` (씬 그래프 루트)
- `unique_ptr<Animation> animation_`
- `vector<UniformBuffer<MaterialUBO>> materialUBO_`
- `vector<DescriptorSet> materialDescriptorSets_`

**설계 포인트:**
- `ModelLoader`의 friend 클래스로, 로딩 시 내부 데이터를 직접 채움
- 이동 전용 클래스
- 바운딩 박스를 자동 계산하여 프러스텀 컬링에 활용

#### ModelNode (`ModelNode.h`)

**역할:** 씬 그래프 트리 노드

```cpp
struct ModelNode {
    string name;
    mat4 localMatrix, worldMatrix;
    vector<uint32_t> meshIndices;           // 부모 Model의 meshes_ 인덱스
    vector<unique_ptr<ModelNode>> children; // 소유권 보장
    ModelNode* parent;                      // non-owning 역참조
    vec3 translation; quat rotation; vec3 scale;

    void updateLocalMatrix();
    void updateWorldMatrix(const mat4& parentMatrix = mat4(1.0f));
    ModelNode* findNode(const string& name);
};
```

**설계 포인트:** `unique_ptr<ModelNode>` children으로 트리 소유권을 보장하며, `findNode()`으로 이름 기반 재귀 검색을 지원한다.

#### Mesh (`Mesh.h/cpp`)

**역할:** 정점/인덱스 배열, GPU 버퍼, AABB 컬링 데이터

```cpp
class Mesh {
    void createBuffers(Context& ctx);
    void cleanup(VkDevice device);
    void calculateBounds();
    void updateWorldBounds(const glm::mat4& modelMatrix);

    // 바이너리 캐시 I/O
    bool readFromBinaryFileStream(std::ifstream& stream);
    bool writeToBinaryFileStream(std::ofstream& stream) const;

    // Public 멤버
    string name_;
    vector<Vertex> vertices_;
    vector<uint32_t> indices_;
    uint32_t materialIndex_;
    VkBuffer vertexBuffer_, indexBuffer_;
    vec3 minBounds, maxBounds;
    AABB worldBounds;
    bool isCulled;
};
```

**설계 포인트:**
- 바이너리 파일 캐시로 로딩 최적화
- `AABB worldBounds`로 프러스텀 컬링 지원
- 이동 전용 클래스

#### Material (`Material.h`)

**역할:** PBR 재질 데이터

```cpp
struct MaterialUBO {
    vec4 emissiveFactor_, baseColorFactor_;
    float roughness_, transparencyFactor_, discardAlpha_, metallicFactor_;
    int baseColorTextureIndex_, emissiveTextureIndex_, normalTextureIndex_;
    int opacityTextureIndex_, metallicRoughnessTextureIndex_, occlusionTextureIndex_;
};

// 플래그 열거형
enum MaterialFlags { sCastShadow, sReceiveShadow, sTransparent };
```

#### Vertex (`Vertex.h`)

**역할:** 정점 데이터 레이아웃 (스켈레탈 애니메이션 포함)

```cpp
struct Vertex {  // 88 bytes, 모두 32비트 float
    alignas(4) vec3 position;       // 12 bytes
    alignas(4) vec3 normal;         // 12 bytes
    alignas(4) vec2 texCoord;       //  8 bytes
    alignas(4) vec3 tangent;        // 12 bytes
    alignas(4) vec3 bitangent;      // 12 bytes
    alignas(4) vec4 boneWeights;    // 16 bytes (최대 4 bone)
    alignas(4) ivec4 boneIndices;   // 16 bytes

    void addBoneData(uint32_t boneIndex, float weight);
    void normalizeBoneWeights();
    static vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    static VkVertexInputBindingDescription getBindingDescription();
};
```

#### ModelLoader (`ModelLoader.h/cpp`)

**역할:** Assimp를 사용하여 파일로부터 `Model` 데이터 채우기

```cpp
class ModelLoader {
    ModelLoader(Model& model);
    void loadFromModelFile(const string& modelFilename, bool readBistroObj);
    void loadFromCache(const string& cacheFilename);
    void writeToCache(const string& cacheFilename);
    void processNode(aiNode*, const aiScene*, ModelNode* parent);
    void processMesh(aiMesh*, const aiScene*, uint32_t meshIndex);
    void processMaterial(aiMaterial*, const aiScene*, uint32_t materialIndex);
};
```

**설계 포인트:** `Model`의 friend 클래스로 내부 데이터에 직접 접근. 바이너리 캐시로 재로딩 최적화.

---

### 3.9 카메라

#### Camera (`Camera.h/cpp`)

**역할:** FPS 모드와 LookAt 모드를 지원하는 카메라

```cpp
class Camera {
    enum CameraType { lookat, firstperson };

    void setPerspective(float fov, float aspect, float znear, float zfar);
    void setPosition(glm::vec3);
    void setRotation(glm::vec3);
    void rotate(glm::vec3 delta);
    void translate(glm::vec3 delta);
    void update(float deltaTime);

    struct { glm::mat4 perspective, view; } matrices;
    struct { bool left, right, forward, backward, up, down; } keys;
};
```

**설계 포인트:**
- `matrices.perspective`와 `matrices.view`를 직접 public으로 노출
- `keys` 구조체로 입력 상태를 플래그로 관리
- `update()` 호출 시 입력 상태에 따라 위치/회전 갱신

---

### 3.10 렌더링 오케스트레이션

#### Renderer (`Renderer.h/cpp`)

**역할:** 렌더링 시스템의 고수준 오케스트레이터 (Façade)

```cpp
class Renderer {
    Renderer(Context& ctx, ShaderManager& shaderManager,
             const uint32_t& kMaxFramesInFlight,
             const string& kAssetsPathPrefix, const string& kShaderPathPrefix);

    // 초기화
    void prepareForModels(vector<Model>& models, VkFormat colorFormat,
                          VkFormat depthFormat, VkSampleCountFlagBits msaaSamples,
                          uint32_t w, uint32_t h);
    void createPipelines(VkFormat, VkFormat, VkSampleCountFlagBits);
    void createTextures(uint32_t w, uint32_t h, VkSampleCountFlagBits);
    void createUniformBuffers();

    // 프레임 갱신
    void update(Camera& camera, uint32_t currentFrame, double time);
    void updateBoneData(const vector<Model>& models, uint32_t currentFrame);

    // 드로우
    void draw(VkCommandBuffer cmd, uint32_t currentFrame, VkImageView swapchainImageView,
              vector<Model>& models, VkViewport viewport, VkRect2D scissor);
    void makeShadowMap(VkCommandBuffer cmd, uint32_t currentFrame, vector<Model>& models);

    // 컬링
    void performFrustumCulling(vector<Model>& models);
    void updateViewFrustum(const glm::mat4& viewProjection);

    // UBO 접근
    auto sceneUBO() -> SceneUniform&;
    auto optionsUBO() -> OptionsUniform&;
};
```

**소유 리소스:**

| 카테고리 | 리소스 |
|----------|--------|
| 이미지 | `msaaColorBuffer_`, `depthStencil_`, `msaaDepthStencil_`, `forwardToCompute_`, `computeToPost_` |
| IBL | `SkyTextures skyTextures_` |
| 그림자 | `ShadowMap shadowMap_` |
| 샘플러 | `samplerLinearRepeat_`, `samplerLinearClamp_`, `samplerAnisoRepeat_`, `samplerAnisoClamp_` |
| 파이프라인 | `unordered_map<string, Pipeline> pipelines_` |
| 유니폼 | `vector<UniformBuffer<SceneUniform>>`, `vector<UniformBuffer<OptionsUniform>>`, `vector<UniformBuffer<BoneDataUniform>>` |
| 컬링 | `ViewFrustum viewFrustum_` |

**GPU 유니폼 구조체 (Renderer.h에 정의):**

```cpp
struct SceneUniform {
    mat4 projection, view;
    vec4 cameraPos, lightDir;
    mat4 lightSpaceMatrix;
};

struct OptionsUniform {
    int textureOn, shadowOn;
    float ssaoRadius, ssaoBias;
    // ...
};

struct BoneDataUniform {
    mat4 boneMatrices[256];    // 최대 256개 본
};

struct PostOptionsUBO {
    int toneMappingType;
    float exposure, gamma, vignette, filmGrain;
    // ...
};
```

---

### 3.11 유틸리티

#### Logger (`Logger.h`)

**역할:** 싱글톤 로거 (`log.txt` + stdout 동시 출력)

```cpp
class Logger {
    static Logger& getInstance();
    static void printLog(string message);
};

// 자유 함수 (std::format 기반)
template<typename... Args>
void printLog(std::format_string<Args...> fmt, Args&&...);

template<typename... Args>
void exitWithMessage(std::format_string<Args...> fmt, Args&&...);  // 오류 시 종료
```

#### Sampler (`Sampler.h/cpp`)

**역할:** 텍스처 샘플러 생성

```cpp
class Sampler {
    void createAnisoRepeat();     // 이방성 필터링 + 반복
    void createAnisoClamp();      // 이방성 필터링 + 클램프
    void createLinearRepeat();    // 선형 필터링 + 반복
    void createLinearClamp();     // 선형 필터링 + 클램프
    auto handle() const -> VkSampler;
};
```

#### PushConstants\<T\> (`PushConstants.h`)

**역할:** 타입-안전 Push Constant 래퍼

```cpp
template <typename T_DATA>
class PushConstants {
    PushConstants(Context& ctx);
    T_DATA& data();
    void push(VkCommandBuffer cmd, VkPipelineLayout layout);
    VkPushConstantRange getPushConstantRange();
    void setStageFlags(VkShaderStageFlags stageFlags);
};
```

#### ViewFrustum (`ViewFrustum.h`)

**역할:** 뷰 프러스텀 평면 추출 및 AABB 가시성 판단

```cpp
struct Plane {
    vec3 normal;
    float distance;
};

struct AABB {
    vec3 min, max;
    vec3 getCenter() const;
    vec3 getExtents() const;
    AABB transform(const mat4& matrix) const;
};

class ViewFrustum {
    void extractFromViewProjection(const glm::mat4& viewProjection);
    bool intersects(const AABB& aabb) const;
    bool contains(const glm::vec3& point) const;
};
```

#### ShadowMap (`ShadowMap.h/cpp`)

**역할:** 그림자 맵 이미지/뷰/샘플러 생성

```cpp
class ShadowMap {
    ShadowMap(Context& ctx);  // 4096×4096, D16_UNORM, 비교 샘플러
    auto resourceBinding() -> ResourceBinding&;
};
```

#### SkyTextures (`SkyTextures.h/cpp`)

**역할:** IBL(Image-Based Lighting) 텍스처 세트 관리

```cpp
class SkyTextures {
    SkyTextures(Context& ctx);
    void loadKtxMaps(const string& prefilteredFilename,
                     const string& irradianceFilename,
                     const string& brdfLutFileName);
    auto prefiltered() -> Image2D&;   // Specular 환경맵
    auto irradiance() -> Image2D&;    // Diffuse 방사조도맵
    auto brdfLUT() -> Image2D&;       // BRDF 룩업 텍스처
};
```

#### Animation (`Animation.h/cpp`)

**역할:** Assimp 기반 스켈레탈 애니메이션 처리

```cpp
class Animation {
    void loadFromScene(const aiScene* scene);
    void updateAnimation(float timeInSeconds);
    void setAnimationIndex(uint32_t index);
    const vector<mat4>& getBoneMatrices() const;
    void play() / pause() / stop();
    bool isPlaying() const;
};
```

**내부 데이터 구조:**

| 구조체 | 역할 |
|--------|------|
| `AnimationClip` | 이름, 시간, `vector<AnimationChannel>` |
| `AnimationChannel` | 노드 이름, 위치/회전/스케일 키프레임 |
| `AnimationKey<T>` | 타임스탬프 + 값 (위치/회전/스케일) |
| `Bone` | 이름, ID, 오프셋 행렬, 최종 변환, 부모 인덱스 |
| `SceneNode` | 애니메이션용 씬 그래프 (Model의 ModelNode와 별도) |

---

## 4. 설계 패턴 요약

| 패턴 | 적용 클래스 | 목적 |
|------|-------------|------|
| **RAII + 이동 전용** | `CommandBuffer`, `Image2D`, `MappedBuffer`, `Shader`, `Pipeline`, `Model`, `Mesh` | GPU 핸들의 이중 해제 방지, 소유권 명확화 |
| **합성(Composition)** | `Image2D` ← `ResourceBinding` ← `BarrierHelper`, `UniformBuffer<T>` ← `MappedBuffer` | 상속 없이 기능 조합 |
| **템플릿 파라미터화** | `UniformBuffer<T>`, `PushConstants<T>`, `AnimationKey<T>` | 타입-안전 GPU 데이터 전송 |
| **Façade** | `Renderer`, `Application` | 복잡한 서브시스템을 단순한 인터페이스로 캡슐화 |
| **빌더(Builder)** | `DescriptorSet::create()` | 바인딩 목록 → 레이아웃 검색 → 할당 → 갱신 자동화 |
| **전략(Strategy)** | `Pipeline::createByName()` | 문자열 이름으로 파이프라인 생성 방식 선택 |
| **싱글톤** | `Logger` | 전역 로깅 접근점 |
| **플루언트 인터페이스** | `ModelConfig`, `CameraConfig` | 메서드 체이닝으로 설정 구성 |
| **팩토리 메서드** | `ApplicationConfig::createDefault()`, `CameraConfig::forBistro()` | 사전 정의된 설정 프리셋 |
| **Friend 접근** | `ModelLoader` ↔ `Model`, `ShaderManager` ↔ `Shader`, `DescriptorSet` ↔ `ResourceBinding` | 캡슐화 유지하면서 특정 클래스에만 내부 접근 허용 |
| **레이아웃 캐싱** | `DescriptorPool` | 동일 바인딩의 디스크립터 세트 레이아웃 재사용 |
| **셰이더 리플렉션** | `Shader` + `ShaderManager` | 바인딩/입력 정보 자동 추출로 수동 설정 제거 |

---

## 5. 클래스 의존성 다이어그램

```
                        Application
                       /    |    \     \        \
                      /     |     \     \        \
                 Window  Context  Swapchain  Camera  GuiRenderer
                          / |  \                      /   |   \
                         /  |   \                Pipeline Image2D MappedBuffer
                        /   |    \
              CmdBuffer  DPool  VulkanTools
                          |
                      DescriptorSet ←── ResourceBinding ←── BarrierHelper
                          |                    ↑     ↑
                          |              Image2D  MappedBuffer
                          |
                       Renderer ──────────────────────────────────────────┐
                      / | | | \                                           |
                     /  | | |  \                                          |
              Pipeline  | | |  UniformBuffer<T>                    ViewFrustum
                        | | |       |
                  Image2D | |   MappedBuffer
                          | |
                   ShadowMap SkyTextures
                          |       |
                    ResourceBinding  Image2D × 3

                        Model
                       / | | \
                      /  | |  \
                   Mesh  | |  ModelNode (트리)
                         | |
                  Material Animation
                         |     |
                   MaterialUBO  AnimationClip
                               Bone (계층)

         ShaderManager ──→ Shader × N ──→ spirv-reflect
              |
              └──→ DescriptorPool (바인딩 정보 전달)

         ModelLoader ──→ Model (friend)
              |
              └──→ assimp
```

**핵심 의존성 흐름:**

```
Application → Context (모든 GPU 리소스의 근원)
           → Renderer (렌더링 전체 조율)
           → ShaderManager (셰이더 + 리플렉션)

Context.DescriptorPool ←── ShaderManager (리플렉션 바인딩)
                       ←── DescriptorSet (할당 요청)

Image2D / MappedBuffer → ResourceBinding → DescriptorSet
                       → BarrierHelper (레이아웃 전환)

Model → ModelLoader (로딩) → Mesh + Material + Animation
      → Image2D (텍스처)
      → UniformBuffer<MaterialUBO> (재질 UBO)
      → DescriptorSet (재질별 바인딩)
```

---

## 6. 외부 의존성

| 라이브러리 | 사용처 | 역할 |
|------------|--------|------|
| Vulkan 1.3 | 엔진 전반 | 그래픽스 API |
| GLFW | `Window` | 윈도우/입력 관리 |
| GLM | `Camera`, `Vertex`, `Animation`, `ViewFrustum` | SIMD 수학 |
| Assimp | `ModelLoader`, `Animation` | 3D 모델 파일 로딩 |
| spirv-reflect | `Shader`, `ShaderManager` | 셰이더 리플렉션 |
| ImGui | `GuiRenderer` | 디버그 UI |
| stb_image | `Image2D` | 이미지 파일 로딩 |
| KTX2 | `Image2D` | KTX2 텍스처 로딩 |
