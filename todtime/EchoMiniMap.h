#pragma once
#include "3DUI/Echo3DUIComponent.h"
#include "3DUI/Echo3DUIComponentManager.h"
#include "EchoVector2.h"
#include "EchoVector3.h"
#include "FairyGUI.h"
#include "Actor/EchoActor.h"
#include "EchoWeatherInfoListener.h"
#include "JobSystem/EchoJobState.h"

namespace Echo
{
	class SceneManager;
	class WorldManager;
	class AirPlaneManager;

	enum CityBuildingType
	{
		CT_McDonald,
		CT_Hospital,
		CT_MAX,
	};

	struct CityUI
	{
		std::string mCityName;
		Vector3 mWorldPosition;
		Vector2 mMapPosition;
		echofgui::GObject* mCityUIObject = nullptr;
		echofgui::GObject* mCityNameObject = nullptr;
		echofgui::GRoot* mUIRoot = nullptr;
	};

	struct UIObjectProperty
	{
		echofgui::GObject* mObject = nullptr;
		Vector3 mInitSize;
		Vector3 mCurrentSize;
		Vector3 mInitPos;
		Vector3 mCurrentPos;
		bool mbVisible = true;
		void setVisible(bool InVal)
		{
			if (InVal == mbVisible || mObject == nullptr)
				return;

			mbVisible = InVal;
			mObject->setVisible(InVal);
		}
	};

	struct CityBuildingProperty
	{
		echofgui::GObject* mBuildingUI = nullptr;
		Vector3 mWorldPosition;
		Vector2 mMapPosition;
		std::string mTypeStr;
		std::string mBuildingName;
		CityBuildingType mType = CT_MAX;
	};

	struct ARect
	{
		Vector2 Start;
		Vector2 Range;
	};

	struct AreaBlockInfo
	{
		ARect mWorldRange;
		std::string mAreaName;  //行政区名称
		uint32 mAreaCode = 0; //行政区代码
		Vector2 mWSGPos;
	};

	struct CityMapPageManager
	{
		CityMapPageManager();
		~CityMapPageManager();
		void Create();
		AreaBlockInfo* CheckCamera(const Vector3& InPos);
		std::map<EchoIndex2, AreaBlockInfo> mMapPartitions;
		std::vector<AreaBlockInfo*> mAreaArray;
		Vector2 mBlockSize;
		Vector2 mWorldOrigin;
	};

	struct st_AreaTex
	{
		Vector2 mWorldOrigin;
		Vector2 mWorldMaxRange;
		std::vector<std::vector<uint32>> mAreaTexData;
		UIObjectProperty mAreaMapUI;
		bool mbVisible = true;
		void setVisible(bool InVal)
		{
			if (InVal == mbVisible || mAreaMapUI.mObject == nullptr)
				return;
			mbVisible = InVal;
			mAreaMapUI.mObject->setVisible(InVal);
		}
	};

	struct BlockInstance
	{
		enum State
		{
			NONE,
			PREPARE_LOAD,
			PREPARE_UNLOAD,
			LOADED,
		};

		struct BuildingInfo
		{
			Vector3 mBound;
			std::string mName;
			Vector3 mPosition;
			Vector3 mWorldPosition;
			Vector3 mScale;
		};

		std::set<Echo3DUIComponent*> mUIComponents;
		std::set<ActorPtr> mActors;
		std::map<BuildingInfo*, ActorPtr> mBuildingInfos;
		//bool mbCreate = false;
		State mState = NONE;
	};

	class KarstGroup;
	class MiniMapManager;
	class TrainLineData;
	/*class WeatherRequestJob : public Job
	{
	public:
		WeatherRequestJob(MiniMapManager*);
		virtual ~WeatherRequestJob();

		virtual void Execute();

		void request(uint32 id);

	private:
		MiniMapManager* m_pMiniManager;
		tthread::recursive_mutex m_queueLock;
		uint32 m_reqAreaId;
	};*/

	class MiniMapManager : public ActorLoadListener, public WorkQueue::RequestHandler, public WorkQueue::ResponseHandler, public FrameListener, public WeatherInfoListener
	{
	public:

		enum EventType
		{
			LoadBlock,
			UnloadBlock,

		};

		union BlockID
		{
			uint32 mID = 0;
			struct
			{
				int16 mX;
				int16 mY;
			};

			bool operator<(const BlockID& InBlockID) const
			{
				return mID < InBlockID.mID;
			}
		};

	public:
		MiniMapManager(SceneManager* InSceneManger, WorldManager* InWorldMgr);
		virtual ~MiniMapManager();
		virtual bool OnActorCreateFinish() override;
		void Initilize();
		void Uninitilize();
		void LoadCityMap(const std::string& InPath);
		void LoadCityBuildingMap(const std::string& InPath);
		void LoadCityArea(const std::string& InCodePath, const std::string& InLevelPath);
		void CreateCityUI(CityUI& InOutCity);
		bool CreateMap();
		void Tick();
		bool GetShowMap() const { return mbShowMap; }
		void SetShowMap(bool InVal);
		void SetShowCityName(bool InVal);
		bool GetShowCityName() { return mbShowCityName; }
		void SetShowBuildings(bool InVal);
		bool GetShowBuildings() { return mbShowBuildings; }
		void SetWorldOrigin(const Vector3& InWorldOrigin) { mWorldOrigin = InWorldOrigin; }
		void SetTerrainWidth(float InWidth) { mTerrainWidth = InWidth; }
		void SetTerrainHeight(float InHeight) { mTerrainHeight = InHeight; }
		void SetShowFullMap(bool InVal);
		bool GetShowFullMap() { return mbShowFullMap; }
		void SetShowSceneUI(bool InVal);
		bool GetShowSceneUI() { return mbShowSceneUI; }
		Vector2 GetPosInMapByWorldPos(const Vector3& InPos);
		Vector3 CalculatePlayerRotate();
		void OnClickShowMap(echofgui::EventContext* InContext);
		void OnClickChangeFullMap(echofgui::EventContext* InContext);
		bool InitUIObjectProperty(UIObjectProperty& InOutUIObjectProperty,echofgui::GObject* InParentObject, const std::string& InChildName);
		UIObjectProperty CreateUIFromResource(const std::string& InPackageName, const std::string& InComponentName);
		void MoveMap();
		void UpdatePlayerPosition(const Vector3& InPosition);
		void NotifyCameraVisible();
		void SetSceneUIRenderDistance(float InDis) { mSceneUIRenderDistance = InDis; }
		//@param inEventType： 参照echofgui::UIEventType
		void AddMiniMapEventListener(int inEventType, const echofgui::EventCallback& callback);
		void RemoveMiniMapEventListener(int inEventType);

		void OnKarstCreateEvent(KarstGroup* inGroup);

		void update(const WeatherInfo&);

		virtual bool frameEnded(const FrameEvent& evt) override;

		/// Implementation for WorkQueue::RequestHandler
		bool canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		/// Implementation for WorkQueue::RequestHandler
		WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		/// Implementation for WorkQueue::RequestHandler
		WorkQueue::Response* handleIOCPRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;
		/// Implementation for WorkQueue::ResponseHandler
		bool canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;
		/// Implementation for WorkQueue::ResponseHandler
		void handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;
	private:
		bool mbShowMap = true;
		inline static float G_Scale = 0.001f;
		SceneManager* mSceneMgr;
		WorldManager* mWorldMgr;
		float mMapWidth = 512.0f;
		float mMapHeight = 512.0f;
		float mTerrainWidth = 0.0f;
		float mTerrainHeight = 0.0f;
		float mTerrainBlockSize = 0.0f;
		Vector3 mWorldOrigin;
		Echo3DUIComponent* mMapUICom = nullptr;
		ActorPtr mMapUIActor;
		bool mbIsCreate = false;
		bool mbShowCityName = false;
		bool mbShowFullMap = false;
		bool mbShowBuildings = true;
		bool mbShowSceneUI = true;
		bool isWeatherChange = false;
		ActorPtr mWeatherUIActor;
		ActorPtr mTemperatureUIActor;
		ActorPtr mElevationUIActor;
		Echo3DUIComponent* mWeatherUICom = nullptr;
		Echo3DUIComponent* mTemperatureUICom = nullptr;
		Echo3DUIComponent* mElevationUICom = nullptr;
		//WeatherRequestJob* mWeatherJob = nullptr;
		UIObjectProperty mWeatherUI;
		UIObjectProperty mTemperatureUI;
		UIObjectProperty mElevationUI;
		UIObjectProperty mMapUI;
		UIObjectProperty mPlayerUI;
		UIObjectProperty mMapObject;
		UIObjectProperty mMapRangeUI;
		UIObjectProperty mCurCityUI;
		UIObjectProperty mCurCityNameUI;
		UIObjectProperty mCurPositionUI;
		UIObjectProperty mShowMapUI;
		UIObjectProperty mChangeFullMapUI;
		UIObjectProperty mCurAreaUI;
		UIObjectProperty mAreaMapUI;
		std::vector<CityUI> mCityMap;
		//std::map<CityBuildingType, std::vector<CityBuildingProperty>> mBuildingMaps;
		std::vector<CityBuildingProperty> mBuildings;
		std::set<Echo3DUIComponent*> mWorldBuildingUIs;
		std::map<uint32, std::string> mAdminRegions;
		std::map<uint32, std::string> mAdminRegionAreaStrs;
		std::map<std::string, std::string> mAreaStrs;
		CityMapPageManager* mPageManager = nullptr;
		std::map<uint32, std::vector<AreaBlockInfo*>> mAreas;
		std::vector<st_AreaTex> mAreaTexs;
		Vector3 mPlayerWorldPosition;

		bool mTimeZeroRefPosInited = false;
		Vector3 mTimeZeroRefPos = Vector3::ZERO;


		std::map<BlockID, BlockInstance> mWorldBlocks;
		std::set<BlockID> mWaitLoadBlocks;
		std::set<BlockID> mWaitUnloadBlocks;
		std::mutex mMtx;
		BlockID mLastCameraBlockID;
		float mSceneUIRenderDistance = 1000.0f;
		bool mbInit = false;
		bool mbWorldMap = false;
		std::set<ActorPtr> mCreateActors;
		WorkQueue* mWorkQueue = nullptr;
		uint16 mWorkQueueChannel = 0;
		AirPlaneManager* mPlaneManager = nullptr;

		std::vector<std::shared_ptr<TrainLineData>> mTrainLines;
	};

	class AirPlane;

	struct AirLine {
		Vector3 start;
		Vector3 end;
	};
	class AirPlaneManager : public FrameListener
	{
	public:
		class PlaneController : public Job
		{
		public:
			PlaneController();
			virtual ~PlaneController();

			virtual void Execute();

			void startOne(const AirLine&);
			void update();
			bool isIdle();
		public:
			std::vector<AirPlane*> mWorking;
			std::vector<AirPlane*> mFinished;
		};
		void addAirLine(const Vector3& start, const Vector3& end);
	public:
		AirPlaneManager();
		~AirPlaneManager();
		void tick(float dt);
		bool frameStarted(const FrameEvent& evt);
	private:
		std::vector<AirLine> mLines;
		PlaneController* mController = nullptr;
	};

	class AirPlane
	{
	public:
		enum FlightStage {
			Ascend,
			Cruise,
			Descend,
			Finished
		};
	public:
		AirPlane() {};
		~AirPlane();
		void reset(const Vector3& start, const Vector3& end);
		void tick(float dt);
		void update();
		bool isFinished() { return mStage == Finished; }
	private:
		void _tick(float dt);
		Vector3 computeFlightPosition(float);
		Quaternion calcRotation(const Vector3& dir);
		void flushData();
	private:
		Vector3 mStartPos;
		Vector3 mEndPos;
		Vector3 mCurrPos;
		Vector3 mBackPos;
		Vector3 mOffset;
		FlightStage mStage;
		Vector3 mDirection;
		Vector3 mBackDirection;

		float mTotalDistance;
		float mTraveledDistance;
		float mCurrentSpeed;

		const float takeoffSpeed = 50.0f;   // 起飞初始速度
		const float cruiseSpeed = 200.0f;   // 巡航速度
		const float acceleration = 5.0f;  // 加速度
		const float deceleration = 1.0f;  // 减速度

		float ascendRatio = 0.2f;   // 起飞占总距离比例
		float descendRatio = 0.2f;  // 降落占总距离比例
		float cruiseAltitude = 400.0f; // 飞行高度提升量

		float mStartTime;
		/*uint64 mTickCount = 0;
		float mTickProcess = 0.0f;*/
		bool mNeedFlush = false;
		std::vector<std::pair<Vector3, Quaternion>> mStateData;
		std::vector<std::pair<Vector3, Quaternion>> mBackStateData;
		int mStateIdx = 0;

		ActorPtr mActor;

	};

	struct TrainLine
	{
		int start;
		int end;
		float startDistance;
		float length;
	};

	struct MoveActor
	{
		virtual ~MoveActor() = default;
		virtual void update(float timeDelay) {}
		virtual void tick() = 0;

		float mTravelLength = 0.0f;
		float mSpeed = 30.0f;
		float mStopTime = 5.0f;
		int mCurrentSection = -1;
		float mStopTimeDelay = 0.0f;
	};

	class TrainLineData : public Job
	{
	public:
		TrainLineData(KarstGroup* karstGroup, const std::vector<Vector2>& pointData);
		~TrainLineData()
		{
			for (auto& actor : mMoveActors) {
				delete actor;
			}
		}

		virtual void Execute() override;

		void getPositionFromDistance(float distantce, Vector3& postion, Vector3& direction);
		void update(float timeDelay, const Vector3& camPos);

		void updateHeightData();

		std::vector<TrainLine> mLineData;
		std::vector<Vector3> mPointData;

		float mTotalLength = 0.0f;
		float mInterspace = 15.0f;
		Vector2 mCurrentCamPos = {};
		float mUpdateDist = 8000.0;
		float mUpdateLineDataDelay = 0.0f;
		float mTimeDelay = 0.0f;
		std::vector<float> mStopLength;
		int mSections = 1;
		std::vector<MoveActor*> mMoveActors;
		KarstGroup* mKarstGroup;
	};

	struct CarActor : public MoveActor
	{
		CarActor(std::string actorName, float travelLen, float scale, TrainLineData* lineData) : mLineData(lineData)
		{
			mTravelLength = travelLen;
			mCarActor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Name("actors/" + actorName), "", true);
			mCarActor->setScale(Vector3(scale));
		}

		void update(float timeDelay) override
		{
			mTravelLength += timeDelay * mSpeed;
			mLineData->getPositionFromDistance(mTravelLength, mPosition, mDirection);
		}

		void tick() override
		{
			mCarActor->setPosition(mPosition);
			Quaternion qua;
			Vector3 zDir = mDirection.crossProduct(Vector3::UP);
			Vector3 upDir = zDir.crossProduct(mDirection);
			qua.FromAxes(mDirection, upDir, zDir);
			mCarActor->setRotation(qua);
		}

		TrainLineData* mLineData;
		ActorPtr mCarActor;
		Vector3 mPosition;
		Vector3 mDirection;
	};

	struct TrainActor : public MoveActor
	{
		TrainActor(std::string headName, std::string bodyName, int bodyCount, float gap, float travelLen, float scale, TrainLineData* lineData)
			: mBodyCount(bodyCount)
			, mGap(gap)
			, mLineData(lineData)
		{
			mTravelLength = std::fmod(travelLen, lineData->mTotalLength);
			for (int i = 0; i < lineData->mStopLength.size(); i++) {
				if (mTravelLength <= lineData->mStopLength[i]) {
					mCurrentSection = i - 1;
					break;
				}
				else {
					continue;
				}
			}
			mHeadActor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Name("actors/" + headName), "", true);
			mHeadActor->setScale(Vector3(scale));
			mBodyActors.resize(bodyCount);

			for (int i = 0; i < bodyCount; i++){
				mBodyActors[i] = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Name("actors/" + bodyName), "", true);
				mBodyActors[i]->setScale(Vector3(scale));
			}

			mPositions.resize(bodyCount + 1, {});
			mDirections.resize(bodyCount + 1, {});
		}

		void update(float timeDelay) override
		{
			if (mLineData->mStopLength.size() && mLineData->mStopLength[mCurrentSection + 1] < mTravelLength) {
				if (mStopTimeDelay < mStopTime) {
					mStopTimeDelay += timeDelay;
				}
				else {
					mCurrentSection++;
					if (mCurrentSection == mLineData->mSections) {
						mTravelLength = 0.0f;
						mCurrentSection = -1;
					}
					mStopTimeDelay = 0.0f;
				}
			}
			else {
				mTravelLength += timeDelay * mSpeed;
			}

			mLineData->getPositionFromDistance(mTravelLength, mPositions[0], mDirections[0]);

			for (int i = 1; i <= mBodyCount; i++){
				mLineData->getPositionFromDistance(mTravelLength - i * mGap, mPositions[i], mDirections[i]);
			}
		}

		void tick() override
		{
			mHeadActor->setPosition(mPositions[0]);
			Quaternion qua;
			Vector3 zDir = mDirections[0].crossProduct(Vector3::UP);
			Vector3 upDir = zDir.crossProduct(mDirections[0]);
			qua.FromAxes(mDirections[0], upDir, zDir);
			mHeadActor->setRotation(qua);

			for (int i = 0; i < mBodyCount; i++){
				mBodyActors[i]->setPosition(mPositions[i + 1]);
				Quaternion qua;
				Vector3 zDir = mDirections[i + 1].crossProduct(Vector3::UP);
				Vector3 upDir = zDir.crossProduct(mDirections[i + 1]);
				qua.FromAxes(mDirections[i + 1], upDir, zDir);
				mBodyActors[i]->setRotation(qua);
			}
		}

		int mBodyCount;
		float mGap;
		TrainLineData* mLineData;
		ActorPtr mHeadActor;
		std::vector<ActorPtr> mBodyActors;
		std::vector<Vector3> mPositions;
		std::vector<Vector3> mDirections;
	};
}
