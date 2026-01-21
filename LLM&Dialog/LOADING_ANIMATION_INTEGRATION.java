/**
 * MainActivity 加载动画集成指南
 * 
 * 需要在 MainActivity.java 中添加以下代码
 */

// ============================================
// 1. 在 MainActivity 类的成员变量区域添加：
// ============================================

// 加载提示相关
private View loadingMessageView;
private LoadingAnimator loadingAnimator;

// ============================================
// 2. 在 onCreate() 或初始化对话框的地方添加：
// ============================================

// 初始化加载提示视图（但不添加到容器中）
LayoutInflater inflater = LayoutInflater.from(this);
loadingMessageView = inflater.inflate(R.layout.dialog_message_loading, null);
loadingAnimator = new LoadingAnimator(loadingMessageView);

// ============================================
// 3. 添加以下三个公共方法：
// ============================================

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

// ============================================
// 4. 现有的 addNpcMessage 方法保持不变
// ============================================

public void addNpcMessage(String message) {
    runOnUiThread(() -> {
        LayoutInflater inflater = LayoutInflater.from(this);
        View messageView = inflater.inflate(R.layout.dialog_message_npc, null);
        
        TextView npcNameText = messageView.findViewById(R.id.npc_name);
        TextView messageText = messageView.findViewById(R.id.npc_message);
        
        npcNameText.setText(dialogManager.getCurrentNpcName());
        messageText.setText(message);
        
        dialogMessagesContainer.addView(messageView);
        
        // 滚动到底部
        dialogScrollView.post(() -> dialogScrollView.fullScroll(View.FOCUS_DOWN));
    });
}

// ============================================
// 工作流程说明：
// ============================================

/*
 * 1. 用户发送消息
 * 2. DialogManager.handlePlayerMessage() 被调用
 * 3. 立即调用 mainActivity.showLoadingMessage() 显示 "●●●" 动画
 * 4. 发送网络请求到 Django 后端
 * 5. 等待 LLM 响应（可能需要几十秒）
 * 6. 收到响应后：
 *    - 调用 mainActivity.hideLoadingMessage() 隐藏加载动画
 *    - 调用 mainActivity.addNpcMessage(reply) 显示 NPC 回复
 * 7. 如果出错：
 *    - 调用 mainActivity.hideLoadingMessage() 隐藏加载动画
 *    - 可选：显示错误提示或使用本地模拟回复
 */

// ============================================
// 关于对话界面的配置：
// ============================================

/*
 * 问题：发起键盘对话框的界面是如何配置的？
 * 
 * 答案：对话界面由两个主要布局文件组成：
 * 
 * 1. dialog_panel.xml - 主对话界面
 *    - 包含对话历史区域（dialog_history_container）
 *    - 包含输入区域（dialog_input_container）
 *    - 位置：左下角，带有半透明背景
 * 
 * 2. input_panel.xml - 独立的输入面板
 *    - 这是一个简单的输入界面
 *    - 包含一个 EditText 和一个确认按钮
 *    - 用于其他场景的文本输入
 * 
 * 对话界面的工作流程：
 * 1. 点击对话按钮（buttonDialog）
 * 2. MainActivity 显示 dialog_container（设置 visibility = VISIBLE）
 * 3. 调用 DialogManager.startDialog(npcName, npcId)
 * 4. 用户在 dialog_input_text 中输入消息
 * 5. 点击 dialog_send_btn 或按回车键发送
 * 6. 消息通过 DialogManager 发送到后端
 * 7. 显示加载动画等待响应
 * 8. 收到响应后显示在 dialog_messages_container 中
 * 
 * 关键控件 ID：
 * - dialog_container: 整个对话界面的容器
 * - dialog_messages_container: 消息列表容器（LinearLayout）
 * - dialog_scroll_view: 滚动视图
 * - dialog_input_text: 输入框（EditText）
 * - dialog_send_btn: 发送按钮
 * - dialog_npc_name: NPC 名称显示
 * - dialog_close_btn: 关闭按钮
 */
