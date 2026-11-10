param(
  [string]$ProjectRoot = "E:\Echo_SU_Develop"
)

$ErrorActionPreference = "Stop"

$wikiRoot = Join-Path $ProjectRoot ".mini-wiki\wiki"
$wikiDir = Get-ChildItem -LiteralPath $wikiRoot -Recurse -Directory -Filter "EchoComponent" |
  Select-Object -First 1 -ExpandProperty FullName
if ([string]::IsNullOrWhiteSpace($wikiDir)) {
  throw "Cannot find EchoComponent wiki directory under $wikiRoot"
}
$importDir = Join-Path $ProjectRoot ".anythingllm\imports\echo-component"

$slugMap = @{
  "3DUI" = "echo-component-domain-3dui.md"
  "Actor-Script-Protocol" = "echo-component-domain-actor-script-protocol.md"
  "Environment-TOD" = "echo-component-domain-environment-tod.md"
  "Gameplay-Other" = "echo-component-domain-gameplay-other.md"
  "IK-Ragdoll" = "echo-component-domain-ik-ragdoll.md"
  "Infrastructure" = "echo-component-domain-infrastructure.md"
  "PCG" = "echo-component-domain-pcg.md"
  "Physics" = "echo-component-domain-physics.md"
  "Render-Effects" = "echo-component-domain-render-effects.md"
  "Scene-Camera-Timeline" = "echo-component-domain-scene-camera-timeline.md"
  "SUToEchoCom" = "echo-component-domain-sutoechocom.md"
  "Terrain-Road" = "echo-component-domain-terrain-road.md"
}

$introMap = @{
  "3DUI" = "Use this index when changing FGUI scene controls, GObject/GRoot binding, 3D UI updates, render priority, or hit-test behavior."
  "Actor-Script-Protocol" = "Use this index when changing Actor/Role components, script components, protocol functions, ProFuncImp bridges, keyboard mappings, or node extension behavior."
  "Environment-TOD" = "Use this index when changing sun, sky, clouds, fog, ocean, lighting, HDR/SSAO, weather, or time-of-day flow."
  "Gameplay-Other" = "Use this index when changing gameplay helper components such as buildings, dialogue, music/audio, traffic objects, tables, or miscellaneous business objects."
  "IK-Ragdoll" = "Use this index when changing IK solvers, pose targets, Grounder/FBBIK, bone debug rendering, ragdoll behavior, ragdoll control, or ragdoll data."
  "Infrastructure" = "Use this index when changing ComponentSystem, export macros, PCH, common macros, registration lists, or module infrastructure."
  "PCG" = "Use this index when changing PCG graphs, task execution, data models, node elements, reflection, serialization, or generated resources."
  "Physics" = "Use this index when changing rigid bodies, colliders, joints, physics pages, force/torque, filter groups, or PhysX object lifecycle."
  "Render-Effects" = "Use this index when changing materials, particles, decals, HLOD, imposters, media, post effects, or renderable presentation components."
  "Scene-Camera-Timeline" = "Use this index when changing scene management, camera management, timelines, tracks, polylines, or animation timeline flow."
  "SUToEchoCom" = "Use this index when changing SketchUp import, dynamic loading, factory switching, Excel/Sqlite helpers, or wrapper entities."
  "Terrain-Road" = "Use this index when changing roads, pavement, curves, planet/spherical/voxel/karst terrain, or vegetation."
}

function Get-HeaderRole([string]$base) {
  switch -Regex ($base) {
    "ComponentSystem" { return "Declares the component-system entry, registration lists, and manager lifecycle APIs." }
    "Manager" { return "Declares manager scheduling, caches, queues, object ownership, or global subsystem state." }
    "BaseComponent" { return "Declares shared component fields, lifecycle hooks, and virtual extension points." }
    "Component$" { return "Declares ActorComponent data, reflected fields, lifecycle hooks, and editable parameters." }
    "ProFunc|Protocol" { return "Declares script/protocol exported functions and bridge entry points." }
    "Info|Data|Settings|Definition|Definitions" { return "Declares config structs, serialized fields, enums, or runtime data models." }
    "Graph|Pin|Task|Compiler|Executor" { return "Declares PCG graph, pin/task relations, compile APIs, or executor APIs." }
    "Element|Sampler|Spawner|Filter|Modifier|Gather|Difference|Distance" { return "Declares PCG node inputs/outputs and node parameters." }
    "Serializer|Reflection|Property|Class|Enum" { return "Declares reflection metadata, properties, enum metadata, serializer contracts, or object metadata." }
    "Collider|RigidBody|Joint|Physics|Force|Controller|Aggregate|Wheel" { return "Declares physics component parameters, PhysX bridge handles, and editor-tunable fields." }
    "Ragdoll|Muscle" { return "Declares ragdoll data, muscle constraints, behavior hooks, or creator APIs." }
    "IK|Bone" { return "Declares IK configuration, target points, bone/pose/constraint fields, and creation checks." }
    "3DUI|UI3D|GRoot|GObject|Button|InputText|Image|Text|Loader|List|Tree|Slider|ScrollBar|ComboBox|ProgressBar" { return "Declares FGUI/3DUI fields, GObject binding, control type, and UI lifecycle APIs." }
    "Terrain|Road|Planet|Karst|Voxel|Vegetation|Pavement|Bezier" { return "Declares terrain/road/planet/voxel/vegetation config, generation parameters, or manager entry points." }
    "TOD|Sun|Sky|Fog|Cloud|Ocean|Weather|Atmosphere|Light|Water|SSAO|HDR" { return "Declares environment, weather, lighting, post-process, or time-of-day parameters." }
    default { return "Declares public fields, interfaces, reflection, or data shape for this class/tool." }
  }
}

function Get-CppRole([string]$base) {
  switch -Regex ($base) {
    "ComponentSystem" { return "Implements component registration, manager creation/release, module initialization order, and cross-domain entry points." }
    "Manager" { return "Implements centralized update, cache maintenance, object registration/removal, and subsystem scheduling." }
    "BaseComponent" { return "Implements shared lifecycle, resource binding, state switching, and subclass reuse logic." }
    "Component$" { return "Implements init/connect/update, reflected-data writeback, runtime behavior, and editor-field synchronization." }
    "ProFunc|Protocol" { return "Implements protocol/script dispatch, parameter conversion, and Actor/Component bridge calls." }
    "Info|Data|Settings|Definition|Definitions" { return "Implements defaults, copy helpers, serialization helpers, or resource loading logic." }
    "Graph|Pin|Task|Compiler|Executor" { return "Implements graph links, pin breakage, task generation, topology compilation, and executor queues." }
    "Element|Sampler|Spawner|Filter|Modifier|Gather|Difference|Distance" { return "Implements concrete PCG node algorithms, point filtering/sampling/spawning, or spatial-data operations." }
    "Serializer|Reflection|Property|Class|Enum" { return "Implements PCG reflection metadata, property access, JSON/object serialization." }
    "Collider|RigidBody|Joint|Physics|Force|Controller|Aggregate|Wheel" { return "Implements PhysX object creation, shape/body/joint sync, filter groups, and paging updates." }
    "RagdollCreator" { return "Implements automatic generation of Node, Collider, RigidBody, Joint, and Muscle objects from skeleton data." }
    "RagdollComponent" { return "Implements ragdoll runtime state, physics ticks, animation mapping, pinning, and muscle blending." }
    "Ragdoll|Muscle" { return "Implements ragdoll import/export, behavior control, muscle constraints, or helper utilities." }
    "IKBase" { return "Implements IK create/destroy, RoleIK binding, async loading, visibility checks, and validity checks." }
    "IK|Bone" { return "Implements concrete IK parameter writeback, target binding, grounder/aim/recoil/hit/limit behavior." }
    "3DUIComponentManager|UI3DComponentManager" { return "Implements 3DUI component queues, render priority, scene UI update, and hit-test related state." }
    "3DUIObject|GObject|GRoot" { return "Implements FGUI object wrapping, GObject lifecycle, and Scene/Screen UI bridge logic." }
    "3DUI|Button|InputText|Image|Text|Loader|List|Tree|Slider|ScrollBar|ComboBox|ProgressBar" { return "Implements concrete FGUI control creation, property sync, events/value update, and child-control binding." }
    "Terrain|Road|Planet|Karst|Voxel|Vegetation|Pavement|Bezier" { return "Implements terrain/road/planet/voxel/vegetation generation, refresh, chunking, or geometry calculations." }
    "TOD|Sun|Sky|Fog|Cloud|Ocean|Weather|Atmosphere|Light|Water|SSAO|HDR" { return "Implements weather/TOD state sync, render-parameter writeback, lighting/sky/water/post-process updates." }
    default { return "Implements runtime behavior for the same-named class/tool; if unclear, continue through references, registration, and call sites." }
  }
}

function Get-ModifyHint([string]$base, [bool]$hasHeader, [bool]$hasSource) {
  switch -Regex ($base) {
    "ComponentSystem" { return "Add/remove component types, managers, or registration macros here; also sync aggregate include headers." }
    "Manager" { return "Start here for global scheduling, batch update, object cache, queue order, or cross-component coordination changes." }
    "BaseComponent" { return "Start here for shared behavior, lifecycle base logic, defaults, and subclass extension points." }
    "ProFunc|Protocol" { return "Start here for script/API/protocol exposed names, parameters, returns, or bridge behavior." }
    "GraphCompiler|GraphExecutor|GraphTask" { return "Start here for PCG execution order, async tasks, DAG compilation, or task-id lifetime." }
    "PCGPin" { return "Start here for PCG node connections, edge breaking, pin compatibility, or stale-edge bugs." }
    "PCGSerializer|PCGReflection|PCGClass|PCGProperty|PCGEnum" { return "Start here for PCG save/load, reflected fields, property editing, and JSON compatibility." }
    "StaticMeshSpawner|SurfaceSampler|DensityFilter|TransformPoints|Difference|Distance|Gather|BoundsModifier" { return "Start here for specific PCG node algorithms, I/O pins, sampling, filtering, or spawn rules." }
    "RigidBody" { return "Start here for mass, inertia, kinematic state, velocity, or PhysX actor create/destroy." }
    "Collider|Shape" { return "Start here for collision shape, attach flow, mesh collider, filter group, or trigger behavior." }
    "Joint" { return "Start here for joint connection, linear/angular limits, drive, breakage, or ragdoll joint behavior." }
    "PhysicsManager|PhysicsPage" { return "Start here for physics paging, queues, StartPhysics/UpdatePhysicsImpl, or world-origin behavior." }
    "RagdollCreator" { return "Start here for generated skeleton rigid bodies, colliders, joint limits, or mass templates." }
    "RagdollComponent" { return "Start here for ragdoll state machine, death/revive, animation-physics blending, or tick order." }
    "RagdollTools|RagdollData" { return "Start here for ragdoll import/export, resource data shape, or historical data compatibility." }
    "IKBase" { return "Start here for IK creation failures, Role switching, async loading, or visibility rebuilds." }
    "Aim|LookAt|Recoil|HitReaction|Grounder|Target|RotationLimit|Limb|CCDIK|FBBIK|BodyTilt|Effector|Penetration|Inertia" { return "Start here for ${base}: inspect IK target binding, weight fields, bone names, collision layer use, and runtime parameter writeback." }
    "BoneRender" { return "Start here for bone visualization, pose import/export, and editor debug display." }
    "3DUIComponentManager" { return "Start here for 3DUI render priority, wait queues, hit state, or global refresh logic." }
    "3DUIComponent$" { return "Start here for package/component loading, Scene/Screen switching, or root GObject binding." }
    "3DUIButton|3DUIInputText|3DUIImage|3DUIText|3DUILoader|3DUIList|3DUITree|3DUISlider|3DUIScrollBar|3DUIComboBox|3DUIProgressBar" { return "Start here for ${base}: inspect concrete control properties, events, value sync, and FGUI type conversion." }
    "Road|Pavement|Bezier" { return "Start here for road network, pavement, curve sampling, or road mesh generation." }
    "Terrain|Planet|Karst|Voxel|Vegetation" { return "Start here for terrain generation, LOD/chunking, planet coordinates, voxel data, or vegetation refresh." }
    "TOD|Sun|Sky|Fog|Cloud|Ocean|Weather|Atmosphere|Light|Water|SSAO|HDR" { return "Start here for weather timeline, sky/sun/cloud/fog/ocean, underwater, or post-process parameters." }
    default {
      if (-not $hasSource) { return "Header-only entry for ${base}: follow include references, registration macros, inheritance, and manager calls for behavior." }
      if (-not $hasHeader) { return "Implementation-only entry for ${base}: inspect static/local helpers and call sites that reference this cpp behavior." }
      return "Start here for ${base}: read .h parameters/reflection first, then .cpp lifecycle, update hooks, and call chain."
    }
  }
}

function LinkCell([string]$rel) {
  if ([string]::IsNullOrWhiteSpace($rel)) { return "No same-name file" }
  $display = $rel -replace "\\", "/"
  return "[$display](file:///E:/Echo_SU_Develop/EchoComponent/$display)"
}

Get-ChildItem -LiteralPath $wikiDir -Filter "*.md" | ForEach-Object {
  $fileName = $_.Name
  $title = $null
  foreach ($candidate in $slugMap.Keys) {
    if ($_.BaseName.EndsWith($candidate, [System.StringComparison]::OrdinalIgnoreCase)) {
      $title = $candidate
      break
    }
  }
  if ([string]::IsNullOrWhiteSpace($title)) { return }
  if (-not $slugMap.ContainsKey($title)) { return }

  $text = Get-Content -Raw -LiteralPath $_.FullName -Encoding UTF8
  $matches = [regex]::Matches($text, '\[(?<rel>(?:Include|Source)/[^\]]+?)\]\(file:///E:/Echo_SU_Develop/EchoComponent/(?<path>(?:Include|Source)/[^\)]+?)\)')
  $pairs = @{}
  foreach ($m in $matches) {
    $rel = ($m.Groups["path"].Value -replace "\\", "/")
    $base = [IO.Path]::GetFileNameWithoutExtension($rel)
    if (-not $pairs.ContainsKey($base)) {
      $pairs[$base] = [ordered]@{ header = $null; source = $null }
    }
    if ($rel -like "Include/*") { $pairs[$base].header = $rel }
    elseif ($rel -like "Source/*") { $pairs[$base].source = $rel }
  }

  $rows = New-Object System.Collections.Generic.List[string]
  foreach ($base in ($pairs.Keys | Sort-Object)) {
    $h = $pairs[$base].header
    $s = $pairs[$base].source
    $hRole = if ($h) { Get-HeaderRole $base } else { "No same-name header; this entry is mainly implementation/tool code." }
    $sRole = if ($s) { Get-CppRole $base } else { "No same-name implementation; usually aggregate include, macro, pure data declaration, or template API." }
    $hint = Get-ModifyHint $base ([bool]$h) ([bool]$s)
    $rows.Add("| $(LinkCell $h) | $(LinkCell $s) | $hRole | $sRole | $hint |")
  }

  $sourceCount = ($pairs.Values | Where-Object { $_.source }).Count
  $headerCount = ($pairs.Values | Where-Object { $_.header }).Count
  $now = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
  $intro = $introMap[$title]
  $content = @"
# EchoComponent Domain Index: $title

> $intro  
> This index was repaired after user feedback: the modification-entry column is no longer a repeated domain-level sentence. Each row now separates header role, implementation role, and file-specific modification entry. Updated: $now.

## Coverage

| Type | Count |
| --- | ---: |
| Source .cpp | $sourceCount |
| Include .h | $headerCount |

## File Index

| Header | Implementation | .h role | .cpp role | Modification entry |
| --- | --- | --- | --- | --- |
$($rows -join "`r`n")

## How To Use

1. Search by concrete feature terms, such as Grounder, PCGPin, Collider, GRoot, TOD, or RoadManager.
2. Use the header column to confirm reflected fields, tunable parameters, inheritance, and public APIs.
3. Use the implementation column to trace lifecycle, runtime behavior, manager scheduling, and actual change points.
4. If a row has no same-name .cpp or .h, keep tracing includes, registration macros, inheritance, manager calls, and call sites.

**Section sources**
- [EchoComponent/Include](file:///E:/Echo_SU_Develop/EchoComponent/Include)
- [EchoComponent/Source](file:///E:/Echo_SU_Develop/EchoComponent/Source)
- [EchoComponentSystem.cpp](file:///E:/Echo_SU_Develop/EchoComponent/Source/EchoComponentSystem.cpp)
"@
  $content | Set-Content -LiteralPath $_.FullName -Encoding UTF8
  $target = Join-Path $importDir $slugMap[$title]
  $content | Set-Content -LiteralPath $target -Encoding UTF8
  Write-Output "rewrote $fileName -> $($slugMap[$title])"
}
