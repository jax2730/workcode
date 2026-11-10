#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoPavementComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoKarstSystem.h"
#include "EchoGeneratedTerrainPhysics.h"
#include "VoxelTerrain/EchoVoxelTerrain.h"

namespace Echo
{
	class KarstRoadComponent;

	struct KarstRoadSection {

		std::vector<Vector3> points;
		std::vector<Vector3> directions;
	};

	class KarstRoadDataHelper : public WorkQueue::RequestHandler, public WorkQueue::ResponseHandler
	{
	public:
		KarstRoadDataHelper(KarstRoadComponent* roadComponent, std::string levelName) : mRoadComponent(roadComponent), mLevelName(levelName) {
			mWorkQueue = Root::instance()->getWorkQueue();
			mWorkQueueChannel = mWorkQueue->getChannel("KarstRoadLoadChannel" + levelName);
			mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
			mWorkQueue->addResponseHandler(mWorkQueueChannel, this);
		}
		~KarstRoadDataHelper() {
			mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
			mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
			mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);
		}

		virtual bool				canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		virtual WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		virtual bool				canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;
		virtual void				handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;

		WorkQueue::RequestID addRequest(EchoIndex2 index);

		KarstRoadComponent* mRoadComponent;
		WorkQueue* mWorkQueue;
		uint16 mWorkQueueChannel;
		std::string mLevelName;
	};

	class KarstRailwayDataHelper : public WorkQueue::RequestHandler, public WorkQueue::ResponseHandler
	{
	public:
		KarstRailwayDataHelper(KarstRoadComponent* component, std::string levelName) : mRoadComponent(component), mLevelName(levelName) {
			mWorkQueue = Root::instance()->getWorkQueue();
			mWorkQueueChannel = mWorkQueue->getChannel("KarstRailwayLoadChannel" + levelName);
			mWorkQueue->addRequestHandler(mWorkQueueChannel, this);
			mWorkQueue->addResponseHandler(mWorkQueueChannel, this);
		}
		~KarstRailwayDataHelper() {
			mWorkQueue->abortRequestsByChannel(mWorkQueueChannel);
			mWorkQueue->removeRequestHandler(mWorkQueueChannel, this);
			mWorkQueue->removeResponseHandler(mWorkQueueChannel, this);
		}

		virtual bool				canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		virtual WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		virtual bool				canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;
		virtual void				handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;

		WorkQueue::RequestID addRequest(EchoIndex2 index);

		KarstRoadComponent* mRoadComponent;
		WorkQueue* mWorkQueue;
		uint16 mWorkQueueChannel;
		std::string mLevelName;
	};
	
	class _EchoComponentExport KarstRoadComponent : public ActorComponent, public PavementFitTerrain, public FrameListener
	{
	public:
		DECLAREACTORCOMTYPE_H(KarstRoadComponent)
			REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual AxisAlignedBox	getLoadBoundingBox() override;
		virtual void onUpdate() override;
	public:
		void setRenderDist(float dist) { mRenderDist = dist; }

		void createRoads(EchoIndex2 index, const std::vector<KarstRoadSection>& roads);
		void createRailways(EchoIndex2 index, const std::vector<KarstRoadSection>& roads);
		void OnKarstCreateEvent(KarstGroup* karstGroup);

		virtual void getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data) override;
		KarstGroup* getKarstGroup() const { return mKarstGroup; }
	private:
		EchoIndex2 getTileIndex(const DVector3& pos);
		EchoIndex2 getSubTileIndex(const DVector3& pos);
		void initRoadDataHelper();
	private:
		const bool mFitTerrain = false;
		float mRenderDist = 2000.f;
		float mHeightLimit = 10000.f;
		ActorPtr mRoadActor;

		KarstGroup* mKarstGroup = nullptr;
		KarstRoadDataHelper* mRoadDataHelper = nullptr;
		KarstRailwayDataHelper* mRailwayDataHelper = nullptr;
		std::unordered_set<uint64_t> roadExist;
		std::unordered_set<uint64_t> roadRequests;
		std::unordered_set<uint64_t> railwayExist;
		std::unordered_set<uint64_t> railwayRequests;
		struct PavementCom {
			PavementComponentPtr mPavement;
			PiecewiseBezierComponentPtr mBezier;
		};
		std::unordered_map<uint64_t, std::vector<PavementCom>> roadIndexes;
		std::unordered_map<uint64_t, std::vector<PavementCom>> railwayIndexes;
	};

	class _EchoComponentExport KarstRoadComponentInfo : public ActorComponentInfo {
	public:
		KarstRoadComponentInfo() {}
		KarstRoadComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual ~KarstRoadComponentInfo() {};
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(KarstRoadComponent)
			_DECLCOMINFO_PARAM_SIM(float, RenderDist, 4096.0f, setRenderDist)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(KarstRoadComponent, ALL_MOVEABLE);

	class KarstRoadManager : public Singleton<KarstRoadManager>
	{
	public:
		KarstRoadManager()
		{
			auto actorManager = Echo::ActorSystem::instance()->getMainActorManager();
			if (!actorManager) return;
			mKarstRoadActor = actorManager->createActor();
			mKarstRoadActor->setActorID("KarstRoadActor");
			mKarstRoadComponent = mKarstRoadActor->AddComponent<KarstRoadComponent>("KarstRoadComponent", mKarstRoadActor->getRootNodeCom());
		}

		~KarstRoadManager()
		{
			if (!mKarstRoadActor) return;
			mKarstRoadActor->delComponent(mKarstRoadComponent);
			mKarstRoadActor->getActorManager()->deleteActor(mKarstRoadActor);
		}
	private:
		ActorPtr mKarstRoadActor;
		EchoWeakPtr<KarstRoadComponent> mKarstRoadComponent;
	};

	class _EchoComponentExport KarstVoxelComponent : public ActorComponent, public GeneratedTerrainPhysics_Voxel
	{
	public:
		DECLAREACTORCOMTYPE_H(KarstVoxelComponent)
			REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void visibleStateChanged(bool visibleState) override;

		void setLevelName(std::string levelName) { mLevelName = levelName; }
		void setRowNum(uint32 rowNum) { mRowNum = rowNum; }
		void setColNum(uint32 colNum) { mColNum = colNum; }
		void setKarstSize(float karstSize) { mKarstSize = karstSize; }
		void setBatchSize(uint32 batchSize) { mBatchSize = batchSize; }
		void setGridNum(uint32 gridNum) { mGridNum = gridNum; }
		void setOriginPosition(DVector3 origin) { mOrigin = origin; }

		virtual AxisAlignedBox	getLoadBoundingBox() override;

		virtual ActorComponent* getThisCompPtr() override;
		virtual bool IsCanPhysics() override;
		virtual void CreatePhysics() override;
		virtual void AddMeshColliderImpl(MeshColliderComponent* pCollider, const EchoIndex3& index) override;
		virtual void getAllBlockIndex(std::vector<EchoIndex3>&) override;
		virtual void getBlockIndex(const AxisAlignedBox&, std::vector<EchoIndex3>&) override;
		virtual AxisAlignedBox getBlockBound(const EchoIndex3&) override;
		virtual bool getIsNearby(const Vector3& worldPos) override;
		static void GenerateVBIBFunction(const EchoIndex3& index, KarstVoxelComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);
	private:
		void CreateVoxelTerrain();
		void CreateKarstTerrain();
		void DestroyVoxelTerrain();
		void DestroyKarstTerrain();

		std::string mLevelName;
		uint32 mRowNum, mColNum, mBatchSize, mGridNum;
		float mKarstSize;
		DVector3 mOrigin;
		KarstGroup* mKarstGroup = nullptr;
		std::shared_ptr<VoxelTerrain> mVoxelTerrain;
	};

	class _EchoComponentExport KarstVoxelComponentInfo : public ActorComponentInfo {
	public:
		KarstVoxelComponentInfo() {}
		KarstVoxelComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual ~KarstVoxelComponentInfo() {};
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(KarstVoxelComponent)
			_DECLCOMINFO_PARAM_SIM(std::string, LevelName, "karstworld", setLevelName)
			_DECLCOMINFO_PARAM_SIM(uint32, RowNum, 129u, setRowNum)
			_DECLCOMINFO_PARAM_SIM(uint32, ColNum, 85u, setColNum)
			_DECLCOMINFO_PARAM_SIM(float, KarstSize, 32768.0f, setKarstSize)
			_DECLCOMINFO_PARAM_SIM(uint32, BatchSize, 32u, setBatchSize)
			_DECLCOMINFO_PARAM_SIM(uint32, GridNum, 8192, setGridNum)
			_DECLCOMINFO_PARAM_SIM(DVector3, Origin, DVector3(-2097152.0, 0.0f, -1048576.0), setOriginPosition)
			_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(KarstVoxelComponent, ALL_MOVEABLE);
};

