#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIObject.h"

#include "EchoProtocolComponent.h"
#include "EchoUIRenderManager.h"
#include "EchoDebugDrawManager.h"
#include "EchoLogManager.h"
#include "UI/UIPackage.h"

namespace Echo
{
	using namespace echofgui;
	UI3DObject::UI3DObject()
	{

	}
	UI3DObject::~UI3DObject()
	{
		clear();
	}

	void UI3DObject::LoadGObjectFromFile(ActorComponentInfo* actorComInfo)
	{
		if (actorComInfo == nullptr) return;

		//Echo3DUIComponentInfo* comInfo = static_cast<Echo3DUIComponentInfo*>(actorComInfo);

		//std::string filePath = comInfo->GetFGUIFilePath();

		//if ("" == filePath) return;

		//std::string package_name = GetPackageName(filePath);
		//UIPackage::addPackage(filePath.substr(0, filePath.find(package_name)) + package_name);

		////UI2DStyle style = comInfo->getUI2DStyle();

		//GObject* gObject = UIPackage::createObject(package_name, comInfo->GetPackageItemName());
		//if (gObject)
		//{
		//	m_pGobject = gObject;
		//	//m_pGobject->retain();
		//	GRoot::getInstance()->addChild(gObject);
		//}
	}

	Echo::UI3DDummyMovable* UI3DObject::getDummyMovable()
	{
		if (m_pGobject)
		{
			//return m_pGobject->getDummyMovable();
			return nullptr;
		}
		return NULL;
	}

	void UI3DObject::setUI3DMovable(UI3DMovable* pUI3DMovable)
	{
		if (NULL == m_pGobject)
		{
			return;
		}


		//UI3DDummyMovable* pMovable = m_pGobject->getDummyMovable();
		//if (pMovable)
		//{
		//	pUI3DMovable->m_axisAlignedBox = pMovable->getLocalAABB();
		//	pUI3DMovable->m_axisAlignedBox.transformAffine(UIRenderManager::instance()->getSceneUIScaleMatrix());
		//}


		//std::queue<GObject*> gobjectQueue;
		//gobjectQueue.push(m_pGobject);
		//while (!gobjectQueue.empty())
		//{

		//	GObject* currentGObject = gobjectQueue.front();
		//	gobjectQueue.pop();

		//	//UI3DDummyMovable* pTempMovable = currentGObject->getDummyMovable();
		//	//uint32 comType = 0x1 << (uint16)currentGObject->getType();

		//	//if (pTempMovable)
		//	//{
		//	//	pUI3DMovable->m_allSubMovables.push_back(pTempMovable);
		//	//	pTempMovable->setParentMovable(pUI3DMovable);
		//	//}

		//	//if (comType & Echo3DUIBaseComponent::UI3D_GCOMPONENT_TYPE)
		//	//{
		//	//	GComponent* gComponent = (GComponent*)currentGObject;
		//	//	int cnt = gComponent->numChildren();
		//	//	for (int i = cnt - 1; i >= 0; i--)
		//	//	{
		//	//		GObject* child = gComponent->getChildAt(i);
		//	//		gobjectQueue.push(child);
		//	//	}

		//	//}
		//}
	}

	void UI3DObject::changeScene3DUIStyle(UI2DStyle newStyle)
	{
		if (NULL == m_pGobject)
		{
			return;
		}
		//std::queue<GObject*> gobjectQueue;
		//gobjectQueue.push(m_pGobject);
		//while (!gobjectQueue.empty())
		//{

		//	GObject* currentGObject = gobjectQueue.front();
		//	gobjectQueue.pop();

		//	//currentGObject->setStyle3D(newStyle);
		//	//UI3DDummyMovable* pMovable = currentGObject->getDummyMovable();
		//	uint32 comType = 0x1 << (uint16)currentGObject->getType();

		//	//if (pMovable)
		//	//{
		//	//	pMovable->setStyle(newStyle);
		//	//}

		//	//if (comType & Echo3DUIBaseComponent::UI3D_GCOMPONENT_TYPE)
		//	//{
		//	//	GComponent* gComponent = (GComponent*)currentGObject;
		//	//	int cnt = gComponent->numChildren();
		//	//	for (int i = cnt - 1; i >= 0; i--)
		//	//	{
		//	//		GObject* child = gComponent->getChildAt(i);
		//	//		gobjectQueue.push(child);
		//	//	}

		//	//}
		//}
	}

	void UI3DObject::clear()
	{
		if (m_pGobject)
		{
			GRoot::getInstance()->removeChild(m_pGobject);
			m_pGobject->release();
			m_pGobject = NULL;
		}
	}

	void* UI3DObject::rayHit3DUIComponentSystem(const HitTest3DUIComponentSystemInfo& hitTestInfo)
	{
		GObject* result = 0;

		std::vector<GObject*> screenTopLevelGObject;
		std::vector<GObject*> sceneTopLevelGObject;

		{
			GRoot* root = (GRoot*)hitTestInfo.root;

			int cnt = root->numChildren();
			switch (root->getChildrenRenderOrder())
			{
			case ChildrenRenderOrder::ASCENT:
			{
				for (int i = cnt - 1; i >= 0; i--)
				{
					//GObject* child = root->getChildAt(i);
					//if (child->getStyle3D() & UI2D_STYLE_SCREEN)
					//{
					//	screenTopLevelGObject.push_back(child);
					//}
					//else
					//{
					//	sceneTopLevelGObject.push_back(child);
					//}
				}
			}
			break;
			case ChildrenRenderOrder::DESCENT:
			{
				for (int i = 0; i < cnt; i++)
				{
					//GObject* child = root->getChildAt(i);
					//if (child->getStyle3D() & UI2D_STYLE_SCREEN)
					//{
					//	screenTopLevelGObject.push_back(child);
					//}
					//else
					//{
					//	sceneTopLevelGObject.push_back(child);
					//}

				}
			}
			break;

			case ChildrenRenderOrder::ARCH:
			{
				int ai = std::min(root->getApexIndex(), cnt);
				for (int i = ai; i < cnt; i++)
				{
					//GObject* child = root->getChildAt(i);

					//if (child->getStyle3D() & UI2D_STYLE_SCREEN)
					//{
					//	screenTopLevelGObject.push_back(child);
					//}
					//else
					//{
					//	sceneTopLevelGObject.push_back(child);
					//}

				}
				for (int i = ai - 1; i >= 0; i--)
				{
					//GObject* child = root->getChildAt(i);
					//if (child->getStyle3D() & UI2D_STYLE_SCREEN)
					//{
					//	screenTopLevelGObject.push_back(child);
					//}
					//else
					//{
					//	sceneTopLevelGObject.push_back(child);
					//}

				}
			}
			}
		}

		auto render_system = Root::instance()->getRenderSystem();
		auto activeSceneManager = Root::instance()->getActiveSceneManager();
		auto mainCamera = activeSceneManager->getMainCamera();
		uint32 w, h;
		render_system->getWindowSize(w, h);
		Vector2 pos = hitTestInfo.screenPos;

		//IMPORTANT(yanghang): First screen 3D UI ray hit test. If hit anything, test will over.
		{
			Ray cameraSpaceHitRay;

			//NOTE: Camera space ray generate.
			{
				float nx = (2.0f * (pos.x / w)) - 1.0f;
				float ny = 1.0f - (2.0f * (pos.y / h));

				Vector3 nearPoint(nx, ny, -1.0f);
				// Use midPoint rather than far point to avoid issues with infinite projection
				Vector3 midPoint(nx, ny, 0);

				Matrix4 inverseVP = (mainCamera->getProjectionMatrix()).inverse();

				// Get ray origin and ray target on near plane in world space
				Vector3 rayOrigin, rayTarget;

				rayOrigin = inverseVP * nearPoint;
				rayTarget = inverseVP * midPoint;

				Vector3 rayDirection = rayTarget - rayOrigin;
				rayDirection.normalise();

				cameraSpaceHitRay.setOrigin(rayOrigin);
				cameraSpaceHitRay.setDirection(rayDirection);

			}
			GObject* screenHitResult = 0;
			for (auto screenCom : screenTopLevelGObject)
			{
				screenHitResult = recursivelyRayHit3DUI(cameraSpaceHitRay, screenCom);
				if (screenHitResult)
					break;
			}

			if (screenHitResult)
			{
				result = screenHitResult;
				return (result);
			}
		}

		//IMPORTANT(yanghang):If don't hit any screen 3D UI Component, now it's time to do Scene 3D UI Component hit
		// test.
		{
			//IMPORTANT(yanghang):Sort scene 3D UI top-level component for corrent hit-test order by depth in camera space.
			struct Scene3DUIComponentInfo
			{
				GObject* com;
				float cameraSpaceZ;
			};

			std::vector<Scene3DUIComponentInfo> sceneTopLevelGObjectInfo;

			DMatrix4 toCameraViewSpace = mainCamera->getViewMatrix();

			Vector4 temp_v;
			Scene3DUIComponentInfo tempInfo;
			for (auto com : sceneTopLevelGObject)
			{
				//temp_v = Vector4(com->getDummyMovable()->getWorldPosition());
				temp_v = toCameraViewSpace * temp_v;
				tempInfo.cameraSpaceZ = temp_v.z;
				tempInfo.com = com;
				sceneTopLevelGObjectInfo.push_back(tempInfo);
			}

			struct Scene3DUIComponentSort
			{
				bool operator()(const Scene3DUIComponentInfo& a, const Scene3DUIComponentInfo& b) const
				{
					return(a.cameraSpaceZ > b.cameraSpaceZ);
				}
			};

			std::stable_sort(sceneTopLevelGObjectInfo.begin(),
				sceneTopLevelGObjectInfo.end(),
				Scene3DUIComponentSort());

			Ray worldSpaceHitRay = mainCamera->getCameraToViewportRay(pos.x / w, pos.y / h);

			GObject* scene3DUIHitResult = 0;
			for (auto sceneComInfo : sceneTopLevelGObjectInfo)
			{
				scene3DUIHitResult = recursivelyRayHit3DUI(worldSpaceHitRay, sceneComInfo.com);
				if (scene3DUIHitResult)
				{
					result = scene3DUIHitResult;
					break;
				}
			}
		}
		return(result);
	}

	bool rayHitWithAABB(const Ray& ray, const AxisAlignedBox& aabb)
	{
		Vector3 aabbVertex[8];
		aabb.GetVerts(aabbVertex);

		//IMPORTANT: Ray-intersect test.
		// NOTE: Per triangle ray-hit test.

		//IMPORTANT(yanghang): Engine internal index of AABB vertices.
		static int s_aabb_triangle_index[36] =
		{
			0,2,1, 0,3,2,
			0,4,7, 0,7,3,
			4,5,6, 4,6,7,
			1,6,5, 1,2,6,
			0,5,4, 0,1,5,
			3,7,6, 3,6,2
		};

		const int* indices = s_aabb_triangle_index;
		bool ray_hit = false;

		for (int triangle_index = 0;
			triangle_index < 12;
			triangle_index++)
		{
			int start_index = triangle_index * 3;
			int i_a = indices[start_index + 0];
			int i_b = indices[start_index + 1];
			int i_c = indices[start_index + 2];
			auto intersect_result = Math::intersects(ray,
				aabbVertex[i_a],
				aabbVertex[i_b],
				aabbVertex[i_c],
				true,
				false);
			if (intersect_result.first)
			{
				ray_hit = true;
				break;
			}
		}

		return (ray_hit);
	}
	echofgui::GObject* UI3DObject::recursivelyRayHit3DUI(const Ray& ray, GObject* pObject)
	{
		if (NULL == pObject)
		{
			return NULL;
		}
		//GObject* result = 0;

		//uint32 comType = 0x1 << (uint16)pObject->getType();

		//if (comType & Echo3DUIBaseComponent::UI3D_GCOMPONENT_TYPE)
		//{
		//	//UI3DDummyMovable* dummyMovable = pObject->getDummyMovable();
		//	//GComponent* gComponent = (GComponent*)pObject;

		//	//if (gComponent && dummyMovable &&
		//	//	gComponent->isTouchable() &&
		//	//	gComponent->getDisplayObject().valid() && gComponent->getDisplayObject().actived())
		//	//{
		//	//	bool isHitComponent = false;

		//	//	auto worldAABB = dummyMovable->getWorldBoundingBox();
		//	//	if (rayHitWithAABB(ray, worldAABB))
		//	//	{
		//	//		isHitComponent = true;

		//	//		//IMPORTANT(yanghang):First test with custom-hit-area or GComponent self.
		//	//		IHitTest* customHitArea = gComponent->getHitArea();
		//	//		if (customHitArea)
		//	//		{
		//	//			//NOTE(yanghang):Turn the hit point to the 3D UI local space to calculate it's
		//	//			// local position.
		//	//			bool isRayInterectPlane = false;
		//	//			Vec2 localPoint;
		//	//			Vec2 globalPoint;

		//	//			GObject* customHitGObject = customHitArea->getCustomHitGObject();
		//	//			//IMPORTANT: The branch comes from GGraph as custom hit test area.
		//	//			//and GGraph maybe 3D transformed.
		//	//			if (customHitGObject)
		//	//			{
		//	//				UI3DDummyMovable* customHitMovable = customHitGObject->getDummyMovable();
		//	//				if (customHitMovable)
		//	//				{
		//	//					Vector3 hitLocalPoint;
		//	//					isRayInterectPlane = customHitMovable->getRayHitUIPlaneLocalPoint(ray, hitLocalPoint);
		//	//					localPoint.x = hitLocalPoint.x;
		//	//					localPoint.y = hitLocalPoint.y;
		//	//					globalPoint = customHitGObject->localToGlobal(localPoint);
		//	//				}
		//	//			}
		//	//			else
		//	//			{
		//	//				Vector3 hitLocalPoint;
		//	//				isRayInterectPlane = dummyMovable->getRayHitUIPlaneLocalPoint(ray, hitLocalPoint);
		//	//				localPoint.x = hitLocalPoint.x;
		//	//				localPoint.y = hitLocalPoint.y;
		//	//			}

		//	//			if (isRayInterectPlane)
		//	//			{
		//	//				echofgui::Rect notUsedRect = {};
		//	//				isHitComponent = customHitArea->hitTest(notUsedRect, localPoint);
		//	//			}
		//	//			else
		//	//			{
		//	//				isHitComponent = false;
		//	//			}
		//	//		}
		//	//	}

		//	//	//IMPORTANT(yanghang):If don't hit component, ray-cast test finish here.
		//	//	if (isHitComponent)
		//	//	{
		//	//		//NOTE: scroll_pane ray-cast hit first.
		//	//		/*
		//	//		  ScrollPane* scrollPane = gComponent->getScrollPane();
		//	//		  if(scrollPane)
		//	//		  {
		//	//		  GObject* hitScrollPaneResult = scrollPane->hitTest();
		//	//		  }
		//	//		*/

		//	//		//NOTE: Use GComponent hit test logic.
		//	//		int cnt = (int)gComponent->numChildren();

		//	//		switch (gComponent->getChildrenRenderOrder())
		//	//		{
		//	//		case ChildrenRenderOrder::ASCENT:
		//	//		{
		//	//			for (int i = cnt - 1; i >= 0; i--)
		//	//			{
		//	//				GObject* child = gComponent->getChildAt(i);
		//	//				if (!child->getDisplayObject().valid())
		//	//					continue;



		//	//				GObject* target = recursivelyRayHit3DUI(ray, child);;

		//	//				if (target)
		//	//					return target;
		//	//			}
		//	//		}
		//	//		break;
		//	//		case ChildrenRenderOrder::DESCENT:
		//	//		{
		//	//			for (int i = 0; i < cnt; i++)
		//	//			{
		//	//				GObject* child = gComponent->getChildAt(i);
		//	//				if (!child->getDisplayObject().valid())
		//	//					continue;

		//	//				GObject* target = recursivelyRayHit3DUI(ray, child);;

		//	//				if (target)
		//	//					return target;
		//	//			}
		//	//		}
		//	//		break;

		//	//		case ChildrenRenderOrder::ARCH:
		//	//		{
		//	//			int ai = MIN(gComponent->getApexIndex(), cnt);
		//	//			for (int i = ai; i < cnt; i++)
		//	//			{
		//	//				GObject* child = gComponent->getChildAt(i);
		//	//				if (!child->getDisplayObject().valid())
		//	//					continue;

		//	//				GObject* target = recursivelyRayHit3DUI(ray, child);;

		//	//				if (target)
		//	//					return target;
		//	//			}
		//	//			for (int i = ai - 1; i >= 0; i--)
		//	//			{
		//	//				GObject* child = gComponent->getChildAt(i);
		//	//				if (!child->getDisplayObject().valid())
		//	//					continue;

		//	//				GObject* target = recursivelyRayHit3DUI(ray, child);;

		//	//				if (target)
		//	//					return target;
		//	//			}
		//	//		}
		//	//		}

		//	//		if (gComponent->getOpaque())
		//	//			result = gComponent;
		//	//	}
		//	//}
		//}
		//else if (comType & Echo3DUIBaseComponent::UI3D_GOBJECT_TYPE)
		//{
		//	//NOTE: Basic render type.

		//	//GObject* gObject = pObject;

		//	//if (gObject &&
		//	//	gObject->isTouchable() &&
		//	//	gObject->getDisplayObject().valid() && gObject->getDisplayObject().actived())
		//	//{
		//	//	UI3DDummyMovable* dummyMovable = gObject->getDummyMovable();
		//	//	if (dummyMovable)
		//	//	{
		//	//		auto worldAABB = dummyMovable->getWorldBoundingBox();

		//	//		//NOTE: Coarse AABB test.
		//	//		if (rayHitWithAABB(ray, worldAABB))
		//	//		{
		//	//			// Exactly ray hit test.
		//	//			//TODO(yanghang):Use intersect function that without resolve
		//	//			// hit point version.
		//	//			if (dummyMovable->rayHitExactHitBox(ray))
		//	//			{
		//	//				result = pObject;
		//	//			}
		//	//		}
		//	//	}
		//	//}
		//}
		//else
		//{
		//	assert(false && "Component type must be one kind of 3DUIBaseComponent, check code!");
		//}
		return nullptr;//(result);
	}

	UI3DObjectBase* UI3DObject::CreateUI3DObject()
	{
		UI3DObjectBase* p = new UI3DObject();
		return p;
	}

} // end namespace echo
