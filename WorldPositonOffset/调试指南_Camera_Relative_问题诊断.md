# Camera-Relative 问题诊断指南

## 🎯 当前状态

### 已修复的关键问题
1. ✅ **GPU层 View 矩阵适配**：3处添加 viewCR（去平移），避免相机被减2次
2. ✅ **世界原点重定位逻辑**：改为动态设置到摄像机附近（不再是固定的 1000000）
3. ✅ **摄像机同步更新**：worldOriginRebasing 中同步更新摄像机位置
4. ✅ **U_WorldMatrix 恢复**：必须保持 camera-relative，否则与其他 uniform 不一致

### 当前问题现象
- ❌ 什么都看不见（黑屏或纯色天空）
- ❌ 可能是地形被完全剔除

---

## 🔍 诊断步骤

### 步骤1：确认 Camera-Relative 是否启用

**检查代码**：`EchoGpuParamsManager.cpp` 第44行
```cpp
static bool g_EnableGpuCameraRelative = true;  // 必须为 true
```

**查看日志**：
```
[CR-STATUS] Frame=120 CameraRelative=ENABLED DebugLog=ON
```

### 步骤2：确认世界原点重定位是否触发

**查看日志**（应在第一帧出现）：
```
[WorldOrigin Rebase] FirstTime=1 Dist=4467.2km 
  OldOrigin=(0.0,0.0,0.0) 
  NewOrigin=(-366000.0,0.0,432000.0) 
  Cam=(-365472.0,160.9,432224.0)
```

**然后应该触发**：
```
[KarstGroup::worldOriginRebasing] 
  OldWorldBase=(0.0,0.0,0.0) 
  NewWorldBase=(-366000.0,0.0,432000.0) 
  Offset=(-366000.0,0.0,432000.0)

[KarstGroup::worldOriginRebasing] Camera 
  OldPos=(-365472.0,160.9,432224.0) 
  NewPos=(528.0,160.9,224.0)  ← 相机变成小坐标！
```

### 步骤3：确认地形块位置是否正确更新

**查看日志**：
```
[TERRAIN-UPDATE] Count=1 
  mBasePos=(-2097152.0,0.0,-1048576.0)  ← 地形初始基准位置
  mWorldBasePos=(-366000.0,0.0,432000.0)  ← 新的世界原点
  mOriginPos=(-1731152.0,0.0,-1480576.0)  ← 计算结果：basePos - worldBasePos

[TERRAIN-UPDATE] Block[0,0] NewPos=(...) KarstSize=32768.0
[TERRAIN-UPDATE] UpdatedBlocks=160/160
```

**关键验证**：
- 摄像机新位置：`(528, 160, 224)`
- 地形块[0,0]新位置应该在摄像机附近（几千米内）
- 如果地形块位置 = `(-1731152, 0, -1480576)`，而摄像机 = `(528, 160, 224)`
- **距离 ≈ 2230 km → 超出视锥 → 被剔除！**

### 步骤4：地形视锥剔除检查

**查看日志**（每60帧）：
```
[TERRAIN] Frame=60 
  CamPos=(528.0,160.9,224.0)  ← 摄像机相对世界原点
  mOriginPos=(-1731152.0,0.0,-1480576.0)  ← 地形相对世界原点
  mWorldBasePos=(-366000.0,0.0,432000.0)
  BlocksTotal=160

[TERRAIN] PartitionLoad px=0 py=13 loadNum=10
[TERRAIN] RenderableCount=0  ← 如果是0，说明被剔除了！
```

---

## 🚨 **根本问题：mOriginPosition 计算可能有误**

### 问题分析

**计算公式**：
```cpp
mOriginPosition = mBasePosition - mWorldBasePosition;
```

**场景数据**（从你的数据推测）：
```
mBasePosition = (-2097152, 0, -1048576)  // 地形在配置中的绝对位置
mWorldBasePosition = (-366000, 0, 432000)  // 新的世界原点（摄像机附近）

mOriginPosition = (-2097152, 0, -1048576) - (-366000, 0, 432000)
                = (-1731152, 0, -1480576)  ← 地形相对于世界原点仍然很远！
```

**摄像机新位置**：
```
相对世界原点 = (-365472, 160, 432224) - (-366000, 0, 432000)
             = (528, 160, 224)  ← 在世界原点附近 ✅
```

**距离**：
```
地形到摄像机 = (-1731152, 0, -1480576) - (528, 160, 224)
             ≈ 2230 km → 超出视锥！❌
```

---

## ✅ **解决方案：修正地形基准位置的理解**

**问题本质**：
- `mBasePosition` 不是"地形块[0,0]的绝对世界位置"
- 而是"地形系统的配置原点"

**可能的修复**（需要验证）：

### 方案A：mBasePosition 就应该是摄像机初始位置附近

检查地形配置：
```cpp
// KarstParams.originPosition 应该设置为什么？
// 如果是 (-2097152, 0, -1048576)，为什么这么远？
```

### 方案B：初始化时应该把世界原点设置到地形基准位置

```cpp
// KarstGroup 构造函数（53-55行）修改为：
mWorldBasePosition = mBasePosition;  // 而不是 Root::instance()->getWorldOrigin()
updateWorldBasePosition();
```

这样：
```
mOriginPosition = mBasePosition - mWorldBasePosition
                = mBasePosition - mBasePosition
                = (0, 0, 0)  ← 地形在世界原点！

地形块[0,0]位置 = (0,0) * 32768 + (0,0,0) = (0,0,0)
```

---

## 📋 **立刻执行的调试步骤**

### 1. 运行并收集以下日志：

**必看日志**（按时间顺序）：
```
[WorldOrigin Rebase] FirstTime=1 ...
[KarstGroup::worldOriginRebasing] ...
[TERRAIN-UPDATE] Count=1 mBasePos=(...) mOriginPos=(...)
[TERRAIN-UPDATE] Block[0,0] NewPos=(...)
[KarstGroup::worldOriginRebasing] Camera OldPos=(...) NewPos=(...)
[TERRAIN] Frame=60 CamPos=(...) mOriginPos=(...) RenderableCount=???
```

### 2. 关键值验证

**计算**：
```
摄像机相对世界原点 = CamPos − mWorldBasePos
地形块相对世界原点 = Block[0,0] NewPos
两者距离 = |摄像机 − 地形块|
```

**如果距离 > 远裁剪面**：
- → 地形被剔除
- → 修复：调整 mBasePosition 或初始 mWorldBasePosition

### 3. 快速验证方案

**临时测试**：把世界原点初始化改为地形基准位置
```cpp
// EchoKarstGroup.cpp:54（构造函数）
// 改为：
mWorldBasePosition = mBasePosition;  // 直接用地形基准作为世界原点
updateWorldBasePosition();
```

然后重新运行，看是否：
- 地形出现在原点附近
- 摄像机位置合理
- 不需要立即触发 changeWorldOrigin

---

## 🎮 预期的正确日志序列

### 初始化（不触发 rebase）
```
[TERRAIN-UPDATE] Count=1 
  mBasePos=(-2097152.0,0.0,-1048576.0)
  mWorldBasePos=(-2097152.0,0.0,-1048576.0)  ← 应该相等
  mOriginPos=(0.0,0.0,0.0)  ← 应该是零向量

[TERRAIN-UPDATE] Block[0,0] NewPos=(-2097152.0,0.0,-1048576.0)
```

### 摄像机移动到初始位置后
```
摄像机实际世界位置 = (-365472, 160, 432224)
地形块[0,0]位置 = (-2097152, 0, -1048576)
距离 ≈ 2230 km  ← 仍然很远，需要 rebase
```

### 第一次触发 rebase
```
[WorldOrigin Rebase] FirstTime=1 Dist=4467.2km
  NewOrigin=(-366000.0,0.0,432000.0)  ← 摄像机附近

[KarstGroup::worldOriginRebasing]
  Offset=(-366000 − (−2097152), 0, 432000 − (−1048576))
       = (1731152, 0, 1480576)  ← 巨大偏移！

Camera NewPos = (-365472, 160, 432224) - (1731152, 0, 1480576)
              = (-2096624, 160, -1048352)  ← 相机也被移得很远！

[TERRAIN-UPDATE] mOriginPos = (-2097152, 0, -1048576) - (-366000, 0, 432000)
                            = (-1731152, 0, -1480576)

Block[0,0] NewPos = (0,0) * 32768 + (-1731152, 0, -1480576)
                  = (-1731152, 0, -1480576)

距离 = |(-2096624, 160, -1048352) − (−1731152, 0, −1480576)|
     ≈ 500 km  ← 仍然太远！
```

---

## 💡 **根本解决方案**

**问题核心**：地形的 `mBasePosition` 与摄像机初始位置相距太远（2000+ km）

**方案1**：修改地形配置，让 `mBasePosition` 接近摄像机初始位置
```cpp
// 在 KarstParams 配置中设置：
params.originPosition = DVector3(-365472, 0, 432224);  // 摄像机初始位置附近
```

**方案2**：初始化时就把世界原点设置到地形基准
```cpp
// EchoKarstGroup.cpp:53-55（构造函数）
mWorldBasePosition = mBasePosition;  // 直接用地形基准
updateWorldBasePosition();
// 这样 mOriginPosition = 0，地形在世界原点
```

**方案3**：增大摄像机远裁剪面（临时）
```cpp
camera->setFarClipDistance(5000000.0f);  // 5000 km
```

---

## 🚀 现在立刻测试

**运行并收集以下日志**：
1. `[WorldOrigin Rebase]` - 第一次重定位
2. `[KarstGroup::worldOriginRebasing]` - 地形响应
3. `[TERRAIN-UPDATE]` - 地形块更新（前3条）
4. `[TERRAIN]` Frame=60 - 运行1秒后的地形状态

**把这4组日志贴给我，我就能精确指出问题在哪！**

---

## 预期修复后的效果

```
摄像机：(528, 160, 224)
地形块[0,0]：(-100, 0, -50)  ← 在摄像机几百米内
距离：√((528+100)² + 160² + (224+50)²) ≈ 700m ✅
→ 在视锥内，正常渲染！
```

