///////////////////////////////////////////////////////////////////////////////
//
// @file EchoKarstSystem.cpp
// @author luoshuquan
// @date 2024/07/19 Friday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "EchoStableHeaders.h"
#include "EchoKarstSystem.h"
#include "EchoKarstGroup.h"
#include "EchoKarstResourceManager.h"
#include "EchoKarstMaterials.h"

namespace Echo
{
	KarstSystem::KarstSystem()
	{

	}

	KarstSystem::~KarstSystem()
	{
		
	}

	std::string getResourcePath(const std::string& levelName)
	{
		std::string jsonPath = "levels/" + levelName + "/karst_resource.json";
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonPath.c_str()));

		std::string levelPath = "";
		if (pDataStream.isNull())
			return levelPath;

		size_t dataSize = pDataStream->size();

		if (dataSize > 0)
		{
			char* pData = new char[dataSize + 1];
			memset(pData, 0, dataSize + 1);
			pDataStream->read(pData, dataSize);
			cJSON* rootNode = cJSON_Parse(pData);

			ReadJSNode(rootNode, "ResourceLevelPath", levelPath);
		}

		return levelPath;
	}

	void LoadSurfaceIDResource(const std::string& resPath, uint32 rowNum, uint32 colNum, uint32 dataPerPage, KarstData<uint8>& surfaceIDData, bool surfaceTile = false)
	{
		if (surfaceTile)
		{
			// 使用分块瓦片格式读取 Surface ID 数据
			DataStreamPtr inputStream(Root::instance()->GetFileDataStream(resPath.c_str()));
			if (inputStream.isNull()) {
				printf("Failed to open surface tile file: %s\n", resPath.c_str());
				return;
			}

			// 读取文件头部
			int totalTiles, tileRows, tileCols;
			inputStream->readData(&totalTiles);
			inputStream->readData(&tileRows);
			inputStream->readData(&tileCols);

			// 初始化数据结构
			surfaceIDData.resize(colNum);
			for (uint32 j = 0; j < colNum; j++)
			{
				surfaceIDData[j].resize(rowNum);
			}

			while (!inputStream->eof()) {
				// 尝试读取瓦片头部
				int tileNumber, row, col, width, height;
				
				// 检查是否还有足够的数据读取瓦片头部
				if (inputStream->size() - inputStream->tell() < sizeof(int) * 5) {
					break;
				}
				
				inputStream->readData(&tileNumber);
				inputStream->readData(&row);
				inputStream->readData(&col);
				inputStream->readData(&width);
				inputStream->readData(&height);

				// 确保瓦片位置在有效范围内
				if (row >= 0 && row < (int)rowNum && col >= 0 && col < (int)colNum) {
					// 读取瓦片数据
					size_t dataSize = width * height;
					surfaceIDData[col][row].resize(dataSize);
					inputStream->read(surfaceIDData[col][row].data(), dataSize);
				}
				else {
					// 跳过无效瓦片的数据
					size_t dataSize = width * height;
					inputStream->skip(dataSize);
				}
			}
		}
		else
		{
			// 原有的读取方式
			DataStreamPtr texStream(Root::instance()->GetFileDataStream(resPath.c_str()));

			uint32 width, height;
			texStream->readData(&width);
			texStream->readData(&height);

			int rowPitch = (dataPerPage + 1) * sizeof(uint8);
			int depthPitch = rowPitch * (dataPerPage + 1);

			int blockSize = (dataPerPage + 1) * (dataPerPage + 1);
			surfaceIDData.resize(colNum);
			for (uint32 j = 0; j < colNum; j++)
			{
				surfaceIDData[j].resize(rowNum);
				for (uint32 i = 0; i < rowNum; i++)
				{
					surfaceIDData[j][i].resize(blockSize);
					texStream->readData(surfaceIDData[j][i].data(), depthPitch);
				}
			}
		}
	}

	void LoadAreaIDResource(const std::string& basePath, std::string suffix, uint32 rowNum, uint32 colNum, uint32 dataPerPage, KarstData<uint32>& areaIDData, bool surfaceTile = false)
	{
		if (surfaceTile)
		{
			// 使用分块瓦片格式读取 Area ID 数据
			std::string resPath = basePath + suffix;
			DataStreamPtr inputStream(Root::instance()->GetFileDataStream(resPath.c_str()));
			if (inputStream.isNull()) {
				printf("Failed to open area tile file: %s\n", resPath.c_str());
				return;
			}

			// 读取文件头部
			int totalTiles, tileRows, tileCols;
			inputStream->readData(&totalTiles);
			inputStream->readData(&tileRows);
			inputStream->readData(&tileCols);

			// 初始化数据结构
			areaIDData.resize(colNum);
			for (uint32 j = 0; j < colNum; j++)
			{
				areaIDData[j].resize(rowNum);
			}

			while (!inputStream->eof()) {
				// 尝试读取瓦片头部
				int tileNumber, row, col, width, height;
				
				// 检查是否还有足够的数据读取瓦片头部
				if (inputStream->size() - inputStream->tell() < sizeof(int) * 5) {
					break;
				}
				
				inputStream->readData(&tileNumber);
				inputStream->readData(&row);
				inputStream->readData(&col);
				inputStream->readData(&width);
				inputStream->readData(&height);

				// 确保瓦片位置在有效范围内
				if (row >= 0 && row < (int)rowNum && col >= 0 && col < (int)colNum) {
					// 读取瓦片数据 (uint32 类型)
					size_t dataSize = width * height * sizeof(uint32);
					areaIDData[col][row].resize(width * height);
					inputStream->read(areaIDData[col][row].data(), dataSize);
				}
				else {
					// 跳过无效瓦片的数据
					size_t dataSize = width * height * sizeof(uint32);
					inputStream->skip(dataSize);
				}
			}
		}
		else
		{
			// 原有的读取方式
			int rowPitch = dataPerPage * sizeof(uint32);
			int depthPitch = rowPitch * dataPerPage;
			int blockSize = (dataPerPage) * (dataPerPage);
			areaIDData.resize(colNum);
			for (uint32 j = 0; j < colNum; j++)
			{
				areaIDData[j].resize(rowNum);
				for (uint32 i = 0; i < rowNum; i++)
				{
					auto areaResPath = basePath + std::to_string(i) + "_" + std::to_string(j) + suffix;
					DataStreamPtr areaStream(Root::instance()->GetFileDataStream(areaResPath.c_str()));
					if (!areaStream.isNull()) {
						areaIDData[j][i].resize(blockSize);
						areaStream->read(areaIDData[j][i].data(), depthPitch);
					}
				}
			}
		}
	}

	void LoadMaskTextureResource(const std::string& basePath, const std::string& suffix, uint32 rowNum, uint32 colNum, uint32 dataPerPage, KarstData<uint8>& areaIDData)
	{
		int rowPitch = dataPerPage * sizeof(uint8);
		int depthPitch = rowPitch * dataPerPage;

		areaIDData.resize(colNum);

		for (int j = 0; j < (int)colNum; j++)
		{
			areaIDData[j].resize(rowNum);
			for (int i = 0; i < (int)rowNum; i++)
			{
				areaIDData[j][i].resize(depthPitch);
				auto resPath = basePath + std::to_string(i) + "_" + std::to_string(j) + suffix;
				DataStreamPtr maskStream(Root::instance()->GetFileDataStream(resPath.c_str()));
				maskStream->read(areaIDData[j][i].data(), depthPitch);
			}
		}
	}

	KarstGroup* KarstSystem::createGroup(const String& name, SceneManager* sm, const KarstParams& params, bool lite)
	{
		if (auto it = mKarstGroupMap.find(name); it != mKarstGroupMap.end())
			return it->second;

		std::string jsonPath = "levels/" + params.levelName + "/karst_resource.json";
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonPath.c_str()));

		std::string resourcePath = "";
		if (pDataStream.isNull())
			return nullptr;

		size_t dataSize = pDataStream->size();

		if(!dataSize)
			return nullptr;

		std::vector<char> pData(dataSize + 1, 0);

		pDataStream->read(pData.data(), dataSize);
		cJSON* rootNode = cJSON_Parse(pData.data());

		ReadJSNode(rootNode, "ResourceLevelPath", resourcePath);
		
		if (resourcePath == "")
			return nullptr;

		auto it = mCommonData.find(resourcePath);
		std::shared_ptr<KarstCommonData> commonData;
	
		if (it != mCommonData.end())
		{
			commonData = it->second;
			mCommonDataCount[resourcePath]++;
		}
		else
		{
			commonData = std::make_shared<KarstCommonData>();

			std::string basePath = "levels/" + resourcePath;

			bool isCompress;
			if (!ReadJSNode(rootNode, "CompressResource", isCompress)) {
				isCompress = false;
			}

			bool surfaceTile;
			if (!ReadJSNode(rootNode, "SurfaceMaskTile", surfaceTile)) {
				surfaceTile = false;
			}
			bool areaIDTile;
			if (!ReadJSNode(rootNode, "AreaIDMaskTile", areaIDTile)) {
				areaIDTile = false;
			}
			cJSON* resourceNode = cJSON_GetObjectItem(rootNode, "KarstResource");

			cJSON* node = cJSON_GetObjectItem(resourceNode, "SurfaceIDMap");
			if (node) {
				String resPath, fileName, suffix;
				ReadJSNode(node, "RelativePath", resPath);
				ReadJSNode(node, "FileName", fileName);
				ReadJSNode(node, "Suffix", suffix);
				uint32 surfaceIDPerPage;
				ReadJSNode(resourceNode, "SurfaceIDPerPage", surfaceIDPerPage);
				LoadSurfaceIDResource(basePath + resPath + fileName + suffix, params.row, params.col, surfaceIDPerPage, commonData->surfaceIDData, surfaceTile);
			}

			node = cJSON_GetObjectItem(resourceNode, "AreaIDTile");
			if (node) {
				String resPath, fileName, suffix;
				ReadJSNode(node, "RelativePath", resPath);
				ReadJSNode(node, "FileName", fileName);
				ReadJSNode(node, "Suffix", suffix);
				uint32 areaIDPerPage;
				ReadJSNode(resourceNode, "AreaIDPerPage", areaIDPerPage);
				LoadAreaIDResource(basePath + resPath + fileName, suffix, params.row, params.col, areaIDPerPage, commonData->areaIDData, areaIDTile);
			}

			if (!isCompress) {
				node = cJSON_GetObjectItem(resourceNode, "MaskMapTile");
				if (node) {
					String resPath, fileName, suffix;
					ReadJSNode(node, "RelativePath", resPath);
					ReadJSNode(node, "FileName", fileName);
					ReadJSNode(node, "Suffix", suffix);
					uint32 maskIDPerPage;
					ReadJSNode(resourceNode, "MaskPixPerPage", maskIDPerPage);
					LoadMaskTextureResource(basePath + resPath + fileName, suffix, params.row, params.col, maskIDPerPage, commonData->maskData);
				}
			}

			mCommonData.insert(std::make_pair(resourcePath, commonData));
			mCommonDataCount[resourcePath] = 1;
		}
		
		if (lite) {
			if (!mMiniKarst)
				mMiniKarst = new KarstGroup(name, sm, params, KarstRenderType::VirtualTexture, commonData);
			return mMiniKarst;
		}

		auto group = new KarstGroup(name, sm, params, KarstRenderType::VirtualTexture, commonData);
		mKarstGroupMap.insert(std::make_pair(name, group));

		for (auto& callback : mCreatedListeners)
		{
			callback(group);
		}

		return group;
	}

	void KarstSystem::destroyGroup(KarstGroup* karstGroup)
	{
		if (!karstGroup || !mKarstGroupMap.size())
			return;

		if (auto it = mKarstGroupMap.find(karstGroup->getName()); it != mKarstGroupMap.end()) {

			for (auto& callback : mDestroyedListeners)
			{
				callback(it->second);
			}

			auto resourcePath = it->second->getResourceLevelPath();

			delete it->second;

			mKarstGroupMap.erase(it);

			mCommonDataCount[resourcePath]--;

			if (mCommonDataCount[resourcePath] == 0) {
				mCommonData.erase(mCommonData.find(resourcePath));
			}
		}
	}

	bool KarstSystem::getHeightAtLocalPos(const Vector2& pos, float& height)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getHeightFromPosition(pos, height)) {		
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getHeightAtWorldPos(const DVector3& pos, float& height)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getHeightFromWorldPosition(pos, height)) {
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getNormalAtLocalPos(const Vector2& pos, Vector3& normal)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getNormalFromPosition(pos, normal)) {
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getNormalAtWorldPos(const DVector3& pos, Vector3& normal)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getNormalFromWorldPosition(pos, normal)) {
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getSurfaceTypeAtLocalPos(const Vector2& pos, uint8& typeID)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getSurfaceID(pos, typeID)) {
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getSurfaceTypeAtWorldPos(const DVector3& pos, uint8& typeID)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			auto localPos = Root::instance()->toLocal(pos);
			if (it->second->getSurfaceID(Vector2((float)localPos.x, (float)localPos.z), typeID)) {
				return true;
			}
		}
		return false;
	}

	bool KarstSystem::getAreaIDAtLocalPos(const Vector2& pos, uint32& areaID)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getAreaID(pos, areaID)) {
				return true;
			}
		}
		return false;
	}

	String KarstSystem::getNameFromAreaID(uint32 areaID)
	{
		if (!mKarstGroupMap.size())
			return "undefine";

		return mKarstGroupMap.begin()->second->getNameFromAreaID(areaID);
	}

	bool KarstSystem::getHeightBufferFromIndex(int pageX, int pageY, int NodeX, int NodeY, int size, float* pData)
	{
		if (!mKarstGroupMap.size())
			return false;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (it->second->getHeightMapBufferFromIndex(pageX, pageY, NodeX, NodeY, size, pData)) {
				return true;
			}
		}
		return false;
	}

	std::shared_ptr<HeightBuffer> KarstSystem::getHeightBufferFromWorldSpace(int page_index_x, int page_index_y, int page_size)
	{
		if (!mKarstGroupMap.size())
			return nullptr;

		for (auto it = mKarstGroupMap.begin(); it != mKarstGroupMap.end(); it++)
		{
			if (auto pBuffer = it->second->getHeightBufferFromWorldSpace(page_index_x, page_index_y, page_size); pBuffer) {
				return pBuffer;
			}
		}
		return nullptr;
	}

	KarstGroup* KarstSystem::getKarstGroup() {
		if (!mKarstGroupMap.size())
			return nullptr;

		return mKarstGroupMap.begin()->second;
	}

	KarstGroup* KarstSystem::getMiniMap()
	{
		return mMiniKarst;
	}

	String KarstSystem::getLevelName() {
		KarstGroup* pGroup = getKarstGroup();
		//todo: 如果pGroup未创建暂时返回默认值
		if (!pGroup)
			return "karst2";

		return pGroup->getLevelName();
	}

	void KarstSystem::addCreatedListener(KarstChangedHandler handler)
	{
		if (!handler)
			return;
		mCreatedListeners.push_back(handler);
	}

	void KarstSystem::addDestroyedListener(KarstChangedHandler handler)
	{
		if (!handler)
			return;
		mDestroyedListeners.push_back(handler);
	}

	void KarstSystem::setDisplayAreaBorder(bool miniMap, uint32 displayMask, uint32 level)
	{
		if (miniMap) {
			if (mMiniKarst != nullptr) {
				mMiniKarst->setDisplayAreaBorder(displayMask, level);
			}
		}
		else {
			if (!mKarstGroupMap.size())
				return;

			mKarstGroupMap.begin()->second->setDisplayAreaBorder(displayMask, level);
		}
	}

} //namespace Echo
