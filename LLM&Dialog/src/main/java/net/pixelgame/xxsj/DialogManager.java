package net.pixelgame.xxsj;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.json.JSONException;
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
 * 对话管理器
 * 支持两种模式：general（通用对话）和 npc（NPC对话）
 */
public class DialogManager {
    
    private static final String TAG = "DialogManager";
    
    // 对话模式
    public enum DialogMode {
        GENERAL,  // 通用对话模式
        NPC       // NPC对话模式
    }
    
    private static DialogManager instance;
    private MainActivity mainActivity;
    private Handler handler;
    
    // 当前对话模式（从配置读取）
    private DialogMode currentMode = DialogConfig.DEFAULT_MODE;
    
    // 是否启用网络请求（从配置读取）
    private boolean networkEnabled = DialogConfig.ENABLE_NETWORK;
    
    // Django后端地址（从配置读取）
    private String serverUrl = DialogConfig.SERVER_URL;
    
    // 当前对话的NPC信息
    private String currentNpcName = "未知NPC";
    private String currentNpcId = "";
    private String sessionId = "";
    private String playerId = "";
    
    // NPC预设回复（测试用，无网络时使用）
    private Map<String, String[]> npcResponses;
    
    private DialogManager() {
        handler = new Handler(Looper.getMainLooper());
        initNpcResponses();
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
    
    /**
     * 初始化NPC预设回复（本地模拟，与npc_configs/中的NPC对应）
     */
    private void initNpcResponses() {
        npcResponses = new HashMap<>();
        
        // blacksmith - 老铁匠
        npcResponses.put("老铁匠", new String[]{
            "哟，又来了个冒险者。看你的样子，是来买武器的吧？",
            "这把剑可是我精心打造的，削铁如泥！",
            "想要好兵器？拿银子来！",
            "你这装备该修理了，再不修就要坏了。",
            "我打铁30年，什么样的武器没见过？"
        });
        
        // merchant - 精明商人
        npcResponses.put("精明商人", new String[]{
            "欢迎欢迎！贵客临门啊！看看有什么需要的？保证全镇最低价！",
            "朋友，这个价格已经很实惠了，我可是亏本卖给你的！",
            "老板眼光真好，这可是从东方运来的稀罕货！",
            "要不要看看这个？绝对物超所值！"
        });
        
        // guard_captain - 守卫队长
        npcResponses.put("守卫队长", new String[]{
            "站住！报上名来！",
            "这里是禁区，闲人免进！",
            "有什么事快说，我还要巡逻呢。"
        });
        
        // herbalist - 草药师
        npcResponses.put("草药师", new String[]{
            "需要治疗吗？我这里有最好的草药。",
            "这种草药很稀有，要好好保存。",
            "受伤了？让我看看。"
        });
        
        // innkeeper - 客栈老板
        npcResponses.put("客栈老板", new String[]{
            "客官里面请！",
            "您需要点些什么？",
            "小店有上好的女儿红，要不要来一壶？",
            "要住店吗？我们这里干净又便宜！"
        });
    }
    
    /**
     * 设置对话模式
     */
    public void setDialogMode(DialogMode mode) {
        this.currentMode = mode;
        Log.d(TAG, "对话模式切换为: " + mode);
    }
    
    /**
     * 设置服务器地址
     */
    public void setServerUrl(String url) {
        this.serverUrl = url;
        Log.d(TAG, "服务器地址设置为: " + url);
    }
    
    /**
     * 设置是否启用网络
     */
    public void setNetworkEnabled(boolean enabled) {
        this.networkEnabled = enabled;
        Log.d(TAG, "网络请求" + (enabled ? "已启用" : "已禁用"));
    }
    
    /**
     * 开始对话（根据模式自动选择）
     */
    public void startDialog(String npcName, String npcId) {
        this.currentNpcName = npcName;
        this.currentNpcId = npcId;
        
        if (mainActivity != null) {
            mainActivity.setDialogNpcName(npcName);
            mainActivity.clearDialogHistory();
            
            // 连接到后端
            connectToBackend();
        }
    }
    
    /**
     * 连接到后端
     */
    private void connectToBackend() {
        // 如果禁用网络，直接使用本地问候语
        if (!networkEnabled) {
            useFallbackGreeting();
            return;
        }
        
        new Thread(() -> {
            try {
                String endpoint = currentMode == DialogMode.GENERAL ? 
                    "/general/connect/" : "/npc/connect/";
                
                URL url = new URL(serverUrl + endpoint);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json");
                conn.setDoOutput(true);
                conn.setConnectTimeout(DialogConfig.CONNECT_TIMEOUT);
                conn.setReadTimeout(DialogConfig.READ_TIMEOUT);
                
                // 构建请求体
                JSONObject requestBody = new JSONObject();
                if (playerId.isEmpty()) {
                    playerId = "player_" + System.currentTimeMillis();
                }
                requestBody.put("player_id", playerId);
                
                // 发送请求
                OutputStream os = conn.getOutputStream();
                os.write(requestBody.toString().getBytes(StandardCharsets.UTF_8));
                os.close();
                
                // 读取响应
                int responseCode = conn.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    BufferedReader br = new BufferedReader(
                        new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
                    StringBuilder response = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) {
                        response.append(line);
                    }
                    br.close();
                    
                    // 解析响应
                    JSONObject jsonResponse = new JSONObject(response.toString());
                    if (jsonResponse.getBoolean("success")) {
                        sessionId = jsonResponse.optString("session_id", "");
                        String greeting = jsonResponse.optString("message", "");
                        
                        // 在主线程显示问候语
                        handler.post(() -> {
                            if (mainActivity != null && !greeting.isEmpty()) {
                                mainActivity.addNpcMessage(greeting);
                            }
                        });
                        
                        Log.d(TAG, "连接成功: " + sessionId);
                    }
                } else {
                    Log.e(TAG, "连接失败: " + responseCode);
                    // 使用本地模拟
                    useFallbackGreeting();
                }
                
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "连接异常: " + e.getMessage());
                e.printStackTrace();
                // 使用本地模拟
                useFallbackGreeting();
            }
        }).start();
    }
    
    /**
     * 使用本地问候语（后备方案）
     */
    private void useFallbackGreeting() {
        handler.postDelayed(() -> {
            String greeting = getRandomResponse(currentNpcName);
            if (mainActivity != null) {
                mainActivity.addNpcMessage(greeting);
            }
        }, 300);
    }
    
    /**
     * 处理玩家消息
     */
    public void handlePlayerMessage(String message) {
        // 显示加载提示
        handler.post(() -> {
            if (mainActivity != null) {
                mainActivity.showLoadingMessage();
            }
        });
        
        // 如果禁用网络，直接使用本地回复
        if (!networkEnabled) {
            useFallbackResponse(message);
            return;
        }
        
        new Thread(() -> {
            try {
                String endpoint;
                JSONObject requestBody = new JSONObject();
                
                if (currentMode == DialogMode.GENERAL) {
                    // 通用对话模式
                    endpoint = "/general/chat/";
                    requestBody.put("message", message);
                    requestBody.put("session_id", sessionId);
                } else {
                    // NPC对话模式
                    endpoint = "/npc/chat/";
                    requestBody.put("npc_id", currentNpcId);
                    requestBody.put("message", message);
                    requestBody.put("player_id", playerId);
                    requestBody.put("session_id", sessionId);
                }
                
                URL url = new URL(serverUrl + endpoint);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json");
                conn.setDoOutput(true);
                conn.setConnectTimeout(DialogConfig.CONNECT_TIMEOUT);
                conn.setReadTimeout(DialogConfig.READ_TIMEOUT);
                
                // 发送请求
                OutputStream os = conn.getOutputStream();
                os.write(requestBody.toString().getBytes(StandardCharsets.UTF_8));
                os.close();
                
                // 读取响应
                int responseCode = conn.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    BufferedReader br = new BufferedReader(
                        new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
                    StringBuilder response = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) {
                        response.append(line);
                    }
                    br.close();
                    
                    // 解析响应
                    JSONObject jsonResponse = new JSONObject(response.toString());
                    if (jsonResponse.getBoolean("success")) {
                        String reply = jsonResponse.optString("reply", "");
                        
                        // 在主线程隐藏加载提示并显示回复
                        handler.post(() -> {
                            if (mainActivity != null) {
                                mainActivity.hideLoadingMessage();
                                if (!reply.isEmpty()) {
                                    mainActivity.addNpcMessage(reply);
                                }
                            }
                        });
                        
                        Log.d(TAG, "收到回复: " + reply);
                    } else {
                        String error = jsonResponse.optString("error", "未知错误");
                        Log.e(TAG, "对话失败: " + error);
                        handler.post(() -> {
                            if (mainActivity != null) {
                                mainActivity.hideLoadingMessage();
                            }
                        });
                        useFallbackResponse(message);
                    }
                } else {
                    Log.e(TAG, "请求失败: " + responseCode);
                    handler.post(() -> {
                        if (mainActivity != null) {
                            mainActivity.hideLoadingMessage();
                        }
                    });
                    useFallbackResponse(message);
                }
                
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "请求异常: " + e.getMessage());
                e.printStackTrace();
                handler.post(() -> {
                    if (mainActivity != null) {
                        mainActivity.hideLoadingMessage();
                    }
                });
                useFallbackResponse(message);
            }
        }).start();
    }
    
    /**
     * 使用本地回复（后备方案）
     */
    private void useFallbackResponse(String message) {
        handler.postDelayed(() -> {
            String response = generateResponse(message);
            if (mainActivity != null) {
                mainActivity.addNpcMessage(response);
            }
        }, 800);
    }
    
    /**
     * 生成NPC回复（模拟）
     */
    private String generateResponse(String playerMessage) {
        // 简单的关键词匹配
        if (playerMessage.contains("你好") || playerMessage.contains("您好")) {
            return "见过" + currentNpcName + "。";
        } else if (playerMessage.contains("再见") || playerMessage.contains("告辞")) {
            return "江湖路远，后会有期。";
        } else if (playerMessage.contains("名字") || playerMessage.contains("何人")) {
            return "在下" + currentNpcName + "，久仰大名。";
        } else {
            return getRandomResponse(currentNpcName);
        }
    }
    
    /**
     * 获取随机回复
     */
    private String getRandomResponse(String npcName) {
        String[] responses = npcResponses.get(npcName);
        if (responses == null || responses.length == 0) {
            return "......";
        }
        int index = (int) (Math.random() * responses.length);
        return responses[index];
    }
    
    /**
     * 清除对话历史
     */
    public void clearHistory() {
        new Thread(() -> {
            try {
                String endpoint = currentMode == DialogMode.GENERAL ? 
                    "/general/clear/" : "/npc/clear/";
                
                URL url = new URL(serverUrl + endpoint);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json");
                conn.setDoOutput(true);
                conn.setConnectTimeout(DialogConfig.CONNECT_TIMEOUT);
                conn.setReadTimeout(DialogConfig.READ_TIMEOUT);
                
                JSONObject requestBody = new JSONObject();
                requestBody.put("session_id", sessionId);
                
                OutputStream os = conn.getOutputStream();
                os.write(requestBody.toString().getBytes(StandardCharsets.UTF_8));
                os.close();
                
                int responseCode = conn.getResponseCode();
                Log.d(TAG, "清除历史响应: " + responseCode);
                
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "清除历史异常: " + e.getMessage());
            }
        }).start();
    }
    
    /**
     * 结束对话
     */
    public void endDialog() {
        currentNpcName = "未知NPC";
        currentNpcId = "";
        sessionId = "";
    }
    
    public String getCurrentNpcName() {
        return currentNpcName;
    }
    
    public String getCurrentNpcId() {
        return currentNpcId;
    }
    
    public DialogMode getCurrentMode() {
        return currentMode;
    }
}
