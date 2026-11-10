//-----------------------------------------------------------------------------
// File:  EchoSoundComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-8-31
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef		_EchoSoundComponent_H__
#define		_EchoSoundComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoBellEvent.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class SoundComponentInfo;
	class _EchoComponentExport SoundComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SoundComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		virtual void							init(ActorComponentInfo* info) override;	// 初始化
		virtual void							unInit() override;
		virtual void							replay(ActorComponentInfo* info) override;  // 重新设置
		virtual void							copyTo(const Echo::ActorComponentPtr& p_actor_com) override;
		virtual void                            setTimelineAttribute(std::vector<float>&,const Name&) override;

		const std::string&						getBankName();		// 返回当前事件所属的bank的名字
		const std::string&						getEventName();		// 返回当前事件的名字
		//返回当前是否有音乐可以播放
		bool									IsLoadBankEvent() { return m_pEvent != nullptr && !getEventName().empty() && !getBankName().empty(); };

		void									playSound();		// 播放一次声音 音乐暂停时恢复播放
		bool									isPlayable();		// 返回当前的 soundCom 是否可以播放	
		bool									playing();			// 若音乐正在播放返回true
		void									pause();			// 将正在播放的音乐暂停 
		void									stop();				// 将正在播放的音乐关闭
		bool									is3D();

		void									SetMusicLoop(bool isLoop = true);				//循环
		void									SetVolume(float val);	//音量设置
		float									GetVolume();
		void									SetMusicMute();				//静音

		int										GetTimePos();		//获取当前播放时间（毫秒/ms）
		void									SetTimePos(int val);

	public:
		void									loadSound(const std::string &bank_name, const std::string &event_name);
		void									loadSound(const std::pair<std::string, std::string>& BankEvent);
		void									reloadSound(const std::string &bank_name, const std::string &event_name);
		void									reloadSound(const std::pair<std::string, std::string>& BankEvent);
		void									removeSound();

	protected:
		virtual void					EditorPlayImp() override;
		virtual void					EditorPauseImp(bool pause) override;
		virtual void					EditorStopImp() override;

	//public:
	//	SoundComponentInfo*						mInfo = nullptr;
	private:
		std::pair<std::string, std::string>		BankEvent = std::pair("", "");
		bool									m_bLoop = false;
		BellEvent*								m_pEvent = nullptr;
	};
	//////////////////////////////////////////////////////////////////////////

	class _EchoComponentExport SoundComponentInfo : public ActorComponentInfo
	{
		friend class SoundComponent;
	public:
		SoundComponentInfo() {}
		SoundComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual ~SoundComponentInfo();

		virtual void		ExportData(cJSON* jsonNode) override;
		virtual void		ImportData(cJSON* jsonNode) override;
		virtual void		CopyTo(ActorComponentInfo* p_actor_com_info) override;
		virtual void		Overlay(ActorComponentInfo* comInfo) override;
		virtual void		OverWrite() override;

	public:

		//std::string m_strBank_name = "";
		//std::string m_strEvent_name = "";
		std::pair<std::string, std::string>		BankEvent = std::pair("", "");
		bool m_bLoop = false;
		bool m_bMute = false;
		float m_Volume = 1.0f;

		EDITOR_REFLECT
		//DECLARECOMINFO_FLOAT(SoundComponent, Volume, 1.0f, SetVolume)
	};

	DEFINECOMFACTORY(SoundComponent, UNMOVEABLE)

	class EchoSoundScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoSoundScript() = default;
		EchoSoundScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

		static EchoSoundScript			Script_GetSoundComponent(const EchoActorScript& pActor, const std::string& name);
	public:
		void									reloadSound(const std::string &bank_name, const std::string &event_name);
		void									removeSound();
		void									playSound();
		bool									playing();
		void									pause();
		void									stop();
	};
}; // end Echo

#endif // end _EchoSoundComponent_H__
