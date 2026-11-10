#pragma once

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoExcelDataHelper.h"
#include "Actor/EchoActor.h"

namespace Echo
{
	class asset_pic_line;
	class wrap_entity_line;
	class actorblock_list_line;
	class particle_list_line;
	class ActorFactoryComponentInfo;
	
	struct ActorCreateInfo
	{
		bool m_bBeginner			= false;
		bool m_bPassPersonStates	= false;
		bool m_bIsFpp				= false;
		bool m_ActorSave			= false;
		bool m_bHasCCT				= false; 
		bool m_bResourceAsync		= false; 
	};

	struct ReplaceMaterial
	{
		int         PartId;
		std::string m_materialPath;
	};

	struct WrapEntityMeshReplaceStruct
	{
		int                          wrapEntityID = 0; 
		std::string                  m_material_default_path;
		std::vector<ReplaceMaterial> m_partMateralData;
	};

	//class ActorBindBase;

	class ActorFactoryComponent :public ActorComponent, Echo::ActorLoadListener
	{
		DECLAREACTORCOMTYPE_H(ActorFactoryComponent)
		REGISTER_LUA_CLASS_KEY
	private:
		struct WrapTransFrom
		{
			Echo::Vector3 vPos		= Echo::Vector3::ZERO;
			Echo::Vector3 vEular	= Echo::Vector3::ZERO;
		};

	public:
		virtual void				init(ActorComponentInfo* info) override;
		virtual void				replay(ActorComponentInfo* info) override {}
		virtual void				connectCom() override;
		virtual void				bindNode() override;
		virtual void				connectActor() override;
		virtual void				disconnectCom() override;
		virtual void				disconnectActor() override;
		virtual bool				OnActorCreateFinish() override;
		bool						IsCreateSuccess();
	public:
		void						RemoveDynamicLoadStructrue();
		void						CreateDynamicLoadStructure();
		void						ClearActorBase();
		void						CreateBaseBlcok();
		void						CreateBaseBuilding();
		void						CreateBaseParticle();
		bool						CreateBuildingActor(int assetpic, bool isDynamic = false);
		bool						CreateBlockActor(bool haveToSave = false);
		std::vector<int>			GetBlockEnumType();
		void						UpdatePivot();
		bool						IsSameBlockEnumType(const std::vector<int>& enumIDs);
		bool						ContainsBlockEnumType(const std::vector<int>& enumIDs);
		void						SetMatrialData(std::vector<WrapEntityMeshReplaceStruct>& mat, bool forceUse = false);
		void						SetMaterialByID(int materialID);
		void						ReplaceCreateID(int id);
		void						InitComMb();
		int							GetBlockExcelID() { return m_ExcelID; }
		void						ChangeParent();
		void						UseMatrial();
		void						ConvertVVFToTransFrom(WrapTransFrom& transfrom, std::vector<std::vector<float>>& data);
		void						SetRunTimeMaterial(int sequence, int materialID, bool recursivly);
		static void					SetDefaultSync(bool b) { m_Defualt_Use_Synchronous = b;}

		bool						HasWrapEntity();
		void						ClearResource();
		void						SetBuildingVisble(bool b);

		int							GetExcelID() { return m_ExcelID; }
		eActorType					GetCreateType() { return m_CreateType; }
		bool						OldDataTransFrom(Echo::ActorComponentInfo* info);
		void						PivotSets(const std::vector<int>& ids);
		void						CallBeforeSave();
		void						TryUpdate(bool haveToSave = false);
		bool						TryUpdateBlock(bool haveToSave = false);
		bool						CreateActor(bool haveToSave = false);
		void						CreateComponentsByID(int id);
		void						RemoveBlock();
		void						RemoveActor();
		void						RmoveOldBlocks();
		void						InitDyn(Echo::ActorComponentInfo* info);
		std::string					ConvertTransfromDataToString();
		void						CreateWrapEntity();
	private:
		void						AssignLoadingBoxForBlockAct(int id);
		bool                        CreateCheckBox(bool havetoSave = false);
		bool                        CreateEntity(bool havetoSave = false);
		bool                        CreatePhysx(bool havetoSave = false);
		bool                        CreateParticle(bool havetoSave = false);
		std::vector<Echo::Vector3>	ConvertVVFToEchoVector3(const std::vector<std::vector<float>>& data);

		std::string					ConvertMeshPathToMeshPhypath(const std::string& path);
		std::string					GenCollsionPath(std::string& mesh_path, std::string& collision_path, bool& isConvex);
		void						UpdateTransFrom();
		void						SetMinMax(ActorPtr act, const Vector3& min, const Vector3& max, float scale);
		void						SetMinMax(ActorPtr act, const std::vector<std::vector<float>>& data, float scale, bool save = false);
		void						UpdateActorMinMax(const Echo::ActorPtr& ptr);

	private:
		bool											m_haveToSave		= false;
		asset_pic_line*									m_line_asset		= nullptr;
		actorblock_list_line*							m_line_actor_block	= nullptr;
		particle_list_line*								m_particle_list		= nullptr;
		building_list_line*								m_build_list		= nullptr;
		actorblock_material_line*						m_blockmaterial		= nullptr;
		int												m_ExcelID			= -1;
		bool											m_bHasWrapEntity	= false;
		bool											m_init				= false;
		bool											haveToSave			= false;

		ActorCreateInfo											m_create_info;
		std::vector<ActorPtr>									m_Actors;
		std::map<Echo::Name, std::vector<ActorComponentPtr>>	m_components;

		Echo::ActorPtr											m_CreateActor;
		static bool												m_Defualt_Use_Synchronous;
		std::vector<WrapEntityMeshReplaceStruct>				m_material_record;
		std::vector<Echo::ActorPtr>								m_DynamicLoadActors;

	private:
		std::string												m_editor_type_name	= "unkownType";
		eActorType												m_CreateType		= eActorType::eActorTypeNone;
		AxisAlignedBox											mMinMaxBox;
	};

	class ActorFactoryComponentInfo :public ActorComponentInfo
	{
	public:
		ActorFactoryComponentInfo() = default;
		ActorFactoryComponentInfo(const Name& type, ActorCOMMoveableType movable);

		~ActorFactoryComponentInfo() = default;

		void							ExportData(cJSON* jsonNode) override;
		void							ImportData(cJSON* jsonNode) override;
		void							CopyTo(ActorComponentInfo* p_actor_com_info) override;
		void							OverWrite() override;
	public:
		int											actor_block_id		= -1;
		int											actor_create_type	= 6; 
		int											actor_particle_id	= -1;
		bool										isFpp				= false;
		bool										init				= true;
		ActorCreateInfo								m_CreateBeginInfo;
		std::vector<WrapEntityMeshReplaceStruct>	m_wrapRepalceMaterial;
		EDITOR_REFLECT
	};

	CLIENTDEFINECOMFACTORY(ActorFactoryComponent, UNMOVEABLE)
	//class ActorFactoryComponentFactory : public Echo::ActorComFactory
	//{
	//	static_assert(std::is_same<decltype(UNMOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!"); 
	//public:
	//	ActorFactoryComponentFactory() :ActorComFactory() { mType = Echo::Name("ActorFactoryComponent"); mMoveableType = UNMOVEABLE; bIsEditorCom = true; mPathGizmoIcon = Echo::Name("ActorFactoryComponent"); }
	//	virtual ~ActorFactoryComponentFactory() { ; }
	//	virtual Echo::EchoSharedPtr<Echo::ActorComponent>		create(const Echo::ActorPtr& pActor)	override { return Echo::EchoSharedPtr<Echo::ActorComponent>(new ActorFactoryComponent(pActor, mMoveableType)); }
	//	virtual Echo::ActorComponentInfo* createComponentInfo()	override { return new ActorFactoryComponentInfo(mType, mMoveableType); }
	//}; 
}
