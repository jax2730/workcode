# InstanceBatch 大世界精度修复 - 最小侵入方案

## 修改目标
在大世界坐标系统（如 -1632838, 269747）下，解决 Trunk/TrunkPBR 等 InstanceBatch 渲染的树干模型抖动和着色错误问题，同时保持最小代码侵入。

## 核心问题
原始代码使用 `updateWorldPositionScaleOrientation` 函数传递**绝对世界坐标**给 shader：
```cpp
DVector3 position;  // 高精度
_world_matrix->decomposition(position, scale, orientation);
uniform_f4_assign_pos(_inst_buff[0], position);  // 隐式转换为 Vector3(float)，丢失精度！
```

在大世界坐标下，`DVector3` → `Vector3` 的隐式转换导致精度丢失，引起抖动。

## 修改方案

### 1. CPU 端修改（EchoInstanceBatchEntity.cpp）

#### 保留原有函数，新增 camera-relative 版本
```cpp
// 原有函数保留不变
void updateWorldPositionScaleOrientation(...) { ... }

// 新增函数：传递 camera-relative 坐标
void updateWorldViewPositionScaleOrientation(const SubEntityVec & vecInst, uint32 iUniformCount,
    Uniformf4Vec & vecData)
{
    size_t iInstNum = vecInst.size();
    vecData.resize(iInstNum * iUniformCount, { { 0.f, 0.f, 0.f, 0.f } } );

    // 获取相机位置（高精度）
    DVector3 camPos = DVector3::ZERO;
    if (iInstNum != 0)
    {
        Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
        camPos = pCam->getDerivedPosition();
    }

    for (size_t i=0u; i<iInstNum; ++i)
    {
        SubEntity *pSubEntity = vecInst[i];
        if (nullptr == pSubEntity)
            continue;

        const DBMatrix4 * _world_matrix = nullptr;
        pSubEntity->getWorldTransforms(&_world_matrix);
        DVector3 position;
        Vector3 scale;
        Quaternion orientation;
        _world_matrix->decomposition(position, scale, orientation);
        
        // 关键：在高精度下做 camera-relative 转换
        position -= camPos;
        
        scale.x = non_zero_round(scale.x, 0.00001f);
        scale.y = non_zero_round(scale.y, 0.00001f);
        scale.z = non_zero_round(scale.z, 0.00001f);
        uniform_f4 * _inst_buff = &vecData[i * iUniformCount];
        
        // 现在 position 是小值，转换为 float 不会丢失精度
        Vector3 tempPos = position;
        uniform_f4_assign_pos(_inst_buff[0], tempPos);
        uniform_f4_assign_scale(_inst_buff[1], scale);
        uniform_f4_assign_orientation(_inst_buff[2], orientation);
    }
}
```

#### 修改注册函数
```cpp
// Trunk/TrunkPBR 使用新的 camera-relative 函数
shader_type_register(IBEPri::Util, Trunk,     3, 58, updateWorldViewPositionScaleOrientation);
shader_type_register(IBEPri::Util, TrunkPBR,  3, 58, updateWorldViewPositionScaleOrientation);
```

### 2. Shader 端修改

#### d3d11/TrunkPBR_VS.txt

**关键修改点：**

1. **实例化路径**：CPU 已传 camera-relative，shader 不需要再转换
```hlsl
float4x4 _matWorld = MakeTransform(vPosition.xyz, vScale.xyz, vOrientation);
// instance data is already camera-relative: DO NOT subtract camera again
matWorld[0] = _matWorld[0];
matWorld[1] = _matWorld[1];
matWorld[2] = _matWorld[2];
matWorldCR = _matWorld;  // 用于后续计算
```

2. **投影计算**：使用向量点乘避免行/列主序差异
```hlsl
// Calculate world position (camera-relative) using manual dot products
float4 worldPosCR;
worldPosCR.x = dot(matWorldCR[0], vObjPos);
worldPosCR.y = dot(matWorldCR[1], vObjPos);
worldPosCR.z = dot(matWorldCR[2], vObjPos);
worldPosCR.w = dot(matWorldCR[3], vObjPos);

// Calculate clip position: mul(matVP, worldPosCR)
vsOut.o_PosInClip.x = dot(matVP[0], worldPosCR);
vsOut.o_PosInClip.y = dot(matVP[1], worldPosCR);
vsOut.o_PosInClip.z = dot(matVP[2], worldPosCR);
vsOut.o_PosInClip.w = dot(matVP[3], worldPosCR);
```

3. **传给 PS 的世界坐标**：加回相机位置
```hlsl
// Calculate world position: first get camera-relative position, then add camera position back
float3 worldPosCR;
worldPosCR.x = dot(matWorld[0], vObjPos);
worldPosCR.y = dot(matWorld[1], vObjPos);
worldPosCR.z = dot(matWorld[2], vObjPos);

// Add camera position back to get absolute world space position for PS
vsOut.o_WorldPos.xyz = worldPosCR + U_VS_CameraPosition.xyz;
vsOut.o_WorldPos.w = 1.0;
```

4. **稳定的风动画相位**：使用绝对世界坐标作为种子
```hlsl
float fBendPhase = matWorld[0][3] + matWorld[1][3] + matWorld[2][3];
// Use world-space translation (camera added back) as stable phase seed to avoid jitter
float3 phaseWorldT = float3(matWorld[0][3], matWorld[1][3], matWorld[2][3]) + U_VS_CameraPosition.xyz;
float fStablePhase = phaseWorldT.x + phaseWorldT.y + phaseWorldT.z;
_TrunkBending(vObjPos.xyz, U_VSGeneralRegister0.x, U_VS_Time.x, fStablePhase, ...);
```

#### gles2/TrunkPBR_VS.txt

**保持与 d3d11 一致的逻辑，但矩阵布局不同：**

```glsl
mat4 _matWorld = MakeTransform(vPosition.xyz, vScale.xyz, vOrientation);
// instance data is already camera-relative: DO NOT subtract camera again
matWorld[0] = _matWorld[0];
matWorld[1] = _matWorld[1];
matWorld[2] = _matWorld[2];

mat4 matVP;
matVP[0] = U_ZeroViewProjectMatrix[0];
matVP[1] = U_ZeroViewProjectMatrix[1];
matVP[2] = U_ZeroViewProjectMatrix[2];
matVP[3] = U_ZeroViewProjectMatrix[3];

// GLSL 列主序：使用 _matWorld * matVP
matWVP = _matWorld * matVP;

// 计算世界位置（传给 PS）
o_WPos.xyz = vObjPos * matWorld;
o_WPos.xyz += U_VS_CameraPosition.xyz;  // 加回相机位置
```

## 为什么使用向量点乘？

**问题：** HLSL (d3d11) 使用行主序，GLSL (gles2) 使用列主序，矩阵乘法语法不同。

**解决方案：** 使用向量点乘统一两者：
- `dot(matrix[i], vector)` 在任何布局下都是：矩阵第 i 行 · 向量
- 这避免了理解和维护两种不同矩阵乘法语法的复杂性

**示例：**
```hlsl
// HLSL: 矩阵×列向量
worldPos = mul(matWorld, vObjPos);

// 等价的向量点乘（统一写法）
worldPos.x = dot(matWorld[0], vObjPos);
worldPos.y = dot(matWorld[1], vObjPos);
worldPos.z = dot(matWorld[2], vObjPos);
worldPos.w = dot(matWorld[3], vObjPos);
```

```glsl
// GLSL: 行向量×矩阵
worldPos = vObjPos * matWorld;

// 等价的向量点乘（但注意 matWorld[i] 在 GLSL 中是列）
// 实际上在 GLSL 中仍然直接用矩阵乘法更清晰
o_PosInClip = vObjPos * matWVP;
```

**建议：** 在 HLSL 中使用向量点乘保证清晰性，在 GLSL 中使用原生矩阵乘法（因为它本身就很清晰）。

## 验证要点

1. **精度验证**：在大世界坐标（-1632838, 269747）下，树干不再抖动
2. **视觉验证**：树干光照正常，没有黑色着色错误
3. **性能验证**：帧率与原始代码相当（camera-relative 计算开销很小）
4. **兼容性验证**：PC (d3d11) 和手机 (gles2) 都正确显示

## 未来改进

如果需要支持更多类型（如 BillboardLeaf），可以：
1. 将它们也改为使用 `updateWorldViewPositionScaleOrientation`
2. 在对应 shader 中移除 camera-relative 转换代码
3. 保持相同的向量点乘模式

## 与原始方案的对比

| 方案 | CPU 传递 | Shader 转换 | 精度 | 侵入性 |
|------|---------|------------|------|--------|
| 原始方案 | 绝对世界坐标 (float) | 减去相机位置 | ❌ 丢失精度 | 低 |
| 修改方案 | Camera-relative (float) | 不需要 | ✅ 保持精度 | 中 |

**结论：** 修改方案通过在高精度下做 camera-relative 转换，然后传递小值（float），完美解决了精度问题，同时保持了代码的清晰性和可维护性。


