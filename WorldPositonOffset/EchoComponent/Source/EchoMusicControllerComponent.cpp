//-----------------------------------------------------------------------------
// File:  EchoMusicControllerComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-9-2
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoMusicControllerComponent.h"
#include "EchoTimelineComponent.h"
#include "EchocJsonHelper.h"
#include "EchoResourceFolderManager.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(MusicControllerComponent)

		EDITOR_REFLECT_BEGIN(MusicControllerComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_strDatPath, "strDatPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_strMusicname, "strMusicname", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Conrtollerlist, "ControllerArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	MusicControllerComponentInfo::MusicControllerComponentInfo(const Name & mtype, ActorCOMMoveableType movableType)
		:ActorComponentInfo(mtype, movableType)
		, m_strDatPath("")
		, m_strMusicname("")
	{
	}

	MusicControllerComponentInfo::~MusicControllerComponentInfo()
	{
		m_Conrtollerlist.clear();
	}

	void MusicControllerComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<MusicControllerComponentInfo>::get()->ExportJson(this, jsonNode, "");
		//SAVECOMINFO_PTR(MusicControllerComponent, strDatPath);
		//SAVECOMINFO_PTR(MusicControllerComponent, strMusicname);

		//if (IsCustomParam("m_Conrtollerlist"))
		
	}

	void MusicControllerComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<MusicControllerComponentInfo>::get()->ImportJson(this, jsonNode, "");
		//LOADCOMINFO_PTR(strDatPath);
		//LOADCOMINFO_PTR(strMusicname);

		
	}

	void MusicControllerComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		MusicControllerComponentInfo* info = static_cast<MusicControllerComponentInfo*>(comInfo);
		reflect::TypeResolver<MusicControllerComponentInfo>::get()->copyTo(this, info);

		//info->m_strDatPath = this->m_strDatPath;
		//info->m_strMusicname = this->m_strMusicname;
		info->m_Conrtollerlist = this->m_Conrtollerlist;
	}

	void MusicControllerComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
		/*if (comInfo == nullptr || comInfo->m_Type != m_Type)
			return;
		MusicControllerComponentInfo* pInfo = static_cast<MusicControllerComponentInfo*> (comInfo);
		OVERLAY_PROP(this, pInfo, strDatPath);
		OVERLAY_PROP(this, pInfo, strDatPath);*/
	}

	void MusicControllerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			MusicControllerComponentInfo* oriInfo = static_cast<MusicControllerComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<MusicControllerComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}

	MusicControllerComponent::MusicControllerComponent(Echo::ActorPtr pActor, ActorCOMMoveableType movableType = ALL_MOVEABLE)
		: ProtocolActorComponent(pActor, movableType)
	{
	}

	MusicControllerComponent::~MusicControllerComponent()
	{
	}

	void MusicControllerComponent::init(ActorComponentInfo* info)
	{
		ProtocolActorComponent::init(info);
		GetMusicList();
		m_pActComInfo = info;
		MusicControllerComponentInfo* mInfo = static_cast<MusicControllerComponentInfo*>(m_pActComInfo);
		if (mInfo == NULL)
			return;
		if (mInfo->m_strDatPath.empty())
			return;

		ReadFile(mInfo->m_strDatPath);
		m_Pmusic = (SoundComponent*)m_pActor->getComponent(mInfo->m_strMusicname).get();
		m_Conrtollerlist = mInfo->m_Conrtollerlist;
	}

	void MusicControllerComponent::bindNode()
	{
	}

	void MusicControllerComponent::replay(ActorComponentInfo * info)
	{
		GetMusicList();
		m_pActComInfo = info;
		MusicControllerComponentInfo* mInfo = static_cast<MusicControllerComponentInfo*>(m_pActComInfo);
		if (mInfo == NULL)
			return;
		if (mInfo->m_strDatPath.empty())
			return;

		ReadFile(mInfo->m_strDatPath);
		m_Pmusic = (SoundComponent*)m_pActor->getComponent(mInfo->m_strMusicname).get();
		m_Conrtollerlist = mInfo->m_Conrtollerlist;
	}

	void MusicControllerComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	void MusicControllerComponent::copyTo(const Echo::ActorComponentPtr& actCom)
	{
	}

	void MusicControllerComponent::dealComponentDelete(const Echo::ActorComponentPtr& actCom)
	{
		SoundComponent* sound = static_cast<SoundComponent*>(actCom.get());
		if (sound == m_Pmusic)
			m_Pmusic = NULL;
	}

	void MusicControllerComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void MusicControllerComponent::beginFrame(EventID eventId, ProtocolHandle handle)
	{
		switch (eventId)
		{
		case EID_EveryFrame:
		{
			if (!m_Conrtollerlist.empty() && m_Pmusic != NULL && m_Pmusic->IsLoadBankEvent())
			{
				for (auto iter : m_Conrtollerlist)
				{
					if (!iter.first.empty() && iter.second.is_create && !iter.second.timelineName.empty())
					{
						if (!m_Pmusic->playing())
						{
							for (auto name : iter.second.timelineName)
							{
								TimelineComponent* lightTimeCom = static_cast<TimelineComponent*>(m_pActor->getComponent(name).get());
								if (lightTimeCom == nullptr)
									continue;
								if (lightTimeCom->getIsPlayScene())
								{
									lightTimeCom->remakeTime();
									lightTimeCom->setIsPlay(false);
								}
								
							}
						}
						else
						{
							int time = GetMusicTime() / 30;
							if (time >= main_frequency.size())
								time = (int)main_frequency.size() - 1;
							if (time > iter.second.time_min && time < iter.second.time_max)
							{
								float frequency = GetMainFrequencyTimeVal(time);
								for (auto name : iter.second.timelineName)
								{
									TimelineComponent* lightTimeCom = static_cast<TimelineComponent*>(m_pActor->getComponent(name).get());
									if (lightTimeCom == nullptr)
										continue;
									if (frequency > iter.second.frequency_min && frequency < iter.second.frequency_max && lightTimeCom->getIsPlayScene() == false)
									{
										lightTimeCom->remakeTime();
										lightTimeCom->setIsPlay(true);
									}
								}
							}
						}	
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}

	Name musicSpectrumName_ = Name("MusicSpectrum");
	void MusicControllerComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (name == musicSpectrumName_)
		{
			if (m_Pmusic)
			{
				int temp = static_cast<int>(data[0]);
				if (temp != -1 && !m_Pmusic->playing())
				{
					m_Pmusic->playSound();
					m_Pmusic->SetTimePos(temp * 30);
				}
				else if (temp == -1 && m_Pmusic->playing())
				{
					m_Pmusic->stop();
				}
			}
		}
	}

	void MusicControllerComponent::ReadFile(const std::string& path)
	{
		if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
			return;

		DataStreamPtr inFile(Root::instance()->GetFileDataStream(path.c_str()));
		if (inFile.isNull())
			return;
		m_strDatPath = path;
		main_frequency.clear();
		while (!inFile->eof())
		{
			float val;
			inFile->readData(&val);
			main_frequency.push_back(val);
		}
		inFile->close();
		main_frequency.shrink_to_fit();
	}

	void MusicControllerComponent::GetMusicList()
	{
		const EchoTMap<String, Echo::ActorComponentPtr>& ComList = m_pActor->getComponents();
		mMusicList.clear();
		for (auto& it : ComList)
		{
			if (it.second->getType() == COMPONENTTYPE(SoundComponent))
				mMusicList.push_back(it.first);
		}
		mMusicList.shrink_to_fit();
	}
	void MusicControllerComponent::GetMusicCom(const std::string & comName)
	{
		SoundComponent* musicCom = static_cast<SoundComponent*>(m_pActor->getComponent(comName).get());
		if (musicCom == NULL)
			return;
		m_Pmusic = musicCom;
		//m_Musicname = comName.c_str();
	}
	void MusicControllerComponent::GetControlTimelineList()
	{
		const EchoTMap<String, Echo::ActorComponentPtr>& ComList = m_pActor->getComponents();
		m_timelinelist.clear();
		for (auto& it : ComList)
		{
			if (it.second->getType() == COMPONENTTYPE(TimelineComponent))
				m_timelinelist.push_back(it.first);
		}
		m_timelinelist.shrink_to_fit();
	}
	int MusicControllerComponent::GetMusicTime()
	{
		if(m_Pmusic == NULL)
			return 0;
		return m_Pmusic->GetTimePos();
	}
	void MusicControllerComponent::SetMusicController(const std::unordered_map<std::string, MusicController>& controllerData)
	{
		m_Conrtollerlist = controllerData;
	}
	MusicControllerComponentScript MusicControllerComponentScript::GetMusicControllerComponent(const EchoActorScript& pActor, const std::string & name)
	{
		return MusicControllerComponentScript(pActor, name);
	}
	void _EchoComponentExport WriteJSNode(cJSON* jsonNode, const char* Name, const std::unordered_map<std::string, MusicController>& m_Conrtollerlist)
	{
		cJSON* ControllerArray = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "ControllerArray", ControllerArray);
		for (auto iter : m_Conrtollerlist)
		{
			cJSON* controller = cJSON_CreateObject();
			cJSON_AddItemToArray(ControllerArray, controller);
			cJSON_AddStringToObject(controller, "ControllerName", iter.first.c_str());
			cJSON_AddBoolToObject(controller, "Is_create", iter.second.is_create);
			cJSON_AddNumberToObject(controller, "Frequency_max", iter.second.frequency_max);
			cJSON_AddNumberToObject(controller, "Frequency_min", iter.second.frequency_min);
			cJSON_AddNumberToObject(controller, "Time_max", iter.second.time_max);
			cJSON_AddNumberToObject(controller, "Time_min", iter.second.time_min);

			cJSON* TimelineArray = cJSON_CreateArray();
			cJSON_AddItemToObject(controller, "TimelineArray", TimelineArray);
			int index = 0;
			for (auto name : iter.second.timelineName)
			{
				char msg[32] = { 0 };
				snprintf(msg, 32, "Timeline_name%d", index);
				cJSON_AddStringToObject(TimelineArray, msg, name.c_str());
				index++;
			}
		}
		
	}
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, std::unordered_map<std::string, MusicController>& m_Conrtollerlist)
	{
		cJSON* ControllerArray = cJSON_GetObjectItem(jsonNode, "ControllerArray");
		if (ControllerArray)
		{
			for (cJSON* pChild = ControllerArray->child; nullptr != pChild; pChild = pChild->next)
			{
				std::string	controllerName;
				MusicController mController;
				ReadJSNode(pChild, "ControllerName", controllerName);
				ReadJSNode(pChild, "Is_create", mController.is_create);
				ReadJSNode(pChild, "Frequency_max", mController.frequency_max);
				ReadJSNode(pChild, "Frequency_min", mController.frequency_min);
				ReadJSNode(pChild, "Time_max", mController.time_max);
				ReadJSNode(pChild, "Time_min", mController.time_min);

				cJSON* TimelineArray = cJSON_GetObjectItem(pChild, "TimelineArray");
				if (TimelineArray)
				{
					size_t csz = cJSON_GetArraySize(TimelineArray);

					for (int i = 0; i < csz; i++)
					{
						std::string name;
						if (cJSON_GetArrayItem(TimelineArray, i))
							name = cJSON_GetArrayItem(TimelineArray, i)->valuestring;
						if (!name.empty())
							mController.timelineName.push_back(name);
					}
				}

				m_Conrtollerlist[controllerName] = mController;
			}
		}
		return true;
	}
}
