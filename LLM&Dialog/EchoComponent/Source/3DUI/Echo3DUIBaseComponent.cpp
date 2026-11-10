// --------------------------------------------------------------------------------------
// File:   Echo3DUIBaseComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIBaseComponent.h"
#include "EchoSpatialNode.h"
#include "EchoDebugDrawManager.h"
#include "EchoUIRenderSystem.h"
#include "3DUI/Echo3DUIComponentManager.h"
#include "3DUI/Echo3DUIComponent.h"

namespace Echo {

	EDITOR_REFLECT_BEGIN(Echo3DUIBaseComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_UniqueID, "UniqueID", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DebugSphereIsVisible, "DebugSphereIsVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DebugSpherePosition, "DebugSpherePosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	// Echo3DUIBaseComponentInfo
	Echo3DUIBaseComponentInfo::Echo3DUIBaseComponentInfo(const Name & type, ActorCOMMoveableType movable)
		: ActorComponentInfo(type, movable)
	{
	}

	void Echo3DUIBaseComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<Echo3DUIBaseComponentInfo>::get()->ExportJson(this, jsonNode, "");
		AfterExportData(jsonNode);
	}

	void Echo3DUIBaseComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<Echo3DUIBaseComponentInfo>::get()->ImportJson(this, jsonNode, "");
		AfterImportData(jsonNode);
	}

	void Echo3DUIBaseComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		Echo3DUIBaseComponentInfo* target = static_cast<Echo3DUIBaseComponentInfo*>(comInfo);
		reflect::TypeResolver<Echo3DUIBaseComponentInfo>::get()->copyTo(this, target);

		this->AfterCopyTo(comInfo);
	}

	void Echo3DUIBaseComponentInfo::OverWrite() {
		
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<Echo3DUIBaseComponentInfo>::get()->overWrite(mOriginComInfo, this);
			AfterOverWrite();
		}
	}

	// ------------------------------------------------- Echo3DUIBaseComponent ------------------------------------------------
	Echo3DUIBaseComponent::Echo3DUIBaseComponent(ActorPtr  actor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(actor, movableType)
	{
        m_priorityLookupIndex = 0;
        m_UI3DType = UI3D_COMPONENT_UNKONWN_TYPE;

		//registerTickTimeBegin();
	}

	Echo3DUIBaseComponent::~Echo3DUIBaseComponent()
	{
	}

    void Echo3DUIBaseComponent::unInit()
    {
		ProtocolNodeActorComponent::unInit();
		m_pCreatorComponent.reset();
		SAFE_DELETE(mHelperObject);

        //TODO(yanghang):Do this free reference when free component from other system.
        // Don't delay to real free the memory. other system reference maybe invalid.
		if (m_dummyMovable)
		{
			detachFromNode(m_dummyMovable);
            m_dummyMovable->resetFreshBoundBoxCallback();
            m_dummyMovable = 0;
		}

        PostReleaseGObject();

        if(mGObject)
        {
            //mGObject->setActorComponent(0);
            mGObject->release();
        }
    }

    static void UI3DFreshBoundBox(void* baseComponnet)
    {
        if(baseComponnet)
        {
            Echo3DUIBaseComponent* baseCom = (Echo3DUIBaseComponent*)baseComponnet;

			auto& creatorCom = baseCom->getCreatorComponent();

			if (UIRenderSystem::instance()->getNewUpdateStyleEnable())
			{
				if (baseCom->getSceneNode() && baseCom->getSceneNode()->getParent())
					baseCom->getSceneNode()->getParent()->requestUpdate(baseCom->getSceneNode(), true);
				UI3DComponentManager::instance()->need3DUIComponentUpdate(static_cast<Echo3DUIComponent*>(creatorCom.get()));
			}
			else
			{
				ActorComponent* tempCom = baseCom;
				while (tempCom != nullptr && tempCom != creatorCom.get())
				{
					if (tempCom->getSceneNode())
					{
						tempCom->getSceneNode()->needUpdateBounds();
						tempCom->getSceneNode()->_updateBounds();
					}
						
					tempCom = tempCom->getParentNodeCom().get();
				}
				if (!creatorCom.expired() && creatorCom->getSceneNode())
				{
					creatorCom->getSceneNode()->needUpdateBounds();
					creatorCom->getSceneNode()->_updateBounds();
				}
			}
		}
	}
    
	void Echo3DUIBaseComponent::init(ActorComponentInfo * actorComInfo)
	{
		ProtocolNodeActorComponent::init(actorComInfo);

		Echo3DUIBaseComponentInfo* uiBaseComInfo = static_cast<Echo3DUIBaseComponentInfo*>(actorComInfo);

		mHelperObject = new HelperObject(Name(""), HelperObjType_Sphere, false);
		if (uiBaseComInfo)
		{
			if (uiBaseComInfo->GetDebugSphereIsVisible())
			{
				registerProtocolCom();
			}
			else
			{
				unregisterProtocolCom();
			}


			//this->addChildToNode(mHelperObject.getSceneNode());
			//UpdateComRootNode(true);
			mHelperObject->setHelperColorType(HelperObjColorType_Green);
			mHelperObject->setPosition(uiBaseComInfo->GetDebugSpherePosition());
			mHelperObject->setScale({ mDebugSphereRadius,mDebugSphereRadius,mDebugSphereRadius });
			mHelperObject->setVisible(uiBaseComInfo->GetDebugSphereIsVisible());
		}
    }

	void Echo3DUIBaseComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
	}

	void Echo3DUIBaseComponent::beginFrame(EventID, ProtocolHandle)
	{
		if (m_pActComInfo == nullptr) return;

		//if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		//{
		//	Echo3DUIBaseComponentInfo* comInfo = static_cast<Echo3DUIBaseComponentInfo*>(m_pActComInfo);

		//	if (Echo3DUIComponentInfo::GetGloabl3DUIDebugShow() && comInfo->GetDebugSphereIsVisible())
		//	{
  //              Vector3 worldPivotPos;
		//		const ActorComponentPtr& parent = getParentNodeCom();
  //              if(parent.get())
  //              {
  //                  worldPivotPos = parent->convertLocalToWorldPosition(getLocalPivot());
  //                  if (mUI2DStyle & UI2DStyle::UI2D_STYLE_SCREEN)
  //                  {
  //                      DebugDrawManager::instance()->drawSphere(worldPivotPos,
  //                                                               mDebugSphereRadius,
  //                                                               mDebugSphereColor,
  //                                                               DebugDrawManager::DrawSpaceType::CAMERA_SPACE);
  //                  }
  //                  else
  //                  {
  //                      DebugDrawManager::instance()->drawSphere(worldPivotPos,
  //                                                               mDebugSphereRadius,
  //                                                               mDebugSphereColor,
  //                                                               DebugDrawManager::DrawSpaceType::WORLD_SPACE);
  //                  }
  //              }
		//	}
		//}
	}

	void Echo3DUIBaseComponent::registerProtocolCom()
	{
		registerTickTimeEnd();
	}

	void Echo3DUIBaseComponent::unregisterProtocolCom()
	{
		ProtocolComponentManager* pMgr = m_pProtocolComponentManager;
		pMgr->unregisterTickTime(this);
		isRegisterTickTime = false;
	}

	void Echo3DUIBaseComponent::releaseGObject()
	{
		if (m_dummyMovable)
		{
			detachFromNode(m_dummyMovable);
            m_dummyMovable->resetFreshBoundBoxCallback();
            m_dummyMovable = 0;
		}

   		PostReleaseGObject();
   
		if (mGObject) {
			//mGObject->setActorComponent(0);
			mGObject->release();
            mGObject = 0;
		}
	}
    
	void Echo3DUIBaseComponent::SetGObject(RenderPriorityLookupIndex lookupIndex, GObject * gObject)
	{
        //TODO(yanghang):Remove when code complet.
        assert(lookupIndex && "The render prioriry lookup index shouldn't be zero, check the code!");
        m_priorityLookupIndex = lookupIndex;
        
		if (mGObject)
        {
        	assert(false && "Can't modify 3DUIBaseComponent and GObject relation, check code!");
			//REMOVE.
			mGObject->release();
            mGObject = 0;
        }

        if(m_dummyMovable)
        {
            m_dummyMovable->resetFreshBoundBoxCallback();
            m_dummyMovable = 0;
        }
        
        mGObject = gObject;
        //mGObject->setActorComponent(this);
        //mGObject->retain();

        //m_dummyMovable = mGObject->getDummyMovable();
        if(m_dummyMovable)
        {
            m_dummyMovable->setFreshBoundBoxCallback(UI3DFreshBoundBox,
                                                     (void*)this);
            attachToNode(m_dummyMovable);

			//m_dummyMovable->notifyParentTransformDirty();
			m_dummyMovable->updateWorldMat();
			m_dummyMovable->freshExact3DUIHitBox();
			m_dummyMovable->executeFreshBoundBoxCallback();
        }
        
        Echo3DUIBaseComponentInfo* uiComInfo = static_cast<Echo3DUIBaseComponentInfo*>(m_pActComInfo);
		if (uiComInfo)
		{
			uiComInfo->SetUniqueID(mGObject->getUniqueInstanceID());

			//setPivot(uiComInfo->GetDebugSpherePosition());
		}
        
        AfterSetGObject();
    }
    
    void Echo3DUIBaseComponent::resetDebugSphere()
    {
		Echo3DUIBaseComponentInfo* uiBaseComInfo = static_cast<Echo3DUIBaseComponentInfo*>(this->m_pActComInfo);
		if (uiBaseComInfo)
		{
			uiBaseComInfo->SetDebugSpherePosition(Vector3::ZERO, this);
		}
		this->mDebugSphereColor = ColorValue::Green;
		this->mDebugSphereRadius = 0.01f;
    }
    
	void Echo3DUIBaseComponent::SetControllerPage(int controller_index, int page)
	{
		if (nullptr == mGObject || mGObject->getType() < ObjectType::COMPONENT) return;
		GComponent* p_com = static_cast<GComponent*>(mGObject);

		GController* p_controller = p_com->getControllerAt(controller_index);
		
		p_controller->setSelectedIndex(page);
		p_controller->runActions();
	}

    void Echo3DUIBaseComponent::setRenderPriorityLookupIndex(RenderPriorityLookupIndex lookupIndex)
    {
        assert(lookupIndex && "The caller use 0 pointer as priority lookup index, check the code!");
        if(lookupIndex)
            m_priorityLookupIndex =  lookupIndex;
    }

    Echo3DUIBaseComponent::SubUI3DComponentType Echo3DUIBaseComponent::get3DUIComponentType()
    {
        return m_UI3DType;
    }

    //void Echo3DUIBaseComponent::setPivot(Vector3 pivot)
    //{
    //    if(mGObject && m_dummyMovable)
    //    {
    //        Vector2 localUISize;
    //        {
    //            auto uiSize = mGObject->getSize();
    //            localUISize = Vector2(uiSize.width, uiSize.height);
    //        }
    //        
    //        Vector3 localPivot;            
    //        localPivot.x = pivot.x * localUISize.x;
    //        localPivot.y = pivot.y * localUISize.y;
    //        localPivot.z = pivot.z;
    //        
    //        m_dummyMovable->setLocalPivot(localPivot);
    //    }
    //}

  //  void Echo3DUIBaseComponent::freshPivot()
  //  {
  //      Echo3DUIBaseComponentInfo* uiComInfo = static_cast<Echo3DUIBaseComponentInfo*>(m_pActComInfo);
		//if (uiComInfo)
	 //       setPivot(uiComInfo->GetDebugSpherePosition());
  //  }


	void Echo3DUIBaseComponent::createComRootNodeImpl()
	{
		mComRootNode = new SpatialNode();
	}

    void Echo3DUIBaseComponent::changeScene3DUIStyle(UI2DStyle newStyle)
    {
        mUI2DStyle = newStyle;
        if (mGObject)
        {
            //mGObject->setStyle3D(newStyle);
        }
        
        if(m_dummyMovable)
        {
            m_dummyMovable->setStyle(newStyle);
        }

        afterChangeScene3DUIStyle(newStyle);
    }

	void Echo3DUIBaseComponent::setCreatorComponent(const ActorComponentPtr& creator)
	{
		m_pCreatorComponent = creator;
	}
} /// namespace Echo
