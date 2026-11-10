///////////////////////////////////////////////////////////////////////////////
//
// @file EchoVegetationAreaComponent.h
// @author chensheng
// @date 2024/02/26 Monday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EchoVegetationAreaComponent_H__
#define _EchoVegetationAreaComponent_H__

#include "EchoComponentPrerequisites.h" // DLL导出宏需要
#include "EchoComDefineMacro.h" // 定义工厂类宏需要
#include "Actor/EchoActorComponent.h"
#include "EchoProtocolComponent.h"

namespace Echo
{
	////////////////////////////////VegetationAreaComponentInfo////////////////////////
	class _EchoComponentExport VegetationAreaComponentInfo : public ActorComponentInfo
	{
		friend class VegetationAreaComponent;
	public:
		struct SubVegParam {
			std::string				layer_name;
			float					density					=	1.f;
			float					scale_base				=	1.f;
			float					scale_range				=	0.1f;
			ColorValue				top_diffuse				=	ColorValue::White;
			float					top_diffuse_ratio		=	1.f;
			ColorValue				bottom_diffuse			=	ColorValue::White;
			float					bottom_diffuse_ratio	=	1.f;
			ColorValue				top_emission			=	ColorValue::Black;
			float					top_emission_ratio		=	1.f;
			ColorValue				bottom_emission			=	ColorValue::Black;
			float					bottom_emission_ratio	=	1.f;
			bool					needRefreshDistribution =   false;
			uint64					distributionSeed		=	0;
		};
		inline static uint32 SubVegParamNum = 12u;
		using SubVegParamVec = std::vector<SubVegParam>;

		struct AreaInfo {
			std::vector<Vector3>	points;					//	Building space 3D area points.
			Vector3					dir;					//	Building space area hit ray direction (hit space front/Y dir).
			Vector2					range;					//	Area hit direction depth: X - Min, Y - Max.
		};

		struct MeshInfo {
			Name					name;					//	Plantable mesh resource path name
			Matrix4					mat;					//	Mesh building space model transform matrix
			Vector3					min;					//	Mesh building space AxisAlignedBox min
			Vector3					max;					//	Mesh building space AxisAlignedBox max
		};
		using MeshInfoVec = std::vector<MeshInfo>;

	public:
		VegetationAreaComponentInfo()						=	default;
		~VegetationAreaComponentInfo()						=	default;
		VegetationAreaComponentInfo(const Name& type, ActorCOMMoveableType movable = UNMOVEABLE);

		virtual void				OverWrite()										override;
		virtual void				CopyTo(ActorComponentInfo* p_actor_com_info)	override;

	protected:
		virtual void				ExportData(cJSON* jsonNode)						override;
		virtual void				ImportData(cJSON* jsonNode)						override;

	public:
		AreaInfo					mAreaInfo;
		MeshInfoVec					mMeshInfoVec;
		SubVegParamVec				mSubVegParamVec;
		EDITOR_REFLECT
	};
	using VegAreaCompInfo = VegetationAreaComponentInfo;
	///////////////////////////////////////////////////////////////////////////////////
	// 
	//////////////////////////////////VegetationAreaComponent//////////////////////////
	class _EchoComponentExport VegetationAreaComponent : public ActorComponent
	{
		friend class VegAreaCompTestHelper;
		friend class EditorVegetationAreaComponentUI;
		DECLAREACTORCOMTYPE_H(VegetationAreaComponent);
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void				init(ActorComponentInfo* info)					override;
		virtual void				unInit()										override;
		virtual void				connectActor()									override;

	private:
		class VegetationArea*		getVegHandleForUI();
		void						changeParamForUI(const VegAreaCompInfo::SubVegParam& param);
		void						addParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, size_t index);
		void						removeParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, const std::string& layerName);
		void						setAreaInfoForUI(const VegAreaCompInfo::AreaInfo& areaInfo);
		void						setMeshInfoForUI(const VegAreaCompInfo::MeshInfoVec& meshInfoVec);
		void						updateParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, const std::string& layerName);
		void						updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask)override;
		void						visibleStateChanged(bool visibleState)override;
		bool						getAreaDesc(std::vector<Vector3>& out_area, Vector3& out_dir, Vector2& out_range) const;
		class VegAreaCompTestHelper&getTestHelper()const;
	private:
		virtual void				NotifiChildImp(int changeMask)					override;

	private:
		struct _Private;
		std::unique_ptr<_Private> _dp;
	};
	using VegAreaComp = VegetationAreaComponent;
	///////////////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////VegetationAreaComponentFactory///////////////////////
	class _EchoComponentExport VegetationAreaComponentFactory : public Echo::ActorComFactory
	{
		static_assert(std::is_same<decltype(UNMOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		VegetationAreaComponentFactory();
		virtual ~VegetationAreaComponentFactory();
		virtual Echo::EchoSharedPtr<Echo::ActorComponent> create(const Echo::ActorPtr& pActor)	override
		{
			return Echo::EchoSharedPtr<Echo::ActorComponent>
				(new VegetationAreaComponent(pActor, UNMOVEABLE), Echo::ActorComponentPtrDeleter);
		};
		virtual Echo::ActorComponentInfo* createComponentInfo()	override
		{
			return new VegetationAreaComponentInfo(mType, UNMOVEABLE);
		};
		static Echo::ProtocolResult ProCom_EnableImposter(void* pos, Echo::ProtocolHandle handle);
	};
	///////////////////////////////////////////////////////////////////////////////////
}
#endif
