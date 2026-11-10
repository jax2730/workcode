#ifndef __EchoPlanetConverterComponent_H__
#define __EchoPlanetConverterComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoMaterialComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoImage.h"
#include "EchoThreadDefines.h"
#include "JobSystem/EchoCommonJobManager.h"


namespace Echo
{
#define SPHERE_NUM 4
	class _EchoComponentExport PlanetConverterComponentInfo : public ActorComponentInfo
	{
	public:
		PlanetConverterComponentInfo() {};
		PlanetConverterComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual ~PlanetConverterComponentInfo();

		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* info) override;
		virtual void OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		// Editor
		bool m_UseMesh = true;
		bool m_UsePlanet = true;
		String m_MeshPath;
		String m_TexturePath;
		String m_SaveTexturePath;
		float m_PolarReserve = 0.0f;
		ColorValue m_Color;
		float m_BlurStrength = 0.0f;
		float m_FillStrength = 0.0f;

		// Client
		//String m_HeightMapPath;
		String m_ClientDiffPath;
		String m_NormalMapPath;
		//int m_ResolutionL0 = 2;
		//int m_ResolutionL1 = 2;
		//int m_ResolutionL2 = 2;
		float m_Radius = 100.0f;
		float m_MaxHeight = 1.0f;
		float m_XRollSpeed = 0.0f;
		float m_YRollSpeed = 0.0f;
		float m_ZRollSpeed = 0.0f;
		//Quaternion m_CurrentRotation = Quaternion::IDENTITY;
		//Quaternion m_CurrentRollSpeed  = Quaternion::IDENTITY;

		bool m_EnableAtmosphere = false;
		float m_AtmosphereHeight = 1.0f;
		//int m_AtmosphereResolutionL0 = 2;
		//int m_AtmosphereResolutionL1 = 2;
		//int m_AtmosphereResolutionL2 = 2;

		bool m_EnableCloud = false;
		String m_CloudTexPath;
		float m_CloudHeight = 1.0f;
		//int m_CloudResolutionL0 = 2;
		//int m_CloudResolutionL1 = 2;
		//int m_CloudResolutionL2 = 2;

		bool m_EnableWater = false;
		String m_WaterNormPath;
		float m_WaterHeight = 1.0f;
		//int m_WaterResolutionL0 = 2;
		//int m_WaterResolutionL1 = 2;
		//int m_WaterResolutionL2 = 2;

		// Entity
		int           m_CullMode = 1;
		float         m_CullDistance = 1000.f;
		float         m_TodCullDistance = 1000.f;
		float         m_DissolveStrength = 0.0f;
		int           m_FpsState = 0;
		bool		  m_CastShadow = false;
		float		  m_LodDis_Level1 = 100.0f;
		float		  m_LodDis_Level2 = 200.0f;

		// Material
		std::map<uint32, float> m_matParam;

		//size_t GetResolution(const size_t lod)const;
		//size_t GetAtmosphereResolution(const size_t lod)const;
		//size_t GetCloudResolution(const size_t lod)const;
		//size_t GetWaterResolution(const size_t lod)const;
		float GetCullDistance() const { return m_CullDistance; }

		EDITOR_REFLECT
	};

	class _EchoComponentExport PlanetConverterComponent : public ProtocolNodeActorComponent//, public CommonTask, public CommonTaskListener
	{
		DECLAREACTORCOMTYPE_H(PlanetConverterComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
		void init(ActorComponentInfo* info) override;
		void unInit() override;
		void initRenderObject(ActorComponentInfo* info) override;
		void initHelperObject(ActorComponentInfo* info) override;
		void bindActor() override;
		virtual void registerTickTimeBegin() override;
		virtual void beginFrame(EventID, ProtocolHandle) override;
		//virtual void Execute() override;
		//virtual void CommonTaskFinish(uint64 requestID) override;
		virtual AxisAlignedBox	getLoadBoundingBox() override;
		// 配置好的话只需要执行这个
		void Generate();

		//Vector2 GetUV(const Vector3 vec);
		//bool ModifyTexture();
		void SetTexturePath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_TexturePath = v;
		}
		void SetSaveTexturePath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_SaveTexturePath = v;
		}
		void SetPolarReserve(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_PolarReserve = v;
		}
		void SetColor(ColorValue v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_Color = v;
		}
		void SetBlurStrength(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_BlurStrength = v;
		}
		void SetFillStrength(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_FillStrength = v;
		}

		void SetMeshPath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_MeshPath = v;
		}

		//void SetHeightMapPath(String v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_HeightMapPath = v; UpdateHeight();
		//}

		void SetNormalMapPath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_NormalMapPath = v; UpdateMaterial();
		}
		void SetClientDiffPath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_ClientDiffPath = v; UpdateMaterial();
		}
		//void SetResolutionL0(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_ResolutionL0 = v; UpdateLod(0);
		//}
		//void SetResolutionL1(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_ResolutionL1 = v; UpdateLod(1);
		//}
		//void SetResolutionL2(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_ResolutionL2 = v; UpdateLod(2);
		//}
		void SetRadius(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_Radius = v; UpdateHeight();
		}

		float GetRadius()
		{
			if (m_pActComInfo == nullptr)
				return 0.0f;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return 0.0f;
			return mPlanetConverterComponentInfo->m_Radius;
		}
		void SetMaxHeight(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_MaxHeight = v; UpdateHeight();
		}

		void ResetRotation()
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_XRollSpeed = 0.0f;
			mPlanetConverterComponentInfo->m_YRollSpeed = 0.0f;
			mPlanetConverterComponentInfo->m_ZRollSpeed = 0.0f;

			UpdateRollQuaternion();

			m_CurrentRotation = Quaternion::IDENTITY;
		}

		void SetXRollSpeed(float speed)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_XRollSpeed = speed;
			UpdateRollQuaternion();
		}
		void SetYRollSpeed(float speed)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_YRollSpeed = speed;
			UpdateRollQuaternion();
		}
		void SetZRollSpeed(float speed)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_ZRollSpeed = speed;
			UpdateRollQuaternion();
		}

		void UpdateRollQuaternion()
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			Vector3 rotation = Vector3(mPlanetConverterComponentInfo->m_XRollSpeed, mPlanetConverterComponentInfo->m_YRollSpeed, mPlanetConverterComponentInfo->m_ZRollSpeed);
			m_CurrentRollSpeed = Quaternion::EulerToQuaternion(rotation);
		}

		void SetUseLoadMesh(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_UseMesh = v;
		}

		void SetUsePlanet(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;

			mPlanetConverterComponentInfo->m_UsePlanet = v; UpdatePlanetEnable();
		}

		void SetEnableAtmosphere(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_EnableAtmosphere = v; UpdateAtmosphereEnable();
		}
		void SetAtmosphereHeight(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_AtmosphereHeight = v; UpdateAtmosphereHeight();
		}
		float GetAtmosphereHeight()
		{
			if (m_pActComInfo == nullptr)
				return 0.0f;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return 0.0f;
			return mPlanetConverterComponentInfo->m_AtmosphereHeight;
		}
		//void SetAtmosphereResolutionL0(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_AtmosphereResolutionL0 = v; UpdateAtmosphereLod(0);
		//}
		//void SetAtmosphereResolutionL1(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_AtmosphereResolutionL1 = v; UpdateAtmosphereLod(1);
		//}
		//void SetAtmosphereResolutionL2(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_AtmosphereResolutionL2 = v; UpdateAtmosphereLod(2);
		//}

		void SetEnableCloud(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_EnableCloud = v; UpdateCloudEnable();
		}
		void SetCloudTexPath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_CloudTexPath = v; UpdateCloudMaterial();
		}
		void SetCloudHeight(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_CloudHeight = v; UpdateCloudHeight();
		}
		//void SetCloudResolutionL0(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_CloudResolutionL0 = v; UpdateCloudLod(0);
		//}
		//void SetCloudResolutionL1(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_CloudResolutionL1 = v; UpdateCloudLod(1);
		//}
		//void SetCloudResolutionL2(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_CloudResolutionL2 = v; UpdateCloudLod(2);
		//}

		void SetEnableWater(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_EnableWater = v; UpdateWaterEnable();
		}
		void SetWaterNormPath(String v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_WaterNormPath = v; UpdateWaterMaterial();
		}
		void SetWaterHeight(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_WaterHeight = v; UpdateWaterHeight();
		}

		//void SetWaterResolutionL0(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_WaterResolutionL0 = v; UpdateWaterLod(0);
		//}
		//void SetWaterResolutionL1(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_WaterResolutionL1 = v; UpdateWaterLod(1);
		//}
		//void SetWaterResolutionL2(int v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_WaterResolutionL2 = v; UpdateWaterLod(2);
		//}

		void SetCullMode(int v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_CullMode = v; UpdateEntityParam();
		}

		void SetCullDistance(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_CullDistance = v; UpdateEntityParam();
		}
		void SetTodCullDistance(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_TodCullDistance = v; UpdateEntityParam();
		}
		void SetDissolveStrength(float v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_DissolveStrength = v; UpdateEntityParam();
		}
		void SetFpsState(int v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_FpsState = v; UpdateEntityParam();
		}
		void SetCastShadow(bool v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_CastShadow = v; UpdateEntityParam();
		}
		//void SetLodDis_Level1(float v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_LodDis_Level1 = v; UpdateEntityParam();
		//}
		//void SetLodDis_Level2(float v)
		//{
		//	if (m_pActComInfo == nullptr)
		//		return;
		//	PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
		//	if (mPlanetConverterComponentInfo == nullptr)
		//		return;
		//	mPlanetConverterComponentInfo->m_LodDis_Level2 = v; UpdateEntityParam();
		//}

		struct MaterialParam
		{
			float min;
			float max;
			float step;
			bool isColor;
			uint8 type; // enum EParamType
			String name;
			st_UniformParam param;
			uint32 onlyId; // 球8位 shader8位 后16位是param
		};

		const std::vector<MaterialParam>& GetBaseParam() const { return m_Base.materialParam; }
		void SetBaseParam(const std::map<uint32, float>& v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_matParam = v; m_Base.UpdateParam(this);
		}
		const std::vector<MaterialParam>& GetAtmosphereParam() const { return m_Atmosphere.materialParam; }
		void SetAtmosphereParam(const std::map<uint32, float>& v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_matParam = v; m_Atmosphere.UpdateParam(this);
		}
		const std::vector<MaterialParam>& GetCloudParam() const { return m_Cloud.materialParam; }
		void SetCloudParam(const std::map<uint32, float>& v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_matParam = v; m_Cloud.UpdateParam(this);
		}
		const std::vector<MaterialParam>& GetWaterParam() const { return m_Water.materialParam; }
		void SetWaterParam(const std::map<uint32, float>& v)
		{
			if (m_pActComInfo == nullptr)
				return;
			PlanetConverterComponentInfo* mPlanetConverterComponentInfo = static_cast<PlanetConverterComponentInfo*>(m_pActComInfo);
			if (mPlanetConverterComponentInfo == nullptr)
				return;
			mPlanetConverterComponentInfo->m_matParam = v; m_Water.UpdateParam(this);
		}
		virtual void createComRootNodeImpl() override;

		struct PlanetSphere
		{
			std::array<SmartPtr<Mesh>, 3> mesh;
			SmartPtr<Material> material;
			std::vector<MaterialParam> materialParam;

			const uint8 id;
			const char* shadername = nullptr;
			const char* genmask = nullptr;
			const char* statemask = nullptr;
			const char* entitySuffix = nullptr;
			std::vector<std::pair<String, const String&>> tex;
			std::vector<std::pair<String, const String&>> normal;

			Entity* entity = nullptr;
			SceneManager* m_pSceneManager = nullptr;
			SceneNode* m_pSceneNode = nullptr;
			PlanetSphere(const uint8 id) : id(id){}
			void Unload(PlanetConverterComponent* owner);
			void Load(PlanetConverterComponent* owner);
			void LoadMesh(PlanetConverterComponent* owner, float radius, Name meshPath);
			void UpdateEntity(PlanetConverterComponent* owner);
			void InitParam(PlanetConverterComponent* owner);
			void UpdateParam(PlanetConverterComponent* owner);
		}m_Base{ 0 }, m_Atmosphere{ 1 }, m_Cloud{ 2 }, m_Water{ 3 }; // 套的各个球自己的数据
	private:
		// 球生成系列 - mesh数据
		//void GenerateBase();
		//void GenerateBase(const size_t level);
		//void CopyBase(size_t lod, const size_t level, PlanetSphere& sphere);

		//void GenerateNormal(size_t lod, PlanetSphere& sphere);
		//void GenerateTangent(size_t lod, PlanetSphere& sphere);
		void ModifySize(size_t lod, const float radius, PlanetSphere& sphere);
		//void Displace(size_t lod, const String& heightMapPath, const float maxHeight, PlanetSphere& sphere);
		//// 球生成系列 - 拼装entity与material
		//void AssembleMesh(const size_t lod, PlanetSphere& sphere);
		//void AssembleVertexBuffer(PlanetSphere& sphere);
		//void AssembleMeshData(PlanetSphere& sphere);
		void AssembleMaterial(const bool refresh, PlanetSphere& sphere);
		//void AssembleLod(PlanetSphere& sphere);
		// 参数设置辅助
		void UpdatePlanetEnable();
		void UpdateHeight();
		//void UpdateLod(const size_t lod);
		void UpdateMaterial();

		void UpdateAtmosphereEnable();
		void UpdateAtmosphereHeight();
		//void UpdateAtmosphereLod(const size_t lod);

		void UpdateCloudEnable();
		void UpdateCloudHeight();
		void UpdateCloudMaterial();
		//void UpdateCloudLod(const size_t lod);

		void UpdateWaterEnable();
		void UpdateWaterHeight();
		void UpdateWaterMaterial();
		//void UpdateWaterLod(const size_t lod);

		void UpdateEntityParam();

	private:
		std::array<std::array<std::vector<Vector3>, LOD_LEVEL_NUM>, SPHERE_NUM> m_Vertices;
		std::array<std::array<std::vector<uint16>, LOD_LEVEL_NUM>, SPHERE_NUM> m_Triangles;
		std::array<std::array<std::vector<Vector3>, LOD_LEVEL_NUM>, SPHERE_NUM> m_Normals;
		std::array<std::array<std::vector<Vector2>, LOD_LEVEL_NUM>, SPHERE_NUM> m_UVs;
		std::array<std::array<std::vector<Vector4>, LOD_LEVEL_NUM>, SPHERE_NUM> m_Tangents;

		std::array<std::array<MemoryDataStreamPtr, LOD_LEVEL_NUM>, SPHERE_NUM> m_VertexDataPtr;
		Quaternion m_CurrentRotation = Quaternion::IDENTITY;
		Quaternion m_CurrentRollSpeed = Quaternion::IDENTITY;

		//uint64 m_ticket = 0;
		//PlanetConverterComponentInfo* m_Settings = nullptr; // 。。。那先就这样 拷吧
	};

	DEFINECOMFACTORY(PlanetConverterComponent, ALL_MOVEABLE)
}

#endif
