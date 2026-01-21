package net.pixelgame.xxsj;

/**
 * 对话系统配置
 * 可以在这里修改对话模式和服务器地址
 */
public class DialogConfig {
    
    // ==================== 配置项 ====================
    
    /**
     * 对话模式
     * - GENERAL: 通用对话模式（使用 /general/chat/ 接口）
     * - NPC: NPC对话模式（使用 /npc/chat/ 接口）
     */
    public static final DialogManager.DialogMode DEFAULT_MODE = DialogManager.DialogMode.NPC;
    
    /**
     * Django服务器地址
     * - Android模拟器访问本机: http://10.0.2.2:8000
     * - 真机访问局域网: http://192.168.x.x:8000
     * - 远程服务器: http://your-server.com:8000
     */
    public static final String SERVER_URL = "http://192.168.5.189:8000";
    
    /**
     * 是否启用网络请求
     * false: 只使用本地模拟回复
     * true: 连接到Django后端
     */
    public static final boolean ENABLE_NETWORK = true;
    
    /**
     * 网络请求超时时间（毫秒）
     * 由于 LLM 响应较慢，设置较长的超时时间
     * CONNECT_TIMEOUT: 连接超时（建立连接的时间）
     * READ_TIMEOUT: 读取超时（等待服务器响应的时间）
     */
    public static final int CONNECT_TIMEOUT = 15000;   // 15秒连接超时
    public static final int READ_TIMEOUT = 120000;     // 120秒读取超时（2分钟）
    
    /**
     * 默认NPC ID（用于NPC模式）
     * 
     * 可用的NPC ID（在 npc_configs/ 目录中）：
     * - blacksmith: 老铁匠（严肃、专业、热心）
     * - merchant: 精明商人（精明、健谈、贪财）
     * - guard_captain: 守卫队长
     * - herbalist: 草药师
     * - innkeeper: 客栈老板
     */
    public static final String DEFAULT_NPC_ID = "blacksmith";
    
    /**
     * 默认NPC名称
     */
    public static final String DEFAULT_NPC_NAME = "老铁匠";
    
    // ==================== 使用说明 ====================
    
    /*
     * 如何切换对话模式：
     * 
     * 1. 使用通用对话模式（general）：
     *    DEFAULT_MODE = DialogManager.DialogMode.GENERAL
     *    - 适合：普通聊天、问答、通用对话
     *    - 接口：/general/connect/, /general/chat/, /general/clear/
     * 
     * 2. 使用NPC对话模式（npc）：
     *    DEFAULT_MODE = DialogManager.DialogMode.NPC
     *    - 适合：游戏NPC对话、角色扮演
     *    - 接口：/npc/connect/, /npc/chat/, /npc/clear/
     *    - 需要指定 npc_id 和 npc_name
     * 
     * 如何修改服务器地址：
     * 
     * 1. Android模拟器测试：
     *    SERVER_URL = "http://10.0.2.2:8000"
     * 
     * 2. 真机测试（局域网）：
     *    SERVER_URL = "http://192.168.1.100:8000"  // 替换为你的电脑IP
     * 
     * 3. 远程服务器：
     *    SERVER_URL = "http://your-domain.com:8000"
     * 
     * 如何禁用网络（仅使用本地模拟）：
     *    ENABLE_NETWORK = false
     */
}
