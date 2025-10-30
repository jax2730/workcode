# 平面植被 Camera-Relative 改造说明

## 背景

- **问题**：平面植被在大世界坐标下会跟随摄像机远近产生抖动，尤其是角色贴身的草叶。
- **根因**：CPU 端上传给 Shader 的位置与角色推力仍使用绝对世界坐标，浮点精度丢失；Shader 已经假定 `U_VSCustom3` 等 uniform 为相机相对坐标，导致 CPU/Shader 坐标系不一致。
- **目标**：在不牺牲原有逻辑的前提下，把平面植被渲染路径改为与行星植被一致的“世界原点重定位”方案。

## 总体思路

1. **缓存双精度世界坐标**：保留 `DVector3` 精度用于包围盒计算或调试。
2. **转换为世界原点相对坐标**：绘制所需的 `Vector3` 都以 `Root::getWorldOrigin()` 为参考，确保浮点数落在摄像机附近。
3. **统一角色推力和相机偏移**：所有提交给 Shader 的 uniform 都遵循相同的相机相对语义，避免 CPU 和 GPU 视角不一致。

## 代码改动

### 1. `_Render::setSphPos` / `_Render::setRenderData`

文件：`g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoPlaneVegetation.cpp`

```cpp
void                                        setSphPos(const Vector3& pos) { setRenderData(pos, mCamPosWorld); };
void                                        setRenderData(const Vector3& sphPos, const DVector3& camPos) {
    // [CR FIX] Cache world positions and convert to world-origin relative space for precision
    const DVector3 worldOrigin = Root::instance()->getWorldOrigin();
    mSphPosWorld = DVector3(sphPos);
    const DVector3 sphRelative = mSphPosWorld - worldOrigin;
    mSphPos = Vector3((float)sphRelative.x, (float)sphRelative.y, (float)sphRelative.z);

    mCamPosWorld = camPos;
    mCamPos = camPos - worldOrigin;

    mTrans.makeTrans(mCamPos);
}
```

**说明**：

- `mSphPosWorld` 和 `mCamPosWorld` 缓存原始双精度坐标，供包围盒和调试使用。
- `mSphPos` 和 `mCamPos` 为相机附近的浮点坐标，直接参与渲染变换和 uniform 上传，避免浮点抖动。
- `setSphPos` 复用缓存的世界相机位置，保证刷新位置时不会丢失基准。

### 2. 成员变量补充

同一文件新增成员：

```cpp
DVector3                mCamPosWorld = DVector3::ZERO;
DVector3                mSphPosWorld = DVector3::ZERO;
```

**说明**：使 `_Render::getBox()` 能继续返回精准的世界包围盒（内部会将 `mBox` 与 `mSphPosWorld` 组合），同时让后续需要绝对坐标的逻辑有据可依。

## 原理解析

- **世界原点重定位**：引擎会在玩家附近维护一个动态世界原点。所有需要高精度的计算都在 CPU 端先减去 `Root::getWorldOrigin()`，使得浮点数保持在 ±几千以内，避免 32 位浮点在 1e6 级别的精度损失。
- **CPU / Shader 一致性**：Shader 侧顶点偏移、角色推力、风动画等都以 camera-relative 数据计算。如果 CPU 上传值仍是绝对坐标，就会出现“CPU 侧大值 + Shader 侧小值”的混合，最终表现为抖动或错位。
- **双轨制数据**：我们同时保留 `DVector3` 与 `Vector3`，既保证了精度，又维持了渲染管线的高效性（GPU 不需要处理双精度）。

## 新人快速理解

1. **牢记两个坐标系**：
   - `*_World` 结尾 ⇒ 双精度世界坐标，只用于 CPU 计算。
   - 无后缀 ⇒ 已减去世界原点的浮点坐标，直接给 GPU 用。
2. **所有提交给 Shader 的位置都要相机相对**。
3. **更新包围盒时记得加回世界偏移**，否则裁剪会出问题。

## 验证步骤

- 在开启相机相对技术的大场景中反复靠近/远离草地，确认没有抖动。
- 打开渲染调试（或日志）检查 `U_VSCustom0/1/3`，确保数值接近零附近而非百万级。
- 若有批渲染路径（`_BatchRender`）仍使用绝对坐标，需要参照本方案同步改动。

## 后续建议

- 审核 `EchoVegetationAreaRender.cpp`、`EchoSubVegetation.cpp` 等其它植被路径，确保同样的相机相对逻辑被落实。
- 同步更新相关 Shader（如 `d3d11/Vegetation_VS.txt`），保持 uniform 语义一致。
- 编写自动化测试：在极大坐标 (1e6) 处渲染植被并截屏对比，以防回归。

---
这份文档可以直接分享给新同事，他们只要掌握“CPU 统一减掉世界原点，再把结果传给 Shader”的习惯，就能快速定位与修复类似的精度问题。
