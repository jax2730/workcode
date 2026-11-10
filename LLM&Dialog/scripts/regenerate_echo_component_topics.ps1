Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = "E:\Echo_SU_Develop"
$wikiDir = Join-Path $projectRoot ".mini-wiki\wiki\组件系统\EchoComponent"
$importDir = Join-Path $projectRoot ".anythingllm\imports\echo-component"
$obsidianDir = Join-Path $projectRoot ".obsidian-knowledge\Echo_SU_Develop"
$progressPath = Join-Path $projectRoot ".mini-wiki\cache\progress.json"

New-Item -ItemType Directory -Force -Path $wikiDir | Out-Null
New-Item -ItemType Directory -Force -Path $importDir | Out-Null
New-Item -ItemType Directory -Force -Path $obsidianDir | Out-Null

$generatedAt = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"

$overview = @'
# EchoComponent 模块知识卡

> 修复版。本文是 EchoComponent 的总览入口，配合 `文件级索引.md` 和 `子域索引导航.md` 使用。索引负责“快速找文件”，专题负责解释“为什么这些文件要一起看”。

## 模块定位

EchoComponent 是 Echo_SU_Develop 中把具体业务能力落到 ActorComponent 体系上的组件库。它不定义最底层的 Actor/Resource/Protocol 框架，而是在 EchoEngine、EchoCommon、EchoPhysics 等基础之上，注册可以被资源、编辑器、脚本和运行时创建的具体组件。

从职责看，它更像一个“能力装配层”：物理、碰撞、IK、布娃娃、3DUI、PCG、环境/TOD、地形道路、材质特效、Actor 脚本桥、SU 导入桥都在这里以组件形式出现。修改 EchoComponent 时，要同时关注 `.h` 中的 Info/反射字段和 `.cpp` 中的生命周期、manager 调度、底层对象创建释放。

## 架构关系

```mermaid
flowchart TB
    Common["EchoCommon\nName / Math / SmartPtr / Stream"] --> Component["EchoComponent\n具体 ActorComponent 库"]
    Engine["EchoEngine\nActor / Protocol / Resource / World"] --> Component
    Physics["EchoPhysics\nPhysX Scene / Shape / RigidDynamic"] --> Component
    Component --> Editor["EchoEditor / Qt / SU 导入"]
    Component --> Game["EchoGame / runtime scenes"]
    Component --> SDK["EchoSDK Include/Lib 输出"]
```

## 子域地图

| 子域 | 入口文档 | 核心职责 | 修改时优先看 |
| --- | --- | --- | --- |
| 组件系统入口 | [组件系统入口与注册.md](组件系统入口与注册.md) | `ComponentSystem` 创建 manager、注册工厂、注册协议函数 | `EchoComponentSystem.h/.cpp` |
| Physics | [Physics刚体碰撞专题.md](Physics刚体碰撞专题.md) | 刚体、碰撞体、关节、PhysX 页面化和回调 | `EchoRigidBodyComponent.*`, `EchoColliderComponent.*`, `EchoPhysicsManager.*` |
| PCG | [PCG图执行专题.md](PCG图执行专题.md) | PCG 图资源、节点、Pin、编译器、执行器、数据与生成资源 | `PCG/EchoPCGComponent.*`, `PCG/Graph/*`, `PCG/Elements/*` |
| 3DUI | [3DUI场景界面专题.md](3DUI场景界面专题.md) | FGUI/GObject 与 ActorComponent 的桥接、场景 UI 管理 | `3DUI/Echo3DUIComponent.*`, `3DUI/Echo3DUIComponentManager.*` |
| IK/Ragdoll | [IK布娃娃专题.md](IK布娃娃专题.md) | 角色 IK、Grounder、Aim/LookAt、布娃娃创建与运行态混合 | `EchoIK*.h/.cpp`, `EchoRagdoll*.h/.cpp` |
| 环境/TOD/地形道路 | [环境TOD地形道路专题.md](环境TOD地形道路专题.md) | 天空、天气、光照、水体、星球地形、道路、铺装、植被 | `EchoTOD*.h/.cpp`, `EchoSphericalTerrain*`, `EchoRoadManager.*` |

## 文件级工作法

1. 先从 [子域索引导航.md](子域索引导航.md) 选择业务面。
2. 再到 [文件级索引.md](文件级索引.md) 搜组件名、类名、字段名或行为关键词。
3. 改编辑器字段、资源保存、序列化，先读 `.h` 的 `Info/Data/_DECLCOMINFO_PARAM/EDITOR_REFLECT`。
4. 改运行时行为，先读 `.cpp` 的 `init/unInit/connectCom/disconnectCom/Create*/Release*/Update*/Execute*`。
5. 改“组件不存在、脚本调不到、编辑器创建失败”这类问题，先查 `ComponentSystem` 的工厂注册和协议函数注册。

## 跨文件夹关系

| 外部文件夹 | EchoComponent 的使用方式 | 风险点 |
| --- | --- | --- |
| EchoCommon | 提供数学、字符串、Name、智能指针、Stream、通用工具 | 类型/宏改动会影响几乎全部组件编译 |
| EchoEngine | 提供 Actor、Component、Resource、Protocol、World/Scene 管理 | 生命周期或协议注册改动会影响组件创建与脚本桥 |
| EchoPhysics | 提供 PhysX 封装对象和 Scene/Shape/RigidDynamic | 物理组件 `.cpp` 中的 Create/Release 路径必须同步检查 |
| EchoEditor / Qt | 通过 Info/反射字段展示和修改组件参数 | `.h` 字段变动需同步反射、保存、replay/copyTo |
| EchoDataBridge / 资源数据 | 地形、星球、道路、PCG 数据可能来自数据桥或资源文件 | 生成/刷新链路要检查资源加载和缓存清理 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | 当前已建立全量文件级索引、12 个子域索引、总览、注册、Physics、PCG、3DUI、IK/Ragdoll、环境/TOD/地形道路专题。 |
| 主要风险 | 同类组件很多，容易只看同名 `.cpp` 而漏掉 `ComponentSystem` 注册、Info/反射字段、manager 生命周期和外部系统依赖。 |
| 下一修改入口 | 按功能词进入子域索引，再落到文件级索引；复杂功能同时打开同名 `.h/.cpp`、manager 文件和 `ComponentSystem` 注册点。 |

**Related documents**
- [文件级索引.md](文件级索引.md)
- [子域索引导航.md](子域索引导航.md)
- [组件系统入口与注册.md](组件系统入口与注册.md)

**Section sources**
- [EchoComponentSystem.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoComponentSystem.h)
- [EchoComponentSystem.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoComponentSystem.cpp)
- [EchoComponent/Include](file:///E:/Echo_SU_Develop/EchoComponent/Include)
- [EchoComponent/Source](file:///E:/Echo_SU_Develop/EchoComponent/Source)
'@

$registration = @'
# EchoComponent 组件系统入口与注册

> 本文解释 EchoComponent 的启动入口：`ComponentSystem` 如何把具体组件接入 EchoEngine 的 Actor 工厂、Protocol 调用、manager 生命周期和可选子系统。

## 核心职责

`ComponentSystem` 继承 `Singleton<ComponentSystem>`，是 EchoComponent 最重要的入口。构造阶段创建 TOD、Scene、Road、LightFlare、Camera、Physics、IK、Ragdoll、PCG 等全局 manager；初始化阶段按 register list 决定是否注册 3DUI、SUToEchoCom、键盘和 Qt 差异协议；关闭阶段反向释放 manager 和协议函数。

这意味着 EchoComponent 的问题不能只从单个组件文件看。一个组件要真正可用，至少要满足三个条件：类型在工厂注册、字段/Info 能正确保存和 replay、脚本/协议入口已经注册。

## 注册模式

| 标志 | 作用 | 修改入口 |
| --- | --- | --- |
| `ECHO3UICOMREGISTER` | 注册 EchoComponent 内的 3DUI/FGUI 组件，并创建 `UI3DComponentManager` | `Echo3DUIRegisterFactory()` |
| `ECHOTESTSCRIPTREGISTER` | 保留测试脚本注册入口 | `init(ComSystemRegisterList list)` |
| `ECHOCOMKEYBOARDREGISTER` | 创建 `EchoKeyBoard` 键盘映射 | `init(...)` / `deinit()` |
| `QTANDECHODIFFPROFUNCTION` | Qt/Editor 差异路径，避免自动初始化 Main manager | `getIsQtInit()` 分支 |
| `SUTOECHOCOMREGISTER` | 初始化 `SqliteMgr`、`ExcelDataManager` 并注册 SU 导入桥组件 | `registerSUToEchoComFactory()` |

## 初始化链路

```mermaid
sequenceDiagram
    participant App as Runtime or Editor
    participant CS as ComponentSystem
    participant Factory as ActorComFactoryManager
    participant Protocol as ProtocolComponentSystem
    participant Managers as TOD/Physics/IK/PCG managers
    App->>CS: new ComponentSystem()
    CS->>Managers: create singletons
    App->>CS: init(register list)
    CS->>Factory: registerFactory()
    CS->>Protocol: initProtocolFunction()
    alt non-Qt runtime
        CS->>Managers: initManager("Main")
        CS->>Protocol: qtNotInitProtocolFunction()
    else Qt editor
        CS->>CS: mark QTANDECHODIFFPROFUNCTION
    end
```

## 具体注册族群

| 族群 | 代表注册类型 | 典型故障 | 优先文件 |
| --- | --- | --- | --- |
| Actor/Role/Script | `NodeComponent`, `EntityComponent`, `RoleComponent`, `ActorScriptComponent` | 资源里有组件名但运行时无法创建 | `EchoComponentSystem.cpp`, `EchoActor*.h/.cpp` |
| Physics | `RigidBodyComponent`, `ColliderComponent`, `Box/Capsule/Sphere/MeshCollider`, `Joint` | 物理组件创建失败、碰撞体没有挂到刚体 | `EchoRigidBodyComponent.*`, `EchoColliderComponent.*` |
| IK/Ragdoll | `IKGrounder*`, `IKAim*`, `IKFBBIK*`, `Ragdoll*` | 姿态控制组件缺失、布娃娃数据不可实例化 | `EchoIK*.h/.cpp`, `EchoRagdoll*.h/.cpp` |
| 3DUI | `Echo3DUIButtonComponent`, `Echo3DUIComponent`, `Echo3DUIComponentManager` | UI 控件无法加载、场景 UI 不刷新 | `3DUI/Echo3DUI*.h/.cpp` |
| Environment/TOD | `SunComponent`, `SkyComponent`, `Cloud*`, `FogComponent`, `TODShadowComponent` | 天空、光照、雾、水体参数不生效 | `EchoTOD*.h/.cpp`, `EchoSkyComponent.*` |
| PCG/Terrain | `PCGComponent`, `SphericalTerrainComponent`, `VoxelTerrainComponent`, `Karst*` | PCG 或地形组件无法创建 | `PCG/*`, `EchoSphericalTerrain*`, `EchoKarst*` |
| 条件注册 | `SSAOComponent`, `DistantBuildingComponentEditor` | Debug/Editor 专用组件缺失 | runtime flag / `ECHO_EDITOR` 分支 |

## Manager 生命周期

`initManager(name)` 从 `ProtocolComponentSystem` 获取指定 world/scene 对应的 manager，然后创建 TOD、LightFlare、IK、Physics manager，并绑定 `WorldSystem::GetWorldManager(name)`。`deinitManager(name)` 反向销毁 KarstRoad、Physics、IK、LightFlare、TOD。

切场景后出现残留对象时，不要只查单个组件析构。要一起查：`deinitManager(name)` 是否调用、对应子系统 `DestroyManager` 是否释放资源、组件自己的 `disconnectCom/unInit/Release*` 是否执行。

## 症状到文件

| 症状 | 检查顺序 |
| --- | --- |
| 组件类存在但创建失败 | `registerFactory()` 是否有 `REGISTEROMFACTORY(Target)`；启动 register list 是否包含目标族群。 |
| 编辑器能创建但脚本/协议调不到 | `initProtocolFunction()`、`qtNotInitProtocolFunction()` 是否注册对应 `ProFuncImp`。 |
| Qt 与 runtime 行为不同 | `QTANDECHODIFFPROFUNCTION`、`getIsQtInit()`、`qtNotInitProtocolFunction()` 分支。 |
| 3DUI 不可用 | register list 是否包含 `ECHO3UICOMREGISTER`，是否创建 `UI3DComponentManager`。 |
| SU 导入组件不可用 | register list 是否包含 `SUTOECHOCOMREGISTER`，`SqliteMgr` 和 `ExcelDataManager` 是否初始化。 |
| 切场景后 Physics/TOD/IK 残留 | `initManager/deinitManager` 与各 manager 的 Create/Destroy 路径。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | `ComponentSystem` 构造/析构、`init(bool)`、`init(list)`、工厂注册、协议注册、manager 创建销毁。 |
| 主要风险 | 工厂注册和协议注册是两条链；一个组件能被创建，不代表脚本/编辑器操作一定可用。 |
| 下一修改入口 | 新增组件时同时改 `.h/.cpp`、Info/反射字段、`ComponentSystem::registerFactory()`；若暴露脚本能力，再改对应 `ProFuncImp`。 |

**Related documents**
- [EchoComponent.md](EchoComponent.md)
- [文件级索引.md](文件级索引.md)
- [子域索引-Infrastructure.md](子域索引-Infrastructure.md)

**Section sources**
- [EchoComponentSystem.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoComponentSystem.h)
- [EchoComponentSystem.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoComponentSystem.cpp)
'@

$physics = @'
# EchoComponent Physics 刚体碰撞专题

> 本文用于定位 EchoComponent 中的物理、碰撞、刚体、关节、物理分页和 PhysX 生命周期问题。

## 子域定位

Physics 子域把 EchoPhysics 的 PhysX 封装对象落到 ActorComponent 生命周期里。`.h` 侧通常声明 Data/Info/反射字段和用户可调 API；`.cpp` 侧负责创建释放 PhysX 对象、把 Actor 节点变换同步给物理、把碰撞体挂到刚体、派发 wake/sleep/contact 回调。

## 核心链路

```mermaid
flowchart TB
    Actor["Actor / NodeComponent"] --> RB["RigidBodyComponent"]
    Collider["ColliderComponent\nBox/Capsule/Sphere/Mesh/Terrain"] --> RB
    RB --> Manager["EchoPhysicsManager"]
    Manager --> Scene["EchoPhysXScene"]
    Scene --> PhysX["PhysXRigidDynamic / PhysXShape"]
    Joint["JointComponent\nFixed/Configurable"] --> RB
    Page["PhysicsPage / PagedLayer"] --> Manager
```

## 关键文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `EchoRigidBodyComponent.*` | 声明 `RigidBodyComponentData/Info`、质量、速度、重力、约束、惯性、力/扭矩、sleep、sweep API | 实现 `CreatePhysics/ReleasePhysics`、Collider attach/detach、ForceAndTorque 缓存、质量惯性重算、callback 派发 | 改刚体参数、运动学、力、碰撞回调先看这里 |
| `EchoColliderComponent.*` | 声明 collider 基类字段：trigger、material、surface、filter group、helper | 实现 shape 创建释放、与 rigidbody 挂接、filter group、可见性与 helper | 改所有碰撞体公共行为先看这里 |
| `EchoBox/Capsule/Sphere/MeshColliderComponent.*` | 声明具体几何参数，例如 half size、radius、height、mesh path/cooking option | 实现具体 shape 数据更新、helper 显示、Info 字段写回 | 改碰撞形状或编辑器参数先看对应文件 |
| `EchoJointComponent.*` | 声明关节基类、连接刚体、break force/torque、collision/projection | 实现关节 PhysX 对象生命周期和连接关系 | 改关节共性或连接对象先看这里 |
| `EchoConfigurableJointComponent.*` | 声明线性/角度运动、limit、drive、visible flag | 实现具体自由度、角度限制、drive 写入 PhysX joint | 改 ragdoll 关节或高级约束先看这里 |
| `EchoPhysicsManager.*` | 声明物理 manager、更新队列、查询、分页入口 | 实现刚体/碰撞体创建队列、物理 tick、事件发送、分页对象更新 | 改物理调度、世界切换、队列残留先看这里 |
| `EchoPhysicsPage.*` / `EchoPhysicsPagedLayer.*` | 声明分页物理对象与空间层 | 实现大世界物理对象进入/离开和更新 | 改远距离物理加载卸载先看这里 |

## 生命周期要点

```mermaid
sequenceDiagram
    participant Actor
    participant RB as RigidBodyComponent
    participant Col as ColliderComponent
    participant Mgr as EchoPhysicsManager
    participant Scene as PhysXScene
    Actor->>RB: connectCom()
    RB->>Mgr: CreatePhysXObject or AddToPhysicsUpdateQueue
    RB->>Scene: AddRigidDynamic(info)
    Col->>RB: AttachShape()
    RB->>Scene: AttachShape / reset mass and inertia
    Actor->>RB: disconnectCom()
    RB->>Col: RemoveSelfCollider()
    RB->>Mgr: ReleasePhysXObject / RemovePhysXActor
```

## 常见症状

| 症状 | 优先检查 |
| --- | --- |
| 刚体存在但不受力 | `RigidBodyComponent::SetIsUsePhysics/CreatePhysics`、`GetIsKinematic`、`ApplyForceAndTorque`、`EchoPhysicsManager` 更新队列。 |
| 碰撞体无效 | 具体 collider 的几何参数、`ColliderComponent::CreatePhysics`、是否附着到 `RigidBodyComponent`。 |
| 质量或惯性异常 | `ResetMassAndInertiaTensor`、`CenterOfMassMode`、Collider attach/detach 后是否触发重算。 |
| 切场景后残留 | `ComponentSystem::deinitManager`、`EchoPhysicsSystem::DestroyManager`、`disconnectCom/unInit`。 |
| MeshCollider 表现不一致 | `EchoMeshColliderComponent` 的 mesh 数据、convex/cooking option、filter group。 |
| 关节不工作 | `EchoJointComponent` 的连接刚体、`EchoConfigurableJointComponent` 的 motion/limit/drive 字段。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | 刚体、碰撞体、关节、物理 manager、物理分页、地形物理桥的主要入口。 |
| 主要风险 | `.h` 的 Info 字段、`.cpp` 的 PhysX 对象生命周期、manager 更新队列必须一起改；只改其中一处容易出现编辑器保存正常但运行时不生效。 |
| 下一修改入口 | 先搜具体组件名，再同时打开同名 `.h/.cpp`、`EchoPhysicsManager.*` 和 `ComponentSystem` 注册点。 |

**Related documents**
- [子域索引-Physics.md](子域索引-Physics.md)
- [文件级索引.md](文件级索引.md)
- [IK布娃娃专题.md](IK布娃娃专题.md)

**Section sources**
- [EchoRigidBodyComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoRigidBodyComponent.h)
- [EchoRigidBodyComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoRigidBodyComponent.cpp)
- [EchoColliderComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoColliderComponent.h)
- [EchoColliderComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoColliderComponent.cpp)
- [EchoPhysicsManager.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoPhysicsManager.h)
- [EchoPhysicsManager.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoPhysicsManager.cpp)
'@

$pcg = @'
# EchoComponent PCG 图执行专题

> 本文用于定位 PCG 图资源、节点、Pin、编译、执行、数据流、生成资源和自定义节点问题。

## 子域定位

PCG v2 是运行在 ActorComponent 上的程序化生成图系统。`PCGComponent` 读取 `PCGGraphV2` 资源，`PCGSystem` 作为 frame listener 驱动 `PCGGraphExecutor`，编译器把 Graph/Node/Pin/Edge 编译成 task，执行器创建 `PCGContext` 并把上游输出传给下游 Element。

## 执行链路

```mermaid
sequenceDiagram
    participant Comp as PCGComponent
    participant Sys as PCGSystem
    participant Compiler as PCGGraphCompiler
    participant Exec as PCGGraphExecutor
    participant Element as PCGElement
    participant Data as PCGDataCollection
    Comp->>Comp: Generate()
    Comp->>Comp: CleanupAllManagedResources()
    Comp->>Sys: ScheduleComponent(this)
    Sys->>Exec: Schedule(PCGComponent)
    Exec->>Compiler: GetCompiledTasks(graph)
    Exec->>Exec: ready tasks -> active tasks
    Exec->>Element: Initialize(context)
    Element->>Data: outputData
    Exec->>Exec: StoreResults(nodeId, outputData)
```

## 关键文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `EchoPCGComponent.*` | 声明 Actor 上的 PCG 组件、GraphPath、Bounds、managed resources | 实现 Graph 加载、Generate、清理旧生成资源、调度组件 | 改“点击 Generate 没结果/旧生成物残留”先看这里 |
| `EchoPCGSystem.*` | 声明单例、GraphExecutor、PCGBaseManager | 挂 Root frame listener，tick 执行图并销毁无引用资源 | 改帧调度、异步任务、全局清理先看这里 |
| `EchoPCGGraph.*` / `EchoPCGPin.*` | 声明 Graph/Node/Pin/Edge、JSON 读写、Pin 类型 | 实现节点增删、边连接、Pin 兼容、导入导出 | 改图编辑、连线、断线、保存加载先看这里 |
| `Graph/EchoPCGGraphCompiler.*` | 声明 task 编译和缓存 | 实现基于入边的 DAG 编译、task 输入依赖 | 改执行顺序、依赖、子图展开先看这里 |
| `Graph/EchoPCGGraphExecutor.*` | 声明 ready/active task、context、输出缓存 | 实现任务执行、上游输出注入、结果缓存 | 改数据传递、执行状态、任务生命周期先看这里 |
| `EchoPCGSettings.*` / `EchoPCGElement.*` | 声明节点配置、PinProperties、Element 生命周期 | 实现节点创建、Element 初始化和执行模板 | 新增节点或改节点参数先看这里 |
| `Data/*` | 声明 `PCGDataCollection`、Spatial/Point/Union/Difference 数据 | 实现采样、投影、bounds/octree、union/difference | 改点数据、空间数据或采样性能先看这里 |
| `Elements/*` | 声明具体节点，例如 SurfaceSampler、DensityFilter、StaticMeshSpawner | 实现节点算法和生成结果 | 改具体 PCG 节点行为先看对应文件 |

## 图结构与数据结构

```mermaid
flowchart LR
    Graph["PCGGraph"] --> Node["PCGNode"]
    Node --> Pin["PCGPin"]
    Pin --> Edge["PCGEdge"]
    Node --> Settings["PCGSettings"]
    Settings --> Element["PCGElement"]
    Element --> Context["PCGContext"]
    Context --> Collection["PCGDataCollection"]
    Collection --> Spatial["PCGSpatialData"]
    Collection --> Point["PCGPointData"]
```

## 具体风险点

| 风险 | 文件 | 说明 |
| --- | --- | --- |
| 调度状态不完整 | `EchoPCGSystem.*`, `EchoPCGComponent.*` | `ScheduleComponent` 和 generation task id 需要一起设计，否则 UI 很难判断“生成中/已完成/取消”。 |
| Pin 断线残留 | `EchoPCGPin.cpp` | 断边、清边逻辑要同时维护两端 `m_Edges`，否则图看似断开但编译仍可能感知旧连接。 |
| Subgraph 未真正执行 | `EchoPCGSubgraph.*` | 文件已预留设置和 Element，但需要明确子图展开或调度语义。 |
| StaticMeshSpawner 固定 mesh | `EchoPCGStaticMeshSpawner.cpp` | Settings 里有 mesh selector 结构，但执行侧要确认是否真正使用 selector，而不是固定 helper cube。 |
| 点数据缓存脏标记 | `EchoPCGPointData.*` | 修改点集后 bounds/octree dirty 逻辑要正确，否则查询和采样结果会错。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | PCGComponent、PCGSystem、Graph/Pin、Compiler/Executor、Settings/Element、Data、Elements、Reflection、ManagedResource。 |
| 主要风险 | PCG 是多层系统，单个节点问题可能来自 Graph JSON、Pin 类型、Compiler 顺序、Executor 输入注入、Element 算法或生成资源清理。 |
| 下一修改入口 | 查具体节点名进入 `Elements/*`；查执行顺序进入 `Graph/*`；查保存加载进入 `EchoPCGGraph.*` 和 `Reflection/*`；查生成残留进入 `EchoPCGComponent.*` 和 `EchoPCGManagedResource.*`。 |

**Related documents**
- [子域索引-PCG.md](子域索引-PCG.md)
- [文件级索引.md](文件级索引.md)
- [环境TOD地形道路专题.md](环境TOD地形道路专题.md)

**Section sources**
- [EchoPCGComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/PCG/EchoPCGComponent.h)
- [EchoPCGComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/PCG/EchoPCGComponent.cpp)
- [EchoPCGGraphExecutor.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/PCG/Graph/EchoPCGGraphExecutor.h)
- [EchoPCGGraphExecutor.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/PCG/Graph/EchoPCGGraphExecutor.cpp)
- [EchoPCGPin.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/PCG/EchoPCGPin.cpp)
- [EchoPCGStaticMeshSpawner.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/PCG/Elements/EchoPCGStaticMeshSpawner.cpp)
'@

$ui3d = @'
# EchoComponent 3DUI 场景界面专题

> 本文用于定位 EchoComponent 中 3DUI、FGUI、GObject、场景 UI 管理、控件事件和值同步问题。

## 子域定位

3DUI 子域把 FGUI 的 GObject/GRoot 控件体系包装成 ActorComponent。它既处理 UI 包加载、控件绑定、Scene/Screen 模式，也处理按钮、文本、列表、进度条、滑块等具体控件的脚本事件和属性同步。

## 结构图

```mermaid
flowchart TB
    ComponentSystem --> UIReg["Echo3DUIRegisterFactory"]
    UIReg --> Manager["UI3DComponentManager"]
    Actor["Actor / NodeComponent"] --> UIComp["Echo3DUIComponent"]
    UIComp --> Base["Echo3DUIBaseComponent"]
    Base --> GObject["FGUI GObject"]
    Manager --> GRoot["GRoot / Stage"]
    Base --> Controls["Button/Text/Image/List/Tree/Slider/etc."]
```

## 关键文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `Echo3DUIComponent.*` | 声明 UI 包路径、PackageItemName、Scene/Screen 模式、根 GObject | 实现包加载、GObject 创建、Actor 生命周期绑定 | 改 UI 包加载、根控件或显示模式先看这里 |
| `Echo3DUIBaseComponent.*` | 声明子控件基类、UniqueID、debug sphere、GObject 访问 | 实现子控件绑定、controller page、调试显示 | 改子控件定位、绑定失败或调试显示先看这里 |
| `Echo3DUIComponentManager.*` | 声明 UI 管理器、组件集合、排序和命中信息 | 实现场景 UI 刷新、相机空间排序、GRoot/Stage 更新 | 改渲染优先级、命中、刷新队列先看这里 |
| `Echo3DUIButtonComponent.*` | 声明按钮脚本事件、ScriptPath | 实现 roll over/out、click、touch begin/end 和 Lua 绑定 | 改按钮交互或脚本事件先看这里 |
| `Echo3DUIText/InputText/RichText.*` | 声明文本类控件字段和接口 | 实现 text 读写、输入同步、脚本访问 | 改文本显示或输入框先看对应文件 |
| `Echo3DUIImage/Loader/Movieclip.*` | 声明图片、加载器、动效控件字段 | 实现资源/控件属性同步 | 改图片资源、加载器或动效先看对应文件 |
| `Echo3DUIList/Tree/ComboBox/Slider/ScrollBar/ProgressBar.*` | 声明复合控件或数值控件接口 | 实现值同步、选项/列表/滚动状态桥接 | 改列表、树、下拉、滑块、滚动或进度先看对应文件 |

## 常见症状

| 症状 | 优先检查 |
| --- | --- |
| UI 包加载失败 | `Echo3DUIComponent` 的 FGUIFilePath、PackageItemName、LoadGObjectFromFile。 |
| 子控件找不到 | `Echo3DUIBaseComponent` 的 UniqueID、GObject 绑定、父 UI 根对象。 |
| 点击/触摸事件不触发 | 具体控件 `.cpp` 的事件绑定、`ScriptPath`、Lua 注册函数。 |
| 场景 UI 深度或遮挡异常 | `Echo3DUIComponentManager` 的相机空间排序、render priority、GRoot 更新。 |
| Screen/Scene 模式表现不同 | `Echo3DUIComponent` 的 UI2DStyle、Scene/Screen 切换字段。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | 3DUI 根组件、子控件基类、manager、常见控件、脚本事件与 FGUI/GObject 桥。 |
| 主要风险 | 3DUI 问题经常跨越包路径、GObject 绑定、Actor 生命周期、manager 刷新和脚本事件。 |
| 下一修改入口 | UI 根问题看 `Echo3DUIComponent.*`；单控件问题看对应 `Echo3DUI*Component.*`；全局排序/命中问题看 `Echo3DUIComponentManager.*`。 |

**Related documents**
- [子域索引-3DUI.md](子域索引-3DUI.md)
- [组件系统入口与注册.md](组件系统入口与注册.md)

**Section sources**
- [Echo3DUIComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/3DUI/Echo3DUIComponent.h)
- [Echo3DUIComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/3DUI/Echo3DUIComponent.cpp)
- [Echo3DUIBaseComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/3DUI/Echo3DUIBaseComponent.h)
- [Echo3DUIComponentManager.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/3DUI/Echo3DUIComponentManager.cpp)
'@

$ik = @'
# EchoComponent IK 与布娃娃专题

> 本文用于定位角色 IK、Grounder、Aim/LookAt、Recoil、HitReaction、RotationLimit、BoneRender、Ragdoll 创建和运行态控制问题。

## 子域定位

IK/Ragdoll 子域连接角色动画、骨骼目标、物理刚体/关节和运行态状态机。IK 文件通常负责把组件字段写入对应 IK 控制器；Ragdoll 文件通常负责创建刚体/碰撞体/关节、保存 ragdoll 数据、控制动画与物理混合。

## 关系图

```mermaid
flowchart TB
    Role["Role / Skeleton / Animation"] --> IKBase["EchoIKBaseComponent"]
    IKBase --> IKs["Aim / LookAt / CCDIK / LimbIK / FBBIK / Grounder"]
    IKs --> Target["IKTargetPoint / IKPoint / Effectors"]
    Physics["Physics RigidBody / Collider / Joint"] --> Ragdoll["RagdollComponent"]
    Ragdoll --> Creator["RagdollCreatorComponent"]
    Ragdoll --> Data["RagdollData / Muscle / Tools"]
```

## 关键文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `EchoIKBaseComponent.*` | 声明 IK 通用生命周期、RoleIK 绑定、异步加载、可见性接口 | 实现 IK 创建/释放、Role 切换、资源绑定和有效性检查 | 改所有 IK 共性问题先看这里 |
| `EchoIKAimIK/LookAtIK/CCDIK/LimbIK.*` | 声明具体 IK 控制器参数、target、骨骼和权重字段 | 实现参数写回、目标绑定和运行态更新 | 改瞄准、看向、链式 IK、肢体 IK 先看对应文件 |
| `EchoIKGrounder*. *` | 声明脚贴地/四足/全身 grounder 字段 | 实现地面探测、权重和 IK solver 写回 | 改脚贴地、坡面适配先看这里 |
| `EchoIKRecoil/HitReaction/Inertia/BodyTilt.*` | 声明反冲、命中反应、惯性、身体倾斜参数 | 实现动画反应和角色姿态附加效果 | 改受击、枪械反冲、惯性反馈先看对应文件 |
| `EchoIKRotationLimit*. *` | 声明角度/铰链旋转限制字段 | 实现旋转限制参数写入 IK 系统 | 改骨骼旋转限制先看这里 |
| `EchoRagdollCreatorComponent.*` | 声明自动生成 ragdoll 的模板和参数 | 实现从骨架生成 collider、rigidbody、joint、muscle | 改自动布娃娃生成先看这里 |
| `EchoRagdollComponent.*` | 声明 ragdoll 运行态、混合、pinning、状态字段 | 实现死亡/复活、动画物理混合、tick 顺序、物理状态切换 | 改布娃娃行为、动画物理混合先看这里 |
| `EchoRagdollData/Muscle/Tools.*` | 声明 ragdoll 数据结构、肌肉约束和工具入口 | 实现数据导入导出、muscle 约束和辅助操作 | 改资源兼容或工具链先看这里 |

## 与 Physics 的关系

Ragdoll 不只是 IK 问题，它实际落到 `RigidBodyComponent`、`ColliderComponent`、`JointComponent` 和 `ConfigurableJointComponent`。如果布娃娃姿态正确但碰撞或约束异常，要同时打开 Physics 专题中对应文件。

## 常见症状

| 症状 | 优先检查 |
| --- | --- |
| IK 组件创建失败 | `EchoIKBaseComponent` 的 Create/Release、RoleIK 绑定、可见性重建。 |
| Aim/LookAt 目标不生效 | 目标点组件、target name、weight 字段、具体 IK 控制器 `.cpp`。 |
| Grounder 脚贴地异常 | grounder 组件字段、地面检测层、角色骨骼名和权重。 |
| 布娃娃生成缺刚体/关节 | `EchoRagdollCreatorComponent` 生成模板和 Physics 注册。 |
| 动画与物理混合抖动 | `EchoRagdollComponent` 的状态机、tick 顺序、pinning/muscle 参数。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | IK 基类、常见 IK 控制器、目标点、Grounder、Recoil/HitReaction、RotationLimit、Ragdoll 创建/运行/数据工具。 |
| 主要风险 | IK/Ragdoll 跨 Role/Animation/Physics 三条链；只改姿态字段不检查物理刚体和关节，容易出现局部有效整体失效。 |
| 下一修改入口 | IK 共性看 `EchoIKBaseComponent.*`；具体姿态看同名 `EchoIK*.h/.cpp`；布娃娃物理看 `EchoRagdoll*.h/.cpp` 加 Physics 专题。 |

**Related documents**
- [子域索引-IK-Ragdoll.md](子域索引-IK-Ragdoll.md)
- [Physics刚体碰撞专题.md](Physics刚体碰撞专题.md)

**Section sources**
- [EchoIKBaseComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoIKBaseComponent.h)
- [EchoIKBaseComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoIKBaseComponent.cpp)
- [EchoRagdollComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoRagdollComponent.h)
- [EchoRagdollComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoRagdollComponent.cpp)
- [EchoRagdollCreatorComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoRagdollCreatorComponent.cpp)
'@

$envTerrain = @'
# EchoComponent 环境 TOD 地形道路专题

> 本文用于定位天空、太阳、天气、雾、云、水体、光照、后处理、星球地形、体素地形、道路、铺装、植被和大世界生成问题。

## 子域定位

这一组文件横跨 Environment-TOD 和 Terrain-Road 两个子域。前者把编辑器/资源字段同步到天空、光照、水体、云雾、后处理和 TOD manager；后者负责地形、星球、体素、道路、铺装、植被、建筑和大世界生成资源。

## 结构图

```mermaid
flowchart TB
    ComponentSystem --> TODSystem["TODComponentSystem / TODComponentManager"]
    TODSystem --> Env["Sun / Sky / Fog / Cloud / Ocean / HDR / SSAO / Light"]
    Terrain["SphericalTerrain / VoxelTerrain / AtlasTerrain"] --> Physics["GeneratedTerrainPhysics / TerrainCollider"]
    Road["RoadManager / Pavement / CrossRoad / PiecewiseBezier"] --> Terrain
    PCG["PCG / SpacePCG"] --> Terrain
    Env --> Render["Render parameters / scene visual state"]
```

## 环境/TOD 文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `EchoTODComponentSystem/Manager.*` | 声明 TOD 系统和多场景 manager | 实现 TOD manager 创建、销毁、参数调度 | 改 TOD 全局调度或多场景环境先看这里 |
| `EchoTodTimelineComponent.*` / `EchoControlTodChangeComponent.*` | 声明时间线/TOD 控制字段 | 实现环境时间变化和控制逻辑 | 改昼夜时间线、天气切换先看这里 |
| `EchoSun/Sky/Fog/Cloud*. *` | 声明太阳、天空、雾、云层参数 | 实现渲染参数写回和运行态刷新 | 改天空/天气外观先看对应文件 |
| `EchoOcean/SphericalOcean/UnderWaterCaustics.*` | 声明海洋、水下焦散、球面水体字段 | 实现水体渲染、可见性、参数同步 | 改水体表现先看这里 |
| `EchoHDR/HDRAdaptive/SSAO/ColorFilter/LocalRadialBlur.*` | 声明后处理字段 | 实现后处理参数写入 | 改画面后处理先看对应文件 |
| `EchoLight/GlobalLight/Pointlight/LightFlare/LensFlare.*` | 声明光源和光晕组件字段 | 实现光源对象、manager、flare 运行态 | 改灯光或光晕先看这里 |

## 地形/道路文件族

| 文件族 | .h 作用 | .cpp 作用 | 修改入口 |
| --- | --- | --- | --- |
| `EchoSphericalTerrainComponent.*` / `EchoAtlasSphericalTerrainComponent.*` | 声明星球/球面地形字段、LOD、资源入口 | 实现地形生成、刷新、坐标和渲染资源 | 改星球地形先看这里 |
| `EchoVoxelTerrainComponent.*` / `EchoKarstTerrainComponent.*` | 声明体素/喀斯特地形参数 | 实现体素数据、地形块和生成刷新 | 改体素或喀斯特地形先看这里 |
| `EchoGeneratedTerrainPhysics.*` / `EchoTerrainColliderComponent.*` | 声明生成地形物理字段 | 实现地形 mesh collider 或物理资源连接 | 改地形碰撞先看这里 |
| `EchoRoadManager.*` / `EchoPlanetRoadNetworkManager.*` | 声明道路网络 manager | 实现道路创建、刷新、路网数据管理 | 改道路网络先看这里 |
| `EchoPavementComponent/Object/Util.*` | 声明铺装对象和工具参数 | 实现铺装生成、曲线采样和 mesh 刷新 | 改铺装和路面先看这里 |
| `EchoPiecewiseBezierComponent/Util.*` | 声明分段贝塞尔曲线字段 | 实现曲线采样、控制点和路径计算 | 改道路曲线或轨迹先看这里 |
| `EchoVegetationAreaComponent.*` | 声明植被区域参数 | 实现植被生成/刷新测试辅助 | 改植被区域先看这里 |

## 常见症状

| 症状 | 优先检查 |
| --- | --- |
| TOD 参数不生效 | `EchoTODComponentManager`、具体环境组件 Info 字段、是否绑定正确 world/manager。 |
| 天空/云/雾显示异常 | `EchoSkyComponent`、`EchoCloud*Component`、`EchoFogComponent` 的渲染参数写回。 |
| 水体或水下效果异常 | `EchoOceanComponent`、`EchoSphericalOceanComponent`、`EchoUnderWaterCausticsComponent`。 |
| 地形有显示但无碰撞 | `EchoGeneratedTerrainPhysics`、`EchoTerrainColliderComponent`、Physics 专题里的 MeshCollider。 |
| 道路刷新不正确 | `EchoRoadManager`、`EchoPavement*`、`EchoPiecewiseBezier*`。 |
| 大世界生成结果与 PCG 不一致 | `EchoSpacePCGComponent`、PCG 专题和地形/道路组件一起查。 |

## 已覆盖范围 / 风险 / 下一修改入口

| 项 | 内容 |
| --- | --- |
| 已覆盖范围 | TOD manager、环境渲染组件、光照/后处理、水体、星球/体素/喀斯特地形、道路/铺装/曲线、植被、地形物理桥。 |
| 主要风险 | 该专题跨 Environment、Render、Terrain、Physics、PCG 多条链；视觉问题可能来自字段未写回，也可能来自 manager/world 绑定或资源刷新。 |
| 下一修改入口 | 环境表现看 `EchoTOD*` 和具体环境组件；地形道路看 `EchoSphericalTerrain*`、`EchoRoadManager.*`、`EchoPavement*`；碰撞问题再转 Physics 专题。 |

**Related documents**
- [子域索引-Environment-TOD.md](子域索引-Environment-TOD.md)
- [子域索引-Terrain-Road.md](子域索引-Terrain-Road.md)
- [PCG图执行专题.md](PCG图执行专题.md)
- [Physics刚体碰撞专题.md](Physics刚体碰撞专题.md)

**Section sources**
- [EchoTODComponentManager.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoTODComponentManager.h)
- [EchoTODComponentManager.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoTODComponentManager.cpp)
- [EchoSphericalTerrainComponent.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoSphericalTerrainComponent.h)
- [EchoSphericalTerrainComponent.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoSphericalTerrainComponent.cpp)
- [EchoRoadManager.h](file:///E:/Echo_SU_Develop/EchoComponent/Include/EchoRoadManager.h)
- [EchoRoadManager.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoRoadManager.cpp)
'@

$docs = @(
  @{ Wiki="EchoComponent.md"; Import="echo-component-overview.md"; Obsidian="EchoComponent-模块知识卡.md"; Text=$overview },
  @{ Wiki="组件系统入口与注册.md"; Import="echo-component-registration.md"; Obsidian="EchoComponent-组件系统入口与注册.md"; Text=$registration },
  @{ Wiki="Physics刚体碰撞专题.md"; Import="echo-component-physics-deep-dive.md"; Obsidian="EchoComponent-Physics刚体碰撞专题.md"; Text=$physics },
  @{ Wiki="PCG图执行专题.md"; Import="echo-component-pcg-deep-dive.md"; Obsidian="EchoComponent-PCG图执行专题.md"; Text=$pcg },
  @{ Wiki="3DUI场景界面专题.md"; Import="echo-component-3dui-deep-dive.md"; Obsidian="EchoComponent-3DUI场景界面专题.md"; Text=$ui3d },
  @{ Wiki="IK布娃娃专题.md"; Import="echo-component-ik-ragdoll-deep-dive.md"; Obsidian="EchoComponent-IK布娃娃专题.md"; Text=$ik },
  @{ Wiki="环境TOD地形道路专题.md"; Import="echo-component-environment-terrain-road-deep-dive.md"; Obsidian="EchoComponent-环境TOD地形道路专题.md"; Text=$envTerrain }
)

foreach ($doc in $docs) {
  $body = $doc.Text.Replace("修复版。", "修复版，生成时间：$generatedAt。")
  $body | Set-Content -LiteralPath (Join-Path $wikiDir $doc.Wiki) -Encoding UTF8
  $body | Set-Content -LiteralPath (Join-Path $importDir $doc.Import) -Encoding UTF8
  $body | Set-Content -LiteralPath (Join-Path $obsidianDir $doc.Obsidian) -Encoding UTF8
}

if (Test-Path -LiteralPath $progressPath) {
  $progress = Get-Content -Raw -LiteralPath $progressPath -Encoding UTF8 | ConvertFrom-Json
  $progress.last_updated = (Get-Date).ToString("o")
  $progress.last_user_feedback = [ordered]@{
    recorded_at = (Get-Date).ToString("o")
    topic = "EchoComponent topic docs regenerated"
    summary = "Regenerated overview, registration, Physics, PCG, 3DUI, IK/Ragdoll, and Environment/Terrain/Road topic docs in readable Chinese; synced local AnythingLLM imports and Obsidian notes."
    action = "Use the repaired topic docs with file-level and subdomain indexes as the EchoComponent knowledge base."
  }
  $progress.echo_component_topics = [ordered]@{
    status = "completed_local_docs"
    completed_at = (Get-Date).ToString("o")
    regenerated = @($docs | ForEach-Object { $_.Wiki })
    import_synced = @($docs | ForEach-Object { $_.Import })
    obsidian_synced = @($docs | ForEach-Object { $_.Obsidian })
    quality_rule = "Topic docs avoid vague batch placeholders and include concrete .h/.cpp entry points, covered scope, risks, and next modification entry."
    remote_anythingllm = "local import md only; remote upload/vectorization can be run when AnythingLLM is reachable and desired."
  }
  $progress | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $progressPath -Encoding UTF8
}

Write-Output "Regenerated EchoComponent topic docs: $($docs.Count)"
