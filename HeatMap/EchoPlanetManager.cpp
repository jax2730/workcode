//-----------------------------------------------------------------------------
// File:   EchoBiomeTerrainManager.cpp
//
// Author: yanghang
//
// Date:  2024-4-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoStableHeaders.h"

#include "EchoPlanetManager.h"

#include "EchoTextureManager.h"

#include "cJSON.h"

#include "EchoImageProcessing.h"

#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalOcean.h"
#include "EchoDistantPlanetOceanManager.h"

#include "EchocJsonHelper.h"
#include "EchoImage.h"

#include "EchoTimer.h"
#include "EchoPlanetWarfogRenderable.h"
#include "EchoTorusTerrainNode.h"
#include "EchoTerrainModify3DUtilities.h"
#include "EchoDeformSnow.h"

namespace Echo
{
    BitmapCube
    generateSDFFromCube(BitmapCube& cube);
    
    void EchoLogToConsole(const String& log) 
    {
#ifdef _WIN32
        OutputDebugString(log.c_str());
#else
        LogManager::instance()->logMessage(log.c_str());
#endif
    }
    
    static constexpr int VERTEX_COUNT =
        SphericalTerrainQuadTreeNode::TILE_VERTICES;
    
std::unordered_map<int, RcBuffer*> PlanetManager::s_sphereVertexBuffers;
std::unordered_set<int> PlanetManager::s_bufferStartGenerated;
    SphereBakeTextureParam PlanetManager::s_sphereParam;
    TorusBakeTextureParam PlanetManager::s_torusParam;
    
    LookUpMapPair
    PlanetManager::generateBiomeLookupMap(const BiomeDistribution& data,
                                          const BiomeComposition& compo,
                                          bool generateTex, int size)
	{
		LookUpMapPair result;

		TexturePtr lookupTex;

		Bitmap tempMap = {};

		tempMap.width = size;
		tempMap.height = size;
		tempMap.bytesPerPixel = BiomeGPUData::lookupTexelSize;
		tempMap.pitch = tempMap.width * tempMap.bytesPerPixel;

		void* mem = malloc(tempMap.size());
		memset(mem, 0, tempMap.size());
		tempMap.pixels = mem;

        if(generateTex)
		{
			lookupTex = TextureManager::instance()->
				createManual(TEX_TYPE_2D,
					tempMap.width,
					tempMap.height,
					BiomeGPUData::lookupTexFormat,
					0,
					0);
			lookupTex->load();
		}



		float deltaTemperatureRange = (data.temperatureRange.max - data.temperatureRange.min);
		float deltaHumidityRange = (data.humidityRange.max - data.humidityRange.min);
		int widthScale = tempMap.width;// -1;
		int heightScale = tempMap.height;// -1;

        //IMPORTANT(yanghang):Distribution one to one
        uint8 distIndex = 0;
		for (const auto& Define : data.distRanges)
		{
            uint8 biomeIndex = compo.getBiomeIndex(distIndex);
            
			ValueRange tempRange = Define.temperature;
			ValueRange humidityRange = Define.humidity;

			tempRange.min = (tempRange.min - data.temperatureRange.min) / deltaTemperatureRange;
			tempRange.max = (tempRange.max - data.temperatureRange.min) / deltaTemperatureRange;
			tempRange.min = EchoClamp(0.f, 1.f, tempRange.min);
			tempRange.max = EchoClamp(0.f, 1.f, tempRange.max);
			int xStar = RoundfToInt(tempRange.min * widthScale);
			int xEnd = RoundfToInt(tempRange.max * widthScale);

			humidityRange.min = (humidityRange.min - data.humidityRange.min) / deltaHumidityRange;
			humidityRange.max = (humidityRange.max - data.humidityRange.min) / deltaHumidityRange;
			humidityRange.min = EchoClamp(0.f, 1.f, humidityRange.min);
			humidityRange.max = EchoClamp(0.f, 1.f, humidityRange.max);
			int yStar = RoundfToInt(humidityRange.min * heightScale);
			int yEnd = RoundfToInt(humidityRange.max * heightScale);

			uint8* row = (uint8*)tempMap.pixels + xStar * tempMap.bytesPerPixel + yStar * tempMap.pitch;
			BiomeGPUData::lookupTexel* pixel = 0;
			for (int y = yStar;
				y < yEnd;
				y++)
			{
				pixel = reinterpret_cast<BiomeGPUData::lookupTexel*>(row);
				for (int x = xStar;
					x < xEnd;
					x++)
				{
					*pixel = biomeIndex;
					pixel++;
				}
				row += tempMap.pitch;
			}

			distIndex++;
		}

        if(generateTex)
		{
			lookupTex->setTextureData(tempMap.pixels, tempMap.size(), false);
			tempMap.freeMemory();
		}

		result.GpuData = lookupTex;
		result.CpuData = tempMap;
		return (result);
	}

#ifdef ECHO_EDITOR
    LookUpMapPair
    PlanetManager::generateRegionLookupMap(const RegionData& data, bool generateTex)
	{
		LookUpMapPair result;

		TexturePtr lookupTex;

		Bitmap tempMap = {};

		tempMap.width = RegionGPUData::lookupTexWidth;
		tempMap.height = RegionGPUData::lookupTexHeight;
		tempMap.bytesPerPixel = RegionGPUData::lookupTexelSize;
		tempMap.pitch = tempMap.width * tempMap.bytesPerPixel;

		void* mem = malloc(tempMap.size());
		memset(mem, 0, tempMap.size());
		tempMap.pixels = mem;

        if(generateTex)
		{
			lookupTex = TextureManager::instance()->
				createManual(TEX_TYPE_2D,
                             tempMap.width,
                             tempMap.height,
                             ECHO_FMT_R16UI,
                             0,
                             0);
			lookupTex->load();
		}

		uint8 Index = 0;

		float deltaTemperatureRange = (data.temperatureRange.max - data.temperatureRange.min);
		float deltaHumidityRange = (data.humidityRange.max - data.humidityRange.min);
		int widthScale = tempMap.width;// -1;
		int heightScale = tempMap.height;// -1;

		for (const auto& Define : data.biomeArray)
		{
			ValueRange tempRange = Define.temperature;
			ValueRange humidityRange = Define.humidity;

			tempRange.min = (tempRange.min - data.temperatureRange.min) / deltaTemperatureRange;
			tempRange.max = (tempRange.max - data.temperatureRange.min) / deltaTemperatureRange;
			tempRange.min = EchoClamp(0.f, 1.f, tempRange.min);
			tempRange.max = EchoClamp(0.f, 1.f, tempRange.max);
			int xStar = RoundfToInt(tempRange.min * widthScale);
			int xEnd = RoundfToInt(tempRange.max * widthScale);

			humidityRange.min = (humidityRange.min - data.humidityRange.min) / deltaHumidityRange;
			humidityRange.max = (humidityRange.max - data.humidityRange.min) / deltaHumidityRange;
			humidityRange.min = EchoClamp(0.f, 1.f, humidityRange.min);
			humidityRange.max = EchoClamp(0.f, 1.f, humidityRange.max);
			int yStar = RoundfToInt(humidityRange.min * heightScale);
			int yEnd = RoundfToInt(humidityRange.max * heightScale);

			uint8* row = (uint8*)tempMap.pixels + xStar * tempMap.bytesPerPixel + yStar * tempMap.pitch;
			uint16* pixel = 0;
			for (int y = yStar;
                 y < yEnd;
                 y++)
			{
				pixel = (uint16*)row;
				for (int x = xStar;
                     x < xEnd;
                     x++)
				{
					*pixel = Index;
					pixel++;
				}
				row += tempMap.pitch;
			}

			Index++;
		}

        if(generateTex)
		{
			lookupTex->setTextureData(tempMap.pixels, tempMap.size(), false);
			tempMap.freeMemory();
		}

		result.GpuData = lookupTex;
		result.CpuData = tempMap;
		return (result);
	}
#endif
    
	// TODO
    Bitmap PlanetManager::ComputeOcclusion(const HeightMap& heightMap, const float amplitude)
    {
	    Bitmap result = {};
 
	    if (heightMap.size() > 0)
	    {
		    const int width      = heightMap.m_x;
		    result.width         = width;
		    const int height     = heightMap.m_y;
		    result.height        = height;
		    result.bytesPerPixel = 1;
		    result.pitch         = result.width * result.bytesPerPixel;

		    result.pixels = malloc(result.size());
		    memset(result.pixels, 0, result.size());

		    auto row = static_cast<uint8*>(result.pixels);
		    // uint8* pixel = 0;

		    std::vector<float> fResult(result.width * result.height, 0.0f);

		    for (int y = 0; y < height; ++y)
		    {
			    // pixel = row;
			    for (int x = 0; x < width; ++x)
			    {
				    float delta       = 0.f;
				    const float* hSrc = heightMap.pixels;
				    const float c     = hSrc[y * width + x];

				    float t;
				    if (y > 0)
				    {
					    t = hSrc[(y - 1) * width + x] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (y < height - 1)
				    {
					    t = hSrc[(y + 1) * width + x] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (x > 0)
				    {
					    t = hSrc[y * width + x - 1] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (x < width - 1)
				    {
					    t = hSrc[y * width + x + 1] - c;
					    if (t > 0.f) delta += t;
				    }

				    // Skip current pixel

				    if (y > 0 && x > 0)
				    {
					    t = hSrc[(y - 1) * width + x - 1] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (y > 0 && x < width - 1)
				    {
					    t = hSrc[(y - 1) * width + x + 1] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (y < height - 1 && x > 0)
				    {
					    t = hSrc[(y + 1) * width + x - 1] - c;
					    if (t > 0.f) delta += t;
				    }
				    if (y < height - 1 && x < width - 1)
				    {
					    t = hSrc[(y + 1) * width + x + 1] - c;
					    if (t > 0.f) delta += t;
				    }

				    // Average delta (divide by 8, scale by amplitude factor)
				    delta *= 0.125f * amplitude;
				    if (delta > 0.f)
				    {
					    // If < 0, then no occlusion
					    const float r                 = Math::Sqrt(1.f + delta * delta);
					    fResult[y * result.width + x] = (r - delta) / r;

					    // *pixel = static_cast<uint8>(std::round((r - delta) / r * 255.f));
					    // pixel++;
				    }
			    }
			    row += result.pitch;
		    }

		    // 5x5 Gaussian Blur
		    constexpr float weight[5][5] =
		    {
			    { 0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f },
			    { 0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f },
			    { 0.023792f, 0.094907f, 0.150342f, 0.094907f, 0.023792f },
			    { 0.015019f, 0.059912f, 0.094907f, 0.059912f, 0.015019f },
			    { 0.003765f, 0.015019f, 0.023792f, 0.015019f, 0.003765f }
		    };
		    for (int y = 0; y < height; ++y)
		    {
			    for (int x = 0; x < width; ++x)
			    {
				    float sum  = 0.f;
				    float wSum = 0.0f;
				    for (int dy = -2; dy <= 2; ++dy)
				    {
					    for (int dx = -2; dx <= 2; ++dx)
					    {
						    if (x + dx >= 0 && x + dx < width &&
							    y + dy >= 0 && y + dy < height)
						    {
							    const float w = weight[dy + 2][dx + 2];
							    sum += fResult[(y + dy) * width + (x + dx)] * w;
							    wSum += w;
						    }
					    }
				    }
				    const float avg = sum / wSum;

				    static_cast<uint8*>(result.pixels)[y * result.pitch + x] = static_cast<uint8>(std::round(avg * 255.f));
			    }
		    }
	    }
	    return result;
    }

    Bitmap PlanetManager::ComputeReverse(const HeightMap& heightMap, const float minVal, const float maxVal)
    {
        Bitmap result = {};
        
        if(heightMap.size()>0)
        {
            result.width = heightMap.m_x;
            result.height = heightMap.m_y;
            result.bytesPerPixel = 1;
            result.pitch = result.width * result.bytesPerPixel;

            result.pixels = malloc(result.size());
            memset(result.pixels, 0, result.size());

            uint8* row = (uint8*)result.pixels;
            uint8* pixel = 0;
            
            const float invRange = 1.0f / (maxVal - minVal);
            for (int y = 0; y < heightMap.m_y; ++y)
            {
                pixel = row;
                for (int x = 0; x < heightMap.m_x; ++x)
                {
                    const float c = heightMap.pixels[y * heightMap.m_x + x];
                    const float r = Math::Clamp(1.0f - invRange * (c - minVal), 0.0f, 1.0f);
                    *pixel = static_cast<uint8>(std::round(r * 255.f));
                    pixel++;
                }
                row += result.pitch;
            }
        }
		return (result);
    }



	PlanetManager::PlanetManager(SceneManager* mgr) : m_mgr(mgr)
    {
        m_currentPlanet = 0;
        m_nearestPlanet = 0;
		m_nearestPlanetToSurface = 0;

        mWorkQueue = Root::instance()->getWorkQueue();
		mWorkQueueChannel = mWorkQueue->getChannel("PlanetWarfogTexture");
		mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->addResponseHandler(mWorkQueueChannel, this);

		m_distantOceanMgr = new DistantPlanetOceanManager();
   }
    
    PlanetManager::~PlanetManager()
    {
        mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
		mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);
        
        uninit();
		for (SphericalTerrain* pt : m_SphericalTerrainSet)
		{
			SAFE_DELETE(pt);
		}
		m_SphericalTerrainSet.clear();

		SAFE_DELETE(m_distantOceanMgr);
    }
    
    bool PlanetManager::readHeightMap(HeightMap* out_map, const char* path)
    {
        bool result = false;
        
        HeightMap map = {};
        auto data = Root::instance()->GetFileDataStream(path);

        if(data)
        {
            if(data->size() > 0)
            {
                map.pixels = (float*)malloc(data->size());
                memcpy((void*)map.pixels, (void*)data->getData(), data->size());
            
#if 1
                map.m_x = (uint32)sqrt(data->size()/4);
                map.m_y = map.m_x;
                map.min = 0;
                map.max = 1;

        
#else
                if(map.pixels)
                {
                    map.m_x = *((int*)map.pixels);
                    map.m_y = *((int*)map.pixels+1);
                    map.min = *(map.pixels+2);
                    map.max = *(map.pixels+3);
                    map.pixels = map.pixels + 4;
                }
#endif
                *out_map = map;
				delete data;
				return true;
            }
            else
            {
                String fileName(path);
                LogManager::getSingleton().logMessage("-error-\t Biome height-map data size is 0 :[" + fileName  + "]", LML_CRITICAL);
            }
            delete data;
        }
        else
        {
            String fileName(path);
            LogManager::getSingleton().logMessage("-error-\t Biome height-map isn't exits:[" + fileName  + "]", LML_CRITICAL);
        }
        return (result);
    }
	
	bool PlanetManager::readBitmap(Bitmap& map, const char* path)
	{
		bool result = false;

		auto data = Root::instance()->GetFileDataStream(path);

		if (data)
		{
			if (data->size() > 0)
			{
				map.pixels = (uint8_t*)malloc(data->size());
				memcpy((void*)map.pixels, (void*)data->getData(), data->size());
				map.width = map.height = (uint32)sqrt(data->size());
				map.bytesPerPixel = 1;
				map.pitch = map.width * map.bytesPerPixel;
				map.byteSize = static_cast<int>(data->size());
				result = true;
			}
			else
			{
				String fileName(path);
				LogManager::getSingleton().logMessage("-error-\t Bitmap data size is 0 :[" + fileName + "]", LML_CRITICAL);
			}
			delete data;
		}
		else
		{
			String fileName(path);
			LogManager::getSingleton().logMessage("-error-\t Bitmap isn't exits:[" + fileName + "]", LML_CRITICAL);
		}
		return (result);
	}

	void PlanetManager::writeBitmap(const Bitmap& map, const char* path)
    {
	    if (map.pixels)
	    {
		    if (auto* data = Root::instance()->CreateDataStream(path))
		    {
			    data->write(map.pixels, map.size());
				data->close();
			    delete data;
		    }
	    }
    }

	//预计算生成材质ID图，每次温度湿度改变后都需要重新生成
	Bitmap PlanetManager::ComputeMaterialIndex(const Bitmap& humidity,
                                               const Bitmap& temperature,
                                               const BiomeComposition& biomeCompo,
                                               const BiomeDistribution& biomeDist)  
	{
		//NOTE(yanghang):GenerateBiomeLookupMap()给查找图生成代码拿出来用。
		Bitmap lut = {};
		{
			Bitmap& tempMap = lut;
			tempMap.width = BiomeGPUData::lookupTexWidth;
			tempMap.height = BiomeGPUData::lookupTexHeight;
			tempMap.bytesPerPixel = BiomeGPUData::lookupTexelSize;
			tempMap.pitch = tempMap.width * tempMap.bytesPerPixel;

			void* mem = malloc(tempMap.size());
			memset(mem, 0, tempMap.size());
			tempMap.pixels = mem;

			const ValueRange& tempRange = biomeDist.temperatureRange;
			const ValueRange& humidityRange = biomeDist.humidityRange;

			float deltaTemperatureRange = (tempRange.max - tempRange.min);
			float deltaHumidityRange = (humidityRange.max - humidityRange.min);
			int widthScale = tempMap.width - 1;
			int heightScale = tempMap.height - 1;

            int distIndex = 0;
			for (const auto& distRange : biomeDist.distRanges)
			{
                uint8 biomeIndex = biomeCompo.getBiomeIndex(distIndex);
				ValueRange tempRange = distRange.temperature;
				ValueRange humidityRange = distRange.humidity;

				tempRange.min = (tempRange.min - biomeDist.temperatureRange.min) / deltaTemperatureRange;
				tempRange.max = (tempRange.max - biomeDist.temperatureRange.min) / deltaTemperatureRange;
				tempRange.min = EchoClamp(0.f, 1.f, tempRange.min);
				tempRange.max = EchoClamp(0.f, 1.f, tempRange.max);
				int xStar = RoundfToInt(tempRange.min * widthScale);
				int xEnd = RoundfToInt(tempRange.max * widthScale);

				humidityRange.min = (humidityRange.min - biomeDist.humidityRange.min) / deltaHumidityRange;
				humidityRange.max = (humidityRange.max - biomeDist.humidityRange.min) / deltaHumidityRange;
				humidityRange.min = EchoClamp(0.f, 1.f, humidityRange.min);
				humidityRange.max = EchoClamp(0.f, 1.f, humidityRange.max);
				int yStar = RoundfToInt(humidityRange.min * heightScale);
				int yEnd = RoundfToInt(humidityRange.max * heightScale);

				uint8* row = (uint8*)tempMap.pixels + xStar * tempMap.bytesPerPixel + yStar * tempMap.pitch;
				uint16* pixel = 0;
				for (int y = yStar;
                     y < yEnd;
                     y++)
				{
					pixel = (uint16*)row;
					for (int x = xStar;
                         x < xEnd;
                         x++)
					{
						*pixel = biomeIndex;
						pixel++;
					}
					row += tempMap.pitch;
				}

				distIndex++;
			}
		}

		const uint8* humidityPixels = static_cast<uint8*>(humidity.pixels);
		const uint8* temperaturePixels = static_cast<uint8*>(temperature.pixels);
		const uint16* lutPixels = static_cast<uint16*>(lut.pixels);

		Bitmap mMaterialIndex;
		mMaterialIndex.height = humidity.height;
		mMaterialIndex.width = humidity.width;
		mMaterialIndex.bytesPerPixel = 1;
		mMaterialIndex.pitch = mMaterialIndex.width * mMaterialIndex.bytesPerPixel;
		
		mMaterialIndex.pixels = malloc(sizeof(uint8) * mMaterialIndex.pitch * mMaterialIndex.height);
		uint8* pixels = static_cast<uint8*>(mMaterialIndex.pixels);
		for (int y = 0; y < mMaterialIndex.height; y++)
		{
			for (int x = 0; x < mMaterialIndex.width; x++)
			{
				uint32 index = y * mMaterialIndex.width + x;
				uint8 humidityValue = humidityPixels[index];//0 - 255
				uint8 temperatureValue = temperaturePixels[index];//0 - 255

				//temperature为lut的x，humidity为y
				uint32 lutIndex = temperatureValue + humidityValue * lut.width;
				uint8 lutValue = static_cast<uint8>(lutPixels[lutIndex]);
				pixels[index] = lutValue;
			}
		}

		return mMaterialIndex;
	}

	Bitmap PlanetManager::ComputeBakedAlbedo(const BiomeComposition& biomeCompo, const Bitmap& biomeMap, const SphericalOceanData& oceanData, int width, int height, bool flipY)
    {
    	if (biomeMap.width % width != 0 || biomeMap.height % height != 0)
	    {
		    //assert(false && "Biome map size is not a multiple of the target size.");
	    	//return {};
	    }

	    if (biomeMap.byteSize != width * height * sizeof(uint8_t))
	    {
			//assert(false && "Biome map size is not equal to the target size.");
			//return {};
	    }

		using PixelFormat = ABGR;
	    Bitmap result = 
		{
			width,
			height,
			nullptr,
			width * static_cast<int>(sizeof(PixelFormat)),
			sizeof(PixelFormat)
	    };
		result.pixels = std::malloc(result.size());
		auto* pixels = static_cast<PixelFormat*>(result.pixels);

	    if (!pixels)
	    {
			LogManager::instance()->logMessage("-ERROR-\t PlanetManager::ComputeBakedAlbedo : Failed to allocate memory for the result.", LML_CRITICAL);
			return {};
	    }
    	std::vector<ColorValue> palette;
		palette.reserve(SphericalTerrain::MaxMaterialTypes);

        int biomeCount = 0;
		for (const auto & biome : biomeCompo.usedBiomeTemplateList)
		{
            if(biomeCount < SphericalTerrain::MaxMaterialTypes)
            {
                const auto& col = biome.color;
				ColorValue baseColor(col.x, col.y, col.z, col.w);
                auto blend = biome.diffuseColor;
                blend.a = 1.0f;

                palette.emplace_back(ColorValue::gammaToLinear(baseColor) * ColorValue::gammaToLinear(blend));
            }

            biomeCount++;
		}
		palette.resize(SphericalTerrain::MaxMaterialTypes, ColorValue::Black);

		const int downSample = biomeMap.width / width;
		const float weight = 1.0f / static_cast<float>(downSample * downSample);
        /*
        auto oceanColor = ColorValue::ZERO;
		if (oceanData.haveOcean)
		{
			oceanColor = 
				0.5f * ColorValue::gammaToLinear(oceanData.shadeSettings.shallowColor) + 
				0.5f * ColorValue::gammaToLinear(oceanData.shadeSettings.deepColor);
			oceanColor.a = 0.25f;
		}
        */
        uint32 rowIndex = 0;

        uint32 flipYRowIndex = (height - 1) * width;
        
		for (int y = 0; y < height; ++y)
		{
            uint32 pixelIndex = rowIndex;

            uint32 flipYPixelIndex = flipYRowIndex;
            
			for (int x = 0; x < width; ++x)
			{
				ColorValue col = ColorValue::ZERO;

                uint32 subRowIndex = y * downSample * biomeMap.width + x * downSample;
				for (int dy = 0; dy < downSample; ++dy)
				{
                    uint32 subPixelIndex = subRowIndex;
					for (int dx = 0; dx < downSample; ++dx)
					{
						const uint8 biomePixel = static_cast<const uint8*>(biomeMap.pixels)[subPixelIndex];
                        col += palette[biomePixel & SphericalTerrain::MaterialTypeMask];

                        subPixelIndex++;
					}
                    subRowIndex += biomeMap.width;
				}
				col *= weight;
				//col = ColorValue::linearToGamma(col);

                if(flipY)
                {
                    pixels[flipYPixelIndex] = col.getAsABGR();
                }
                else
                {
                    pixels[pixelIndex] = col.getAsABGR();
                }
                
                pixelIndex++;

                flipYPixelIndex++;
			}
            
            rowIndex += width;
            flipYRowIndex -= width;
		}
		return result;
    }

	Bitmap PlanetManager::ComputeBakedEmission(const BiomeComposition& biomeCompo, const Bitmap& biomeMap, const SphericalOceanData& oceanData, int width, int height, bool flipY)
	{
		if (biomeMap.width % width != 0 || biomeMap.height % height != 0)
		{
			//assert(false && "Biome map size is not a multiple of the target size.");
			//return {};
		}

		if (biomeMap.byteSize != width * height * sizeof(uint8_t))
		{
			//assert(false && "Biome map size is not equal to the target size.");
			//return {};
		}

		using PixelFormat = ABGR;
		Bitmap emissionResult = {
			width,
			height,
			nullptr,
			width * static_cast<int>(sizeof(PixelFormat)),
			sizeof(PixelFormat)
		};
		emissionResult.pixels = std::malloc(emissionResult.size());
		auto* emissionPixels = static_cast<PixelFormat*>(emissionResult.pixels);
		if (!emissionPixels)
		{
			LogManager::instance()->logMessage("-ERROR-\t PlanetManager::ComputeBakedEmission : Failed to allocate memory for the result.", LML_CRITICAL);
			return {};
		}
		std::vector<ColorValue> palette;
		palette.reserve(SphericalTerrain::MaxMaterialTypes);

		int biomeCount = 0;
		for (const auto& biome : biomeCompo.usedBiomeTemplateList)
		{
			if (biomeCount < SphericalTerrain::MaxMaterialTypes)
			{
				ColorValue emission = ColorValue(0.0);
				if (!biome.enableTextureEmission)
				{
					emission = ColorValue::gammaToLinear(biome.emission);
					emission.a = biome.emission.a * 0.02f;
					palette.emplace_back(emission);
				}
			}
			biomeCount++;
		}
		palette.resize(SphericalTerrain::MaxMaterialTypes, ColorValue::Black);

		const int downSample = biomeMap.width / width;
		const float weight = 1.0f / static_cast<float>(downSample * downSample);
		//auto oceanColor = ColorValue::ZERO;

        uint32 rowIndex = 0;

        uint32 flipYRowIndex = (height - 1) * width;
        
		for (int y = 0; y < height; ++y)
		{
            uint32 pixelIndex = rowIndex;

            uint32 flipYPixelIndex = flipYRowIndex;
            
			for (int x = 0; x < width; ++x)
			{
				ColorValue col = ColorValue::ZERO;

                uint32 subRowIndex = y * downSample * biomeMap.width + x * downSample;
				for (int dy = 0; dy < downSample; ++dy)
				{
                    uint32 subPixelIndex = subRowIndex;
					for (int dx = 0; dx < downSample; ++dx)
					{
						const uint8 biomePixel = static_cast<const uint8*>(biomeMap.pixels)[subPixelIndex];
                        col += palette[biomePixel & SphericalTerrain::MaterialTypeMask];

                        subPixelIndex++;
					}
                    subRowIndex += biomeMap.width;
				}
				col *= weight;
				//col = ColorValue::linearToGamma(col);

                if(flipY)
                {
                    emissionPixels[flipYPixelIndex] = col.getAsABGR();
                }
                else
                {
                    emissionPixels[pixelIndex] = col.getAsABGR();
                }

                pixelIndex++;

                flipYPixelIndex++;
			}
            
            rowIndex += width;
            flipYRowIndex -= width;
		}

        //IMPORTANT:Generate mip maps;

        
		return emissionResult;
	}

	std::vector<ABGR> PlanetManager::CreateCubeImage(Image cubeImages[6], int mips)
    {
	    if (std::any_of(cubeImages, cubeImages + 6, [](const auto& img)
	    {
		    return img.getData() == nullptr;
	    }))
	    {
			assert(false && "Cube map image data is null.");
			return {};
	    }

    	const uint32_t w = cubeImages[0].getWidth();
	    const uint32_t h = cubeImages[0].getHeight();

    	auto isPowerOf2 = [](const int i)
		{
			return (i & (i - 1)) == 0;
		};
		if (!isPowerOf2(w) || !isPowerOf2(h))
		{
			//assert(false && "Cube map size and target size must be power of 2.");
			//return {};
		}
		if (w != h)
		{
			//assert(false && "Cube map width and height must be equal.");
			//return {};
		}

    	if (w == 0 || h == 0)
	    {
		    return {};
	    }
	    const uint32_t pixelCount = w * h;
		mips = std::clamp(mips, 1, static_cast<int>(std::log2f(static_cast<float>(w)) + 1));
	    const uint32_t slicePitch = cubeImages[0].getSize();

	    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/cubic-environment-mapping
	    // https://www.khronos.org/opengl/wiki/Cubemap_Texture
		//
        //IMPORTANT(yanghang):Flip Y on image generate time. so don't need any more flip.
		constexpr std::bitset<6> d3dFlipU = 0b000000;
		constexpr std::bitset<6> d3dFlipV = 0b000000;//0b111111;
		constexpr std::bitset<6> glFlipU = 0b000000;
		constexpr std::bitset<6> glFlipV = 0b000000;//0b111111;
		constexpr std::bitset<6> mtlFlipU = 0b000000;
		constexpr std::bitset<6> mtlFlipV = 0b000000;//0b111111;

	    for (int i = 0; i < 6; ++i)
	    {
		    auto& img = cubeImages[i];
		    switch (Root::instance()->getDeviceType())
		    {
		    case ECHO_DEVICE_DX9C:
		    case ECHO_DEVICE_DX11:
		    case ECHO_DEVICE_DX12:
			    if (d3dFlipV[i]) img.flipAroundX();
			    if (d3dFlipU[i]) img.flipAroundY();
			    break;
		    // TODO
		    case ECHO_DEVICE_GLES3:
		    case ECHO_DEVICE_VULKAN:
			    if (glFlipV[i]) img.flipAroundX();
			    if (glFlipU[i]) img.flipAroundY();
			    break;
		    case ECHO_DEVICE_METAL:
			    if (mtlFlipV[i]) img.flipAroundX();
			    if (mtlFlipU[i]) img.flipAroundY();
				break;
		    default: assert(false);
			    break;
		    }
	    }

		// TODO: support non-power-of-2 mipmap
		size_t depthPitch = 0;
	    for (int i = 0; i < mips; ++i)
	    {
		    depthPitch += slicePitch >> (2 * i);
	    }
		const size_t totalSize = depthPitch * 6;
	    std::vector<ABGR> cubeTexture(totalSize / sizeof(ABGR));
	    for (int i = 0; i < 6; ++i)
	    {
		    std::memcpy(&cubeTexture[i * depthPitch / sizeof(ABGR)], cubeImages[i].getData(), slicePitch);

	    	size_t srcOffset = i * depthPitch / sizeof(ABGR);
	    	size_t dstOffset = srcOffset + pixelCount;
		    for (int m = 1; m < mips; ++m)
		    {
				// const size_t srcH = h >> (m - 1);
				const size_t srcW = w >> (m - 1);
				const size_t dstH = h >> m;
				const size_t dstW = w >> m;

		    	const ABGR* src = &cubeTexture[srcOffset];
		    	ABGR* dst = &cubeTexture[dstOffset];
			    for (size_t col = 0; col < dstH; ++col)
			    {
				    for (size_t row = 0; row < dstW; ++row)
				    {
						const ABGR* src0 = src + col * 2 * srcW + row * 2;
				    	const ABGR* src1 = src0 + 1;
						const ABGR* src2 = src0 + srcW;
						const ABGR* src3 = src2 + 1;
						ABGR* dst0 = dst + col * dstW + row;
						ColorValue c0, c1, c2, c3;
						c0.setAsABGR(*src0);
						c1.setAsABGR(*src1);
						c2.setAsABGR(*src2);
						c3.setAsABGR(*src3);
						*dst0 = ((c0 + c1 + c2 + c3) * 0.25f).getAsABGR();
				    }
			    }

				srcOffset = dstOffset;
				dstOffset += pixelCount >> (2 * m);
		    }
	    }
		return cubeTexture;
    }

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
    
    std::vector<ABGR> PlanetManager::CreateImageMip(Image& cubeImages)
    {
    	uint32_t w = cubeImages.getWidth();
	    uint32_t h = cubeImages.getHeight();

    	if (w == 0 || h == 0)
	    {
		    return {};
	    }

        int mips = calculateMipCount(std::min(w,h));
        
	    uint32_t pixelCount = w * h;
	    uint32_t slicePitch = cubeImages.getSize();

		size_t depthPitch = 0;
	    for (int i = 0; i < mips; ++i)
	    {
            uint32_t sliceSize = (w >> i) * ( h >> i) * sizeof(ARGB);
		    depthPitch += sliceSize;
	    }

	    std::vector<ABGR> cubeTexture(depthPitch / sizeof(ABGR));
	    {
		    std::memcpy(&cubeTexture[0], cubeImages.getData(), slicePitch);

	    	size_t srcOffset = 0;
	    	size_t dstOffset = srcOffset + pixelCount;
		    for (int m = 1; m < mips; ++m)
		    {
				const size_t srcW = w >> (m - 1);
				const size_t dstH = h >> m;
				const size_t dstW = w >> m;

		    	const ABGR* src = &cubeTexture[srcOffset];
		    	ABGR* dst = &cubeTexture[dstOffset];
			    for (size_t col = 0; col < dstH; ++col)
			    {
				    for (size_t row = 0; row < dstW; ++row)
				    {
						const ABGR* src0 = src + col * 2 * srcW + row * 2;
				    	const ABGR* src1 = src0 + 1;
						const ABGR* src2 = src0 + srcW;
						const ABGR* src3 = src2 + 1;
						ABGR* dst0 = dst + col * dstW + row;
						ColorValue c0, c1, c2, c3;
						c0.setAsABGR(*src0);
						c1.setAsABGR(*src1);
						c2.setAsABGR(*src2);
						c3.setAsABGR(*src3);
						*dst0 = ((c0 + c1 + c2 + c3) * 0.25f).getAsABGR();
				    }
			    }

				srcOffset = dstOffset;

                size_t mipPixelCount = dstH * dstW;
				dstOffset += mipPixelCount;
		    }
	    }
		return cubeTexture;
    }

	void PlanetManager::uninit()
    {	
    }

    void PlanetManager::updateCurrentPlanet(const Camera* pCamera)
    {
        //IMPORTANT(yang):Find nearest planet.
        //TODO(yang):If planets are too much. this may be bottle-neck.
        double nearestDistanceSqr = DBL_MAX;
		auto planetSave = m_nearestPlanet;
		double nearestDistanceToSurface = DBL_MAX;
        m_nearestPlanet = 0;
        for (auto planet : m_SphericalTerrainSet)
        {
	        DVector3 camPos = pCamera->getDerivedPosition();
            DVector3 planetPos = planet->m_pos;
            
            double distanceSqua = (camPos - planetPos).squaredLength();
			double distanceToSurface = std::max(sqrt(distanceSqua) - planet->m_radius, 1e-5);
			if(distanceSqua < nearestDistanceSqr)
            {
                nearestDistanceSqr = distanceSqua;
                m_nearestPlanet = planet;
            }

			if (distanceToSurface < nearestDistanceToSurface)
			{
				nearestDistanceToSurface = distanceToSurface;
				m_nearestPlanetToSurface = planet;
			}
        }
        
		if (planetSave && planetSave != m_nearestPlanet) {
			if (planetSave->m_deformSnow) {
				planetSave->m_deformSnow->clearAll();
			}
		}

        //IMPORTANT:First check inside the planet or not.
        if(m_currentPlanet)
        {
            double maxRadius = m_currentPlanet->getMaxRealRadius();
            double maxRadiusSqua = maxRadius * maxRadius;
            
            DVector3 camPos = pCamera->getDerivedPosition();
            DVector3 planetPos = m_currentPlanet->m_pos; 

            double distanceSqua = (camPos - planetPos).squaredLength();
            if(distanceSqua < maxRadiusSqua)
            {
                return;
            }
            else
            {
                m_currentPlanet = 0;            
            }
        }

        //IMPORTANT(yang):Assume that each plants will be completely seperate from each
        // other. never intesect.
        for (auto planet : m_SphericalTerrainSet)
        {
            double maxRadius = planet->getMaxRealRadius();
            double maxRadiusSqua = maxRadius * maxRadius;
            
            DVector3 camPos = pCamera->getDerivedPosition();
            DVector3 planetPos = planet->m_pos;

            double distanceSqua = (camPos - planetPos).squaredLength();
            if(distanceSqua < maxRadiusSqua)
            {
                m_currentPlanet = planet;
                break;
            }
        }
    }
    
    void PlanetManager::findVisibleObjects(const Camera* pCamera,
                                           RenderQueue* pQueue)
    {
     
#if 0
        //NOTE:region SDF region fast test code.
        {
#if 1
            static bool init =true;
#else
            static bool init =false;
#endif
            
            if(!init)
            {
                init = true;
                String filePath = "echo/biome_terrain/spherical_voronoi_map/";
                String name = "21_[6x177x153]";

                BitmapCube fineCube = {};
                for(int i = 0;
                    i < 6;
                    i++)
                {
                    Bitmap& fineMap = fineCube.maps[i];
                    fineMap.width = 512;
                    fineMap.height = 512;
                    fineMap.bytesPerPixel = 1;
                    fineMap.pitch = fineMap.width * fineMap.bytesPerPixel;
                    fineMap.pixels = malloc(fineMap.pitch * fineMap.height);
                        
                    String fineMapName = filePath + name +
                        +"_" + std::to_string(i) + ".map";
                    auto File = Root::instance()->GetFileDataStream(fineMapName.c_str());
                    if(File->getData())
                    {
                        memcpy(fineMap.pixels,
                               File->getData(),
                               fineMap.pitch * fineMap.height);
                    }
                }

                BitmapCube sdfCube = generateSDFFromCube(fineCube);

                for(int i = 0;
                    i < 6;
                    i++)
                {
                    Bitmap& sdfMap = sdfCube.maps[i];
                    Bitmap& fineMap = fineCube.maps[i];

                    String filePath = "spherical_voronoi_map/";
                    
                    String fineSDFMapName = String("echo/biome_terrain/spherical_voronoi_map/") + "fine" + name
                        +"_" + std::to_string(i) + ".sdf";

                    DataStream* data_stream = 0;
                    data_stream = Root::instance()->CreateDataStream(fineSDFMapName.c_str());
                    if(data_stream)
                    { 
                        assert(data_stream);
                        FileDataStream* file_stream = (FileDataStream *)data_stream;
                        file_stream->write(sdfMap.pixels,
                                           sdfMap.pitch * sdfMap.height);
                        SAFE_DELETE(file_stream);
                    }

                    String fineMapName = filePath + name +
                        +"_" + std::to_string(i) + ".png";

                    int error = stbi_write_png(fineMapName.c_str(),
                                               sdfMap.width,
                                               sdfMap.height,
                                               1,
                                               sdfMap.pixels,
                                               sdfMap.pitch);

                    fineMapName = filePath + name +
                        +"_origin_" + std::to_string(i) + ".png";
                    error = stbi_write_png(fineMapName.c_str(),
                                               fineMap.width,
                                               fineMap.height,
                                               1,
                                               fineMap.pixels,
                                               fineMap.pitch);
                }
            }
        }
#endif
        
        
        updateCurrentPlanet(pCamera);
        
        if(m_displayTerrain)
        {
			for (auto planet : m_SphericalTerrainSet) {
				planet->findVisibleObjects(pCamera, pQueue);
			}
        }

#ifdef _WIN32
        if(m_printProfileData)
        {
			uint64 m_biomeTexMemory = SphericalTerrainResourceManager::m_biomeTexMemory;
            String log = String("--------Biome texture memory size:") + String("-------\n");
            OutputDebugString(log.c_str());
            log = std::to_string(m_biomeTexMemory) +String(" bytes \n ") +
                std::to_string(m_biomeTexMemory / 1024) +String(" KB\n") +
                std::to_string(m_biomeTexMemory / 1024 / 1024) +String(" MB\n");
            OutputDebugString(log.c_str());

            log = String("Total triangle count:") + std::to_string(m_drawTriangleCount) +String(" ") +
                std::to_string(m_drawTriangleCount/1000) +String("k ") +
                std::to_string(m_drawTriangleCount/1000000) +String("m\n");
            OutputDebugString(log.c_str());
        }
#else
        if(m_printProfileData)
        {
            uint64 m_biomeTexMemory = SphericalTerrainResourceManager::m_biomeTexMemory;
            String log = String("--------Biome texture memory size:") + String("-------\n");
            LogManager::instance()->logMessage(log.c_str());
            log = std::to_string(m_biomeTexMemory) +String(" bytes \n ") +
                std::to_string(m_biomeTexMemory / 1024) +String(" KB\n") +
                std::to_string(m_biomeTexMemory / 1024 / 1024) +String(" MB\n");
            LogManager::instance()->logMessage(log.c_str());

            log = String("Total triangle count:") + std::to_string(m_drawTriangleCount) +String(" ") +
                std::to_string(m_drawTriangleCount/1000) +String("k ") +
                std::to_string(m_drawTriangleCount/1000000) +String("m\n");
            LogManager::instance()->logMessage(log.c_str());
        }        
#endif
        m_drawTriangleCount = 0;
    }

    void PlanetManager::displayTerrain(bool visible)
    {
        m_displayTerrain = visible;
    }

    Bitmap PlanetManager::climateProcess(const Bitmap& bitmap,
                                         ImageProcess process)
    {
        Bitmap result = {};
        
        if(bitmap.pixels &&
           bitmap.width > 0 &&
           bitmap.height > 0)
        {
            result.width = bitmap.width;
            result.height = bitmap.height;
            result.pitch = bitmap.pitch;
            result.bytesPerPixel = bitmap.bytesPerPixel;
            result.pixels = malloc(bitmap.size());
            
            ImageProcessor imageProcessor;

            std::vector<uint8> rawPixels;
            rawPixels.insert(rawPixels.begin(), (uint8*)bitmap.pixels, ((uint8*)bitmap.pixels + bitmap.size()));

            if(process.brightness != 0)
            {
                imageProcessor.Brightness(rawPixels, process.brightness);
            }
            
            if(process.exposure != 0 ||
               process.exposureGamma != 1 ||
               process.exposureOffset != 0)
            {
                imageProcessor.Exposure(rawPixels,
                                        process.exposure,
                                        process.exposureOffset,
                                        process.exposureGamma);
            }

            if(process.invert)
            {
                imageProcessor.Invert(rawPixels);
            }
            
            if(process.clamp.min != 0 || process.clamp.max != 1)
            {
                imageProcessor.Clamp(rawPixels,
                                     process.clamp.max,
                                     process.clamp.min);
            }

            memcpy(result.pixels, rawPixels.data(), bitmap.size());
        }

        return (result);
    }

    void PlanetManager::useClimateProcess(Bitmap& originMap,
                                          Bitmap& map,
                                          TexturePtr& tex,
                                          ImageProcess& ip)
    {
        map = climateProcess(originMap,
                             ip);

        if(map.pixels)
        {
            tex->setTextureData(map.pixels,
                                map.size(),
                                false);
        }
    }

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
    
    void PlanetManager::testLoadPlanet(bool load)
    {
        static std::vector<SphericalTerrain*> s_planets;
        if(load)
        {
            //Free old.
            for(auto planet : s_planets)
            {
                destroySphericalTerrain(planet);
            }
            s_planets.clear();
            
            //Build new
            cJSON* root = readJSONFile("biome_terrain/testPlanet.list");
            cJSON* planetArray = cJSON_GetObjectItem(root,"planets path array");
            if(planetArray->type == cJSON_Array)
            {
                int planetCount = cJSON_GetArraySize(planetArray);

                //Debug Test, remove. 
                //planetCount = 1;
                
                float radius = 5000;

                float pitch = radius * 2.1f;
                
                for(int planetIndex = 0;
                    planetIndex < planetCount;
                    planetIndex++)
                {
                    cJSON* planetNode = cJSON_GetArrayItem(planetArray, planetIndex);
                    if(planetNode->type == cJSON_String)
                    {
                        String filePath = planetNode->valuestring;
                        filePath = "biome_terrain/" + filePath + ".terrain";
                        
                        cJSON* terrainFile = readJSONFile(filePath);
                        
                        if(terrainFile)
                        {
                            SphericalTerrain* planet = createSphericalTerrain(filePath);
                            s_planets.push_back(planet);

                            int x = planetIndex % 16;
                            int z = planetIndex / 16;
                            planet->setWorldPos(Vector3(pitch * x, pitch, pitch * z));

							cJSON_Delete(terrainFile);
                        }
                    }
                }
            }
        }
        else
        {
            for(auto planet : s_planets)
            {
                destroySphericalTerrain(planet);
            }
            s_planets.clear();
        }
    }

	void PlanetManager::displaySphericalVoronoiRegion(bool mode)
	{
		if (!Root::instance()->m_useSphericalVoronoiRegion)
			return;
		m_showSphericalVoronoiRegion = mode;
		for (auto& terrain : m_SphericalTerrainSet)
		{
			terrain->displaySphericalVoronoiRegion(mode);
		}
	}

	SphericalTerrain*
    PlanetManager::createSphericalTerrain(const String& terrain, bool bSync, bool terrainEditor, bool bAtlas, const DTransform& trans, const Vector3& scale) {
		if (terrain.empty()) return nullptr;

		SphericalTerrain* pTerrain = new SphericalTerrain(this);
		pTerrain->setWorldTrans(trans, scale);
		pTerrain->init(terrain, bSync, terrainEditor, bAtlas);
		m_SphericalTerrainSet.insert(pTerrain);
		return pTerrain;
	}
    
	bool PlanetManager::destroySphericalTerrain(SphericalTerrain* pTerrain) {
		if (pTerrain == nullptr) return false;
		const auto terrain = m_SphericalTerrainSet.find(pTerrain);
		if (terrain == m_SphericalTerrainSet.end()) return false;
    	m_SphericalTerrainSet.erase(terrain);

        //IMPORTANT(yang):Free planet, check the cached pointer state.
        if(pTerrain == m_currentPlanet)
        {
            m_currentPlanet = 0;
        }

        if(pTerrain == m_nearestPlanet)
        {
            m_nearestPlanet = 0;
        }

		if (pTerrain == m_nearestPlanetToSurface)
		{
			m_nearestPlanetToSurface = 0;
		}

		SAFE_DELETE(pTerrain);
		return true;
	}

	SphericalTerrain* PlanetManager::getNearestSphericalTerrain(const DVector3& worldPosition) const {
		SphericalTerrain* pNearestSphTer = nullptr;
		double nearestDistance = DBL_MAX;
		for (SphericalTerrain* pSphTer : m_SphericalTerrainSet) {
			DVector3 planetPos = pSphTer->m_pos;
			double distance = (worldPosition - planetPos).length();
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				pNearestSphTer = pSphTer;
			}
		}
		return pNearestSphTer;
	}

    SphericalTerrain*  PlanetManager::getCurrentPlanet()
    {
        return m_currentPlanet;
    }
    
    SphericalTerrain*  PlanetManager::getNearestPlanet()
    {
        return m_nearestPlanet;
    }
	SphericalTerrain* PlanetManager::getNearestPlanetToSurface()
	{
		return m_nearestPlanetToSurface;
	}

    void PlanetManager::enableOceanSSR(const bool enable)
    {
		m_enableOceanSSR = enable;
	    for (auto planet : m_SphericalTerrainSet)
	    {
		    if (auto* ocean = planet->getOcean())
		    {
			    ocean->enableSSR(enable);
		    }
	    }
    }

    SphericalOcean*  PlanetManager::getCurrentOcean()
    {
		SphericalOcean* result = 0;
        if(m_currentPlanet)
        {
            result = m_currentPlanet->getOcean();
        }
        return(result);
    }

 	TexturePtr PlanetManager::generateTextureFromBitmap(const Bitmap& tempMap) 
	{
		TexturePtr lookupTex = TextureManager::instance()->
			createManual(TEX_TYPE_2D,
                         tempMap.width,
                         tempMap.height,
                         ECHO_FMT_R16UI,
                         0,
                         0);
		lookupTex->load();
		lookupTex->setTextureData(tempMap.pixels, tempMap.size(), false);
		return lookupTex;
	}   

    //NOTE: Generate SDF map.
    enum CubeFaceDir
    {
        left,
        right,
        top,
        down,
    };

    enum CubeFace
    {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    static Vector3 CubeFaceBaseVector[6]=
    {
        {1, 0, 0},
        {-1, 0,0},
        { 0, 1, 0},
        { 0, -1, 0},
        { 0, 0, 1},
        { 0, 0, -1},
    };

    Vector3 getCubeFacePos(CubeFace face, float x, float y) 
    {
        Vector3 v;
        switch(face)
        {
            case PositiveX:
                v.x = 1;
                v.y =  y;
                v.z = -x;
                break;
            case NegativeX:
                v.x = -1;
                v.y = y;
                v.z = x;
                break;
            case PositiveY:
                v.x = x;
                v.y = 1;
                v.z = -y;
                break;
            case NegativeY:
                v.x = x;
                v.y = -1;
                v.z = y;
                break;
            case PositiveZ:
                v.x = x;
                v.y = y;
                v.z = 1;
                break;
            case NegativeZ:
                v.x = -x;
                v.y = y;
                v.z = -1;
                break;
        }
        return (v);
    };
    
    CubeFace getCubeNeighbour(CubeFace face,
                              CubeFaceDir dir)
    {
        if(face == PositiveX)
        {
            switch(dir)
            {
                case left:
                    return PositiveZ;
                case right:
                    return NegativeZ;
                case top:
                    return PositiveY;
                case down:
                    return NegativeY;
            }
        }

        if(face == PositiveY)
        {
            switch(dir)
            {
                case left:
                    return NegativeX;
                case right:
                    return PositiveX;
                case top:
                    return NegativeZ;
                case down:
                    return PositiveZ;
            }
        }

        if(face == PositiveZ)
        {
            switch(dir)
            {
                case left:
                    return NegativeX;
                case right:
                    return PositiveX;
                case top:
                    return PositiveY;
                case down:
                    return NegativeY;
            }
        }

        if(face == NegativeX)
        {
            switch(dir)
            {
                case left:
                    return NegativeZ;
                case right:
                    return PositiveZ;
                case top:
                    return PositiveY;
                case down:
                    return NegativeY;
            }
        }

        if(face == NegativeY)
        {
            switch(dir)
            {
                case left:
                    return NegativeX;
                case right:
                    return PositiveX;
                case top:
                    return PositiveZ;
                case down:
                    return NegativeZ;
            }
        }

        if(face == NegativeZ)
        {
            switch(dir)
            {
                case left:
                    return PositiveX;
                case right:
                    return NegativeX;
                case top:
                    return PositiveY;
                case down:
                    return NegativeY;
            }
        }
        assert(false&&"Unkonwn face relationship!");
        return PositiveX;
    }

    //NOTE:Find neighbour pixel
    struct FindNeighbourPixelResult
    {
        uint8 pixel;
        bool valid;
    };
        
    FindNeighbourPixelResult
    getNeighbourCubefacePixel(int face, int x, int y, BitmapCube in_cube)
    {
        FindNeighbourPixelResult result = {};

        Bitmap* cube = &in_cube.maps[0];
        
        assert(face < 6 && 0 <= face);
        assert((x < 0 ||
                y < 0 ||
                cube[face].width <= x ||
                cube[face].height <= y));

        //NOTE:Calc neighbour face.
        //
        //    -----------------------
        //    |      |      |      |
        //    |  N   |      |  N   |      
        //    |------|------|------|
        //    |      |      |      |
        //    |-----[0,0]---|------|
        //    |  N   |      |  N   |
        //    |-----------------------
        // The N word neighbour isn't in cube. so is invalid.
        if(!((x < 0 || cube[face].width <= x) &&
             (y < 0 || cube[face].height <= y)))
        {
            int width = cube[face].width;
            int height = cube[face].height;
            
            CubeFaceDir dir = {};
            int newx = 0,
                newy = 0;
            CubeFace newFace = {};
            if(x < 0)
            {
                dir = left;
                if(face == PositiveY)
                {
                    newx = width - 1 - y;
                    newy = height + x;
                }
                else if(face == NegativeY)
                {
                    newx = y;
                    newy = -x - 1;
                }
                else if(face == PositiveX ||
                        face == NegativeX ||
                        face == PositiveZ ||
                        face == NegativeZ)
                {
                    newy = y;
                    newx = width + x;
                }
            }
            else if(width <= x)
            {
                dir = right;
                
                if(face == PositiveY)
                {
                    newx = y;
                    newy = (height - 1 ) - (x - width);
                }
                else if(face == NegativeY)
                {
                    newx = width - 1 - y;
                    newy = x - width;
                }
                else if(face == PositiveX ||
                        face == NegativeX ||
                        face == PositiveZ ||
                        face == NegativeZ)
                {
                    newy = y;
                    newx = x - width;
                }
            }
            else if(y < 0)
            {
                dir = down;

                if(face == PositiveX)
                {
                    newx = width + y;
                    newy = (height -1) - x;
                }
                else if(face == NegativeX)
                {
                    newx = -y - 1;
                    newy = x;
                }
                else if(face == PositiveY)
                {
                    newx = x;
                    newy = height + y;
                }
                else if(face == NegativeY)
                {
                    newx = width - 1 - x;
                    newy = -y - 1;
                }
                else if(face == PositiveZ)
                {
					newx = x;
					newy = height + y;
                }
                else if(face == NegativeZ)
                {
					newx = width -1 - x;
					newy = - y - 1;
                }
            }
            else if(height <= y)
            {
                dir = top;

                if(face == PositiveX)
                {
                    newx = (width - 1) - (y - height);
                    newy = x;
                }
                else if(face == NegativeX)
                {
                    newx = y - height;
                    newy = (height - 1) - x;
                }
                else if(face == PositiveY)
                {
                    newx = width - 1 - x;
                    newy = height - 1 - (y - height);
                }
                else if(face == NegativeY)
                {
                    newx = x;
                    newy = y - height;
                }
                else if(face == PositiveZ)
                {
                    newx = x;
                    newy = y - height;
                }
                else if(face == NegativeZ)
                {
                    newx = (width - 1) - x;
                    newy = (height- 1) - (y - height);
                }
            }
            else
            {
                assert(false && "Unkonw situation!");
            }

            newFace = getCubeNeighbour((CubeFace)face, dir);

            Bitmap* newFaceMap = &cube[newFace];
            result.pixel = *((uint8*)newFaceMap->pixels +
                             newx * newFaceMap->bytesPerPixel +
                             newy * newFaceMap->pitch);
            result.valid = true;
        }
        return(result);
    }

    static const int s_scanRadius = 16;
    static const float s_invScanRadius = 255.0f / float(s_scanRadius);
    static const int s_MaxDistance = s_scanRadius * s_scanRadius;
    static const int s_aroundPointCount = 128;
    
    struct PointAndDistance
    {
        int x,y;
        int distance;

        struct DistanceCompare
        {
            bool operator()(const PointAndDistance& a, const PointAndDistance& b)
            {
                return (a.distance < b.distance);
            }
        };
    };
    
	static Bitmap generateSDFWarp(const Bitmap& plane)
	{
		
		Bitmap result = {};

		const Bitmap& originMap = plane;
		Bitmap& sdfMap          = result;

		sdfMap.width         = originMap.width;
		sdfMap.height        = originMap.height;
		sdfMap.bytesPerPixel = originMap.bytesPerPixel;

		sdfMap.pitch = sdfMap.width * sdfMap.bytesPerPixel;

		sdfMap.pixels = std::malloc(sdfMap.height * sdfMap.pitch);

		auto row           = static_cast<uint8*>(originMap.pixels);
		const uint32 pitch = originMap.pitch;

		auto sdfRow           = static_cast<uint8*>(sdfMap.pixels);
		const uint32 sdfPitch = sdfMap.pitch;

        uint8 minSDF = 255;
        
		for (int y = 0; y < originMap.height; y++)
		{
			const uint8* pixel = row;
			uint8* sdfPixel    = sdfRow;

			for (int x = 0; x < originMap.width; x++)
			{
				const uint8 id = *pixel;


                std::vector<PointAndDistance> minDistanceList;
				for (int sy = y - s_scanRadius; sy < y + s_scanRadius; sy++)
				{
					for (int sx = x - s_scanRadius; sx < x + s_scanRadius; sx++)
					{
                        int new_distance   = s_MaxDistance;
                        
						const auto warpY = (sy + originMap.height) % originMap.height;
						const auto warpX = (sx + originMap.width) % originMap.width;
						if (static_cast<const uint8_t*>(originMap.pixels)[warpY * originMap.width + warpX] != id)
						{
							const int dw = x - sx;
							const int dh = y - sy;

							new_distance = std::min(dw * dw + dh * dh, new_distance);
						}

                        //Insert small number.
                        if(new_distance < s_MaxDistance)
                        {
                            PointAndDistance tp;
                            tp.distance = new_distance;
                            tp.x = sx;
                            tp.y = sy;

                            minDistanceList.push_back(tp);
                        }
					}
				}

                int distance = s_MaxDistance;
                if(!minDistanceList.empty())
                {
                    std::stable_sort(minDistanceList.begin(), minDistanceList.end(),
                                     PointAndDistance::DistanceCompare());
                        
                    distance = 0;
                    int validCount = 0;
                    for(int p_index = 0;
                        p_index < s_aroundPointCount;
                        p_index++)
                    {
                        if(p_index < minDistanceList.size())
                        {
                            distance += minDistanceList[p_index].distance;
                            validCount++;
                        }
                    }
                    distance = distance / validCount;
                }

                uint8 sdf = (uint8)(roundf(sqrt(float(distance)) * s_invScanRadius));
                *sdfPixel = sdf;

                if(sdf < minSDF)
                {
                    minSDF = sdf;
                }

				pixel += originMap.bytesPerPixel;
				sdfPixel += sdfMap.bytesPerPixel;
			}
			row += pitch;
			sdfRow += sdfPitch;
		}

        const float f_minSDF = minSDF/ 255.0f;
        const float inv_one_minus_f_minSDF = 1.0f / (1.0f - f_minSDF);

        sdfRow = static_cast<uint8*>(sdfMap.pixels);
        for (int y = 0; y < sdfMap.height; y++)
		{
			uint8* sdfPixel    = sdfRow;

			for (int x = 0; x < sdfMap.width; x++)
			{
                uint8 sdf = *sdfPixel;

                float f_sdf = ((sdf / 255.0f) - f_minSDF) * inv_one_minus_f_minSDF;
                sdf = (uint8)(f_sdf * 255.0f);
                    
                *sdfPixel = sdf;

				sdfPixel += sdfMap.bytesPerPixel;
			}
			row += pitch;
			sdfRow += sdfPitch;
		}

		return result;
	}

    BitmapCube
    generateSDFFromCube(BitmapCube& cube)
    {
        BitmapCube result = {};

        uint8 minSDF = 255;
        
        for(int i = 0;
            i < 6;
            i++)
        {
            Bitmap& originMap = cube.maps[i];
            Bitmap& sdfMap = result.maps[i];

            sdfMap.width = originMap.width;
            sdfMap.height = originMap.height;
            sdfMap.bytesPerPixel = originMap.bytesPerPixel;
            
            sdfMap.pitch = sdfMap.width * sdfMap.bytesPerPixel;
            
            sdfMap.pixels = malloc(sdfMap.height * sdfMap.pitch);

            uint8* row = (uint8*)originMap.pixels;
            uint32 pitch = originMap.pitch;

            uint8* sdfRow = (uint8*)sdfMap.pixels;
            uint32 sdfPitch = sdfMap.pitch;

            for(int h = 0;
                h < originMap.height;
                h++)
            {
                uint8* pixel = row;
                uint8* sdfPixel = sdfRow;
                
                for(int w = 0;
                    w < originMap.width;
                    w++)
                {
                    //NOTE:Debug purpose. For find texture coord in the cube.
                    #if 0
                    {
                        uint8* sdfPixel = (uint8*)sdfMap.pixels + sdfMap.pitch* h + sdfMap.bytesPerPixel * w;
                        if( w <16 && h < 16)
                        {
                            *sdfPixel = 255;
                        }
                        else if( w < sdfMap.width && 32 < w && h < 16)
                        {
                            *sdfPixel = 255;
                        }
                        else if( h < sdfMap.height && 32 < h && w < 16)
                        {
                            *sdfPixel = 196;
                        }
                        else
                        {
                            *sdfPixel = 32 * (6 -i);
                        }
                        continue;
                    }
                    #endif
                    
                    uint8 id = *pixel;
                
                    uint8* s_row = row - (s_scanRadius * pitch) + (w - s_scanRadius);

                    std::vector<PointAndDistance> minDistanceList;
                    for(int sh = h - s_scanRadius;
                        sh < h + s_scanRadius;
                        sh++)
                    {
                        uint8* s_pixel = s_row;
                        for(int sw = w - s_scanRadius;
                            sw < w + s_scanRadius;
                            sw++)
                        {
                            int new_distance = s_MaxDistance;

                            if(0 <= sh && 0 <= sw &&
                               sh < originMap.height &&
                               sw < originMap.width)
                            {
                                uint8 s_id = *s_pixel;
                                if(s_id != id)
                                {
                                    int dw = (w-sw);
                                    int dh = (h-sh);
                                    new_distance = dw*dw + dh*dh; 
                                }
                            }
                            else
                            {
                                //IMPORTANT:Find other cube face pixel.
                                FindNeighbourPixelResult neighbourResult =
                                    getNeighbourCubefacePixel(i,
                                                              sw,
                                                              sh,
                                                              cube);
                                if(neighbourResult.valid)
                                {
                                    uint8 s_id = neighbourResult.pixel;
                                    if(s_id != id)
                                    {
                                        int dw = (w-sw);
                                        int dh = (h-sh);
                                        new_distance = dw*dw + dh*dh;
                                    }   
                                }
                            }
                            
                            //Insert small number.
                            if(new_distance < s_MaxDistance)
                            {
                                PointAndDistance tp;
                                tp.distance = new_distance;
                                tp.x = sw;
                                tp.y = sh;

                                minDistanceList.push_back(tp);
                            }
                            
                            s_pixel++;
                        }// sw
                        s_row += pitch;
                    }// sh

                    int distance = s_MaxDistance;
                    if(!minDistanceList.empty())
                    {
                        std::stable_sort(minDistanceList.begin(), minDistanceList.end(),
                                         PointAndDistance::DistanceCompare());
                        
                        distance = 0;
                        int validCount = 0;
                        for(int p_index = 0;
                            p_index < s_aroundPointCount;
                            p_index++)
                        {
                            if(p_index < minDistanceList.size())
                            {
                                distance += minDistanceList[p_index].distance;
                                validCount++;
                            }
                        }
                        distance = distance / validCount;
                    }

                    uint8 sdf = (uint8)(roundf(sqrt(float(distance)) * s_invScanRadius));
                    *sdfPixel = sdf;

                    if(sdf < minSDF)
                    {
                        minSDF = sdf;
                    }
                    
                    pixel+= originMap.bytesPerPixel;
                    sdfPixel+= sdfMap.bytesPerPixel;
                } //w

                row += pitch;
                sdfRow += sdfPitch;
            }//h
        }// end face

        const float f_minSDF = minSDF/ 255.0f;
        const float inv_one_minus_f_minSDF = 1.0f / (1.0f - f_minSDF);
        
        for(int i = 0;
            i < 6;
            i++)
        {
            Bitmap& sdfMap = result.maps[i];
            uint8* sdfRow = (uint8*)sdfMap.pixels;
            uint32 sdfPitch = sdfMap.pitch;

            for(int h = 0;
                h < sdfMap.height;
                h++)
            {
                uint8* sdfPixel = sdfRow;
                
                for(int w = 0;
                    w < sdfMap.width;
                    w++)
                {
                    uint8 sdf = *sdfPixel;

                    float f_sdf = ((sdf / 255.0f) - f_minSDF) * inv_one_minus_f_minSDF;
                    sdf = (uint8)(f_sdf * 255.0f);
                    
                    *sdfPixel = sdf;
                    
                    sdfPixel += sdfMap.bytesPerPixel;
                } //w

                sdfRow += sdfPitch;
            }//h
        }// end face

        return (result);
    }
    
    //NOTE: Need idMapCube must be 6 bitmap.
    GenerateRegionSDFResult
    GeneratePlanetRegionSDF(BitmapCube& regionIDCube,
                            const std::vector<SphericalVoronoiRegionDefine>& defines)
    {
        GenerateRegionSDFResult result = {};
            
        //NOTE:For fast lookup.
        std::vector<int> parentLookupTable;
        for(auto& define : defines)
        {
            parentLookupTable.push_back(define.Parent);
        }

        result = GeneratePlanetRegionSDF(regionIDCube,parentLookupTable);
        
        return (result);
    }

    //NOTE:generate coarse id map and merged with fine id map.
    BitmapCube
    GenerateCoarseRegion(BitmapCube& regionIDCube,
                         const std::vector<SphericalVoronoiRegionDefine>& defines,
                         int slices)
    {
        BitmapCube coarseIDCube = {};

        //NOTE:For fast lookup.
        std::vector<int> parentLookupTable;
        for(auto& define : defines)
        {
            parentLookupTable.push_back(define.Parent);
        }
        
        for(int i = 0;
            i < slices;
            i++)
        {
            Bitmap& originMap = regionIDCube.maps[i];
            Bitmap& coarseMap = coarseIDCube.maps[i];

            coarseMap.width = originMap.width;
            coarseMap.height = originMap.height;
            coarseMap.bytesPerPixel = originMap.bytesPerPixel;

            coarseMap.pitch = coarseMap.width * coarseMap.bytesPerPixel;
            coarseMap.pixels = malloc(coarseMap.pitch * coarseMap.height);

            uint8* row = (uint8*)originMap.pixels;
            uint8* mergedRow =  (uint8*)coarseMap.pixels;
            for(int h = 0;
                h < originMap.height;
                h++)
            {
                uint8* pixel = row;
                uint8* mergedPixel = mergedRow;
                for(int w = 0;
                    w < originMap.width;
                    w++)
                {
                    uint8 id = *pixel;
                    uint8 coarseID = 0;
                        
                    assert(id < parentLookupTable.size());
                        
                    if(id < parentLookupTable.size())
                    {
                        coarseID = parentLookupTable[id];
                    }

                    uint8* gChannel = mergedPixel;
                    *gChannel = coarseID;
                                        
                    pixel += originMap.bytesPerPixel;
                    mergedPixel += coarseMap.bytesPerPixel;
                }
                    
                row+= originMap.pitch;
                mergedRow += coarseMap.pitch;
            }
        }
        
        return (coarseIDCube);
    }

    GenerateRegionSDFResult
    GeneratePlanetRegionSDF(BitmapCube& regionIDCube,
                            const std::vector<int>& parentLookupTable)
    {
        GenerateRegionSDFResult result = {};

        //NOTE:generate coarse id map
        {
            BitmapCube coarseIDCube = {};
            BitmapCube& mergedIDCube = result.mergedIDCube;

            for(int i = 0;
                i < 6;
                i++)
            {
                Bitmap& coarseMap  = coarseIDCube.maps[i];
                Bitmap& originMap = regionIDCube.maps[i];
                Bitmap& mergedMap = mergedIDCube.maps[i];
                
                coarseMap.width = originMap.width;
                coarseMap.height = originMap.height;
                coarseMap.bytesPerPixel = originMap.bytesPerPixel;

                coarseMap.pitch = coarseMap.width * coarseMap.bytesPerPixel;
                coarseMap.pixels = malloc(coarseMap.pitch * coarseMap.height);

                mergedMap.width = originMap.width;
                mergedMap.height = originMap.height;
                mergedMap.bytesPerPixel = originMap.bytesPerPixel +
                    coarseMap.bytesPerPixel;

                mergedMap.pitch = mergedMap.width * mergedMap.bytesPerPixel;
                mergedMap.pixels = malloc(mergedMap.pitch * mergedMap.height);

                uint8* row = (uint8*)originMap.pixels;
                uint8* coarseRow = (uint8*)coarseMap.pixels;
                uint8* mergedRow =  (uint8*)mergedMap.pixels;
                for(int h = 0;
                    h < originMap.height;
                    h++)
                {
                    uint8* pixel = row;
                    uint8* coarsePixel = coarseRow;
                    uint8* mergedPixel = mergedRow;
                    for(int w = 0;
                        w < originMap.width;
                        w++)
                    {
                        uint8 id = *pixel;
                        uint8 coarseID = 0;
                        
                        assert(id < parentLookupTable.size());
                        
                        if(id < parentLookupTable.size())
                        {
                            coarseID = parentLookupTable[id];
                        }

                        *coarsePixel = coarseID;

                        uint8* rChannel = mergedPixel;
                        uint8* gChannel = mergedPixel + 1;
                        *rChannel = id;
                        *gChannel = coarseID;
                        
                        pixel += originMap.bytesPerPixel;
                        coarsePixel += coarseMap.bytesPerPixel;
                        mergedPixel += mergedMap.bytesPerPixel;
                    }
                    
                    row+= originMap.pitch;
                    coarseRow+=coarseMap.pitch;
                    mergedRow += mergedMap.pitch;
                }
            }

            result.coarseSDFCube = generateSDFFromCube(coarseIDCube);
            //NOTE:Free temparory cube memeory.
            for(int i = 0;
                i < 6;
                i++)
            {
                Bitmap& coarseMap  = coarseIDCube.maps[i];
                free(coarseMap.pixels); 
            }
        }

        result.sdfCube = generateSDFFromCube(regionIDCube);

        return (result);
    }

    GenerateRegionSDFResultPlane GeneratePlanetRegionSDFWarp(const Bitmap& regionMap,
                                                             const std::vector<int>& parentLookupTable)
    {
	    GenerateRegionSDFResultPlane result = {};

	    Bitmap coarseMap      = {};
	    const Bitmap& fineMap = regionMap;
	    Bitmap& mergedMap     = result.mergedId;

	    coarseMap.width         = fineMap.width;
	    coarseMap.height        = fineMap.height;
	    coarseMap.bytesPerPixel = fineMap.bytesPerPixel;

	    coarseMap.pitch  = coarseMap.width * coarseMap.bytesPerPixel;
	    coarseMap.pixels = std::malloc(coarseMap.pitch * coarseMap.height);

	    mergedMap.width         = fineMap.width;
	    mergedMap.height        = fineMap.height;
	    mergedMap.bytesPerPixel = fineMap.bytesPerPixel + coarseMap.bytesPerPixel;

	    mergedMap.pitch  = mergedMap.width * mergedMap.bytesPerPixel;
	    mergedMap.pixels = std::malloc(mergedMap.pitch * mergedMap.height);

	    auto row       = static_cast<uint8*>(fineMap.pixels);
	    auto coarseRow = static_cast<uint8*>(coarseMap.pixels);
	    auto mergedRow = static_cast<uint8*>(mergedMap.pixels);
	    for (int h = 0; h < fineMap.height; h++)
	    {
		    const uint8* pixel = row;
		    uint8* coarsePixel = coarseRow;
		    uint8* mergedPixel = mergedRow;
		    for (int w = 0;
		         w < fineMap.width;
		         w++)
		    {
			    const uint8 id = *pixel;
			    uint8 coarseId = 0;

			    assert(id < parentLookupTable.size());

			    if (id < parentLookupTable.size())
			    {
				    coarseId = static_cast<uint8>(parentLookupTable[id]);
			    }

			    *coarsePixel = coarseId;

			    uint8* rChannel = mergedPixel;
			    uint8* gChannel = mergedPixel + 1;
			    *rChannel       = id;
			    *gChannel       = coarseId;

			    pixel += fineMap.bytesPerPixel;
			    coarsePixel += coarseMap.bytesPerPixel;
			    mergedPixel += mergedMap.bytesPerPixel;
		    }

		    row += fineMap.pitch;
		    coarseRow += coarseMap.pitch;
		    mergedRow += mergedMap.pitch;
	    }

	    result.coarseSdf = generateSDFWarp(coarseMap);
	    coarseMap.freeMemory();

	    result.fineSdf = generateSDFWarp(fineMap);

	    return result;
    }

    TexturePtr
    PlanetManager::asyncLoadTexture(const Name& path)
    {
        EchoZoneScoped;

        auto createResult = TextureManager::instance()->createOrRetrieve(path);
        TexturePtr tex = createResult.first;
        
        if(!(tex->isPrepared() || tex->isLoaded()))
        {
            {
                Name request = path;
                mWorkQueue->addRequest(mWorkQueueChannel, 0, Any(request));
            }   
        }
		return (tex);
    }

    WorkQueue::Response*
    PlanetManager::handleRequest(const WorkQueue::Request* req,
                                 const WorkQueue* srcQ)
    {
        EchoZoneScoped;

        WorkQueue::Response *response = 0;
        const Name* texPath = any_cast<Name>(&req->getData());
        if (!req->getAborted())
        {
            TextureManager::instance()->prepareTexture(*texPath);
            response = new WorkQueue::Response(req, true, Any(*texPath), "", texPath->toString());
        }

        return response;
    }

    void PlanetManager::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
    {
        EchoZoneScoped;

        if (!res->succeeded())
            return;

        const WorkQueue::Request *req = res->getRequest();
        const Name* texPath = any_cast<Name>(&req->getData());
        if (!req->getAborted())
        {
            TextureManager::instance()->loadTexture(*texPath);
        }   
    }

    void PlanetManager::OutPutPrepareResourceLog(DataStreamPtr& out)
    {
		std::vector<WorkQueue::Request*> requestLists;
		mWorkQueue->getRequestList(mWorkQueueChannel, requestLists);

		stringstream ss;
		ss << "WorkQueueChannel : " << "PlanetWarfogTexture" << "\n";
		for (const auto& req : requestLists)
		{
			if (req == nullptr || req->getAborted()) continue;

			Any pData = req->getData();

			if (pData.isEmpty()) continue;

			const Name* texPath = any_cast<Name>(&pData);
			ss << "Request Type : Texture\t" << "Resource Name : " << (*texPath) << "\n";
			
		}
		out->write(ss.str().c_str(), ss.str().size());
    }

    void
    PlanetManager::freeGlobalSphereGeometryResource()
    {
        auto renderSystem = Root::instance()->getRenderSystem();
        for(auto& ite : s_sphereVertexBuffers)
        {
            if(ite.second)
            {
                renderSystem->destoryBuffer(ite.second);
            }
        }

        WarfogRenderable::s_material.setNull();
        PlanetLowLODCloud::s_material.setNull();
		PlanetPlotDivision::s_material.setNull();
		WarfogRenderable::s_TorusMaterial.setNull();
    }
    
    void
    PlanetManager::getSphereGeometryBuffer(int gempType,
                                           std::vector<PlanetVertex>& outVertices)
    {
//#define Profile_Time 0
#if Profile_Time
        unsigned long updateTime;

        Timer* timer = Root::instance()->getTimer();
        updateTime = timer->getMicroseconds();
#endif
        const int sliceX = 6;
        const int GRID_RESOLUTION = SphericalTerrainQuadTreeNode::GRID_RESOLUTION;

        //IMPORTANT:Array[0] as default gemp type.
        TerrainGenerator* generator = 0;
        generator = TerrainGeneratorLibrary::instance()->get((uint32)gempType);
        
        {
            outVertices.resize(static_cast<size_t>(sliceX) * VERTEX_COUNT);

            for (int slice = 0;
                 slice < sliceX;
                 ++slice)
            {
                std::vector<Vector3> points;
                {
                    constexpr float gridSize = (float)SphericalTerrainQuadTreeNode::getGridSizeOnCube(0);
                    points =
                        SphericalTerrainQuadTreeNode::getSphericalPositionsUniform(
                            slice,
                            GRID_RESOLUTION + 2,
                            GRID_RESOLUTION + 2,
                            Vector2(gridSize),
                            Vector2(-1.0f - gridSize),
                            generator);
                }

                auto nor = SphericalTerrain::computeNormal(GRID_RESOLUTION,
                                                           GRID_RESOLUTION,
                                                           points);

                std::vector<Vector3> pos(VERTEX_COUNT);
				for (int i = 0; i < GRID_RESOLUTION; ++i)
				{
					for (int j = 0; j < GRID_RESOLUTION; ++j)
                    {
                        const int idx   = i * GRID_RESOLUTION + j;
						const auto curr = points[(i + 1) * (GRID_RESOLUTION + 2) + j + 1];
                        pos[idx]        = curr;
                    }
                }
                
                for (int i = 0; i < VERTEX_COUNT; ++i)
                {
                    Vector2 xy;
                    SphericalTerrainQuadTreeNode::Face face = slice;
                    SphericalTerrainQuadTreeNode::MapToCube_withFace(pos[i],
                                                                     face,
                                                                     xy);
                    Vector3 uv;
                    uv.x = (xy.x + 1.0f) *0.5f;
                    uv.y = (xy.y + 1.0f) *0.5f;
                    uv.z = (float)slice;
                    auto& v = outVertices[VERTEX_COUNT * slice + i];
                    v.pos = pos[i];
                    v.normal = nor[i];
                    v.uv = uv;
                }
            }
        }
#if Profile_Time
        updateTime = timer->getMicroseconds() - updateTime;
        String log = 
            String("generate sphere geometry time ") +
            std::to_string(updateTime) +String(" micro second ") +
            std::to_string(updateTime / 1000) +String(" ms\n");
        EchoLogToConsole(log.c_str());
#endif
    }

    void
    PlanetManager::getTorusGeometry(float relativeMinorRadius,
                                    float elevationMax,
                                    std::vector<PlanetVertex>& outVertices)
    {
#if Profile_Time
        unsigned long updateTime;

        Timer* timer = Root::instance()->getTimer();
        updateTime = timer->getMicroseconds();
#endif
        const int sliceX = 3;

        constexpr int GRID_RESOLUTION                           = SphericalTerrainQuadTreeNode::GRID_RESOLUTION;
        constexpr SphericalTerrainQuadTreeNode::IndexType QUADS = GRID_RESOLUTION - 1;

        //TODO:Using global variable.
        {
            outVertices.resize(static_cast<size_t>(sliceX) * VERTEX_COUNT);

            for (int slice = 0;
                 slice < sliceX;
                 ++slice)
            {
                std::vector<Vector3> points;
                {
                    const Vector2 gridSize = 1.0f / Vector2(static_cast<float>(QUADS * sliceX), static_cast<float>(QUADS));
                
                    points =
                        TorusTerrainNode::getTorusPositions(GRID_RESOLUTION + 2,
                                                            GRID_RESOLUTION + 2,
                                                            relativeMinorRadius * (1.0f + elevationMax),
                                                            gridSize,
                                                            Vector2(static_cast<float>(slice) / sliceX, 0.0f) - gridSize);
                }
                
                std::vector<Vector2> uv(VERTEX_COUNT);
                {
                    const auto uvOffset = Vector2(static_cast<float>(slice) / sliceX, 0.0f);
                    const Vector2 uvStep = 1.0f / Vector2(static_cast<float>(QUADS * sliceX), static_cast<float>(QUADS));
                    for (int i = 0; i < GRID_RESOLUTION; ++i)
                    {
                        for (int j = 0; j < GRID_RESOLUTION; ++j)
                        {
                            const int idx = i * GRID_RESOLUTION + j;
                            uv[idx]       = uvOffset + uvStep * Vector2(static_cast<float>(j), static_cast<float>(i));
                        }
                    }
                }

                auto nor = SphericalTerrain::computeNormal(GRID_RESOLUTION, GRID_RESOLUTION, points);

                std::vector<Vector3> pos(VERTEX_COUNT);
				for (int i = 0; i < GRID_RESOLUTION; ++i)
				{
					for (int j = 0; j < GRID_RESOLUTION; ++j)
                    {
                        const int idx   = i * GRID_RESOLUTION + j;
						const auto curr = points[(i + 1) * (GRID_RESOLUTION + 2) + j + 1];
                        pos[idx]        = curr;
                    }
                }
                
                for (int i = 0; i < VERTEX_COUNT; ++i)
                {
                    auto& v = outVertices[VERTEX_COUNT * slice + i];
                    v.pos = pos[i];
                    v.normal = nor[i];
                    v.uv = Vector3(uv[i].x, uv[i].y, 0);
                }
            }
        }

#if Profile_Time
        updateTime = timer->getMicroseconds() - updateTime;
        String log = 
            String("generate torus geometry time ") +
            std::to_string(updateTime) +String(" micro second ") +
            std::to_string(updateTime / 1000) +String(" ms\n");
        EchoLogToConsole(log.c_str());
#endif
    }

    void AsyncSphereGenerator::CommonTaskFinish(uint64 requestID)
    {
        assert(!PlanetManager::isBufferReady(gempType) &&
               "global vertex has been exist!");
        if(!PlanetManager::isBufferReady(gempType) &&
           !tempVertices.empty())
        {
            int dataSize = (int)(tempVertices.size() * sizeof(PlanetVertex));
            auto renderSystem = Root::instance()->getRenderSystem();
            vertexBuffer = renderSystem->createStaticVB(tempVertices.data(),
                                                        dataSize);
            PlanetManager::s_sphereVertexBuffers[gempType] = vertexBuffer;
        }

        delete(this);
    }
    
    void AsyncSphereGenerator::Execute()
    {
        PlanetManager::getSphereGeometryBuffer(gempType, tempVertices);
    }

    void AsyncTorusGenerator::CommonTaskFinish(uint64 requestID)
    {
        if(!tempVertices.empty())
        {
            int dataSize = (int)(tempVertices.size() * sizeof(PlanetVertex));
            auto renderSystem = Root::instance()->getRenderSystem();
			vertexBuffer = renderSystem->createStaticVB(tempVertices.data(),
                                                        dataSize);   
        }

        if(listener)
        {
            listener->taskFinish();
        }
    }
    
    void AsyncTorusGenerator::Execute()
    {
        PlanetManager::getTorusGeometry(relativeMinorRadius, elevationMax, tempVertices);
    }

    void PlanetManager::initGlobalSphereGeometryResource()
    {
        
    }

    bool PlanetManager::isBufferStartedGenerated(int gempType)
    {
        bool result = false;
        if(s_bufferStartGenerated.find(gempType) != s_bufferStartGenerated.end())
        {
            result = true;
        }
        return (result);
    }
    
    bool PlanetManager::isBufferReady(int gempType)
    {
        bool result = false;
        if(s_sphereVertexBuffers.find(gempType) != s_sphereVertexBuffers.end())
        {
            result = true;
        }
        return (result);   
    }

    RcBuffer* PlanetManager::getBuffer(int gempType)
    {
        RcBuffer* result = 0;
        if(s_sphereVertexBuffers.find(gempType) != s_sphereVertexBuffers.end())
        {
            result = s_sphereVertexBuffers[gempType];
        }
        return (result);
    }

    AxisAlignedBox PlanetManager::calculateBounds(const Vector3 bezier[4])
    {
	    return TerrainModify::CalculateBounds(bezier);
    }

    void
    PlanetManager::setSphereBakeTextureParam(const SphereBakeTextureParam& param)
    {
        s_sphereParam = param;
    }
    
    uint32
    PlanetManager::getSphereBakeTextureResolution(float planetRadius)
    {
        uint32 result;
        if(planetRadius <= s_sphereParam.lowMaxRadius)
        {
            result = s_sphereParam.lowResolution.width;
        }
        else if(planetRadius <= s_sphereParam.middleMaxRadius)
        {
            result = s_sphereParam.middleResolution.width;
        }
        else
        {
            result = s_sphereParam.highResolution.width;
        }
        return (result);
    }



    void
    PlanetManager::setTorusBakeTextureParam(const TorusBakeTextureParam& param)
    {
        s_torusParam = param;
    }
    
    uint32
    PlanetManager::getTorusBakeTextureResolution(float radius)
    {
        uint32 result;
        if(radius <= s_torusParam.lowMaxRadius)
        {
            result = s_torusParam.lowResolution.width;
        }
        else if(radius <= s_torusParam.middleMaxRadius)
        {
            result = s_torusParam.middleResolution.width;
        }
        else
        {
            result = s_torusParam.highResolution.width;
        }
        return (result);
    }

    float
    PlanetManager::getSphereHlodRadius(float planetRadius)
    {
        float result;
        if(planetRadius <= s_sphereParam.lowMaxRadius)
        {
            result = s_sphereParam.lowResolution.hlodRadius;
        }
        else if(planetRadius <= s_sphereParam.middleMaxRadius)
        {
            result = s_sphereParam.middleResolution.hlodRadius;
        }
        else
        {
            result = s_sphereParam.highResolution.hlodRadius;
        }

        //NOTE:At leaset one radius distance.
        if(result < 0.0f)
        {
            result = 0.0f;
        }
        
        return (result);
    }

    float
    PlanetManager::getTorusHlodRadius(float radius)
    {
        float result;
        if(radius <= s_torusParam.lowMaxRadius)
        {
            result = s_torusParam.lowResolution.hlodRadius;
        }
        else if(radius <= s_torusParam.middleMaxRadius)
        {
            result = s_torusParam.middleResolution.hlodRadius;
        }
        else
        {
            result = s_torusParam.highResolution.hlodRadius;
        }

        //NOTE:At leaset one radius distance.
        if(result < 0.0f)
        {
            result = 0.0f;
        }
        
        return (result);
    }
}//namespace 
