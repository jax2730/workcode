#ifndef __EchoPlanetEntryControlComponent_H__
#define __EchoPlanetEntryControlComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoAreaManager.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoPlanetConverterComponent.h"
#include "EchoSphericalTerrainFog.h"
namespace Echo
{
	class SphericalTerrainComponent;
	class AtlasSphericalTerrainComponent;

	//大气加一个渐隐
	//用区域解决,在距离星球的一定高度切换进入星球的区域,可以添加TOD效果
	class _EchoComponentExport PlanetEntryControlComponentInfo : public ActorComponentInfo
	{
	public:
		PlanetEntryControlComponentInfo() {};
		PlanetEntryControlComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual ~PlanetEntryControlComponentInfo();

		virtual void			ExportData(cJSON* jsonNode) override;
		virtual void			ImportData(cJSON* jsonNode) override;
		virtual void			CopyTo(ActorComponentInfo* comInfo) override;
		virtual void			OverWrite() override;

		// 星球雾效的参数
		ColorValue				mFogColor				= ColorValue(1.f);
		float					mFogExpTerm				= 0.0f;
		float					mFogExpScale			= 0.0f;
		ColorValue				mHorizonColor			= ColorValue(1.f);
		float					mFogStart				= 0.0f;
		float					mFogEnd					= 0.f;
		float					mColorChangeGradient	= 1.f;
		float					mFogMaxAlpha			= 1.f;
		Vector4					mFogSkyParam			= Vector4(1.f, 1.f, 0.f, 0.f);

		// 星球大气散射的参数
		Vector3					mRayleighScatter	= { 0.05802f, 0.13558f, 0.331f };
		Vector3					mMieScatter			= { 0.03996f, 0.03996f, 0.03996f };
		Vector3					mMieAbsorption		= { 0.0440f, 0.0440f, 0.0440f };
		Vector3					mOzoneAbsorption	= { 0.00650f, 0.01881f, 0.00085f };
		Vector3					mGroundIrradiance	= { 0.5, 0.5f, 0.5f };
		float					mRayleighExpScale	= 0.3f;
		float					mMieExpScale		= 0.064f;
		float					mRayleighExpTerm	= 1.f;
		float					mMieExpTerm			= 1.f;

		// 模拟大气相关的参数
		float					mAtmosApproxAlphaPower		= 1.f;
		float					mAtmosApproxLightPower		= 1.f;
		ColorValue				mAtmosApproxLightColor		= ColorValue(1.f, 1.f, 1.f, 1.f);
		float					mAtmosApproxAverageDensity	= 0.01f;

		// 大气半径相关
		float					mAtmosphereRTop				= 20000.f;
		float					mAtmosphereRTopArchLight	= 20000.f;
		float					mAtmosphereRBottom			= 1.f;
		float					mAtmosphereBlendDistance	= 15000.f;
		float					mAtmosphereBlendRange		= 3000.f;

		// 一些开关
		bool					mEnableAtmosphere	= false;	// 是否启用大气、雾效
		bool					mEnableSecondLayer	= false; // 是否启用两层效果过渡

		// 两层效果根据距离blend
		float					mLayerBlendDistance = 15000.f;
		float					mLayerBlendRange	= 3000.f;

		String					mVersion = "1.0";
#ifdef ECHO_EDITOR
		virtual bool			getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		EDITOR_REFLECT
	};

	class _EchoComponentExport PlanetEntryControlComponent : public ProtocolNodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(PlanetEntryControlComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
		void							init(ActorComponentInfo* info) override;
		void							unInit() override;
		void							connectCom() override;
		void							disconnectCom() override;
		void							connectActor() override;
		void							disconnectActor() override;
		AxisAlignedBox					getLoadBoundingBox() override;
		virtual void					registerTickTimeBegin() override;
		virtual void					beginFrame(EventID, ProtocolHandle) override;
		void							visibleStateChanged(bool visible) override;
		void							UpdateAtmosphere();
		void							UpdateTemplate();

		// 设置星球雾效的接口
		void							SetFogColor(const ColorValue& val);
		void							SetFogExpScale(float val);
		void							SetFogExpTerm(float val);
		void							SetHorizonColor(const ColorValue& val);
		void							SetFogStart(float val);
		void							SetFogEnd(float val);
		void							SetColorChangeGradient(float value);
		void							SetMaxAlpha(float val);
		void							SetSkyParam(float val);

		// 设置模拟大气弧光效果的接口
		void							SetApproxAlphaPower(float val);
		void							SetApproxLightPower(float val);
		void							SetApproxLightColor(const ColorValue& val);
		void							SetApproxAverageDensity(float val);

		// 设置大气散射相关参数的接口
		void							SetRayleighScatter(const Vector3& rs);
		void							SetMieScatter(const Vector3& ms);
		void							SetMieAbsorption(const Vector3& ma);
		void							SetOzoneAbsorption(const Vector3& oa);
		void							SetGroundIrradiance(const Vector3& groundIrradiance);
		void							SetRayleighExpScale(float rayleighExpScaleOutterSpace);
		void							SetMieExpScale(float mieExpScaleOutterSpace);
		void							SetRayleighExpTerm(float rayleighExpTermOutterSpace);
		void							SetMieExpTerm(float mieExpTermOutterSpace);

		// 设置大气半径的接口
		void							SetAtmosphereRTop(float rTop);
		void							SetAtmosphereRTopArchLight(float rTopArchLight);
		void							SetAtmosphereRBottom(float rBottom);

		// 设置大气过渡范围的接口
		void							SetAtmosBlendRange(float blendRange);
		void							SetAtmosBlendDistance(float blendDistance);

		// 效果开关
		void							SetEnable(bool val);
		void							SetEnableSecondLayerEffect(bool val);
		
		// 往后期设置参数的接口
		void							SetAtmosphere();
		void							SetFog();
		void							SetAtmosApprox();
		void							SetAtmosphereRange();
		void							EnableFog();
		void							EnableSecondLayerAtmosphere();

		void							SetSphericalTerrainComPtr(SphericalTerrainComponent* ptr);
		void							SetAtlasSphericalTerrainComPtr(AtlasSphericalTerrainComponent* ptr);

		SphericalTerrainAtmosphereApproxProfile*		GetApproxProfile();
		SphericalTerrainAtmosphereBoundary*				GetBoundary();
		SphericalTerrainAtmosphereProperty*				GetAtmosphereProperty();

	public:
		SphericalTerrainAtmosphereProperty				mAtmosProperty;
		bool							mPropertyDirty = true;

		// 现在参数直接往星球雾后期里面塞，需要一个星球雾后期的指针
		SphericalTerrainFog*			mFog = nullptr;

		Camera*							mMainCamera = nullptr;
		SphericalTerrainComponent*		mSphericalTerrainComponent = nullptr;			// 真实星球
		AtlasSphericalTerrainComponent* mAtlasSphericalTerrainComponent = nullptr;		// 小地图星球
		SphericalTerrain*				mSphericalTerrain = nullptr;
	private:
	};
	DEFINECOMFACTORY(PlanetEntryControlComponent, UNMOVEABLE)
}
#endif
