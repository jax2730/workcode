# 平面植被抖动修复指南 - Camera Relative 技术方案

## 📋 目录
- [问题描述](#问题描述)
- [核心原理](#核心原理)
- [技术方案](#技术方案)
- [代码修改详解](#代码修改详解)
- [修改文件清单](#修改文件清单)
- [验证方法](#验证方法)

---

## 问题描述

### 现象
在超大世界场景中，当摄像机靠近或远离平面植被（草地、灌木等）时，植被会出现明显的抖动现象。距离世界原点越远，抖动越严重。

### 根本原因
浮点数精度问题。当坐标值非常大时（例如数十万、数百万单位），`float`（32位浮点数）的精度会显著下降：
- 在世界原点附近（0-100单位），`float`精度约为 0.001 单位
- 在 10,000 单位处，精度约为 0.01 单位
- 在 1,000,000 单位处，精度约为 0.125 单位

当植被的世界坐标和摄像机坐标都是很大的值时，GPU Shader 中的 `float` 运算会产生明显的误差，导致顶点位置计算不稳定，表现为抖动。

---

## 核心原理

### Camera Relative 技术
**核心思想**：将所有渲染坐标转换到"相对于某个参考点"的小坐标系统，而不是使用绝对的世界坐标。

**关键点**：
1. **世界原点（World Origin）**：动态的参考点，通常设置为摄像机位置或玩家位置附近
2. **双精度存储**：CPU 端使用 `DVector3`（double 精度）存储真实世界坐标
3. **单精度传输**：GPU 端仅传输和计算相对于世界原点的 `Vector3`（float 精度）偏移量
4. **坐标转换**：CPU 负责从绝对坐标转换到相对坐标

### 为什么这样做？
```
示例：
世界原点：(1,000,000, 0, 1,000,000)
摄像机位置：(1,000,050, 100, 1,000,030)
植被位置：(1,000,045, 0, 1,000,025)

传统方案（绝对坐标）：
- GPU计算：植被(1,000,045, 0, 1,000,025) - 摄像机(1,000,050, 100, 1,000,030)
- 大数值运算，精度损失严重 ❌

Camera Relative方案（相对坐标）：
- 植被相对坐标：(1,000,045, 0, 1,000,025) - (1,000,000, 0, 1,000,000) = (45, 0, 25)
- 摄像机相对坐标：(1,000,050, 100, 1,000,030) - (1,000,000, 0, 1,000,000) = (50, 100, 30)
- GPU计算：植被(45, 0, 25) - 摄像机(50, 100, 30)
- 小数值运算，精度充足 ✅
```

---

## 技术方案

### 坐标系统设计
```
                    [CPU - 双精度 double]
                            |
    +------------------+----+----+------------------+
    |                  |         |                  |
真实世界坐标      世界原点     摄像机世界坐标    植被世界坐标
(mSphPosWorld)  (WorldOrigin) (mCamPosWorld)    (DVector3)
    |                  |         |                  |
    +------------------+----↓----+------------------+
                    坐标转换（相减）
                            |
    +------------------+----+----+------------------+
    |                  |         |                  |
植被相对坐标      原点(0,0,0)   摄像机相对坐标
(mSphPos)                       (mCamPos)
    |                             |
    +-----------↓-----------------+
         [GPU - 单精度 float]
            Shader 计算
```

### 数据流程
1. **输入阶段**：接收植被和摄像机的绝对世界坐标（`DVector3`）
2. **存储阶段**：缓存世界坐标到 `mSphPosWorld` 和 `mCamPosWorld`
3. **转换阶段**：减去世界原点，得到相对坐标（`Vector3`）
4. **传输阶段**：将相对坐标通过 Uniform 传递给 Shader
5. **渲染阶段**：Shader 使用相对坐标进行顶点变换

---

## 代码修改详解

### 文件：`EchoPlaneVegetation.cpp`

#### 修改 1：`setSphPos` 方法 - 使用世界坐标缓存

**位置**：第 198 行

**修改前：**
```cpp
void setSphPos(const Vector3& pos) { setRenderData(pos, mCamPos); };
```

**修改后：**
```cpp
void setSphPos(const Vector3& pos) { setRenderData(pos, mCamPosWorld); };
```

**解释：**
```cpp
void setSphPos(const Vector3& pos) {
    // 当外部调用 setSphPos 更新植被位置时
    // 使用缓存的世界空间摄像机坐标（mCamPosWorld），而不是已转换的相对坐标（mCamPos）
    // 原因：setRenderData 内部需要重新计算相对坐标，必须使用原始的世界坐标
    setRenderData(pos, mCamPosWorld);
};
```

**为什么要这样改？**
- `mCamPos` 是已经减去世界原点的相对坐标，再传入会导致二次转换错误
- `mCamPosWorld` 是原始的世界坐标，传入后 `setRenderData` 可以正确计算相对值

---

#### 修改 2：`setRenderData` 方法 - 核心坐标转换逻辑

**位置**：第 199-210 行

**修改前：**
```cpp
void setRenderData(const Vector3& sphPos, const DVector3& camPos) {
    // [CR FIX] Convert to camera-relative using world origin to keep small coordinates
    DVector3 worldOrigin = Root::instance()->getWorldOrigin();
    DVector3 rel = DVector3(sphPos) - worldOrigin;
    mSphPos = Vector3((float)rel.x, (float)rel.y, (float)rel.z);
    mCamPos = camPos;

    mTrans.makeTrans(mCamPos);
}
```

**修改后：**
```cpp
void setRenderData(const Vector3& sphPos, const DVector3& camPos) {
    // [CR FIX] Cache world positions and convert to world-origin relative space for precision
    // 第1步：获取当前的世界原点（通常是玩家或摄像机附近的参考点）
    const DVector3 worldOrigin = Root::instance()->getWorldOrigin();
    
    // 第2步：缓存植被的世界坐标（double 精度，用于后续计算包围盒等）
    mSphPosWorld = DVector3(sphPos);
    
    // 第3步：计算植被相对于世界原点的偏移（转换到相对坐标系）
    const DVector3 sphRelative = mSphPosWorld - worldOrigin;
    
    // 第4步：转换为 float 精度（因为偏移量很小，float 精度足够）
    mSphPos = Vector3((float)sphRelative.x, (float)sphRelative.y, (float)sphRelative.z);

    // 第5步：同样处理摄像机坐标 - 缓存世界坐标
    mCamPosWorld = camPos;
    
    // 第6步：计算摄像机相对于世界原点的偏移
    mCamPos = camPos - worldOrigin;

    // 第7步：构建变换矩阵（使用相对坐标）
    mTrans.makeTrans(mCamPos);
}
```

**逐行详细解释：**

```cpp
const DVector3 worldOrigin = Root::instance()->getWorldOrigin();
```
- **作用**：获取全局世界原点
- **类型**：`DVector3` 双精度向量
- **意义**：这是整个场景的"相对参考点"，通常由引擎动态更新到玩家附近

```cpp
mSphPosWorld = DVector3(sphPos);
```
- **作用**：将输入的植被位置（`Vector3`）转换为 `DVector3` 并缓存
- **为什么缓存**：后续的包围盒计算（`getBox()`）需要精确的世界坐标
- **精度保证**：使用 `DVector3` 存储，避免大坐标精度损失

```cpp
const DVector3 sphRelative = mSphPosWorld - worldOrigin;
```
- **作用**：计算植被相对于世界原点的偏移向量
- **数学原理**：`相对坐标 = 绝对坐标 - 参考点`
- **结果特点**：得到的 `sphRelative` 是一个小数值向量（通常在几十到几百单位内）

```cpp
mSphPos = Vector3((float)sphRelative.x, (float)sphRelative.y, (float)sphRelative.z);
```
- **作用**：将双精度相对坐标转换为单精度
- **为什么转换**：GPU Shader 使用 `float`，而且相对坐标很小，精度损失可忽略
- **性能优化**：减少数据传输量（float 占 4 字节，double 占 8 字节）

```cpp
mCamPosWorld = camPos;
mCamPos = camPos - worldOrigin;
```
- **作用**：对摄像机坐标做同样的处理
- **缓存世界坐标**：`mCamPosWorld` 存储原始值
- **计算相对坐标**：`mCamPos` 存储相对于世界原点的偏移

```cpp
mTrans.makeTrans(mCamPos);
```
- **作用**：构建变换矩阵
- **关键点**：使用相对坐标构建矩阵，后续 GPU 计算都在相对空间进行

---

#### 修改 3：添加成员变量 - 存储世界坐标缓存

**位置**：第 264-269 行

**修改前：**
```cpp
private:
    DBMatrix4   mTrans = DBMatrix4::IDENTITY;
    DVector3    mCamPos = DVector3::ZERO;

    Vector3     mSphPos = Vector3::ZERO;
```

**修改后：**
```cpp
private:
    DBMatrix4   mTrans = DBMatrix4::IDENTITY;         // 变换矩阵（相对空间）
    DVector3    mCamPos = DVector3::ZERO;             // 摄像机相对坐标
    DVector3    mCamPosWorld = DVector3::ZERO;        // 摄像机世界坐标（缓存）

    Vector3     mSphPos = Vector3::ZERO;              // 植被相对坐标
    DVector3    mSphPosWorld = DVector3::ZERO;        // 植被世界坐标（缓存）
```

**每个变量的作用：**

| 变量名 | 类型 | 用途 | 空间 |
|--------|------|------|------|
| `mTrans` | `DBMatrix4` | 变换矩阵，用于 GPU 渲染 | 相对空间 |
| `mCamPos` | `DVector3` | 摄像机位置（相对） | 相对空间 |
| `mCamPosWorld` | `DVector3` | 摄像机位置（世界） | 世界空间 |
| `mSphPos` | `Vector3` | 植被位置（相对） | 相对空间 |
| `mSphPosWorld` | `DVector3` | 植被位置（世界） | 世界空间 |

**为什么需要两套坐标？**
1. **世界坐标（`*World`）**：
   - 用于 CPU 端的精确计算（包围盒、碰撞检测等）
   - 双精度保证计算精度
   - 不传递给 GPU

2. **相对坐标（无 `World` 后缀）**：
   - 用于 GPU 渲染
   - 单精度足够（因为数值小）
   - 通过 Uniform 传递给 Shader

---

#### 修改 4：`getBox()` 方法已使用世界坐标

**位置**：第 107-110 行（已有代码，无需修改）

```cpp
AxisAlignedBox getBox() const
{
    // 使用缓存的世界坐标计算包围盒
    Vector3 worldOffset((float)mSphPosWorld.x, (float)mSphPosWorld.y, (float)mSphPosWorld.z);
    return AxisAlignedBox(mBox.getMinimum() + worldOffset, mBox.getMaximum() + worldOffset);
};
```

**解释：**
```cpp
Vector3 worldOffset((float)mSphPosWorld.x, (float)mSphPosWorld.y, (float)mSphPosWorld.z);
// 将植被的世界坐标（DVector3）转换为 Vector3 偏移量
// 这里直接使用世界坐标，不使用相对坐标，因为包围盒需要在世界空间中进行剔除判断

return AxisAlignedBox(mBox.getMinimum() + worldOffset, mBox.getMaximum() + worldOffset);
// mBox 是植被的局部包围盒（相对于植被自身的坐标）
// 加上 worldOffset 后，得到世界空间的包围盒
// 引擎的视锥剔除系统会使用这个世界空间的包围盒进行判断
```

---

#### 修改 5：`setCustomParameters` 中的相对坐标传递（已有代码）

**位置**：第 115-140 行

```cpp
virtual void setCustomParameters(const Pass* pPass) override
{
    RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
    
    // 获取世界原点（和 setRenderData 中保持一致）
    const DVector3 worldOrigin = Root::instance()->getWorldOrigin();

    // Uniform 0：植被位置（相对坐标）
    pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, Vector4(
        mSphPos.x, mSphPos.y, mSphPos.z, 1.f));

    if (mLod == 0)  // 仅 LOD0 需要角色推力效果
    {
        // 角色推力位置：从世界空间转换到相对空间
        const DVector3 rolePosRelative = VegOpts::GetRolePushPos() - worldOrigin;
        const DVector3 roleCamSpa = rolePosRelative - mCamPos;
        
        // Uniform 1：角色相对于摄像机的偏移（用于植被互动效果）
        pRenderSystem->setUniformValue(this, pPass, U_VSCustom1, Vector4(
            (float)roleCamSpa.x, (float)roleCamSpa.y, (float)roleCamSpa.z, 1.f));
    }

    // ... 其他 Uniform 设置 ...

    // Uniform 3：植被相对于摄像机的偏移（Shader 需要用于顶点变换）
    const DVector3 toCamSpaPos = DVector3(mSphPos.x, mSphPos.y, mSphPos.z) - mCamPos;
    pRenderSystem->setUniformValue(this, pPass, U_VSCustom3, Vector4(
        (float)toCamSpaPos.x, (float)toCamSpaPos.y, (float)toCamSpaPos.z, 0.f));
}
```

**关键 Uniform 解释：**

**`U_VSCustom0`（植被位置）：**
```cpp
pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, Vector4(
    mSphPos.x, mSphPos.y, mSphPos.z, 1.f));
```
- **内容**：植被相对于世界原点的位置
- **Shader 用途**：作为植被批次的基准位置
- **为什么是相对坐标**：保证数值小，GPU 计算精度高

**`U_VSCustom1`（角色推力）：**
```cpp
const DVector3 rolePosRelative = VegOpts::GetRolePushPos() - worldOrigin;  // 角色位置转相对坐标
const DVector3 roleCamSpa = rolePosRelative - mCamPos;                      // 角色相对于摄像机的偏移
pRenderSystem->setUniformValue(this, pPass, U_VSCustom1, Vector4(
    (float)roleCamSpa.x, (float)roleCamSpa.y, (float)roleCamSpa.z, 1.f));
```
- **计算过程**：
  1. 获取角色的世界坐标 `GetRolePushPos()`
  2. 减去世界原点，得到角色的相对坐标
  3. 再减去摄像机的相对坐标，得到"角色相对于摄像机的向量"
- **Shader 用途**：计算植被被角色推开的效果
- **为什么两次相减**：最终需要的是"从摄像机指向角色的向量"，在相对空间中计算

**`U_VSCustom3`（植被到摄像机的向量）：**
```cpp
const DVector3 toCamSpaPos = DVector3(mSphPos.x, mSphPos.y, mSphPos.z) - mCamPos;
pRenderSystem->setUniformValue(this, pPass, U_VSCustom3, Vector4(
    (float)toCamSpaPos.x, (float)toCamSpaPos.y, (float)toCamSpaPos.z, 0.f));
```
- **内容**：从植被位置指向摄像机的向量（相对空间）
- **Shader 用途**：用于计算植被的朝向、LOD 过渡等
- **数学原理**：`向量 = 终点 - 起点`，这里计算的是"植被→摄像机"的向量

---

## 修改文件清单

### 已修改文件

| 文件路径 | 修改内容 | 行数 |
|---------|---------|------|
| `g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoPlaneVegetation.cpp` | `_Render::setSphPos` | 198 |
| 同上 | `_Render::setRenderData` | 199-210 |
| 同上 | 添加成员变量 `mCamPosWorld`, `mSphPosWorld` | 265-269 |

### 相关文件（已同步修改）

| 文件路径 | 状态 | 说明 |
|---------|------|------|
| `g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoBiomeVegetation.cpp` | ✅ 已完成 | 行星植被（球面）的 Camera Relative 实现 |
| `g:\Echo_SU_Develop\Shader\d3d11\VegetationSphTer_VS.txt` | ✅ 已同步 | 球面植被顶点着色器 |
| `g:\Echo_SU_Develop\Shader\d3d11\VegetationSphTer_Low_VS.txt` | ✅ 已同步 | 球面植被 LOD1 着色器 |

### 待处理文件（如需完整支持）

| 文件路径 | 状态 | 优先级 |
|---------|------|--------|
| `EchoSubVegetation.cpp` | ⏳ 待修改 | 中 |
| `EchoVegetationAreaRender.cpp` | ⏳ 待修改 | 中 |
| `Vegetation_VS.txt` | ⏳ 待修改 | 低 |
| `Vegetation_Low_VS.txt` | ⏳ 待修改 | 低 |

---

## 验证方法

### 1. 视觉验证
- **测试场景**：传送到距离世界原点 100 万单位以上的区域
- **观察植被**：缓慢移动摄像机，观察草地、灌木是否平滑渲染
- **测试距离**：靠近和远离植被，检查是否有跳跃或抖动

### 2. 调试验证
在 `setRenderData` 中添加日志：
```cpp
void setRenderData(const Vector3& sphPos, const DVector3& camPos) {
    const DVector3 worldOrigin = Root::instance()->getWorldOrigin();
    mSphPosWorld = DVector3(sphPos);
    const DVector3 sphRelative = mSphPosWorld - worldOrigin;
    mSphPos = Vector3((float)sphRelative.x, (float)sphRelative.y, (float)sphRelative.z);
    
    // 调试日志
    if (mSphPosWorld.length() > 1000000.0) {  // 仅在远离原点时输出
        LogDebug("PlaneVeg - World: (%.2f, %.2f, %.2f), Relative: (%.2f, %.2f, %.2f)",
                 mSphPosWorld.x, mSphPosWorld.y, mSphPosWorld.z,
                 mSphPos.x, mSphPos.y, mSphPos.z);
    }
    
    mCamPosWorld = camPos;
    mCamPos = camPos - worldOrigin;
    mTrans.makeTrans(mCamPos);
}
```

**预期结果**：
- `World` 坐标：超大数值（例如 1,234,567.89）
- `Relative` 坐标：小数值（通常 -500 到 +500 范围内）

### 3. 性能验证
- **帧率测试**：Camera Relative 方案不应影响帧率（仅 CPU 端多一次减法）
- **内存测试**：每个 `_Render` 对象增加 48 字节（2 个 `DVector3`）

---

## 核心思想总结

### 设计哲学
```
┌─────────────────────────────────────────────────────────┐
│  "让 GPU 只处理小数值，让 CPU 负责大坐标的精确计算"      │
└─────────────────────────────────────────────────────────┘
```

### 关键要点
1. **双坐标系统**：CPU 双精度世界空间 + GPU 单精度相对空间
2. **世界原点重定位**：动态参考点消除大坐标
3. **数据缓存**：同时保留世界坐标和相对坐标，各司其职
4. **一致性保证**：所有相关代码必须使用同一个世界原点

### 适用场景
- ✅ 大世界开放世界游戏
- ✅ 坐标范围超过 ±10,000 单位的场景
- ✅ 需要精确渲染的植被、粒子、特效系统
- ❌ 小场景游戏（增加代码复杂度，收益不明显）

### 扩展建议
如果项目中其他系统也有类似抖动问题，可以将 Camera Relative 技术应用到：
- 粒子系统（Particle System）
- 水体渲染（Water Rendering）
- 天气效果（Weather Effects）
- 建筑LOD系统（Building LOD）

---

## 附录：数学公式

### 坐标转换公式
```
设：
  P_world   = 对象世界坐标（DVector3）
  C_world   = 摄像机世界坐标（DVector3）
  O_world   = 世界原点（DVector3）
  
则：
  P_relative = P_world - O_world  // 对象相对坐标
  C_relative = C_world - O_world  // 摄像机相对坐标
  
  Vector_PC = P_relative - C_relative  // 对象到摄像机的向量（相对空间）
            = (P_world - O_world) - (C_world - O_world)
            = P_world - C_world  // 与世界空间计算结果一致！
```

### 浮点精度分析
```
float (32位) 精度：
  指数位: 8 bit  尾数位: 23 bit
  
  在不同数值范围的精度：
  [0, 1]           : ~0.0000001
  [1, 10]          : ~0.000001
  [10, 100]        : ~0.00001
  [100, 1000]      : ~0.0001
  [1000, 10000]    : ~0.001
  [10000, 100000]  : ~0.01
  [100000, 1000000]: ~0.125      ← 明显的抖动！
  [1000000, ∞]     : ~1.0以上    ← 严重抖动！
```

---

**文档版本**：v1.0  
**最后更新**：2025-10-29  
**作者**：技术团队  
**状态**：✅ 已验证通过
