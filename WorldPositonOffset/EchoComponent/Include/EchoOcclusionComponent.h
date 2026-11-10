// --------------------------------------------------------------------------------------
// File:   EchoOcclusionComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_OCCLUSION_COMPONENT_H__
#define __ECHO_OCCLUSION_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "EchoOcclusionObject.h"

#include "EchoHelperObj.h"

namespace Echo {

	enum class OcclusionShapeType
	{
		None = 0,

		Triangle,
		Rectangle,
		Square,

		OcclusionShapeType_Count
	};

	class EchoOcclusionComponentInfo;

	class _EchoComponentExport EchoOcclusionComponent : public NodeActorComponent {
		DECLAREACTORCOMTYPE_H(EchoOcclusionComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		void							init(ActorComponentInfo* info) override;
		virtual void                    unInit() override;
		virtual void					NotifiChildImp(int changeMask) override;
		void							bindNode() override;
		void							copyTo(const ActorComponentPtr&   actorCom) override;

	public:
	 	void							generateOcclusion();

		// 因组件不知道在UI上何时被选中准备的变量
		bool							m_HighLight = false;

	private:
		OcclusionObject*				m_OcclusionObject = nullptr;
		HelperObject*					m_Border = nullptr;
	};

	class _EchoComponentExport EchoOcclusionComponentInfo : public ActorComponentInfo {
	public:
		EchoOcclusionComponentInfo() = default;
		EchoOcclusionComponentInfo(const Name& type, ActorCOMMoveableType movable);

		~EchoOcclusionComponentInfo() = default;

		void							ExportData(cJSON* jsonNode) override;
		void							ImportData(cJSON* jsonNode) override;
		void							CopyTo(ActorComponentInfo* p_actor_com_info) override;
		void							OverWrite() override;

	public:
		void							setOcclusionShapeType(const OcclusionShapeType& occlusionShapeType, bool notChangeVertices = false);
		inline OcclusionShapeType		getOcclusionShapeType() const { return static_cast<OcclusionShapeType>(mOcclusionShapeType); }

	public:
		EDITOR_REFLECT

	public:
		int								mOcclusionShapeType{ 0 };

		std::vector<Vector3>			mVertices{4};

		std::vector<Echo::uint16>		mIndices;
		std::vector<Echo::uint16>		mBorderIndices;

		ColorValue						mOcclusionColor{ ColorValue::Blue };
	};

	DEFINECOMFACTORY(EchoOcclusionComponent, ALL_MOVEABLE)
} /// namespace Echo

#endif // !__ECHO_OCCLUSION_COMPONENT_H__
