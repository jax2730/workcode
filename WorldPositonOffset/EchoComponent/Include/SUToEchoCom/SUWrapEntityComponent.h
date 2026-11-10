#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class WrapEntityComponentInfo;
	class asset_pic_line;
	struct WrapEntityMeshReplaceStruct;
	class WrapEntityComponent :public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(WrapEntityComponent)
	public:
		virtual void				init(Echo::ActorComponentInfo* info) override;
		void						programInit(Echo::ActorComponentInfo& info);
		virtual void				bindNode() override;
		virtual void				connectCom()override;
		virtual void				disconnectCom()override;
		virtual void				replay(Echo::ActorComponentInfo* info) override {}
	public:
		struct PhyxData
		{
			int surfaceType;
			PhyxData(int t = 0) :
				surfaceType(t)
			{
			}
		};
		void						DestoryGroup();
		void						TryUpdate(int assetpic, bool is_fpp = false, bool regist = false);
		void						SetMaterial(std::vector<WrapEntityMeshReplaceStruct>& materialGroup);
		void						Create();
		ActorComponentPtr			CreatePhysx(const std::string& cmptName, std::string pathPhys, std::string pathMesh, bool convex, const PhyxData& phyData);
		ActorComponentPtr			CreatePhysx(const std::string& cmptName, const Echo::Vector3& collider, const PhyxData& phyData);
		ActorComponentPtr			CreateEntity(const std::string& cmptName, std::string pathMesh);
		void						SetInteractionColliderType(uint32 type);
		void						SetSelfColliderType(int GroupID, int filter_group);
		void						ResetInteractionColliderType();
		void						SetInteractionColliderTypeByFilterGroupID(int GroupID, int id);
		struct WrapTransFrom
		{
			Echo::Vector3 vPos = Echo::Vector3::ZERO;
			Echo::Vector3 vEular = Echo::Vector3::ZERO;
		};
		WrapTransFrom						ConvertDataToTransfrom(std::vector<float>& data);
		ActorComponentPtr					CreatePhysx(int id, int entityid);
		ActorComponentPtr					CreateEntity(int id, int entityid);
		ActorComponentPtr					CreateAnimator(int id);
		bool								FinishLoading() { return finishLoading; }
		void								ForceEntityRenderDis(); //强制设置
		void								RecoverEntityRenderDis();//强制回复
		std::vector<int>					GetGroupIDs();
		int									GetAssetPicID();
		void								FppWrapEntityEnterFpp();
		void								FppWrapEntityOutFpp();
		bool								IsClientPlayerOnSeat();
		void								TppWrapEntityEnterFpp();
		void								TppWrapEntityOutFpp();
		//对组内TransFrom
		void								SetEntityPosition(int EntityID, const Echo::Vector3& vPos);
		Echo::Vector3						GetEntityPosition(int EntityID);
		void								SetEntityRotation(int EnittyID, const Echo::Vector3& vEular);
		Echo::Vector3						GetEntityRotation(int EntityID);
		bool								HasEntity(int EntityID);
		Echo::ActorComponentPtr				GetColliderByID(int EntityID);
		//WrapEntityComponent::WrapTransFrom	ConvertMeshPathToMeshPhypath(std::basic_string_view<float>& data);
		std::string							GenCollsionPath(std::string& mesh_path, std::string& collision_path, bool& isConvex, int collider_type);
		std::string							ConvertMeshPathToMeshPhypath(const std::string& path);
	private:
		struct WrapEntityDataGroup
		{
			int								GroupID;
			int								mesh_group_id;
			Echo::ActorComponentPtr			mesh_node;
			Echo::ActorComponentPtr			mesh_relation;
			Echo::ActorComponentPtr			meshphy_relation;
			uint32							meshphy_relation_filter = -1;

		};

		std::map<int, WrapEntityDataGroup>		m_EntityData;
		asset_pic_line*							line_asset = nullptr;
		//int									m_asset_pic_id = -1;
		bool									finishLoading = false;

		bool                                     m_isFppWrap = false;
		std::vector<int>                         m_mesh_group_ids;
		bool                                     DoRegistCallBack = false;
		Echo::Vector3                            retZero;
	};

	class WrapEntityComponentInfo :public ActorComponentInfo
	{
	public:
		WrapEntityComponentInfo() = default;
		WrapEntityComponentInfo(const Name& type, ActorCOMMoveableType movable);

		~WrapEntityComponentInfo() = default;

		void							ExportData(cJSON* jsonNode) override;
		void							ImportData(cJSON* jsonNode) override;
		void							CopyTo(ActorComponentInfo* p_actor_com_info) override;
		void							OverWrite() override;
	public:
		int					asset_pic_id = -1;
		int					mesh_group_id = -1;
		int					wrap_mesh_group = -1;
		bool				m_is_fpp = false;
		bool				registCallBack = false;
		EDITOR_REFLECT
	};

	CLIENTDEFINECOMFACTORY(WrapEntityComponent, ALL_MOVEABLE)
}

