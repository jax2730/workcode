// Author: anhaoxiang
#include "EchoComponentStableHeaders.h"
#include "EchoLightComponent.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoActor.h"
#include "EchoSceneManager.h"
#include "EchoEntity.h"
#include "EchoTimelineComponent.h"
#include "EchoStringTable.h"

//#define PointLightMesh "Editor/editorResources/cx_dengpao_001.mesh"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(LightComponent)

		EDITOR_REFLECT_BEGIN(LightComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOriginColor, "mOriginColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mInRadius, "InRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOutRadius, "OutRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIntensity, "intensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseTod, "IsUseTod", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsProduceShadow, "IsProduceShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()


		String LightComponent::POINTLIGHTMESH = ""; 


	LightComponent::LightComponent(ActorPtr pActor, ActorCOMMoveableType isMoveable)
		: NodeActorComponent(pActor, isMoveable)
	{
		setActorComActiveType(ActorComActiveType::LIGHT_BIT);
		if (POINTLIGHTMESH.empty())
			POINTLIGHTMESH = StringTable::instance()->getString("Light", "PointLightMesh");
	}

	LightComponent::~LightComponent()
	{
	}



	void LightComponent::init(ActorComponentInfo * info)
	{
		if (info != nullptr)
		{
			LightComponentInfo* mPointLightInfo = static_cast<LightComponentInfo*>(info);
			mContext = mPointLightInfo->mContext;
		}
		
	}

	void LightComponent::unInit()
	{
		SceneManager* p_sMgr = getSceneManager();
		if (mPointLight != nullptr && p_sMgr)
		{
			p_sMgr->destroyLight(mPointLight);
			mPointLight = nullptr;
		}
#ifdef ECHO_EDITOR
		destroyHelperAndLightEntity();
		SetNotifiedUpdate(false);
#endif
	}

	void LightComponent::bindNode()
	{
	}

	void LightComponent::initRenderObject(ActorComponentInfo* info)
	{
		SceneManager* scMgr = getSceneManager();
		if (!scMgr)
			return;
		if (mPointLight)
			scMgr->destroyLight(mPointLight);
		mPointLight = scMgr->createLight(Name(getCombinedName() + "||Light"));
		
		if (mPointLight != nullptr)
		{
			const Vector4& color = mContext.mOriginColor;
			mPointLight->setDiffuseColour(ColorValue(color.x, color.y, color.z, color.w));
			mPointLight->setAttenuation(mContext.mInRadius, mContext.mOutRadius);
			mPointLight->setIlluminatePriority(mContext.mPriority);
			mPointLight->setPowerScale(mContext.mIntensity);
			mPointLight->setUseTODChange(mContext.mIsUseTod);
			mPointLight->setIsProduceShadow(mContext.mIsProduceShadow);
			mPointLight->setVisible(getRealVisibleState());
			attachToNode(mPointLight);
		}
	}

	void LightComponent::initHelperObject(ActorComponentInfo* info)
	{
		
#ifdef ECHO_EDITOR
		SceneManager* scMgr = getSceneManager();
		if(scMgr)
		{
			if (mEntity)
				scMgr->destroyEntity(mEntity);
			if (childNode_)
				scMgr->destroySceneNode(childNode_);
			mEntity = scMgr->createEntity(Name(getCombinedName() + "||" + POINTLIGHTMESH), Name(POINTLIGHTMESH));
			childNode_ = scMgr->createSceneNode(Name(getCombinedName() + "||" + "dengpao_"));

			if (childNode_ && mEntity)
			{
				mEntity->setQueryFlags(m_QtFlags);
				childNode_->attachObject(mEntity);
				childNode_->setScale(0.3f, 0.3f, 0.3f);
				childNode_->setInheritScale(false);
				addChildToNode(childNode_);
			}
			if (mHelperObj) {
				delete mHelperObj;
				mHelperObj = nullptr;
			}
			CreateHelper();
			showHelper(true);
			showLightEnity(true);
			if (mHelperObj)
			{
				mHelperObj->getSceneNode()->setInheritScale(false);
				setHelperRadius();
				SetNotifiedUpdate(true);
			}
		}
		UpdateComRootNode();
#endif
	}

	void LightComponent::createComRootNodeImpl()
	{
		SceneManager* p_sMgr = getSceneManager();
		if(p_sMgr)
		   mComRootNode = p_sMgr->createSceneNode();
	}

#ifdef ECHO_EDITOR
	void LightComponent::setPointLightSelectedColor()
	{
		if (mHelperObj)
			mHelperObj->setHelperColorType(HelperObjColorType::HelperObjColorType_Red);
	}

	void LightComponent::restorePointLightHelperColor()
	{
		if (mHelperObj)
			mHelperObj->setHelperColorType(HelperObjColorType::HelperObjColorType_white);
	}
#endif

	Name pointLightRadiusName_ = Name("PointLightRadius");
	Name pointLightColorName_ = Name("PointLightColor");
	Name pointLightIntensityName_ = Name("PointLightIntensity");

	void LightComponent::setTimelineAttribute(std::vector<float>& data,const Name& name)
	{
		NodeActorComponent::setTimelineAttribute(data, name);
		
		if (name == pointLightRadiusName_)
		{
			setRadius(data[0], data[1]);
		}
		else if (name == pointLightColorName_)
		{
			setPointLightColor(Vector4(data[0],data[1],data[2],data[3]));
		}
		else if (name == pointLightIntensityName_)
		{
			setLightIntensity(data[0]);
		}
	}

	void LightComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}

	void LightComponent::showEditorHelper(bool isShow)
	{
#ifdef ECHO_EDITOR
           showHelper(isShow);
		   showLightEnity(isShow);
#endif	
	}

	void LightComponent::updateCombinedName()
	{
		if (mPointLight)
		{
			Echo::Name lightName = Name(getCombinedName().append("||Light"));
			mPointLight->setName(lightName);
		}

#ifdef ECHO_EDITOR
		if (mHelperObj)
		{
			Echo::Name helperName = Name(getCombinedName() + "||" + "Helper");
			mHelperObj->setName(helperName);
		}
		if (mEntity && childNode_)
		{
			Echo::Name childName = Name(getCombinedName() + "||" + "dengpao_");
			childNode_->setName(childName);
			Echo::Name entityName = Name(getCombinedName() + "||" + POINTLIGHTMESH);
			mEntity->setName(entityName);
		}
#endif
	}

	void LightComponent::visibleStateChanged(bool isVisible)
	{
		if (mPointLight == nullptr)
			return;
		mPointLight ->setVisible(isVisible);
	}

	void LightComponent::NotifiChildImp(int changeMask)
	{
		(void)changeMask;

#ifdef ECHO_EDITOR
		if (!mHelperObj)
			return;

		mHelperObj->setPosition(getWorldPosition());
		setHelperRadius();
		mHelperObj->setOrientation(getWorldRotation());
#endif
	}

	void Echo::LightComponent::setPointLightColor(const Vector4 & color)
	{
		mContext.mOriginColor = color;
		if (mPointLight == nullptr)
			return;
	    mPointLight->setDiffuseColour(ColorValue(color.x,color.y,color.z,color.w));
	}

	const Vector4& Echo::LightComponent::getPointLightColor() 
	{
		return mContext.mOriginColor;
	}

	void Echo::LightComponent::setLightIntensity(float intensity_)
	{
		mContext.mIntensity = intensity_;
		if (mPointLight == nullptr)
			return;
		mPointLight->setPowerScale(intensity_);
	}

	float  Echo::LightComponent::getLightIntensity() 
	{
	    return  mContext.mIntensity;
	}

	void LightComponent::setRadius(const float& in_, const float& out_)
	{
		mContext.mInRadius = in_;
		mContext.mOutRadius = out_;
		if (mPointLight == nullptr)
			return;
		mPointLight->setAttenuation(in_, out_);

#ifdef ECHO_EDITOR
		setHelperRadius();
#endif
	}

	void LightComponent::setInRadius(const float& in_)
	{
		mContext.mInRadius = in_;
		if (mPointLight == nullptr)
			return;
		mPointLight->setAttenuationInnerRadiu(in_);

	}

	void LightComponent::setOutRadius(const float & out_)
	{
		mContext.mOutRadius = out_;
		if (mPointLight == nullptr)
			return;
		mPointLight->setAttenuationOuterRadiu(out_);

#ifdef ECHO_EDITOR
		setHelperRadius();
#endif

	}

	float LightComponent::getInRadius()
	{
		return mContext.mInRadius;;
	}

	float LightComponent::getOutRadius()
	{
		return mContext.mOutRadius;
	}

	void LightComponent::setPriority(LightIlluminatePriority priority_)
	{
		mContext.mPriority = priority_;
		if (mPointLight == nullptr)
			return;
		mPointLight->setIlluminatePriority(priority_);
	}

	LightIlluminatePriority LightComponent::getPriority()
	{
		return mContext.mPriority;
	}

	void LightComponent::setIsUseTod(bool isUse_)
	{
		mContext.mIsUseTod = isUse_;
		if (mPointLight)
			mPointLight->setUseTODChange(isUse_);
	}

	void LightComponent::setIsProduceShadow(bool isUse_)
	{
		mContext.mIsProduceShadow = isUse_;
		if (mPointLight)
			mPointLight->setIsProduceShadow(isUse_);
	}

#ifdef ECHO_EDITOR
	void LightComponent::CreateHelper()
	{
		if (mHelperObj != nullptr)
			return;

		Name helperName;
		if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			helperName = Name(getCombinedName() + "||" + "Helper");
		mHelperObj = new HelperObject(helperName, HelperObjType_Sphere, false, NULL, getSceneManager());
		if (mHelperObj)
		{
			mHelperObj->setRenderType(ECHO_LINE_STRIP);
			mHelperObj->setHelperColorType(HelperObjColorType_white);
			UpdateComRootNode();
		}
	}

	void LightComponent::destroyHelperAndLightEntity()
	{
		SceneManager* p_sMgr = getSceneManager();
		if (mHelperObj != nullptr)
			delete mHelperObj;
		mHelperObj = nullptr;
		if (childNode_ != nullptr && p_sMgr)
		{
			p_sMgr->destroySceneNode(childNode_);
		}
		childNode_ = nullptr;
		if (mEntity && p_sMgr)
			p_sMgr->destroyEntity(mEntity);
		mEntity = nullptr;
	}
#ifdef ECHO_EDITOR
	void LightComponent::setHelperRadius()
	{
		if (mPointLight && mHelperObj)
		{
			float  temp;
			temp = 2 * mPointLight->getAttenuationOutRadius();
			Vector3 scale = Vector3(temp, temp, temp);
			mHelperObj->setScale(scale);
		}
	}
#endif

	void LightComponent::showHelper(bool isshow_)
	{
		if (mHelperObj == nullptr)
			return;
	    mHelperObj->setVisible(isshow_);
	}

	void LightComponent::showLightEnity(bool isshow_)
	{
		if (childNode_ == nullptr || mEntity == nullptr)
			return;
	    childNode_->setVisible(isshow_);
		mEntity->setVisible(isshow_);
	}
#endif
	
	EchoLightScript::EchoLightScript(const EchoActorScript& pActor, const std::string& name)
		:EchoActorComponentScript(pActor, name)
	{
	}

	EchoLightScript::EchoLightScript(const ActorComponentPtr& scriptCom)
		: EchoActorComponentScript(scriptCom)
	{
	}

	EchoLightScript  Echo::EchoLightScript::Script_GetLightComponent(ActorPtr pActor, const std::string & name)
	{
		return EchoLightScript(pActor, name);
	}
	LightComponentInfo::LightComponentInfo(const Name& name,ActorCOMMoveableType moveable)
		: ActorComponentInfo(name,moveable)
	{
		
	}

	LightComponentInfo::~LightComponentInfo()
	{
	}

	void LightComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<LightComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void LightComponentInfo::ImportData(cJSON * jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<LightComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void LightComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void LightComponentInfo::CopyTo(ActorComponentInfo * info)
	{
		if (info == nullptr)
			return;
		auto newObjectInfo = static_cast<LightComponentInfo*>(info);
        reflect::TypeResolver<LightComponentInfo>::get()->copyTo(this,newObjectInfo);
	}

	void LightComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<LightComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

}

