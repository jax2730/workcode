#include "EchoComponentStableHeaders.h"
#include "SUToEchoCom/SUWrapEntityComponent.h"
#include "EchoSqliteObject.h"
#include "Actor/EchoActor.h"
#include "SUToEchoCom/EchoExcelDataHelper.h"
#include "EchoMeshColliderComponent.h"
#include "EchoBoxColliderComponent.h"
#include "EchoActorEntityComponent.h"
#include "SUToEchoCom/SUActorFactoryComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(WrapEntityComponent)

	WrapEntityComponent::WrapEntityComponent(ActorPtr pActor, ActorCOMMoveableType moveType)
		:NodeActorComponent(pActor, moveType)
	{

	}

	WrapEntityComponent::~WrapEntityComponent()
	{
		
	}

	void WrapEntityComponent::init(Echo::ActorComponentInfo* info)
	{
	}

	void WrapEntityComponent::programInit(Echo::ActorComponentInfo& info)
	{
	}

	void WrapEntityComponent::bindNode()
	{
	}

	void WrapEntityComponent::connectCom()
	{
		if (m_pActComInfo == nullptr)
			return;
		auto m_info = static_cast<WrapEntityComponentInfo*>(m_pActComInfo);
		TryUpdate(m_info->asset_pic_id, m_info->m_is_fpp, m_info->registCallBack);
	}

	void WrapEntityComponent::disconnectCom()
	{

		//for (auto& p : m_EntityData)
		{
			//this->getActor()->delComponent(p.second.meshphy_relation);
			//this->getActor()->delComponent(p.second.mesh_relation);
		}
		for (auto& p : m_EntityData)
		{
			if (!p.second.meshphy_relation.expired())
			{
				this->getActor()->delComponent(p.second.meshphy_relation);
			}
			if (!p.second.mesh_relation.expired())
			{
				this->getActor()->delComponent(p.second.mesh_relation);
			}
		}
		m_EntityData.clear();//卸载掉
	}

	void WrapEntityComponent::DestoryGroup()
	{
		for (auto& p : m_EntityData)
		{
			if (!p.second.mesh_relation.expired())
			{
				getActor()->delComponent(p.second.mesh_relation);
			}

			if (!p.second.meshphy_relation.expired())
			{
				getActor()->delComponent(p.second.meshphy_relation);
			}
		}
		m_EntityData.clear();
	}

	void WrapEntityComponent::SetMaterial(std::vector<WrapEntityMeshReplaceStruct>& material)
	{
		for (auto& repalceMat : material)
		{
			if (m_EntityData.find(repalceMat.wrapEntityID) != m_EntityData.end())
			{
				std::map<uint32, st_MaterialInfo> vtMatNames;
				vtMatNames.clear();
				for (auto& p : repalceMat.m_partMateralData)
				{
					Echo::st_MaterialInfo matInfo;
					std::string           name = p.m_materialPath;

					matInfo.name = Echo::Name(name.c_str());
					matInfo.index = (uint32)p.PartId;
					vtMatNames.insert(std::make_pair((uint32)p.PartId, matInfo));
				}
				auto entity = static_cast<Echo::EntityComponent*>(m_EntityData[repalceMat.wrapEntityID].mesh_relation.get());
				if (entity)
				{
					entity->setMatNames(vtMatNames);
				}
			}
		}
	}

	void WrapEntityComponent::TryUpdate(int assetpic, bool is_fpp, bool regist)
	{
		if (m_pActComInfo != nullptr)
		{
			static_cast<WrapEntityComponentInfo*>(m_pActComInfo)->asset_pic_id = assetpic;
			static_cast<WrapEntityComponentInfo*>(m_pActComInfo)->m_is_fpp = is_fpp;
			static_cast<WrapEntityComponentInfo*>(m_pActComInfo)->registCallBack = regist;
			m_isFppWrap = is_fpp;
			DoRegistCallBack = regist;
		}

		//auto asset_ = static_cast<asset_pic_block*>(ExcelDataManager::instance()->getExcelByName("asset_pic_block"));
		line_asset = GetExcelLineData(asset_pic_block, assetpic);

		if (!line_asset || line_asset->isNull())
			return;

		std::vector<int> wrap_ent = line_asset->wrap_entity_id();
		//m_EntityData.resize(wrap_ent.size());
		for (int i = 0; i < wrap_ent.size(); ++i)
		{
			int _wrap_entity_id = wrap_ent[i];

			WrapEntityDataGroup& rEntity = m_EntityData[_wrap_entity_id];

			rEntity.GroupID = _wrap_entity_id;
			auto state = line_asset->mesh_group_state_id();
			if ( state.size() > i)
			{
				rEntity.mesh_group_id = state[i];
			}
			else
			{
				rEntity.mesh_group_id = -1;
			}
			rEntity.mesh_node = getActor()->AddComponent<Echo::NodeComponent>("EntityNode", this->weak_from_this(), nullptr, 0);
			rEntity.mesh_relation = CreateEntity(i, _wrap_entity_id);
			rEntity.meshphy_relation = CreatePhysx(i, _wrap_entity_id);
			getActor()->addComponent(rEntity.mesh_node, rEntity.mesh_relation);
			getActor()->addComponent(rEntity.mesh_node, rEntity.meshphy_relation);

			auto mWrapEnittyList = GetExcelLineData(wrap_entity, _wrap_entity_id);
			if (mWrapEnittyList)
			{
				auto excel_reverse = mWrapEnittyList->axis_reverse();
				if (excel_reverse.size() == 3)
				{
					Echo::Vector3 scale{ excel_reverse[0], excel_reverse[1], excel_reverse[2] };
					this->setScale(scale);
				}
				SetSelfColliderType(rEntity.GroupID, PhysXGroup::GetMask(Name("ActorGroup")));
				SetInteractionColliderTypeByFilterGroupID(rEntity.GroupID, PhysXGroup::GetMask(Name("all")));
			}
			//走默认数据调整
			auto extra_tran = line_asset->extra_transfrom();
			if (extra_tran.size() > i)
			{
				auto view = line_asset->extra_transfrom();
				auto trans = ConvertDataToTransfrom(view);
				if (rEntity.mesh_node)
				{
					rEntity.mesh_node.get<NodeComponent>()->setPosition(trans.vPos);
					rEntity.mesh_node.get<NodeComponent>()->setEulerAngles(trans.vEular);
					//rEntity.mesh_relation->setPosition(trans.vPos);
					//rEntity.mesh_relation->setEulerAngles(trans.vEular);
					//if (!rEntity.meshphy_relation.expired())
					//{
					//    rEntity.meshphy_relation->setPosition(trans.vPos);
					//    rEntity.meshphy_relation->setEulerAngles(trans.vEular);
					//}
				}

			}
		}

		//ForceEntityRenderDis();
	}

	void WrapEntityComponent::Create()
	{

	}

	Echo::ActorComponentPtr WrapEntityComponent::CreatePhysx(const std::string& cmptName, const Echo::Vector3& collider, const PhyxData& phyData)
	{
		if (!collider.AlmostZero())
		{
			auto		boxname = cmptName.empty() ? "BoxColliderComponent" : cmptName;
			auto        boxcomponent = getActor()->AddComponent<BoxColliderComponent>(boxname, this->weak_from_this(), nullptr, 0, true);
			//auto        boxinfo = static_cast<Echo::BoxColliderComponentInfo*>(boxcomponent->m_pActComInfo);
			//boxinfo->SetVisible(true);
			//boxinfo->SetHalfSize(Echo::Vector3(collider[0] / 2.f, collider[2] / 2.f, collider[1] / 2.f));
			//boxinfo->SetPosition(Echo::Vector3(0, collider[2] / 2.f, 0)); //向下偏移
			boxcomponent->SetHalfSize(Vector3(collider[0] / 2.f, collider[2] / 2.f, collider[1] / 2.f));
			boxcomponent->setPosition(Vector3(0, collider[2] / 2.f, 0));
			boxcomponent->setVisible(true);
			boxcomponent->SetSurfaceType(phyData.surfaceType);

			return boxcomponent->weak_from_this();
		}
		return nullptr;
		
	}

	Echo::ActorComponentPtr WrapEntityComponent::CreatePhysx(const std::string& cmptName, std::string pathPhys, std::string pathMesh, bool convex, const PhyxData& phyData)
	{
		std::replace(std::begin(pathMesh), std::end(pathMesh), '\\', '/');
		std::replace(std::begin(pathPhys), std::end(pathPhys), '\\', '/');
		bool isConvex = true;
		pathPhys = GenCollsionPath(pathMesh, pathPhys, isConvex, 1);
		isConvex = convex;

		// 碰撞模型
		if (!pathPhys.empty() || !pathMesh.empty())
		{
			auto        name = cmptName.empty() ? "MeshColliderComponent" : cmptName;
			auto        mesh = getActor()->AddComponent<MeshColliderComponent>(name, this->weak_from_this(), nullptr, 0);
			if (mesh->m_pActComInfo)
			{
				auto meshInfo = static_cast<Echo::MeshColliderComponentInfo*>(mesh->m_pActComInfo);
				meshInfo->SetIsConvex(false);
				meshInfo->SetIsTrigger(true);
				meshInfo->SetVisible(true);
				mesh->SetIsConvex(meshInfo->GetIsConvex());
				mesh->setVisible(meshInfo->GetVisible());
			}
			else
			{
				mesh->SetIsConvex(false);
				mesh->setVisible(true);
			}

			mesh->SetMeshName_meshphy(Echo::Name(pathPhys.c_str()));
			mesh->SetSurfaceType(phyData.surfaceType);
			return mesh->weak_from_this();
		}
		return nullptr;
	}

	Echo::ActorComponentPtr WrapEntityComponent::CreateEntity(const std::string & cmptName, std::string pathMesh)
	{
		if (!pathMesh.empty())
		{
			auto        comp = this->weak_from_this();
			auto        name = cmptName.empty() ? "EntityComponent" : cmptName;
			auto        entity = getActor()->AddComponent<Echo::EntityComponent>(name, comp, nullptr, 0);


			std::string strPath = pathMesh;
			std::replace(std::begin(strPath), std::end(strPath), '\\', '/'); //统一统一资产导出格式
			if (entity->m_pActComInfo != nullptr)
			{
				auto        info = static_cast<Echo::EntityComponentInfo*>(entity->m_pActComInfo);
				info->SetMeshName(strPath.c_str());
			}

			bool sync = true;
			if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
				sync = false;

			entity->loadEntity(Echo::Name(strPath), sync);

			return entity->weak_from_this();
		}
		return nullptr;
	}

//	WrapEntityComponent::WrapTransFrom WrapEntityComponent::ConvertMeshPathToMeshPhypath(std::basic_string_view<float>& data)
//	{
//		WrapTransFrom trans;
//		for (int i = 0; i < data.size(); ++i)
//		{
//			if (i < 3)
//			{
//				trans.vPos[i] = data[i];
//			}
//			else if (i >= 3 && i < 6)
//			{
//				trans.vEular[i - 3] = data[i];
//			}
//		}
//		return trans;
//	}

	std::string WrapEntityComponent::ConvertMeshPathToMeshPhypath(const std::string& path)
	{
		std::string mesh_path = path;
		std::replace(std::begin(mesh_path), std::end(mesh_path), '\\', '/'); //统一统一资产导出格式
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
			if (mesh_path.find('/') == std::string::npos) loop = false;
		}

		std::string retPath = "collision/mesh/";
		for (auto& p : path_elements)
		{
			retPath += p;
			retPath += "_";
		}
		retPath += mesh_path;
		retPath += ".meshphy";
		return retPath;
	}

	WrapEntityComponent::WrapTransFrom WrapEntityComponent::ConvertDataToTransfrom(std::vector<float>& data)
	{
		WrapTransFrom trans;
		for (int i = 0; i < data.size(); ++i)
		{
			if (i < 3)
			{
				trans.vPos[i] = data[i];
			}
			else if (i >= 3 && i < 6)
			{
				trans.vEular[i - 3] = data[i];
			}
		}
		return trans;
	}

	std::vector<int> WrapEntityComponent::GetGroupIDs()
	{
		std::vector<int> group_ids;
		for (auto& [p, q] : m_EntityData)
		{
			group_ids.push_back(p);
		}
		return group_ids;
	}
	int WrapEntityComponent::GetAssetPicID()
	{
		if (m_pActComInfo == nullptr)
			return -1;

		return static_cast<WrapEntityComponentInfo*>(m_pActComInfo)->asset_pic_id;
	}

	Echo::ActorComponentPtr WrapEntityComponent::CreatePhysx(int id, int entityid)
	{
		Echo::ActorComponentPtr pPhysxCmpt = nullptr;

		//auto wrapentity_ = static_cast<wrap_entity*>(ExcelDataManager::instance()->getExcelByName("wrap_entity"));
		auto wrapentity_list = GetExcelLineData(wrap_entity, entityid);
		if (nullptr != wrapentity_list && !wrapentity_list->isNull())
		{
			const int		collider_type = wrapentity_list->collider_type();
			std::string		collision = wrapentity_list->meshphy_path();
			std::string		mesh_path = wrapentity_list->mesh_path();
			Echo::Vector3	collider_box = Echo::Vector3::ZERO;
			auto col = wrapentity_list->collider_box();
			if (col.size() >= 3)
			{
				collider_box.x =col[0];
				collider_box.y =col[1];
				collider_box.z =col[2];
			}
			switch (collider_type)
			{
			case 0:
			{
				// return;
			}
			break;
			case 1:
			{
				std::string szName = "MeshColliderComponent" + std::to_string(id);
				pPhysxCmpt = CreatePhysx(szName, collision, mesh_path, wrapentity_list->convex_type(), {wrapentity_list->surface_type()});
			}
			break;
			case 2:
			{
				std::string szName = "BoxColliderComponent" + std::to_string(id);
				pPhysxCmpt = CreatePhysx(szName, collider_box, { wrapentity_list->surface_type()});
			}
			break;
			default:
			break;
			}
		}
		return pPhysxCmpt;
	}

	Echo::ActorComponentPtr WrapEntityComponent::CreateEntity(int id, int entityid)
	{
		//auto wrapentity_ = static_cast<wrap_entity*>(ExcelDataManager::instance()->getExcelByName("wrap_entity"));
		auto wrapentity_list = GetExcelLineData(wrap_entity, entityid);
		if (nullptr != wrapentity_list && !wrapentity_list->isNull())
		{
			std::string					name = "EntityComponent" + std::to_string(id);
			std::string					strMesh = wrapentity_list->mesh_path();

			Echo::ActorComponentPtr		newCmpt = CreateEntity(name, strMesh);
			if (nullptr != newCmpt)
			{
				auto info = static_cast<Echo::EntityComponentInfo*>(newCmpt->m_pActComInfo);
				auto entity = newCmpt.getStaticCastWeakPtr<Echo::EntityComponent>();

				if (info)
				{
					info->SetCullMode(wrapentity_list->sight_dis_type());
					info->SetFpsState(wrapentity_list->fps_state());
					info->m_CastShadow = wrapentity_list->cast_shadow();
				}

				entity->setCullMode(wrapentity_list->sight_dis_type());
				entity->setEnityFpsState(wrapentity_list->fps_state());
				entity->setCastShadow(wrapentity_list->cast_shadow());
				if (wrapentity_list->sight_dis_param() != 0.f)
				{
					if(info)
						info->SetCullDistance(wrapentity_list->sight_dis_param());
					entity->setRenderDistance(wrapentity_list->sight_dis_param());
				}
				auto lod = wrapentity_list->lod_cut();
				if ( lod.size() > 0)
				{
					if (info)
						info->m_LodDis_Level1 = lod[0];
					entity->setLodDisLevel1(lod[0]);
				}
				if (lod.size() > 1)
				{
					if (info)
						info->m_LodDis_Level2 = lod[1];
					entity->setLodDisLevel2(lod[1]);
				}
				//
				return newCmpt;
			}
		}
		return nullptr;
	}
	void WrapEntityComponent::SetInteractionColliderTypeByFilterGroupID(int GroupID, int id)
	{
		if (m_EntityData.find(GroupID) == m_EntityData.end()) return;
		if (m_EntityData[GroupID].meshphy_relation.expired()) return;
		auto line = GetExcelLineData(physics_filter_groups, id);
		if (!line) return;
		auto mask = Echo::PhysXGroup::GetMask(Echo::Name(line->name().c_str()));

		if (m_EntityData[GroupID].meshphy_relation->getType() == Echo::BoxColliderComponent::mType)
		{
			static_cast<Echo::BoxColliderComponent*>(m_EntityData[GroupID].meshphy_relation.get())->SetCanInteractFilterGroupType(mask);
		}
		else if (m_EntityData[GroupID].meshphy_relation->getType() == Echo::MeshColliderComponent::mType)
		{
			static_cast<Echo::MeshColliderComponent*>(m_EntityData[GroupID].meshphy_relation.get())->SetCanInteractFilterGroupType(mask);
		}
	}
	void WrapEntityComponent::SetSelfColliderType(int GroupID, int filter_group)
	{
		if (m_EntityData.find(GroupID) == m_EntityData.end()) return;
		if (m_EntityData[GroupID].meshphy_relation.expired()) return;
		int Group = 0;
		if (filter_group > 0)
		{
			Group = 1 << filter_group;
		}
		else
		{
			return;
		}

		if (m_EntityData[GroupID].meshphy_relation->getType() == Echo::BoxColliderComponent::mType)
		{
			static_cast<Echo::BoxColliderComponent*>(m_EntityData[GroupID].meshphy_relation.get())->SetFilterGroupType(Group);
		}
		else if (m_EntityData[GroupID].meshphy_relation->getType() == Echo::MeshColliderComponent::mType)
		{
			static_cast<Echo::BoxColliderComponent*>(m_EntityData[GroupID].meshphy_relation.get())->SetFilterGroupType(Group);
		}
	}

	void WrapEntityComponent::SetInteractionColliderType(uint32 type)
	{
		for (auto& p : m_EntityData)
		{
			if (p.second.meshphy_relation.expired()) continue;
			if (p.second.meshphy_relation->getType() == MeshColliderComponent::mType)
			{
				auto meshCollider = static_cast<MeshColliderComponent*>(p.second.meshphy_relation.get());
				if (p.second.meshphy_relation_filter == -1)
					p.second.meshphy_relation_filter = meshCollider->GetCanInteractFilterGroupType();
				meshCollider->SetCanInteractFilterGroupType(type);
			}
			else if (p.second.meshphy_relation->getType() == BoxColliderComponent::mType)
			{
				auto boxCollider = static_cast<BoxColliderComponent*>(p.second.meshphy_relation.get());
				if (p.second.meshphy_relation_filter == -1)
					p.second.meshphy_relation_filter = boxCollider->GetCanInteractFilterGroupType();
				boxCollider->SetCanInteractFilterGroupType(type);
			}
		}
	}
	void WrapEntityComponent::ResetInteractionColliderType()
	{
		for (auto& p : m_EntityData)
		{
			if (p.second.meshphy_relation_filter == -1) continue;
			if (p.second.meshphy_relation.expired()) continue;
			if (p.second.meshphy_relation->getType() == MeshColliderComponent::mType)
			{
				auto meshCollider = static_cast<MeshColliderComponent*>(p.second.meshphy_relation.get());
				meshCollider->SetCanInteractFilterGroupType(p.second.meshphy_relation_filter);
			}
			else if (p.second.meshphy_relation->getType() == BoxColliderComponent::mType)
			{
				auto boxCollider = static_cast<BoxColliderComponent*>(p.second.meshphy_relation.get());
				boxCollider->SetCanInteractFilterGroupType(p.second.meshphy_relation_filter);
			}
			p.second.meshphy_relation_filter = -1;
		}
	}

	Echo::ActorComponentPtr WrapEntityComponent::CreateAnimator(int id)
	{
		if (0 >= id)
			return nullptr;
		//auto anima = static_cast<cha_fsm_list*>(ExcelDataManager::instance()->getExcelByName("cha_fsm_list"));
		auto mbLine = GetExcelLineData(cha_fsm_list, id);
		if (nullptr == mbLine)
			return nullptr;

		Echo::Name  szName = Echo::Name("EntityComponent_Animator");

		auto        pRoot = this->weak_from_this();
		//auto        pCmpt = getActor()->AddComponent<su::AnimatorComponent>(szName, pRoot, nullptr, 0);
		//auto        pInfo = static_cast<su::AnimatorComponentInfo*>(pCmpt->m_pActComInfo);

		//if (pInfo->ApplyFsmList(id))
		//{
		//	pCmpt->IgnoreAnimation(true);
		//	pCmpt->OnSupply(pInfo);
		//}

		//return pCmpt;
		return nullptr;
	}

	void WrapEntityComponent::FppWrapEntityEnterFpp()
	{
		std::string name = this->getActor()->getActorIDStr().c_str();
		if (m_isFppWrap)
		{
			//Fpp 在座位上
			if (IsClientPlayerOnSeat())
			{
				for (auto& [p, q] : m_EntityData)
				{
					//auto mesh_group = static_cast<mesh_group_state_list*>(ExcelDataManager::instance()->getExcelByName("mesh_group_state_list"));
					auto line = GetExcelLineData(mesh_group_state_list, q.mesh_group_id);
					if (!line || line->isNull()) continue;
					if (m_EntityData[p].mesh_relation.expired()) continue;
					auto entity = static_cast<Echo::EntityComponent*>(q.mesh_relation.get());
					bool SeatShow = false;
					auto seat = line->fpp_seat_state_group_id();
					if (seat.size() > 0)
					{
						if (seat[0] >= 1)
						{
							SeatShow = true;
						}
					}
					if (SeatShow)
					{
						entity->setEnityFpsState(FP_STATE_ENTITY);
						entity->setVisible(true);
						entity->setCastShadow(false); //fpp 无阴影
					}
					else
					{
						entity->setEnityFpsState(FP_STATE_ENTITY);
						entity->setVisible(false);
						entity->setCastShadow(false);
					}
				}
			}
			else //Fpp 不在座位上
			{
				for (auto& [p, q] : m_EntityData)
				{
					//auto mesh_group = static_cast<mesh_group_state_list*>(ExcelDataManager::instance()->getExcelByName("mesh_group_state_list"));
					auto line = GetExcelLineData(mesh_group_state_list, q.mesh_group_id);
					if (!line || line->isNull()) continue;
					if (m_EntityData[p].mesh_relation.expired()) continue;
					auto entity = static_cast<Echo::EntityComponent*>(m_EntityData[p].mesh_relation.get());
					bool fppShow = false;
					auto fpp = line->fpp_state_group_id();
					if (fpp.size() > 0)
					{
						if (fpp[0] >= 1)
						{
							fppShow = true;
						}
					}
					if (fppShow)
					{
						entity->setEnityFpsState(FP_STATE_ENTITY);
						entity->setVisible(true);
						entity->setCastShadow(false);
					}
					else
					{
						entity->setEnityFpsState(FP_STATE_ENTITY);
						entity->setVisible(false);
						entity->setCastShadow(false);
					}
				}
			}
		}
	}

	void WrapEntityComponent::TppWrapEntityEnterFpp()
	{
		std::string name = this->getActor()->getActorIDStr().c_str();
		if (!m_isFppWrap)
		{
			for (auto& [p, q] : m_EntityData)
			{
				//auto mesh_group = static_cast<mesh_group_state_list*>(ExcelDataManager::instance()->getExcelByName("mesh_group_state_list"));
				auto line = GetExcelLineData(mesh_group_state_list, q.mesh_group_id);
				if (!line || line->isNull()) continue;
				if (IsClientPlayerOnSeat()) //在座位上
				{
					bool m_bVis = false;
					auto tpp = line->tpp_state_group_id();
					if (tpp.size() > 0)
					{
						if (tpp[0] >= 1)
						{
							m_bVis = true;
						}
					}
					auto fpp_tpp = line->fpp_tpp_seat_sign_id();
					if (fpp_tpp.size() > 0)
					{
						if (fpp_tpp[0] >= 1)
						{
							m_bVis = false;
						}
					}
					if (m_EntityData[p].mesh_relation.expired()) continue;
					auto entity = static_cast<Echo::EntityComponent*>(q.mesh_relation.get());
					if (m_bVis)
					{
						entity->setVisible(true);
						entity->setCastShadow(true);
						entity->setEnityFpsState(FP_STATE_NONE);
					}
					else
					{
						entity->setVisible(false);
						entity->setCastShadow(true);
						entity->setEnityFpsState(FP_STATE_NONE);
					}
				}
				else
				{
					bool m_bVis = false;
					auto  tpp = line->tpp_state_group_id();
					if (tpp.size() > 0)
					{
						if (tpp[0] >= 1)
						{
							m_bVis = true;
						}
					}
					auto fpp_tpp = line->fpp_tpp_sign_id();
					if (fpp_tpp.size() > 0)
					{
						if (fpp_tpp[0] >= 1)
						{
							m_bVis = false;
						}
					}
					if (m_EntityData[p].mesh_relation.expired()) continue;
					auto entity = static_cast<Echo::EntityComponent*>(q.mesh_relation.get());
					if (m_bVis)
					{
						entity->setVisible(true);
						entity->setCastShadow(true);
						entity->setEnityFpsState(FP_STATE_NONE);
					}
					else
					{
						entity->setVisible(false);
						entity->setCastShadow(true);
						entity->setEnityFpsState(FP_STATE_NONE);
					}
				}
			}
		}
	}

	void WrapEntityComponent::SetEntityPosition(int EntityID, const Echo::Vector3& vPos)
	{
		if (m_EntityData.find(EntityID) != m_EntityData.end())
		{
			m_EntityData[EntityID].mesh_node.get<NodeComponent>()->setPosition(vPos);
			//m_EntityData[EntityID].mesh_relation->setPosition(vPos);
			//m_EntityData[EntityID].meshphy_relation->setPosition(vPos);
		}
	}
	Echo::Vector3 WrapEntityComponent::GetEntityPosition(int EntityID)
	{
		if (m_EntityData.find(EntityID) != m_EntityData.end())
		{
			return m_EntityData[EntityID].mesh_relation->getPosition();
		}
		return retZero;
	}
	void WrapEntityComponent::SetEntityRotation(int EntityID, const Echo::Vector3& vEular)
	{
		if (m_EntityData.find(EntityID) != m_EntityData.end())
		{
			m_EntityData[EntityID].mesh_node.get<NodeComponent>()->setEulerAngles(vEular);
			//m_EntityData[EntityID].mesh_relation->setEulerAngles(vEular);
			//m_EntityData[EntityID].meshphy_relation->setEulerAngles(vEular);
		}
	}
	Echo::Vector3 WrapEntityComponent::GetEntityRotation(int EntityID)
	{
		if (m_EntityData.find(EntityID) != m_EntityData.end())
		{
			return m_EntityData[EntityID].mesh_relation->getEulerAngles();
		}
		return retZero;
	}


	bool WrapEntityComponent::HasEntity(int EntityID)
	{
		return m_EntityData.find(EntityID) != m_EntityData.end();
	}

	ActorComponentPtr WrapEntityComponent::GetColliderByID(int EntityID)
	{
		if (HasEntity(EntityID))
		{
			return m_EntityData.find(EntityID)->second.meshphy_relation.getDynamicCastWeakPtr<ColliderComponent>();
		}
		return nullptr;
	}

	void WrapEntityComponent::TppWrapEntityOutFpp()
	{
		if (!m_isFppWrap)
		{
			for (auto& [p, q] : m_EntityData)
			{
				//auto mesh_group = static_cast<mesh_group_state_list*>(ExcelDataManager::instance()->getExcelByName("mesh_group_state_list"));
				auto line = GetExcelLineData(mesh_group_state_list, q.mesh_group_id);
				if (!line || line->isNull()) continue;
				if (m_EntityData[p].mesh_relation.expired()) continue;
				auto entity = static_cast<Echo::EntityComponent*>(q.mesh_relation.get());
				bool tppVis = false;
				auto tpp = line->tpp_state_group_id();
				if (tpp.size() > 0)
				{
					if (tpp[0] >= 1)
					{
						tppVis = true;
					}
				}
				if (tppVis)
				{
					entity->setVisible(true);
					entity->setCastShadow(true);
					entity->setEnityFpsState(FP_STATE_NONE);

				}
				else
				{
					entity->setVisible(false);
					entity->setCastShadow(true);
					entity->setEnityFpsState(FP_STATE_NONE);
				}
			}
		}
	}

	void WrapEntityComponent::RecoverEntityRenderDis()
	{
		for (auto& p : m_EntityData)
		{
			//auto wrapentity_list = static_cast<wrap_entity*>(ExcelDataManager::instance()->getExcelByName("wrap_entity"));
			auto line = GetExcelLineData(wrap_entity, p.first);
			if (!line || line->isNull()) continue;
			if (p.second.mesh_relation.get())
			{
				static_cast<Echo::EntityComponent*>(p.second.mesh_relation.get())->setCullMode(line->sight_dis_type());
				static_cast<Echo::EntityComponent*>(p.second.mesh_relation.get())->setRenderDistance(line->sight_dis_param());
			}
		}
	}

	void WrapEntityComponent::ForceEntityRenderDis()
	{
		for (auto& [p, q] : m_EntityData)
		{
			if (q.mesh_relation.get())
			{
				static_cast<Echo::EntityComponent*>(q.mesh_relation.get())->setCullMode(1);
				static_cast<Echo::EntityComponent*>(q.mesh_relation.get())->setRenderDistance(500.0f);
			}
		}
	}

	bool WrapEntityComponent::IsClientPlayerOnSeat()
	{
		//auto pact = getActor()->getParentActor();
		//if (!pact) return false;
		//auto seat = GetComponent<SeatComponent>(pact);
		//if (seat) return true;
		return false;
	}

	void WrapEntityComponent::FppWrapEntityOutFpp()
	{
		if (m_isFppWrap)
		{
			for (auto& [p, q] : m_EntityData)
			{
				//auto mesh_group = static_cast<mesh_group_state_list*>(ExcelDataManager::instance()->getExcelByName("mesh_group_state_list"));
				auto line = GetExcelLineData(mesh_group_state_list, q.mesh_group_id);
				if (!line || line->isNull()) continue;
				if (m_EntityData[p].mesh_relation.expired()) continue;
				auto entity = static_cast<Echo::EntityComponent*>(q.mesh_relation.get());
				entity->setEnityFpsState(FP_STATE_NONE);
				entity->setVisible(false);
				entity->setCastShadow(false);
			}
		}
	}

	std::string	WrapEntityComponent::GenCollsionPath(std::string& mesh_path, std::string& collision_path, bool& isConvex, int collider_type)
	{
		if (collider_type != 1 || collision_path != "")
		{
			return collision_path;
		}
		std::string assetPath = "";
		auto        mesh_to_meshPhy_str = ConvertMeshPathToMeshPhypath(mesh_path);
		return mesh_to_meshPhy_str;
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
		//return mesh_path;
	}

	WrapEntityComponentInfo::WrapEntityComponentInfo(const Name& type, ActorCOMMoveableType movable)
		:ActorComponentInfo(type, movable)
	{

	}

	void WrapEntityComponentInfo::ExportData(cJSON* jsonNode)
	{
		cJSON* autoSave = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "autosave", autoSave);
		cJSON* item1 = cJSON_CreateObject();
		cJSON_AddItemToArray(autoSave, item1);
		cJSON_AddItemToObject(item1, "asset_pic_id", cJSON_CreateNumber(asset_pic_id));
	}
	void WrapEntityComponentInfo::ImportData(cJSON* jsonNode)
	{
		auto autoSave = cJSON_GetObjectItem(jsonNode, "autosave");
		ReadJSNode(cJSON_GetArrayItem(autoSave, 0), "asset_pic_id", asset_pic_id);
	}
	void WrapEntityComponentInfo::CopyTo(ActorComponentInfo* p_actor_com_info)
	{
		WrapEntityComponentInfo* pinfo = static_cast<WrapEntityComponentInfo*>(p_actor_com_info);
		//reflect::TypeResolver<ActorFactoryComponentInfo>::get()->copyTo(this, pinfo);
		pinfo->asset_pic_id = asset_pic_id;
	}
	void WrapEntityComponentInfo::OverWrite()
	{
	}
}
