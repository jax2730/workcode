#include "EchoComponentStableHeaders.h"
#include "EchoAtmosphereComponent.h"
#include "EchoAtmosphereManager.h"
#include "EchoColorValue.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(AtmosphereComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(bottomRadiusMeter, "BottomRadiusMeter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(groundAlbedo, "GroundAlbedo", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(atmosphereHeightMeter, "AtmosphereHeightMeter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(traceSampleCountScale, "TraceSampleCountScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(multiScatteringFactor, "MultiScatteringFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(rayleighScattering, "RayleighScattering", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(rayleighScatteringScale, "RayleighScatteringScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(rayleighExponentialDistribution, "RayleighExponentialDistribution", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieScattering, "MieScattering", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieScatteringScale, "MieScatteringScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieAbsorption, "MieAbsorption", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieAbsorptionScale, "MieAbsorptionScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieExponentialDistribution, "MieExponentialDistribution", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mieAnisotropy, "MieAnisotropy", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(sunAngularDiameter, "SunAngularDiameter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ozoTentTipAltitudeMeter, "OzoTentTipAltitudeMeter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ozoTentWidthMeter, "OzoTentWidthMeter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ozoTentTipValue, "OzoTentTipValue", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ozoAbsorption, "OzoAbsorption", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ozoAbsorptionScale, "OzoAbsorptionScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(skyLuminanceFactor, "SkyLuminanceFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(aerialPerspectiveViewDistanceScale, "AerialPerspectiveViewDistanceScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(heightFogContribution, "HeightFogContribution", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(transmittanceMinLightElevationAngle, "TransmittanceMinLightElevationAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(aerialPerspectiveStartDepthMeter, "AerialPerspectiveStartDepthMeter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(transformType, "AtmosphereCenterTransformType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(distanceAndSizeScale, "AtmosphereDistanceAndSizeScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(renderSun, "AtmosphereRenderSun", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(atmosphereAffectSunLight, "AtmosphereAffectSunLight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(renderPostProcess, "AtmosphereRenderPostProcess", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(enablePrecomputedAtmosphere, "EnablePrecomputedAtmosphere", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();



	void AtmosphereComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if (actCom == nullptr) return;
		AtmosphereComponent* pComp = static_cast<AtmosphereComponent*>(actCom.getPointer());
		pComp->SetBottomRadiusMeter(GetBottomRadiusMeter());
		pComp->SetGroundAlbedo(GetGroundAlbedo());
		pComp->SetAtmosphereHeightMeter(GetAtmosphereHeightMeter());
		pComp->SetTraceSampleCountScale(GetTraceSampleCountScale());
		pComp->SetMultiScatteringFactor(GetMultiScatteringFactor());
		pComp->SetRayleighScattering(GetRayleighScattering());
		pComp->SetRayleighScatteringScale(GetRayleighScatteringScale());
		pComp->SetRayleighExponentialDistribution(GetRayleighExponentialDistribution());
		pComp->SetMieScattering(GetMieScattering());
		pComp->SetMieScatteringScale(GetMieScatteringScale());
		pComp->SetMieAbsorption(GetMieAbsorption());
		pComp->SetMieAbsorptionScale(GetMieAbsorptionScale());
		pComp->SetMieExponentialDistribution(GetMieExponentialDistribution());
		pComp->SetMieAnisotropy(GetMieAnisotropy());
		pComp->SetSunAngularDiameter(GetSunAngularDiameter());
		pComp->SetOzoTentTipAltitudeMeter(GetOzoTentTipAltitudeMeter());
		pComp->SetOzoTentWidthMeter(GetOzoTentWidthMeter());
		pComp->SetOzoTentTipValue(GetOzoTentTipValue());
		pComp->SetOzoAbsorption(GetOzoAbsorption());
		pComp->SetOzoAbsorptionScale(GetOzoAbsorptionScale());
		pComp->SetSkyLuminanceFactor(GetSkyLuminanceFactor());
		pComp->SetAerialPerspectiveViewDistanceScale(GetAerialPerspectiveViewDistanceScale());
		pComp->SetHeightFogContribution(GetHeightFogContribution());
		pComp->SetTransmittanceMinLightElevationAngle(GetTransmittanceMinLightElevationAngle());
		pComp->SetAerialPerspectiveStartDepthMeter(GetAerialPerspectiveStartDepthMeter());
		pComp->SetAtmosphereCenterTransformType(GetAtmosphereCenterTransformType());
		pComp->SetDistanceAndSizeScale(GetDistanceAndSizeScale());
		//pComp->SetAtmosphereRenderSun(GetAtmosphereRenderSun());
		pComp->SetAtmosphereAffectSunLight(GetAtmosphereAffectSunLight());
		//pComp->SetAtmosphereEnablePostProcess(GetAtmosphereEnablePostProcess());
		pComp->SetEnablePrecomputedAtmosphere(GetEnablePrecomputedAtmosphere());
	}

	void AtmosphereComponent::init(ActorComponentInfo* info)
	{
		mDataWrapper = getSceneManager()->getSkyAtmosphereManager()->CreateAtmospohereInstance(getPosition());
		if (mDataWrapper)
			mAtmosphereInstance = mDataWrapper->atmosphereInfo.get();
		if (info)
		{
			AtmosphereComponentInfo* atmosInfo = dynamic_cast<AtmosphereComponentInfo*>(info);
			if (atmosInfo)
			{
				SetBottomRadiusMeter(atmosInfo->bottomRadiusMeter);
				SetGroundAlbedo(atmosInfo->groundAlbedo);
				SetAtmosphereHeightMeter(atmosInfo->atmosphereHeightMeter);
				SetTraceSampleCountScale(atmosInfo->traceSampleCountScale);
				SetMultiScatteringFactor(atmosInfo->multiScatteringFactor);
				SetRayleighScattering(atmosInfo->rayleighScattering);
				SetRayleighScatteringScale(atmosInfo->rayleighScatteringScale);
				SetRayleighExponentialDistribution(atmosInfo->rayleighExponentialDistribution);
				SetMieScattering(atmosInfo->mieScattering);
				SetMieScatteringScale(atmosInfo->mieScatteringScale);
				SetMieAbsorption(atmosInfo->mieAbsorption);
				SetMieAbsorptionScale(atmosInfo->mieAbsorptionScale);
				SetMieExponentialDistribution(atmosInfo->mieExponentialDistribution);
				SetMieAnisotropy(atmosInfo->mieAnisotropy);
				SetSunAngularDiameter(atmosInfo->sunAngularDiameter);
				SetOzoTentTipAltitudeMeter(atmosInfo->ozoTentTipAltitudeMeter);
				SetOzoTentWidthMeter(atmosInfo->ozoTentWidthMeter);
				SetOzoTentTipValue(atmosInfo->ozoTentTipValue);
				SetOzoAbsorption(atmosInfo->ozoAbsorption);
				SetOzoAbsorptionScale(atmosInfo->ozoAbsorptionScale);
				SetSkyLuminanceFactor(atmosInfo->skyLuminanceFactor);
				SetAerialPerspectiveViewDistanceScale(atmosInfo->aerialPerspectiveViewDistanceScale);
				SetHeightFogContribution(atmosInfo->heightFogContribution);
				SetTransmittanceMinLightElevationAngle(atmosInfo->transmittanceMinLightElevationAngle);
				SetAerialPerspectiveStartDepthMeter(atmosInfo->aerialPerspectiveStartDepthMeter);
				SetAtmosphereCenterTransformType(static_cast<AtmosphereCenterTransformType>(atmosInfo->transformType));
				SetDistanceAndSizeScale(atmosInfo->distanceAndSizeScale);
				//SetAtmosphereRenderSun(atmosInfo->renderSun);
				SetAtmosphereAffectSunLight(atmosInfo->atmosphereAffectSunLight);
				//SetAtmosphereEnablePostProcess(atmosInfo->renderPostProcess);
				SetEnablePrecomputedAtmosphere(atmosInfo->enablePrecomputedAtmosphere);

				/*				mAtmosphereInstance->SetBottomRadiusMeter(atmosInfo->bottomRadiusMeter);
								mAtmosphereInstance->SetGroundAlbedo(atmosInfo->groundAlbedo);
								mAtmosphereInstance->SetAtmosphereHeightMeter(atmosInfo->atmosphereHeightMeter);
								mAtmosphereInstance->SetTraceSampleCountScale(atmosInfo->traceSampleCountScale);
								mAtmosphereInstance->SetMultiScatteringFactor(atmosInfo->multiScatteringFactor);
								mAtmosphereInstance->SetRayleighScattering(atmosInfo->rayleighScattering);
								mAtmosphereInstance->SetRayleighScatteringScale(atmosInfo->rayleighScatteringScale);
								mAtmosphereInstance->SetRayleighExponentialDistribution(atmosInfo->rayleighExponentialDistribution);
								mAtmosphereInstance->SetMieScattering(atmosInfo->mieScattering);
								mAtmosphereInstance->SetMieScatteringScale(atmosInfo->mieScatteringScale);
								mAtmosphereInstance->SetMieAbsorption(atmosInfo->mieAbsorption);
								mAtmosphereInstance->SetMieAbsorptionScale(atmosInfo->mieAbsorptionScale);
								mAtmosphereInstance->SetMieExponentialDistribution(atmosInfo->mieExponentialDistribution);
								mAtmosphereInstance->SetMieAnisotropy(atmosInfo->mieAnisotropy);
								mAtmosphereInstance->SetSunAngularDiameter(atmosInfo->sunAngularDiameter);
								mAtmosphereInstance->SetOzoTentTipAltitudeMeter(atmosInfo->ozoTentTipAltitudeMeter);
								mAtmosphereInstance->SetOzoTentWidthMeter(atmosInfo->ozoTentWidthMeter);
								mAtmosphereInstance->SetOzoTentTipValue(atmosInfo->ozoTentTipValue);
								mAtmosphereInstance->SetOzoAbsorption(atmosInfo->ozoAbsorption);
								mAtmosphereInstance->SetOzoAbsorptionScale(atmosInfo->ozoAbsorptionScale);
								mAtmosphereInstance->SetSkyLuminanceFactor(atmosInfo->skyLuminanceFactor);
								mAtmosphereInstance->SetAerialPerspectiveViewDistanceScale(atmosInfo->aerialPerspectiveViewDistanceScale);
								mAtmosphereInstance->SetHeightFogContribution(atmosInfo->heightFogContribution);
								mAtmosphereInstance->SetTransmittanceMinLightElevationAngle(atmosInfo->transmittanceMinLightElevationAngle);
								mAtmosphereInstance->SetAerialPerspectiveStartDepthMeter(atmosInfo->aerialPerspectiveStartDepthMeter);
								mAtmosphereInstance->SetAtmosphereCenterTransformType(static_cast<AtmosphereCenterTransformType>(atmosInfo->transformType));
								mAtmosphereInstance->SetDistanceAndSizeScale(atmosInfo->distanceAndSizeScale); */
								//mDataWrapper->atmosphereRenderSun			= atmosInfo->renderSun;
								//mDataWrapper->atmosphereAffectSunLight		= atmosInfo->atmosphereAffectSunLight;
								//mDataWrapper->atmosphereEnablePostProcess	= atmosInfo->renderPostProcess;
			}
		}
		SetNotifiedUpdate(true);
		bool visible = getVisible();
		visibleStateChanged(visible);
	}

	void AtmosphereComponent::unInit()
	{
		SetNotifiedUpdate(false);
		if (mDataWrapper != nullptr)
		{
			getSceneManager()->getSkyAtmosphereManager()->DeleteAtmosphereInstance(mDataWrapper);
			mAtmosphereInstance = nullptr;
			mDataWrapper = nullptr;
		}
	}
	void AtmosphereComponent::SetEnablePrecomputedAtmosphere(bool enable)
	{
		if (enable) {
			if (mPrecomputedAtmosphereRenderable != nullptr)
				return;
			mPrecomputedAtmosphereRenderable = getSceneManager()->getSkyAtmosphereManager()->
				CreatePrecomputedAtmosphereRenderable(mDataWrapper, getPosition());
		}
		else
		{
			if (mPrecomputedAtmosphereRenderable != nullptr)
			{
				getSceneManager()->getSkyAtmosphereManager()->
					DeletePrecomputedAtmosphereRenderable(mPrecomputedAtmosphereRenderable);
				mPrecomputedAtmosphereRenderable = nullptr;
				return;
			}
		}
	}

	bool AtmosphereComponent::GetEnablePrecomputedAtmosphere()const
	{
		return (mPrecomputedAtmosphereRenderable != nullptr);
	}

	void AtmosphereComponent::SetBottomRadiusMeter(float bottomRadius)
	{
		mAtmosphereInstance->SetBottomRadiusMeter(bottomRadius);
		if (mPrecomputedAtmosphereRenderable)
		{
			float topRadius = GetAtmosphereTopRadiusMeter();
			mPrecomputedAtmosphereRenderable->setScale(Vector3(topRadius, topRadius, topRadius));
		}
	}

	void AtmosphereComponent::SetAtmosphereHeightMeter(float atmosphereHeight)
	{
		mAtmosphereInstance->SetAtmosphereHeightMeter(atmosphereHeight);
		if (mPrecomputedAtmosphereRenderable)
		{
			float topRadius = GetAtmosphereTopRadiusMeter();
			mPrecomputedAtmosphereRenderable->setScale({ topRadius, topRadius, topRadius });
		}
	}

	float AtmosphereComponent::GetAtmosphereTopRadiusMeter()const
	{
		float bottomRadius = GetBottomRadiusMeter();
		float atmosphereHeight = GetAtmosphereHeightMeter();
		return bottomRadius + atmosphereHeight;
	}

	void AtmosphereComponent::PrecomputeCurrentAtmosphere() {
		getSceneManager()->getSkyAtmosphereManager()->PreComputeAndSaveAtmosphere("", mDataWrapper);
	}

	AtmosphereComponentInfo::AtmosphereComponentInfo() :
		bottomRadiusMeter(6360000.0f),
		groundAlbedo(ColorValue::linearToGamma(ColorValue(0.401878f, 0.401978f, 0.401978f, 1.f))),
		atmosphereHeightMeter(60000.0f),
		traceSampleCountScale(1.0f),
		multiScatteringFactor(1.0f),
		rayleighScattering(ColorValue::linearToGamma(ColorValue(0.175287f, 0.409607f, 1.0f))),
		rayleighScatteringScale(0.0331f),
		rayleighExponentialDistribution(8.0f),
		mieScattering(ColorValue::linearToGamma(ColorValue(1.f))),
		mieScatteringScale(0.003996f),
		mieAbsorption(ColorValue::linearToGamma(ColorValue(1.f))),
		mieAbsorptionScale(0.000444f),
		mieExponentialDistribution(1.2f),
		mieAnisotropy(0.8f),
		sunAngularDiameter(0.545f),
		ozoTentTipAltitudeMeter(25000.0f),
		ozoTentWidthMeter(15000.0f),
		ozoTentTipValue(1.f),
		ozoAbsorption(ColorValue::linearToGamma(ColorValue(0.345561f, 1.0f, 0.045189f))),
		ozoAbsorptionScale(0.001881f),
		skyLuminanceFactor(1.0f),
		aerialPerspectiveViewDistanceScale(1.0f),
		heightFogContribution(1.0f),
		transmittanceMinLightElevationAngle(-90.f),
		aerialPerspectiveStartDepthMeter(100.f),
		transformType(0),
		distanceAndSizeScale(1.f),
		renderSun(true),
		atmosphereAffectSunLight(false),
		renderPostProcess(true),
		enablePrecomputedAtmosphere(false)
	{

	}

	AtmosphereComponentInfo::AtmosphereComponentInfo(const Name& type, ActorCOMMoveableType moveable) :
		ActorComponentInfo(type, moveable),
		bottomRadiusMeter(6360000.0f),
		groundAlbedo(ColorValue::linearToGamma(ColorValue(0.401878f, 0.401978f, 0.401978f, 1.f))),
		atmosphereHeightMeter(60000.0f),
		traceSampleCountScale(1.0f),
		multiScatteringFactor(1.0f),
		rayleighScattering(ColorValue::linearToGamma(ColorValue(0.175287f, 0.409607f, 1.0f))),
		rayleighScatteringScale(0.0331f),
		rayleighExponentialDistribution(8.0f),
		mieScattering(ColorValue::linearToGamma(ColorValue(1.f))),
		mieScatteringScale(0.003996f),
		mieAbsorption(ColorValue::linearToGamma(ColorValue(1.f))),
		mieAbsorptionScale(0.000444f),
		mieExponentialDistribution(1.2f),
		mieAnisotropy(0.8f),
		sunAngularDiameter(0.545f),
		ozoTentTipAltitudeMeter(25000.0f),
		ozoTentWidthMeter(15000.0f),
		ozoTentTipValue(1.f),
		ozoAbsorption(ColorValue::linearToGamma(ColorValue(0.345561f, 1.0f, 0.045189f))),
		ozoAbsorptionScale(0.001881f),
		skyLuminanceFactor(1.0f),
		aerialPerspectiveViewDistanceScale(1.0f),
		heightFogContribution(1.0f),
		transmittanceMinLightElevationAngle(-90.f),
		aerialPerspectiveStartDepthMeter(100.f),
		transformType(0),
		distanceAndSizeScale(1.f),
		renderSun(true),
		atmosphereAffectSunLight(false),
		renderPostProcess(true),
		enablePrecomputedAtmosphere(false)
	{

	}

	AtmosphereComponentInfo::~AtmosphereComponentInfo() = default;

	void AtmosphereComponent::visibleStateChanged(bool visible)
	{
		if(mDataWrapper)
			mDataWrapper->hide = !visible;
	}

	//void AtmosphereComponent::setPosition(const DVector3& pos, bool updateInstantly, bool redirectParent)
	//{
	//	ActorComponentPtr parentNode = getParentNodeCom();
	//	if (parentNode)
	//	{
	//		const DVector3& parentPos = parentNode->getWorldPosition();
	//		mAtmosphereInstance->SetComponentCenterMeter(pos + parentPos);
	//	}
	//	else {
	//		mAtmosphereInstance->SetComponentCenterMeter(pos);
	//	}
	//}

	//void AtmosphereComponent::setWorldPosition(const DVector3& pos, bool updateInstantly, bool redirectParent)
	//{
	//	mAtmosphereInstance->SetComponentCenterMeter(pos);
	//}

	void AtmosphereComponent::NotifiChildImp(int mask)
	{
		const DVector3& componentPosition = getWorldPosition();
		mAtmosphereInstance->SetComponentCenterMeter(componentPosition);
		if (mPrecomputedAtmosphereRenderable != nullptr)
		{
			mPrecomputedAtmosphereRenderable->setWorldPosition(componentPosition);
		}
	}

	void AtmosphereComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->ExportJson(this, jsonNode, "");
		//WriteJSNode(jsonNode, "Version", "1.0");
	}

	void AtmosphereComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->ImportJson(this, jsonNode, "");
		//ReadJSNode(jsonNode, "Version", mVersion);
	}

	void AtmosphereComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		AtmosphereComponentInfo* info = static_cast<AtmosphereComponentInfo*>(comInfo);
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->copyTo(this, info);
		//info->mVersion = mVersion;
	}

	void AtmosphereComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<AtmosphereComponentInfo>::get()->overWrite(mOriginComInfo, this);
			//AtmosphereComponentInfo* info = static_cast<AtmosphereComponentInfo*>(mOriginComInfo);
			//info->mVersion = mVersion;
		}
	}

	AtmosphereComponent::AtmosphereComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :
		NodeActorComponent{ pActor, movableType } {
	}

	AtmosphereComponent::~AtmosphereComponent() = default;

#ifdef ECHO_EDITOR
	bool AtmosphereComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		return false;
	}
#endif

	DECLAREACTORCOMTYPE_CPP(AtmosphereComponent)
} // namespace ECHO
