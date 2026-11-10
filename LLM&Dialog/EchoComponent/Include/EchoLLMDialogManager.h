/**
 * @file EchoLLMDialogManager.h
 * @brief LLM对话管理器 - 用于与Ollama对话服务器通信
 * 
 * 使用方式:
 * 1. 创建管理器实例
 * 2. 调用 Connect() 建立会话
 * 3. 调用 Chat() 发送消息并获取回复
 * 4. 可选: 调用 ClearHistory() 清除对话历史
 */

#pragma once

#include <string>
#include <functional>
#include <memory>

namespace Echo {

/**
 * @brief LLM对话响应结构
 */
struct LLMDialogResponse
{
    int success;           ///< 1=成功, 0=失败
    std::string msg;       ///< AI回复消息或错误信息
    std::string sessionId; ///< 会话ID (Connect时返回)
    
    LLMDialogResponse() : success(0) {}
};

/**
 * @brief 异步回调函数类型
 */
using LLMDialogCallback = std::function<void(const LLMDialogResponse&)>;

/**
 * @brief LLM对话管理器
 * 
 * 提供与Ollama对话服务器的HTTP通信接口
 */
class LLMDialogManager
{
public:
    LLMDialogManager();
    ~LLMDialogManager();
    
    /**
     * @brief 设置服务器地址
     * @param url 服务器URL, 例如 "http://192.168.5.189:8000"
     */
    void SetServerUrl(const std::string& url);
    
    /**
     * @brief 建立对话会话 (同步)
     * @return 响应结果, 包含sessionId和欢迎消息
     */
    LLMDialogResponse Connect();
    
    /**
     * @brief 建立对话会话 (异步)
     * @param callback 回调函数
     */
    void ConnectAsync(LLMDialogCallback callback);
    
    /**
     * @brief 发送消息并获取回复 (同步)
     * @param message 用户消息
     * @return 响应结果
     */
    LLMDialogResponse Chat(const std::string& message);
    
    /**
     * @brief 发送消息并获取回复 (异步)
     * @param message 用户消息
     * @param callback 回调函数
     */
    void ChatAsync(const std::string& message, LLMDialogCallback callback);
    
    /**
     * @brief 清除对话历史
     * @return 是否成功
     */
    bool ClearHistory();
    
    /**
     * @brief 获取当前会话ID
     */
    const std::string& GetSessionId() const { return m_sessionId; }
    
    /**
     * @brief 是否已连接
     */
    bool IsConnected() const { return !m_sessionId.empty(); }
    
private:
    std::string m_serverUrl;
    std::string m_sessionId;
    
    // HTTP请求实现 (需要根据项目的HTTP库实现)
    std::string HttpPost(const std::string& endpoint, 
                         const std::string& body,
                         const std::string& contentType = "text/plain");
    
    // 解析JSON响应
    LLMDialogResponse ParseJsonResponse(const std::string& json);
};

} // namespace Echo
