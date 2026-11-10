//-----------------------------------------------------------------------------
// File:  EchoBoxComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-8-26
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoBoxComponent_h__
#define _EchoBoxComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoHermiteCurve.h"
#include "EchoHermite.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class BoxComponentInfo;
	//=============================================================================
	class _EchoComponentExport BoxComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(BoxComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		virtual void							init(ActorComponentInfo * info) override;
		virtual void							unInit() override;
		virtual void							replay(ActorComponentInfo* info) override;
		virtual void							bindNode() override;
		virtual void							copyTo(const ActorComponentPtr&  actCom) override;
		virtual void							showEditorHelper(bool isShow) override;
		virtual void							updateCombinedName() override;
		virtual AxisAlignedBox					getLoadBoundingBox() override;

		void									ShowBoxHelper(bool isShow);
	    bool									IsShowHelper() { return isShowHelper; }
		void                                    showBoxEpitaxyHelper(bool isShow);

		void									IsCreateRender(bool isRender);

		void									CreateBox();

		void									setHelperDesc(HelperObjColorType color, HelperObjType type = HelperObjType_Box, ECHO_DRAW renderType = ECHO_LINE_STRIP);	//color0(r,g,b,a)  type:box类型  renderType:渲染类型
		
		void									setColor(int color);
		HelperObjColorType						getColor();

		void									setBoxType(int type);
		HelperObjType							getBoxType();

		void									setBoxRenderType(int);
		ECHO_DRAW								getBoxRenderType();
		
		void									setPosition(const Vector3& pos);
		Vector3									getPosition();

		void									setOrientation(const Quaternion& qua);
		Quaternion						        getQuaternion();

		void									setScale(const Vector3& scale);
		//Vector3									getScale();

		bool									isPosInBox(const Vector3& pos);
		bool                                    isPosInEpitaxyBox(const Vector3& pos);
		bool									isPosInTripleEpitaxyBox(const Vector3& pos);

		void									setPriority(int prior);
		int 									getPriority() { return mPriority;}

		void                                    setEpitaxyDistance(float epitaxyDistance_);
		float                                   getEpitaxyDistance();
	//public:
		//BoxComponentInfo*						mBoxInfo = nullptr;
	private:
		void									CreateHelper();
		void									ReleaseHelper();
		void                                    createEpitaxyObject();
		void                                    destroyEpitaxyObject();
	private:
		bool									isShowHelper;
		//temporary
		HelperObject*							mHelperObj = nullptr;
		HelperObject*                           mEpitaxyHelperObj = nullptr;
		int										mPriority = 0;
		float                                   mEpitaxyDistance = 0.f;
	};

	//=============================================================================
	class _EchoComponentExport BoxComponentInfo : public ActorComponentInfo
	{
		friend class BoxComponent;
	public:
		BoxComponentInfo() {}
		BoxComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		~BoxComponentInfo();
		virtual void							ExportData(cJSON* jsonNode)override;
		virtual void							ImportData(cJSON* jsonNode)override;
		virtual void							CopyTo(ActorComponentInfo * info) override;
		virtual void							Overlay(ActorComponentInfo* comInfo) override;
		virtual void							OverWrite() override;

	public:
		Echo::Name					m_HelperName;
		Name						m_CustomName;
		//std::string					m_HelperName;
		//std::string					m_CustomName;
		int							m_m_ObjType = 0;
		int							m_m_RenderType = 2;
		int							m_ColorType = HelperObjColorType_Green;
		Vector3						m_InitPos = Vector3::ZERO;
		Quaternion					m_InitQua = Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
		Vector3						m_InitScale = Vector3::UNIT_SCALE;
		int							m_Priority = 0;
		bool						m_ShowBox = true;
		bool						m_bCustomSync;
		std::vector<Vector3>		m_vCustomVBVec;
		std::vector<uint16>			m_CustomIBVec;
		float                       mEpitaxyDistance = 0.f;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(BoxComponent,ALL_MOVEABLE);


	// 脚本类
	class EchoBoxScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		EchoBoxScript() = default;
		EchoBoxScript(const EchoActorScript& pActor, const std::string& name);

	public:
		static EchoBoxScript						Script_GetBoxComponent(const EchoActorScript&  pActor, const std::string& name);

		bool									    isPosInBox(const Vector3& pos);
		int                                         getPriority();
		void                                        setEpitaxyDistance(float epitaxyDistance_);
		bool                                        isPosInEpitaxyBox(const Vector3& pos);
	};

	class _EchoComponentExport EchoBoxComponentProFuncImp
	{
	   public:
		  static void initProtocolFunction();
		  static void uninitProtocolFunction();

		  static Echo::ProtocolResult  ProCom_SetBoxEpitaxyDistance(void* distance_, ProtocolHandle comHandle);
	};
}
// namespace Echo
#endif 
