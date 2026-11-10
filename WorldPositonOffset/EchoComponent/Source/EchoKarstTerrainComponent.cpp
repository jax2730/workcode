#include "EchoComponentStableHeaders.h"
#include "EchoKarstTerrainComponent.h"
#include "EchoKarstGroup.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoWorldManager.h"
#include "VoxelTerrain/EchoVoxelWorld.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(KarstRoadComponent)
		EDITOR_REFLECT_BEGIN(KarstRoadComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RenderDist, "RenderDist", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(KarstRoadComponentInfo);

	KarstRoadComponent::KarstRoadComponent(ActorPtr pActor, ActorCOMMoveableType type)
	: ActorComponent(pActor, type)
	{
	}

	KarstRoadComponent::~KarstRoadComponent()
	{
	}

	void KarstRoadComponent::init(ActorComponentInfo* info)
	{
		Root::instance()->addFrameListener(this);
	}

	void KarstRoadComponent::unInit()
	{
		Root::instance()->removeFrameListener(this);
	}

	void KarstRoadComponent::replay(ActorComponentInfo* info)
	{
		if (!info)
			return;
		KarstRoadComponentInfo* pInfo = static_cast<KarstRoadComponentInfo*>(info);
		setRenderDist(pInfo->GetRenderDist());
	}

	void KarstRoadComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if (actCom == nullptr)
			return;
		KarstRoadComponent* pComp = static_cast<KarstRoadComponent*>(actCom.getPointer());
		pComp->setRenderDist(mRenderDist);
	}

	void KarstRoadComponent::connectCom()
	{
		mRoadActor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Name("KarstRoadActor"), "", false, true);
		mRoadActor->setPosition(DVector3(0.0));

		mKarstGroup = Echo::KarstSystem::instance()->getVoxelKarstGroup();
		if (mKarstGroup != nullptr) {
			initRoadDataHelper();
		}
		else {
			Echo::KarstSystem::instance()->addCreatedListener(std::bind(&KarstRoadComponent::OnKarstCreateEvent, this, std::placeholders::_1));
		}
	}

	void KarstRoadComponent::initRoadDataHelper()
	{
		if(!mKarstGroup)
			return;
		String levelName = mKarstGroup->getLevelName();
		DataStreamPtr roadLineStream(Root::instance()->GetFileDataStream(("levels/" + levelName + "/roads/roadline.idx").c_str(), true));
		if (!roadLineStream.isNull())
		{
			uint32 roadSectionSize = 0;
			roadLineStream->readData(&roadSectionSize);
			for (uint32 i = 0; i < roadSectionSize; ++i) {
				EchoIndex2 index;
				roadLineStream->readData(&index.x, 1);
				roadLineStream->readData(&index.z, 1);
				roadExist.insert(index.ToUInt64());
			}
			mRoadDataHelper = new KarstRoadDataHelper(this, levelName);
		}

		DataStreamPtr railwayStream(Root::instance()->GetFileDataStream(("levels/" + levelName + "/railway/railway.idx").c_str(), true));
		if (!railwayStream.isNull())
		{
			uint32 roadSectionSize = 0;
			railwayStream->readData(&roadSectionSize);
			for (uint32 i = 0; i < roadSectionSize; ++i) {
				EchoIndex2 index;
				railwayStream->readData(&index.x, 1);
				railwayStream->readData(&index.z, 1);
				railwayExist.insert(index.ToUInt64());
			}
			mRailwayDataHelper = new KarstRailwayDataHelper(this, levelName);
		}
	}

	void KarstRoadComponent::OnKarstCreateEvent(KarstGroup* karstGroup)
	{
		if (karstGroup != nullptr) {
			mKarstGroup = karstGroup;
			initRoadDataHelper();
		}
	}

	void KarstRoadComponent::disconnectCom()
	{

	}

	AxisAlignedBox KarstRoadComponent::getLoadBoundingBox()
	{
		if (mKarstGroup) {
			Vector3 vMin = Vector3(0, mKarstGroup->getMinHeight(), 0) + mKarstGroup->getOriginPosition();
			auto options = mKarstGroup->getOptions();
			Vector3 vMax = Vector3(options->mKarstSize * mKarstGroup->getRowNum(), mKarstGroup->getMaxHeight(), options->mKarstSize * mKarstGroup->getColNum()) + mKarstGroup->getOriginPosition();
			AxisAlignedBox aaBox(vMin, vMax);
			return aaBox;
		}
		else {
			return AxisAlignedBox();
		}
	}

	void KarstRoadComponent::getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data)
	{
		if (mKarstGroup) {
			if (mKarstGroup->computeHeightAtPos(Vector2(worldPos.x, worldPos.z), data.terrainHeight, true))
			{
				data.terrainNormal = Echo::Vector3::UP;
				data.terrainPos = Vector3(worldPos.x, data.terrainHeight, worldPos.z);
			}
			else
			{
				mKarstGroup->getHeightFromPosition(Vector2(worldPos.x, worldPos.z), data.terrainHeight);
				data.terrainNormal = Echo::Vector3::UP;
				data.terrainPos = Vector3(worldPos.x, data.terrainHeight, worldPos.z);
			}
		}
	}

	void KarstRoadComponent::createRoads(EchoIndex2 index, const std::vector<KarstRoadSection>& roads)
	{
		auto& paveComs = roadIndexes[index.ToUInt64()];
		for (const auto& road : roads)
		{
			if (road.points.size() < 2) continue;
			auto& paveCom = paveComs.emplace_back();
			paveCom.mPavement = mRoadActor->AddComponent<PavementComponent>("KarstRoad", mRoadActor->getRootNodeCom());

			ActorComFactory* bezierFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(PiecewiseBezierComponent));
			PiecewiseBezierComponentInfo* bezier_info = static_cast<PiecewiseBezierComponentInfo*>(bezierFactory->createComponentInfo());

			for (int i = 0; i < road.points.size(); i++) {
				PiecewiseBezierComponentInfo::BezierInfoPoint p;
				p.point = road.points[i];
				p.source_control = road.directions[i];
				p.destination_control = -road.directions[i];
				p.normal = Vector3::UNIT_Y;

				bezier_info->mPoints.push_back(p);
			}
			paveCom.mBezier = mRoadActor->addComponent(COMPONENTTYPE(PiecewiseBezierComponent), "RoadBezier", mRoadActor->getRootNodeCom(), bezier_info/*, 0, false*/);
			paveCom.mPavement->setBezierCom(paveCom.mBezier);

			PavementMeshBaseData data;
			data.groupName = "KarstRoadGroup";
			data.meshName = "model/block/jt/jt_gonglu_1_1.mesh";
			data.quat = Echo::Quaternion(Degree(90.0f), Vector3(0.0, 1.0, 0.0));
			data.scale = 0.5f;
			paveCom.mPavement->setFitGroundModel(mFitTerrain);
			if (mFitTerrain) {
				paveCom.mPavement->setPavementFitTerrain(this);
			}
			paveCom.mPavement->createPavement(data, PavementComponent::UpdateAll);
			paveCom.mPavement->SetRenderDistance(mRenderDist, Name("KarstRoadGroup"));
		}

		if (auto it = roadRequests.find(index.ToUInt64()); it != roadRequests.end()) {
			roadRequests.erase(it);
		}
	}

	void KarstRoadComponent::createRailways(EchoIndex2 index, const std::vector<KarstRoadSection>& roads)
	{
		auto& paveComs = railwayIndexes[index.ToUInt64()];
		for (const auto& road : roads)
		{
			auto& paveCom = paveComs.emplace_back();
			paveCom.mPavement = mRoadActor->AddComponent<PavementComponent>("KarstRail", mRoadActor->getRootNodeCom());
			ActorComFactory* bezierFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(PiecewiseBezierComponent));
			PiecewiseBezierComponentInfo* bezier_info = static_cast<PiecewiseBezierComponentInfo*>(bezierFactory->createComponentInfo());


				for (int i = 0; i < road.points.size(); i++) {

					PiecewiseBezierPoint p;
					//if (mFitTerrain) {
						p.point = Vector3(road.points[i].x, 0.0f, road.points[i].z);
						p.source_control = Vector3(road.directions[i].x, 0.f, road.directions[i].z);
					//}
					//else {
					//	p.point = Vector3(road.points[i].x, road.points[i].y, road.points[i].z);
					//	p.source_control = Vector3(road.directions[i].x, road.directions[i].y, road.directions[i].z);
					//}
					p.destination_control = -p.source_control;
					p.normal = Vector3::UNIT_Y;
					bezier_info->mPoints.push_back(p);
				}

			paveCom.mBezier = mRoadActor->addComponent(COMPONENTTYPE(PiecewiseBezierComponent), "RailBezier", mRoadActor->getRootNodeCom(), bezier_info/*, 0, false*/);
			paveCom.mPavement->setBezierCom(paveCom.mBezier);

			PavementMeshBaseData data;
			data.groupName = "KarstRailGroup";
			data.meshName = "model/block/bs/dt_guidao_001_lod1.mesh";
			data.quat = Echo::Quaternion(Degree(90.0), Vector3(0.0, 1.0, 0.0));
			data.scale = 5;
			paveCom.mPavement->setFitGroundModel(true);
			//if (mFitTerrain) {
			mFitTerrain = true;
				paveCom.mPavement->setPavementFitTerrain(this);
			mFitTerrain = false;
			//}
			paveCom.mPavement->createPavement(data, PavementComponent::UpdateAll);
			paveCom.mPavement->SetRenderDistance(mRenderDist, Name("KarstRailGroup"));
		}

		if (auto it = roadRequests.find(index.ToUInt64()); it != roadRequests.end()) {
			roadRequests.erase(it);
		}
		//std::vector<PavementCom> paveComs;
		//for (auto& road : roads)
		//{
		//	PavementCom paveCom;
		//	paveCom.mPavement = mRoadActor->AddComponent<PavementComponent>("KarstRailway", mRoadActor->getRootNodeCom());
		//	paveCom.mBezier = mRoadActor->AddComponent<PiecewiseBezierComponent>("RailwayBezier", mRoadActor->getRootNodeCom(), nullptr, 0, false);

		//	for (int i = 0; i < road.points.size(); i++) {

		//		PiecewiseBezierPoint p;
		//		if (mFitTerrain) {
		//			p.point = Vector3(road.points[i].x, 0.0f, road.points[i].z);
		//			p.source_control = Vector3(road.directions[i].x, 0.f, road.directions[i].z);
		//		}
		//		else {
		//			p.point = Vector3(road.points[i].x, road.points[i].y, road.points[i].z);
		//			p.source_control = Vector3(road.directions[i].x, road.directions[i].y, road.directions[i].z);
		//		}
		//		p.destination_control = -p.source_control;
		//		p.normal = Vector3::UNIT_Y;
		//		paveCom.mBezier->addPoint(p);
		//	}

		//	paveCom.mPavement->setBezierCom(paveCom.mBezier);

		//	PavementMeshBaseData data;
		//	data.groupName = "KarstRailwayGroup";
		//	data.meshName = "model/block/bs/dt_guidao_001_lod1.mesh";
		//	data.quat = Echo::Quaternion(Degree(0.0f), Vector3(0, 1.0, 0));
		//	data.scale = 5.0f;
		//	paveCom.mPavement->setFitGroundModel(mFitTerrain);
		//	if (mFitTerrain) {
		//		paveCom.mPavement->setPavementFitTerrain(this);
		//	}
		//	paveCom.mPavement->createPavement(data, PavementComponent::UpdateAll);
		//	paveCom.mPavement->SetRenderDistance(mRenderDist, Name("KarstRailwayGroup"));

		//	paveComs.push_back(paveCom);
		//}
		//uint64_t num = index.ToUInt64();
		//railwayIndexes.insert(std::make_pair(num, paveComs));
		//if (auto it = railwayRequests.find(num); it != railwayRequests.end()) {
		//	railwayRequests.erase(it);
		//}
	}

	EchoIndex2 KarstRoadComponent::getTileIndex(const DVector3& pos)
	{
		auto origin = mKarstGroup->getOriginPositionDouble();
		auto options = mKarstGroup->getOptions();
		return EchoIndex2((int)floor((pos.x - origin.x) / options->mKarstSize), (int)floor((pos.z - origin.z) / options->mKarstSize));
	}

	EchoIndex2 KarstRoadComponent::getSubTileIndex(const DVector3& pos)
	{
		auto origin = mKarstGroup->getOriginPositionDouble();
		auto options = mKarstGroup->getOptions();
		return EchoIndex2((int)floor((pos.x - origin.x) / options->mKarstSize * 8.0), (int)floor((pos.z - origin.z) / options->mKarstSize * 8.0));
	}

	void KarstRoadComponent::onUpdate()
	{
		if (!mKarstGroup)
			return;

		auto camPos = Root::instance()->getMainSceneManager()->getMainCamera()->getPosition();

		if (camPos.y > mHeightLimit)
			return;

		EchoIndex2 subTileIndex = getSubTileIndex(camPos);
		int range = 1;
		if (mRoadDataHelper) {
			for (int i = -range; i <= range; i++) {
				for (int j = -range; j <= range; j++) {
					EchoIndex2 _index(subTileIndex.x + i, subTileIndex.z + j);
					uint64_t num = _index.ToUInt64();
					if (roadExist.find(num) != roadExist.end()
						&& roadIndexes.find(num) == roadIndexes.end()
						&& roadRequests.find(num) == roadRequests.end())
					{
						mRoadDataHelper->addRequest(_index);
						roadRequests.insert(num);
					}
				}
			}
		}

		if (mRailwayDataHelper) {
			for (int i = -range; i <= range; i++) {
				for (int j = -range; j <= range; j++) {
					EchoIndex2 _index(subTileIndex.x + i, subTileIndex.z + j);
					uint64_t num = _index.ToUInt64();
					if (railwayExist.find(num) != railwayExist.end()
						&& railwayIndexes.find(num) == railwayIndexes.end()
						&& railwayRequests.find(num) == railwayRequests.end())
					{
						mRailwayDataHelper->addRequest(_index);
						railwayRequests.insert(num);
					}
				}
			}
		}

		for (auto it = roadIndexes.begin(); it != roadIndexes.end(); )
		{
			EchoIndex2 index;
			index.FromUInt64(it->first);
			if ((std::abs(subTileIndex.x - index.x) + std::abs(subTileIndex.z - index.z)) > 2 * range) {
				auto& paveComs = it->second;
				for (auto& paveCom : paveComs) {
					mRoadActor->delComponent(paveCom.mPavement);
					mRoadActor->delComponentInfo(paveCom.mBezier->m_pActComInfo);
					mRoadActor->delComponent(paveCom.mBezier);
				}
				it = roadIndexes.erase(it);
			}
			else {
				it++;
			}
		}

		for (auto it = railwayIndexes.begin(); it != railwayIndexes.end(); )
		{
			EchoIndex2 index;
			index.FromUInt64(it->first);
			if ((std::abs(subTileIndex.x - index.x) + std::abs(subTileIndex.z - index.z)) > 2 * range) {
				auto& paveComs = it->second;
				for (auto& paveCom : paveComs) {
					mRoadActor->delComponent(paveCom.mPavement);
					mRoadActor->delComponentInfo(paveCom.mBezier->m_pActComInfo);
					mRoadActor->delComponent(paveCom.mBezier);
				}
				it = railwayIndexes.erase(it);
			}
			else {
				it++;
			}
		}
	}

	struct KarstRoadResourceRequest
	{
		EchoIndex2 index;
		std::string levelName;
		bool haveRoad;
		std::vector<KarstRoadSection> roads;
	};

	struct LoadKarstRoadRequest
	{
		KarstRoadResourceRequest* request = nullptr;
		~LoadKarstRoadRequest()
		{
			if (request)
				delete request;
		}
		friend std::ostream& operator << (std::ostream& o, const LoadKarstRoadRequest& r)
		{
			return o;
		}
	};

	WorkQueue::RequestID KarstRoadDataHelper::addRequest(EchoIndex2 index)
	{
		LoadKarstRoadRequest* req = new LoadKarstRoadRequest;
		req->request = new KarstRoadResourceRequest;
		req->request->levelName = mLevelName;
		req->request->index = index;
		return mWorkQueue->addRequest(mWorkQueueChannel, 0u, Any(req), 0, WorkQueue::RESOURCE_TYPE_TERRAIN, false);
	}

	WorkQueue::RequestID KarstRailwayDataHelper::addRequest(EchoIndex2 index)
	{
		LoadKarstRoadRequest* req = new LoadKarstRoadRequest;
		req->request = new KarstRoadResourceRequest;
		req->request->levelName = mLevelName;
		req->request->index = index;
		return mWorkQueue->addRequest(mWorkQueueChannel, 0u, Any(req), 0, WorkQueue::RESOURCE_TYPE_TERRAIN, false);
	}

	bool KarstRoadDataHelper::canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		return true;
	}

	bool KarstRailwayDataHelper::canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		return true;
	}

	void ProcessRoadSections(const std::vector<KarstRoadSection>& roadSections, const LoadKarstRoadRequest* resReq)
	{
		for (const auto& roadSection : roadSections){
			resReq->request->roads.push_back(roadSection);
		}
	}

	WorkQueue::Response* KarstRoadDataHelper::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		WorkQueue::Response* response = nullptr;
		const LoadKarstRoadRequest* resReq = any_cast<LoadKarstRoadRequest*>(req->getData());
		auto roadCreator = mRoadComponent->getKarstGroup()->getKarstRoadMeshCreator().lock();
		auto page = EchoKarst::packIndex(resReq->request->index.x, resReq->request->index.z);
		if (roadCreator && roadCreator->IsRoadMeshExist(page)) {
			resReq->request->haveRoad = true;
			const auto& roads = roadCreator->getRoadsInPage(page);
			for (const auto& road : roads)
			{
				auto& roadSection = resReq->request->roads.emplace_back();
				roadSection.points = road.points;
				if (road.points.size() < 2) continue;
				for (int i = 0; i < road.points.size(); ++i)
				{
					if (i == 0) {
						roadSection.directions.push_back((road.points[i] - road.points[i + 1]).GetNormalized() * 10.f);
					}
					else if (i == road.points.size() - 1) {
						roadSection.directions.push_back((road.points[i - 1] - road.points[i]).GetNormalized() * 10.f);
					}
					else {
						roadSection.directions.push_back((road.points[i - 1] - road.points[i + 1]).GetNormalized() * 10.f);
					}
				}
			}
			response = new WorkQueue::Response(req, true, Any(), "", "");
		}
		else {
			resReq->request->haveRoad = false;
		}
		return response;
	}

	WorkQueue::Response* KarstRailwayDataHelper::handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)
	{
		WorkQueue::Response* response = nullptr;
		const LoadKarstRoadRequest* resReq = any_cast<LoadKarstRoadRequest*>(req->getData());
		auto roadCreator = mRoadComponent->getKarstGroup()->getKarstRailMeshCreator().lock();
		auto page = EchoKarst::packIndex(resReq->request->index.x, resReq->request->index.z);
		if (roadCreator && roadCreator->IsRoadMeshExist(page))
		{
			resReq->request->haveRoad = true;
			const auto& roads = roadCreator->getRoadsInPage(page);
			for (const auto& road : roads)
			{
				auto& roadSection = resReq->request->roads.emplace_back();
				roadSection.points = road.points;
				for (int i = 0; i < road.points.size(); ++i) {
					if (i == 0) {
						roadSection.directions.push_back((road.points[i] - road.points[i + 1]).GetNormalized() * 10.f);
					}
					else if (i == road.points.size() - 1) {
						roadSection.directions.push_back((road.points[i - 1] - road.points[i]).GetNormalized() * 10.f);
					}
					else {
						roadSection.directions.push_back((road.points[i - 1] - road.points[i + 1]).GetNormalized() * 10.f);
					}
				}
			}
			response = new WorkQueue::Response(req, true, Any(), "", "");
		}
		//auto roadIndex = resReq->request->index;
		//DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(("levels/" + resReq->request->levelName + "/railway/jinghu" + std::to_string(roadIndex.x) + "_" + std::to_string(roadIndex.z) + std::string(".line")).c_str(), true));
		//if (!pDataStream.isNull()) {
		//	resReq->request->haveRoad = true;
		//	int roadSize = 0;
		//	pDataStream->readData(&roadSize);
		//	std::vector<KarstRoadSection> roadSections(roadSize);
		//	for (int i = 0; i < roadSize; ++i) {
		//		int pointSize;
		//		pDataStream->readData(&pointSize);
		//		roadSections[i].points.resize(pointSize);
		//		roadSections[i].directions.resize(pointSize);
		//		for (int j = 0; j < pointSize; j++) {
		//			pDataStream->readData(&roadSections[i].points[j], 1);
		//			pDataStream->readData(&roadSections[i].directions[j], 1);
		//		}
		//	}
		//	ProcessRoadSections(roadSections, resReq);
		//	response = new WorkQueue::Response(req, true, Any(), "", "");
		//}
		else {
			resReq->request->haveRoad = false;
		}
		return response;
	}

	bool KarstRoadDataHelper::canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		return true;
	}

	bool KarstRailwayDataHelper::canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		return true;
	}

	void KarstRoadDataHelper::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		const WorkQueue::Request* req = res->getRequest();
		const LoadKarstRoadRequest* resReq = any_cast<LoadKarstRoadRequest*>(req->getData());

		mRoadComponent->createRoads(resReq->request->index, resReq->request->roads);
		delete resReq;
	}

	void KarstRailwayDataHelper::handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ)
	{
		const WorkQueue::Request* req = res->getRequest();
		const LoadKarstRoadRequest* resReq = any_cast<LoadKarstRoadRequest*>(req->getData());

		mRoadComponent->createRailways(resReq->request->index, resReq->request->roads);
		delete resReq;
	}

	DECLAREACTORCOMTYPE_CPP(KarstVoxelComponent)
		EDITOR_REFLECT_BEGIN(KarstVoxelComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LevelName, "LevelName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RowNum, "RowNum", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ColNum, "ColNum", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_KarstSize, "KarstSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BatchSize, "BatchSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_GridNum, "GridNum", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Origin, "Origin", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(KarstVoxelComponentInfo);

	KarstVoxelComponent::KarstVoxelComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type)
	{
	}

	KarstVoxelComponent::~KarstVoxelComponent()
	{
	}

	void KarstVoxelComponent::init(ActorComponentInfo* info)
	{
		if (!info)
			return;

		KarstVoxelComponentInfo* pInfo = reinterpret_cast<KarstVoxelComponentInfo*>(info);
		mLevelName = pInfo->GetLevelName();
		mRowNum = pInfo->GetRowNum();
		mColNum = pInfo->GetColNum();
		mKarstSize = pInfo->GetKarstSize();
		mBatchSize = pInfo->GetBatchSize();
		mGridNum = pInfo->GetGridNum();
		mOrigin = pInfo->GetOrigin();

		SetNotifiedUpdate(true);
		RegisterPhysicsTerrain();
	}

	void KarstVoxelComponent::unInit()
	{
		SetNotifiedUpdate(false);
		UnregisterPhysicsTerrain();
	}

	void KarstVoxelComponent::replay(ActorComponentInfo* info)
	{
		if (!info)
			return;

		KarstVoxelComponentInfo* pInfo = static_cast<KarstVoxelComponentInfo*>(info);

		setLevelName(pInfo->GetLevelName());
		setRowNum(pInfo->GetRowNum());
		setColNum(pInfo->GetColNum());
		setKarstSize(pInfo->GetKarstSize());
		setBatchSize(pInfo->GetBatchSize());
		setGridNum(pInfo->GetGridNum());
		setOriginPosition(pInfo->GetOrigin());
	}

	void KarstVoxelComponent::copyTo(const ActorComponentPtr& actCom)
	{

	}

	void KarstVoxelComponent::connectCom()
	{
		getActor()->setPosition({ 0.f, 0.f, 0.f });

		CreateKarstTerrain();

		CreateVoxelTerrain();
	}

	void KarstVoxelComponent::disconnectCom()
	{
		DestroyVoxelTerrain();

		DestroyKarstTerrain();
	}

	void KarstVoxelComponent::CreateVoxelTerrain()
	{
		if (mVoxelTerrain != nullptr)
			return;

		Echo::SceneManager* pSceneManager = Root::instance()->getMainSceneManager();
		if (pSceneManager == nullptr)
			return;

		mVoxelTerrain = std::make_shared<VoxelTerrain>(pSceneManager);
		if (mVoxelTerrain)
		{
			mVoxelTerrain->randomKarst(mKarstGroup);
			ChangePhysics();
		}
	}

	void KarstVoxelComponent::DestroyVoxelTerrain()
	{
		if (mVoxelTerrain == nullptr)
			return;
		DestroyPhysics();
		mVoxelTerrain.reset();
	}

	void KarstVoxelComponent::CreateKarstTerrain()
	{
		if (mKarstGroup != nullptr)
			return;

		Echo::KarstParams params;
		params.levelName = mLevelName;
		params.batchSize = mBatchSize;
		params.originPosition = mOrigin;
		params.gridNum = mGridNum;
		params.karstSize = mKarstSize;
		params.row = mRowNum;
		params.col = mColNum;

		mKarstGroup = Echo::KarstSystem::instance()->createGroup(mLevelName, getSceneManager(), params, false, true);
	}

	void KarstVoxelComponent::DestroyKarstTerrain()
	{
		if (mKarstGroup == nullptr)
			return;
		Echo::KarstSystem::instance()->destroyGroup(mKarstGroup);
		mKarstGroup = nullptr;
	}

	void KarstVoxelComponent::visibleStateChanged(bool visible)
	{
		if (mVoxelTerrain)
		{
			VoxelWorld* world = mVoxelTerrain->GetVoxelWorld();
			if (world != nullptr) {
				world->SetVisible(visible);
			}
		}
	}

	AxisAlignedBox KarstVoxelComponent::getLoadBoundingBox()
	{
		if (mKarstGroup) {
			Vector3 vMin = Vector3(0, mKarstGroup->getMinHeight(), 0) + mKarstGroup->getOriginPosition();
			auto options = mKarstGroup->getOptions();
			Vector3 vMax = Vector3(options->mKarstSize * mKarstGroup->getRowNum(), mKarstGroup->getMaxHeight(), options->mKarstSize * mKarstGroup->getColNum()) + mKarstGroup->getOriginPosition();
			AxisAlignedBox aaBox(vMin, vMax);
			return aaBox;
		}
		else {
			return AxisAlignedBox();
		}
	}

	ActorComponent* KarstVoxelComponent::getThisCompPtr()
	{
		return this;
	}

	bool KarstVoxelComponent::getIsNearby(const Vector3& pos) {
		if (!mVoxelTerrain)
		{
			return false;
		}
		VoxelWorld* world = mVoxelTerrain->GetVoxelWorld();
		VoxelIntBox worldBounds = world->GetWorldBounds();
		IntVector3 iPos = world->GlobalToLocal(pos);
		uint64 sqrDist = worldBounds.ComputeSquaredDistanceFromBoxToPoint(iPos);
		return sqrDist < Math::Sqr(600.f * world->m_VoxelSizeInv);
	}

	bool KarstVoxelComponent::IsCanPhysics()
	{
		return mVoxelTerrain != nullptr;
	}

	void KarstVoxelComponent::AddMeshColliderImpl(MeshColliderComponent* pCollider, const EchoIndex3& index)
	{
		pCollider->SetMeshGenFun(std::bind<void>(&KarstVoxelComponent::GenerateVBIBFunction, index, this, std::placeholders::_1, std::placeholders::_2));
		pCollider->setPosition(mVoxelTerrain->GetVoxelWorld()->GetFinestChunkPosition(index));
	}

	void KarstVoxelComponent::getAllBlockIndex(std::vector<EchoIndex3>& leavesIndexs)
	{
		leavesIndexs = mVoxelTerrain->GetVoxelWorld()->GetFinestChunkIndex(AxisAlignedBox::BOX_INFINITE);
	}

	void KarstVoxelComponent::getBlockIndex(const AxisAlignedBox& localBound, std::vector<EchoIndex3>& indexs)
	{
		indexs = mVoxelTerrain->GetVoxelWorld()->GetFinestChunkIndex(localBound);
	}

	AxisAlignedBox KarstVoxelComponent::getBlockBound(const EchoIndex3& index)
	{
		return mVoxelTerrain->GetVoxelWorld()->GetFinestChunkBounds(index);
	}

	void KarstVoxelComponent::GenerateVBIBFunction(const EchoIndex3& index, KarstVoxelComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
		if (_thisPtr != nullptr)
		{
			if (_thisPtr->mVoxelTerrain == nullptr) return;
			_thisPtr->mVoxelTerrain->GetVoxelWorld()->CreateFinestChunkGeometry(index, ibVec, vbVec);
		}
	}
}
