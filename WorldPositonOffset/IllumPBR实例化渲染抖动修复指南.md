# IllumPBR 实例化渲染抖动修复指南

## 问题背景

### 症状描述
在大世界场景中，使用 IllumPBR 材质的实例化模型（如建筑、树木等）会出现明显的抖动现象。这种抖动在相机移动时尤为明显，严重影响视觉质量。

### 问题根源
**双重相机位置偏移导致的浮点精度丢失**

在实例化渲染管线中，系统对相机位置进行了两次处理：
1. **CPU 端**：`EchoInstanceBatchEntity.cpp` 中 `updateWorldViewTransform` 函数已经将世界矩阵转换为 camera-relative（相对摄像机）坐标
2. **GPU 端**：`IllumPBR_VS.txt` shader 中又通过 `U_VS_CameraPosition` 进行了一次相机位置补偿

这导致在大坐标场景下：
```
最终位置 = 世界矩阵（已减摄像机） - 摄像机位置 + 摄像机位置 = 世界矩阵
```

虽然数学上正确，但在浮点数运算中，**大数值的加减会累积精度误差**，造成顶点位置的微小抖动。

---

## 修复方案概览

### 核心思路
**统一 CPU 和 GPU 端的坐标系统，避免重复的相机位置计算**

1. **CPU 端**：继续使用 `updateWorldViewTransform` 上传 camera-relative 的世界矩阵
2. **GPU 端**：简化世界坐标重建逻辑，直接从相机空间坐标恢复世界坐标

### 修改文件
1. **Shader 层**：`g:\Echo_SU_Develop\Shader\d3d11\IllumPBR_VS.txt`
2. **CPU 层**：`g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoInstanceBatchEntity.cpp`
3. **Drift 批处理**：`g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoInstanceBatchDrift.cpp`

---

## 详细修改说明

### 一、Shader 层修改（IllumPBR_VS.txt）

#### 1.1 HWSKINNED 分支（带骨骼动画的实例）

**修改前代码：**
```hlsl
#ifdef HWSKINNED
    WorldMatrix[0] = U_VSCustom0[idxInst + 0];
    WorldMatrix[1] = U_VSCustom0[idxInst + 1];
    WorldMatrix[2] = U_VSCustom0[idxInst + 2];
    
    // 使用 camera-relative 矩阵计算相机空间坐标
    float4 vObjPosInCam;
    vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
    vObjPosInCam.w = 1.f;
    vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

    // ❌ 错误：在矩阵平移列上加回相机位置
    WorldMatrix[0][3] += U_VS_CameraPosition[0];
    WorldMatrix[1][3] += U_VS_CameraPosition[1];
    WorldMatrix[2][3] += U_VS_CameraPosition[2];

    // ❌ 再次变换，导致大数值浮点运算
    float4 Wpos;
    Wpos.xyz = mul((float3x4)WorldMatrix, pos);
    Wpos.w = 1.0;
#endif
```

**修改后代码：**
```hlsl
#ifdef HWSKINNED
    WorldMatrix[0] = U_VSCustom0[idxInst + 0];
    WorldMatrix[1] = U_VSCustom0[idxInst + 1];
    WorldMatrix[2] = U_VSCustom0[idxInst + 2];
    InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
    InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
    InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
    
    // ✅ 步骤1：使用 camera-relative 矩阵直接计算相机空间坐标
    float4 vObjPosInCam;
    vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
    vObjPosInCam.w = 1.f;
    
    // ✅ 步骤2：投影到裁剪空间（U_ZeroViewProjectMatrix 是不含相机平移的 VP 矩阵）
    vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

    // ✅ 步骤3：直接从相机空间坐标恢复世界坐标
    //   公式：世界坐标 = 相机空间坐标 + 相机世界坐标
    float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
#endif
```

**逐行解释：**

1. **加载实例矩阵**
   ```hlsl
   WorldMatrix[0] = U_VSCustom0[idxInst + 0];
   WorldMatrix[1] = U_VSCustom0[idxInst + 1];
   WorldMatrix[2] = U_VSCustom0[idxInst + 2];
   ```
   - `U_VSCustom0` 是 CPU 端上传的实例数据数组
   - 每个实例占用 6 个 `float4`（3 个世界矩阵行 + 3 个逆转置矩阵行）
   - `idxInst = i_InstanceID * UniformCount` 计算当前实例的数据起始位置
   - 这里读取的矩阵**已经是 camera-relative 的**（CPU 端已减去摄像机位置）

2. **相机空间变换**
   ```hlsl
   float4 vObjPosInCam;
   vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
   vObjPosInCam.w = 1.f;
   ```
   - `pos` 是模型局部空间的顶点位置
   - `mul((float3x4)WorldMatrix, pos)` 将顶点从模型空间变换到相机空间
   - **关键点**：因为 `WorldMatrix` 是 camera-relative 的，变换结果就是相对于相机的坐标（小数值）
   - 设置 `w = 1.0` 用于后续齐次坐标计算

3. **投影变换**
   ```hlsl
   vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);
   ```
   - `U_ZeroViewProjectMatrix` 是**不包含相机平移的 View-Projection 矩阵**
   - 它只包含相机的旋转和投影，没有平移分量
   - 输入的 `vObjPosInCam` 已经是相机空间坐标，因此直接投影即可得到裁剪空间坐标
   - 这一步避免了大坐标的浮点运算

4. **世界坐标重建（关键改动）**
   ```hlsl
   float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
   ```
   - **修改前**：在矩阵列上加相机位置，再做矩阵乘法 → 大数值运算
   - **修改后**：直接向量加法 `相机空间坐标 + 相机世界坐标 = 世界坐标`
   - **优势**：
     - 避免了矩阵乘法中的累积误差
     - 简单的三维向量加法，编译器优化友好
     - 数值上等价但精度损失最小

#### 1.2 非骨骼实例分支

**修改后代码：**
```hlsl
#else  // 非 HWSKINNED 分支
    WorldMatrix[0] = U_VSCustom0[idxInst + 0];
    WorldMatrix[1] = U_VSCustom0[idxInst + 1];
    WorldMatrix[2] = U_VSCustom0[idxInst + 2];
    InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
    InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
    InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
    
    // ✅ instance data is camera-relative; use it directly for clip-space transform
    float4 vObjPosInCam;
    vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
    vObjPosInCam.w = 1.0f;
    vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

    // ✅ 直接从相机空间恢复世界坐标
    float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
#endif
```

**说明：**
- 与骨骼动画分支逻辑完全一致
- 唯一区别是不需要先进行骨骼蒙皮变换
- 同样避免了在矩阵列上操作导致的精度问题

---

### 二、CPU 层修改（EchoInstanceBatchEntity.cpp）

#### 2.1 修改实例数据更新函数注册

**文件位置：** `g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoInstanceBatchEntity.cpp`

**修改内容（第 1397-1401 行）：**

```cpp
// 修改前：
shader_type_register(IBEPri::Util, Illum,           6, 58, updateWorldTransform);
shader_type_register(IBEPri::Util, IllumPBR,        6, 58, updateWorldTransform);      // ❌ 错误
shader_type_register(IBEPri::Util, IllumPBR2022,    6, 58, updateWorldTransform);      // ❌ 错误
shader_type_register(IBEPri::Util, IllumPBR2023,    6, 58, updateWorldTransform);      // ❌ 错误
shader_type_register(IBEPri::Util, SpecialIllumPBR, 6, 58, updateWorldTransform);      // ❌ 错误

// 修改后：
shader_type_register(IBEPri::Util, Illum,           6, 58, updateWorldTransform);
shader_type_register(IBEPri::Util, IllumPBR,        6, 58, updateWorldViewTransform);  // ✅ 改为 camera-relative
shader_type_register(IBEPri::Util, IllumPBR2022,    6, 58, updateWorldViewTransform);  // ✅ 改为 camera-relative
shader_type_register(IBEPri::Util, IllumPBR2023,    6, 58, updateWorldViewTransform);  // ✅ 改为 camera-relative
shader_type_register(IBEPri::Util, SpecialIllumPBR, 6, 58, updateWorldViewTransform);  // ✅ 改为 camera-relative
```

**逐行解释：**

1. **shader_type_register 宏的作用**
   ```cpp
   #define shader_type_register(util, type, inst_uni_count, rev_uni_count, _func) \
       _l_register_shader(util, eIT_##type, #type, (inst_uni_count), (rev_uni_count), _func);
   ```
   - 注册 shader 类型与实例数据更新函数的映射关系
   - `type`：shader 名称（如 `IllumPBR`）
   - `inst_uni_count`：每个实例占用的 uniform 数量（6 = 3 行世界矩阵 + 3 行逆转置矩阵）
   - `rev_uni_count`：保留的全局 uniform 数量（58）
   - `_func`：数据更新函数指针

2. **updateWorldTransform vs updateWorldViewTransform**
   
   **updateWorldTransform（绝对坐标）：**
   ```cpp
   void updateWorldTransform(const SubEntityVec & vecInst, uint32 iUniformCount,
           Uniformf4Vec & vecData)
   {
       for (size_t i=0u; i<iInstNum; ++i) {
           const DBMatrix4 * _world_matrix = nullptr;
           pSubEntity->getWorldTransforms(&_world_matrix);
           
           // ❌ 直接上传世界矩阵（大坐标）
           for (int i=0; i<3; ++i) {
               _inst_buff[i].x = (float)_world_matrix->m[i][0];
               _inst_buff[i].y = (float)_world_matrix->m[i][1];
               _inst_buff[i].z = (float)_world_matrix->m[i][2];
               _inst_buff[i].w = (float)_world_matrix->m[i][3];  // 包含大坐标的平移
           }
       }
   }
   ```

   **updateWorldViewTransform（相机相对坐标）：**
   ```cpp
   void updateWorldViewTransform(const SubEntityVec& vecInst, uint32 iUniformCount,
       Uniformf4Vec& vecData)
   {
       DVector3 camPos = DVector3::ZERO;
       if (iInstNum != 0) {
           Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
           camPos = pCam->getDerivedPosition();  // ✅ 获取相机位置
       }

       for (size_t i = 0u; i < iInstNum; ++i) {
           const DBMatrix4* _world_matrix = nullptr;
           pSubEntity->getWorldTransforms(&_world_matrix);
           
           // ✅ 创建 camera-relative 矩阵
           DBMatrix4 world_matrix_not_cam = *_world_matrix;
           world_matrix_not_cam[0][3] -= camPos[0];  // 平移列减去相机 X
           world_matrix_not_cam[1][3] -= camPos[1];  // 平移列减去相机 Y
           world_matrix_not_cam[2][3] -= camPos[2];  // 平移列减去相机 Z
           
           // ✅ 上传相对坐标（小数值）
           uniform_f4* _inst_buff = &vecData[i * iUniformCount];
           for (int i = 0; i < 3; ++i) {
               _inst_buff[i].x = (float)world_matrix_not_cam.m[i][0];
               _inst_buff[i].y = (float)world_matrix_not_cam.m[i][1];
               _inst_buff[i].z = (float)world_matrix_not_cam.m[i][2];
               _inst_buff[i].w = (float)world_matrix_not_cam.m[i][3];  // 小数值平移
           }
           
           // 逆转置矩阵不需要调整（只用于法线变换）
           DBMatrix4 inv_transpose_world_matrix;
           const DBMatrix4* inv_world_matrix = nullptr;
           pSubEntity->getInvWorldTransforms(&inv_world_matrix);
           inv_transpose_world_matrix = inv_world_matrix->transpose();
           _inst_buff += 3;
           for (int i = 0; i < 3; ++i) {
               _inst_buff[i].x = (float)inv_transpose_world_matrix.m[i][0];
               _inst_buff[i].y = (float)inv_transpose_world_matrix.m[i][1];
               _inst_buff[i].z = (float)inv_transpose_world_matrix.m[i][2];
               _inst_buff[i].w = (float)inv_transpose_world_matrix.m[i][3];
           }
       }
   }
   ```

3. **为什么只改 IllumPBR 系列**
   - `Illum` shader 可能使用不同的世界坐标计算方式（保留原样以避免影响）
   - `IllumPBR`、`IllumPBR2022`、`IllumPBR2023`、`SpecialIllumPBR` 都使用相同的实例路径逻辑
   - 这些 shader 在顶点着色器中都使用 `U_VS_CameraPosition` 进行世界坐标重建
   - 其他 shader（如 `BillboardLeaf`、`Trunk`）使用 Position-Scale-Orientation 传递方式，不受影响

#### 2.2 Drift 批处理同步修改（已回退）

**文件位置：** `g:\Echo_SU_Develop\EchoEngine\Code\Core\Source\EchoInstanceBatchDrift.cpp`

**注意：** 该文件的修改已回退，因为 Drift 批处理系统使用不同的数据结构（`DriftData`），没有对应的 `updateWorldViewTransform` 函数。

**当前状态（保持不变）：**
```cpp
shader_type_register(IBAPri::Util, Illum, 6, 58, updateWorldTransform);
shader_type_register(IBAPri::Util, IllumPBR, 6, 58, updateWorldTransform);          // 保持原样
shader_type_register(IBAPri::Util, IllumPBR2022, 6, 58, updateWorldTransform);      // 保持原样
shader_type_register(IBAPri::Util, IllumPBR2023, 6, 58, updateWorldTransform);      // 保持原样
shader_type_register(IBAPri::Util, SpecialIllumPBR, 6, 58, updateWorldTransform);   // 保持原样
```

**说明：**
- Drift 系统主要用于动态物体（如飘浮物）
- 这些物体通常离相机较近，浮点精度问题不明显
- 如果后续需要支持远距离 Drift 物体，需要单独实现 `updateWorldViewTransform` 的 Drift 版本

---

## 原理深度解析

### 1. Camera-Relative 渲染技术

#### 1.1 什么是 Camera-Relative 坐标系

**传统渲染（World Space）：**
```
顶点世界坐标 = 100000.5 (可能是几公里外的物体)
相机世界坐标 = 100000.0
相对距离 = 0.5 米
```

**Camera-Relative 渲染：**
```
顶点相对坐标 = 0.5 (相对于相机)
相机世界坐标 = 100000.0 (只在需要时使用)
```

#### 1.2 浮点精度问题示例

**float 类型精度：**
- 23 位尾数 ≈ 7 位十进制精度
- 当数值达到 100000 时，最小可表示增量约为 0.0078 米（7.8 毫米）
- 当数值达到 1000000 时，最小增量约为 0.0625 米（6.25 厘米）

**抖动产生机制：**
```cpp
// 场景：物体位置 = (100000.5, 0, 0)，相机位置 = (100000.0, 0, 0)

// 错误做法（修改前）：
float worldX = 100000.5f;              // ❌ 存储大数值
float camX = 100000.0f;
float relativeX = worldX - camX;       // ❌ 大数减法，精度损失
// 结果：relativeX 可能是 0.5, 0.500001, 或 0.499999（精度抖动）

// 正确做法（修改后）：
double worldX_d = 100000.5;            // ✅ double 存储精确值
double camX_d = 100000.0;
float relativeX = (float)(worldX_d - camX_d);  // ✅ 先用 double 精确减法，再转 float
// 结果：relativeX = 0.5（精确无抖动）
```

### 2. 世界坐标重建方式对比

#### 2.1 修改前的错误方式

```hlsl
// 步骤1：CPU 上传 camera-relative 矩阵
DBMatrix4 world_matrix_not_cam = *_world_matrix;
world_matrix_not_cam[0][3] -= camPos[0];  // 矩阵平移列 = 绝对坐标 - 相机坐标

// 步骤2：GPU 读取 camera-relative 矩阵
WorldMatrix[0] = U_VSCustom0[idxInst + 0];  // 平移列已是相对坐标

// 步骤3：❌ 错误地在矩阵列上加回相机位置
WorldMatrix[0][3] += U_VS_CameraPosition[0];
WorldMatrix[1][3] += U_VS_CameraPosition[1];
WorldMatrix[2][3] += U_VS_CameraPosition[2];

// 步骤4：❌ 再次矩阵变换（涉及大坐标的浮点运算）
float4 Wpos;
Wpos.xyz = mul((float3x4)WorldMatrix, pos);  // 内部计算：
// Wpos.x = WorldMatrix[0][0]*pos.x + WorldMatrix[1][0]*pos.y + WorldMatrix[2][0]*pos.z + WorldMatrix[0][3]
//        = 旋转缩放 + (相对坐标 + 相机坐标)
//        = 旋转缩放 + 绝对坐标（大数值，精度损失）
```

**精度损失分析：**
1. CPU 端：`double` 坐标减法（精确）
2. 转换为 `float` 上传（第一次精度损失）
3. GPU 端：矩阵列加法 `float + float`（可能引入舍入误差）
4. 矩阵乘法：多次浮点运算累积误差
5. 最终结果：每帧可能产生微小偏移（抖动）

#### 2.2 修改后的正确方式

```hlsl
// 步骤1：CPU 上传 camera-relative 矩阵（同上）

// 步骤2：GPU 直接使用 camera-relative 矩阵变换到相机空间
float4 vObjPosInCam;
vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);  // 结果是小数值（相对坐标）

// 步骤3：✅ 直接向量加法恢复世界坐标
float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
// Wpos.x = vObjPosInCam.x + U_VS_CameraPosition.x
//        = 相对坐标 + 相机坐标
//        = 绝对坐标（但只在最后一步涉及大数值）
```

**精度优化分析：**
1. CPU 端：`double` 坐标减法（精确）
2. 转换为 `float` 上传（第一次精度损失，但数值小，相对误差低）
3. GPU 端矩阵乘法：**只涉及小数值**（相对坐标），精度高
4. 最终向量加法：一次性加回相机坐标，误差不累积
5. 结果：抖动消失

### 3. U_ZeroViewProjectMatrix 的作用

#### 3.1 传统 View-Projection 矩阵

```
ViewMatrix = [R | -R*T]  // R=旋转, T=相机平移
ProjectionMatrix = [Proj]

VP = Projection * View = [Proj * R | -Proj * R * T]
```

#### 3.2 Zero View-Projection 矩阵

```
ZeroViewMatrix = [R | 0]  // 平移项为 0
ProjectionMatrix = [Proj]

ZeroVP = Projection * ZeroView = [Proj * R | 0]
```

**使用场景：**
- 输入顶点已经是**相机空间坐标**（相对于相机原点）
- 只需要应用相机旋转和投影，不需要再次平移
- 避免在投影阶段引入大坐标的浮点运算

**代码对应：**
```hlsl
// vObjPosInCam 已经是相机空间坐标（小数值）
float4 vObjPosInCam;
vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);  // camera-relative 变换

// 使用 ZeroVP 直接投影，不再平移
vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);
```

---

## 修改效果对比

### 修改前
```
物体世界坐标：(100000.5, 50.0, 200000.3)
相机世界坐标：(100000.0, 50.0, 200000.0)

CPU 端上传：
  矩阵平移列 = (0.5, 0.0, 0.3) ✅

GPU 端处理：
  1. 读取矩阵：(0.5, 0.0, 0.3)
  2. 加回相机：(100000.5, 50.0, 200000.3) ❌ 大数值
  3. 矩阵变换：mul(WorldMatrix, pos) ❌ 涉及大数运算
  4. 每帧可能产生 ±0.01 米的误差 → 抖动

精度路径：double → float(小) → float(大) → 累积误差
```

### 修改后
```
物体世界坐标：(100000.5, 50.0, 200000.3)
相机世界坐标：(100000.0, 50.0, 200000.0)

CPU 端上传：
  矩阵平移列 = (0.5, 0.0, 0.3) ✅

GPU 端处理：
  1. 读取矩阵：(0.5, 0.0, 0.3)
  2. 矩阵变换：mul(WorldMatrix, pos) = (0.5, 0.0, 0.3) ✅ 小数值
  3. 向量加法：(0.5, 0.0, 0.3) + (100000.0, 50.0, 200000.0) ✅ 一次性
  4. 结果稳定，无抖动

精度路径：double → float(小) → 保持小数值运算 → 最后加法
```

---

## 测试验证

### 验证步骤

1. **编译 Shader**
   - 确保 `IllumPBR_VS.txt` 被重新编译
   - 检查 shader cache 是否更新

2. **编译引擎**
   - 重新编译 `EchoInstanceBatchEntity.cpp`
   - 确认链接正确的函数指针

3. **场景测试**
   - 在大世界场景（坐标 > 100,000）放置 IllumPBR 模型
   - 移动相机观察模型是否抖动
   - 特别注意边缘和轮廓线

4. **性能测试**
   - 对比修改前后的帧率（应无明显变化）
   - 检查 GPU 占用（向量加法比矩阵乘法更快）

### 预期结果
- ✅ 模型完全不抖动
- ✅ 光照和阴影稳定
- ✅ 性能无明显下降（甚至略有提升）
- ✅ 所有 IllumPBR 变体正常工作

---

## 常见问题排查

### 问题1：模型消失或位置错误

**可能原因：**
- CPU 端未使用 `updateWorldViewTransform`
- Shader 中仍在矩阵列上操作

**排查步骤：**
1. 确认 `shader_type_register` 使用 `updateWorldViewTransform`
2. 检查 shader 中是否移除了 `WorldMatrix[i][3] +=` 操作
3. 验证 `U_VS_CameraPosition` 在 shader 中正确使用

### 问题2：特定角度或距离仍有抖动

**可能原因：**
- 其他渲染路径（如阴影投射）未修改
- Drift 批处理使用绝对坐标

**解决方案：**
- 检查 `SHADOWPASS` 分支是否也使用 camera-relative
- 如果 Drift 物体抖动，实现对应的 `updateWorldViewTransform`

### 问题3：法线或光照异常

**可能原因：**
- 逆转置矩阵计算错误
- 世界坐标重建影响光照计算

**排查步骤：**
1. 确认 `InvTransposeWorldMatrix` 正确上传（无需减相机位置）
2. 检查 `o_WPos` 是否正确传递给像素着色器
3. 验证 `U_PS_CameraPosition` 在片元着色器中的使用

---

## 扩展阅读

### 相关技术文档
1. **平面植被 Camera-Relative 修复**：`平面植被抖动修复指南.md`
2. **大世界浮点精度方案**：`大世界浮点精度_CameraRelative技术方案.md`

### 推荐优化方向
1. **静态批处理**：为 `InstanceStaticBatchEntity` 实现类似修复
2. **GPU Skinning**：骨骼动画也可以应用 camera-relative
3. **LOD 系统**：远距离物体可以使用更激进的精度优化
4. **阴影系统**：确保阴影投射也使用相对坐标

### 性能考虑
- **CPU 开销**：`updateWorldViewTransform` 需要每帧减相机位置（可接受）
- **GPU 开销**：向量加法比矩阵乘法更快（性能提升）
- **内存占用**：无变化（仍是 6 个 float4 per instance）

---

## 总结

### 核心改动
1. **Shader 层**：简化世界坐标重建，直接使用向量加法
2. **CPU 层**：为 IllumPBR 系列注册 `updateWorldViewTransform` 函数
3. **数据流**：统一使用 camera-relative 坐标系

### 技术亮点
- **精度优化**：将大数值运算延迟到最后一步
- **代码简化**：移除冗余的矩阵列操作
- **通用性**：适用于所有 IllumPBR 变体

### 适用范围
- ✅ 大世界场景（坐标 > 100,000）
- ✅ 实例化渲染的 IllumPBR 模型
- ✅ 动态相机移动场景
- ⚠️ Drift 批处理需单独实现（当前未修改）

---

## 附录：完整代码片段

### A. Shader 修改（IllumPBR_VS.txt，第 200-240 行）

```hlsl
#ifdef USEINSTANCE
    // ... 前置代码 ...
    
    int idxInst = i_InstanceID * UniformCount;
    #ifdef HWSKINNED
        // 加载 camera-relative 矩阵
        WorldMatrix[0] = U_VSCustom0[idxInst + 0];
        WorldMatrix[1] = U_VSCustom0[idxInst + 1];
        WorldMatrix[2] = U_VSCustom0[idxInst + 2];
        InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
        InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
        InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
        
        // 变换到相机空间（小数值）
        float4 vObjPosInCam;
        vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
        vObjPosInCam.w = 1.f;
        
        // 投影到裁剪空间
        vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

        // 直接向量加法恢复世界坐标
        float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
    #else
        // 非骨骼动画分支（同样逻辑）
        WorldMatrix[0] = U_VSCustom0[idxInst + 0];
        WorldMatrix[1] = U_VSCustom0[idxInst + 1];
        WorldMatrix[2] = U_VSCustom0[idxInst + 2];
        InvTransposeWorldMatrix[0] = U_VSCustom0[idxInst + 3];
        InvTransposeWorldMatrix[1] = U_VSCustom0[idxInst + 4];
        InvTransposeWorldMatrix[2] = U_VSCustom0[idxInst + 5];
        
        float4 vObjPosInCam;
        vObjPosInCam.xyz = mul((float3x4)WorldMatrix, pos);
        vObjPosInCam.w = 1.0f;
        vsOut.o_PosInClip = mul((float4x4)U_ZeroViewProjectMatrix, vObjPosInCam);

        float4 Wpos = float4(vObjPosInCam.xyz + U_VS_CameraPosition.xyz, 1.0f);
    #endif
    
    // 计算手性（用于法线修正）
    fHandedness = determinant(float3x3(WorldMatrix[0].xyz, WorldMatrix[1].xyz, WorldMatrix[2].xyz));
    fHandedness = fHandedness >= 0.f ? 1.f : -1.f;
#endif
```

### B. CPU 修改（EchoInstanceBatchEntity.cpp，第 1397-1409 行）

```cpp
void InstanceBatchEntity::Init()
{
    // ... 前置代码 ...
    
    // 注册 shader 类型与数据更新函数
    shader_type_register(IBEPri::Util, Illum,           6, 58, updateWorldTransform);
    shader_type_register(IBEPri::Util, IllumPBR,        6, 58, updateWorldViewTransform);  // 修改
    shader_type_register(IBEPri::Util, IllumPBR2022,    6, 58, updateWorldViewTransform);  // 修改
    shader_type_register(IBEPri::Util, IllumPBR2023,    6, 58, updateWorldViewTransform);  // 修改
    shader_type_register(IBEPri::Util, SpecialIllumPBR, 6, 58, updateWorldViewTransform);  // 修改
    shader_type_register(IBEPri::Util, BillboardLeaf,   3, 58, updateWorldPositionScaleOrientation);
    shader_type_register(IBEPri::Util, BillboardLeafPBR,3, 58, updateWorldPositionScaleOrientation);
    shader_type_register(IBEPri::Util, Trunk,           3, 58, updateWorldViewPositionScaleOrientation);
    shader_type_register(IBEPri::Util, TrunkPBR,        3, 58, updateWorldViewPositionScaleOrientation);
    shader_type_register(IBEPri::Util, SimpleTreePBR,   6, 58, updateWorldTransform);
    shader_type_register(IBEPri::Util, BaseWhiteNoLight,3, 58, updateWorldPositionScaleOrientation);
    
    // ... 后续代码 ...
}
```

---

**文档版本：** v1.0  
**最后更新：** 2025-10-30  
**作者：** 技术团队  
**审核：** 已通过场景测试验证  
