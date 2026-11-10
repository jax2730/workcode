//-----------------------------------------------------------------------------
// File:  EchoBoxComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-8-26
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoBoxComponent.h"
#include "EchoWorldNode.h"
#include "EchoSceneManager.h"
#include "EchoSceneNode.h"
#include "Actor/EchoActor.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoActorProFuncImp.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(BoxComponent)
	BoxComponent::BoxComponent(ActorPtr  pActor, ActorCOMMoveableType type)
		:NodeActorComponent(pActor,type)
		,isShowHelper(true)
		, mHelperObj(NULL)
		, mPriority(0)
	{
	}

	BoxComponent::~BoxComponent()
	{
	}

	void BoxComponent::init(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		BoxComponentInfo* boxComInfo = static_cast<BoxComponentInfo*>(m_pActComInfo);

		CreateHelper();

		if(boxComInfo != nullptr)
		{
			if (mHelperObj != nullptr)
			{
				ShowBoxHelper(true);
				mHelperObj->changeHelperType(static_cast<HelperObjType>(boxComInfo->m_m_ObjType));
				mHelperObj->setRenderType(static_cast<ECHO_DRAW>(boxComInfo->m_m_RenderType));
				mHelperObj->setHelperColorType((HelperObjColorType)boxComInfo->m_ColorType);
				if (boxComInfo->m_m_ObjType == HelperObjType_Custom)
					mHelperObj->changeCustomMesh(boxComInfo->m_CustomName, boxComInfo->m_bCustomSync);
				else if (boxComInfo->m_m_ObjType == HelperObjType_CustomPolygon)
					mHelperObj->changeCustomPolygon(boxComInfo->m_vCustomVBVec, boxComInfo->m_CustomIBVec);
				//mHelperObj->setScale(boxComInfo->m_InitScale);
				setEpitaxyDistance(boxComInfo->mEpitaxyDistance);
			}
			if (boxComInfo->m_Priority > 255) boxComInfo->m_Priority = 255;
			else if (boxComInfo->m_Priority < 0) boxComInfo->m_Priority = 0;
			mPriority = boxComInfo->m_Priority;
		}

		if (mHelperObj != nullptr) {
			addChildToNode(mHelperObj->getSceneNode());
			UpdateComRootNode();
		}
	}

	void BoxComponent::unInit()
	{
		ReleaseHelper();
		destroyEpitaxyObject();
	}

	void BoxComponent::replay(ActorComponentInfo * info)
	{
		BoxComponentInfo* boxComInfo = static_cast<BoxComponentInfo*>(info);
		if (info == NULL)
			return;
		ShowBoxHelper(boxComInfo->m_ShowBox);
		setBoxType(boxComInfo->m_m_ObjType);
		setBoxRenderType(boxComInfo->m_m_RenderType);
		setColor((HelperObjColorType)boxComInfo->m_ColorType);

		if (mHelperObj == NULL)
			return;
		if (boxComInfo->m_m_ObjType == HelperObjType_Custom)
			mHelperObj->changeCustomMesh(boxComInfo->m_CustomName, boxComInfo->m_bCustomSync);
		else if (boxComInfo->m_m_ObjType == HelperObjType_CustomPolygon)
			mHelperObj->changeCustomPolygon(boxComInfo->m_vCustomVBVec, boxComInfo->m_CustomIBVec);

		//setPosition(boxComInfo->mHelperDesc.initPos);
		setScale(boxComInfo->m_InitScale);
		mHelperObj->setOrientation(boxComInfo->m_InitQua);
	}

	void BoxComponent::bindNode()
	{
	}

	void BoxComponent::copyTo(const ActorComponentPtr&  target_com)
	{
	}

	void BoxComponent::showEditorHelper(bool isShow)
	{
		ShowBoxHelper(isShow);
		showBoxEpitaxyHelper(isShow);
	}

	void BoxComponent::updateCombinedName()
	{
		if (mHelperObj == NULL)
			return;
		Name helperName = Name(getCombinedName() + "||" + "_box");
		mHelperObj->setName(helperName);
	}

	AxisAlignedBox BoxComponent::getLoadBoundingBox()
	{
		if (mHelperObj == nullptr)
		{
			return ActorComponent::getLoadBoundingBox();
		}

		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();

		AxisAlignedBox loadBox(minBox - mHelperObj->GetRanderingDistance(), maxBox + mHelperObj->GetRanderingDistance());
		return loadBox;
	}

	void BoxComponent::ReleaseHelper()
	{
		if (mHelperObj == NULL)
			return;
		delete mHelperObj;
		mHelperObj = NULL;
	}

	void BoxComponent::ShowBoxHelper(bool isShow)
	{
		if (mHelperObj == NULL)
			return;
        mHelperObj->setVisible(isShow);
	}

	void BoxComponent::showBoxEpitaxyHelper(bool isShow)
	{
		if (mEpitaxyHelperObj == nullptr)
			return;
		mEpitaxyHelperObj->setVisible(isShow);
	}

	void BoxComponent::IsCreateRender(bool isRender)
	{
		if (!mHelperObj)
			return;
		mHelperObj->setCreateRender(isRender, true);
	}


	void BoxComponent::CreateBox()
	{
		CreateHelper();
	}


	void BoxComponent::CreateHelper()
	{
		ReleaseHelper();
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client)
			return;
		Name helperName = Name(getCombinedName() + "||" + "_box");
		//mHelperObj = new HelperObject(helperName, HelperObjType_Box,false,nullptr,nullptr,true);
		HelperObjectDesc desc;
		desc.IsBlendEnable = true;
		//desc.isRender = false;
		mHelperObj = new HelperObject(helperName, desc);
		mHelperObj->setScale(Vector3(1.f, 1.f, 1.f));
		mHelperObj->setVisible(true);
	}

	void BoxComponent::setHelperDesc(HelperObjColorType color, HelperObjType type, ECHO_DRAW renderType)
	{
		if (mHelperObj == NULL)
			return;
		mHelperObj->setHelperColorType(color);

		mHelperObj->changeHelperType(type);

		mHelperObj->setRenderType(renderType);	
	}

	HelperObjColorType BoxComponent::getColor()
	{
		if (mHelperObj == NULL)
			return HelperObjColorType_Green;
		return mHelperObj->getHelperColorType();
	}

	HelperObjType BoxComponent::getBoxType()
	{
		if (mHelperObj == NULL)
			return HelperObjType_Box;
		//return mHelperDesc.type;
		return mHelperObj->getType();
	}

	ECHO_DRAW BoxComponent::getBoxRenderType()
	{
		if (mHelperObj == NULL)
			return ECHO_DRAW_MODE_COUNT;
		//return mHelperDesc.renderType;
		return mHelperObj->getRenderType();
	}

	void BoxComponent::setBoxType(int type)
	{
		if (mHelperObj == NULL)
			return;
		//mHelperDesc.type = type;
		mHelperObj->changeHelperType(static_cast<HelperObjType>(type));
		if (mEpitaxyHelperObj != nullptr)
		{
			mEpitaxyHelperObj->changeHelperType(static_cast<HelperObjType>(type));
		}
	}

	void BoxComponent::setBoxRenderType(int rType)
	{
		if (mHelperObj == NULL)
			return;
		mHelperObj->setRenderType(static_cast<ECHO_DRAW>(rType));
		if (mEpitaxyHelperObj != nullptr)
		{
			mEpitaxyHelperObj->setRenderType(static_cast<ECHO_DRAW>(rType));
		}
		//mHelperDesc.renderType = rType;
	}

	void BoxComponent::setPosition(const Vector3 & pos)
	{
		if (mHelperObj == NULL)
			return;
		//mHelperDesc.initPos = pos;
		mHelperObj->setPosition(pos);
	}

	Vector3 BoxComponent::getPosition()
	{
		if (mHelperObj == NULL)
			return Vector3(0, 0, 0);
		return mHelperObj->getPosition();
	}

	void BoxComponent::setOrientation(const Quaternion & qua)
	{
		if (mHelperObj == NULL)
			return;
		mHelperObj->setOrientation(qua);
	}

	Quaternion BoxComponent::getQuaternion()
	{
		if (mHelperObj == NULL)
			return Quaternion(0, 0, 0, 0);
		return mHelperObj->getOrientation();
	}

	void BoxComponent::setScale(const Vector3 & scale)
	{
		if (mHelperObj == NULL)
			return;
		mHelperObj->setScale(scale);
	}

	/*Vector3 BoxComponent::getScale()
	{
		if (mHelperObj == NULL)
			return Vector3(1, 1, 1);
		return mHelperObj->getScale();
	}*/

	bool BoxComponent::isPosInBox(const Vector3 & pos)
	{
		if (mHelperObj == NULL)
			return false;
		return mHelperObj->isPosIn(pos);
		//return false;
	}

	bool BoxComponent::isPosInEpitaxyBox(const Vector3& pos)
	{
		if(mEpitaxyHelperObj == nullptr)
		   return false;
		return mEpitaxyHelperObj->isPosIn(pos);
	}

	bool BoxComponent::isPosInTripleEpitaxyBox(const Vector3& pos)
	{
		if (mEpitaxyHelperObj == nullptr)
			return false;
		return mEpitaxyHelperObj->isPosInTripleSphere(pos);
	}

	void BoxComponent::setPriority(int prior)
	{
		if (mHelperObj == NULL)
			return;
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mPriority = prior;
	}

	void BoxComponent::setEpitaxyDistance(float epitaxyDistance_)
	{
		if (mHelperObj == nullptr)
			return;
		mEpitaxyDistance = epitaxyDistance_;
		if (mEpitaxyDistance == 0.f)
		{
			destroyEpitaxyObject();
			return;
		}

		if (mEpitaxyDistance != 0.f && mEpitaxyHelperObj == nullptr)
		{
			createEpitaxyObject();
		}
		Vector3 tempScale;
		tempScale.x = epitaxyDistance_  / getScale().x;
		tempScale.y = epitaxyDistance_ / getScale().y;
		tempScale.z = epitaxyDistance_ / getScale().z;

		mEpitaxyHelperObj->setScale(tempScale +mHelperObj->getScale());
	}

	float BoxComponent::getEpitaxyDistance()
	{
		if (mEpitaxyHelperObj == nullptr)
			return 0.f;
		return mEpitaxyDistance;
	}

	void BoxComponent::createEpitaxyObject()
	{
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client)
			return;
		if (mHelperObj == nullptr)
			return;
		if (mEpitaxyHelperObj)
			destroyEpitaxyObject();
		Name helperName = Name(getCombinedName() + "||" + "EpitaxyBox");
		mEpitaxyHelperObj  = new HelperObject(helperName, mHelperObj->getType(), false, nullptr, nullptr, true);
		mEpitaxyHelperObj->setHelperColorType(HelperObjColorType_Red);
		
		mEpitaxyHelperObj->setVisible(true);
		mEpitaxyHelperObj->setRenderType(mHelperObj->getRenderType());
		addChildToNode(mEpitaxyHelperObj->getSceneNode());
		UpdateComRootNode();
	}

	void BoxComponent::destroyEpitaxyObject()
	{
		if (mEpitaxyHelperObj)
			delete mEpitaxyHelperObj;
		mEpitaxyHelperObj = nullptr;
	}

	void BoxComponent::setColor(int color)
	{
		if (mHelperObj == NULL)
			return;
		//mHelperDesc.color = color;
		mHelperObj->setHelperColorType((HelperObjColorType)color);
	}

	EchoBoxScript::EchoBoxScript(const EchoActorScript& pActor, const std::string& name) : 
		EchoActorComponentScript(pActor, name)
	{
	}

	EchoBoxScript EchoBoxScript::Script_GetBoxComponent(const EchoActorScript&  pActor, const std::string & name)
	{
		return EchoBoxScript(pActor, name);
	}

	bool EchoBoxScript::isPosInBox(const Vector3& pos)
	{
		BoxComponent* pBox = static_cast<BoxComponent*>(m_Ptr.get());
		if (pBox) {
			return pBox->isPosInBox(pos);
		}
		return false;
	}


	int EchoBoxScript::getPriority()
	{
		BoxComponent* currCom = static_cast<BoxComponent*>(m_Ptr.get());
		if (currCom == nullptr)
		   return 0;
		return currCom->getPriority();
	}

	void EchoBoxScript::setEpitaxyDistance(float epitaxyDistance_)
	{
		BoxComponent* currCom = static_cast<BoxComponent*>(m_Ptr.get());
		if (currCom == nullptr)
			return;
		currCom->setEpitaxyDistance(epitaxyDistance_);
	}

	bool EchoBoxScript::isPosInEpitaxyBox(const Vector3& pos)
	{
		BoxComponent* currCom = static_cast<BoxComponent*>(m_Ptr.get());
		if (currCom == nullptr)
			return false;
		return currCom->isPosInEpitaxyBox(pos);
	}

	EDITOR_REFLECT_BEGIN(BoxComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_m_ObjType, "m_ObjType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_m_RenderType, "m_RenderType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ColorType, "ColorType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_InitPos, "InitPos", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_InitQua, "InitQua", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_InitScale, "InitScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Priority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ShowBox, "ShowBox", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mEpitaxyDistance, "EpitaxyDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	BoxComponentInfo::BoxComponentInfo(const Name& comType, ActorCOMMoveableType MoveType)
		:ActorComponentInfo(comType, MoveType)
		, m_HelperName("")

		, m_CustomName("")
		, m_bCustomSync(true)
	{
	}
	BoxComponentInfo::~BoxComponentInfo()
	{
	}
	void BoxComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<BoxComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void BoxComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<BoxComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void BoxComponentInfo::CopyTo(ActorComponentInfo * info)
	{
		BoxComponentInfo* target_info = static_cast<BoxComponentInfo*>(info);
		reflect::TypeResolver<BoxComponentInfo>::get()->copyTo(this, target_info);
	}
	void BoxComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
	}
	void BoxComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			BoxComponentInfo* oriInfo = static_cast<BoxComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<BoxComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}
	void EchoBoxComponentProFuncImp::initProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->registerProtocolExtFunc(Name("SetBoxEpitaxyDistance"), &EchoBoxComponentProFuncImp::ProCom_SetBoxEpitaxyDistance);
	}
	void EchoBoxComponentProFuncImp::uninitProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->unregisterProtocolExtFunc(Name("SetBoxEpitaxyDistance"));
	}
	Echo::ProtocolResult EchoBoxComponentProFuncImp::ProCom_SetBoxEpitaxyDistance(void* distance_, ProtocolHandle comHandle)
	{
		if (comHandle == 0)
			return PR_Falied;
		BoxComponent* boxCom = (BoxComponent*)comHandle;
		if (boxCom == nullptr)
			return PR_Falied;
		boxCom->setEpitaxyDistance(boxCom->getEpitaxyDistance());
		return PR_OK;
	}
}
