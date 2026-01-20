# TrunkPBR Camera-Relative 修复记录

## 问题描述

**症状：** 
- 合批前（非实例化渲染）：Trunk模型显示正常
- 合批后（实例化渲染）：Trunk模型颜色异常、显示不正常

**根本原因：**
实例化渲染路径中的相机相对坐标处理错误，导致世界空间坐标计算错误。

## 问题分析

### CPU端数据传递
```cpp
// EchoInstanceBatchEntity.cpp - updateWorldViewPositionScaleOrientation()
void updateWorldViewPositionScaleOrientation(const SubEntityVec & vecInst, uint32 iUniformCount,
    Uniformf4Vec & vecData)
{
    // 获取相机位置
    DVector3 camPos = DVector3::ZERO;
    if (iInstNum != 0) {
        Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
        camPos = pCam->getDerivedPosition();
    }
    
    // 发送camera-relative位置到GPU
    DVector3 position;
    _world_matrix->decomposition(position, scale, orientation);
    position -= camPos;  // camera-relative
    
    uniform_f4_assign_pos(_inst_buff[0], Vector3(position));  // 传递camera-relative坐标
}
```

**关键点：** CPU传递给GPU的 `vPosition` 是 **camera-relative** 的坐标（world_position - camera_position）

### Shader端处理错误

#### 原始错误代码（GLES）
```glsl
#ifdef USEINSTANCE
    vec3 vMVPosition = vPosition.xyz;  // vPosition 是 camera-relative
    mat4 _matWorld = MakeTransform(vMVPosition.xyz, vScale.xyz, vOrientation);
    matWorld[0] = _matWorld[0];  // matWorld 存储的是 camera-relative 矩阵
    
    // 错误！matWorld 是 camera-relative，但后续代码把它当作 world-space 使用
    o_WPos.xyz = vObjPos * matWorld;  // 输出的是 camera-relative 坐标
    // 片段着色器期望 world-space 坐标进行光照计算！
#endif
```

#### 问题
1. `matWorld` 使用 camera-relative 坐标构建
2. 片段着色器期望 `o_WPos` 是 world-space 坐标用于光照计算
3. 结果：光照计算错误 → 颜色异常

## 解决方案

### 核心思路
**分离两种矩阵：**
1. **matWorld (世界空间)**: 用于光照计算、法线变换、风力相位等
2. **_matWorld (相机相对空间)**: 用于顶点变换和裁剪空间计算

### 修复代码

#### GLES2 (TrunkPBR_VS.txt, Trunk_VS.txt)
```glsl
#ifdef USEINSTANCE
    vec4 vPosition = U_VSCustom0[idxInst + 0];  // camera-relative from CPU
    vec4 vScale = U_VSCustom0[idxInst + 1];
    vec4 vOrientation = U_VSCustom0[idxInst + 2];

    // 重建世界空间坐标
    vec3 vWorldPosition = vPosition.xyz + U_VS_CameraPosition.xyz;

    // 构建世界空间变换矩阵（用于光照）
    mat4 _matWorldSpace = MakeTransform(vWorldPosition, vScale.xyz, vOrientation);
    matWorld[0] = _matWorldSpace[0];
    matWorld[1] = _matWorldSpace[1];
    matWorld[2] = _matWorldSpace[2];

    // 构建相机相对变换矩阵（用于渲染）
    mat4 _matWorld = MakeTransform(vPosition.xyz, vScale.xyz, vOrientation);

    // 使用世界空间坐标计算逆转置矩阵
    matInvTransposeWorld = MakeInverseTransform(vWorldPosition, vScale.xyz, vOrientation);
    matInvTransposeWorld = transpose(matInvTransposeWorld);

    // 使用相机相对矩阵计算裁剪空间坐标
    matWVP = _matWorld * U_ZeroViewProjectMatrix;
#endif

// 使用世界空间矩阵计算世界坐标（片段着色器需要）
o_WPos.x = dot(vObjPos, matWorld[0]);
o_WPos.y = dot(vObjPos, matWorld[1]);
o_WPos.z = dot(vObjPos, matWorld[2]);
o_WPos.w = 1.f;

// 使用世界空间坐标计算风力相位（避免抖动）
float fBendPhase = matWorld[0][3] + matWorld[1][3] + matWorld[2][3];
_TrunkBending(vObjPos.xyz, U_VSGeneralRegister0.x, U_VS_Time.x, fBendPhase, ...);
```

#### D3D11 (TrunkPBR_VS.txt, Trunk_VS.txt)
```hlsl
#ifdef USEINSTANCE
    float3 vPosition = U_VSCustom0[idxInst + 0].xyz;  // camera-relative from CPU
    float3 vScale = U_VSCustom0[idxInst + 1].xyz;
    float4 vOrientation = U_VSCustom0[idxInst + 2];

    // 重建世界空间坐标
    float3 vWorldPos = vPosition.xyz + U_VS_CameraPosition.xyz;

    // 构建世界空间变换矩阵（用于光照）
    float4x4 _matWorldSpace = MakeTransform(vWorldPos, vScale.xyz, vOrientation);
    matWorld[0] = _matWorldSpace[0];
    matWorld[1] = _matWorldSpace[1];
    matWorld[2] = _matWorldSpace[2];

    // 构建相机相对变换矩阵（用于渲染）
    float4x4 matWorldCR = MakeTransform(vPosition.xyz, vScale.xyz, vOrientation);

    // 使用世界空间坐标计算逆转置矩阵
    matInvTransposeWorld = MakeInverseTransform(vWorldPos, vScale.xyz, vOrientation);
    matInvTransposeWorld = transpose(matInvTransposeWorld);
#endif

// D3D11已经正确处理世界坐标输出（在TrunkPBR中显式添加相机位置）
vsOut.o_WorldPos.xyz = mul(matWorld, vObjPos);
vsOut.o_WorldPos.xyz += U_VS_CameraPosition.xyz;  // 转换回世界空间
```

## 修复的关键点

### 1. 矩阵职责分离
- **matWorld**: 始终保持世界空间，用于：
  - 计算世界坐标 `o_WPos`（片段着色器光照）
  - 法线变换 `matInvTransposeWorld`
  - 风力相位 `fBendPhase`（避免抖动）
  
- **_matWorld / matWorldCR**: 相机相对空间，用于：
  - 顶点变换到裁剪空间
  - 避免大世界坐标精度损失

### 2. 坐标空间转换链
```
CPU: world_position → position = world_position - camera_position (camera-relative)
GPU: vPosition (camera-relative from CPU)
     ↓
     vWorldPosition = vPosition + U_VS_CameraPosition (reconstruct world)
     ↓
     matWorld = MakeTransform(vWorldPosition, ...) (world-space matrix)
     _matWorld = MakeTransform(vPosition, ...) (camera-relative matrix)
     ↓
     o_WPos = vObjPos * matWorld (world-space output)
     o_PosInClip = vObjPos * _matWorld * VP (camera-relative rendering)
```

### 3. 为什么需要世界空间
片段着色器进行光照计算时需要：
- 世界空间位置（计算光照方向）
- 世界空间法线（计算光照强度）
- 相机到片元向量（计算视角相关效果）

如果传递camera-relative坐标，光照计算会以相机位置为原点，导致光照错误。

## 测试验证

### 预期结果
- ✅ 合批前模型显示正常
- ✅ 合批后模型显示正常
- ✅ 两种模式颜色一致
- ✅ 风力摆动相位稳定（无抖动）

### 验证步骤
1. 在手机端运行修改后的shader
2. 观察合批前后的Trunk模型
3. 确认颜色、光照、风力效果一致

## 相关文件

### 已修复的Shader
- `g:\Echo_SU_Develop\Shader\gles2\TrunkPBR_VS.txt`
- `g:\Echo_SU_Develop\Shader\gles2\Trunk_VS.txt`
- `g:\Echo_SU_Develop\Shader\d3d11\TrunkPBR_VS.txt`
- `g:\Echo_SU_Develop\Shader\d3d11\Trunk_VS.txt`

### CPU端代码
- `g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoInstanceBatchEntity.cpp`
  - `updateWorldViewPositionScaleOrientation()` - 发送camera-relative数据

## 经验总结

### Camera-Relative渲染规则
1. **CPU → GPU传递**: camera-relative坐标（精度优化）
2. **Shader内部**: 
   - 重建世界空间矩阵用于光照计算
   - 保持camera-relative矩阵用于顶点变换
3. **输出给片段着色器**: 世界空间坐标

### 常见错误
❌ 混淆camera-relative和world-space矩阵用途
❌ 假设CPU传递的是世界空间坐标
❌ 在需要世界空间的地方使用camera-relative坐标

### 调试技巧
1. 对比合批前后的数据流
2. 检查片段着色器期望的坐标空间
3. 验证矩阵构建使用的坐标系
4. 使用调试颜色可视化中间结果

## 日期
2025-01-12
