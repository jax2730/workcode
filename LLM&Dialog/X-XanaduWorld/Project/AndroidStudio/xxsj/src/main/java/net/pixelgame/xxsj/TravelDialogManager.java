package net.pixelgame.xxsj;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

public class TravelDialogManager {

    private static final String TAG = "TravelDialogManager";

    private static TravelDialogManager instance;
    private MainActivity mainActivity;
    private final Handler handler;

    private String serverUrl = DialogConfig.SERVER_URL;
    private boolean networkEnabled = DialogConfig.ENABLE_NETWORK;
    private String sessionId = "";
    private String assistantName = DialogConfig.TRAVEL_ASSISTANT_NAME;
    private boolean hasShownWelcomeMessage = false;

    public static class TravelContext {
        public final Double x;
        public final Double y;
        public final String state;
        public final String city;
        public final String county;
        public final String areaName;
        public final Double temperature;
        public final String weatherName;
        public final String weatherDescription;
        public final String weatherSource;
        public final boolean weatherPending;

        public TravelContext(Double x, Double y, String state, String city, String county, String areaName,
                             Double temperature, String weatherName, String weatherDescription,
                             String weatherSource, boolean weatherPending) {
            this.x = x;
            this.y = y;
            this.state = state;
            this.city = city;
            this.county = county;
            this.areaName = areaName;
            this.temperature = temperature;
            this.weatherName = weatherName;
            this.weatherDescription = weatherDescription;
            this.weatherSource = weatherSource;
            this.weatherPending = weatherPending;
        }
    }

    private TravelDialogManager() {
        handler = new Handler(Looper.getMainLooper());
    }

    public static TravelDialogManager getInstance() {
        if (instance == null) {
            instance = new TravelDialogManager();
        }
        return instance;
    }

    public void setMainActivity(MainActivity activity) {
        this.mainActivity = activity;
    }

    public void setServerUrl(String url) {
        this.serverUrl = url;
    }

    public void setNetworkEnabled(boolean enabled) {
        this.networkEnabled = enabled;
    }

    public void startDialog() {
        startDialog(null);
    }

    public void startDialog(TravelContext context) {
        if (mainActivity != null) {
            mainActivity.setTravelDialogAssistantName(assistantName);
            if (!hasShownWelcomeMessage) {
                mainActivity.addTravelAssistantMessage(buildStartDialogMessage(context));
                hasShownWelcomeMessage = true;
            }
        }
    }

    private String buildStartDialogMessage(TravelContext context) {
        String base = "你好，我是小助手。";
        if (context == null) {
            return base + "输入你的需求，我会结合当前位置和天气给你相关信息和建议。";
        }

        String areaText = firstNonEmpty(context.areaName, context.county, context.city, context.state);

        String weatherText = "";
        if (context.weatherDescription != null && !context.weatherDescription.isEmpty()) {
            weatherText = context.weatherDescription;
        } else if (context.weatherName != null && !context.weatherName.isEmpty()) {
            weatherText = context.weatherName;
        }

        String temperatureText = context.temperature != null ? String.format("%.1f℃", context.temperature) : "";

        StringBuilder builder = new StringBuilder(base);
        if (areaText != null && !areaText.isEmpty()) {
            builder.append("当前地区：").append(areaText).append("。 ");
        }

        if (!weatherText.isEmpty() || !temperatureText.isEmpty()) {
            builder.append("天气：");
            if (!weatherText.isEmpty()) {
                builder.append(weatherText);
            }
            if (!temperatureText.isEmpty()) {
                if (!weatherText.isEmpty()) {
                    builder.append("，");
                }
                builder.append(temperatureText);
            }
            builder.append("。 ");
        }

        builder.append("输入你的需求，我会结合当前位置和天气给你相关信息和建议。");
        return builder.toString();
    }

    private String firstNonEmpty(String... candidates) {
        if (candidates == null) return "";
        for (String candidate : candidates) {
            if (candidate != null && !candidate.trim().isEmpty()) {
                return candidate.trim();
            }
        }
        return "";
    }

    public void handlePlayerMessage(String message, TravelContext context) {
        if (!networkEnabled) {
            showMessage("当前是离线模式，无法获取旅行建议。请开启网络后重试。");
            return;
        }

        new Thread(() -> {
            try {
                URL url = new URL(serverUrl + DialogConfig.getTravelChatEndpoint());
                HttpURLConnection conn = createConnection(url);

                JSONObject body = new JSONObject();
                body.put("message", message);
                body.put("debug", DialogConfig.TRAVEL_DEBUG);
                if (!sessionId.isEmpty()) {
                    body.put("session_id", sessionId);
                }

                if (context != null) {
                    if (context.x != null) body.put("x", context.x);
                    if (context.y != null) body.put("y", context.y);
                    if (context.state != null && !context.state.isEmpty()) body.put("state", context.state);
                    if (context.city != null && !context.city.isEmpty()) body.put("city", context.city);
                    if (context.county != null && !context.county.isEmpty()) body.put("county", context.county);
                    if (context.areaName != null && !context.areaName.isEmpty()) body.put("area_name", context.areaName);
                    if (context.temperature != null) body.put("temperature", context.temperature);
                    if (context.weatherName != null && !context.weatherName.isEmpty()) body.put("weather_name", context.weatherName);
                    if (context.weatherDescription != null && !context.weatherDescription.isEmpty()) {
                        body.put("weather_description", context.weatherDescription);
                        body.put("weather_desc", context.weatherDescription);
                    }
                    if (context.weatherSource != null && !context.weatherSource.isEmpty()) {
                        body.put("weather_source", context.weatherSource);
                    }
                    body.put("weather_pending", context.weatherPending);

                    boolean hasAnyWeather = context.temperature != null
                            || (context.weatherName != null && !context.weatherName.isEmpty())
                            || (context.weatherDescription != null && !context.weatherDescription.isEmpty());
                    if (!hasAnyWeather) {
                        Log.w(TAG, "travel request has no weather context (name/desc/temp all empty)");
                    }

                    Log.i("TravelWeather", "travel request weather fields: source=" + context.weatherSource
                            + ", pending=" + context.weatherPending
                            + ", name=" + context.weatherName
                            + ", desc=" + context.weatherDescription
                            + ", temp=" + context.temperature);
                    Log.i("TravelWeather", "travel request location fields: state=" + context.state
                            + ", city=" + context.city
                            + ", county=" + context.county
                            + ", area_name=" + context.areaName);
                }

                Log.d(TAG, "travel request body=" + body);

                sendRequest(conn, body);

                int responseCode = conn.getResponseCode();
                String responseText = readResponseText(conn);
                Log.d(TAG, "travel response code=" + responseCode + ", body=" + responseText);

                if (responseCode == HttpURLConnection.HTTP_OK) {
                    JSONObject resp = new JSONObject(responseText);
                    if (resp.optBoolean("success", false)) {
                        sessionId = resp.optString("session_id", sessionId);
                        showMessage(resp.optString("reply", "已收到，但未返回可读文本。"));
                    } else {
                        String errorCode = resp.optString("error_code", "UNKNOWN_ERROR");
                        String errorMessage = resp.optString("error", "旅行服务返回失败");
                        showMessage("错误(" + errorCode + "): " + errorMessage);
                    }
                } else {
                    String errorCode = "HTTP_" + responseCode;
                    String errorMessage = responseText;
                    try {
                        JSONObject errorJson = new JSONObject(responseText);
                        errorCode = errorJson.optString("error_code", errorCode);
                        errorMessage = errorJson.optString("error", responseText);
                    } catch (Exception ignored) {
                    }
                    showMessage("错误(" + errorCode + "): " + errorMessage);
                }
                conn.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "travel chat error: " + e.getMessage());
                String errorText = e.getMessage() == null ? "unknown" : e.getMessage();
                if (errorText.toLowerCase().contains("timeout")) {
                    showMessage("错误(NETWORK_TIMEOUT): 请求超时，请稍后重试。\n如需大范围检索，可再次发送“扩大范围”。");
                } else {
                    showMessage("错误(NETWORK_ERROR): " + errorText);
                }
            }
        }).start();
    }

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

    private String readResponseText(HttpURLConnection conn) throws Exception {
        BufferedReader br;
        InputStream stream;
        if (conn.getResponseCode() >= HttpURLConnection.HTTP_BAD_REQUEST) {
            stream = conn.getErrorStream();
        } else {
            stream = conn.getInputStream();
        }

        if (stream == null) {
            stream = new ByteArrayInputStream(new byte[0]);
        }

        br = new BufferedReader(new InputStreamReader(stream, StandardCharsets.UTF_8));
        StringBuilder sb = new StringBuilder();
        String line;
        while ((line = br.readLine()) != null) {
            sb.append(line);
        }
        br.close();
        return sb.toString();
    }

    private void showMessage(String msg) {
        if (msg == null || msg.isEmpty()) return;
        handler.post(() -> {
            if (mainActivity != null) mainActivity.addTravelAssistantMessage(msg);
        });
    }
}
