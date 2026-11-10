#include "EchoComponentStableHeaders.h"
#include "EchoAtmosphereComponent.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(AtmosphereComponentInfo)

		////EDITOR_REFLECT_MEMBER_UPDATE(mMinHeight, "MinHetght", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mMaxHeight, "MaxHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//EDITOR_REFLECT_MEMBER_UPDATE(mFogColor, "FogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogExpScale, "FogExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogExpTerm, "FogExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mHorizonColor, "HorizonColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogStart, "FogStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogEnd, "FogEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mColorChangeGradient, "ColorChangeGradient", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogMaxAlpha, "FogMaxAlpha", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mFogSkyParam, "FogSkyParam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxAlphaPower, "AtmosApproxAlphaPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxLightPower, "AtmosApproxLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxLightColor, "AtmosApproxLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosApproxAverageDensity, "AtmosApproxAverageDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		////EDITOR_REFLECT_MEMBER_UPDATE(mRayleighExpScale, "RExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mRayleighExpTerm, "RExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mGroundIrradiance, "GroundIrradiance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mMieExpScale, "MExpScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mMieExpTerm, "MExpTerm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRTop, "RTop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRTopArchLight, "RTopArchLight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereRBottom, "RBottom", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereBlendDistance, "RBlendDisatnce", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mAtmosphereBlendRange, "RBlendRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		//EDITOR_REFLECT_MEMBER_UPDATE(mEnableAtmosphere, "Enable", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mEnableSecondLayer, "EnableSecondLayer", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		////EDITOR_REFLECT_MEMBER_UPDATE(mTODTemplateName, "TODTemplatePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	AtmosphereComponentInfo::AtmosphereComponentInfo() :
		mGroundAlbedo{ 0.1f },
		mRayleighScattering{ 0.005802f, 0.013558f, 0.033100f },
		mMaxSunZenithAngle(-0.207912f),
		mSunAngularRadius(0.004675f),
		mMiePhaseFunctionG(0.8f),
		mMieScattering{ 0.003996f, 0.003996f, 0.003996f },
		mMieExtinction{ 0.004440f, 0.004440f, 0.004440f },
		mAbsorptionExtinction{ 0.000650f, 0.001881f, 0.000085f },
		mBottomRadius(6360.f),
		mTopRadius(6420.f)
	{
		loadDefaultDensityProfiles();
	}

	AtmosphereComponentInfo::AtmosphereComponentInfo(const Name& type, ActorCOMMoveableType moveable) :
		ActorComponentInfo(type, moveable),
		mGroundAlbedo{ 0.1f },
		mRayleighScattering{ 0.005802f, 0.013558f, 0.033100f },
		mMaxSunZenithAngle(-0.207912f),
		mSunAngularRadius(0.004675f),
		mMiePhaseFunctionG(0.8f),
		mMieScattering{ 0.003996f, 0.003996f, 0.003996f },
		mMieExtinction{ 0.004440f, 0.004440f, 0.004440f },
		mAbsorptionExtinction{ 0.000650f, 0.001881f, 0.000085f },
		mBottomRadius(6360.f),
		mTopRadius(6420.f)
	{
		loadDefaultDensityProfiles();
	}

	AtmosphereComponentInfo::~AtmosphereComponentInfo() = default;

	void AtmosphereComponentInfo::loadDefaultDensityProfiles()
	{
		mDensityProfileLayers = {
			DPLayer{.mWidth = 0.000000f, .mExpTerm = 0.000000f, .mHeightScale = INFINITY, .mLinearTerm = 0.000000f, .mConstantTerm = 0.000000f },
			DPLayer{.mWidth = 0.000000f, .mExpTerm = 1.000000f, .mHeightScale = 8.000000f, .mLinearTerm = 0.000000f, .mConstantTerm = 0.000000f },

			DPLayer{.mWidth = 0.000000f, .mExpTerm = 0.000000f, .mHeightScale = INFINITY, .mLinearTerm = 0.000000f, .mConstantTerm = 0.000000f },
			DPLayer{.mWidth = 0.000000f, .mExpTerm = 1.000000f, .mHeightScale = 1.200000f, .mLinearTerm = 0.000000f, .mConstantTerm = 0.000000f },

			DPLayer{.mWidth = 25.000000f, .mExpTerm = 0.000000f, .mHeightScale = INFINITY, .mLinearTerm = 0.066667f, .mConstantTerm = -0.666667f },
			DPLayer{.mWidth = 0.000000f, .mExpTerm = 0.000000f, .mHeightScale = INFINITY, .mLinearTerm = -0.066667f, .mConstantTerm = 2.666667f },
		};
	}

	void AtmosphereComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->ExportJson(this, jsonNode, "");
		 WriteJSNode(jsonNode, "Version", "1.0");
	}

	void AtmosphereComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->ImportJson(this, jsonNode, "");
		ReadJSNode(jsonNode, "Version", mVersion);
	}

	void AtmosphereComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		AtmosphereComponentInfo* info = static_cast<AtmosphereComponentInfo*>(comInfo);
		reflect::TypeResolver<AtmosphereComponentInfo>::get()->copyTo(this, info);
		info->mVersion = mVersion;
	}

	void AtmosphereComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<AtmosphereComponentInfo>::get()->overWrite(mOriginComInfo, this);
			AtmosphereComponentInfo* info = static_cast<AtmosphereComponentInfo*>(mOriginComInfo);
			info->mVersion = mVersion;
		}
	}

	AtmosphereComponent::AtmosphereComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType) :
		ActorComponent{ pActor, movableType },
		mRenderStrategy(nullptr),
		mMiePhaseFunctionG(0),
		mMaxSunZenithAngle(0),
		mSunAngularRadius(0),
		mBottomRadius(0),
		mTopRadius(0) {
	}

	AtmosphereComponent::~AtmosphereComponent() = default;

	void AtmosphereComponent::apply() const
	{
		if (Root::instance()->getEnabledRenderStrategy() != RenderStrategyType::RS_Deferred) return;

		AtmosphereParams params{};
		//const ColorValue& color = Root::instance()->getMainSceneManager()->getMainLightColor();
		params.solarIrradiance = Vector3{ 1.0f };
		params.sunAngularRadius = mSunAngularRadius;
		params.absorptionExtinction = mAbsorptionExtinction;
		params.bottomRadius = mBottomRadius;
		params.groundAlbedo = mGroundAlbedo;
		params.maxSunZenithAngle = mMaxSunZenithAngle;
		params.mieExtinction = mMieExtinction;
		params.miePhaseFunctionG = mMiePhaseFunctionG;
		params.mieScattering = mMieScattering;
		params.rayleighScattering = mRayleighScattering;
		params.topRadius = mTopRadius;
		for (uint8_t i = 0; i < ECHO_DF_NUM; ++i)
		{
			params.densityProfiles[i].width = mDensityProfiles[i].mWidth;
			params.densityProfiles[i].constantTerm = mDensityProfiles[i].mConstantTerm;
			params.densityProfiles[i].expTerm = mDensityProfiles[i].mExpTerm;
			params.densityProfiles[i].expScale = -1 / mDensityProfiles[i].mHeightScale;
			params.densityProfiles[i].linearTerm = mDensityProfiles[i].mLinearTerm;
		}
		// 执行预计算
		static_cast<DeferredRenderStrategy*>(mRenderStrategy)->updateAtmosphereData(params);
	}

#ifdef ECHO_EDITOR
	bool AtmosphereComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		return false;
	}
#endif

	DECLAREACTORCOMTYPE_CPP(AtmosphereComponent)
} // namespace ECHO
