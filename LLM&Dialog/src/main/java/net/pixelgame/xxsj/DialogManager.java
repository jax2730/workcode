package net.pixelgame.xxsj;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

/**
 * 对话管理器 - 统一接口版
 * 支持任意后端: general, test, test_chat, npc 等
 * 切换后端只需修改 DialogConfig.CHAT_BACKEND
 */
public class DialogManager {

    private static final String TAG = "DialogManager";

    private static DialogManager instance;
    private MainActivity mainActivity;
    private Handler handler;

    // 配置 (从 DialogConfig 读取)
    private String serverUrl = DialogConfig.SERVER_URL;
    private boolean networkEnabled = DialogConfig.ENABLE_NETWORK;

    // 当前对话状态
    private String currentNpcName = "未知NPC";
    private String currentNpcId = "";
    private String sessionId = "";
    private String playerId = "";

    // 本地备用回复
    private Map<String, String[]> fallbackResponses;

    private DialogManager() {
        handler = new Handler(Looper.getMainLooper());
        initFallbackResponses();
    }

    public static DialogManager getInstance() {
        if (instance == null) {
            instance = new DialogManager();
        }
        return instance;
    }

    public void setMainActivity(MainActivity activity) {
        this.mainActivity = activity;
    }

    public void setServerUrl(String url) { this.serverUrl = url; }
    public void setNetworkEnabled(boolean enabled) { this.networkEnabled = enabled; }

    private void initFallbackResponses() {
        fallbackResponses = new HashMap<>();
        fallbackResponses.put("老铁匠", new String[]{"哟，又来了个冒险者。", "想要好兵器？拿银子来！"});
        fallbackResponses.put("精明商人", new String[]{"欢迎欢迎！", "这个价格已经很实惠了！"});
        fallbackResponses.put("守卫队长", new String[]{"站住！报上名来！", "有什么事快说。"});
        fallbackResponses.put("草药师", new String[]{"需要治疗吗？", "这种草药很稀有。"});
        fallbackResponses.put("客栈老板", new String[]{"客官里面请！", "要住店吗？"});
        fallbackResponses.put("default", new String[]{"你好！", "有什么可以帮你的？"});
    }

    /**
     * 开始对话
     */
    public void startDialog(String npcName, String npcId) {
        this.currentNpcName = npcName;
        this.currentNpcId = npcId;

        if (mainActivity != null) {
            mainActivity.setDialogNpcName(npcName);
            mainActivity.clearDialogHistory();
            mainActivity.showLoadingMessage();
            connectToBackend();
        }
    }

    /** 连接到后端 */
    private void connectToBackend() {
        if (!networkEnabled) {
            useFallbackGreeting();
            return;
        }

        new Thread(() -> {
            try {
                URL url = new URL(serverUrl + DialogConfig.getConnectEndpoint());
                HttpURLConnection conn = createConnection(url);

                // 构建请求体
                JSONObject body = new JSONObject();
                if (playerId.isEmpty()) playerId = "player_" + System.currentTimeMillis();
                body.put("player_id", playerId);
                body.put("npc_id", currentNpcId);  // npc后端需要
                sendRequest(conn, body);

                // 处理响应
                if (conn.getResponseCode() == HttpURLConnection.HTTP_OK) {
                    JSONObject resp = readResponse(conn);
                    if (resp.getBoolean("success")) {
                        sessionId = resp.optString("session_id", "");
                        String greeting = resp.optString("reply", resp.optString("message", ""));
                        showMessage(greeting);
                        Log.d(TAG, "连接成功: " + sessionId);
                    }
                } else {
                    useFallbackGreeting();
                }
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "连接异常: " + e.getMessage());
                useFallbackGreeting();
            }
        }).start();
    }

    /** 处理玩家消息 */
    public void handlePlayerMessage(String message) {
        if (!networkEnabled) {
            useFallbackResponse(message);
            return;
        }

        new Thread(() -> {
            try {
                URL url = new URL(serverUrl + DialogConfig.getChatEndpoint());
                HttpURLConnection conn = createConnection(url);

                // 统一请求格式
                JSONObject body = new JSONObject();
                body.put("message", message);
                body.put("session_id", sessionId);
                body.put("npc_id", currentNpcId);      // npc后端需要
                body.put("player_id", playerId);       // npc后端需要
                sendRequest(conn, body);

                // 处理响应
                if (conn.getResponseCode() == HttpURLConnection.HTTP_OK) {
                    JSONObject resp = readResponse(conn);
                    if (resp.getBoolean("success")) {
                        String reply = resp.optString("reply", "");
                        showMessage(reply);
                        Log.d(TAG, "收到回复: " + reply);
                    } else {
                        useFallbackResponse(message);
                    }
                } else {
                    useFallbackResponse(message);
                }
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "请求异常: " + e.getMessage());
                useFallbackResponse(message);
            }
        }).start();
    }

    /** 清除对话历史 */
    public void clearHistory() {
        new Thread(() -> {
            try {
                URL url = new URL(serverUrl + DialogConfig.getClearEndpoint());
                HttpURLConnection conn = createConnection(url);
                JSONObject body = new JSONObject();
                body.put("session_id", sessionId);
                sendRequest(conn, body);
                Log.d(TAG, "清除历史响应: " + conn.getResponseCode());
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "清除历史异常: " + e.getMessage());
            }
        }).start();
    }

    // ==================== 工具方法 ====================

    private HttpURLConnection createConnection(URL url) throws Exception {
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();
        conn.setRequestMethod("POST");
        conn.setRequestProperty("Content-Type", "application/json");
        conn.setDoOutput(true);
        conn.setConnectTimeout(DialogConfig.CONNECT_TIMEOUT);
        conn.setReadTimeout(DialogConfig.READ_TIMEOUT);
        return conn;
    }

    private void sendRequest(HttpURLConnection conn, JSONObject body) throws Exception {
        OutputStream os = conn.getOutputStream();
        os.write(body.toString().getBytes(StandardCharsets.UTF_8));
        os.close();
    }

    private JSONObject readResponse(HttpURLConnection conn) throws Exception {
        BufferedReader br = new BufferedReader(
                new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
        StringBuilder sb = new StringBuilder();
        String line;
        while ((line = br.readLine()) != null) sb.append(line);
        br.close();
        return new JSONObject(sb.toString());
    }

    private void showMessage(String msg) {
        if (msg == null || msg.isEmpty()) return;
        handler.post(() -> {
            if (mainActivity != null) mainActivity.addNpcMessage(msg);
        });
    }

    private void useFallbackGreeting() {
        handler.postDelayed(() -> {
            if (mainActivity != null) mainActivity.addNpcMessage(getRandomResponse(currentNpcName));
        }, 300);
    }

    private void useFallbackResponse(String message) {
        handler.postDelayed(() -> {
            if (mainActivity != null) mainActivity.addNpcMessage(generateResponse(message));
        }, 800);
    }

    private String generateResponse(String msg) {
        if (msg.contains("你好") || msg.contains("您好")) return "你好！";
        if (msg.contains("再见") || msg.contains("告辞")) return "江湖路远，后会有期。";
        return getRandomResponse(currentNpcName);
    }

    private String getRandomResponse(String name) {
        String[] responses = fallbackResponses.getOrDefault(name, fallbackResponses.get("default"));
        return responses[(int) (Math.random() * responses.length)];
    }

    public void endDialog() {
        currentNpcName = "未知NPC";
        currentNpcId = "";
        sessionId = "";
    }

    public String getCurrentNpcName() { return currentNpcName; }
    public String getCurrentNpcId() { return currentNpcId; }
}
