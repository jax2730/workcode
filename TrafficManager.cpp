#include "TrafficManager.h"
#include <functional>
#include "EchoSceneManager.h"
#include "EchoWorldManager.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActor.h"
#include "EchoSphericalTerrainComponent.h"
#include"EchoPlanetRoad.h"
#include"EchoPlanetRoadResource.h"
#include "EchoPiecewiseBezierUtil.h"
#include"chrono"
#include <ctime>
#include <cstdlib>
#include"CarFollowingModel.h"
#include <functional>
#include"EchoRoot.h"
#include"EchoLogManager.h"

namespace Echo
{

	void Traffic::Barrier::wait()
	{
		/*m_promise.get_future().wait();
		std::promise<void> _promise;
		std::swap(m_promise, _promise);*/

		std::unique_lock<std::mutex> lock(m_mutex);
		// wait解锁mutex并等待，直到被唤醒且m_signaled为true
		m_cv.wait(lock, [this] { return m_signaled; });
		// 成功等待后，重置标志以便下次使用
		m_signaled = false;

	}

	void Traffic::Barrier::signal()
	{
		//m_promise.set_value();
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_signaled = true;
		}
		// 通知一个正在等待的线程
		m_cv.notify_one();
	}

	Traffic::Traffic(SceneManager* InSceneManger, WorldManager* InWorldMgr)
		:mSceneMgr(InSceneManger)
		, mWorldMgr(InWorldMgr)
	{
		initializeCarFollowingModels();
		srand(static_cast<unsigned int>(time(nullptr)));
		//m_workBarrier.signal();
		Root::instance()->addFrameListener(this);
	}

	Traffic::~Traffic()
	{
		m_bQuite = true;
		m_mainBarrier.signal();

		if (m_thread && m_thread->joinable())
		{
			m_thread->join();
		}


		// 清理所有车辆
		for (Vehicle* vehicle : m_Vehicles) {
			if (vehicle) {
				delete vehicle;
			}
		}
		m_Vehicles.clear();

		// 清理所有道路
		for (Road* road : m_allRoads) {
			if (road) {
				delete road;
			}
		}
		m_allRoads.clear();

		// 清理城市连接映射
		m_nodeConnections.clear();

		// 清理路径数据
		m_allPaths.clear();
	}
	//ve
	void Traffic::_tick(float dt)
	{
		for (Road* road : m_allRoads)
		{
			if (road) {
				road->update(dt);
			}
		}
	}

	void Traffic::UpdatePositon()
	{
		for (Vehicle* vehicle : m_Vehicles)
		{
			if (vehicle)
			{

				vehicle->updatePos();
			}
		}
	}

	bool Traffic::OnActorCreateFinish()
	{


		LogManager::instance()->logMessage("FINISH");

		return false;
	}


	void Traffic::OnCreateFinish()
	{
		LogManager::instance()->logMessage("=== OnCreateFinish() starting with thread-safe initialization ===");

		// 在主线程中获取 level name
		std::string safeLevelName = "karst2"; // 默认值
		bool getLevelSuccess = false;

		// 尝试在主线程中安全获取关卡名
		if (mWorldMgr) {

			LogManager::instance()->logMessage("Attempting to get level name in main thread...");
			safeLevelName = mWorldMgr->getLevelName();
			getLevelSuccess = true;
			LogManager::instance()->logMessage("Successfully got level name in main thread: " + safeLevelName);

		}

		// 
		m_safeLevelName = safeLevelName;



	}

	void Traffic::onTick()
	{
		using namespace std::chrono;
		static auto lastTikcBegin = steady_clock::now();
		// 工作线程等待初始化完成



		auto frameStart = steady_clock::now();
		std::chrono::nanoseconds dt = frameStart - lastTikcBegin;

		// 测量_tick计算时间
		auto computeStart = steady_clock::now();
		_tick(std::chrono::duration<float>(dt).count());
		checkVehicle();
		//
		auto computeEnd = steady_clock::now();
		auto computeTime = duration_cast<milliseconds>(computeEnd - computeStart);

		lastTikcBegin = frameStart;

		// 通知主线程计算完成


		// 每100帧输出一次性能统计
		static int frameCount = 0;
		static float computecount = 0;

		/*if (++frameCount % 100 == 0) {
			LogManager::instance()->logMessage("Tick WorkThread - Compute: " + std::to_string(computecount / 100) +
				"ms, Wait: " + "ms");
			frameCount = 0;
			computecount = 0;

		}
		else
		{
			computecount += computeTime.count();

		}*/
		//计算完成的标记

		m_onTickCompleted = true;

	}
	void Traffic::checkVehicle()
	{

		for (Road* road : m_allRoads)
		{
			if (road) {

				road->checkVisible();
			}
		}
	}
	void Road::checkVisible()
	{

		if (!Root::instance()->getMainSceneManager() || !Root::instance()->getMainSceneManager()->getMainCamera()) return;
		Camera* mainCamera = Root::instance()->getMainSceneManager()->getMainCamera();

		auto isVisible = [mainCamera](const AxisAlignedBox& bound) {
			if (bound.isNull())
				return false;

			// Infinite boxes always visible
			if (bound.isInfinite())
				return true;

			// Get centre of the box
			Vector3 centre = bound.getCenter();
			// Get the half-size of the box
			Vector3 halfSize = bound.getHalfSize();

			// For each plane, see if all points are on the negative side
			// If so, object is not visible
			for (int plane = 0; plane < 6; ++plane)
			{
				// Skip far plane if infinite view frustum
				if (plane == FRUSTUM_PLANE_FAR && mainCamera->getFarClipDistance() == 0)
					continue;

				Plane::Side side = mainCamera->getFrustumPlane(plane).getSide(centre, halfSize);
				if (side == Plane::NEGATIVE_SIDE)
				{
					return false;
				}

			}
			return true;
		};

		for (Vehicle* vehicle : mCars)
		{
			const AxisAlignedBox& aabb = vehicle->mCar->getWorldBounds();
			vehicle->Visible = isVisible(aabb);
		}
	}


	//等待工作线程完成当前帧的数据计算，更新数据 

	void Traffic::initVehicle()
	{
		if (m_pathsGenerated && !m_Vehicles.empty())
		{
			// 如果车辆已存在，先清理 (避免重复创建)
			for (auto v : m_Vehicles) delete v;
			m_Vehicles.clear();
		}

		if (m_pathsGenerated)
		{
			addMultipleVehicles(2);
			assignPathsToAllVehicles();
		}
	}

	void Traffic::initRoads()
	{
		LogManager::instance()->logMessage("=== initRoads() starting with thread-safe implementation ===");

		// 使用预存储的安全级别名称，避免在工作线程中访问WorldManager
		std::string levelName = m_safeLevelName;


		// 构建JSON文件路径
		std::string sLevelName = "levels/" + levelName;
		std::string geoJsonFilePath = sLevelName + "/other/biejing_highway_connect_graph.json";
		std::string connectJsonFilePath = sLevelName + "/other/biejing_highway_connect.json";


		HighwayConnectData connectData = parseHighwayConnectData(connectJsonFilePath);
		HighwayGraphData graphData = parseHighwayGraphData(geoJsonFilePath);

		//JsonRoadData jsonRoadData = parseJsonRoadData(geoJsonFilePath);

		// 如果Highway数据有效，创建Highway道路网络
		if (!connectData.features.empty() && !graphData.links.empty()) {
			LogManager::instance()->logMessage("Highway JSON data loaded successfully. Creating Highway road network...");
			createRoadNetworkFromHighwayData(connectData, graphData);

			// 使用Highway数据创建的第一条道路作为起始道路
			if (!m_allRoads.empty()) {
				m_roadManager = m_allRoads[0];
				LogManager::instance()->logMessage("Highway road network initialization completed with " + std::to_string(m_allRoads.size()) + " roads.");
				generateAllPaths();
				return; // 成功加载Highway数据后直接返回
			}
		}






	}


	void Traffic::OnDestroy()
	{
		//移除小车  销毁
	}
	//道路链接




	Road::Road(const HighwayLink& linkData, const HighwayNode& sourceNode, const HighwayNode& targetNode, const std::vector<Vector3>& coordinates)
		:mRoadId(linkData.road_id), mRoadName(linkData.road_name), mSourceNodeId(linkData.source), mTargetNodeId(linkData.target), m_coordinatePath(coordinates)
	{
		m_isHighwayRoad = true;
		std::vector<Vector3> adjustedCoordinates = coordinates;

		if (coordinates.size() >= 2) {
			// 检查coordinates的实际方向是否与期望的source->target方向一致
			Vector3 coordStartPos = coordinates[0];
			Vector3 coordEndPos = coordinates.back();
			Vector3 expectedStartPos = sourceNode.geometry;
			Vector3 expectedEndPos = targetNode.geometry;

			// 计算距离来判断方向是否正确
			float startDistanceMatch = (coordStartPos - expectedStartPos).length();
			float endDistanceMatch = (coordEndPos - expectedEndPos).length();
			float startDistanceReverse = (coordStartPos - expectedEndPos).length();
			float endDistanceReverse = (coordEndPos - expectedStartPos).length();

			bool needReverse = (startDistanceReverse + endDistanceReverse) < (startDistanceMatch + endDistanceMatch);

			if (needReverse) {
				LogManager::instance()->logMessage("🔄 Road[" + std::to_string(linkData.road_id) +
					"] '" + linkData.road_name + "' coordinates auto-reversed to match source->target direction");
				std::reverse(adjustedCoordinates.begin(), adjustedCoordinates.end());
			}
			else {
				LogManager::instance()->logMessage("✅ Road[" + std::to_string(linkData.road_id) +
					"] '" + linkData.road_name + "' coordinates direction verified correct");
			}
		}

		// 使用调整后的坐标
		m_coordinatePath = adjustedCoordinates;

		//mLens = linkData.length;
		if (m_coordinatePath.size() >= 2)
		{
			m_segmentLengths.reserve(m_coordinatePath.size() - 1);
			m_cumulativeLengths.reserve(m_coordinatePath.size());
			m_cumulativeLengths.push_back(0.0f);
			for (size_t i = 1; i < m_coordinatePath.size(); ++i)
			{
				Vector3 segment = m_coordinatePath[i] - m_coordinatePath[i - 1];
				float segmentLength = sqrt(segment.x * segment.x + segment.y * segment.y + segment.z * segment.z);

				if (segmentLength <= 0.0f) {
					segmentLength = 0.1f; // 最小长度
				}
				m_segmentLengths.push_back(segmentLength);
				m_cumulativeLengths.push_back(m_cumulativeLengths.back() + segmentLength);
			}

			mLens = m_cumulativeLengths.back();
		}
		else
		{
			mLens = linkData.length;
		}

	}	void Road::addCar(Vehicle* car)
	{
		if (car)
		{
			mCars.push_back(car);
		}
	}

	void Road::update(float deltaTime)
	{

		updateEnvironment();
		// 收集需要转移的车辆
		std::vector<Vehicle*> vehiclesToTransfer;

		for (Vehicle* car : mCars)
		{
			Vehicle* leadingVehicle = findLeadingVehicle(car);
			if (leadingVehicle && car->getCarFollowingModel())
			{
				float gap = calculateGapToLeadingVehicle(car);
				car->applyCarFollowing(deltaTime, leadingVehicle, gap);
			}
			else
			{
				// 没有前车时使用自由流行驶
				if (car->getCarFollowingModel()) {
					/// 使用跟车模型计算自由流加速度（传入大间距模拟无前车）
					const float largeGap = 1000.0f; // 大间距模拟自由流
					const float dummyLeadSpeed = car->speed; // 虚拟前车速度与自己相同

					// 使用跟车模型计算自由流加速度
					car->acc = car->getCarFollowingModel()->calculateAcceleration(
						largeGap, car->speed, dummyLeadSpeed, 0.0f);

					// 限制加速度范围
					car->acc = std::max(-8.0f, std::min(3.0f, car->acc));


					// 更新速度
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);
					//
					car->speed = std::min(car->speed, 18.0f);

					// 更新位置

					// 更新速度和位置
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);
					if (car->laneDirection == Vehicle::LaneDirection::Forward) {
						car->s += car->speed * deltaTime;
					}
					else {
						car->s -= car->speed * deltaTime;
						// 反向车辆边界检查
						while (car->s < 0 && mLens > 0) {
							car->s += mLens; // 循环到道路末端
						}
					}
				}
				else {
					// 没有跟车模型的简单车辆
					const float freeFlowAcceleration = 1.0f; // m/s²
					car->acc = std::max(-3.0f, std::min(freeFlowAcceleration, car->acc));
					car->speed = std::max(0.0f, car->speed + car->acc * deltaTime);

					// 更新位置
					if (car->laneDirection == Vehicle::LaneDirection::Forward) {
						car->s += car->speed * deltaTime;
					}
					else {
						car->s -= car->speed * deltaTime;
						// 反向车辆边界检查
						while (car->s < 0 && mLens > 0) {
							car->s += mLens; // 循环到道路末端
						}
					}
				}
			}

			// 2. 检查车辆是否到达道路末端需要转移
			if (car->laneDirection == Vehicle::LaneDirection::Forward && car->s >= mLens)
			{
				vehiclesToTransfer.push_back(car);
				continue; // 跳过位置计算，因为即将转移
			}
			// 检查逆向车辆是否到达道路起点需要转移
			else if (car->laneDirection == Vehicle::LaneDirection::Backward && car->s <= 0.0f)
			{
				vehiclesToTransfer.push_back(car);
				continue; // 跳过位置计算，因为即将转移
			}

			// 3. 边界处理：如果车辆超出道路范围，进行循环或转移处理
			if (car->laneDirection == Vehicle::LaneDirection::Forward && car->s > mLens && mLens > 0) {
				bool hasNextDestination = (m_trafficManager && car->hasPath()) || m_nextRoad;
				if (!hasNextDestination) {
					car->s = fmod(car->s, mLens); // 正向车辆从头开始循环
				}
			}
			else if (car->laneDirection == Vehicle::LaneDirection::Backward && car->s < 0.0f && mLens > 0) {
				bool hasNextDestination = (m_trafficManager && car->hasPath()) || m_nextRoad;
				if (!hasNextDestination) {
					car->s = mLens + fmod(car->s, mLens); // 逆向车辆从末端开始循环
				}
			}
			float effective_s = car->m_needReverseDirection ? (mLens - car->s) : car->s;

			Vector3 centerPos = getPositionAtDistance(effective_s);
			Vector3 direction = getDirectionAtDistance(effective_s);

			// 🔧 如果车辆需要反向方向计算（反向连接），反转方向
			if (car->m_needReverseDirection) {
				direction = -direction;
			}

			Vector3 normal = getNormalAtDistance(car->s);

			Vector3 right = direction.crossProduct(normal);
			if (right.length() < 0.1f)
			{
				Vector3 up = Vector3::UNIT_Y;
				right = direction.crossProduct(up);
				if (right.length() < 0.1f)
				{
					up = Vector3::UNIT_Z;
					right = direction.crossProduct(up);
				}
			}
			right.normalise();

			// 根据车辆方向调整行驶方向
			Vector3 vehicleDirection = direction;
			if (car->laneDirection == Vehicle::LaneDirection::Backward) {
				vehicleDirection = -direction;
			}

			// 设置车辆的位置和方向
			Vector3 offsetVector = right * car->laneOffset;
			car->pos = centerPos + offsetVector;
			car->dir = vehicleDirection;

			// 设置车辆旋转矩阵
			Matrix3 mat;
			mat.SetColumn(0, vehicleDirection);
			mat.SetColumn(1, normal);
			mat.SetColumn(2, right);
			car->rot.FromRotationMatrix(mat);



			//car->updatePos();
		}

		// 执行车辆转移
		for (Vehicle* vehicle : vehiclesToTransfer)
		{
			// 从当前道路移除车辆
			auto it = std::find(mCars.begin(), mCars.end(), vehicle);
			if (it != mCars.end())
			{
				mCars.erase(it);
			}

			// 计算超出距离
			float overshoot = 0.0f;
			if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
				overshoot = vehicle->s - mLens;
			}
			else {
				// 逆向车辆从道路起点移动到末端
				overshoot = -vehicle->s; // s为负值，overshoot为正值
			}

			// 根据车辆路径决定下一条道路
			Road* nextRoad = nullptr;
			if (m_trafficManager && vehicle->hasPath()) {
				// 车辆有分配的路径
				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					// 正向车辆：移动到路径中的下一条道路
					if (vehicle->moveToNextRoad()) {
						uint16 nextRoadId = vehicle->getCurrentRoadId();
						nextRoad = m_trafficManager->getRoadById(nextRoadId);						// 关键修复：检查道路节点连接是否正确
						if (nextRoad) {
							uint16_t departureNodeId = vehicle->m_needReverseDirection ? getSourceNodeId() : getDestinationNodeId();
							uint16_t currentRoadTargetNode = departureNodeId; // 使用正确的出发节点进行连接判断

							uint16_t nextRoadSourceNode = nextRoad->getSourceNodeId();
							uint16_t nextRoadTargetNode = nextRoad->getDestinationNodeId();

							// 获取车辆在当前道路起点的初始坐标
							Vector3 currentInitialPos = getPositionAtDistance(0.0f);
							Vector3 vehicleCurrentPos = vehicle->pos;

							LogManager::instance()->logMessage("[VEHICLE_TRANSFER] Vehicle " + std::to_string(vehicle->id) +
								" | DIRECTION: FORWARD (target→source) | ANALYSIS:" +
								"\n  Current Road[" + std::to_string(getRoadId()) + "] (src:" + std::to_string(getSourceNodeId()) + "→tgt:" + std::to_string(currentRoadTargetNode) + ")" +
								"\n  Next Road[" + std::to_string(nextRoadId) + "] (src:" + std::to_string(nextRoadSourceNode) + "→tgt:" + std::to_string(nextRoadTargetNode) + ")" +
								"\n  Vehicle initial coords from road start: (" + std::to_string(currentInitialPos.x) + ", " + std::to_string(currentInitialPos.y) + ")" +
								"\n  Current position: (" + std::to_string(vehicleCurrentPos.x) + ", " + std::to_string(vehicleCurrentPos.y) + ")" +
								"\n  Distance along road: " + std::to_string(vehicle->s) + "/" + std::to_string(mLens) +
								"\n  Overshoot: " + std::to_string(overshoot));

							// 🎯 智能连接判断：优先选择正确的连接方式
							bool canConnectToSource = (currentRoadTargetNode == nextRoadSourceNode);
							bool canConnectToTarget = (currentRoadTargetNode == nextRoadTargetNode);
							if (canConnectToSource && !canConnectToTarget) {
								// 标准正向连接：当前道路终点 → 下一道路起点
								vehicle->s = std::max(0.0f, overshoot);
								vehicle->m_needReverseDirection = false; // 重置反向标记

								Vector3 nextRoadInitialPos = nextRoad->getPositionAtDistance(0.0f);
								LogManager::instance()->logMessage("[CONNECTION_SUCCESS] Vehicle " + std::to_string(vehicle->id) +
									" | TYPE: Standard Forward Connection (target→source)" +
									"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
									"\n  Connection Node: " + std::to_string(currentRoadTargetNode) +
									"\n  New Position: " + std::to_string(vehicle->s) + " (from start)" +
									"\n  Next road initial coords: (" + std::to_string(nextRoadInitialPos.x) + ", " + std::to_string(nextRoadInitialPos.y) + ")" +
									"\n  Direction: Normal forward movement");
							}
							else if (canConnectToTarget && !canConnectToSource) {
								// 反向连接：当前道路终点 → 下一道路终点（需要从终点向起点行驶）
								vehicle->s = std::max(0.0f, overshoot); // [核心修改] 车辆的逻辑位移仍从0开始
								vehicle->m_needReverseDirection = true;  // 标记需要反向方向计算

								Vector3 nextRoadEndPos = nextRoad->getPositionAtDistance(nextRoad->mLens);
								LogManager::instance()->logMessage("[CONNECTION_SUCCESS] Vehicle " + std::to_string(vehicle->id) +
									" | TYPE: Reverse Connection (target→target)" +
									"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
									"\n  Connection Node: " + std::to_string(currentRoadTargetNode) +
									"\n  New Position: " + std::to_string(vehicle->s) + " (from end, reverse direction)" +
									"\n  Next road end coords: (" + std::to_string(nextRoadEndPos.x) + ", " + std::to_string(nextRoadEndPos.y) + ")" +
									"\n  Direction: Reverse movement (end→start)");
							}
							else if (canConnectToSource && canConnectToTarget) {
								// 双重连接（可能是环路）：优先选择正向连接
								vehicle->s = std::max(0.0f, overshoot);

								Vector3 nextRoadStartPos = nextRoad->getPositionAtDistance(0.0f);
								LogManager::instance()->logMessage("[CONNECTION_SUCCESS] Vehicle " + std::to_string(vehicle->id) +
									" | TYPE: Dual Connection (both source&target match)" +
									"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
									"\n  Connection Node: " + std::to_string(currentRoadTargetNode) + " (choosing standard forward)" +
									"\n  New Position: " + std::to_string(vehicle->s) + " (from start)" +
									"\n  Next road start coords: (" + std::to_string(nextRoadStartPos.x) + ", " + std::to_string(nextRoadStartPos.y) + ")" +
									"\n  Direction: Standard forward (dual option resolved)");
							}
							else {
								// 无连接：这表明路径生成有问题，使用启发式方法
								vehicle->s = std::max(0.0f, overshoot);

								Vector3 nextRoadFallbackPos = nextRoad->getPositionAtDistance(0.0f);
								LogManager::instance()->logMessage("[CONNECTION_ERROR] Vehicle " + std::to_string(vehicle->id) +
									" | TYPE: No Valid Connection Found" +
									"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
									"\n  Current target node: " + std::to_string(currentRoadTargetNode) +
									"\n  Next road nodes: (src:" + std::to_string(nextRoadSourceNode) + ", tgt:" + std::to_string(nextRoadTargetNode) + ")" +
									"\n  Fallback position: " + std::to_string(vehicle->s) + " (using road start)" +
									"\n  Next road fallback coords: (" + std::to_string(nextRoadFallbackPos.x) + ", " + std::to_string(nextRoadFallbackPos.y) + ")" +
									"\n  ⚠️ WARNING: Path may need regeneration - node connectivity broken");
							}
						}

					}
				}
				else {
					// 逆向车辆：移动到路径中的上一条道路
					if (vehicle->moveToPreviousRoad()) {
						uint16 nextRoadId = vehicle->getCurrentRoadId();
						nextRoad = m_trafficManager->getRoadById(nextRoadId);						// 逆向车辆的节点连接检查
						if (nextRoad) {
							uint16 currentRoadSourceNode = getSourceNodeId();
							uint16 nextRoadTargetNode = nextRoad->getDestinationNodeId();
							uint16 nextRoadSourceNode = nextRoad->getSourceNodeId();

							// 获取车辆在当前道路起点的初始坐标信息
							Vector3 currentInitialPos = getPositionAtDistance(0.0f);
							Vector3 vehicleCurrentPos = vehicle->pos;

							LogManager::instance()->logMessage("[VEHICLE_TRANSFER] Vehicle " + std::to_string(vehicle->id) +
								" | DIRECTION: BACKWARD (source→target) | ANALYSIS:" +
								"\n  Current Road[" + std::to_string(getRoadId()) + "] (src:" + std::to_string(currentRoadSourceNode) + "→tgt:" + std::to_string(getDestinationNodeId()) + ")" +
								"\n  Next Road[" + std::to_string(nextRoadId) + "] (src:" + std::to_string(nextRoadSourceNode) + "→tgt:" + std::to_string(nextRoadTargetNode) + ")" +
								"\n  Vehicle initial coords from road start: (" + std::to_string(currentInitialPos.x) + ", " + std::to_string(currentInitialPos.y) + ")" +
								"\n  Current position: (" + std::to_string(vehicleCurrentPos.x) + ", " + std::to_string(vehicleCurrentPos.y) + ")" +
								"\n  Distance along road: " + std::to_string(vehicle->s) + "/" + std::to_string(mLens) +
								"\n  Overshoot: " + std::to_string(overshoot));							if (currentRoadSourceNode == nextRoadTargetNode) {
								// 正确连接：从新道路末端开始
								vehicle->s = nextRoad->mLens - std::max(0.0f, overshoot);

								Vector3 nextRoadEndPos = nextRoad->getPositionAtDistance(nextRoad->mLens);
								LogManager::instance()->logMessage("[CONNECTION_SUCCESS] Vehicle " + std::to_string(vehicle->id) +
									" | TYPE: Standard Backward Connection (source→target)" +
									"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
									"\n  Connection Node: " + std::to_string(currentRoadSourceNode) +
									"\n  New Position: " + std::to_string(vehicle->s) + " (from end)" +
									"\n  Next road end coords: (" + std::to_string(nextRoadEndPos.x) + ", " + std::to_string(nextRoadEndPos.y) + ")" +
									"\n  Direction: Backward movement (end→start)");
							}
								else {
								// 检查是否需要从起点进入
								if (currentRoadSourceNode == nextRoadSourceNode) {
									// 需要从下一道路的起点开始
									vehicle->s = std::max(0.0f, overshoot);

									Vector3 nextRoadStartPos = nextRoad->getPositionAtDistance(0.0f);
									LogManager::instance()->logMessage("[CONNECTION_SUCCESS] Vehicle " + std::to_string(vehicle->id) +
										" | TYPE: Backward-to-Source Connection (source→source)" +
										"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
										"\n  Connection Node: " + std::to_string(currentRoadSourceNode) +
										"\n  New Position: " + std::to_string(vehicle->s) + " (from start)" +
										"\n  Next road start coords: (" + std::to_string(nextRoadStartPos.x) + ", " + std::to_string(nextRoadStartPos.y) + ")" +
										"\n  Direction: Enter from start point");
								}
								else {
									// 无法找到正确连接，使用默认位置
									vehicle->s = nextRoad->mLens - std::max(0.0f, overshoot);

									Vector3 nextRoadFallbackPos = nextRoad->getPositionAtDistance(vehicle->s);
									LogManager::instance()->logMessage("[CONNECTION_ERROR] Vehicle " + std::to_string(vehicle->id) +
										" | TYPE: Backward Node Mismatch" +
										"\n  Route: Road[" + std::to_string(getRoadId()) + "] → Road[" + std::to_string(nextRoadId) + "]" +
										"\n  Current source node: " + std::to_string(currentRoadSourceNode) +
										"\n  Next road nodes: (src:" + std::to_string(nextRoadSourceNode) + ", tgt:" + std::to_string(nextRoadTargetNode) + ")" +
										"\n  Fallback position: " + std::to_string(vehicle->s) + " (using road end)" +
										"\n  Next road fallback coords: (" + std::to_string(nextRoadFallbackPos.x) + ", " + std::to_string(nextRoadFallbackPos.y) + ")" +
										"\n  ⚠️ WARNING: Node connectivity mismatch for backward vehicle");
								}
							}
						}
					}
				}				if (!nextRoad) {
					LogManager::instance()->logMessage("[PATH_END] Vehicle " + std::to_string(vehicle->id) +
						" | REASON: Reached end of assigned path" +
						"\n  Last Road: [" + std::to_string(getRoadId()) + "]" +
						"\n  Action: Using default next road as fallback" +
						"\n  Vehicle initial coords from road start: (" + std::to_string(getPositionAtDistance(0.0f).x) + ", " + std::to_string(getPositionAtDistance(0.0f).y) + ")");
					nextRoad = m_nextRoad; // 路径结束，使用默认下一条道路
				}
			}
			else {
				// 没有路径或Traffic引用，使用默认下一条道路
				nextRoad = m_nextRoad;

				// 获取车辆坐标信息用于调试
				Vector3 currentInitialPos = getPositionAtDistance(0.0f);
				Vector3 vehicleCurrentPos = vehicle->pos;

				LogManager::instance()->logMessage("[NO_PATH] Vehicle " + std::to_string(vehicle->id) +
					" | REASON: No assigned path or TrafficManager reference" +
					"\n  Current Road: [" + std::to_string(getRoadId()) + "]" +
					"\n  Vehicle initial coords from road start: (" + std::to_string(currentInitialPos.x) + ", " + std::to_string(currentInitialPos.y) + ")" +
					"\n  Current position: (" + std::to_string(vehicleCurrentPos.x) + ", " + std::to_string(vehicleCurrentPos.y) + ")" +
					"\n  Action: Using default next road");

				// 对于没有路径的车辆，设置默认位置
				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					vehicle->s = std::max(0.0f, overshoot); // 从新道路起点开始
				}
				else {
					if (nextRoad) {
						vehicle->s = nextRoad->mLens - std::max(0.0f, overshoot); // 从新道路末端开始
					}
				}
			}			// 添加到目标道路
			if (nextRoad) {
				nextRoad->addCar(vehicle);

				// 获取详细的转移结果信息
				Vector3 currentRoadStart = getPositionAtDistance(0.0f);
				Vector3 nextRoadStart = nextRoad->getPositionAtDistance(0.0f);
				Vector3 vehicleNewPos = nextRoad->getPositionAtDistance(vehicle->s);

				LogManager::instance()->logMessage("[TRANSFER_COMPLETE] Vehicle " + std::to_string(vehicle->id) +
					" | TRANSFER SUMMARY:" +
					"\n  From: Road[" + std::to_string(getRoadId()) + "] (start coords: " + std::to_string(currentRoadStart.x) + ", " + std::to_string(currentRoadStart.y) + ")" +
					"\n  To: Road[" + std::to_string(nextRoad->getRoadId()) + "] (start coords: " + std::to_string(nextRoadStart.x) + ", " + std::to_string(nextRoadStart.y) + ")" +
					"\n  Overshoot distance: " + std::to_string(overshoot) +
					"\n  New position on road: " + std::to_string(vehicle->s) + "/" + std::to_string(nextRoad->mLens) +
					"\n  New world coordinates: (" + std::to_string(vehicleNewPos.x) + ", " + std::to_string(vehicleNewPos.y) + ")" +
					"\n  Direction: " + (vehicle->laneDirection == Vehicle::LaneDirection::Forward ? "Forward" : "Backward") +
					"\n  Reverse flag: " + (vehicle->m_needReverseDirection ? "True" : "False"));
			}
			else {
				Vector3 currentRoadStart = getPositionAtDistance(0.0f);
				Vector3 vehicleCurrentPos = vehicle->pos;

				LogManager::instance()->logMessage("[TRANSFER_FAILED] Vehicle " + std::to_string(vehicle->id) +
					" | NO DESTINATION ROAD AVAILABLE" +
					"\n  Last Road: [" + std::to_string(getRoadId()) + "] (start coords: " + std::to_string(currentRoadStart.x) + ", " + std::to_string(currentRoadStart.y) + ")" +
					"\n  Vehicle position: (" + std::to_string(vehicleCurrentPos.x) + ", " + std::to_string(vehicleCurrentPos.y) + ")" +
					"\n  Overshoot: " + std::to_string(overshoot) +
					"\n  Action: Removing vehicle from simulation");
			}
		}


	}



	Vehicle::Vehicle(float initialSpeed, LaneDirection direction)
		: s(0.f), speed(initialSpeed > 0 ? initialSpeed : 1.0f), acc(0.f), pos(Vector3::ZERO), laneDirection(direction)
	{
		// 随机选择不同的车型（如果有多种可用）
		std::vector<std::string> carModels = {
			"actors/car_005.act"
			// 
			// "actors/car_001.act",
			// "actors/car_002.act",
			// "actors/truck_001.act"
		};

		int modelIndex = rand() % carModels.size();
		mCar = Echo::ActorSystem::instance()->getActiveActorManager()->createActor(Name(carModels[modelIndex]), "", false, false);
	}



	void Vehicle::update(float deltaTime)
	{
		s += speed * deltaTime;
	}

	void Vehicle::updatePos()
	{
		if (!mCar.Expired())
		{
			mCar->setPosition(pos);
			mCar->setRotation(rot);

		}
	}

	// Road连接相关方法实现
	void Road::setNextRoad(Road* nextRoad) { m_nextRoad = nextRoad; }

	//默认跟车模型参数设置
	void Traffic::initializeCarFollowingModels()
	{
		// 设置IDM默认参数
		m_idmParams.v0 = 10.0f;       // 期望速度 (m/s)
		m_idmParams.T = 1.0f;         // 期望时间间隔 (s)
		m_idmParams.s0 = 3.0f;        // 最小间距 (m)
		m_idmParams.a = 8.0f;         // 最大加速度 (m/s²)
		m_idmParams.b = 10.5f;         // 舒适减速度 (m/s²)
		m_idmParams.bmax = 20.0f;     // 最大减速度 (m/s²)
		m_idmParams.noiseLevel = 0.1f;

		// 设置ACC默认参数
		m_accParams.v0 = 10.0f;
		m_accParams.T = 1.0f;
		m_accParams.s0 = 3.0f;
		m_accParams.a = 8.0f;
		m_accParams.b = 10.5f;
		m_accParams.bmax = 20.0f;
		m_accParams.cool = 0.3f;
		m_accParams.noiseLevel = 0.1f;
	}

	std::unique_ptr<ICarFollowingModel> Traffic::createModelForVehicle(Vehicle::VehicleType type)
	{
		switch (m_defaultModelType) {
		case CarFollowingModelFactory::ModelType::IDM:
			return CarFollowingModelFactory::createIDM(m_idmParams);
		case CarFollowingModelFactory::ModelType::ACC:
			return CarFollowingModelFactory::createACC(m_accParams);
		default:
			return CarFollowingModelFactory::createIDM(m_idmParams);
		}
	}

	void Traffic::setDefaultCarFollowingModel(CarFollowingModelFactory::ModelType modelType)
	{
		m_defaultModelType = modelType;
	}

	void Traffic::updateGlobalModelParameters(const IDMModel::Parameters& idmParams, const ACCModel::Parameters& accParams)
	{
		m_idmParams = idmParams;
		m_accParams = accParams;

		// 更新所有现有车辆的模型参数
		for (Vehicle* vehicle : m_Vehicles) {
			if (vehicle->getCarFollowingModel()) {
				// 重新创建跟车模型以应用新参数
				auto newModel = createModelForVehicle(Vehicle::VehicleType::Car);
				newModel->setDriverVariation(vehicle->getDriverFactor());
				vehicle->setCarFollowingModel(std::move(newModel));
			}
		}
	}

	// 驾驶模式配置方法 =

	void Traffic::useConservativeDriving()
	{
		// 保守驾驶：更低速度，更大间距，更温和的加减速
		m_idmParams.v0 = 12.0f;
		m_idmParams.T = 1.8f;
		m_idmParams.s0 = 3.0f;
		m_idmParams.a = 1.5f;
		m_idmParams.b = 2.0f;
		m_idmParams.bmax = 15.0f;
		m_idmParams.noiseLevel = 0.02f;

		m_accParams.v0 = 12.0f;
		m_accParams.T = 1.8f;
		m_accParams.s0 = 3.0f;
		m_accParams.a = 1.5f;
		m_accParams.b = 2.0f;
		m_accParams.bmax = 8.0f;
		m_accParams.cool = 0.95f;
		m_accParams.noiseLevel = 0.02f;

		m_driverVariationCoeff = 0.08f; // 较小的变异系数

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Conservative driving mode activated");
	}

	void Traffic::useAggressiveDriving()
	{
		// 激进驾驶：更高速度，更小间距，更强烈的加减速
		m_idmParams.v0 = 20.0f;
		m_idmParams.T = 0.8f;
		m_idmParams.s0 = 1.5f;
		m_idmParams.a = 2.8f;
		m_idmParams.b = 1.2f;
		m_idmParams.bmax = 20.0f;
		m_idmParams.noiseLevel = 0.08f;

		m_accParams.v0 = 20.0f;
		m_accParams.T = 0.8f;
		m_accParams.s0 = 1.5f;
		m_accParams.a = 2.8f;
		m_accParams.b = 1.2f;
		m_accParams.bmax = 12.0f;
		m_accParams.cool = 0.85f;
		m_accParams.noiseLevel = 0.08f;

		m_driverVariationCoeff = 0.25f; // 较大的变异系数

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Aggressive driving mode activated");
	}

	void Traffic::useNormalDriving()
	{
		// 正常驾驶：使用默认参数
		initializeCarFollowingModels();
		m_driverVariationCoeff = 0.15f;

		// 更新所有现有车辆
		updateGlobalModelParameters(m_idmParams, m_accParams);

		LogManager::instance()->logMessage("Traffic: Normal driving mode activated");
	}

	void Traffic::setDriverVariationLevel(float level)
	{
		m_driverVariationCoeff = std::max(0.0f, std::min(1.0f, level));

		// 更新所有现有车辆的驾驶员变异
		for (Vehicle* vehicle : m_Vehicles) {
			vehicle->setDriverVariation(m_driverVariationCoeff);
		}

		LogManager::instance()->logMessage("Traffic: Driver variation level set to " + std::to_string(m_driverVariationCoeff));
	}

	// 车辆管理方法

	void Traffic::addMultipleVehicles(int numVehicles)
	{
		if (m_allRoads.empty() || numVehicles <= 0) return;

		const float baseSpeed = 3.0f;  // 基础速度 (m/s)
		const float laneOffset = 4.5f;  // 车道偏移
		const float minGap = 80.0f;     // 车辆之间的最小间距

		// 计算正向和逆向车辆数量（100%正向，暂时简化）
		int forwardVehicles = static_cast<int>(numVehicles);
		int backwardVehicles = 0; // 暂时不添加逆向车辆，简化问题

		LogManager::instance()->logMessage("Creating " + std::to_string(forwardVehicles) + " vehicles for distributed road assignment...");

		// 创建车辆但暂时不分配到特定道路
		// 之后在 assignPathsToAllVehicles 中会重新分配到正确的起始道路
		for (int i = 0; i < forwardVehicles; ++i) {

			float vehicleSpeed = baseSpeed + (rand() % 5 - 2) * 0.5;
			vehicleSpeed = std::max(1.0f, vehicleSpeed);

			Vehicle* newVehicle = new Vehicle(vehicleSpeed, Vehicle::LaneDirection::Forward);
			newVehicle->id = m_Vehicles.size() + 1;
			// 正向车辆使用正车道偏移（右侧车道）
			newVehicle->laneOffset = laneOffset + (rand() % 3 - 1) * 0.3f;
			// 临时设置位置，稍后会在路径分配时重新设置
			newVehicle->s = 0.0f;

			auto carFollowingModel = createModelForVehicle(Vehicle::VehicleType::Car);
			newVehicle->setCarFollowingModel(std::move(carFollowingModel));

			// 不立即添加到特定道路，而是先加入车辆列表
			// 将在 assignPathsToAllVehicles 中分配到正确的起始道路
			m_Vehicles.push_back(newVehicle);
		}



		// 添加逆向车辆
		/*for (int i = 0; i < backwardvehicles; ++i) {
			float vehiclespeed = basespeed + (rand() % 5 - 2);
			vehiclespeed = std::max(5.0f, vehiclespeed);

			vehicle* newvehicle = new vehicle(vehiclespeed, vehicle::lanedirection::backward);
			newvehicle->id = m_vehicles.size() + 1;
			 逆向车辆使用负车道偏移（左侧车道）- 更大的偏移确保隔离
			newvehicle->laneoffset = laneoffset + (rand() % 3 - 1) * 0.3f;
			 逆向车辆从道路末端开始
			newvehicle->s = m_roadmanager->mlens - (i * mingap);

			auto carfollowingmodel = createmodelforvehicle(vehicle::vehicletype::car);
			newvehicle->setcarfollowingmodel(std::move(carfollowingmodel));
			 设置驾驶员变异
			newvehicle->setdrivervariation(m_drivervariationcoeff);

			 添加到道路和车辆列表
			m_roadmanager->addcar(newvehicle);
			m_vehicles.push_back(newvehicle);
		}*/

		LogManager::instance()->logMessage("Traffic: Added " + std::to_string(numVehicles) + " vehicles. Total vehicles: " + std::to_string(m_Vehicles.size()));
	}
	//车辆密度
	void Traffic::setVehicleDensity(float vehiclesPerKm)
	{
		if (!m_roadManager || vehiclesPerKm <= 0) return;

		// 计算道路总长度（假设）
		float totalRoadLength = 1000.0f; // 1km，可以根据实际道路长度调整
		int targetVehicleCount = static_cast<int>(vehiclesPerKm * totalRoadLength / 1000.0f);

		// 清理现有车辆
		for (Vehicle* vehicle : m_Vehicles) {
			delete vehicle;
		}
		m_Vehicles.clear();

		// 重新创建车辆
		addMultipleVehicles(targetVehicleCount);

		LogManager::instance()->logMessage("Traffic: Set vehicle density to " + std::to_string(vehiclesPerKm) + " vehicles/km, created " + std::to_string(targetVehicleCount) + " vehicles");
	}

	// Road前方车辆


	void Road::sortVehicles()
	{
		// 
		// 注意：这个排序适用于所有车辆，前车查找逻辑会根据方向进行调整
		std::sort(mCars.begin(), mCars.end(), [](Vehicle* a, Vehicle* b) {
			return a->s > b->s;
			});
	}

	void Road::updateEnvironment()
	{
		if (mCars.empty()) return;

		// 首先对车辆按位置排序
		sortVehicles();

		// 为每个车辆更新环境索引
		for (int i = 0; i < static_cast<int>(mCars.size()); ++i) {
			updateLeadIndex(i);
			updateLagIndex(i);
		}
	}

	void Road::updateLeadIndex(int i)
	{
		int n = static_cast<int>(mCars.size());
		if (n == 0 || i < 0 || i >= n) return;

		Vehicle* currentVehicle = mCars[i];
		currentVehicle->iLead = -1; // 默认无前车

		// 非环形道路：根据车辆方向查找前车
		if (currentVehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：前车是位置更大的同方向车辆（在排序数组中的更前位置）
			for (int j = i - 1; j >= 0; j--) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Forward &&
					mCars[j]->s > currentVehicle->s) {
					currentVehicle->iLead = j;
					break;
				}
			}
		}
		else {
			// 逆向车辆：前车是位置更小的同方向车辆（在排序数组中的更后位置）
			for (int j = i + 1; j < n; j++) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Backward &&
					mCars[j]->s < currentVehicle->s) {
					currentVehicle->iLead = j;
					break;
				}
			}
		}
	}

	void Road::updateLagIndex(int i)
	{
		int n = static_cast<int>(mCars.size());
		if (n == 0 || i < 0 || i >= n) return;

		Vehicle* currentVehicle = mCars[i];
		if (currentVehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：后车是位置更小的同方向车辆（在排序数组中的更后位置）
			for (int j = i + 1; j < n; j++) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Forward &&
					mCars[j]->s < currentVehicle->s) {
					currentVehicle->iLag = j;
					break;
				}
			}
		}
		else {
			// 逆向车辆：后车是位置更大的同方向车辆（在排序数组中的更前位置）
			for (int j = i - 1; j >= 0; j--) {
				if (mCars[j]->laneDirection == Vehicle::LaneDirection::Backward &&
					mCars[j]->s > currentVehicle->s) {
					currentVehicle->iLag = j;
					break;
				}
			}
		}
	}

	Vehicle* Road::getVehicleByIndex(int index) const
	{
		if (index >= 0 && index < static_cast<int>(mCars.size())) {
			return mCars[index];
		}
		return nullptr;
	}

	// 使用索引系统的新findLeadingVehicle实现
	Vehicle* Road::findLeadingVehicle(const Vehicle* vehicle) const
	{
		if (!vehicle || vehicle->iLead < 0) return nullptr;

		Vehicle* leadingVehicle = getVehicleByIndex(vehicle->iLead);

		// 验证前车确实在前方且同方向
		if (leadingVehicle && leadingVehicle->laneDirection == vehicle->laneDirection) {
			// 非环形道路：简单位置比较
			bool isAhead = false;
			if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
				isAhead = (leadingVehicle->s > vehicle->s);
			}
			else {
				isAhead = (leadingVehicle->s < vehicle->s);
			}

			if (isAhead) {
				return leadingVehicle;
			}
		}

		return nullptr;
	}
	//车辆间隔 - 距离计算
	float Road::calculateGapToLeadingVehicle(const Vehicle* vehicle) const
	{
		if (!vehicle || vehicle->iLead < 0) {
			return 1000.0f; // 无前车时返回大值（自由流）
		}

		Vehicle* leadingVehicle = getVehicleByIndex(vehicle->iLead);
		if (!leadingVehicle) {
			return 1000.0f;
		}

		// 非环形道路：直接计算间距
		float gap = 0.0f;
		if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
			// 正向车辆：前车位置 - 前车长度 - 当前车辆位置
			gap = leadingVehicle->s - leadingVehicle->m_length - vehicle->s;
		}
		else {
			// 逆向车辆：当前车辆位置 - 当前车辆长度 - 前车位置
			gap = vehicle->s - vehicle->m_length - leadingVehicle->s;
		}

		return std::max(0.0f, gap); // 确保间距非负
	}

	//  跟车模型

	void Vehicle::setCarFollowingModel(std::unique_ptr<ICarFollowingModel> model)
	{
		m_carFollowingModel = std::move(model);
		if (m_carFollowingModel) {
			m_carFollowingModel->setDriverVariation(m_driverFactor);
		}
	}

	void Vehicle::setDriverVariation(float variationCoeff)
	{
		m_driverVariationCoeff = variationCoeff;
		// 计算驾驶员因子：使用均匀分布，变异系数转换为标准差
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

		m_driverFactor = 1.0f + std::sqrt(12.0f) * variationCoeff * dist(gen);
		m_driverFactor = std::max(0.1f, std::min(2.0f, m_driverFactor)); // 限制在合理范围内

		if (m_carFollowingModel) {
			m_carFollowingModel->setDriverVariation(m_driverFactor);
		}
	}

	void Vehicle::applyCarFollowing(float deltaTime, const Vehicle* leadingVehicle, float gap)
	{
		if (!m_carFollowingModel || !leadingVehicle) {
			return;
		}

		// 使用跟车模型计算加速度
		float newAcceleration = m_carFollowingModel->calculateAcceleration(
			gap, speed, leadingVehicle->speed, leadingVehicle->acc);
		// 限制加速度
		newAcceleration = std::max(-15.0f, std::min(8.0f, newAcceleration));
		// 更新加速度
		acc = newAcceleration;
		//// 如果速度过低且间距足够，给一个最小速度以避免完全停止
		//if (speed < 0.5f && gap > 10.0f) {
		//	speed = 0.5f; // 最小移动速度
		//}
		// 更新速度（确保不为负）
		speed = std::max(0.0f, speed + acc * deltaTime);

		// 更新位置
		if (laneDirection == LaneDirection::Forward) {
			s += speed * deltaTime;
		}
		else {
			s -= speed * deltaTime;
		}
	}
	//城市路径生成
	void Traffic::generateAllPaths()
	{
		if (m_pathsGenerated) return;

		m_allPaths.clear();
		LogManager::instance()->logMessage("=== 开始基于节点网络生成路径 ===");
		LogManager::instance()->logMessage("道路总数: " + std::to_string(m_allRoads.size()) + ", 节点总数: " + std::to_string(m_highwayNodes.size()));

		// 构建节点邻接表（节点ID -> 连接的节点ID列表）
		std::map<uint16, std::vector<uint16>> nodeAdjacencyMap;
		std::map<std::pair<uint16, uint16>, uint16> nodeToRoadMap; // (源节点,目标节点) -> 道路ID

		// 遍历所有道路，构建节点连接图
		for (Road* road : m_allRoads) {
			if (road && road->isHighwayRoad()) {
				uint16 sourceNode = road->getSourceNodeId();
				uint16 targetNode = road->getDestinationNodeId();
				uint16 roadId = road->getRoadId();

				// 构建双向邻接表
				nodeAdjacencyMap[sourceNode].push_back(targetNode);
				nodeAdjacencyMap[targetNode].push_back(sourceNode);

				// 记录节点对到道路的映射
				nodeToRoadMap[{sourceNode, targetNode}] = roadId;
				nodeToRoadMap[{targetNode, sourceNode}] = roadId;

				LogManager::instance()->logMessage("添加节点连接: " + std::to_string(sourceNode) + " <-> " + std::to_string(targetNode) + " (Road " + std::to_string(roadId) + ")");
			}
		}		LogManager::instance()->logMessage("节点邻接图构建完成，连接节点数: " + std::to_string(nodeAdjacencyMap.size()));

		// 动态计算网络特征参数
		int totalNodes = static_cast<int>(nodeAdjacencyMap.size());
		int totalRoads = static_cast<int>(m_allRoads.size());

		// 根据网络规模动态调整参数
		int maxPathsToGenerate = std::min(200, std::max(20, totalRoads / 5)); // 基于道路数量调整
		int minPathLength = std::max(3, totalNodes / 20); // 最小路径长度：网络节点数的5%
		int maxSearchDepth = std::min(100, totalNodes); // 最大搜索深度：不超过总节点数
		int targetPathLength = std::max(10, totalNodes / 3); // 目标路径长度：网络节点数的1/3

		LogManager::instance()->logMessage("动态参数设置 - 最大路径数: " + std::to_string(maxPathsToGenerate) +
			", 最小长度: " + std::to_string(minPathLength) +
			", 目标长度: " + std::to_string(targetPathLength) +
			", 最大深度: " + std::to_string(maxSearchDepth));

		// 自适应起点选择策略
		std::set<std::vector<uint16>> allNodePaths;
		std::vector<std::pair<uint16, int>> candidateStartNodes;

		// 统计度数分布
		std::map<int, int> degreeDistribution;
		for (const auto& entry : nodeAdjacencyMap) {
			int degree = static_cast<int>(entry.second.size());
			degreeDistribution[degree]++;
		}
		// 🔧 增强起点选择策略：确保最大起点多样性
		for (const auto& entry : nodeAdjacencyMap) {
			int degree = static_cast<int>(entry.second.size());
			bool shouldInclude = false;

			if (totalNodes < 50) {
				// 小网络：包含更多节点作为候选起点
				shouldInclude = (degree <= 4);
			}
			else if (totalNodes < 200) {
				// 中等网络：包含度数≤5的节点，以及一些度数为6-7的节点
				shouldInclude = (degree <= 5) || (degree <= 7 && candidateStartNodes.size() < maxPathsToGenerate);
			}
			else {
				// 大网络：更加宽松的选择策略，确保足够的起点多样性
				shouldInclude = (degree <= 6) || (degree <= 10 && candidateStartNodes.size() < maxPathsToGenerate);
			}

			if (shouldInclude) {
				candidateStartNodes.push_back({ entry.first, degree });
			}
		}

		// 🔧 如果候选起点过少，则包含更多节点
		if (candidateStartNodes.size() < std::max(5, totalNodes / 10)) {
			LogManager::instance()->logMessage("起点候选过少，扩展选择范围");
			candidateStartNodes.clear();

			// 重新选择，包含所有度数≤平均度数的节点
			int totalDegree = 0;
			for (const auto& entry : nodeAdjacencyMap) {
				totalDegree += static_cast<int>(entry.second.size());
			}
			int avgDegree = totalDegree / totalNodes;

			for (const auto& entry : nodeAdjacencyMap) {
				int degree = static_cast<int>(entry.second.size());
				if (degree <= avgDegree + 1) { // 允许比平均度数稍高的节点
					candidateStartNodes.push_back({ entry.first, degree });
				}
			}
		}

		// 按度数排序，优先使用低度数节点，但确保起点多样性
		std::sort(candidateStartNodes.begin(), candidateStartNodes.end(),
			[](const auto& a, const auto& b) { return a.second < b.second; });

		LogManager::instance()->logMessage("自适应选择起点节点: " + std::to_string(candidateStartNodes.size()) + " 个");
		for (const auto& [degree, count] : degreeDistribution) {
			LogManager::instance()->logMessage("度数 " + std::to_string(degree) + ": " + std::to_string(count) + " 个节点");
		}

		// 🔧 关键修复：限制每个起点的路径数量，确保多样性
		std::map<uint16, int> pathsPerStartNode;
		const int maxPathsPerStartNode = 2; // 每个起点最多生成2条路径

		LogManager::instance()->logMessage("开始多样化路径生成策略");
		LogManager::instance()->logMessage("候选起点数: " + std::to_string(candidateStartNodes.size()));

		// 🔧 新策略：轮流为每个起点生成路径，而不是一次性完成一个起点
		for (int round = 0; round < maxPathsPerStartNode && allNodePaths.size() < maxPathsToGenerate; ++round) {
			LogManager::instance()->logMessage("第 " + std::to_string(round + 1) + " 轮路径生成");

			for (size_t i = 0; i < candidateStartNodes.size() && allNodePaths.size() < maxPathsToGenerate; ++i) {
				uint16 startNode = candidateStartNodes[i].first;

				// 检查该起点是否已经达到最大路径数
				if (pathsPerStartNode[startNode] >= maxPathsPerStartNode) {
					continue;
				}

				LogManager::instance()->logMessage("  为起点Node[" + std::to_string(startNode) +
					"] 生成第 " + std::to_string(pathsPerStartNode[startNode] + 1) + " 条路径...");

				// 为当前起点生成一条路径
				std::function<void(uint16, std::vector<uint16>&, std::set<uint16>&, int, bool&)> generateSinglePathDFS =
					[&](uint16 currentNode, std::vector<uint16>& path, std::set<uint16>& visited, int depth, bool& pathFound) {
					if (pathFound || depth > maxSearchDepth) return;

					// 🔧 更灵活的路径保存条件
					if (path.size() >= minPathLength) {
						bool shouldSave = false;

						// 条件1：达到目标长度
						if (path.size() >= targetPathLength) {
							shouldSave = true;
						}
						// 条件2：到达了不同的起点候选节点
						else if (path.size() >= targetPathLength / 2) {
							auto nodeIt = std::find_if(candidateStartNodes.begin(), candidateStartNodes.end(),
								[currentNode](const auto& p) { return p.first == currentNode; });
							if (nodeIt != candidateStartNodes.end() && nodeIt->first != startNode) {
								shouldSave = true;
							}
						}
						// 条件3：对于连通性较差的起点，降低要求
						else if (path.size() >= std::max(minPathLength, targetPathLength / 3)) {
							int startDegree = static_cast<int>(nodeAdjacencyMap[startNode].size());
							if (startDegree <= 2) { // 端点节点
								shouldSave = true;
							}
						}

						if (shouldSave) {
							allNodePaths.insert(path);
							pathsPerStartNode[startNode]++;
							pathFound = true;
							LogManager::instance()->logMessage("    起点Node[" + std::to_string(startNode) +
								"] 成功生成路径 (长度: " + std::to_string(path.size()) + " 节点)");
							return;
						}
					}

					// 继续DFS搜索
					auto adjIt = nodeAdjacencyMap.find(currentNode);
					if (adjIt != nodeAdjacencyMap.end()) {
						for (uint16 nextNode : adjIt->second) {
							if (visited.find(nextNode) == visited.end()) {
								path.push_back(nextNode);
								visited.insert(nextNode);
								generateSinglePathDFS(nextNode, path, visited, depth + 1, pathFound);
								if (pathFound) return; // 找到路径后立即返回
								path.pop_back();
								visited.erase(nextNode);
							}
						}
					}
				};

				// 为当前起点生成路径
				std::vector<uint16> initialPath = { startNode };
				std::set<uint16> initialVisited = { startNode };
				bool pathFound = false;
				generateSinglePathDFS(startNode, initialPath, initialVisited, 0, pathFound);

				if (!pathFound) {
					LogManager::instance()->logMessage("    起点Node[" + std::to_string(startNode) + "] 未能生成路径");
				}
			}
		}
		LogManager::instance()->logMessage("多样化路径生成完成，生成了 " + std::to_string(allNodePaths.size()) + " 条路径");
		LogManager::instance()->logMessage("生成长距离路径数: " + std::to_string(allNodePaths.size()));

		// 🔍 分析路径起点多样性
		std::map<uint16, int> startNodePathCount;
		for (const auto& nodePath : allNodePaths) {
			if (!nodePath.empty()) {
				startNodePathCount[nodePath[0]]++;
			}
		}

		LogManager::instance()->logMessage("=== 路径起点多样性分析 ===");
		LogManager::instance()->logMessage("不同起点节点数: " + std::to_string(startNodePathCount.size()));
		for (const auto& [nodeId, count] : startNodePathCount) {
			LogManager::instance()->logMessage("起点Node[" + std::to_string(nodeId) + "] 生成了 " + std::to_string(count) + " 条路径");
		}		// 分析生成的路径长度分布
		std::map<int, int> pathLengthDistribution;
		int maxNodePathLength = 0;
		int minNodePathLength = 999;
		for (const auto& nodePath : allNodePaths) {
			int pathLength = static_cast<int>(nodePath.size());
			pathLengthDistribution[pathLength]++;
			maxNodePathLength = std::max(maxNodePathLength, pathLength);
			minNodePathLength = std::min(minNodePathLength, pathLength);
		}

		LogManager::instance()->logMessage("路径长度分析 - 最短: " + std::to_string(minNodePathLength) +
			" 节点, 最长: " + std::to_string(maxNodePathLength) + " 节点");
		for (const auto& [length, count] : pathLengthDistribution) {
			LogManager::instance()->logMessage("长度 " + std::to_string(length) + " 节点: " + std::to_string(count) + " 条路径");
		}
		// 将节点路径转换为道路路径
		for (const auto& nodePath : allNodePaths) {
			if (nodePath.size() < 2) continue; // 至少需要两个节点形成道路

			std::vector<uint16> roadPath;
			bool validPath = true;

			// 将相邻节点对转换为道路ID
			for (size_t i = 0; i < nodePath.size() - 1; ++i) {
				uint16 fromNode = nodePath[i];
				uint16 toNode = nodePath[i + 1];

				auto roadIt = nodeToRoadMap.find({ fromNode, toNode });
				if (roadIt != nodeToRoadMap.end()) {
					roadPath.push_back(roadIt->second);
				}
				else {
					// ⚠️ 无法找到对应道路，标记为无效路径
					LogManager::instance()->logMessage("警告: 无法找到节点 " + std::to_string(fromNode) + " -> " + std::to_string(toNode) + " 对应的道路");
					validPath = false;
					break;
				}
			}

			if (validPath && !roadPath.empty()) {
				// 🔍 添加起点道路分析，确保路径起点多样化
				uint16 startRoad = roadPath[0];
				LogManager::instance()->logMessage("转换节点路径为道路路径: [" +
					std::to_string(nodePath[0]) + "->" + std::to_string(nodePath[1]) +
					"] = Road[" + std::to_string(startRoad) + "] (长度:" + std::to_string(roadPath.size()) + "段)");
				m_allPaths.push_back(roadPath);
			}
		}
		LogManager::instance()->logMessage("=== 自适应路径生成完成 ===");
		LogManager::instance()->logMessage("有效完整路径总数: " + std::to_string(m_allPaths.size()));

		// 分析生成的道路路径长度分布
		std::map<int, int> roadPathLengthDistribution;
		int maxRoadPathLength = 0;
		int minRoadPathLength = 999;
		for (const auto& roadPath : m_allPaths) {
			int pathLength = static_cast<int>(roadPath.size());
			roadPathLengthDistribution[pathLength]++;
			maxRoadPathLength = std::max(maxRoadPathLength, pathLength);
			minRoadPathLength = std::min(minRoadPathLength, pathLength);
		}

		LogManager::instance()->logMessage("道路路径长度分析 - 最短: " + std::to_string(minRoadPathLength) +
			" 段, 最长: " + std::to_string(maxRoadPathLength) + " 段");
		for (const auto& [length, count] : roadPathLengthDistribution) {
			LogManager::instance()->logMessage("长度 " + std::to_string(length) + " 段: " + std::to_string(count) + " 条路径");
		}

		// 智能输出路径样本（显示不同长度的代表性路径）
		std::vector<size_t> sampleIndices;

		// 选择短、中、长路径的代表样本
		for (size_t i = 0; i < m_allPaths.size(); ++i) {
			int pathLength = static_cast<int>(m_allPaths[i].size());

			// 选择代表性路径：最短的、中等长度的、最长的
			if (pathLength == minRoadPathLength ||
				pathLength == maxRoadPathLength ||
				(pathLength >= (minRoadPathLength + maxRoadPathLength) / 2 - 2 &&
					pathLength <= (minRoadPathLength + maxRoadPathLength) / 2 + 2)) {
				sampleIndices.push_back(i);
				if (sampleIndices.size() >= 10) break; // 限制输出数量
			}
		}
		LogManager::instance()->logMessage("=== 代表性路径样本 ===");
		for (size_t idx : sampleIndices) {
			std::string pathStr = "AdaptivePath[" + std::to_string(idx) + "] (" + std::to_string(m_allPaths[idx].size()) + "段): ";
			for (size_t j = 0; j < m_allPaths[idx].size(); ++j) {
				pathStr += "Road[" + std::to_string(m_allPaths[idx][j]) + "]";
				if (j < m_allPaths[idx].size() - 1) pathStr += "->";
			}
			LogManager::instance()->logMessage(pathStr);
		}

		// 🔧 关键新增：路径连续性验证和修复
		LogManager::instance()->logMessage("=== 开始路径连续性验证和修复 ===");
		std::vector<std::vector<uint16>> validatedPaths;
		int fixedPathCount = 0;
		int removedPathCount = 0;

		for (size_t pathIdx = 0; pathIdx < m_allPaths.size(); ++pathIdx) {
			const auto& originalPath = m_allPaths[pathIdx];
			if (originalPath.size() < 2) continue;

			std::vector<uint16> fixedPath;
			bool pathIsValid = true;
			bool pathWasFixed = false;

			// 验证路径中每相邻两条道路的连接性
			for (size_t i = 0; i < originalPath.size() - 1; ++i) {
				uint16 currentRoadId = originalPath[i];
				uint16 nextRoadId = originalPath[i + 1];

				Road* currentRoad = getRoadById(currentRoadId);
				Road* nextRoad = getRoadById(nextRoadId);

				if (!currentRoad || !nextRoad) {
					LogManager::instance()->logMessage("❌ Path[" + std::to_string(pathIdx) + "] contains invalid road IDs");
					pathIsValid = false;
					break;
				}

				// 检查连接性
				uint16 currentTarget = currentRoad->getDestinationNodeId();
				uint16 currentSource = currentRoad->getSourceNodeId();
				uint16 nextSource = nextRoad->getSourceNodeId();
				uint16 nextTarget = nextRoad->getDestinationNodeId();

				bool standardConnection = (currentTarget == nextSource);
				bool reverseConnection = (currentTarget == nextTarget);
				bool backwardConnection = (currentSource == nextTarget);
				bool selfConnection = (currentSource == nextSource);

				if (standardConnection) {
					// 标准正向连接，保持原路径
					fixedPath.push_back(currentRoadId);
					if (i == originalPath.size() - 2) fixedPath.push_back(nextRoadId);
				}
				else if (reverseConnection || backwardConnection || selfConnection) {
					// 发现问题连接，尝试修复
					LogManager::instance()->logMessage("🔧 Path[" + std::to_string(pathIdx) + "] fixing connection: Road[" +
						std::to_string(currentRoadId) + "] → Road[" + std::to_string(nextRoadId) + "]");
					LogManager::instance()->logMessage("   Current road: source=" + std::to_string(currentSource) +
						", target=" + std::to_string(currentTarget));
					LogManager::instance()->logMessage("   Next road: source=" + std::to_string(nextSource) +
						", target=" + std::to_string(nextTarget));

					// 尝试修复：保留当前道路，但标记需要特殊处理
					fixedPath.push_back(currentRoadId);
					if (i == originalPath.size() - 2) fixedPath.push_back(nextRoadId);
					pathWasFixed = true;
				}
				else {
					// 无法连接，移除此路径
					LogManager::instance()->logMessage("❌ Path[" + std::to_string(pathIdx) + "] has disconnected roads: Road[" +
						std::to_string(currentRoadId) + "] (target:" + std::to_string(currentTarget) +
						") → Road[" + std::to_string(nextRoadId) + "] (source:" + std::to_string(nextSource) +
						", target:" + std::to_string(nextTarget) + ")");
					pathIsValid = false;
					break;
				}
			}

			if (pathIsValid && !fixedPath.empty()) {
				validatedPaths.push_back(fixedPath);
				if (pathWasFixed) {
					fixedPathCount++;
					LogManager::instance()->logMessage("✅ Path[" + std::to_string(pathIdx) + "] was fixed and validated");
				}
			}
			else {
				removedPathCount++;
				LogManager::instance()->logMessage("🗑️ Path[" + std::to_string(pathIdx) + "] was removed due to invalid connections");
			}
		}

		// 更新路径列表
		m_allPaths = validatedPaths;

		LogManager::instance()->logMessage("=== 路径验证完成 ===");
		LogManager::instance()->logMessage("原始路径数: " + std::to_string(m_allPaths.size() + removedPathCount));
		LogManager::instance()->logMessage("修复路径数: " + std::to_string(fixedPathCount));
		LogManager::instance()->logMessage("移除路径数: " + std::to_string(removedPathCount));
		LogManager::instance()->logMessage("最终有效路径数: " + std::to_string(m_allPaths.size()));

		// 输出验证后的路径样本
		if (!m_allPaths.empty()) {
			LogManager::instance()->logMessage("=== 验证后的路径样本 ===");
			size_t sampleCount = std::min(static_cast<size_t>(5), m_allPaths.size());
			for (size_t i = 0; i < sampleCount; ++i) {
				const auto& path = m_allPaths[i];
				std::string pathStr = "ValidatedPath[" + std::to_string(i) + "] (" + std::to_string(path.size()) + "段): ";
				for (size_t j = 0; j < path.size(); ++j) {
					Road* road = getRoadById(path[j]);
					if (road) {
						pathStr += "Road[" + std::to_string(path[j]) + "](";
						pathStr += std::to_string(road->getSourceNodeId()) + "→" + std::to_string(road->getDestinationNodeId()) + ")";
					}
					else {
						pathStr += "Road[" + std::to_string(path[j]) + "](INVALID)";
					}
					if (j < path.size() - 1) pathStr += " → ";
				}
				LogManager::instance()->logMessage(pathStr);
			}
		}

		m_pathsGenerated = true;
	}

	void Traffic::generateNodePathsBFS(uint16 startNodeId,
		const std::map<uint16, std::vector<uint16>>& nodeAdjacencyMap,
		std::set<std::vector<uint16>>& allNodePaths, int maxPathLength)
	{
		// BFS队列：存储 (当前路径, 当前节点)
		std::queue<std::pair<std::vector<uint16>, uint16>> bfsQueue;

		// 初始状态：从起始节点开始
		std::vector<uint16> initialPath = { startNodeId };
		bfsQueue.push({ initialPath, startNodeId });

		while (!bfsQueue.empty()) {
			auto current = bfsQueue.front();
			bfsQueue.pop();

			std::vector<uint16> currentPath = current.first;
			uint16 currentNodeId = current.second;

			// 如果路径长度达到最大值，停止扩展
			if (currentPath.size() >= maxPathLength) {
				if (currentPath.size() >= 2) {
					allNodePaths.insert(currentPath);
				}
				continue;
			}

			// 添加当前路径（如果长度 >= 2）
			if (currentPath.size() >= 2) {
				allNodePaths.insert(currentPath);
			}

			// 获取当前节点的所有邻接节点
			auto adjacencyIt = nodeAdjacencyMap.find(currentNodeId);
			if (adjacencyIt != nodeAdjacencyMap.end()) {
				for (uint16 nextNodeId : adjacencyIt->second) {
					// 避免环路：检查节点是否已经在当前路径中
					bool alreadyInPath = false;
					for (uint16 pathNodeId : currentPath) {
						if (pathNodeId == nextNodeId) {
							alreadyInPath = true;
							break;
						}
					}

					if (!alreadyInPath) {
						// 创建新路径
						std::vector<uint16> newPath = currentPath;
						newPath.push_back(nextNodeId);
						bfsQueue.push({ newPath, nextNodeId });
					}
				}
			}
		}
	}
	void Traffic::generatePathsRecursive(Road* currentRoad, std::vector<uint16>& currentPath,
		std::set<uint16>& visited, std::set<std::vector<uint16>>& uniquePaths, int maxDepth)
	{
		if (maxDepth <= 0) return;

		Road* nextRoad = currentRoad->getNextRoad();
		if (nextRoad && visited.find(nextRoad->getRoadId()) == visited.end()) {
			// 添加到当前路径
			currentPath.push_back(nextRoad->getRoadId());
			visited.insert(nextRoad->getRoadId());
			uniquePaths.insert(currentPath);

			// 递归继续
			generatePathsRecursive(nextRoad, currentPath, visited, uniquePaths, maxDepth - 1);

			// 回溯
			currentPath.pop_back();
			visited.erase(nextRoad->getRoadId());
		}

		// 尝试从当前道路的目标城市出发的其他道路

			// 尝试从当前道路的目标节点出发的其他道路
		uint16 destNodeId = currentRoad->getDestinationNodeId();
		if (destNodeId != 0)
		{
			auto it = m_nodeConnections.find(destNodeId);
			if (it != m_nodeConnections.end())
			{
				for (Road* candidateRoad : it->second)
				{
					if (candidateRoad->getSourceNodeId() == destNodeId &&
						visited.find(candidateRoad->getRoadId()) == visited.end())
					{

						currentPath.push_back(candidateRoad->getRoadId());
						visited.insert(candidateRoad->getRoadId());
						uniquePaths.insert(currentPath);

						generatePathsRecursive(candidateRoad, currentPath, visited, uniquePaths, maxDepth - 1);

						currentPath.pop_back();
						visited.erase(candidateRoad->getRoadId());
					}
				}
			}
		}
	}

	void Traffic::assignPathToVehicle(Vehicle* vehicle, int pathIndex)
	{
		if (!vehicle || pathIndex < 0 || pathIndex >= m_allPaths.size()) return;

		const auto& path = m_allPaths[pathIndex];
		if (path.empty()) return;

		float originalPosition = vehicle->s;

		// 验证车辆当前道路与路径起始道路是否匹配
		uint16 currentRoadId = 0;
		Road* currentRoad = nullptr;

		for (Road* road : m_allRoads) {
			// 检查车辆是否在这条道路上
			auto& cars = road->mCars;
			if (std::find(cars.begin(), cars.end(), vehicle) != cars.end()) {
				currentRoadId = road->getRoadId();
				currentRoad = road;
				break;
			}
		}

		// 将车辆添加到路径起始道路（无论当前在哪里）
		Road* startRoad = getRoadById(path[0]);
		if (startRoad) {
			// 如果车辆已经在其他道路上，先移除
			if (currentRoad && currentRoadId != path[0]) {
				auto& cars = currentRoad->mCars;
				auto it = std::find(cars.begin(), cars.end(), vehicle);
				if (it != cars.end()) {
					cars.erase(it);
				}
			}

			// 将车辆添加到路径起始道路（如果不在的话）
			if (currentRoadId != path[0]) {
				vehicle->s = originalPosition; // 保持设置的位置
				startRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Vehicle " + std::to_string(vehicle->id) +
					" moved to start road " + std::to_string(path[0]) + " at position " + std::to_string(vehicle->s));
			}
		}

		vehicle->setPath(path);
		LogManager::instance()->logMessage("Vehicle[" + std::to_string(vehicle->id) + "] 分配路径[" + std::to_string(pathIndex) + "] 起始道路:" + std::to_string(path[0]) + " 位置:" + std::to_string(vehicle->s));
	}	//反向车辆分配方法   7.9间距位置
	void Traffic::assignBackwardPathToVehicle(Vehicle* vehicle, int pathIndex)
	{
		if (!vehicle || pathIndex < 0 || pathIndex >= m_allPaths.size()) return;

		const auto& forwardPath = m_allPaths[pathIndex];
		if (forwardPath.empty()) return;

		// 创建反向路径（颠倒原路径）
		std::vector<uint16> backwardPath;
		for (auto it = forwardPath.rbegin(); it != forwardPath.rend(); ++it) {
			backwardPath.push_back(*it);
		}

		// 验证车辆当前道路与反向路径起始道路是否匹配
		uint16 currentRoadId = 0;
		Road* currentRoad = nullptr;

		for (Road* road : m_allRoads) {
			auto& cars = road->mCars;
			if (std::find(cars.begin(), cars.end(), vehicle) != cars.end()) {
				currentRoadId = road->getRoadId();
				currentRoad = road;
				break;
			}
		}

		// 将车辆添加到反向路径起始道路
		Road* startRoad = getRoadById(backwardPath[0]);
		if (startRoad) {
			// 如果车辆已经在其他道路上，先移除
			if (currentRoad && currentRoadId != backwardPath[0]) {
				auto& cars = currentRoad->mCars;
				auto it = std::find(cars.begin(), cars.end(), vehicle);
				if (it != cars.end()) {
					cars.erase(it);
				}
			}

			// 将车辆添加到反向路径起始道路（如果不在的话）
			if (currentRoadId != backwardPath[0]) {
				vehicle->s = startRoad->getRoadLength(); // 逆向车辆从道路末端开始
				startRoad->addCar(vehicle);
				LogManager::instance()->logMessage("Backward Vehicle " + std::to_string(vehicle->id) +
					" moved to start road " + std::to_string(backwardPath[0]) + " for backward path assignment");
			}
		}

		vehicle->setPath(backwardPath);
		LogManager::instance()->logMessage("Backward Vehicle[" + std::to_string(vehicle->id) + "] 分配反向路径[" + std::to_string(pathIndex) + "] 起始道路:" + std::to_string(backwardPath[0]));

	}
	//路径分配


	void Traffic::assignPathsToAllVehicles()
	{
		if (m_allPaths.empty()) {
			LogManager::instance()->logMessage("No paths available for vehicle assignment.");
			return;
		}

		LogManager::instance()->logMessage("Starting adaptive path assignment for " + std::to_string(m_Vehicles.size()) + " vehicles with " + std::to_string(m_allPaths.size()) + " available paths.");
		// 分析路径起始道路分布
		std::map<uint16, std::vector<size_t>> startRoadToPathIndex;
		for (size_t i = 0; i < m_allPaths.size(); ++i) {
			if (!m_allPaths[i].empty()) {
				uint16 startRoad = m_allPaths[i][0];
				startRoadToPathIndex[startRoad].push_back(i);
			}
		}

		LogManager::instance()->logMessage("=== 路径起始道路分布诊断 ===");
		LogManager::instance()->logMessage("总路径数: " + std::to_string(m_allPaths.size()));
		LogManager::instance()->logMessage("不同起始道路数: " + std::to_string(startRoadToPathIndex.size()));

		for (const auto& entry : startRoadToPathIndex) {
			std::string pathList = "";
			for (size_t pathIdx : entry.second) {
				pathList += "Path[" + std::to_string(pathIdx) + "] ";
			}
			LogManager::instance()->logMessage("起始道路 Road[" + std::to_string(entry.first) + "] 被用于 " +
				std::to_string(entry.second.size()) + " 条路径: " + pathList);

			// 警告：如果某个起始道路被过多路径使用
			if (entry.second.size() > m_allPaths.size() / 2) {
				LogManager::instance()->logMessage("⚠️ 警告: 超过一半的路径都从道路 Road[" +
					std::to_string(entry.first) + "] 开始! 这可能导致车辆聚集问题!");
			}
		}
		// 分析路径特征，智能分配策略
		std::vector<std::pair<size_t, int>> pathsByLength; // (路径索引, 路径长度)
		for (size_t i = 0; i < m_allPaths.size(); ++i) {
			pathsByLength.push_back({ i, static_cast<int>(m_allPaths[i].size()) });
		}

		// 按路径长度排序：长路径优先分配
		std::sort(pathsByLength.begin(), pathsByLength.end(),
			[](const auto& a, const auto& b) { return a.second > b.second; });

		LogManager::instance()->logMessage("路径长度分布分析 - 最长: " + std::to_string(pathsByLength[0].second) +
			" 段, 最短: " + std::to_string(pathsByLength.back().second) + " 段");

		// 智能车辆分配策略
		const float minGap = 80.0f;
		std::map<size_t, std::vector<Vehicle*>> pathVehicleGroups; // 每条路径分配的车辆

		// 计算每条路径应该分配的车辆数量（基于路径长度权重）
		std::vector<int> vehiclesPerPath(m_allPaths.size(), 0);
		int totalPathLength = 0;
		for (const auto& [pathIdx, pathLength] : pathsByLength) {
			totalPathLength += pathLength;
		}

		// 根据路径长度权重分配车辆
		int assignedVehicles = 0;
		for (size_t i = 0; i < pathsByLength.size(); ++i) {
			size_t pathIdx = pathsByLength[i].first;
			int pathLength = pathsByLength[i].second;

			// 长路径分配更多车辆，但至少每条路径分配1辆车
			int vehicleCount = std::max(1, static_cast<int>(
				static_cast<double>(pathLength) / totalPathLength * m_Vehicles.size()));

			// 确保不超过剩余车辆数
			vehicleCount = std::min(vehicleCount, static_cast<int>(m_Vehicles.size()) - assignedVehicles);
			vehiclesPerPath[pathIdx] = vehicleCount;
			assignedVehicles += vehicleCount;

			LogManager::instance()->logMessage("路径[" + std::to_string(pathIdx) + "] 长度" +
				std::to_string(pathLength) + "段，分配" + std::to_string(vehicleCount) + "辆车");
		}

		// 将剩余车辆分配给最长的路径
		if (assignedVehicles < m_Vehicles.size()) {
			size_t longestPathIdx = pathsByLength[0].first;
			vehiclesPerPath[longestPathIdx] += (m_Vehicles.size() - assignedVehicles);
			LogManager::instance()->logMessage("剩余" + std::to_string(m_Vehicles.size() - assignedVehicles) +
				"辆车分配给最长路径[" + std::to_string(longestPathIdx) + "]");
		}

		// 执行车辆分配
		int vehicleIndex = 0;
		for (size_t pathIdx = 0; pathIdx < m_allPaths.size(); ++pathIdx) {
			int vehicleCount = vehiclesPerPath[pathIdx];
			if (vehicleCount == 0) continue;

			const auto& path = m_allPaths[pathIdx];
			if (path.empty()) continue;

			// 获取路径起始道路
			Road* startRoad = getRoadById(path[0]);
			if (!startRoad) {
				LogManager::instance()->logMessage("警告: 无法找到路径起始道路 Road[" + std::to_string(path[0]) + "]");
				continue;
			}

			// 为这条路径分配指定数量的车辆
			for (int v = 0; v < vehicleCount && vehicleIndex < m_Vehicles.size(); ++v, ++vehicleIndex) {
				Vehicle* vehicle = m_Vehicles[vehicleIndex];

				// 设置车辆在起始道路上的位置（基于车辆在路径中的顺序）
				float vehiclePosition = v * minGap;
				vehicle->s = vehiclePosition;

				// 分配路径
				if (vehicle->laneDirection == Vehicle::LaneDirection::Forward) {
					assignPathToVehicle(vehicle, static_cast<int>(pathIdx));
				}
				else {
					assignBackwardPathToVehicle(vehicle, static_cast<int>(pathIdx));
				}

				// 记录分配
				pathVehicleGroups[pathIdx].push_back(vehicle);

				LogManager::instance()->logMessage("Vehicle[" + std::to_string(vehicle->id) + "] 分配到路径[" +
					std::to_string(pathIdx) + "] (" + std::to_string(path.size()) + "段) 位置:" + std::to_string(vehiclePosition));
			}
		}

		LogManager::instance()->logMessage("Adaptive path assignment completed. " + std::to_string(vehicleIndex) +
			" vehicles assigned across " + std::to_string(pathVehicleGroups.size()) + " paths.");

		// 输出分配统计
		for (const auto& [pathIdx, vehicles] : pathVehicleGroups) {
			LogManager::instance()->logMessage("路径[" + std::to_string(pathIdx) + "] 分配了 " +
				std::to_string(vehicles.size()) + " 辆车，路径长度: " + std::to_string(m_allPaths[pathIdx].size()) + " 段");
		}
	}



	// Vehicle路径跟踪方法

	void Vehicle::setPath(const std::vector<uint16>& roadIds)
	{
		m_pathRoads = roadIds;
		if (!roadIds.empty()) {
			m_currentRoadIndex = 0;  // 确保从路径的第一个道路开始
		}
		else {
			m_currentRoadIndex = -1; // 无效状态
		}
	}

	bool Vehicle::moveToNextRoad()
	{
		if (m_currentRoadIndex + 1 < m_pathRoads.size()) {
			m_currentRoadIndex++;

			return true;
		}
		else if (!m_pathRoads.empty()) {
			// 路径循环：回到路径起点
			m_currentRoadIndex = 0;

			return true;
		}
		return false; // 空路径时返回false
	}


	bool Vehicle::moveToPreviousRoad()
	{
		if (m_currentRoadIndex > 0)
		{
			m_currentRoadIndex--;

			return true;
		}
		return false;
	}

	uint16 Vehicle::getCurrentRoadId() const
	{
		if (m_currentRoadIndex >= 0 && m_currentRoadIndex < m_pathRoads.size()) {
			return m_pathRoads[m_currentRoadIndex];
		}
		return 0; // 无效ID
	}
	bool Vehicle::hasPath() const
	{
		return !m_pathRoads.empty() && m_currentRoadIndex >= 0 && m_currentRoadIndex < m_pathRoads.size();
	}

	//路径管理


	Road* Traffic::getRoadById(uint16 roadId)
	{
		for (Road* road : m_allRoads) {
			if (road && road->getRoadId() == roadId) {
				return road;
			}
		}
		return nullptr;
	}

	bool Traffic::frameStarted(const FrameEvent& evt)

	{
		static auto lastTikcBegin = std::chrono::steady_clock::now();
		static int counter = 0;
		static float sumOfDt = 0.0f;
		static float sumOfUpdate = 0.0f;
		// 工作线程等待初始化完成
		auto frameStart = std::chrono::steady_clock::now();
		std::chrono::nanoseconds dt = frameStart - lastTikcBegin;
		lastTikcBegin = frameStart;
		++counter;
		sumOfDt += std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();




		if (!m_isInitialized)
		{
			// 仅在第一次满足条件时执行
			static bool jobSubmitted = false;
			if (!jobSubmitted)
			{

				// 创建并运行后台Job，负责初始化道路 (initRoads)
				LogManager::instance()->logMessage("Submitting VehicleTiker job for road initialization...");
				vehicleTiker = std::make_unique<VehicleTiker>(this);

				vehicleTiker->RunJob();
				jobSubmitted = true;
			}

			// 主线程等待后台Job完成道路初始化 (initRoads)
			//    VehicleTiker::Execute 会在 initRoads 后 signal m_workBarrier
			//m_workBarrier.wait();

			// 道路初始化已完成，现在主线程可以安全地初始化车辆 (initVehicle)
			LogManager::instance()->logMessage("Roads initialized. Starting vehicle creation on main thread...");
			//initVehicle();

			// 所有初始化完成
			m_isInitialized = true;
			LogManager::instance()->logMessage("All initialization finished. Traffic system is running.");

			// 初始化完成后，让 onTick 的第一次计算可以开始
			m_mainBarrier.signal();
		}

		// 检查上一帧的计算是否完成
		if (m_isInitialized && m_onTickCompleted.exchange(false))
		{
			// 获取计算结果并更新渲染对象的位置
			UpdatePositon();

			auto afterUpdate = std::chrono::steady_clock::now();
			sumOfUpdate += std::chrono::duration_cast<std::chrono::milliseconds>(afterUpdate - frameStart).count();
			// 提交新的VehicleTiker Job到后台执行下一帧的计算



			if (vehicleTiker)
			{
				vehicleTiker->RunJob();
			}
		}

		if (counter % 100 == 0)
		{
			std::stringstream ss;
			ss << "frameStarted duration: ";
			ss << sumOfDt / 100.0f;
			ss << ", update duration: ";
			ss << sumOfUpdate / 100.0f;
			ss << "\n";

			//LogManager::instance()->logMessage(ss.str());

			counter = 0;
			sumOfDt = 0.0f;
			sumOfUpdate = 0.0f;
		}

		return true; // 继续渲染
	}





	HighwayConnectData Traffic::parseHighwayConnectData(const std::string& jsonFilePath)
	{


		HighwayConnectData result;

		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonFilePath.c_str(), false, "json"));
		if (pDataStream.isNull() || pDataStream->size() == 0)
		{
			return result;
		}

		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->seek(0);
		pDataStream->read(pData, nSize);
		cJSON* root = cJSON_Parse(pData);



		delete[] pData;

		if (!root)
		{
			return result;
		}

		cJSON* type = cJSON_GetObjectItem(root, "type");
		if (type && type->type == cJSON_String) result.type = type->valuestring;

		cJSON* name = cJSON_GetObjectItem(root, "name");
		if (name && name->type == cJSON_String) result.name = name->valuestring;

		cJSON* features = cJSON_GetObjectItem(root, "features");
		if (features && features->type == cJSON_Array)
		{
			int featureCount = cJSON_GetArraySize(features);

			for (int f = 0; f < featureCount; f++)
			{
				cJSON* feature = cJSON_GetArrayItem(features, f);
				if (!feature) continue;

				HighwayFeature hwFeature;


				//annalys properties

				cJSON* properties = cJSON_GetObjectItem(feature, "properties");
				if (properties)
				{
					cJSON* NAME = cJSON_GetObjectItem(properties, "NAME");
					if (NAME && NAME->type == cJSON_String)
					{
						hwFeature.properties.NAME = NAME->valuestring;
					}

					cJSON* connect_bridge_name = cJSON_GetObjectItem(properties, "connect_bridge_name");
					if (connect_bridge_name && connect_bridge_name->type == cJSON_Array)
					{
						int bridgeNameCount = cJSON_GetArraySize(connect_bridge_name);
						for (int bn = 0; bn < bridgeNameCount; bn++)
						{
							cJSON* bridge_name = cJSON_GetArrayItem(connect_bridge_name, bn);
							if (bridge_name && bridge_name->type == cJSON_String)
							{
								hwFeature.properties.connect_bridge_name.push_back(bridge_name->valuestring);
							}
						}
					}

					cJSON* connect_bridge_id = cJSON_GetObjectItem(properties, "connect_bridge_id");
					if (connect_bridge_id && connect_bridge_id->type == cJSON_Array)
					{
						int bridgeIdCount = cJSON_GetArraySize(connect_bridge_id);
						for (int bi = 0; bi < bridgeIdCount; bi++)
						{
							cJSON* bridge_id = cJSON_GetArrayItem(connect_bridge_id, bi);
							if (bridge_id && bridge_id->type == cJSON_Number)
							{
								hwFeature.properties.connect_bridge_id.push_back(static_cast<uint16>(bridge_id->valueint));
							}
						}
					}

					cJSON* length = cJSON_GetObjectItem(properties, "length");
					if (length && length->type == cJSON_String)
					{
						hwFeature.properties.length = length->valuestring;
					}

				}

				//analyz geometry
				cJSON* geometry = cJSON_GetObjectItem(feature, "geometry");
				if (geometry)
				{
					cJSON* coordinates = cJSON_GetObjectItem(geometry, "coordinates");
					if (coordinates && coordinates->type == cJSON_Array)
					{
						int coordCount = cJSON_GetArraySize(coordinates);
						for (int c = 0; c < coordCount; c++)
						{
							cJSON* coordinate = cJSON_GetArrayItem(coordinates, c);
							if (coordinate && coordinate->type == cJSON_Array && cJSON_GetArraySize(coordinate) >= 3)
							{
								Vector3 point;
								cJSON* x = cJSON_GetArrayItem(coordinate, 0);
								cJSON* y = cJSON_GetArrayItem(coordinate, 1);
								cJSON* z = cJSON_GetArrayItem(coordinate, 2);
								if (x && x->type == cJSON_Number && y && y->type == cJSON_Number && z && z->type == cJSON_Number)
								{
									point.x = static_cast<float>(x->valuedouble);
									point.y = static_cast<float>(y->valuedouble);
									point.z = static_cast<float>(z->valuedouble);
									hwFeature.geometry.coordinates.push_back(point);
								}
							}
						}
					}
				}

				result.features.push_back(hwFeature);
			}
		}
		cJSON_Delete(root);
		return result;
	}


	HighwayGraphData Traffic::parseHighwayGraphData(const std::string& jsonFilePath)
	{
		HighwayGraphData result;


		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(jsonFilePath.c_str(), false, "json"));
		if (pDataStream.isNull() || pDataStream->size() == 0)
		{
			return result;
		}


		size_t nSize = pDataStream->size();
		char* pData = new char[nSize + 1];
		memset(pData, 0, nSize + 1);
		pDataStream->seek(0);
		pDataStream->read(pData, nSize);


		cJSON* root = cJSON_Parse(pData);
		delete[] pData;

		if (!root)
		{
			return result;
		}


		cJSON* directed = cJSON_GetObjectItem(root, "directed");
		if (directed && (directed->type == cJSON_True || directed->type == cJSON_False))
		{
			result.directed = (directed->type == cJSON_True);
		}

		cJSON* multigraph = cJSON_GetObjectItem(root, "multigraph");
		if (multigraph && (multigraph->type == cJSON_True || multigraph->type == cJSON_False))
		{
			result.multigraph = (multigraph->type == cJSON_True);
		}


		cJSON* nodes = cJSON_GetObjectItem(root, "nodes");
		if (nodes && nodes->type == cJSON_Array)
		{
			int nodeCount = cJSON_GetArraySize(nodes);
			for (int n = 0; n < nodeCount; n++)
			{
				cJSON* node = cJSON_GetArrayItem(nodes, n);
				if (!node)
				{
					continue;
				}
				HighwayNode hwNode;

				cJSON* geometry = cJSON_GetObjectItem(node, "geometry");
				if (geometry && geometry->type == cJSON_Array && cJSON_GetArraySize(geometry) >= 3)
				{
					cJSON* x = cJSON_GetArrayItem(geometry, 0);
					cJSON* y = cJSON_GetArrayItem(geometry, 1);
					cJSON* z = cJSON_GetArrayItem(geometry, 2);
					if (x && x->type == cJSON_Number && y && y->type == cJSON_Number && z && z->type == cJSON_Number)
					{
						hwNode.geometry.x = static_cast<float>(x->valuedouble);
						hwNode.geometry.y = static_cast<float>(y->valuedouble);
						hwNode.geometry.z = static_cast<float>(z->valuedouble);
					}
				}

				cJSON* name = cJSON_GetObjectItem(node, "name");
				if (name && name->type == cJSON_String)
				{
					hwNode.name = name->valuestring;
				}

				cJSON* id = cJSON_GetObjectItem(node, "id");
				if (id && id->type == cJSON_Number)
				{
					hwNode.id = static_cast<uint16>(id->valueint);
				}

				result.nodes.push_back(hwNode);
			}
		}

		// analys links
		cJSON* links = cJSON_GetObjectItem(root, "links");
		if (links && links->type == cJSON_Array)
		{
			int linkCount = cJSON_GetArraySize(links);
			for (int l = 0; l < linkCount; l++)
			{
				cJSON* link = cJSON_GetArrayItem(links, l);
				if (!link) continue;

				HighwayLink hwLink;

				cJSON* length = cJSON_GetObjectItem(link, "length");
				if (length && length->type == cJSON_Number)
				{
					hwLink.length = static_cast<float>(length->valuedouble);
				}

				cJSON* road_id = cJSON_GetObjectItem(link, "road_id");
				if (road_id && road_id->type == cJSON_Number)
				{
					hwLink.road_id = static_cast<uint16>(road_id->valueint);
				}

				cJSON* road_name = cJSON_GetObjectItem(link, "road_name");
				if (road_name && road_name->type == cJSON_String)
				{
					hwLink.road_name = road_name->valuestring;
				}

				cJSON* source = cJSON_GetObjectItem(link, "source");
				if (source && source->type == cJSON_Number)
				{
					hwLink.source = static_cast<uint16>(source->valueint);
				}

				cJSON* target = cJSON_GetObjectItem(link, "target");
				if (target && target->type == cJSON_Number)
				{
					hwLink.target = static_cast<uint16>(target->valueint);
				}

				result.links.push_back(hwLink);
			}
		}

		cJSON_Delete(root);

		return result;
	}

	//combine json
	void Traffic::createRoadNetworkFromHighwayData(const HighwayConnectData& connectData, const HighwayGraphData& graphData)
	{
		LogManager::instance()->logMessage("Creating highway road network from combined data...");

		m_highwayGraphData = graphData;
		m_highwayNodes = graphData.nodes;


		std::map<std::pair<uint16, uint16>, std::vector<Vector3>> nodeIdPairToCoordinates;
		for (const auto& feature : connectData.features)
		{
			if (feature.properties.connect_bridge_id.size() >= 2) {
				uint16 originalSourceId = static_cast<uint16>(feature.properties.connect_bridge_id[0]);
				uint16 originalTargetId = static_cast<uint16>(feature.properties.connect_bridge_id[1]);

				// 原始方向的坐标（从connect_bridge_id[0]到connect_bridge_id[1]）
				std::pair<uint16, uint16> originalPair = std::make_pair(originalSourceId, originalTargetId);
				nodeIdPairToCoordinates[originalPair] = feature.geometry.coordinates;

				// 反向的坐标（需要反转coordinates数组以匹配反向逻辑）
				std::pair<uint16, uint16> reversePair = std::make_pair(originalTargetId, originalSourceId);
				std::vector<Vector3> reverseCoordinates = feature.geometry.coordinates;
				std::reverse(reverseCoordinates.begin(), reverseCoordinates.end());
				nodeIdPairToCoordinates[reversePair] = reverseCoordinates;

				LogManager::instance()->logMessage(" Processed bidirectional coordinates for nodes (" +
					std::to_string(originalSourceId) + "<->" + std::to_string(originalTargetId) +
					"): '" + feature.properties.NAME + "' (" + std::to_string(feature.geometry.coordinates.size()) + " points)");
			}
			else {
				LogManager::instance()->logMessage("Warning: Feature '" + feature.properties.NAME + "' has insufficient connect_bridge_id data");
			}
		}

		// 创建节点ID到节点的映射
		std::map<uint16, HighwayNode> nodeIdToNode;
		for (const auto& node : graphData.nodes)
		{
			nodeIdToNode[node.id] = node;
		}

		// 为每个link创建道路
		for (const auto& link : graphData.links) {
			// 查找源节点和目标节点
			auto sourceNodeIt = nodeIdToNode.find(link.source);
			auto targetNodeIt = nodeIdToNode.find(link.target);

			if (sourceNodeIt == nodeIdToNode.end() || targetNodeIt == nodeIdToNode.end())
			{
				LogManager::instance()->logMessage("Warning: Link[" + std::to_string(link.road_id) + "] '" + link.road_name + "' has missing nodes (source=" + std::to_string(link.source) + ", target=" + std::to_string(link.target) + ")");
				continue;
			}

			// 使用节点ID对查找对应的坐标数据
			std::pair<uint16, uint16> nodeIdPair = std::make_pair(link.source, link.target);
			auto coordIt = nodeIdPairToCoordinates.find(nodeIdPair);
			std::vector<Vector3> coordinates;

			if (coordIt == nodeIdPairToCoordinates.end())
			{
				// 如果找不到匹配的坐标，使用节点位置创建简单的直线坐标
				LogManager::instance()->logMessage("Warning: No coordinate data found for node pair (" + std::to_string(link.source) + " -> " + std::to_string(link.target) + ") of road '" + link.road_name + "', creating fallback coordinates");
				coordinates.push_back(sourceNodeIt->second.geometry);
				coordinates.push_back(targetNodeIt->second.geometry);
			}
			else
			{
				coordinates = coordIt->second;
				LogManager::instance()->logMessage("Found " + std::to_string(coordinates.size()) + " coordinates for node pair (" + std::to_string(link.source) + " -> " + std::to_string(link.target) + ") of road '" + link.road_name + "'");
			}

			// 确保至少有两个坐标点
			if (coordinates.size() < 2)
			{
				LogManager::instance()->logMessage("Warning: Road '" + link.road_name + "' has insufficient coordinates, using node positions");
				coordinates.clear();
				coordinates.push_back(sourceNodeIt->second.geometry);
				coordinates.push_back(targetNodeIt->second.geometry);
			}

			// 创建Road对象
			Road* newRoad = createRoadFromHighwayLink(link, sourceNodeIt->second, targetNodeIt->second, coordinates);
			if (newRoad)
			{
				m_allRoads.push_back(newRoad);
				LogManager::instance()->logMessage("Successfully created Road[" + std::to_string(link.road_id) + "] '" + link.road_name + "' with " + std::to_string(coordinates.size()) + " coordinates");
			}
		}


		LogManager::instance()->logMessage("Highway road network creation completed. Total roads: " + std::to_string(m_allRoads.size()));
	}

	//
	Road* Traffic::createRoadFromHighwayLink(const HighwayLink& link, const HighwayNode& sourceNode, const HighwayNode& targetNode, const std::vector<Vector3>& coordinates)
	{
		Road* road = new Road(link, sourceNode, targetNode, coordinates);
		road->setTrafficManager(this);

		return road;
	}


	uint16 Road::getSourceNodeId() const
	{
		return mSourceNodeId;
	}

	uint16 Road::getDestinationNodeId() const
	{
		return mTargetNodeId;
	}

	Vector3 Road::getPositionAtDistance(float distance) const
	{
		if (!m_isHighwayRoad || m_coordinatePath.empty()) {
			return Vector3::ZERO;
		}

		// 处理边界情况
		if (distance <= 0.0f) {
			return m_coordinatePath[0];
		}
		if (distance >= mLens) {
			return m_coordinatePath.back();
		}

		// 查找包含该距离的线段
		for (size_t i = 0; i < m_cumulativeLengths.size() - 1; ++i) {
			if (distance <= m_cumulativeLengths[i + 1]) {
				float segmentStart = m_cumulativeLengths[i];
				float segmentEnd = m_cumulativeLengths[i + 1];
				float segmentLength = segmentEnd - segmentStart;

				if (segmentLength < 0.001f) {
					return m_coordinatePath[i];
				}

				// 计算在线段内的插值参数
				float t = (distance - segmentStart) / segmentLength;
				t = std::max(0.0f, std::min(1.0f, t)); // 确保t在[0,1]范围内

				// 线性插值
				return m_coordinatePath[i] + (m_coordinatePath[i + 1] - m_coordinatePath[i]) * t;
			}
		}

		return m_coordinatePath.back();
	}

	Vector3 Road::getDirectionAtDistance(float distance) const
	{
		if (!m_isHighwayRoad || m_coordinatePath.size() < 2) {
			return Vector3::UNIT_X; // 默认方向
		}

		// 处理边界情况
		if (distance <= 0.0f) {
			Vector3 direction = m_coordinatePath[1] - m_coordinatePath[0];
			direction.Normalize();
			return direction;
		}
		if (distance >= mLens) {
			Vector3 direction = m_coordinatePath.back() - m_coordinatePath[m_coordinatePath.size() - 2];
			direction.Normalize();
			return direction;
		}

		// 查找包含该距离的线段
		for (size_t i = 0; i < m_cumulativeLengths.size() - 1; ++i) {
			if (distance <= m_cumulativeLengths[i + 1]) {
				Vector3 direction = m_coordinatePath[i + 1] - m_coordinatePath[i];
				direction.Normalize();
				return direction;
			}
		}

		// 默认返回最后一段的方向
		Vector3 direction = m_coordinatePath.back() - m_coordinatePath[m_coordinatePath.size() - 2];
		direction.Normalize();
		return direction;
	}

	Vector3 Road::getNormalAtDistance(float distance) const
	{
		if (!m_isHighwayRoad) {
			return Vector3::UNIT_Y; // 法向量
		}

		// Highway道路，假设法向量总是向上


		return Vector3::UNIT_Y;
	}


	Traffic::VehicleTiker::VehicleTiker(Traffic* pTraffic) : Job(Name("VehicleTiker"), Normal)
		, mTraffic(pTraffic)
	{
	}

	Traffic::VehicleTiker::~VehicleTiker()
	{
		CancelOrWait();
	}

	void Traffic::VehicleTiker::Execute()
	{
		if (mTraffic)
		{
			static bool jobInitialized = false;

			if (!jobInitialized)
			{
				// Job中的一次性初始化：道路创建和车辆生成
				mTraffic->initRoads();
				mTraffic->initVehicle();
				// 
				// 通知主线程道路初始化完成，可以进行车辆初始化


				jobInitialized = true;
			}

			mTraffic->onTick();
		}
	}


}
