#include "EchoComponentStableHeaders.h"
#include "EchoSpotLightComponent.h"
#include "EchoEntity.h"
#include "Actor/EchoActor.h"
#include "EchoStringTable.h"

//#define PointLightMesh "Editor/editorResources/cx_dengpao_001.mesh"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(SpotLightComponent)

	EDITOR_REFLECT_BEGIN(SpotLightComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColor, "mColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mInRadius, "InRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOutRadius, "OutRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIntensity, "intensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mInnerAngle, "InnerAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOuterAngle, "OuterAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDirection, "Direction", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseTodChange, "IsUseTodChange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsProduceShadow, "IsProduceShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

		String SpotLightComponent::POINTLIGHTMESH = "";

    Name spotLightColorName = Name("SpotLightColor");
	Name spotLightInRadiusName = Name("SpotLightInRadius");
	Name spotLightOutRadiusName = Name("SpotLightOutRadius");
	Name spotLightIntensityName = Name("SpotLightIntensity");
	Name spotLightInnerAngleName = Name("SpotLightInnerAngle");
	Name spotLightOuterAngleName = Name("SpotLightOuterAngle");

	SpotLightComponent::SpotLightComponent(ActorPtr pActor, ActorCOMMoveableType isMoveable)
		: NodeActorComponent(pActor, isMoveable)
	{
		setActorComActiveType(ActorComActiveType::LIGHT_BIT);
		if (POINTLIGHTMESH.empty())
			POINTLIGHTMESH = StringTable::instance()->getString("Light", "PointLightMesh");
	}
	SpotLightComponent::~SpotLightComponent()
	{

	}
	void SpotLightComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
			mContext = static_cast<SpotLightComponentInfo*>(info)->mContext;
	}
	void SpotLightComponent::unInit()
	{
		if (mSpotLight != nullptr)
		{
			SceneManager* scMgr = getSceneManager();
			if(scMgr)
				scMgr->destroyLight(mSpotLight);
		}
		mSpotLight = nullptr;
#ifdef ECHO_EDITOR
		destroyHelper();
		SetNotifiedUpdate(false);
#endif
	}
	void  SpotLightComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}
	void SpotLightComponent::showEditorHelper(bool isShow)
	{
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			showHelper(isShow);
		}
	}
	void SpotLightComponent::updateCombinedName()
	{
		if (mSpotLight)
		{
			Echo::Name lightName = Name(getCombinedName().append("||Light"));
			mSpotLight->setName(lightName);
		}
	}
	void SpotLightComponent::visibleStateChanged(bool isVisible)
	{
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setVisible(isVisible);
	}

	void SpotLightComponent::NotifiChildImp(int changeMask)
	{
		(void)changeMask;

#ifdef ECHO_EDITOR
		if (!mHelperObj)
			return;

		auto node = mHelperObj->getParentSceneNode();

		node->_setDerivedPosition(getWorldPosition());
		node->_setDerivedOrientation(getWorldRotation());
		node->_setDerivedScale(getWorldScale());
		node->_update(true);
#endif
	}

	void SpotLightComponent::initRenderObject(ActorComponentInfo* info)
	{
		SceneManager* scMgr = getSceneManager();
		if (scMgr == nullptr)
			return;
		if (mSpotLight)
			scMgr->destroyLight(mSpotLight);
		mSpotLight = scMgr->createLight(Name(getCombinedName() + "||Light"));
		if (mSpotLight)
		{
			mSpotLight->setVisible(getRealVisibleState());
			mSpotLight->setType(Light::LT_SPOTLIGHT);
			attachToNode(mSpotLight);
		}
		remakeSate();
	}

	void SpotLightComponent::initHelperObject(ActorComponentInfo* info)
	{
#ifdef ECHO_EDITOR
		createHelper();
		drawSpotLight();
		UpdateComRootNode();
		SetNotifiedUpdate(true);
#endif
	}

	void SpotLightComponent::setSpotLightColor(const ColorValue& color)
	{
		mContext.mColor = color;
		if (mSpotLight == nullptr)
			return ;
		mSpotLight->setDiffuseColour(color);
	}
	const ColorValue& SpotLightComponent::getSpotLightColor()
	{
		return mContext.mColor;
	}
	void SpotLightComponent::setIntensity(float intensity_)
	{
		mContext.mIntensity = intensity_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setPowerScale(intensity_);
	}
	float SpotLightComponent::getIntensity()
	{
		return mContext.mIntensity;
	}
	void SpotLightComponent::setInRadius(float in_)
	{
		mContext.mInRadius = in_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setAttenuationInnerRadiu(in_);
	}
	void SpotLightComponent::setOutRadius(float out_)
	{
		mContext.mOutRadius = out_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setAttenuationOuterRadiu(out_);

#ifdef ECHO_EDITOR
		mOutRadius_ = mSpotLight->getAttenuationOutRadius();
	    drawSpotLight();
#endif
	}
	float SpotLightComponent::getInRadius()
	{
		return  mContext.mInRadius;
	}
	float SpotLightComponent::getOutRadius()
	{
		return  mContext.mOutRadius;
	}
	void SpotLightComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setIlluminatePriority(static_cast<LightIlluminatePriority>(priority_));
	}
	int SpotLightComponent::getPriority()
	{ 
		return static_cast<int>(mContext.mPriority);
	}
	void SpotLightComponent::setInnerAngle(float in_)
	{
		in_ = std::max(0.f, std::min(180.f, in_));

		mContext.mInnerAngle = in_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setSpotlightInnerAngle(Degree(in_));

#ifdef ECHO_EDITOR
		mInnerAngle_ = Degree(in_);
		drawSpotLight();
#endif
	}
	float SpotLightComponent::getInnerAngle()
	{
		return mContext.mInnerAngle;
	}
	void SpotLightComponent::setOuterAngle(float out_)
	{
		out_ = std::max(0.f, std::min(180.f, out_));

		mContext.mOuterAngle = out_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setSpotlightOuterAngle(Degree(out_));

#ifdef ECHO_EDITOR
		mOuterAngle_ = Degree(out_);
		drawSpotLight();
#endif
	}
	float SpotLightComponent::getOuterAngle()
	{
		return mContext.mOuterAngle;
	}
	void SpotLightComponent::setDirection(const Vector3& direction_)
	{
		mContext.mDirection = direction_;
		if (mSpotLight == nullptr)
			return;
		mSpotLight->setDirection(direction_);
	}
	Vector3 SpotLightComponent::getDirection()
	{
		return mContext.mDirection;
	}
	void SpotLightComponent::showHelper(bool isshow_)
	{
#ifdef ECHO_EDITOR
		if (mHelperObj == nullptr)
			return;
		mHelperObj->setVisible(isshow_);
#endif
	}

	void SpotLightComponent::setIsUseTodChange(bool isChange_)
	{
		mContext.mIsUseTodChange = isChange_;
		if (mSpotLight)
			mSpotLight->setUseTODChange(isChange_);
	}

	void SpotLightComponent::setIsProduceShadow(bool isUse_)
	{
		mContext.mIsProduceShadow = isUse_;
		if (mSpotLight)
			mSpotLight->setIsProduceShadow(isUse_);
	}
#ifdef ECHO_EDITOR
	void SpotLightComponent::destroyHelper()
	{
		SceneManager* scMgr = getSceneManager();
		if (mHelperObj && mHelperNode)
		{
			mHelperNode->detachObject(mHelperObj);
			if (scMgr)
			{
				scMgr->destroyManualObject(mHelperObj);
				scMgr->destroySceneNode(mHelperNode);
			}
		}
		mHelperObj = nullptr;
		mHelperNode = nullptr;
	}
	void SpotLightComponent::createHelper()
	{
		SceneManager* scMgr = getSceneManager();

		if (!scMgr ||  Root::instance()->getRuntimeMode() != Root::eRM_Editor)
			return;

		destroyHelper();
		std::string manualObjName_ = getCombinedName() + "|| spotLight";
		std::string tempName = manualObjName_;
		int idx = 0;
		while (scMgr->getManualObject(Name(tempName.data())) != nullptr)
		{
			tempName = manualObjName_;
			idx = idx + 1;
			String temp = std::to_string(idx);
			tempName = tempName + temp;
		}
		mHelperObj = scMgr->createManualObject(Name(tempName.data()));

		mHelperObj->begin("BaseWhiteNoLight_VS.txt", "BaseWhiteNoLight_PS.txt"/*LVHLightTemplate, LVHLightTemplate*/, Echo::ECHO_LINE_STRIP);
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->end();

		mHelperObj->begin("BaseWhiteNoLight_VS.txt", "BaseWhiteNoLight_PS.txt"/*LVHLightTemplate, LVHLightTemplate*/, Echo::ECHO_LINE_STRIP);
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->end();

		mHelperObj->begin("BaseWhiteNoLight_VS.txt", "BaseWhiteNoLight_PS.txt"/*LVHLightTemplate, LVHLightTemplate*/, Echo::ECHO_LINE_STRIP);
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->end();

		mHelperObj->begin("BaseWhiteNoLight_VS.txt", "BaseWhiteNoLight_PS.txt"/*LVHLightTemplate, LVHLightTemplate*/, Echo::ECHO_LINE_STRIP);
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->end();

		mHelperObj->begin("BaseWhiteNoLight_VS.txt", "BaseWhiteNoLight_PS.txt"/*LVHLightTemplate, LVHLightTemplate*/, Echo::ECHO_LINE_LIST);
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->position(0, 0, 0);  // dummy
		mHelperObj->colour(1, 1, 1);    // dummy
		mHelperObj->end();
		
		//attachToNode(mHelperObj);

		if (mHelperObj->getParentSceneNode())
		{
			mHelperObj->getParentSceneNode()->detachObject(mHelperObj);
		}
		mHelperNode = scMgr->createSceneNode();
		mHelperNode->attachObject(mHelperObj);
		mHelperNode->_setDerivedPosition(getWorldPosition());
		mHelperNode->_setDerivedScale(getWorldScale());
		mHelperNode->_setDerivedOrientation(getWorldRotation());
		mHelperNode->_update(true);
			
	}

	void SpotLightComponent::drawCircle(const float radius, const float zOffset, const Echo::ColorValue& colour)
	{
		for (unsigned int i = 0; i <= gCircleVertexCount; i++)
		{
			float x = Echo::Math::Cos(Echo::Radian((Echo::Math::TWO_PI * i) / gCircleVertexCount)) * radius;
			float y = Echo::Math::Sin(Echo::Radian((Echo::Math::TWO_PI * i) / gCircleVertexCount)) * radius;
			mHelperObj->position(x, y, zOffset);
			mHelperObj->colour(colour);
		}
	}
#endif
	void SpotLightComponent::drawSpotLight()
	{
#ifdef ECHO_EDITOR
		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
			return;
		if (mHelperObj == nullptr || mSpotLight == nullptr)
			return;
		float mSpotlightStartCircleOffset = 0.65f;
		Echo::Radian outerAngle, innerAngle;
		if (mOuterAngle_ < Echo::Radian(0.0f))
		{
			outerAngle = 0.0f;
			innerAngle = 0.0f;
		}
		else
		{
			if (mOuterAngle_ > Echo::Radian(Echo::Math::PI))
				outerAngle = Echo::Radian(Echo::Math::PI);
			else
				outerAngle = mOuterAngle_;

			if (mInnerAngle_ > outerAngle)
				innerAngle = outerAngle;
			else
				innerAngle = mInnerAngle_;
		}

		// draw inner circles
		mHelperObj->beginUpdate(0);
		float firstInnerRadius = Echo::Math::Tan(innerAngle / 2) * mSpotlightStartCircleOffset;
		drawCircle(firstInnerRadius, mSpotlightStartCircleOffset, mColour2);
		mHelperObj->end();

		mHelperObj->beginUpdate(1);
		float secondInnerRadius = Echo::Math::Tan(innerAngle / 2) * mOutRadius_;
		drawCircle(secondInnerRadius, mOutRadius_, mColour2);
		mHelperObj->end();

		// draw first outer circles
		mHelperObj->beginUpdate(2);
		float firstOuterRadius = Echo::Math::Tan(outerAngle / 2) * mSpotlightStartCircleOffset;
		drawCircle(firstOuterRadius, mSpotlightStartCircleOffset, mColour1);
		mHelperObj->end();

		mHelperObj->beginUpdate(3);
		float secondOuterRadius = Echo::Math::Tan(outerAngle / 2) * mOutRadius_;
		drawCircle(secondOuterRadius, mOutRadius_, mColour1);
		mHelperObj->end();

		mHelperObj->beginUpdate(4);

		// draw 4 lines between inner circles
		mHelperObj->position(firstInnerRadius, 0, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour2);
		mHelperObj->position(secondInnerRadius, 0, mOutRadius_);
		mHelperObj->colour(mColour2);
		mHelperObj->position(0, firstInnerRadius, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour2);
		mHelperObj->position(0, secondInnerRadius, mOutRadius_);
		mHelperObj->colour(mColour2);
		mHelperObj->position(0, -firstInnerRadius, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour2);
		mHelperObj->position(0, -secondInnerRadius, mOutRadius_);
		mHelperObj->colour(mColour2);
		mHelperObj->position(-firstInnerRadius, 0, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour2);
		mHelperObj->position(-secondInnerRadius, 0, mOutRadius_);
		mHelperObj->colour(mColour2);

		// draw 4 lines between outer circles
		mHelperObj->position(firstOuterRadius, 0, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour1);
		mHelperObj->position(secondOuterRadius, 0, mOutRadius_);
		mHelperObj->colour(mColour1);
		mHelperObj->position(0, firstOuterRadius, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour1);
		mHelperObj->position(0, secondOuterRadius, mOutRadius_);
		mHelperObj->colour(mColour1);
		mHelperObj->position(0, -firstOuterRadius, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour1);
		mHelperObj->position(0, -secondOuterRadius, mOutRadius_);
		mHelperObj->colour(mColour1);
		mHelperObj->position(-firstOuterRadius, 0, mSpotlightStartCircleOffset);
		mHelperObj->colour(mColour1);
		mHelperObj->position(-secondOuterRadius, 0, mOutRadius_);
		mHelperObj->colour(mColour1);

		mHelperObj->end();
#endif
	}
	void SpotLightComponent::remakeSate()
	{
		setIntensity(mContext.mIntensity);
		setSpotLightColor(mContext.mColor);
		setInRadius(mContext.mInRadius);
		setOutRadius(mContext.mOutRadius);
		setInnerAngle(mContext.mInnerAngle);
		setOuterAngle(mContext.mOuterAngle);
		setDirection(mContext.mDirection);
		setPriority(mContext.mPriority);
		setIsUseTodChange(mContext.mIsUseTodChange);
		setIsProduceShadow(mContext.mIsProduceShadow);

#ifdef ECHO_EDITOR
		if (mHelperNode)
		{
			mHelperNode->_setDerivedPosition(getWorldPosition());
			mHelperNode->_setDerivedOrientation(getWorldRotation());
			mHelperNode->_setDerivedScale(getWorldScale());
			mHelperNode->_update(true);
		}
#endif
	}

	void SpotLightComponent::createComRootNodeImpl()
	{
		SceneManager* scMgr = getSceneManager();
		if(scMgr)
		   mComRootNode = scMgr->createSceneNode();
	}

	void SpotLightComponent::setTimelineAttribute(std::vector<float>& data, const Name& name_)
	{
		NodeActorComponent::setTimelineAttribute(data, name_);
		if (name_ == spotLightColorName)
			setSpotLightColor(ColorValue(data[0], data[1], data[2], data[3]));
		else if (name_ == spotLightInRadiusName)
			setInRadius(data[0]);
		else if (name_ == spotLightOutRadiusName)
			setOutRadius(data[0]);
		else if (name_ == spotLightIntensityName)
			setIntensity(data[0]);
		else if (name_ == spotLightInnerAngleName)
			setInnerAngle(data[0]);
		else if (name_ == spotLightOuterAngleName)
			setOuterAngle(data[0]);
	}

#ifdef ECHO_EDITOR
	void SpotLightComponent::setSpotLightSelectedColor()
	{
		mColour1 = ColorValue::Red;
		mColour2 = ColorValue::Blue;
		drawSpotLight();
	}

	void SpotLightComponent::restoreSpotLightHelperColor()
	{
		mColour1 = ColorValue::White;
		mColour2 = ColorValue::Black;
		drawSpotLight();
	}
#endif


	SpotLightComponentInfo::SpotLightComponentInfo(const Name& name, ActorCOMMoveableType isMoveable)
		: ActorComponentInfo(name, isMoveable)
	{
	}
	SpotLightComponentInfo::~SpotLightComponentInfo()
	{
		
	}
	void SpotLightComponentInfo::ExportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<SpotLightComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void SpotLightComponentInfo::ImportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<SpotLightComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void SpotLightComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		auto newObjectInfo = static_cast<SpotLightComponentInfo*>(info);
		reflect::TypeResolver<SpotLightComponentInfo>::get()->copyTo(this, newObjectInfo);
	}
	void SpotLightComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<SpotLightComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
	EchoSpotLightScript::EchoSpotLightScript(const EchoActorScript& pActor, const std::string& name)
		:EchoActorComponentScript(pActor, name)
	{
	}
	EchoSpotLightScript EchoSpotLightScript::Script_GetSpotLightComponent(ActorPtr pActor, const std::string& name)
	{
		return EchoSpotLightScript(pActor, name);
	}
	void EchoSpotLightComponentProFuncImp::initProtocolFunction()
	{
	/*	ProtocolComponentSystem* p = Echo::ProtocolComponentSystem::instance();
		p->registerProtocolExtFunc(Name("QuaternionToSpotDirection"), &EchoSpotLightComponentProFuncImp::ProCom_QuaternionToSpotDirection);*/
	}
	void EchoSpotLightComponentProFuncImp::uninitProtocolFunction()
	{
		/*ProtocolComponentSystem* p = Echo::ProtocolComponentSystem::instance();
		p->unregisterProtocolExtFunc(Name("QuaternionToSpotDirection"));*/
	}
	/*Echo::ProtocolResult EchoSpotLightComponentProFuncImp::ProCom_QuaternionToSpotDirection(void* quaternion_, ProtocolHandle comHandle)
	{
		if (quaternion_ == nullptr || comHandle == 0)
			return PR_Falied;
		SpotLightComponent* spotCom = (SpotLightComponent*)comHandle;
		if (spotCom == nullptr)
			return PR_Falied;
		Quaternion* qua = (Quaternion*)quaternion_;
		Vector3 direction(0, 0, 1);
		direction = *qua * direction;
		spotCom->setDirection(direction);
		return PR_OK;
	}*/
};

