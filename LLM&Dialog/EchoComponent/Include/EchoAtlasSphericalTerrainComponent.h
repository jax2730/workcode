#ifndef EchoAtlasSphericalTerrainComponent_h__
#define EchoAtlasSphericalTerrainComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainComponent.h"

namespace Echo
{
	class PlanetEntryControlComponent;
	class AtlasSphericalTerrainComponent;
	typedef EchoWeakPtr<AtlasSphericalTerrainComponent> AtlasSphericalTerrainComponentPtr;


	struct AtlasSphericalData {
		float    m_WarFogYawAngle = 0.f;    //偏航
		float    m_WarFogPitchAngle = 90.f; //俯仰
		float    m_WarFogFlowingWidth = 0.1f;
		ColorValue  m_WarFogFlowingColor = ColorValue::White;
		float    m_WarFogFlowingSpeed = 0.1f;
		float    m_WarFogFlowingHDRFactor = 1.f;
		bool     m_bUseWarFogFlowingLight = false;

		bool     m_bRegionVisible = false;
		std::string m_WarFogTexture = "echo/biome_terrain/Textures/point.dds";
		ColorValue m_WarFogColor = ColorValue::Red;

		float       m_WarFogDissolveTillingScale = 1.f;
		std::string m_WarFogDissolveTexture = "";
		Vector3     m_WarFogDissolveColor = Vector3::UNIT_SCALE;
		float       m_WarFogDissolveColorValue = 0.f;
		float       m_WarFogDissolveSoftValue = 0.f;
		float       m_WarFogDissolveWidth = 0.f;

		float       m_OutLineWidth = 0.1f;
#ifdef ECHO_EDITOR
		//下面参数仅用于ui看效果用
		int         m_EditorRegionID = 0; 
		bool        m_bEffectAllRegion = false;
		float       m_DissolveEditorValue = 0.f;
#endif
	};

	class _EchoComponentExport AtlasSphericalTerrainComponent : public ProtocolActorComponent, public SphericalTerrain::LoadListener
	{
	public:
		DECLAREACTORCOMTYPE_H(AtlasSphericalTerrainComponent)
		REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void visibleStateChanged(bool activeState) override;
		virtual void OnCreateFinish() override;

		/*void    testFunc(float alph_);
		int width = 0,height = 0;*/

		void OnDestroy() override;

		virtual void registerTickTimeBegin() override;
		virtual void beginFrame(EventID, ProtocolHandle handle) override;
		virtual AxisAlignedBox getLoadBoundingBox() override;
	public:
		const String& GetTerrainPath() { return m_sTerrainPath; }
		void SetTerrainPath(const String& name);

		inline bool IsExistTerrain() { return m_pSphericalTerrain != nullptr; }
		bool IsCreateFinish() const;
	public:
		float getMaxRealRadius() const;

		void setToggleEnable(bool value);
		bool getToggleEnable() { return m_bToggleState; }
		void setToggleDistance(float dist);
		float getToggleDistance() const { return m_fToggleDistance; }

		void setDissolveTime(float value);
		float getDissolveTime() const { return m_fDissolveTime; }
        //NOTE: Visual effect control [Region display on the planet]. 
        //NOTE: selected region will be highlight display.
        void enableRegionVisualize(bool enable);
		bool getEnableRegionVisualize() { return mContext.m_bRegionVisible; }

        bool setHighlightFineRegionID(int id);
        void resetHighlightFineRegionID();
        
        bool setHighlightCorseRegionID(int id);
        void resetHighlightCorseRegionID();
        
        void setDisplayCorseOrFineRegion(bool enableCorse);
        
        void setRegionGlowColor(const ColorValue& color);

        void setRegionFillColor(const ColorValue& color);
        void setCoarseRegionFillColor(const ColorValue& color);

        void setSingleRegionFillColor(int id, const ColorValue& color);
        void setSingleCoarseRegionFillColor(int id, const ColorValue& color);
        
        void setRegionHighlightColor(const ColorValue& color);

        void setRegionHighlightWarfogColor(const ColorValue& color);
        void setWarfogMaskUVScale(const float scale);
        void setWarfogMaskScale(const float scale);
        void setOutlineSmoothRange(const float range);
        
        void setOutlineWidth(float width);
        void setCommonRegionAlpha(float alpha);
        void setHighlightRegionAlpha(float alpha);

        void setAdditionOutLineColor(const ColorValue& color);
        void setAdditionOutLineAlpha(const float& v);
        void setAdditionOutLineWidth(const float& v);
        void setAdditionOutLineDistance(const float& v);

        void setRegionWarfogAlpha(int id, float alpha);
        void setCoarseRegionWarfogAlpha(int id, float alpha);
        
        void setRegionWarfogColor(const ColorValue& color);
		const ColorValue& getRegionWarFogColor() { return mContext.m_WarFogColor; }

        void setWarfogMaskTexture(const String& path);
		const std::string& getWarfogMaskTexture() { return mContext.m_WarFogTexture; }
		void setWarFogSelfEmissiveLightColor(const ColorValue& color);
		void setWarFogSelfEmissiveLightPower(float power);



		void setWarFogFlowingDirByYawAndPitch(float yaw, float pitch);
		void setWarFogFlowingYawAngle(float yaw); 
		void setWarFogFlowingPitchAngle(float pitch); 

		//void setWarFogFlowingDir(const Vector3& dir);
		void setWarFogFlowingWidth(float width);
		void setWarFogFlowingSpeed(float speed);
		void setWarFogFlowingColor(const ColorValue& color);
		void setWarFogFlowingHDRFactor(float factor);
		void setWarFogIsUseFlowingLight(bool bUse);

		float getWarFogFlowingYawAngle() { return mContext.m_WarFogYawAngle; }
		float getWarFogFlowingPitchAngle() { return mContext.m_WarFogPitchAngle; }
		float getWarFogFlowingWidth() { return mContext.m_WarFogFlowingWidth; }
		float getWarFogFlowingSpeed() { return mContext.m_WarFogFlowingSpeed; }
		const ColorValue& getWarFogFlowingColor() { return mContext.m_WarFogFlowingColor; }
		float getWarFogFlowingHDRFactor() { return mContext.m_WarFogFlowingHDRFactor; }
		bool  getWarFogIsUseFlowingLight() { return mContext.m_bUseWarFogFlowingLight; }

		void  setWarFogDissolveTexture(const std::string& path);
		std::string& getWarFogDissolveTexture() { return mContext.m_WarFogDissolveTexture; }
		void setWarFogDissolveTillingScale(float scale);
		float getWarFogDissolveTillingScale() { return mContext.m_WarFogDissolveTillingScale; }
		void setWarFogDissolveSoftValue(float value);
		float getWarFogDissolveSoftValue() { return mContext.m_WarFogDissolveSoftValue; }
		void setWarFogDissolveWidth(float width);
		float getWarFogDissolveWidth() { return mContext.m_WarFogDissolveWidth; }
		void setWarFogDissolveColor(const Vector3& color);
		const Vector3& getWarFogDissolveColor() { return mContext.m_WarFogDissolveColor; }
		void setWarFogDissolveColorValue(float value);
		float getWarFogDissolveColorValue() { return mContext.m_WarFogDissolveColorValue; }

		float getOutLineWidth() { return mContext.m_OutLineWidth; }

		void setTempPlanetMesh(const Name& meshName);

		void setTempPlanetPosition(const DBVector3& pos);
		const DBVector3& getTempPlanetPosition();

		void setTempPlanetOrientation(const Quaternion& qua);
		const Quaternion& getTempPlanetOrientation();

		void setTempPlanetScale(const Vector3& scale);
		const Vector3& getTempPlanetScale();

		void setTempPlanetWorldPosition(const DBVector3& pos);
		const DBVector3& getTempPlanetWorldPosition();

		void setTempPlanetWorldOrientation(const Quaternion& qua);
		const Quaternion& getTempPlanetWorldOrientation();

		void setTempPlanetWorldScale(const Vector3& scale);
		const Vector3& getTempPlanetWorldScale();

		CameraCullMode getTempPlanetCullMode();
		void setTempPlanetCullMode(int mode = 0);

		void setTempPlanetRenderDistance(float value);
		float getTempPlanetRenderDistance();

#ifdef ECHO_EDITOR
		void  setEditorRegionID(int id);
		int   getEditorRegionID() { return mContext.m_EditorRegionID; }
		void  setWarFogEditorDissolveValue(float value);
		float getWarFogEditorDissolveValue() { return mContext.m_DissolveEditorValue; }
		void  setEnableEffectAllRegion(bool isEffect);
		bool  getEnableEffectAllRegion() { return mContext.m_bEffectAllRegion; }

#endif

		void  setWarFogDissolveValue(int i, float value);

		//NOTE:Low LOD Cloud visual effect control.
		void setEnableLowLODCloud(bool enalbe);
		void setCloudUVScale(float v);				// v range: [0 ~ 100]
		void setCloudFlowSpeed(float v);			// v range: [0 ~ 10]
		void setCloudHDRFactor(float v);			// v range: [0 ~ 10]
		void setCloudMetallic(float v);				// v range: [0 ~ 1]
		void setCloudRoughness(float v);			// v range: [0 ~ 1]
		void setCloudAmbientFactor(float v);		// v range: [0 ~ 1]
		void setCloudNoiseOffsetScale(float v);		// v range: [0 ~ 1]
        void setCloudTexture(const String& path);

		bool  getEnableLowLODCloud();
		float getCloudUVScale();
		float getCloudFlowSpeed();
		float getCloudHDRFactor();
		float getCloudMetallic();
		float getCloudRoughness();
		float getCloudAmbientFactor();
		float getCloudNoiseOffsetScale();
		const String& getCloudTexture();

		DVector3 getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceLs(const Vector3& localPos, Vector3* faceNormal = nullptr) const;

		DVector3 getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal = nullptr) const;
		Vector3 getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal = nullptr) const;

		int getRegionID(const DVector3& worldPos) const;
		NaiveWrapper<SphericalVoronoiRegionDefine> getSphericalVoronoiRegionDefine(int id);
		std::vector<Vector3> GetCoarseLayerCenters(bool ToWorldSpace = false) const;
		std::vector<Vector3> GetFineLayerCenters(bool ToWorldSpace = false) const;
		std::vector<int> GetCoarseLayerAdjacentRegions(int id) const;
		std::vector<int> GetFineLayerAdjacentRegions(int id) const;
		void SetAtmosPropertyDirty();
		bool SetPlanetEntryComPtr(PlanetEntryControlComponent* ptr);

		void setEnablePlotDivision(bool bEnable);
		bool getEnablePlotDivision() { return m_bEnablePlotDivision; }

		//PTS_Torus类型的星球 idx传0 || PTS_Sphere idx 0 - 前 ，1 - 后， 2 - 左 ，3 - 右 ，4 - 上 ，5 -下 
		void setPlotDivisionColor(int idx, int x, int y, const ColorValue& color);
		void setPlotDivisionNums(int x,int y);

		bool getPlotDivisionDataByPosition(const Vector3& pos, int& idx, int& x, int& y);
		bool getPlotDivisionDataByWorldPosition(const DVector3& pos, int& idx, int& x, int& y);

		const EntityObject* getTempPlanet() const { return m_pTempPlanet; }
	protected:
		void UpdateTerrainAtmos();
		void CreateTerrain();
		void DestroyTerrain();
		void ChangeTerrain();
		void setForceLowLod(bool value);

		void loadTempPlanet(const Name& meshPath);
		//void UpdateLowLodCloudParameter();

		SphericalTerrain* GetSphericalTerrain() { return m_pSphericalTerrain; }
	private:
		PlanetEntryControlComponent* m_AtmosCom = nullptr;
		String m_sTerrainPath;
		float m_fToggleDistance = 1000.0f;
		bool m_bToggleState = false;
		float m_fDissolveTime = 0.0f;
		float m_fDissolveTimeBegin = 0.0f;
		SphericalTerrain* m_pSphericalTerrain = nullptr;
		EntityObject* m_pTempPlanet = nullptr;

		bool	m_bEnableLowLODCloud = false;
		float	m_fCloudUVScale = 1.0f;
		float	m_fCloudFlowSpeed = 1.0f;
		float	m_fCloudHDRFactor = 1.0f;
		float	m_fCloudMetallic = 0.0f;
		float	m_fCloudRoughness = 1.0f;
		float	m_fCloudAmbientFactor = .05f;
		float	m_fCloudNoiseOffsetScale = .03f;
		String  m_strCloudTexture = "";

		bool    m_bEnablePlotDivision = false;

		Name m_nTempPlanetPath;

		AtlasSphericalData mContext;
	};

	class _EchoComponentExport AtlasSphericalTerrainComponentInfo : public ActorComponentInfo {
	public:
		AtlasSphericalTerrainComponentInfo() {}
		AtlasSphericalTerrainComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual ~AtlasSphericalTerrainComponentInfo() {};

#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		_DECLCOMINFO_VIRFUN
	public:
		AtlasSphericalData m_AtlasSphericalData;

		_DECLCOMINFO_PARAM_BEGIN(AtlasSphericalTerrainComponent)
		_DECLCOMINFO_PARAM(String, TerrainPath, String(""), SetTerrainPath)
		_DECLCOMINFO_PARAM(Name, TempPlanetPath, Name::g_cEmptyName, setTempPlanetMesh)

		_DECLCOMINFO_PARAM_SIM(float, DissolveTime, 0.0f, setDissolveTime)
		_DECLCOMINFO_PARAM_SIM(bool, EnableLowLODCloud, false, setEnableLowLODCloud)
		_DECLCOMINFO_PARAM_SIM(float, CloudUVScale, 1.0f, setCloudUVScale)
		_DECLCOMINFO_PARAM_SIM(float, CloudFlowSpeed, 1.0f, setCloudFlowSpeed)
		_DECLCOMINFO_PARAM_SIM(float, CloudHDRFactor, 1.0f, setCloudHDRFactor)
		_DECLCOMINFO_PARAM_SIM(float, CloudMetallic, 0.0f, setCloudMetallic)
		_DECLCOMINFO_PARAM_SIM(float, CloudRoughness, 1.0f, setCloudRoughness)
		_DECLCOMINFO_PARAM_SIM(float, CloudAmbientFactor, .05f, setCloudAmbientFactor)
		_DECLCOMINFO_PARAM_SIM(float, CloudNoiseOffsetScale, .03f, setCloudNoiseOffsetScale)
		_DECLCOMINFO_PARAM(String, CloudTexture, "", setCloudTexture)
			//_DECLCOMINFO_PARAM_SIM(AtlasSphericalData, AtlasSphericalData,AtlasSphericalData(), setAtlasSphericalData)
		_DECLCOMINFO_PARAM_END()
	};

	class _EchoComponentExport AtlasSphericalTerrainComponentFactory : public Echo::ActorComFactory
	{
		friend class AtlasSphericalTerrainComponent;
		static_assert(std::is_same<decltype(UNMOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		AtlasSphericalTerrainComponentFactory();
		virtual ~AtlasSphericalTerrainComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new AtlasSphericalTerrainComponent(pActor, UNMOVEABLE), Echo::ActorComponentPtrDeleter); }
		virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new AtlasSphericalTerrainComponentInfo(mType, UNMOVEABLE); }

		void setToggleEnable(bool value);
		bool getToggleEnable() const { return m_bToggleEnable; }
	protected:
		void setComponentToggleEnable(const EchoWeakPtr<AtlasSphericalTerrainComponent>& pComponent, bool value);
		const Echo::EchoWeakPtr<AtlasSphericalTerrainComponent>& getToggleComponent() const { return m_pToggleComponent; }
	private:
		Echo::EchoWeakPtr<AtlasSphericalTerrainComponent> m_pToggleComponent;
		bool m_bToggleEnable = true;
	};
}
#endif
