// --------------------------------------------------------------------------------------
// File:   Echo3DUIInputTextComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIInputTextComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIInputTextComponentInfo ------------------------------------------------------------ //
	Echo3DUIInputTextComponentInfo::Echo3DUIInputTextComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIInputTextComponentInfo::~Echo3DUIInputTextComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIInputTextComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIInputTextComponent)

	Echo3DUIInputTextComponent::Echo3DUIInputTextComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_INPUTTEXTCOMPONENT;
	}

	Echo3DUIInputTextComponent::~Echo3DUIInputTextComponent() {

	}

	std::string Echo3DUIInputTextComponent::GetText()
	{
		return this->m_Text;
	}

	void Echo3DUIInputTextComponent::SetText(const std::string& text)
	{
		if (nullptr == this->mGObject) return;

		this->m_Text = text;

		static_cast<GTextInput*>(this->mGObject)->setText(m_Text);
	}

	void Echo3DUIInputTextComponent::AfterSetGObject()
	{
		//GTextInput* gTextInput = static_cast<GTextInput*>(this->mGObject);

		//UI2DCustomPic* caretRender = gTextInput->getCaretRender();
		//if (caretRender)
		//{
		//	this->attachToNode(caretRender);
  //          //caretRender->setRenderPriorityLookupIndex(m_priorityLookupIndex);
		//	//caretRender->notifyParentTransformDirty();
		//	caretRender->updateWorldMat();
		//	caretRender->freshExact3DUIHitBox();
		//	caretRender->executeFreshBoundBoxCallback();
		//}

		//UI2DCustomPic* selectionRender = gTextInput->getSelectionRender();
		//if (selectionRender)
		//{
		//	this->attachToNode(selectionRender);
  //          //selectionRender->setRenderPriorityLookupIndex(m_priorityLookupIndex);
		//	//selectionRender->notifyParentTransformDirty();
		//	selectionRender->updateWorldMat();
		//	selectionRender->freshExact3DUIHitBox();
		//	selectionRender->executeFreshBoundBoxCallback();
		//}

        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}
    }

    void Echo3DUIInputTextComponent::afterChangeScene3DUIStyle(UI2DStyle newStyle)
    {
        //if(mGObject)
        //{
        //    GTextInput* gTextInput = static_cast<GTextInput*>(this->mGObject);
        //    UI2DCustomPic* caretRender = gTextInput->getCaretRender();
        //    if (caretRender)
        //    {
        //        caretRender->setStyle(newStyle);
        //    }

        //    UI2DCustomPic* selectionRender = gTextInput->getSelectionRender();
        //    if (selectionRender)
        //    {
        //        selectionRender->setStyle(newStyle);
        //    }
        //}
    }
    
    void Echo3DUIInputTextComponent::PostReleaseGObject()
    {
        //if(mGObject)
        //{
        //    GTextInput* gTextInput = static_cast<GTextInput*>(this->mGObject);
        //    
        //    UI2DCustomPic* caretRender = gTextInput->getCaretRender();
        //    if(caretRender)
        //        detachFromNode(caretRender);

        //    UI2DCustomPic* selectionRender = gTextInput->getSelectionRender();
        //    if(selectionRender)
        //        detachFromNode(selectionRender);
        //}
    }

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIInputTextComponentScript::Echo3DUIInputTextComponentScript(const EchoActorScript& pActor, const std::string& name) 
	//	: Echo3DUIBaseComponentScript(pActor, name)
	//{
	//}

	//Echo3DUIInputTextComponentScript Echo3DUIInputTextComponentScript::GetEcho3DUIInputTextComponent(const EchoActorScript& p_actor, const std::string& name)
	//{
	//	return Echo3DUIInputTextComponentScript(p_actor, name);
	//}

	//std::string Echo3DUIInputTextComponentScript::GetText()
	//{
	//	Echo3DUIInputTextComponent* pCom = static_cast<Echo3DUIInputTextComponent*>(m_Ptr.get());
	//	if (pCom) {
	//		return pCom->GetText();
	//	}
	//	return "";
	//}

	//void Echo3DUIInputTextComponentScript::SetText(const std::string& text)
	//{
	//	Echo3DUIInputTextComponent* pCom = static_cast<Echo3DUIInputTextComponent*>(m_Ptr.get());
	//	if (pCom) {
	//		return pCom->SetText(text);
	//	}
	//}

} /// Echo
