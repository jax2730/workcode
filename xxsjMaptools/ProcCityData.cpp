#include <limits>
#include <memory>
#include <map>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <Vector>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <unordered_set>
#include "FreeImage/FreeImage.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#ifdef _DEBUG
constexpr uint8_t indent = 4;
#else
constexpr uint8_t indent = -1;
#endif

extern std::string g_ResPath;

void ConvertWorldHeightMapData(const std::string& originalPath, const std::string& resPath, bool highResolution, float KarstSize = 32768.0f, int targetPixPerPage = 256);

void CalculateHeightMapDifference(const std::string& terrainHMPath, const std::string& cityHMPath, const std::string& differenceMaskPath) {
	FILE* terrainHMFile = nullptr;
	FILE* cityHMFile = nullptr;
	FILE* differenceMaskFile = nullptr;
	fopen_s(&terrainHMFile, terrainHMPath.c_str(), "rb");
	fopen_s(&cityHMFile, cityHMPath.c_str(), "rb");
	fopen_s(&differenceMaskFile, differenceMaskPath.c_str(), "wb");

	unsigned int HeightMapFileMagicID;

	fread(&HeightMapFileMagicID, sizeof(unsigned int), 1, terrainHMFile);
	fread(&HeightMapFileMagicID, sizeof(unsigned int), 1, cityHMFile);

	int terrainHMsrcSize = 0;
	int cityHMsrcSize = 0;
	fread(&terrainHMsrcSize, sizeof(int), 1, terrainHMFile);
	fread(&cityHMsrcSize, sizeof(int), 1, cityHMFile);
	if (terrainHMsrcSize != cityHMsrcSize) {
		return;
	}

	int terrainHMtargetSize = 0;
	int cityHMtargetSize = 0;
	fread(&terrainHMtargetSize, sizeof(int), 1, terrainHMFile);
	fread(&cityHMtargetSize, sizeof(int), 1, cityHMFile);
	if (terrainHMtargetSize != cityHMtargetSize) {
		return;
	}

	if (cityHMsrcSize != cityHMtargetSize) {
		return;
	}

	int size = terrainHMtargetSize + 3;

	std::vector<uint32_t> terrainHMData(size * size);
	std::vector<uint32_t> cityHMData(size * size);

	fread(terrainHMData.data(), sizeof(uint32_t), size * size, terrainHMFile);
	fread(cityHMData.data(), sizeof(uint32_t), size * size, cityHMFile);

	fclose(terrainHMFile);
	fclose(cityHMFile);


	std::vector<int32_t> differenceMaskData(size * size);
	for (int i = 0; i < size * size; i++) {
		differenceMaskData[i] = static_cast<int32_t>(cityHMData[i]) - static_cast<int32_t>(terrainHMData[i]);
	}

	fwrite(differenceMaskData.data(), sizeof(uint8_t), size * size, differenceMaskFile);
	fclose(differenceMaskFile);
#ifdef _DEBUG
	FIBITMAP* diffImg = FreeImage_AllocateT(FIT_INT32, size, size, 32);
	if (!diffImg) {
		return;
	}

	for (int y = 0; y < size; y++) {
		int32_t* pLine = (int32_t*)FreeImage_GetScanLine(diffImg, y);

		for (int x = 0; x < size; x++) {
			pLine[x] = differenceMaskData[(size - y - 1) * size + x];
		}
	}
	// for (int i = 0; i < size * size; i++) {
	// 	FreeImage_SetPixelIndex(diffImg, i % size, size - (i / size) - 1, &differenceMaskData[i]);
	// }
	std::string differenceTIFFPath = differenceMaskPath + ".tiff";
	FreeImage_Save(FIF_TIFF, diffImg, differenceTIFFPath.c_str(), 0);
	FreeImage_Unload(diffImg);
#endif
}

void ConvertCityData(const std::filesystem::path& cityDir, const std::filesystem::path& cityIndexPath) {
	// iterate over all *.tif files in the directory
	std::string cityName = cityDir.filename().string();
	nlohmann::json cityIndexJson;
	if (std::filesystem::exists(cityIndexPath)) {
		std::ifstream cityIndexFile(cityIndexPath);
		cityIndexFile >> cityIndexJson;
		cityIndexFile.close();
	}
	else {
		cityIndexJson["cities"] = nlohmann::json::array();
		cityIndexJson["tiles"] = nlohmann::json::array();
	}
	auto& citiesListJson = cityIndexJson["cities"];
	if (!citiesListJson.is_array()) {
		return;
	}
	auto& tiles = cityIndexJson["tiles"];
	if (!tiles.is_array()) {
		return;
	}

	{
		std::unordered_set<std::string> citiesList = citiesListJson.get<std::unordered_set<std::string>>();
		if (std::find(citiesList.begin(), citiesList.end(), cityName) != citiesList.end()) {
			// erase tile with empty cities
			for (int i = static_cast<int>(tiles.size()) - 1; i >= 0; --i) {
				auto& ctitiesJson = tiles[i]["cities"];
				if (!ctitiesJson.is_array()) {
					continue;
				}
				std::unordered_set<std::string> ctities = ctitiesJson.get<std::unordered_set<std::string>>();
				if (ctities.find(cityName) != ctities.end()) {
					ctities.erase(cityName);
					if (ctities.empty()) {
						tiles.erase(tiles.begin() + i);
					}
					else {
						ctitiesJson = nlohmann::json(ctities);
					}
				}
			}
		}
		else {
			citiesListJson.emplace_back(cityName);
		}
	}

	for (const auto& entry : std::filesystem::directory_iterator(cityDir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".tif") {
			std::string filename = entry.path().filename().string();
			std::string withoutExt = filename.substr(0, filename.find_last_of("."));
			int col = -1;
			int row = -1;
			size_t pos = withoutExt.find_last_of("_");
			if (pos != std::string::npos) {
				col = atoi(withoutExt.substr(pos + 1).c_str()) - 1;
				withoutExt = withoutExt.substr(0, pos);
				pos = withoutExt.find_last_of("_");
				if (pos != std::string::npos) {
					row = atoi(withoutExt.substr(pos + 1).c_str()) - 1;
					withoutExt = withoutExt.substr(0, pos);
				}
			}
			if (col == -1 || row == -1) {
				continue;
			}
			std::string heightPath = (cityDir / filename).string();
			std::string resPath = (cityDir / ("city" + std::to_string(col) + "_" + std::to_string(row) + ".hm")).string();
			std::string diffPath = (cityDir / ("city" + std::to_string(col) + "_" + std::to_string(row) + ".diff")).string();
			std::string terrainPath = (std::filesystem::path(g_ResPath) / ("world" + std::to_string(col) + "_" + std::to_string(row) + ".hm")).string();

			ConvertWorldHeightMapData(heightPath, resPath, true);
			CalculateHeightMapDifference(terrainPath, resPath, diffPath);

			bool has_tile = false;
			for (auto& tile : tiles) {
				const auto& tileIndex = tile["index"];
				if (tileIndex[1] == row && tileIndex[0] == col) {
					auto& ctities = tile["cities"];
					ctities.push_back(cityName);
					has_tile = true;
					break;
				}
			}
			if (!has_tile) {
				std::vector<int> index = { col, row };
				std::vector<std::string> cities = { cityName };
				nlohmann::json tile;
				tile["index"] = index;
				tile["cities"] = cities;
				tiles.push_back(tile);
			}
		}
	}

	{
		std::ofstream cityIndexFile(cityIndexPath);
		cityIndexFile << cityIndexJson.dump(indent) << std::endl;
		cityIndexFile.close();
	}
}
