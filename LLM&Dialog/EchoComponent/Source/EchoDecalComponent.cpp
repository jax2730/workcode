//-----------------------------------------------------------------------------
// File:  EchoDecalComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-8-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoDecalComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "EchoResourceFolderManager.h"
namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(DecalComponent)
	DecalComponent::DecalComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		:NodeActorComponent(pActor, movableType)
	{
		
	}

	DecalComponent::~DecalComponent()
	{
	}

	void DecalComponent::init(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		DecalComponentInfo* mDecalInfo = static_cast<DecalComponentInfo*>(m_pActComInfo);
		//if (mDecalInfo == NULL)
		//	return;

		if (mHelperObj == nullptr)
			CreateHelper(getActor()->isSyncLoadActor());

		if(!mDecalInfo->m_PefFile.empty())
			sPefFile = mDecalInfo->m_PefFile;
		if (!mDecalInfo->m_DecalPath.empty())
			sDecalTexture = mDecalInfo->m_DecalPath;

		if (mParticleHolder == nullptr)
			CreateDecal(getActor()->isSyncLoadActor());

		changeDiffuseTexture(sDecalTexture, getActor()->isSyncLoadActor());
		if(mDecalInfo != nullptr)
		{
			SetParticleColor(mDecalInfo->m_color);
			ShowDecal(mDecalInfo->m_ShowDecal);
		}

		//if (mHelperObj != NULL)
		//	addChildToNode(mHelperObj->getSceneNode());

		UpdateComRootNode();
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Editor)
			SetNotifiedUpdate(true);
	}

	void DecalComponent::unInit()
	{
		ReleaseParticleHolder();
		ReleaseHelper();
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Editor)
			SetNotifiedUpdate(false);
	}

	void DecalComponent::bindNode()
	{
	}

	void DecalComponent::replay(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		DecalComponentInfo* mDecalInfo = static_cast<DecalComponentInfo*>(m_pActComInfo);
		//if (mDecalInfo == NULL)
		//	return;

		if (mHelperObj == nullptr)
			CreateHelper(getActor()->isSyncLoadActor());

		if (!mDecalInfo->m_PefFile.empty())
			sPefFile = mDecalInfo->m_PefFile;
		if (!mDecalInfo->m_DecalPath.empty())
			sDecalTexture = mDecalInfo->m_DecalPath;

		if (mParticleHolder == nullptr)
			CreateDecal(getActor()->isSyncLoadActor());

		changeDiffuseTexture(sDecalTexture, getActor()->isSyncLoadActor());
		if (mDecalInfo != nullptr)
		{
			SetParticleColor(mDecalInfo->m_color);
			ShowDecal(mDecalInfo->m_ShowDecal);
		}

		//if (mHelperObj != NULL)
		//	addChildToNode(mHelperObj->getSceneNode());

		UpdateComRootNode();
	}

	void DecalComponent::copyTo(const ActorComponentPtr&  actCom)
	{
	}

	void DecalComponent::showEditorHelper(bool isShow)
	{
		IsShowEditorHelper = isShow;
		ShowHelper(bShowHelper);
	}

	void DecalComponent::updateCombinedName()
	{
		if (mHelperObj == NULL)
			return;
		Name helperName = Name(getCombinedName() + "||" + "Helper");
		mHelperObj->setName(helperName);
	}

	void DecalComponent::NotifiChildImp(int changeMask)
	{
		(void)changeMask;
		if (mHelperObj == nullptr)
			return;

		mHelperObj->setPosition(getWorldPosition());
		mHelperObj->setScale(getWorldScale());
		mHelperObj->setOrientation(getWorldRotation());
	}

	void DecalComponent::ShowHelper(bool isShow)
	{
		if (mHelperObj == NULL)
			CreateHelper(getActor()->isSyncLoadActor());
		if (mHelperObj == NULL)
			return;
		if (IsShowEditorHelper)
			mHelperObj->setVisible(isShow);
		else
			mHelperObj->setVisible(false);
		bShowHelper = isShow;
	}

	void DecalComponent::ShowDecal(bool isShow)
	{
		if (mHelperObj == NULL)
			CreateHelper();
		if (mParticleHolder == NULL)
			CreateDecal();
		if (mParticleHolder == nullptr)
			return;

		mParticleHolder->setVisible(isShow);
		bShowDecal = isShow;
	}

	void DecalComponent::UpdateDecalWithBox(bool synchronous/* = false*/)
	{
		if (m_pParentNodeCom != nullptr)
			return;
		CreateHelper(synchronous);
		CreateParticleHolder(synchronous);
		//UpdateVertex();	
		UpdateComRootNode();
	}

	void DecalComponent::UpdateBox(bool synchronous/* = false*/)
	{
		if (m_pParentNodeCom != nullptr)
			return;
		CreateHelper(synchronous);
		//UpdateVertex();
		UpdateComRootNode();
	}

	void DecalComponent::CreateDecal(bool synchronous /*= false*/)
	{
		CreateParticleHolder(synchronous);
	}

	void DecalComponent::setActor(const ActorPtr&  pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	Name decalColorName_ = Name("DecalColor");

	void DecalComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		NodeActorComponent::setTimelineAttribute(data, name);
		
		if (name == decalColorName_)
			SetParticleColor(Vector4(data[0], data[1], data[2], data[3]));
	}
	void DecalComponent::CreateHelper(bool synchronous /*= false*/)
	{
		if (mHelperObj != NULL)
			return;
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client)
			return;

		Name helperName = Name(getCombinedName() + "||" + "Helper");
		mHelperObj = new HelperObject(helperName, HelperObjType_Box, synchronous, nullptr, nullptr, true);

		setDecalType(HelperObjType_Box);
		setDecalRenderType(ECHO_LINE_STRIP);
		setColor(mHelperDesc.color);
		mHelperDesc.customSync = synchronous;
		if (mHelperDesc.type == HelperObjType_Custom)
			mHelperObj->changeCustomMesh(mHelperDesc.customName, mHelperDesc.customSync);
		else if (mHelperDesc.type == HelperObjType_CustomPolygon)
			mHelperObj->changeCustomPolygon(mHelperDesc.customVBVec, mHelperDesc.customIBVec);

		mHelperObj->setVisible(false);

	}

	void DecalComponent::ReleaseHelper()
	{
		if (mHelperObj == NULL)
			return;
		/*if (m_pParentNodeCom != NULL && getParentNode() != NULL)
			getParentNode()->destroyChild(mHelperObj->getSceneNode());*/
		//Echo::Root::instance()->getMainSceneManager()->destroyManualObject(mHelperObj);
		delete mHelperObj;
		mHelperObj = NULL;
		bShowHelper = false;
	}

	void DecalComponent::CreateParticleHolder(bool synchronous /*= false*/)
	{
		if (mParticleHolder != NULL)
			return;
		if (mHelperObj == NULL || mHelperObj->getSceneNode() == NULL)
			return;

		mParticleHolder = new EffectHolder(Name(""), Name(sPefFile), Echo::Root::instance()->getMainSceneManager(), IsPrior, true, synchronous);
		mParticleHolder->setCameraCullMode(DistanceCull);
		mParticleHolder->setPosition(0.0f, -0.001f, 0.0f);
		mParticleHolder->setVisible(bShowDecal);
		if (mParticleHolder != NULL)
		{
			addChildToNode(mParticleHolder->getSceneNode());
			UpdateComRootNode();
		}
	}

	void DecalComponent::ChangeParticleHolder(bool synchronous/* = false*/)
	{
		ReleaseParticleHolder();
		CreateParticleHolder(synchronous);
	}

	void DecalComponent::SetParticleColor(const Vector4 & _color)
	{
		if (mParticleHolder == NULL || _color == Vector4(0, 0, 0, 1))
			return;
		ColorValue mColor;
		mColor.r = _color.x;
		mColor.g = _color.y;
		mColor.b = _color.z;
		mColor.a = _color.w;
		mParticleHolder->SetColor(mColor);
	}

	Vector4 DecalComponent::GetParticleColor()
	{
		//DecalComponentInfo* minfo = static_cast<DecalComponentInfo*>(m_pActComInfo);
		if(m_pActComInfo == NULL)
			return Vector4(0,0,0,1);
		DecalComponentInfo* mDecalInfo = static_cast<DecalComponentInfo*>(m_pActComInfo);
		return mDecalInfo->m_color;
	}

	void DecalComponent::ReleaseParticleHolder()
	{
		if (mParticleHolder == NULL)
			return;

		if (mHelperObj != NULL && mHelperObj->getSceneNode() != NULL)
			mHelperObj->getSceneNode()->destroyChild(mParticleHolder->getSceneNode());
		mParticleHolder = NULL;
		bShowDecal = false;
	}

	void DecalComponent::setColor(HelperObjColorType color)
	{
		if (mHelperObj == NULL)
			return;
		mHelperDesc.color = color;
		mHelperObj->setHelperColorType(color);
	}

	void DecalComponent::setShowDecal(bool isShow)
	{
		bShowDecal = isShow;
	}

	void DecalComponent::setPefFile(const String & path)
	{
		sPefFile = path;
	}

	String DecalComponent::getPefFile()
	{
		return sPefFile;
	}

	void DecalComponent::setDecalType(HelperObjType type)
	{
		if (mHelperObj == NULL)
			return;
		mHelperDesc.type = type;
		mHelperObj->changeHelperType(type);
	}

	void DecalComponent::setDecalRenderType(ECHO_DRAW rType)
	{
		if (mHelperObj == NULL)
			return;
		mHelperObj->setRenderType(rType);
		mHelperDesc.renderType = rType;
	}

	void DecalComponent::setDecalPath(const String & path)
	{
		sDecalTexture = path;
	}

	String DecalComponent::getDecalPath()
	{
		return sDecalTexture;
	}

	void DecalComponent::ShowPrior(bool isPrior)
	{
		IsPrior = isPrior;
		ChangeParticleHolder();
	}

	//void DecalComponent::setPosition(const Vector3 & pos)
	//{
	//	mHelperObj->setPosition(pos);
	//}

	//Vector3 DecalComponent::getPosition()
	//{
	//	return mHelperObj->getPosition();
	//}

	//void DecalComponent::setOrientation(const Quaternion & q)
	//{
	//	mHelperObj->setOrientation(q);
	//}

	//Quaternion DecalComponent::getOrientation()
	//{
	//	return mHelperObj->getOrientation();
	//}

	//void DecalComponent::setScale(const Vector3 & scale)
	//{
	//	mHelperObj->setScale(scale);
	//}

	//Vector3 DecalComponent::getScale()
	//{
	//	return mHelperObj->getScale();
	//}

	void DecalComponent::changeDiffuseTexture(const String & path, bool synchronous /*= false*/)
	{	
		if (mParticleHolder == NULL)
			return;
		mParticleHolder->ChangeDiffuseTexture(path, "", synchronous);
		sDecalTexture = path;
	}

	const std::vector<Vector3>& DecalComponent::getHelperPointLocalPos()
	{
		if (mHelperObj == NULL)
			return mHelperPointLocal;
		mHelperObj->getPointsLocalPos(mHelperPointLocal);
		return mHelperPointLocal;
	}

	const std::vector<Vector3>& DecalComponent::getHelperPointWorldPos()
	{
		if (mHelperObj == NULL)
			return mHelperPointWorld;
		mHelperObj->getPointsWorldPos(mHelperPointWorld);
		return mHelperPointWorld;
	}


	void EchoDecalProFuncImp::initProtocolFunction()
	{
		/*auto p = ProtocolComponentSystem::instance();
		p->registerFactory(COMPONENTTYPE(DecalComponent), new DecalComponentFactory());*/
	}

	void EchoDecalProFuncImp::uninitProtocolFunction()
	{

	}

	EchoDecalScript EchoDecalScript::Script_GetDecalComponent(const EchoActorScript& pActor, const std::string & name)
	{
		//if (pActor == NULL)
		//	return NULL;
		//return (DecalComponent*)pActor->getComponent(Name(name));
		return EchoDecalScript(pActor, name);
	}

	void EchoDecalScript::changeDiffuseTexture(const std::string & path)
	{
		DecalComponent* decalCom = static_cast<DecalComponent*>(m_Ptr.get());
		if (decalCom) {
			return decalCom->changeDiffuseTexture(path);
		}
	}

	EDITOR_REFLECT_BEGIN(DecalComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_PefFile, "PefFile", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DecalPath, "DecalPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ShowDecal, "ShowDecal", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsPrior, "IsPrior", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_color, "color", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	DecalComponentInfo::DecalComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType)
	{
	}

	DecalComponentInfo::~DecalComponentInfo()
	{
		//Root::instance()->getRootResourcePath();
	}

	void DecalComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<DecalComponentInfo>::get()->ExportJson(this, jsonNode, "");
		//cJSON_AddStringToObject(jsonNode, "DiffuseTexture", m_strDecalPath.c_str());
		//cJSON_AddStringToObject(jsonNode, "Particle_PefFile", m_strPefFile.c_str());

		//cJSON_AddBoolToObject(jsonNode, "ShowDecal", m_bShowDecal);
		//cJSON_AddBoolToObject(jsonNode, "IsPrior", m_bIsPrior);

		//cJSON* _color = cJSON_CreateObject();
		//cJSON_AddItemToObject(jsonNode, "Color", _color);
		//cJSON_AddNumberToObject(_color, "R", m_v4color.x);
		//cJSON_AddNumberToObject(_color, "G", m_v4color.y);
		//cJSON_AddNumberToObject(_color, "B", m_v4color.z);
		//cJSON_AddNumberToObject(_color, "A", m_v4color.w);

		//SAVECOMINFO_STRING(DecalComponent, DecalPath);
		//SAVECOMINFO_STRING(DecalComponent, PefFile);
		//SAVECOMINFO_BOOL(DecalComponent, ShowDecal);
		//SAVECOMINFO_BOOL(DecalComponent, IsPrior);
		//SAVECOMINFO_VECTOR4(DecalComponent, color);
	}

	void DecalComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<DecalComponentInfo>::get()->ImportJson(this, jsonNode, "");
		//ReadJSNode(jsonNode, "DiffuseTexture", m_strDecalPath);
		//ReadJSNode(jsonNode, "Particle_PefFile", m_strPefFile);
		//ReadJSNode(jsonNode, "ShowDecal", m_bShowDecal);
		//ReadJSNode(jsonNode, "IsPrior", m_bIsPrior);

		//cJSON* _color = cJSON_GetObjectItem(jsonNode, "Color");
		//if (_color)
		//{
		//	ReadJSNode(_color, "R", m_v4color.x);
		//	ReadJSNode(_color, "G", m_v4color.y);
		//	ReadJSNode(_color, "B", m_v4color.z);
		//	ReadJSNode(_color, "A", m_v4color.w);
		//}

		//LOADCOMINFO_STRING(DecalPath);
		//LOADCOMINFO_STRING(PefFile);
		//LOADCOMINFO_BOOL(ShowDecal);
		//LOADCOMINFO_BOOL(IsPrior);
		//LOADCOMINFO_VECTOR4(color);
	}
	void DecalComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		DecalComponentInfo* decalInfo = static_cast<DecalComponentInfo*>(comInfo);
		reflect::TypeResolver<DecalComponentInfo>::get()->copyTo(this, decalInfo);

		decalInfo->m_ShowDecal = this->m_ShowDecal;
		decalInfo->m_IsPrior = this->m_IsPrior;
		decalInfo->m_PefFile = this->m_PefFile;
		decalInfo->m_DecalPath = this->m_DecalPath;
		decalInfo->m_color = this->m_color;
	}
	void DecalComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
		/*if (comInfo == nullptr || comInfo->m_Type != m_Type)
			return;
		DecalComponentInfo* pInfo = static_cast<DecalComponentInfo*> (comInfo);
		OVERLAY_PROP(this, pInfo, PefFile);
		OVERLAY_PROP(this, pInfo, DecalPath);
		OVERLAY_PROP(this, pInfo, ShowDecal);
		OVERLAY_PROP(this, pInfo, IsPrior);
		OVERLAY_PROP(this, pInfo, color);*/
	}
	void DecalComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			DecalComponentInfo* oriInfo = static_cast<DecalComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<DecalComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool DecalComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_DecalPath.empty() && m_PefFile.empty()) {
			return false;
		}
		if (!m_PefFile.empty())
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_PefFile));
		if (!m_DecalPath.empty())
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_DecalPath));
		return true;
	}
#endif
}
