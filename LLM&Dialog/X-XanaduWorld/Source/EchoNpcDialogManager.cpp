#include "EchoNpcDialogManager.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActor.h"
#include <cmath>



void NpcProximityManager::addNpc(const std::string& npcId, const std::string& npcName,
                                  const std::string& actPath, const Echo::DVector3& pos, float dist)
{
	NpcEntry entry;
	entry.npcId = npcId;
	entry.npcName = npcName;
	entry.actPath = actPath;
	entry.position = pos;
	entry.triggerDist = dist;
	entry.inRange = false;

	// 创建actor
	entry.actor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(
		Echo::Name(actPath.c_str()), "", true);
	if (!entry.actor.Expired())
	{
		entry.actor->setPosition(pos);
		entry.actor->setVisiable(true);
		Echo::LogManager::instance()->logMessage(
			std::string("[NpcDialog] Actor loaded OK: ") + npcId + " (" + actPath + ")");
	}
	else
	{
		Echo::LogManager::instance()->logMessage(
			std::string("[NpcDialog] Actor load FAILED: ") + npcId + " (" + actPath + "), using position only.");
	}

	m_Npcs.push_back(std::move(entry));
}

void NpcProximityManager::activate(Echo::Camera* camera)
{
	if (!m_Active && !m_Npcs.empty() && camera)
	{
		m_Camera = camera;
		Echo::Root::instance()->addFrameListener(this);
		m_Active = true;
		Echo::LogManager::instance()->logMessage(
			"[NpcDialog] NpcProximityManager activated with " + std::to_string(m_Npcs.size()) + " NPCs.");
	}
}

void NpcProximityManager::destroy()
{
	if (m_Active)
	{
		Echo::Root::instance()->removeFrameListener(this);
		m_Active = false;
	}
	m_Npcs.clear();
	m_NearestIndex = -1;
	m_Camera = nullptr;
}

bool NpcProximityManager::frameStarted(const Echo::FrameEvent& evt)
{
	if (!m_Camera) return true;

	const Echo::DVector3& cam = m_Camera->getDerivedPosition();

	int newNearest = -1;
	double nearestDistSq = 1e30;

	for (size_t i = 0; i < m_Npcs.size(); ++i)
	{
		NpcEntry& npc = m_Npcs[i];
		double dx = cam.x - npc.position.x;
		double dz = cam.z - npc.position.z;
		double distSq = dx * dx + dz * dz;
		double threshSq = (double)npc.triggerDist * npc.triggerDist;

		bool wasInRange = npc.inRange;
		npc.inRange = (distSq <= threshSq);

		if (npc.inRange != wasInRange)
		{
			Echo::LogManager::instance()->logMessage(
				std::string("[NpcDialog] ") + npc.npcId + " "
				+ (npc.inRange ? "ENTER" : "LEAVE")
				+ " dist=" + std::to_string(std::sqrt(distSq)));
		}

		if (npc.inRange && distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			newNearest = (int)i;
		}
	}

	if (newNearest != m_NearestIndex)
	{
		if (newNearest >= 0)
		{
			Echo::LogManager::instance()->logMessage(
				std::string("[NpcDialog] Nearest NPC changed to: ") + m_Npcs[newNearest].npcId);
		}
		else
		{
			Echo::LogManager::instance()->logMessage("[NpcDialog] No NPC in range.");
		}
		m_NearestIndex = newNearest;
	}
	return true;
}

std::string NpcProximityManager::getGameData() const
{
	if (m_NearestIndex >= 0 && m_NearestIndex < (int)m_Npcs.size())
	{
		const NpcEntry& npc = m_Npcs[m_NearestIndex];
		return npc.npcId + "|" + npc.npcName;
	}
	return "";
}
