# 大世界浮点精度问题 - Camera-Relative 坐标系技术方案

## 问题背景

### 浮点精度损失

在大世界场景中，世界坐标可达 7 位数以上（如 3,500,000），而 `float` 只有 23 位尾数，有效数字约 7-8 位。

**问题示例**：
```cpp
// 世界坐标
Vector3 worldPos = (3,500,000.5625, 100.125, 2,800,000.3125);

// 转换为 float 后
float x = 3500000.5625f;  
// 实际存储 ≈ 3500000.0 或 3500001.0
// 小数部分 0.5625 完全丢失！
```

**后果**：
- 物体抖动（顶点位置在整数间跳变）
- 贴图错位（UV计算不准）
- 法线闪烁（法线空间计算误差）

---

## 解决方案：Camera-Relative 坐标系

### 核心思想

将所有坐标转换为"相对于摄像机"的局部坐标：

$$
\mathbf{p}_{\text{cr}} = \mathbf{p}_{\text{world}} - \mathbf{c}
$$


[


\]

其中：
- $$
  \( \mathbf{p}_{\text{world}} \) ：世界坐标
  $$

- 

- 

- $$
  \( \mathbf{c} \)：摄像机世界位置
  $$

- 

- $$
  \( \mathbf{p}_{\text{cr}} \)：相机相对坐标（Camera-Relative）
  $$

- 

### 效果对比

**未使用 CR**：

```
摄像机：(3,500,000, 100, 2,800,000)
物体：  (3,500,010.5, 102.3, 2,800,005.2)
→ float 精度损失，小数丢失，物体抖动
```

**使用 CR**：
```
摄像机（世界）：(3,500,000, 100, 2,800,000)
摄像机（CR）：  (0, 0, 0)  ← 相机是原点
物体（CR）：    (10.5, 2.3, 5.2)  ← 小坐标，精度完美！
```

---

## 数学推导

### 1. 变换矩阵

世界变换矩阵（仿射）：

$$
 \[
\mathbf{M} = \begin{bmatrix} \mathbf{R} & \mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$


其中：
$$
\( \mathbf{R} \)：旋转缩放（3×3）
$$

$$
\( \mathbf{t} \)：平移（3×1）
$$


**Camera-Relative 变换**：

$$
\[
\mathbf{M}' = \begin{bmatrix} \mathbf{R} & \mathbf{t} - \mathbf{c} \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$


**代码实现**（仅修改平移列）：

```cpp
Matrix4 worldCR = *m_pWorldMatrix;  // 复制原矩阵
worldCR[0][3] -= (float)camPos.x;   // tx' = tx - cx
worldCR[1][3] -= (float)camPos.y;   // ty' = ty - cy  
worldCR[2][3] -= (float)camPos.z;   // tz' = tz - cz
```

**为什么只改平移列**：
- 保持旋转/缩放不变
- 避免引入额外的数值误差
- 最小化代码改动

---

### 2. 逆矩阵

**原理**：
$$
\[
\mathbf{M}^{-1} = \begin{bmatrix} \mathbf{R}^{-1} & -\mathbf{R}^{-1}\mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$



$$
\[
(\mathbf{M}')^{-1} = \begin{bmatrix} \mathbf{R}^{-1} & -\mathbf{R}^{-1}(\mathbf{t} - \mathbf{c}) \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$


**代码实现**：
```cpp
// 错误：直接用旧的 mInvWorldMatrix
*pMatrix4 = *mInvWorldMatrix;  

// 对相机相对后的矩阵求逆
Matrix4 worldCR = *m_pWorldMatrix;
worldCR[0][3] -= (float)camPos.x;
worldCR[1][3] -= (float)camPos.y;
worldCR[2][3] -= (float)camPos.z;
*pMatrix4 = worldCR.inverseAffine();
```

---

### 3. 法线变换（逆转置）

法线变换使用逆转置矩阵：
$$
\[
\mathbf{n}' = \left((\mathbf{M}')^{-1}\right)^T \mathbf{n}
\]
$$




**代码实现**：
```cpp
Matrix4 worldCR = *m_pWorldMatrix;
worldCR[0][3] -= (float)camPos.x;
worldCR[1][3] -= (float)camPos.y;
worldCR[2][3] -= (float)camPos.z;
*pMatrix4 = worldCR.inverseAffine().transpose();
```

---

### 4. **关键问题：View 矩阵适配**

#### 传统 View 矩阵：

$$
\[
\mathbf{V} = \begin{bmatrix} \mathbf{R}^T & -\mathbf{R}^T\mathbf{c} \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$



#### Camera-Relative 模式下的 View 矩阵：

**方案A：移除 View 中的平移**（推荐）

$$
\[
\mathbf{V}' = \begin{bmatrix} \mathbf{R}^T & \mathbf{0} \\ \mathbf{0}^T & 1 \end{bmatrix}
\]
$$


```cpp
// 代码实现
Matrix4 viewCR = pCam->getViewMatrix();
viewCR[0][3] = 0.0f;
viewCR[1][3] = 0.0f;
viewCR[2][3] = 0.0f;
viewCR[3][3] = 1.0f;
```

**方案B：使用引擎原有的双精度组合函数**

```cpp
// DBMatrix4::concatenate 已经处理了相机相对逻辑
*pMatrix4 = DBMatrix4::concatenate(projectMatrix, viewMatrix, *m_pWorldMatrix, pCam->getPosition());
```

#### 问题根源：

当前实现同时做了：
1. W' = W − c（在 WorldMatrix 中）
2. V 仍然包含 −R^T·c

结果：
$$
\[
\mathbf{V} \cdot \mathbf{W}' = \begin{bmatrix} \mathbf{R}^T & -\mathbf{R}^T\mathbf{c} \end{bmatrix} \cdot \begin{bmatrix} \mathbf{R} & \mathbf{t} - \mathbf{c} \end{bmatrix} = \begin{bmatrix} \mathbf{I} & \mathbf{R}^T\mathbf{t} - 2\mathbf{R}^T\mathbf{c} \end{bmatrix}
\]
$$


**相机位置 c 被减了 2 次！** 这就是为什么：
```
W.t = (0, -2.9, -9.5)  ← 小数值 ✅
VW.t = (365472, -12.5, -432233)  ← 又变巨大！❌
```

---

## 代码修改方案

### 方案1：修改 View 矩阵（推荐，最小改动）

在 camera-relative 模式下，移除 View 矩阵的平移：

```cpp
case U_WorldViewProjectMatrix:
{
    Camera* pCam = m_sceneManager->getActiveCamera();
    const DMatrix4& viewMatrix = pCam->getViewMatrix();
    Matrix4 projectMatrix = /* ... */;
    
    if (g_EnableGpuCameraRelative)
    {
        // 1. 世界矩阵做相机相对
        Matrix4 w = *m_pWorldMatrix;
        const DVector3 camPos = pCam->getPosition();
        w[0][3] -= (float)camPos.x;
        w[1][3] -= (float)camPos.y;
        w[2][3] -= (float)camPos.z;
        
        // 2. View 矩阵移除平移（相机相对模式下相机在原点）
        Matrix4 viewCR = viewMatrix;
        viewCR[0][3] = 0.0f;
        viewCR[1][3] = 0.0f;
        viewCR[2][3] = 0.0f;
        
        // 3. 组合：P·V'·W'
        *pMatrix4 = projectMatrix * viewCR * w;
    }
    else
    {
        *pMatrix4 = DBMatrix4::concatenate(projectMatrix, viewMatrix, *m_pWorldMatrix, pCam->getPosition());
    }
}
```

### 方案2：禁用 GPU 层的 Camera-Relative（回退方案）

如果改动太大，暂时禁用：

```cpp
static bool g_EnableGpuCameraRelative = false;  // 关闭
```

然后只在 CPU 层（EchoKarstGroup.cpp）做世界原点重定位。

---

## 调试断点位置

### 1. 断点：`EchoGpuParamsManager.cpp:1110`（U_WorldMatrix）
观察：
- `camPos`：摄像机位置
- `tempMat[0][3], tempMat[1][3], tempMat[2][3]`（减法前后）

### 2. 断点：`EchoGpuParamsManager.cpp:900`（U_WorldViewProjectMatrix）
观察：
- `w[0][3]`：相机相对后的 tx
- `viewMatrix[0][3]`：View 矩阵的平移列
- 最终 `*pMatrix4` 的第四列

### 3. 断点：`EchoKarstGroup.cpp:449`（worldOriginRebasing）
观察：
- `offset`：世界原点变化量
- `mBasePosition`：地形基准位置
- `mOriginPosition`：计算结果

---

## 预期日志输出

开启 `g_DebugCameraRelative = true` 后：

**正常情况**（修复后）：
```
[CR-FULL] U_WorldMatrix W.t=(5.2,-1.3,12.8) VW.t=(3.1,-2.5,8.9) MVP.t=(...)
```
- W.t 小，VW.t 也小 ✅

**当前异常**（修复前）：
```
[CR-FULL] U_WorldMatrix W.t=(0.0,-2.9,-9.5) VW.t=(365472.0,...,-432233.5) MVP.t=(440863.4,...)
[CR-CRITICAL] DOUBLE_SUBTRACT_DETECTED! W.t小但VW.t巨大，相机位置被减了2次！
```
- W.t 小，但 VW.t 巨大 ❌
- 自动触发 [CR-CRITICAL] 警告

---

## 修复步骤

### 立刻修复（3处，每处5行）

在以下3个case中，同时修改 View 矩阵：

1. **U_WorldViewProjectMatrix**
2. **U_VS_WorldViewMatrix**
3. **U_VS_InvWorldViewMatrix**

模板代码：
```cpp
if (g_EnableGpuCameraRelative)
{
    Matrix4 w = *m_pWorldMatrix;
    const DVector3 camPos = pCam->getPosition();
    w[0][3] -= (float)camPos.x; w[1][3] -= (float)camPos.y; w[2][3] -= (float)camPos.z;
    
    // 关键：View 也要去掉平移
    Matrix4 viewCR = viewMatrix;
    viewCR[0][3] = 0.0f; viewCR[1][3] = 0.0f; viewCR[2][3] = 0.0f;
    
    *pMatrix4 = viewCR.concatenate(w);  // 或 proj * viewCR * w
}
```

---

## 抖动原因总结

1. **根因**：View 矩阵未适配 Camera-Relative，导致相机位置被减2次
2. **表现**：
   - W.t 小（正确）
   - VW.t、MVP.t 巨大（错误）
   - 地面/人物被计算到远处 → 剔除/不可见
   - 树木可能只用 W → 正常显示
3. **抖动**：混用两套坐标系，每帧计算不一致

---

## 验证方法

### 检查日志：
```
[CR-CRITICAL] ... DOUBLE_SUBTRACT_DETECTED!
```
如果出现此行 → 确认问题存在

### 修复后预期：
```
[CR-FULL] U_WorldMatrix W.t=(10.5,2.3,5.2) VW.t=(8.1,1.9,3.7) MVP.t=(...)
```
- W.t、VW.t、MVP.t 都是小数值
- 不再有 [CR-CRITICAL] 警告
- 地面/人物/树木全部正常显示
- 画面不再抖动

---

---

## View 矩阵的 Camera-Relative 变换（详细推导）

### 问题背景

在传统渲染管线中：
```
顶点变换：p_clip = P · V · W · p_local
```

其中：
- `W`：世界变换（Local → World）
- `V`：视图变换（World → Camera Space）
- `P`：投影变换（Camera Space → Clip Space）

当我们对世界矩阵做 camera-relative 变换后（`W' = T · W`），需要同步调整 View 矩阵，避免相机位置被重复处理。

---

### 1. View 矩阵的数学结构

#### 传统 View 矩阵

$$
\mathbf{V} = \begin{bmatrix} 
\mathbf{R}^T & -\mathbf{R}^T \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

其中：
- `R`：摄像机的旋转矩阵（World → Camera 的旋转）
- `c`：摄像机在世界空间的位置
- `R^T`：旋转矩阵的转置（等于旋转的逆）
- `-R^T · c`：摄像机位置经过旋转变换后取反

**物理意义**：
将世界坐标系原点移动到摄像机位置，并旋转坐标轴使其与摄像机朝向对齐。

#### 逆 View 矩阵

$$
\mathbf{V}^{-1} = \begin{bmatrix} 
\mathbf{R} & \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

**物理意义**：
从摄像机空间变换回世界空间。

---

### 2. Camera-Relative 模式下的 View 矩阵变换

#### 目标

在 camera-relative 模式下：
- 世界坐标已经以摄像机为原点：`p_world' = p_world - c`
- 摄像机在新坐标系中位于原点：`c' = 0`
- 因此 View 矩阵不应再包含平移分量

**期望的 View 矩阵**：

$$
\mathbf{V}' = \begin{bmatrix} 
\mathbf{R}^T & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

---

### 3. 方法一：矩阵乘法形式（统一变换）

#### 3.1 View 矩阵变换推导

**思路**：用平移矩阵 `T^-1` 右乘 View 矩阵，消去平移分量。

**推导**：

$$
\mathbf{V}' = \mathbf{V} \cdot \mathbf{T}^{-1}
$$

其中：

$$
\mathbf{T}^{-1} = \begin{bmatrix} 
\mathbf{I} & \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

展开计算：

$$
\mathbf{V}' = \begin{bmatrix} 
\mathbf{R}^T & -\mathbf{R}^T \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} \cdot \begin{bmatrix} 
\mathbf{I} & \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

矩阵乘法规则：

$$
\begin{aligned}
\text{左上 3×3:} \quad & \mathbf{R}^T \cdot \mathbf{I} = \mathbf{R}^T \\
\text{右上 3×1:} \quad & \mathbf{R}^T \cdot \mathbf{c} + (-\mathbf{R}^T \mathbf{c}) \cdot 1 \\
& = \mathbf{R}^T \mathbf{c} - \mathbf{R}^T \mathbf{c} = \mathbf{0} \\
\text{左下 1×3:} \quad & \mathbf{0}^T \cdot \mathbf{I} = \mathbf{0}^T \\
\text{右下 1×1:} \quad & \mathbf{0}^T \cdot \mathbf{c} + 1 \cdot 1 = 1
\end{aligned}
$$

**结果**：

$$
\mathbf{V}' = \begin{bmatrix} 
\mathbf{R}^T & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

**代码实现**：

```cpp
// 构造 T^-1（平移 +c）
static DMatrix4 T_INV_CR;
T_INV_CR.makeTransform(camPos, Vector3::UNIT_SCALE, Quaternion::IDENTITY);

// View 矩阵变换：V' = V · T^-1
const DMatrix4& viewOrig = m_sceneManager->getActiveCamera()->getViewMatrix();
static DMatrix4 viewCR;
viewCR = viewOrig * T_INV_CR;

// 验证：viewCR 的平移列应该接近 0
// viewCR[0][3] ≈ 0
// viewCR[1][3] ≈ 0
// viewCR[2][3] ≈ 0
```

---

#### 3.2 逆 View 矩阵变换推导

**思路**：用平移矩阵 `T` 左乘逆 View 矩阵。

**推导**：

$$
(\mathbf{V}')^{-1} = \mathbf{T} \cdot \mathbf{V}^{-1}
$$

其中：

$$
\mathbf{T} = \begin{bmatrix} 
\mathbf{I} & -\mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

展开计算：

$$
(\mathbf{V}')^{-1} = \begin{bmatrix} 
\mathbf{I} & -\mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} \cdot \begin{bmatrix} 
\mathbf{R} & \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

矩阵乘法：

$$
\begin{aligned}
\text{左上 3×3:} \quad & \mathbf{I} \cdot \mathbf{R} = \mathbf{R} \\
\text{右上 3×1:} \quad & \mathbf{I} \cdot \mathbf{c} + (-\mathbf{c}) \cdot 1 \\
& = \mathbf{c} - \mathbf{c} = \mathbf{0} \\
\text{左下 1×3:} \quad & \mathbf{0}^T \cdot \mathbf{R} = \mathbf{0}^T \\
\text{右下 1×1:} \quad & \mathbf{0}^T \cdot \mathbf{c} + 1 \cdot 1 = 1
\end{aligned}
$$

**结果**：

$$
(\mathbf{V}')^{-1} = \begin{bmatrix} 
\mathbf{R} & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

**代码实现**：

```cpp
// 构造 T（平移 -c）
static DMatrix4 T_CR;
T_CR.makeTransform(-camPos, Vector3::UNIT_SCALE, Quaternion::IDENTITY);

// 逆 View 矩阵变换：invV' = T · invV
const DMatrix4& invViewOrig = m_sceneManager->getActiveCamera()->getInverseViewMatrix();
static DMatrix4 invViewCR;
invViewCR = T_CR * invViewOrig;

// 验证：invViewCR 的平移列应该接近 0
```

---

#### 3.3 验证：V' 与 invV' 互逆

检验：`V' · invV' = I`

$$
\begin{bmatrix} 
\mathbf{R}^T & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} \cdot \begin{bmatrix} 
\mathbf{R} & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} = \begin{bmatrix} 
\mathbf{R}^T \mathbf{R} & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} = \begin{bmatrix} 
\mathbf{I} & \mathbf{0} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

**结论**：验证通过！ ✅

---

### 4. 方法二：直接修改法（减法形式）

#### 4.1 原理

**观察**：矩阵乘法最终结果是将平移列置零，我们可以直接修改。

**推导步骤**：

1. **原始 View 矩阵**：

$$
\mathbf{V} = \begin{bmatrix} 
R_{11} & R_{12} & R_{13} & -\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_1 \\ 
R_{21} & R_{22} & R_{23} & -\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_2 \\ 
R_{31} & R_{32} & R_{33} & -\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_3 \\ 
0 & 0 & 0 & 1 
\end{bmatrix}
$$

其中第四列（平移列）为：

$$
\begin{bmatrix} 
-\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_1 \\ 
-\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_2 \\ 
-\mathbf{R}^T \mathbf{c} \cdot \mathbf{e}_3 \\ 
1 
\end{bmatrix}
$$

2. **Camera-Relative 模式下**，摄像机位置 `c' = 0`，因此：

$$
-\mathbf{R}^T \mathbf{c}' = -\mathbf{R}^T \cdot \mathbf{0} = \mathbf{0}
$$

3. **直接置零**：

$$
\mathbf{V}' = \begin{bmatrix} 
R_{11} & R_{12} & R_{13} & 0 \\ 
R_{21} & R_{22} & R_{23} & 0 \\ 
R_{31} & R_{32} & R_{33} & 0 \\ 
0 & 0 & 0 & 1 
\end{bmatrix}
$$

**代码实现**：

```cpp
// 方法二：直接修改（最简洁）
Matrix4 viewCR = pCam->getViewMatrix();
viewCR[0][3] = 0.0f;  // 置零第四列的前三行
viewCR[1][3] = 0.0f;
viewCR[2][3] = 0.0f;
// viewCR[3][3] 保持为 1.0f
```

---

#### 4.2 为什么减法形式等价于矩阵乘法？

**矩阵乘法形式**：

$$
\mathbf{V}' = \mathbf{V} \cdot \mathbf{T}^{-1} = \begin{bmatrix} 
\mathbf{R}^T & -\mathbf{R}^T \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix} \cdot \begin{bmatrix} 
\mathbf{I} & \mathbf{c} \\ 
\mathbf{0}^T & 1 
\end{bmatrix}
$$

右上角计算：

$$
\mathbf{R}^T \mathbf{c} + (-\mathbf{R}^T \mathbf{c}) = \mathbf{0}
$$

**减法形式**：

```cpp
// 原值：viewMatrix[0][3] = -(R^T · c)_x
// 新值：viewCR[0][3] = 0
// 等价于：viewCR[0][3] = viewMatrix[0][3] - viewMatrix[0][3]
//       = -(R^T · c)_x - (-(R^T · c)_x) = 0
```

**结论**：减法形式是矩阵乘法的优化实现，避免了完整的矩阵乘法开销。

---

### 5. 涉及的 Uniform 及代码修改

#### 5.1 U_ViewProjectMatrix

**定义**：`VP = P · V`

**Camera-Relative 模式**：

```cpp
case U_ViewProjectMatrix:
{
    Camera* pCam = m_sceneManager->getActiveCamera();
    Matrix4 projectMatrix = pCam->getProjectionMatrixWithRSDepth();
    
    if (g_EnableGpuCameraRelative)
    {
        // 方法一：矩阵乘法形式
        const DMatrix4& viewOrig = pCam->getViewMatrix();
        static DMatrix4 T_INV_CR;
        T_INV_CR.makeTransform(pCam->getPosition(), Vector3::UNIT_SCALE, Quaternion::IDENTITY);
        DMatrix4 viewCR = viewOrig * T_INV_CR;
        *pMatrix4 = projectMatrix * viewCR;
        
        // 或 方法二：减法形式（推荐，更高效）
        DMatrix4 viewCR = pCam->getViewMatrix();
        viewCR[0][3] = 0.0f;
        viewCR[1][3] = 0.0f;
        viewCR[2][3] = 0.0f;
        *pMatrix4 = projectMatrix * viewCR;
    }
    else
    {
        *pMatrix4 = projectMatrix * pCam->getViewMatrix();
    }
}
break;
```

---

#### 5.2 U_InvViewProjectMatrix

**定义**：`(VP)^-1 = V^-1 · P^-1`

**Camera-Relative 模式**：

```cpp
case U_InvViewProjectMatrix:
{
    Camera* pCam = m_sceneManager->getActiveCamera();
    Matrix4 invProjectMatrix = pCam->getProjectionMatrixWithRSDepth().inverse();
    
    if (g_EnableGpuCameraRelative)
    {
        // 方法一：矩阵乘法形式
        const DMatrix4& invViewOrig = pCam->getInverseViewMatrix();
        static DMatrix4 T_CR;
        T_CR.makeTransform(-pCam->getPosition(), Vector3::UNIT_SCALE, Quaternion::IDENTITY);
        DMatrix4 invViewCR = T_CR * invViewOrig;
        *pMatrix4 = invViewCR * invProjectMatrix;
        
        // 或 方法二：从 viewCR 求逆（推荐）
        DMatrix4 viewCR = pCam->getViewMatrix();
        viewCR[0][3] = 0.0f;
        viewCR[1][3] = 0.0f;
        viewCR[2][3] = 0.0f;
        DMatrix4 invViewCR = viewCR.inverse();  // 或 inverseAffine()
        *pMatrix4 = invViewCR * invProjectMatrix;
    }
    else
    {
        *pMatrix4 = pCam->getInverseViewMatrix() * invProjectMatrix;
    }
}
break;
```

---

#### 5.3 getViewMatrix() 返回值的处理

**问题**：是否需要修改 `Camera::getViewMatrix()` 本身？

**答案**：**不需要！**

**理由**：
1. `getViewMatrix()` 返回的是传统 View 矩阵（包含 `-R^T · c`）
2. 我们在 GPU 提交时动态转换为 camera-relative 版本
3. 保持引擎其他部分（UI、物理、AI）仍使用传统坐标系
4. 只有 GPU 渲染管线使用 camera-relative

**架构优势**：
- **最小侵入**：不改动 Camera 类
- **兼容性好**：非渲染模块不受影响
- **易于调试**：可随时开关 `g_EnableGpuCameraRelative`

---

### 6. 方法对比总结

| 方法 | 优点 | 缺点 | 推荐场景 |
|------|------|------|----------|
| **矩阵乘法形式** | 数学推导清晰，易于理解 | 多一次矩阵乘法开销 | 教学、验证 |
| **减法形式（直接置零）** | 高效，零开销 | 需理解背后数学原理 | 生产环境 |

**最佳实践**：
- 开发/调试阶段：用矩阵乘法形式，打印中间结果验证
- 正式发布：用减法形式，性能最优

---

### 7. 完整的坐标变换链

#### Camera-Relative 模式下的完整流程

1. **World 矩阵变换**（在 commitUniformParams 源头）：
   ```cpp
   T = [I | -c]
   W' = T · W
   invW' = invW · T^-1
   ```

2. **View 矩阵变换**（各 uniform case 中）：
   ```cpp
   V' = V · T^-1  或  直接置零平移列
   invV' = T · invV  或  从 V' 求逆
   ```

3. **顶点变换链**：
   ```
   p_world' = W' · p_local = (T · W) · p_local
   p_camera = V' · p_world' = (V · T^-1) · (T · W) · p_local
            = V · (T^-1 · T) · W · p_local
            = V · I · W · p_local
            = V · W · p_local
   ```

**结论**：`T^-1 · T = I`，两个变换完全抵消，最终等价于传统变换！但中间过程使用小坐标，避免精度损失。

---

### 8. 相机位置 U_CameraPosition 的处理

**问题**：Shader 中 `u_CameraPosition` 应该传什么值？

**答案**：**传 (0, 0, 0)** 或 **不传（使用相对坐标）**

**理由**：

在 Camera-Relative 模式下：
- 摄像机在新坐标系中位于原点
- Shader 中计算 `viewDir = normalize(cameraPos - worldPos)` 应该是：
  ```glsl
  // worldPos 已经是 camera-relative（p_world - c）
  // cameraPos 在 camera-relative 下是 (0, 0, 0)
  vec3 viewDir = normalize(vec3(0,0,0) - worldPos);
               = normalize(-worldPos);
  ```

**代码修改**：

```cpp
case U_CameraPosition:
case U_VS_CameraPosition:
case U_PS_CameraPosition:
{
    if (g_EnableGpuCameraRelative)
    {
        // Camera-Relative 模式：相机在原点
        pVector4->x = 0.0f;
        pVector4->y = 0.0f;
        pVector4->z = 0.0f;
        pVector4->w = 1.0f;
    }
    else
    {
        // 传统模式：传世界位置
        const Vector3& pCameraPosition = m_sceneManager->getActiveCamera()->getPosition();
        pVector4->x = pCameraPosition.x;
        pVector4->y = pCameraPosition.y;
        pVector4->z = pCameraPosition.z;
        pVector4->w = 1.0f;
    }
}
break;
```

---

### 9. 数学验证：为什么不会"相机被减2次"

#### 错误的做法（导致相机被减2次）

```cpp
// 错误：W 减了 c，但 V 还包含 -R^T·c
W'[0][3] -= c.x;  // W' = [R | t - c]
V = [R^T | -R^T·c]  // 未修改

// 计算 V · W'：
VW = [R^T | -R^T·c] · [R | t - c]
   = [I | R^T·(t - c) - R^T·c]
   = [I | R^T·t - 2·R^T·c]  ❌ 相机被减2次！
```

#### 正确的做法

```cpp
// 正确：W 减了 c，V 也去掉 -R^T·c
W' = T · W = [R | t - c]
V' = V · T^-1 = [R^T | 0]

// 计算 V' · W'：
V'W' = [R^T | 0] · [R | t - c]
     = [I | R^T·(t - c)]  ✅ 正确！
```

---

## 总结

**问题**：GPU 层 W 做了 t−c，但 View 未适配，相机被减2次  
**修复**：Camera-Relative 模式下，View 也要去掉平移  
**改动量**：3处 case，每处加 3 行（去掉 View 平移）  
**验证**：日志中 VW.t 应该与 W.t 同量级（小数值）

**两种实现方式**：
1. **矩阵乘法**：`V' = V · T^-1`，`invV' = T · invV`（数学清晰）
2. **直接置零**：`viewCR[0..2][3] = 0`（高效简洁，推荐生产环境）

**核心数学**：`T^-1 · T = I`，两个变换相互抵消，保证最终结果与传统变换等价，但中间使用小坐标避免精度损失。

