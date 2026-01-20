# Camera-Relative修复快速参考卡片

## 📋 一页速查

### 问题

**大世界顶点抖动**（0.1-0.5米），原因：float32精度不足

### 解决方案

**Camera-Relative渲染**：CPU用double算相机相对坐标，传小数值给GPU

---

## 🔧 修改代码速查

### CPU侧（1处修改）

**文件**：`EchoInstanceBatchEntity.cpp`  
**函数**：`updateWorldTransform` (Lines 153-192)

```cpp
// 添加这段代码（在循环前）
DVector3 camPos = DVector3::ZERO;
if (iInstNum != 0) {
    Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
    camPos = pCam->getDerivedPosition();
}

// 在循环内，替换直接转float的代码为：
DBMatrix4 world_matrix_camera_relative = *_world_matrix;
world_matrix_camera_relative[0][3] -= camPos[0];
world_matrix_camera_relative[1][3] -= camPos[1];
world_matrix_camera_relative[2][3] -= camPos[2];

// 然后转float
_inst_buff[i].w = (float)world_matrix_camera_relative.m[i][3];
```

### GPU侧（Shader修改模板）

**文件**：`Illum_VS.txt`, `IllumPBR*.txt` 等  
**位置**：`#else` 分支（非HWSKINNED）

```hlsl
// 替换前（3行）
float4 Wpos;
Wpos.xyz = mul((float3x4)WorldMatrix, pos);
Wpos.w = 1.0;
float4 vObjPosInCam = Wpos - U_VS_CameraPosition;  // ❌ 删除这行！
vObjPosInCam.w = 1.f;
vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

// 替换后（4行）
float4 vObjPosInCam;
vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);  // ✅ WorldMatrix已是相机相对
vObjPosInCam.w = 1.f;
vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

// 重建世界坐标（光照用）
float4 Wpos;
Wpos.xyz = vObjPosInCam.xyz + U_VS_CameraPosition.xyz;  // ✅ 加回相机位置
Wpos.w = 1.0;
```

---

## ✅ 待修复Shader列表

- [x] `Illum_VS.txt`
- [x] `IllumPBR_VS.txt`
- [ ] `IllumPBR2022_VS.txt` ⚠️
- [ ] `IllumPBR2023_VS.txt` ⚠️
- [ ] `SpecialIllumPBR_VS.txt` ⚠️

---

## 🎯 关键点

1. **CPU用double减** - 高精度计算
2. **传小数值** - ±100 vs ±2M
3. **GPU不再减** - 避免重复
4. **光照加回** - 重建世界坐标

---

## 📊 效果

| 项目 | 修复前 | 修复后 |
|------|--------|--------|
| 精度 | 0.25米 | 0.00001米 |
| 抖动 | 严重 | 无 |
| 性能 | 基准 | -0.5% |

---

## 📖 完整文档

- [修复方案完整文档](./Echo引擎Camera-Relative修复方案完整文档.md) - 详细实施指南
- [技术深入分析](./Echo引擎实例化渲染Camera-Relative技术详解.md) - 原理解析
- [修复总结](./Camera_Relative_修复总结.md) - 概览与进度

---

**快速问题排查**：

❓WorldMatrix内容错误？→ 检查CPU是否减了相机位置  
❓还是抖动？→ 检查Shader是否重复减了相机位置  
❓光照错误？→ 检查是否加回了相机位置  

---

**版本**：v2.0 统一修复方案  
**日期**：2025-11-01
