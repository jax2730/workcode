package net.pixelgame.xxsj;


public class DialogConfig {



    /**
     *
     * general test", "test_chat", "npc"
     * 多NPC模式建议使用 "npc" 后端
     */
    public static final String CHAT_BACKEND = "npc";

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
     * 默认NPC
     */
    public static final String DEFAULT_NPC_ID = "blacksmith";
    public static final String DEFAULT_NPC_NAME = "老铁匠";

    // ==================== 自动生成的端点 ====================

    /**
     * 对话端点:
     * - npc后端: /npc/chat/ (NPC系统专用，支持npc_id)
     * - 其他后端: /chat/{backend}/
     */
    public static String getChatEndpoint() {
        if ("npc".equals(CHAT_BACKEND)) {
            return "/npc/chat/";
        }
        return "/chat/" + CHAT_BACKEND + "/";
    }

    /**
     * 连接端点:
     * - npc后端: /npc/connect/
     * - 其他后端: /connect/{backend}/
     */
    public static String getConnectEndpoint() {
        if ("npc".equals(CHAT_BACKEND)) {
            return "/npc/connect/";
        }
        return "/connect/" + CHAT_BACKEND + "/";
    }

    /**
     * 清除端点:
     * - npc后端: /npc/clear/
     * - 其他后端: /clear/{backend}/
     */
    public static String getClearEndpoint() {
        if ("npc".equals(CHAT_BACKEND)) {
            return "/npc/clear/";
        }
        return "/clear/" + CHAT_BACKEND + "/";
    }

    /** Cookie名: {backend}_session_id */
    public static String getSessionCookieName() {
        return CHAT_BACKEND + "_session_id";
    }
}
