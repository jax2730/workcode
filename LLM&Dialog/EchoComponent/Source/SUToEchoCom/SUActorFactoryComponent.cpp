#include "EchoComponentStableHeaders.h"
#include "Actor/EchoActorManager.h"
#include "SUToEchoCom/SUActorFactoryComponent.h"
#include "EchoActorEntityComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoBoxColliderComponent.h"
#include "EchoMeshColliderComponent.h"
#include "EchoParticleSystemComponent.h"
#include "EchoSqliteMgr.h"
#include "EchoSqliteObject.h"
#include "EchoSqlite.h"
#include "SUToEchoCom/SUWrapEntityComponent.h"
#include "SUToEchoCom/EchoExcelDataHelper.h"
#include "SUToEchoCom/SUDynamicLoadComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(ActorFactoryComponent)
	//EDITOR_REFLECT_BEGIN(ActorFactoryComponentInfo)
	//EDITOR_REFLECT_MEMBER_UPDATE(useNewStyle, "useNewStyle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(actor_block_id, "actor_block_id", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_MEMBER_UPDATE(actor_particle_id, "actor_particle_id", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	//EDITOR_REFLECT_END()

	ActorFactoryComponent::ActorFactoryComponent(ActorPtr pActor, ActorCOMMoveableType moveType)
		:ActorComponent(pActor, moveType)
	{
	}

	ActorFactoryComponent::~ActorFactoryComponent()
	{
		//SAFE_DELETE(m_line_actor_block)
		//SAFE_DELETE(m_line_asset)
		//SAFE_DELETE(m_particle_list)
	}

	void ActorFactoryComponent::init(ActorComponentInfo* info)
	{
		InitDyn(info);
	}

	void ActorFactoryComponent::connectCom()
	{
		TryUpdate(false);
		auto inf = static_cast<ActorFactoryComponentInfo*>(m_pActComInfo);
		SetMatrialData(inf->m_wrapRepalceMaterial, true);
	}

	void ActorFactoryComponent::connectActor()
	{
	}

	void ActorFactoryComponent::bindNode()
	{
	}

	void ActorFactoryComponent::disconnectCom()
	{
	}

	void ActorFactoryComponent::disconnectActor()
	{

	}

	bool ActorFactoryComponent::OnActorCreateFinish()
	{
		return true;
		if (!m_CreateActor.expired())
		{
			std::deque<Echo::ActorPtr> noFacActors;
			noFacActors.push_back(m_CreateActor);
			while (noFacActors.size() > 0)
			{
				if (noFacActors.front()->GetFirstComByType<ActorFactoryComponent>().expired())
				{
					auto root = static_cast<Echo::NodeComponent*>(noFacActors.front()->getRootNodeCom().get());
					root->setLoadBoundingBox(mMinMaxBox);
					root->RootNodeUpdate();
					for (auto& p : noFacActors.front()->getChildActors())
					{
						noFacActors.push_back(p.second);
					}
				}
				noFacActors.pop_front();
			}
		}
		return true;
	}

	bool ActorFactoryComponent::IsCreateSuccess()
	{
		if (m_CreateType == eActorTypeBlock
			|| m_CreateType == eActorTypeBuilding
			|| m_CreateType == eActorTypeParticle)
		{
			return true;
		}

		//if (!create_base) return false;
		//else
		//	return create_base->m_createSuccess;
		return false;
	}

	bool ActorFactoryComponent::OldDataTransFrom(Echo::ActorComponentInfo* info)
	{
		auto m_info = static_cast<ActorFactoryComponentInfo*>(info);
		if (m_info->actor_particle_id != -1)
		{
			m_CreateType = eActorType::eActorTypeParticle;
			m_ExcelID = m_info->actor_particle_id;
			return true;
		}
		return false;
	}

	void ActorFactoryComponent::PivotSets(const std::vector<int>& ids)
	{
		if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
			return;
	}

	bool ActorFactoryComponent::HasWrapEntity()
	{
		return m_bHasWrapEntity;
	}
	void ActorFactoryComponent::ClearResource()
	{
		RmoveOldBlocks();
		RemoveActor();
		ClearActorBase();
	}

	void ActorFactoryComponent::SetBuildingVisble(bool b)
	{
		if (m_CreateActor.expired()) return;
		m_CreateActor->setRuntimeVisible(b);
	}

	void ActorFactoryComponent::CallBeforeSave()
	{

	}
	void ActorFactoryComponent::TryUpdate(bool haveToSave /*= false*/)
	{
		ClearResource();//先清了
		switch (m_CreateType)
		{
		case eActorTypeNone:
			break;
		case eActorTypeBlock:
			CreateBaseBlcok();
			break;
		case eActorTypeBuilding:
			if (false)	//修改是否动态加载结构
			{
				CreateDynamicLoadStructure();
			}
			else
			{
				CreateBaseBuilding();
			}
			break;
		case eActorTypeParticle:
			CreateBaseParticle();
			break;
		default: //走ActorBase 创建
		{
			//Echo::ActorPtr act = getActor()->weak_from_this();
			//if (create_base)
			//{
			//	ClearActorBase();
			//}
			//if (!m_create_info.m_bIsFpp)
			//{
			//	create_base = ClientActorCreationFactory::GetInst()->TppCreation(m_CreateType, act, m_ExcelID, m_init, m_create_info);
			//}
			//else
			//{
			//	create_base = ClientActorCreationFactory::GetInst()->FppCreation(m_CreateType, act, m_ExcelID, m_init, m_create_info);
			//}
			break;
		}
		}
		UpdateTransFrom();
		UpdatePivot();
		//if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		//{
		//	getActor()->setDynamic(true);
		//}
	}

	bool ActorFactoryComponent::TryUpdateBlock(bool haveToSave /*= false*/)
	{
		CreateBlockActor();
		CreateWrapEntity();
		//CreateParticle(havetoSave); 

		return true;
	}

	void  ActorFactoryComponent::CreateBaseBlcok()
	{
		TryUpdateBlock();
	}
	void ActorFactoryComponent::CreateBaseBuilding()
	{
		if (!m_build_list || m_build_list->isNull())
			return;
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			CreateBuildingActor(m_build_list->asset_id(), true);
		else
			CreateBuildingActor(m_build_list->asset_id());
		InitComMb();

	}

	void ActorFactoryComponent::ClearActorBase()
	{

	}

	bool ActorFactoryComponent::CreateBuildingActor(int assetpic, bool isDynamic)
	{
		auto line_asset = GetExcelLineData(asset_pic_building, assetpic);

		if (!line_asset || line_asset->isNull()) return false;
		auto comp = this->getParentNodeCom();
		std::string path = line_asset->actor_path();
		if (path.empty())
			return false;
		path = "actors/" + path;

		std::replace(path.begin(), path.end(), '\\', '/');
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			ExcelDataManager::instance()->collectRes(path);
		}
		ActorPtr actor  = nullptr;
		Echo::SceneManager* customManager = nullptr;
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			isDynamic = true;
			customManager = this->getActor()->GetSceneManager();
		}

		st_ActorCreateInfo aci{};
		aci.m_pEntityBatchCom = getActor()->getEntityObjBatchCom();
		aci.m_pBuildingCom = getActor()->getBuildingCom();
		if (isDynamic) 
		{
#ifdef ECHO_EDITOR
			actor = Echo::ActorSystem::instance()->getMainActorManager()->createVirtualActor(Echo::Name(path), "", true, false, customManager,&aci);
#else
			actor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Echo::Name(path), "", true, false, customManager, &aci);
#endif
		}
		else  
		{
			actor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Echo::Name(path), "", false, true, customManager, &aci);
		}
		this->getActor()->addActor(comp, actor, false);
		if (!m_CreateActor.expired())
		{
			m_CreateActor->addActorLoadListener(nullptr);
			Echo::ActorSystem::instance()->getMainActorManager()->deleteActor(m_CreateActor);
		}
		m_CreateActor = actor;
		if (!m_CreateActor.expired())
		{
			// m_CreateActor->setPosition(Echo::Vector3::ZERO);
			// m_CreateActor->setRotation(Echo::Quaternion::IDENTITY);
			// m_CreateActor->setScale(Echo::Vector3(1, 1, 1));
			m_CreateActor->addActorLoadListener(this);
		}
		return true;
	}

	bool ActorFactoryComponent::CreateBlockActor(bool haveToSave)
	{
		if (m_line_actor_block && !m_line_actor_block->isNull())
		{

			auto           comp = weak_from_this();
			auto path = std::string(m_line_asset->actor_path());
			if (path.empty())
				return false;
			path = "actors/" + path;
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			{
				ExcelDataManager::instance()->collectRes(path);
			}
			Echo::ActorPtr actor;

			st_ActorCreateInfo aci{};
			aci.m_pEntityBatchCom = getActor()->getEntityObjBatchCom();
			aci.m_pBuildingCom = getActor()->getBuildingCom();

			{
#ifdef ECHO_EDITOR
				actor = Echo::ActorSystem::instance()->getMainActorManager()->createVirtualActor(Echo::Name(path), "", true, false, NULL, &aci);
#else
				actor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(Echo::Name(path), "", true, false, NULL, &aci);
#endif

				if (!actor.expired())
				{
					this->getActor()->addActor(comp, actor, false);
					m_components[actor->getRootNodeCom()->getType()].push_back(actor->getRootNodeCom()->weak_from_this());
				}
			}
			return true;
		}
		return false;
	}

	void ActorFactoryComponent::CreateBaseParticle()
	{
		//CreateCheckBox(false); //创建盒子
		CreateParticle(false);
		//CreateSlot();
		//CreateAtb();
		//CreateEnum();
	}

	std::vector<int> ActorFactoryComponent::GetBlockEnumType()
	{
		std::vector<int> ret;
		
		if (m_line_asset && !m_line_asset->isNull())
		{

			if (m_line_actor_block&& !m_line_actor_block->isNull())
			{
				for (auto& p : m_line_actor_block->actor_enum_list())
				{
					ret.push_back(p);
				}
			}
		}
		return ret;
	}


	bool ActorFactoryComponent::ContainsBlockEnumType(const std::vector<int>& enumIDs)
	{
		if (enumIDs.size() == 0)return false;
		auto block_enum = GetBlockEnumType();
		size_t  counter = enumIDs.size();
		auto block_enum_input = enumIDs;
		for (auto iter = block_enum_input.begin(); iter != block_enum_input.end();)
		{
			bool find = false;
			for (auto& p : block_enum)
			{
				if (p == *iter)
				{
					iter = block_enum_input.erase(iter);
					find = true;
					counter--;
					break;
				}
			}
			if (!find)
				++iter;
		}
		if (counter == 0) return true;
		return false;

	}

	bool ActorFactoryComponent::IsSameBlockEnumType(const std::vector<int>& enumIDs)
	{
		auto block_enum = GetBlockEnumType();
		if (enumIDs.size() != block_enum.size()) return false;
		size_t counter = block_enum.size();
		for (auto iter = block_enum.begin(); iter != block_enum.end();)
		{
			bool find = false;
			for (auto& p : enumIDs)
			{
				if (p == *iter)
				{
					iter = block_enum.erase(iter);
					find = true;
					counter--;
					break;
				}
			}
			if (!find)
				++iter;
		}
		if (counter == 0) return true;
		return false;
	}

	void ActorFactoryComponent::SetMatrialData(std::vector<WrapEntityMeshReplaceStruct>& mat, bool forceUse)
	{
		if (forceUse)
		{
			UseMatrial();
		}
	}

	void ActorFactoryComponent::SetRunTimeMaterial(int sequence, int materialID, bool recursivly)
	{
		if (m_CreateType == eActorType::eActorTypeBlock)
		{
			return;
		}
		std::vector<Echo::ActorPtr> m_Actors;
		if (recursivly)
		{
			std::deque<Echo::ActorPtr> m_queueActor;
			m_queueActor.push_back(this->getActor());

			while (m_queueActor.size() > 0)
			{
				if (!m_queueActor.front().expired())
				{
					for (auto& p : m_queueActor.front()->getChildActors())
					{
						m_queueActor.push_back(p.second);
					}
				}
				m_Actors.push_back(m_queueActor.front());
				m_queueActor.pop_front();
			}
		}
		else
		{
			m_Actors.push_back(this->getActor());
		}

		//for (auto& p : m_Actors)
		//{
		//	auto entities = GetComponents<WrapEntityComponent>(p);
		//	for (auto& entity : entities)
		//	{

		//	}
		//	auto roles = GetComponents<WrapRoleComponent>(p);
		//	for (auto& role : roles)
		//	{


		//	}
		//}
	}


	void ActorFactoryComponent::ReplaceCreateID(int id)
	{
		auto m_Info = static_cast<ActorFactoryComponentInfo*>(this->m_pActComInfo);
		m_Info->actor_block_id = id;
		m_Info->m_wrapRepalceMaterial.clear();
		InitDyn(m_Info);//重新初始化
		TryUpdate();
	}

	void ActorFactoryComponent::SetMaterialByID(int materialID)
	{
		//重新刷新

		//TryUpdate();

		if (!m_blockmaterial || m_blockmaterial->isNull())
			return;

		if (materialID == 1)
		{
			std::vector<WrapEntityMeshReplaceStruct> mats;
			for (auto& p : m_components[WrapEntityComponent::mType])
			{
				auto entity = static_cast<WrapEntityComponent*>(p.get());
				auto groups = entity->GetGroupIDs();

				for (auto& q : m_material_record)
				{
					q.m_partMateralData.clear();
				}
			}
			SetMatrialData(m_material_record, true);
			
		}
		else
		{

			std::vector<WrapEntityMeshReplaceStruct> mats;
			for (auto& p : m_components[WrapEntityComponent::mType])
			{
				auto entity = static_cast<WrapEntityComponent*>(p.get());
				auto groups = entity->GetGroupIDs();
				for (auto& p : groups)
				{
					WrapEntityMeshReplaceStruct v;
					v.wrapEntityID = p;
					v.m_material_default_path = "";
					v.m_partMateralData.clear();
					for (auto& q : m_blockmaterial->material_Type())
					{
						ReplaceMaterial mat;
						mat.PartId = q;
						mat.m_materialPath = m_blockmaterial->material();
						mat.m_materialPath += ".material";
						v.m_partMateralData.emplace_back(std::move(mat));
					}
					mats.emplace_back(std::move(v));
				}
			}
			SetMatrialData(mats, true);
		}
	}

	void ActorFactoryComponent::UseMatrial()
	{
		for (auto& p : m_components[WrapEntityComponent::mType])
		{
			if (!p.expired())
			{
				static_cast<WrapEntityComponent*>(p.get())->SetMaterial(m_material_record);
			}
		}
	}

	void ActorFactoryComponent::UpdatePivot()
	{
		if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
			return;

		auto wrapEntity = getActor()->GetFirstComByType<WrapEntityComponent>();
		if (wrapEntity.expired()) return;
		else
		{
			PivotSets(wrapEntity->GetGroupIDs()); //设置
		}

	}

	bool ActorFactoryComponent::CreateActor(bool haveToSave/* = false*/)
	{
		if (!m_line_asset)
		{
			return false;
		}
		auto comp = this->getParentNodeCom();
		auto path = "actors/" + m_line_asset->actor_path();
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			ExcelDataManager::instance()->collectRes(path);
		}
		Echo::ActorPtr actor;			
		st_ActorCreateInfo aci{};
		aci.m_pEntityBatchCom = getActor()->getEntityObjBatchCom();
		aci.m_pBuildingCom = getActor()->getBuildingCom();

		{
			if (!m_line_asset->isNull())
			{

				actor = Echo::ActorSystem::instance()->getMainActorManager()->createActor(nullptr, false, &aci);
				actor->setActorID("BlcokPic");
			}
		}
		//else
		//{
		//	actor = Echo::ActorSystem::instance()->getMainActorManager()->createVirtualActor(Echo::Name(path));
		//}
		this->getActor()->addActor(comp, actor);
		
		return true;
	}
	void ActorFactoryComponent::CreateComponentsByID(int id)
	{
	}

	void ActorFactoryComponent::RemoveBlock()
	{
	}
	void ActorFactoryComponent::RemoveActor()
	{
		if (!m_CreateActor.expired())
			Echo::ActorSystem::instance()->getMainActorManager()->deleteActor(m_CreateActor);
	}
	void ActorFactoryComponent::RmoveOldBlocks()
	{
		for (auto& p : m_components)
		{
			for (auto& q : p.second)
			{
				if (!q.expired())
					getActor()->delComponent(q);
			}
		}
		m_components.clear();
		m_bHasWrapEntity = false;
	}



	void ActorFactoryComponent::RemoveDynamicLoadStructrue()
	{
		for (auto& p : m_DynamicLoadActors)
		{
			if (p.expired()) continue;
			Echo::ActorSystem::instance()->getMainActorManager()->deleteActor(p);
		}
		m_DynamicLoadActors.clear();
	}

	void ActorFactoryComponent::CreateDynamicLoadStructure()
	{
		auto act1 = Echo::ActorSystem::instance()->getMainActorManager()->createActor(nullptr, true);
		auto act2 = Echo::ActorSystem::instance()->getMainActorManager()->createActor(nullptr, true);
		auto act3 = Echo::ActorSystem::instance()->getMainActorManager()->createActor(nullptr, true);
		act1->setActorID("ImposterController");
		act1->setActorInfoID("ImposterController");
		act2->setActorID("HlodController");
		act2->setActorInfoID("HlodController");
		act3->setActorID("BuildingController");
		act3->setActorInfoID("BuildingController");
		act1->m_bRetain = true;
		act2->m_bRetain = true;
		act3->m_bRetain = true;
		//act1->getStaticKey();
		//act1->getRootNodeCom()->getLoadBoundingBox()
		this->getActor()->addActor(this->getActor()->getRootNodeCom(), act1);
		act1->addActor(act1->getRootNodeCom(), act2);
		act2->addActor(act2->getRootNodeCom(), act3);

		auto d1 = act1->AddComponent<DynamicLoadComponent>("DynamicLoadComponent", act1->getRootNodeCom(), nullptr, 0, true, true); //不自动connectcom
		auto d2 = act2->AddComponent<DynamicLoadComponent>("DynamicLoadComponent", act2->getRootNodeCom(), nullptr, 0, true, true);
		auto d3 = act3->AddComponent<DynamicLoadComponent>("DynamicLoadComponent", act3->getRootNodeCom(), nullptr, 0, true, true);
		d1->m_bRetain = true;
		d2->m_bRetain = true;
		d3->m_bRetain = true;

		auto buildingInfo = GetExcelLineData(building_info, m_ExcelID);
		auto buildlist = GetExcelLineData(building_list, m_ExcelID);
		if (buildingInfo && buildlist)
		{
			auto imScale = buildlist->Imposter_scale();
			if (imScale == 0) imScale = 10;
			auto hlScale = buildlist->hlod_scale();
			if (hlScale == 0) hlScale = 5;
			auto buildingScale = buildlist->building_scale();
			if (buildingScale == 0) buildingScale = 2;
			auto boxSize = buildingInfo->boxSize();
			SetMinMax(act1, boxSize, imScale);
			SetMinMax(act2, boxSize, hlScale);
			SetMinMax(act3, boxSize, buildingScale, true);
			auto m_min_max = ConvertVVFToEchoVector3(buildingInfo->boxSize());
			if (m_min_max.size() >= 2)
			{
				//float half = (m_min_max[1] - m_min_max[0]).length() / 2.f;
				//auto  hfLength = half * 1;
				//auto actual_min = m_min_max[0] - Echo::Vector3(hfLength);
				//auto actual_max = m_min_max[1] + Echo::Vector3(hfLength);
				d1->SetMinMax(m_min_max[0], m_min_max[1]);
			}
		}
		else
		{
			SetMinMax(act1, Echo::Vector3(-100, 0, -100), Echo::Vector3(100, 100, 100), 10);
			SetMinMax(act2, Echo::Vector3(-100, 0, -100), Echo::Vector3(100, 100, 100), 5);
			SetMinMax(act3, Echo::Vector3(-100, 0, -100), Echo::Vector3(100, 100, 100), 2);
		}

		//数据设置
		d1->SetLevel(RenderSwitchLevel::InposterRender);
		if (buildlist)
		{
			if (buildlist->generate_imposter())
			{
				d1->CustomerData.push_back(buildlist->imposter_render_dis());
			}
			if (buildlist->generate_hlod())
			{
				auto hlodfac = buildlist->hlod_factor();
				if (hlodfac.size() > 0)
				{
					d2->CustomerData.push_back(hlodfac[0]);
				}
				if (buildlist->hlod_factor().size() > 1)
				{
					d2->CustomerData.push_back(hlodfac[1]);
				}

				if (buildlist->hlod_factor().size() > 2)
				{
					d2->CustomerData.push_back(hlodfac[2]);
				}
			}
		}
		else
		{
			//if (buildlist->generate_imposter())
			{
				d1->CustomerData.push_back(1500);
			}
			//if (buildlist->generate_hlod())
			{
				d2->CustomerData.push_back(50);
				d2->CustomerData.push_back(500);
				d2->CustomerData.push_back(1500);
			}
		}
		d2->SetLevel(RenderSwitchLevel::HlodRender);
		d3->SetLevel(RenderSwitchLevel::NormalRender);
		act1->setLoad(false);
		act2->setLoad(false);
		act3->setLoad(false);
		m_DynamicLoadActors.emplace_back(act1);
		m_DynamicLoadActors.emplace_back(act2);
		m_DynamicLoadActors.emplace_back(act3);

	}

	void ActorFactoryComponent::InitDyn(Echo::ActorComponentInfo* info)
	{
		haveToSave = false;
		if (info)
		{
			auto comInfo = static_cast<ActorFactoryComponentInfo*>(info);
			if (!OldDataTransFrom(info))
			{
				m_CreateType = eActorType(comInfo->actor_create_type);
				m_ExcelID = comInfo->actor_block_id;
				m_init = comInfo->init;
				m_create_info = comInfo->m_CreateBeginInfo;
				m_material_record = comInfo->m_wrapRepalceMaterial;
			}
			//auto build_ = static_cast<building_list*>(ExcelDataManager::instance()->getExcelByName("building_list"));
			m_build_list = GetExcelLineData(building_list, m_ExcelID);
			m_haveToSave = haveToSave;
			//auto actor_block = static_cast<actorblock_list*>(ExcelDataManager::instance()->getExcelByName("actorblock_list"));
			m_line_actor_block = GetExcelLineData(actorblock_list, m_ExcelID);
			if (!m_line_actor_block)
				return;
			//auto asset_ = static_cast<asset_pic_block*>(ExcelDataManager::instance()->getExcelByName("asset_pic_block"));
			m_line_asset = GetExcelLineData(asset_pic_block, m_line_actor_block->asset_pic());
			//auto particle_ = static_cast<particle_list*>(ExcelDataManager::instance()->getExcelByName("particle_list"));
			m_particle_list = GetExcelLineData(particle_list, comInfo->actor_particle_id);

		}

		
	}
	bool ActorFactoryComponent::CreateCheckBox(bool havetoSave/* = false*/)
	{
		return false;
	}

	bool ActorFactoryComponent::CreateEntity(bool havetoSave /*= false*/)
	{
		if (!m_line_actor_block || m_line_actor_block->isNull())
			return false;

		if (!m_line_asset || m_line_asset->isNull())
			return false;



		std::string str_mesh = "";
		std::vector<int> wrap_entitys = { m_line_asset->wrap_entity_id() };
		//str_mesh                     = line_asset->mesh_path();


		if (wrap_entitys.size() == 0)
			return false;

		auto parentCom = weak_from_this(); 

		for (int i = 0; i < wrap_entitys.size(); i++)
		{
			int  id = wrap_entitys[i];

			//wrap_entity* wrap_ = static_cast<wrap_entity*>(ExcelDataManager::instance()->getExcelByName("wrap_entity"));
			auto line_wrap = GetExcelLineData(wrap_entity, id);
			if (line_wrap &&!line_wrap->isNull())
			{
				std::string str_mesh = line_wrap->mesh_path();
				auto        comp = weak_from_this();
				std::string name = "EntityComponent" + std::to_string(id);
				auto        entity = getActor()->AddComponent<EntityComponent>(name, weak_from_this(), nullptr, 0, !haveToSave);

				auto        info = static_cast<Echo::EntityComponentInfo*>(entity->m_pActComInfo);
				std::string strPath = str_mesh;
				std::replace(std::begin(strPath), std::end(strPath), '\\', '/');

				info->SetMeshName(strPath.c_str());
				entity->loadEntity(Echo::Name(info->GetMeshName()));
				m_components[entity->getType()].push_back(entity->weak_from_this());
			}
		}
		
		return true;
	}

	void ActorFactoryComponent::CreateWrapEntity()
	{
		if (!m_line_asset || m_line_asset->isNull())
			return;

		auto component = this->getActor()->AddComponent<WrapEntityComponent>("WrapEntityComponent", this->getParentNodeCom(), 0, 0, true);
		if (component)
		{
			m_editor_type_name = "block";
			component->TryUpdate(m_line_asset->id());      //wrap entity 组件
			component->SetMaterial(m_material_record); //更新entity
			m_components[component->getType()].push_back(component->weak_from_this());
		}
		m_bHasWrapEntity = true;
	}

	void ActorFactoryComponent::ChangeParent()
	{
		for (auto& p : m_components)
		{
			for (auto& q : p.second)
			{
				if (!q.expired())
				{
					getActor()->addComponent(weak_from_this(), q);
				}
			}
		}
	}

	void ActorFactoryComponent::AssignLoadingBoxForBlockAct(int id)
	{
		return;
		float render_length = 0;
		auto line_actor_block = GetExcelLineData(actorblock_list, m_ExcelID); //actor block
		if (line_actor_block)
		{
			auto line_asset = GetExcelLineData(asset_pic_block, line_actor_block->asset_pic());
			if (!line_asset) return;

			for (auto p : line_asset->wrap_entity_id())
			{
				auto wrap = GetExcelLineData(wrap_entity, p);
				if (!wrap) continue;
				if (wrap->sight_dis_param() > render_length) render_length = wrap->sight_dis_param();
			}
			if (render_length == 0) render_length = 1500;

			line_actor_block->boxSize();
			Echo::Vector3 min = Echo::Vector3(-5, -5, -5);
			Echo::Vector3 max = Echo::Vector3(5, 5, 5);
			if (line_actor_block->boxSize().size() >= 3)
			{
				min = { -line_actor_block->boxSize()[0] / 2,
					   0,
					   -line_actor_block->boxSize()[1] / 2 };
				max = { line_actor_block->boxSize()[0] / 2,
					   line_actor_block->boxSize()[2],
					   line_actor_block->boxSize()[1] / 2 };
			}
			min = min - Echo::Vector3(render_length);
			max = max + Echo::Vector3(render_length);

			AxisAlignedBox box;
			box.setMinimum(min);
			box.setMaximum(max);
			auto root = static_cast<Echo::NodeComponent*>(getActor()->getRootNodeCom().get());
			root->setLoadBoundingBox(box);
			root->RootNodeUpdate();
		}
	}

	void ActorFactoryComponent::ConvertVVFToTransFrom(WrapTransFrom& transfrom, std::vector<std::vector<float>>& data)
	{
		if (data.size() >= 1)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (data[0].size() > i)
				{
					transfrom.vPos[i] = data[0][i];
				}
			}
		}
		if (data.size() >= 2)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (data[1].size() > i)
				{
					transfrom.vEular[i] = data[1][i];
				}
			}
		}
	}

	std::string ActorFactoryComponent::ConvertTransfromDataToString()
	{
		auto vPos = getPosition();
		auto vEular = getEulerAngles();

		std::string retData;
		for (int i = 0; i < 3; ++i)
		{
			retData += std::to_string(vPos[i]);
			if (i < 2)
			{
				retData += "*";
			}
		}
		retData += "|";
		for (int i = 0; i < 3; ++i)
		{
			retData += std::to_string(vEular[i]);
			if (i < 2)
			{
				retData += "*";
			}
		}
		return retData;
	}


	bool ActorFactoryComponent::CreatePhysx(bool havetoSave /*= false*/)
	{
		if (!m_line_asset || m_line_asset->isNull())
			return false;

		Echo::Vector3 boxSize = Echo::Vector3::ZERO;
		Echo::Vector3 collider_box = Echo::Vector3::ZERO;
		std::vector<float> boxsize = m_line_actor_block->boxSize();
		if (m_line_asset && boxsize.size() >= 3)
		{
			collider_box = { boxsize[0], boxsize[1], boxsize[2] };
		}

		if (!m_line_asset->isNull())
		{
			int excelID = -1;
			excelID = m_line_asset->id();

			auto parentCom = getActor()->GetComponent<Echo::MeshColliderComponent>("MeshColliderComponent_node");
			if (parentCom.get() == nullptr)
			{
				parentCom = getActor()->AddComponent<MeshColliderComponent>("MeshColliderComponent_node", this->getParentNodeCom(), nullptr, 0, !havetoSave);
				m_components[parentCom->getType()].push_back(parentCom->weak_from_this());
			}

			std::vector<int> wrap_entitys = m_line_asset->wrap_entity_id();
			for (int i = 0; i < wrap_entitys.size(); i++)
			{
				int  id = wrap_entitys[i];
				auto line_wrap = GetExcelLineData(wrap_entity, id);

				if (line_wrap && !line_wrap->isNull())
				{
					int         collider_type = 0;
					std::string collision = "";
					std::string mesh_path = "";
					bool        isConvex = true;
					collider_type = line_wrap->collider_type();
					mesh_path = line_wrap->mesh_path();
					collision = line_wrap->meshphy_path();
					collision = GenCollsionPath(mesh_path, collision, isConvex);

					bool useMesh = false;

					if (collider_type == 0)
					{
						useMesh = true;
					}
					else if (collider_type == 2)
					{
						useMesh = false;
					}

					if (useMesh)
					{
						if (std::string(collision) != "" || std::string(mesh_path) != "")
						{
							std::string meshName = "MeshColliderComponent";// +std::to_string(id);
							auto        mesh = getActor()->GetComponent<MeshColliderComponent>(meshName);
							if (mesh == nullptr)
							{
								mesh = getActor()->AddComponent<MeshColliderComponent>(meshName, parentCom->weak_from_this(), nullptr, 0, !havetoSave);
							}
							auto        meshInfo = static_cast<Echo::MeshColliderComponentInfo*>(mesh->m_pActComInfo);
							meshInfo->SetIsConvex(true);
							meshInfo->SetIsTrigger(true);
							meshInfo->SetVisible(true);

							if (!isConvex)
							{
								meshInfo->SetMeshName(Echo::Name(mesh_path.c_str()));
								mesh->SetMeshName_mesh(meshInfo->GetMeshName());
								mesh->SetIsConvex(meshInfo->GetIsConvex());
								mesh->setVisible(meshInfo->GetVisible());
								if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
									mesh->ExportMeshphyToCooked();

							}
							else
							{
								meshInfo->SetMeshName(Echo::Name(collision.c_str()));
								mesh->SetIsConvex(meshInfo->GetIsConvex());
								mesh->setVisible(meshInfo->GetVisible());

								mesh->SetMeshName_mesh(meshInfo->GetMeshName());
								mesh->setVisible(true); //暂时先强制开启
							}
							//mesh->SetMeshData
							m_components[mesh->getType()].push_back(mesh->weak_from_this());
							}
						else
						{
							goto LAST_BOX;
						}
						}
					else
					{
						if (collider_box.length() != 0.f)
						{
							std::string colliderName = "BoxColliderComponent" + std::to_string(id);
							auto        boxcomponent = getActor()->AddComponent<BoxColliderComponent>(colliderName, parentCom->weak_from_this(), nullptr, 0, !havetoSave);
							auto        boxinfo = static_cast<Echo::BoxColliderComponentInfo*>(boxcomponent->m_pActComInfo);
							boxinfo->SetVisible(true);
							boxinfo->SetHalfSize(Echo::Vector3(collider_box[0] / 2.f, collider_box[2] / 2.f, collider_box[1] / 2.f));
							boxinfo->SetPosition(Echo::Vector3(0, collider_box[2] / 2.f, 0)); //向下偏移
							boxcomponent->SetHalfSize(boxinfo->GetHalfSize());
							boxcomponent->setPosition(boxinfo->GetPosition());
							boxcomponent->setVisible(boxinfo->GetVisible());
							m_components[boxcomponent->getType()].push_back(boxcomponent->weak_from_this());
						}
						else
						{
						LAST_BOX:
							std::string colliderName = "BoxColliderComponent" + std::to_string(id);
							if (boxSize.length() != 0)
							{
								auto boxcomponent = getActor()->AddComponent<BoxColliderComponent>(colliderName, parentCom->weak_from_this(), nullptr, 0, !havetoSave);
								auto boxinfo = static_cast<Echo::BoxColliderComponentInfo*>(boxcomponent->m_pActComInfo);
								boxinfo->SetVisible(true);
								boxinfo->SetHalfSize(Echo::Vector3(boxSize[0] / 2.f, boxSize[2] / 2.f, boxSize[1] / 2.f));
								boxinfo->SetPosition(Echo::Vector3(0, boxSize[2] / 2.f, 0)); //向下偏移
								boxcomponent->SetHalfSize(boxinfo->GetHalfSize());
								boxcomponent->setPosition(boxinfo->GetPosition());
								boxcomponent->setVisible(boxinfo->GetVisible()); 
								m_components[boxcomponent->getType()].push_back(boxcomponent->weak_from_this());
							}
						}
					}
					}
				}
			}
		return true;
	}
	bool ActorFactoryComponent::CreateParticle(bool havetoSave /*= false*/)
	{
		if (!m_particle_list || m_particle_list->isNull())
			return false;
		m_editor_type_name = "particle";
		auto m_comp = getActor()->AddComponent<Echo::ParticleSystemComponent>("ComponentParticle", getActor()->getRootNodeCom(), nullptr, 0, !havetoSave);
		if (!m_comp.expired())
			m_components[m_comp->getType()].push_back(m_comp->weak_from_this());
		else
			return false;
		auto info = static_cast<Echo::ParticleSystemComponentInfo*>(m_comp->m_pActComInfo);
		info->m_path = m_particle_list->filename();
		//info->m_is_loop = true;

		m_comp->LoadParticle(info->m_path, false, getSceneManager());
		m_comp->LoopParticle(true);
		return true;
	}
	std::string ActorFactoryComponent::ConvertMeshPathToMeshPhypath(const std::string& path)
	{
		std::string mesh_path = path;
		std::replace(std::begin(mesh_path), std::end(mesh_path), '\\', '/');
		auto meshDotPos = mesh_path.rfind(".mesh");
		if (meshDotPos != std::string::npos)
		{
			mesh_path = mesh_path.substr(0, meshDotPos);
		}
		std::vector<std::string> path_elements;
		bool                     loop = true;
		while (loop)
		{
			path_elements.push_back(mesh_path.substr(0, mesh_path.find('/')));
			mesh_path = mesh_path.substr(mesh_path.find('/') + 1, mesh_path.size());
			if (mesh_path.find('/') == std::string::npos) 
				loop = false;
		}

		std::string retPath = "collision/mesh/";
		for (auto& p : path_elements)
		{
			retPath += p;
			retPath += "_";
		}
		retPath += mesh_path;
		retPath += "_phy.meshphy";
		return retPath;
	}

	void ActorFactoryComponent::UpdateTransFrom()
	{

	}

	std::vector<Echo::Vector3> ActorFactoryComponent::ConvertVVFToEchoVector3(const std::vector<std::vector<float>>& data)
	{
		std::vector<Echo::Vector3> ret;
		for (int i = 0; i < data.size(); ++i)
		{
			Echo::Vector3 v;
			if (data[i].size() >= 3)
			{
				for (int j = 0; j < 3; ++j)
				{
					v[j] = data[i][j];
				}
			}
			else
			{
				for (int j = 0; j < data[i].size(); ++j)
				{
					v[j] = data[i][j];
				}
			}
			ret.emplace_back(v);
		}
		return ret;
	}

	void ActorFactoryComponent::SetMinMax(ActorPtr act, const Vector3& min, const Vector3& max, float scale)
	{
		AxisAlignedBox box;
		float          half = (max - min).length();
		auto           hfLength = half * scale;
		box.setMinimum(min - Echo::Vector3(hfLength));
		box.setMaximum(max + Echo::Vector3(hfLength));
		static_cast<Echo::NodeComponent*>(act->getRootNodeCom().get())->setLoadBoundingBox(box);
	}

	void ActorFactoryComponent::SetMinMax(ActorPtr act, const std::vector<std::vector<float>>& data, float scale, bool save)
	{
		auto m_min_max = ConvertVVFToEchoVector3(data);
		if (m_min_max.size() >= 2)
		{
			AxisAlignedBox box;
			float half = (m_min_max[1] - m_min_max[0]).length();
			auto hfLength = half * scale;
			box.setMinimum(m_min_max[0] - Echo::Vector3(hfLength));
			box.setMaximum(m_min_max[1] + Echo::Vector3(hfLength));
			static_cast<Echo::NodeComponent*>(act->getRootNodeCom().get())->setLoadBoundingBox(box);
			if (save)
			{
				mMinMaxBox = box;
			}
		}
	}

	void ActorFactoryComponent::UpdateActorMinMax(const Echo::ActorPtr& ptr)
	{
		auto buildingInfo = GetExcelLineData(building_info, m_ExcelID);
		auto buildlist = GetExcelLineData(building_list, m_ExcelID);
		if (buildingInfo && buildlist)
		{
			auto imScale = buildlist->Imposter_scale();
			if (imScale == 0) imScale = 10;
			auto hlScale = buildlist->hlod_scale();
			if (hlScale == 0) hlScale = 5;
			auto buildingScale = buildlist->building_scale();
			if (buildingScale == 0) buildingScale = 2;
			SetMinMax(ptr, buildingInfo->boxSize(), imScale);
		}
	}

	void ActorFactoryComponent::InitComMb()
	{

	}

	std::string ActorFactoryComponent::GenCollsionPath(std::string& mesh_path, std::string& collision_path, bool& isConvex)
	{
		if (!collision_path.empty())
			return collision_path;

		//std::string assetPath = "";
		//auto mesh_to_meshPhy_str = ConvertMeshPathToMeshPhypath(mesh_path);
		//if (Root::instance()->TestResExist((collision_path).c_str()))
		//{
		//	isConvex = true;
		//	return collision_path;
		//}

		//if (Root::instance()->TestResExist((mesh_to_meshPhy_str).c_str()))
		//{
		//	isConvex = true;
		//	return mesh_to_meshPhy_str;
		//}

		//if (Root::instance()->TestResExist((mesh_path).c_str()))
		//{
		//	isConvex = false;
		//	return mesh_path;
		//}
		return mesh_path;
	}

	ActorFactoryComponentInfo::ActorFactoryComponentInfo(const Name& type, ActorCOMMoveableType movable)
		:ActorComponentInfo(type, movable)
	{

	}

	void ActorFactoryComponentInfo::ExportData(cJSON* jsonNode) 
	{
		//reflect::TypeResolver<ActorFactoryComponentInfo>::get()->ExportJson(this, jsonNode, "autosave");
		cJSON* autoSave = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "autosave", autoSave);
		cJSON* item1 = cJSON_CreateObject();
		cJSON_AddItemToArray(autoSave, item1);
		cJSON_AddItemToObject(item1, "actor_block_id", cJSON_CreateNumber(actor_block_id));
		cJSON* item2 = cJSON_CreateObject();
		cJSON_AddItemToArray(autoSave, item2);
		cJSON_AddItemToObject(item2, "actor_particle_id", cJSON_CreateNumber(actor_particle_id));
		cJSON* item3 = cJSON_CreateObject();
		cJSON_AddItemToArray(autoSave, item3);
		cJSON_AddItemToObject(item3, "actor_create_type", cJSON_CreateNumber(actor_create_type));
		cJSON* item4 = cJSON_CreateObject();
		cJSON_AddItemToArray(autoSave, item4);
		cJSON_AddItemToObject(item4, "isFpp", cJSON_CreateBool(isFpp));

		cJSON* item5 = cJSON_CreateObject();

		cJSON* CreateBeginInfo = cJSON_CreateArray();
		cJSON_AddItemToArray(item5, CreateBeginInfo);
		cJSON_AddItemToObject(autoSave, "m_CreateBeginInfo", CreateBeginInfo);

		cJSON* item6 = cJSON_CreateObject();
		cJSON_AddItemToObject(item6, "m_bBeginner", cJSON_CreateNumber(m_CreateBeginInfo.m_bBeginner));
		
		cJSON* item7 = cJSON_CreateObject();
		cJSON_AddItemToObject(item7, "m_bPassPersonStates", cJSON_CreateNumber(m_CreateBeginInfo.m_bPassPersonStates));

		cJSON* item8 = cJSON_CreateObject();
		cJSON_AddItemToObject(item8, "m_bIsFpp", cJSON_CreateBool(m_CreateBeginInfo.m_bIsFpp));
		cJSON* item9 = cJSON_CreateObject();
		cJSON_AddItemToObject(item9, "m_ActorSave", cJSON_CreateBool(m_CreateBeginInfo.m_ActorSave));
		cJSON* item10 = cJSON_CreateObject();
		cJSON_AddItemToObject(item10, "m_bHasCCT", cJSON_CreateBool(m_CreateBeginInfo.m_bHasCCT));

		cJSON* item11 = cJSON_CreateObject();

		cJSON* wrapRepalceMaterial = cJSON_CreateArray();
		cJSON_AddItemToArray(item11, wrapRepalceMaterial);
		cJSON_AddItemToObject(autoSave, "m_wrapRepalceMaterial", wrapRepalceMaterial);

	}
	void ActorFactoryComponentInfo::ImportData(cJSON* jsonNode) 
	{
		//reflect::TypeResolver<ActorFactoryComponentInfo>::get()->ImportJson(this, jsonNode, "autosave");
		auto autoSave = cJSON_GetObjectItem(jsonNode, "autosave");
		bool isread = false;
		isread = ReadJSNode(cJSON_GetArrayItem(autoSave, 0), "actor_block_id", actor_block_id);
		if (!isread)
		{
			ReadJSNode(cJSON_GetArrayItem(autoSave, 1), "actor_block_id", actor_block_id);
			ReadJSNode(cJSON_GetArrayItem(autoSave, 2), "actor_particle_id", actor_particle_id);

		}
		if(isread)
			ReadJSNode(cJSON_GetArrayItem(autoSave, 1), "actor_particle_id", actor_particle_id);
		ReadJSNode(cJSON_GetArrayItem(autoSave, 2), "actor_create_type", actor_create_type);
		ReadJSNode(cJSON_GetArrayItem(autoSave, 3), "isFpp", isFpp);



		auto createInfo = cJSON_GetArrayItem(autoSave, 4);
		if (createInfo == nullptr)
			return;

		ReadJSNode(cJSON_GetArrayItem(createInfo, 0), "m_bBeginner", m_CreateBeginInfo.m_bBeginner);
		ReadJSNode(cJSON_GetArrayItem(createInfo, 1), "m_bBeginner", m_CreateBeginInfo.m_bPassPersonStates);
		ReadJSNode(cJSON_GetArrayItem(createInfo, 2), "m_bBeginner", m_CreateBeginInfo.m_bIsFpp);
		ReadJSNode(cJSON_GetArrayItem(createInfo, 3), "m_bBeginner", m_CreateBeginInfo.m_ActorSave);
		ReadJSNode(cJSON_GetArrayItem(createInfo, 4), "m_bBeginner", m_CreateBeginInfo.m_bHasCCT);


		//auto mater = cJSON_GetArrayItem(autoSave, 5);


	}
	void ActorFactoryComponentInfo::CopyTo(ActorComponentInfo* p_actor_com_info)
	{
		ActorFactoryComponentInfo* pinfo = static_cast<ActorFactoryComponentInfo*>(p_actor_com_info);
		//reflect::TypeResolver<ActorFactoryComponentInfo>::get()->copyTo(this, pinfo);
		pinfo->actor_block_id = actor_block_id;
		pinfo->actor_particle_id = actor_particle_id;
		pinfo->actor_create_type = actor_create_type;
		pinfo->m_CreateBeginInfo = m_CreateBeginInfo;
		pinfo->m_wrapRepalceMaterial = m_wrapRepalceMaterial;
	}
	void ActorFactoryComponentInfo::OverWrite()
	{

	}
}
