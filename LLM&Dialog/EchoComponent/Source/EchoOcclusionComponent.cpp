// --------------------------------------------------------------------------------------
// File:   EchoOcclusionComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoOcclusionComponent.h"

#include "Actor/EchoActor.h"
#include "EchoOcclusionPlane.h"
#include "EchoProtocolComponent.h"

namespace Echo {

	EDITOR_REFLECT_BEGIN(EchoOcclusionComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mOcclusionShapeType, "OcclusionShapeType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mVertices, "Vertices", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mOcclusionColor, "OcclusionColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	EchoOcclusionComponentInfo::EchoOcclusionComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: ActorComponentInfo(type, movable)
	{
	}

	void EchoOcclusionComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<EchoOcclusionComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void EchoOcclusionComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<EchoOcclusionComponentInfo>::get()->ImportJson(this, jsonNode, "");

		// 兼容旧 json
		{
			ReadJSNode(jsonNode, "VertexFirst", mVertices[0]);
			ReadJSNode(jsonNode, "VertexSecond", mVertices[1]);
			ReadJSNode(jsonNode, "VertexThird", mVertices[2]);
			ReadJSNode(jsonNode, "VertexFourth", mVertices[3]);
		}

		this->setOcclusionShapeType(static_cast<OcclusionShapeType>(this->mOcclusionShapeType), true);
	}

	void EchoOcclusionComponentInfo::CopyTo(ActorComponentInfo * actorComInfo)
	{
		EchoOcclusionComponentInfo* targetComInfo = static_cast<EchoOcclusionComponentInfo*>(actorComInfo);
		reflect::TypeResolver<EchoOcclusionComponentInfo>::get()->copyTo(this, targetComInfo);

		targetComInfo->mIndices = mIndices;
		targetComInfo->mBorderIndices = mBorderIndices;
	}

	void EchoOcclusionComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<EchoOcclusionComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	void EchoOcclusionComponentInfo::setOcclusionShapeType(const OcclusionShapeType & occlusionShapeType, bool notChangeVertices)
	{
		COMPONENT_INFO_PARAM_CHANGE(EchoOcclusionComponentInfo, this, mOcclusionShapeType, static_cast<int>(occlusionShapeType))
		switch (occlusionShapeType)
		{
		case OcclusionShapeType::None:
			if (notChangeVertices == false)
			{
				this->mVertices[0] = Vector3::ZERO;
				this->mVertices[1] = Vector3::ZERO;
				this->mVertices[2] = Vector3::ZERO;
				this->mVertices[3] = Vector3::ZERO;
			}

			std::vector<Echo::uint16>().swap(this->mIndices);
			std::vector<Echo::uint16>().swap(this->mBorderIndices);
			break;
		case OcclusionShapeType::Triangle:
			if (notChangeVertices == false)
			{
				this->mVertices[0] = { 0, 0, 0 };
				this->mVertices[1] = { 1, 1, 0 };
				this->mVertices[2] = { 2, 0, 0 };
			}

			this->mIndices = { 0, 1, 2 };
			this->mBorderIndices = { 0, 1, 1, 2, 2, 0 };
			break;
		case OcclusionShapeType::Rectangle:
			if (notChangeVertices == false)
			{
				this->mVertices[0] = { 0, 0, 0 };
				this->mVertices[1] = { 0, 1, 0 };
				this->mVertices[2] = { 3, 1, 0 };
				this->mVertices[3] = { 3, 0, 0 };
			}

			this->mIndices = { 0, 1, 2, 0, 2, 3 };
			this->mBorderIndices = { 0, 1, 1, 2, 2, 3, 3, 0 };
			break;
		case OcclusionShapeType::Square:
			if (notChangeVertices == false)
			{
				this->mVertices[0] = { 0, 0, 0 };
				this->mVertices[1] = { 0, 1, 0 };
				this->mVertices[2] = { 1, 1, 0 };
				this->mVertices[3] = { 1, 0, 0 };
			}

			this->mIndices = { 0, 1, 2, 0, 2, 3 };
			this->mBorderIndices = { 0, 1, 1, 2, 2, 3, 3, 0 };
			break;
		default:
			assert(false && "Unknow occlusion shape type");
		}
	}

	DECLAREACTORCOMTYPE_CPP(EchoOcclusionComponent)

	EchoOcclusionComponent::EchoOcclusionComponent(ActorPtr  p_actor, ActorCOMMoveableType movable) : NodeActorComponent(p_actor, movable)
	{
	}

	EchoOcclusionComponent::~EchoOcclusionComponent()
	{
		if (this->m_OcclusionObject)
		{
			this->RemoveChildFromNode(this->m_OcclusionObject->getOcclusionPlaneSceneNode());
			delete this->m_OcclusionObject;
			this->m_OcclusionObject = nullptr;
		}

		if (m_Border)
		{
			//this->RemoveChildFromNode(m_Border->getSceneNode());
			delete m_Border;
			m_Border = nullptr;
		}
	}

	void EchoOcclusionComponent::init(ActorComponentInfo * info)
	{
#ifdef ECHO_EDITOR
		SetNotifiedUpdate(true);
#endif
	}

	void EchoOcclusionComponent::unInit()
	{
#ifdef ECHO_EDITOR
		SetNotifiedUpdate(false);
#endif
	}

	void EchoOcclusionComponent::NotifiChildImp(int changeMask)
	{
		(void)changeMask;
		if (!m_Border)
			return;

		m_Border->setPosition(getWorldPosition());
		m_Border->setScale(getWorldScale());
		m_Border->setOrientation(getWorldRotation());
	}

	void EchoOcclusionComponent::bindNode()
	{
		this->m_OcclusionObject = new OcclusionObject(Name(this->getComName()));
		
		if (m_OcclusionObject)
		{
			this->addChildToNode(m_OcclusionObject->getOcclusionPlaneSceneNode());

			// 辅助线框 编辑器模式才会创建
			if (Root::instance()->getRuntimeMode() != Root::eRM_Client)
			{
				m_Border = new HelperObject(Name(this->getComName()), HelperObjType::HelperObjType_CustomPolygon);
				if (m_Border)
				{
					m_Border->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
					//this->addChildToNode(m_Border->getSceneNode());
				}
			}
		}

		this->generateOcclusion();
	}

	void EchoOcclusionComponent::copyTo(const ActorComponentPtr&   actorCom)
	{
		static_cast<EchoOcclusionComponent*>(actorCom.get())->generateOcclusion();
	}

	void EchoOcclusionComponent::generateOcclusion()
	{
		EchoOcclusionComponentInfo* comInfo = static_cast<EchoOcclusionComponentInfo*>(this->m_pActComInfo);

		if (comInfo && m_OcclusionObject)
		{
			std::vector<Vector3> vectices = { comInfo->mVertices[0], comInfo->mVertices[1], comInfo->mVertices[2], comInfo->mVertices[3] };
			std::vector<Echo::uint16> indices = comInfo->mIndices;

			if (comInfo->getOcclusionShapeType() == OcclusionShapeType::None)
			{
				vectices = { {0, 0, 0} };
				indices = { 0 ,0 , 0 };
			}

			m_OcclusionObject->getOcclusionPlane()->getMaterial()->getDiffuseColor() = comInfo->mOcclusionColor;
			
			// 高亮临时代码
			if (this->m_HighLight)
			{
				m_OcclusionObject->getOcclusionPlane()->getMaterial()->getDiffuseColor().a = 1.0f;
			}
			else
			{
				m_OcclusionObject->getOcclusionPlane()->getMaterial()->getDiffuseColor().a = 0.3f;
			}

			m_OcclusionObject->createOcclusionPlane(vectices, indices);

			// 辅助线框 编辑器模式才会被使用
			if (m_Border)
			{
				std::vector<Vector3> borderVectices = { comInfo->mVertices[0], comInfo->mVertices[1], comInfo->mVertices[2], comInfo->mVertices[3] };
				std::vector<Echo::uint16> borderIndices = comInfo->mBorderIndices;

				if (comInfo->getOcclusionShapeType() == OcclusionShapeType::None)
				{
					borderVectices = { { 0, 0, 0 } };
					borderIndices = { 0 ,0 , 0 };
				}

				m_Border->changeCustomPolygon(borderVectices, borderIndices);
			}
		}
	}

} /// namespace Echo
