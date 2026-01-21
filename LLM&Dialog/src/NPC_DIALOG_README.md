# NPC对话系统使用说明

## 📱 UI布局

### 对话框位置
- **对话历史区**: 左下角，480dp × 360dp
- **输入区**: 底部，520dp高度
- **触发按钮**: `buttonDialog` (control_panel.xml 第251行)

### 视觉设计
- **半透明背景**: 不遮挡游戏画面
- **气泡样式**: 
  - 玩家消息：绿色气泡，右对齐
  - NPC消息：蓝色气泡，左对齐
- **古风配色**: 金色按钮 + 青色/绿色文字

---

## 🎮 使用方法

### 1. 打开对话框
点击 `buttonDialog` 按钮，对话框从左下角展开。

### 2. 发送消息
- 在输入框输入文字
- 点击"发送"按钮或按回车键

### 3. 关闭对话框
点击对话框右上角的关闭按钮。

---

## 🔧 代码集成

### MainActivity中的关键方法

```java
// 开始与NPC对话
dialogManager.startDialog("苏清寒", "npc_001");

// 添加NPC消息
addNpcMessage("你好，少侠");

// 清空对话历史
clearDialogHistory();
```

### 从C++触发对话

```java
// 在C++中调用（通过JNI）
mainActivity.startNpcDialog("铁匠", "npc_002");
```

---

## 🌐 连接Django后端

### 修改 DialogManager.java

在 `handlePlayerMessage()` 方法中添加HTTP请求：

```java
private void handlePlayerMessage(String message) {
    // 发送到Django后端
    String url = "http://localhost:8000/npc/chat/";
    
    // 使用OkHttp或Volley发送POST请求
    JSONObject json = new JSONObject();
    json.put("message", message);
    json.put("npc_id", currentNpcId);
    json.put("session_id", sessionId);
    
    // 接收响应并显示
    // response -> mainActivity.addNpcMessage(response);
}
```

### Django API端点

```python
# urls.py
path('npc/chat/', npc_chat_view),

# views.py
@csrf_exempt
def npc_chat_view(request):
    message = request.POST.get('message')
    npc_id = request.POST.get('npc_id')
    session_id = request.COOKIES.get('npc_session_id')
    
    # 调用npc_chat.py
    result = npc_chat.chat(message, npc_id, session_id)
    return JsonResponse(result)
```

---

## 📝 自定义NPC

### 在 DialogManager.java 中添加NPC

```java
// 添加新NPC的预设回复
npcResponses.put("药师", new String[]{
    "这是我祖传的秘方，包治百病！",
    "你看起来气色不太好，要不要来点补药？",
    "江湖险恶，记得常备金创药。"
});
```

---

## 🎨 UI自定义

### 修改颜色
- `bubble_player.xml`: 玩家气泡颜色
- `bubble_npc.xml`: NPC气泡颜色
- `send_btn_bg.xml`: 发送按钮颜色

### 修改尺寸
- `dialog_panel.xml`: 对话框大小和位置
- `dialog_message_*.xml`: 气泡最大宽度

---

## ✅ 功能清单

- [x] 对话框展开/收起
- [x] 玩家消息发送
- [x] NPC消息显示
- [x] 消息气泡样式
- [x] 自动滚动到底部
- [x] 软键盘管理
- [x] NPC名称显示
- [x] 对话历史清空
- [ ] 表情功能
- [ ] Django后端连接
- [ ] 语音输入
- [ ] 消息历史保存

---

## 🐛 已知问题

1. **表情按钮**: 目前只显示Toast，未实现表情选择
2. **网络请求**: 需要添加OkHttp或Volley依赖
3. **会话管理**: 需要实现session_id的持久化

---

## 📦 依赖项

```gradle
// build.gradle
dependencies {
    // 网络请求（可选）
    implementation 'com.squareup.okhttp3:okhttp:4.11.0'
    
    // JSON解析（可选）
    implementation 'com.google.code.gson:gson:2.10.1'
}
```

---

## 🚀 下一步

1. **连接Django**: 实现HTTP请求到NPC对话API
2. **会话管理**: 保存对话历史到本地数据库
3. **表情系统**: 添加表情选择面板
4. **语音输入**: 集成语音识别功能
5. **动画效果**: 添加消息出现动画
