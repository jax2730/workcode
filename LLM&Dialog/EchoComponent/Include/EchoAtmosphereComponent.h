#ifndef __EchoAtmosphereComponent_H__
#define __EchoAtmosphereComponent_H__

#include <EchoDeferredRenderStrategy.h>

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoAtmosphere.h"

namespace Echo
{
	struct DPLayer
	{
		// density = constantTerm + linearTerm * h + expTerm * exp(h * expScale)
		float mWidth = 0.f;
		float mExpTerm = 1.f;
		float mHeightScale = 2.f;		// = -1.f / heightScale
		float mLinearTerm = 0.f;		// 0 - 1
		float mConstantTerm = 0.f;
	};

	class _EchoComponentExport AtmosphereComponentInfo : public ActorComponentInfo
	{
	public:
		AtmosphereComponentInfo();
		AtmosphereComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		AtmosphereComponentInfo(const AtmosphereComponentInfo&) = default;
		AtmosphereComponentInfo(AtmosphereComponentInfo&&) noexcept = default;
		virtual ~AtmosphereComponentInfo() override;

		void loadDefaultDensityProfiles();
		virtual void			ExportData(cJSON* jsonNode) override;
		virtual void			ImportData(cJSON* jsonNode) override;
		virtual void			CopyTo(ActorComponentInfo* comInfo) override;
		virtual void			OverWrite() override;

		AtmosphereComponentInfo& operator=(AtmosphereComponentInfo&&) noexcept = default;
		AtmosphereComponentInfo& operator=(const AtmosphereComponentInfo&) = default;

		static constexpr uint8_t kMaxDensityProfileLayerCount = 2;

		Vector3 mGroundAlbedo;
		Vector3 mRayleighScattering;
		float mMaxSunZenithAngle;
		float mSunAngularRadius;
		float mMiePhaseFunctionG;
		Vector3 mMieScattering;
		Vector3 mMieExtinction;
		Vector3 mAbsorptionExtinction;
		float mBottomRadius;
		float mTopRadius;
		std::array<DPLayer, ECHO_DF_NUM> mDensityProfileLayers;

		String mVersion = "1.0";
#ifdef ECHO_EDITOR
		virtual bool			getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		EDITOR_REFLECT
	};

	class _EchoComponentExport AtmosphereComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(AtmosphereComponent)

	public:
		REGISTER_LUA_CLASS_KEY
		void init(ActorComponentInfo* info) override
		{
			mRenderStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
			AtmosphereComponentInfo& atmosphereInfo = *static_cast<AtmosphereComponentInfo*>(info);

			mMiePhaseFunctionG = atmosphereInfo.mMiePhaseFunctionG;
			mRayleighScattering = atmosphereInfo.mRayleighScattering;
			mMaxSunZenithAngle = atmosphereInfo.mMaxSunZenithAngle;
			mMieScattering = atmosphereInfo.mMieScattering;
			mSunAngularRadius = atmosphereInfo.mSunAngularRadius;
			mMieExtinction = atmosphereInfo.mMieExtinction;
			mBottomRadius = atmosphereInfo.mBottomRadius;
			mAbsorptionExtinction = atmosphereInfo.mAbsorptionExtinction;
			mTopRadius = atmosphereInfo.mTopRadius;
			mGroundAlbedo = atmosphereInfo.mGroundAlbedo;

			for (uint8_t i = 0; i < AtmosphereComponentInfo::kMaxDensityProfileLayerCount; ++i)
			{
				mDensityProfiles[ECHO_DF_RAYLEIGH_0 + i] = atmosphereInfo.mDensityProfileLayers[ECHO_DF_RAYLEIGH_0 + i];
				mDensityProfiles[ECHO_DF_MIE_0 + i] = atmosphereInfo.mDensityProfileLayers[ECHO_DF_MIE_0 + i];
				mDensityProfiles[ECHO_DF_ABSORPTION_0 + i] = atmosphereInfo.mDensityProfileLayers[ECHO_DF_ABSORPTION_0 + i];
			}
		}
		void unInit() override { }

		void setMiePhaseFunctionG(float g) { mMiePhaseFunctionG = g; }
		void setRayleighScattering(const Vector3& scattering) { mRayleighScattering = scattering; }
		void setMaxSunZenithAngle(float f) { mMaxSunZenithAngle = f; }
		void setMieScattering(const Vector3& scattering) { mMieScattering = scattering; }
		void setSunAngularRadius(float r) { mSunAngularRadius = r; }
		void setMieExtinction(const Vector3& e) { mMieExtinction = e; }
		void setBottomRadius(float r) { mBottomRadius = r; }
		void setTopRadius(float r) { mTopRadius = r; }
		void setAbsorptionExtinction(const Vector3& e) { mAbsorptionExtinction = e; }
		void setGroundAlbedo(const Vector3& albedo) { mGroundAlbedo = albedo; }

		void setDPWidth(float width, uint8_t type, uint8_t layer) { mDensityProfiles[getDPIndex(type, layer)].mWidth = width; }
		void setDPExpTerm(float t, uint8_t type, uint8_t layer) { mDensityProfiles[getDPIndex(type, layer)].mExpTerm = t; }
		void setHeightScale(float s, uint8_t type, uint8_t layer) { mDensityProfiles[getDPIndex(type, layer)].mHeightScale = s; }
		void setDPLinearTerm(float t, uint8_t type, uint8_t layer) { mDensityProfiles[getDPIndex(type, layer)].mLinearTerm = t; }
		void setDPConstantTerm(float t, uint8_t type, uint8_t layer) { mDensityProfiles[getDPIndex(type, layer)].mConstantTerm = t; }


		float getMiePhaseFunctionG() const { return mMiePhaseFunctionG; }
		const Vector3& getRayleighScattering() const { return mRayleighScattering; }
		float getMaxSunZenithAngle() const { return mMaxSunZenithAngle; }
		const Vector3& getMieScattering() const { return mMieScattering; }
		float getSunAngularRadius() const { return mSunAngularRadius; }
		const Vector3& getMieExtinction() const { return mMieExtinction; }
		float getBottomRadius() const { return mBottomRadius; }
		float getTopRadius() const { return mTopRadius; }
		const Vector3& getAbsorptionExtinction() const { return mAbsorptionExtinction; }
		const Vector3& getGroundAlbedo() const { return mGroundAlbedo; }

		const DPLayer& getDPLayer(uint8_t type, uint8_t layer) const { return mDensityProfiles[getDPIndex(type, layer)]; }

		void apply() const;

	private:
		static uint8_t getDPIndex(uint8_t type, uint8_t layer) { return std::min<uint8_t>(AtmosphereComponentInfo::kMaxDensityProfileLayerCount * type + layer, ECHO_DF_NUM); }

		RenderStrategy* mRenderStrategy;

		float mMiePhaseFunctionG;
		Vector3 mRayleighScattering;
		float mMaxSunZenithAngle;
		Vector3 mMieScattering;
		float mSunAngularRadius;
		Vector3 mMieExtinction;
		float mBottomRadius;
		Vector3 mAbsorptionExtinction;
		float mTopRadius;
		Vector3 mGroundAlbedo;

		std::array<DPLayer, ECHO_DF_NUM> mDensityProfiles;
		//AtmosphereParams mAtmosphereParams;
	};
	DEFINECOMFACTORY(AtmosphereComponent, UNMOVEABLE)
}
#endif
