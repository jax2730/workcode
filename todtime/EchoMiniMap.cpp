#include "EchoMiniMap.h"
#include "EchoSceneManager.h"
#include "EchoWorldManager.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActor.h"
#include "EchoKarstSystem.h"
#include "EchoKarstGroup.h"
#include "EchoKarstExportUtil.h"
#include "EchoTimer.h"
//#include "AsyncApp.h"
#include "EchoEngineImpl.h"
#include "EchoVideoConfigSystem.h"
#include "EchoSphericalController.h"
#include "../cpp-httplib/httplib.h"
#include "EchoWorldSystem.h"
#include"EchoTODManager.h"
extern Echo::Name mainWorld;
extern Echo::Name mapWorld;

namespace Echo
{
	static float INIT_SIZE_WIDTH = 1280.0f;
	static float INIT_SIZE_HIGHT = 720.0f;
	static float ADAPTATION_SCALE_X = 1.0f;
	static float ADAPTATION_SCALE_Y = 1.0f;

	static echofgui::Color32 FGUI_Color_Red(255, 255, 0, 0);
	static echofgui::Color32 FGUI_Color_Green(255, 0, 255, 0);
	static echofgui::Color32 FGUI_Color_Blue(255, 0, 0, 255);

	static std::map<uint32, echofgui::Color32> mAreaColors;

	static echofgui::Color32 CalculateAreaColor(uint32 InID)
	{
		if (mAreaColors.count(InID))
			return mAreaColors[InID];

		static int tempi = 2;
		static uint8 colorr = 25;
		colorr = Math::Min(255, 25 * tempi);
		++tempi;
		static uint8 colorg = 25;
		if (colorr >= 255)
		{
			static int tempj = 2;
			colorg = Math::Min(255, 25 * tempj);
			++tempj;
		}

		static uint8 colorb = 25;
		if (colorg >= 255)
		{
			static int tempk = 2;
			colorb = Math::Min(255, 25+2 * tempk);
			++tempk;
		}

		echofgui::Color32 tempColor(255, colorr, colorg, colorb);
		mAreaColors.emplace(InID, tempColor);
		return tempColor;
	}


	static bool CheckInRange(const ARect& InRect, const Vector3& InPos)
	{
		Vector2 vfMaxRange;
		vfMaxRange.x = InRect.Start.x + InRect.Range.x;
		vfMaxRange.y = InRect.Start.y + InRect.Range.y;
		return   InRect.Start.x < InPos.x && vfMaxRange.x > InPos.x
			&& InRect.Start.y < InPos.z && vfMaxRange.y > InPos.z;
	}

	static bool CheckInRange(const Vector2& InMin, const Vector2& InMax, const Vector2& InPos)
	{
		
		return   InMin.x < InPos.x && InMin.y < InPos.y
			&& InMax.x > InPos.x && InMax.y > InPos.y;
	}

	static Vector3 FguiVecToEchoVec(const echofgui::Vec3& InVec)
	{
		Vector3 Result;
		Result.x = InVec.x;
		Result.y = InVec.y;
		Result.z = InVec.z;
		return Result;
	}

	static echofgui::Vec3 EchoVecToFguiVec(const Vector3& InVec)
	{
		echofgui::Vec3 Result;
		Result.x = InVec.x;
		Result.y = InVec.y;
		Result.z = InVec.z;
		return Result;
	}

#define WORLD_BLOCK_SIZE 1000

	static bool GenerateBlockID(const Vector3& InWorldPos, MiniMapManager::BlockID& OutID)
	{
		
		int CurrentBlockX = (int)std::floor( InWorldPos.x / WORLD_BLOCK_SIZE);
		int CurrentBlockY = (int)std::floor(InWorldPos.z / WORLD_BLOCK_SIZE);

		if (CurrentBlockX > 32767 ||
			CurrentBlockY > 32767 ||
			CurrentBlockX < -32768 ||
			CurrentBlockY < -32768)
		{
			return false;
		}

		OutID.mX = (int16)CurrentBlockX;
		OutID.mY = (int16)CurrentBlockY;
		return true;
	}

	static bool isMiniMap = false;


	void onClickMiniMap(echofgui::EventContext* context)
	{
		static bool isActive = false;

		GetEchoEngine()->ChangeWorld(isMiniMap ? Echo::Name("Main") : mapWorld);
		isMiniMap = !isMiniMap;
		if (isMiniMap) {
			//VideoConfigSystem::instance()->getActiveVideoConfig()->setHDREnable(false);
			//VideoConfigSystem::instance()->getActiveVideoConfig()->setColorGradingEnable(false);
			isActive = EchoSphericalController::Alive();
			if (isActive)
			{
				delete EchoSphericalController::instance();
			}
			SceneManager* sceneManager = Echo::Root::instance()->getActiveSceneManager();
			if (sceneManager && sceneManager->getMainCamera())
			{
				DVector3 oriPos{ 0.0 ,100.0, 0.0 };
				Quaternion oriDir{ -0.236106098, -0.0335169360, -0.00814899895, 0.971120179 };
				sceneManager->getMainCamera()->setPosition(oriPos);
				sceneManager->getMainCamera()->setOrientation(oriDir);
			}
		}
		else
		{
			if (isActive)
			{
				new EchoSphericalController(Echo::Root::instance()->getMainSceneManager()->getMainCamera());
			}
		}
	}

	void MiniMapManager::update(const WeatherInfo& info)
	{
		if (mWeatherUICom && mTemperatureUI.mObject)
		{
			const WeatherDetailInfo& detailInfo = static_cast<const WeatherDetailInfo&>(info);
			String uiURL = "ui://chinese_minimap/" + detailInfo.ui;
			mWeatherUICom->staticImageMode(uiURL.c_str());
			echofgui::GObject* pObject = mTemperatureUI.mObject->as<echofgui::GComponent>()->getChild("n0");
			pObject->setText(std::to_string(detailInfo.temperature) + " " + detailInfo.desc);
		}
	}

	MiniMapManager::MiniMapManager(SceneManager* InSceneManger, WorldManager* InWorldMgr)
		: mSceneMgr(InSceneManger)
		, mWorldMgr(InWorldMgr)
	{
		mPageManager = new CityMapPageManager();

		mWorkQueue = Root::instance()->getWorkQueue();
		mWorkQueueChannel = mWorkQueue->getChannel("Echo/MiniMapLoad");
		mWorkQueue->addResponseHandler(mWorkQueueChannel, this);
		mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
		//mWeatherJob = new WeatherRequestJob(this);
		//mTrainActor = mWorldMgr->getActorManager()->createActor(Name("actors/stadium.act"), "", true);

		KarstSystem::instance()->addCreatedListener(std::bind(&MiniMapManager::OnKarstCreateEvent, this, std::placeholders::_1));
		auto pKarstGrpup = KarstSystem::instance()->getKarstGroup();
		if (!pKarstGrpup)
			return;

		mbWorldMap = pKarstGrpup->getLevelName().compare("karst2") != 0;
		mWorldOrigin = pKarstGrpup->getOriginPosition();
		mTerrainWidth = pKarstGrpup->getRowNum() * pKarstGrpup->getOptions()->mGridSize;// 16 * 32768.0f;
		mTerrainHeight = pKarstGrpup->getColNum() * pKarstGrpup->getOptions()->mGridSize;//10 * 32768.0f;
		assert(mSceneMgr != nullptr);
		assert(mWorldMgr != nullptr);
		//mMapHeight = 386.0f;
		mbIsCreate = CreateMap();
		std::string sLevelName = "levels/" + mWorldMgr->getLevelName();
		LoadCityMap(sLevelName + "/other/city_output.txt");
		LoadCityBuildingMap(sLevelName + "/other/ActorResourceInfo.json");
		//LoadCityArea("minimap/beijingadministrations.txt", "minimap/beijing_grid_info.json");

		mbInit = true;
	}

	MiniMapManager::~MiniMapManager()
	{
		mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
		mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
		mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);
		SAFE_DELETE(mPageManager);
		//SAFE_DELETE(mWeatherJob);
		Uninitilize();

		if (mPlaneManager)
		{
			delete mPlaneManager;
		}
	}

	bool MiniMapManager::OnActorCreateFinish()
	{
		Uninitilize();
		Initilize();
		return true;
	}

	void MiniMapManager::Initilize()
	{
		if (mbInit)
			return;

		auto pKarstGrpup = KarstSystem::instance()->getKarstGroup();
		if (!pKarstGrpup)
			return;

		mWorldOrigin = pKarstGrpup->getOriginPosition();
		mTerrainBlockSize = pKarstGrpup->getOptions()->mKarstSize;
		mTerrainWidth = pKarstGrpup->getRowNum() * pKarstGrpup->getOptions()->mKarstSize;// 16 * 32768.0f;
		mTerrainHeight = pKarstGrpup->getColNum() * pKarstGrpup->getOptions()->mKarstSize;//10 * 32768.0f;
		assert(mSceneMgr != nullptr);
		assert(mWorldMgr != nullptr);
		//mMapHeight = 386.0f;
		mbIsCreate = CreateMap();
		std::string sLevelName = "levels/" + mWorldMgr->getLevelName();
		LoadCityMap(sLevelName + "/other/city_output.txt");
		LoadCityBuildingMap(sLevelName + "/other/ActorResourceInfo.json");
		//LoadCityArea("minimap/beijingadministrations.txt", sLevelName + "/other/");
		Root::instance()->addFrameListener(this);

		AddMiniMapEventListener(echofgui::UIEventType::TouchBegin, onClickMiniMap);
		KarstExportUtil::AddWeatherListener(this);

		DataStreamPtr pLineDataStream(Root::instance()->GetFileDataStream(("levels/" + mWorldMgr->getLevelName() + "/railway/train.line").c_str(), true));
		if (!pLineDataStream.isNull()) {
			while (true) {
				int pointSize;
				pLineDataStream->readData(&pointSize);
				if (pLineDataStream->eof())
					break;
				std::vector<Vector2> points(pointSize);
				pLineDataStream->readData(points.data(), pointSize);

				auto line = std::make_shared<TrainLineData>(KarstSystem::instance()->getKarstGroup(), points);

				int num = int(ceilf(line->mTotalLength / 7230.0f));
				for (int i = 0; i < num; i++)
					line->mMoveActors.push_back(new TrainActor("huochetou.act", "huocheshen.act", 5, 65.0f, i * 7230.0f, 10.0f, line.get()));

				mTrainLines.push_back(line);
			}
		}

		mbInit = true;
	}

	void MiniMapManager::Uninitilize()
	{
		if (!mbInit)
			return;

		mbInit = false;
		if (mbIsCreate)
		{
			//mShowMapUI.mObject->removeEventListener(echofgui::UIEventType::TouchBegin, echofgui::EventTag(mShowMapUI.mObject));
			mChangeFullMapUI.mObject->removeEventListener(echofgui::UIEventType::TouchBegin, echofgui::EventTag(mChangeFullMapUI.mObject));
			mWorldMgr->getActorManager()->deleteActor(mMapUIActor);
			mWorldMgr->getActorManager()->deleteActor(mWeatherUIActor);
			mWorldMgr->getActorManager()->deleteActor(mTemperatureUIActor);
			mWorldMgr->getActorManager()->deleteActor(mElevationUIActor);
			mMapUIActor = nullptr;
			mMapUICom = nullptr;
			mWeatherUIActor = nullptr;
			mWeatherUICom = nullptr;
			mTemperatureUIActor = nullptr;
			mTemperatureUICom = nullptr;
			mElevationUIActor = nullptr;
			mElevationUICom = nullptr;
			mbIsCreate = false;
			mBuildings.clear();
			mWorldBuildingUIs.clear();
			mAdminRegions.clear();
			mAdminRegionAreaStrs.clear();
			mAreaStrs.clear();
			mAreas.clear();
			mAreaTexs.clear();
			for (auto&& block : mWorldBlocks)
			{
				for (auto&& info : block.second.mBuildingInfos)
				{
					if (info.first)
						delete info.first;
				}
			}
			mWorldBlocks.clear();
			for (auto&& pActor : mCreateActors)
			{
				mWorldMgr->getActorManager()->deleteActor(pActor);
			}
			mCreateActors.clear();
			//mMapObject = nullptr;
			//mMapUI = nullptr;
			//mPlayerUI = nullptr;
		}
		Root::instance()->removeFrameListener(this);
		KarstExportUtil::RemoveWeatherListener(this);
	}

	void MiniMapManager::LoadCityMap(const std::string& InPath)
	{
		if (!mbIsCreate)
			return;

		echofgui::UIPackage::addPackage("fgui/bytes/chinese_minimap");
		
		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(InPath.c_str(), false, "txt"));
		if (pDataStream.isNull() || pDataStream->size() == 0)
		{
			return;
		}
		std::string LineCity;
		while (!pDataStream->eof())
		{
			LineCity = pDataStream->getLine(true);
			std::vector<std::string> CityName = StringUtil::split(LineCity, "(");

			if (CityName.size() > 1)
			{
				std::vector<std::string> CityPosition = StringUtil::split(CityName[1].substr(0, CityName[1].size() - 1), ",");
				CityUI TempCtUI;
				for (size_t i = 0; i < CityPosition.size(); ++i)
				{
					std::string RealCityPos;
					for (size_t j = 0; j < CityPosition[i].size(); ++j)
					{
						if (CityPosition[i][j] == ' ')
							continue;
						RealCityPos.push_back(CityPosition[i][j]);
					}
					TempCtUI.mWorldPosition[i] = std::stof(RealCityPos);
				}
				TempCtUI.mMapPosition = GetPosInMapByWorldPos(TempCtUI.mWorldPosition);

				TempCtUI.mCityUIObject = echofgui::UIPackage::createObject("chinese_minimap", "text");
				
				if (TempCtUI.mCityUIObject)
				{
					TempCtUI.mCityNameObject = TempCtUI.mCityUIObject->as<echofgui::GComponent>()->getChild("n0");
					if (TempCtUI.mCityNameObject)
					{
						CreateCityUI(TempCtUI);
						TempCtUI.mMapPosition -= Vector2(TempCtUI.mCityNameObject->getActualWidth() / 2.0f, TempCtUI.mCityNameObject->getActualHeight() / 2.0f);
						TempCtUI.mCityNameObject->setPosition(TempCtUI.mMapPosition.x, TempCtUI.mMapPosition.y);
						TempCtUI.mCityNameObject->setText(CityName[0]);
						TempCtUI.mCityNameObject->setVisible(mbShowCityName);
						TempCtUI.mCityName = CityName[0];
					}
				}
				mCityMap.push_back(TempCtUI);
			}
		}


		
	}

	void MiniMapManager::LoadCityBuildingMap(const std::string& InPath)
	{
		if (!mbIsCreate)
			return;



		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(InPath.c_str(), false, "json"));
		if (pDataStream.isNull() || pDataStream->size() == 0)
		{
			return;
		}

		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->seek(0);
		pDataStream->read(pData, nSize);
		cJSON* pRoot = cJSON_Parse(pData);
		SAFE_DELETE_ARRAY(pData);

		if (pRoot == nullptr)
		{
			return;
		}
		
		std::string TempBuildingTypeStr;

		//这里创建的是小地图中显示的小色块
		auto CreateMapBlock = [&](const Vector3& InPosition, const echofgui::Color32& InColor)
			{
				auto tempUI = echofgui::UIPackage::createObject("chinese_minimap", "block")->as<echofgui::GComponent>()->getChild("n0");
				mMapUI.mObject->as<echofgui::GComponent>()->addChild(tempUI);
				Vector2 MapPos = GetPosInMapByWorldPos(InPosition);// -Vector2(tempUI->getWidth(), tempUI->getHeight());
				tempUI->setPosition(MapPos.x, MapPos.y);
				tempUI->setWidth(4.0f);
				tempUI->setHeight(4.0f);
				tempUI->setColor(InColor);
			};

		//这里只是读取场景中功能建筑的属性信息，并未实际创建对象，实际对象的创建是在NotifyCameraVisible()的时候根据当前相
		// 机位置动态加载和卸载的，因为ui不支持异步，所以采用了分帧加载卸载，放在了帧末，可以在frameEnded()函数中找到相关代码。
		for (cJSON* pChild = pRoot->child; nullptr != pChild; pChild = pChild->next)
		{
			cJSON* pBuildingArray = cJSON_GetObjectItem(pChild, "data");
			Vector3 readBound;
			ReadJSNode(pChild, "bbox", readBound);
			if (pBuildingArray)
			{
				for (cJSON* pBuilding = pBuildingArray->child; nullptr != pBuilding; pBuilding = pBuilding->next)
				{
					if (pChild->string)
					{
						TempBuildingTypeStr = pChild->string;
					}
					bool isActive = true;
					ReadJSNode(pBuilding, "isActive", isActive);
					if (!isActive)
					{
						continue;
					}
					BlockInstance::BuildingInfo* TempInfo = new BlockInstance::BuildingInfo();
					ReadJSNode(pBuilding, "name", TempInfo->mName);
					ReadJSNode(pBuilding, "pos", TempInfo->mPosition);
					
					bool isRead = ReadJSNode(pBuilding, "bbox", TempInfo->mBound);
					TempInfo->mWorldPosition = TempInfo->mPosition;
					if (isRead)
					{
						TempInfo->mWorldPosition.y += TempInfo->mBound.y;
					}
					else
					{
						TempInfo->mWorldPosition.y += readBound.y;
					}

					TempInfo->mWorldPosition.y += 3.0f;
					TempInfo->mWorldPosition.x -= 18.0f;
					TempInfo->mScale = Vector3(56.0f);
					if (TempBuildingTypeStr == "City")
					{
						TempInfo->mWorldPosition.x -= 25.0f * 18;
						TempInfo->mScale = Vector3(2000.0f);
					}

					//创建小地图中显示的小块
					if (TempBuildingTypeStr == "City")
					{
						CreateMapBlock(TempInfo->mPosition, FGUI_Color_Green);
					}
					else if (TempBuildingTypeStr == "DT36" || TempBuildingTypeStr == "DT10" || TempBuildingTypeStr == "FD72")
					{
						CreateMapBlock(TempInfo->mPosition, FGUI_Color_Blue);
					}

					BlockID CurrentBlockID;
					bool isGen = GenerateBlockID(TempInfo->mWorldPosition, CurrentBlockID);
					if (isGen)
					{
						mWorldBlocks[CurrentBlockID].mBuildingInfos.emplace(TempInfo, nullptr);
					}
				}
				
			}
			
		}

		if (!mPlaneManager)
		{
			mPlaneManager = new AirPlaneManager();
			//std::map<int, std::vector<Vector3>> posMap;
			std::vector<std::pair<int, Vector3>> areaPosVec;
			cJSON* airport = cJSON_GetObjectItem(pRoot, "Airport");
			cJSON* dataNode = cJSON_GetObjectItem(airport, "data");
			int len = cJSON_GetArraySize(dataNode);
			for (int i = 0; i < len; ++i)
			{
				cJSON* ele = cJSON_GetArrayItem(dataNode, i);
				bool isActive = false;
				READJS(ele, isActive)
				if (isActive)
				{
					int area = 1;
					READJS(ele, area);
					Vector3 pos;
					ReadJSNode(ele, "pos", &pos.x, 3);
					areaPosVec.push_back(std::pair(area, pos));
				}
			}

			for (int i = 0, lastJ = i; i < areaPosVec.size(); ++i)
			{
				auto& ele = areaPosVec[i];
				int area = ele.first;
				int j = (lastJ + 1) % areaPosVec.size();

				while (j != lastJ)
				{
					auto& nextEle = areaPosVec[j];
					if (nextEle.first == area)
					{
						j = (j + 1) % areaPosVec.size();
					}
					else
					{
						lastJ = j;
						mPlaneManager->addAirLine(ele.second, nextEle.second);
						break;
					}
					
				}
			}

		}

		cJSON_Delete(pRoot);
	}

	void MiniMapManager::LoadCityArea(const std::string& InCodePath, const std::string& InLevelPath)
	{
		mAreaColors.emplace(0, echofgui::Color32(255, 255, 255, 255));

		//mAreaMapUI = CreateUIFromResource("chinese_minimap", "areas");//echofgui::UIPackage::createObject("chinese_minimap", "areas");
		//加载行政区域代码
		{
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(InCodePath.c_str(), false, "txt"));
			if (pDataStream.isNull() || pDataStream->size() == 0)
			{
				return;
			}
			std::string LineCity;
			while (!pDataStream->eof())
			{
				LineCity = pDataStream->getLine(true);
				std::vector<std::string> CityName = StringUtil::split(LineCity, ":");
				if (CityName.size() != 2)
					continue;
				mAdminRegions.emplace(std::stoul(CityName[0]), CityName[1]);
				mAdminRegionAreaStrs.emplace(std::stoul(CityName[0]), CityName[0].substr(0, 6));
			}
		}

		{
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream("minimap/beijingqu.txt", false, "txt"));
			if (pDataStream.isNull() || pDataStream->size() == 0)
			{
				return;
			}
			std::string LineCity;
			while (!pDataStream->eof())
			{
				LineCity = pDataStream->getLine(true);
				std::vector<std::string> CityName = StringUtil::split(LineCity, ":");
				if (CityName.size() != 2)
					continue;
				mAreaStrs.emplace(CityName[0].substr(0,6), CityName[1]);
			}
		}

		//读取依赖资源
		bool bReadDependency = false;
		struct DependencyResource
		{
			int x = 0;
			int y = 0;
			std::string path;
		};

		std::vector<DependencyResource> mDDRs;
		{
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream((InLevelPath + "minimap_resource.txt").c_str(), false, "txt"));
			if (pDataStream.isNull() || pDataStream->size() == 0)
			{
				return;
			}
			
			while (!pDataStream->eof())
			{
				int tempcount = std::stoi(pDataStream->getLine(true));
				for (int i = 0; i < tempcount; ++i)
				{
					DependencyResource ddr;
					ddr.path = pDataStream->getLine(true);
					std::string serialNumber = pDataStream->getLine(true);
					std::vector<std::string> serialNumbers = StringUtil::split(serialNumber, ",");
					if (serialNumbers.size() != 2)
						continue;
					ddr.x = std::stoi(serialNumbers[0]);
					ddr.y = std::stoi(serialNumbers[1]);
					mDDRs.push_back(ddr);
				}
			}
		}

		//加载区域信息
		mAreaTexs.resize(mDDRs.size());

		{

			for (size_t index = 0; index < mDDRs.size(); ++index)
			{
				mAreaTexs[index].mWorldOrigin = Vector2(mWorldOrigin.x + mDDRs[index].x * mTerrainBlockSize, mWorldOrigin.z + mDDRs[index].y * mTerrainBlockSize);
				mAreaTexs[index].mWorldMaxRange = mAreaTexs[index].mWorldOrigin + mTerrainBlockSize;
				mAreaTexs[index].mAreaTexData = std::vector<std::vector<uint32>>(1024, std::vector<uint32>(1024, 0));

				DataStreamPtr baseTexStream(Root::instance()->GetFileDataStream((InLevelPath + mDDRs[index].path).c_str()));
				if (!baseTexStream.isNull())
				{
					for (int i = 0; i < 1024; ++i)
					{
						for (int j = 0; j < 1024; ++j)
						{
							baseTexStream->readData(&mAreaTexs[index].mAreaTexData[j][i]);

							//auto tempUI = echofgui::UIPackage::createObject("chinese_minimap", "block")->as<echofgui::GComponent>()->getChild("n0");
							//mAreaMapUI.mObject->as<echofgui::GComponent>()->addChild(tempUI);
							//tempUI->setWidth(2);
							//tempUI->setHeight(2);
							//float posUIx = tempUI->getSize().width * j;
							//float posUIy = tempUI->getSize().height * (i);
							//tempUI->setPosition(posUIx, posUIy);
							//tempUI->setColor(CalculateAreaColor(mAreaTexs[0].mAreaTexData[j][i]));
						}
					}
					//mAreaTexs[0].setVisible(false);
				}
			}
		}		
	}

	void MiniMapManager::CreateCityUI(CityUI& InOutCity)
	{
		mMapObject.mObject->as<echofgui::GComponent>()->addChild(InOutCity.mCityNameObject);
	}

	bool MiniMapManager::CreateMap()
	{
		echofgui::UIPackage::addPackage("fgui/bytes/chinese_minimap");

		uint32 UWidth, UHeight;
		Root::instance()->getRenderSystem()->getWindowSize(UWidth, UHeight);
		float RealWindowWidthWithOneSideOffset = UWidth * 0.95f;
		float RealWindowHeightWithOneSideOffset = UHeight * 0.95f;
		ADAPTATION_SCALE_X = UWidth / INIT_SIZE_WIDTH;
		ADAPTATION_SCALE_Y = UHeight / INIT_SIZE_HIGHT;
		ADAPTATION_SCALE_X = std::min(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
		ADAPTATION_SCALE_Y = ADAPTATION_SCALE_X;

		//echofgui::UIPackage::addPackage("fgui/bytes/chinese_minimap");
		//echofgui::UIPackage::createObject("chinese_minimap", "chinese_minimap");
		mMapUIActor = mWorldMgr->getActorManager()->createActor(Name("actors/chinese_minimap.act"), "", true);
		if (mMapUIActor.Expired())
			return false;

		mMapUICom = mMapUIActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
		if (mMapUICom == nullptr)
			return false;
		
		mMapObject.mObject = mMapUICom->getGObject();
		if (!mMapObject.mObject)
			return false;

		{
			mMapObject.mInitPos = FguiVecToEchoVec(mMapObject.mObject->getPosition());
			mMapObject.mCurrentPos = mMapObject.mInitPos;
			mMapObject.mInitSize = Vector3(mMapObject.mObject->getActualWidth() , mMapObject.mObject->getActualHeight() , 0.0f);
			mMapObject.mCurrentSize = mMapObject.mInitSize;
		}

		//map box ui
		{
			if (!InitUIObjectProperty(mMapRangeUI, mMapObject.mObject, "n1"))
				return false;
			mMapRangeUI.mObject->setSortingOrder(10);
		}

		//map texture ui
		{
			//if (!InitUIObjectProperty(mMapUI, mMapObject.mObject, "n3"))
			//	return false;

			//mMapUI.mObject->setVisible(false);
			mMapUI.mObject = echofgui::UIPackage::createObject("chinese_minimap", mbWorldMap ? "worldmapall" : "chinesemapall");
			if (!mMapUI.mObject)
				return false;

			mMapUI.mInitPos = FguiVecToEchoVec(mMapUI.mObject->getPosition());
			mMapUI.mCurrentPos = mMapUI.mInitPos;
			mMapUI.mInitSize = Vector3(mMapUI.mObject->getActualWidth(), mMapUI.mObject->getActualHeight(), 0.0f);
			mMapUI.mCurrentSize = mMapObject.mInitSize;
			mMapUI.mObject->setSortingOrder(9);
			mMapObject.mObject->as<echofgui::GComponent>()->addChild(mMapUI.mObject);
		}

		//player ui
		{
			if (!InitUIObjectProperty(mPlayerUI, mMapObject.mObject, "n4"))
				return false;
			mPlayerUI.mObject->setSortingOrder(10);
		}

		//weather
		{
			mWeatherUIActor = mWorldMgr->getActorManager()->createActor(Name("actors/weather.act"), "", true);
			if (mWeatherUIActor.Expired())
			{
				LogManager::instance()->logMessage("[weather]!!! -> mWeatherUIActor.Expired()");
				return false;
			}
			mWeatherUICom = mWeatherUIActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
			if (mWeatherUICom == nullptr)
			{
				LogManager::instance()->logMessage("[weather]!!! -> mWeatherUICom == nullptr");
				return false;
			}

			mWeatherUI.mObject = mWeatherUICom->getGObject();
			if (!mWeatherUI.mObject)
			{
				LogManager::instance()->logMessage("[weather]!!! -> !mWeatherUI.mObject");
				return false;
			}

			mWeatherUI.mInitPos = FguiVecToEchoVec(mWeatherUI.mObject->getPosition());
			mWeatherUI.mCurrentPos = mWeatherUI.mInitPos;
			mWeatherUI.mInitSize = Vector3(mWeatherUI.mObject->getActualWidth(), mWeatherUI.mObject->getActualHeight(), 0.0f);
			mWeatherUI.mCurrentSize = mWeatherUI.mInitSize;
			mWeatherUI.mObject->setSortingOrder(9);

			//mTemperatureUI.mObject = echofgui::UIPackage::createObject("chinese_minimap", "text");
			//if (mTemperatureUI.mObject)
			//{
			//	mMapObject.mObject->as<echofgui::GComponent>()->addChild(mTemperatureUI.mObject);
			//	mTemperatureUI.mObject->setSortingOrder(10);
			//	//mTemperatureUI.mObject->setPosition(50, 0);
			//	/*mTemperatureUI.mObject->setWidth(36);
			//	mTemperatureUI.mObject->setHeight(16);*/
			//	echofgui::GObject*  pObject = mTemperatureUI.mObject->as<echofgui::GComponent>()->getChild("n0");
			//	pObject->setPosition((mWeatherUI.mObject->getActualWidth() - pObject->getActualWidth()) / 2, mWeatherUI.mObject->getActualHeight() * 0.8f);

			//}

			mTemperatureUIActor = mWorldMgr->getActorManager()->createActor(Name("actors/temperature.act"), "", true);
			if (mTemperatureUIActor.Expired())
			{
				LogManager::instance()->logMessage("[weather]!!! -> mTemperatureUIActor.Expired()");
				return false;
			}
			mTemperatureUICom = mTemperatureUIActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
			if (mTemperatureUICom == nullptr)
			{
				LogManager::instance()->logMessage("[weather]!!! -> mTemperatureUICom == nullptr");
				return false;
			}

			mTemperatureUI.mObject = mTemperatureUICom->getGObject();
			if (!mTemperatureUI.mObject)
			{
				LogManager::instance()->logMessage("[weather]!!! -> !mTemperatureUI.mObject");
				return false;
			}

			mTemperatureUI.mInitPos = FguiVecToEchoVec(mTemperatureUI.mObject->getPosition());
			mTemperatureUI.mCurrentPos = mTemperatureUI.mInitPos;
			mTemperatureUI.mInitSize = Vector3(mTemperatureUI.mObject->getActualWidth(), mTemperatureUI.mObject->getActualHeight(), 0.0f);
			mTemperatureUI.mCurrentSize = mTemperatureUI.mInitSize;
			mTemperatureUI.mObject->setSortingOrder(9);


			mElevationUIActor = mWorldMgr->getActorManager()->createActor(Name("actors/elevation.act"), "", true);
			if (mElevationUIActor.Expired())
			{
				LogManager::instance()->logMessage("[weather]!!! -> mElevationUIActor.Expired()");
				return false;
			}
			mElevationUICom = mElevationUIActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
			if (mElevationUICom == nullptr)
			{
				LogManager::instance()->logMessage("[weather]!!! -> mElevationUICom == nullptr");
				return false;
			}

			mElevationUI.mObject = mElevationUICom->getGObject();
			if (!mElevationUI.mObject)
			{
				LogManager::instance()->logMessage("[weather]!!! -> !mElevationUI.mObject");
				return false;
			}

			mElevationUI.mInitPos = FguiVecToEchoVec(mElevationUI.mObject->getPosition());
			mElevationUI.mCurrentPos = mElevationUI.mInitPos;
			mElevationUI.mInitSize = Vector3(mElevationUI.mObject->getActualWidth(), mElevationUI.mObject->getActualHeight(), 0.0f);
			mElevationUI.mCurrentSize = mElevationUI.mInitSize;
			mElevationUI.mObject->setSortingOrder(9);
		}
		

		//current show city ui
		{
			mCurCityUI = CreateUIFromResource("chinese_minimap", "cityname");
		}
		
		if (!mCurCityUI.mObject)
			return false;

		//show current city name ui
		{
			if (!InitUIObjectProperty(mCurCityNameUI, mCurCityUI.mObject, "n0"))
				return false;
		}

		// show map button
		{
			if (!InitUIObjectProperty(mShowMapUI, mCurCityUI.mObject, "n1"))
				return false;
			//mShowMapUI.mObject->addEventListener(echofgui::UIEventType::TouchBegin, std::bind(&MiniMapManager::OnClickShowMap, this, std::placeholders::_1), echofgui::EventTag(mShowMapUI.mObject));
		}

		//show current position ui
		{
			if (!InitUIObjectProperty(mCurPositionUI, mCurCityUI.mObject, "n2"))
				return false;
		}

		//show shange full map ui
		{
			if (!InitUIObjectProperty(mChangeFullMapUI, mCurCityUI.mObject, "n3"))
				return false;
			//mMapObject.mObject->as<echofgui::GComponent>()->addChild(mChangeFullMapUI.mObject);
			mChangeFullMapUI.mObject->setSortingOrder(12);
			mChangeFullMapUI.mObject->removeEventListener(echofgui::UIEventType::TouchBegin, echofgui::EventTag(mChangeFullMapUI.mObject));

			mChangeFullMapUI.mObject->addEventListener(echofgui::UIEventType::TouchBegin, std::bind(&MiniMapManager::OnClickChangeFullMap, this, std::placeholders::_1), echofgui::EventTag(mChangeFullMapUI.mObject));


			{
				if (InitUIObjectProperty(mCloseFullMapUI, mCurCityUI.mObject, "n10"))
				{
					mCloseFullMapUI.mObject->setSortingOrder(13);
					// n10 关闭
					mCloseFullMapUI.mObject->addEventListener(echofgui::UIEventType::TouchBegin, std::bind(&MiniMapManager::OnClickChangeFullMap, this, std::placeholders::_1), echofgui::EventTag(mCloseFullMapUI.mObject));
					mCloseFullMapUI.mObject->setVisible(false);
				}
			}
		}

		// current area ui
		{
			if (!InitUIObjectProperty(mCurAreaUI, mCurCityUI.mObject, "n4"))
				return false;
		}

		{
			UIObjectProperty TempObj;
			if (!InitUIObjectProperty(TempObj, mMapObject.mObject, "n5"))
				return false;

			TempObj.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
		}

		{
			mMapObject.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset);
			mMapObject.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);

			mWeatherUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset);
			mWeatherUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);

			mTemperatureUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset + mWeatherUI.mInitSize.y * ADAPTATION_SCALE_Y * 0.8);
			mTemperatureUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);

			mElevationUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset + mWeatherUI.mInitSize.y * ADAPTATION_SCALE_Y * 0.8 + mTemperatureUI.mInitSize.y * ADAPTATION_SCALE_Y);
			mElevationUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);

			mChangeFullMapUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset + mMapRangeUI.mCurrentSize.y * ADAPTATION_SCALE_Y);
			mChangeFullMapUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			mCurCityNameUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X - mCurCityNameUI.mCurrentSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset);
			mCurCityNameUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			mCurPositionUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X - mCurCityNameUI.mCurrentSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset + mCurCityNameUI.mObject->getSize().height * ADAPTATION_SCALE_Y);
			mCurPositionUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			//mCurAreaUI.mObject->setPosition(RealWindowWidth - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X - mCurCityNameUI.mCurrentSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeight + mCurCityNameUI.mObject->getSize().height * ADAPTATION_SCALE_Y);
			//mCurAreaUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			mShowMapUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset);
			mShowMapUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			mCurCityUI.mObject->setWidth(UWidth);
			mCurCityUI.mObject->setHeight(UHeight);
//#if ECHO_PLATFORM == ECHO_PLATFORM_ANDROID
			//mShowMapUI.mObject->setVisible(false);
			mChangeFullMapUI.mObject->setVisible(true);
			//mCurPositionUI.mObject->setVisible(false);
//#endif
			mMapWidth = mMapUI.mInitSize.x;
			mMapHeight = mMapUI.mInitSize.y;
			//mMapWidth *= ADAPTATION_SCALE_X;
			//mMapHeight *= ADAPTATION_SCALE_Y;
		}

		return true;
	}

	void MiniMapManager::MoveMap()
	{

	}

	void MiniMapManager::UpdatePlayerPosition(const Vector3& InPosition)
	{
		mPlayerWorldPosition = InPosition;
	}

	void MiniMapManager::NotifyCameraVisible()
	{
		if (!mbShowSceneUI)
			return;

		BlockID CurrentBlockID;
		bool isGen = GenerateBlockID(mPlayerWorldPosition, CurrentBlockID);
		if (!isGen /* || mLastCameraBlockID.mID == CurrentBlockID.mID*/)
			return;

		auto CameraRotation = mSceneMgr->getMainCamera()->getDerivedOrientation().Inverse();
		Vector3 CameraRotationVec3 = CameraRotation.ToEuler();

		float SquaredDis = mSceneUIRenderDistance * mSceneUIRenderDistance;

		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				int tempX = CurrentBlockID.mX + x;
				int tempY = CurrentBlockID.mY + y;

				if (tempX > 32767 ||
					tempY > 32767 ||
					tempX < -32768 ||
					tempY < -32768)
				{
					continue;
				}

				BlockID TempID;
				TempID.mX = (int16)tempX;
				TempID.mY = (int16)tempY;

				auto iter = mWorldBlocks.find(TempID);

				if (iter != mWorldBlocks.end())
				{
					if (iter->second.mState == BlockInstance::NONE )
					{
						//std::lock_guard<std::mutex> lock_range(mMtx);
						mWaitLoadBlocks.insert(TempID);
						iter->second.mState = BlockInstance::PREPARE_LOAD;
						continue;
					}
					else if (iter->second.mState == BlockInstance::PREPARE_UNLOAD)
					{
						iter->second.mState = BlockInstance::LOADED;
						mWaitUnloadBlocks.erase(TempID);
					}

					if (iter->second.mState == BlockInstance::LOADED)
					{
						for (auto&& build : iter->second.mUIComponents)
						{
							if (build->getWorldPosition().squaredDistance(mPlayerWorldPosition) < SquaredDis)
							{
								build->setRuntimeVisible(true);

								if (mbShowSceneUI)
								{
									build->getGObject()->setRotation(EchoVecToFguiVec(CameraRotationVec3));
								}
							}
							else
							{
								build->setRuntimeVisible(false);
							}
						}
					}

				}
			}
		}

		if (mLastCameraBlockID.mID != CurrentBlockID.mID)
		{
			for (int x = -1; x <= 1; ++x)
			{
				for (int y = -1; y <= 1; ++y)
				{
					int tempX = mLastCameraBlockID.mX + x;
					int tempY = mLastCameraBlockID.mY + y;

					if (tempX > 32767 ||
						tempY > 32767 ||
						tempX < -32768 ||
						tempY < -32768)
					{
						continue;
					}

					BlockID TempID;
					TempID.mX = (int16)tempX;
					TempID.mY = (int16)tempY;

					auto iter = mWorldBlocks.find(TempID);

					if (iter != mWorldBlocks.end())
					{
						if (iter->second.mState == BlockInstance::LOADED && (std::abs(CurrentBlockID.mX - TempID.mX) > 1 || std::abs(CurrentBlockID.mY - TempID.mY) > 1))
						{
							iter->second.mState = BlockInstance::PREPARE_UNLOAD;
							mWaitUnloadBlocks.insert(TempID);
						}
						else if (iter->second.mState == BlockInstance::PREPARE_LOAD && (std::abs(CurrentBlockID.mX - TempID.mX) > 1 || std::abs(CurrentBlockID.mY - TempID.mY) > 1))
						{
							iter->second.mState = BlockInstance::PREPARE_UNLOAD;
							mWaitUnloadBlocks.insert(TempID);
						}
					}
				}
			}
		}
		mLastCameraBlockID = CurrentBlockID;
	}

	bool MiniMapManager::canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		return true;
	}

	WorkQueue::Response* MiniMapManager::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		if (req->getAborted())
		{
			return nullptr;
		}
		//UI 不支持异步创建，改成帧末分帧创建，这块异步可以去做其他任务

		return nullptr;
	}

	WorkQueue::Response* MiniMapManager::handleIOCPRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		return nullptr;
	}

	bool MiniMapManager::canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		return true;
	}

	void MiniMapManager::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		if (res->getRequest()->getAborted())
		{
			return ;
		}

		EventType CurrentEventType = any_cast<EventType>(res->getData());
		switch (CurrentEventType)
		{
		case LoadBlock:
			for (auto&& currentBlock : mWaitLoadBlocks)
			{
				auto iter = mWorldBlocks.find(currentBlock);
				if (iter != mWorldBlocks.end())
				{
					for (auto&& info : iter->second.mBuildingInfos)
					{
						auto pActor = info.second;
						if (!pActor.Expired())
						{
							pActor->setWorldPosition(info.first->mWorldPosition);
							pActor->setWorldScale(info.first->mScale);
						}
					}
					iter->second.mState = BlockInstance::LOADED;
				}
			}
			break;
		case UnloadBlock:
			for (auto&& currentBlock : mWaitUnloadBlocks)
			{
				auto iter = mWorldBlocks.find(currentBlock);
				if (iter != mWorldBlocks.end())
				{
					iter->second.mState = BlockInstance::NONE;
				}
			}
			
			break;
		default:
			break;
		}
		return;
	}

	bool MiniMapManager::frameEnded(const FrameEvent& evt)
	{
		uint64 StartTime = Root::instance()->getTimer()->getMilliseconds();
		for (auto&& waitloadIter = mWaitLoadBlocks.begin(); waitloadIter != mWaitLoadBlocks.end(); )
		{
			auto iter = mWorldBlocks.find(*waitloadIter);
			if (iter != mWorldBlocks.end())
			{
				for (auto&& info : iter->second.mBuildingInfos)
				{
					auto pActor = mWorldMgr->getActorManager()->createActor(Name("actors/building.act"), "", true);
					if (!pActor.Expired())
					{
						mCreateActors.insert(pActor);
						auto uiCom = pActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
						iter->second.mActors.insert(pActor);
						iter->second.mUIComponents.insert(uiCom);
						auto uiGobject = uiCom->getGObject()->as<echofgui::GComponent>()->getChild("worldbuilding");
						if (uiGobject)
						{
							uiGobject->setText(info.first->mName);
						}
						pActor->setWorldPosition(info.first->mWorldPosition);
						pActor->setWorldScale(info.first->mScale);
						uiCom->setRuntimeVisible(false);
						mWorldBuildingUIs.insert(uiCom);
					}
				}

				iter->second.mState = BlockInstance::LOADED;
				waitloadIter = mWaitLoadBlocks.erase(waitloadIter);
				uint64 CurTime = Root::instance()->getTimer()->getMilliseconds();

				if (CurTime - StartTime > 2)
				{
					return true;
				}

			}
			else
			{
				waitloadIter++;
			}
		}

		for (auto&& currentBlock  = mWaitUnloadBlocks.begin(); currentBlock != mWaitUnloadBlocks.end();)
		{
			auto iter = mWorldBlocks.find(*currentBlock);

			if (iter != mWorldBlocks.end())
			{
				for (auto&& pActor : iter->second.mActors)
				{
					if (!pActor.Expired())
					{
						mCreateActors.erase(pActor);
						auto uiCom = pActor->GetFirstComByType<Echo3DUIComponent>().getPointer();
						mWorldBuildingUIs.erase(uiCom);
						mWorldMgr->getActorManager()->deleteActor(pActor);
					}
				}
				iter->second.mActors.clear();
				iter->second.mUIComponents.clear();
				iter->second.mState = BlockInstance::NONE;
				currentBlock = mWaitUnloadBlocks.erase(currentBlock);
			}
			else
			{
				++currentBlock;
			}

			uint64 CurTime = Root::instance()->getTimer()->getMilliseconds();

			if (CurTime - StartTime > 2)
			{
				return true;
			}
		}

		return true;
	}

	void MiniMapManager::AddMiniMapEventListener(int inEventType, const echofgui::EventCallback& callback)
	{
		if(mShowMapUI.mObject && !mbWorldMap)
			mShowMapUI.mObject->addEventListener(inEventType, callback, echofgui::EventTag(mShowMapUI.mObject));
	}

	void MiniMapManager::RemoveMiniMapEventListener(int inEventType)
	{
		if (mShowMapUI.mObject && !mbWorldMap)
			mShowMapUI.mObject->removeEventListener(inEventType, echofgui::EventTag(mShowMapUI.mObject));
	}

	void MiniMapManager::OnKarstCreateEvent(KarstGroup* inGroup)
	{
		if (inGroup == nullptr)
			return;
		mbWorldMap = inGroup->getLevelName().compare("karst2") != 0;
		Uninitilize();
		Initilize();
	}

	void MiniMapManager::Tick()
	{
		if (!mbInit)
			return;

		if (!mbIsCreate)
			return;

		const Vector3& CameraWorldPos = mPlayerWorldPosition;
		if (!mTimeZeroRefPosInited)
		{
			//mTimeZeroRefPos = CameraWorldPos;
			mTimeZeroRefPos = Vector3(-365472.0f, 9.64f, 432224.0f);
			mTimeZeroRefPosInited = true;
		}


		{
			TODManager* todMgr = TODSystem::instance()->GetManager(WorldSystem::instance()->GetActiveWorld());
			if (todMgr && mTerrainWidth > 1e-3f)
			{


				Vector3 refPos = mTimeZeroRefPos;

				if (mTerrainWidth > 1e-6f)
				{

					const float zoneWidth = mTerrainWidth / 24.0f;
					int curZone = (int)std::floor((CameraWorldPos.x - mWorldOrigin.x) / zoneWidth);
					int refZone = (int)std::floor((refPos.x - mWorldOrigin.x) / zoneWidth);
					int rawDelta = curZone - refZone; // 可能超出[-24,24]

					rawDelta = (rawDelta % 24 + 24) % 24;
					if (rawDelta > 12) rawDelta -= 24;
					const int hourSec = 3600;
					int targetOffset = rawDelta * hourSec;
					static int s_lastOffset = 0;
					int diff = targetOffset - s_lastOffset;
					const int step = 5; //调整秒
					if (diff > step) s_lastOffset += step;
					else if (diff < -step) s_lastOffset -= step;
					else s_lastOffset = targetOffset;
					todMgr->setLocationTimeOffsetSeconds(s_lastOffset);
				}
				else
				{
					todMgr->setLocationTimeOffsetSeconds(0);
				}



				float playerNormX = (CameraWorldPos.x - mWorldOrigin.x) / mTerrainWidth;
				float refNormX = (refPos.x - mWorldOrigin.x) / mTerrainWidth;
				float deltaNorm = playerNormX - refNormX;
				// 归一化到[-0.5, 0.5)，避免远距离跳变
				deltaNorm = deltaNorm - floorf(deltaNorm + 0.5f);

				const int daySec = 24 * 3600;
				int offsetSeconds = (int)std::lround(deltaNorm * daySec);
				todMgr->setLocationTimeOffsetSeconds(offsetSeconds);
			}
		}
		
		//auto CameraRotation = mSceneMgr->getMainCamera()->getDerivedOrientation().Inverse();
		//Vector3 CameraRotationVec3 = CameraRotation.ToEuler();

		NotifyCameraVisible();

		//CityUI* CurrentCityUI = nullptr;
		//float CurrentMinDis = std::numeric_limits<float>::max();
		//for (auto&& curCity : mCityMap)
		//{
		//	if (!curCity.mCityUIObject)
		//		continue;

		//	float TempDis = curCity.mWorldPosition.squaredDistance(CameraWorldPos);
		//	if (CurrentMinDis > TempDis)
		//	{
		//		CurrentMinDis = TempDis;
		//		CurrentCityUI = &curCity;
		//	}
		//}


		bool isCheck = false;
		Vector2 TempPos;
		//if (!mbWorldMap)
		{
			Vector2 CameraPos2D(CameraWorldPos.x, CameraWorldPos.z);
			echofgui::GObject* pObject = mElevationUI.mObject->as<echofgui::GComponent>()->getChild("n0");
			char elevInfo[256] = { 0 };
			sprintf(elevInfo, "海拔: %.2f", KarstExportUtil::GetElevation());
			pObject->setText(elevInfo);
			mCurPositionUI.mObject->setText("坐标：" + CameraWorldPos.ToString());
			static uint32 GAreaID;
			if (KarstExportUtil::GetAreaIDAtPos(CameraPos2D, GAreaID)) {
				mCurCityNameUI.mObject->setText("地区：" + KarstExportUtil::GetNameFromAreaID(GAreaID));
			}
			else {
				mCurCityNameUI.mObject->setText("地区：未知");
			}
		}
		//if (CurrentCityUI)
		//{
		//	
		//	//AreaBlockInfo* CurrentAreaBlockInfo = mPageManager->CheckCamera(CameraWorldPos);
		//	
		//	uint32 CheckAreaID = 0;
		//	static echofgui::GObject* CurrCheckUI = nullptr;
		//	for (auto&& areaTex : mAreaTexs)
		//	{
		//		if (!isCheck && CheckInRange(areaTex.mWorldOrigin, areaTex.mWorldMaxRange, CameraPos2D))
		//		{
		//			Vector2 areaSize = areaTex.mWorldMaxRange - areaTex.mWorldOrigin;
		//			Vector2 areaIndex = (CameraPos2D - areaTex.mWorldOrigin) / areaSize;
		//			areaIndex *= 1024;

		//			//Vector2  CurIndex = (CameraPos2D - areaTex.mWorldOrigin) / 128;
		//			int CurIndexx = std::min(1023, (int)std::ceil(areaIndex.x));
		//			int CurIndexy = std::min(1023, (int)std::ceil(areaIndex.y));
		//			
		//			CheckAreaID = (uint32)areaTex.mAreaTexData[CurIndexx][CurIndexy];
		//			isCheck = true;
		//			areaTex.setVisible(true);
		//			TempPos = areaIndex * 2.0f;
		//			//CurrCheckUI = areaTex.mAreaMapUI.mObject;
		//			break;
		//		}
		//		//if (CurrCheckUI && CurrCheckUI != areaTex.mAreaMapUI.mObject)
		//		//{
		//		//	areaTex.setVisible(false);
		//		//}
		//	}

		//	//mMapUI.setVisible(!isCheck);

		//	if (mCurAreaUI.mObject)
		//	{
		//		if (isCheck)
		//		{
		//			//mCurAreaUI.mObject->setText("城市：北京市   " + mAreaStrs[mAdminRegionAreaStrs[CheckAreaID]] + "  " + mAdminRegions[CheckAreaID]);
		//			mCurCityNameUI.mObject->setText("地区：北京市   " + mAreaStrs[mAdminRegionAreaStrs[CheckAreaID]] + "  " + mAdminRegions[CheckAreaID]);
		//		}
		//		else
		//		{
		//			//CurrCheckUI = nullptr;
		//			//mCurAreaUI.mObject->setText("行政划区： 未划分 ");
		//			mCurCityNameUI.mObject->setText("地区：" + CurrentCityUI->mCityName);
		//		}
		//	}
		//}

		if (!mbShowMap)
			return;

		//计算地图玩家位置
		Vector2 N1Size(mMapRangeUI.mInitSize.x, mMapRangeUI.mInitSize.y);
		Vector2 PlayerPosInMap = GetPosInMapByWorldPos(CameraWorldPos);
		//if (isCheck)
		//	PlayerPosInMap = TempPos;

		Vector3 PlayerUIRot = CalculatePlayerRotate();
		mPlayerUI.mObject->setRotation(PlayerUIRot.z);

		uint32 UWidth, UHeight;
		Root::instance()->getRenderSystem()->getWindowSize(UWidth, UHeight);
		float RealWindowWidthWithOneSideOffset = UWidth * 0.95f;
		float RealWindowHeightWithOneSideOffset = UHeight * 0.95f;
		float RealStartPosX = UWidth - RealWindowWidthWithOneSideOffset;
		float RealStartPosY = UHeight - RealWindowHeightWithOneSideOffset;
		if (!mbShowFullMap)
		{
			RealStartPosX = 0.0f;
			RealStartPosY = 0.0f;
		}


		if (mbShowFullMap)
		{

			float RealWindowWidth = UWidth * 0.90f;
			float RealWindowHeight = UHeight * 0.90f;

			float mapAspect = mMapUI.mInitSize.x / mMapUI.mInitSize.y;

			Vector2 PlayerHalfSize(mPlayerUI.mCurrentSize.x / 2.0f, mPlayerUI.mCurrentSize.y / 2.0f);

			float uiWidth = RealWindowWidth * 1.0f;
			float uiHeight = std::min(uiWidth / mapAspect, RealWindowHeight * 1.0f);
			
			Vector2 scaleFactor(uiWidth/mMapUI.mInitSize.x, uiHeight/mMapUI.mInitSize.y);
			PlayerPosInMap = (PlayerPosInMap - PlayerHalfSize) * scaleFactor;

			mPlayerUI.mObject->setPosition(PlayerPosInMap.x, PlayerPosInMap.y);
		}
		else
		{
			PlayerPosInMap -= N1Size / 2.0f;
			//PlayerPosInMap += Vector2(mPlayerUI.mInitSize.x / 2.0f, mPlayerUI.mInitSize.y / 2.0f);
			//误差偏移
			//PlayerPosInMap.y = PlayerPosInMap.y ;
			//移动地图
			Vector2 MapMoveOffset = -PlayerPosInMap;
			mMapUI.mObject->setPosition(MapMoveOffset.x, MapMoveOffset.y);
			
		}

		if (mbShowCityName)
		{
			for (auto&& curCity : mCityMap)
			{
				if (!curCity.mCityNameObject)
					continue;

				curCity.mCityNameObject->setPosition(RealStartPosX + curCity.mMapPosition.x - PlayerPosInMap.x, RealStartPosY + curCity.mMapPosition.y - PlayerPosInMap.y + 30.0f);
			}
		}
		
		for (auto& line : mTrainLines)
		{
			line->update(Root::instance()->getTimeSinceLastFrame(), CameraWorldPos);
			line->RunJob();
		}
	}

	void MiniMapManager::SetShowMap(bool InVal)
	{
		if (InVal == mbShowMap)
			return;

		mbShowMap = InVal;
		mMapObject.mObject->setVisible(InVal);
		//mChangeFullMapUI.mObject->setVisible(InVal);
	}

	void MiniMapManager::SetShowCityName(bool InVal)
	{
		if (InVal == mbShowCityName)
			return;

		mbShowCityName = InVal;
		for (auto&& curCity : mCityMap)
		{
			if (!curCity.mCityNameObject)
				continue;

			curCity.mCityNameObject->setVisible(InVal);
		}
	}

	void MiniMapManager::SetShowBuildings(bool InVal)
	{
		if (InVal == mbShowBuildings)
			return;

		mbShowBuildings = InVal;
		for (auto&& build : mBuildings)
		{
			if (!build.mBuildingUI)
				continue;

			build.mBuildingUI->setVisible(InVal);
		}
	}

	void MiniMapManager::SetShowFullMap(bool InVal)
	{
		mbShowFullMap = InVal;

		uint32 UWidth, UHeight;
		Root::instance()->getRenderSystem()->getWindowSize(UWidth, UHeight);
		float RealWindowWidthWithOneSideOffset = UWidth * 0.95f;
		float RealWindowHeightWithOneSideOffset = UHeight * 0.95f;
		float RealStartPosX = UWidth - RealWindowWidthWithOneSideOffset;
		float RealStartPosY = UHeight - RealWindowHeightWithOneSideOffset;
		float RealWindowWidth = UWidth * 0.90f;
		float RealWindowHeight = UHeight * 0.90f;
		if (!mbShowFullMap)
		{
			RealStartPosX = 0.0f;
			RealStartPosY = 0.0f;
		}

		if (mbShowFullMap)
		{
			float mapAspect = mMapUI.mInitSize.x / mMapUI.mInitSize.y;

			float uiWidth = RealWindowWidth * 1.0f;
			float uiHeight = std::min(uiWidth / mapAspect, RealWindowHeight * 1.0f);
			mMapObject.mObject->setWidth(uiWidth);
			mMapObject.mObject->setHeight(uiHeight);
			mMapObject.mObject->setPosition(RealStartPosX, RealStartPosY);
			mMapObject.mObject->setScale(1.0f, 1.0f);
			mMapRangeUI.mObject->setWidth(uiWidth);
			mMapRangeUI.mObject->setHeight(uiHeight);
			mMapRangeUI.mObject->setPosition(0, 0);
			mMapUI.mObject->setPosition(0, 0);
			mMapUI.mObject->setScale(uiWidth / mMapUI.mInitSize.x, uiHeight / mMapUI.mInitSize.y);
			mPlayerUI.mObject->setScale(1.0f, 1.0f);
			mMapWidth = mMapUI.mInitSize.x;
			mMapHeight = mMapUI.mInitSize.y;
			/*mMapHeight *= ADAPTATION_SCALE_X;
			mMapWidth *= ADAPTATION_SCALE_Y;*/

			// 全图状态
			if (mChangeFullMapUI.mObject)
			{
				mChangeFullMapUI.mObject->setVisible(false);
			}
			if (mCloseFullMapUI.mObject)
			{
				mCloseFullMapUI.mObject->setVisible(true);
				mCloseFullMapUI.mObject->setPosition(RealStartPosX + 8.0f, RealStartPosY + 8.0f);
				mCloseFullMapUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			}

		}
		else
		{
			mMapObject.mObject->setWidth(mMapObject.mInitSize.x);
			mMapObject.mObject->setHeight(mMapObject.mInitSize.y);
			mMapObject.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mInitSize.x * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset);
			mMapObject.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			mMapRangeUI.mObject->setWidth(mMapRangeUI.mInitSize.x);
			mMapRangeUI.mObject->setHeight(mMapRangeUI.mInitSize.y);
			mMapRangeUI.mObject->setPosition(mMapRangeUI.mInitPos.x, mMapRangeUI.mInitPos.y);
			mMapUI.mObject->setScale(1.0f, 1.0f);
			mPlayerUI.mObject->setPosition(mPlayerUI.mInitPos.x, mPlayerUI.mInitPos.y);
			mPlayerUI.mObject->setScale(1.0f, 1.0f);
			mMapWidth = mMapUI.mInitSize.x;
			mMapHeight = mMapUI.mInitSize.y;
			//mMapHeight *= ADAPTATION_SCALE_X;
			//mMapWidth *= ADAPTATION_SCALE_Y;

			// 小地图状态
			mChangeFullMapUI.mObject->setPosition(RealWindowWidthWithOneSideOffset - mMapRangeUI.mObject->getWidth() * ADAPTATION_SCALE_X, UHeight - RealWindowHeightWithOneSideOffset + mMapRangeUI.mObject->getHeight() * ADAPTATION_SCALE_Y);
			mChangeFullMapUI.mObject->setScale(ADAPTATION_SCALE_X, ADAPTATION_SCALE_Y);
			if (mChangeFullMapUI.mObject) mChangeFullMapUI.mObject->setVisible(true);
			if (mCloseFullMapUI.mObject) mCloseFullMapUI.mObject->setVisible(false);
		}

		
	}

	void MiniMapManager::SetShowSceneUI(bool InVal)
	{
		if (InVal == mbShowSceneUI)
			return;
		mbShowSceneUI = InVal;

		for (auto&& sceneUI : mWorldBuildingUIs)
		{
			sceneUI->setVisible(InVal);
		}
	}

	Vector2 MiniMapManager::GetPosInMapByWorldPos(const Vector3& InPos)
	{
		Vector3 LocalPos = InPos - mWorldOrigin;
		Vector2 PlayerProportionScene;
		PlayerProportionScene.x = LocalPos.x / mTerrainWidth;
		PlayerProportionScene.y = LocalPos.z / mTerrainHeight;
		Vector2 MapPos = PlayerProportionScene * Vector2(mMapWidth, mMapHeight);
		return MapPos;
	}

	Vector3 MiniMapManager::CalculatePlayerRotate()
	{
		Vector3 PlayerLookAt = mSceneMgr->getMainCamera()->getDerivedDirection();
		PlayerLookAt.Normalize();
		Vector3 PlayerLookAtXZ = PlayerLookAt - PlayerLookAt.dotProduct(Vector3::UNIT_Y) * Vector3::UNIT_Y;
		PlayerLookAtXZ.Normalize();
		static Vector3 ForwardVec = Vector3(0.0f, 0.0f, 1.0f);
		Radian RotRadian = Math::ACos(PlayerLookAtXZ.dotProduct(ForwardVec));
		if(PlayerLookAtXZ.x > 1e-6)
			return Vector3(0.0f, 0.0f,  -RotRadian.valueDegrees());
		return Vector3(0.0f, 0.0f, RotRadian.valueDegrees());
	}

	void MiniMapManager::OnClickShowMap(echofgui::EventContext* InContext)
	{
		SetShowMap(!GetShowMap());
	}

	void MiniMapManager::OnClickChangeFullMap(echofgui::EventContext* InContext)
	{
		SetShowFullMap(!GetShowFullMap());
	}

	bool MiniMapManager::InitUIObjectProperty(UIObjectProperty& InOutUIObjectProperty, echofgui::GObject* InParentObject, const std::string& InChildName)
	{
		InOutUIObjectProperty.mObject = InParentObject->as<echofgui::GComponent>()->getChild(InChildName);

		if (!InOutUIObjectProperty.mObject)
			return false;

		{
			InOutUIObjectProperty.mInitPos = FguiVecToEchoVec(InOutUIObjectProperty.mObject->getPosition());
			InOutUIObjectProperty.mCurrentPos = InOutUIObjectProperty.mInitPos;
			InOutUIObjectProperty.mInitSize = Vector3(InOutUIObjectProperty.mObject->getActualWidth(), InOutUIObjectProperty.mObject->getActualHeight(), 0.0f);
			InOutUIObjectProperty.mCurrentSize = InOutUIObjectProperty.mInitSize;
		}
		return true;
	}

	UIObjectProperty MiniMapManager::CreateUIFromResource(const std::string& InPackageName, const std::string& InComponentName)
	{
		UIObjectProperty Result;
		Result.mObject = echofgui::UIPackage::createObject(InPackageName, InComponentName);
		Result.mInitPos = FguiVecToEchoVec(Result.mObject->getPosition());
		Result.mCurrentPos = Result.mInitPos;
		Result.mInitSize = Vector3(Result.mObject->getActualWidth(), Result.mObject->getActualHeight(), 0.0f);
		Result.mCurrentSize = Result.mInitSize;
		echofgui::GRoot* pDefaultRoot = echofgui::GRoot::defaultRoot();
		echofgui::GRoot* pRoot = echofgui::GRoot::create();
		pDefaultRoot->addChild(pRoot);
		pRoot->setUIStyle(UI2D_STYLE_SCREEN);
		pRoot->setVisible(true);
		pRoot->addChild(Result.mObject);
		return Result;
	}


	CityMapPageManager::CityMapPageManager()
	{
		mAreaArray.reserve(512);
	}

	CityMapPageManager::~CityMapPageManager()
	{
		for (auto pArea : mAreaArray)
		{
			SAFE_DELETE(pArea);
		}
	}

	AreaBlockInfo* CityMapPageManager::CheckCamera(const Vector3& InPos)
	{
		for (auto pArea : mAreaArray)
		{
			if (CheckInRange(pArea->mWorldRange, InPos))
			{
				return pArea;
			}
		}
		return nullptr;
	}

	void CityMapPageManager::Create()
	{
	}


	AirPlaneManager::AirPlaneManager()
	{
		mController = new PlaneController();
		Root::instance()->addFrameListener(this);
	}

	AirPlaneManager::~AirPlaneManager()
	{
		delete mController;
		Root::instance()->removeFrameListener(this);
	}
	
	void AirPlaneManager::tick(float dt)
	{
		static float durationTime = 0.0f;
		static int idx = 0;
		int len = static_cast<int>(mLines.size());
		durationTime += dt;
		if (durationTime >= 200.0f)
		{
			if (mController->isIdle())
			{
				mController->startOne(mLines[idx]);
				idx = (idx + 1) % len;
				durationTime -= 200.0f;
			}
		}
	}

	bool AirPlaneManager::frameStarted(const FrameEvent& evt)
	{
		mController->update();
		tick(evt.timeSinceLastFrame * 1000.0f);
		mController->RunJob();
		return true;
	}

	void AirPlaneManager::addAirLine(const Vector3& start, const Vector3& end)
	{
		mLines.push_back({ start, end });
	}

	AirPlaneManager::PlaneController::PlaneController() : Job(Name("PlaneController"), Normal)
	{
	}


	AirPlaneManager::PlaneController::~PlaneController()
	{
		CancelOrWait();
		for (auto plane : mWorking)
		{
			delete plane;
		}
		for (auto plane : mFinished)
		{
			delete plane;
		}
	}
	/*void EchoLogToConsole(const String& log)
	{
#ifdef _WIN32
		OutputDebugString(log.c_str());
#else
		LogManager::instance()->logMessage(log.c_str());
#endif
	}*/

	void AirPlaneManager::PlaneController::startOne(const AirLine& line)
	{
		AirPlane* pPlane;
		if (mFinished.size() > 0)
		{
			pPlane = mFinished.back();
			mFinished.pop_back();
		}
		else
		{
			pPlane = new AirPlane();
		}
		pPlane->reset(line.start, line.end);
		pPlane->update();
		mWorking.push_back(pPlane);
	}

	void AirPlaneManager::PlaneController::Execute()
	{
		for (auto plane : mWorking)
		{
			plane->tick(0.1f);
		}

		int idx = 0;
		int bound = static_cast<int>(mWorking.size());
		while (idx < bound)
		{
			if (mWorking[idx]->isFinished())
			{
				std::swap(mWorking[idx], mWorking[bound - 1]);
				--bound;
			}
			else
			{
				++idx;
			}
		}
		while (bound < mWorking.size())
		{
			mFinished.push_back(mWorking.back());
			mWorking.pop_back();
		}
	}

	void AirPlaneManager::PlaneController::update()
	{
		for (auto plane : mWorking)
		{
			plane->update();
		}
		LogManager::instance()->logMessage("PlaneCount: " + std::to_string(mWorking.size()) + ", " + std::to_string(mFinished.size()) + "\n");
	}

	bool AirPlaneManager::PlaneController::isIdle()
	{
		return !mJobState || IsFinish();
	}

	AirPlane::~AirPlane()
	{
		if (!mActor.Expired())
		{
			mActor = NULL;
		}
	}

	void AirPlane::reset(const Vector3& start, const Vector3& end)
	{
		
		Vector3 dir = end - start;
		dir.normalise();
		mStartPos = start + dir * 50.0f;

		/*float height;
		if (KarstSystem::instance()->getHeightAtWorldPos(mStartPos, height))
		{
			mStartPos.y = height;
		}*/

		mEndPos = end;
		mCurrPos = mStartPos;
		mStage = Ascend;
		mOffset = mEndPos - mStartPos;
		mTotalDistance = mOffset.length();
		mTraveledDistance = 0.0f;
		mCurrentSpeed = takeoffSpeed;

		mStateData.resize(3);
		mBackStateData.resize(3);
		_tick(0.1f);
		mBackStateData[0].first = mStartPos;
		mBackStateData[0].second = calcRotation(mDirection);//mDirection;
		mBackStateData[1].first = mCurrPos;
		_tick(0.1f);
		mBackStateData[1].second = calcRotation(mDirection);//mDirection;
		mBackStateData[2].first = mCurrPos;
		mNeedFlush = true;
		mStartTime = Root::instance()->getTimeElapsed();

		if (mActor.Expired())
		{
			mActor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Name("actors/airplane.act"), "", false, false);
		}
		mActor->setVisiable(true);
	}

	void AirPlane::tick(float dt)
	{
		float currTime = Root::instance()->getTimeElapsed();
		float duration = currTime - mStartTime;
		if (duration < dt)
		{
			//EchoLogToConsole("Skip\n");
			return;
		}
		//EchoLogToConsole("Tick\n");
		mStartTime += floorf(duration * 10.0f) / 10.0f;

		std::swap(mBackStateData[0], mBackStateData[1]);
		std::swap(mBackStateData[1], mBackStateData[2]);
		_tick(dt);
		mBackStateData[1].second = calcRotation(mDirection);//mDirection;
		mBackStateData[2].first = mCurrPos;
		mNeedFlush = true;
	}

	void AirPlane::flushData()
	{
		memcpy(mStateData.data(), mBackStateData.data(), sizeof(mStateData[0]) * mStateData.size());
	}

	void AirPlane::_tick(float dt)
	{
		if (mStage == FlightStage::Finished)
			return;

		// 阶段切换和速度控制
		float ascendEnd = mTotalDistance * ascendRatio;
		float descendStart = mTotalDistance * (1.0f - descendRatio);

		if (mTraveledDistance < ascendEnd) {
			mStage = FlightStage::Ascend;
			mCurrentSpeed = std::min(cruiseSpeed, mCurrentSpeed + acceleration * dt);
		}
		else if (mTraveledDistance < descendStart) {
			mStage = FlightStage::Cruise;
			mCurrentSpeed = cruiseSpeed;
		}
		else if (mTraveledDistance < mTotalDistance) {
			mStage = FlightStage::Descend;
			mCurrentSpeed = std::max(takeoffSpeed, mCurrentSpeed - deceleration * dt);
		}
		else {
			mStage = FlightStage::Finished;
			mCurrPos = mEndPos;
			mCurrentSpeed = 0.0f;
			return;
		}

		// 前进
		mTraveledDistance += mCurrentSpeed * dt;
		mTraveledDistance = std::min(mTraveledDistance, mTotalDistance);
		Vector3 pos = computeFlightPosition(mTraveledDistance);
		//防止抖动，以更前方位置计算方向
		Vector3 frontPos = computeFlightPosition(mTraveledDistance + 50.0f);
		mDirection = frontPos - mCurrPos;
		mDirection.normalise();
		mCurrPos = pos;
	}

	Quaternion AirPlane::calcRotation(const Vector3& dir)
	{
		Quaternion qua;
		Vector3 zDir = dir.crossProduct(Vector3::UP);
		Vector3 upDir = zDir.crossProduct(dir);
		qua.FromAxes(dir, upDir, zDir);
		return qua;
	}

	Vector3 AirPlane::computeFlightPosition(float distanceAlongPath)
	{
		double t = distanceAlongPath / mTotalDistance;
		Vector3 flatPos = mStartPos + mOffset * t;

		// 设置Y（高度）
		float height = 0.0f;
		if (mStage == FlightStage::Ascend) {
			float progress = distanceAlongPath / (mTotalDistance * ascendRatio);
			height = cruiseAltitude * progress;
		}
		else if (mStage == FlightStage::Cruise) {
			height = cruiseAltitude;
		}
		else if (mStage == FlightStage::Descend) {
			float progress = (distanceAlongPath - mTotalDistance * (1.0f - descendRatio)) / (mTotalDistance * descendRatio);
			height = cruiseAltitude * (1.0f - progress);
		}

		return Vector3(flatPos.x, flatPos.y + height, flatPos.z);
	}

	void AirPlane::update()
	{
		if (!mActor.Expired())
		{
			if (mStage == FlightStage::Finished)
			{
				mActor->setVisiable(false);
				return;
			}

			if (mNeedFlush)
			{
				flushData();
				mNeedFlush = false;
			}
			
			float currTime = Root::instance()->getTimeElapsed();
			int idx = 0;
			float duration = currTime - mStartTime;
			float process = duration * 10.0f;
			if (duration >= 0.1f)
			{
				idx = 1;
				process -= floorf(process);
			}

			
			Vector3 offset = mStateData[idx + 1].first - mStateData[idx].first;
			Vector3 pos = mStateData[idx].first + offset * process;

			if (Root::instance()->getMainSceneManager())
			{
				Camera* camera = Root::instance()->getMainSceneManager()->getMainCamera();
				if (camera)
				{
					Vector3 cameraPos = camera->getPosition();
					bool visible = std::powf(cameraPos.x - pos.x, 2.0f) + std::powf(cameraPos.z - pos.z, 2.0f) < 1e6;
					mActor->setVisiable(visible);
					if (!visible) return;
				}
			}
		
			mActor->setPosition(pos + Vector3(0.0f, 20.0f, 0.0f));
			mActor->setRotation(mStateData[idx].second);
		}
	}

	TrainLineData::TrainLineData(KarstGroup* karstGroup, const std::vector<Vector2>& pointData) : Job(Name("TrainLineData"), Normal),
		mKarstGroup(karstGroup)
	{
		int size = (int)pointData.size();
		if (size < 2) {
			LogManager::instance()->logMessage("karstline point num is less than 2");
			throw;
		}
		mPointData.resize(size);
		mLineData.resize(size - 1);
		mTotalLength = 0;

		for (int i = 0; i < size; i++)
		{
			mPointData[i] = { pointData[i].x, 0.0f, pointData[i].y };
			if (i != 0) {
				Vector2 vec = pointData[i] - pointData[i - 1];

				mLineData[i - 1].start = i - 1;
				mLineData[i - 1].end = i;
				mLineData[i - 1].length = vec.length();
				mLineData[i - 1].startDistance = mTotalLength;

				mTotalLength += mLineData[i - 1].length;
			}
		}

		mSections = int(ceilf(mTotalLength / 3500.0f));

		for (int i = 0; i < mSections; i++) {
			mStopLength.push_back(i * 3500.0f);
		}
		mStopLength.push_back(mTotalLength);
	}

	inline Vector3 LerpVector3(const Vector3& v0, const Vector3& v1, float t)
	{
		return { Lerp(v0.x, v1.x, t), Lerp(v0.y, v1.y, t),Lerp(v0.z, v1.z, t) };
	}

	void TrainLineData::getPositionFromDistance(float distantce, Vector3& postion, Vector3& direction)
	{
		float lineDistance = std::min(distantce, mTotalLength);

		for (const auto& line : mLineData)
		{
			if (lineDistance >= line.startDistance && lineDistance < (line.startDistance + line.length)) {
				float t = (lineDistance - line.startDistance) / line.length;
				postion = LerpVector3(mPointData[line.start], mPointData[line.end], t);
				direction = mPointData[line.end] - mPointData[line.start];
				direction.Normalize();
			}
		}
	}

	void TrainLineData::updateHeightData()
	{
		auto originPos = mKarstGroup->getOriginPosition();
		const float& invKarstSize = mKarstGroup->getOptions()->mInvKarstSize;
		for (auto& point : mPointData) {
			if ((mCurrentCamPos - Vector2(point.x, point.z)).length() < mUpdateDist) {
				EchoIndex2 index = EchoIndex2((int)floor((point.x - originPos.x) * invKarstSize),
					(int)floor((point.z - originPos.z) * invKarstSize));
				float height;
				if (mKarstGroup->computeHeightAtPos(Vector2(point.x, point.z), height, index, nullptr, nullptr)) {
					point.y = height + mInterspace;
					continue;
				}
			}
			point.y = mInterspace;
		}
	}

	void TrainLineData::update(float timeDelay, const Vector3& camPos)
	{
		mUpdateLineDataDelay += timeDelay;
		mCurrentCamPos = { camPos.x, camPos.z };
		mTimeDelay = timeDelay;

		for (auto& actor : mMoveActors)
		{
			actor->tick();
		}
	}

	void TrainLineData::Execute()
	{
		if (mUpdateLineDataDelay > 10.0f)
		{
			updateHeightData();
			mUpdateLineDataDelay = 0.0f;
		}

		for (auto& actor : mMoveActors)
		{
			actor->update(mTimeDelay);
		}
	}
}

