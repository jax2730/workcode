package net.pixelgame.xxsj;

/**
 * 对话系统配置
 * 可以在这里修改对话后端和服务器地址
 * 
 * 【快速切换后端】只需修改 CHAT_BACKEND 即可:
 *   - "general"   : 通用对话 (general_chat.py)
 *   - "test"      : 简单测试对话 (test.py)  
 *   - "test_chat" : 环境提取对话 (test_chat.py)
 *   - "npc"       : NPC对话系统 (npc_system/)
 */
public class DialogConfig {

    // ==================== 核心配置 ====================

    /**
     * 【修改这里切换后端】
     * 可选值: "general", "test", "test_chat", "npc"
     */
    public static final String CHAT_BACKEND = "general";

    /**
     * Django服务器地址
     */
    public static final String SERVER_URL = "http://192.168.5.189:8000";

    /**
     * 是否启用网络请求
     */
    public static final boolean ENABLE_NETWORK = true;

    /**
     * 网络超时 (毫秒)
     */
    public static final int CONNECT_TIMEOUT = 15000;
    public static final int READ_TIMEOUT = 120000;

    /**
     * 默认NPC (仅 npc 后端使用)
     */
    public static final String DEFAULT_NPC_ID = "blacksmith";
    public static final String DEFAULT_NPC_NAME = "老铁匠";

    // ==================== 自动生成的端点 ====================

    /** 对话端点: /api/{backend}/chat/ */
    public static String getChatEndpoint() {
        return "/api/" + CHAT_BACKEND + "/chat/";
    }

    /** 连接端点: /api/{backend}/connect/ */
    public static String getConnectEndpoint() {
        return "/api/" + CHAT_BACKEND + "/connect/";
    }

    /** 清除端点: /api/{backend}/clear/ */
    public static String getClearEndpoint() {
        return "/api/" + CHAT_BACKEND + "/clear/";
    }

    /** Cookie名: {backend}_session_id */
    public static String getSessionCookieName() {
        return CHAT_BACKEND + "_session_id";
    }
}
