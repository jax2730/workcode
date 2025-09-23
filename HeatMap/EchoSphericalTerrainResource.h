//-----------------------------------------------------------------------------
// File:   EchoSphericalTerrainResource.h
//
// Author: qiaoyuzhi
//
// Date:  2024-5-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoSphericalTerrainResource_h__
#define EchoSphericalTerrainResource_h__

#include "EchoPrerequisites.h"
#include "EchoHeightMapStamp3D.h"
#include "EchoResourceBase.h"
#include "EchoResourceManagerBase.h"
#include "EchoMath.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoOrientedBox.h"
#include "EchoBiomeVegetation.h"
#include "EchoBiomeVegLayerCluster.h"
#include "EchoPlanetRegionResource.h"
#include <array>
#include <mutex>
#include "EchoWorkQueueBase.h"
#include "EchoBiomeGenObjectLibrary.h"

#define MAX_REGION_STREX_SIZE 256
namespace Echo {
	class TerrainGenerator;
    class PlanetTextureResource;
	class SphericalTerrainResource;
    typedef SmartPtr<SphericalTerrainResource> SphericalTerrainResourcePtr;
    typedef SmartPtr<PlanetTextureResource> PlanetTextureResourcePtr;
	class PlanetRoadResource;
	typedef SmartPtr<PlanetRoadResource> PlanetRoadPtr;


	struct ValueRange {
		float min = 0;
		float max = 0;
	};

    struct SnowTemplateData
    {
        //TODO(dsw):Give it default value.
        float snowHeight = 0.0f;
        float tile = 6.0f;
        float displacement = 0.20974f;
        float functionOffset = 0.32f;
        float steepness = 3.408f;
        float snowRange = 0.204f;
        float snowScale = 1.396f;
        float shapeSmoothStart = 0.0f;
        float shapeSmoothScale = 0.0f;
    };
    
    struct _EchoExport BiomeTemplate
    {
        //IMPORTANT(yanghang): Unique identifier in library.
        const static uint32 s_invalidBiomeID;
        uint32 biomeID = s_invalidBiomeID;

        //TODO(yanghang): Editor use only.
        String name;
        
        String diffuse;
		String normal;

		ColorValue emission = ColorValue(0.f, 0.f, 0.f, 0.f);

		bool enableTextureEmission = false;
		float customMetallic = 1.f;

		float customTilling = 1;
		float normalStrength = 1;

		//NOTE(yanghang):Record physical surface type.
		uint32 phySurfaceType = 0;
		Vector4 color;

		//NOTE:Use this color to blend diffuse texture.
		ColorValue diffuseColor = ColorValue::White;

        SnowTemplateData snow;
        
		[[nodiscard]] ColorValue getsRGBFinalColor() const;
	};

	struct Biome
	{
		BiomeTemplate biomeTemp;
		BiomeVegetation::VegLayerInfoArray vegLayers;
		PCG::BiomeGenObjectInfo genObjects;
	};

	struct GetBiomeResult
	{
		BiomeTemplate biome;
		bool success;
	};


	class _EchoExport BiomeLibrary
	{
	public:
		BiomeLibrary();

		bool load();
		//NOTE: Editor function.
		void save();
		GetBiomeResult getBiome(const String& name);
		GetBiomeResult getBiome(const uint32& biomeID);

	public:
		const static int s_maxGlobalBiomeCount;
		//NOTE:How many different textures(diffuse/normal) used in the library.
		const static int s_maxTextureLimit;
		std::vector<BiomeTemplate> m_allBiomes;
		String m_filePath;

		//NOTE:Internal use only.
		bool m_internalMode = false;
		static bool m_enableUnlimitBiomeCount;
		static int m_unlimitBiomeCount;
	};

	//云参数
	struct PlanetCloudTemplateData
	{
		float       m_UVScale = 1.0f;
		float       m_flowSpeed = 1.0f;
		float       m_hdr = 1.0f;
		float       m_metallic = 0.0f;
		float       m_roughness = 1.0f;
		float       m_ambientFactor = 0.05f;
		float       m_noiseOffsetScale = 0.03f;
		float       m_yaw = 0.f;
		float       m_pitch = 90.f;
		std::string m_DiffusePath = "echo/biome_terrain/Textures/cloud.dds";

		std::string m_TemplateName;
	};

	class _EchoExport PlanetCloudLibrary
	{
	public:
		PlanetCloudLibrary() = default;

		bool load();
		void save();

		bool                                   getCloudTemplateData(int id, PlanetCloudTemplateData& data) const;
		std::vector<PlanetCloudTemplateData>&  getCloudLibraryData() { return m_AllClouds; }
		bool                                   judgeIsExitTemplateByName(const std::string& name);

		void                                   addPlanetCloudTemplate(const std::string& name);
		void                                   addPlanetCloudTemplate(const PlanetCloudTemplateData& data);

		std::int32_t                           getCloudLibraryMaxSize() { return m_MaxCount; }
#ifdef ECHO_EDITOR  
		std::string                            m_TextureFolderPath = "biome_terrain/Textures";
#endif //  


	private:
		std::int32_t                           m_MaxCount = 64;
		std::string                            m_FilePath = "biome_terrain/default.cloudlibrary";
		std::vector<PlanetCloudTemplateData>   m_AllClouds;
	};


	struct RegionDefine
	{
#ifdef ECHO_EDITOR
		String name;
		ValueRange humidity;
		ValueRange temperature;
		Vector4 color;//只用于编辑器UI界面
#endif
		String TemplateTOD;
		String StrEx;
		int IntEx = 0;
	};


	struct BitmapCube
	{
		Bitmap maps[6];
	};

	struct ImageProcess
	{
		bool invert = false;
		float exposure = 0;
		float exposureOffset = 0;
		float exposureGamma = 1.0f;
		int brightness = 0;
		ValueRange clamp = { 0, 1.0f };

		bool operator==(const ImageProcess& r) const
		{
			const float EPSINON = 0.00001f;
			return r.invert == invert
				&& std::abs(r.exposure - exposure) < EPSINON
				&& std::abs(r.exposureOffset - exposureOffset) < EPSINON
				&& std::abs(r.exposureGamma - exposureGamma) < EPSINON
				&& r.brightness == brightness
				&& std::abs(r.clamp.min - clamp.min) < EPSINON
				&& std::abs(r.clamp.max - clamp.max) < EPSINON;
		}

		bool operator!=(const ImageProcess& r) const
		{
			return !operator==(r);
		}
	};

	struct AoParameter
	{
		float radius = 0.01f;
		float amplitude = 1000.0f;

		bool operator==(const AoParameter& r) const
		{
			constexpr float eps = 1e-5f;
			return Math::FloatEqual(r.radius, radius, eps)
				&& Math::FloatEqual(r.amplitude, amplitude, eps);
		}

		bool operator!=(const AoParameter& r) const
		{
			return !operator==(r);
		}
	};

	struct Noise3DData
	{
		Vector3 featureNoiseSeed = Vector3(0.0f);
		float lacunarity = 2.01f;
		float baseFrequency = 1.0f;
		float baseAmplitude = 1.0f;
		float detailAmplitude = 1.0f;
		float gain = 0.5f;
		int baseOctaves = 10;
		Vector3 sharpnessNoiseSeed = Vector3(1.0f, 2.0f, 3.0f);
		Vector2 sharpnessRange = Vector2(0.0f, 1.0f);
		float sharpnessFrequency = 1.5f;
		Vector3 slopeErosionNoiseSeed = Vector3(4.0f, 5.0f, 6.0f);
		Vector2 slopeErosionRange = Vector2(0.0f, 1.0f);
		float slopeErosionFrequency = 1.5f;
		Vector3 perturbNoiseSeed = Vector3(7.0f, 8.0f, 9.0f);
		Vector2 perturbRange = Vector2(0.0f, 0.2f);
		float perturbFrequency = 1.5f;
		float terrainMinBorder = -1.0f;
		float terrainBorderSize = 0.0f;
		float terrainBorderHeight = 0.0f;
	};

	struct StampTerrainData : public StampTerrain3D::Base
	{
		int InstanceID{ -1 };
		String templateID;
		HeightMapDataPtr mHeightmap;
	};

	struct RegionUniform
	{
		float	innerRadius{ 1.0f };
		float	outerRadius{ 2.0f };
		float	lambda{ 0.50f };
		float	threshold{ 0.00f };
	};

	struct RegionData
	{
		String name;
		std::vector<RegionDefine> biomeArray;
		std::array<Bitmap, 6> RegionMaps{};

		bool Loaded = false;


#ifdef ECHO_EDITOR
		// Editor only
		ValueRange humidityRange;
		ValueRange temperatureRange;
		RegionUniform uniform;
		int RegionNum = 0;
#endif

	};

	struct SphericalVoronoiRegionDefineImpl
	{
		String StrEx;
		int IntEx = 0;
	};

	struct SphericalVoronoiRegionDefineBase : public SphericalVoronoiRegionDefineImpl
	{
#ifdef ECHO_EDITOR
		String name;
#endif
	};

	struct SphericalVoronoiRegionDefine : public SphericalVoronoiRegionDefineBase
	{
		int Parent = -1;
		String TemplateTOD;
	};


#ifdef ECHO_EDITOR
	struct _EchoExport SphericalVoronoiRegionData
#else
	struct SphericalVoronoiRegionData
#endif	
	{
		enum ErrorType
		{
			INCOMPATIBLE = -1,
			ILLEGAL = 0,
			COMPATIBLE = ILLEGAL,
			LEGAL = 1
		};
		std::atomic<bool> Loaded{ false };
		PlanetRegionPtr mData;
		Vector2 offset = Vector2::ZERO;
		String prefix;
		String coarse_prefix;
		std::vector<SphericalVoronoiRegionDefine> defineArray; // fine regions
		std::vector<SphericalVoronoiRegionDefineBase> defineBaseArray; // coarse regions
		std::vector<std::vector<int>> coarse_adjacent_regions; // coaser regions
		std::vector<Vector3> coarse_centers; // coaser regions


		SphericalVoronoiRegionData() = default;
		SphericalVoronoiRegionData(const SphericalVoronoiRegionData& Other);
		SphericalVoronoiRegionData& operator=(const SphericalVoronoiRegionData& Other);
		~SphericalVoronoiRegionData() = default;

	public:
		bool IsFineLevelDataLegal() const;
		ErrorType IsCoarseLevelDataLegal() const;
		bool SelectCoarseLevelCenter();

		static bool computeRegionParentIdPlane(
			const std::string& prefix, const String& ErrorHead, const std::vector<std::vector<int>>& FineAdjacency,
			std::vector<int>& parentArray, int MaxSelect = 4);

#ifdef ECHO_EDITOR
		bool GenerateFineLevelData(const PlanetRegionParameter& ParamsRef);
		bool GenerateCoarseLevelData();
	private:
		bool GenerateHierachicalStructure();
		bool GenerateCoarseLevelAdjacency();
#endif

	};

	//NOTE:Control biome display in what range of temperature and humidity.
	struct BiomeDistributeRange
	{
		//NOTE:Editor for note.
		String name;

		ValueRange humidity;
		ValueRange temperature;
	};

	struct ClimateProcessData
	{
		ImageProcess humidityIP;
		ImageProcess temperatureIP;
		AoParameter aoParam;
	};

	struct BiomeDistribution
	{
		String filePath;

		ClimateProcessData climateProcess;
		std::vector<BiomeDistributeRange> distRanges;
		//NOTE:the whole range of the planet.
		ValueRange humidityRange;      //湿度
		ValueRange temperatureRange;   //温度

		Vector2 voronoiSeed{ 0.0f, 0.0f };
		int	voronoiSqrtQuads{ 2 }; // voronoiSqrtQuads^2 * 6 regions on sphere
	};

	struct SphereWaveInfo {
		float amp = 0.1f;   // amplitude
		float sharp = 0.0f; // sharpness
		float freq = 1.f;  // 2*PI / wavelength
		float phase = 1.0f; // speed * 2 * PI / wavelength
		Vector3 dir;// sphereical wave ori on the unit sphere
	};

	struct SphereOceanFileSettings {
		String MeshFile;
		String NormalTextureName;
		String ReflectTextureName;
		String CausticTextureName;
		String TunnelTextureName;
		String FoamTextureName;
	};

	struct SphereOceanGeomSettings {
		float radius = 1.0f;
		float relativeMinorRadius = 0.05f;
		int resolution = 32;
		int waveNum = 4;
		int maxDepth = 6;
		SphereWaveInfo sphereWaveInfo[4];
	};

	struct SphereOceanShadeSettings {
		ColorValue shallowColor = ColorValue(42.f / 255.f, 179.f / 255.f, 197.f / 255.f, 1.0f);
		ColorValue deepColor = ColorValue(23.f / 255.f, 105.f / 255.f, 253.f / 255.f, 1.0f);
		ColorValue m_foamColor = ColorValue::White;
		ColorValue m_reflectColor = ColorValue::Blue;

		Vector2 textureScale;

		float waveFrequency = 0.0f;
		float curWaveAmplitude = 0.0f;
		float bumpScale = 0.0f;
		float flowSpeed = 0.0f;

		float reflectionAmount = 0.0f;
		float waveAmount = 0.0f;
		float flowParam_z = 0.0f;
		float flowParam_w = 0.0f;

		float fresnelBias = 0.0f;
		float fresnelIntensity = 0.0f;
		float fresnelRatio = 0.0f;

		float foamBias = 0.0f;
		float foamSoft = 0.0f;
		float foamTilling = 0.0f;
		float borderSoft = 0.0f;

		float specularPower = 0.0f;
		float specularStrength = 0.0f;
		//An artist sitisfied default value
		float lightStrengthRatio = .3f;
		float reflectionBlur = 0.0f;
		float refractAmount = 0.0f;
	};

	struct SphericalOceanData
	{
		String filePath;
		bool haveOcean = false;
		SphereOceanFileSettings fileSetting;
		SphereOceanGeomSettings geomSettings;
		SphereOceanShadeSettings shadeSettings;
		bool m_bUseTod = false;
	};


	struct RegionGPUData
	{
		static const int lookupTexWidth = 256;
		static const int lookupTexHeight = 256;
		static const int lookupTexelSize = sizeof(uint16);
		TexturePtr RegionlookupTex;
		// int RegionNum{ 1 };
	};

	struct BiomeGPUData
	{
		//IMPORTANT:Gray map's value is 8bit uint8: range is [0 ~ 255].
		// So lookup map's dimension is 256 pixels.
		// 255 * 255 = 65535 kinds biome type.
		static constexpr int lookupTexWidth = 256;
		static constexpr int lookupTexHeight = 256;
		using lookupTexel = uint8_t;
		static constexpr int lookupTexelSize = sizeof(lookupTexel);
		static constexpr auto lookupTexFormat = ECHO_FMT_R8UI;


		TexturePtr diffuseTexArray;
		TexturePtr normalTexArray;
        TexturePtr matIDTex[6];
	};

	struct BiomeDefine_Cache
	{
		DataStreamPtr diffuse;
		DataStreamPtr normal;
	};

	struct BiomeGPUData_Cache
	{
		Bitmap cpuData;
		DataStreamPtr defaultLayerTexData;
		std::vector<BiomeDefine_Cache> biomeArray;
	};

	enum ProceduralTerrainTopology : uint8_t
	{
		PTS_Sphere = 0,
		PTS_Torus
	};

	struct SphereGeometry
	{
		float radius = 2000.0f;
		float elevationRatio = 0.05f;
		int gempType = -1;
	};

	struct TorusGeometry
	{
		float radius = 2000.0f;
		float elevationRatio = 0.2f;
		float relativeMinorRadius = 0.3125f;
		int maxSegments[2]{ 3, 1 };
	};

	class _EchoExport TerrainGeneratorLibrary : public Singleton<TerrainGeneratorLibrary>
	{
	public:
		TerrainGeneratorLibrary();
		~TerrainGeneratorLibrary();

		TerrainGenerator* get(uint32 type) const;
		std::shared_ptr<TerrainGenerator> add(uint32 type) const;
		void remove(std::shared_ptr<TerrainGenerator>& ptr) const;
		const std::map<uint32, String>& getTypes() const { return mTypes; }
		const std::map<uint32, String>& getNames() const { return mNames; }
		const std::unordered_map<uint32, std::shared_ptr<TerrainGenerator>>& getDatas() const { return mDataMaps; }

		bool load();
		void save();
		void clear();
	private:
		std::unordered_map<uint32, std::shared_ptr<TerrainGenerator>> mDataMaps;
		std::map<uint32, String> mTypes;
		std::map<uint32, String> mNames;
	};

	namespace SphericalPlanetGeometryType {
		struct PlanetGeometryType {
			int type = -1;
			std::string file;
		};
		struct PlanetGeometryTypeLibrary {
			std::vector<PlanetGeometryType> types;
		};
		struct PlanetGeometry {
			std::vector<Vector3> vertexs;
			std::vector<uint16> triangles;
		};
		_EchoExport bool DataParse(PlanetGeometryTypeLibrary& types, const char* data);
		_EchoExport bool DataParse(PlanetGeometry& geom, const char* _pData, size_t _nData);
	}

	struct SphericalTerrainAtmosData: public Singleton<SphericalTerrainAtmosData> {
		void load();
		bool loadMesh();
		bool loadMaterial();
		MeshPtr mAtmosMesh;
		MaterialPtr mAtmosMaterial;
		bool mLoaded = false;
	};

	struct TerrainGeometry
	{
		String filePath;
		Noise3DData noise;
		std::vector<StampTerrainData> StampTerrainInstances;

		ProceduralTerrainTopology topology = PTS_Sphere;
		union
		{
			SphereGeometry sphere{};
			TorusGeometry torus;
		};

		int quadtreeMaxDepth = 0;
		float elevationMin = -0.15f;
		float elevationMax = 0.15f;
		float noiseElevationMin = 0.0f;
		float noiseElevationMax = 0.f;

		std::vector<AxisAlignedBox> bounds;
		//std::vector<OrientedBox> orientedBounds;

		void setBoundDirty()
		{
			bounds.clear();
			elevationMin = -0.15f;
			elevationMax = 0.15f;
			noiseElevationMin = 0.0f;
			noiseElevationMax = 0.0f;
			//orientedBounds.clear();
		}
	};

	namespace TerrainModify
	{
		struct Modifier;
	}
	struct StaticModifiers
	{
		String filePath;
		std::shared_ptr<std::vector<TerrainModify::Modifier>> instances;
		std::unordered_map<int, std::vector<uint32>> nodeToInstances;
	};

	struct _EchoExport BiomeComposition
	{
		const static int s_maxBiomeCountInPlanet;

		String filePath;
		std::vector<Biome> biomes;
		String genObjectVirLibrary;
		uint32 randomSeed = 0;

		//NOTE:Run-time generate unique biome template list.
		// Use this to build material index map. and texture array.
		std::vector<BiomeTemplate> usedBiomeTemplateList;

		uint8 getBiomeIndex(int biomeCompositionIndex)const;
	};

	struct _EchoExport SphericalTerrainData
	{
		enum Version
		{
			VERSION_0 = 0,//Biome library combined with vegatation.
			VERSION_1, // biome library seperat with vegation.
			VERSION_2, // planet make with geometry\biome composition\ biome distribution.
			VERSION_3, // ocean data as a new type file.
		};
		Version version = VERSION_0;

		String filePath{};

		TerrainGeometry geometry{};

		StaticModifiers modifiers{};

		BiomeDistribution distribution{};

		BiomeComposition composition{};

		SphericalOceanData sphericalOcean{};

		std::vector<String> matIndexMaps{};

		RegionData region{};

		SphericalVoronoiRegionData sphericalVoronoiRegion{};

		SphericalTerrainData() = default;

		void CollectResources(set<String>::type& resSet);
		static void CollectStaticResources(set<String>::type& resSet);

		bool bExistResource = false;

		void freshUsedBiomeTextureList();

		static String sphericalVoronoiPath;

		//云的模板id
		int32_t  m_CloudTemplateId = -1;
	};
    
	class _EchoExport SphericalTerrainResource : public ResourceBase, public SphericalTerrainData {
	public:
		SphericalTerrainResource(Echo::ResourceManagerBase* creator, const Echo::Name& name, bool isManual, Echo::ManualResourceLoader* loader);
		virtual ~SphericalTerrainResource();

		//NOTE(yanghang):New function for more flexible terrain data save and load.
		static bool loadSphericalTerrain(const String& filePath,
			SphericalTerrainData& terrain, bool editorMode = false);

		static bool loadBiome_v1(const String& biomeFilePath,
			SphericalTerrainData& terrainData);

		static bool loadSphericalTerrain_v1(const String& filePath,
			SphericalTerrainData& terrain,
			bool editorMode);

		static bool loadSphericalTerrain_v2(const String& filePath,
			SphericalTerrainData& terrain,
			bool editorMode);

		static bool loadSphericalTerrain_v3(const String& filePath,
			SphericalTerrainData& terrain,
			bool editorMode = false);
		[[deprecated("Use loadSphericalTerrain(..., editorMode = true) instead.")]]
		static bool loadSphericalTerrainFile(const String& filePath, SphericalTerrainData& terrain);
		[[deprecated("Use saveSphericalTerrain(..., editorMode = true) instead.")]]
		static void saveSphericalTerrainFile(const SphericalTerrainData& terrain);
		static void saveSphericalTerrain(SphericalTerrainData& terrain,
			bool forceSaveComposition = false, bool editorMode = false);

		//New version that data in .terrain file.
		static bool load3DNoiseParameters(cJSON* noiseNode, Noise3DData& noise);
		static void save3DNoiseParameters(cJSON* noiseNode, Noise3DData& noise);

		static bool loadTerrainGeometry(TerrainGeometry& geometry);
		static void saveTerrainGeometry(TerrainGeometry& geometry);

		static bool loadModifiers(StaticModifiers& staticModifiers);
		static bool saveModifiers(const StaticModifiers& staticModifiers);
		static cJSON* serializeModifiers(const StaticModifiers& staticModifiers);

		static bool loadBiomeComposition(BiomeComposition& biome);
		static void saveBiomeComposition(const BiomeComposition& biome);

		static bool loadBiomeDistribution(BiomeDistribution& biome);
		static void saveBiomeDistribution(const BiomeDistribution& biome);

		static void loadRegionData(cJSON* pNode, RegionData& data, bool& result);
		static void saveRegionData(cJSON* pNode, const RegionData& data);

		static void loadBiomeComposition(cJSON* pNode, BiomeComposition& data, bool& result);
		static void saveBiomeComposition(cJSON* pNode, const BiomeComposition& data);

		static void saveVisualGroupData(cJSON* pNode);
		static void loadVisualGroupData(cJSON* pNode);

		static bool loadRegion(cJSON* pNode, RegionData& region);
		static void saveRegion(cJSON* pNode, const RegionData& region);
		static bool loadOfflineRegionMap(const std::string& path, std::array<Bitmap, 6>& maps);
		static void saveOfflineRegionMap(const std::string& path, const std::array<Bitmap, 6>& maps);

		static bool loadSphericalVoronoiRegion(cJSON* pNode, SphericalVoronoiRegionData& sphericalVoronoiRegion);
		static void saveSphericalVoronoiRegion(cJSON* pNode, const SphericalVoronoiRegionData& sphericalVoronoiRegion);
		static bool verifySphericalVoronoiRegion(SphericalVoronoiRegionData& region, const String& terrain);

		static bool loadClimateProcess(cJSON* climateProcessNode, ClimateProcessData& climateProcess);
		static void saveClimateProcess(cJSON* climateProcessNode, const ClimateProcessData& climateProcess);

		static bool loadSphericalOcean(SphericalOceanData& sphericalOcean);
		static void saveSphericalOcean(SphericalOceanData& sphericalOcean);

		static bool loadSphericalOcean(cJSON* sphericalOceanNode, SphericalOceanData& sphericalOcean);
		static void saveSphericalOcean(cJSON* sphericalOceanNode, SphericalOceanData& sphericalOcean);

        //NOTE:request once before one planet using biome texture for rendering.
        // and free after the planet didn't need it.
        // check before using biome texture.
        void requestBiomeTexture(SphericalTerrainResourcePtr selfSmartPtr, bool sync = false);
        void freeBiomeTexture();
        bool isBiomeTextureReady();
        
		void Clear();
	protected:
		void prepareImpl(void) override;

		void unprepareImpl(void) override;

		void loadImpl(void) override;

		void unloadImpl(void) override;

		size_t calcMemUsage(void) const override;
	public:
		bool m_bLoadResult = false;
		std::unordered_map<String, HeightMap> mTextures;
		std::array<Bitmap, 6> m_matIndexMaps;

        //NOTE:Load to memory when need. Free when nobody use.
		PlanetTextureResourcePtr m_planetTexRes;

		TexturePtr m_bakedEmission;
		std::vector<ARGB>m_bakedEmissionData;
		TexturePtr m_bakedAlbedo;
		std::vector<ARGB> m_bakedAlbedoData;
		Bitmap m_regionCPUData;
		std::shared_ptr<TerrainGenerator> m_pTerrainGenerator;
		static String TPath;
		static String VPath;
		static std::string selectTVGname;
		static std::string selectVVGname;

        bool m_loadBiomeTexture = false;
        int m_requireBiomeTextureCount = 0;

        TexturePtr lookupTex;

        WorkQueue::RequestID m_requestID = 0;

		//PlanetRoadPtr		mPlanetRoad;
	};



	class _EchoExport SphericalTerrainResourceManager :
        public ResourceManagerBase,
        public Echo::ResourceBackgroundQueue::Listener,
        public Singleton< SphericalTerrainResourceManager>
	{
	public:
		SphericalTerrainResourceManager();
		virtual ~SphericalTerrainResourceManager();

		//NOTE:Call when biome diffuse texture changed.
		static BiomeGPUData fillBiomeDiffuseTextureArray(BiomeComposition& biome);
		static void fillBiomeDiffuseTextureArray_Async(BiomeComposition& biome, BiomeGPUData_Cache& dataCache);
		static void fillBiomeDiffuseTextureArray_Sync(BiomeGPUData_Cache& dataCache, BiomeGPUData& data);

		const BiomeLibrary& getBiomeLibrary() { return m_biomeLib; }
		void  reloadBiomeLibrary() { m_biomeLib.load(); }

		const PlanetCloudLibrary& getPlanetCloudLibrary() { return m_CloudLibrary; }
		void  setPlanetCloudLibrary(const PlanetCloudLibrary& lib);
		void  reloadPlanetCloudLibrary() { m_CloudLibrary.load(); }
	public:
		using CallbackFunc = std::function<void(const SphericalTerrainResourcePtr& resource)>;
		void AsyncLoad(const Name& name, const CallbackFunc& callback);

		GetBiomeResult getBiomeFromLibrary(const String& name);
		GetBiomeResult getBiomeFromLibrary(const int32& biomeID);

		bool getCloudTemplateFromLibrary(uint8 cloudTemId, PlanetCloudTemplateData& data);

        TexturePtr getPointTexture();
	protected:
		void operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;
		Echo::ResourceBase* createImpl(const Echo::Name& name, bool isManual, Echo::ManualResourceLoader* loader, const Echo::NameValuePairList* createParams) override;
	private:
		unordered_map<Name, Echo::BackgroundProcessTicket>::type m_preparingResources;
		std::unordered_map<Echo::BackgroundProcessTicket, std::vector<CallbackFunc>> m_preparingCallbacks;

		BiomeLibrary m_biomeLib;

        TexturePtr m_pointTexture;
		//云
		PlanetCloudLibrary  m_CloudLibrary;
	public:
		static uint64 m_biomeTexMemory;

        static DataStreamPtr defaultLayerTexData;
	};

    class PlanetTextureResource : public ResourceBase
    {
        friend class PlanetTextureResourceManager;
    public:
        ~PlanetTextureResource();


    protected:
        PlanetTextureResource(Echo::ResourceManagerBase* creator,
                              const Echo::Name& name,
                              bool isManual,
                              Echo::ManualResourceLoader* loader,
                              SphericalTerrainResource* planetRes);
        /// @copydoc ResourceBase::prepareImpl
        void prepareImpl(void);
        /// @copydoc ResourceBase::unprepareImpl
        void unprepareImpl(void);
        /// @copydoc ResourceBase::loadImpl
        void loadImpl(void);
        /// @copydoc ResourceBase::unloadImpl
        void unloadImpl(void);

        size_t calcMemUsage(void) const;

    public:
 		BiomeGPUData m_biomeGPUData;
        BiomeGPUData_Cache* pBiomeGPUData_Cache = nullptr;
        //NOTE:Parent planet biome composition.
        SphericalTerrainResourcePtr m_planetRes;
        Bitmap m_matIDMaps[6];

        bool m_matMapValid = false;
    };

    class PlanetTextureResourceManager :
        public ResourceManagerBase,
        public Singleton<PlanetTextureResourceManager>,
        public WorkQueue::RequestHandler,
        public WorkQueue::ResponseHandler,
		public WorkQueue::WorkQueueOutPutLog
    {
    public:
        PlanetTextureResourceManager();
        virtual ~PlanetTextureResourceManager() override;

        WorkQueue::RequestID asyncLoadRes(PlanetTextureResourcePtr res,
                                          SphericalTerrainResourcePtr parentRes);
        
        void abortRequest(WorkQueue::RequestID reqID);
		virtual void OutPutPrepareResourceLog(DataStreamPtr& out) override;
    protected:
        virtual Echo::ResourceBase* createImpl(const Echo::Name& name,
                                       bool isManual,
                                       Echo::ManualResourceLoader* loader,
                                       const Echo::NameValuePairList* createParams) override;

        virtual bool canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)	override
		{ return true; }

		virtual WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;

		virtual bool canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override
		{ return true; }

		virtual void handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;

        WorkQueue *mWorkQueue = 0;
		uint16 mWorkQueueChannel = 0;

        std::unordered_map<WorkQueue::RequestID, PlanetTextureResourcePtr> m_requestLoadResMap;
        
    };

    struct PlanetTextureResourceLoadRequest
    {
        PlanetTextureResourcePtr res;
        SphericalTerrainResourcePtr parentRes;
        friend std::ostream & operator << (std::ostream &o,
                                           const PlanetTextureResourceLoadRequest &r);
    };

    std::ostream & operator << (std::ostream &o,
                                const PlanetTextureResourcePtr &r);
};

#endif // EchoSphericalTerrainResource_h__
