# 大世界浮点精度问题 - Camera-Relative 技术实现记录

## 项目背景

### 核心问题
在大世界游戏场景中，当使用绝对世界坐标（例如 (-365478.4, 11.7, 432220.3)）时，由于坐标数值过大（7位数以上），`float` 类型的精度损失会导致：
- 物体抖动（jittering）
- 纹理错位
- 法线闪烁
- 远处地形变成一条"线"

### 解决方案：Camera-Relative (CR) 渲染
采用"相机相对坐标系"技术：
- **CPU侧**：所有逻辑、UI、寻路等保持使用**世界绝对坐标**（大坐标）
- **GPU侧**：渲染时转换为**相机相对坐标** `p_relative = p_world - camera_world`
- **世界原点重定位**：定期将 `worldOrigin` 设置为相机附近，使相对坐标保持在较小范围内

---

## 关键用户提问与技术要点

### 问题1：坐标系一致性
**用户提问**：
> "更新的摄像机大坐标是人物在整个地图中的位置，你如果要改的话就应该也用相对。例如你改了后，人物的坐标在(10, 10, 10)，那么人物位置就处于地图的(10, 10, 10)点，而(-365472.0,6.8,432214.4)点是人物处于地图的正确位置。"

**核心要点**：
- CPU必须始终保持**世界绝对坐标**作为唯一真值
- GPU渲染时才转换为相对坐标
- 不能修改Camera的`mPosition`为相对坐标，否则外部系统会读取错误位置

### 问题2：偏移基准选择
**用户提问**：
> "我们采用的偏移方法，计算等逻辑？UI显示的(-365478.4,11.7,432220.3)坐标确实是人物所处的位置"

**技术决策**：
采用 `worldOrigin` 作为偏移基准：
- `camRelative = camWorld - worldOrigin`
- `objectRelative = objectWorld - worldOrigin`
- 统一所有对象在同一参照系下进入GPU管线

### 问题3：矩阵变换统一
**日志现象**：
```
W.t=(0.000,839.120,0.000) VW.t=(365472.031,678.229,-432223.969)
```
W的平移已很小，但VW（ViewWorld）的平移仍然巨大，说明View矩阵与World矩阵不在同一坐标系。

**根本原因**：
- World矩阵做了CR转换，但View矩阵仍保留世界坐标的相机平移
- 导致 `View * World` 时坐标系混用，产生错误的变换

---

## 代码修改演进过程

### 第一阶段：尝试条件判定（失败）
```cpp
// 尝试根据对象类型决定是否做CR转换
bool worldMatHasLargeTrans = (fabs(w[0][3]) > 10000.0f);
bool worldPosIsLarge = (fabs(pWorldPosition->x) > 10000.0);
if (worldMatHasLargeTrans || worldPosIsLarge) {
    w[0][3] -= camRelative.x; // 只对"世界坐标对象"减
}
```

**问题**：
- 地形Patch的坐标已经是相对的，不需要再减
- 但植被/人物是世界坐标，需要减
- 条件判定导致不同对象在不同坐标系，View矩阵无法统一

### 第二阶段：世界原点偏移方案（失败）
```cpp
// 尝试统一减 worldOrigin
w[0][3] -= worldOrigin.x;
// View 平移改为 -R^T*(camWorld - worldOrigin)
viewCR[0][3] = viewMatrix[0][3] + R^T*worldOrigin;
```

**问题**：
- 增加了View矩阵的复杂度
- 与原始View矩阵定义冲突
- 仍然存在坐标系混用

### 第三阶段：最终方案（当前实现）

#### 核心原则
1. **CPU侧**：所有数据保持世界绝对坐标
2. **GPU侧**：矩阵链路统一做Camera-Relative转换

#### 具体实现

**A. Uniform变量输出策略**

```cpp
// U_WorldMatrix / U_InvWorldMatrix / U_WorldPosition
// 输出：世界坐标（不做CR转换）
case U_WorldMatrix:
{
    Matrix4 tempMat = *m_pWorldMatrix; // 保持世界坐标
    // 直接拷贝给GPU
}

// U_CameraPosition / U_VS_CameraPosition
// CR模式：(0,0,0) - 相机在CR空间的位置
case U_VS_CameraPosition:
{
    if (g_EnableGpuCameraRelative) {
        pVector4->x = 0.0f; pVector4->y = 0.0f; pVector4->z = 0.0f;
    } else {
        pVector4 = camWorld; // 非CR模式用世界坐标
    }
}
```

**B. 矩阵变换链路统一**

```cpp
// U_WorldViewProjectMatrix / U_VS_WorldViewMatrix / U_VS_InvWorldViewMatrix
if (g_EnableGpuCameraRelative)
{
    Matrix4 w = *m_pWorldMatrix;
    const DVector3 camWorld = pCam->getPosition();
    const DVector3 worldOrigin = Root::instance()->getWorldOrigin();
    const DVector3 camRelative = camWorld - worldOrigin;
    
    // 关键1：无条件对W做CR转换
    w[0][3] -= (float)camRelative.x;
    w[1][3] -= (float)camRelative.y;
    w[2][3] -= (float)camRelative.z;
    
    // 关键2：View矩阵去掉平移（相机在原点）
    Matrix4 viewCR = viewMatrix;
    viewCR[0][3] = 0.0f;
    viewCR[1][3] = 0.0f;
    viewCR[2][3] = 0.0f;
    
    // MVP = P * V' * W'
    *pMatrix4 = projectMatrix * viewCR * w;
}
```

**C. Shader端配合修改**

```hlsl
// KarstTerrainBase_VS.txt - 地形球面映射修复
float3 remapPosition(float3 pos)
{
    bool crMode = (length(U_VS_CameraPosition.xyz) < 0.5);
    if (crMode)
    {
        // CR模式：从ViewMatrix还原相机世界坐标
        float3 camWorld = getCameraWorldFromView();
        float3 worldPos = pos + camWorld; // 转回世界坐标
        // 执行球面映射
        float3 dir = normalize(worldPos - EARTHCENTER);
        float3 sphereWorld = dir * (RADIUS + height) + EARTHCENTER;
        return sphereWorld - camWorld; // 转回CR坐标
    }
    // 非CR模式：原始逻辑
}
```

---

## 技术细节说明

### 1. 为什么View矩阵要去平移？

**原始View矩阵**：
```
V = [ R | -R^T * C ]
```
其中 C 是相机世界坐标，R 是旋转矩阵。

**CR模式下的View**：
相机在CR空间位于原点，所以：
```
V' = [ R | 0 ]
```

**推导**：
```
原始: p_view = V * p_world = R * p_world - R^T * C
CR:   p_view = V' * p_cr = R * (p_world - C) = R * p_world - R * C
```
两者等价，但CR模式下直接将平移设为0更简洁。

### 2. 为什么要无条件对W做CR转换？

**错误做法**（条件判定）：
```cpp
if (是世界坐标对象) w -= camRelative;
if (是相对坐标对象) w不变;
```
导致不同对象在不同坐标系，View无法统一。

**正确做法**（统一转换）：
```cpp
// 所有对象的W都从世界坐标开始
w = worldMatrix; // 来自CPU，世界坐标
// 统一做CR转换
w.translation -= camRelative;
// 配合 View' (平移为0)
MVP = P * V' * W'
```

这样所有对象在GPU端统一进入CR空间。

### 3. 地形Patch的特殊处理

**问题**：地形的 `PatchPosition` 已经是相对坐标（base - worldOrigin）

**解决方案1（当前）**：
- Shader接收的 `PatchPosition` 保持相对坐标
- 但在构建worldPos时，由于GPU端的World矩阵是Identity，直接使用
- View矩阵已调整为CR模式，整体坐标系一致

**解决方案2（可选）**：
- 让地形也传世界坐标的PatchPosition
- GPU端统一做CR转换
- 更彻底的统一，但需要修改CPU端地形代码

---

## 当前状态与待解决问题

### 已完成
- ✅ CPU侧保持世界绝对坐标
- ✅ GPU矩阵链路统一CR转换
- ✅ View矩阵去平移
- ✅ U_VS_CameraPosition 在CR模式传(0,0,0)
- ✅ U_WorldMatrix 保持世界坐标输出
- ✅ 地形shader球面映射修复

### 当前问题
根据最新日志：
```
[TERRAIN] RenderableCount=16  // 地形有16个可渲染块
[TERRAIN] TerrainOriginToCamera Distance=2278.5km  // 距离仍然很大
```

**症状**：地形、植被、人物模型都消失

**可能原因**：
1. **Frustum Culling问题**：距离2278km远超剔除阈值(<50km)，被错误剔除
2. **坐标系残留混用**：某些路径仍在使用错误的坐标系
3. **Shader端距离计算错误**：植被淡出、LOD等使用了错误的距离

### 下一步排查方向

#### A. 确认CR日志输出
需要查看最新日志中的：
```
[CR-FULL] U_WorldViewProjectMatrix ... W.t=? VW.t=?
```
- 期望：W.t 为小值（几百），VW.t 也为小值
- 如果VW.t仍然巨大，说明View矩阵修改未生效

#### B. 地形距离计算修复
```cpp
// EchoKarstGroup.cpp - findVisibleObjects
// 当前问题：mOriginPosition 是相对坐标，但camPos是世界坐标
DVector3 terrainToCamera = camPos - mOriginPosition;  // 坐标系混用！

// 修复：统一为相对坐标
DVector3 camRelative = camPos - worldOrigin;
DVector3 terrainToCamera = camRelative - mOriginPosition;
```

#### C. 植被shader距离修复
```hlsl
// Vegetation_Instance_VS.txt
// 当前：
float3 vCamToVeg = vWorldRoot - U_VS_CameraPosition.xyz;
// U_VS_CameraPosition = (0,0,0) 在CR模式
// vWorldRoot 是世界坐标，计算错误！

// 修复：显式转换
float3 vCamToVeg = vWorldRoot - getCameraWorldFromView();
// 或者确保 vWorldRoot 已经是CR坐标
```

---

## 关键经验总结

### 1. 坐标系一致性原则
**所有参与同一次运算的坐标，必须在同一坐标系下。**
- CPU逻辑：统一世界坐标
- GPU渲染：统一CR坐标
- 不允许在同一个矩阵链中混用

### 2. CPU作为唯一真值源
**Camera::mPosition、Object::mWorldPosition 等必须始终保持世界绝对坐标。**
- UI显示：直接读取世界坐标
- 物理/寻路：使用世界坐标
- 渲染提交时才转换为CR

### 3. 矩阵变换的传递性
```
若 W' = W - C (CR转换)
则 V' 必须配合调整为 V' = V + R^T*C
或者简化为 V' = [R | 0]
```
否则 `V * W'` 会产生双重减法或加法。

### 4. Shader端的隐式假设
许多shader假设：
- `U_VS_CameraPosition` 是相机位置（用于计算距离）
- `U_WorldMatrix` 是对象世界变换
- `worldPos = transform(U_WorldMatrix, objectPos)` 是世界坐标

在CR模式下这些假设会被打破，需要：
- 显式文档说明CR模式的语义
- 或者在shader内部做坐标系转换

### 5. 调试日志的重要性
```cpp
// 关键诊断信息
[CR-FULL] W.t=? VW.t=? MVP.t=?
[TERRAIN] Distance=? RenderableCount=?
[GPU-CR] OriginalTrans=? CamRelative=?
```
通过这些日志可以快速定位：
- 矩阵是否正确转换
- 对象是否被错误剔除
- 坐标系是否一致

---

## 代码文件清单

### 核心修改文件
1. **WorldPositonOffset/EchoGpuParamsManager.cpp**
   - Uniform变量提交逻辑
   - CR模式的矩阵变换
   - 关键函数：`commitUniform()`

2. **WorldPositonOffset/EchoKarstGroup.cpp**
   - 地形渲染和加载
   - 世界原点重定位监听
   - 距离计算修复

3. **WorldPositonOffset/EchoMiniMap.cpp**
   - 世界原点重定位触发
   - 玩家位置管理

4. **WorldPositonOffset/EchoRoot.cpp**
   - 世界原点广播
   - Listener回调管理

5. **d3d11/KarstTerrainBase_VS.txt**
   - 地形vertex shader
   - 球面映射CR修复

6. **d3d11/Vegetation_Instance_VS.txt**
   - 植被vertex shader
   - 距离淡出计算（待修复）

7. **d3d11/IllumPBR2023_VS.txt**
   - 角色vertex shader
   - 可能需要类似修复

### 配置变量
```cpp
// EchoGpuParamsManager.cpp
static bool g_EnableGpuCameraRelative = true;  // CR模式开关
static bool g_DebugCameraRelative = false;      // 调试日志开关
```

---

## 测试验证清单

### 功能验证
- [ ] 地形正常显示，无抖动
- [ ] 地形远处不变成"线"
- [ ] 植被正常显示
- [ ] 角色正常显示
- [ ] UI坐标显示正确（世界绝对坐标）
- [ ] 移动流畅，无卡顿

### 精度验证
- [ ] 近距离观察物体，无微小抖动
- [ ] 纹理对齐正确
- [ ] 法线无闪烁
- [ ] 阴影稳定

### 性能验证
- [ ] 帧率无明显下降
- [ ] CPU占用正常
- [ ] GPU占用正常

### 边界测试
- [ ] 世界原点重定位时无卡顿
- [ ] 快速移动无异常
- [ ] 视角旋转流畅
- [ ] 切换CR模式开关，对比效果

---

## 参考资料

### 相关文档
- `大世界浮点精度_CameraRelative技术方案.md` - 数学原理详细推导
- `2025-10-21_06-35Z-终极编程牛马的身份定位.md` - 早期实现记录

### 关键概念
- **Floating-point precision loss**: IEEE 754标准下float的有效精度约7位
- **Camera-Relative Rendering**: 业界标准大世界精度解决方案
- **Frustum Culling**: 视锥体剔除，基于距离判定
- **World Origin Rebasing**: 动态重定位世界原点

### 业界实践
- Unreal Engine 5: Large World Coordinates (LWC)
- Unity: Floating Origin
- 许多MMO游戏：分区+相对坐标

---

## 待优化项

### 性能优化
1. 减少矩阵运算次数
2. 缓存常用的CR转换结果
3. 批量更新WorldOrigin

### 代码优化
1. 抽取CR转换为统一函数
2. 减少重复的坐标系判定
3. 简化日志输出

### 功能扩展
1. 支持多相机CR
2. 支持动态切换CR模式
3. 自动化测试脚本

---

## 联系与支持

如果遇到问题：
1. 检查 `Echo.log` 中的 `[CR-FULL]`, `[TERRAIN]`, `[GPU-CR]` 日志
2. 确认 `g_EnableGpuCameraRelative` 和 `g_DebugCameraRelative` 设置
3. 对比修改前后的日志差异
4. 参考本文档的"下一步排查方向"章节

---

**文档版本**: v1.0  
**最后更新**: 2025-10-27  
**状态**: 开发中 - 等待最新测试结果

