//-----------------------------------------------------------------------------
// File:  EchoDecalComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-8-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoDecalComponent_h__
#define _EchoDecalComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "EchoHermiteCurve.h"
#include "EchoHermite.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"


namespace Echo
{
	class DecalComponentInfo;
	//=============================================================================
	class _EchoComponentExport DecalComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(DecalComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		virtual void							init(ActorComponentInfo* info) override;
		virtual void							unInit() override;
		virtual void							bindNode() override;
		virtual void							replay(ActorComponentInfo* info) override;
		virtual void							copyTo(const ActorComponentPtr&  actCom) override;
		virtual void							showEditorHelper(bool isShow) override;
		virtual void							updateCombinedName() override;
		virtual void							NotifiChildImp(int changeMask) override;
		void									ShowHelper(bool isShow);
		void									ShowDecal(bool isShow);
		bool									IsShowHelper() { return bShowHelper; }
		bool									IsShowDecal() { return bShowDecal; }
		bool									IsShowPrior() { return IsPrior; };
		// recreate decal
		void									UpdateDecalWithBox(bool synchronous = false);
		// recreate box
		void									UpdateBox(bool synchronous = false);
		// create a box and decal
		void									CreateDecal(bool synchronous = false);

		virtual	void							setActor(const ActorPtr& pActor) override;
		virtual void                            setTimelineAttribute(std::vector<float>&,const Name& ) override;

		void									setShowDecal(bool isShow);

		void									setPefFile(const String& path);
		String									getPefFile();
		void									setDecalPath(const String& path);
		String									getDecalPath();
		//优先显示
		void									ShowPrior(bool isPrior);
		// change decal's texture
		void									changeDiffuseTexture(const String& path, bool synchronous = false);
		//更改粒子
		void									ChangeParticleHolder(bool synchronous = false);

		void									SetParticleColor(const Vector4& _color);
		Vector4									GetParticleColor();

		//获取盒子顶点位置数据
		const std::vector<Vector3>&				getHelperPointLocalPos();
		const std::vector<Vector3>&				getHelperPointWorldPos();

	private:
		void									CreateHelper(bool synchronous = false);
		void									ReleaseHelper();
		void									CreateParticleHolder(bool synchronous = false);
		void									ReleaseParticleHolder();

		void									setColor(HelperObjColorType color);
		void									setDecalType(HelperObjType type);
		void									setDecalRenderType(ECHO_DRAW);
	public:
		HelperDesc								mHelperDesc;
		//DecalComponentInfo*						mDecalInfo = nullptr;
	private:
		Echo::String							sPefFile = "gfx/pef/ceshi/ceshi_tiehua_005.pef";
		Echo::String							sDecalTexture = "";

		bool									IsShowEditorHelper;
		bool									bShowHelper = true;
		bool									bShowDecal = false;
		bool									IsPrior = false;

		std::vector<Vector3>					mHelperPointLocal;
		std::vector<Vector3>					mHelperPointWorld;

		Echo::HelperObject*						mHelperObj = nullptr;
		Echo::EffectHolder*					mParticleHolder = nullptr;
	};

	class _EchoComponentExport DecalComponentInfo : public ActorComponentInfo
	{
		friend class DecalComponet;
	public:
		DecalComponentInfo() {}
		DecalComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~DecalComponentInfo();
		virtual void							ExportData(cJSON* jsonNode)override;
		virtual void							ImportData(cJSON* jsonNode)override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void							Overlay(ActorComponentInfo* comInfo) override;
		virtual void							OverWrite() override;
#ifdef ECHO_EDITOR
		virtual bool							getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		//DECLARECOMINFO_STRING(DecalComponent, PefFile,"", setPefFile)
		//DECLARECOMINFO_STRING(DecalComponent, DecalPath,"", setDecalPath)
		//DECLARECOMINFO_BOOL(DecalComponent, ShowDecal, false,  ShowDecal)
		//DECLARECOMINFO_BOOL(DecalComponent, IsPrior, false, ShowDecal)
		//DECLARECOMINFO_VECTOR4(DecalComponent, color, Vector4(0.0f, 0.0f, 0.0f, 1.0f), SetParticleColor)

		std::string m_PefFile;
		std::string m_DecalPath;
		bool m_ShowDecal = false;
		bool m_IsPrior = false;
		Vector4 m_color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(DecalComponent, ALL_MOVEABLE);

	class _EchoComponentExport EchoDecalProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								uninitProtocolFunction();
	};

	class EchoDecalScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoDecalScript() = default;
		EchoDecalScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

		static EchoDecalScript						Script_GetDecalComponent(const EchoActorScript& pActor, const std::string& name);
	public:
		void										changeDiffuseTexture(const std::string& path);
	};
}
// namespace Echo
#endif 
