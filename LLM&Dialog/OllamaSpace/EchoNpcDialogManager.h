#pragma once

#include <string>
#include <vector>
#include "EchoRoot.h"
#include "EchoLogManager.h"
#include "Actor/EchoActorSystem.h"

// 前向声明
namespace Echo
{
	class Camera;
}

// --------------------------------- NPC 对话接近检测系统 --------------------------------- //

/// 单个NPC的信息
struct NpcEntry
{
	std::string     npcId;          // 后端NPC标识 (如 "blacksmith", "guard_captain")
	std::string     npcName;        // 显示名称 (如 "老铁匠", "守卫")
	std::string     actPath;        // actor资源路径 (如 "actors/car_005.act")
	Echo::ActorPtr  actor;
	Echo::DVector3  position;
	float           triggerDist = 200.0f;
	bool            inRange = false;
};

/// NPC 接近检测管理器
/// 每帧通过 FrameListener::frameStarted() 检测玩家（相机）与所有NPC的距离，
/// 记录最近且在触发范围内的NPC，供 Java 层通过 JNI getGameData() 查询。
///
/// 用法：
///   1. addNpc() 注册NPC
///   2. activate(camera) 开始每帧检测
///   3. getGameData() 获取最近NPC信息 → "npc_id|npc_name" 或 ""
///   4. destroy() 清理
class NpcProximityManager : public Echo::FrameListener
{
public:
	NpcProximityManager() = default;
	~NpcProximityManager() { destroy(); }

	/// 添加一个NPC
	void addNpc(const std::string& npcId, const std::string& npcName,
	            const std::string& actPath, const Echo::DVector3& pos, float dist = 200.0f);

	/// 注册FrameListener，开始每帧检测
	/// @param camera 用于获取玩家位置的相机指针
	void activate(Echo::Camera* camera);

	/// 停止检测并清理所有NPC
	void destroy();

	/// 每帧检测回调（由引擎自动调用）
	bool frameStarted(const Echo::FrameEvent& evt) override;

	/// 获取当前最近NPC的信息
	/// @return "npc_id|npc_name" 或 "" (无NPC在范围内)
	std::string getGameData() const;

	/// 获取当前NPC总数
	int getNpcCount() const { return (int)m_Npcs.size(); }

	/// 是否已激活
	bool isActive() const { return m_Active; }

private:
	std::vector<NpcEntry>  m_Npcs;
	int                    m_NearestIndex = -1;   // 当前最近的NPC索引，-1表示无
	bool                   m_Active = false;
	Echo::Camera*          m_Camera = nullptr;    // 相机引用（获取玩家位置）
};
