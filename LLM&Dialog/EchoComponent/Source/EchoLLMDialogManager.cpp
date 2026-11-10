/**
 * @file EchoLLMDialogManager.cpp
 * @brief LLM对话管理器实现
 */

#include "EchoLLMDialogManager.h"

// TODO: 根据项目实际使用的HTTP库替换
// 可选方案:
// 1. libcurl
// 2. WinHTTP (Windows)
// 3. Boost.Beast
// 4. cpp-httplib (单头文件库, 推荐)

#include <sstream>
#include <iostream>

// 如果使用 cpp-httplib, 取消下面的注释
// #define CPPHTTPLIB_OPENSSL_SUPPORT
// #include "httplib.h"

// 简单的JSON解析 (生产环境建议使用 nlohmann/json 或 rapidjson)
namespace {
    std::string extractJsonString(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";
        
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return "";
        
        size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
        if (valueStart == std::string::npos) return "";
        
        if (json[valueStart] == '"') {
            // 字符串值
            size_t valueEnd = json.find('"', valueStart + 1);
            while (valueEnd != std::string::npos && json[valueEnd - 1] == '\\') {
                valueEnd = json.find('"', valueEnd + 1);
            }
            if (valueEnd != std::string::npos) {
                return json.substr(valueStart + 1, valueEnd - valueStart - 1);
            }
        }
        return "";
    }
    
    int extractJsonInt(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return 0;
        
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return 0;
        
        size_t valueStart = json.find_first_of("0123456789-", colonPos + 1);
        if (valueStart == std::string::npos) return 0;
        
        size_t valueEnd = json.find_first_not_of("0123456789", valueStart + 1);
        std::string numStr = json.substr(valueStart, valueEnd - valueStart);
        
        return std::atoi(numStr.c_str());
    }
}

namespace Echo {

LLMDialogManager::LLMDialogManager()
    : m_serverUrl("http://192.168.5.189:8000")  // 默认服务器地址
{
}

LLMDialogManager::~LLMDialogManager()
{
}

void LLMDialogManager::SetServerUrl(const std::string& url)
{
    m_serverUrl = url;
    // 移除末尾的斜杠
    if (!m_serverUrl.empty() && m_serverUrl.back() == '/') {
        m_serverUrl.pop_back();
    }
}

LLMDialogResponse LLMDialogManager::Connect()
{
    LLMDialogResponse response;
    
    try {
        std::string result = HttpPost("/general/connect/", "");
        response = ParseJsonResponse(result);
        
        if (response.success) {
            m_sessionId = response.sessionId;
        }
    }
    catch (const std::exception& e) {
        response.success = 0;
        response.msg = std::string("连接失败: ") + e.what();
    }
    
    return response;
}

void LLMDialogManager::ConnectAsync(LLMDialogCallback callback)
{
    // TODO: 使用线程池或异步IO实现
    // 简单实现: 创建新线程
    // std::thread([this, callback]() {
    //     LLMDialogResponse response = Connect();
    //     callback(response);
    // }).detach();
    
    // 临时同步实现
    if (callback) {
        callback(Connect());
    }
}

LLMDialogResponse LLMDialogManager::Chat(const std::string& message)
{
    LLMDialogResponse response;
    
    if (m_sessionId.empty()) {
        response.success = 0;
        response.msg = "未连接, 请先调用 Connect()";
        return response;
    }
    
    try {
        std::string result = HttpPost("/general/chat/", message);
        response = ParseJsonResponse(result);
    }
    catch (const std::exception& e) {
        response.success = 0;
        response.msg = std::string("请求失败: ") + e.what();
    }
    
    return response;
}

void LLMDialogManager::ChatAsync(const std::string& message, LLMDialogCallback callback)
{
    // TODO: 使用线程池或异步IO实现
    if (callback) {
        callback(Chat(message));
    }
}

bool LLMDialogManager::ClearHistory()
{
    if (m_sessionId.empty()) {
        return false;
    }
    
    try {
        std::string result = HttpPost("/general/clear/", "");
        LLMDialogResponse response = ParseJsonResponse(result);
        return response.success == 1;
    }
    catch (...) {
        return false;
    }
}

std::string LLMDialogManager::HttpPost(const std::string& endpoint, 
                                        const std::string& body,
                                        const std::string& contentType)
{
    // ============================================================
    // TODO: 根据项目使用的HTTP库实现此函数
    // ============================================================
    
    // 示例1: 使用 cpp-httplib (推荐, 单头文件)
    // ----------------------------------------------------------
    /*
    #include "httplib.h"
    
    // 解析URL
    std::string host = m_serverUrl;
    int port = 8000;
    
    // 移除 http://
    if (host.find("http://") == 0) {
        host = host.substr(7);
    }
    
    // 提取端口
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    }
    
    httplib::Client client(host, port);
    client.set_connection_timeout(10);  // 10秒超时
    
    httplib::Headers headers;
    if (!m_sessionId.empty()) {
        headers.emplace("Cookie", "general_session_id=" + m_sessionId);
    }
    
    auto res = client.Post(endpoint.c_str(), headers, body, contentType.c_str());
    
    if (res && res->status == 200) {
        // 从Set-Cookie中提取session_id
        if (res->has_header("Set-Cookie")) {
            std::string cookie = res->get_header_value("Set-Cookie");
            // 解析 general_session_id=xxx
            size_t pos = cookie.find("general_session_id=");
            if (pos != std::string::npos) {
                size_t start = pos + 19;
                size_t end = cookie.find(';', start);
                m_sessionId = cookie.substr(start, end - start);
            }
        }
        return res->body;
    }
    
    throw std::runtime_error("HTTP请求失败");
    */
    
    // 示例2: 使用 WinHTTP (Windows)
    // ----------------------------------------------------------
    /*
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
    
    // ... WinHTTP 实现代码 ...
    */
    
    // 示例3: 使用 libcurl
    // ----------------------------------------------------------
    /*
    #include <curl/curl.h>
    
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string url = m_serverUrl + endpoint;
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        
        // 设置Cookie
        if (!m_sessionId.empty()) {
            std::string cookie = "general_session_id=" + m_sessionId;
            curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
        }
        
        // 回调函数写入response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            return response;
        }
    }
    throw std::runtime_error("CURL请求失败");
    */
    
    // 临时占位实现 - 实际使用时请替换
    std::cerr << "[LLMDialogManager] HttpPost 未实现!" << std::endl;
    std::cerr << "  URL: " << m_serverUrl << endpoint << std::endl;
    std::cerr << "  Body: " << body << std::endl;
    
    throw std::runtime_error("HttpPost 未实现, 请根据项目HTTP库完成实现");
}

LLMDialogResponse LLMDialogManager::ParseJsonResponse(const std::string& json)
{
    LLMDialogResponse response;
    
    response.success = extractJsonInt(json, "success");
    response.msg = extractJsonString(json, "msg");
    response.sessionId = extractJsonString(json, "session_id");
    
    return response;
}

} // namespace Echo


// ============================================================
// 使用示例
// ============================================================
/*
#include "EchoLLMDialogManager.h"
#include <iostream>

int main() {
    Echo::LLMDialogManager manager;
    manager.SetServerUrl("http://192.168.5.189:8000");
    
    // 1. 连接
    auto connectResult = manager.Connect();
    if (!connectResult.success) {
        std::cerr << "连接失败: " << connectResult.msg << std::endl;
        return 1;
    }
    std::cout << "AI: " << connectResult.msg << std::endl;
    
    // 2. 对话循环
    std::string input;
    while (true) {
        std::cout << "你: ";
        std::getline(std::cin, input);
        
        if (input == "quit") break;
        if (input == "clear") {
            manager.ClearHistory();
            std::cout << "历史已清除" << std::endl;
            continue;
        }
        
        auto chatResult = manager.Chat(input);
        if (chatResult.success) {
            std::cout << "AI: " << chatResult.msg << std::endl;
        } else {
            std::cerr << "错误: " << chatResult.msg << std::endl;
        }
    }
    
    return 0;
}
*/
