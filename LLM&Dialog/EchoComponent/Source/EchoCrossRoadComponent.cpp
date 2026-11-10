#include "EchoComponentStableHeaders.h"
#include "EchoCrossRoadComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoRoadManager.h"
#include "ControlHelperRenderable.h"

#define CheckIndex(Index, VecData, Vec1Data, Vec2Data)	\
	while(VecData.size() <= Index)	\
	{								\
		VecData.emplace_back();		\
		Vec1Data.emplace_back();	\
		Vec2Data.emplace_back();	\
	}

namespace Echo
{
	EDITOR_REFLECT_BEGIN(CrossRoadComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(PickPlace, "PickPlace", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_vPlace, "PlaceData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_vCenter, "PointData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_vRay, "RayData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RenderDistance, "RenderDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Lod1Distance, "Lod1Distance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Lod2Distance, "Lod2Distance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	CrossRoadComponentInfo::CrossRoadComponentInfo(
		const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{

	}

	void CrossRoadComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CrossRoadComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CrossRoadComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CrossRoadComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void CrossRoadComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;

		CrossRoadComponentInfo* pInfo = static_cast<CrossRoadComponentInfo*> (info);
		reflect::TypeResolver<CrossRoadComponentInfo>::get()->copyTo(this, pInfo);
	}

	void CrossRoadComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			CrossRoadComponentInfo* m_pInfo = static_cast<CrossRoadComponentInfo*>(mOriginComInfo);
			reflect::TypeResolver<CrossRoadComponentInfo>::get()->overWrite(m_pInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool CrossRoadComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_MeshName.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Mesh, m_MeshName.toString()));
		return true;
	}
#endif
	void CrossRoadComponentInfo::SetPlace(size_t index, ControlPlace place)
	{
		CheckIndex(index, m_vPlace, m_vCenter, m_vRay);
		m_vPlace[index] = place;
	}

	void CrossRoadComponentInfo::SetCenter(size_t index, const Vector3& center)
	{
		CheckIndex(index, m_vPlace, m_vCenter, m_vRay);
		m_vCenter[index] = center;
	}

	void CrossRoadComponentInfo::SetRay(size_t index, const Vector3& ray)
	{
		CheckIndex(index, m_vPlace, m_vCenter, m_vRay);
		m_vRay[index] = ray;
		m_vRay[index].Normalize();
	}

	void CrossRoadComponentInfo::DeleteData(int index)
	{
		if (index == -1)
		{
			m_vCenter.pop_back();
			m_vPlace.pop_back();
			m_vRay.pop_back();

		}
		else if (index < m_vPlace.size())
		{
			m_vCenter.erase(std::next(m_vCenter.begin(), index));
			m_vPlace.erase(std::next(m_vPlace.begin(), index));
			m_vRay.erase(std::next(m_vRay.begin(), index));
		}
	}


#ifdef ECHO_EDITOR

	struct ControlConnect
	{
		BoxHelper* mCenter;
		Vector3 mControl;
		ControlConnect(Echo::SceneManager* sMgr) { mCenter = new BoxHelper(sMgr); }
		~ControlConnect() { delete mCenter; }
	};

	struct CrossRoadEditorData
	{
		std::vector<std::pair<ControlPlace, ControlConnect*>> m_ControlPoints;
		//std::map<ControlPlace, ControlConnect*> m_mapControlPoint;
		float m_fScale = 1.f;
		bool m_bUpdate = true;
		bool m_bVisible = false;

		bool CalculateControlPoint(int pick_index = -1);

		int HitTest(BoxHelper* p_hit, Entity* p_CrossMode);

		void Update();
		int createVecData(int p_place);
		bool deleteVecData(int index = -1);

		void keepScale(const Vector3& pos);
		bool PickHelper(const Ray& ray, size_t& pick_index);
		void hitLightPoint(const Ray& ray);

		bool getError() { return m_bUpdate; }
		ControlConnect* getHelper(size_t p_place);
		size_t getVecDataSize() { return m_ControlPoints.size(); }
		void setPrivateData(const std::vector<Vector3>& data, int pick_place);
		void setPrivateDataIndex(const std::vector<Vector3>& data, int pick_index);
		std::vector<std::pair<ControlPlace, ControlConnect*>>& getVecData() { return m_ControlPoints; }
		bool setControlPoint(ControlPlace p_place, const Vector3& point, const Vector3& vec);
	};

	bool CrossRoadEditorData::CalculateControlPoint(int pick_index)
	{
		if (pick_index != -1 && pick_index >= m_ControlPoints.size()) return false;
		for (size_t i = 0; i < m_ControlPoints.size(); ++i)
		{
			if (pick_index != -1) i = pick_index;
			auto& ControlPoint = m_ControlPoints[i];
			if (ControlPoint.second)
			{
				BoxHelper* box = ControlPoint.second->mCenter;
				box->setBox(0.5f, 0.5f, 0.5f);
				box->setColor(Echo::ColorValue(0.8f, 0.6f, 0.12f, 0.8f));
				//m_pNode->addChild(box->GetSceneNode());
				box->setVisible(m_bVisible);
			}
			if (pick_index != -1) break;
		}

		return true;
	}

	int CrossRoadEditorData::HitTest(BoxHelper* p_hit, Entity* p_CrossMode)
	{
		int iVecIndex = -1;

		if (!p_CrossMode) return iVecIndex;

		AxisAlignedBox box = p_CrossMode->getWorldBoundingBox();
		AxisAlignedBox hit_box = p_hit->GetSceneNode()->getWorldAABB();

		if (!box.Intersect(hit_box)) return iVecIndex;

		if (m_ControlPoints.empty()) return iVecIndex;

		float min_distance = -1.f;

		Vector3 this_point;
		Vector3 point = p_hit->getWorldPosition();
		for (size_t i = 0; i < m_ControlPoints.size(); ++i)
		{
			auto& controlpoint = m_ControlPoints[i];
			if (controlpoint.second)
			{
				this_point = controlpoint.second->mCenter->getWorldPosition();

				float distance = point.distance(this_point);

				if (min_distance == -1.f || min_distance > distance)
				{
					min_distance = distance;
					iVecIndex = (int)i;
				}
			}
		}

		return iVecIndex;
	}

	void CrossRoadEditorData::Update()
	{
		for (auto& ControlPoint : m_ControlPoints)
		{
			if (ControlPoint.second)
				ControlPoint.second->mCenter->setVisible(m_bVisible);
		}
	}

	bool CrossRoadEditorData::deleteVecData(int index)
	{
		bool isDelete = false;
		if (index == -1)
		{
			for (auto& v : m_ControlPoints)
			{
				if (v.second && v.second->mCenter)
				{
					delete v.second;
					isDelete = true;
				}
			}
			m_ControlPoints.clear();
		}
		else
		{
			if (m_ControlPoints.size() <= index || index < 0) return isDelete;
			delete m_ControlPoints[index].second;
			isDelete = true;
			m_ControlPoints.erase(std::next(m_ControlPoints.begin(), index));
		}

		return isDelete;
	}

	int CrossRoadEditorData::createVecData(int p_place)
	{
		//bool isDelete = deleteVecData();
		int pick_index = (int)m_ControlPoints.size();

		if (p_place & Left)
			m_ControlPoints.push_back({ Left, nullptr });

		if (p_place & Right)
			m_ControlPoints.push_back({ Right, nullptr });

		if (p_place & Front)
			m_ControlPoints.push_back({ Front, nullptr });

		if (p_place & Back)
			m_ControlPoints.push_back({ Back, nullptr });

		return pick_index;
	}

	void CrossRoadEditorData::keepScale(const Vector3& cam_pos)
	{
		// 控制点随屏幕距离缩放
		for (size_t i = 0; i < m_ControlPoints.size(); ++i)
		{
			auto& box = m_ControlPoints[i].second->mCenter;
			auto& point = box->getWorldPosition();
			float dist = cam_pos.distance(point);
			dist *= 0.05f;
			if (box->getScale() != dist)
				box->setScale(dist);

			/*if (mDestinationControlHelpers.size() > i)
			{
				auto& destination = mDestinationControlHelpers[i]->getWorldPosition();
				float dist_DControl = cam_pos.distance(destination);
				dist_DControl *= 0.05f;
				if (mDestinationControlHelpers[i]->getScale() != dist_DControl)
					mDestinationControlHelpers[i]->setScale(dist_DControl);
			}

			if (mSourceControlHelpers.size() > i)
			{
				auto& source = mSourceControlHelpers[i]->getWorldPosition();
				float dist_SControl = cam_pos.distance(source);
				dist_SControl *= 0.05f;
				if (mSourceControlHelpers[i]->getScale() != dist_SControl)
					mSourceControlHelpers[i]->setScale(dist_SControl);
			}*/

		}
	}

	bool CrossRoadEditorData::PickHelper(const Ray& ray, size_t& pick_index)
	{
		BoxHelper* _helper = nullptr;
		pick_index = m_ControlPoints.size();
		float closest_distance = std::numeric_limits<float>::infinity();
		for (size_t i = 0; i < m_ControlPoints.size(); ++i)
		{
			auto& box = m_ControlPoints[i].second->mCenter;
			auto res = box->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = box;
				closest_distance = res.second;
				pick_index = i;
			}
		}
		
		return pick_index != m_ControlPoints.size();
	}

	void CrossRoadEditorData::hitLightPoint(const Ray& ray)
	{
		BoxHelper* _helper = nullptr;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (auto& helper : m_ControlPoints)
		{
			auto& box = helper.second->mCenter;
			box->setHighlight(false);
			auto res = box->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = box;
				closest_distance = res.second;
			}
		}

		if (_helper != nullptr)
			_helper->setHighlight(true);
	}

	ControlConnect* CrossRoadEditorData::getHelper(size_t p_place)
	{
		if (p_place < m_ControlPoints.size())
			return m_ControlPoints[p_place].second;
		return nullptr;
	}

	bool CrossRoadEditorData::setControlPoint(ControlPlace p_place, const Vector3& point, const Vector3& vec)
	{
		bool result = false;
		for (auto& ControlPoint : m_ControlPoints)
		{
			if (!(ControlPoint.first ^ p_place))
			{
				ControlPoint.second->mCenter->setPosition(point);
				ControlPoint.second->mCenter->setScale(m_fScale);
				ControlPoint.second->mControl = vec;
				result |= true;
			}
		}

		return result;
	}

	void CrossRoadEditorData::setPrivateData(const std::vector<Vector3>& data, int pick_place)
	{
		if (pick_place & Left)
			setControlPoint(Left, data[0], data[1]);

		if (pick_place & Right)
			setControlPoint(Right, data[2], data[3]);

		if (pick_place & Front)
			setControlPoint(Front, data[4], data[5]);

		if (pick_place & Back)
			setControlPoint(Back, data[6], data[7]);
	}

	void CrossRoadEditorData::setPrivateDataIndex(const std::vector<Vector3>& data, int pick_index)
	{
		if (m_ControlPoints.size() <= pick_index) return;

		auto& ControlPoint = m_ControlPoints[pick_index];
		ControlPlace& place = m_ControlPoints[pick_index].first;

		if (!ControlPoint.second) return;

		int num = CrossRoadComponent::GetPlaceNoEnum(place);

		if (2 * num + 1 < data.size())
		{
			ControlPoint.second->mCenter->setPosition(data[2 * num]);
			ControlPoint.second->mCenter->setScale(m_fScale);
			ControlPoint.second->mControl = data[2 * num + 1];
			//ControlPoint.second->control_point = data[2 * num + 1];
		}
	}
#endif // ECHO_EDITOR


	class CrossRoadComponentPrivate : public ResourceBackgroundQueue::Listener
	{
	public:
		CrossRoadComponentPrivate(Echo::SceneManager* sMgr);
		~CrossRoadComponentPrivate();

		virtual void operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;

		void CreateEntityInMesh(const MeshPtr p_mesh);
		void CreateControlInMesh(const MeshPtr p_mesh);
		void SetEditorDataInPrivate(int index = -1);

		void setVecData(int index, ControlPlace place);
		void setVecData(int index, const Vector3& pos);
		void setRayData(int index, const Vector3& ray);
		void SetPosInPlace(Vector3& pos, ControlPlace place);

		void GetPlaceBaseData(ControlPlace place, Vector3& pos, Vector3& ray);

		SceneNode* getSceneNode() { return m_pNode; }

#ifdef ECHO_EDITOR
		std::vector<Vector3> m_PlaceBaseData;
		CrossRoadEditorData* m_pEditorData = nullptr;
#endif // ECHO_EDITOR
		Echo::SceneManager* m_pSceneManager = nullptr;
		SceneNode* m_pNode = nullptr;
		Entity* m_pIntersectionMode = nullptr;
		std::set<BackgroundProcessTicket> m_Tickets;
	};

	/*bool CrossRoadComponentPrivate::notifDisConnectBezier(PiecewiseBezierComponentPtr p_bezier)
	{
		auto iter = m_mapLinkRoads.find(p_bezier);
		if (iter != m_mapLinkRoads.end())
		{
			delete iter->second;
			m_mapLinkRoads.erase(iter);
			return true;
		}
		return false;
	}

	bool CrossRoadComponentPrivate::notityConnectBezier(
		PiecewiseBezierComponentPtr p_bezier, BoxHelper* p_center, SphereHelper* p_control)
	{
		auto iter = m_mapLinkRoads.find(p_bezier);
		if (iter == m_mapLinkRoads.end())
		{
			ControlConnect* insertpoint = new ControlConnect();
			insertpoint->mCenter = p_center;
			insertpoint->mControl = p_control;
			m_mapLinkRoads[p_bezier] = insertpoint;
			return true;
		}
		return false;
	}*/

	CrossRoadComponentPrivate::CrossRoadComponentPrivate(Echo::SceneManager* sMgr)
	{
#ifdef ECHO_EDITOR
		m_pEditorData = new CrossRoadEditorData();
#endif // ECHO_EDITOR
		m_pSceneManager = sMgr;
	}

	CrossRoadComponentPrivate::~CrossRoadComponentPrivate()
	{
		if (m_pNode)
		{
			m_pNode->detachAllObject();
			m_pSceneManager->destroySceneNode(m_pNode);
		}

		if (m_pIntersectionMode)
			m_pSceneManager->destroyEntity(m_pIntersectionMode);
#ifdef ECHO_EDITOR
		m_pEditorData->deleteVecData();
		delete m_pEditorData;
#endif
	}

#ifdef ECHO_EDITOR
	void CrossRoadComponentPrivate::SetEditorDataInPrivate(int index)
	{
		AxisAlignedBox ab = m_pIntersectionMode->getLocalBox();
		m_pEditorData->m_fScale = (ab.vMax.y - ab.vMin.y);

		if (index != -1 && index >= m_pEditorData->m_ControlPoints.size()) return;

		for (size_t i = 0; i < m_pEditorData->m_ControlPoints.size(); ++i)
		{
			if (index != -1) i = index;
			auto& point = m_pEditorData->m_ControlPoints[i];
			if (!point.second)
			{
				point.second = new ControlConnect(m_pSceneManager);
				m_pNode->addChild(point.second->mCenter->GetSceneNode());
			}
			if (index != -1) break;
		}

		m_pEditorData->m_bUpdate = true;
	}

	void CrossRoadComponentPrivate::setVecData(int index, ControlPlace place)
	{
		if (index < m_pEditorData->m_ControlPoints.size())
		{
			auto& point = m_pEditorData->m_ControlPoints[index];
			point.first = place;
			m_pEditorData->setPrivateDataIndex(m_PlaceBaseData, index);
		}
	}

	void CrossRoadComponentPrivate::setVecData(int index, const Vector3& pos)
	{
		if (index < m_pEditorData->m_ControlPoints.size())
		{
			auto& point = m_pEditorData->m_ControlPoints[index];
			if (point.second)
				point.second->mCenter->setPosition(pos);
			//m_pEditorData->setPrivateDataIndex(m_PlaceBaseData, index);
		}
	}

	void CrossRoadComponentPrivate::setRayData(int index, const Vector3& ray)
	{
		if (index < m_pEditorData->m_ControlPoints.size())
		{
			auto& point = m_pEditorData->m_ControlPoints[index];
			if (point.second)
				point.second->mControl = ray.normalisedCopy();
				//point.second->control_point = ray.normalisedCopy();
			//m_pEditorData->setPrivateDataIndex(m_PlaceBaseData, index);
		}
	}

	void CrossRoadComponentPrivate::SetPosInPlace(Vector3& pos, ControlPlace place)
	{
		/*
		y  x		  Front
		| /				|
		|/___ z  Left --+-- Right
		*				|
					   Back
		*/

		AxisAlignedBox ab = m_pIntersectionMode->getBoundingBox();

		size_t Index = 8;

		switch (place)
		{
		case ControlPlace::Left:
			pos.x =
				pos.x > m_PlaceBaseData[Index + 1].x ? m_PlaceBaseData[Index + 1].x :
				pos.x < m_PlaceBaseData[Index].x ? m_PlaceBaseData[Index].x : pos.x;
			pos.y = pos.y > ab.vMax.y ? ab.vMax.y : pos.y < ab.vMin.y ? ab.vMin.y : pos.y;
			pos.z = m_PlaceBaseData[0].z;
			break;
		case ControlPlace::Right:
			Index += 2;
			pos.x = 
				pos.x > m_PlaceBaseData[Index + 1].x ? m_PlaceBaseData[Index + 1].x :
				pos.x < m_PlaceBaseData[Index].x ? m_PlaceBaseData[Index].x : pos.x;
			pos.y = pos.y > ab.vMax.y ? ab.vMax.y : pos.y < ab.vMin.y ? ab.vMin.y : pos.y;
			pos.z = m_PlaceBaseData[2].z;
			break;
		case ControlPlace::Front:
			Index += 4;
			pos.x = m_PlaceBaseData[4].x;
			pos.y = pos.y > ab.vMax.y ? ab.vMax.y : pos.y < ab.vMin.y ? ab.vMin.y : pos.y;
			pos.z = 
				pos.z > m_PlaceBaseData[Index + 1].z ? m_PlaceBaseData[Index + 1].z : 
				pos.z < m_PlaceBaseData[Index].z ? m_PlaceBaseData[Index].z : pos.z;
			break;
		case ControlPlace::Back:
			Index += 6;
			pos.x = m_PlaceBaseData[6].x;
			pos.y = pos.y > ab.vMax.y ? ab.vMax.y : pos.y < ab.vMin.y ? ab.vMin.y : pos.y;
			pos.z = 
				pos.z > m_PlaceBaseData[Index + 1].z ? m_PlaceBaseData[Index + 1].z :
				pos.z < m_PlaceBaseData[Index].z ? m_PlaceBaseData[Index].z : pos.z;
			break;
		default:
			break;
		}

	}

	void CrossRoadComponentPrivate::GetPlaceBaseData(ControlPlace place, Vector3& pos, Vector3& ray)
	{
		int re = CrossRoadComponent::GetPlaceNoEnum(place);
		pos = m_PlaceBaseData[2 * re];
		ray = m_PlaceBaseData[2 * re + 1];
	}

#else
	void CrossRoadComponentPrivate::SetEditorDataInPrivate(int index) {}
	void CrossRoadComponentPrivate::setVecData(int index, ControlPlace place) {}
	void CrossRoadComponentPrivate::setVecData(int index, const Vector3& pos) {}
	void CrossRoadComponentPrivate::setRayData(int index, const Vector3& ray) {}
	void CrossRoadComponentPrivate::SetPosInPlace(Vector3& pos, ControlPlace place) {}
	void CrossRoadComponentPrivate::GetPlaceBaseData(ControlPlace place, Vector3& pos, Vector3& ray) {}
#endif // ECHO_EDITOR

	void CrossRoadComponentPrivate::CreateEntityInMesh(const MeshPtr p_mesh)
	{
		if (p_mesh.isNull())
		{
#ifdef ECHO_EDITOR
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				Echo::LogManager::instance()->logMessage(Echo::LML_NORMAL, Echo::String("(CrossRoadComponentPrivate) Load Mesh is Null"));
			m_pEditorData->m_bUpdate = false; 
#endif
			return;
		}

		if (m_pIntersectionMode != nullptr)
			m_pSceneManager->destroyEntity(m_pIntersectionMode);

		if (m_pNode == nullptr)
			m_pNode = m_pSceneManager->createSceneNode();
		else
			m_pNode->detachAllObject();

		m_pIntersectionMode = m_pSceneManager->createEntity(p_mesh);
		m_pNode->attachObject(m_pIntersectionMode);
	}

	void CrossRoadComponentPrivate::CreateControlInMesh(const MeshPtr p_mesh)
	{
		CreateEntityInMesh(p_mesh);

		SetEditorDataInPrivate();
	}

	void CrossRoadComponentPrivate::operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr)
	{
	}

	DECLAREACTORCOMTYPE_CPP(CrossRoadComponent)
		CrossRoadComponent::CrossRoadComponent(ActorPtr _actor,
			ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/)
		: NodeActorComponent(_actor, movableType)
	{
		m_pPrivate = new CrossRoadComponentPrivate(getSceneManager());
#ifdef ECHO_EDITOR
		RoadManager::instance()->SetCrossRoadVec(this);
		RoadManager::instance()->ListenerPick |= CrossRoadLink_Listener;
#endif // ECHO_EDITOR

	}

	CrossRoadComponent::~CrossRoadComponent()
	{
		delete m_pPrivate;
		//if (!m_pPhyMode.expired())
		//{
		//	auto comInfo = m_pPhyMode->m_pActComInfo;
		//	getActor()->delComponent(m_pPhyMode.getStaticCastWeakPtr<ActorComponent>());
		//	getActor()->delComponentInfo(comInfo);
		//}
		//RoadManager::instance()->SetComponentVaild(m_strComKey);
#ifdef ECHO_EDITOR
		RoadManager::instance()->SetCrossRoadVec(this);
#endif // ECHO_EDITOR
	}

	void CrossRoadComponent::init(ActorComponentInfo* info)
	{
		
	}


	void CrossRoadComponent::frameWork() 
	{
		//CreatePhyMode();
#ifdef ECHO_EDITOR
		if (!RoadManager::instance()->m_pBezierEditor.expired() && m_pActComInfo)
		{
			PiecewiseBezierComponentPtr be = RoadManager::instance()->m_pBezierEditor;
			PiecewiseBezierUtil* beutil = be->getBezierTool();
			size_t pick_index = be->getPickHelperIndex();
			BoxHelper* control_point = beutil->GetPickBoxHelepr(pick_index);
			if (control_point == nullptr) return;
			int hit_index = m_pPrivate->m_pEditorData->HitTest(control_point, m_pPrivate->m_pIntersectionMode);

			if (hit_index >= 0 && hit_index < m_pPrivate->m_pEditorData->getVecDataSize())
			{
				ControlConnect* entity_box = m_pPrivate->m_pEditorData->getHelper(hit_index);

				if (entity_box)
				{
					WorldNode* node = entity_box->mCenter->GetSceneNode();
					WorldNode* bNode = be->getSceneNode();

					Vector3 world_pos = entity_box->mCenter->getWorldPosition();
					be->setBoxPoint(bNode->convertWorldToLocalPosition(world_pos), pick_index);
					Vector3 local_pos = bNode->convertWorldToLocalPosition(world_pos);

					Vector3 opoint = control_point->getPosition();
					Vector3 center_pos = entity_box->mCenter->getPosition();
					Vector3 world_normal = node->convertLocalToWorldPosition(Vector3::UNIT_Y);

					PiecewiseTool::PiecewiseBezier place = beutil->CalculatePlace(pick_index);

					auto func = [&](SphereHelper* p_box, size_t pick_index)->Vector3
					{
						float distance = opoint.distance(p_box->getPosition()) / entity_box->mControl.length();

						BoxHelper* co = beutil->GetPickBoxHelepr(pick_index);
						co->setPosition(local_pos);
						WorldNode* bu_node = co->GetSceneNode();

						auto cpoint = be->getPoint(pick_index);
						cpoint->AxisXAngle = 0;
						cpoint->normal = bu_node->convertWorldToLocalPosition(world_normal);
						
						be->setPoint(pick_index, *cpoint);

						Vector3 control = node->convertLocalToWorldPosition(entity_box->mControl * distance);
						
						control = bu_node->convertWorldToLocalPosition(control);
						control = control * distance / control.distance(Vector3::ZERO);
						return control;
					};

					switch (place)
					{
					case Echo::PiecewiseTool::Begin:
					case Echo::PiecewiseTool::Middle:
					{
						SphereHelper* des_point = beutil->GetControlHelper(pick_index, PiecewiseTool::DestinationControl);
						if (des_point == nullptr) break;
						Vector3 controlV = func(des_point, pick_index);
						be->setDestinationPoint(controlV, pick_index);
						//RoadManager::instance()->m_bUpdateBezier = true;
					} break;
					case Echo::PiecewiseTool::End:
					{
						SphereHelper* sou_point = beutil->GetControlHelper(pick_index, PiecewiseTool::SourceControl);
						if (sou_point == nullptr) break;
						Vector3 controlV = func(sou_point, pick_index);
						be->setSourcePoint(controlV, pick_index);
						//RoadManager::instance()->m_bUpdateBezier = true;
					} break;
					default:
						break;
					}

				}
			}
		}
#endif
	};
	
	void CrossRoadComponent::bindNode()
	{
		if (m_pActComInfo)
		{
			auto Info = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
			bool haveInitData = false;
			if (Info->m_vCenter.size() != 0)
				haveInitData = true;
			loadMeshAndData(haveInitData);
			CreatePhyMode();
		}
		else
		{
			assert(!" m_pInfo is NULL ");
		}
	};

#ifdef ECHO_EDITOR
	void CrossRoadComponent::update()
	{
		m_pPrivate->m_pEditorData->Update();
		if (m_pPrivate->getSceneNode())
			UpdateComRootNode(true);
	}

	size_t CrossRoadComponent::getPointCount()
	{
		return m_pPrivate->m_pEditorData->getVecDataSize();
	}
#endif // ECHO_EDITOR

	

	void CrossRoadComponent::setPickHelperIndex(size_t value)
	{
		m_iPickControlNum = value;
		m_isTrackChange = true;
	}

	bool CrossRoadComponent::TryUpdate()
	{
		return m_pPrivate->m_pIntersectionMode != nullptr;
	}


	void CrossRoadComponent::ResponseRequest(MeshPtr p_Mesh)
	{
		if (!p_Mesh.isNull())
		{
			//m_pPrivate->createMapData(pick_id);
			//m_pInfo->PickPlace = pick_id;
			m_pPrivate->CreateControlInMesh(p_Mesh);
			//m_pPrivate->CalculateControlPoint();
			//setPrivateData(p_Data);

			if (m_pPrivate->getSceneNode())
			{
				addChildToNode(m_pPrivate->getSceneNode());
				UpdateComRootNode(true);
			}

		}
	}

	void CrossRoadComponent::CreatePhyMode()
	{
		if (m_pActComInfo)
		{
			Name mesh_name = static_cast<CrossRoadComponentInfo*>(m_pActComInfo)->m_MeshName;
			mesh_name = Mesh::getDstPhyMeshName(mesh_name);
			if (!m_pPhyMode.expired() && m_pPhyMode->GetMeshName() == mesh_name) return;

			if (!m_pPhyMode.expired())
			{
				auto comInfo = m_pPhyMode->m_pActComInfo;
				getActor()->delComponent(m_pPhyMode.getStaticCastWeakPtr<ActorComponent>());
				getActor()->delComponentInfo(comInfo);
			}

			m_pPhyMode = getActor()->addComponent(Name("MeshColliderComponent")
				, mesh_name.toString(), weak_from_this(), nullptr, this->getGroupID()
			).getStaticCastWeakPtr<MeshColliderComponent>();

			m_pPhyMode->SetMeshName(mesh_name);
		}
	}

	void CrossRoadComponent::SetPosInPlace(Vector3& pos, ControlPlace place)
	{
		// 查看设置的吸附点位置是否偏离所处模型的位置
		return m_pPrivate->SetPosInPlace(pos, place);
	}

#ifdef ECHO_EDITOR
	void CrossRoadComponent::SetVisitable(bool value)
	{
		m_pPrivate->m_pEditorData->m_bVisible = value;
	}

	void CrossRoadComponent::KeepScale(const Vector3& cam_pos)
	{
		m_pPrivate->m_pEditorData->keepScale(cam_pos);
	}

	bool CrossRoadComponent::HitPick(const Ray& ray)
	{
		bool result = false;
		size_t tempIndex = 0;
		result = m_pPrivate->m_pEditorData->PickHelper(ray, tempIndex);
		if (result)
			setPickHelperIndex(tempIndex);
		return result;
	}

	void CrossRoadComponent::HighlightPoint(const Ray& ray)
	{
		m_pPrivate->m_pEditorData->hitLightPoint(ray);
	}

	Vector3 CrossRoadComponent::GetPickHelperPoint(int pick_index)
	{
		if (pick_index == -1) pick_index = (int)m_iPickControlNum;
		auto& map_d = m_pPrivate->m_pEditorData->m_ControlPoints;
		if (pick_index < map_d.size())
			return map_d[pick_index].second->mCenter->getPosition();
		else
			return Vector3::ZERO;
	}

	ControlPlace CrossRoadComponent::GetPickIndexPlace(int pick_index)
	{
		if (pick_index == -1) pick_index = (int)m_iPickControlNum;
		auto& map_d = m_pPrivate->m_pEditorData->m_ControlPoints;
		if (pick_index < map_d.size())
			return map_d[pick_index].first;
		else
			return ControlPlace::NoPlace;
	}

	void CrossRoadComponent::GetPlaceVecData(ControlPlace place, Vector3& pos, Vector3& ray)
	{
		m_pPrivate->GetPlaceBaseData(place, pos, ray);
	}

	void CrossRoadComponent::CreateControlPoint()
	{
		int pick_index = m_pPrivate->m_pEditorData->createVecData(ControlPlace::Left);
		m_pPrivate->SetEditorDataInPrivate(pick_index);
		m_pPrivate->m_pEditorData->CalculateControlPoint(pick_index);
		m_pPrivate->m_pEditorData->setPrivateDataIndex(m_pPrivate->m_PlaceBaseData, pick_index);

		if (m_pPrivate->getSceneNode())
			UpdateComRootNode(true);
	}

	void CrossRoadComponent::CreateInfoData()
	{
		auto& map_d = m_pPrivate->m_pEditorData->m_ControlPoints;
		auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);

		while (pInfo->m_vPlace.size() < map_d.size())
		{
			size_t pick_index = pInfo->m_vPlace.size();
			pInfo->SetPlace(pick_index, map_d[pick_index].first);
			pInfo->SetCenter(pick_index, map_d[pick_index].second->mCenter->getPosition());
			pInfo->SetRay(pick_index, map_d[pick_index].second->mControl);
		}
	}

	// 对外提供使用，负责修改Component与Info内部数据
	void CrossRoadComponent::SetControlPoint(int pick_index, ControlPlace place)
	{
		m_pPrivate->setVecData(pick_index, place);

		if (m_pActComInfo)
		{
			auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
			int num = CrossRoadComponent::GetPlaceNoEnum(place);

			if (2 * num + 1 < m_pPrivate->m_PlaceBaseData.size())
			{
				pInfo->SetPlace(pick_index, place);
				pInfo->SetCenter(pick_index, m_pPrivate->m_PlaceBaseData[2 * num]);
				pInfo->SetRay(pick_index, m_pPrivate->m_PlaceBaseData[2 * num + 1]);
			}
		}
		
		if (m_pPrivate->getSceneNode())
			UpdateComRootNode(true);
	}

	// 对外提供使用，负责修改Component与Info内部数据
	void CrossRoadComponent::SetControlPoint(const Vector3& pos, int pick_index)
	{
		if (pick_index == -1) pick_index = (int)m_iPickControlNum;
		m_pPrivate->setVecData(pick_index, pos);
		if (m_pActComInfo)
		{
			auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
			pInfo->SetCenter(pick_index, pos);
		}
		if (m_pPrivate->getSceneNode())
			UpdateComRootNode(true);
	}

	// 对外提供使用，负责修改Component与Info内部数据
	void CrossRoadComponent::SetControlRay(const Vector3& ray, int pick_index)
	{
		if (pick_index == -1) pick_index = (int)m_iPickControlNum;
		m_pPrivate->setRayData(pick_index, ray);
		if (m_pActComInfo)
		{
			auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
			pInfo->SetRay(pick_index, ray);
		}

		if (m_pPrivate->getSceneNode())
			UpdateComRootNode(true);
	}

	void CrossRoadComponent::DeleteControlPoint(int pick_index)
	{
		if (pick_index == -1) pick_index = (int)m_pPrivate->m_pEditorData->getVecDataSize() - 1;
		//if (pick_index == 0) return;

		m_pPrivate->m_pEditorData->deleteVecData(pick_index);
	}

	size_t CrossRoadComponent::GetPrivateDataSize()
	{
		return m_pPrivate->m_pEditorData->getVecDataSize();
	}

	Vector3 CrossRoadComponent::GetPrivateDataPoint(size_t pick_index, bool isControl)
	{
		if (pick_index >= GetPrivateDataSize()) return Vector3::ZERO;
		if (!isControl)
		{
			return m_pPrivate->m_pEditorData->getVecData()[pick_index].second->mCenter->getPosition();
		}
		else
		{
			return m_pPrivate->m_pEditorData->getVecData()[pick_index].second->mControl;
		}
	}

	ControlPlace CrossRoadComponent::GetPrivateControlPlace(size_t pick_index)
	{
		if (pick_index >= GetPrivateDataSize()) return ControlPlace::NoPlace;
		return m_pPrivate->m_pEditorData->getVecData()[pick_index].first;
	}
#endif // ECHO_EDITOR

	float Echo::CrossRoadComponent::GetRenderDistance()
	{
		if (!m_pPrivate || !m_pPrivate->m_pIntersectionMode)
			return 0.0f;
		return m_pPrivate->m_pIntersectionMode->getRenderingDistance();
	}
	void Echo::CrossRoadComponent::SetRenderDistance(float distance)
	{
		if (!m_pPrivate || !m_pPrivate->m_pIntersectionMode)
			return;
		m_pPrivate->m_pIntersectionMode->setRenderingDistance(distance);
	}
	float Echo::CrossRoadComponent::GetLodDistance(EM_LOD_LEVEL lodlevel)
	{
		if (!m_pPrivate || !m_pPrivate->m_pIntersectionMode)
			return 0.0f;
		return m_pPrivate->m_pIntersectionMode->m_fLodDistance[lodlevel];
	}
	void Echo::CrossRoadComponent::SetLodDistance(EM_LOD_LEVEL lodlevel, float distance)
	{
		if (!m_pPrivate || !m_pPrivate->m_pIntersectionMode)
			return;
		m_pPrivate->m_pIntersectionMode->m_fLodDistance[lodlevel] = distance;
	}

	void CrossRoadComponent::loadMeshAndData(bool haveInitData)
	{
		auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
		if (!pInfo || pInfo->m_MeshName.empty())
		{
#ifdef ECHO_EDITOR
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				Echo::LogManager::instance()->logMessage(Echo::LML_NORMAL, Echo::String("(CrossRoadComponent) Load MeshName is Null"));
#endif // ECHO_EDITOR
			return;
		}

#ifdef ECHO_EDITOR
		MeshPtr mesh;
		LoadMesh(mesh);
		LoadData(mesh, haveInitData);
#else
		stringstream ss;
		ss << this->getComNameToString() << "_" << this;
		m_strComKey = ss.str();

		//RoadManager::instance()->SetComponentVaild(m_strComKey, true, this);
		//RoadManager::instance()->addMeshRequest(m_strComKey, pInfo->m_MeshName);
#endif
	}

#ifdef ECHO_EDITOR

	void CrossRoadComponent::LoadMesh(MeshPtr& mesh)
	{
		auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);
		if (!pInfo || pInfo->m_MeshName.empty()) return;
		try
		{
			mesh = MeshMgr::instance()->load(pInfo->m_MeshName);
			std::vector<std::vector<Vector3>> sub_data;
			for (size_t i = 0; i < mesh->getNumSubMesh(); ++i)
			{
				std::vector<Vector3> data;
				mesh->getSubMesh((uint32)i)->getVertexData(data);
				sub_data.push_back(data);
			}

			std::vector<Vector3>& result = m_pPrivate->m_PlaceBaseData;
			result.clear();
			int pick_id = RoadManager::instance()->CalculateCrossRoadPointAndVec(mesh, sub_data, result);
			if (pInfo->PickPlace == 0 && pick_id != 0)
				pInfo->PickPlace = pick_id;
			//pInfo->PickPlace = pick_id;
		}
		catch (...)
		{
			Echo::LogManager::instance()->logMessage(Echo::LML_NORMAL, Echo::String("(CrossRoadComponent) Load Mesh Error"));
		}
	}

	void CrossRoadComponent::LoadData(const MeshPtr& mesh, bool haveInitData)
	{
		auto pInfo = static_cast<CrossRoadComponentInfo*>(m_pActComInfo);

		if (pInfo && !mesh.isNull())
		{
			// 若pInfo内部存在设置好的吸附点数据，则沿用已设置的吸附点数据
			if (haveInitData)
			{
				m_pPrivate->CreateEntityInMesh(mesh);
				addChildToNode(m_pPrivate->getSceneNode());

				std::vector<int>& places = pInfo->m_vPlace;
				std::vector<Vector3>& centers = pInfo->m_vCenter;
				std::vector<Vector3>& rays = pInfo->m_vRay;

				size_t count = pInfo->m_vPlace.size();
				for (size_t i = 0; i < count; ++i)
				{
					CreateControlPoint();
					m_pPrivate->setVecData((int)i, (ControlPlace)(places[i] == 0 ? 1 : places[i]));
					m_pPrivate->setVecData((int)i, centers[i]);
					m_pPrivate->setRayData((int)i, rays[i]);
				}
			}
			else
			{	// 若pInfo内部不存在设置的吸附点数据，则通过检测选用的模型信息创建适合的吸附点数据
				bool isDelete = m_pPrivate->m_pEditorData->deleteVecData();
				m_pPrivate->m_pEditorData->createVecData(pInfo->PickPlace);
				m_pPrivate->CreateControlInMesh(mesh);
				m_pPrivate->m_pEditorData->CalculateControlPoint();
				m_pPrivate->m_pEditorData->setPrivateData(m_pPrivate->m_PlaceBaseData, pInfo->PickPlace);
				if (m_pPrivate->getSceneNode())
					if (!isDelete) addChildToNode(m_pPrivate->getSceneNode());

				auto& map_d = m_pPrivate->m_pEditorData->getVecData();
				for (size_t i = 0; i < map_d.size(); ++i)
				{
					pInfo->SetPlace(i, map_d[i].first);
					pInfo->SetCenter(i, map_d[i].second->mCenter->getPosition());
					pInfo->SetRay(i, map_d[i].second->mControl);
				}
			}
			if (m_pPrivate->getSceneNode())
				UpdateComRootNode(true);
		}
	}

#endif // ECHO_EDITOR

}
