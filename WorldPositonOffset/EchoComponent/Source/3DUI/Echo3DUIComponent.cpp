// --------------------------------------------------------------------------------------
// File:   Echo3DUIComponent.cpp
//
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIComponent.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchocJsonHelper.h"
#include "UI/UIPackage.h"
#include "FieldTypes.h"
#include "UI/GComponent.h"
#include "UI/UIObjectFactory.h"
#include "Core/DisplayComponent.h"
#include "EchoDebugDrawManager.h"
#include "EchoUIRenderSystem.h"
#include "EchoUI3DMovable.h"
#include "UI/Transition.h"
#include "UI/GObject.h"
#include "UI/Stage.h"

using namespace echofgui;
namespace Echo
{

	class Echo3DUIComponent::Private
	{
	public:
		echofgui::Handle m_pObject;
		echofgui::Handle m_pRoot;
	};


    EDITOR_REFLECT_BEGIN(Echo3DUIComponentInfo)
    EDITOR_REFLECT_MEMBER_UPDATE(m_FGUIFilePath, "FGUIFilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
    EDITOR_REFLECT_MEMBER_UPDATE(m_PackageItemName, "PackageItemName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
    EDITOR_REFLECT_MEMBER_UPDATE(m_UI2DStyle, "UI2DStyle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
    EDITOR_REFLECT_END();

    //ACTOR_FACTORY_REG(Echo3DUIComponent)

	Echo3DUIComponentInfo::Echo3DUIComponentInfo(const Name & type, ActorCOMMoveableType movable)
    : ActorComponentInfo(type, movable)
	{
	}

	void Echo3DUIComponentInfo::ExportData(cJSON * jsonNode)
	{
        reflect::TypeResolver<Echo3DUIComponentInfo>::get()->ExportJson(this, jsonNode, "");

        if (jsonNode) {
            // 打上版本号
            Echo::WriteJSNode(jsonNode, "Version2", true);
        }
	}

	void Echo3DUIComponentInfo::ImportData(cJSON * jsonNode)
	{

        reflect::TypeResolver<Echo3DUIComponentInfo>::get()->ImportJson(this, jsonNode, "");
        if (jsonNode) {
            // Version1 -> Version2:
            // UI2DStyle Scene 0, Screen 1 -> UI2D_STYLE_SCENE  0x1, UI2D_STYLE_SCENE_FIRST_PERSON = 0x3, UI2D_STYLE_SCREEN = 0x4
             bool isVersion2 = false;
            Echo::ReadJSNode(jsonNode, "Version2", isVersion2);

            if (!isVersion2) {
                m_UI2DStyle = m_UI2DStyle == 0 ? 0x1 : 0x4;
            }
        }
	}

	void Echo3DUIComponentInfo::CopyTo(ActorComponentInfo * p_actor_com_info)
    {

		Echo3DUIComponentInfo* target = static_cast<Echo3DUIComponentInfo*>(p_actor_com_info);
        reflect::TypeResolver<Echo3DUIComponentInfo>::get()->copyTo(this, target);
	}

    void Echo3DUIComponentInfo::OverWrite()
    {

        if (mOriginComInfo != nullptr)
        {
            reflect::TypeResolver<Echo3DUIComponentInfo>::get()->overWrite(mOriginComInfo, this);
        }
    }
#ifdef ECHO_EDITOR
	bool Echo3DUIComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		resMap.emplace(ResourceType::NeedCopyRes, m_FGUIFilePath);
		return true;
	}
#endif

	// ----------------------------------------------------- Echo3DUIComponent -------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIComponent)

	Echo3DUIComponent::Echo3DUIComponent(ActorPtr p_actor, ActorCOMMoveableType movable_type) :
        NodeActorComponent(p_actor, movable_type),
        m_movable(nullptr),
        m_bTouchDisable(false)
    {
		m_Private = new Private();
		// registerTickTimeBegin();

	}

	Echo3DUIComponent::~Echo3DUIComponent()
	{
		SAFE_DELETE(m_Private);
	}

	//void Echo3DUIComponent::registerTickTimeBegin()
 //   {
	//	registerTickTimeEnd();
	//}

	void Echo3DUIComponent::init(ActorComponentInfo * p_info)
    {

        m_movable = nullptr;
        NodeActorComponent::init(p_info);

        //if (!m_movable) m_movable = new UI3DMovable();
        // ((SceneNode*)mComRootNode)->attachObject(m_movable);
		Reload();
		if (getGRoot())
		{
			getGRoot()->setVisible(true);
		}
    }

    void Echo3DUIComponent::unInit()
    {
        GRoot* pRoot = m_Private->m_pRoot.get<GRoot>();
        if(pRoot) {
            pRoot->removeFromParent(); // 2D模式下要从2D树上移除
            if(pRoot->getUIStyle() & UI2DStyle::UI2D_STYLE_SCENE) {
                Stage::getInstance()->on3DComponentRemoved(pRoot); // 3D模式下通知Stage移除
                Stage::getInstance()->on3dUIRootTouchableChanged(pRoot,false);
            }
            pRoot->release();
			m_Private->m_pRoot = nullptr;
        }
        if(m_movable) { // 清理movable
            auto sceneNode = ((SceneNode*)mComRootNode);
            sceneNode->detachObject(m_movable);
            sceneNode->setUpdateMovableBound(false);
            delete m_movable;
            m_movable = nullptr;
        }
    }

	void Echo3DUIComponent::bindNode()
    {

	}

	void Echo3DUIComponent::copyTo(const ActorComponentPtr&   p_actor_com)
    {

	}

	void Echo3DUIComponent::connectCom()
    {

	}

    void Echo3DUIComponent::setTouchable(bool b)
    {
        m_bTouchDisable = !b;

        auto pRoot   = m_Private->m_pRoot.get<GRoot>();
        auto pObject = m_Private->m_pObject.get<GObject>();

        if (pObject)
        {
            pObject->setTouchable(b);
        }

        //auto defaultRoot = GRoot::defaultRoot();
        if (m_Private->m_pRoot) // 不再需要判断是不是defaultRoot
        {
            pRoot->setTouchable(b);

            Stage::getInstance()->on3dUIRootTouchableChanged(pRoot, (pRoot->getUIStyle() & UI2DStyle::UI2D_STYLE_SCENE) && b);

        }
    }

    void Echo3DUIComponent::createComRootNodeImpl()
    {
        SceneManager* scMgr = getSceneManager();
        if (scMgr)
            mComRootNode = scMgr->createSceneNode();
    }

	void Echo3DUIComponent::changeUI2DStyle(UI2DStyle style)
    {


        //场景ui需要root和界面一样大，屏幕ui 需要root监听窗口缩放，
        // 窗口缩放的监听放到m_pRoot->setUIStyle(style);里了

        if (m_Private->m_pObject)
        {
            GRoot* pRoot = nullptr;
            GRoot* defaultRoot = GRoot::defaultRoot();
            auto pObject = m_Private->m_pObject.get<GObject>();
            if(!m_Private->m_pRoot) { // 现在这个m_pRoot一定是存在的，所以在开头就要确定它不为空
				m_Private->m_pRoot = GRoot::create();
                pRoot = m_Private->m_pRoot.get<GRoot>();
                pRoot->addChild(pObject);
            } else {
                pRoot = m_Private->m_pRoot.get<GRoot>();
            }

            if (style & UI2DStyle::UI2D_STYLE_SCENE) // 3DUI 模式
            {
                // 如果之前2DUI，它挂到2DUI树上
                pRoot->removeFromParent();
                pRoot->setTouchable(!m_bTouchDisable);
                Stage::getInstance()->add3DComponent(pRoot);
                Stage::getInstance()->on3dUIRootTouchableChanged(pRoot,!m_bTouchDisable);
            }
            else if(style & UI2DStyle::UI2D_STYLE_SCREEN)
            {
                if(pRoot->getUIStyle() & UI2DStyle::UI2D_STYLE_SCENE) {
                    Stage::getInstance()->on3DComponentRemoved(pRoot);
                    Stage::getInstance()->on3dUIRootTouchableChanged(pRoot,false);
                }
                defaultRoot->addChild(pRoot);
            }
            pRoot->setUIStyle(style);

            // pRoot->addChild(pObject);

            if ((style & Echo::UI2D_STYLE_SCENE) != 0)
            {
                const auto& size = pObject->getRawSize();
                pRoot->setSize(size.width, size.height, false);
                pRoot->setScale(1.f, 1.f);
                pObject->setPosition(0.f, 0.f);
                // 只有场景UI才会让引擎去管理界面的剔除
                {
                    auto sceneNode = ((SceneNode*)mComRootNode);
                    if(!m_movable) {
                        m_movable = new UI3DMovable();
                        sceneNode->attachObject(m_movable);
                    }
                    // FGUI和引擎的坐标系掺和起来有点混乱，尤其是带了anchor的，最终是转换成引擎的空间
                    // echofgui::Rect rc = { {0.f, 0.f}, {size.width, size.height} };
                    Echo::AxisAlignedBox aabb({0.f, -size.height, 0}, {size.width, 0, 0.1f});
                    if(pObject->isPivotAsAnchor()) {
                        aabb.vMin.x += (-size.width * pObject->getPivot().x);
                        aabb.vMax.x += (-size.width * pObject->getPivot().x);
                        aabb.vMin.y += size.height * pObject->getPivot().y;
                        aabb.vMax.y += size.height * pObject->getPivot().y;
                    }
                    m_movable->m_axisAlignedBox = aabb;
                    m_movable->m_axisAlignedBox.transformAffine(UIRenderSystem::instance()->getSceneUIScaleMatrix()); // 乘三维空间系数
                    m_movable->setGRoot(m_Private->m_pRoot);
                    //sceneNode->setUpdateMovableBound(true);
                    sceneNode->needUpdateBounds();
                    sceneNode->_update(true, false);
                }
            }
            else
            {
                if(m_movable)
                { // 否则，清理movable
                    auto sceneNode = ((SceneNode*)mComRootNode);
                    sceneNode->detachObject(m_movable);
                    sceneNode->setUpdateMovableBound(false);
                    delete m_movable;
                    m_movable = nullptr;
                }
                pRoot->setWorldMatrix(Echo::DBMatrix4::IDENTITY); // 给默认世界矩阵
            }

        }
    }

    void Echo3DUIComponent::updateAABB(bool _isScale)
    {
        auto pObject = m_Private->m_pObject.get<GObject>();
        if(!pObject) {
            return;
        }
        auto sceneNode = ((SceneNode*)mComRootNode);
        if(!sceneNode) {
            return;
        }
        if(!m_movable) {
            m_movable = new UI3DMovable();
            sceneNode->attachObject(m_movable);
        }
        const auto& size = pObject->getRawSize();
        echofgui::Rect rc;
        if (_isScale)
            rc = {{0.f, -size.height}, {size.width, 0}};
        else
            rc = {{0.f, 0.f}, {size.width, size.height}};
        rc = pObject->localToGlobal(rc);
        Echo::AxisAlignedBox aabb({rc.x, rc.y,  0}, {rc.x + rc.w, rc.y + rc.h, 0.1f});
        m_movable->m_axisAlignedBox = aabb;
        if (_isScale)
            m_movable->m_axisAlignedBox.transformAffine(UIRenderSystem::instance()->getSceneUIScaleMatrix()); // 乘三维空间系数
        m_movable->setGRoot(m_Private->m_pRoot);
        //sceneNode->setUpdateMovableBound(true);
        sceneNode->needUpdateBounds();
        sceneNode->_update(true, false);
    }

    void Echo3DUIComponent::setScaleType(echofgui::UI3dScalerMode type)
    {
        // 其实这里应该限定SCREEN UI
        auto pObject = m_Private->m_pObject.get<GObject>();
        switch (type)
        {
            case UI3dScalerMode::None:
                break;
            case UI3dScalerMode::FullScreen:
                pObject->makeFullScreen();
                break;
            case UI3dScalerMode::FullScreenIgnoreMargin:
                pObject->makeFullScreen();
                break;
            default:
                break;
        }
    }
  //void Echo3DUIComponent::updateUIFacing()
  //{
        //
		//Echo3DUIComponentInfo* info = static_cast<Echo3DUIComponentInfo*>(m_pActComInfo);
  //
  //      if (info == nullptr)
  //      {
  //          return;
  //      }

  //      auto facingType = info->GetFacingType();
		//switch(facingType)
  //      {
  //          case Echo3DUIComponentInfo::FACING_STABLE :
  //          {
  //              //IMPORTANT(yanghang):For switch function jump table optimize.
  //          }break;
  //          case Echo3DUIComponentInfo::FACING_CAMERA :
  //          {
  //              float maxLookUpAngle = info->GetMaxLookUpAngle();
  //              Quaternion currentOrient = getWorldRotation();
  //
  //              if(info->GetUI2DStyle() & UI2DStyle::UI2D_STYLE_SCENE)
  //              {
  //                  auto camera = Root::instance()->getMainSceneManager()->getMainCamera();

  //                  //NOTE(yanghang): Use pivot as rotate center.
  //                  Vector3 worldPivotPos = getLocalPivot();
  //                  auto parent = getParentNodeCom();
  //                  if(parent.get())
  //                  {
  //                      worldPivotPos = parent->convertLocalToWorldPosition(worldPivotPos);
  //                  }
  //
  //                  Vector3 newLookDirV = camera->getPosition() - worldPivotPos;
  //                  //Quaternion newOrient = OrientDirection(newLookDirV,
  //                  //                                       currentOrient,
  //                  //                                       maxLookUpAngle);
  //                  //setWorldRotation(newOrient);
  //              }
  //              else
  //              {
  //                  //NOTE(yanghang):The screen point calculate from 3D UI ray-cast.
  //                  auto mainSceneManager = Root::instance()->getMainSceneManager();
  //                  auto mainCamera = mainSceneManager->getMainCamera();
  //                  auto render_system = Root::instance()->getRenderSystem();
  //                  uint32 w,h;
  //                  MouseInfo pos = UIRenderManager::instance()->getMouseInfo();
  //                  render_system->getWindowSize(w,h);

  //                  float nx = (2.0f * (pos.x/w)) - 1.0f;
  //                  float ny = 1.0f - (2.0f * (pos.y/h));
  //
  //                  Vector3 nearPoint(nx, ny, -1.0f);
  //                  Matrix4 inverseVP = (mainCamera->getProjectionMatrix()).inverse();
  //                  Vector3 rayOrigin;
  //                  rayOrigin = inverseVP * nearPoint;
  //
		//			//IMPORTANT(yanghang): Screen 3D UI space origin is in front of view
		//			// space Z=1.0f point.
  //                  Vector3 pivotPos = getLocalPivot();
  //                  pivotPos = getParentNodeCom()->convertLocalToWorldPosition(pivotPos);
  //
  //                  Vector3 newLookDirV = rayOrigin - pivotPos;
  //                  //Quaternion newOrient = OrientDirection(newLookDirV,
  //                  //                                       currentOrient,
  //                  //                                       maxLookUpAngle);
  //                  //setWorldRotation(newOrient);
  //              }
  //          }break;
  //
  //          case Echo3DUIComponentInfo::FACING_CHASE_CAMERA :
  //          {
  //              float maxLookUpAngle = info->GetMaxLookUpAngle();
  //              float chaseAngleSpeedX = info->GetChaseSpeedX();
  //              float chaseAngleSpeedY = info->GetChaseSpeedY();
  //              Quaternion currentOrient = getWorldRotation();
  //
  //              if(info->GetUI2DStyle() & UI2DStyle::UI2D_STYLE_SCENE)
  //              {
  //                  auto camera = Root::instance()->getMainSceneManager()->getMainCamera();

  //                  //NOTE(yanghang): Use pivot as rotate center.
  //                  Vector3 worldPivotPos = getLocalPivot();
  //                  auto parent = getParentNodeCom();
  //                  if(parent.get())
  //                  {
  //                      worldPivotPos = parent->convertLocalToWorldPosition(worldPivotPos);
  //                  }
  //
  //                  Vector3 newLookDirV = camera->getPosition() - worldPivotPos;

  //                  //Quaternion newOrient = ChaseDirectionInXYAxis(newLookDirV,
  //                  //                                              currentOrient,
  //                  //                                              chaseAngleSpeedX,
  //                  //                                              chaseAngleSpeedY,
  //                  //                                              maxLookUpAngle);
  //                  //setWorldRotation(newOrient);
  //              }
  //              else
  //              {
  //                  //NOTE(yanghang):The screen point calculate from 3D UI ray-cast.
  //                  auto mainSceneManager = Root::instance()->getMainSceneManager();
  //                  auto mainCamera = mainSceneManager->getMainCamera();
  //                  auto render_system = Root::instance()->getRenderSystem();
  //                  uint32 w,h;
  //                  MouseInfo pos = UIRenderManager::instance()->getMouseInfo();
  //                  render_system->getWindowSize(w,h);

  //                  float nx = (2.0f * (pos.x/w)) - 1.0f;
  //                  float ny = 1.0f - (2.0f * (pos.y/h));
  //
  //                  Vector3 nearPoint(nx, ny, -1.0f);
  //                  Matrix4 inverseVP = (mainCamera->getProjectionMatrix()).inverse();
  //                  Vector3 rayOrigin;
  //                  rayOrigin = inverseVP * nearPoint;
  //
		//			//IMPORTANT(yanghang): Screen 3D UI space origin is in front of view
		//			// space Z=1.0f point.
  //                  Vector3 pivotPos = getLocalPivot();
  //                  pivotPos = getParentNodeCom()->convertLocalToWorldPosition(pivotPos);
  //
  //                  Vector3 newLookDirV = rayOrigin - pivotPos;

  //                  //Quaternion newOrient = ChaseDirectionInXYAxis(newLookDirV,
  //                  //                                              currentOrient,
  //                  //                                              chaseAngleSpeedX,
  //                  //                                              chaseAngleSpeedY,
  //                  //                                              maxLookUpAngle);
  //                  //setWorldRotation(newOrient);
  //              }
  //          }break;
  //          case Echo3DUIComponentInfo::FACING_TEST :
  //          {
  //              float deltaTime = Root::instance()->getTimeSinceLastFrame();
  //              static Vector3 euler_angle;
  //              euler_angle.x += 10.0f * deltaTime;
  //              euler_angle.y += 10.0f * deltaTime;
  //
  //              Quaternion rotate;
  //              rotate.FromEuler(euler_angle);
  //              setWorldRotation(rotate);
  //          }break;
  //          default:
  //          {
  //          }
  //      }
  //}

    void Echo3DUIComponent::Reload(const Echo::Name& pack_name, const Echo::Name& item_name, UI2DStyle type)
    {
        UI2DStyle style = type;

        auto pObject = m_Private->m_pObject.get<GObject>();
        //IMPORTANT(yanghang):Generate UI node of FGUI system.
        if (m_Private->m_pObject)
        {
            pObject->release();
        }
		m_Private->m_pObject = LoadGObjectFromFile(pack_name,item_name);
        pObject   = m_Private->m_pObject.get<GObject>();
        if (!m_Private->m_pObject)
        {
            return;
        }
        changeUI2DStyle(style);

    }

	void Echo3DUIComponent::Reload()
    {


        if (!m_pActComInfo) return;
		Echo3DUIComponentInfo* comInfo = static_cast<Echo3DUIComponentInfo*>(m_pActComInfo);
        UI2DStyle style = comInfo->getUI2DStyle();

        auto pObject = m_Private->m_pObject.get<GObject>();
        //IMPORTANT(yanghang):Generate UI node of FGUI system.
        if (m_Private->m_pObject)
        {
            pObject->release();
        }
		m_Private->m_pObject = LoadGObjectFromFile(m_pActComInfo);
        pObject   = m_Private->m_pObject.get<GObject>();
        if (!m_Private->m_pObject)
        {
            return;
        }

        changeUI2DStyle(style);
	}

        echofgui::GObject* Echo3DUIComponent::LoadGObjectFromFile(const Echo::Name& filePath, const Echo::Name& item_path)
        {
            std::string package_name = GetPackageName(filePath.c_str());
            std::string m_file_path  = filePath.c_str();
            UIPackage::addPackage(m_file_path.substr(0, m_file_path.find(package_name)) + package_name);
            GObject* gObject = UIPackage::createObject(package_name, item_path.c_str());
            return gObject;
        }


	GObject * Echo3DUIComponent::LoadGObjectFromFile(ActorComponentInfo* actorComInfo)
    {

		Echo3DUIComponentInfo* comInfo = static_cast<Echo3DUIComponentInfo*>(actorComInfo);
        if (comInfo == nullptr)
        {
            return nullptr;
        }

		std::string filePath = comInfo->GetFGUIFilePath();

		if ("" == filePath) return nullptr;

		std::string package_name = GetPackageName(filePath);
		UIPackage::addPackage(filePath.substr(0, filePath.find(package_name)) + package_name);

		GObject* gObject = UIPackage::createObject(package_name, comInfo->GetPackageItemName());

        return gObject;
    }

    GObject* Echo3DUIComponent::getGObject() const
    {


        return m_Private->m_pObject.get<GObject>();
    }

    echofgui::GRoot* Echo3DUIComponent::getGRoot() const
    {
        return m_Private->m_pRoot.get<GRoot>();
    }


	std::string GetPackageName(const std::string & filePath)
    {

		int start = filePath.find_last_of('/');
		if (start == std::string::npos) return "";

		int end = filePath.find_last_of('_');
		if (end == std::string::npos) return "";

		return filePath.substr(start + 1, end - start - 1);
	}

    // ----------------------------------------------------- Script --------------------------------------------------------------- //
    //Echo3DUIComponentScript::Echo3DUIComponentScript(const EchoActorScript& pActor, const std::string& name) :
    //    EchoActorComponentScript(pActor, name)
    //{ }

    //Echo3DUIComponentScript Echo3DUIComponentScript::GetEcho3DUIComponent(const EchoActorScript& pActor, const std::string& name)
    //{
    //    return Echo3DUIComponentScript(pActor, name);
    //}

    Quaternion
    OrientDirection(const Vector3&    orientDir,
                    const Quaternion& currentOrientation,
                    float             maxLookUpAngle)
    {
        Quaternion result = currentOrientation;

        Vector3 newLookDirV = orientDir.GetSafeNormal();
        // IMPORTANT: Construct new look dir quaternion with angle limit.
        // NOTE: Rotate around Y and X axis.
        {
            // IMPORTANT:Avoid new look dir overlap with Y axis.
            Vector3 XZProject = Vector3(newLookDirV.x, 0, newLookDirV.z);

            bool outMaxAngleBound = false;
            if (XZProject.IsZero())
            {
                outMaxAngleBound = true;
            }
            else
            {
                XZProject.Normalize();

                float YZ_plane_angle_cos = XZProject.dotProduct(newLookDirV);
                if (YZ_plane_angle_cos < Math::Cos(maxLookUpAngle))
                {
                    outMaxAngleBound = true;
                }
            }

            if (outMaxAngleBound)
            {
                // IMPORTANT: Construct new look dir in the maximum lookup angle.
                XZProject.y = Math::Tan(maxLookUpAngle) * (newLookDirV.y > 0 ? 1.0f : -1.0f);
                newLookDirV = XZProject.GetNormalized();
            }
        }

        Vector3 right = Vector3::UNIT_Y.crossProduct(newLookDirV);
        right.Normalize();

        Vector3 up = newLookDirV.crossProduct(right);
        up.Normalize();

        Quaternion target_rotate(right, up, newLookDirV);
        result = target_rotate;
        return (result);
    }


    Quaternion
    ChaseDirectionInXYAxis(const Vector3&    chaseDir,
                           const Quaternion& currentOrientation,
                           float             chaseAngleSpeedX,
                           float             chaseAngleSpeedY,
                           float             maxLookUpAngle)
    {
        Quaternion result = currentOrientation;
        //IMPORTANT(yanghang):When current angle distance with target angle less than the small angle,
        // the chasing will stop.
        static float small_angle = 1.0f;

        Vector3 newLookAngle;
        // IMPORTANT: Construct new look dir quaternion with angle limit.
        // NOTE: Rotate around Y and X axis.
        {
            Vector3 newLookDirV = chaseDir.GetSafeNormal();

            // IMPORTANT: Construct new look dir quaternion with angle limit.
            // NOTE: Rotate around Y and X axis.
            {
                // IMPORTANT:Avoid new look dir overlap with Y axis.
                Vector3 XZProject = Vector3(newLookDirV.x, 0, newLookDirV.z);

                bool outMaxAngleBound = false;
                if (XZProject.IsZero())
                {
                    outMaxAngleBound = true;
                }
                else
                {
                    XZProject.Normalize();

                    float YZ_plane_angle_cos = XZProject.dotProduct(newLookDirV);
                    if (YZ_plane_angle_cos < Math::Cos(maxLookUpAngle))
                    {
                        outMaxAngleBound = true;
                    }
                }

                if (outMaxAngleBound)
                {
                    // IMPORTANT: Construct new look dir in the maximum lookup angle.
                    XZProject.y = Math::Tan(maxLookUpAngle) * (newLookDirV.y > 0 ? 1.0f : -1.0f);
                    newLookDirV = XZProject.GetNormalized();
                }
            }

            // NOTE: Calculte angle between current look dir with new camera dir.
            Vector3 right = Vector3::UNIT_Y.crossProduct(newLookDirV);
            right.Normalize();
            Vector3 up = newLookDirV.crossProduct(right);
            up.Normalize();

            //TODO(yanghang):Optimize to build matrix.
            Quaternion newLookDirQ;
            newLookDirQ.FromAxes(right, up, newLookDirV);
            newLookAngle = newLookDirQ.ToEulerPositive();
        }

        Quaternion currentOrient    = currentOrientation;
        Vector3    currentLookAngle = currentOrient.ToEulerPositive();
        //IMPORTANT: Avoid float overflow.
        currentLookAngle.x = EchoClampDegreeIn2PI(currentLookAngle.x);
        currentLookAngle.y = EchoClampDegreeIn2PI(currentLookAngle.y);

        Vector3 deltaAngle = newLookAngle - currentLookAngle;
        if (abs(deltaAngle.y) <= small_angle)
        {
            if (abs(deltaAngle.x) > small_angle)
            {
                float deltaTime = Root::instance()->getTimeSinceLastFrame();

                // NOTE: Flags for later rotate-float-integrate over bound
                // of huge speed.

                //NOTE: Angle range is [0 ~ 360^].
                {
                    Vector3 A = newLookAngle;
                    Vector3 B = currentLookAngle;
                    //IMPORTANT: This is only for [0 ~ 90^) limit angle.
                    // Limit angle axis rotate: like 45^ of X axis.
                    // [0 ~ 45^) && (315^ ~ 360^] is valid range.
                    // When abs(A -B) is beyond 180^, Flip move direction.
                    // IF(A - B) > 180^; delta = A - (B + 360^)
                    // IF(A - B) < -180^; delta = (A + 360) - B
                    // TODO: If break [0~90^) limit. the rule will change.
                    if (abs(deltaAngle.x) > 180.0f)
                    {
                        if (deltaAngle.x > 180.0f)
                            deltaAngle.x = A.x - (B.x + 360.0f);
                        else
                            deltaAngle.x = (A.x + 360.0f) - B.x;
                    }
                }

                if (deltaAngle.x < 0)
                {
                    currentLookAngle.x -= deltaTime * chaseAngleSpeedX;
                }
                else
                {
                    currentLookAngle.x += deltaTime * chaseAngleSpeedX;
                }

                result.FromEuler(currentLookAngle);
            }
        }
        else
        {

            float deltaTime = Root::instance()->getTimeSinceLastFrame();

            // NOTE: Flags for later rotate-float-integrate overe bound
            // of huge speed.
            //NOTE: Angle range is [0 ~ 360^].
            {
                Vector3 A = newLookAngle;
                Vector3 B = currentLookAngle;
                // IMPORTANT: Axis free rotate:
                // Angle:A,B. When abs(A - B) is beyond 180^, Flip move direction.
                // IF(A - B) < -180^; delta = (A + 360) - B
                // IF(A - B) > 180^; delta = A - (B + 360)
                if (abs(deltaAngle.y) > 180.0f)
                {
                    if (deltaAngle.y > 180.0f)
                        deltaAngle.y = A.y - (B.y + 360.f);
                    else
                        deltaAngle.y = (A.y + 360.f) - B.y;
                }
            }

            if (deltaAngle.y < 0)
            {
                currentLookAngle.y -= deltaTime * chaseAngleSpeedY;
            }
            else
            {
                currentLookAngle.y += deltaTime * chaseAngleSpeedY;
            }

            result.FromEuler(currentLookAngle);
        }
        return (result);

    }

    void Echo3DUIComponent::setTransitionFramerate(float framerate) {
        auto gobj = getGObject();
        if(!gobj) {
            return;
        }
        assert(gobj->getType() >= ObjectType::COMPONENT && "gobj must be a component");
        GComponent* comp = dynamic_cast<GComponent*>(gobj);
        std::vector<GComponent*> comps;
        comps.push_back(comp);
        while(!comps.empty()) {
            comp = comps.back();
            comps.pop_back();
            auto const& transitions = comp->getTransitions();
            for(auto& ptrans : transitions) {
                ptrans->setInterval(1.0f / framerate);
            }
            auto const& children = comp->getChildren();
            for(auto child: children) {
                if(child->getType() >= ObjectType::COMPONENT) {
                    comp = (GComponent*)child;
                    comps.push_back(comp);
                }
            }
        };
    }

    void Echo3DUIComponent::stopTransitions() {
        auto gobj = getGObject();
        if(!gobj) {
            return;
        }
        assert(gobj->getType() >= ObjectType::COMPONENT && "gobj must be a component");
        GComponent* comp = dynamic_cast<GComponent*>(gobj);
        GComponent* r = comp;
        if(r->getParent()) {
            r = r->getParent();
        }
        GRoot* root = (GRoot*)r;
        TransitionUpdater* updater = root->transitionUpdater();
        std::vector<GComponent*> comps;
        comps.push_back(comp);
        while(!comps.empty()) {
            comp = comps.back();
            comps.pop_back();
            auto const& transitions = comp->getTransitions();
            for(auto& ptrans : transitions) {
                updater->kick(ptrans->getHandle());
            }
            auto const& children = comp->getChildren();
            for(auto child: children) {
                if(child->getType() >= ObjectType::COMPONENT) {
                    comp = (GComponent*)child;
                    comps.push_back(comp);
                }
            }
        };
    }

    void Echo3DUIComponent::staticImageMode(char const* imgUrl) {
        if(m_Private->m_pObject) {
            auto comp = m_Private->m_pObject.get<GComponent>();
            comp->releaseAllChildren();
            // comp->setupOverflow(OverflowType::HIDDEN);
            auto loader = (GLoader*)UIObjectFactory::newObject(ObjectType::LOADER);
            loader->setFill(FillType::SCALE);
            loader->setSize(comp->getSize().width, comp->getSize().height);
            comp->addChild(loader);
            loader->setURL(imgUrl);
        }
    }

} /// namespace Echo
