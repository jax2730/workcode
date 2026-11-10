Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = "E:\Echo_SU_Develop"
$componentRoot = Join-Path $projectRoot "EchoComponent"
$wikiDir = Join-Path $projectRoot ".mini-wiki\wiki\组件系统\EchoComponent"
$importDir = Join-Path $projectRoot ".anythingllm\imports\echo-component"
$progressPath = Join-Path $projectRoot ".mini-wiki\cache\progress.json"

if (-not (Test-Path -LiteralPath $componentRoot)) { throw "EchoComponent not found: $componentRoot" }
New-Item -ItemType Directory -Force -Path $wikiDir | Out-Null
New-Item -ItemType Directory -Force -Path $importDir | Out-Null

$domainDefs = @(
  [ordered]@{ Key="3DUI"; Cn="3DUI 场景界面"; Wiki="子域索引-3DUI.md"; Import="echo-component-domain-3dui.md"; Pattern="(?:\\|/)3DUI(?:\\|/)|Echo3DUI|UI3D|GObject|GRoot|FGUI" },
  [ordered]@{ Key="PCG"; Cn="PCG 图与程序化生成"; Wiki="子域索引-PCG.md"; Import="echo-component-domain-pcg.md"; Pattern="(?:\\|/)PCG(?:\\|/)|PCG|InstancedStaticMesh" },
  [ordered]@{ Key="IK-Ragdoll"; Cn="IK 与布娃娃"; Wiki="子域索引-IK-Ragdoll.md"; Import="echo-component-domain-ik-ragdoll.md"; Pattern="IK|Ragdoll|BoneRender|Muscle" },
  [ordered]@{ Key="Physics"; Cn="物理、碰撞与关节"; Wiki="子域索引-Physics.md"; Import="echo-component-domain-physics.md"; Pattern="Physics|PhysX|RigidBody|Collider|Joint|Force|Wheel|Capsule|SphereCollider|BoxCollider|MeshCollider|CharactorController|TerrainCollider" },
  [ordered]@{ Key="Environment-TOD"; Cn="环境、天气与 TOD"; Wiki="子域索引-Environment-TOD.md"; Import="echo-component-domain-environment-tod.md"; Pattern="TOD|Tod|Sun|Sky|Fog|Cloud|Ocean|Weather|Atmosphere|Light|Water|SSAO|HDR|Star|Wind|Lightning|BendHorizon|Drift|UnderWater|VolumeLight|GlobalLight|Pointlight|LensFlare|EnvTrigger|ColorFilter" },
  [ordered]@{ Key="Terrain-Road"; Cn="地形、道路与大世界生成"; Wiki="子域索引-Terrain-Road.md"; Import="echo-component-domain-terrain-road.md"; Pattern="Terrain|Road|Pavement|Bezier|Planet|Karst|Voxel|Vegetation|Fractal|Spherical|Atlas|DistantBuilding|Building|HLOD|Imposter|CSG|Destructible|Herd|FunctionalBuilding|Traffic|SpacePCG" },
  [ordered]@{ Key="Actor-Script-Protocol"; Cn="Actor、脚本与协议桥"; Wiki="子域索引-Actor-Script-Protocol.md"; Import="echo-component-domain-actor-script-protocol.md"; Pattern="Actor|Script|Protocol|ProFunc|Role|Node|Entity|NpcDialog|HelperObj|ResBase|DebugScript|MaterialProFunction|CameraProFunction|ParticleProFunction" },
  [ordered]@{ Key="Scene-Camera-Timeline"; Cn="场景、相机与时间线"; Wiki="子域索引-Scene-Camera-Timeline.md"; Import="echo-component-domain-scene-camera-timeline.md"; Pattern="Scene|Camera|Timeline|TimeLine|AnimationTimeline|Trajectory|Polyline|Music|Media|Sheet|KeyBoard|Controller|Flying" },
  [ordered]@{ Key="Render-Effects"; Cn="渲染材质与特效组件"; Wiki="子域索引-Render-Effects.md"; Import="echo-component-domain-render-effects.md"; Pattern="Material|Decal|Particle|Emitter|Sound|Occlusion|Flare|LocalRadialBlur|NewDecal" },
  [ordered]@{ Key="SUToEchoCom"; Cn="SU 到 Echo 组件桥"; Wiki="子域索引-SUToEchoCom.md"; Import="echo-component-domain-sutoechocom.md"; Pattern="(?:\\|/)SUToEchoCom(?:\\|/)|SUWrap|SUDynamic|SUActor|ExcelData" },
  [ordered]@{ Key="Infrastructure"; Cn="组件系统基础设施"; Wiki="子域索引-Infrastructure.md"; Import="echo-component-domain-infrastructure.md"; Pattern="ComponentSystem|StableHeaders|Prerequisites|DefineMacro|ComInfoDefine|PhysicsComponentInclude|IKComponentInclude|Aggregate|Unknown|HelperRenderable" },
  [ordered]@{ Key="Gameplay-Other"; Cn="玩法与其他组件"; Wiki="子域索引-Gameplay-Other.md"; Import="echo-component-domain-gameplay-other.md"; Pattern=".*" }
)

function Get-RelativePath([string]$path) {
  if ([string]::IsNullOrWhiteSpace($path)) { return "" }
  $full = [IO.Path]::GetFullPath($path)
  $base = [IO.Path]::GetFullPath($componentRoot).TrimEnd('\') + '\'
  return $full.Substring($base.Length).Replace('\', '/')
}

function Read-Text([string]$path) {
  if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path -LiteralPath $path)) { return "" }
  return Get-Content -Raw -LiteralPath $path -Encoding UTF8
}

function Limit-List([string[]]$items, [int]$max = 8) {
  $clean = @($items | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
  if ($clean.Count -gt $max) { return @($clean[0..($max - 1)]) }
  return $clean
}

function Extract-Names([string]$text, [string]$kind) {
  if ([string]::IsNullOrWhiteSpace($text)) { return @() }
  $patterns = switch ($kind) {
    "types" { @('\b(?:class|struct)\s+(?:(?:_EchoComponentExport|ECHO\w+|final)\s+)*(?<name>[A-Za-z_][A-Za-z0-9_]*)', '\benum\s+(?:class\s+)?(?<name>[A-Za-z_][A-Za-z0-9_]*)') }
    "methods" { @('\b(?<name>(?:init|unInit|replay|copyTo|connectCom|disconnectCom|Create[A-Za-z0-9_]*|Release[A-Za-z0-9_]*|Execute|Schedule|Build[A-Za-z0-9_]*|Store[A-Za-z0-9_]*|Clear[A-Za-z0-9_]*|Set[A-Za-z0-9_]*|Get[A-Za-z0-9_]*|Update[A-Za-z0-9_]*|Add[A-Za-z0-9_]*|Remove[A-Za-z0-9_]*|Register[A-Za-z0-9_]*|Destroy[A-Za-z0-9_]*|Load[A-Za-z0-9_]*|Save[A-Za-z0-9_]*|On[A-Za-z0-9_]*))\s*\(') }
    "factories" { @('REGISTEROMFACTORY\((?<name>[A-Za-z_][A-Za-z0-9_:]*)\)', 'DEFINECOMFACTORY\((?<name>[A-Za-z_][A-Za-z0-9_:]*)') }
    "fields" { @('_DECLCOMINFO_PARAM(?:_SIM)?\([^,]+,\s*(?<name>[A-Za-z_][A-Za-z0-9_]*)', 'EDITOR_REFLECT_MEMBER(?:_UPDATE)?\([^,]+,\s*"(?<name>[^"]+)"') }
    "includes" { @('#include\s+"(?<name>[^"]+)"') }
  }
  $out = New-Object System.Collections.Generic.List[string]
  foreach ($pattern in $patterns) {
    foreach ($m in [regex]::Matches($text, $pattern)) {
      $name = $m.Groups["name"].Value.Trim()
      if ($name -and $name -notmatch '^(public|private|protected|return|if|for|while|switch)$') { $out.Add($name) }
    }
  }
  return @(Limit-List $out.ToArray() 12)
}

function Resolve-Domain([string]$rel, [string]$base) {
  $probe = "$rel $base"
  foreach ($d in $domainDefs) {
    if ($d.Key -eq "Gameplay-Other") { continue }
    if ($probe -match $d.Pattern) { return $d }
  }
  return $domainDefs[-1]
}

function Link-File([string]$rel) {
  if ([string]::IsNullOrWhiteSpace($rel)) { return "无同名文件" }
  $display = $rel.Replace('\','/')
  return "[$display](file:///E:/Echo_SU_Develop/EchoComponent/$display)"
}

function Summarize-Api([string[]]$methods, [string[]]$fields) {
  $parts = New-Object System.Collections.Generic.List[string]
  if ($methods.Count -gt 0) { $parts.Add("关键接口 " + ((Limit-List $methods 6) -join " / ")) }
  if ($fields.Count -gt 0) { $parts.Add("反射/配置字段 " + ((Limit-List $fields 5) -join " / ")) }
  if ($parts.Count -eq 0) { return "接口信号较少，主要依赖类型声明、宏注册或被其他组件包含使用" }
  return ($parts -join "；")
}

function Get-Domain-Action([string]$domainKey, [string]$base, [string[]]$methods, [string[]]$fields) {
  $m = if ($methods.Count -gt 0) { ((Limit-List $methods 4) -join " / ") } else { "构造、注册或调用点" }
  $f = if ($fields.Count -gt 0) { "字段 " + ((Limit-List $fields 3) -join " / ") } else { "同名类型和依赖 include" }
  switch ($domainKey) {
    "3DUI" { return "改 $base 的 UI 行为时，先看控件/包体/节点绑定字段，再追 $m；常见影响点是 FGUI 对象生命周期、事件同步、Scene/Screen 显示模式。" }
    "PCG" { return "改 $base 的生成链路时，先确认 $f，再追 $m；注意图编译、pin 数据传递、PCGContext 输入输出和资源释放。" }
    "IK-Ragdoll" { return "改 $base 的角色 IK/布娃娃表现时，先看权重、骨骼名、target 或 muscle 参数，再追 $m；同步检查动画、物理 tick 与可见性切换。" }
    "Physics" { return "改 $base 的物理行为时，先看质量/碰撞/关节/页面化字段，再追 $m；同步检查 EchoPhysicsManager、PhysX 对象创建释放和 callback/filter group。" }
    "Environment-TOD" { return "改 $base 的环境表现时，先看光照、天空、雾、水体、后处理或 TOD 字段，再追 $m；注意 TODComponentManager 和渲染参数写回。" }
    "Terrain-Road" { return "改 $base 的地形/道路/大世界逻辑时，先看几何、LOD、采样、路网或资源字段，再追 $m；注意 RoadManager、Terrain/Planet 坐标和生成缓存。" }
    "Actor-Script-Protocol" { return "改 $base 的脚本或协议入口时，先看注册名、PFID、Lua/Protocol 暴露参数，再追 $m；要同步 ComponentSystem 的协议注册/反注册。" }
    "Scene-Camera-Timeline" { return "改 $base 的场景调度、相机或时间线时，先看轨道/属性名/manager 字段，再追 $m；注意多场景 manager、timeline attribute 和 Actor 节点变换。" }
    "Render-Effects" { return "改 $base 的材质/特效时，先看资源、材质参数、粒子或 decal 字段，再追 $m；注意渲染对象创建、资源加载和可见状态。" }
    "SUToEchoCom" { return "改 $base 的 SU 导入桥时，先看 Excel/SQLite/动态工厂字段，再追 $m；同步检查 ComponentSystem 的 SUTOECHOCOMREGISTER 分支。" }
    "Infrastructure" { return "改 $base 的基础设施时，先确认宏、导出、聚合 include 或 manager 生命周期，再追 $m；它会影响多个子域的注册和编译边界。" }
    default { return "改 $base 时，先看 $f，再追 $m；必要时从 ComponentSystem 注册点和 ActorComponent 生命周期反查调用链。" }
  }
}

function Get-Concrete-Role([string]$domainKey, [string]$base, [string[]]$types) {
  $typeText = if ($types.Count -gt 0) { "（" + ((Limit-List $types 4) -join " / ") + "）" } else { "" }
  switch -Regex ($base) {
    "ComponentSystem" { return "全局组件系统入口：创建 TOD、Scene、Road、Physics、IK、Ragdoll、PCG 等 manager，并集中执行组件工厂与协议函数注册。" }
    "StableHeaders|Prerequisites|DefineMacro|ComInfoDefine|Include$" { return "编译/宏基础设施：提供预编译头、导出宏、组件声明宏、Info 参数宏或聚合 include，影响大量组件的编译边界。" }
    "Manager|System" { return "$base$typeText 是子系统调度/生命周期管理入口，通常负责创建、缓存、更新或销毁同域组件对象。" }
    "ProFunc|Protocol" { return "$base$typeText 是脚本/协议桥，负责把 C++ 组件能力暴露给 Lua、编辑器或 ProtocolComponentSystem。" }
    "Info$|Data$" { return "$base$typeText 是组件配置/序列化数据结构，通常承接编辑器反射字段并在 init/replay/copyTo 时写入运行态组件。" }
    "RigidBody" { return "$base$typeText 是 PhysX 动态刚体组件，处理质量、速度、重力、碰撞体挂接、睡眠/唤醒、力/扭矩与回调事件。" }
    "Collider|Shape" { return "$base$typeText 是碰撞形状组件，负责几何体、trigger/filter group、surface type、与 RigidBody 的 attach/detach。" }
    "Joint" { return "$base$typeText 是物理关节组件，连接刚体并管理线性/角度限制、驱动、断裂或布娃娃约束。" }
    "PCGGraphExecutor" { return "$base$typeText 执行已编译 PCG 图任务：调度 ready task、创建 context、转发上游输出、缓存节点结果。" }
    "PCGGraphCompiler" { return "$base$typeText 将 PCGGraph 编译成可执行 task DAG，处理节点依赖、pin 连接和 task id。" }
    "PCGPin" { return "$base$typeText 管理 PCG 节点输入输出 pin、边连接、断边与数据兼容关系。" }
    "PCG.*(Data|Point|Surface|Spatial|Landscape|Union|Difference)" { return "$base$typeText 是 PCG 数据层，描述点集、空间数据、表面/地形采样结果和差集/合并数据。" }
    "PCG.*(Sampler|Spawner|Filter|Transform|Gather|Distance|Difference|Bounds|Getter|Subgraph)" { return "$base$typeText 是具体 PCG 节点/Element，负责采样、过滤、变换、聚合、生成实例或调用子图。" }
    "3DUIComponentManager|UI3DComponentManager" { return "$base$typeText 管理 3DUI 全局队列、组件刷新、渲染优先级和 UI 命中/显示状态。" }
    "3DUIComponent$|3DUIBase|3DUIObject|3DUINode" { return "$base$typeText 是 3DUI 基类/对象桥，负责 FGUI 包体加载、GObject 绑定、Scene/Screen 模式和子控件访问。" }
    "3DUI" { return "$base$typeText 是具体 3DUI 控件组件，负责同名 FGUI 控件属性、事件、值同步和资源/子节点绑定。" }
    "IKBase" { return "$base$typeText 是 IK 组件基类，管理 IK 对象创建/释放、RoleIK 绑定、异步资源与可见性重建。" }
    "IK" { return "$base$typeText 是具体 IK 控制器/约束/目标组件，写回目标点、骨骼、权重、grounder/aim/recoil/hit/limit 参数。" }
    "RagdollCreator" { return "$base$typeText 生成布娃娃骨架刚体、碰撞体、关节和 muscle 模板，是自动构建 ragdoll 的入口。" }
    "RagdollComponent" { return "$base$typeText 管理布娃娃运行态、动画/物理混合、死亡/复活、pinning、tick 顺序和状态切换。" }
    "Ragdoll" { return "$base$typeText 处理布娃娃数据、行为、控制器、muscle 或工具导入导出。" }
    "TOD|Tod|Sun|Sky|Fog|Cloud|Ocean|Weather|Atmosphere|Water|SSAO|HDR|Star|Wind|Lightning|UnderWater|Light" { return "$base$typeText 是环境/TOD/渲染参数组件，负责把编辑器字段同步到天空、光照、水体、云雾或后处理运行对象。" }
    "Terrain|Road|Pavement|Bezier|Planet|Karst|Voxel|Vegetation|Fractal|Spherical|Atlas" { return "$base$typeText 参与地形、道路、星球、体素、植被或曲线生成/刷新，通常影响大世界空间数据和渲染/物理资源。" }
    "Actor|Script|Role|Entity|Node" { return "$base$typeText 连接 Actor/Role/Entity/Script 体系，处理组件生命周期、脚本能力或节点/角色对象访问。" }
    "Timeline|TimeLine|Trajectory|Camera|Scene|Animation" { return "$base$typeText 管理时间线、轨迹、动画或相机场景状态，常与 manager 和 Actor 节点变换联动。" }
    default { return "$base$typeText 属于 $domainKey 子域，职责需从同名类型、反射字段、生命周期函数和 ComponentSystem 注册点共同定位。" }
  }
}

function Build-Record([string]$base, [string]$headerPath, [string]$sourcePath) {
  $rel = if ($headerPath) { Get-RelativePath $headerPath } else { Get-RelativePath $sourcePath }
  $domain = Resolve-Domain $rel $base
  $hText = Read-Text $headerPath
  $sText = Read-Text $sourcePath
  $allText = "$hText`n$sText"
  $types = @(Limit-List (Extract-Names $allText "types") 8)
  $methods = @(Limit-List (Extract-Names $allText "methods") 10)
  $fields = @(Limit-List (Extract-Names $allText "fields") 8)
  $factories = @(Limit-List (Extract-Names $sText "factories") 8)
  $includes = @(Extract-Names $allText "includes" | Where-Object { $_ -notmatch 'StableHeaders|Prerequisites|stdafx|CommonStableHeaders' } | Select-Object -Unique | Select-Object -First 8)
  $role = Get-Concrete-Role $domain.Key $base $types
  $api = Summarize-Api $methods $fields
  $hRole = if ($headerPath) {
    $bits = @("声明/配置 $base：$role", $api)
    if ($hText -match 'DECLAREACTORCOMTYPE_H') { $bits += "接入 ActorComponent 类型系统" }
    if ($hText -match 'DEFINECOMFACTORY|DECLARECOMFACTORY|_DECLCOMINFO') { $bits += "包含工厂或 Info 参数宏，影响编辑器序列化和组件创建" }
    ($bits -join "；")
  } else {
    "无同名 .h：该条主要是实现文件或工具 cpp，需要从 include、静态函数和调用点反查入口。"
  }
  $cppBits = New-Object System.Collections.Generic.List[string]
  if ($sourcePath) {
    $cppBits.Add("实现 $base 的运行逻辑：$role")
    if ($methods.Count -gt 0) { $cppBits.Add("核心实现函数 " + ((Limit-List $methods 8) -join " / ")) }
    if ($fields.Count -gt 0 -and $sText -match 'EDITOR_REFLECT') { $cppBits.Add("写入编辑器反射字段 " + ((Limit-List $fields 5) -join " / ")) }
    if ($factories.Count -gt 0) { $cppBits.Add("注册工厂 " + ((Limit-List $factories 6) -join " / ")) }
    if ($sText -match 'PhysX|CreatePhysics|ReleasePhysics') { $cppBits.Add("包含 PhysX 对象创建/释放或物理同步路径") }
    if ($sText -match 'ProtocolComponentSystem|callProtocolFunc|initProtocolFunction') { $cppBits.Add("包含协议函数注册或脚本调用桥") }
    if ($sText -match 'Execute\(|Schedule\(|PCGContext') { $cppBits.Add("包含任务调度/执行/上下文结果处理") }
    if ($sText -match 'FGUI|GObject|UI3D') { $cppBits.Add("包含 FGUI/GObject/3DUI 绑定逻辑") }
  } else {
    $cppBits.Add("无同名 .cpp：多为纯声明、宏聚合、template/inline、数据结构或由其他 cpp 集中实现。")
  }
  $hint = Get-Domain-Action $domain.Key $base $methods $fields
  [pscustomobject]@{
    Base = $base
    DomainKey = $domain.Key
    DomainCn = $domain.Cn
    HeaderPath = if ($headerPath) { Get-RelativePath $headerPath } else { "" }
    SourcePath = if ($sourcePath) { Get-RelativePath $sourcePath } else { "" }
    Types = $types
    Methods = $methods
    Fields = $fields
    Factories = $factories
    Includes = $includes
    Role = $role
    HeaderRole = $hRole
    CppRole = ($cppBits.ToArray() -join "；")
    ModifyHint = $hint
  }
}

function Table-Escape([string]$s) {
  if ($null -eq $s) { return "" }
  return ($s -replace '\|', '/' -replace "`r?`n", "<br>")
}

$files = Get-ChildItem -LiteralPath $componentRoot -Recurse -File |
  Where-Object { $_.Extension -in @(".h", ".cpp") -and $_.FullName -notmatch '\\(?:Intermediate|Lib|Project|\.svn)\\' }

$byBase = @{}
foreach ($f in $files) {
  $base = [IO.Path]::GetFileNameWithoutExtension($f.Name)
  if (-not $byBase.ContainsKey($base)) { $byBase[$base] = [ordered]@{ Header=$null; Source=$null } }
  if ($f.Extension -ieq ".h") { $byBase[$base].Header = $f.FullName }
  elseif ($f.Extension -ieq ".cpp") { $byBase[$base].Source = $f.FullName }
}

$records = foreach ($base in ($byBase.Keys | Sort-Object)) {
  Build-Record $base $byBase[$base].Header $byBase[$base].Source
}

$now = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"
$totalHeaders = @($records | Where-Object { $_.HeaderPath }).Count
$totalSources = @($records | Where-Object { $_.SourcePath }).Count

$fileRows = foreach ($r in ($records | Sort-Object DomainKey, Base)) {
  $roleCell = Table-Escape $r.Role
  $hRoleCell = Table-Escape $r.HeaderRole
  $cppRoleCell = Table-Escape $r.CppRole
  $symbolsCell = Table-Escape ((($r.Types + $r.Methods) | Select-Object -Unique | Select-Object -First 10) -join " / ")
  $hintCell = Table-Escape $r.ModifyHint
  "| $($r.DomainCn)<br>$roleCell | $($r.Base) | $(Link-File $r.HeaderPath) | $(Link-File $r.SourcePath) | $hRoleCell | $cppRoleCell | $symbolsCell | $hintCell |"
}

$fileIndex = @"
# EchoComponent 文件级索引

> 修复版，生成时间：$now。  
> 本索引用来回答“改某个功能应该先看哪些 .h/.cpp”的问题，不再把“领域”写成重复标签，而是把每个文件族的具体职责、头文件作用、实现文件作用和修改入口拆开。子域索引仍然需要保留，它们是 Obsidian/AnythingLLM 检索分片和人工导航入口。

## 覆盖范围

| 指标 | 数量 |
| --- | ---: |
| 文件族（按同名 .h/.cpp 合并） | $($records.Count) |
| .h 文件 | $totalHeaders |
| .cpp 文件 | $totalSources |
| 子域索引 | $($domainDefs.Count) |

## 阅读方式

1. 先按“子域/具体作用”定位业务面，再用“文件族”检索类名或组件名。
2. 改字段、反射、序列化、编辑器面板时优先读 `.h`，尤其是 `Info`、`Data`、`_DECLCOMINFO_PARAM`、`EDITOR_REFLECT` 对应项。
3. 改运行时行为、生命周期、manager 调度、PhysX/FGUI/PCG 执行时优先读 `.cpp`。
4. 如果某行缺少同名 `.h` 或 `.cpp`，不要视为缺失；它通常是聚合头、宏基础设施、工具实现或集中注册文件，需要顺着 include 和 ComponentSystem 注册点继续追。

## 文件级索引表

| 子域 / 具体作用 | 文件族 | .h 文件 | .cpp 文件 | .h 作用 | .cpp 作用 | 关键类/函数 | 修改入口提示 |
| --- | --- | --- | --- | --- | --- | --- | --- |
$($fileRows -join "`r`n")

## 子域导航

- [子域索引导航](子域索引导航.md)：按 3DUI、PCG、Physics、IK/Ragdoll、环境/TOD、地形道路等拆分，适合检索和分批维护。
- [组件系统入口与注册](组件系统入口与注册.md)：解释 `ComponentSystem` 如何把组件、协议函数和 manager 接起来。
- [EchoComponent 总览](EchoComponent.md)：模块级位置、与 EchoCommon / EchoPhysics / EchoDataBridge 等文件夹的关系。

**Section sources**
- [EchoComponent/Include](file:///E:/Echo_SU_Develop/EchoComponent/Include)
- [EchoComponent/Source](file:///E:/Echo_SU_Develop/EchoComponent/Source)
- [EchoComponentSystem.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoComponentSystem.cpp)
"@

$fileIndexPath = Join-Path $wikiDir "文件级索引.md"
$fileIndex | Set-Content -LiteralPath $fileIndexPath -Encoding UTF8
$fileIndex | Set-Content -LiteralPath (Join-Path $importDir "echo-component-file-index.md") -Encoding UTF8

function Domain-Overview([string]$key) {
  switch ($key) {
    "3DUI" { return "3DUI 子域把 Echo ActorComponent 与 FGUI/GObject 场景界面连接起来，重点是控件组件、UI 节点、包体加载、屏幕/场景显示模式、事件和值同步。" }
    "PCG" { return "PCG 子域负责程序化生成图：节点/Pin/Settings/Data/Element/Graph compiler/executor 共同完成从图结构到任务执行和结果缓存的流程。" }
    "IK-Ragdoll" { return "IK 与布娃娃子域连接角色骨骼、IK 求解器、物理 ragdoll 数据和运行态控制器，常见修改会同时影响动画、物理和可见性。" }
    "Physics" { return "Physics 子域封装 PhysX 刚体、碰撞体、关节、页面化物理对象、碰撞回调和编辑器参数，是运行时物理行为的主要入口。" }
    "Environment-TOD" { return "环境/TOD 子域把天气、天空、太阳、云雾、水体、星空、光源、后处理参数同步到渲染和 TOD manager。" }
    "Terrain-Road" { return "地形道路子域覆盖地形、星球、体素、道路、铺装、建筑、植被和大世界生成组件，是空间数据和可视资源生成的导航区。" }
    "Actor-Script-Protocol" { return "Actor/脚本/协议子域把组件能力暴露给 Lua、编辑器和 ProtocolComponentSystem，同时处理 Actor、Role、Entity、Node 等对象访问。" }
    "Scene-Camera-Timeline" { return "场景/相机/时间线子域处理多场景 manager、相机组件、时间线属性、轨迹曲线、媒体/音乐控制和键盘/飞行控制入口。" }
    "Render-Effects" { return "渲染特效子域聚焦材质、decal、粒子、发射器、声音、遮挡和局部特效，常与资源加载和可见状态同步有关。" }
    "SUToEchoCom" { return "SUToEchoCom 子域负责 SketchUp/SU 侧数据进入 Echo 组件体系，包括 Excel/SQLite 数据辅助、动态工厂和包裹实体。" }
    "Infrastructure" { return "基础设施子域包含 ComponentSystem、预编译头、导出/Info/组件声明宏、聚合 include 和兜底组件，影响所有子域的注册与编译。" }
    default { return "玩法与其他组件子域收纳暂未归入上述强业务域的组件，仍按文件族给出可修改入口；发现稳定链路后可提升为独立子域。" }
  }
}

function Domain-Relations([string]$key) {
  switch ($key) {
    "3DUI" { return "依赖 EchoCommon 的 Actor/Resource 基础，通常通过 ComponentSystem 的 Echo3DUIRegisterFactory 注册，并与 Scene/Actor 节点可见性联动。" }
    "PCG" { return "依赖 EchoComponent 的 ActorComponent 生命周期，也会把生成结果写入 InstancedStaticMesh、Terrain/Road 或资源对象；图执行由 PCGSystem/PCGGraphManager 管。" }
    "IK-Ragdoll" { return "依赖角色 Role/Animation、Physics 刚体/关节和 EchoIKSystem；修改时经常要同时检查 Physics 子域与 Actor/Role 子域。" }
    "Physics" { return "依赖 EchoPhysics/PhysX 封装、Actor 节点变换和 ComponentSystem 的 EchoPhysicsSystem manager 创建；碰撞体与刚体之间有双向挂接。" }
    "Environment-TOD" { return "依赖渲染系统、TODComponentSystem/TODComponentManager、SceneComponentManager；很多组件从编辑器参数写回渲染对象。" }
    "Terrain-Road" { return "依赖 RoadManager/RoadHelper、Terrain/Planet 坐标、PCG 或渲染资源；生成结果可能继续影响 Physics collider 和 Environment 展示。" }
    "Actor-Script-Protocol" { return "依赖 EchoProtocolComponent、Lua/script manager、ActorSystem 和 ComponentSystem 的 initProtocolFunction/qtNotInitProtocolFunction 分支。" }
    "Scene-Camera-Timeline" { return "依赖 SceneComponentManager、CameraComponentManager、TimeLineInfoManager、Actor 节点变换和 Protocol 暴露函数。" }
    "Render-Effects" { return "依赖资源系统、渲染组件、Actor 可见性和 Environment/TOD 参数；粒子/材质/音频常从脚本或编辑器触发。" }
    "SUToEchoCom" { return "只在 ComponentSystem 的 SUTOECHOCOMREGISTER 分支下注册，依赖 ExcelDataManager、SqliteMgr 和动态工厂切换。" }
    "Infrastructure" { return "被所有子域依赖；ComponentSystem 同时连接 EchoCommon、Actor、World、TOD、Physics、IK、Ragdoll、PCG、SUToEchoCom。" }
    default { return "与 ActorComponent 生命周期、ComponentSystem 注册点和具体 include 关系相关；若出现稳定主题，应拆出新子域。" }
  }
}

$navRows = New-Object System.Collections.Generic.List[string]
foreach ($d in $domainDefs) {
  $domainRecords = @($records | Where-Object { $_.DomainKey -eq $d.Key } | Sort-Object Base)
  $hCount = @($domainRecords | Where-Object { $_.HeaderPath }).Count
  $sCount = @($domainRecords | Where-Object { $_.SourcePath }).Count
  $entry = ($domainRecords | Select-Object -First 5 | ForEach-Object { $_.Base }) -join " / "
  $navRows.Add("| $($d.Cn) | $($domainRecords.Count) | $hCount | $sCount | [$($d.Wiki)]($($d.Wiki)) | $entry |")

  $rows = foreach ($r in $domainRecords) {
    $deps = if ($r.Includes.Count -gt 0) { (($r.Includes | Select-Object -First 6) -join " / ") } else { "从 ComponentSystem 注册点、ActorComponent 生命周期或同名文件继续追踪" }
    $roleCell = Table-Escape $r.Role
    $hRoleCell = Table-Escape $r.HeaderRole
    $cppRoleCell = Table-Escape $r.CppRole
    $depsCell = Table-Escape $deps
    $hintCell = Table-Escape $r.ModifyHint
    "| $($r.Base) | $(Link-File $r.HeaderPath) | $(Link-File $r.SourcePath) | $roleCell | $hRoleCell | $cppRoleCell | $depsCell | $hintCell |"
  }
  $overview = Domain-Overview $d.Key
  $relations = Domain-Relations $d.Key
  $content = @"
# EchoComponent 子域索引：$($d.Cn)

> 修复版，生成时间：$now。  
> 子域索引不是临时文件：它把文件级索引拆成可检索分片，便于 AnythingLLM/Obsidian 在查询“某个功能从哪里改”时召回更小、更准的上下文。

## 子域职责

$overview

## 与其他文件夹/子域的关系

$relations

## 覆盖统计

| 指标 | 数量 |
| --- | ---: |
| 文件族 | $($domainRecords.Count) |
| .h 文件 | $hCount |
| .cpp 文件 | $sCount |

## 文件族索引

| 文件族 | .h 文件 | .cpp 文件 | 具体职责 | .h 作用 | .cpp 作用 | 主要依赖/关联 | 修改入口提示 |
| --- | --- | --- | --- | --- | --- | --- | --- |
$($rows -join "`r`n")

## 检索建议

- 查字段、编辑器面板、保存/回放差异：优先搜文件族名加 `Info`、`Data`、`_DECLCOMINFO_PARAM` 或 `EDITOR_REFLECT`。
- 查运行时 bug：优先搜文件族名加生命周期函数，例如 `init`、`connectCom`、`CreatePhysics`、`Execute`、`Set*`、`Get*`。
- 查跨域影响：从“主要依赖/关联”列进入 manager、Actor、Physics、TOD、PCG 或 SUToEchoCom 的相邻子域。
- 本文已覆盖当前扫描到的同名 .h/.cpp 文件族；避免空泛占位。若要深挖专题，应在本索引行上追加“已覆盖范围 / 未覆盖风险 / 下一修改入口”三段式说明。

**Section sources**
- [EchoComponent/Include](file:///E:/Echo_SU_Develop/EchoComponent/Include)
- [EchoComponent/Source](file:///E:/Echo_SU_Develop/EchoComponent/Source)
- [文件级索引.md](文件级索引.md)
"@
  $content | Set-Content -LiteralPath (Join-Path $wikiDir $d.Wiki) -Encoding UTF8
  $content | Set-Content -LiteralPath (Join-Path $importDir $d.Import) -Encoding UTF8
}

$graphLines = $domainDefs | ForEach-Object {
  $node = $_.Key.Replace("-","_")
  "    ComponentSystem --> $node[`"$($_.Cn)`"]"
}
$nav = @"
# EchoComponent 子域索引导航

> 修复版，生成时间：$now。  
> 子域索引导航的作用是把 EchoComponent 这个 497 个源/头文件量级的模块拆成稳定检索入口。它不是“可删的目录页”：AnythingLLM/Obsidian 更适合召回这些较小的子域分片，再由文件级索引定位具体 .h/.cpp。

## 为什么保留子域索引

| 用途 | 说明 |
| --- | --- |
| 人工导航 | 先按 3DUI、PCG、Physics、IK/Ragdoll、环境/TOD、地形道路等业务面进入，再落到文件族。 |
| LLM 检索分片 | 避免一次召回超大的 `文件级索引.md`，提高对“改某功能找哪个文件”的命中率。 |
| 质量检查 | 每个子域可独立检查是否有空文件、重复修改入口、缺少 .h/.cpp 说明。 |
| 专题衔接 | 深挖专题应从子域索引行继续补，并记录已覆盖范围、未覆盖风险和下一修改入口。 |

## 子域列表

| 子域 | 文件族 | .h | .cpp | Wiki 文件 | 代表入口 |
| --- | ---: | ---: | ---: | --- | --- |
$($navRows.ToArray() -join "`r`n")

## 关系图

``````mermaid
flowchart LR
    ComponentSystem["ComponentSystem 注册 / manager 生命周期"]
$($graphLines -join "`r`n")
    Physics --> IK_Ragdoll["IK 与布娃娃"]
    PCG --> Terrain_Road["地形、道路与大世界生成"]
    Environment_TOD["环境、天气与 TOD"] --> Render_Effects["渲染材质与特效组件"]
    Actor_Script_Protocol["Actor、脚本与协议桥"] --> Scene_Camera_Timeline["场景、相机与时间线"]
    SUToEchoCom["SU 到 Echo 组件桥"] --> Actor_Script_Protocol
``````

## 对应 AnythingLLM 导入文件

这些文件已同步到 `E:\Echo_SU_Develop\.anythingllm\imports\echo-component`，用于 AnythingLLM 在线时通过 API 或 UI 重新导入/vectorize：

| Wiki | AnythingLLM import |
| --- | --- |
| 文件级索引.md | echo-component-file-index.md |
$(($domainDefs | ForEach-Object { "| $($_.Wiki) | $($_.Import) |" }) -join "`r`n")

**Section sources**
- [EchoComponent/Include](file:///E:/Echo_SU_Develop/EchoComponent/Include)
- [EchoComponent/Source](file:///E:/Echo_SU_Develop/EchoComponent/Source)
- [AnythingLLM imports](file:///E:/Echo_SU_Develop/.anythingllm/imports/echo-component)
"@

$nav | Set-Content -LiteralPath (Join-Path $wikiDir "子域索引导航.md") -Encoding UTF8
$nav | Set-Content -LiteralPath (Join-Path $importDir "echo-component-domain-index.md") -Encoding UTF8

if (Test-Path -LiteralPath $progressPath) {
  try {
    $progress = Get-Content -Raw -LiteralPath $progressPath -Encoding UTF8 | ConvertFrom-Json
    $progress.last_updated = (Get-Date).ToString("o")
    $progress.last_user_feedback = [ordered]@{
      recorded_at = (Get-Date).ToString("o")
      topic = "EchoComponent index repair completed"
      summary = "Regenerated EchoComponent file-level index, subdomain indexes, navigation, and local AnythingLLM import markdown. Subdomain indexes are retained as navigation and retrieval shards."
      action = "Continue future EchoComponent work from repaired file-level/subdomain indexes; avoid empty shards, repeated domain labels, repeated modification hints, and vague placeholder text."
    }
    $progress.echo_component_repair = [ordered]@{
      status = "completed_local_docs"
      completed_at = (Get-Date).ToString("o")
      regenerated = @("文件级索引.md","子域索引导航.md") + ($domainDefs | ForEach-Object { $_.Wiki })
      import_synced = @("echo-component-file-index.md","echo-component-domain-index.md") + ($domainDefs | ForEach-Object { $_.Import })
      quality_rule = "Each row separates concrete role, .h role, .cpp role, key symbols, dependencies, and file-specific modification entry."
      remote_anythingllm = "not attempted by this script; local import md regenerated for later upload/vectorization."
    }
    $progress | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $progressPath -Encoding UTF8
  } catch {
    Write-Warning "Progress update failed: $($_.Exception.Message)"
  }
}

$emptyWiki = @(Get-ChildItem -LiteralPath $wikiDir -Filter "子域索引-*.md" | Where-Object { $_.Length -lt 1000 })
$emptyImport = @(Get-ChildItem -LiteralPath $importDir -Filter "echo-component-domain-*.md" | Where-Object { $_.Length -lt 1000 })
Write-Output "Regenerated EchoComponent indexes."
Write-Output "Records: $($records.Count), headers: $totalHeaders, sources: $totalSources"
Write-Output "Small wiki subdomain indexes: $($emptyWiki.Count)"
Write-Output "Small import subdomain indexes: $($emptyImport.Count)"
