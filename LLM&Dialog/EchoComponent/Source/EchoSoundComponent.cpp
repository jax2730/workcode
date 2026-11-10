//-----------------------------------------------------------------------------
// File:  EchoSoundComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-8-31
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoSoundComponent.h"
#include "EchoName.h"
#include "EchoBellEvent.h"
#include "EchoBellSystem.h"
#include "EchoActorProFuncImp.h"
namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(SoundComponent)

		EDITOR_REFLECT_BEGIN(SoundComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(BankEvent.first , "bank_name", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(BankEvent.second, "event_name", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_bLoop, "loop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_bMute, "mute", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Volume, "volume", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	SoundComponentInfo::SoundComponentInfo(const Name& type, ActorCOMMoveableType moveable) :
		ActorComponentInfo(type, moveable)
		, m_bLoop(false)
		, m_bMute(false)
	{
	}

	SoundComponentInfo::~SoundComponentInfo()
	{
	}
	void SoundComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SoundComponentInfo>::get()->ExportJson(this, jsonNode, "");
		/*if (!jsonNode)
			return;
		cJSON_AddStringToObject(jsonNode, "bank_name", m_strBank_name.c_str());
		cJSON_AddStringToObject(jsonNode, "event_name", m_strEvent_name.c_str());
		cJSON_AddNumberToObject(jsonNode, "volume", m_Volume);
		cJSON_AddBoolToObject(jsonNode, "loop", m_bLoop);
		cJSON_AddBoolToObject(jsonNode, "mute", m_bMute);*/
	}

	void SoundComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SoundComponentInfo>::get()->ImportJson(this, jsonNode, "");
		/*if (!jsonNode)
			return;

		ReadJSNode(jsonNode, "bank_name", m_strBank_name);
		ReadJSNode(jsonNode, "event_name", m_strEvent_name);
		ReadJSNode(jsonNode, "volume", m_Volume);
		ReadJSNode(jsonNode, "loop", m_bLoop);
		ReadJSNode(jsonNode, "mute", m_bMute);*/
	}

	void SoundComponentInfo::CopyTo(ActorComponentInfo* p_actor_com_info)
	{
		SoundComponentInfo* p_target_com_info = static_cast<SoundComponentInfo*>(p_actor_com_info);
		reflect::TypeResolver<SoundComponentInfo>::get()->copyTo(this, p_target_com_info);
		/*p_target_com_info->m_strBank_name = this->m_strBank_name;
		p_target_com_info->m_strEvent_name = this->m_strEvent_name;
		p_target_com_info->m_Volume = this->m_Volume;
		p_target_com_info->m_bLoop = this->m_bLoop;
		p_target_com_info->m_bMute = this->m_bMute;*/
	}

	void SoundComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
		/*if (comInfo == nullptr || comInfo->m_Type != m_Type)
			return;
		SoundComponentInfo* pInfo = static_cast<SoundComponentInfo*> (comInfo);
		OVERLAY_PROP(this, pInfo, Volume);*/
	}

	void SoundComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			SoundComponentInfo* oriInfo = static_cast<SoundComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<SoundComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	SoundComponent::SoundComponent(Echo::ActorPtr pActor, ActorCOMMoveableType moveable):
		ActorComponent(pActor, moveable)
	{

	}

	SoundComponent::~SoundComponent()
	{
	}

	void SoundComponent::init(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		SoundComponentInfo* mInfo = static_cast<SoundComponentInfo*> (m_pActComInfo);
		if (info == nullptr)
			return;
		if (!(mInfo->BankEvent.first.empty() || mInfo->BankEvent.second.empty()))
		{
			loadSound(mInfo->BankEvent);
			if (mInfo->m_Volume != m_pEvent->getVolume())
				SetVolume(mInfo->m_Volume);
			if (mInfo->m_bLoop)
				SetMusicLoop();
			if (mInfo->m_bMute)
				SetMusicMute();
		}
	}

	void SoundComponent::unInit()
	{
		removeSound();
	}

	void SoundComponent::replay(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		SoundComponentInfo* mInfo = static_cast<SoundComponentInfo*> (m_pActComInfo);
		if (info == nullptr)
			return;
		if ( (!(mInfo->BankEvent.first.empty() || mInfo->BankEvent.second.empty())))
			reloadSound(mInfo->BankEvent);
	}

	void SoundComponent::copyTo(const Echo::ActorComponentPtr& p_actor_com)
	{
	}

	Name musicPlayName_ = Name("MusicPlay");
	void SoundComponent::setTimelineAttribute(std::vector<float>& data,const Name& name)
	{
		if (name == musicPlayName_)
		{
			if (data[0] == 1)
				playSound();
			else
				pause();
		}
	}

	const std::string& SoundComponent::getBankName()
	{
		return BankEvent.first;
	}

	const std::string& SoundComponent::getEventName()
	{
		return BankEvent.second;
	}

	void SoundComponent::loadSound(const std::string & m_strBank_name, const std::string & m_strEvent_name)
	{
		if (m_strBank_name.empty() || m_strEvent_name.empty())
			return;
		loadSound(std::pair(m_strBank_name, m_strEvent_name));
	}

	void SoundComponent::loadSound(const std::pair<std::string, std::string>& _BankEvent)
	{
		if (_BankEvent.first.empty() || _BankEvent.second.empty())
			return;

		BankEvent.first = _BankEvent.first;
		BankEvent.second = _BankEvent.second;

		m_pEvent = BellSystem::instance()->createEvent(BankEvent.first.c_str(), BankEvent.second.c_str());
		m_pEvent->init();
		if (m_pEvent->isVisible())
		{
			Vector3 pos = BellSystem::instance()->getLisenterPosition();
			m_pEvent->set3DAttributes(&pos, 0, 0, 0);
		}
		else
		{
			removeSound();
		}
	}

	void SoundComponent::reloadSound(const std::string & m_strBank_name, const std::string & m_strEvent_name)
	{
		removeSound();
		loadSound(m_strBank_name,m_strEvent_name);
	}

	void SoundComponent::reloadSound(const std::pair<std::string, std::string>& BankEvent)
	{
		removeSound();
		loadSound(BankEvent);
	}

	void SoundComponent::removeSound()
	{
		if (!m_pEvent)
			return;
		BellSystem::instance()->destroyEvent(m_pEvent);
		m_pEvent = NULL;
	}

	void SoundComponent::EditorPlayImp()
	{
		playSound();
	}

	void SoundComponent::EditorPauseImp(bool pause)
	{
		if (pause)
			this->pause();
		else
			playSound();
	}

	void SoundComponent::EditorStopImp()
	{
		stop();
	}

	void SoundComponent::playSound()
	{
		if (m_pEvent == NULL)
			return;
		if (m_pEvent->getPauseState())
			m_pEvent->setPause(false);
		else if(!playing())
			m_pEvent->play();

	}

	bool SoundComponent::isPlayable()
	{
		if (m_pEvent == NULL)
			return false;
		return true;
	}

	bool SoundComponent::playing()
	{
		if (!m_pEvent)
			return false;
		BELL_PLAYBACK_STATE tmp;
		m_pEvent->getPlayState(tmp);
		if ( (tmp == BELL_PLAYBACK_PLAYING || tmp == BELL_PLAYBACK_STARTING) && (!m_pEvent->getPauseState()) )
			return true;
		return false;
	}

	void SoundComponent::pause()
	{
		if (!m_pEvent)
			return;
		m_pEvent->setPause(true);

	}

	void SoundComponent::stop()
	{
		if (!m_pEvent)
			return;
		m_pEvent->setPause(false);
		m_pEvent->stop();
	}

	bool SoundComponent::is3D()
	{
		if (!m_pEvent)
			return false;
		return m_pEvent->is3D();
	}

	void SoundComponent::SetMusicLoop(bool isLoop)
	{
		m_bLoop = isLoop;
		if (!m_pEvent || !m_bLoop)
			return;
		if (!playing())
			playSound();
	}

	void SoundComponent::SetVolume(float val)
	{
		if (!m_pEvent)
			return;
		m_pEvent->setVolume(val);
	}

	float SoundComponent::GetVolume()
	{
		if (!m_pEvent)
			return 0.0f;
		return m_pEvent->getVolume();
	}

	void SoundComponent::SetMusicMute()
	{
		if (!m_pEvent)
			return;
		m_pEvent->setVolume(0);
	}

	int SoundComponent::GetTimePos()
	{
		if (!m_pEvent)
			return 0;
		return m_pEvent->getTimelinePosition();
	}

	void SoundComponent::SetTimePos(int val)
	{
		if (!m_pEvent)
			return;
		m_pEvent->setTimelinePosition(val);
	}

	EchoSoundScript EchoSoundScript::Script_GetSoundComponent(const EchoActorScript& pActor, const std::string & name)
	{
		//if (pActor == NULL)
		//	return NULL;
		//return (SoundComponent*)pActor->getComponent(Name(name));
		return EchoSoundScript(pActor, name);
	}

	void EchoSoundScript::reloadSound(const std::string & bank_name, const std::string & event_name)
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->reloadSound(bank_name, event_name);
		}
	}

	void EchoSoundScript::removeSound()
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->removeSound();
		}
	}

	void EchoSoundScript::playSound()
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->playSound();
		}
	}

	bool EchoSoundScript::playing()
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->playing();
		}
		return false;
	}

	void EchoSoundScript::pause()
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->pause();
		}
	}

	void EchoSoundScript::stop()
	{
		SoundComponent* sound = static_cast<SoundComponent*>(m_Ptr.get());
		if (sound) {
			return sound->stop();
		}
	}

}; // namesapce Echo

