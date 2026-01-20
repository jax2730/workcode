# Vulkan植被渲染黑色问题 - 完整诊断方案

## 问题现状
- D3D11和GLES：植被渲染正常 ✅
- Vulkan：植被渲染全黑 ❌
- Shader逻辑已确认与D3D11一致

## 诊断步骤

### 第一步：验证Uniform Buffer数据传输

#### 1.1 检查公共Uniform Buffer (binding = 0)

**关键变量检查：**
```cpp
// 在C++渲染代码中添加调试输出
// 检查这些值是否正确设置：
U_PBR_MainLightColor    // 主光颜色，如果为(0,0,0,0)会导致全黑！
U_VS_CameraPosition     // 相机位置
U_WorldViewProjectMatrix // MVP矩阵
U_VS_MainLightDirection // 光照方向
```

**验证方法：**
```cpp
// 在绑定Uniform Buffer之前，打印关键值
printf("U_PBR_MainLightColor = (%f, %f, %f, %f)\n", 
    uniformData.U_PBR_MainLightColor.x,
    uniformData.U_PBR_MainLightColor.y,
    uniformData.U_PBR_MainLightDirection.z,
    uniformData.U_PBR_MainLightColor.w);
```

#### 1.2 检查私有Uniform Buffer (binding = 2)

**关键变量：**
```cpp
U_WorldMatrix[3]           // 世界矩阵
U_WorldViewProjectMatrix[4] // WVP矩阵
U_VSCustom2                // 植被缩放参数
U_VSCustom11[3]            // Hit space矩阵
U_VSCustom12               // 网格边界
U_VSGeneralRegister4/5     // 淡出参数
```

**特别注意：** 如果 `U_WorldMatrix` 全为0，位置计算会错误！

#### 1.3 检查材质Uniform Buffer (binding = 3)

```cpp
U_MaterialDiffuse   // 材质漫反射颜色，如果为(0,0,0)会黑！
U_MaterialSpecular  // 金属度/高光参数
U_PSGeneralRegister3 // AO和粗糙度
```

### 第二步：验证顶点属性绑定

**Vulkan的location绑定必须精确对应！**

#### 2.1 检查VkVertexInputAttributeDescription数组

Shader期望的布局：
```glsl
// Stream 0 - 网格数据
layout(location = 0) in vec3 POSITION3;  // 顶点位置
layout(location = 1) in vec3 NORMAL3;    // 法线
layout(location = 2) in vec3 TANGENT3;   // 切线
layout(location = 3) in vec2 TEXCOORD3;  // UV坐标
layout(location = 4) in vec4 COLOR3;     // 顶点颜色

// Stream 1 - 实例数据
layout(location = 5) in vec4 POSITION2;  // 实例位置+缩放
layout(location = 6) in vec4 NORMAL2;    // 实例四元数（归一化到0-1）
```

C++代码中应该这样设置：
```cpp
VkVertexInputAttributeDescription attributes[] = {
    // Stream 0
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT,   offsetof(Vertex, position)},  // location 0
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT,   offsetof(Vertex, normal)},    // location 1
    {2, 0, VK_FORMAT_R32G32B32_SFLOAT,   offsetof(Vertex, tangent)},   // location 2
    {3, 0, VK_FORMAT_R32G32_SFLOAT,      offsetof(Vertex, texcoord)},  // location 3
    {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},     // location 4
    
    // Stream 1 - Per Instance
    {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, position_scale)}, // location 5
    {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, quaternion)},     // location 6
};
```

**重点检查：**
1. `location` 索引必须匹配Shader的 `layout(location = N)`
2. `binding` 索引（第二个参数）：Stream 0用0，Stream 1用1
3. Format必须匹配：vec3→R32G32B32, vec4→R32G32B32A32, vec2→R32G32
4. offset必须正确对应结构体成员偏移

#### 2.2 验证数据正确性

**检查实例数据NORMAL2（四元数）：**
```cpp
// 四元数必须归一化到 [0, 1] 范围
// Shader中会用 vQua = NORMAL2 * 2.f - 1.f 转回 [-1, 1]

// 错误的数据示例：
// NORMAL2 = (0, 0, 0, 0) → vQua = (-1,-1,-1,-1) → 错误的旋转
// NORMAL2 = (1, 0, 0, 0) → vQua = (1,-1,-1,-1) → 非归一化四元数

// 正确的数据示例：
// NORMAL2 = (0.5, 0.5, 0.5, 0.5) → vQua = (0,0,0,0) → 单位四元数（无旋转）
```

**检查顶点颜色COLOR3：**
```cpp
// COLOR3.xyz 会直接乘到最终颜色上
// 如果COLOR3 = (0,0,0,1)，最终颜色会变黑！

// 正常情况应该是：
// COLOR3 = (1, 1, 1, 1) 或其他非零颜色
```

### 第三步：纹理绑定检查

#### 3.1 漫反射纹理 (binding = 5)

```cpp
// 检查纹理是否正确绑定到 binding point 5
// 检查采样器状态
// 检查纹理数据是否有效（非全黑）
```

#### 3.2 环境贴图 (binding = 7)

```cpp
// 检查cube map是否绑定
// PBR光照需要环境贴图提供间接光照
```

### 第四步：Shader编译验证

#### 4.1 检查Shader编译日志

```cpp
// Vulkan编译SPIR-V时可能有警告但不报错
// 检查是否有：
// - 未使用的变量
// - 类型不匹配
// - Location冲突
```

#### 4.2 使用RenderDoc进行调试

1. 在RenderDoc中捕获一帧
2. 查看Vegetation draw call
3. 检查：
   - Pipeline State → Vertex Input：确认attribute绑定
   - Pipeline State → Vertex Shader：查看uniform buffer内容
   - Pipeline State → Fragment Shader：查看uniform buffer和纹理
   - Mesh Viewer：查看顶点数据（特别是COLOR3和NORMAL2）

### 第五步：对比D3D11渲染路径

**关键差异检查：**

1. **矩阵布局**
   - D3D11：row-major (行主序)
   - Vulkan：可以选择row-major或column-major
   - 确保Vulkan的Uniform Buffer声明了 `layout(std140, row_major)`

2. **NDC坐标系**
   - D3D11：深度 [0, 1]
   - Vulkan：深度 [0, 1]（使用VK_EXT_depth_range_unrestricted）
   - Y轴方向可能需要翻转

3. **Uniform Buffer对齐**
   - Vulkan要求std140布局，每个vec4必须16字节对齐
   - D3D11较宽松

## 最可能的原因排序

根据"只有Vulkan黑色，D3D11/GLES正常"的现象，**最可能的原因依次是：**

### 🔴 1. Uniform Buffer数据未正确传输（可能性：70%）

**症状：** Shader编译无错，但运行时颜色全黑

**原因：**
- `U_PBR_MainLightColor` 为 (0,0,0,0)
- `U_MaterialDiffuse` 为 (0,0,0,0)  
- Uniform Buffer绑定点错误或未更新

**检查方法：**
```cpp
// 在vkCmdDrawIndexed之前添加：
vkCmdPushConstants(commandBuffer, pipelineLayout, ...);
// 或者打印uniform buffer内容
```

### 🟡 2. 顶点数据传输错误（可能性：20%）

**症状：** 植被位置可能正确但颜色黑

**原因：**
- `COLOR3` 数据为 (0,0,0,1)
- `NORMAL2`（四元数）数据错误导致法线错误
- Vertex attribute location绑定错误

**检查方法：**
- RenderDoc查看实际顶点数据
- 对比D3D11的顶点buffer内容

### 🟢 3. 纹理未绑定（可能性：8%）

**症状：** 如果漫反射纹理未绑定，采样会返回(0,0,0)

**检查方法：**
- RenderDoc查看Texture Viewer
- 确认binding = 5的纹理是否有效

### 🔵 4. Pipeline State错误（可能性：2%）

**症状：** 深度测试、混合模式等配置错误

**检查方法：**
- RenderDoc查看Graphics Pipeline State
- 对比D3D11的渲染状态

## 快速诊断命令

### 方法1：强制输出固定颜色（修改Fragment Shader）

在 `VegetationSphTer_PS.txt` 的最后，添加：

```glsl
void main()
{
    // ... 原有代码 ...
    
    // 🔴 调试：强制输出红色
    o_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    return;
    
    // 如果看到红色植被 → Shader执行了，问题在光照计算或纹理
    // 如果还是黑色 → Pipeline或输出有问题
}
```

### 方法2：输出法线作为颜色（Vertex Shader）

修改Vertex Shader输出，将法线可视化：

```glsl
// 在 Vegetation_Low_Instance_VS.txt 中
void main()
{
    // ... 原有计算 ...
    
    // 将法线映射到[0,1]范围作为颜色输出
    o_Diffuse.xyz = o_WNormal * 0.5 + 0.5;
    o_Diffuse.w = 1.0;
}
```

如果看到彩色植被→法线计算正确
如果全灰→法线可能全为(0,0,0)

### 方法3：输出顶点颜色

```glsl
o_Diffuse = COLOR3; // 直接输出输入的顶点颜色
```

如果黑色→顶点数据COLOR3就是黑色
如果有颜色→说明后续计算有问题

## 总结

**最关键的检查点：**
1. ✅ 打印 `U_PBR_MainLightColor` 的值
2. ✅ 打印 `U_MaterialDiffuse` 的值  
3. ✅ 使用RenderDoc查看顶点数据中的 `COLOR3` 值
4. ✅ 验证Vertex Attribute的location绑定

**推荐调试顺序：**
1. 先用"强制输出红色"测试Pipeline是否工作
2. 再用"输出顶点颜色"测试数据传输
3. 最后用"输出法线颜色"测试法线计算
4. 检查Uniform Buffer的光照颜色值

这样可以快速定位到是**数据层问题**还是**计算层问题**。
