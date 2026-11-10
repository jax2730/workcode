#include "EchoComponentStableHeaders.h"
#include "EchoPiecewiseBezierUtil.h"
#include "EchoManualObject.h"
#include "EchoEntity.h"
#include "EchoSubMesh.h"
#include "EchoSkeletonInstance.h"
#include <numeric>
#include "EchoRoadManager.h"
#include "EchoTerrainSystem.h"
#include "EchoSphericalTerrainManager.h"

namespace Echo
{
#define ClearError(vector3v)								  \
	vector3v.x = (abs(vector3v.x) < 1e-6 ? 0.f : vector3v.x); \
	vector3v.y = (abs(vector3v.y) < 1e-6 ? 0.f : vector3v.y); \
	vector3v.z = (abs(vector3v.z) < 1e-6 ? 0.f : vector3v.z);

	namespace {
		double C[] = {
			0.1527533871307258,
			0.1527533871307258,
			0.1491729864726037,
			0.1491729864726037,
			0.1420961093183820,
			0.1420961093183820,
			0.1316886384491766,
			0.1316886384491766,
			0.1181945319615184,
			0.1181945319615184,
			0.1019301198172404,
			0.1019301198172404,
			0.0832767415767048,
			0.0832767415767048,
			0.0626720483341091,
			0.0626720483341091,
			0.0406014298003869,
			0.0406014298003869,
			0.0176140071391521,
			0.0176140071391521
		};
		double T[] = {
			-0.0765265211334973,
			0.0765265211334973	 ,
			-0.2277858511416451,
			0.2277858511416451	 ,
			-0.3737060887154195,
			0.3737060887154195	 ,
			-0.5108670019508271,
			0.5108670019508271	 ,
			-0.6360536807265150,
			0.6360536807265150	 ,
			-0.7463319064601508,
			0.7463319064601508	 ,
			-0.8391169718222188,
			0.8391169718222188	 ,
			-0.9122344282513259,
			0.9122344282513259	 ,
			-0.9639719272779138,
			0.9639719272779138	 ,
			-0.9931285991850949,
			0.9931285991850949
		};

		/*Echo::SceneManager* scenemgr()
		{
			return Root::instance()->getActiveSceneManager();
		}*/
	}
	namespace PiecewiseTool
	{
		float calculateMinTerrainLength(Ray& ray, const Vector3& ModelSize, const Matrix3& tranformMatrix, bool pickEntity)
		{
			auto p = [&](const Vector3& point, std::pair<Vector3, bool>* length)->void {
				Ray p_ray(point, ray.getDirection());
				std::pair<Vector3, bool> result = ToolGetHeight(p_ray, pickEntity);

				if (length->second && result.second)
				{
					if (length->first.y > result.first.y) length->first = result.first.y;
				}
				else if (!length->second && !result.second)
				{
					if (length->first.y > result.first.y) length->first = result.first.y;
				}
				else if (length->second && !result.second)
				{
					length->first.y = result.first.y;
					length->second = result.second;
				}

			};

			std::pair<Vector3, bool> HeightResult = ToolGetHeight(ray, pickEntity);
			Vector3 position = ray.getOrigin();

			std::vector<Vector3> calculPoint;
			for (int i = 0; i < 3 && ModelSize != Vector3::ZERO; i = i + 2)
			{
				Vector3 modelsize = ModelSize;
				float v = modelsize[i];
				if (v == 0) continue;
				Vector3 point = Vector3::ZERO;
				point[i] = v / 2.f;
				calculPoint.push_back(position + tranformMatrix * point);
				calculPoint.push_back(position - tranformMatrix * point);
			}

			std::for_each(calculPoint.begin(), calculPoint.end(), std::bind(p, std::placeholders::_1, &HeightResult));

			return HeightResult.first.y;
		}

#ifdef ECHO_EDITOR


		bool PickEntity(Echo::Ray& ray, Echo::Entity** result, Echo::Vector3& hitpoint, Echo::uint32 uQueryMask, float max_distance, bool keyval)
		{
			Echo::RaySceneQuery* g_pRaySceneQuery = Echo::Root::instance()->getActiveSceneManager()->createRayQuery(ray);
			g_pRaySceneQuery->setRay(ray);
			g_pRaySceneQuery->setQueryMask(uQueryMask);
			g_pRaySceneQuery->setSortByDistance(true);

			unsigned int mVisibilityMask = Echo::Root::instance()->getActiveSceneManager()->getVisibilityMask();

			if (g_pRaySceneQuery->execute().size() <= 0) return (false);

			// at this point we have raycast to a series of different objects bounding boxes.
			// we need to test these different objects to see which is the first polygon hit.
			// there are some minor optimizations (distance based) that mean we wont have to
			// check all of the objects most of the time, but the worst case scenario is that
			// we need to test every triangle of every object.
			float closest_distance = max_distance;
			Echo::Vector3 closest_result;
			Echo::RaySceneQueryResult& query_result = g_pRaySceneQuery->getLastResults();

			for (size_t qr_idx = 0; qr_idx < query_result.size(); qr_idx++)
			{
				// stop checking if we have found a raycast hit that is closer
				// than all remaining entities
				if ((closest_distance >= 0.0f) && (closest_distance < query_result[qr_idx].distance))
					break;

				// only check this result if its a hit against an entity
				if ((query_result[qr_idx].movable != NULL) && (query_result[qr_idx].movable->getMovableType() == Echo::MovableObjectType_Entity))
				{
					// get the entity to check
					Echo::Entity* pentity = static_cast<Echo::Entity*>(query_result[qr_idx].movable);

					if (keyval && pentity->getName().toString().find("_roadmesh") == Echo::String::npos)
						continue;

					if (!(pentity->getVisibilityFlags() & mVisibilityMask))
						continue;

					if (!pentity->getVisible())
						continue;

					bool new_closest_found = false;

					Echo::Vector3 hitnrm = Echo::Vector3::ZERO;
					Echo::Vector3 outPos = Echo::Vector3::ZERO;

					float hit_dis = std::numeric_limits<float>::infinity();
					bool hit = RoadManager::instance()->GetPointPickEntity(query_result[qr_idx].movable, ray, outPos, hit_dis);
					if (hit)
					{
						if ((closest_distance < 0.0f) || (hit_dis < closest_distance))
						{
							closest_distance = hit_dis;
							new_closest_found = true;
						}
					}

					// if we found a new closest raycast for this object, update the
					// closest_result before moving on to the next object.
					if (new_closest_found)
					{
						closest_result = outPos;
						(*result) = pentity;
					}
				}
			}
			// return the result
			if (closest_distance != max_distance)
			{
				hitpoint = closest_result;
				return (true);
			}
			else
			{
				// raycast failed
				return (false);
			}
		}
#else
		bool PickEntity(Echo::Ray& ray, Echo::Entity** result, Echo::Vector3& hitpoint, Echo::uint32 uQueryMask, float max_distance, bool keyval) { return false; }

#endif // ECHO_EDITOR

		std::pair<Vector3, bool> ToolGetHeight(Ray& ray, bool pickEntity)
		{
			//Ray ray = Ray(point, Vector3::NEGATIVE_UNIT_Y);
			Vector3 point = ray.getOrigin();
			Vector3 LastPos;
			float distance = -1.f;
			float terrlen = 0.f;
			bool isTerrain = true;
			TerrainSystem::instance()->requestHeightMipLevel(point.x, point.z, 0, 0.f, true);
			TerrainSystem::instance()->getHeight(point, terrlen);
			if (terrlen != 0.f)
			{
				LastPos = point;
				LastPos.y = terrlen;
				distance = (LastPos - point).length();
				if (ray.getDirection() == Vector3::UNIT_Y)
					distance = std::numeric_limits<float>::infinity();
			}

#ifdef ECHO_EDITOR
			if (pickEntity) {
				Echo::Entity* result = nullptr;
				Vector3 hitPos;
				if (PickEntity(ray, &result, hitPos, 2, distance))
				{
					LastPos = hitPos;
					isTerrain = false;
				}
			}
#endif // ECHO_EDITOR

			return { LastPos , isTerrain };
		}
	}

	PiecewiseBezierUtil::PiecewiseBezierUtil(SceneManager* sceneManager)
	{
		mSceneManager = sceneManager;
		mNode = mSceneManager->createSceneNode();
		mNode->setVisible(false);
	}

	PiecewiseBezierUtil::~PiecewiseBezierUtil()
	{
		Clear();
		if (mNode != nullptr)
		{
			mNode->detachAllObject();
			mNode->getCreator()->destroySceneNode(mNode);
			mNode = nullptr;
		}

		for (TrafficObject* mObject : mObjects)
		{
			//scenemgr()->destroyManualObject(mObject);
			delete mObject;
			mObject = nullptr;
			mAABBs.clear();
		}
		mObjects.clear();

		for (BoxHelper* helper : mPointHelpers)
			delete helper;
		mPointHelpers.clear();

		for (SphereHelper* helper : mSourceControlHelpers)
			delete helper;
		mSourceControlHelpers.clear();

		for (SphereHelper* helper : mDestinationControlHelpers)
			delete helper;
		mDestinationControlHelpers.clear();

		for (ArrowHelper* helper : mCenterNormalHelpers)
			delete helper;
		mCenterNormalHelpers.clear();
	}

	float PiecewiseBezierUtil::GetPickHeight(const Vector3& point)
	{
		Ray ray(point, Vector3::NEGATIVE_UNIT_Y);
		return PiecewiseTool::ToolGetHeight(ray, true).first.y;
	}

	Vector3 PiecewiseBezierUtil::GetPickPos(Ray& ray)
	{
		return PiecewiseTool::ToolGetHeight(ray).first;
	}

	void PiecewiseBezierUtil::DefaultHelper(const Vector3& offset)
	{
		/*if (mNode == nullptr)
			mNode = mSceneManager->createSceneNode();*/

		mPartLengths.clear();

		ClearHelper(offset);
	}

	void PiecewiseBezierUtil::ClearHelper(const Vector3& offset)
	{
		if (mNode != nullptr)
			mNode->removeAllChildren();

		if (!mObjects.empty())
		{
			for (auto& mObject : mObjects)
				delete mObject;
			//scenemgr()->destroyManualObject(mObject);
			mObjects.clear();
			mAABBs.clear();
		}

		for (auto helper : mPointHelpers)
			delete helper;
		mPointHelpers.clear();

		for (auto helper : mSourceControlHelpers)
			delete helper;
		mSourceControlHelpers.clear();

		for (auto helper : mDestinationControlHelpers)
			delete helper;
		mDestinationControlHelpers.clear();

		for (ArrowHelper* helper : mCenterNormalHelpers)
			delete helper;
		mCenterNormalHelpers.clear();

		//mNode->setPosition(offset);
		if (mNode)
			mNode->_update(true, true);
	}

	void PiecewiseBezierUtil::UpdateHelper(size_t pickIndex, bool AdTerrain, const Vector3& offset, PiecewiseTool::ActionType action, PiecewiseTool::PiecewiseType linetype)
	{
		assert(mHaveValue);
		int index = (int)pickIndex - 1;
		if (!(action & PiecewiseTool::DELETE_ACTION))
			assert((pickIndex < mPoint.size()) && "The UpdateHelper pickIndex out of mPoints range!");

		PiecewiseTool::PiecewiseBezier place;
		place = CalculatePlace(pickIndex);

		/*bool isVisible = true;
		switch (linetype)
		{
		case PiecewiseTool::Curve:
			isVisible = true; break;
		case PiecewiseTool::StraightLine:
			isVisible = false; break;
		default:
			isVisible = true; break;
		}*/

		UpdateHelper(action, GetPoint(pickIndex), GetDestination(pickIndex), GetSource(pickIndex), GetNormal(pickIndex), offset,
			GetPoint(index), GetDestination(index), GetSource(index), place, pickIndex, AdTerrain, mNeedVisit);
	}

	void PiecewiseBezierUtil::UpdateHelper(PiecewiseTool::ActionType action,
		const Vector3& point, const Vector3& destination, const Vector3& source, const Vector3& normal, 
		const Vector3& offset, const Vector3& point_bf, const Vector3& destination_bf, const Vector3& source_bf,
		PiecewiseTool::PiecewiseBezier place, size_t index, bool AdTerrain, bool isVisiable)
	{
		switch (action)
		{
		case PiecewiseTool::CREATE_ACTION:
		{
			mObjects.push_back(new TrafficObject(mSceneManager->generateMovableName()));
			mNode->attachObject(mObjects[index]);

			mObjects[index]->setVisible(mNeedVisit);

			mAABBs.emplace_back();
			//mLiffts.emplace_back();
			
			BoxHelper* _box = new BoxHelper(mSceneManager);
			_box->setBox(0.5f, 0.5f, 0.5f);
			_box->setPosition(point + offset);
			_box->setColor((mPointHelpers.size() % 2 == 0) ? Echo::ColorValue(0.0f, 0.4f, 0.8f, 0.8f) : Echo::ColorValue(0.5f, 0.7f, 0.9f, 0.8f));
			mPointHelpers.push_back(_box);
			mNode->addChild(_box->GetSceneNode());

			ArrowHelper* _arrow = new ArrowHelper(mSceneManager);
			_arrow->setPosition(point + offset);
			_arrow->setColor((mCenterNormalHelpers.size() % 2 == 0) ? Echo::ColorValue(0.0f, 0.4f, 0.8f, 0.8f) : Echo::ColorValue(0.5f, 0.7f, 0.9f, 0.8f));
			_arrow->setRadius(0.2f);
			_arrow->SetNormal(normal);
			_arrow->setVisible(true);

			mCenterNormalHelpers.push_back(_arrow);
			mNode->addChild(_arrow->GetSceneNode());

			switch (place)
			{
			case PiecewiseTool::Begin:
			{
				SphereHelper* _dest_sphere = new SphereHelper(mSceneManager);
				_dest_sphere->setRadius(0.2f);
				_dest_sphere->setPosition(point + destination + offset);
				_dest_sphere->setColor(mDestinationColor);
				_dest_sphere->setVisible(true);
				mDestinationControlHelpers.push_back(_dest_sphere);
				mSourceControlHelpers.push_back(new SphereHelper(mSceneManager));
				mNode->addChild(_dest_sphere->GetSceneNode());
			}break;
			case PiecewiseTool::Middle:
			{
				SphereHelper* _src_sphere = new SphereHelper(mSceneManager);
				_src_sphere->setRadius(0.2f);
				_src_sphere->setPosition(point + source + offset);
				_src_sphere->setColor(mSourceColors[mSourceControlHelpers.size()]);
				_src_sphere->setVisible(true);
				mSourceControlHelpers.push_back(_src_sphere);
				mNode->addChild(_src_sphere->GetSceneNode());

				SphereHelper* _dest_sphere = new SphereHelper(mSceneManager);
				_dest_sphere->setRadius(0.2f);
				_dest_sphere->setPosition(point + destination + offset);
				_dest_sphere->setColor(mDestinationColor);
				_dest_sphere->setVisible(true);
				mDestinationControlHelpers.push_back(_dest_sphere);
				mNode->addChild(_dest_sphere->GetSceneNode());
			}break;
			case PiecewiseTool::End:
			{
				SphereHelper* _src_sphere = new SphereHelper(mSceneManager);
				_src_sphere->setRadius(0.2f);
				_src_sphere->setPosition(point + source + offset);
				_src_sphere->setColor(mSourceColors[mSourceControlHelpers.size()]);
				_src_sphere->setVisible(true);
				mSourceControlHelpers.push_back(_src_sphere);
				mDestinationControlHelpers.push_back(new SphereHelper(mSceneManager));
				mNode->addChild(_src_sphere->GetSceneNode());
			}break;
			default: break;
			}

		} break;
		case PiecewiseTool::REMOVE_ACTION:
		{
			mObjects[index]->clear();
			mAABBs[index].setDefault();
			//mLiffts[index] = 0.f;

			if (index > 0 || (place & PiecewiseTool::Middle))
			{
				mObjects[index - 1]->clear();
				mAABBs[index - 1].setDefault();
			}

			mPointHelpers[index]->setPosition(point + offset);
			mCenterNormalHelpers[index]->setPosition(point + offset);
			mCenterNormalHelpers[index]->SetNormal(normal);
			mCenterNormalHelpers[index]->setVisible(mNeedVisit);

			switch (place)
			{
			case PiecewiseTool::Begin:
			{
				mDestinationControlHelpers[index]->setPosition(point + destination + offset);
			}break;
			case PiecewiseTool::Middle:
			{
				mSourceControlHelpers[index]->setPosition(point + source + offset);
				mDestinationControlHelpers[index]->setPosition(point + destination + offset);
			}break;
			case PiecewiseTool::End:
			{
				mSourceControlHelpers[index]->setPosition(point + source + offset);
			}break;
			default: break;
			}

		} break;
		case PiecewiseTool::DELETE_ACTION:
		{
			mNode->detachObject(mObjects[index]);
			mObjects.erase(mObjects.begin() + index);
			mAABBs.erase(mAABBs.begin() + index);

			if (mBfTangents.size() > index)
			{
				mBfTangents.erase(std::next(mBfTangents.begin(), index));
				mBfNormals.erase(std::next(mBfNormals.begin(), index));
				mBfPoints.erase(std::next(mBfPoints.begin(), index));
			}
			//mLiffts.erase(mLiffts.begin() + index);

			/*if (index < mObjects.size())
				mObjects[index]->clear();*/
			if (int(index - 1) >= 0)
				mObjects[index - 1]->clear();

			if (index == mObjects.size() && index != 0)
			{
				mNode->removeChild(mDestinationControlHelpers[index - 1]->GetSceneNode());
				delete mDestinationControlHelpers[index - 1];
				mDestinationControlHelpers[index - 1] = new SphereHelper(mSceneManager);
			}
			if (index == 0 && mObjects.size() > 0)
			{
				mNode->removeChild(mSourceControlHelpers[index + 1]->GetSceneNode());
				delete mSourceControlHelpers[index + 1];
				mSourceControlHelpers[index + 1] = new SphereHelper(mSceneManager);
			}

			mNode->removeChild(mPointHelpers[index]->GetSceneNode());
			mNode->removeChild(mSourceControlHelpers[index]->GetSceneNode());
			mNode->removeChild(mDestinationControlHelpers[index]->GetSceneNode());
			mNode->removeChild(mCenterNormalHelpers[index]->GetSceneNode());
			delete mPointHelpers[index];
			delete mSourceControlHelpers[index];
			delete mDestinationControlHelpers[index];
			delete mCenterNormalHelpers[index];
			mPointHelpers.erase(mPointHelpers.begin() + index);
			mSourceControlHelpers.erase(mSourceControlHelpers.begin() + index);
			mDestinationControlHelpers.erase(mDestinationControlHelpers.begin() + index);
			mCenterNormalHelpers.erase(mCenterNormalHelpers.begin() + index);

			if (mObjects.size() == 1)
			{
				SphereHelper* _dest_sphere = new SphereHelper(mSceneManager);
				_dest_sphere->setRadius(0.2f);
				_dest_sphere->setPosition(point + destination + offset);
				_dest_sphere->setColor(mDestinationColor);
				_dest_sphere->setVisible(true);
				mDestinationControlHelpers[0] = _dest_sphere;
				mSourceControlHelpers[0] = new SphereHelper(mSceneManager);
				mNode->addChild(_dest_sphere->GetSceneNode());
			}
		} break;
		/*case PiecewiseTool::INSERT_ACTION:
		{

		}break;*/
		default: break;
		}

		if (index > 1 && (action & PiecewiseTool::CREATE_ACTION))
		{
			if (mDestinationControlHelpers[index - 1]->isInitData())
			{
				SphereHelper* _dest_sphere = new SphereHelper(mSceneManager);
				_dest_sphere->setRadius(0.2f);
				_dest_sphere->setPosition(point_bf + destination_bf + offset);
				_dest_sphere->setColor(mDestinationColor);
				_dest_sphere->setVisible(true);
				mNode->removeChild(mDestinationControlHelpers[index - 1]->GetSceneNode());
				delete mDestinationControlHelpers[index - 1];
				mDestinationControlHelpers[index - 1] = _dest_sphere;
				mNode->addChild(_dest_sphere->GetSceneNode());
			}
		}

		/*if (!(action & PiecewiseTool::DELETE_ACTION) && AdTerrain)
		{
			Echo::Vector3 point = mPointHelpers[index]->getPosition();
			float TerrainHeight = 0.f;
			TerrainHeight = PiecewiseTool::ToolGetHeight(point);
			point.y = TerrainHeight;
			mPointHelpers[index]->setPosition(point);
			point = point - offset;
		}*/

		mNode->_update(true, true);
	}

	void PiecewiseBezierUtil::UpdateHelper(
		PiecewiseTool::ActionType action, const Vector3& point_bf, const Vector3& point,
		PiecewiseTool::PiecewiseBezier place, size_t index, bool AdTerrain, const Vector3& offset)
	{
		switch (action)
		{
		case PiecewiseTool::CREATE_ACTION:
		{
			mObjects.push_back(new TrafficObject(mSceneManager->generateMovableName()));
			mNode->attachObject(mObjects[index]);
			mObjects[index]->setRenderDistance(10000.0f);

			mObjects[index]->setVisible(mNeedVisit);

			mAABBs.emplace_back();

			BoxHelper* _box = new BoxHelper(mSceneManager);
			_box->setBox(0.5f, 0.5f, 0.5f);
			_box->setPosition(point + offset);
			_box->setVisible(true);
			_box->setColor(Echo::ColorValue(0.0f, 0.4f, 0.8f, 0.8f));
			mPointHelpers.push_back(_box);
			mNode->addChild(_box->GetSceneNode());
		} break;
		case PiecewiseTool::REMOVE_ACTION:
		{
			mObjects[index]->clear();
			mAABBs[index].setDefault();

			if (index > 0 || (place & PiecewiseTool::Middle))
			{
				mObjects[index - 1]->clear();
				mAABBs[index - 1].setDefault();
			}

			mPointHelpers[index]->setPosition(point + offset);
		} break;
		case PiecewiseTool::DELETE_ACTION:
		{
			mNode->detachObject(mObjects[index]);
			mObjects.erase(mObjects.begin() + index);
			mAABBs.erase(mAABBs.begin() + index);

			if (int(index - 1) >= 0)
				mObjects[index - 1]->clear();

			mNode->removeChild(mPointHelpers[index]->GetSceneNode());
			delete mPointHelpers[index];
			mPointHelpers.erase(mPointHelpers.begin() + index);
		} break;
		/*case PiecewiseTool::INSERT_ACTION:
		{

		}break;*/
		default:
			break;
		}

		mNode->_update(true, true);
	}

	void PiecewiseBezierUtil::UpdateBezierLine(size_t pickIndex, bool AdTerrain, const Vector3& offset)
	{
		assert(mHaveValue);
		size_t index = size_t(pickIndex + 1);
		if (pickIndex >= mPoint.size()) return ;
		//assert((pickIndex < mPoint.size()) && "The UpdateBezierLine pickIndex out of mPoints range!");

		PiecewiseTool::PiecewiseBezier place;
		if (pickIndex == 0)
			place = PiecewiseTool::PiecewiseBezier::Begin;
		else if (pickIndex == mPoint.size() - 1)
			place = PiecewiseTool::PiecewiseBezier::End;
		else
			place = PiecewiseTool::PiecewiseBezier::Middle;

		if (index < mPoint.size())
			UpdateBezierLine(
				mPoint[pickIndex], mDestinationControl[pickIndex], mSourceControl[pickIndex],
				mPoint[index], mSourceControl[index], pickIndex, AdTerrain, place, offset);
		else
			UpdateBezierLine(
				mPoint[pickIndex], mDestinationControl[pickIndex], mSourceControl[pickIndex],
				Vector3::ZERO, Vector3::ZERO, pickIndex, AdTerrain, place, offset);
	}

	void PiecewiseBezierUtil::UpdateBezierLine(
		const Vector3& from, const Vector3& from_des, const Vector3& from_sou,
		const Vector3& to, const Vector3& to_sou, size_t index, bool AdTerrain,
		PiecewiseTool::PiecewiseBezier place, const Vector3& offset)
	{
		auto& object = mObjects[index];
		object->setRenderDistance(10000.0f);
		object->setVisible(mNeedVisit);
		object->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_STRIP);
		{
			CalculateCurve(from, from_des, from_sou, to, to_sou, index, AdTerrain, place, offset, object);
			
			object->getSection(0)->setUsetMaterialDiffuse(true);
			object->getSection(0)->setUseDepthWrite(false);
			object->getSection(0)->generateShaderMask();
		}
		object->end();

		mNode->_update(true, true);
	}

	void PiecewiseBezierUtil::CalculateCurve(
		const Vector3& from, const Vector3& from_des, const Vector3& from_sou,
		const Vector3& to, const Vector3& to_sou, size_t index, bool AdTerrain,
		PiecewiseTool::PiecewiseBezier place, const Vector3& offset, TrafficObject* obejct)
	{
		obejct->position(from);
		obejct->colour(Echo::ColorValue::Blue);
		switch (place)
		{
		case PiecewiseTool::Begin:
		{
			obejct->position(from + from_des);
			obejct->position(from);
		}break;
		case PiecewiseTool::Middle:
		{
			obejct->position(from + from_sou);
			obejct->position(from);
			obejct->position(from + from_des);
			obejct->position(from);
		}break;
		case PiecewiseTool::End:
		{
			obejct->position(from + from_sou);
			obejct->position(from);
		}break;
		default: break;
		}

		// 最后一个点无法构成线段，所以只存储控制点信息
		if ((place != PiecewiseTool::End) &&
			!(place & PiecewiseTool::Begin && mPoint.size() == 1))
		{
			if (mPartLengths[index] == 0.f)
				mPartLengths[index] = LengthFromTime(from, from_des, to, to_sou, 1.f);

			if(m_pBezierPoint)
			{
				while (m_pBezierPoint->size() <= index)
				{
					std::vector<Vector3> spoint;
					m_pBezierPoint->push_back(spoint);
				}
				(*m_pBezierPoint)[index].clear();
			}


			float PiecewiseLength = mPartLengths[index];
			int segment = (int)std::ceil(PiecewiseLength);
			float segment_length = PiecewiseLength / segment;
			float step = segment_length / PiecewiseLength;
			float t = 0.f;

			for (int s = 0; s <= segment; ++s, t += step)
			{
				Echo::Vector3 point = CInterpolate(from, from_des, to, to_sou, t);
				if (false && AdTerrain)
				{
					/*point = point + offset;
					point.y += 0.1f;
					float TerrainHeight = 0.f;
					TerrainHeight = PiecewiseTool::ToolGetHeight(point).first;
					point.y = TerrainHeight;
					point -= offset;*/
				}

				/*{
					Echo::Vector3 position = point + offset;
					Ray ray(position, Vector3::UNIT_Y);
					float maxlength = PiecewiseTool::calculateMinTerrainLength(ray, Vector3::ZERO, Matrix3::IDENTITY, 
						true, Root::instance()->getMainSceneManager()->generateMovableName().toString());
					float maxL = (maxlength - position.y > 0 ? maxlength - position.y : 0.f);
				}*/

				mAABBs[index].merge(point);
				obejct->position(point);
				(*m_pBezierPoint)[index].push_back(point);
				if (s == 0) obejct->colour(Echo::ColorValue::Green);
			}
		}
	}

	void PiecewiseBezierUtil::CalculateLine(const Vector3& from, const Vector3& to, size_t index, bool AdTerrain,
		PiecewiseTool::PiecewiseBezier place, const Vector3& offset, TrafficObject* object)
	{
		if ((place != PiecewiseTool::End) &&
			!(place & PiecewiseTool::Begin && mPoint.size() == 1))
		{
			object->position(from);
			object->colour(Echo::ColorValue::Green);
			object->position(to);

			mAABBs[index].merge(from);
			mAABBs[index].merge(to);

			if (mPartLengths[index] == 0.f)
				mPartLengths[index] = from.distance(to);
		}
	}

	void PiecewiseBezierUtil::UpdatePartLengths(
		PiecewiseTool::ActionType action, size_t pick_index,
		size_t other_index, bool flag)
	{
		if (flag)
		{
			switch (action)
			{
			case PiecewiseTool::CREATE_ACTION:
				mPartLengths.emplace_back();
				break;
			case PiecewiseTool::REMOVE_ACTION:
				if (mPartLengths.size() > pick_index)
					mPartLengths[pick_index] = 0.f;
				if (int(pick_index - 1) >= 0)
					mPartLengths[pick_index - 1] = 0.f;
				break;
			case PiecewiseTool::DELETE_ACTION:
				if (pick_index == mPartLengths.size() - 1)
					mPartLengths.erase(std::next(mPartLengths.begin(), pick_index - 1));
				if (mPartLengths.size() > pick_index)
				{
					mPartLengths.erase(std::next(mPartLengths.begin(), pick_index));
					if (int(pick_index) - 1 >= 0)
						mPartLengths[pick_index - 1] = 0.f;
				}
				break;
			default:
				break;
			}
		}
		else
		{

		}
	}

	void PiecewiseBezierUtil::HighlightPoint(const Echo::Ray& ray)
	{
		ControlHelperRenderable* _helper = nullptr;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (auto helper : mPointHelpers)
		{
			helper->setHighlight(false);
			auto res = helper->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = helper;
				closest_distance = res.second;
			}
		}

		for (ArrowHelper* helper : mCenterNormalHelpers)
		{
			helper->setHighlight(false);
			auto res = helper->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = helper;
				closest_distance = res.second;
			}
		}

		for (auto helper : mSourceControlHelpers)
		{
			if (!helper->isInitData())
			{
				helper->setHighlight(false);
				auto res = helper->hitTest(ray);
				if (res.first && res.second < closest_distance)
				{
					_helper = helper;
					closest_distance = res.second;
				}
			}
		}

		for (auto helper : mDestinationControlHelpers)
		{
			if (!helper->isInitData())
			{
				helper->setHighlight(false);
				auto res = helper->hitTest(ray);
				if (res.first && res.second < closest_distance)
				{
					_helper = helper;
					closest_distance = res.second;
				}
			}
		}

		if (_helper != nullptr)
			_helper->setHighlight(true);
	}

	void PiecewiseBezierUtil::KeepScale(const Vector3& cam_pos)
	{
		// 控制点随屏幕距离缩放
		for (size_t i = 0; i < mPointHelpers.size(); ++i)
		{
			auto& point = mPointHelpers[i]->getWorldPosition();
			float dist = cam_pos.distance(point);
			dist *= 0.05f;
			if (mPointHelpers[i]->getScale() != dist)
				mPointHelpers[i]->setScale(dist);

			if (mCenterNormalHelpers[i]->getScale() != dist)
				mCenterNormalHelpers[i]->setScale(dist);

			if (mDestinationControlHelpers.size() > i)
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
			}

		}
	}

	bool PiecewiseBezierUtil::PickHelper(const Ray& ray, size_t& pickIndex, PiecewiseTool::ControlType& controlType)
	{
		ControlHelperRenderable* _helper = nullptr;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (auto helper : mPointHelpers)
		{
			auto res = helper->hitTest(ray);
			if (res.first && res.second < closest_distance)
			{
				_helper = helper;
				closest_distance = res.second;
				controlType = PiecewiseTool::ControlType::BoxControl;
			}
		}

		for (ArrowHelper* helper : mCenterNormalHelpers)
		{
			if (helper->getVisible())
			{
				auto res = helper->hitTest(ray);
				if (res.first && res.second < closest_distance)
				{
					_helper = helper;
					closest_distance = res.second;
					controlType = PiecewiseTool::ControlType::CenterNormal;
				}
			}
		}

		for (auto helper : mSourceControlHelpers)
		{
			if (!helper->isInitData() && helper->getVisible())
			{
				auto res = helper->hitTest(ray);
				if (res.first && res.second < closest_distance)
				{
					_helper = helper;
					closest_distance = res.second;
					controlType = PiecewiseTool::ControlType::SourceControl;
				}
			}
		}

		for (auto helper : mDestinationControlHelpers)
		{
			if (!helper->isInitData() && helper->getVisible())
			{
				auto res = helper->hitTest(ray);
				if (res.first && res.second < closest_distance)
				{
					_helper = helper;
					closest_distance = res.second;
					controlType = PiecewiseTool::ControlType::DestinationControl;
				}
			}
		}

		switch (controlType)
		{
		case PiecewiseTool::BoxControl:
			pickIndex = std::find(mPointHelpers.begin(), mPointHelpers.end(), _helper) - mPointHelpers.begin();
			break;
		case PiecewiseTool::DestinationControl:
			pickIndex = std::find(mDestinationControlHelpers.begin(), mDestinationControlHelpers.end(), _helper) - mDestinationControlHelpers.begin();
			break;
		case PiecewiseTool::SourceControl:
			pickIndex = std::find(mSourceControlHelpers.begin(), mSourceControlHelpers.end(), _helper) - mSourceControlHelpers.begin();
			break;
		case PiecewiseTool::CenterNormal:
			pickIndex = std::find(mCenterNormalHelpers.begin(), mCenterNormalHelpers.end(), _helper) - mCenterNormalHelpers.begin();
			break;
		default:
			pickIndex = mPointHelpers.size();
			break;
		}

		return pickIndex != mPointHelpers.size();
	}

	float PiecewiseBezierUtil::LengthsTot(float s, size_t road_part, float t) const
	{
		assert(mHaveValue);
		size_t index = size_t(road_part + 1);
		assert((index < mPoint.size()) && "The LengthsTot piece_index out of mPoints range!");

		float BezierLens = mPartLengths[road_part];
		/*LengthFromTime(
		mPoint[road_part], mDestinationControl[road_part],
		mPoint[index], mSourceControl[index], 1.f);*/

		if (s <= 0.f)
		{
			if (road_part == 0 || s == 0.f)
				return 0.f;
			else
			{
				road_part -= 1;
				s = BezierLens + s;
			}
		}

		return LengthsTot(
			mPoint[road_part], mDestinationControl[road_part],
			mPoint[index], mSourceControl[index], s, t);
	}

	float PiecewiseBezierUtil::LengthsTot(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float s, float t)
	{
		assert((s >= 0.f) && "LengthsTot s must larger than 0");
		assert((t >= 0.f) && "LengthsTot t must larger than 0");

		float BezierLens = PiecewiseBezierUtil::LengthFromTime(from, from_des, to, to_sou, 1.f);
		if (s >= BezierLens) return 1.f;

		t = (t == 0.f ? s / BezierLens : t);
		float tlower = 0.f, tupper = 1.f;
		for (int i = 0; i < 10; ++i)
		{
			float v = PiecewiseBezierUtil::LengthFromTime(from, from_des, to, to_sou, t);
			float F = v - s;
			if (abs(F) <= 1e-6 || t >= 1.0f || t < 0.f) return t;
			float DFDT = PiecewiseBezierUtil::CTangent(from, from_des, to, to_sou, t).length();
			float tC = t - F / DFDT;
			if (F > 0)
			{
				tupper = t;
				if (tC <= tlower)
					t = (tupper + tlower) / 2.f;
				else
					t = tC;
			}
			else
			{
				tlower = t;
				if (tC >= tupper)
					t = (tupper + tlower) / 2.f;
				else
					t = tC;
			}
		}
		return t;
	}

	float PiecewiseBezierUtil::LengthFromTime(float time, size_t piece_index) const
	{
		assert(mHaveValue);
		size_t index = size_t(piece_index + 1);
		assert((index < mPoint.size()) && "The LengthFromTime piece_index out of mPoint range");

		return LengthFromTime(
			mPoint[piece_index], mDestinationControl[piece_index],
			mPoint[index], mSourceControl[index], time);
	}

	float PiecewiseBezierUtil::LengthFromTime(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float time)
	{
		assert((time >= 0.f) && "The LengthFromTime t must in range [0, 1]!");

		float z = time / 2.f;
		float length = 0;
		for (int i = 0; i < sizeof(C) / sizeof(double); ++i)
		{
			float t = z * (float)T[i] + z;
			if (t >= 1.f) t = 1.f;
			float v = CTangent(from, from_des, to, to_sou, t).length();
			length += (float)C[i] * v;
		}
		return z * length;
	}

	Vector3 PiecewiseBezierUtil::Interpolate(float all_t) const
	{
		assert(mHaveValue);
		assert((all_t >= 0.f && all_t <= 1.f) && "The interpolate t must in range [0, 1]!");

		const size_t point_count = mPoint.size();
		if (0 == point_count)
			return Vector3::ZERO;
		else if (1 == point_count)
			return mPoint[0];

		if (all_t <= 0.f)
			return mPoint[0];
		else if (all_t >= 1.f)
			return mPoint[point_count - 1];

		size_t piece_count = point_count - 1;
		std::vector<float> piece_lengths(piece_count);
		float piece_length_total = 0.f;
		for (size_t p = 0; p < piece_count; ++p)
		{
			piece_lengths[p] = mPartLengths[p];//LengthFromTime(1.f, p);
			if (piece_lengths[p] <= 1e-6)
				piece_lengths[p] = 0.f;
			piece_length_total += piece_lengths[p];
		}

		unsigned piece_index = 0;
		float piece_t = 0.f, doublet = all_t;
		for (piece_index = 0; piece_index < piece_count && all_t > 0; ++piece_index)
		{
			float piece_ratio = piece_lengths[piece_index] / piece_length_total;
			float t_diff = doublet - piece_ratio;
			if (t_diff < 0)
			{
				piece_t = doublet / piece_ratio;
				break;
			}
			else
				doublet -= piece_ratio;
		}

		return Interpolate(piece_index, (float)piece_t);
	}

	Vector3 PiecewiseBezierUtil::Interpolate(unsigned int piece_index, float t) const
	{
		assert(mHaveValue);

		size_t index = size_t(piece_index + 1);
		assert((index < mPoint.size()) && "The interpolate piece_index out of mPoints range!");

		return CInterpolate(
			mPoint[piece_index], mDestinationControl[piece_index],
			mPoint[index], mSourceControl[index], t);
	}

	Vector3 PiecewiseBezierUtil::CInterpolate(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t)
	{
		if (t <= 0.f)
			return from;
		else if (t >= 1.f)
			return to;

		float t2 = t * t;
		float t3 = t2 * t;

		Vector4 tpowers(1, t, t2, t3);
		// 4 degree Bernstein polynomial
		static const DMatrix4 m4coeffs(
			1.0, 0.0, 0.0, 0.0,
			-3.0, 3.0, 0.0, 0.0,
			3.0, -6.0, 3.0, 0.0,
			-1.0, 3.0, -3.0, 1.0
		);

		const Vector3& p0 = from;
		const Vector3 p1 = p0 + from_des;
		const Vector3 p2 = to + to_sou;
		const Vector3& p3 = to;
		DMatrix4 pt(
			p0.x, p0.y, p0.z, 0.f,
			p1.x, p1.y, p1.z, 0.f,
			p2.x, p2.y, p2.z, 0.f,
			p3.x, p3.y, p3.z, 0.f
		);

		Vector4 ret = tpowers * m4coeffs.transpose() * pt.transpose();
		return Vector3(ret.x, ret.y, ret.z);
	}

	Vector3 PiecewiseBezierUtil::SInterpolate(const Vector3& from, const Vector3& to, float t)
	{
		if (t <= 0.f)
			return from;
		else if (t >= 1.f)
			return to;

		if (from != Vector3::ZERO && to != Vector3::ZERO)
			return from + (to - from) * t;
		else if (from != Vector3::ZERO)
			return from;
		else
			return to;
	}

	Vector3 PiecewiseBezierUtil::Tangent(float t) const
	{
		assert(mHaveValue);
		assert((t >= 0.f && t <= 1.f) && "The tangent t must in range [0, 1]!");

		const size_t point_count = mPoint.size();
		if (point_count <= 1)
			return Vector3::UNIT_X;

		size_t piece_count = point_count - 1;
		if (t <= 0.f)
			return Tangent(0, 0.f);
		else if (t >= 1.f)
			return Tangent((unsigned)piece_count - 1, 1.f);

		t = Math::Clamp(t, 0.f, 1.f);
		std::vector<double> piece_lengths(piece_count);
		double piece_length_total = 0.f;
		for (size_t p = 0; p < piece_count; ++p)
		{
			piece_lengths[p] = mPartLengths[p];//LengthFromTime(1.f, p);
			if (piece_lengths[p] <= 0.000001)
				piece_lengths[p] = 0.000001;
			piece_length_total += piece_lengths[p];
		}

		unsigned piece_index = 0;
		double piece_t = 0.f, doublet = t;
		for (piece_index = 0; piece_index < piece_count && t > 0; ++piece_index)
		{
			double piece_ratio = piece_lengths[piece_index] / piece_length_total;
			double t_diff = doublet - piece_ratio;
			if (t_diff < 0)
			{
				piece_t = doublet / piece_ratio;
				break;
			}
			else
				doublet -= piece_ratio;
		}

		return Tangent(piece_index, (float)piece_t);
	}

	Vector3 PiecewiseBezierUtil::Tangent(unsigned int piece_index, float t) const
	{
		assert(mHaveValue);

		size_t index = size_t(piece_index + 1);
		assert((index < mPoint.size()) && "The tangent piece_index out of mPoints range!");

		return CTangent(
			mPoint[piece_index], mDestinationControl[piece_index],
			mPoint[index], mSourceControl[index], t);
	}

	Vector3 PiecewiseBezierUtil::CTangent(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t)
	{
		assert((t >= 0.f && t <= 1.f) && "The interpolate t must in range [0, 1]!");

		Vector3 tpowers(1, t, t * t);
		// 4 degree Bernstein polynomial
		static const Matrix3 m3coeffs(
			1.0, 0.0, 0.0,
			-2.0, 2.0, 0.0,
			1.0, -2.0, 1.0
		);

		const Vector3& p0 = from;
		const Vector3 p1 = p0 + from_des;
		const Vector3 p2 = to + to_sou;
		const Vector3& p3 = to;
		const Vector3 w0 = 3 * (p1 - p0);
		const Vector3 w1 = 3 * (p2 - p1);
		const Vector3 w2 = 3 * (p3 - p2);

		Matrix3 pt(
			w0.x, w0.y, w0.z,
			w1.x, w1.y, w1.z,
			w2.x, w2.y, w2.z
		);

		//Vector3 ret = tpowers * m3coeffs * pt;
		return tpowers * m3coeffs * pt;
	}

	Vector3 PiecewiseBezierUtil::STangent(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t, PiecewiseTool::PiecewiseBezier place)
	{

		if (from != Vector3::ZERO && to != Vector3::ZERO)
		{
			switch (place)
			{
			case PiecewiseTool::Begin:
			{
				return Vector3::ZERO;
			}	break;
			case PiecewiseTool::Middle:
			{
				return to - from;
			}	break;
			case PiecewiseTool::End:
			{
				return Vector3::ZERO;
			}	break;
			default:
				return Vector3::ZERO;
			}
		}
		else if (from != Vector3::ZERO)
			return from;
		else
			return to;
	}

	void PiecewiseBezierUtil::CalculateControlNormal()
	{
		// 提前计算出所有控制点的法线信息。
		mBfTangents.clear();
		mBfNormals.clear();
		mBfPoints.clear();

		for (int i = 0; i < int(mPoint.size()) - 1; ++i)
		{
			Echo::Vector3 point = mPoint[i];
			Echo::Vector3 tangent = PiecewiseBezierUtil::CTangent(mPoint[i],
				mDestinationControl[i], mPoint[i + 1], mSourceControl[i + 1], 0.f).normalisedCopy();

			if (i == 0)
			{
				mBfNormals.push_back(Vector3(0.f, 1.f, 0.f));
			}
			else
			{
				size_t index = mBfTangents.size() - 1;

				Vector3 N, parnormal;
				Vector3 bef = mBfPoints[index];
				Vector3 bef_tangent = mBfTangents[index];
				Vector3 bef_normal = mBfNormals[index];
				PiecewiseBezierUtil::GetNormalInTangent(0, point, tangent, N, parnormal, bef, bef_tangent, bef_normal);

				mBfNormals.push_back(N);
			}

			mBfTangents.push_back(tangent);
			mBfPoints.push_back(point);
		}
	}

	void PiecewiseBezierUtil::CalculateNormal(int UpdateWay, size_t index, float length, 
		const Vector3& point, Vector3& tangent, Vector3& normal, Vector3& paranormal)
	{
		static Vector3 bf, bf_tangent, bf_normal;

		if (UpdateWay == 0)
			GetBfData(index, bf, bf_tangent, bf_normal);

		if (index + 1 >= mCenterNormal.size() || index > mPartLengths.size()) return;

		Vector3 vec = mCenterNormal[index + 1] - mCenterNormal[index];
		vec *= length / mPartLengths[index];
		vec += mCenterNormal[index];

		GetNormalInTangent(UpdateWay, point, tangent, normal, paranormal, bf, bf_tangent, bf_normal, vec);
	}

	Vector3 PiecewiseBezierUtil::CalcualteCenterNormal(float length, float all_length, Vector3 normal1, Vector3 normal2)
	{
		Vector3 world_n = normal1;
		if (world_n != normal2)
		{
			float nw = length / all_length;
			world_n = nw * (normal2 - normal1);
			world_n += normal1;
		}
		return world_n;
	}

	void PiecewiseBezierUtil::GetNormalInTangent(
		int UpdateWay, const Vector3& point, 
		Vector3& tangent, Vector3& normal, Vector3& paranormal,
		Vector3& bef, Vector3& bef_tangent, Vector3& bef_normal, const Vector3& world_n)
	{
		if (UpdateWay == 1)
			PiecewiseBezierUtil::CalculateTangent(tangent, normal, paranormal);
		else if (UpdateWay == 2)
			PiecewiseBezierUtil::CalculateNormal(tangent, normal, paranormal, world_n);
		else if (UpdateWay == 0)
		{
			PiecewiseBezierUtil::CalculateNormal(point, tangent, paranormal, normal, bef, bef_tangent, bef_normal);
			bef_tangent = tangent;
			bef_normal = normal;
			bef = point;
		}
	}

	void PiecewiseBezierUtil::CalculateTangent(const Vector3& tangent, const Vector3& normal, Vector3& T, Vector3& N)
	{
		Matrix3 mat;
		mat.SetFromToRotation(tangent, T);
		N = mat * normal;
		N.normalise();
	}

	void PiecewiseBezierUtil::CalculateTangent(Echo::Vector3& Tangent, Echo::Vector3& Normal, Echo::Vector3& ParNormal)
	{
		Echo::Vector3 WorldNormal = DVector3(0.0, 1.0, 0.0);
		Echo::Vector3 PlaneNormal = WorldNormal.crossProduct(Tangent);
		Echo::Vector3 PlaneTanget = PlaneNormal.crossProduct(WorldNormal);
		PlaneNormal = Tangent.crossProduct(WorldNormal);
		Tangent = PlaneTanget.normalisedCopy();
		Normal = WorldNormal.normalisedCopy();
		ParNormal = PlaneNormal.normalisedCopy();
	}

	void PiecewiseBezierUtil::CalculateNormal(Echo::Vector3 Tangent, Echo::Vector3& Normal, Echo::Vector3& ParNormal, const Vector3& pNormal)
	{
		static Vector3 mBfRay;
		static Vector3 mBfTangent;
		Echo::Vector3 WorldNormal = pNormal;
		Echo::Vector3 PlaneRay = Tangent.crossProduct(WorldNormal);
		Echo::Vector3 PlaneNormal;
		if (PlaneRay.length() > 1e-6)
		{
			mBfRay = PlaneRay;
			mBfTangent = Tangent;
			PlaneNormal = PlaneRay.crossProduct(Tangent);
			PlaneRay = Tangent.crossProduct(PlaneNormal);
		}
		else
		{
			PlaneNormal = mBfRay.crossProduct(Tangent);
			PlaneRay = Tangent.crossProduct(PlaneNormal);
		}
		Normal = PlaneNormal.normalisedCopy();
		ParNormal = PlaneRay.normalisedCopy();
	}

	/*
	Computation of Rotation Minimizing Frames
	每次计算当前顶点的法线信息都需要获取：
	当前顶点的坐标，切线数据，以及上一个顶点的坐标，切线，法线数据。
	*/
	void PiecewiseBezierUtil::CalculateNormal(
		const Vector3& PointNow, Vector3& TangentNow, Vector3& ParNormal, Vector3& NormalNow,
		const Vector3& FrontPoint, const Vector3& FrontTangent, const Vector3& FrontNormal)
	{
		Echo::Vector3 v1 = PointNow - FrontPoint;
		float c1 = v1.dotProduct(v1);

		if (c1 > 1e-5)
		{
			Echo::Vector3 r = FrontTangent.crossProduct(FrontNormal);
			ClearError(r);
			Echo::Vector3 riL = r - v1 * 2 / c1 * v1.dotProduct(r);
			ClearError(riL);
			Echo::Vector3 tiL = FrontTangent - v1 * 2 / c1 * v1.dotProduct(FrontTangent);
			ClearError(tiL);

			Echo::Vector3 v2 = TangentNow - tiL;
			ClearError(v2);
			float c2 = v2.dotProduct(v2);
			c2 = (c2 == 0 ? 1.f : c2);
			ParNormal = riL - v2 * 2 / c2 * v2.dotProduct(riL);
			ParNormal = ParNormal.normalisedCopy();
			ClearError(ParNormal);
			NormalNow = ParNormal.crossProduct(TangentNow).normalisedCopy();
		}
		else
		{
			TangentNow = FrontTangent;
			NormalNow = FrontNormal;
			ParNormal = FrontTangent.crossProduct(NormalNow).normalisedCopy();
			ClearError(ParNormal);
		}

		ClearError(NormalNow);
	}

	//--------------------------------------------------
	PiecewiseTool::PiecewiseBezier PiecewiseBezierUtil::CalculatePlace(size_t pick_index)
	{
		PiecewiseTool::PiecewiseBezier place;
		if (pick_index == 0)
			place = PiecewiseTool::PiecewiseBezier::Begin;
		else if (pick_index == mPoint.size() - 1)
			place = PiecewiseTool::PiecewiseBezier::End;
		else
			place = PiecewiseTool::PiecewiseBezier::Middle;
		return place;
	}

	void PiecewiseBezierUtil::SetPoints(const std::vector<Vector3>& point, const std::vector<Vector3>& source
		, const std::vector<Vector3>& destination, const std::vector<Vector3>& normal, const std::vector<bool>& pGray)
	{
		if (point.empty() || source.empty() || destination.empty() || normal.empty())
			return;

		if (mHaveValue)
		{
			mPoint.clear();
			mSourceControl.clear();
			mDestinationControl.clear();
			mCenterNormal.clear();
			mSourceColors.clear();
		}

		mPoint.insert(mPoint.begin(), point.begin(), point.end());
		mSourceControl.insert(mSourceControl.begin(), source.begin(), source.end());
		mDestinationControl.insert(mDestinationControl.begin(), destination.begin(), destination.end());
		mCenterNormal.insert(mCenterNormal.begin(), normal.begin(), normal.end());

		for (size_t i = 0; i < pGray.size(); ++i)
			mSourceColors.push_back(pGray[i] ? mSourceGrayColor : mSourceColor);

		mHaveValue = true;

		mPartLengths.clear();
		for (size_t i = 0; i < mPoint.size() - 1; ++i)
			mPartLengths.push_back(LengthFromTime(1.f, i));
	}

	void PiecewiseBezierUtil::SetPoint(size_t index, const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal)
	{
		assert(mHaveValue);
		assert((index < mPoint.size()) && "The SetPoint index out of range mPoint.size()");

		mPoint[index] = point;
		mSourceControl[index] = source;
		mDestinationControl[index] = destination;
		mCenterNormal[index] = normal;
	}

	void PiecewiseBezierUtil::SetPointOffset(const std::vector<float>& liffts)
	{
		assert(mHaveValue);
		assert((liffts.size() == mPoint.size()) && "The SetPointOffset liffts.size() out of range mPoint.size()");

		for (size_t index = 0; index < mPoint.size(); ++index)
		{
			mPoint[index].y += liffts[index];
		}
	}

	void PiecewiseBezierUtil::SetControlToLine(size_t index)
	{
		assert(mHaveValue);
		for (size_t i = 0; i < mPoint.size(); ++i)
		{
			if (index != -1) i = index;

			if (i == 0)
			{
				mDestinationControl[i] = (mPoint[i + 1] - mPoint[i]).normalisedCopy() * 5;
			}
			else if (i == mPoint.size() - 1)
			{
				mSourceControl[i] = (mPoint[i - 1] - mPoint[i]).normalisedCopy() * 5;
			}
			else
			{
				mDestinationControl[i] = (mPoint[i + 1] - mPoint[i - 1]).normalisedCopy() * 5;
				mSourceControl[i] = -mDestinationControl[i];
			}

			if (index != -1) break;
		}
	}

	void PiecewiseBezierUtil::AddPoint(const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal)
	{
		mPoint.push_back(point);
		mSourceControl.push_back(source);
		mDestinationControl.push_back(destination);
		mCenterNormal.push_back(normal);
		mSourceColors.push_back(mSourceGrayColor);
		//mDestinationColors.push_back(mDestinationColor);

		mHaveValue = true;
	}

	size_t PiecewiseBezierUtil::GetPointCount()
	{
		return mPoint.size();
	}

	Vector3 PiecewiseBezierUtil::GetPoint(size_t index) const
	{
		assert(mHaveValue);
		if (index < mPoint.size() && index >= 0)
			return mPoint[index];
		else
			return Vector3::ZERO;
	}

	Vector3 PiecewiseBezierUtil::GetSource(size_t index) const
	{
		assert(mHaveValue);
		if (index < mSourceControl.size() && index >= 0)
			return mSourceControl[index];
		else
			return Vector3::ZERO;
	}

	Vector3 PiecewiseBezierUtil::GetNormal(size_t index) const
	{
		assert(mHaveValue);
		if (index < mCenterNormal.size() && index >= 0)
			return mCenterNormal[index];
		else
			return Vector3::ZERO;
	}

	Vector3 PiecewiseBezierUtil::GetDestination(size_t index) const
	{
		assert(mHaveValue);
		if (index < mDestinationControl.size() && index >= 0)
			return mDestinationControl[index];
		else
			return Vector3::ZERO;
	}

	void PiecewiseBezierUtil::InsertPoint(size_t index, const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal)
	{
		assert((index <= mPoint.size()) && "The InsertPoint index out of range mPoint.size()");

		mPoint.insert(std::next(mPoint.begin(), index), point);
		mSourceControl.insert(std::next(mSourceControl.begin(), index), source);
		mDestinationControl.insert(std::next(mDestinationControl.begin(), index), destination);
		mCenterNormal.insert(std::next(mCenterNormal.begin(), index), normal);
		mSourceColors.insert(std::next(mSourceColors.begin(), index), mSourceGrayColor);
		//mDestinationColors.insert(std::next(mDestinationColors.begin(), index), mDestinationColor);
		mHaveValue = true;
	}

	void PiecewiseBezierUtil::DeletePoint(size_t index)
	{
		assert(mHaveValue);
		assert((index < mPoint.size()) && "The DeletePoint index out of range mPoint.size()");

		mPoint.erase(std::next(mPoint.begin(), index));
		mSourceControl.erase(std::next(mSourceControl.begin(), index));
		mDestinationControl.erase(std::next(mDestinationControl.begin(), index));
		mCenterNormal.erase(std::next(mCenterNormal.begin(), index));
		mSourceColors.erase(std::next(mSourceColors.begin(), index));
		//mDestinationColors.erase(std::next(mDestinationColors.begin(), index));
	}

	void PiecewiseBezierUtil::ClearPoint()
	{
		if (mHaveValue)
		{
			mPoint.clear();
			mSourceControl.clear();
			mDestinationControl.clear();
			mCenterNormal.clear();
			mSourceColors.clear();
			//mDestinationColors.clear();
			mHaveValue = false;
		}
	}

	void PiecewiseBezierUtil::Clear()
	{
		ClearPoint();
	}

	BoxHelper* PiecewiseBezierUtil::GetPickBoxHelepr(size_t pick)
	{
		return mPointHelpers.size() > pick ? mPointHelpers[pick] : nullptr;
	}

	SphereHelper* PiecewiseBezierUtil::GetControlHelper(size_t pick, PiecewiseTool::ControlType type)
	{
		if (type == PiecewiseTool::DestinationControl)
			return mDestinationControlHelpers.size() <= pick ? nullptr : mDestinationControlHelpers[pick];
		else if (type == PiecewiseTool::SourceControl)
			return mSourceControlHelpers.size() <= pick ? nullptr : mSourceControlHelpers[pick];
		return nullptr;
	}

	void PiecewiseBezierUtil::SetControlPoint(size_t pick, PiecewiseTool::ControlType type, const Vector3& vec)
	{
		if (type == PiecewiseTool::DestinationControl && mDestinationControl.size() > pick)
			mDestinationControl[pick] = vec;
		else if (type == PiecewiseTool::SourceControl && mSourceControl.size() > pick)
			mSourceControl[pick] = vec;
		else if (type == PiecewiseTool::BoxControl && mPoint.size() > pick)
			mPoint[pick] = vec;
	}

	void PiecewiseBezierUtil::SetArrowVisitable(bool visit)
	{
		mNeedVisit = visit;
		for (size_t i = 0; i < mCenterNormalHelpers.size(); ++i)
		{
			if (mCenterNormalHelpers[i] != nullptr)
				mCenterNormalHelpers[i]->setVisible(visit);
		}
	}

	void PiecewiseBezierUtil::SetSourceColor(size_t index, bool pGray)
	{
		if (index < mSourceColors.size())
		{
			ColorValue value = pGray ? mSourceGrayColor : mSourceColor;
			mSourceColors[index] = value;
			if (index < mSourceControlHelpers.size())
				mSourceControlHelpers[index]->setColor(value);
		}
	}

	void PiecewiseBezierUtil::SetSourceDefaultColor(const std::vector<bool> pGray)
	{
		mSourceGrayColor = Echo::ColorValue(m_oSourceColor.x / 255.f, 
			m_oSourceColor.y / 255.f, m_oSourceColor.z / 255.f, m_oSourceColor.w / 255.f);

		for (size_t i = 0; i < pGray.size(); ++i)
			SetSourceColor(i, pGray[i]);
	}

	void PiecewiseBezierUtil::SetTrajectoryData(const std::vector<Vector3>& points)
	{
		if (mHaveValue)
		{
			// 初始化所有数据
			DefaultHelper(Vector3::ZERO);
			Clear();
			ClearBfData();
		}

		for (const Vector3& point : points)
			AddTrajectoryData(point);
	}

	void PiecewiseBezierUtil::AddTrajectoryData(const Vector3& point)
	{
		AddPoint(point, Vector3::ZERO, Vector3::ZERO, Vector3::UNIT_Y);

		int point_index = (int)mPoint.size();
		if (point_index > 1)
		{
			point_index -= 1;
			CalculateControlPosition(point_index - 1);
			CalculateControlPosition(point_index);
			CalculateControlPosition(point_index + 1);
			CalculateControlPosition(point_index - 2);
			CalculateControlPosition(point_index + 2);
		}
	}

	void PiecewiseBezierUtil::CalculateControlPosition(int index)
	{
		int point_count = (int)mPoint.size();
		int p1 = index;
		if (p1 < 0 || p1 >= point_count) return;
		int p0 = index - 1;
		if (p0 < 0 || p0 >= point_count) p0 = index;
		int p2 = index + 1;
		if (p2 < 0 || p2 >= point_count) p2 = index;

		float l1 = mPoint[p0].distance(mPoint[p1]);
		float l2 = mPoint[p2].distance(mPoint[p1]);

		mSourceControl[p1] = l1 / (l1 + l2) * mWeight * (mPoint[p0] - mPoint[p2]);
		mDestinationControl[p1] = l2 / (l1 + l2) * mWeight * (mPoint[p2] - mPoint[p0]);
	}

	void PiecewiseBezierUtil::SetBfData(size_t index, const Vector3& point, const Vector3& tangent, const Vector3& normal)
	{
		if (mBfTangents.size() == index)
		{
			mBfNormals.push_back(normal);
			mBfTangents.push_back(tangent);
			mBfPoints.push_back(point);
		}
		else if(mBfTangents.size() > index)
		{
			mBfNormals[index] = normal;
			mBfTangents[index] = tangent;
			mBfPoints[index] = point;
		}
		else
			assert(!"[PiecewiseBezierUtil::SetBfData] Index value transfer error !");
	}

	void PiecewiseBezierUtil::CalculateBfData(std::vector<Vector3>& Tangents, std::vector<Vector3>& Normals)
	{
		mBfNormals.clear();
		mBfTangents.clear();
		//mBfPoints.clear();
		mBfNormals = Normals;
		mBfTangents = Tangents;
	}

	void PiecewiseBezierUtil::ClearBfData()
	{
		mBfNormals.clear();
		mBfTangents.clear();
		mBfPoints.clear();
	}

	void PiecewiseBezierUtil::GetBfData(size_t index, Vector3& point, Vector3& tangent, Vector3& normal)
	{
		if (mBfTangents.size() == 0)
		{
			mBfTangents.push_back(Vector3(1.f, 0.f, 0.f));
			mBfNormals.push_back(Vector3(0.f, 1.f, 0.f));
			mBfPoints.push_back(Vector3(0.f, 0.f, 0.f));
		}

		if (index < mBfTangents.size())
		{
			if (mBfPoints.size() == 0)
				mBfPoints.resize(mBfNormals.size());
			point = mBfPoints[index];
			tangent = mBfTangents[index];
			normal = mBfNormals[index];
		}
		else
			assert(!"[PiecewiseBezierUtil::GetBfData] Index value transfer error !");
	}
	void PiecewiseBezierUtil::SetHelperVisible(bool visible)
	{
		if (mNeedVisit == visible)
			return;
		mNeedVisit = visible;
		mNode->setVisible(mNeedVisit);
		for (auto& helper : mObjects)
		{
			helper->setVisible(mNeedVisit);
		}
		if (mNeedVisit)
			KeepScale(mSceneManager->getMainCamera()->getPosition());
		return;
	}
	std::pair<Vector3, Matrix3> PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float length,
		const Vector3& normal1, const Vector3& normal2, const Transform& transform,
		int CalculateType, const ActorComponentPtr& com, int fitGroundLevel1, int fitGroundLevel2, PavementFitTerrain* terrain)
	{
		std::unordered_map<float, float>tempdata;
		float t = PavementModelUtil::CalculatePointT(length, 0, tempdata, p0, p1, p3, p2);
		t = t > 1.f ? 1.f : t;
		t = t < 0.f ? 0.f : t;
		if (Math::FloatEqual(t, 0.0f, 1.0e-5f))
			t = 0.0f;

		return PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromT(p0, p1, p2, p3, t, normal1, normal2, transform, CalculateType, com, fitGroundLevel1, fitGroundLevel2, terrain);

	}
	std::pair<Vector3, Matrix3> PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromT(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t,
		const Vector3& normal1, const Vector3& normal2, const Transform& transform,
		int CalculateType, const ActorComponentPtr& com, int fitGroundLevel1, int fitGroundLevel2, PavementFitTerrain* terrain)
	{
		std::pair<Vector3, Matrix3> point;
		if (CalculateType > 2 || CalculateType < 0)
			CalculateType = 2;

		if (Math::FloatEqual(t, 0.0f, 1.0e-5f))
			t = 0.0f;
		//Matrix3 pointTransformMatrix;
		Vector3 pointAxis_X;
		Vector3 pointAxis_Y;
		Vector3 pointAxis_Z;

		float length = LengthFromTime(p0, p1, p3, p2, t);
		float Alllength = LengthFromTime(p0, p1, p3, p2, 1.0f);

		point.first = PiecewiseBezierUtil::CInterpolate(p0, p1, p3, p2, t);

		pointAxis_X = PiecewiseBezierUtil::CTangent(p0, p1, p3, p2, t).normalisedCopy();

		t = length / Alllength;

		if (Math::FloatEqual(t, 0.0f, 1.0e-5f))
			t = 0.0f;

		//顶点局部空间坐标轴计算
		{
			Vector3 Axis_Y = Vector3::UNIT_Y;

			if (CalculateType == 2 || CalculateType == 0)//依赖法线旋转方向计算
			{
				Axis_Y = PiecewiseBezierUtil::CalcualteCenterNormal(length, Alllength, normal1, normal2);
				PiecewiseBezierUtil::CalculateNormal(pointAxis_X, pointAxis_Y, pointAxis_Z, Axis_Y);
			}
			else if (CalculateType == 1)//只依赖世界坐标的y轴方向
			{
				PiecewiseBezierUtil::CalculateTangent(pointAxis_X, pointAxis_Y, pointAxis_Z);
			}

			if (pointAxis_X.IsZero())
				pointAxis_X = Vector3::UNIT_X;
			if (pointAxis_Y.IsZero())
				pointAxis_Y = Vector3::UNIT_Y;
			if (pointAxis_X.IsZero())
				pointAxis_Z = Vector3::UNIT_Z;
		}

		point.second = {
				pointAxis_X.x, pointAxis_Y.x, pointAxis_Z.x,
				pointAxis_X.y, pointAxis_Y.y, pointAxis_Z.y,
				pointAxis_X.z, pointAxis_Y.z, pointAxis_Z.z
		};

		point.first += point.second * transform.translation;

		if (com && terrain)
		{
			PavementFitTerrain::TerrainData data;
			terrain->getTerrainData(com->convertLocalToWorldPosition(point.first), data);

			if (fitGroundLevel1 != fitGroundLevel2 && (fitGroundLevel1 == -1 || fitGroundLevel2 == -1))
				data.terrainPos += data.terrainNormal * (Math::Max(fitGroundLevel1, fitGroundLevel2) * RoadManager::instance()->m_pavementFitGroundLevelHight);
			else
				data.terrainPos += data.terrainNormal * (fitGroundLevel1 > 0 ? (fitGroundLevel1 * RoadManager::instance()->m_pavementFitGroundLevelHight) : 0);

			if (fitGroundLevel1 != fitGroundLevel2)
			{
				float height, height_t;
				if (fitGroundLevel1 == -1 || fitGroundLevel2 == -1)
				{
					float height1, height2;
					PavementFitTerrain::TerrainData data1, data2;
					terrain->getTerrainData(com->convertLocalToWorldPosition(p3), data1);
					terrain->getTerrainData(com->convertLocalToWorldPosition(p0), data2);
					height1 = data1.terrainHeight;
					height2 = data2.terrainHeight;
					if (fitGroundLevel1 == -1)
					{
						height = height2 - height1;
						height_t = 1.0f - t;
					}
					else
					{
						height = height1 - height2;
						height_t = t;
					}

				}
				else
				{
					height = (fitGroundLevel2 - fitGroundLevel1) * RoadManager::instance()->m_pavementFitGroundLevelHight;
					if (t < 0.5)
						height_t = (height / Alllength) * t * t;
					else
						height_t = 1.0f - (height / Alllength) * (1.0f - t) * (1.0f - t);
				}

				data.terrainPos += data.terrainNormal * height_t * height;
			}
			data.terrainPos += data.terrainNormal * transform.translation.y;

			data.terrainPos = com->convertWorldToLocalPosition(data.terrainPos);

			data.terrainNormal = com->getWorldRotation() * data.terrainNormal;

			/*PiecewiseBezierUtil::CalculateNormal(pointAxis_X, pointAxis_Y, pointAxis_Z, data.terrainNormal);

			point.second = {
				pointAxis_X.x, pointAxis_Y.x, pointAxis_Z.x,
				pointAxis_X.y, pointAxis_Y.y, pointAxis_Z.y,
				pointAxis_X.z, pointAxis_Y.z, pointAxis_Z.z
			};*/

			if (fitGroundLevel1 != fitGroundLevel2)
			{
				Vector3 position = point.first;
				Vector3 normal = pointAxis_Y;

				position = Vector3::lerp(data.terrainPos, point.first, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));
				normal = Vector3::lerp(data.terrainNormal, pointAxis_Y, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));

				if (fitGroundLevel1 == -1 || fitGroundLevel2 == -1)
				{
					if (fitGroundLevel1 == -1)
					{
						position = Vector3::lerp(point.first, data.terrainPos, t);
						normal = Vector3::lerp(pointAxis_Y, data.terrainNormal, t);
					}
					else
					{
						position = Vector3::lerp(data.terrainPos, point.first, t);
						normal = Vector3::lerp(data.terrainNormal, pointAxis_Y, t);

					}

				}
				else
				{
					position = Vector3::lerp(data.terrainPos, point.first, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));
					normal = Vector3::lerp(data.terrainNormal, pointAxis_Y, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));
				}

				point.first = position;
				pointAxis_Y = normal;
			}
			else
			{
				point.first = data.terrainPos;
				pointAxis_Y = data.terrainNormal;
			}
			PiecewiseBezierUtil::CalculateNormal(pointAxis_X, pointAxis_Y, pointAxis_Z, std::move(Vector3(pointAxis_Y.x, pointAxis_Y.y, pointAxis_Y.z)));

			point.second = {
				pointAxis_X.x, pointAxis_Y.x, pointAxis_Z.x,
				pointAxis_X.y, pointAxis_Y.y, pointAxis_Z.y,
				pointAxis_X.z, pointAxis_Y.z, pointAxis_Z.z
			};
		}
		return point;
	}
	Vector3 PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float length,
		const Vector3& normal1, const Vector3& normal2, const Matrix4& loacalTransform, const Matrix4& parementTransform,
		int fitGroundLevel1, int fitGroundLevel2, PavementFitTerrain* terrain)
	{
		std::unordered_map<float, float>tempdata;
		float t = PavementModelUtil::CalculatePointT(length, 0, tempdata, p0, p1, p3, p2);
		t = t > 1.f ? 1.f : t;
		t = t < 0.f ? 0.f : t;
		if (Math::FloatEqual(t, 0.0f, 1.0e-5f))
			t = 0.0f;

		return PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromT(p0, p1, p2, p3, t, normal1, normal2, loacalTransform, parementTransform, fitGroundLevel1, fitGroundLevel2, terrain);

	}
	Vector3 PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromT(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t, const Vector3& normal1, const Vector3& normal2,
		const Matrix4& loacalTransform, const Matrix4& parementTransform, int fitGroundLevel1, int fitGroundLevel2, PavementFitTerrain* terrain)
	{
		Vector3 point;
		Vector3 pointAxis_X;
		Vector3 pointAxis_Y;
		Vector3 pointAxis_Z;
		float length = LengthFromTime(p0, p1, p3, p2, t);
		float Alllength = LengthFromTime(p0, p1, p3, p2, 1.0f);

		point = PiecewiseBezierUtil::CInterpolate(p0, p1, p3, p2, t);

		pointAxis_X = PiecewiseBezierUtil::CTangent(p0, p1, p3, p2, t).normalisedCopy();

		Vector3 Axis_Y = PiecewiseBezierUtil::CalcualteCenterNormal(length, Alllength, normal1, normal2);
		PiecewiseBezierUtil::CalculateNormal(pointAxis_X, pointAxis_Y, pointAxis_Z, Axis_Y);

		if (pointAxis_X.IsZero())
			pointAxis_X = Vector3::UNIT_X;
		if (pointAxis_Y.IsZero())
			pointAxis_Y = Vector3::UNIT_Y;
		if (pointAxis_Z.IsZero())
			pointAxis_Z = Vector3::UNIT_Z;

		Matrix3 pointTransformMatrix = {
				pointAxis_X.x, pointAxis_Y.x, pointAxis_Z.x,
				pointAxis_X.y, pointAxis_Y.y, pointAxis_Z.y,
				pointAxis_X.z, pointAxis_Y.z, pointAxis_Z.z
		};

		t = length / Alllength;
		if (Math::FloatEqual(t, 0.0f, 1.0e-5f))
			t = 0.0f;
		point += pointTransformMatrix * (parementTransform * loacalTransform.getTrans());

		if (terrain)
		{
			PavementFitTerrain::TerrainData data;
			terrain->getTerrainData(point, data);

			if (fitGroundLevel1 != fitGroundLevel2 && (fitGroundLevel1 == -1 || fitGroundLevel2 == -1))
				data.terrainPos += data.terrainNormal * (Math::Max(fitGroundLevel1, fitGroundLevel2) * RoadManager::instance()->m_pavementFitGroundLevelHight);
			else
				data.terrainPos += data.terrainNormal * (fitGroundLevel1 > 0 ? (fitGroundLevel1 * RoadManager::instance()->m_pavementFitGroundLevelHight) : 0);

			if (fitGroundLevel1 != fitGroundLevel2)
			{
				float height, height_t;
				if (fitGroundLevel1 == -1 || fitGroundLevel2 == -1)
				{
					float height1, height2;
					PavementFitTerrain::TerrainData data1, data2;
					terrain->getTerrainData(p3, data1);
					terrain->getTerrainData(p0, data2);
					height1 = data1.terrainHeight;
					height2 = data2.terrainHeight;
					if (fitGroundLevel1 == -1)
					{
						height = height2 - height1;
						height_t = 1.0f - t;
					}
					else
					{
						height = height1 - height2;
						height_t = t;
					}
				}
				else
				{
					height = (fitGroundLevel2 - fitGroundLevel1) * RoadManager::instance()->m_pavementFitGroundLevelHight;
					if (t < 0.5)
						height_t = (height / Alllength) * t * t;
					else
						height_t = 1.0f - (height / Alllength) * (1.0f - t) * (1.0f - t);
				}
				data.terrainPos += data.terrainNormal * height_t * height;
			}

			data.terrainPos += data.terrainNormal * loacalTransform.getTrans().y;

			data.terrainNormal = parementTransform.extractQuaternion() * data.terrainNormal;

			if (fitGroundLevel1 != fitGroundLevel2)
			{
				Vector3 position = point;

				position = Vector3::lerp(data.terrainPos, point, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));

				if (fitGroundLevel1 == -1 || fitGroundLevel2 == -1)
				{
					if (fitGroundLevel1 == -1)
						position = Vector3::lerp(point, data.terrainPos, t);
					else
						position = Vector3::lerp(data.terrainPos, point, t);

				}
				else
					position = Vector3::lerp(data.terrainPos, point, t < 0.5f ? t * 2.0f : (1.0f - (t - 0.5f) * 2.0f));

				point = position;
			}
			else
				point = data.terrainPos;
		}

		return point;
	}
	PiecewiseBezierUtil::BezierControlPoint PiecewiseBezierUtil::CalculateSplittingBezierCurvesFormT(BezierControlPoint& point1, BezierControlPoint& point2, float t)
	{
		BezierControlPoint newPoint;

		Vector3 p1 = point1.point;
		Vector3 p2 = point1.point + point1.destination_control;
		Vector3 p3 = point2.point + point2.source_control;
		Vector3 p4 = point2.point;

		point1.point = p1;
		point1.destination_control = p2 * t - p1 * (t - 1.0f);
		newPoint.source_control = p3 * t * t - 2 * t * (t - 1.0f) * p2 + (t - 1.0f) * (t - 1.0f) * p1;
		newPoint.point = t * t * t * p4 - 3 * t * t * (t - 1.0f) * p3 + 3 * t * (t - 1.0f) * (t - 1.0f) * p2 - (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * p1;
		newPoint.destination_control = t * t * p4 - 2 * t * (t - 1.0f) * p3 + (t - 1.0f) * (t - 1.0f) * p2;
		point2.source_control = t * p4 - (t - 1.0f) * p3;
		point2.point = p4;

		point1.destination_control -= point1.point;
		newPoint.destination_control -= newPoint.point;
		newPoint.source_control -= newPoint.point;
		point2.source_control -= point2.point;

		return newPoint;
	}
}
