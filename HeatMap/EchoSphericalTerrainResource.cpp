#include "EchoStableHeaders.h"
#include "EchoSphericalTerrainResource.h"

#include "EchoBase64.h"
#include "EchoResourceManagerFactory.h"
#include "EchoTexture.h"
#include "EchoTextureManager.h"
#include "EchoVegetationLayer.h"
#include "EchoBiomeVegVirGroup.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrianQuadTreeNode.h"
#include "EchoTerrainModifyData.h"
#include "EchoMeshMgr.h"
#include "EchoSubMesh.h"
#include "EchoMaterial.h"
#include "EchoMaterialManager.h"
#include "EchoShaderContentManager.h"

#ifdef ECHO_EDITOR
#include <filesystem>
#endif

#define Profile_Time 0

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

#include "EchoTimer.h"
#include "EchoPlanetRoadResource.h"

namespace Echo {

	String SphericalTerrainData::sphericalVoronoiPath = "echo/biome_terrain/spherical_voronoi_map/";

    uint64 SphericalTerrainResourceManager::m_biomeTexMemory = 0;
    DataStreamPtr SphericalTerrainResourceManager::defaultLayerTexData;

    bool BiomeLibrary::m_enableUnlimitBiomeCount = false;
    int BiomeLibrary::m_unlimitBiomeCount = 0;
	
    const int BiomeLibrary::s_maxGlobalBiomeCount = 4096;
    const int BiomeLibrary::s_maxTextureLimit = 128;
    
    const int BiomeComposition::s_maxBiomeCountInPlanet = SphericalTerrain::MaxMaterialTypes;

	const uint32 BiomeTemplate::s_invalidBiomeID = 0xffffffff;

	String SphericalTerrainResource::TPath = "biome_terrain/default.biomeVirLibrary";
	String SphericalTerrainResource::VPath = "biome_terrain/defaultBiomeVegVirGroup.bvvg";
	std::string SphericalTerrainResource::selectTVGname = "";
	std::string SphericalTerrainResource::selectVVGname = "";
#ifdef ECHO_EDITOR
	static void GetAllFormatFiles(const std::string& path, std::vector<std::string>& files, std::string&& format)
	{
		if (!std::filesystem::exists(path))
		{
			std::filesystem::create_directories(path);
		}
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory()) {
				GetAllFormatFiles(entry.path().string(), files, std::move(format));
			}
			else if (entry.is_regular_file()) {
				if (entry.path().extension() == format)
				{
					files.push_back(entry.path().string());
				}
			}
		}
	}
#endif
	static String
	addSurfix(const String& filePath,
			  const String& surfix)
	{
		String result = filePath;
		if(!result.empty() &&
		   result.find(surfix) == String::npos)
		{
			result += surfix;
		}
		return (result);
	}

	static void logToConsole(const String& log)
	{
#ifdef _WIN32
		OutputDebugString(log.c_str());
#else
		LogManager::instance()->logMessage(log.c_str());
#endif
	}

    TexturePtr& GetDummyR8Ui(const bool create)
	{
		static TexturePtr dummy {};
		if (create && dummy.isNull())
		{
			constexpr uint8_t pix[] = { 255u };		// TIGHT TIGHT TIGHT blue, yellow, pink
			static_assert(std::size(pix) == 1);
			dummy = TextureManager::instance()->createManual(TEX_TYPE_2D, 1, 1, ECHO_FMT_R8UI, &pix, sizeof(pix));
		}
		return dummy;
	}
    
	static void saveJSONFile(const String& path, cJSON* json, const bool unformatted = false)
	{
		EchoZoneScoped;

		if(json)
		{
			char* json_string = unformatted ? cJSON_PrintUnformatted(json) : cJSON_Print(json);

			DataStream* data_stream = 0;
			data_stream = Root::instance()->CreateDataStream(path.c_str());
			if(data_stream)
			{ 
				assert(data_stream);
				FileDataStream* file_stream = (FileDataStream *)data_stream;
				file_stream->write(json_string, strlen(json_string));
				SAFE_DELETE(json_string);
				SAFE_DELETE(file_stream);
			}
		}
	}

    //IMPORTANT(yanghang):It's your duty to free the return "cJSON*"
	static cJSON* readJSONFile(const String& path)
	{
		EchoZoneScoped;

		cJSON* result = 0;
		DataStream* cfgFile = Root::instance()->GetFileDataStream(path.c_str());
		if(cfgFile)
		{
			if(cfgFile->size() > 0)
			{
				cJSON* cfgJsNode = cJSON_Parse(cfgFile->getData());
				if(cfgJsNode)
				{
					result = cfgJsNode;
				}
				else
				{
					LogManager::instance()->logMessage("-error-\t "+ path + " file JSON parse fail!", LML_CRITICAL);
				}
			}
			else
			{
				LogManager::instance()->logMessage("-error-\t "+ path +" file is empty!", LML_CRITICAL);
			}
			delete cfgFile;
		}
		else
		{
			LogManager::instance()->logMessage("-error-\t " + path +" file didn't find!", LML_CRITICAL);
		}

		return (result);
	}

	struct cJSONDelete
	{
		void operator()(cJSON* node) const
		{
			if (!node) return;
			cJSON_Delete(node);
		}
	};

    static int calculateMipCount(int minDimension)
    {
        int mip = 0;
        while(minDimension > 0)
        {
            minDimension = minDimension >> 1;
            mip++;
        }
        return (mip);
    }
    
	bool VoronoiRegionPack::loadAdjacents(const std::string& path)
	{
		if (!Params.isParsed && (path.empty() || !PlanetRegionRelated::Parse(path, Params)))
		{
			LogManager::instance()->logMessage("-error-\t Planet Region prefix [" + path + "] isn't successfully parsed !", LML_CRITICAL);
			return false;
		}

		const String& p = path + ".adjacent";
		if (cJSON* pNode = readJSONFile(p.c_str()))
		{
			if (pNode->type == cJSON_Object)
			{
				int typeCount = cJSON_GetArraySize(pNode);
				bool OK = (typeCount == Params.Total());
				if (OK)
				{
					std::vector<std::vector<int>> fine_adjacent_regions;
					fine_adjacent_regions.resize(typeCount);
					for (int type_index = 0; type_index < typeCount; ++type_index)
					{
						auto typeNode = cJSON_GetArrayItem(pNode, type_index);
						if (std::to_string(type_index) != String(typeNode->string)
							|| type_index > 255 || typeNode->type != cJSON_Array)
						{
							OK = false;
							break;
						}
						auto& vec = fine_adjacent_regions[type_index];
						int count = cJSON_GetArraySize(typeNode);
						for (int i = 0; i < count; ++i)
						{
							if (auto iNode = cJSON_GetArrayItem(typeNode, i))
							{
								if (iNode->valueint >= 0 && iNode->valueint < Params.Total())
								{
									vec.push_back(iNode->valueint);
								}
								else
								{
									OK = false;
									break;
								}
							}
						}
						if (!OK)
						{
							break;
						}

					}

					if (OK)
					{
						std::swap(
							mAdjacents,
							fine_adjacent_regions
						);
						cJSON_Delete(pNode);
						return true;
					}
				}
			}

			cJSON_Delete(pNode);
		}

		return false;
	}

	bool PlanetRegionGenerator::SaveTo(const String& prefix)
	{
		bool isTorus = params.z == 1;
		bool isSphere = params.z == 6;
		if (!isTorus && !isSphere) return false;
		String path = prefix;
		if (!ToRelativePrefix(params.x, params.y, params.z, params.sx, params.sy, path))
		{
			return false;
		}

		// 1. map
		for (int i = 0; i < (int)params.z; ++i)
		{
			const std::string mapPath = path + "_" + std::to_string(i) + ".map";
			PlanetManager::writeBitmap(maps[i], mapPath.c_str());
		}
#if 1
		if(params.z == 1)
		{
			const auto& map = maps[0];
			std::vector<uint8> color(map.width * map.height * 4, 255);
			for (int i = 0; i < map.width * map.height; ++i)
			{
				auto id = static_cast<uint8*>(map.pixels)[i];

				uint8& r = color[i * 4];
				uint8& g = color[i * 4 + 1];
				uint8& b = color[i * 4 + 2];

				r = (uint8)((id % params.x) * 10);
				g = (uint8)((id % params.x > 12 ? id % params.x : 0) * 10);
				b = (uint8)((id / params.x) * 30);

			}
			for (const auto& center : v2)
			{
				Vector2 coord = Fract(center) * Vector2(static_cast<float>(map.width), static_cast<float>(map.height));
				int x = static_cast<int>(std::floorf(coord.x));
				int y = static_cast<int>(std::floorf(coord.y));
				int id = x + y * map.width;
				if (id < map.size())
				{
					uint8& r = color[id * 4];
					uint8& g = color[id * 4 + 1];
					uint8& b = color[id * 4 + 2];
					r = g = b = 255;
				}
			}
			{
				Image image;
				image.loadFromData((uint8*)color.data(), map.width, map.height, PixelFormat::PF_A8R8G8B8);
				image.save(Root::instance()->getRootResourcePath() + path + "_0_with_center_n.png");
			}
		}
#endif
		// 2. bin
		{
			const std::string binPath = path + ".bin";
			if ((!v2.empty() && isTorus) || (!v3.empty() && isSphere))
			{
				if (auto* data = Root::instance()->CreateDataStream(binPath.c_str()))
				{
					if (isTorus)
					{
						data->write(v2.data(), v2.size() * sizeof(v2[0]));
					}
					if (isSphere)
					{
						data->write(v3.data(), v3.size() * sizeof(v3[0]));
					}
					data->close();
					delete data;
				}
			}
		}
		// 3. adjacent
		{
			const std::string adjPath = path + ".adjacent";
			if (adjacency.size() == params.x * params.y * params.z)
			{
				if(cJSON* Node = cJSON_CreateObject())
				{
					for (int i = 0; i < (int)adjacency.size(); ++i)
					{
						if (adjacency.count(i) <= 0)
						{
							cJSON_Delete(Node);
							return false;
						}
						std::vector<int> adj(adjacency[i].begin(), adjacency[i].end());
						cJSON* iNode = cJSON_CreateIntArray(adj.data(), (int)adj.size());
						cJSON_AddItemToObject(Node, std::to_string(i).c_str(), iNode);
					}
					saveJSONFile(adjPath, Node);
					cJSON_Delete(Node);
				}
			}
		}
		// 4. num
		{
			const std::string numPath = path + ".num";
			if (!num.empty())
			{
				if (auto* data = Root::instance()->CreateDataStream(numPath.c_str()))
				{
					data->write(num.data(), num.size() * sizeof(num[0]));
					data->close();
					delete data;
					return true;
				}
			}
		}
		return false;
	}

	void SphericalTerrainData::CollectStaticResources(set<String>::type& _resSet)
	{
		_resSet.insert("biome_terrain/default.biomelibrary");
		_resSet.insert("echo/biome_terrain/geometryType/TerrainGeometry.library");
		_resSet.insert("biome_terrain/library.cfg");
		_resSet.insert("biome_terrain/default.cloudlibrary");
		_resSet.insert("biome_terrain/PlanetRoadAssembly.roadamb");
		_resSet.insert("biome_terrain/planetvegassembly.veglibrary");
		//_resSet.insert("echo/biome_terrain/Textures/fbm.dds");
		//_resSet.insert("echo/biome_terrain/Textures/default_layer.dds");
	}

	void SphericalTerrainData::CollectResources(set<String>::type& _resSet)
	{
		if (!bExistResource) return;

		_resSet.insert(filePath);
		_resSet.insert(geometry.filePath);
		_resSet.insert(composition.filePath);
			
		for (const auto& bd : composition.biomes)
		{
			_resSet.insert(bd.biomeTemp.diffuse);
			_resSet.insert(bd.biomeTemp.normal);
		}
		
		_resSet.insert(distribution.filePath);
		
		//RegionData region;
		{
			_resSet.insert(region.name);
			for (const auto& rd : region.biomeArray) {
				_resSet.insert(rd.TemplateTOD);
			}
		}
		//SphericalOceanData sphericalOcean;
		{
			_resSet.insert(sphericalOcean.filePath);
			_resSet.insert(sphericalOcean.fileSetting.MeshFile);
			_resSet.insert(sphericalOcean.fileSetting.NormalTextureName);
			_resSet.insert(sphericalOcean.fileSetting.ReflectTextureName);
			_resSet.insert(sphericalOcean.fileSetting.CausticTextureName);
			_resSet.insert(sphericalOcean.fileSetting.TunnelTextureName);
			_resSet.insert(sphericalOcean.fileSetting.FoamTextureName);
		}
		//std::vector<std::pair<StampTerrainData, String>> StampTerrainInstances;
		{
			for (const auto& stIns : geometry.StampTerrainInstances) {
				_resSet.insert(stIns.templateID);
			}
		}

		for (auto& biome : composition.biomes)
		{
			for (auto&& layer : biome.vegLayers)
			{
				const std::string vegLayerResName = BiomeVegLayerTemp::ResToPathName(layer.name);
				std::set<std::string> vegLayerResSet = BiomeVegLayerTemp::GetCollectResources(vegLayerResName);

				_resSet.insert(vegLayerResName);
				_resSet.insert(vegLayerResSet.begin(), vegLayerResSet.end());
			}
		}

		for (auto& biome : composition.biomes)
		{
			PCG::BiomeGenObjectLibraryManager::instance()->CollectResources(biome.genObjects, _resSet);
		}

		for (auto&& map : matIndexMaps)
		{
			_resSet.insert(map);
		}

		if (sphericalVoronoiRegion.Loaded)
		{
			const auto& Params = sphericalVoronoiRegion.mData->mData.Params;
			const auto& prefix = sphericalVoronoiRegion.prefix;
			
			String path = prefix + ".bin";
			_resSet.insert(path);
			path = prefix + ".adjacent";
			_resSet.insert(path);

			for (int i = 0; i < (int)Params.z; ++i)
			{
				path = prefix + "_" + std::to_string(i) + ".map";
				_resSet.insert(path);
			}

			for (const auto& rd : sphericalVoronoiRegion.defineArray) {
				_resSet.insert(rd.TemplateTOD);
			}

		}

		if (const auto& modFile = modifiers.filePath; !modFile.empty())
		{
			_resSet.insert(modFile);
		}

		//NOTE:Collect region SDF map files.
		if(sphericalVoronoiRegion.Loaded &&
		   Root::instance()->m_enablePlanetRegionVisual)
		{
			const auto& Params = sphericalVoronoiRegion.mData->mData.Params;
			String& sphericalVoronoiPath = SphericalTerrainData::sphericalVoronoiPath;
			for(int i = 0;
				i < static_cast<int>(Params.z);
				i++)
			{
				String prefix = sphericalVoronoiRegion.prefix.substr(sphericalVoronoiRegion.prefix.find_last_of('/') + 1);
				String coarse_prefix = sphericalVoronoiRegion.coarse_prefix.substr(sphericalVoronoiRegion.coarse_prefix.find_last_of("/") + 1);
				String coarseSDFFileName;
				//NOTE: Level 1 region doesn't have coarse map. using the global_white map instead.
				if (Params.isParsed && Params.z == 6)
				{
					if (Params.x == 1 && Params.y == 1)
					// if(SphericalVoronoi::Parse(prefix) == 1)
					{
						coarseSDFFileName = sphericalVoronoiPath +
							"coarse_global_white.sdf";
					}
					else
					{
						coarseSDFFileName = sphericalVoronoiPath + "coarse_" + prefix +
							"_" + coarse_prefix + "_" + std::to_string(i) + ".sdf";
					}
				}
				else if (Params.isParsed && Params.z == 1)
				{
					coarseSDFFileName = sphericalVoronoiPath;
					if (Params.x == 3 && Params.y == 1)
					{
						coarseSDFFileName.append("coarse_global_white_torus.sdf");
					}
					else
					{
						char buff[64];
						static_cast<void>(std::sprintf(buff, "coarse_%s_%d.sdf", prefix.c_str(), i));
						coarseSDFFileName.append(buff);
					}
				}

				String fineSDFFileName = sphericalVoronoiPath + "fine" +
					prefix +"_" + std::to_string(i) + ".sdf";

				_resSet.insert(fineSDFFileName);
				_resSet.insert(coarseSDFFileName);
			}
		}

		if (geometry.topology == PTS_Sphere && geometry.sphere.gempType != -1) {
			const auto& terGenNames = TerrainGeneratorLibrary::instance()->getNames();
			auto iter = terGenNames.find(geometry.sphere.gempType);
			if (iter != terGenNames.end()) {
				_resSet.insert(iter->second);
			}
		}


		//收集远景云贴图
		if (m_CloudTemplateId != -1)
		{
			PlanetCloudTemplateData data;
			const PlanetCloudLibrary& cloudLib = SphericalTerrainResourceManager::instance()->getPlanetCloudLibrary();
			cloudLib.getCloudTemplateData(static_cast<int>(m_CloudTemplateId), data);
			if(!data.m_DiffusePath.empty())
			   _resSet.insert(data.m_DiffusePath);
		}
	}
		
	void
	SphericalTerrainResource::saveBiomeComposition(const BiomeComposition& compo)
	{
		if (compo.filePath.empty())
		{
			return;
		}

		cJSON* rootNode = cJSON_CreateObject();

		saveVisualGroupData(rootNode);
		
		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(rootNode, "compositions", typeArrayNode);
		if (!compo.biomes.empty())
		{
			cJSON* typeNode;
 
			int typeCount = (int)compo.biomes.size();
			for (int type_index = 0;
				 type_index < typeCount;
				 type_index++)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				const Biome& temp = compo.biomes[type_index];
				
				WriteJSNode(typeNode, "biome id", temp.biomeTemp.biomeID);

				cJSON* vegLayersArray = cJSON_CreateArray();
				cJSON_AddItemToObject(typeNode, "VegLayers", vegLayersArray);
				for (auto& vegLayer : temp.vegLayers)
				{
					cJSON* vegLayerItem = cJSON_CreateObject();
					cJSON_AddItemToArray(vegLayersArray, vegLayerItem);
					WriteJSNode(vegLayerItem, "name", vegLayer.name);
					WriteJSNode(vegLayerItem, "customParam", vegLayer.customParam);
					WriteJSNode(vegLayerItem, "scaleBase", vegLayer.scaleBase);
					WriteJSNode(vegLayerItem, "scaleRange", vegLayer.scaleRange);
					WriteJSNode(vegLayerItem, "density", vegLayer.density);

					WriteJSNode(vegLayerItem, "topDiffuse", vegLayer.topDiffuse);
					WriteJSNode(vegLayerItem, "topDiffuseRatio", vegLayer.topDiffuseRatio);
					WriteJSNode(vegLayerItem, "bottomDiffuse", vegLayer.bottomDiffuse);
					WriteJSNode(vegLayerItem, "bottomDiffuseRatio", vegLayer.bottomDiffuseRatio);
					WriteJSNode(vegLayerItem, "lerpFactor", vegLayer.lerpFactor);
				}

				WriteJSNode(typeNode, "genObjects", temp.genObjects);
			}
		}

		WriteJSNode(rootNode, "genObjectVirLibrary", compo.genObjectVirLibrary);
		WriteJSNode(rootNode, "randomSeed", compo.randomSeed);

		saveJSONFile(compo.filePath, rootNode);
		cJSON_Delete(rootNode);
	}
	
	void SphericalTerrainResource::saveVisualGroupData(cJSON* pNode)
	{
		cJSON* virLibrary_ = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "TerrainGroup", virLibrary_);
		WriteJSNode(virLibrary_, "path", TPath);
		WriteJSNode(virLibrary_, "selectName", selectTVGname);

		cJSON* vegLibrary_ = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "VegGroup", vegLibrary_);
		WriteJSNode(vegLibrary_, "path", VPath);
		WriteJSNode(vegLibrary_, "selectName", selectVVGname);
	}

	void SphericalTerrainResource::loadVisualGroupData(cJSON* pNode)
	{
		cJSON* TerrainGroupNode = cJSON_GetObjectItem(pNode, "TerrainGroup");
		if (TerrainGroupNode)
		{
			ReadJSNode(TerrainGroupNode, "path", TPath);
			ReadJSNode(TerrainGroupNode, "selectName", selectTVGname);
		}

		cJSON* VegGroupNode = cJSON_GetObjectItem(pNode, "VegGroup");
		if (VegGroupNode)
		{
			ReadJSNode(VegGroupNode, "path", VPath);
			ReadJSNode(VegGroupNode, "selectName", selectVVGname);
		}
	}

	void
	SphericalTerrainResource::saveBiomeDistribution(const BiomeDistribution& dist)
	{
		if (dist.filePath.empty())
		{
			return;
		}

		cJSON* rootNode = cJSON_CreateObject();

		cJSON* climateProcessNode = cJSON_CreateObject();
		cJSON_AddItemToObject(rootNode, "image process", climateProcessNode);
		saveClimateProcess(climateProcessNode, dist.climateProcess);
		
		//TODO(yanghang):Remove edit time data at release.
		cJSON* humidityRangeNode = cJSON_CreateObject();
		cJSON_AddItemToObject(rootNode, "humidity range", humidityRangeNode);
		WriteJSNode(humidityRangeNode, "min", dist.humidityRange.min);
		WriteJSNode(humidityRangeNode, "max", dist.humidityRange.max);

		cJSON* tempRangeNode = cJSON_CreateObject();
		cJSON_AddItemToObject(rootNode, "temperature range", tempRangeNode);
		WriteJSNode(tempRangeNode, "min", dist.temperatureRange.min);
		WriteJSNode(tempRangeNode, "max", dist.temperatureRange.max);

		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(rootNode, "biome distributions", typeArrayNode);
		if (!dist.distRanges.empty())
		{
			cJSON* typeNode;

			int typeCount = (int)dist.distRanges.size();
			for (int type_index = 0;
				 type_index < typeCount;
				 type_index++)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				const auto& temp = dist.distRanges[type_index];

				WriteJSNode(typeNode, "name", temp.name);

				cJSON* humidityNode = cJSON_CreateObject();
				cJSON_AddItemToObject(typeNode, "humidity", humidityNode);
				WriteJSNode(humidityNode, "min", temp.humidity.min);
				WriteJSNode(humidityNode, "max", temp.humidity.max);

				cJSON* tempNode = cJSON_CreateObject();
				cJSON_AddItemToObject(typeNode, "temperature", tempNode);
				WriteJSNode(tempNode, "min", temp.temperature.min);
				WriteJSNode(tempNode, "max", temp.temperature.max);
			}
		}

		cJSON* sphericalVoronoiNode = cJSON_CreateObject();
		cJSON_AddItemToObject(rootNode, "spherical voronoi region", sphericalVoronoiNode);
		cJSON* voronoiSeed = cJSON_CreateArray();
		cJSON_AddItemToArray(voronoiSeed, cJSON_CreateNumber(dist.voronoiSeed.x));
		cJSON_AddItemToArray(voronoiSeed, cJSON_CreateNumber(dist.voronoiSeed.y));
		cJSON_AddItemToObject(sphericalVoronoiNode, "voronoi seed", voronoiSeed);
		WriteJSNode(sphericalVoronoiNode, "voronoi sqrt quads", dist.voronoiSqrtQuads);
		
		saveJSONFile(dist.filePath, rootNode);
		cJSON_Delete(rootNode);
	}

	bool
	SphericalTerrainResource::loadTerrainGeometry(TerrainGeometry& geometry)
	{
		bool result = true;
		cJSON* rootNode = readJSONFile(geometry.filePath.c_str());
		if(rootNode)
		{
			if(cJSON* noiseNode = cJSON_GetObjectItem(rootNode, "3d noise param"))
			{
				result &= load3DNoiseParameters(noiseNode, geometry.noise);
			}

			int topology = PTS_Sphere;
			ReadJSNode(rootNode, "topology", topology);
			geometry.topology = static_cast<ProceduralTerrainTopology>(topology);

			switch (geometry.topology)
			{
			case PTS_Sphere:
			{
				auto& sphere = geometry.sphere;
				if (ReadJSNode(rootNode, "radius", sphere.radius))
				{
					result &= true;
				}
				else
				{
					result &= false;
					sphere.radius = 2000.0f;
				}

				if (ReadJSNode(rootNode, "elevationRatio", sphere.elevationRatio))
				{
					result &= true;
				}
				else
				{
					result &= false;
					sphere.elevationRatio = 0.05f;
				}
				if (ReadJSNode(rootNode, "gempType", sphere.gempType))
				{
					result &= true;
				}
				else
				{
					result &= false;
					sphere.gempType = -1;
				}
			}
				break;
			case PTS_Torus:
			{
				auto& torus = geometry.torus;
				if (ReadJSNode(rootNode, "radius", torus.radius))
				{
					result &= true;
				}
				else
				{
					result &= false;
					torus.radius = 2000.0f;
				}

				if (ReadJSNode(rootNode, "relativeMinorRadius", torus.relativeMinorRadius))
				{
					result &= true;
				}
				else
				{
					result &= false;
					torus.relativeMinorRadius = 0.1f;
				}

				if (ReadJSNode(rootNode, "maxSegmentsX", torus.maxSegments[0]))
				{
					result &= true;
				}
				else
				{
					result                    = false;
					torus.maxSegments[0] = 16;
				}

				if (ReadJSNode(rootNode, "maxSegmentsY", torus.maxSegments[1]))
				{
					result &= true;
				}
				else
				{
					result                 = false;
					torus.maxSegments[1] = 1;
				}

				if (ReadJSNode(rootNode, "elevationRatio", torus.elevationRatio))
				{
					result &= true;
				}
				else
				{
					result &= false;
					torus.elevationRatio = 0.05f;
				}
			}
				break;
			}

			if (ReadJSNode(rootNode, "quadtreeMaxDepth", geometry.quadtreeMaxDepth))
			{
				result &= true;
			}
			else
			{
				result = false;
				geometry.quadtreeMaxDepth = 0;
			}

			if (ReadJSNode(rootNode, "elevationMin", geometry.elevationMin))
			{
				result &= true;
			}
			else
			{
				result &= false;
				geometry.elevationMin = -1.0f;
			}

			if (ReadJSNode(rootNode, "elevationMax", geometry.elevationMax))
			{
				result &= true;
			}
			else
			{
				result &= false;
				geometry.elevationMax = std::numeric_limits<float>::max();
			}

			if (ReadJSNode(rootNode, "noiseElevationMin", geometry.noiseElevationMin))
			{
				result &= true;
			}
			else
			{
				result &= false;
				geometry.noiseElevationMin = 0.0f;
			}

			if (ReadJSNode(rootNode, "noiseElevationMax", geometry.noiseElevationMax))
			{
				result &= true;
			}
			else
			{
				result &= false;
				geometry.noiseElevationMax = 0.0f;
			}

			if (int boundCnt; ReadJSNode(rootNode, "boundCnt", boundCnt) && boundCnt > 0)
			{
				const cJSON* aabbs = cJSON_GetObjectItem(rootNode, "bounds");
				std::vector<AxisAlignedBox> vAabb;
				if (aabbs != nullptr)
				{
					result &= true;
					vAabb.resize(boundCnt, AxisAlignedBox(AxisAlignedBox::EXTENT_FINITE));
					std::vector<float> minMax(boundCnt * 6);
					Base64::Decode(aabbs->valuestring, minMax.data());
					for (int i = 0; i < boundCnt; ++i)
					{
						std::memcpy(&vAabb[i], minMax.data() + i * 6, 6 * sizeof(float));
					}
				}
				geometry.bounds = vAabb;
			}
			result &= HeightMapStamp3D::Load(rootNode, geometry.StampTerrainInstances);

			cJSON_Delete(rootNode);
		}
		return(result);
	}

	void
	SphericalTerrainResource::saveTerrainGeometry(TerrainGeometry& geometry)
	{
		if (geometry.filePath.empty())
		{
			return;
		}
		cJSON* rootNode = cJSON_CreateObject();

		cJSON* noiseNode = cJSON_CreateObject();
		cJSON_AddItemToObject(rootNode, "3d noise param", noiseNode);
		save3DNoiseParameters(noiseNode, geometry.noise);

		WriteJSNode(rootNode, "topology", static_cast<int>(geometry.topology));
		switch (geometry.topology)
		{
		case PTS_Sphere:
		{
			auto& sphere = geometry.sphere;
			WriteJSNode(rootNode, "radius", sphere.radius);
			WriteJSNode(rootNode, "elevationRatio", sphere.elevationRatio);
			WriteJSNode(rootNode, "gempType", sphere.gempType);
		}
		break;
		case PTS_Torus:
		{
			auto& torus = geometry.torus;
			WriteJSNode(rootNode, "radius", torus.radius);
			WriteJSNode(rootNode, "relativeMinorRadius", torus.relativeMinorRadius);
			WriteJSNode(rootNode, "maxSegmentsX", torus.maxSegments[0]);
			WriteJSNode(rootNode, "maxSegmentsY", torus.maxSegments[1]);
			WriteJSNode(rootNode, "elevationRatio", torus.elevationRatio);
		}
		break;
		}
		
		HeightMapStamp3D::Save(rootNode, geometry.StampTerrainInstances);
		
		WriteJSNode(rootNode, "quadtreeMaxDepth", geometry.quadtreeMaxDepth);
		WriteJSNode(rootNode, "elevationMin", geometry.elevationMin);
		WriteJSNode(rootNode, "elevationMax", geometry.elevationMax);
		WriteJSNode(rootNode, "noiseElevationMin", geometry.noiseElevationMin);
		WriteJSNode(rootNode, "noiseElevationMax", geometry.noiseElevationMax);

		{
			WriteJSNode(rootNode, "boundCnt", static_cast<int>(geometry.bounds.size()));
			std::vector<float> minMax;
			constexpr auto szInFlt = (sizeof(AxisAlignedBox::vMin) + sizeof(AxisAlignedBox::vMax)) / sizeof(float);
			minMax.resize(geometry.bounds.size() * szInFlt);
			int offset = 0;
			static_assert(offsetof(AxisAlignedBox, vMin) == 0 && sizeof(AxisAlignedBox::vMin) == 12 &&
						  offsetof(AxisAlignedBox, vMax) == 12 && sizeof(AxisAlignedBox::vMax) == 12, "AxisAlignedBox layout error");
			for (auto&& aabb : geometry.bounds)
			{
				std::memcpy(minMax.data() + offset, &aabb, szInFlt * sizeof(float));
				offset += szInFlt;
			}
			const size_t dataLen = minMax.size() * sizeof(float);
			const size_t szBase64 = Base64::EncodeLength(dataLen);
			std::unique_ptr<char []> encoded(new char[szBase64 + 1]);
			Base64::Encode(minMax.data(), dataLen, encoded.get());
			encoded[szBase64] = '\0';
			cJSON* aabbs = cJSON_CreateString(encoded.get());
			encoded.reset();
			cJSON_AddItemToObject(rootNode, "bounds", aabbs);
		}

		saveJSONFile(geometry.filePath, rootNode);
		cJSON_Delete(rootNode);
	}

	bool SphericalTerrainResource::loadModifiers(StaticModifiers& staticModifiers)
	{
		bool success = true;
		const std::unique_ptr<cJSON, cJSONDelete> rootNodePtr(readJSONFile(staticModifiers.filePath));
		const auto rootNode = rootNodePtr.get();
		if (!rootNode)
		{
			return false;
		}

		do
		{
			// Load modifier data.
			int instanceCount = 0;
			ReadJSNode(rootNode, "instance count", instanceCount);

			if (instanceCount == 0)
			{
				success = false;
				break;
			}

			constexpr auto elementSize = sizeof(TerrainModify::Modifier);
			const auto instances       = std::make_unique<std::byte[]>(instanceCount * elementSize);

			const cJSON* instancesNode = cJSON_GetObjectItem(rootNode, "instances");
			if (instancesNode == nullptr || instancesNode->type != cJSON_String)
			{
				success = false;
				break;
			}

			auto* data = instances.get();
			const auto dataLen = Base64::Decode(instancesNode->valuestring, data);

			staticModifiers.instances = std::make_shared<std::vector<TerrainModify::Modifier>>();
			auto& modifierInstances   = *staticModifiers.instances;
			modifierInstances.reserve(instanceCount);

			size_t offset = 0;
			for (int instIndex = 0; offset < dataLen; ++instIndex)
			{
				const int8 type = reinterpret_cast<int8&>(data[offset]);
				offset += sizeof(int8);
				assert(type >= 0);
				auto decode = [data, &offset](const int8 tp)
				{
					TerrainModify::Modifier mod;
					switch (static_cast<TerrainModify::ModType>(tp))
					{
					case TerrainModify::ModType::CubicBezier:
					{
						auto curve = *reinterpret_cast<const TerrainModify::CubicBezier*>(data + offset);
						offset += sizeof(TerrainModify::CubicBezier);
						mod.Data = curve;
						break;
					}
					case TerrainModify::ModType::FlatRectangle:
					{
						TerrainModify::FlatRectangle rect(*reinterpret_cast<const TerrainModify::FlatRectangleBase*>(data + offset));
						offset += sizeof(TerrainModify::FlatRectangleBase);
						mod.Data = rect;
						break;
					}
					default: break;
					}
					const Vector3 min = *reinterpret_cast<Vector3*>(data + offset);
					offset += sizeof(Vector3);
					const Vector3 max = *reinterpret_cast<Vector3*>(data + offset);
					offset += sizeof(Vector3);
					mod.Bounds = { min, max };
					return mod;
				};
				modifierInstances.push_back(decode(type));
			}

			if (modifierInstances.size() != instanceCount)
			{
				success = false;
				break;
			}

			// Load node intersection.
			auto* intersectionNode = cJSON_GetObjectItem(rootNode, "intersection");
			if (!intersectionNode || intersectionNode->type != cJSON_Array)
			{
				success = false;
				break;
			}

			std::unordered_map<int, std::set<uint32>> nodeToInstances;
			const auto nodeCount = cJSON_GetArraySize(intersectionNode);
			// [{"1436":[1626,1627]},{"1664":[1626]}]
			for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
			{
				const auto* node = cJSON_GetArrayItem(intersectionNode, nodeIndex);
				if (node->type != cJSON_Object) continue;
				auto* idNode = node->child;
				if (!idNode) continue;
				if (idNode->type != cJSON_Array) continue;
				int id = -1;
				id     = std::stoi(idNode->string);
				if (id < 0) continue;

				const auto instCount = cJSON_GetArraySize(idNode);
				if (instCount == 0) continue;

				auto& instSet = nodeToInstances[id];
				for (int instIndex = 0; instIndex < instCount; ++instIndex)
				{
					const auto* inst = cJSON_GetArrayItem(idNode, instIndex);
					if (inst->type != cJSON_Number) continue;
					const int instId = inst->valueint;
					if (instId < 0 || instId >= instanceCount) continue;
					instSet.emplace(static_cast<uint32>(instId));
				}
			}

			for (const auto& [node, inst] : nodeToInstances)
			{
				auto [ite, insert] = staticModifiers.nodeToInstances.emplace(
					node, std::vector<uint32> { inst.begin(), inst.end() });
				ite->second.shrink_to_fit();
			}
		}
		while (false);

		if (!success)
		{
			staticModifiers.instances.reset();
			staticModifiers.nodeToInstances.clear();
		}

		return success;
	}

	bool SphericalTerrainResource::saveModifiers(const StaticModifiers& staticModifiers)
	{
		if (staticModifiers.filePath.empty())
		{
			LogManager::instance()->logMessage("-ERROR-\tSphericalTerrainResource::saveModifiers: file path is empty.", LML_CRITICAL);
			return false;
		}
		const std::unique_ptr<cJSON, cJSONDelete> rootNodePtr(serializeModifiers(staticModifiers));
		if (!rootNodePtr) return false;
		saveJSONFile(staticModifiers.filePath, rootNodePtr.get(), true);
		return true;
	}

	cJSON* SphericalTerrainResource::serializeModifiers(const StaticModifiers& staticModifiers)
	{
		const auto& [filePath, instances, intersect] = staticModifiers;
		if (instances->empty() || intersect.empty())
		{
			return {};
		}

		auto* rootNode = cJSON_CreateObject();

		{
			// Save Instances
			const int instanceCount = static_cast<int>(instances->size());

			WriteJSNode(rootNode, "instance count", instanceCount);

			constexpr auto elementSize = sizeof(TerrainModify::Modifier) + sizeof(Vector3) * 2;	// min max
			const size_t dataLen = instanceCount * elementSize;
			std::vector<std::byte> instancesData(dataLen);
			size_t offset = 0;
			for (size_t instIndex = 0; instIndex < instanceCount; ++instIndex)
			{
				const auto& [Data, Bounds] = instances->at(instIndex);
				assert(!Data.valueless_by_exception());

				reinterpret_cast<int8&>(instancesData[offset]) = static_cast<int8>(Data.index());
				offset += sizeof(int8); // padding
				auto encoder = [&instancesData, &offset]<typename T>(const T& value)
				{
					if constexpr (std::is_same_v<std::decay_t<T>, TerrainModify::FlatRectangle>)
					{
						reinterpret_cast<TerrainModify::FlatRectangleBase&>(instancesData[offset]) = value;
						offset += sizeof(TerrainModify::FlatRectangleBase);
						return;
					}

					reinterpret_cast<std::decay_t<T>&>(instancesData[offset]) = value;
					offset += sizeof(std::decay_t<T>);
				};
				std::visit(encoder, Data);
				reinterpret_cast<Vector3&>(instancesData[offset]) = Bounds[0];
				offset += sizeof(Vector3);
				reinterpret_cast<Vector3&>(instancesData[offset]) = Bounds[1];
				offset += sizeof(Vector3);
			}

			const size_t szBase64 = Base64::EncodeLength(offset);
			const std::unique_ptr<char[]> encoded(new char[szBase64 + 1]);
			Base64::Encode(instancesData.data(), offset, encoded.get());
			encoded[szBase64] = '\0';

			auto* instancesNode = cJSON_CreateString(encoded.get());
			cJSON_AddItemToObject(rootNode, "instances", instancesNode);
		}

		{
			// Save Intersection
			auto* intersectionNode = cJSON_CreateArray();
			for (const auto& [nodeId, instIds] : intersect)
			{
				if (instIds.empty()) continue;

				auto* node = cJSON_CreateObject();
				auto* instArray = cJSON_CreateArray();
				for (const auto& instId : instIds)
				{
					cJSON_AddItemToArray(instArray, cJSON_CreateNumber(instId));
				}
				cJSON_AddItemToObject(node, std::to_string(nodeId).c_str(), instArray);
				cJSON_AddItemToArray(intersectionNode, node);
			}
			cJSON_AddItemToObject(rootNode, "intersection", intersectionNode);
		}
		return rootNode;
	}

	void
	SphericalTerrainResource::saveRegionData(cJSON* pNode, const RegionData& data)
	{
		cJSON* UniNode = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "uniform", UniNode);
		
#ifdef ECHO_EDITOR
		WriteJSNode(UniNode, "innerRadius", data.uniform.innerRadius);
		WriteJSNode(UniNode, "outerRadius", data.uniform.outerRadius);
		WriteJSNode(UniNode, "threshold", data.uniform.threshold);
		WriteJSNode(UniNode, "lambda", data.uniform.lambda);

		
		cJSON* humidityRangeNode = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "humidity range", humidityRangeNode);
		WriteJSNode(humidityRangeNode, "min", data.humidityRange.min);
		WriteJSNode(humidityRangeNode, "max", data.humidityRange.max);

		cJSON* tempRangeNode = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "temperature range", tempRangeNode);
		WriteJSNode(tempRangeNode, "min", data.temperatureRange.min);
		WriteJSNode(tempRangeNode, "max", data.temperatureRange.max);
#endif
		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(pNode, "type", typeArrayNode);

		if (!data.biomeArray.empty())
		{
			cJSON* typeNode = 0;

			int typeCount = (int)data.biomeArray.size();
			for (int type_index = 0;
				 type_index < typeCount;
				 type_index++)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				const auto& temp = data.biomeArray[type_index];

				WriteJSNode(typeNode, "TemplateTOD", temp.TemplateTOD);
#ifdef ECHO_EDITOR
				WriteJSNode(typeNode, "name", temp.name);

				cJSON* humidityNode = cJSON_CreateObject();
				cJSON_AddItemToObject(typeNode, "humidity", humidityNode);
				WriteJSNode(humidityNode, "min", temp.humidity.min);
				WriteJSNode(humidityNode, "max", temp.humidity.max);

				cJSON* tempNode = cJSON_CreateObject();
				cJSON_AddItemToObject(typeNode, "temperature", tempNode);
				WriteJSNode(tempNode, "min", temp.temperature.min);
				WriteJSNode(tempNode, "max", temp.temperature.max);

				WriteJSNode(tempNode, "color", temp.color);
#endif
				WriteJSNode(typeNode, "ExtraStr", temp.StrEx);
				WriteJSNode(typeNode, "ExtraInt", temp.IntEx);
			}
		}

	}

	void  SphericalTerrainResource::loadRegionData(cJSON* pNode, RegionData& data, bool& result)
	{
		cJSON* typeArrayNode = cJSON_GetObjectItem(pNode, "type");

		if (typeArrayNode && typeArrayNode->type == cJSON_Array)
		{
			int typeCount = cJSON_GetArraySize(typeArrayNode);
			if (typeCount > 0xffff)
			{
				LogManager::instance()->logMessage("-error-\t Region " + data.name + "] 's type count is beyond uint16 range!", LML_CRITICAL);
				typeCount = 0xffff;
			}

			cJSON* typeNode = 0;

			if (!data.biomeArray.empty())
				data.biomeArray.clear();

			RegionDefine temp;
			for (int type_index = 0;
				type_index < typeCount;
				type_index++)
			{
				typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
				if (typeNode)
				{
					result &= ReadJSNode(typeNode, "TemplateTOD", temp.TemplateTOD);

#ifdef ECHO_EDITOR
					result &= ReadJSNode(typeNode, "name", temp.name);

					cJSON* humidityNode = cJSON_GetObjectItem(typeNode, "humidity");
					if (humidityNode)
					{
						result &= ReadJSNode(humidityNode, "min", temp.humidity.min);
						result &= ReadJSNode(humidityNode, "max", temp.humidity.max);
					}
					else
					{
						result = false;
					}

					cJSON* tempNode = cJSON_GetObjectItem(typeNode, "temperature");
					if (tempNode)
					{
						result &= ReadJSNode(tempNode, "min", temp.temperature.min);
						result &= ReadJSNode(tempNode, "max", temp.temperature.max);
					}
					else
					{
						result = false;
					}

					ReadJSNode(typeNode, "color", temp.color);
#endif
					result &= ReadJSNode(typeNode, "ExtraStr", temp.StrEx);
					result &= ReadJSNode(typeNode, "ExtraInt", temp.IntEx);

					data.biomeArray.push_back(temp);
				}
				else
				{
					result = false;
				}
			}
		}
		else
		{
			result = false;
		}

#ifdef ECHO_EDITOR
		data.RegionNum = (int)data.biomeArray.size();
		cJSON* UniNode = cJSON_GetObjectItem(pNode, "uniform");
		if (UniNode)
		{
			result &= ReadJSNode(UniNode, "innerRadius", data.uniform.innerRadius);
			result &= ReadJSNode(UniNode, "outerRadius", data.uniform.outerRadius);
			result &= ReadJSNode(UniNode, "threshold", data.uniform.threshold);
			result &= ReadJSNode(UniNode, "lambda", data.uniform.lambda);
		}
		else
		{
			result = false;
		}

		cJSON* humidityNode = cJSON_GetObjectItem(pNode, "humidity range");
		if (humidityNode)
		{
			result &= ReadJSNode(humidityNode, "min", data.humidityRange.min);
			result &= ReadJSNode(humidityNode, "max", data.humidityRange.max);
		}
		else
		{
			result = false;
		}

		cJSON* tempNode = cJSON_GetObjectItem(pNode, "temperature range");
		if (tempNode)
		{
			result &= ReadJSNode(tempNode, "min", data.temperatureRange.min);
			result &= ReadJSNode(tempNode, "max", data.temperatureRange.max);
		}
		else
		{
			result = false;
		}
#endif
	}

	SphericalTerrainResource::SphericalTerrainResource(Echo::ResourceManagerBase* creator, const Echo::Name& name, bool isManual, Echo::ManualResourceLoader* loader) 
		: ResourceBase(creator, name, isManual, loader) {
	}
	SphericalTerrainResource::~SphericalTerrainResource() {
		Clear();
	}

	void SphericalTerrainResource::Clear() {

        if(m_requestID != 0 &&
           !m_planetTexRes.isNull())
        {
            PlanetTextureResourceManager::instance()->abortRequest(m_requestID);
        }
        
		for (auto&& pair:mTextures) {
			pair.second.freeMemory();
		}
		mTextures.clear();
		for (auto && bitmap : m_matIndexMaps) bitmap.freeMemory();
		for (auto&& bitmap : region.RegionMaps) bitmap.freeMemory();
        
		region.biomeArray.clear();
		region.Loaded = false;
		m_regionCPUData.freeMemory();

		SphericalVoronoiRegionData region;
		sphericalVoronoiRegion = region;
		TerrainGeneratorLibrary::instance()->remove(m_pTerrainGenerator);
	}

	void SphericalTerrainResource::prepareImpl() {
		m_bLoadResult = loadSphericalTerrain(getName().toString(), *this, false);

		if (!bExistResource) return;

		if (!geometry.StampTerrainInstances.empty()) 
		{
			using StampTerrain3D::HeightMapDataManager;
			int size = (int)geometry.StampTerrainInstances.size();
			for (int i = 0; i != size; ++i) {
				auto&& path = geometry.StampTerrainInstances[i].templateID;
				try {
					geometry.StampTerrainInstances[i].mHeightmap = HeightMapDataPtr(HeightMapDataManager::instance()->prepare(Name(path)));
				}
				catch (...)
				{
					geometry.StampTerrainInstances[i].mHeightmap.setNull();
				}
				if (!geometry.StampTerrainInstances[i].mHeightmap.isNull())
				{
					if (!HeightMapDataManager::isResolutionLegal(geometry.StampTerrainInstances[i].mHeightmap))
					{
						LogManager::instance()->logMessage("-error-\t " + path + " file is not of the resolution allowed!", LML_CRITICAL);
					}
				}
			}
		}

		for (int i = 0; i < matIndexMaps.size(); ++i)
		{
			auto& map = m_matIndexMaps[i];
			PlanetManager::readBitmap(map, matIndexMaps[i].c_str());

			int w = 0, h = 0;
			if (geometry.topology == PTS_Sphere)
			{
				w = h = SphericalTerrain::BiomeTextureResolution;
			}
			else if (geometry.topology == PTS_Torus)
			{
				w = SphericalTerrain::BiomeCylinderTextureWidth;
				h = map.byteSize / w;
			}

			if (w * h * sizeof(uint8_t) != map.byteSize)
			{
				LogManager::instance()->logMessage("-error-\t" + getName().toString() + " invalid material index map.", LML_CRITICAL);
				map.freeMemory();
				break;
			}

			map.width         = w;
			map.height        = h;
			map.bytesPerPixel = sizeof(uint8_t);
			map.pitch         = map.width * map.bytesPerPixel;
		}

#if Profile_Time
        unsigned long updateTime;

        Timer* timer = Root::instance()->getTimer();
        updateTime = timer->getMicroseconds();
#endif
        
		const bool sphere = geometry.topology == PTS_Sphere;
		const bool torus = geometry.topology == PTS_Torus;
		static constexpr int mapCount[] = { 6, 1 };
		auto first = m_matIndexMaps.begin();
		auto last  = m_matIndexMaps.begin() + mapCount[geometry.topology];
		if (std::any_of(first, last, [](const Bitmap& bitmap) { return bitmap.pixels == nullptr; }))
		{
			LogManager::instance()->logMessage("-error-\t " + 
				getName().toString() + " baking albedo failed, material map not exists.", LML_CRITICAL);
		}
		else if (sphere)
		{
			Image images[6];
			Image emissionImages[6];

            uint32 bakeTexRes = PlanetManager::getSphereBakeTextureResolution(geometry.sphere.radius);
			for (int i = 0; i < 6; ++i)
			{
				const int w = (int)bakeTexRes;
				const int h = (int)bakeTexRes;
				auto bitmap = PlanetManager::ComputeBakedAlbedo(composition, m_matIndexMaps[i], sphericalOcean, w, h, true);
				auto emissionBitmap = PlanetManager::ComputeBakedEmission(composition, m_matIndexMaps[i], sphericalOcean, w, h, true);
                
				images[i].loadFromData(static_cast<uint8*>(bitmap.pixels), bitmap.width, bitmap.height, PF_A8B8G8R8, true);
				emissionImages[i].loadFromData(static_cast<uint8*>(emissionBitmap.pixels), emissionBitmap.width, emissionBitmap.height, PF_A8B8G8R8, true);
			}
			int mips = calculateMipCount(bakeTexRes);
			m_bakedAlbedoData  = PlanetManager::CreateCubeImage(images, mips);
			m_bakedEmissionData = PlanetManager::CreateCubeImage(emissionImages, mips);
		}
		else if (torus)
		{
			Image image;
			Image emissionImage;

            uint32 bakeTexRes =
                PlanetManager::getTorusBakeTextureResolution(geometry.torus.radius);
			const int w = bakeTexRes;
            float scaleFactor = (float)w / (float)m_matIndexMaps[0].width;
			const int h = (int)(m_matIndexMaps[0].height * scaleFactor);
            
			auto bitmap = PlanetManager::ComputeBakedAlbedo(composition, m_matIndexMaps[0], sphericalOcean, w, h);
			auto emissionBitmap = PlanetManager::ComputeBakedEmission(composition, m_matIndexMaps[0], sphericalOcean, w, h);
            
			image.loadFromData(static_cast<uint8*>(bitmap.pixels), bitmap.width, bitmap.height, PF_A8B8G8R8, true);
			emissionImage.loadFromData(static_cast<uint8*>(emissionBitmap.pixels), emissionBitmap.width, emissionBitmap.height, PF_A8B8G8R8, true);
            
			if (const auto* data  = reinterpret_cast<const ARGB*>(image.getData()))
			{
				//m_bakedAlbedoData = { data, data + static_cast<long long>(w * h) };
                m_bakedAlbedoData = PlanetManager::CreateImageMip(image);
			}
            
			if (const auto* emissionData = reinterpret_cast<const ARGB*>(emissionImage.getData()))
			{
				//m_bakedEmissionData = { emissionData, emissionData + static_cast<long long>(w * h) };
                m_bakedEmissionData = PlanetManager::CreateImageMip(emissionImage);
			}
		}

#if Profile_Time
        updateTime = timer->getMicroseconds() - updateTime;

        String geoStr;
        if(geometry.topology == PTS_Sphere)
        {
            geoStr = String(" [sphere radius:") + std::to_string(geometry.sphere.radius);
        }
        else
        {
            geoStr = String(" [torus radius:") + std::to_string(geometry.torus.radius);
        }
        String log = 
            filePath + geoStr + String(" bake texture time: ") +
            std::to_string(updateTime) +String(" micro second ") +
            std::to_string(updateTime / 1000) +String(" ms\n");
        EchoLogToConsole(log.c_str());
#endif

		if (sphere) {
			m_pTerrainGenerator = TerrainGeneratorLibrary::instance()->add(geometry.sphere.gempType);
		}

		//planetroad
		//{
		//	std::string roadfile = mName.toString();
		//	if (!roadfile.empty() &&
		//		roadfile.find(".terrain") == String::npos)
		//	{
		//		roadfile.append(".planetroad");
		//	}
		//	else if (roadfile.find_last_of(".") != String::npos)
		//	{
		//		roadfile.erase(roadfile.find_last_of("."));
		//		roadfile.append(".planetroad");
		//	}
		//	mPlanetRoad = PlanetRoadResourceManager::instance()->createOrRetrieve(Name(roadfile)).first;
		//	mPlanetRoad->setPlanetRes(Name(addSurfix(mName.toString(), ".terrain")));
		//	try
		//	{
		//		mPlanetRoad->load();
		//	}
		//	catch (...)
		//	{
		//		LogManager::instance()->logMessage("planetroad load faild!");
		//	}
		//}
	}

	void SphericalTerrainResource::unprepareImpl() {
		Clear();
	}

	void SphericalTerrainResource::loadImpl() {

		if (!bExistResource) {
			LogManager::instance()->logMessage("SphericalTerrainResource::[" + filePath + "] could not be found.");
			return;
		}

		const bool sphere = geometry.topology == PTS_Sphere;
		const bool torus = geometry.topology == PTS_Torus;

		if (sphere && !m_bakedAlbedoData.empty())
		{
            uint32 bakeTexRes = PlanetManager::getSphereBakeTextureResolution(geometry.sphere.radius);
			uint32 size = bakeTexRes;
			 int mips = calculateMipCount(bakeTexRes);
			m_bakedAlbedo      = TextureManager::instance()->createManual(TEX_TYPE_CUBE_MAP, size, size, 1, mips, ECHO_FMT_RGBA8888_UNORM,
                                                                          m_bakedAlbedoData.data(),
                                                                          static_cast<uint32>(m_bakedAlbedoData.size() * sizeof(ARGB)),
                                                                          nullptr);
			m_bakedAlbedo->load();
		}
		else if (torus && !m_bakedAlbedoData.empty())
		{
            uint32 bakeTexRes =
                PlanetManager::getTorusBakeTextureResolution(geometry.torus.radius);

            const int w = bakeTexRes;
            float scaleFactor = (float)w / (float)m_matIndexMaps[0].width;
			const int h = (int)(m_matIndexMaps[0].height * scaleFactor);

            
			int mips = calculateMipCount(h);
			m_bakedAlbedo      = TextureManager::instance()->createManual(TEX_TYPE_2D, w, h, 1, mips, ECHO_FMT_RGBA8888_UNORM,
                                                                          m_bakedAlbedoData.data(),
                                                                          static_cast<uint32>(m_bakedAlbedoData.size() * sizeof(ABGR)),
                                                                          nullptr);
			m_bakedAlbedo->load();
		}
		m_bakedAlbedoData.clear();
		m_bakedAlbedoData.shrink_to_fit();

		if (sphere && !m_bakedEmissionData.empty())
		{
            uint32 bakeTexRes = PlanetManager::getSphereBakeTextureResolution(geometry.sphere.radius);
			uint32 size = bakeTexRes;
			int mips = calculateMipCount(bakeTexRes);
			m_bakedEmission = TextureManager::instance()->createManual(TEX_TYPE_CUBE_MAP, size, size, 1, mips, ECHO_FMT_RGBA8888_UNORM,
                                                                       m_bakedEmissionData.data(),
                                                                       static_cast<uint32>(m_bakedEmissionData.size() * sizeof(ARGB)),
                                                                       nullptr);
			m_bakedEmission->load();
		}
		else if (torus && !m_bakedEmissionData.empty())
		{
            uint32 bakeTexRes =
                PlanetManager::getTorusBakeTextureResolution(geometry.torus.radius);

            const int w = bakeTexRes;
            float scaleFactor = (float)w / (float)m_matIndexMaps[0].width;
			const int h = (int)(m_matIndexMaps[0].height * scaleFactor);

            
			int mips = calculateMipCount(h);
			m_bakedEmission = TextureManager::instance()->createManual(TEX_TYPE_2D, w, h, 1, mips, ECHO_FMT_RGBA8888_UNORM,
                                                                       m_bakedEmissionData.data(),
                                                                       static_cast<uint32>(m_bakedEmissionData.size() * sizeof(ABGR)),
                                                                       nullptr);
			m_bakedEmission->load();
		}
		m_bakedEmissionData.clear();
		m_bakedEmissionData.shrink_to_fit();

	}

	void SphericalTerrainResource::unloadImpl()
    {
        if(m_requestID != 0 &&
           !m_planetTexRes.isNull())
        {
            PlanetTextureResourceManager::instance()->abortRequest(m_requestID);
            m_requestID = 0;
        }
	}

	size_t SphericalTerrainResource::calcMemUsage(void) const {
		return 0;
	}

	SphericalTerrainResourceManager::SphericalTerrainResourceManager() {
		mResourceType = "SphericalTerrain";
		ResourceManagerFactory::instance()->registerResourceManager(mResourceType, this);

		static PlanetRegionRelated related;
		m_biomeLib.load();
		new TerrainGeneratorLibrary();
		TerrainGeneratorLibrary::instance()->load();

		new SphericalTerrainAtmosData;

        PlanetManager::initGlobalSphereGeometryResource();

		m_CloudLibrary.load();
	}

	SphericalTerrainResourceManager::~SphericalTerrainResourceManager() {

        PlanetManager::freeGlobalSphereGeometryResource();
        
		ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);
		delete TerrainGeneratorLibrary::instance();
		delete SphericalTerrainAtmosData::instance();
	}

	ResourceBase* SphericalTerrainResourceManager::createImpl(const Echo::Name& name, bool isManual, Echo::ManualResourceLoader* loader, const Echo::NameValuePairList* createParams)
	{
		SphericalTerrainResource* ret = new SphericalTerrainResource(this, name, isManual, loader);
		return ret;
	}

	void SphericalTerrainResourceManager::setPlanetCloudLibrary(const PlanetCloudLibrary& lib)
	{
		m_CloudLibrary = lib;
	}

	void SphericalTerrainResourceManager::AsyncLoad(const Name& name, const CallbackFunc& callback) {
		auto res = getByName(name);

		// 资源已经准备好了
		if (!res.isNull() && res->isPrepared())
		{
			auto meshRes = (SphericalTerrainResourcePtr)res;
			callback(meshRes);
			return;
		}

		auto iter = m_preparingResources.find(name);
		if (iter != m_preparingResources.end())
		{
			auto ticket = iter->second;
			m_preparingCallbacks[ticket].push_back(callback);
			return;
		}

		auto rbq = Echo::ResourceBackgroundQueue::instance();
		auto ticket = rbq->prepare(getResourceType(), name,
			Echo::WorkQueue::MakePriority(0, Echo::WorkQueue::RESOURCE_TYPE_TERRAIN),
			false, NULL, NULL, this);

		m_preparingResources[name] = ticket;
		m_preparingCallbacks[ticket].push_back(callback);
	}

	

	void SphericalTerrainResourceManager::operationCompleted(Echo::BackgroundProcessTicket ticket, const Echo::ResourceBackgroundQueue::ResourceRequest& request, const Echo::ResourceBasePtr& resourcePtr)
	{
		if (m_preparingCallbacks.find(ticket) != m_preparingCallbacks.end())
		{
			
			SphericalTerrainResourcePtr res = SphericalTerrainResourcePtr(resourcePtr);

			for (auto& record : m_preparingCallbacks[ticket])
				record(res);

			m_preparingResources.erase(request.name);
			m_preparingCallbacks.erase(ticket);
		}
	}

	bool
	SphericalTerrainResource::loadSphericalTerrain(const String& filePath,
												   SphericalTerrainData& terrain,
												   bool editorMode)
	{
		bool result = true;

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		terrain.filePath = addSurfix(filePath,
									 ".terrain");

		cJSON* terrainNode = readJSONFile(terrain.filePath.c_str());
		if(terrainNode)
		{
			int version = SphericalTerrainData::VERSION_0;
			ReadJSNode(terrainNode, "version", version);
			terrain.version = (SphericalTerrainData::Version)version;

			//TODO(yanghang):Remove this after translate all virtaul group data
			// to biome composition.
			if (!editorMode)
			{
				loadVisualGroupData(terrainNode);
			}
			
			if(version == SphericalTerrainData::VERSION_1)
			{
				result = loadSphericalTerrain_v1(filePath,
										terrain,
										editorMode);
			}
			else if(version == SphericalTerrainData::VERSION_2)
			{
				result = loadSphericalTerrain_v2(filePath,
										terrain,
										editorMode);
			}
			else if(version == SphericalTerrainData::VERSION_3)
			{
				result = loadSphericalTerrain_v3(filePath,
												 terrain,
												 editorMode);
			}
			else 
			{
				//TODO(yanghang):Log diagnostic
				LogManager::instance()->
					logMessage("-error-\t Unsupport version planet file: " + terrain.filePath + std::to_string(terrain.version),
							   LML_CRITICAL);
				result = false;
			}

			if(!editorMode)
			{
				terrain.freshUsedBiomeTextureList();
			}

            cJSON_Delete(terrainNode);
			terrain.bExistResource = true;
		}
		else
		{
			result = false;
		}
		return (result);
	}
	
	bool
	SphericalTerrainResource::loadBiome_v1(const String& biomeFilePath,
										   SphericalTerrainData& terrainData)
	{
		bool result = true;

		BiomeDistribution& biomeDist = terrainData.distribution;
		BiomeComposition& biomeCompo = terrainData.composition;
		
		cJSON* rootNode = readJSONFile(biomeFilePath.c_str());
		if(rootNode)
		{
			//IMPORTANT(yanghang):Clear the old data.
			biomeCompo.biomes.clear();
			biomeDist.distRanges.clear();
			
			cJSON* humidityNode = cJSON_GetObjectItem(rootNode, "humidity range");
			if (humidityNode)
			{
				result &= ReadJSNode(humidityNode, "min", biomeDist.humidityRange.min);
				result &= ReadJSNode(humidityNode, "max", biomeDist.humidityRange.max);
			}
			else
			{
				result = false;
			}

			cJSON* tempNode = cJSON_GetObjectItem(rootNode, "temperature range");
			if (tempNode)
			{
				result &= ReadJSNode(tempNode, "min", biomeDist.temperatureRange.min);
				result &= ReadJSNode(tempNode, "max", biomeDist.temperatureRange.max);
			}
			else
			{
				result = false;
			}

			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "type");

			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);

				int biomeLimit = BiomeComposition::s_maxBiomeCountInPlanet;

				if(BiomeLibrary::m_enableUnlimitBiomeCount)
				{
					biomeLimit = BiomeLibrary::m_unlimitBiomeCount;
				}
								
				if (typeCount > biomeLimit)
				{
					LogManager::instance()->
						logMessage("-error-\t Biome count in planet is out of max limit: " + std::to_string(biomeLimit) + "!",
								   LML_CRITICAL);

					typeCount = biomeLimit;
				}
				
				cJSON* typeNode = 0;
				Biome tempBiome;
				BiomeDistributeRange tempDistRange;
				
				for (int type_index = 0;
					 type_index < typeCount;
					 type_index++)
				{
					//NOTE:Clear temp data.
					tempDistRange = {};
					tempBiome = {};
				
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						//IMPORTANT:This is biome define name.
						ReadJSNode(typeNode, "name", tempDistRange.name);
					
						//IMPORTANT:Version 0 use the name as unqiue id for biome
						// template. other version using the biomeID.
						ReadJSNode(typeNode, "biome id", tempBiome.biomeTemp.biomeID);

						GetBiomeResult biomeResult =
							SphericalTerrainResourceManager::instance()->getBiomeFromLibrary(tempBiome.biomeTemp.biomeID);
						
						if (biomeResult.success)
						{
							tempBiome.biomeTemp = biomeResult.biome;
						}
						else
						{
							LogManager::instance()->
								logMessage("-error-\t biome file [" + biomeFilePath + "] 's biome [" + tempBiome.biomeTemp.name + "] isn't find in the global biome library!",
										   LML_CRITICAL);
						}

						cJSON* vegLayersArray = cJSON_GetObjectItem(typeNode, "VegLayers");
						if (vegLayersArray)
						{
							int vegLayerCount = cJSON_GetArraySize(vegLayersArray);
							tempBiome.vegLayers.resize(vegLayerCount);
							for (int layerIndex = 0; layerIndex < vegLayerCount; layerIndex++)  
							{
								cJSON* vegLayerItem = cJSON_GetArrayItem(vegLayersArray, layerIndex);
								auto& vegLayer = tempBiome.vegLayers[layerIndex];
								ReadJSNode(vegLayerItem, "name", vegLayer.name);
								ReadJSNode(vegLayerItem, "scaleBase", vegLayer.scaleBase);
								ReadJSNode(vegLayerItem, "scaleRange", vegLayer.scaleRange);
								ReadJSNode(vegLayerItem, "density", vegLayer.density);
							}
						}

						ReadJSNode(typeNode, "genObjects", tempBiome.genObjects);

						biomeCompo.biomes.push_back(tempBiome);
						
						cJSON* humidityNode = cJSON_GetObjectItem(typeNode, "humidity");
						if (humidityNode)
						{
							result &= ReadJSNode(humidityNode, "min", tempDistRange.humidity.min);
							result &= ReadJSNode(humidityNode, "max", tempDistRange.humidity.max);
						}
						else
						{
							result = false;
						}

						cJSON* tempNode = cJSON_GetObjectItem(typeNode, "temperature");
						if (tempNode)
						{
							result &= ReadJSNode(tempNode, "min", tempDistRange.temperature.min);
							result &= ReadJSNode(tempNode, "max", tempDistRange.temperature.max);
						}
						else
						{
							result = false;
						}
						biomeDist.distRanges.push_back(tempDistRange);
					}
					else
					{
						result = false;
					}
				}
			}
			else
			{
				result = false;
			}
			ReadJSNode(rootNode, "genObjectVirLibrary", biomeCompo.genObjectVirLibrary);
			ReadJSNode(rootNode, "randomSeed", biomeCompo.randomSeed);

			cJSON_Delete(rootNode);
		}
		else
		{
			result = false;
		}
		return (result);
	}
	
	bool
	SphericalTerrainResource::loadSphericalTerrain_v1(const String& filePath,
													  SphericalTerrainData& terrain,
													  bool editorMode)
	{
		bool result = true;

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		terrain.filePath = filePath;
		if(!terrain.filePath.empty() &&
		   terrain.filePath.find(".terrain") == String::npos)
		{
			terrain.filePath += ".terrain";            
		}

		cJSON* terrainNode = readJSONFile(terrain.filePath.c_str());
		if(terrainNode)
		{
			//IMPORTANT(yanghang): New version storage all data in .terrain file,
			// except biome data.
			// First try to find data in .terrain, if not exist, try to find old version in the other
			// file.
			if(cJSON* noiseNode = cJSON_GetObjectItem(terrainNode, "3d noise param"))
			{
				result &= load3DNoiseParameters(noiseNode, terrain.geometry.noise);                
			}
			else
			{
				//TODO:Log diagnostic.
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s terrain noise data is missing!",
							   LML_CRITICAL);
			}

			//IMPORTANT(yanghang):Give new two type file file path name.
			String terrainName = terrain.filePath;
			terrainName = terrainName.substr(0, terrainName.find(".terrain"));

			//terrain.distribution.filePath = addSurfix(terrainName, ".biome");
			//terrain.composition.filePath = addSurfix(terrainName, ".biomecompo");
			//terrain.geometry.filePath = addSurfix(terrainName, ".terraingeo");
			
			//IMPORTANT:Load biome file.
			{
				String biomeFilePath;
				if(ReadJSNode(terrainNode, "biome", biomeFilePath))
				{
					result &= loadBiome_v1(biomeFilePath,
										   terrain);
				}
				else
				{
					//TODO:Log diagnostic.
					LogManager::instance()->
						logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome file path is missing!",
								   LML_CRITICAL);
				}
			}

			if (cJSON* regionNode = cJSON_GetObjectItem(terrainNode, "region"))
			{
				result &= loadRegion(regionNode, terrain.region);
			}
			else
			{
				result = false;
			}

			ReadJSNode(terrainNode, "spherical voronoi regionmap prefix", terrain.sphericalVoronoiRegion.prefix);
			if (!Root::instance()->m_useSphericalVoronoiRegion)
			{
				terrain.region.Loaded = loadOfflineRegionMap(filePath, terrain.region.RegionMaps);
				result &= terrain.region.Loaded;
			}
			else
			{
				if (!terrain.sphericalVoronoiRegion.prefix.empty())
				{
#if 0
					auto result = SphericalTerrainResourceManager::instance()->getSphericalVoronoiBitmap(terrain.sphericalVoronoiRegion.prefix, true);
					terrain.sphericalVoronoiRegion.pMaps = result.Pointer;
					terrain.sphericalVoronoiRegion.sqrtQuads = result.sqrtQuads;
					bool match = terrain.sphericalVoronoiRegion.defineArray.size() == 6 * result.sqrtQuads * result.sqrtQuads;
					terrain.sphericalVoronoiRegion.Loaded = (terrain.sphericalVoronoiRegion.pMaps != nullptr) && match;
#endif
					terrain.sphericalVoronoiRegion.mData = PlanetRegionManager::instance()->prepare(Name(terrain.sphericalVoronoiRegion.prefix));
					const auto& mData = terrain.sphericalVoronoiRegion.mData->mData;
					bool match = terrain.sphericalVoronoiRegion.defineArray.size() == mData.Params.x * mData.Params.y * mData.Params.z;
					terrain.sphericalVoronoiRegion.Loaded = mData.mbDataMaps && match;

					if (!match)
					{
						LogManager::instance()->logMessage("-error-\t Planet [" + terrain.filePath + "]'s spherical voronoi regions doesn't match the number of region defines!", LML_CRITICAL);
					}
				}
				else
				{
					LogManager::instance()->logMessage("-error-\t Planet [" + terrain.filePath + "]'s spherical voronoi regionmap prefix is empty!", LML_CRITICAL);
				}
				result &= terrain.sphericalVoronoiRegion.Loaded;
			}
			
			

			if(cJSON* imageProcessNode = cJSON_GetObjectItem(terrainNode, "image process"))
			{
				result &= loadClimateProcess(imageProcessNode, terrain.distribution.climateProcess);
			}
			else
			{
				result = false;
			}
			result &= HeightMapStamp3D::Load(terrainNode, terrain.geometry.StampTerrainInstances);

			terrain.geometry.topology = PTS_Sphere;
			auto& sphere = terrain.geometry.sphere;
			if(ReadJSNode(terrainNode, "radius", sphere.radius))
			{
				result &= true;
			}
			else
			{
				result &= false;
				sphere.radius = 2000.0f;
			}

			if (ReadJSNode(terrainNode, "quadtreeMaxDepth", terrain.geometry.quadtreeMaxDepth))
			{
				result &= true;
			}
			else
			{
				result = false;
				terrain.geometry.quadtreeMaxDepth = 0;
			}
			
			if(ReadJSNode(terrainNode, "elevationRatio", sphere.elevationRatio))
			{
				result &= true;
			}
			else
			{
				result &= false;
				sphere.elevationRatio = 0.05f;
			}

			if (ReadJSNode(terrainNode, "elevationMin", terrain.geometry.elevationMin))
			{
				result &= true;
			}
			else
			{
				result &= false;
				terrain.geometry.elevationMin = -1.0f;
			}

			if (ReadJSNode(terrainNode, "elevationMax", terrain.geometry.elevationMax))
			{
				result &= true;
			}
			else
			{
				result &= false;
				terrain.geometry.elevationMax = std::numeric_limits<float>::max();
			}

			if (ReadJSNode(terrainNode, "noiseElevationMin", terrain.geometry.noiseElevationMin))
			{
				result &= true;
			}
			else
			{
				result &= false;
				terrain.geometry.noiseElevationMin = 0.0f;
			}

			if (ReadJSNode(terrainNode, "noiseElevationMax", terrain.geometry.noiseElevationMax))
			{
				result &= true;
			}
			else
			{
				result &= false;
				terrain.geometry.noiseElevationMax = 0.0f;
			}

			// Load spherical ocean data
			if (cJSON* sphericalOceanNode = cJSON_GetObjectItem(terrainNode, "spherical ocean"))
			{
				result &= loadSphericalOcean(sphericalOceanNode, terrain.sphericalOcean);
				terrain.sphericalOcean.haveOcean = true;
			}
			else
			{
				terrain.sphericalOcean.haveOcean = false;
			}

			result &= ReadJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);

			if (int boundCnt; ReadJSNode(terrainNode, "boundCnt", boundCnt) && boundCnt > 0)
			{
				const cJSON* aabbs = cJSON_GetObjectItem(terrainNode, "bounds");
				std::vector<AxisAlignedBox> vAabb;
				if (aabbs != nullptr)
				{
					result &= true;
					vAabb.resize(boundCnt, AxisAlignedBox(AxisAlignedBox::EXTENT_FINITE));
					std::vector<float> minMax(boundCnt * 6);
					Base64::Decode(aabbs->valuestring, minMax.data());
					for (int i = 0; i < boundCnt; ++i)
					{
						std::memcpy(&vAabb[i], minMax.data() + i * 6, 6 * sizeof(float));
					}
				}
				terrain.geometry.bounds = vAabb;
			}
			// result &= ReadJSNode(terrainNode, "bounds", terrain.bounds);

			/*
			{
				if (int orientedBoundCnt; ReadJSNode(terrainNode, "orientedBoundCnt", orientedBoundCnt) && orientedBoundCnt > 0)
				{
					const cJSON* obbs = cJSON_GetObjectItem(terrainNode, "orientedBounds");
					std::vector<OrientedBox> vObb;
					if (obbs != nullptr)
					{
						result &= true;
						vObb.resize(orientedBoundCnt);
						Base64::Decode(obbs->valuestring, vObb.data());
					}
					terrain.geometry.orientedBounds = vObb;
				}
			}
			*/
			// result &= ReadJSNode(terrainNode, "orientedBounds", terrain.orientedBounds);

			cJSON_Delete(terrainNode);
		}
		else
		{
			result = false;
		}
		return (result);   
	}
	
	bool
	SphericalTerrainResource::loadSphericalTerrain_v2(const String& filePath,
												   SphericalTerrainData& terrain,
												   bool editorMode)
	{
		bool result = true;

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		terrain.filePath = addSurfix(filePath,
								 ".terrain");

		cJSON* terrainNode = readJSONFile(terrain.filePath.c_str());
		if(terrainNode)
		{
			//IMPORTANT(yanghang): New version storage all data in .terrain file,
			// except biome data.
			// First try to find data in .terrain, if not exist, try to find old version in the other
			// file.
			if(ReadJSNode(terrainNode, "terrain geometry", terrain.geometry.filePath))
			{
				result &= loadTerrainGeometry(terrain.geometry);
			}
			else
			{
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s terrain geometry file reference is missing!",
							   LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "biome distribution",
						   terrain.distribution.filePath))
			{
				result &= loadBiomeDistribution(terrain.distribution);
			}
			else
			{
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome distribution file reference is missing!",
							   LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "biome composition",
						   terrain.composition.filePath))
			{
				result &= loadBiomeComposition(terrain.composition);
			}
			else
			{
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome compositoin file reference is missing!",
							   LML_CRITICAL);
			}

			// region related

			if (cJSON* regionNode = cJSON_GetObjectItem(terrainNode, "region"))
			{
				result &= loadRegion(regionNode, terrain.region);
			}
			 
			// spherical region related
			result &= loadSphericalVoronoiRegion(terrainNode, terrain.sphericalVoronoiRegion);

			if (!Root::instance()->m_useSphericalVoronoiRegion)
			{
				terrain.region.Loaded = loadOfflineRegionMap(filePath, terrain.region.RegionMaps);
				result &= terrain.region.Loaded;
			}
			else
			{
				verifySphericalVoronoiRegion(terrain.sphericalVoronoiRegion, terrain.filePath);
				result &= terrain.sphericalVoronoiRegion.Loaded;
			}


			// Load spherical ocean data
			if (cJSON* sphericalOceanNode = cJSON_GetObjectItem(terrainNode, "spherical ocean"))
			{
				result &= loadSphericalOcean(sphericalOceanNode, terrain.sphericalOcean);
				terrain.sphericalOcean.haveOcean = true;
			}

			result &= ReadJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);

			
			cJSON_Delete(terrainNode);
		}
		
		return (result);
	}

	bool
	SphericalTerrainResource::loadSphericalTerrain_v3(const String& filePath,
													  SphericalTerrainData& terrain,
													  bool editorMode)
	{
		bool result = true;

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		terrain.filePath = addSurfix(filePath,
									 ".terrain");

		cJSON* terrainNode = readJSONFile(terrain.filePath.c_str());
		if(terrainNode)
		{
			//IMPORTANT(yanghang): New version storage all data in .terrain file,
			// except biome data.
			// First try to find data in .terrain, if not exist, try to find old version in the other
			// file.
			if(ReadJSNode(terrainNode, "terrain geometry", terrain.geometry.filePath))
			{
				result &= (editorMode? true : loadTerrainGeometry(terrain.geometry));
			}
			else
			{
				if (editorMode) result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s terrain geometry file reference is missing!",
							   LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "static modifier", terrain.modifiers.filePath))
			{
				if (!editorMode) loadModifiers(terrain.modifiers);
			}

			// Load spherical ocean data
			if (ReadJSNode(terrainNode, "ocean", terrain.sphericalOcean.filePath))
			{
				result &= (editorMode ? true : loadSphericalOcean(terrain.sphericalOcean));
				terrain.sphericalOcean.haveOcean = true;
			}
			else
			{
				if (editorMode) result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s ocean file reference is missing!",
							   LML_CRITICAL);
			}
			
			if (ReadJSNode(terrainNode, "biome distribution",
						   terrain.distribution.filePath))
			{
				result &= (editorMode ? true : loadBiomeDistribution(terrain.distribution));
			}
			else
			{
				if (editorMode) result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome distribution file reference is missing!",
							   LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "biome composition",
						   terrain.composition.filePath))
			{
				result &= (editorMode ? true : loadBiomeComposition(terrain.composition));
			}
			else
			{
				if (editorMode) result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome compositoin file reference is missing!",
							   LML_CRITICAL);
			}

			// region related

			if (cJSON* regionNode = cJSON_GetObjectItem(terrainNode, "region"))
			{
				result &= loadRegion(regionNode, terrain.region);
			}
			 
			// spherical region related
			result &= loadSphericalVoronoiRegion(terrainNode, terrain.sphericalVoronoiRegion);

			if(!editorMode)
			{
				if (!Root::instance()->m_useSphericalVoronoiRegion)
				{
					terrain.region.Loaded = loadOfflineRegionMap(filePath, terrain.region.RegionMaps);
					result &= terrain.region.Loaded;
				}
				else
				{
					verifySphericalVoronoiRegion(terrain.sphericalVoronoiRegion, terrain.filePath);
					result &= terrain.sphericalVoronoiRegion.Loaded;
				}
			}

			result &= ReadJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);

			result &= ReadJSNode(terrainNode, "CloudTemplateID", terrain.m_CloudTemplateId);
			
			cJSON_Delete(terrainNode);
		}
		else if(editorMode)
		{
			result = false;
		}
		return (result);
	}

	bool SphericalTerrainResource::loadSphericalTerrainFile(const String& filePath, SphericalTerrainData& terrain)
	{
		bool result = true;

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		terrain.filePath = addSurfix(filePath,
			".terrain");

		cJSON* terrainNode = readJSONFile(terrain.filePath.c_str());
		if (terrainNode)
		{
			int version = SphericalTerrainData::VERSION_0;
			ReadJSNode(terrainNode, "version", version);
			terrain.version = (SphericalTerrainData::Version)version;
			//IMPORTANT(yanghang): New version storage all data in .terrain file,
			// except biome data.
			// First try to find data in .terrain, if not exist, try to find old version in the other
			// file.
			if (ReadJSNode(terrainNode, "terrain geometry", terrain.geometry.filePath))
			{
				result &= true;
			}
			else
			{
				result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s terrain geometry file reference is missing!",
						LML_CRITICAL);
			}

			// Load spherical ocean data
			if (ReadJSNode(terrainNode, "ocean", terrain.sphericalOcean.filePath))
			{
				result &= true;
				terrain.sphericalOcean.haveOcean = true;
			}
			else
			{
				result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s ocean file reference is missing!",
						LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "biome distribution",terrain.distribution.filePath))
			{
				result &= true;
			}
			else
			{
				result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome distribution file reference is missing!",
						LML_CRITICAL);
			}

			if (ReadJSNode(terrainNode, "biome composition", terrain.composition.filePath))
			{
				result &= true;
			}
			else
			{
				result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s biome compositoin file reference is missing!",
						LML_CRITICAL);
			}

			// region related

			if (cJSON* regionNode = cJSON_GetObjectItem(terrainNode, "region"))
			{
				if (loadRegion(regionNode, terrain.region))
				{
					result &= true;
				}
				else
				{
					result &= false;
					LogManager::instance()->
						logMessage("-error-\t Planet [" + terrain.filePath + "]'s legacy region data is missing!",
							LML_CRITICAL);
				}
			}
			

			// spherical region related
			if (loadSphericalVoronoiRegion(terrainNode, terrain.sphericalVoronoiRegion))
			{
				result &= true;
			}
			else
			{
				result &= false;
				LogManager::instance()->
					logMessage("-error-\t Planet [" + terrain.filePath + "]'s voronoi region data is missing!",
						LML_CRITICAL);
			}

#if 0
			if (!Root::instance()->m_useSphericalVoronoiRegion)
			{
				terrain.region.Loaded = loadOfflineRegionMap(filePath, terrain.region.RegionMaps);
				result &= terrain.region.Loaded;
			}
			else
			{
				verifySphericalVoronoiRegion(terrain.sphericalVoronoiRegion, terrain.filePath);
				result &= terrain.sphericalVoronoiRegion.Loaded;
			}

#endif


			result &= ReadJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);
			result &= ReadJSNode(terrainNode, "CloudTemplateID", terrain.m_CloudTemplateId);

			cJSON_Delete(terrainNode);
		}
		else
		{
			LogManager::instance()->
				logMessage("-error-\t Planet [" + terrain.filePath + "]'s terrain file cannot be open!",
					LML_CRITICAL);
			result = false;
		}
		return (result);
	}

	void SphericalTerrainResource::saveSphericalTerrainFile(const SphericalTerrainData& terrain)
	{
		if (terrain.filePath.empty())
		{
			return;
		}

		String terrainName = terrain.filePath;
		terrainName = terrainName.substr(0, terrainName.find(".terrain"));

		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		String terrainFilePath = addSurfix(terrain.filePath,
			".terrain");

		cJSON* terrainNode = cJSON_CreateObject();

		{
			//IMPORTANT:Write version info.
			int version = terrain.version;
			WriteJSNode(terrainNode, "version", version);
		}

		{
			//IMPORTANT:biome distribution is only a reference.
			//if (terrain.geometry.filePath.empty())
			//{
			//	//IMPORTANT:Save the terrain geometry as single new file.
			//	terrain.geometry.filePath = addSurfix(terrainName,
			//		".terraingeo");
			//	saveTerrainGeometry(terrain.geometry);
			//}
			//else
			//{
			//	terrain.geometry.filePath = addSurfix(terrain.geometry.filePath,
			//		".terraingeo");
			//}

			WriteJSNode(terrainNode, "terrain geometry", terrain.geometry.filePath);
		}

		{
			//IMPORTANT:biome distribution is only a reference.
			/*if (terrain.distribution.filePath.empty())
			{
				terrain.distribution.filePath = addSurfix(terrainName,
					".biome");
				saveBiomeDistribution(terrain.distribution);
			}
			else
			{
				terrain.distribution.filePath = addSurfix(terrain.distribution.filePath,
					".biome");
			}*/

			WriteJSNode(terrainNode, "biome distribution", terrain.distribution.filePath);
		}

		{
			//if (terrain.composition.filePath.empty())
			//{
			//	terrain.composition.filePath = addSurfix(terrainName,
			//		".biomecompo");
			//	saveBiomeComposition(terrain.composition);
			//}
			//else
			//{
			//	//IMPORTANT:biome composition is only  a reference.
			//	terrain.composition.filePath = addSurfix(terrain.composition.filePath,
			//		".biomecompo");

			//	//TODO(yanghang):Only for correct biome virtual group storage error.
			//	// remove when this fixed.
			//	if (forceSaveComposition)
			//	{
			//		saveBiomeComposition(terrain.composition);
			//	}
			//}
			WriteJSNode(terrainNode, "biome composition", terrain.composition.filePath);
		}

		if (terrain.sphericalOcean.haveOcean)
		{
			/*if (terrain.sphericalOcean.filePath.empty())
			{
				terrain.sphericalOcean.filePath = addSurfix(terrainName,
					".ocean");
			}
			else
			{
				terrain.sphericalOcean.filePath = addSurfix(terrain.sphericalOcean.filePath,
					".ocean");
			}*/
			WriteJSNode(terrainNode, "ocean", terrain.sphericalOcean.filePath);
			// saveSphericalOcean(terrain.sphericalOcean);
		}

		cJSON* regionNode = cJSON_CreateObject();
		saveRegion(regionNode, terrain.region);
		cJSON_AddItemToObject(terrainNode, "region", regionNode);


		saveSphericalVoronoiRegion(terrainNode, terrain.sphericalVoronoiRegion);

		/*if (!Root::instance()->m_useSphericalVoronoiRegion)
		{
			saveOfflineRegionMap(terrainFilePath, terrain.region.RegionMaps);
		}*/

		WriteJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);

		if (!terrain.modifiers.filePath.empty())
		{
			WriteJSNode(terrainNode, "static modifier", terrain.modifiers.filePath);
		}

		//云模板id
		WriteJSNode(terrainNode, "CloudTemplateID", terrain.m_CloudTemplateId);

		saveJSONFile(terrainFilePath, terrainNode);

		/*std::set<Name> vegLayersName;
		for (auto&& biomeIt : terrain.composition.biomes)
		{
			for (auto&& VegLayerInfo : biomeIt.vegLayers)
				vegLayersName.insert(Name(VegLayerInfo.name));
		}*/

		//const std::string vegLayerPath = biomeFilePath.substr(0, biomeFilePath.find(".biome")) + ".veglayer";
		//VegetationLayerManager::instance()->write(vegLayerPath.c_str(), vegLayersName);

		cJSON_Delete(terrainNode);
	}
	
	void
	SphericalTerrainResource::saveSphericalTerrain(SphericalTerrainData& terrain,
												   bool forceSaveComposition, bool editorMode)
	{
		if(terrain.filePath.empty())
		{
			return;
		}

		String terrainName = terrain.filePath;
		terrainName = terrainName.substr(0, terrainName.find(".terrain"));
		
		//NOTE:File extension post-suffix check.if fix lost, will give it one.
		String terrainFilePath = addSurfix(terrain.filePath,
										   ".terrain");

		cJSON* terrainNode = cJSON_CreateObject();
		
		{
			//IMPORTANT:Write version info.
			int version = SphericalTerrainData::VERSION_3;
			WriteJSNode(terrainNode, "version", version);
		}
		
		{
			//IMPORTANT:biome distribution is only a reference.
			if(!editorMode)
			{
				if (terrain.geometry.filePath.empty())
				{
					//IMPORTANT:Save the terrain geometry as single new file.
					terrain.geometry.filePath = addSurfix(terrainName,
						".terraingeo");
					saveTerrainGeometry(terrain.geometry);
				}
				else
				{
					terrain.geometry.filePath = addSurfix(terrain.geometry.filePath,
						".terraingeo");
				}
			}

			WriteJSNode(terrainNode,"terrain geometry", terrain.geometry.filePath);
		}
		
		{
			//IMPORTANT:biome distribution is only a reference.
			if (!editorMode)
			{
				if (terrain.distribution.filePath.empty())
				{
					terrain.distribution.filePath = addSurfix(terrainName,
						".biome");
					saveBiomeDistribution(terrain.distribution);
				}
				else
				{
					terrain.distribution.filePath = addSurfix(terrain.distribution.filePath,
						".biome");
				}
			}

			WriteJSNode(terrainNode,"biome distribution", terrain.distribution.filePath);
		}

		{
			if (!editorMode)
			{
				if (terrain.composition.filePath.empty())
				{
					terrain.composition.filePath = addSurfix(terrainName,
						".biomecompo");
					saveBiomeComposition(terrain.composition);
				}
				else
				{
					//IMPORTANT:biome composition is only  a reference.
					terrain.composition.filePath = addSurfix(terrain.composition.filePath,
						".biomecompo");

					//TODO(yanghang):Only for correct biome virtual group storage error.
					// remove when this fixed.
					if (forceSaveComposition)
					{
						saveBiomeComposition(terrain.composition);
					}
				}
			}
			WriteJSNode(terrainNode,"biome composition", terrain.composition.filePath);
		}

		if (terrain.sphericalOcean.haveOcean)
		{
			if (!editorMode)
			{
				if (terrain.sphericalOcean.filePath.empty())
				{
					terrain.sphericalOcean.filePath = addSurfix(terrainName,
						".ocean");
				}
				else
				{
					terrain.sphericalOcean.filePath = addSurfix(terrain.sphericalOcean.filePath,
						".ocean");
				}
			}
			WriteJSNode(terrainNode,"ocean", terrain.sphericalOcean.filePath);
			if (!editorMode)
			{
				saveSphericalOcean(terrain.sphericalOcean);
			}
		}

		cJSON* regionNode = cJSON_CreateObject();
		saveRegion(regionNode, terrain.region);
		cJSON_AddItemToObject(terrainNode, "region", regionNode);

		
		saveSphericalVoronoiRegion(terrainNode, terrain.sphericalVoronoiRegion);

		if (!editorMode)
		{
			if (!Root::instance()->m_useSphericalVoronoiRegion)
			{
				saveOfflineRegionMap(terrainFilePath, terrain.region.RegionMaps);
			}
		}

		WriteJSNode(terrainNode, "matIndexMaps", terrain.matIndexMaps);

		if (!terrain.modifiers.filePath.empty())
		{
			WriteJSNode(terrainNode, "static modifier", terrain.modifiers.filePath);
		}

		//云模板id
		WriteJSNode(terrainNode, "CloudTemplateID", terrain.m_CloudTemplateId);

		saveJSONFile(terrainFilePath, terrainNode);

		if (!editorMode)
		{
			std::set<Name> vegLayersName;
			for (auto&& biomeIt : terrain.composition.biomes)
			{
				for (auto&& VegLayerInfo : biomeIt.vegLayers)
					vegLayersName.insert(Name(VegLayerInfo.name));
			}

		}
		//const std::string vegLayerPath = biomeFilePath.substr(0, biomeFilePath.find(".biome")) + ".veglayer";
		//VegetationLayerManager::instance()->write(vegLayerPath.c_str(), vegLayersName);

		cJSON_Delete(terrainNode);
	}

	bool
	SphericalTerrainResource::load3DNoiseParameters(cJSON* noiseNode,
													Noise3DData& noise)
	{
		bool result = true;

		if (cJSON* noiseConfigJsNode = noiseNode)
		{
			result &= ReadJSNode(noiseConfigJsNode, "featureNoiseSeed", noise.featureNoiseSeed);
			result &= ReadJSNode(noiseConfigJsNode, "lacunarity", noise.lacunarity);
			result &= ReadJSNode(noiseConfigJsNode, "baseFrequency", noise.baseFrequency);
			result &= ReadJSNode(noiseConfigJsNode, "baseAmplitude", noise.baseAmplitude);
			result &= ReadJSNode(noiseConfigJsNode, "detailAmplitude", noise.detailAmplitude);
			result &= ReadJSNode(noiseConfigJsNode, "gain", noise.gain);
			result &= ReadJSNode(noiseConfigJsNode, "baseOctaves", noise.baseOctaves);
			result &= ReadJSNode(noiseConfigJsNode, "sharpnessNoiseSeed", noise.sharpnessNoiseSeed);
			result &= ReadJSNode(noiseConfigJsNode, "sharpnessRange", noise.sharpnessRange);
			result &= ReadJSNode(noiseConfigJsNode, "sharpnessFrequency", noise.sharpnessFrequency);
			result &= ReadJSNode(noiseConfigJsNode, "slopeErosionNoiseSeed", noise.slopeErosionNoiseSeed);
			result &= ReadJSNode(noiseConfigJsNode, "slopeErosionRange", noise.slopeErosionRange);
			result &= ReadJSNode(noiseConfigJsNode, "slopeErosionFrequency", noise.slopeErosionFrequency);
			result &= ReadJSNode(noiseConfigJsNode, "perturbNoiseSeed", noise.perturbNoiseSeed);
			result &= ReadJSNode(noiseConfigJsNode, "perturbRange", noise.perturbRange);
			result &= ReadJSNode(noiseConfigJsNode, "perturbFrequency", noise.perturbFrequency);
			ReadJSNode(noiseConfigJsNode, "terrainMinBorder", noise.terrainMinBorder);
			ReadJSNode(noiseConfigJsNode, "terrainBorderSize", noise.terrainBorderSize);
			ReadJSNode(noiseConfigJsNode, "terrainBorderHeight", noise.terrainBorderHeight);
		}
		else
		{
			result = false;
		}

		return result;
	}
	
	void
	SphericalTerrainResource::save3DNoiseParameters(cJSON* noiseNode,
													Noise3DData& noise)
	{
		cJSON* noiseConfigJsNode = noiseNode;

		cJSON* featureSeed = cJSON_CreateArray();
		cJSON_AddItemToArray(featureSeed, cJSON_CreateNumber(noise.featureNoiseSeed.x));
		cJSON_AddItemToArray(featureSeed, cJSON_CreateNumber(noise.featureNoiseSeed.y));
		cJSON_AddItemToArray(featureSeed, cJSON_CreateNumber(noise.featureNoiseSeed.z));
		cJSON_AddItemToObject(noiseConfigJsNode, "featureNoiseSeed", featureSeed);

		cJSON_AddNumberToObject(noiseConfigJsNode, "lacunarity", noise.lacunarity);
		cJSON_AddNumberToObject(noiseConfigJsNode, "baseFrequency", noise.baseFrequency);
		cJSON_AddNumberToObject(noiseConfigJsNode, "baseAmplitude", noise.baseAmplitude);
		cJSON_AddNumberToObject(noiseConfigJsNode, "detailAmplitude", noise.detailAmplitude);
		cJSON_AddNumberToObject(noiseConfigJsNode, "gain", noise.gain);
		cJSON_AddNumberToObject(noiseConfigJsNode, "baseOctaves", noise.baseOctaves);

		cJSON* sharpnessSeed = cJSON_CreateArray();
		cJSON_AddItemToArray(sharpnessSeed, cJSON_CreateNumber(noise.sharpnessNoiseSeed.x));
		cJSON_AddItemToArray(sharpnessSeed, cJSON_CreateNumber(noise.sharpnessNoiseSeed.y));
		cJSON_AddItemToArray(sharpnessSeed, cJSON_CreateNumber(noise.sharpnessNoiseSeed.z));
		cJSON_AddItemToObject(noiseConfigJsNode, "sharpnessNoiseSeed", sharpnessSeed);

		cJSON* sharpnessRange = cJSON_CreateArray();
		cJSON_AddItemToArray(sharpnessRange, cJSON_CreateNumber(noise.sharpnessRange.x));
		cJSON_AddItemToArray(sharpnessRange, cJSON_CreateNumber(noise.sharpnessRange.y));
		cJSON_AddItemToObject(noiseConfigJsNode, "sharpnessRange", sharpnessRange);

		cJSON_AddNumberToObject(noiseConfigJsNode, "sharpnessFrequency", noise.sharpnessFrequency);

		cJSON* slopeErosionSeed = cJSON_CreateArray();
		cJSON_AddItemToArray(slopeErosionSeed, cJSON_CreateNumber(noise.slopeErosionNoiseSeed.x));
		cJSON_AddItemToArray(slopeErosionSeed, cJSON_CreateNumber(noise.slopeErosionNoiseSeed.y));
		cJSON_AddItemToArray(slopeErosionSeed, cJSON_CreateNumber(noise.slopeErosionNoiseSeed.z));
		cJSON_AddItemToObject(noiseConfigJsNode, "slopeErosionNoiseSeed", slopeErosionSeed);

		cJSON* slopeErosionRange = cJSON_CreateArray();
		cJSON_AddItemToArray(slopeErosionRange, cJSON_CreateNumber(noise.slopeErosionRange.x));
		cJSON_AddItemToArray(slopeErosionRange, cJSON_CreateNumber(noise.slopeErosionRange.y));
		cJSON_AddItemToObject(noiseConfigJsNode, "slopeErosionRange", slopeErosionRange);

		cJSON_AddNumberToObject(noiseConfigJsNode, "slopeErosionFrequency", noise.slopeErosionFrequency);

		cJSON* perturbSeed = cJSON_CreateArray();
		cJSON_AddItemToArray(perturbSeed, cJSON_CreateNumber(noise.perturbNoiseSeed.x));
		cJSON_AddItemToArray(perturbSeed, cJSON_CreateNumber(noise.perturbNoiseSeed.y));
		cJSON_AddItemToArray(perturbSeed, cJSON_CreateNumber(noise.perturbNoiseSeed.z));
		cJSON_AddItemToObject(noiseConfigJsNode, "perturbNoiseSeed", perturbSeed);

		cJSON* perturbRange = cJSON_CreateArray();
		cJSON_AddItemToArray(perturbRange, cJSON_CreateNumber(noise.perturbRange.x));
		cJSON_AddItemToArray(perturbRange, cJSON_CreateNumber(noise.perturbRange.y));
		cJSON_AddItemToObject(noiseConfigJsNode, "perturbRange", perturbRange);

		cJSON_AddNumberToObject(noiseConfigJsNode, "perturbFrequency", noise.perturbFrequency);

		cJSON_AddNumberToObject(noiseConfigJsNode, "terrainMinBorder", noise.terrainMinBorder);
		cJSON_AddNumberToObject(noiseConfigJsNode, "terrainBorderSize", noise.terrainBorderSize);
		cJSON_AddNumberToObject(noiseConfigJsNode, "terrainBorderHeight", noise.terrainBorderHeight);
	}

	bool
	SphericalTerrainResource::loadBiomeDistribution(BiomeDistribution& biome)
	{
		bool result = true;
		cJSON* rootNode = readJSONFile(biome.filePath.c_str());
		if(rootNode)
		{
			cJSON* humidityNode = cJSON_GetObjectItem(rootNode, "humidity range");
			if (humidityNode)
			{
				result &= ReadJSNode(humidityNode, "min", biome.humidityRange.min);
				result &= ReadJSNode(humidityNode, "max", biome.humidityRange.max);
			}

			cJSON* tempNode = cJSON_GetObjectItem(rootNode, "temperature range");
			if (tempNode)
			{
				result &= ReadJSNode(tempNode, "min", biome.temperatureRange.min);
				result &= ReadJSNode(tempNode, "max", biome.temperatureRange.max);
			}

			if(cJSON* climateProcessNode = cJSON_GetObjectItem(rootNode, "image process"))
			{
				loadClimateProcess(climateProcessNode, biome.climateProcess);
			}
		
			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "biome distributions");

			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);

				int biomeLimit = BiomeComposition::s_maxBiomeCountInPlanet;

				if(BiomeLibrary::m_enableUnlimitBiomeCount)
				{
					biomeLimit = BiomeLibrary::m_unlimitBiomeCount;
				}                
				
				if (typeCount > biomeLimit)
				{
					LogManager::instance()->
						logMessage("-error-\t Biome count in planet is out of max limit: " + std::to_string(biomeLimit) + "!",
								   LML_CRITICAL);
				}
			
				biome.distRanges.clear();

				cJSON* typeNode = 0;
				for (int type_index = 0;
					 type_index < typeCount;
					 type_index++)
				{
					//NOTE:Clear temp data.
					BiomeDistributeRange temp = {};
				
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						cJSON* humidityNode = cJSON_GetObjectItem(typeNode, "humidity");
						if (humidityNode)
						{
							result &= ReadJSNode(humidityNode, "min", temp.humidity.min);
							result &= ReadJSNode(humidityNode, "max", temp.humidity.max);
						}

						cJSON* tempNode = cJSON_GetObjectItem(typeNode, "temperature");
						if (tempNode)
						{
							result &= ReadJSNode(tempNode, "min", temp.temperature.min);
							result &= ReadJSNode(tempNode, "max", temp.temperature.max);
						}

						result &= ReadJSNode(typeNode, "name", temp.name);
						
						biome.distRanges.push_back(temp);
					}
				}
			}

			if (cJSON* sphericalVoronoiNode = cJSON_GetObjectItem(rootNode, "spherical voronoi region"))
			{
				result &= ReadJSNode(sphericalVoronoiNode, "voronoi seed", biome.voronoiSeed);
				result &= ReadJSNode(sphericalVoronoiNode, "voronoi sqrt quads", biome.voronoiSqrtQuads);
			}

			cJSON_Delete(rootNode);
		}

		return (result);
	}
		
	bool
	SphericalTerrainResource::loadBiomeComposition(BiomeComposition& compo)
	{
		bool result = true;
		cJSON* rootNode = readJSONFile(compo.filePath.c_str());
		if(rootNode)
		{
			loadVisualGroupData(rootNode);

			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "compositions");
			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);

				int biomeLimit = BiomeComposition::s_maxBiomeCountInPlanet;

				if(BiomeLibrary::m_enableUnlimitBiomeCount)
				{
					biomeLimit = BiomeLibrary::m_unlimitBiomeCount;
				}                
				
				if (typeCount > biomeLimit)
				{
					LogManager::instance()->
						logMessage("-error-\t Biome count in planet is out of max limit: " + std::to_string(biomeLimit) + "!",
								   LML_CRITICAL);
					typeCount = biomeLimit;
				}
				
				compo.biomes.clear();

				std::vector<uint32> tempUsedBiomeIDArray;
				
				cJSON* typeNode = 0;
				for (int type_index = 0;
					 type_index < typeCount;
					 type_index++)
				{
					//NOTE:Clear temp data.
					Biome temp = {};
				
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						uint32 biomeID = BiomeTemplate::s_invalidBiomeID;
						result &= ReadJSNode(typeNode, "biome id", biomeID);

						//IMPORTANT(yanghang):Old version use name as ID. new versiong use
						// integer type ID.
						// TODO:Remove when name as ID doesn't using any more.
						GetBiomeResult biomeResult;
					
						biomeResult =
							SphericalTerrainResourceManager::instance()->getBiomeFromLibrary(biomeID);   
					
						if (biomeResult.success)
						{
							bool biomeUsed = false;
							for(auto& id : tempUsedBiomeIDArray)
							{
								if(id == biomeResult.biome.biomeID)
								{
									biomeUsed = true;
									break;
								}
							}

							//IMPORTANT(yanghang):Different biome compositions can't have the same biome template.
							// Using the invalid biome template to instead.
							if(!biomeUsed)
							{
								temp.biomeTemp = biomeResult.biome;
								tempUsedBiomeIDArray.push_back(biomeResult.biome.biomeID);
							}
						}
						else
						{
							LogManager::instance()->
								logMessage("-error-\t biome file [" + compo.filePath + "] 's biome [" + temp.biomeTemp.name + "] isn't find in the global biome library!",
										   LML_CRITICAL);
						}

						cJSON* vegLayerArray = cJSON_GetObjectItem(typeNode, "VegLayers");
						if(vegLayerArray)
						{
							int vegCount = cJSON_GetArraySize(vegLayerArray);
							for (int vegIndex = 0;
								 vegIndex < vegCount;
								 vegIndex++)
							{
								BiomeVegetation::VegLayerInfo vegLayer;
								cJSON* vegLayerItem = cJSON_GetArrayItem(vegLayerArray, vegIndex);
								ReadJSNode(vegLayerItem, "name", vegLayer.name);
								ReadJSNode(vegLayerItem, "customParam", vegLayer.customParam);
								ReadJSNode(vegLayerItem, "scaleBase", vegLayer.scaleBase);
								ReadJSNode(vegLayerItem, "scaleRange", vegLayer.scaleRange);
								ReadJSNode(vegLayerItem, "density", vegLayer.density);

								ReadJSNode(vegLayerItem, "topDiffuse", vegLayer.topDiffuse);
								ReadJSNode(vegLayerItem, "topDiffuseRatio", vegLayer.topDiffuseRatio);
								ReadJSNode(vegLayerItem, "bottomDiffuse", vegLayer.bottomDiffuse);
								ReadJSNode(vegLayerItem, "bottomDiffuseRatio", vegLayer.bottomDiffuseRatio);
								ReadJSNode(vegLayerItem, "lerpFactor", vegLayer.lerpFactor);
								
								temp.vegLayers.push_back(vegLayer);
							}
						}

						ReadJSNode(typeNode, "genObjects", temp.genObjects);

						compo.biomes.push_back(temp);
					}
				}


				
			}

			ReadJSNode(rootNode, "genObjectVirLibrary", compo.genObjectVirLibrary);
			ReadJSNode(rootNode, "randomSeed", compo.randomSeed);

			cJSON_Delete(rootNode);
		}
		else
		{
			result = false;
			LogManager::instance()->logMessage("-error-\t biome compositoin file [" + compo.filePath + "]  parse failed!",
											   LML_CRITICAL);
		}
		return (result);
	}

	bool
	SphericalTerrainResource::loadRegion(cJSON* pNode, RegionData& region)
	{
		bool result = true;
		
		if (pNode)
		{
			loadRegionData(pNode, region, result);
		}
		else
		{
			result = false;
		}
		return (result);
	}

	void
	SphericalTerrainResource::saveRegion(cJSON* pNode, const RegionData& region)
	{
		saveRegionData(pNode, region);
	}

	bool SphericalTerrainResource::loadOfflineRegionMap(const std::string& path, std::array<Bitmap, 6>& maps)
	{
		if (path.empty()) return false;

		String offline_region_path = path;
		
		auto n = offline_region_path.find(".terrain");
		if (n != String::npos)
		{
			offline_region_path = offline_region_path.substr(0, n);
		}

		bool result = true;

		for (int j = 0; j < maps.size(); ++j)
		{
			maps[j].freeMemory();
		}
		
		for (int i = 0; i < maps.size(); ++i)
		{
			std::string p = offline_region_path + "_" + std::to_string(i) + ".offlinemap";

			if (!PlanetManager::readBitmap(maps[i], p.c_str()))
			{
				result = false;
				break;
			}
		}
		if (!result)
		{
			for (int j = 0; j < 6; ++j)
			{
				maps[j].freeMemory();
			}
		}
		return result;
	}

	void SphericalTerrainResource::saveOfflineRegionMap(const std::string& path, const std::array<Bitmap, 6>& maps)
	{
		if (path.empty()) return;

		String offline_region_path = path;
		
		auto n = offline_region_path.find(".terrain");
		if (n != String::npos)
		{
			offline_region_path = offline_region_path.substr(0, n);
		}

		for (int i = 0; i < maps.size(); ++i)
		{
			std::string p = offline_region_path + "_" + std::to_string(i) + ".offlinemap";
			PlanetManager::writeBitmap(maps[i], p.c_str());
		}
		
	}

	bool SphericalTerrainResource::loadSphericalVoronoiRegion(cJSON* pNode, SphericalVoronoiRegionData& sphericalVoronoiRegion)
	{
		bool Result = false;
		if (cJSON* sphericalVoronoiRegionNode = cJSON_GetObjectItem(pNode, "spherical voronoi region"))
		{
			if (sphericalVoronoiRegionNode->type == cJSON_Array)
			{
				Result = true;
				sphericalVoronoiRegion.defineArray.clear();

				int typeCount = cJSON_GetArraySize(sphericalVoronoiRegionNode);

				cJSON* typeNode = 0;
				for (int type_index = 0; type_index < typeCount; type_index++)
				{
					SphericalVoronoiRegionDefine define = {};

					typeNode = cJSON_GetArrayItem(sphericalVoronoiRegionNode, type_index);
					if (typeNode)
					{
						Result &= ReadJSNode(typeNode, "ExtraStr", define.StrEx);
						Result &= ReadJSNode(typeNode, "ExtraInt", define.IntEx);
						Result &= ReadJSNode(typeNode, "TemplateTOD", define.TemplateTOD);
						Result &= ReadJSNode(typeNode, "Parent", define.Parent);
#ifdef ECHO_EDITOR
						Result &= ReadJSNode(typeNode, "name", define.name);
#endif

						sphericalVoronoiRegion.defineArray.push_back(define);
					}
				}
			}
		}

		bool Result__ = false;
		if (cJSON* sphericalVoronoiRegionNode = cJSON_GetObjectItem(pNode, "spherical voronoi coarse region"))
		{
			if (sphericalVoronoiRegionNode->type == cJSON_Array)
			{
				Result__ = true;
				sphericalVoronoiRegion.defineBaseArray.clear();

				int typeCount = cJSON_GetArraySize(sphericalVoronoiRegionNode);

				cJSON* typeNode = 0;
				for (int type_index = 0; type_index < typeCount; type_index++)
				{
					SphericalVoronoiRegionDefineBase define = {};

					typeNode = cJSON_GetArrayItem(sphericalVoronoiRegionNode, type_index);
					if (typeNode)
					{
						Result__ &= ReadJSNode(typeNode, "ExtraStr", define.StrEx);
						Result__ &= ReadJSNode(typeNode, "ExtraInt", define.IntEx);
#ifdef ECHO_EDITOR
						Result__ &= ReadJSNode(typeNode, "name", define.name);
#endif

						sphericalVoronoiRegion.defineBaseArray.push_back(define);
					}
				}
			}
		}

		bool Result_ = false;
		if (cJSON* sphericalVoronoiAdjRegionNode = cJSON_GetObjectItem(pNode, "spherical voronoi coarse adjacent regions"))
		{
			if (sphericalVoronoiAdjRegionNode->type == cJSON_Object)
			{
				int typeCount = cJSON_GetArraySize(sphericalVoronoiAdjRegionNode);
				assert(typeCount <= 255);
				sphericalVoronoiRegion.coarse_adjacent_regions.clear();
				sphericalVoronoiRegion.coarse_adjacent_regions.resize(typeCount);
				for (int type_index = 0; type_index < typeCount; ++type_index)
				{
					cJSON* typeNode = cJSON_GetArrayItem(sphericalVoronoiAdjRegionNode, type_index);
					assert(std::to_string(type_index) == String(typeNode->string));
					{
						auto& vec = sphericalVoronoiRegion.coarse_adjacent_regions[type_index];
						assert(typeNode->type == cJSON_Array);
						int count = cJSON_GetArraySize(typeNode);
						for (int i = 0; i < count; ++i)
						{
							if (auto iNode = cJSON_GetArrayItem(typeNode, i))
							{
								vec.push_back(iNode->valueint);
							}
						}
					}
				}
				Result_ = true;
			}

		}
		
		Result &= ReadJSNode(pNode, "spherical voronoi regionmap prefix", sphericalVoronoiRegion.prefix);
		Result &= ReadJSNode(pNode, "spherical voronoi regionmap coarse prefix", sphericalVoronoiRegion.coarse_prefix);
		ReadJSNode(pNode, "region offset", sphericalVoronoiRegion.offset);

		return Result && Result_ && Result__;
	}

	void SphericalTerrainResource::saveSphericalVoronoiRegion(cJSON* pNode, const SphericalVoronoiRegionData& sphericalVoronoiRegion)
	{
		WriteJSNode(pNode, "spherical voronoi regionmap prefix", sphericalVoronoiRegion.prefix);
		WriteJSNode(pNode, "spherical voronoi regionmap coarse prefix", sphericalVoronoiRegion.coarse_prefix);
		WriteJSNode(pNode, "region offset", sphericalVoronoiRegion.offset);

		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(pNode, "spherical voronoi region", typeArrayNode);

		if (!sphericalVoronoiRegion.defineArray.empty())
		{
			cJSON* typeNode = 0;

			int typeCount = (int)sphericalVoronoiRegion.defineArray.size();
			for (int type_index = 0; type_index < typeCount; type_index++)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				const auto& define = sphericalVoronoiRegion.defineArray[type_index];

				WriteJSNode(typeNode, "ExtraStr", define.StrEx);
				WriteJSNode(typeNode, "ExtraInt", define.IntEx);
				WriteJSNode(typeNode, "TemplateTOD", define.TemplateTOD);
				WriteJSNode(typeNode, "Parent", define.Parent);
#ifdef ECHO_EDITOR
				WriteJSNode(typeNode, "name", define.name);
#endif
				
			}
		}

		cJSON* typeBaseArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(pNode, "spherical voronoi coarse region", typeBaseArrayNode);

		if (!sphericalVoronoiRegion.defineBaseArray.empty())
		{
			cJSON* typeNode = 0;

			int typeCount = (int)sphericalVoronoiRegion.defineBaseArray.size();
			for (int type_index = 0; type_index < typeCount; type_index++)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeBaseArrayNode, typeNode);

				const auto& define = sphericalVoronoiRegion.defineBaseArray[type_index];

				WriteJSNode(typeNode, "ExtraStr", define.StrEx);
				WriteJSNode(typeNode, "ExtraInt", define.IntEx);
#ifdef ECHO_EDITOR
				WriteJSNode(typeNode, "name", define.name);
#endif

			}
		}

		cJSON* typeObjNode = cJSON_CreateObject();
		cJSON_AddItemToObject(pNode, "spherical voronoi coarse adjacent regions", typeObjNode);

		for (int i = 0; i < (int)sphericalVoronoiRegion.coarse_adjacent_regions.size(); ++i)
		{
			auto& vec = sphericalVoronoiRegion.coarse_adjacent_regions[i];
			cJSON* iNode = cJSON_CreateIntArray(vec.data(), (int)vec.size());
			cJSON_AddItemToObject(typeObjNode, std::to_string(i).c_str(), iNode);
		}


	}

	bool SphericalTerrainResource::verifySphericalVoronoiRegion(SphericalVoronoiRegionData& CurrentRegion, const String& terrain)
	{
		assert(!CurrentRegion.Loaded);
		const auto& prefix = CurrentRegion.prefix;
		try
		{
			CurrentRegion.mData = PlanetRegionManager::instance()->prepare(Name(prefix));
		}
		catch (...)
		{
			LogManager::instance()->logMessage("-error-\t An exception is generated when preparing Region {" + prefix + "} of terrain {" + terrain + "}.", LML_CRITICAL);
			CurrentRegion.mData.setNull();
		}

		if (CurrentRegion.IsFineLevelDataLegal())
		{
			auto error = CurrentRegion.IsCoarseLevelDataLegal();

#ifdef ECHO_EDITOR
			if (error == SphericalVoronoiRegionData::INCOMPATIBLE)
			{
				LogManager::instance()->logMessage(
					"-error-\t Planet [" + terrain + "]'s coarse level data seems incompatible! No coarse level data auto-generation will happen.", LML_CRITICAL);
			}
			if (error == SphericalVoronoiRegionData::COMPATIBLE)
			{
				LogManager::instance()->logMessage( "-Beginning-\t Planet [" + terrain + "]'s coarse level data auto-generation", LML_CRITICAL);		
				{
					auto Region = CurrentRegion;
					
					if (Region.GenerateCoarseLevelData())
					{
						CurrentRegion = Region;
						error = SphericalVoronoiRegionData::LEGAL;
#ifdef _DEBUG
						LogManager::instance()->logMessage(
							"-info-\t Planet [" + terrain + "]'s coarse level data auto-generation succeeded.", LML_CRITICAL);
#endif
					}
					else
					{
						LogManager::instance()->logMessage(
							"-error-\t Planet [" + terrain + "]'s coarse level data auto-generation failed.", LML_CRITICAL);

					}

				}
				LogManager::instance()->logMessage( "-Ending-\t Planet [" + terrain + "]'s coarse level data auto-generation", LML_CRITICAL);
			}
#endif
			if (error != SphericalVoronoiRegionData::LEGAL)
			{
				LogManager::instance()->logMessage(
					"-error-\t Planet [" + terrain + "]'s coarse level data is not legal!", LML_CRITICAL);
			}

			CurrentRegion.Loaded = (error == SphericalVoronoiRegionData::LEGAL);

		}
		else
		{
			LogManager::instance()->logMessage(
				"-error-\t Planet [" + terrain + "]'s fine level data is not legal!", LML_CRITICAL);
		}


		if (!CurrentRegion.Loaded)
		{
			LogManager::instance()->logMessage("-error-\t Planet [" + terrain + "] 两级区域没有加载成功!", LML_CRITICAL);
		}
#ifdef _DEBUG
		else
		{
			LogManager::instance()->logMessage("-info-\t Planet [" + terrain + "] 两级区域加载成功!", LML_CRITICAL);
		}
#endif

		return CurrentRegion.Loaded;
	}

	bool
	SphericalTerrainResource::loadClimateProcess(cJSON* climateProcessNode, ClimateProcessData& climateProcess)
	{
		bool result = true;
		cJSON* jsNode = climateProcessNode;
		if(jsNode)
		{
			cJSON* humidityNode = cJSON_GetObjectItem(jsNode, "humidity");
			if(humidityNode)
			{
				result &= ReadJSNode(humidityNode,"invert",climateProcess.humidityIP.invert);
				result &= ReadJSNode(humidityNode,"exposure",climateProcess.humidityIP.exposure);
				result &= ReadJSNode(humidityNode,"exposure offset",climateProcess.humidityIP.exposureOffset);
				result &= ReadJSNode(humidityNode,"exposure gamma",climateProcess.humidityIP.exposureGamma);
				result &= ReadJSNode(humidityNode,"brightness",climateProcess.humidityIP.brightness);
				result &= ReadJSNode(humidityNode,"clamp min",climateProcess.humidityIP.clamp.min);
				result &= ReadJSNode(humidityNode,"clamp max",climateProcess.humidityIP.clamp.max);
			}

			cJSON* tempNode = cJSON_GetObjectItem(jsNode, "temperature");
			if(tempNode)
			{
				result &= ReadJSNode(tempNode,"invert",climateProcess.temperatureIP.invert);
				result &= ReadJSNode(tempNode,"exposure",climateProcess.temperatureIP.exposure);
				result &= ReadJSNode(tempNode,"exposure offset",climateProcess.temperatureIP.exposureOffset);
				result &= ReadJSNode(tempNode,"exposure gamma",climateProcess.temperatureIP.exposureGamma);
				result &= ReadJSNode(tempNode,"brightness",climateProcess.temperatureIP.brightness);
				result &= ReadJSNode(tempNode,"clamp min",climateProcess.temperatureIP.clamp.min);
				result &= ReadJSNode(tempNode,"clamp max",climateProcess.temperatureIP.clamp.max);
			}

			if (cJSON* aoNode = cJSON_GetObjectItem(jsNode, "ao"))
			{
				result &= ReadJSNode(aoNode, "radius", climateProcess.aoParam.radius);
				result &= ReadJSNode(aoNode, "amplitude", climateProcess.aoParam.amplitude);
			}
		}
		else
		{
			result = false;
		}

		return (result);        
	}

	bool
	SphericalTerrainResource::loadSphericalOcean(SphericalOceanData& ocean)
	{
		bool result = false;
		cJSON* rootNode = readJSONFile(ocean.filePath.c_str());
		if(rootNode)
		{
			result = loadSphericalOcean(rootNode, ocean);
            cJSON_Delete(rootNode);
		}
		else
		{
			result = false;
			LogManager::instance()->logMessage("-error-\t ocean file [" + ocean.filePath + "]  parse failed!",
											   LML_CRITICAL);
		}
		return result;
	}

	void
	SphericalTerrainResource::saveSphericalOcean(SphericalOceanData& ocean)
	{
		if (ocean.filePath.empty())
		{
			return;
		}

		cJSON* rootNode = cJSON_CreateObject();
		saveSphericalOcean(rootNode, ocean);
		
		saveJSONFile(ocean.filePath, rootNode);
		cJSON_Delete(rootNode);
	}
	
	bool
	SphericalTerrainResource::loadSphericalOcean(cJSON* sphericalOceanNode, SphericalOceanData& sphericalOcean)
	{
		bool result = true;
		cJSON* jsNode = sphericalOceanNode;
		if (jsNode)
		{
			cJSON* TexPathNode = cJSON_GetObjectItem(jsNode, "texture path");
			if (TexPathNode)
			{
				//ReadJSNode(TexPathNode, "mesh file", sphericalOcean.fileSetting.MeshFile);
				result &= ReadJSNode(TexPathNode, "normal texture", sphericalOcean.fileSetting.NormalTextureName);
				result &= ReadJSNode(TexPathNode, "reflect texture", sphericalOcean.fileSetting.ReflectTextureName);
				result &= ReadJSNode(TexPathNode, "caustic texture", sphericalOcean.fileSetting.CausticTextureName);
				result &= ReadJSNode(TexPathNode, "tunnel texture", sphericalOcean.fileSetting.TunnelTextureName);
				result &= ReadJSNode(TexPathNode, "foam texture", sphericalOcean.fileSetting.FoamTextureName);
			}

			cJSON* GeometryNode = cJSON_GetObjectItem(jsNode, "geometry");
			if (GeometryNode)
			{
				result &= ReadJSNode(GeometryNode, "radius", sphericalOcean.geomSettings.radius);
				result &= ReadJSNode(GeometryNode, "resolution", sphericalOcean.geomSettings.resolution);
				ReadJSNode(GeometryNode, "max Depth", sphericalOcean.geomSettings.maxDepth);
				ReadJSNode(GeometryNode, "relativeMinorRadius", sphericalOcean.geomSettings.relativeMinorRadius);
				for (int i = 0; i < 4; i++)
				{
					std::vector<float> waveInfo;
					result &= ReadJSNode(GeometryNode, (std::string("SphereWaveInfo_") + std::to_string(i)).c_str(), waveInfo);
					int index = 0;
					sphericalOcean.geomSettings.sphereWaveInfo[i].amp = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].sharp = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].freq = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].phase = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].dir.x = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].dir.y = waveInfo[index++];
					sphericalOcean.geomSettings.sphereWaveInfo[i].dir.z = waveInfo[index++];
				}
			}

			cJSON* ShadeNode = cJSON_GetObjectItem(jsNode, "shade param");
			if (ShadeNode)
			{
				result &= ReadJSNode(ShadeNode, "shallow water color", sphericalOcean.shadeSettings.shallowColor);
				result &= ReadJSNode(ShadeNode, "deep water color", sphericalOcean.shadeSettings.deepColor);

				result &= ReadJSNode(ShadeNode, "foamColor", sphericalOcean.shadeSettings.m_foamColor);
				result &= ReadJSNode(ShadeNode, "reflectColor", sphericalOcean.shadeSettings.m_reflectColor);
												
				result &= ReadJSNode(ShadeNode, "texture scale", sphericalOcean.shadeSettings.textureScale);
				result &= ReadJSNode(ShadeNode, "bumpScale", sphericalOcean.shadeSettings.bumpScale);
				result &= ReadJSNode(ShadeNode, "flowSpeed", sphericalOcean.shadeSettings.flowSpeed);
				result &= ReadJSNode(ShadeNode, "reflectionAmount", sphericalOcean.shadeSettings.reflectionAmount);
				result &= ReadJSNode(ShadeNode, "waveAmount", sphericalOcean.shadeSettings.waveAmount);
				result &= ReadJSNode(ShadeNode, "flowParam_z", sphericalOcean.shadeSettings.flowParam_z);
				result &= ReadJSNode(ShadeNode, "flowParam_w", sphericalOcean.shadeSettings.flowParam_w);
				result &= ReadJSNode(ShadeNode, "fresnelBias", sphericalOcean.shadeSettings.fresnelBias);
				result &= ReadJSNode(ShadeNode, "fresnelIntensity", sphericalOcean.shadeSettings.fresnelIntensity);
				result &= ReadJSNode(ShadeNode, "fresnelRatio", sphericalOcean.shadeSettings.fresnelRatio);
				result &= ReadJSNode(ShadeNode, "foamBias", sphericalOcean.shadeSettings.foamBias);
				result &= ReadJSNode(ShadeNode, "foamSoft", sphericalOcean.shadeSettings.foamSoft);
				result &= ReadJSNode(ShadeNode, "foamTilling", sphericalOcean.shadeSettings.foamTilling);
				result &= ReadJSNode(ShadeNode, "borderSoft", sphericalOcean.shadeSettings.borderSoft);
				result &= ReadJSNode(ShadeNode, "specularPower", sphericalOcean.shadeSettings.specularPower);
				result &= ReadJSNode(ShadeNode, "specularStrength", sphericalOcean.shadeSettings.specularStrength);
				
				bool hasLightRatio = ReadJSNode(ShadeNode, "lightStrengthRatio", sphericalOcean.shadeSettings.lightStrengthRatio);
				if (!hasLightRatio) {
					//An artist sitisfied default value
					sphericalOcean.shadeSettings.lightStrengthRatio = .3f;
				}
				
				result &= ReadJSNode(ShadeNode, "reflectionBlur", sphericalOcean.shadeSettings.reflectionBlur);
				result &= ReadJSNode(ShadeNode, "refractAmount", sphericalOcean.shadeSettings.refractAmount);
			}


			result &= ReadJSNode(jsNode, "bIsUseTod", sphericalOcean.m_bUseTod);
		}
		else
		{
			result = false;
		}

		return (result);
	}

	void
	SphericalTerrainResource::saveSphericalOcean(cJSON* sphericalOceanNode, SphericalOceanData& sphericalOcean)
	{
		cJSON* SphericalOceanNode = sphericalOceanNode;
		cJSON* TexPathNode = cJSON_CreateObject();
		cJSON* GeometryNode = cJSON_CreateObject();
		cJSON* ShadeNode = cJSON_CreateObject();

		cJSON_AddItemToObject(SphericalOceanNode, "texture path", TexPathNode);
		cJSON_AddItemToObject(SphericalOceanNode, "geometry", GeometryNode);
		cJSON_AddItemToObject(SphericalOceanNode, "shade param", ShadeNode);

		//TexPathNode
		//WriteJSNode(TexPathNode, "mesh file", sphericalOcean.fileSetting.MeshFile);
		WriteJSNode(TexPathNode, "normal texture", sphericalOcean.fileSetting.NormalTextureName);
		WriteJSNode(TexPathNode, "reflect texture", sphericalOcean.fileSetting.ReflectTextureName);
		WriteJSNode(TexPathNode, "caustic texture", sphericalOcean.fileSetting.CausticTextureName);
		WriteJSNode(TexPathNode, "tunnel texture", sphericalOcean.fileSetting.TunnelTextureName);
		WriteJSNode(TexPathNode, "foam texture", sphericalOcean.fileSetting.FoamTextureName);
		// End TexPathNode

		// GeometryNode
		WriteJSNode(GeometryNode, "radius", sphericalOcean.geomSettings.radius);
		WriteJSNode(GeometryNode, "resolution", sphericalOcean.geomSettings.resolution);
		WriteJSNode(GeometryNode, "max Depth", sphericalOcean.geomSettings.maxDepth);
		WriteJSNode(GeometryNode, "wave num", sphericalOcean.geomSettings.waveNum);
		WriteJSNode(GeometryNode, "relativeMinorRadius", sphericalOcean.geomSettings.relativeMinorRadius);

		for (int i = 0; i < 4; i++)
		{
			std::vector<float> waveInfo{ 									
			sphericalOcean.geomSettings.sphereWaveInfo[i].amp,
			sphericalOcean.geomSettings.sphereWaveInfo[i].sharp,
			sphericalOcean.geomSettings.sphereWaveInfo[i].freq,
			sphericalOcean.geomSettings.sphereWaveInfo[i].phase,
			sphericalOcean.geomSettings.sphereWaveInfo[i].dir.x,
			sphericalOcean.geomSettings.sphereWaveInfo[i].dir.y,
			sphericalOcean.geomSettings.sphereWaveInfo[i].dir.z
			};
			WriteJSNode(GeometryNode, (std::string("SphereWaveInfo_") + std::to_string(i)).c_str(), waveInfo);
		}
		// End GeometryNode


		// ShadeNode
		cJSON* shallowWater = cJSON_CreateArray();
		cJSON_AddItemToArray(shallowWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.shallowColor.r));
		cJSON_AddItemToArray(shallowWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.shallowColor.g));
		cJSON_AddItemToArray(shallowWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.shallowColor.b));
		cJSON_AddItemToArray(shallowWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.shallowColor.a));
		cJSON_AddItemToObject(ShadeNode, "shallow water color", shallowWater);

		cJSON* deepWater = cJSON_CreateArray();
		cJSON_AddItemToArray(deepWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.deepColor.r));
		cJSON_AddItemToArray(deepWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.deepColor.g));
		cJSON_AddItemToArray(deepWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.deepColor.b));
		cJSON_AddItemToArray(deepWater, cJSON_CreateNumber(sphericalOcean.shadeSettings.deepColor.a));
		cJSON_AddItemToObject(ShadeNode, "deep water color", deepWater);

		cJSON* foamColor = cJSON_CreateArray();
		cJSON_AddItemToArray(foamColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_foamColor.r));
		cJSON_AddItemToArray(foamColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_foamColor.g));
		cJSON_AddItemToArray(foamColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_foamColor.b));
		cJSON_AddItemToArray(foamColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_foamColor.a));
		cJSON_AddItemToObject(ShadeNode, "foamColor", foamColor);

		cJSON* reflectColor = cJSON_CreateArray();
		cJSON_AddItemToArray(reflectColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_reflectColor.r));
		cJSON_AddItemToArray(reflectColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_reflectColor.g));
		cJSON_AddItemToArray(reflectColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_reflectColor.b));
		cJSON_AddItemToArray(reflectColor, cJSON_CreateNumber(sphericalOcean.shadeSettings.m_reflectColor.a));
		cJSON_AddItemToObject(ShadeNode, "reflectColor", reflectColor);

		cJSON* textureScale = cJSON_CreateArray();
		cJSON_AddItemToArray(textureScale, cJSON_CreateNumber(sphericalOcean.shadeSettings.textureScale.x));
		cJSON_AddItemToArray(textureScale, cJSON_CreateNumber(sphericalOcean.shadeSettings.textureScale.y));
		cJSON_AddItemToObject(ShadeNode, "texture scale", textureScale);

		WriteJSNode(ShadeNode, "bumpScale", sphericalOcean.shadeSettings.bumpScale);
		WriteJSNode(ShadeNode, "flowSpeed", sphericalOcean.shadeSettings.flowSpeed);
		WriteJSNode(ShadeNode, "reflectionAmount", sphericalOcean.shadeSettings.reflectionAmount);
		WriteJSNode(ShadeNode, "waveAmount", sphericalOcean.shadeSettings.waveAmount);
		WriteJSNode(ShadeNode, "flowParam_z", sphericalOcean.shadeSettings.flowParam_z);
		WriteJSNode(ShadeNode, "flowParam_w", sphericalOcean.shadeSettings.flowParam_w);

		WriteJSNode(ShadeNode, "fresnelBias", sphericalOcean.shadeSettings.fresnelBias);
		WriteJSNode(ShadeNode, "fresnelIntensity", sphericalOcean.shadeSettings.fresnelIntensity);
		WriteJSNode(ShadeNode, "fresnelRatio", sphericalOcean.shadeSettings.fresnelRatio);

		WriteJSNode(ShadeNode, "foamBias", sphericalOcean.shadeSettings.foamBias);
		WriteJSNode(ShadeNode, "foamSoft", sphericalOcean.shadeSettings.foamSoft);
		WriteJSNode(ShadeNode, "foamTilling", sphericalOcean.shadeSettings.foamTilling);
		WriteJSNode(ShadeNode, "borderSoft", sphericalOcean.shadeSettings.borderSoft);

		WriteJSNode(ShadeNode, "specularPower", sphericalOcean.shadeSettings.specularPower);
		WriteJSNode(ShadeNode, "specularStrength", sphericalOcean.shadeSettings.specularStrength);
		WriteJSNode(ShadeNode, "lightStrengthRatio", sphericalOcean.shadeSettings.lightStrengthRatio);
		WriteJSNode(ShadeNode, "reflectionBlur", sphericalOcean.shadeSettings.reflectionBlur);
		WriteJSNode(ShadeNode, "refractAmount", sphericalOcean.shadeSettings.refractAmount);

		WriteJSNode(SphericalOceanNode, "bIsUseTod", sphericalOcean.m_bUseTod);
	}

	void
	SphericalTerrainResource::saveClimateProcess(cJSON* climateProcessNode, const ClimateProcessData& climateProcess)
	{
		cJSON* jsNode = climateProcessNode;

		cJSON* humidityNode = cJSON_CreateObject();
		cJSON_AddItemToObject(jsNode, "humidity", humidityNode);
		WriteJSNode(humidityNode,"invert",climateProcess.humidityIP.invert);
		WriteJSNode(humidityNode,"exposure",climateProcess.humidityIP.exposure);
		WriteJSNode(humidityNode,"exposure offset",climateProcess.humidityIP.exposureOffset);
		WriteJSNode(humidityNode,"exposure gamma",climateProcess.humidityIP.exposureGamma);
		WriteJSNode(humidityNode,"brightness",climateProcess.humidityIP.brightness);
		WriteJSNode(humidityNode,"clamp min",climateProcess.humidityIP.clamp.min);
		WriteJSNode(humidityNode,"clamp max",climateProcess.humidityIP.clamp.max);
		
		cJSON* tempNode = cJSON_CreateObject();
		cJSON_AddItemToObject(jsNode, "temperature", tempNode);
		WriteJSNode(tempNode,"invert",climateProcess.temperatureIP.invert);
		WriteJSNode(tempNode,"exposure",climateProcess.temperatureIP.exposure);
		WriteJSNode(tempNode,"exposure offset",climateProcess.temperatureIP.exposureOffset);
		WriteJSNode(tempNode,"exposure gamma",climateProcess.temperatureIP.exposureGamma);
		WriteJSNode(tempNode,"brightness",climateProcess.temperatureIP.brightness);
		WriteJSNode(tempNode,"clamp min",climateProcess.temperatureIP.clamp.min);
		WriteJSNode(tempNode,"clamp max",climateProcess.temperatureIP.clamp.max);

		cJSON* aoNode = cJSON_CreateObject();
		cJSON_AddItemToObject(jsNode, "ao", aoNode);
		WriteJSNode(aoNode, "radius", climateProcess.aoParam.radius);
		WriteJSNode(aoNode, "amplitude", climateProcess.aoParam.amplitude);
	}

	BiomeGPUData
	SphericalTerrainResourceManager::fillBiomeDiffuseTextureArray(BiomeComposition& biome)
	{
		BiomeGPUData_Cache cacheData;
		fillBiomeDiffuseTextureArray_Async(biome, cacheData);
		BiomeGPUData result = {};
		fillBiomeDiffuseTextureArray_Sync(cacheData, result);
		return result;
	}

	void 
	SphericalTerrainResourceManager::fillBiomeDiffuseTextureArray_Async(BiomeComposition& biomeCompo, BiomeGPUData_Cache& dataCache)
	{
		// dataCache.cpuData = PlanetManager::generateBiomeLookupMap(biomeCompo, false).CpuData;

		{
#if defined(__ANDROID__) || defined(__APPLE__)
			DataStreamPtr defaultLayerTexData(Root::instance()->
				GetMemoryDataStream("echo/biome_terrain/Textures/default_layer.ktx"));
#else
			DataStreamPtr defaultLayerTexData(Root::instance()->
				GetMemoryDataStream("echo/biome_terrain/Textures/default_layer.dds"));
#endif
			if (!defaultLayerTexData.isNull())
			{
				dataCache.defaultLayerTexData = defaultLayerTexData;
			}
			else
			{
				LogManager::instance()->logMessage("-error-\t Biome texture array's default_layer texture isn't exist!", LML_CRITICAL);
			}
		}

		{
			int biomeTexLayerCount = (int)biomeCompo.usedBiomeTemplateList.size();
			
			if (biomeTexLayerCount > 0)
			{
				dataCache.biomeArray.resize(biomeTexLayerCount);
				for (int i = 0; i != biomeTexLayerCount; ++i)
				{
					const BiomeTemplate& biomeTemp = biomeCompo.usedBiomeTemplateList[i];
					BiomeDefine_Cache& biomeC = dataCache.biomeArray[i];

					String diffusePath, normalPath;
					diffusePath = biomeTemp.diffuse;
					normalPath = biomeTemp.normal;
#if defined(__ANDROID__) || defined(__APPLE__)
					{
						auto fixPos = diffusePath.find_first_of(".");
						if (fixPos != String::npos)
						{
							diffusePath = diffusePath.substr(0, fixPos);
						}
						diffusePath += ".ktx";
					}
#endif

#if defined(__ANDROID__) || defined(__APPLE__)
					{
						auto fixPos = normalPath.find_first_of(".");
						if (fixPos != String::npos)
						{
							normalPath = normalPath.substr(0, fixPos);
						}
						normalPath += ".ktx";
					}
#endif
					{
						DataStreamPtr data(Root::instance()->GetMemoryDataStream(diffusePath.c_str()));
						if (!data.isNull())
						{
							biomeC.diffuse = data;
						}
						else
						{
							LogManager::instance()->logMessage("-error-\t biome [" + biomeTemp.name + "] diffuse texture isn't exist:" + biomeTemp.diffuse, LML_CRITICAL);
						}
					}

					{
						DataStreamPtr data(Root::instance()->GetMemoryDataStream(normalPath.c_str()));
						if (!data.isNull())
						{
							biomeC.normal = data;
						}
						else
						{
							LogManager::instance()->logMessage("-error-\t biome [" + biomeTemp.name + "] diffuse texture isn't exist:" + biomeTemp.diffuse, LML_CRITICAL);
						}
					}
				}
			}

		}
	}
	
	void 
	SphericalTerrainResourceManager::fillBiomeDiffuseTextureArray_Sync(BiomeGPUData_Cache& dataCache, BiomeGPUData& result)
	{
		// result.lookupTex = PlanetManager::generateTextureFromBitmap(dataCache.cpuData);
		// dataCache.cpuData.freeMemory();

		DataStream* defaultLayerTexData = dataCache.defaultLayerTexData.getPointer();
		if (defaultLayerTexData && defaultLayerTexData->size() == 0) {
			defaultLayerTexData = nullptr;
		}

		int biomeTexLayerCount = (int)dataCache.biomeArray.size();

		//IMPORTANT(yang):Use 512 texture now.
		static int s_texWidth = 512;
		static int s_texHeight = 512;
		static int s_texMipLevel = 10;

		static int s_normalWidth = 512;
		static int s_normalHeight = 512;
		static int s_normalMipLevel = 10;

#ifdef _WIN32
		static ECHO_FMT s_texFormat = ECHO_FMT_DDS_BC3;
		static ECHO_FMT s_normalFormat = ECHO_FMT_DDS_BC3;
#else 
		static ECHO_FMT s_texFormat = ECHO_FMT_RGBA_ASTC_6x6;
		static ECHO_FMT s_normalFormat = ECHO_FMT_RGBA_ASTC_4x4;
#endif
		if (biomeTexLayerCount > 0)
		{
			result.diffuseTexArray = TextureManager::instance()->
				createManual(
					TEX_TYPE_2D_ARRAY,
					s_texWidth,
					s_texHeight,
					biomeTexLayerCount,
					s_texMipLevel,
					s_texFormat,
					0,
					0);
			result.diffuseTexArray->load();

			result.normalTexArray = TextureManager::instance()->
				createManual(
					TEX_TYPE_2D_ARRAY,
					s_normalWidth,
					s_normalHeight,
					biomeTexLayerCount,
					s_normalMipLevel,
					s_normalFormat,
					0,
					0);
			result.normalTexArray->load();


			int layer_index = 0;
			for (auto& biomeC : dataCache.biomeArray)
			{

				DataStream* data = biomeC.diffuse.getPointer();
				if (!data || 
					(data && data->size() == 0)) {
					data = defaultLayerTexData;
				}
				if (data)
				{
					result.diffuseTexArray->setTex2DArrayData(data->getData(),
						(uint32)data->size(),
						layer_index,
						false);
				}

				data = biomeC.normal.getPointer();
				if (!data ||
					(data && data->size() == 0)) {
					data = defaultLayerTexData;
				}
				if (data)
				{
					result.normalTexArray->setTex2DArrayData(data->getData(),
						(uint32)data->size(),
						layer_index,
						false);
				}

				layer_index++;
			}
		}
		else
		{
			//IMPORTANT:Rendering need a texture array, so create a default
			// 1 layer texture array.
			result.diffuseTexArray = TextureManager::instance()->
				createManual(
					TEX_TYPE_2D_ARRAY,
					s_texWidth,
					s_texHeight,
					1,
					s_texMipLevel,
					s_texFormat,
					0,
					0);


			result.normalTexArray = TextureManager::instance()->
				createManual(
					TEX_TYPE_2D_ARRAY,
					s_texWidth,
					s_texHeight,
					1,
					s_texMipLevel,
					s_texFormat,
					0,
					0);

			if (defaultLayerTexData)
			{
				result.diffuseTexArray->setTex2DArrayData(defaultLayerTexData->getData(),
					(uint32)defaultLayerTexData->size(),
					0,
					false);
				result.normalTexArray->setTex2DArrayData(defaultLayerTexData->getData(),
					(uint32)defaultLayerTexData->size(),
					0,
					false);
			}
		}

		uint64* biomeTexMemory = &m_biomeTexMemory;
		uint64 singleTexLayerSize = result.diffuseTexArray->getPixelMemorySize(s_texWidth, s_texHeight, s_texFormat);
		*biomeTexMemory += singleTexLayerSize * biomeTexLayerCount * 3;
		*biomeTexMemory +=
			BiomeGPUData::lookupTexWidth *
			BiomeGPUData::lookupTexHeight *
			BiomeGPUData::lookupTexelSize;

	}

	void Bitmap::freeMemory()
	{
		if(pixels)
		{
			free(pixels);
			pixels = 0;
		}
	}

	int Bitmap::size() const
	{
		int result = 0;
		result = pitch * height;
		return (result);
	}

	void Bitmap::copyTo(Bitmap& other) const
	{
		if (this == &other) return;
		other.width = width;
		other.height = height;
		other.pitch = pitch;
		other.bytesPerPixel = bytesPerPixel;
		other.byteSize = byteSize;
		other.freeMemory();
		if (pixels)
		{
			size_t _size = size();
			other.pixels = static_cast<float*>(malloc(_size));
			if (other.pixels) 
			{
				memcpy((void*)other.pixels, (void*)pixels, _size);
			}
		}
	}

	bool BiomeLibrary::load()
	{
		bool result = true;

		{
			cJSON* cfgNode = readJSONFile("biome_terrain/library.cfg");
			if(cfgNode)
			{
				String newLibraryPath;
				bool useCustomPath = false;
				ReadJSNode(cfgNode, "enableLibPath", useCustomPath);
				if(ReadJSNode(cfgNode, "libPath", newLibraryPath))
				{
					if(!newLibraryPath.empty() && useCustomPath)
					{
						m_filePath = newLibraryPath;
					}
				}

				ReadJSNode(cfgNode, "internalMode", m_internalMode);

				cJSON* limitNode = cJSON_GetObjectItem(cfgNode, "biomeCountUnlimit");
				if(limitNode)
				{
					ReadJSNode(limitNode, "enable",
							   m_enableUnlimitBiomeCount);
					ReadJSNode(limitNode, "maxCount",
							   m_unlimitBiomeCount);
				}
                cJSON_Delete(cfgNode);
			}
		}

		cJSON* rootNode = readJSONFile(m_filePath.c_str());
		if(rootNode)
		{
			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "type");

			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);
				if (typeCount > s_maxGlobalBiomeCount)
				{
					LogManager::instance()->logMessage("-error-\t global biome library count is out of max limit " + std::to_string(s_maxGlobalBiomeCount) + "!",
													   LML_CRITICAL);
				}

				cJSON* typeNode = 0;
				if (!m_allBiomes.empty())
				{
					m_allBiomes.clear();
				}

				for (int type_index = 0;
					 type_index < typeCount;
					 type_index++)
				{
					BiomeTemplate temp;
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						bool nodeParseSuccess = true;
						//TODO(yang):Only this on editor mode.
						nodeParseSuccess &= ReadJSNode(typeNode, "name", temp.name);
						
						nodeParseSuccess &= ReadJSNode(typeNode, "diffuse texture", temp.diffuse);
						nodeParseSuccess &= ReadJSNode(typeNode, "normal texture", temp.normal);
						nodeParseSuccess &= ReadJSNode(typeNode, "custom tilling", temp.customTilling);
						nodeParseSuccess &= ReadJSNode(typeNode, "normal strength", temp.normalStrength);
                        nodeParseSuccess &= ReadJSNode(typeNode, "phy surface type", temp.phySurfaceType);

						nodeParseSuccess &= ReadJSNode(typeNode, "diffuse color", temp.diffuseColor);
						nodeParseSuccess &= ReadJSNode(typeNode, "emission", temp.emission);
						nodeParseSuccess &= ReadJSNode(typeNode, "enableTextureEmission", temp.enableTextureEmission);
						nodeParseSuccess &= ReadJSNode(typeNode, "customMetallic", temp.customMetallic);
						temp.biomeID = type_index;

						if(!nodeParseSuccess)
						{
							LogManager::instance()->logMessage("-error-\t global biome library parse error: item["+ std::to_string(type_index) +"]is broke!",
															   LML_CRITICAL);
						}

						cJSON* snowNode = cJSON_GetObjectItem(typeNode, "snow");
						if (snowNode)
						{
							nodeParseSuccess &= ReadJSNode(snowNode, "height", temp.snow.snowHeight);
							nodeParseSuccess &= ReadJSNode(snowNode, "tile", temp.snow.tile);
							nodeParseSuccess &= ReadJSNode(snowNode, "displacement", temp.snow.displacement);
							nodeParseSuccess &= ReadJSNode(snowNode, "fuctionOffset", temp.snow.functionOffset);
							nodeParseSuccess &= ReadJSNode(snowNode, "steepness", temp.snow.steepness);
							nodeParseSuccess &= ReadJSNode(snowNode, "range", temp.snow.snowRange);
							nodeParseSuccess &= ReadJSNode(snowNode, "scale", temp.snow.snowScale);
							nodeParseSuccess &= ReadJSNode(snowNode, "shapeSmoothStart", temp.snow.shapeSmoothStart);
							nodeParseSuccess &= ReadJSNode(snowNode, "shapeSmoothScale", temp.snow.shapeSmoothScale);
						}

						result &= nodeParseSuccess;



						//TODO(yang):Remove when old to new file format, translate finished.
						nodeParseSuccess &= ReadJSNode(typeNode, "color", temp.color);
						m_allBiomes.push_back(temp);
           
                    }
                }
            }
            else
            {
                LogManager::instance()->logMessage("-error-\t global biome library parse error: file structure is broke!",
                                                   LML_CRITICAL);
                result = false;
            }
            
            cJSON_Delete(rootNode);
        }
        else
        {
            LogManager::instance()->logMessage("-error-\t global biome library load failed! file: [" + m_filePath + "] isn't exist.",
                                               LML_CRITICAL);
            result = false;
        }
        
        return (result);
    }

    GetBiomeResult
    BiomeLibrary::getBiome(const String& name)
    {
        GetBiomeResult result = {};
        for(auto& biome: m_allBiomes)
        {
            if(biome.name == name)
            {
                result.success = true;
                result.biome = biome;
                break;
            }
        }

        return (result);
    }

    GetBiomeResult BiomeLibrary::getBiome(const uint32& biomeID)
    {
        GetBiomeResult result = {};

        if(biomeID != BiomeTemplate::s_invalidBiomeID &&
           biomeID < m_allBiomes.size())
        {
            BiomeTemplate& biome = m_allBiomes[biomeID];
            result.success = true;
            result.biome = biome;
        }

        return (result);
    }
    
    void BiomeLibrary::save()
    {
		cJSON* rootNode = cJSON_CreateObject();

		//TODO(yanghang):Remove edit time data at release.
		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(rootNode, "type", typeArrayNode);

		if (!m_allBiomes.empty())
		{
			cJSON* typeNode = 0;

			for (auto& temp : m_allBiomes)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				WriteJSNode(typeNode, "name", temp.name);
				
				WriteJSNode(typeNode, "diffuse texture", temp.diffuse);
				WriteJSNode(typeNode, "normal texture", temp.normal);

				WriteJSNode(typeNode, "custom tilling", temp.customTilling);
				WriteJSNode(typeNode, "normal strength", temp.normalStrength);

				WriteJSNode(typeNode, "phy surface type", temp.phySurfaceType);

				WriteJSNode(typeNode, "color", temp.color);

				WriteJSNode(typeNode, "diffuse color", temp.diffuseColor);
				WriteJSNode(typeNode, "emission", temp.emission);
				WriteJSNode(typeNode, "enableTextureEmission", temp.enableTextureEmission);
				WriteJSNode(typeNode, "customMetallic", temp.customMetallic);

				cJSON* snowNode = cJSON_CreateObject();
                cJSON_AddItemToObject(typeNode, "snow", snowNode);
                WriteJSNode(snowNode, "height", temp.snow.snowHeight);
                WriteJSNode(snowNode, "tile", temp.snow.tile);
                WriteJSNode(snowNode, "displacement", temp.snow.displacement);
                WriteJSNode(snowNode, "fuctionOffset", temp.snow.functionOffset);
                WriteJSNode(snowNode, "steepness", temp.snow.steepness);
                WriteJSNode(snowNode, "range", temp.snow.snowRange);
                WriteJSNode(snowNode, "scale", temp.snow.snowScale);
                WriteJSNode(snowNode, "shapeSmoothStart", temp.snow.shapeSmoothStart);
                WriteJSNode(snowNode, "shapeSmoothScale", temp.snow.shapeSmoothScale);
			}
		}
		saveJSONFile(m_filePath, rootNode);
		cJSON_Delete(rootNode);
    }

    GetBiomeResult
    SphericalTerrainResourceManager::getBiomeFromLibrary(const String& name)
    {
        return m_biomeLib.getBiome(name);
    }

	GetBiomeResult 
	SphericalTerrainResourceManager::getBiomeFromLibrary(const int32& biomeID)
	{
		return m_biomeLib.getBiome(biomeID);
	}

	bool SphericalTerrainResourceManager::getCloudTemplateFromLibrary(uint8 cloudTemId, PlanetCloudTemplateData& data)
	{
		return m_CloudLibrary.getCloudTemplateData(cloudTemId,data);
	}

	uint8 BiomeComposition::getBiomeIndex(int biomeCompositionIndex)const
	{
		uint8 biomeIndex = 0;
		if(biomeCompositionIndex < biomes.size())
		{
			uint32 biomeID = biomes[biomeCompositionIndex].biomeTemp.biomeID;
			for(int index = 0;
				index < usedBiomeTemplateList.size();
				index++)
			{
				const BiomeTemplate& biomeTemp = usedBiomeTemplateList[index];
				if(biomeTemp.biomeID == biomeID)
				{
					biomeIndex = index;
					break;
				}
			}
		}
		return biomeIndex;
	}

	ColorValue BiomeTemplate::getsRGBFinalColor() const
	{
		const ColorValue base(color.x, color.y, color.z);
		ColorValue mask = diffuseColor;
		mask.a          = 1.0f;
		return base * mask;
	}

	BiomeLibrary::BiomeLibrary()
	{
		m_filePath = "biome_terrain/default.biomelibrary"; 
	}

	void SphericalTerrainData::freshUsedBiomeTextureList()
	{
		//IMPORTANT(yanghang):Scan all biome composition find out textures
		// that aren't duplicated.
		composition.usedBiomeTemplateList.clear();

		int distCount = (int)distribution.distRanges.size();
		for(int distIndex = 0;
			distIndex < distCount;
			distIndex++)
		{
			if(distIndex < composition.biomes.size())
			{
				Biome& biome = composition.biomes[distIndex];

				if(biome.biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
				{
					bool isDuplicated = false;
					for(auto& useTexture : composition.usedBiomeTemplateList)
					{
						if(useTexture.biomeID == biome.biomeTemp.biomeID)
						{
							isDuplicated = true;
							break;
						}
					}

					if(!isDuplicated)
					{
						composition.usedBiomeTemplateList.push_back(biome.biomeTemp);
					}
				}
			}
		}
	}
	
	// const String SphericalVoronoiRegionLibrary::relativePath = "echo/biome_terrain/spherical_voronoi_map";
	



	SphericalVoronoiRegionData::SphericalVoronoiRegionData(const SphericalVoronoiRegionData& Other)
		: mData(Other.mData)
		, offset(Other.offset)
		, prefix(Other.prefix)
		, coarse_prefix(Other.coarse_prefix)
		, defineArray(Other.defineArray)
		, defineBaseArray(Other.defineBaseArray)
		, coarse_adjacent_regions(Other.coarse_adjacent_regions)
		, coarse_centers(Other.coarse_centers)
	{
		Loaded.store(Other.Loaded);
	}

	SphericalVoronoiRegionData& SphericalVoronoiRegionData::operator=(const SphericalVoronoiRegionData& Other)
	{
		if (this == &Other) return *this;
		Loaded.store(Other.Loaded);
		mData = Other.mData;
		offset = Other.offset;
		prefix = Other.prefix;
		coarse_prefix = Other.coarse_prefix;
		defineArray = Other.defineArray;
		defineBaseArray = Other.defineBaseArray;
		coarse_adjacent_regions = Other.coarse_adjacent_regions;
		coarse_centers = Other.coarse_centers;
		
		return *this;
	}

	bool SphericalVoronoiRegionData::IsFineLevelDataLegal() const
	{
		String ErrorHead = String("-error-\t ") + __FUNCTION__;
		if (!prefix.empty())
		{
			ErrorHead = ErrorHead + ": [" + prefix + "] ";
			if (mData.isNull() || mData->getName() != Name(prefix))
			{
				LogManager::instance()->logMessage(
					ErrorHead + "unknown error.", LML_CRITICAL);
				return false;
			}
			if (!mData->mData.Params.isParsed)
			{
				LogManager::instance()->logMessage(
					ErrorHead + "cannot parse the prefix.", LML_CRITICAL);
				return false;
			}

			if (!mData->mData.mbDataMaps)
			{
				LogManager::instance()->logMessage(
					ErrorHead + "cannot load maps.", LML_CRITICAL);
			}

			if (!mData->mData.mbAdjacents)
			{
				LogManager::instance()->logMessage(
					ErrorHead + "cannot load adjacencies.", LML_CRITICAL);
			}

			if (!mData->mData.mbPoints)
			{
				LogManager::instance()->logMessage(
					ErrorHead + "cannot load centers.", LML_CRITICAL);
			}

			bool isMatch = (!defineArray.empty() && defineArray.size() == mData->mData.Params.Total());
			if (!isMatch)
			{
				LogManager::instance()->logMessage(
					ErrorHead + "length of defineArray {" + std::to_string(defineArray.size())
					+ "} is not equal to {" + std::to_string(mData->mData.Params.Total()) + "}.", LML_CRITICAL);
				return false;
			}

			return mData->mData.mbDataMaps && mData->mData.mbAdjacents && mData->mData.mbPoints && isMatch;
		}

		LogManager::instance()->logMessage(
			ErrorHead + ": empty prefix.", LML_CRITICAL);

		return false;
	}

	SphericalVoronoiRegionData::ErrorType SphericalVoronoiRegionData::IsCoarseLevelDataLegal() const
	{
		assert(IsFineLevelDataLegal());
		String ErrorHead = String("-error-\t ") + __FUNCTION__;

		bool ParentLegal = !defineArray.empty() && defineArray[0].Parent != -1;

		if (!ParentLegal)
		{
			LogManager::instance()->logMessage(
				ErrorHead + ": parents are invalid.", LML_CRITICAL);
		}

		bool isAdjacencies = !coarse_adjacent_regions.empty();

		if (!isAdjacencies)
		{
			LogManager::instance()->logMessage(
				ErrorHead + ": empty coarse level adjacency.", LML_CRITICAL);
		}

		bool isArray = !defineBaseArray.empty();

		if (!isArray)
		{
			LogManager::instance()->logMessage(
				ErrorHead + ": empty coarse level define array.", LML_CRITICAL);
		} 

		if (ParentLegal && isArray && isAdjacencies)
		{
			return LEGAL;
		}

		if ((!ParentLegal && (isAdjacencies || isArray))
			|| (!isAdjacencies && isArray))
		{
			return  INCOMPATIBLE;
		}

		return ILLEGAL;
	}

	bool SphericalVoronoiRegionData::SelectCoarseLevelCenter()
	{
		assert(IsFineLevelDataLegal());
		assert(IsCoarseLevelDataLegal() == SphericalVoronoiRegionData::LEGAL);
		if (!coarse_centers.empty()) return false;
		if (defineBaseArray.size() == 1u) return true; // There is no center when there exist only one region.
		const auto& centers = mData->mData.mPoints;
		coarse_centers.resize(defineBaseArray.size());
		for (int id = 0; id < (int)coarse_centers.size(); ++id)
		{
			for (int i = 0; i < (int)defineArray.size(); ++i)
			{
				if (defineArray[i].Parent == id)
				{
					coarse_centers[id] = centers[i];
					break;
				}
			}
		}
		return true;
	}
#ifdef ECHO_EDITOR
	bool SphericalVoronoiRegionData::GenerateFineLevelData(const PlanetRegionParameter& ParamsRef)
	{
		const String ErrorHead = String("--error--\t ") + __FUNCTION__ + ": ";
		if (!ParamsRef.isLegalRef())
		{
			LogManager::instance()->logMessage(ErrorHead + ParamsRef.ToString() + " is not a legal ref parameter.", LML_CRITICAL);
			return false;
		}
		std::vector<String> PrefixRef;
		PlanetRegionRelated::GetPrefixWithRef(ParamsRef, PrefixRef);
		auto SelectFineLevelPrefix = [&PrefixRef, this]()
			{
				if (PrefixRef.empty()) return String();
				if (PrefixRef.size() == 1u) return PrefixRef[0];
				int n = (int)PrefixRef.size();
				int id = Math::RangeRandom(0, n);
				if (prefix == PrefixRef[id])
				{
					id = (id + 1) % n;
				}
				return PrefixRef[id];
			};
		prefix = SelectFineLevelPrefix();

		try
		{
			mData = PlanetRegionManager::instance()->prepare(Name(prefix));
		}
		catch (...)
		{
			LogManager::instance()->logMessage(ErrorHead + "An exception is generated when preparing Region {" + prefix + "}.", LML_CRITICAL);
			mData.setNull();
		}
		defineArray.clear();
		if(!mData.isNull() && mData->mData.Params.isParsed)
		{
			defineArray.resize(mData->mData.Params.Total());

			if(mData->mData.Params.z == 1u)
			{
				offset = Vector2(Math::UnitRandom(), Math::UnitRandom());
			}
		}

		

		return IsFineLevelDataLegal();
	}
	bool SphericalVoronoiRegionData::GenerateCoarseLevelData()
	{
		assert(IsFineLevelDataLegal());
		assert(IsCoarseLevelDataLegal() == SphericalVoronoiRegionData::COMPATIBLE);

		bool ParentLegal = !defineArray.empty() && defineArray[0].Parent != -1;

		if (ParentLegal || (ParentLegal = GenerateHierachicalStructure()))
		{
			bool isAdjacencies = !coarse_adjacent_regions.empty();

			if (isAdjacencies || (isAdjacencies = GenerateCoarseLevelAdjacency()))
			{
				if (defineBaseArray.empty())
				{
					defineBaseArray.resize(coarse_adjacent_regions.size());
				}
				return true;
			}
		}
		return false;
	}

	bool SphericalVoronoiRegionData::computeRegionParentIdPlane(
		const std::string& prefix, const String& ErrorHead, const std::vector<std::vector<int>>& FineAdjacency,
		std::vector<int>& parentArray, const int MaxSelect)
	{
		std::unordered_set<uint8> Done;
		int Parent = 0;
		for (int id = 0; id < (int)FineAdjacency.size(); ++id)
		{
			int num = 0;
			if (Done.contains(id)) continue;
			{
				if (parentArray[id] != -1)
				{
					LogManager::instance()->logMessage(ErrorHead + "Conflict Parent? {" + prefix + "}.");
					return false;
				}
				parentArray[id] = Parent;
				Done.insert(id);
				num++;
			}
			const auto& adjacency = FineAdjacency[id];
			for (auto i : adjacency)
			{
				if (Done.contains(i)) continue;
				{
					if (parentArray[i] != -1)
					{
						LogManager::instance()->logMessage(ErrorHead + "Conflict Parent? {" + prefix + "}.");
						return false;
					}
					parentArray[i] = Parent;
					Done.insert(i);
					num++;
				}
				if (num == MaxSelect)break;
			}
			Parent++;
		}
		for (const auto& define : parentArray)
		{
			if (define == -1) return false;
		}
		return true;
	}

	bool SphericalVoronoiRegionData::GenerateHierachicalStructure()
	{
		if (!IsFineLevelDataLegal()) return false;
		if (defineArray.empty() || defineArray[0].Parent != -1) return false;
		if (!coarse_adjacent_regions.empty() || !defineBaseArray.empty()) return false;

		const String ErrorHead = String("--error--\t ") + __FUNCTION__ + ": ";

		coarse_prefix = "";
		const auto& Data = mData->mData;
		if (Data.Params.z == 1u) // Torus
		{
			
			if (Data.Params.x == 1u && Data.Params.y == 1u) return false;
#if 0
			int x = (int)Data.Params.x, y = (int)Data.Params.y;
			x = std::max(x - 1, 1), y = std::max(y - 1, 1);
			float r = static_cast<float>(Data.Params.Total()) / (x * y * Data.Params.z);
			const int MaxSelect = static_cast<int>(ceil(r));

			if (MaxSelect == 1)
			{
				
				for (auto& define : defineArray)
				{
					define.Parent = 0;
				}
				return true;
			}
#else
			const int MaxSelect = 4;
#endif

			std::vector<int> parentArray;
			parentArray.reserve(defineArray.size());
			for (const auto & define : defineArray)
			{
				parentArray.push_back(define.Parent);
			}
			const bool success =  computeRegionParentIdPlane(prefix, ErrorHead, Data.mAdjacents, parentArray, MaxSelect);
			for (int i = 0; i < defineArray.size(); ++i)
			{
				defineArray[i].Parent = parentArray[i];
			}
			return success;
		}
		else if(Data.Params.z == 6) //Sphere
		{
			if (Data.Params.x == 1) // In this case, there is only one coarse region.
			{
				for (auto& define : defineArray)
				{
					define.Parent = 0;
				}
				return true;
			}
			PlanetRegionParameter ParamsRef;
			ParamsRef.x = Data.Params.x - 1;
			ParamsRef.y = Data.Params.y - 1;
			ParamsRef.z = Data.Params.z;
			std::vector<String> PrefixRef;
			PlanetRegionRelated::GetPrefixWithRef(ParamsRef, PrefixRef);
			auto SelectCoarseLevelPrefix = [&PrefixRef, this]()
				{
					if (PrefixRef.empty()) return String();
					int n = (int)PrefixRef.size();
					int id = Math::RangeRandom(0, n);
					return PrefixRef[id];
				};
			coarse_prefix = SelectCoarseLevelPrefix();
			PlanetRegionPtr CoarseData;
			try
			{
				CoarseData = PlanetRegionManager::instance()->prepare(Name(coarse_prefix));
			}
			catch (...)
			{
				LogManager::instance()->logMessage(ErrorHead + "An exception is generated when preparing Coarse Region {" + coarse_prefix + "}.", LML_CRITICAL);
				CoarseData.setNull();
			}

			if (!CoarseData.isNull() && CoarseData->mData.mbDataMaps)
			{
				const auto& FinePoints = Data.mPoints;
				auto& CoarseMaps = CoarseData->mData.mDataMaps;
				for (int id = 0; id < (int)defineArray.size(); ++id)
				{
					defineArray[id].Parent = SphericalTerrain::computeSphericalRegionParentId(FinePoints[id], CoarseMaps);
				}
				for (const auto& define : defineArray)
				{
					if (define.Parent == -1) return false;
				}
				return true;
			}

		}
		return false;
	}
	bool SphericalVoronoiRegionData::GenerateCoarseLevelAdjacency()
	{
		if (!IsFineLevelDataLegal()) return false;
		if (defineArray.empty() || defineArray[0].Parent == -1) return false;
		if (!coarse_adjacent_regions.empty() || !defineBaseArray.empty()) return false;

		const int n = (int)defineArray.size();

		int m = -1;
		for (int i = 0; i < n; ++i)
		{
			const auto& define = defineArray[i];
			if (define.Parent > m) m = define.Parent;
		}
		m = m + 1;

		std::vector<std::vector<int>> temp;
		temp.resize(m);

		for (int i = 0; i < n; ++i)
		{
			const auto& define = defineArray[i];
			const auto& adjacency = mData->mData.mAdjacents[i];
			for (auto e : adjacency)
			{
				temp[define.Parent].push_back(e);
			}
		}
		for (int i = 0; i < (int)temp.size(); ++i)
		{
			for (auto& e : temp[i])
			{
				e = defineArray[e].Parent;
			}
			std::vector<int> adjs;
			std::sort(temp[i].begin(), temp[i].end());
			for (const auto& e : temp[i])
			{
				if (e == i) continue;
				if (adjs.empty() || adjs.back() != e)
				{
					adjs.push_back(e);
				}
			}
			std::swap(temp[i], adjs);
		}
		// Final check
		for (int i = 0; i < temp.size(); ++i)
		{
			const auto& itemp = temp[i];
			std::unordered_set<int> duplicate;
			for (const auto e : itemp)
			{
				if (e >= temp.size()) return false;
				const auto Iter = std::find(temp[e].begin(), temp[e].end(), i);
				if (Iter == temp[e].end()) return false;
				duplicate.insert(e);
			}
			if (duplicate.size() != itemp.size()) return false;
		}
		std::swap(coarse_adjacent_regions, temp);
		return true;
	}
#endif
	TerrainGeneratorLibrary::TerrainGeneratorLibrary() {}
	TerrainGeneratorLibrary::~TerrainGeneratorLibrary() {
		clear();
	}
	namespace SphericalPlanetGeometryType {

		bool DataParse(PlanetGeometryTypeLibrary& types, const char* data) {
			cJSON* rootNode = cJSON_Parse(data);
			if (rootNode == nullptr) {
				return false;
			}
			types.types.clear();
			cJSON* array_node = cJSON_GetObjectItem(rootNode, "type_name");
			if (array_node && array_node->type == cJSON_Array) {
				for (JSNODE node = array_node->child; node; node = node->next)
				{
					PlanetGeometryType newType;
					ReadJSNode(node, "type", newType.type);
					ReadJSNode(node, "file", newType.file);
					types.types.push_back(newType);
				}
				return true;
			}
			return false;
		}

		template<typename T>
		inline bool Read(T& v, const char*& p, const char* end) {
			if ((size_t)(end - p) < (size_t)sizeof(T)) return false;
			std::memcpy((void*)&v, p, sizeof(T));
			p += sizeof(T);
			return true;
		}

		template<typename T>
		inline bool Read(T* v, uint32 _nCount, const char*& p, const char* end) {
			if ((size_t)(end - p) < (size_t)(sizeof(T) * _nCount)) return false;
			std::memcpy((void*)v, p, sizeof(T) * _nCount);
			p += sizeof(T) * _nCount;
			return true;
		}

		inline bool ReadString(std::string& v, const char*& p, const char* end) {
			unsigned int len = 0;
			if (Read(len, p, end) && len)
			{
				if (Read((const char*)v.data(), len, p, end)) {
					return true;
				}
				else {
					v.clear();
				}
			}
			return false;
		}

		inline bool ReadVector3(Vector3& v, const char*& p, const char* end) {
			return Read(v.ptr(), 3, p, end);
		}

		inline bool ReadQuaternion(Quaternion& v, const char*& p, const char* end) {
			return Read(v.ptr(), 4, p, end);
		}

		const uint32 PLANET_GEOMETRY_HEADER_CHUNK_ID = 'P' | ('T' << 8) | ('G' << 16) | ('Y' << 24);
		bool DataParse(PlanetGeometry& geom, const char* _pData, size_t _nData) {
			if (_pData == nullptr) return false;
			const char* DATA_PTR = (const char*)_pData;
			const char* END_DATA_PTR = DATA_PTR + _nData;

			uint32 headerID = 0;
			uint32 version = 0;
			float vertexScale = 1.0f;
			uint32 nbVertex = 0;
			uint32 nbTriangle = 0;

			if (!Read(headerID, DATA_PTR, END_DATA_PTR)) return false;

			if (headerID != PLANET_GEOMETRY_HEADER_CHUNK_ID) {
				return false;
			}

			if (!Read(version, DATA_PTR, END_DATA_PTR)) return false;
			if (!Read(vertexScale, DATA_PTR, END_DATA_PTR)) return false;
			if (!Read(nbVertex, DATA_PTR, END_DATA_PTR)) return false;
			if (!Read(nbTriangle, DATA_PTR, END_DATA_PTR)) return false;

			if (nbVertex == 0 || nbTriangle == 0) return false;

			geom.vertexs.resize(nbVertex);
			if (!Read(geom.vertexs.data(), nbVertex, DATA_PTR, END_DATA_PTR)) return false;
			geom.triangles.resize(nbTriangle);
			if (!Read(geom.triangles.data(), nbTriangle, DATA_PTR, END_DATA_PTR)) return false;

			return true;
		}

	}

	bool TerrainGeneratorLibrary::load()
	{
		bool result = false;

		cJSON* rootNode = readJSONFile("echo/biome_terrain/geometryType/TerrainGeometry.library");
		if (rootNode)
		{
			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "gemp");

			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);

				cJSON* typeNode = 0;
				clear();

				for (int type_index = 0; type_index < typeCount; type_index++) {
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						uint32 gempType;
						String gempName;
						String gempPath;
						String meshPath;
						String gravityPath;
						String borderPath;
						bool bloadGemp = true;

						ReadJSNode(typeNode, "type", gempType);
						ReadJSNode(typeNode, "name", gempName);
						ReadJSNode(typeNode, "file", gempPath);
						ReadJSNode(typeNode, "load", bloadGemp);
						ReadJSNode(typeNode, "mesh", meshPath);
						ReadJSNode(typeNode, "gravity", gravityPath);
						ReadJSNode(typeNode, "border", borderPath);

						if (bloadGemp && !mDataMaps.contains(gempType)) {
							std::shared_ptr<TerrainGenerator> pGemp = std::make_shared<TerrainGenerator>(gempType, gempPath, meshPath, gravityPath, borderPath);
							mTypes.insert(std::make_pair(gempType, gempName));
							mNames.insert(std::make_pair(gempType, gempPath));
							mDataMaps.insert(std::make_pair(gempType, pGemp));
						}
					}
				}
				result = true;
			}

            cJSON_Delete(rootNode);
		}
		return (result);
	}

	TerrainGenerator* TerrainGeneratorLibrary::get(uint32 type) const
	{
		auto iter = mDataMaps.find(type);
		if (iter != mDataMaps.end()) {
			return iter->second.get();
		}
		return nullptr;
	}
	std::shared_ptr<TerrainGenerator> TerrainGeneratorLibrary::add(uint32 type) const
	{
		auto iter = mDataMaps.find(type);
		if (iter != mDataMaps.end()) {
			if (!iter->second->isSuccess()) {
				iter->second->reload();
			}
			return iter->second;
		}
		return std::shared_ptr<TerrainGenerator>(nullptr);
	}
	void TerrainGeneratorLibrary::remove(std::shared_ptr<TerrainGenerator>& ptr) const {
		if (ptr == nullptr) return;
		if (ptr.use_count() == 2) {
			ptr->unload();
		}
		ptr = nullptr;
	}

	void TerrainGeneratorLibrary::clear() {
		for (auto& iter : mDataMaps) {
			iter.second = nullptr;
		}
		mDataMaps.clear();
		mTypes.clear();
	}

	void TerrainGeneratorLibrary::save()
	{
		//cJSON* rootNode = cJSON_CreateObject();

		// cJSON* typeArrayNode = cJSON_CreateArray();
		
		//saveJSONFile("echo/biome_terrain/geometryType/TerrainGeometry.library", rootNode);
		//cJSON_Delete(rootNode);
	}

	void SphericalTerrainAtmosData::load() {
		bool loadSuccess = true;
		loadSuccess &= loadMesh();
		loadSuccess &= loadMaterial();
		mLoaded = loadSuccess;
	}

	bool SphericalTerrainAtmosData::loadMesh() {
		try {
			mAtmosMesh = MeshMgr::instance()->load(Name("echo/biome_terrain/Atmosphere/atmosphere_mesh.mesh"));

			if (mAtmosMesh.isNull()) {
				return false;
			}

			SubMesh* mSubMesh = mAtmosMesh->getSubMesh(0);
			if (!mSubMesh)
				return false;
			
			VertexData* vertexData = mSubMesh->getVertexData();
			IndexData* indexData = mSubMesh->getIndexData();
			if (vertexData == nullptr || indexData == nullptr)
				return false;

			return true;
		}
		catch (...) {
			LogManager::instance()->logMessage("SphericalAtmosphere:: Failed to load atmosphere model");
			return false;
		}
	}

	bool SphericalTerrainAtmosData::loadMaterial() {

		// load shader
		{
			String vsShader, psShader;
			ShaderContentManager::instance()->GetOrLoadShaderContent(Name("SphericalAtmosphere_VS.txt"), vsShader);
			ShaderContentManager::instance()->GetOrLoadShaderContent(Name("SphericalAtmosphereApprox_PS.txt"), psShader);

			if (vsShader.empty() || psShader.empty())
				return false;
			
			mAtmosMaterial = MaterialManager::instance()->createManual();
			mAtmosMaterial->setShaderSrcCode(vsShader, psShader);
			mAtmosMaterial->setShaderName("SphericalAtmosphere_VS.txt", "SphericalAtmosphereApprox_PS.txt");
		}

		// render state
		{
			ECHO_RENDER_STATE& state = mAtmosMaterial->m_desc.RenderState;
			state.CullMode = ECHO_CULL_BACK;
			state.DrawMode = ECHO_TRIANGLE_LIST;

			state.DepthTestEnable = ECHO_TRUE;
			state.DepthFunc = ECHO_CMP_LESSEQUAL;
			state.DepthWriteEnable = ECHO_FALSE;
			state.StencilEnable = ECHO_FALSE;

			state.BlendEnable = ECHO_TRUE;
			state.BlendOp = ECHO_BLEND_OP_ADD;
			state.BlendSrc = ECHO_BLEND_ONE;
			state.BlendDst = ECHO_BLEND_INVSRCALPHA;

			// 第一人称
			state.StencilEnable = ECHO_TRUE;
			state.StencilRef = TERRAIN_STENCIL_MASK;
			state.StencilMask = TERRAIN_STENCIL_MASK;
			state.StencilWriteMask = 0xff ^ FIRST_PERSON_STENCIL_MASK;
			state.StencilFunc = ECHO_CMP_GREATEREQUAL;
		}

		// IA
		{
			ECHO_IA_DESC& atmosphereIA = mAtmosMaterial->m_desc.IADesc;
			atmosphereIA.ElementArray[0].nStream = 0u;
			atmosphereIA.ElementArray[0].nOffset = 0u;
			atmosphereIA.ElementArray[0].eType = ECHO_FLOAT3;
			atmosphereIA.ElementArray[0].nIndex = 0u;

			atmosphereIA.nElementCount = 1;

			atmosphereIA.StreamArray[0].nStride = VertexType::typeSize(ECHO_FLOAT3);
			atmosphereIA.StreamArray[0].bPreInstance = ECHO_FALSE;
			atmosphereIA.StreamArray[0].nRate = 1;
			atmosphereIA.nStreamCount = 1;
		}

		mAtmosMaterial->createPass(NormalPass);
		mAtmosMaterial->loadInfo();
		return true;
	}

    void SphericalTerrainResource::requestBiomeTexture(SphericalTerrainResourcePtr selfSmartPtr,
                                                       bool sync)
    {
        if(!m_loadBiomeTexture &&
           m_requireBiomeTextureCount == 0)
        {
            NameValuePairList params;
            String biomeTexResName =  mName.toString();
            params["parentResName"] = biomeTexResName;
            auto result  = PlanetTextureResourceManager::instance()->
                createOrRetrieve(mName,
                                 false,
                                 0,
                                 &params);

            m_planetTexRes = result.first;
            if(!(m_planetTexRes->isPrepared() || m_planetTexRes->isLoaded()) &&
               m_requestID == 0)
            {
                if(sync)
                {
                    m_planetTexRes->load();
                }
                else
                {
                    PlanetTextureResourceManager::instance()->
                        asyncLoadRes(m_planetTexRes,
                                     selfSmartPtr);
                }
            }
        }
        m_requireBiomeTextureCount++;

        //logToConsole("request biome tex count:" + std::to_string(m_requireBiomeTextureCount) + "\n");
    }
    
    void SphericalTerrainResource::freeBiomeTexture()
    {
		m_requireBiomeTextureCount--;
        if(m_requireBiomeTextureCount == 0)
        {
            //IMPORTANT:Avoid resource had prepared and then loading with unload-state.
            // go back to prepare again.
            if(m_loadBiomeTexture)
            {
                m_planetTexRes.setNull();
                m_loadBiomeTexture = false;
            }
            else
            {
                //IMPORTANT:Resource is loading. so just free the reference.
                m_planetTexRes.setNull();

                //NOTE: abort async task.
                if(m_requestID != 0)
                {
                    PlanetTextureResourceManager::instance()->abortRequest(m_requestID);
                    m_requestID = 0;
                }
            }
        }
        
        //logToConsole("free biome tex count:" + std::to_string(m_requireBiomeTextureCount) + "\n");
    }
    
    bool SphericalTerrainResource::isBiomeTextureReady()
    {
        bool result = false;
        if(!m_planetTexRes.isNull() &&
           m_loadBiomeTexture)
        {
            result = true;
        }
        return (result); 
    }

    // Planet texture resource & manager
    PlanetTextureResource::PlanetTextureResource(ResourceManagerBase* creator,
                                                 const Name& name,
                                                 bool isManual,
                                                 ManualResourceLoader* loader,
                                                 SphericalTerrainResource* planetRes)
            : ResourceBase(creator, name, isManual, loader)
    {
        m_planetRes = planetRes;
    }

    PlanetTextureResource::~PlanetTextureResource()
    {
        unload();
    }

    void PlanetTextureResource::prepareImpl(void)
    {
        assert(!m_planetRes.isNull() && "Parent planet resource is null! Are you forget to set it?");
        pBiomeGPUData_Cache = new BiomeGPUData_Cache;
        SphericalTerrainResourceManager::fillBiomeDiffuseTextureArray_Async(m_planetRes->composition,
                                                                            *pBiomeGPUData_Cache);


        
        if(m_planetRes->geometry.topology == PTS_Sphere)
        {
            const auto& idMaps = m_planetRes->m_matIndexMaps;

            int idMapSize = SphericalTerrain::BiomeTextureResolution * SphericalTerrain::BiomeTextureResolution * sizeof(uint8_t);
            bool matValid = true;
            for(auto& map : idMaps)
            {
                if(map.byteSize != idMapSize)
                {
                    matValid = false;
                }
            }
            m_matMapValid = matValid;
            
            if (m_matMapValid)
            {
                for (int i = 0;
                     i < 6;
                     ++i)
                {
                    assert(m_planetRes->m_matIndexMaps[i].pixels);
                    const auto& map = m_planetRes->m_matIndexMaps[i];
                    Bitmap& realIDMap = m_matIDMaps[i];
                    realIDMap = map;
                    
                    int mapSize = realIDMap.pitch * realIDMap.height;
                    realIDMap.pixels = malloc(mapSize);

                    uint8_t* originPixel = (uint8_t*)map.pixels;
                    uint8_t* pixel = (uint8_t*)realIDMap.pixels;
                    for(int i = 0;
                        i < mapSize;
                        i++)
                    {
                        *pixel = *originPixel & SphericalTerrain::MaterialTypeMask;
                        pixel++;
                        originPixel++;
                    }
                }    
            }
        }
        else if(m_planetRes->geometry.topology == PTS_Torus)
        {
            int biomeTextureWidth  = SphericalTerrain::BiomeCylinderTextureWidth;
            int biomeTextureHeight = std::clamp(static_cast<int>(static_cast<float>(biomeTextureWidth) * m_planetRes->geometry.torus.relativeMinorRadius),
                                              1, biomeTextureWidth);
            int idMapSize = biomeTextureWidth * biomeTextureHeight * sizeof(uint8_t);
            m_matMapValid = m_planetRes->m_matIndexMaps[0].byteSize == idMapSize;
            
            if(m_matMapValid)
            {
                const auto& map = m_planetRes->m_matIndexMaps[0];
                Bitmap& realIDMap = m_matIDMaps[0];
                realIDMap.width = biomeTextureWidth;
                realIDMap.height = biomeTextureHeight;
                realIDMap.bytesPerPixel = sizeof(uint8_t);
                realIDMap.pitch = realIDMap.width * realIDMap.bytesPerPixel;

                int mapSize = realIDMap.pitch * realIDMap.height;
                realIDMap.pixels = malloc(mapSize);

                uint8_t* originPixel = (uint8_t*)map.pixels;
                uint8_t* pixel = (uint8_t*)realIDMap.pixels;
                for(int i = 0;
                    i < mapSize;
                    i++)
                {
                    *pixel = *originPixel & SphericalTerrain::MaterialTypeMask;
                    pixel++;
                    originPixel++;
                }
            }
        }
        else
        {
            assert(false&& "Unknown planet topology type!");
        }
    }

    void PlanetTextureResource::unprepareImpl(void)
    {
        if (pBiomeGPUData_Cache) {
			pBiomeGPUData_Cache->cpuData.freeMemory();
			SAFE_DELETE(pBiomeGPUData_Cache);
		}
    }
    
    void PlanetTextureResource::loadImpl(void)
    {
        if (pBiomeGPUData_Cache)
        {
			SphericalTerrainResourceManager::fillBiomeDiffuseTextureArray_Sync(*pBiomeGPUData_Cache,
                                                                               m_biomeGPUData);
			SAFE_DELETE(pBiomeGPUData_Cache);
        }
        else
        {
            LogManager::instance()->logMessage("-error-\t " + 
                                               getName().toString() + " biome's texture load failed!.", LML_CRITICAL);
        }

        if(m_planetRes->geometry.topology == PTS_Sphere)
        {
            if (m_matMapValid)
            {
                for (int i = 0; i < 6; ++i)
                {
                    auto& matIdx = m_matIDMaps[i];
                    m_biomeGPUData.matIDTex[i] =
                        TextureManager::instance()->createManual(
                            TEX_TYPE_2D,
                            matIdx.width,
                            matIdx.height,
                            ECHO_FMT_R8UI,
                            matIdx.pixels,
                            matIdx.pitch * matIdx.height);
					m_biomeGPUData.matIDTex[i]->load();
                    matIdx.freeMemory();
                }
            }
            else
            {
                for (int i = 0; i < 6; ++i)
                {
                    m_biomeGPUData.matIDTex[i] = GetDummyR8Ui(true);	// dummy R8UI texture to silence D3D ERROR
					m_biomeGPUData.matIDTex[i]->load();
                }
            }
        }
        else if(m_planetRes->geometry.topology == PTS_Torus)
        {
            if (m_matMapValid)
            {
                const auto& matIdx = m_matIDMaps[0];
                m_biomeGPUData.matIDTex[0] =
                    TextureManager::instance()->createManual(
                        TEX_TYPE_2D,
                        matIdx.width,
                        matIdx.height,
                        ECHO_FMT_R8UI,
                        matIdx.pixels,
                        matIdx.pitch * matIdx.height);
				m_biomeGPUData.matIDTex[0]->load();
            }
            else
            {
                m_biomeGPUData.matIDTex[0] = GetDummyR8Ui(true);	// dummy R8UI texture to silence D3D ERROR
				m_biomeGPUData.matIDTex[0]->load();
            }
        }
        else
        {
            assert(false&& "Unknown planet topology type!");
        }

        if(!m_planetRes.isNull())
        {
            //IMPORTANT:Parent resource didn't free me.
            if(m_planetRes->m_planetTexRes.get() == this)
            {
                m_planetRes->m_loadBiomeTexture = true;
            }
            
			m_planetRes->m_requestID = 0;
            m_planetRes.setNull();
        }
    }

    void PlanetTextureResource::unloadImpl(void)
    {
        m_planetRes.setNull();
	}

    size_t PlanetTextureResource::calcMemUsage(void) const
    {
        size_t result = 0;
        if(!m_biomeGPUData.diffuseTexArray.isNull())
            result += m_biomeGPUData.diffuseTexArray->calcMemUsage();

        if(!m_biomeGPUData.normalTexArray.isNull())
            result += m_biomeGPUData.normalTexArray->calcMemUsage();

        for(int i = 0 ;
            i < 6;
            i++)
        {
            if(!m_biomeGPUData.matIDTex[i].isNull())
                m_biomeGPUData.matIDTex[i]->calcMemUsage();
        }
        
        result += sizeof(PlanetTextureResource);
        return (result);
    }
    
	PlanetTextureResourceManager::PlanetTextureResourceManager()
            :ResourceManagerBase()
	{
        EchoZoneScoped;

		mResourceType = "EchoPlanetTexture";
		ResourceManagerFactory::instance()->registerResourceManager(mResourceType,
                                                                    this);

        mWorkQueue = Root::instance()->getWorkQueue();
		mWorkQueueChannel = mWorkQueue->getChannel("PlanetBiomeTextureRes");
		mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->addResponseHandler(mWorkQueueChannel, this);
	}

	PlanetTextureResourceManager::~PlanetTextureResourceManager()
	{
        EchoZoneScoped;

		ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);

        m_requestLoadResMap.clear();
        mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
		mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);

        //NOTE:Free global dummy texture.
        auto globalDummyTex = GetDummyR8Ui(false);
        if(!globalDummyTex.isNull())
        {
            globalDummyTex.setNull();
        }
    }

	Echo::ResourceBase* PlanetTextureResourceManager::createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams)
	{
        EchoZoneScoped;

		PlanetTextureResource* ret = 0;
        if(createParams && !createParams->empty())
        {
            //NOTE:Get parent plaent resource.
            auto findResult = createParams->find("parentResName");
            if(findResult != createParams->end())
            {
                Name parentResName = Name(findResult->second.c_str());
                ResourceBasePtr planetRes = SphericalTerrainResourceManager::instance()->getByName(parentResName);
            
                ret = new PlanetTextureResource(this,
                                                name,
                                                isManual,
                                                loader,
                                                (SphericalTerrainResource*)planetRes.get());
            }
        }
        
		return ret;
	}

    std::ostream&
    operator << (std::ostream &o,
                 const PlanetTextureResourceLoadRequest &r)
    {
        EchoZoneScoped;
        return o;
    }

    std::ostream & operator << (std::ostream &o,
                                       const PlanetTextureResourcePtr &r)
    {
        EchoZoneScoped;
        return o;   
    }
    
    WorkQueue::Response*
    PlanetTextureResourceManager::handleRequest(const WorkQueue::Request* req,
                                                const WorkQueue* srcQ)
    {
        EchoZoneScoped;

        WorkQueue::Response *response = 0;
        const PlanetTextureResourceLoadRequest* loadRequest = any_cast<PlanetTextureResourceLoadRequest>(&req->getData());
        if (!req->getAborted())
        {
            loadRequest->res->prepare();
            response = new WorkQueue::Response(req, true, Any(*loadRequest), "", loadRequest->res->getName().toString());
        }

        return response;
    }

    void
    PlanetTextureResourceManager::handleResponse(const WorkQueue::Response* res,
                                                 const WorkQueue* srcQ)
    {
        EchoZoneScoped;

        if (!res->succeeded())
			return;

        const WorkQueue::Request *req = res->getRequest();
        const PlanetTextureResourceLoadRequest *pReq = any_cast<PlanetTextureResourceLoadRequest>(&req->getData());
        if (!req->getAborted())
        {
            pReq->res->load();
            m_requestLoadResMap.erase(req->getID());
        }   
    }

	void PlanetTextureResourceManager::OutPutPrepareResourceLog(DataStreamPtr& out)
	{
		std::vector<WorkQueue::Request*> requestLists;
		mWorkQueue->getRequestList(mWorkQueueChannel, requestLists);

		stringstream ss;
		ss << "WorkQueueChannel : " << "PlanetBiomeTextureRes" << "\n";
		for (const auto& req : requestLists)
		{
			if (req == nullptr || req->getAborted()) continue;

			Any pData = req->getData();

			if (pData.isEmpty()) continue;

			const PlanetTextureResourceLoadRequest* loadRequest = any_cast<PlanetTextureResourceLoadRequest>(&pData);
			ss << "Request Type : Texture\t" << "Resource Name : " << loadRequest->res->getName() << "\n";
		}
		out->write(ss.str().c_str(), ss.str().size());
	}

    void PlanetTextureResourceManager::abortRequest(WorkQueue::RequestID reqID)
    {
        if(m_requestLoadResMap.find(reqID) != m_requestLoadResMap.end())
        {
            mWorkQueue->abortRequest(reqID);
            m_requestLoadResMap.erase(reqID);
        }
    }
        
    WorkQueue::RequestID
    PlanetTextureResourceManager::asyncLoadRes(PlanetTextureResourcePtr res,
                                               SphericalTerrainResourcePtr parentRes)
    {
        EchoZoneScoped;

        WorkQueue::RequestID result = 0;
        if(!res->isLoaded())
        {
            PlanetTextureResourceLoadRequest request = {res, parentRes};
            WorkQueue::RequestID resID = mWorkQueue->addRequest(mWorkQueueChannel, 0, Any(request));
            m_requestLoadResMap[resID] = res;
            result = resID;

            res->m_planetRes = parentRes;
        }
        
        return (result);
    }

    TexturePtr
    SphericalTerrainResourceManager::getPointTexture()
    {
        if(m_pointTexture.isNull())
        {
            m_pointTexture = TextureManager::instance()->loadTexture(Name("echo/biome_terrain/Textures/point.dds"));
        }
        return m_pointTexture;
    }
	bool PlanetCloudLibrary::load()
	{
		bool result = true;


		{
			cJSON* cfgNode = readJSONFile("biome_terrain/library.cfg");
			if (cfgNode)
			{
				String newLibraryPath;
				bool useCustomPath = false;
				ReadJSNode(cfgNode, "enableLibPath", useCustomPath);
				if (ReadJSNode(cfgNode, "cloudLibraryPath", newLibraryPath))
				{
					if (!newLibraryPath.empty() && useCustomPath)
					{
						m_FilePath = newLibraryPath;
					}
				}
			}
		}


		cJSON* rootNode = readJSONFile(m_FilePath.c_str());
		if (rootNode)
		{
			cJSON* typeArrayNode = cJSON_GetObjectItem(rootNode, "type");

			if (typeArrayNode && typeArrayNode->type == cJSON_Array)
			{
				int typeCount = cJSON_GetArraySize(typeArrayNode);
				if (typeCount > m_MaxCount)
				{
					LogManager::instance()->logMessage("-error-\t Planet Cloud library count is out of max limit " + std::to_string(m_MaxCount) + "!",
						LML_CRITICAL);
				}

				cJSON* typeNode = 0;
				if (!m_AllClouds.empty())
				{
					m_AllClouds.clear();
				}

				for (int type_index = 0; type_index < typeCount;++type_index)
				{
					PlanetCloudTemplateData temp;
					typeNode = cJSON_GetArrayItem(typeArrayNode, type_index);
					if (typeNode)
					{
						bool nodeParseSuccess = true;
						//TODO(yang):Only this on editor mode.
						//nodeParseSuccess &= ReadJSNode(typeNode, "name", temp.name);

						nodeParseSuccess &= ReadJSNode(typeNode, "CloudUVScale", temp.m_UVScale);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudFlowSpeed", temp.m_flowSpeed);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudHDRFactor", temp.m_hdr);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudMetallic", temp.m_metallic);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudRoughness", temp.m_roughness);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudAmbientFactor", temp.m_ambientFactor);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudNoiseOffsetScale", temp.m_noiseOffsetScale);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudTexture", temp.m_DiffusePath);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudTemplateName", temp.m_TemplateName);

						nodeParseSuccess &= ReadJSNode(typeNode, "CloudPitch", temp.m_pitch);
						nodeParseSuccess &= ReadJSNode(typeNode, "CloudYaw", temp.m_yaw);

						if (!nodeParseSuccess)
						{
							LogManager::instance()->logMessage("-error-\t Planet Cloud library parse error: item[" + std::to_string(type_index) + "]is broke!",
								LML_CRITICAL);
						}

						result &= nodeParseSuccess;
						m_AllClouds.emplace_back(temp);
					}
				}
			}
			else
			{
				LogManager::instance()->logMessage("-error-\t  Planet Cloud library parse error: file structure is broke!",
					LML_CRITICAL);
				result = false;
			}



#ifdef ECHO_EDITOR
			ReadJSNode(rootNode, "TextureFolderPath", m_TextureFolderPath);
#endif

			cJSON_Delete(rootNode);
		}
		else
		{
			LogManager::instance()->logMessage("-error-\t  Planet Cloud library load failed! file: [" + m_FilePath + "] isn't exist.",
				LML_CRITICAL);
			result = false;
		}

		return result;
	}
	void PlanetCloudLibrary::save()
	{
		cJSON* rootNode = cJSON_CreateObject();

		//TODO(yanghang):Remove edit time data at release.
		cJSON* typeArrayNode = cJSON_CreateArray();
		cJSON_AddItemToObject(rootNode, "type", typeArrayNode);

		if (!m_AllClouds.empty())
		{
			cJSON* typeNode = 0;

			for (const PlanetCloudTemplateData& temp : m_AllClouds)
			{
				typeNode = cJSON_CreateObject();
				cJSON_AddItemToArray(typeArrayNode, typeNode);

				WriteJSNode(typeNode, "CloudUVScale", temp.m_UVScale);
				WriteJSNode(typeNode, "CloudFlowSpeed", temp.m_flowSpeed);
				WriteJSNode(typeNode, "CloudHDRFactor", temp.m_hdr);
				WriteJSNode(typeNode, "CloudMetallic", temp.m_metallic);
				WriteJSNode(typeNode, "CloudRoughness", temp.m_roughness);
				WriteJSNode(typeNode, "CloudAmbientFactor", temp.m_ambientFactor);
				WriteJSNode(typeNode, "CloudNoiseOffsetScale", temp.m_noiseOffsetScale);
				WriteJSNode(typeNode, "CloudTexture", temp.m_DiffusePath);
				WriteJSNode(typeNode, "CloudTemplateName", temp.m_TemplateName);
				WriteJSNode(typeNode, "CloudPitch", temp.m_pitch);
				WriteJSNode(typeNode, "CloudYaw", temp.m_yaw);
			}
		}


#ifdef ECHO_EDITOR
		WriteJSNode(rootNode,"TextureFolderPath",m_TextureFolderPath);
#endif
		saveJSONFile(m_FilePath, rootNode);
		cJSON_Delete(rootNode);
	}
	bool PlanetCloudLibrary::getCloudTemplateData(int id, PlanetCloudTemplateData& data) const
	{
		if(id >= m_AllClouds.size())
		   return false;
		data = m_AllClouds[id];
		return true;
	}
	bool PlanetCloudLibrary::judgeIsExitTemplateByName(const std::string& name)
	{
		for (size_t i = 0; i < m_AllClouds.size(); ++i)
		{
			if (m_AllClouds[i].m_TemplateName == name)
				return true;
		}
		return false;
	}
	void PlanetCloudLibrary::addPlanetCloudTemplate(const std::string& name)
	{
		if (m_AllClouds.size() >= m_MaxCount)
		{
			LogManager::instance()->logMessage("Planet Cloud Library size is Max, Add Cloud Templace False");
			return;
		}
		PlanetCloudTemplateData cloudTem;
		cloudTem.m_TemplateName = name;
		m_AllClouds.emplace_back(cloudTem);
	}
	void PlanetCloudLibrary::addPlanetCloudTemplate(const PlanetCloudTemplateData& data)
	{
		if (m_AllClouds.size() >= m_MaxCount)
		{
			LogManager::instance()->logMessage("Planet Cloud Library size is Max, Add Cloud Templace False");
			return;
		}
		m_AllClouds.emplace_back(data);
	}
}
