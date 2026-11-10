#ifndef __EchoAtmosphereComponent_H__
#define __EchoAtmosphereComponent_H__

#include <EchoDeferredRenderStrategy.h>

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoAtmosphere.h"
#include "EchoAtmosphereManager.h"
namespace Echo
{
	struct AtmosphereInfo;
	class _EchoComponentExport AtmosphereComponentInfo : public ActorComponentInfo
	{
	public:
		AtmosphereComponentInfo();
		AtmosphereComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		AtmosphereComponentInfo(const AtmosphereComponentInfo&) = default;
		AtmosphereComponentInfo(AtmosphereComponentInfo&&) noexcept = default;
		virtual ~AtmosphereComponentInfo() override;

		virtual void			ExportData(cJSON* jsonNode) override;
		virtual void			ImportData(cJSON* jsonNode) override;
		virtual void			CopyTo(ActorComponentInfo* comInfo) override;
		virtual void			OverWrite() override;

		AtmosphereComponentInfo& operator=(AtmosphereComponentInfo&&) noexcept = default;
		AtmosphereComponentInfo& operator=(const AtmosphereComponentInfo&) = default;

		float							bottomRadiusMeter;
		ColorValue						groundAlbedo;
		float							atmosphereHeightMeter;

		float							traceSampleCountScale;

		float							multiScatteringFactor;

		ColorValue						rayleighScattering;
		float							rayleighScatteringScale;
		float							rayleighExponentialDistribution;

		ColorValue						mieScattering;
		float							mieScatteringScale;
		ColorValue						mieAbsorption;
		float							mieAbsorptionScale;
		float							mieExponentialDistribution;
		float							mieAnisotropy;

		float							sunAngularDiameter;

		float							ozoTentTipAltitudeMeter;
		float							ozoTentWidthMeter;
		float							ozoTentTipValue;

		ColorValue						ozoAbsorption;
		float							ozoAbsorptionScale;

		ColorValue						skyLuminanceFactor;
		float							aerialPerspectiveViewDistanceScale;
		float							heightFogContribution;
		float							transmittanceMinLightElevationAngle;
		float							aerialPerspectiveStartDepthMeter;

		uint32							transformType;
		float							distanceAndSizeScale;
		bool							renderSun;
		bool							atmosphereAffectSunLight;
		bool							renderPostProcess;
		bool							enablePrecomputedAtmosphere;

		//String							mVersion = "1.0";
#ifdef ECHO_EDITOR
		virtual bool					getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		EDITOR_REFLECT
	};

	class _EchoComponentExport AtmosphereComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(AtmosphereComponent)

	public:
		REGISTER_LUA_CLASS_KEY
			void init(ActorComponentInfo* info) override;
		void unInit() override;
		void copyTo(const ActorComponentPtr& actCom)override;
		void visibleStateChanged(bool visible) override;
		//void setPosition(const DVector3& pos, bool updateInstantly = true, bool redirectParent = false)override;
		void NotifiChildImp(int changeMask)override;
		//void setWorldPosition(const DVector3& pos, bool updateInstantly = true, bool redirectParent = false);
		bool							GetEnableAtmosphere()const { return mDataWrapper->enable; }
		void							SetEnableAtmosphere(bool enable) { mDataWrapper->enable = enable; }
		//bool							GetAtmosphereRenderSun()const { return mDataWrapper->atmosphereRenderSun; }
		//void							SetAtmosphereRenderSun(bool enable) { mDataWrapper->atmosphereRenderSun = enable; }
		bool							GetAtmosphereAffectSunLight()const { return mDataWrapper->atmosphereAffectSunLight; }
		void							SetAtmosphereAffectSunLight(bool enable) { mDataWrapper->atmosphereAffectSunLight = enable; }
		//bool							GetAtmosphereEnablePostProcess()const { return mDataWrapper->atmosphereEnablePostProcess; }
		//void							SetAtmosphereEnablePostProcess(bool enable) { mDataWrapper->atmosphereEnablePostProcess = enable; }

		void							SetBottomRadiusMeter(float bottomRadius);
		float							GetBottomRadiusMeter()const { return mAtmosphereInstance->GetBottomRadiusMeter(); }
		void							SetAtmosphereHeightMeter(float atmosphereHeight);
		float							GetAtmosphereHeightMeter()const { return mAtmosphereInstance->GetAtmosphereHeightMeter(); }
		void							SetGroundAlbedo(const ColorValue& color) { mAtmosphereInstance->SetGroundAlbedo(color); }
		ColorValue						GetGroundAlbedo()const { return mAtmosphereInstance->GetGroundAlbedo(); }

		void							SetTraceSampleCountScale(float traceSampleCountScale) { mAtmosphereInstance->SetTraceSampleCountScale(traceSampleCountScale); }
		float							GetTraceSampleCountScale()const { return mAtmosphereInstance->GetTraceSampleCountScale(); }

		void							SetMultiScatteringFactor(float multiScatteringFactor) { mAtmosphereInstance->SetMultiScatteringFactor(multiScatteringFactor); }
		float							GetMultiScatteringFactor()const { return mAtmosphereInstance->GetMultiScatteringFactor(); }

		void							SetRayleighScattering(const ColorValue& color) { mAtmosphereInstance->SetRayleighScattering(color); }
		ColorValue						GetRayleighScattering()const { return mAtmosphereInstance->GetRayleighScattering(); }
		void							SetRayleighScatteringScale(float rayleiScatteringScale) { mAtmosphereInstance->SetRayleighScatteringScale(rayleiScatteringScale); }
		float							GetRayleighScatteringScale()const { return mAtmosphereInstance->GetRayleighScatteringScale(); }
		void							SetRayleighExponentialDistribution(float rayleighExponentialDistribution) { return mAtmosphereInstance->SetRayleighExponentialDistribution(rayleighExponentialDistribution); }
		float							GetRayleighExponentialDistribution()const { return mAtmosphereInstance->GetRayleighExponentialDistribution(); }

		void							SetMieScattering(const ColorValue& color) { mAtmosphereInstance->SetMieScattering(color); }
		ColorValue						GetMieScattering()const { return mAtmosphereInstance->GetMieScattering(); }
		void							SetMieScatteringScale(float mieScatteringScale) { mAtmosphereInstance->SetMieScatteringScale(mieScatteringScale); }
		float							GetMieScatteringScale()const { return mAtmosphereInstance->GetMieScatteringScale(); }
		void							SetMieAbsorption(const ColorValue& color) { mAtmosphereInstance->SetMieAbsorption(color); }
		ColorValue						GetMieAbsorption()const { return mAtmosphereInstance->GetMieAbsorption(); }
		void							SetMieAbsorptionScale(float mieAbsorptionScale) { mAtmosphereInstance->SetMieAbsorptionScale(mieAbsorptionScale); }
		float							GetMieAbsorptionScale()const { return mAtmosphereInstance->GetMieAbsorptionScale(); }
		void							SetMieExponentialDistribution(float mieExponentialDistribution) { mAtmosphereInstance->SetMieExponentialDistribution(mieExponentialDistribution); }
		float							GetMieExponentialDistribution()const { return mAtmosphereInstance->GetMieExponentialDistribution(); }
		void							SetMieAnisotropy(float mieAnisotripy) { mAtmosphereInstance->SetMieAnisotropy(mieAnisotripy); }
		float							GetMieAnisotropy()const { return mAtmosphereInstance->GetMieAnisotropy(); }

		void							SetSunAngularDiameter(float sunAngularDiameter) { mAtmosphereInstance->SetSunAngularDiameter(sunAngularDiameter); } // size of sun
		float							GetSunAngularDiameter()const { return mAtmosphereInstance->GetSunAngularDiameter(); }

		void							SetOzoTentTipAltitudeMeter(float ozoTentAltitude) { mAtmosphereInstance->SetOzoTentTipAltitudeMeter(ozoTentAltitude); }
		float							GetOzoTentTipAltitudeMeter()const { return mAtmosphereInstance->GetOzoTentTipAltitudeMeter(); }

		void							SetOzoTentWidthMeter(float ozoTentWidth) { mAtmosphereInstance->SetOzoTentWidthMeter(ozoTentWidth); }
		float							GetOzoTentWidthMeter()const { return mAtmosphereInstance->GetOzoTentWidthMeter(); }

		void							SetOzoTentTipValue(float ozoTentTipValue) { mAtmosphereInstance->SetOzoTentTipValue(ozoTentTipValue); }
		float							GetOzoTentTipValue()const { return mAtmosphereInstance->GetOzoTentTipValue(); }

		void							SetOzoAbsorption(const ColorValue& ozoAbsorption) { mAtmosphereInstance->SetOzoAbsorption(ozoAbsorption); }
		ColorValue						GetOzoAbsorption()const { return mAtmosphereInstance->GetOzoAbsorption(); }
		void							SetOzoAbsorptionScale(float value) { mAtmosphereInstance->SetOzoAbsorptionScale(value); }
		float							GetOzoAbsorptionScale()const { return mAtmosphereInstance->GetOzoAbsorptionScale(); }

		void							SetSkyLuminanceFactor(const ColorValue& value) { mAtmosphereInstance->SetSkyLuminanceFactor(value); }
		ColorValue						GetSkyLuminanceFactor()const { return mAtmosphereInstance->GetSkyLuminanceFactor(); }
		void							SetAerialPerspectiveViewDistanceScale(float value) { mAtmosphereInstance->SetAerialPerspectiveViewDistanceScale(value); }
		float							GetAerialPerspectiveViewDistanceScale()const { return mAtmosphereInstance->GetAerialPerspectiveViewDistanceScale(); }
		void							SetHeightFogContribution(float value) { mAtmosphereInstance->SetHeightFogContribution(value); }
		float							GetHeightFogContribution()const { return mAtmosphereInstance->GetHeightFogContribution(); }
		void							SetTransmittanceMinLightElevationAngle(float value) { mAtmosphereInstance->SetTransmittanceMinLightElevationAngle(value); }
		float							GetTransmittanceMinLightElevationAngle()const { return mAtmosphereInstance->GetTransmittanceMinLightElevationAngle(); }

		void							SetAerialPerspectiveStartDepthMeter(float value) { mAtmosphereInstance->SetAerialPerspectiveStartDepthMeter(value); }
		float							GetAerialPerspectiveStartDepthMeter()const { return mAtmosphereInstance->GetAerialPerspectiveStartDepthMeter(); }

		void							SetAtmosphereCenterTransformType(AtmosphereCenterTransformType type) { mAtmosphereInstance->SetAtmosphereCenterTransformType(type); }
		AtmosphereCenterTransformType	GetAtmosphereCenterTransformType()const { return mAtmosphereInstance->GetAtmosphereCenterTransformType(); }

		void							SetDistanceAndSizeScale(float value) { mAtmosphereInstance->SetDistanceAndSizeScale(value); }
		float							GetDistanceAndSizeScale()const { return mAtmosphereInstance->GetDistanceAndSizeScale(); }


		void							SetEnablePrecomputedAtmosphere(bool enable);
		bool							GetEnablePrecomputedAtmosphere()const;

		void							PrecomputeCurrentAtmosphere();
	private:
		float							GetAtmosphereTopRadiusMeter()const;
		//bool mRenderSun = true;
		//bool mAtmosphereAffectSunLight = false;
		//bool mRenderPostProcess = false;
		//RenderStrategy* mRenderStrategy;
		PrecomputedAtmosphereRenderable* mPrecomputedAtmosphereRenderable;
		SkyAtmosphereComponentDataWrapper* mDataWrapper;
		AtmosphereInfo* mAtmosphereInstance;
	};
	DEFINECOMFACTORY(AtmosphereComponent, UNMOVEABLE)
}
#endif
