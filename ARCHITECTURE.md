# kcShaders 架构文档

## 项目概述

kcShaders 是一个基于 OpenGL 4.3+ 的现代化实时渲染引擎，支持多种渲染模式，包括传统光栅化、实时光线追踪（基于 Compute Shader）和 Shadertoy 着色器。项目采用模块化设计，易于扩展和维护。

**核心特性**：
- 多渲染管线架构（Forward/Deferred/RayTracing/Shadertoy）
- OpenUSD 场景加载与渲染
- 实时着色器热重载
- BVH 加速结构
- Temporal Accumulation 降噪
- 插值法线（Shading Normal）

---

## 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                         Application                          │
│                        (GUI + App)                           │
└──────────────┬──────────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────────┐
│                         Renderer                             │
│  - Pipeline Management (Forward/Deferred/RT/Shadertoy)       │
│  - Resource Management (FBO, VAO, VBO)                       │
└──────────────┬──────────────────────────────────────────────┘
               │
     ┌─────────┴─────────┬──────────┬───────────┐
     ▼                   ▼          ▼           ▼
┌──────────┐     ┌──────────┐  ┌─────────┐  ┌─────────────┐
│ Forward  │     │ Deferred │  │Shadertoy│  │ RayTracing  │
│ Pipeline │     │ Pipeline │  │Pipeline │  │  Pipeline   │
└──────────┘     └──────────┘  └─────────┘  └─────────────┘
     │                   │                          │
     │                   ▼                          ▼
     │            ┌──────────┐             ┌────────────────┐
     │            │ GBuffer  │             │ BVH + SSBO     │
     │            │  Pass    │             │ Compute Shader │
     │            └──────────┘             └────────────────┘
     │                   │
     ▼                   ▼
┌─────────────────────────────────┐
│          Scene Graph            │
│  - Nodes (Transform Hierarchy)  │
│  - Meshes (Geometry Data)       │
│  - Materials (PBR Properties)   │
│  - Lights (Point/Directional)   │
│  - Camera (View + Projection)   │
└─────────────────────────────────┘
```

---

## 目录结构

```
kcShaders/
├── src/
│   ├── main.cpp                    # 程序入口
│   ├── graphics/                   # 渲染核心
│   │   ├── renderer.h/cpp          # 渲染器主类（管理所有管线）
│   │   ├── ShaderProgram.h/cpp     # 着色器封装
│   │   ├── BVH.h/cpp               # BVH 加速结构
│   │   ├── gbuffer.h/cpp           # G-Buffer（延迟渲染）
│   │   ├── MaterialBinder.h/cpp    # 材质绑定工具
│   │   ├── RenderContext.h         # 渲染上下文（Camera, Scene, 时间等）
│   │   ├── RenderPass.h            # 渲染 Pass 基类
│   │   ├── pipeline/               # 渲染管线实现
│   │   │   ├── RenderPipeline.h            # 管线基类（接口）
│   │   │   ├── ForwardPipeline.h/cpp       # 前向渲染
│   │   │   ├── DeferredPipeline.h/cpp      # 延迟渲染
│   │   │   ├── ShadertoyPipeline.h/cpp     # Shadertoy 兼容
│   │   │   └── RayTracingPipeline.h/cpp    # 光线追踪（Compute Shader）
│   │   └── passes/                 # 渲染 Pass 实现
│   │       ├── GBufferPass.h/cpp           # G-Buffer 几何 Pass
│   │       ├── SSAOPass.h/cpp              # SSAO 计算与模糊 Pass
│   │       └── LightingPass.h/cpp          # 延迟光照 Pass
│   ├── scene/                      # 场景管理
│   │   ├── scene.h/cpp             # 场景图（树形结构）
│   │   ├── camera.h/cpp            # 相机（Z-up, FPS 控制）
│   │   ├── mesh.h/cpp              # 网格数据（顶点、索引、法线）
│   │   ├── material.h/cpp          # PBR 材质
│   │   ├── light.h/cpp             # 光源（点光源、方向光）
│   │   ├── texture.h/cpp           # 纹理加载
│   │   ├── geometry.h/cpp          # 几何体生成（Cube, Sphere 等）
│   │   └── demo_scene.h            # 演示场景
│   ├── loaders/                    # 资源加载器
│   │   ├── obj_loader.h/cpp        # Wavefront OBJ 加载
│   │   └── usd_loader.h/cpp        # OpenUSD 场景加载
│   ├── gui/                        # 用户界面
│   │   ├── app.h/cpp               # 应用程序主类（ImGui 界面）
│   │   └── imfilebrowser.h         # 文件浏览器
│   └── shaders/                    # GLSL 着色器
│       ├── default.vert/frag               # 前向渲染着色器
│       ├── deferred/                       # 延迟渲染着色器目录
│       │   ├── geometry.vert/frag          # 几何 Pass
│       │   ├── lighting.vert/frag          # 光照 Pass
│       │   ├── ssao.vert/frag              # SSAO 计算
│       │   └── ssao_blur.vert/frag         # SSAO 模糊
│       ├── shadertoy.vert                  # Shadertoy 顶点着色器
│       └── raytracing/                     # 光线追踪着色器
│           ├── default.comp                # 默认 RT 着色器（BVH 遍历）
│           ├── demo.comp                   # 演示场景（球体）
│           ├── display.vert/frag           # RT 结果显示
│           └── ...
├── external/                       # 第三方库
│   ├── glad/                       # OpenGL 加载器
│   ├── glfw/                       # 窗口管理
│   ├── glm/                        # 数学库
│   ├── imgui/                      # GUI 库
│   ├── stb/                        # 图像加载
│   └── ...
├── CMakeLists.txt                  # CMake 构建脚本
└── README.md                       # 项目说明
```

---

## 核心模块详解

### 1. **Renderer（渲染器）**

**职责**：
- 管理所有渲染管线的生命周期
- 提供统一的渲染接口
- 管理 OpenGL 资源（FBO, VAO, VBO）
- 处理窗口 resize 事件

**关键方法**：
```cpp
void render(Scene* scene, Camera* camera);
void setRenderingMode(RenderingMode mode);
bool loadForwardShaders(...);
bool loadDeferredShaders(...);
bool loadShadertoyShaders(...);
bool loadRayTracingShaders(...);
```

**设计模式**：
- **策略模式**：通过切换不同的 `RenderPipeline` 实现不同的渲染策略
- **工厂模式**：在 `initialize()` 中创建所有管线实例

---

### 2. **RenderPipeline（渲染管线基类）**

**接口定义**：
```cpp
class RenderPipeline {
public:
    virtual bool initialize() = 0;
    virtual void execute(RenderContext& ctx) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void cleanup() = 0;
    virtual const char* getName() const = 0;
};
```

**派生类**：

#### a) **ForwardPipeline（前向渲染）**
- **单 Pass 渲染**：几何 + 光照一次完成
- **适用场景**：简单场景、透明物体
- **着色器**：`default.vert/frag`

#### b) **DeferredPipeline（延迟渲染）**
- **多 Pass 架构**：
  1. **GBufferPass**：渲染几何信息到 G-Buffer（位置、法线、颜色、深度）
  2. **SSAOPass**（可选）：计算屏幕空间环境光遮蔽
  3. **LightingPass**：使用 G-Buffer 计算光照，应用 SSAO
- **优势**：高效处理多光源场景，支持后处理效果
- **着色器**：
  - 几何：`deferred/geometry.vert/frag`
  - 光照：`deferred/lighting.vert/frag`
  - SSAO：`deferred/ssao.vert/frag`, `deferred/ssao_blur.vert/frag`

#### c) **ShadertoyPipeline（Shadertoy 兼容）**
- **自动包装**：将用户的 `mainImage(out vec4, in vec2)` 函数包装为标准 OpenGL 着色器
- **标准 Uniform**：提供 `iResolution`, `iTime`, `iFrame`, `iMouse` 等
- **着色器**：`shadertoy.vert` + 用户 fragment 代码

#### d) **RayTracingPipeline（光线追踪）**
- **Compute Shader 实现**：使用 OpenGL 4.3 Compute Shader 进行 GPU 光线追踪
- **核心技术**：
  - **BVH 加速结构**：CPU 构建，GPU 遍历
  - **SSBO 数据传输**：顶点、三角形、BVH 节点、材质
  - **Temporal Accumulation**：帧间累积降噪
  - **Shading Normal**：插值顶点法线（重心坐标）
- **双纹理系统**：
  - `outputTexture_`：当前帧渲染结果
  - `accumulationTexture_`：累积的历史帧
- **着色器**：`raytracing/*.comp`, `display.vert/frag`

---

### 3. **Scene Graph（场景图）**

**层次结构**：
```cpp
Scene
 └─ SceneNode (root)
     ├─ SceneNode (child1)
     │   ├─ Mesh*
     │   ├─ Material*
     │   └─ Transform (position, rotation, scale)
     └─ SceneNode (child2)
         └─ ...
```

**关键类**：

#### **Scene**
- 管理场景树的根节点
- 收集所有可渲染对象：`collectRenderItems(vector<RenderItem>&)`
- 提供 `addLight()`, `getLights()` 接口

#### **SceneNode**
- 树形结构节点（父节点 + 子节点列表）
- 存储 Transform（位置、旋转、缩放）
- 可选绑定 Mesh 和 Material

#### **RenderItem**
- 扁平化的渲染数据结构：
  ```cpp
  struct RenderItem {
      Mesh* mesh;
      Material* material;
      glm::mat4 modelMatrix;  // 世界空间变换矩阵
  };
  ```

---

### 4. **Camera（相机系统）**

**坐标系统**：
- **Z-up 世界坐标系**（+Z 向上，+X 向右，+Y 向前）
- **FPS 控制**：
  - `up_` 始终为 `(0, 0, 1)`
  - `yaw` 控制水平旋转
  - `pitch` 只影响视线方向，不影响移动平面

**关键方法**：
```cpp
glm::mat4 GetViewMatrix();           // 视图矩阵
glm::mat4 GetProjectionMatrix();     // 投影矩阵
void ProcessKeyboard(movement, deltaTime);
void ProcessMouseMovement(xoffset, yoffset);
```

**射线生成**（Ray Tracing）：
```glsl
vec3 rayDir = normalize(
    cameraFront + 
    uv.x * halfWidth * cameraRight + 
    uv.y * halfHeight * cameraUp
);
```

---

### 5. **BVH（层次包围盒）**

**数据结构**：
```cpp
struct BVHNode {
    glm::vec3 boundsMin;
    uint32_t leftFirst;   // 左子节点索引 / 三角形起始索引
    glm::vec3 boundsMax;
    uint32_t triCount;    // 0=内部节点, >0=叶子节点
};
```

**构建流程**：
1. **CPU 端**（`BVHBuilder::build()`）：
   - 计算所有三角形的 AABB 和质心
   - 使用 **SAH（Surface Area Heuristic）** 递归分割
   - 重排三角形索引以提高缓存一致性
2. **GPU 端**（`intersectBVH()` in shader）：
   - 栈式遍历（无递归）
   - 先测试 AABB，再测试三角形
   - 返回最近交点

**性能优化**：
- 三角形重排序：叶子节点的三角形在数组中连续存储
- 早期剔除：AABB 测试失败立即跳过整个子树

---

### 6. **Material System（材质系统）**

**CPU 端**（`Material` 类）：
```cpp
class Material {
    glm::vec3 albedo;        // 基础颜色
    float metallic;          // 金属度
    float roughness;         // 粗糙度
    float ao;                // 环境光遮蔽
    glm::vec3 emissive;      // 自发光
    float emissiveStrength;  // 自发光强度
    float opacity;           // 不透明度
    // 纹理 ID...
};
```

**GPU 端**（`GpuMaterial` 结构）：
```cpp
struct GpuMaterial {
    vec3 albedo;
    float metallic;
    vec3 emissive;
    float roughness;
    float ao;
    float opacity;
    float emissiveStrength;
    float _pad0;  // std430 对齐
};
```

**上传流程**：
1. 遍历 `RenderItem`，收集所有唯一材质
2. 构建 `Material* → uint32_t` 映射表
3. 打包到 `vector<GpuMaterial>`
4. 上传到 SSBO（binding = 4）

---

### 7. **SSAO（屏幕空间环境光遮蔽）**

**技术原理**：
- 在屏幕空间中，根据像素周围的几何关系估算被遮挡程度
- 凹陷区域（如墙角、缝隙）会接收到更少的环境光

**实现细节**（`SSAOPass`）：

#### Pass 1: SSAO 计算
```glsl
// 输入：G-Buffer 的 Position、Normal
// 输出：单通道 AO 值（RED format）

1. 读取片段的视空间位置和法线
2. 生成随机旋转向量（4x4 噪声纹理）
3. 构建 TBN 矩阵（Tangent-Bitangent-Normal）
4. 在半球内采样 N 个点（默认 32）
5. 将采样点投影到屏幕空间，查询深度
6. 如果采样点在表面后方 → 计入遮蔽
7. 归一化并应用幂次曲线（artistic control）
```

#### Pass 2: SSAO 模糊
```glsl
// 输入：原始 SSAO 纹理
// 输出：模糊后的 SSAO 纹理

简单 4x4 box blur 减少噪声
```

#### 集成到 Lighting Pass
```glsl
// 在计算最终颜色时：
vec3 ambient = ambientLight * albedo * ao * ssao;
vec3 color = ambient + Lo;  // Lo = 直接光照
```

**可调参数**：
- `radius`：采样半径（视空间单位）
- `bias`：深度偏移，防止自遮挡伪影
- `power`：幂次曲线，控制暗化程度
- `sampleCount`：采样数量（8-64）

**性能特点**：
- 与场景复杂度无关（屏幕空间算法）
- 成本：两个 fullscreen pass + 32 次纹理采样
- 典型性能：~2-5ms @ 1080p

---

### 8. **Shader Hot Reload（着色器热重载）**

**监控机制**：
- 使用 `std::filesystem::last_write_time()` 轮询文件修改时间
- 在 `App::Update()` 中每帧检查
- 检测到变化后调用对应管线的 `loadShaders()`

**容错设计**：
- 编译失败不崩溃：保留旧的有效着色器
- 日志输出错误信息到控制台
- GUI 显示当前着色器路径

---

## 渲染流程详解

### Forward Rendering（前向渲染）
```
1. Clear framebuffer
2. For each RenderItem:
   a. Bind material (textures, uniforms)
   b. Set model matrix
   c. Draw mesh
3. Display result
```

### Deferred Rendering（延迟渲染）
```
1. GBufferPass:
   a. Bind G-Buffer FBO
   b. For each RenderItem:
      - Write position, normal, albedo to G-Buffer
2. LightingPass:
   a. Bind screen FBO
   b. Bind G-Buffer textures
   c. For each light:
      - Accumulate lighting contribution
   d. Draw fullscreen quad
```

### Ray Tracing（光线追踪）
```
1. Check camera movement → Reset accumulation if moved
2. Compute Shader Dispatch:
   a. Bind SSBO (vertices, triangles, BVH, materials)
   b. Bind output texture (write-only)
   c. Bind accumulation texture (read-write)
   d. Set uniforms (camera, resolution, frame count)
   e. Dispatch compute: (width/16) × (height/16) groups
   f. Each thread:
      - Generate ray from camera
      - Traverse BVH
      - Compute shading (interpolated normals)
      - Mix with previous accumulation: mix(prev, current, 1.0/frameCount)
3. Display Pass:
   a. Bind screen FBO
   b. Draw fullscreen quad with output texture
```

---

## 数据流图

### Scene → GPU Pipeline

```
┌─────────────┐
│ USD/OBJ File│
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ USD Loader  │ ← Parse scene hierarchy, materials, meshes
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────┐
│          Scene Graph            │
│  - SceneNode (Transform Tree)   │
│  - Mesh (Vertex/Index Data)     │
│  - Material (PBR Properties)    │
└──────────┬──────────────────────┘
           │
           │ collectRenderItems()
           ▼
┌─────────────────────────────────┐
│   vector<RenderItem>            │
│   { mesh, material, modelMat }  │
└──────────┬──────────────────────┘
           │
           │ [Ray Tracing Path]
           ▼
┌─────────────────────────────────┐
│  RayTracingPipeline::upload()   │
│  1. Transform vertices to world │
│  2. Build BVH (CPU)             │
│  3. Reorder triangles           │
│  4. Pack materials              │
└──────────┬──────────────────────┘
           │
           ▼
┌─────────────────────────────────┐
│        GPU SSBOs                │
│  Binding 1: GpuVertex[]         │
│  Binding 2: GpuTriangle[]       │
│  Binding 3: BVHNode[]           │
│  Binding 4: GpuMaterial[]       │
└─────────────────────────────────┘
```

## 依赖关系

```
main.cpp
 └─ App (GUI)
     └─ Renderer
         ├─ ForwardPipeline
         │   └─ ShaderProgram
         ├─ DeferredPipeline
         │   ├─ GBuffer
         │   ├─ GBufferPass
         │   └─ LightingPass
         ├─ ShadertoyPipeline
         │   └─ ShaderProgram
         └─ RayTracingPipeline
             ├─ BVH
             └─ Compute Shader (raw GLuint)

Scene
 ├─ SceneNode (tree)
 ├─ Mesh
 ├─ Material
 ├─ Light
 └─ Camera

Loaders
 ├─ OBJLoader
 └─ USDLoader
```

---

## 性能优化策略

### CPU 端：
1. **BVH 预计算**：场景加载时构建，避免运行时开销
2. **三角形重排序**：提高 GPU 缓存命中率
3. **材质去重**：共享材质减少 SSBO 大小

### GPU 端：
1. **Early AABB Culling**：BVH 遍历时尽早剔除
2. **Workgroup Size**：16×16 tiles 充分利用 GPU 并行性
3. **避免分支**：使用 `mix()` 代替 `if-else`

### 内存管理：
1. **SSBO 静态上传**：场景数据 `GL_STATIC_DRAW`
2. **纹理压缩**：使用 mipmaps 减少带宽
3. **智能重载**：着色器编译失败保留旧版本

---

## 扩展点

### 易于扩展的部分：
1. **新增渲染管线**：继承 `RenderPipeline` 接口
2. **新增 Loader**：实现 `loadScene()` 函数
3. **新增几何体**：在 `geometry.cpp` 添加生成函数
4. **新增材质类型**：扩展 `GpuMaterial` 结构

### 困难的扩展：
1. **多 GPU 支持**：需要重构整个资源管理
2. **Vulkan 后端**：需要完全重写渲染层
3. **分布式渲染**：需要网络同步机制

---

## 已知限制

1. **光线追踪性能**：
   - 无硬件 RT 核心加速（纯 Compute Shader）
   - 复杂场景帧率较低（<30 FPS）

2. **材质系统**：
   - 不支持透明度混合

3. **光照模型**：
   - Ray Tracing 只支持 Lambertian + Metal
   - 缺少玻璃折射、次表面散射

4. **内存限制**：
   - 所有场景数据驻留在 GPU（无 streaming）
   - 超大场景可能 OOM

---

## 未来路线图

### 短期目标（已完成）：
- ✅ BVH 加速结构
- ✅ Temporal Accumulation 降噪
- ✅ 插值法线
- ✅ 材质系统
- ✅ SSAO（屏幕空间环境光遮蔽）

### 中期目标：
- 🔲 纹理采样（Albedo, Normal, Metallic, Roughness）
- 🔲 重要性采样（MIS）
- 🔲 环境贴图
- 🔲 体积渲染（雾、烟）
- 🔲 更多后处理效果（Bloom, Tone Mapping, TAA）

### 长期目标：
- 🔲 实时 GI（全局光照）
- 🔲 硬件光线追踪（RTX）
- 🔲 场景编辑器

---

## 参考资料

- [Ray Tracing in One Weekend](https://raytracing.github.io/)
- [Physically Based Rendering](http://www.pbr-book.org/)
- [Learn OpenGL](https://learnopengl.com/)
