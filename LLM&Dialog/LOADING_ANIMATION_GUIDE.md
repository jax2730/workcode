# MainActivity 加载动画集成指南

## 概述

本指南说明如何在 MainActivity 中集成加载动画，在等待 LLM 响应时显示 "●●●" 的动态效果。

---

## 第一步：添加成员变量

在 MainActivity 类中，找到对话框相关的成员变量区域（dialogMessagesContainer、dialogScrollView 等附近），添加：

```java
// 加载提示相关
private View loadingMessageView;
private LoadingAnimator loadingAnimator;
```

---

## 第二步：初始化加载视图

在 MainActivity 的 `onCreate()` 方法中，找到初始化对话框控件的地方（在 `dialogMessagesContainer = ...` 附近），添加：

```java
// 初始化加载提示视图
LayoutInflater inflater = LayoutInflater.from(this);
loadingMessageView = inflater.inflate(R.layout.dialog_message_loading, null);
loadingAnimator = new LoadingAnimator(loadingMessageView);
```

---

## 第三步：添加三个公共方法

在 MainActivity 中，找到 `addNpcMessage()` 方法附近，添加以下三个方法：

```java
/**
 * 显示加载提示
 */
public void showLoadingMessage() {
    runOnUiThread(() -> {
        // 如果加载视图还没有添加到容器中，则添加
        if (loadingMessageView.getParent() == null) {
            dialogMessagesContainer.addView(loadingMessageView);
            
            // 滚动到底部
            dialogScrollView.post(() -> dialogScrollView.fullScroll(View.FOCUS_DOWN));
        }
        
        // 启动动画
        loadingAnimator.start();
    });
}

/**
 * 隐藏加载提示
 */
public void hideLoadingMessage() {
    runOnUiThread(() -> {
        // 停止动画
        loadingAnimator.stop();
        
        // 从容器中移除加载视图
        if (loadingMessageView.getParent() != null) {
            dialogMessagesContainer.removeView(loadingMessageView);
        }
    });
}

/**
 * 检查是否正在显示加载提示
 */
public boolean isLoadingMessageShowing() {
    return loadingMessageView.getParent() != null;
}
```

---

## 完成！

现在 DialogManager 已经配置好了调用这些方法：

1. **发送消息时**：自动显示加载动画
2. **收到响应时**：自动隐藏加载动画并显示 NPC 回复
3. **出错时**：自动隐藏加载动画

---

## 工作流程

```
用户发送消息
    ↓
显示 "●●●" 加载动画（波浪式闪烁）
    ↓
发送请求到 Django 后端
    ↓
等待 LLM 响应（最多 2 分钟）
    ↓
收到响应
    ↓
隐藏加载动画
    ↓
显示 NPC 回复
```

---

## 关于对话界面配置

### 主要布局文件

1. **dialog_panel.xml** - 主对话界面
   - 对话历史区域（左下角，420x320dp）
   - 输入区域（底部，带发送按钮）
   - 半透明背景，古风设计

2. **input_panel.xml** - 独立输入面板
   - 简单的 EditText + 确认按钮
   - 用于其他场景的文本输入

### 对话界面的关键控件

- `dialog_container` - 整个对话界面容器
- `dialog_messages_container` - 消息列表（LinearLayout）
- `dialog_scroll_view` - 滚动视图
- `dialog_input_text` - 输入框（EditText）
- `dialog_send_btn` - 发送按钮
- `dialog_npc_name` - NPC 名称
- `dialog_close_btn` - 关闭按钮

### 消息气泡样式

- **NPC 消息**：蓝色气泡（bubble_npc.xml），左对齐
- **玩家消息**：绿色气泡（bubble_player.xml），右对齐
- **加载动画**：使用 NPC 气泡样式，三个点波浪闪烁

---

## 已创建的文件

1. ✅ `dialog_message_loading.xml` - 加载动画布局
2. ✅ `LoadingAnimator.java` - 动画控制类
3. ✅ `DialogManager.java` - 已集成加载提示调用
4. ✅ `DialogConfig.java` - 超时时间已延长到 2 分钟

---

## 测试步骤

1. 重新编译并运行应用
2. 点击对话按钮，打开对话界面
3. 输入消息并发送
4. 观察加载动画（三个点波浪式闪烁）
5. 等待 LLM 响应
6. 加载动画消失，显示 NPC 回复

---

## 动画效果说明

- **三个点**：● ● ●
- **动画时长**：1.2 秒一个循环
- **效果**：波浪式淡入淡出
  - 第 1 个点：0ms 开始
  - 第 2 个点：200ms 开始
  - 第 3 个点：400ms 开始
- **透明度变化**：0.3 → 1.0 → 0.3
- **无限循环**：直到收到响应

---

## 注意事项

1. 确保 MainActivity 中已经初始化了 `dialogMessagesContainer` 和 `dialogScrollView`
2. 如果编译出错，检查 `R.layout.dialog_message_loading` 是否正确生成
3. 加载动画会自动添加到消息列表底部，收到响应后自动移除
4. 动画使用 ObjectAnimator，性能开销很小
