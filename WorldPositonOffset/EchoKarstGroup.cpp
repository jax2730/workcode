///////////////////////////////////////////////////////////////////////////////
//
// @file EchoKarstRenderManager.cpp
// @author liyaming
// @date 2024/08/14
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "EchoStableHeaders.h"
#include "EchoKarstGroup.h"
#include "EchoKarstPage.h"
#include "EchoShaderContentManager.h"
#include "EchoComputable.h"
#include "EchoRenderStrategy.h"
#include "EchoKarstMaterials.h"
#include "EchoUberNoiseFBM2D.h"
#include "EchoResourceFolderManager.h"
#include "EchoImage.h"
#include "EchoTextureManager.h"
#include "EchoShadowmapManager.h"
#include "EchoPointLightShadowManager.h"

namespace Echo
{
	const static std::string KarstResouceHeightMapName = "HeightMap";
	//const static std::string KarstResouceNormalMapName = "NormalMap";
	const static std::string KarstResouceTextureName = "TextureMap";
	const static std::string KarstResouceMaterialIDName = "MaterialIDMap";
	const static std::string KarstAreaIDName = "AreaIDMap";

	const static uint16 ReqType_KarstNodeMap = 0u;
	const static uint16 ReqType_KarstPlaneMesh = 1u;

	KarstGroup::KarstGroup(const String& name, SceneManager* _sm, const KarstParams& params, uint8 renderType, std::shared_ptr<KarstCommonData> commonData)
		: mOptions(new KarstOptions(params))
		, mRenderOptions(new KarstRenderOptions)
		, mName(name)
		, mLevelName(params.levelName)
		, mSceneManager(_sm)
		, mBasePosition(params.originPosition)
		, mRowNum(params.row)
		, mColNum(params.col)
		, mMaterials(new KarstMaterials(mSceneManager, renderType))
		, mKarstMesh(new KarstMesh(params.batchSize))
		, mRenderNodeRowNum(params.row* (1 << mOptions->mTreeDepth))
		, mRenderNodeColNum(params.col* (1 << mOptions->mTreeDepth))
		, mKarstPageFactory(this, renderType)
		, mRenderType(renderType)
	{
		//设置基准世界坐标
		mWorldBasePosition = Root::instance()->getWorldOrigin();
		updateWorldBasePosition();

		//初始化渲染设置
		initRenderConfig();

		//初始化地形设置
		mOptions->setMaxDetailViewDistance(mRenderOptions->mMaxDetailViewDistance, mRenderOptions->mMaxViewDistanceLimit, mSceneManager->getMainCamera());

		//创建公用地块四叉树
		mQuadtreeObj = std::make_unique<KarstQuadTreeObject>(mOptions.get());

		//初始化渲染对象池	
		mRenderablePool.initManager(mOptions->mTreeDepth + 1, mRenderOptions->mMaxRenderableNum, 0.5f);
		mRenderables.reserve(mRenderOptions->mMaxRenderableNum);

		//初始化结点遍历跟踪表
		mTraceMap[0].resize(mOptions->mTreeDepth + 1);
		mTraceMap[1].resize(mOptions->mTreeDepth + 1);

		//初始化生成式地形噪声设置
		mNoiseOptions.updateHashValue();

		//初始化渲染资源	
		mRenderResource = std::make_unique<KarstRenderResource>(this, mDataResType, commonData);
		mRenderResource->setNoiseOptions(mNoiseOptions);
		mRenderResource->loadResources();

		//初始化地表平面几何对象(河流、湖泊)
		mPlaneMeshPool.initManager(1, 20, 10.0f);

		//初始化虚拟内存页表
		{
			mDataVTOptions = std::make_unique<KarstVirtualTextureOptions>(mRenderOptions->mDataVTTileRowNum, mRenderOptions->mDataVTTileRowNum, mOptions->mBatchSize + 1, mRenderOptions->mVTLevels, mRenderOptions->mDataVTSize);
			mTexVTOptions = std::make_unique<KarstVirtualTextureOptions>(mRenderOptions->mTexVTTileRowNum, mRenderOptions->mTexVTTileRowNum, mRenderOptions->mTexVTTileSize, mRenderOptions->mVTLevels, mRenderOptions->mTexVTSize);
			createVirtualPageTable();
		}

		//创建地块
		createKarstPages();

		//创建虚拟纹理(内存)资源池
		mDataResourcePool = std::make_unique<KarstDataResourcePool>(mDataVTOptions.get());
		mTexResourcePool = std::make_unique<KarstDataResourcePool>(mTexVTOptions.get());

		if (mRenderType == KarstRenderType::VirtualTexture)
		{
			//创建合批渲染对象
			createKarstBatchRenders();
			//添加虚拟纹理资源
			mDataResourcePool->addResourceLayer(KarstResouceHeightMapName, KarstRenderType::VirtualTexture, ECHO_FMT_R32UI, sizeof(uint32), true);
			mDataResourcePool->addResourceLayer(KarstResouceMaterialIDName, KarstRenderType::VirtualTexture, ECHO_FMT_R8UI, sizeof(uint8));
			mTexResourcePool->addResourceLayer(KarstAreaIDName, KarstRenderType::VirtualTexture, ECHO_FMT_R32UI, sizeof(uint32));
			mTexResourcePool->addResourceLayer(KarstResouceTextureName, KarstRenderType::VirtualTexture, ECHO_FMT_RGBA8888_UNORM, sizeof(uint32));
		}
		else if (mRenderType == KarstRenderType::VirtualMemory) // 添加虚拟内存资源
		{
			mDataResourcePool->addResourceLayer(KarstResouceHeightMapName, KarstRenderType::VirtualMemory, ECHO_FMT_INVALID, sizeof(float), true);
		}

		//绑定初始化材质贴图
		bindBatchTextures();

		//创建迷雾缓冲区
		mAreaFogBuffer.resize(mAreaFogBufferSize, 0.0f);
		mAreaFogRcBuffer = Root::instance()->getRenderSystem()->createDynamicSB(mAreaFogBuffer.data(), sizeof(float), mAreaFogBufferSize * sizeof(float));

		//添加事件监听
		{
			mSceneManager->addListener(this);
			Root::instance()->addFrameListener(this);
			Root::instance()->addWorldOriginListener(this);

			mWorkQueue = Root::instance()->getWorkQueue();
			mWorkQueueChannel = mWorkQueue->getChannel("KarstResourceLoadChannel" + mLevelName);
			mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
			mWorkQueue->addResponseHandler(mWorkQueueChannel, this);
		}

		mActive = true;
	}

	KarstGroup::~KarstGroup()
	{
		mActive = false;

		mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
		mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);
		Root::instance()->removeFrameListener(this);
		Root::instance()->removeWorldOriginListener(this);
		mSceneManager->removeListener(this);

		for (auto it : mKarstBlocks) {
			if (it.second->instance) {
				it.second->instance.reset();
			}
		}

		mSceneManager->removeListener(this);
	}

	void KarstGroup::initRenderConfig()
	{
		try
		{
			String jsonPath = "levels/" + mLevelName + "/render_config.json";
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonPath.c_str()));
			if (pDataStream.isNull()) {
				throw std::runtime_error("Karst: render_config.json is not found");
			}
			auto dataSize = pDataStream->size();
			if (dataSize > 0)
			{
				char* pData = new char[dataSize + 1];
				memset(pData, 0, dataSize + 1);
				pDataStream->read(pData, dataSize);
				cJSON* rootNode = cJSON_Parse(pData);

				ReadJSNode(rootNode, "MinRiverMeshHeight", mRenderOptions->mMinRiverMeshHeight);
				ReadJSNode(rootNode, "RiverDepth", mRenderOptions->mRiverDepth);
				ReadJSNode(rootNode, "MaxInstanceNum", mRenderOptions->mMaxInstanceNum);
				ReadJSNode(rootNode, "MaxRenderableNum", mRenderOptions->mMaxRenderableNum);
				ReadJSNode(rootNode, "DataVTTileRowNum", mRenderOptions->mDataVTTileRowNum);
				ReadJSNode(rootNode, "DataVTSize", mRenderOptions->mDataVTSize);
				ReadJSNode(rootNode, "TexVTTileRowNum", mRenderOptions->mTexVTTileRowNum);
				ReadJSNode(rootNode, "TexVTSize", mRenderOptions->mTexVTSize);
				ReadJSNode(rootNode, "TexVTTileSize", mRenderOptions->mTexVTTileSize);
				ReadJSNode(rootNode, "MaxDetailViewDistance", mRenderOptions->mMaxDetailViewDistance);
				ReadJSNode(rootNode, "MaxViewDistanceLimit", mRenderOptions->mMaxViewDistanceLimit);

				cJSON* arrayNode = cJSON_GetObjectItem(rootNode, "VTLevelMap");
				uint32 arraySize = cJSON_GetArraySize(arrayNode);
				mRenderOptions->mVTLevels = arraySize;
				mRenderOptions->mVTLevelMap.resize(arraySize);
				for (uint32 i = 0; i < arraySize; i++) {
					mRenderOptions->mVTLevelMap[i] = cJSON_GetArrayItem(arrayNode, i)->valueint;
				}

				delete[] pData;
			}
			else {
				throw std::runtime_error("Karst: render_config.json is null");
			}
		}
		catch (std::exception& e)
		{
			LogManager::instance()->logMessage(e.what());
		}
	}

	void KarstGroup::createVirtualPageTable()
	{
		int levels = mOptions->mTreeDepth + 1;
		mVirtualPageTables.resize(levels);
		for (int i = 0; i < levels; ++i)
		{
			int tileSize = (int)pow(2, i);
			mVirtualPageTables[i].tileSize = tileSize;
			mVirtualPageTables[i].Page.resize(tileSize);

			for (int x = 0; x < tileSize; ++x)
			{
				mVirtualPageTables[i].Page[x].resize(tileSize);
				for (int y = 0; y < tileSize; ++y)
				{
					mVirtualPageTables[i].Page[x][y].tileID = 0;
					mVirtualPageTables[i].Page[x][y].currLevel = 0;
					mVirtualPageTables[i].Page[x][y].status = KARST_PAGE_STATUS_EMPTY;
					mVirtualPageTables[i].Page[x][y].addRequest = false;
				}
			}
		}
	}

	void KarstGroup::createKarstPage(int x, int y)
	{
		auto idx = EchoKarst::packIndex(x, y);
		if (auto block = mKarstBlocks.find(idx); block != mKarstBlocks.end())
		{
			auto& pageBlock = block->second;
			if (pageBlock->available && !pageBlock->usable) {
				LogManager::instance()->logMessage("Create Karst Block : " + std::to_string(x) + "," + std::to_string(y));
				pageBlock->instance = mKarstPageFactory.createKarstPage(x, y, pageBlock->minH, pageBlock->maxH);
				pageBlock->usable = true;
			}
		}
	}

	void KarstGroup::destroyKarstPage(int x, int y)
	{
		auto idx = EchoKarst::packIndex(x, y);
		if (auto block = mKarstBlocks.find(idx); block != mKarstBlocks.end())
		{
			auto& pageBlock = mKarstBlocks.at(idx);
			if (pageBlock->available && pageBlock->usable) {
				LogManager::instance()->logMessage("Destroy Karst Block : " + std::to_string(x) + "," + std::to_string(y));
				pageBlock->instance.reset();
				pageBlock->usable = false;
			}
		}
	}

	void KarstGroup::saveKarstBlockInfo()
	{
		try
		{
			String jsonPath = "levels/" + mLevelName + "/karst_block_cache.json";
			DataStreamPtr pDataStream(Root::instance()->CreateDataStream(jsonPath.c_str()));
			if (pDataStream.isNull()) {
				LogManager::instance()->logMessage("Karst: Failed to create karst_block_cache.json for writing");
				return;
			}

			cJSON* rootNode = cJSON_CreateObject();
			cJSON* blocksArray = cJSON_CreateArray();

			for (const auto& block : mKarstBlocks)
			{
				auto idx = EchoKarst::KeyPack(block.first);
				cJSON* blockNode = cJSON_CreateObject();

				cJSON_AddNumberToObject(blockNode, "x", idx.x);
				cJSON_AddNumberToObject(blockNode, "y", idx.y);
				cJSON_AddNumberToObject(blockNode, "minH", block.second->minH);
				cJSON_AddNumberToObject(blockNode, "maxH", block.second->maxH);
				cJSON_AddBoolToObject(blockNode, "available", block.second->available);

				cJSON_AddItemToArray(blocksArray, blockNode);
			}

			cJSON_AddItemToObject(rootNode, "blocks", blocksArray);
			cJSON_AddNumberToObject(rootNode, "rowNum", mRowNum);
			cJSON_AddNumberToObject(rootNode, "colNum", mColNum);

			char* jsonString = cJSON_Print(rootNode);
			if (jsonString)
			{
				pDataStream->writeData(jsonString, strlen(jsonString));
				pDataStream->close();
				free(jsonString);
				LogManager::instance()->logMessage("Karst: Successfully saved block cache to " + jsonPath);
			}

			cJSON_Delete(rootNode);
		}
		catch (std::exception& e)
		{
			LogManager::instance()->logMessage("Karst: Error saving block cache - " + String(e.what()));
		}
	}

	bool KarstGroup::loadKarstBlockInfo()
	{
		try
		{
			String jsonPath = "levels/" + mLevelName + "/karst_block_cache.json";
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonPath.c_str()));
			if (pDataStream.isNull()) {
				LogManager::instance()->logMessage("Karst: karst_block_cache.json not found, will generate new block info");
				return false;
			}

			auto dataSize = pDataStream->size();
			if (dataSize == 0) {
				LogManager::instance()->logMessage("Karst: karst_block_cache.json is empty");
				return false;
			}

			char* pData = new char[dataSize + 1];
			memset(pData, 0, dataSize + 1);
			pDataStream->read(pData, dataSize);

			cJSON* rootNode = cJSON_Parse(pData);
			if (!rootNode) {
				LogManager::instance()->logMessage("Karst: Failed to parse karst_block_cache.json");
				delete[] pData;
				return false;
			}

			// 验证行列数是否匹配
			uint32 cachedRowNum = 0, cachedColNum = 0;
			ReadJSNode(rootNode, "rowNum", cachedRowNum);
			ReadJSNode(rootNode, "colNum", cachedColNum);

			if (cachedRowNum != mRowNum || cachedColNum != mColNum) {
				LogManager::instance()->logMessage("Karst: Cached block info dimensions don't match current terrain size, regenerating");
				cJSON_Delete(rootNode);
				delete[] pData;
				return false;
			}

			cJSON* blocksArray = cJSON_GetObjectItem(rootNode, "blocks");
			if (!blocksArray) {
				LogManager::instance()->logMessage("Karst: Invalid blocks array in cache file");
				cJSON_Delete(rootNode);
				delete[] pData;
				return false;
			}

			int blockCount = cJSON_GetArraySize(blocksArray);
			int loadedCount = 0;

			for (int i = 0; i < blockCount; i++)
			{
				cJSON* blockNode = cJSON_GetArrayItem(blocksArray, i);
				if (!blockNode) continue;

				uint32 x = 0, y = 0;
				float minH = 0.0f, maxH = 0.0f;
				bool available = false;

				if (ReadJSNode(blockNode, "x", x) &&
					ReadJSNode(blockNode, "y", y) &&
					ReadJSNode(blockNode, "minH", minH) &&
					ReadJSNode(blockNode, "maxH", maxH) &&
					ReadJSNode(blockNode, "available", available))
				{
					if (x < mRowNum && y < mColNum)
					{
						auto block = std::make_shared<KarstPageBlock>();
						block->minH = minH;
						block->maxH = maxH;
						block->available = available;
						block->usable = false;
						block->instance = nullptr;

						mKarstBlocks.insert(std::make_pair(EchoKarst::packIndex(x, y), block));
						loadedCount++;
					}
				}
			}

			cJSON_Delete(rootNode);
			delete[] pData;

			LogManager::instance()->logMessage("Karst: Successfully loaded " + std::to_string(loadedCount) + " cached blocks from " + jsonPath);
			return loadedCount > 0;
		}
		catch (std::exception& e)
		{
			LogManager::instance()->logMessage("Karst: Error loading block cache - " + String(e.what()));
			return false;
		}
	}

	void KarstGroup::createKarstPages()
	{
		// 首先尝试从缓存文件加载
		if (loadKarstBlockInfo())
		{
			// 如果成功加载了缓存，只需要创建实例
			for (const auto& block : mKarstBlocks)
			{
				if (!mPartitionLoad && block.second->available) {
					auto idx = EchoKarst::KeyPack(block.first);
					createKarstPage(idx.x, idx.y);
				}
			}
		}
		else
		{
			// 如果没有缓存或加载失败，按原来的方式生成
			for (uint32 x = 0; x < mRowNum; x++)
			{
				for (uint32 y = 0; y < mColNum; y++)
				{
					auto block = std::make_shared<KarstPageBlock>();
					block->available = mRenderResource->checkPageValidate(x, y, block->minH, block->maxH);
					block->usable = false;
					block->instance = nullptr;
					mKarstBlocks.insert(std::make_pair(EchoKarst::packIndex(x, y), block));
					if (!mPartitionLoad) {
						createKarstPage(x, y);
					}
				}
			}

			// 保存新生成的数据到缓存文件
			saveKarstBlockInfo();
		}
	}

	void KarstGroup::updateWorldBasePosition()
	{
		mOriginPosition = mBasePosition - mWorldBasePosition;
		for (auto& block : mKarstBlocks)
		{
			auto idx = EchoKarst::KeyPack(block.first);
			if (block.second->instance) {
				block.second->instance->setBasePosition(DVector3((double)idx.x, 0.0, (double)idx.y) * mOptions->mKarstSize + mOriginPosition);
			}
		}
	}

	void KarstGroup::worldOriginRebasing(double origin_x, double orign_y, double origin_z)
	{
		mWorldBasePosition = DVector3(origin_x, orign_y, origin_z);
		updateWorldBasePosition();

		/*for (auto& planeMesh : mPlaneMeshMap) {
			planeMesh.second.renderable->setWorldBasePosition(mWorldBasePosition);
		}*/
	}

	void KarstGroup::createKarstBatchRenders()
	{
		for (uint32 i = 0; i < 16; i++) {
			std::unique_ptr<KarstBatchRender> render(new KarstBatchRender(mRenderOptions->mMaxInstanceNum, mMaterials->mBatchBaseMaterial, i, mKarstMesh.get(), this));
			mKarstRenders.push_back(std::move(render));
		}
	}

	void KarstGroup::bindBatchTextures()
	{
		auto& matWrapper = mMaterials->mBatchBaseMaterial;
		if (matWrapper.isV1())
		{
			Material* material = matWrapper.getV1();
			material->m_textureList[MAT_TEXTURE_CUSTOMER] = getCustomTexture();
			material->m_textureList[MAT_TEXTURE_CUSTOMER3] = getCustomNormal();
			Name s_fbmPathStr = Name("echo/biome_terrain/Textures/fbm.dds");
			material->m_textureList[MAT_TEXTURE_CUSTOMER6] = TextureManager::instance()->loadTexture(s_fbmPathStr, TEX_TYPE_2D);
			Name s_voronoiPathStr = Name("echo/biome_terrain/Textures/T_Voronoi_Perturbed.dds");
			material->m_textureList[MAT_TEXTURE_CUSTOMER7] = TextureManager::instance()->loadTexture(s_voronoiPathStr, TEX_TYPE_2D);
		}
		else if (matWrapper.isV2())
		{
			MaterialV2* material = matWrapper.getV2();
			material->setTexture(Mat::TEXTURE_BIND_CUSTOMER, getCustomTexture());
			material->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, getCustomNormal());
			Name s_fbmPathStr = Name("echo/biome_terrain/Textures/fbm.dds");
			material->setTexture(Mat::TEXTURE_BIND_CUSTOMER6, TextureManager::instance()->loadTexture(s_fbmPathStr, TEX_TYPE_2D));
			Name s_voronoiPathStr = Name("echo/biome_terrain/Textures/T_Voronoi_Perturbed.dds");
			material->setTexture(Mat::TEXTURE_BIND_CUSTOMER7, TextureManager::instance()->loadTexture(s_voronoiPathStr, TEX_TYPE_2D));
		}
	}

	uint32 KarstGroup::getKarstMeshType(KarstRenderable* node)
	{
		auto pageNodeOffset = node->mKarstPage->mPageOffset;
		auto depth = node->mQuadtreeNode->mDepth;
		auto edgeType = [this, node, pageNodeOffset, depth](int xOffset, int yOffset) {
			return getKarstRenderNodeLod(depth, pageNodeOffset, node->mQuadtreeNode->mNodeOffset, xOffset, yOffset) ? 0 : 1;
		};
		//left << 3 + right << 2 + top << 1 + bottom
		return (edgeType(-1, 0) << 3) + (edgeType(1, 0) << 2) + (edgeType(0, 1) << 1) + edgeType(0, -1);
	}

	void KarstGroup::insertTrace(uint32 level, uint64 key, uint32 status)
	{
		mTraceMap[mIndex][level].insert(std::make_pair(key, status));
	}

	void KarstGroup::abortRequest(KarstPage* karstPage, const KarstQuadtreeNode* node, int index, int tileID, KarstResourceDataType dataType)
	{
		KarstTileInfo tileInfo;
		tileInfo.px = node->mXIndex;
		tileInfo.py = node->mYIndex;
		tileInfo.mipLevel = node->mDepth;
		karstPage->clearTableTileInfo(tileInfo, dataType);

		setTileUnLocked(index, tileID, dataType);
		setTileReserved(index, tileID, dataType);
	}

	void KarstGroup::onUpdate()
	{
		mIndex = (mIndex + 1) % 2;
		mInitial = mIndex == 1 ? false : mInitial;
		mDataResourcePool->onUpdate();
		mTexResourcePool->onUpdate();
		bool forceUpdate = mRequestDelay > 0.1;

		for (auto& requests : requestMap)
		{
			if (requests.second.size() > 30 || (forceUpdate && requests.second.size())) {
				LoadKarstResourceRequest* req = new LoadKarstResourceRequest;
				req->karstGroup = this;
				EchoKarst::KeyPack page(requests.first);
				req->pageX = page.x;
				req->pageY = page.y;
				req->requests.swap(requests.second);
				loadResourceRequest(req);
			}
		}

		if (forceUpdate) {
			mRequestDelay = 0.0f;
		}
		else {
			mRequestDelay += Root::instance()->getTimeSinceLastFrame();
		}

		if (mAreaFogDirty) {
			Root::instance()->getRenderSystem()->updateBuffer(mAreaFogRcBuffer, mAreaFogBuffer.data(), mAreaFogBufferSize * sizeof(float), true);
			mAreaFogDirty = false;
		}
	}

	void KarstGroup::updateKarstPages(const Camera* pCamera, int px, int py, int loadNum)
	{
		for (int x = px - loadNum; x <= px + loadNum; x++) {
			for (int y = py - loadNum; y <= py + loadNum; y++) {
				createKarstPage(x, y);
			}
		}
		for (auto block : mKarstBlocks) {
			auto idx = EchoKarst::KeyPack(block.first);
			if (block.second->instance) {
				if (abs(idx.x - px) > (loadNum + 1) || abs(idx.y - py) > (loadNum + 1)) {
					destroyKarstPage(idx.x, idx.y);
				}
			}
		}
	}

	void KarstGroup::findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue)
	{
		bool useLight = (Root::instance()->getClusterForwardLightEnable() &&
			Root::instance()->getClusterForwadLightLevel() != ClusteForwardLighting::CFL_Off);
		bool useDpsm = useLight && mSceneManager->getPointShadowManager()->IsEnabled();

		if (pCamera == mSceneManager->getMainCamera()) {
			mRenderables.clear();
			for (uint32 i = 0; i < mOptions->mTreeDepth + 1; i++) {
				mTraceMap[mIndex][i].clear();
			}
			if (mPartitionLoad) {
				int loadNum = mLoadRange == 0 ? CeilfToInt(pCamera->getFarClipDistance() / mOptions->mKarstSize) : std::max(mLoadRange, 1);
				DVector3 p = (pCamera->getPosition() - mOriginPosition) / mOptions->mKarstSize;
				int px = (int)floor(p.x); int py = (int)floor(p.z);
				loadNum = std::min(loadNum, (int)std::max(mRowNum, mColNum));
				loadNum = std::min(loadNum, 20);
				updateKarstPages(pCamera, px, py, loadNum);
				std::set<uint64> loadSet;

				for (int x = px - loadNum; x <= px + loadNum; x++) {
					for (int y = py - loadNum; y <= py + loadNum; y++) {
						uint64 key = EchoKarst::KeyPack(std::min(std::max(0, x), int(mRowNum - 1)), std::min(std::max(0, y), int(mColNum - 1))).key;
						if (loadSet.find(key) == loadSet.end()) {
							auto& pBlock = mKarstBlocks.at(key);
							if (pBlock->available && pBlock->usable) {
								pBlock->instance->checkRenderables(mQuadtreeObj->getQuadTree(), mResConserv, pCamera, mRenderables);
							}
							loadSet.insert(key);
						}
					}
				}
			}
			else {
				for (auto it : mKarstBlocks) {
					if (it.second->available && it.second->usable) {
						it.second->instance->checkRenderables(mQuadtreeObj->getQuadTree(), mResConserv, pCamera, mRenderables);
					}
				}
			}
			if (KarstRenderType::VirtualMemory == mRenderType) {
				std::sort(mRenderables.begin(), mRenderables.end(), [](KarstRenderable* rend1, KarstRenderable* rend2) {
					return (rend1->mDist < rend2->mDist);
					});
			}
			if (mRenderOptions->mMinRiverMeshHeight > 0.0 && pCamera->getPosition().y < mRenderOptions->mMinRiverMeshHeight) {
				auto page = (pCamera->getPosition() - mOriginPosition) / (mOptions->mKarstSize / 8.0);
				int river_x = (int)floor(page.x); int river_y = (int)floor(page.z);
				int range = 1;
				for (int i = -range; i <= range; i++) {
					for (int j = -range; j <= range; j++) {
						uint64 packKey = EchoKarst::packIndex(river_x + i, river_y + j);
						int blockID;
						auto planeMeshPtr = mPlaneMeshPool.findBlock(0, packKey, blockID);
						if (planeMeshPtr) {
							planeMeshPtr->renderable->addRenderable(pQueue->getRenderQueueGroup(RenderQueue_Water));
						}
						else if (mPlaneMeshReqSet.find(packKey) == mPlaneMeshReqSet.end()) {
							LoadKarstResourceRequest* req = new LoadKarstResourceRequest;
							req->karstGroup = this;
							req->pageX = (uint16)(river_x + i);
							req->pageY = (uint16)(river_y + j);
							mWorkQueue->addRequest(mWorkQueueChannel, ReqType_KarstPlaneMesh, Any(req), 0, WorkQueue::RESOURCE_TYPE_TERRAIN, false);
							mPlaneMeshReqSet.insert(packKey);
						}
					}
				}
			}
			RenderQueueGroup* pqg = pQueue->getRenderQueueGroup(RenderQueue_Terrain);
			if (mRenderType == KarstRenderType::VirtualTexture) {
				for (auto& render : mKarstRenders) {
					render->clearNodeInfo();
				}
				for (auto it : mRenderables) {
					uint32 meshType = getKarstMeshType(it);
					mKarstRenders[meshType]->addRenderNode(it);
				}
				if (mMaterials->mBatchBaseMaterial.isV1())
				{
					auto passType = mSceneManager->getPsmshadowManager()->getShadowEnable() ? ShadowResiveInstancePass : InstancePass;
					for (auto& render : mKarstRenders) {
						if (render->updateRenderData(mSceneManager)) {
							pqg->addRenderable(mMaterials->mBatchBaseMaterial.getV1()->getPass(passType), render.get());
						}
					}
				}
				else if (mMaterials->mBatchBaseMaterial.isV2())
				{
					PassMaskFlags postMaskFlags;
					postMaskFlags |= PassMaskFlag::UseInstance;
					postMaskFlags |= PassMaskFlag::UseLight;
					if (useDpsm)
					{
						postMaskFlags |= PassMaskFlag::UseDpsm;
					}
					postMaskFlags |= mSceneManager->getPsmshadowManager()->getShadowEnable() ? PassMaskFlag::ReceiveShadow : PassMaskFlag::None;

					PostRenderableParams postParams{
						.material = mMaterials->mBatchBaseMaterial.getV2(),
						.renderQueue = pQueue,
						.passMaskFlags = postMaskFlags
					};

					for (auto& render : mKarstRenders)
					{
						if (render->updateRenderData(mSceneManager))
						{
							postParams.renderable = render.get();
							MaterialShaderPassManager::fastPostRenderable(postParams);
						}
					}
				}
#ifdef _WIN32
				//mKarstVTViewer->addRenderable(pqg);
#endif
			}
			else if (mRenderType == KarstRenderType::VirtualMemory) {
				if (mMaterials->mNodeBaseMaterial.isV1())
				{
					auto passType = mSceneManager->getPsmshadowManager()->getShadowEnable() ? ShadowResivePass : NormalPass;
					for (auto it : mRenderables)
					{
						it->renderUpdate();
						pqg->addRenderable(mMaterials->mNodeBaseMaterial.getV1()->getPass(passType), it);
					}
				}
				else if (mMaterials->mNodeBaseMaterial.isV2())
				{
					PassMaskFlags postMaskFlags;
					postMaskFlags |= mSceneManager->getPsmshadowManager()->getShadowEnable() ? PassMaskFlag::ReceiveShadow : PassMaskFlag::None;

					PostRenderableParams postParams{
						.material = mMaterials->mNodeBaseMaterial.getV2(),
						.renderQueue = pQueue,
						.passMaskFlags = postMaskFlags
					};

					for (auto it : mRenderables)
					{
						it->renderUpdate();

						postParams.renderable = it;
						MaterialShaderPassManager::fastPostRenderable(postParams);
					}
				}
			}
		}
		else
		{
			RenderQueueGroup* pqg = pQueue->getRenderQueueGroup(RenderQueue_Terrain);

			if (mRenderType == KarstRenderType::VirtualTexture) {
				if (mMaterials->mBatchBaseMaterial.isV1())
				{
					for (auto& render : mKarstRenders)
					{
						if (render->needRender())
						{
							pqg->addRenderable(mMaterials->mBatchBaseMaterial.getV1()->getPass(InstancePass), render.get());
						}
					}
				}
				else if (mMaterials->mBatchBaseMaterial.isV2())
				{
					PassMaskFlags postMaskFlags;
					postMaskFlags |= PassMaskFlag::UseInstance;
					postMaskFlags |= PassMaskFlag::UseLight;
					if (useDpsm)
					{
						postMaskFlags |= PassMaskFlag::UseDpsm;
					}

					PostRenderableParams postParams{
						.material = mMaterials->mBatchBaseMaterial.getV2(),
						.renderQueue = pQueue,
						.passMaskFlags = postMaskFlags
					};


					for (auto& render : mKarstRenders)
					{
						if (render->needRender())
						{
							postParams.renderable = render.get();
							MaterialShaderPassManager::fastPostRenderable(postParams);
						}
					}
				}
			}
			else if (mRenderType == KarstRenderType::VirtualMemory)
			{
				if (mMaterials->mNodeBaseMaterial.isV1())
				{
					for (auto it : mRenderables)
					{
						pqg->addRenderable(mMaterials->mNodeBaseMaterial.getV1()->getPass(NormalPass), it);
					}
				}
				else if (mMaterials->mNodeBaseMaterial.isV2())
				{
					PostRenderableParams postParams{
						.material = mMaterials->mNodeBaseMaterial.getV2(),
						.renderQueue = pQueue
					};

					for (auto it : mRenderables)
					{
						postParams.renderable = it;
						MaterialShaderPassManager::fastPostRenderable(postParams);
					}
				}
			}
		}
	}

	bool KarstGroup::isTileInTrace(KarstTileInfo& tileinfo) const
	{
		uint32 level = tileinfo.mipLevel;
		int x = tileinfo.pageX * (1 << level) + tileinfo.px;
		int y = tileinfo.pageY * (1 << level) + tileinfo.py;
		uint64 index = EchoKarst::packIndex(std::min((uint32)std::max(x, 0), mRenderNodeRowNum - 1),
			std::min((uint32)std::max(y, 0), mRenderNodeColNum - 1));

		uint32 i = mIndex ? 0 : 1;
		if (auto it = mTraceMap[i][level].find(index); it != mTraceMap[i][level].end()) {
			return true;
		}
		else {
			return false;
		}
	}

	bool KarstGroup::getKarstRenderNodeLod(uint32 level, EchoKarst::KeyPack pageOffset, EchoKarst::KeyPack nodeOffset, int xOffset, int yOffset) const
	{
		int x = pageOffset.x * (1 << level) + nodeOffset.x + xOffset;
		int y = pageOffset.y * (1 << level) + nodeOffset.y + yOffset;
		uint64 index = EchoKarst::packIndex(std::min((uint32)std::max(x, 0), mRenderNodeRowNum - 1),
			std::min((uint32)std::max(y, 0), mRenderNodeColNum - 1));

		if (auto it = mTraceMap[mIndex][level].find(index); it != mTraceMap[mIndex][level].end()) {
			return it->second == NodeResStatus_LocalHighRes || it->second == NodeResStatus_NoLocal;
		}
		else {
			return false;
		}
	}

	int	KarstGroup::requestTileResource(int level, KarstResourceDataType dataType) const
	{
		static auto func = std::bind(&KarstGroup::isTileInTrace, this, std::placeholders::_1);
		int tileID = -1;
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			KarstTileInfo tileInfo;
			bool needClear;
			tileID = resourcePool->reserveTileSlot(level, tileInfo, needClear, func);
			if (needClear) {
				auto karstPage = getKarstPage(tileInfo.pageX, tileInfo.pageY);
				if (karstPage)
					karstPage->clearTableTileInfo(tileInfo, dataType);
			}
		}
		return tileID;
	}

	void KarstGroup::getTileHeightRcBuffer(int level, int tileID, RcBuffer** pHeightBuff)
	{
		if (mRenderType == KarstRenderType::VirtualMemory) {
			if (auto resouce = mDataResourcePool->getDataResource(KarstResouceHeightMapName); resouce)
			{
				reinterpret_cast<KarstTileVMResource*>(resouce->mTileResource)->getTileRcBuffer(level, tileID, pHeightBuff);
				return;
			}
		}
		*pHeightBuff = nullptr;
	}

	void KarstGroup::setTileVisited(int level, int tileID, KarstResourceDataType dataType)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			resourcePool->visitTile(level, tileID);
		}
	}

	void KarstGroup::setTileLocked(int level, int tileID, KarstResourceDataType dataType)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			resourcePool->lockTile(level, tileID);
		}
	}

	void KarstGroup::setTileUnLocked(int level, int tileID, KarstResourceDataType dataType)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			resourcePool->unlockTile(level, tileID);
		}
	}

	void KarstGroup::setTileReserved(int level, int tileID, KarstResourceDataType dataType)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			resourcePool->setTileReserved(level, tileID, true);
		}
	}

	void KarstGroup::setTileUnReserved(int level, int tileID, KarstResourceDataType dataType)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			resourcePool->setTileReserved(level, tileID, false);
		}
	}

	void KarstGroup::setTileInfo(uint32 index, uint32 tileID, KarstResourceDataType dataType, const KarstQuadtreeNode* node, uint16 pageX, uint16 pageY)
	{
		if (auto resourcePool = getResourcePool(dataType); resourcePool) {
			KarstTileInfo tileInfo;
			tileInfo.pageX = (int)pageX;
			tileInfo.pageY = (int)pageY;
			tileInfo.mipLevel = node->mDepth;
			tileInfo.px = node->mXIndex;
			tileInfo.py = node->mYIndex;
			resourcePool->setTileInfo(index, tileID, tileInfo);
		}
	}

	KarstDataResourcePool* KarstGroup::getResourcePool(KarstResourceDataType dataType) const
	{
		if (dataType == KarstResourceDataType::HeightNormalMap) {
			return mDataResourcePool.get();
		}
		else if (dataType == KarstResourceDataType::TextureMap) {
			return mTexResourcePool.get();
		}
		return nullptr;
	}

	std::shared_ptr<KarstPageBlock> KarstGroup::getKarstBlock(uint32 pageX, uint32 pageY) const {
		if (!mKarstBlocks.size())
			return nullptr;

		if (auto it = mKarstBlocks.find(EchoKarst::packIndex(pageX, pageY)); it != mKarstBlocks.end()) {
			return it->second;
		}
		else {
			return nullptr;
		}
	}

	KarstPage* KarstGroup::getKarstPage(uint32 pageX, uint32 pageY) const {
		if (!mKarstBlocks.size())
			return nullptr;

		if (auto it = mKarstBlocks.find(EchoKarst::packIndex(pageX, pageY)); it != mKarstBlocks.end()) {
			return it->second->instance.get();
		}
		else {
			return nullptr;
		}
	}

	bool KarstGroup::checkKarstPageAvailable(uint32 pageX, uint32 pageY) const {
		if (!mKarstBlocks.size())
			return false;

		if (auto it = mKarstBlocks.find(EchoKarst::packIndex(pageX, pageY)); it != mKarstBlocks.end()) {
			return it->second->available;
		}
		else {
			return false;
		}
	}

	bool KarstGroup::getSurfaceID(const Vector2& pos, uint8& value)
	{
		if (!mActive)
			return false;
		return mRenderResource->getSurfaceID(pos, value);
	}

	const uint8* KarstGroup::getSurfaceIDPage(uint32 pageX, uint32 pageY)
	{
		if (!mActive)
			return nullptr;
		return mRenderResource->mCommonData->surfaceIDData[pageX][pageY].data();
	}

	bool KarstGroup::getAreaID(const Vector2& pos, uint32& value)
	{
		if (!mActive)
			return false;
		return mRenderResource->getAreaID(pos, value);
	}

	const uint32* KarstGroup::getAreaIDPage(uint32 pageX, uint32 pageY)
	{
		if (!mActive)
			return nullptr;
		return mRenderResource->mCommonData->areaIDData[pageX][pageY].data();
	}

	bool KarstGroup::getHeightFromPosition(const Vector2& pos, float& height)
	{
		if (!mActive)
			return false;
		int pageX = (int)floor(((double)pos.x - mOriginPosition.x) / mOptions->mKarstSize);
		int pageY = (int)floor(((double)pos.y - mOriginPosition.z) / mOptions->mKarstSize);

		if (pageX < 0 || pageX >= (int)mRowNum || pageY < 0 || pageY >= (int)mColNum)
		{
			return false;
		}

		if (auto it = mKarstBlocks.find(EchoKarst::packIndex(pageX, pageY)); it != mKarstBlocks.end() && it->second->instance)
		{
			auto& karstPage = it->second->instance;
			const auto& basePos = karstPage->getBasePosition();
			auto rPos = pos - Vector2((float)basePos.x, (float)basePos.z);
			int nx = (int)floor(((double)pos.x - basePos.x) / (mOptions->mGridSize * mOptions->mBatchSize));
			int ny = (int)floor(((double)pos.y - basePos.z) / (mOptions->mGridSize * mOptions->mBatchSize));

			KarstTileCoord tileCoord;
			KarstRenderable renderable;
			renderable.mKarstPage = karstPage.get();
			renderable.mQuadtreeNode = mQuadtreeObj->getQuadtreeNode(mOptions->mMaxLodLevel, nx, ny);
			if (karstPage->getNodeTileCoord(tileCoord, &renderable, KarstResourceDataType::HeightNormalMap, false))
			{
				auto pResource = mDataResourcePool->getDataResource(KarstResouceHeightMapName)->mTileResource;
				float* pData = reinterpret_cast<float*>(pResource->getTileDataCache(tileCoord.index));
				int tileSize = mDataVTOptions->mVTCacheSize * mDataVTOptions->mVTCacheSize;
				pData += tileCoord.tileID * tileSize;

				float nodeSize = mOptions->mGridSize * mOptions->mBatchSize * (1 << tileCoord.levelDiff);

				nx = (int)floor(rPos.x / nodeSize);
				ny = (int)floor(rPos.y / nodeSize);

				float offsetX = rPos.x - nodeSize * nx;
				float offsetY = rPos.y - nodeSize * ny;

				float px = offsetX / nodeSize * mOptions->mBatchSize;
				float py = offsetY / nodeSize * mOptions->mBatchSize;

				int ix = (int)floor(px), iy = (int)floor(py);
				float u = px - ix, v = py - iy;

				uint32 idx0 = iy * mDataVTOptions->mVTCacheSize + ix;
				uint32 idx1 = iy * mDataVTOptions->mVTCacheSize + ix + 1;
				uint32 idx2 = (iy + 1) * mDataVTOptions->mVTCacheSize + ix;
				uint32 idx3 = (iy + 1) * mDataVTOptions->mVTCacheSize + ix + 1;

				height = (pData[idx0] * (1.0f - u) + pData[idx1] * u) * (1.0f - v) + (pData[idx2] * (1.0f - u) + pData[idx3] * u) * v;

				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	bool KarstGroup::getHeightFromWorldPosition(const DVector3& pos, float& height)
	{
		if (!mActive)
			return false;
		auto localPos = Root::instance()->toLocal(pos);
		return getHeightFromPosition(Vector2((float)localPos.x, (float)localPos.z), height);
	}

	Vector3 getNormalFromU32(uint32 value)
	{
		float b = (float)(value & 0xff);
		float g = (float)(value >> 8 & 0xff);
		float r = (float)(value >> 16 & 0xff);
		//float a = (float)(value >> 24 & 0xff);
		return Vector3(r, g, b) / 255.0f * 2.0f - 1.0f;
	}

	bool KarstGroup::getNormalFromPosition(const Vector2& pos, Vector3& normal)
	{
		return false;
	}

	bool KarstGroup::getNormalFromWorldPosition(const DVector3& pos, Vector3& normal)
	{
		if (!mActive)
			return false;
		auto localPos = Root::instance()->toLocal(pos);
		return getNormalFromPosition(Vector2((float)localPos.x, (float)localPos.z), normal);
	}

	Vector3	KarstGroup::getPageBasePosition(uint32 pageX, uint32 pageY)
	{
		assert(pageX < mRowNum&& pageY < mColNum);

		if (auto it = mKarstBlocks.find(EchoKarst::packIndex(pageX, pageY)); it != mKarstBlocks.end() && it->second->instance) {
			return it->second->instance->mBasePosition;
		}
		else {
			return Vector3(0.0f);
		}
	}

	int KarstGroup::getTextureLevel(uint32 depth) const
	{
		for (uint32 i = 0; i < mRenderOptions->mVTLevelMap.size(); i++) {
			if (depth < mRenderOptions->mVTLevelMap[i]) {
				return i;
			}
		}
		return 0;
	}

	float KarstGroup::getMaxHeight() const
	{
		if (mDataResType == KarstDataResType::DataFile)
		{
			return mRenderResource->getHeightMapMaxHeight();
		}
		else if (mDataResType == KarstDataResType::PCG)
		{
			return mNoiseOptions.height;
		}
		return 0.0f;
	}

	float KarstGroup::getMinHeight() const
	{
		if (mDataResType == KarstDataResType::DataFile)
		{
			return mRenderResource->getHeightMapMinHeight();
		}
		else if (mDataResType == KarstDataResType::PCG)
		{
			return -mNoiseOptions.height;
		}
		return 0.0f;
	}

	RcTexture* KarstGroup::getTileHeightTexture() const
	{
		if (mRenderType != KarstRenderType::VirtualTexture)
			return nullptr;

		if (auto resouce = mDataResourcePool->getDataResource(KarstResouceHeightMapName); resouce)
		{
			return reinterpret_cast<KarstTileVTResource*>(resouce->mTileResource)->getTileRcTexture();
		}

		return nullptr;
	}

	RcTexture* KarstGroup::getTileTextureTexture() const
	{
		if (auto resouce = mTexResourcePool->getDataResource(KarstResouceTextureName); resouce)
		{
			return reinterpret_cast<KarstTileVTResource*>(resouce->mTileResource)->getTileRcTexture();
		}

		return nullptr;
	}

	RcTexture* KarstGroup::getTileMaterialTexture() const
	{
		if (auto resouce = mDataResourcePool->getDataResource(KarstResouceMaterialIDName); resouce)
		{
			return reinterpret_cast<KarstTileVTResource*>(resouce->mTileResource)->getTileRcTexture();
		}

		return nullptr;
	}

	RcTexture* KarstGroup::getAreaTexture() const
	{
		if (auto resouce = mTexResourcePool->getDataResource(KarstAreaIDName); resouce)
		{
			return reinterpret_cast<KarstTileVTResource*>(resouce->mTileResource)->getTileRcTexture();
		}

		return nullptr;
	}

	void KarstGroup::addRequest(KarstNodeResourceRequest* request)
	{
		auto karstPage = getKarstPage(request->pageX, request->pageY);
		if (karstPage) {
			auto pageKey = EchoKarst::packIndex(request->pageX, request->pageY);
			if (auto it = requestMap.find(pageKey); it != requestMap.end()) {
				it->second.push_back(request);
			}
			else {
				std::vector<KarstNodeResourceRequest*> requests;
				requests.push_back(request);
				requestMap.insert(std::make_pair(pageKey, requests));
			}
		}
	}

	bool KarstGroup::getHeightMapBufferFromIndex(int pageX, int pageY, int NodeX, int NodeY, int size, float* pData) const
	{
		if (!mActive)
			return false;

		if (!checkKarstPageAvailable(pageX, pageY)) {
			return false;
		}

		return mRenderResource->getHeightMapBufferFromIndex(pageX, pageY, NodeX, NodeY, size, pData);
	}

	std::shared_ptr<HeightBuffer> KarstGroup::getHeightBufferFromWorldSpace(int page_index_x, int page_index_y, int page_size)
	{
		if (!mActive)
			return nullptr;

		return mRenderResource->getHeightBufferFromWorldSpace(page_index_x, page_index_y, page_size);
	}

	bool KarstGroup::computeHeightAtPos(const Vector2& pos, float& height, EchoIndex2 pageIndex, void* heightData, void* slopeData) const
	{
		if (!mActive)
			return false;

		return mRenderResource->computeHeightAtPos(pos, height, pageIndex, heightData, slopeData);
	}

	void KarstGroup::getHeightRawData(int pageX, int pageY, void** pHeightData, void** pSlopeData)
	{
		if (!mActive)
			return;
		mRenderResource->getHeightRawData(pageX, pageY, pHeightData, pSlopeData);
	}

	void KarstGroup::setBaseHeight(float height) {
		mNoiseOptions.height = height;
		mNoiseOptions.updateHashValue();
	}

	void KarstGroup::setFeatureNoiseSeed(const Vector2& seed) {
		mNoiseOptions.featureNoiseSeed = seed;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setLacunarity(float lacunarity) {
		mNoiseOptions.lacunarity = lacunarity;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setBaseFrequency(float frequency) {
		mNoiseOptions.baseFrequency = frequency;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setBaseAmplitude(float amplitude) {
		mNoiseOptions.baseAmplitude = amplitude;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setGain(float gain) {
		mNoiseOptions.gain = gain;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setBaseOctaves(int octaves) {
		mNoiseOptions.baseOctaves = octaves;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSharpnessNoiseSeed(const Vector2& seed)
	{
		mNoiseOptions.sharpnessNoiseSeed = seed;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSharpness(float expectation, float sigma3)
	{
		mNoiseOptions.sharpnessRange = Vector2(expectation, sigma3);
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSharpnessFrequency(float frequency)
	{
		mNoiseOptions.sharpnessFrequency = frequency;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSlopeErosionNoiseSeed(const Vector2& seed)
	{
		mNoiseOptions.slopeErosionNoiseSeed = seed;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSlopeErosion(float expectation, float sigma3)
	{
		mNoiseOptions.slopeErosionRange = Vector2(expectation, sigma3);
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setSlopeErosionFrequency(float frequency)
	{
		mNoiseOptions.slopeErosionFrequency = frequency;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setPerturbNoiseSeed(const Vector2& seed)
	{
		mNoiseOptions.perturbNoiseSeed = seed;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setPerturb(float expectation, float sigma3)
	{
		mNoiseOptions.perturbRange = Vector2(expectation, sigma3);
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setPerturbFrequency(float frequency)
	{
		mNoiseOptions.perturbFrequency = frequency;
		mNoiseOptions.updateHashValue();
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	void KarstGroup::setNoiseOptions(const KarstNoiseOptions& options)
	{
		mNoiseOptions = options;
		mRenderResource->setNoiseOptions(mNoiseOptions);
	}

	WorkQueue::RequestID KarstGroup::loadResourceRequest(LoadKarstResourceRequest* req)
	{
		return mWorkQueue->addRequest(mWorkQueueChannel, ReqType_KarstNodeMap, Any(req), 0, WorkQueue::RESOURCE_TYPE_TERRAIN, false);
	}

	bool KarstGroup::canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		const LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(req->getData());
		return this == resReq->karstGroup;
	}

	WorkQueue::Response* KarstGroup::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		WorkQueue::Response* response = nullptr;
		if (req->getType() == ReqType_KarstNodeMap)
		{
			LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(req->getData());
			if (auto karstBlock = resReq->karstGroup->getKarstBlock(resReq->pageX, resReq->pageY); karstBlock && karstBlock->instance) {
				mRenderResource->getRenderResource(karstBlock->instance.get(), resReq->requests);
				response = new WorkQueue::Response(req, true, Any(), "", "");
			}
			else {
				response = new WorkQueue::Response(req, false, Any(), "", "");
			}
		}
		else if (req->getType() == ReqType_KarstPlaneMesh)
		{
			LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(req->getData());
			resReq->pPlaneMesh.reset(new EchoKarstPlaneMesh("levels/" + mLevelName + "/waterway/water" + std::to_string(resReq->pageX) + "_" + std::to_string(resReq->pageY) + ".plane", false));
			resReq->pPlaneMesh->setInterspace(-5.0f);
			response = new WorkQueue::Response(req, true, Any(), "", "");
		}
		return response;
	}

	bool KarstGroup::canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		const LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(res->getRequest()->getData());

		return this == resReq->karstGroup && res->succeeded();
	}

	void KarstGroup::updateTileData(const KarstNodeResourceRequest* req)
	{
		if (req->dataType == KarstResourceDataType::HeightNormalMap) {
			if (int size = (int)req->pHeightData.size(); size) {
				mDataResourcePool->updateTileData(KarstResouceHeightMapName, req->index, req->tileID, (void*)req->pHeightData.data(), size * sizeof(uint32));
			}
			if (int size = (int)req->pMaterialData.size(); size) {
				mDataResourcePool->updateTileData(KarstResouceMaterialIDName, req->index, req->tileID, (void*)req->pMaterialData.data(), size * sizeof(uint8));
			}
			setTileUnLocked(req->index, req->tileID, req->dataType);
			setTileInfo(req->index, req->tileID, req->dataType, req->pNode, req->pageX, req->pageY);
		}
		else if (req->dataType == KarstResourceDataType::TextureMap) {
			if (int size = (int)req->pTextureData.size(); size) {
				mTexResourcePool->updateTileData(KarstResouceTextureName, req->index, req->tileID, (void*)req->pTextureData.data(), size * sizeof(uint32));
			}
			if (int size = (int)req->pAreaIDData.size(); size) {
				mTexResourcePool->updateTileData(KarstAreaIDName, req->index, req->tileID, (void*)req->pAreaIDData.data(), size * sizeof(uint32));
			}
			setTileUnLocked(req->index, req->tileID, req->dataType);
			setTileInfo(req->index, req->tileID, req->dataType, req->pNode, req->pageX, req->pageY);
		}
	}

	void KarstGroup::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		const WorkQueue::Request* req = res->getRequest();
		switch (req->getType())
		{
		case ReqType_KarstNodeMap:
		{
			if (!req->getAborted()) {
				LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(req->getData());
				auto& requests = resReq->requests;
				for (auto& request : requests) {
					auto karstPage = getKarstPage(request->pageX, request->pageY);
					if (karstPage) {
						updateTileData(request);
						karstPage->updateVirtalTableInfo(request);
					}
					delete request;
				}
				delete resReq;
			}
		}
		break;
		case ReqType_KarstPlaneMesh:
		{
			if (!req->getAborted()) {
				LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(req->getData());
				if (resReq->pPlaneMesh->getIsAvalible()) {
					bool nodata; int blockID;
					auto planMeshPtr = mPlaneMeshPool.retrieveBlock(0, EchoKarst::packIndex(resReq->pageX, resReq->pageY), nodata, blockID);
					if (planMeshPtr && nodata) {
						resReq->pPlaneMesh->createDynamicVertexBuffer();
						if (planMeshPtr->renderable) {
							if (auto it = mPlaneMeshSet.find(planMeshPtr->renderable->getKarstPlaneMesh()); it != mPlaneMeshSet.end()) {
								mPlaneMeshSet.erase(it);
							}
						}
						mPlaneMeshSet.insert(resReq->pPlaneMesh.get());
						planMeshPtr->renderable.reset(new KarstPlaneRenderable(std::move(resReq->pPlaneMesh)));
					}
				}
				auto it = mPlaneMeshReqSet.find(EchoKarst::packIndex((int)floor(resReq->pageX), (int)floor(resReq->pageY)));
				assert(it != mPlaneMeshReqSet.end());
				mPlaneMeshReqSet.erase(it);
				delete resReq;
			}
		}
		break;
		default:
			break;
		}
	}
	void KarstGroup::OutPutPrepareResourceLog(DataStreamPtr& out)
	{
		return; // TODO
		std::vector<WorkQueue::Request*> requestLists;
		mWorkQueue->getRequestList(mWorkQueueChannel, requestLists);

		for (const auto& req : requestLists)
		{
			if (req == nullptr || req->getAborted()) continue;

			Any pData = req->getData();

			if (pData.isEmpty()) continue;

			const LoadKarstResourceRequest* resReq = any_cast<LoadKarstResourceRequest*>(pData);
			KarstPage* karstPage = resReq->karstGroup->getKarstPage(resReq->pageX, resReq->pageY);

			if (!karstPage) continue;

			stringstream ss;
			mRenderResource->outputRenderResourceFile(karstPage, resReq->requests, ss);
		}
	}

	void KarstGroup::setupAreaFogData(const std::vector<AreaFogParam>& params)
	{
		if (!params.size())
			return;

		for (const AreaFogParam& param : params)
		{
			const uint32& idx = param.areaMaskID;
			if (idx >= mAreaFogBufferSize) {
				LogManager::instance()->logMessage("Karst: AreaFogParam areaMaskID over limit :" + std::to_string(idx));
				continue;
			}
			mAreaFogBuffer[idx] = Clamp(0.0f, 1.0f, param.density);
		}
		mAreaFogDirty = true;
	}

	void KarstGroup::setDisplayAreaBorder(uint32 areaMask, uint32 level)
	{
		mDisplayAreaLevel = level;
		mDisplayAreaBorder = 0;
	}
}
