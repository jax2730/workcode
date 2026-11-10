//-----------------------------------------------------------------------------
// File:  EchoMusicControllerComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-9-2
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoMusicControllerComponent_H__
#define _EchoMusicControllerComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoRoot.h"
#include "EchoSoundComponent.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoProtocolActorComponent.h"

namespace Echo
{
	struct MusicController
	{
		std::vector<std::string>		timelineName;

		int			frequency_min = 0;
		int			frequency_max = 1000;

		int			time_min = 0;
		int			time_max = 1000;

		bool		is_create = false;

		void init()
		{
			timelineName.clear();
			frequency_min = 0;
			frequency_max = 1000;
			time_min = 0;
			time_max = 1000;
			is_create = true;
		}
	};
	class MusicControllerComponentInfo;
	
	//--------------------------------------------------------------------------------------
	class _EchoComponentExport MusicControllerComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(MusicControllerComponent)
		REGISTER_LUA_CLASS_KEY
	public:
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						bindNode() override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual	void						setActor(const ActorPtr& pActor) override;
		virtual void						copyTo(const Echo::ActorComponentPtr& actCom) override;
		virtual void						dealComponentDelete(const Echo::ActorComponentPtr& actCom) override;
	/********************************************************************************************************************/
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;
		virtual void						endFrame(EventID, ProtocolHandle handle) override {};
		virtual void						preUpdateAnimation(EventID, ProtocolHandle handle) override {}
		virtual void						postUpdateAnimation(EventID, ProtocolHandle handle) override {}

		virtual void						preUpdateScene(EventID, ProtocolHandle handle) override {}
		virtual void						postUpdateScene(EventID, ProtocolHandle handle) override {}
		//ImGui Set Value
		virtual void						preRenderScene(EventID, ProtocolHandle handle) override {};
		//ImGui Get Value
		virtual void						postRenderScene(EventID, ProtocolHandle handle) override {};

		virtual void						ResetRegisterFuns() {};
		virtual void                        setTimelineAttribute(std::vector<float>&,const Name&) override;
	/********************************************************************************************************************/
		void								ReadFile(const std::string& path);
		int									GetTimeSize() { return (int)main_frequency.size() - 1; };
		std::vector<float>					GetMainFrequency() { return main_frequency; };

		void								GetMusicList();
		void								GetMusicCom(const std::string & comName);
		SoundComponent*						GetMusic() { return m_Pmusic; };

		void								GetControlTimelineList();
		
		float								GetMainFrequencyTimeVal(int time) { return main_frequency[time]; };
		int									GetMusicTime();

		void								SetMusicController(const std::unordered_map<std::string, MusicController>& controllerData);
	public:
		//MusicControllerComponentInfo*		mInfo = nullptr;

		std::vector<std::string>			mMusicList;
		std::vector<std::string>			m_timelinelist;

		std::string							m_strDatPath = "";
		std::unordered_map<std::string, MusicController>	m_Conrtollerlist;
	private:
		SoundComponent*						m_Pmusic = nullptr;

		std::vector<float>					main_frequency;
	};
	//--------------------------------------------------------------------------------------------------------
	class _EchoComponentExport MusicControllerComponentInfo : public ActorComponentInfo
	{
		friend class MusicControllerComponent;
	public:
		MusicControllerComponentInfo() {}
		MusicControllerComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = UNMOVEABLE);
		virtual ~MusicControllerComponentInfo();
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void						Overlay(ActorComponentInfo* comInfo) override;
		virtual void						OverWrite() override;

		//DECLARECOMINFO_PTR1(MusicControllerComponent, std::string, strDatPath, "", ReadFile)
		//DECLARECOMINFO_PTR1(MusicControllerComponent, std::string, strMusicname, "", GetMusicCom)
	public:
		std::string		m_strDatPath = "";
		std::string		m_strMusicname = "";
		std::unordered_map<std::string, MusicController>	m_Conrtollerlist;

		EDITOR_REFLECT
	};
	//--------------------------------------------------------------------------------------------------------
	class MusicControllerComponentScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		MusicControllerComponentScript() = default;
		MusicControllerComponentScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

		static MusicControllerComponentScript GetMusicControllerComponent(const EchoActorScript& pActor, const std::string& name);
	};
	void _EchoComponentExport WriteJSNode(cJSON* jsonNode, const char* Name, const std::unordered_map<std::string, MusicController>& data);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, std::unordered_map<std::string, MusicController>& data);

	DEFINECOMFACTORY(MusicControllerComponent, UNMOVEABLE);
}
#endif
