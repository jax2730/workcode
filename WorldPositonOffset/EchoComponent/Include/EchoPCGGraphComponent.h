//-----------------------------------------------------------------------------
// File:   EchoPCGGraphComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2025-2-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoPCGGraphComponent_h__
#define __EchoPCGGraphComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoBiomeGenObjectLibrary.h"

namespace Echo
{
	class EchoPCGObject;

	class PCGGraphComponent;  typedef EchoWeakPtr<PCGGraphComponent> PCGGraphComponentPtr;
	class _EchoComponentExport PCGGraphComponent : public NodeActorComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(PCGGraphComponent)
		REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void visibleStateChanged(bool activeState) override;
		virtual void updateCombinedName() override;
		virtual AxisAlignedBox getLoadBoundingBox() override;
		virtual void showEditorHelper(bool isShow) override;
	public:
		void SetPCGGraphPath(const String& file);
		const String& GetPCGGraphPath() const { return mPCGGraphPath; }

		void SetBoundSize(const Vector3& size);
		const Vector3& GetBoundSize() const { return mBoundSize; }

		void SetSeed(uint32 seed);
		uint32 GetSeed() const { return mSeed; }

		void SetHelperVisible(bool value);
		bool GetHelperVisible() const { return m_IsHelperVisible; }

		void SetPCGInfo(const PCG::BiomeGenObjectLayerInfo& info);
		PCG::BiomeGenObjectLayerInfo& GetPCGInfo() { return mPCGInfo; }

		void SetPreExecuteCallback(const std::function<void(EchoPCGObject*)>& _PreExecuteCallback) { mPreExecuteCallback = _PreExecuteCallback; }

		ECHO_FORCE_INLINE void MarkNeedExcutePCGGraph() { mbNeedExcutePCGGraph = true; }
		ECHO_FORCE_INLINE bool NeedExcutePCGGraph() { return mbNeedExcutePCGGraph;}
	protected:
		void CreateHelper();
		void ReleaseHelper();
		void UpdateHelper();
		void UpdateHelperVisible();
		HelperObject* m_pHelperObj = nullptr;
	private:
		void ExcutePCGGraph();
		void ClearPCGObject();
	private:
		bool bInit : 1 = false;
		bool mbNeedExcutePCGGraph : 1 = true;
		bool m_IsHelperVisible : 1 = false;
		String mPCGGraphPath;
		Vector3 mBoundSize = Vector3::UNIT_SCALE * 256.0f;
		uint32 mSeed = 0;
		PCG::BiomeGenObjectLayerInfo mPCGInfo;
		EchoPCGObject* m_pEchoPCGObject = nullptr;
		std::function<void(EchoPCGObject*)> mPreExecuteCallback = nullptr;
		friend class PCGGraphComponentManager;
	};

	class _EchoComponentExport PCGGraphComponentInfo : public ActorComponentInfo{
	public:
		PCGGraphComponentInfo() {}
		PCGGraphComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual ~PCGGraphComponentInfo() {};
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(PCGGraphComponent)
		_DECLCOMINFO_PARAM(String, PCGGraphPath, String(""), SetPCGGraphPath)
		_DECLCOMINFO_PARAM(Vector3, BoundSize, Vector3(128.0f, 128.0f, 128.0f), SetBoundSize)
		_DECLCOMINFO_PARAM_SIM(uint32, Seed, 0, SetSeed)
		_DECLCOMINFO_PARAM(PCG::BiomeGenObjectLayerInfo, PCGInfo, {}, SetPCGInfo)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(PCGGraphComponent,ALL_MOVEABLE)

};

#endif // __EchoPCGGraphComponent_h__
