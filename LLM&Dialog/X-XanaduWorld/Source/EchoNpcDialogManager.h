#pragma once

#include <string>
#include <vector>
#include "EchoRoot.h"
#include "EchoLogManager.h"
#include "Actor/EchoActorSystem.h"
namespace Echo
{
	class Camera;
}


struct NpcEntry
{
	std::string     npcId;          // 后端NPC标识guard_captain)
	std::string     npcName;        
	std::string     actPath;        
	Echo::ActorPtr  actor;
	Echo::DVector3  position;
	float           triggerDist = 200.0f;
	bool            inRange = false;
};


class NpcProximityManager : public Echo::FrameListener
{
public:
	NpcProximityManager() = default;
	~NpcProximityManager() { destroy(); }

	/// 添加NPC
	void addNpc(const std::string& npcId, const std::string& npcName,
	            const std::string& actPath, const Echo::DVector3& pos, float dist = 200.0f);

	void activate(Echo::Camera* camera);


	void destroy();


	bool frameStarted(const Echo::FrameEvent& evt) override;

	
	std::string getGameData() const;

	int getNpcCount() const { return (int)m_Npcs.size(); }


	bool isActive() const { return m_Active; }

private:
	std::vector<NpcEntry>  m_Npcs;
	int                    m_NearestIndex = -1;   // 当前最近的NPC索引，-1表示无
	bool                   m_Active = false;
	Echo::Camera*          m_Camera = nullptr;    
};
