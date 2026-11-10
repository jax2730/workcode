#include "EchoComponentStableHeaders.h"
#include "EchoPCGGenerateObjectManager.h"

#include "EchoPCGGraphComponent.h"
#include "EchoPCGObject.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainPCGGenScheduler.h"
#include "EchoActorEntityComponent.h"
#include "EchoEntityObject.h"
#include "EchoSceneManager.h"
#include "EchoPCGBaseElement.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActorSystem.h"
#include "Actor/EchoActorInfoManager.h"
#include "EchoMeshMgr.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoEngine.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXSystem.h"
#include "EchoPhysXResource.h"
#include "EchoManualObject.h"
#include "EchoPCGReflect.h"
#include "EchoPhysXManager.h"
#include "EchoMeshColliderComponent.h"
#include "EchoPhysXResource.h"

namespace Echo {

	namespace PCG {

		struct StaticMeshObject : public PCGGeneralObject, public ResourceBackgroundQueue::Listener {
			StaticMeshObject(bool _bComponentPhysX, PCGGeneralObjectCache* _pCache) : PCGGeneralObject(_pCache), bComponentPhysX(_bComponentPhysX) {}
			virtual ~StaticMeshObject() { Release(); }
			bool bPhysics = true;
			bool bCastShadow = true;
			const bool bComponentPhysX = false;
			uint32 surface_type = 0;
			Name meshphyName;
			MeshPtr meshPtr;
			PhysXTriangleMeshResourcePtr physxPtr;
			std::vector<EntityObject*> entitys;
			void* physicsPtr = nullptr;
			std::vector<MeshColliderComponentPtr> colliders;

			Name meshName;
			float lod1_distance = 0.0f;
			float lod2_distance = 0.0f;
			float render_distance = 0.0f;
			uint32 custom_type = 0;

			void Release();
			virtual bool PreCreateObjects() override;
			virtual void CreateObjects() override;
			virtual void CollectObjects(std::map<Name, uint32>& data, uint32 mode) const override;
			virtual void CollectDrawInfo(std::map<Name, PCG::PCGDrawInfo>& data) const override;
			virtual void ChangeVisibleObjects(const std::set<uint16>& ObjectIds, bool bChange) override;
			virtual bool SetObjectsVisible(const std::pair<uint16, std::set<uint16>>& objIdIter, bool v) override;
			virtual bool GetObjectData(uint16 id0, uint16 id1, DVector3& worldPos, Quaternion& worldRot, Vector3& worldScl, Name& dataName, uint32& customType) override;
			virtual void QueryObjects(const OrientedBox& obb, std::set<uint64>& objIds, bool checkVisible) override;
			virtual void QueryObjects(const Sphere& obb, std::set<uint64>& objIds, bool checkVisible) override;
			virtual void operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;

			BackgroundProcessTicket mTicketMesh = 0;
			BackgroundProcessTicket mTicketPhysX = 0;

			static Name GetPhysXMeshPath(const Name& file) {
				String phyMeshName = file.toString() + "phy";

				std::replace(phyMeshName.begin(), phyMeshName.end(), '\\', '/');
				std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');

				phyMeshName.insert(0, "collision/mesh/");
				return Name(phyMeshName);
			}
		};

		void StaticMeshObject::operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) {
			if (resourcePtr.isNull()) return;
			if (ticket == mTicketMesh) {
				mTicketMesh = 0;
			}
			if (!meshPtr->isPrepared() && !meshPtr->isLoaded())
			{
				return;
			}
			if (bPhysics && !physxPtr->isPrepared() && !physxPtr->isLoaded()) {
				return;
			}
			CreateObjects();
		}

		void StaticMeshObject::Release() {
			if (mTicketMesh != 0) {
				ResourceBackgroundQueue::instance()->abortRequest(mTicketMesh);
				mTicketMesh = 0;
			}
			if (mTicketPhysX != 0) {
				ResourceBackgroundQueue::instance()->abortRequest(mTicketPhysX);
				mTicketPhysX = 0;
			}
			for (EntityObject* pEntity : entitys) {
				SAFE_DELETE(pEntity);
			}
			meshPtr.setNull();
			physxPtr.setNull();
			entitys.clear();
			if (!bComponentPhysX) {
				if (physicsPtr) {
					m_pCache->m_pPhysXWorldManager->RemoveStatic(physicsPtr);
				}
			}
			else {
				if (!m_pCache->mRootActor.Expired()) {
					for (MeshColliderComponentPtr& collider : colliders) {
						m_pCache->mRootActor->delComponent(collider);
					}
				}
				colliders.clear();
			}
		}
		bool StaticMeshObject::PreCreateObjects() {
			meshPtr = MeshMgr::instance()->createOrRetrieve(meshName).first;
			if (meshPtr.isNull()) {
				return false;
			}
			bool bExistResource = true;
			if (!meshPtr->isPrepared() && !meshPtr->isLoaded())
			{
				mTicketMesh = ResourceBackgroundQueue::instance()->prepare(MeshMgr::instance()->getResourceType(), meshName,
					Echo::WorkQueue::MakePriority(0, Echo::WorkQueue::RESOURCE_TYPE_TERRAIN),
					false, NULL, NULL, this);
				bExistResource = false;
			}

			if (bPhysics) {
				Name physxName = meshphyName;
				if (physxName.empty()) {
					physxName = GetPhysXMeshPath(meshName);
				}
				physxPtr = PhysXTriangleMeshResourceManager::instance()->createOrRetrieve(physxName).first;
				if (!physxPtr.isNull()) {
					if (!physxPtr->isPrepared() && !physxPtr->isLoaded())
					{
						mTicketPhysX = ResourceBackgroundQueue::instance()->prepare(PhysXTriangleMeshResourceManager::instance()->getResourceType(), physxName,
							Echo::WorkQueue::MakePriority(0, Echo::WorkQueue::RESOURCE_TYPE_TERRAIN),
							false, NULL, NULL, this);
						bExistResource = false;
					}
				}
			}
			return bExistResource;
		}
		void StaticMeshObject::CreateObjects() {
			if (meshPtr.isNull()) return;
			bool bConfigDis = (render_distance != 0.0f && lod1_distance != 0.0f && lod2_distance != 0.0f);
			int cacheSize = (int)caches.size();
			float cullDis = 1.f, lod1Dis = 1.0f, lod2Dis = 1.0f;
			EntityObjectDesc desc;
			desc.pSceneMgr = m_pCache->m_pSceneMgr;
			desc.pWorldMgr = m_pCache->m_pActorMgr->getWorldManager();
			desc.bUseQuadTree = true;
			desc.name = m_pCache->mActorName;
			for (int cacheIndex = 0; cacheIndex != cacheSize; ++cacheIndex) {
				const auto& cache = caches[cacheIndex];
				EntityObject* pEntity = new EntityObject(desc);
				entitys.push_back(pEntity);
#ifdef ECHO_EDITOR
				pEntity->setModuleName("PCG");
#endif // ECHO_EDITOR
				pEntity->changeEntity(meshPtr->getName(), false);
				pEntity->enableCastShadow(bCastShadow);
				pEntity->setCameraCullMode(CameraCullMode::DistanceCull);
				pEntity->setCulled(true);

				if (bConfigDis) {
					AxisAlignedBox bound = meshPtr->getAABB();
					float oldHalfSizeLength = bound.getHalfSize().length();
					bound.scale(cache.scale);
					float newHalfSizeLength = bound.getHalfSize().length();
					float chaScale = newHalfSizeLength / oldHalfSizeLength;
					cullDis = render_distance * chaScale;
					lod1Dis = lod1_distance * chaScale;
					lod2Dis = lod2_distance * chaScale;
				}
				else {
					AxisAlignedBox bound = meshPtr->getAABB();
					bound.scale(cache.scale);
					Vector3 halfSize = bound.getHalfSize();
					float halfSizeLength = fabsf(halfSize[halfSize.MainAxis() - 1]);
					m_pCache->getCullingDistance(halfSizeLength, cullDis, lod1Dis, lod2Dis);
				}

				pEntity->setRenderingDistance(cullDis);
				pEntity->setLodDis(LOD_LEVEL_1, lod1Dis);
				pEntity->setLodDis(LOD_LEVEL_2, lod2Dis);

				m_pCache->m_pSceneNode->addChild(pEntity->getSceneNode());
				pEntity->setPosition(cache.position);
				pEntity->setOrientation(cache.rotation);
				pEntity->setScale(cache.scale);
				pEntity->setVisible(true);
			}

			if (m_pCache->m_pPhysXWorldManager && !physxPtr.isNull() && physxPtr->ExistPhysXMesh()) {

				if (!bComponentPhysX) {
					std::vector<PhysXWorldManager::ActorInfo> infos;
					infos.resize(cacheSize);
					for (int cacheIndex = 0; cacheIndex != cacheSize; ++cacheIndex) {
						const auto& cache = caches[cacheIndex];
						auto& info = infos[cacheIndex];
						std::get<0>(info) = m_pCache->mOriginSpace.ToWorldPosition(cache.position);
						std::get<1>(info) = m_pCache->mOriginSpace.ToWorldRotation(cache.rotation);
						std::get<2>(info) = m_pCache->mOriginSpace.ToWorldScale(cache.scale);
						std::get<3>(info) = surface_type;
					}
					if (!infos.empty()) {
						physicsPtr = m_pCache->m_pPhysXWorldManager->AddStatic_TriangleMesh(infos, physxPtr);
					}
				}
				else {
					if (m_pCache->mRootActor) {
						PCGID pcgId;
						pcgId.pid = m_pCache->mID;
						pcgId.cid_0 = id;
						colliders.resize(cacheSize);
						for (int cacheIndex = 0; cacheIndex != cacheSize; ++cacheIndex) {
							const auto& cache = caches[cacheIndex];
							auto& collider = colliders[cacheIndex];
							pcgId.cid_1 = cacheIndex;
							std::string str = "MeshCollider_" + std::to_string(pcgId.id);
							collider = m_pCache->mRootActor->AddComponent<MeshColliderComponent>(str, m_pCache->mRootActor->getRootNodeCom());
							collider->SetIsUsePhysXPage(false);
							collider->setTransform(cache.position, cache.rotation, cache.scale, true);
							collider->SetSurfaceType(surface_type);
							collider->SetMeshName(physxPtr->getName());
							collider->SetFilterGroupType(ePhysXGroup_Mesh);
							collider->SetCanInteractFilterGroupType(ePhysXGroup_All);
							collider->SetIsUseAsyncRes(false);
							collider->SetIsUsePhysics(true);
						}
					}
				}
			}
			caches.resize(0);
			auto delObjsIter = m_pCache->mDeletedObjects.find(id);
			if (delObjsIter != m_pCache->mDeletedObjects.end()) {
				ChangeVisibleObjects(delObjsIter->second, false);
			}
		}

		void StaticMeshObject::CollectObjects(std::map<Name, uint32>& data, uint32 mode) const {
			if ((mode & eGenObjectType_Entity) != 0 && !entitys.empty()) {
				data[meshPtr->getName()] += (uint32)entitys.size();
			}
			if (!bComponentPhysX) {
				if ((mode & eGenObjectType_Physics) != 0 && (physicsPtr != nullptr)) {
					data[physxPtr->getName()] += (uint32)entitys.size();
				}
			}
			else {
				if ((mode & eGenObjectType_Physics) != 0 && !colliders.empty()) {
					data[physxPtr->getName()] += (uint32)colliders.size();
				}
			}
			if ((mode & 256) != 0 && !entitys.empty()) {
				auto& childData = data[meshPtr->getName()];
				for (EntityObject* pEntity : entitys) {
					if (!pEntity->getCulled()) {
						childData += pEntity->getResourceTriangleCount();
					}
					pEntity->setCulled(true);
				}
			}
			if ((mode & 512) != 0 && !entitys.empty()) {
				auto& childData = data[meshPtr->getName()];
				for (EntityObject* pEntity : entitys) {
					childData += pEntity->getResourceTriangleCount();
				}
			}
		}
		void StaticMeshObject::CollectDrawInfo(std::map<Name, PCG::PCGDrawInfo>& data) const {
			if (entitys.empty()) return;
			auto& childData = data[meshPtr->getName()];
			childData.totalNumber += (uint32)entitys.size();
			for (EntityObject* pEntity : entitys) {
				if (!pEntity->getCulled()) {
					childData.visibleTriangle += pEntity->getResourceTriangleCount();
					childData.visibleNumber += 1;
				}
				pEntity->setCulled(true);
			}
		}

		void StaticMeshObject::ChangeVisibleObjects(const std::set<uint16>& ObjectIds, bool bChange) {
			if (entitys.empty()) return;
			uint32 size = (uint32)entitys.size();
			std::vector<uint8_t> viss;
			if (bChange) {
				viss.resize(size, 2);
				auto delObjsIter = m_pCache->mDeletedObjects.find(id);
				if (delObjsIter != m_pCache->mDeletedObjects.end()) {
					for (auto& index : ObjectIds) {
						if (index < size) {
							viss[index] = delObjsIter->second.contains(index) ? 0 : 1;
						}
					}
				}
				else {
					for (auto& index : ObjectIds) {
						if (index < size) {
							viss[index] = 1;
						}
					}
				}
			}
			else {
				viss.resize(size, 1);
				for (auto& index : ObjectIds) {
					if (index < size) {
						viss[index] = 0;
					}
				}
			}

			for (uint32 index = 0; index != size; ++index) {
				if (viss[index] == 0) {
					entitys[index]->setVisible(false);
				}
				else if (viss[index] == 1) {
					entitys[index]->setVisible(true);
				}
			}
			if (!bComponentPhysX) {
				if (physicsPtr && m_pCache->m_pPhysXWorldManager) {
					m_pCache->m_pPhysXWorldManager->SetStaticVisible(physicsPtr, viss);
				}
			}
			else {
				for (uint32 index = 0; index != size; ++index) {
					if (viss[index] == 0) {
						colliders[index]->SetIsUsePhysics(false);
					}
					else if (viss[index] == 1) {
						colliders[index]->SetIsUsePhysics(true);
					}
				}
			}
		}

		bool StaticMeshObject::SetObjectsVisible(const std::pair<uint16, std::set<uint16>>& objIdIter, bool v) {
			uint32 size = (uint32)entitys.size();
			if (!bComponentPhysX) {
				std::map<uint32, bool> vismap;
				for (uint16 id : objIdIter.second) {
					if (id < size) {
						entitys[id]->setVisible(v);
						vismap[id] = v;
					}
				}
				m_pCache->m_pPhysXWorldManager->SetStaticVisible(physicsPtr, vismap, size);
			}
			else {
				for (uint16 id : objIdIter.second) {
					if (id < size) {
						entitys[id]->setVisible(v);
						colliders[id]->SetIsUsePhysics(v);
					}
				}
			}
			return false;
		}
		bool StaticMeshObject::GetObjectData(uint16 id0, uint16 id1, DVector3& worldPos, Quaternion& worldRot, Vector3& worldScl, Name& dataName, uint32& customType) {
			uint32 size = (uint32)entitys.size();
			if (id1 >= size) return false;
			SceneNode* pNode = entitys[id1]->getSceneNode();
			worldPos = pNode->_getDerivedPosition();
			worldRot = pNode->_getDerivedOrientation();
			worldScl = pNode->_getDerivedScale();
			dataName = meshPtr->getName();
			customType = custom_type;
			return true;
		}

		void StaticMeshObject::QueryObjects(const OrientedBox& obb, std::set<uint64>& objIds, bool checkVisible) {
			if (entitys.empty()) return;
			PCGID pcgId;
			pcgId.pid = m_pCache->mID;
			Sphere sph;
			pcgId.cid_0 = id;
			Vector3 testCenter = meshPtr->getAABB().getCenter();
			float testRadius = meshPtr->getAABB().getHalfSize().length();
			uint32 size = (uint32)entitys.size();
			for (uint32 index = 0; index != size; ++index) {
				EntityObject* pNode = entitys[index];
				if (checkVisible && !pNode->getVisible()) continue;
				sph.mCenter = pNode->getOrientation() * (testCenter * pNode->getScale()) + pNode->getPosition();
				sph.mRadius = testRadius * pNode->getScale()[pNode->getScale().MainAxis() - 1];
				if (obb.Intersect(sph)) {
					pcgId.cid_1 = index;
					objIds.insert(pcgId.id);
				}
			}
		}

		void StaticMeshObject::QueryObjects(const Sphere& qSph, std::set<uint64>& objIds, bool checkVisible) {
			if (entitys.empty()) return;
			PCGID pcgId;
			pcgId.pid = m_pCache->mID;
			Sphere sph;
			pcgId.cid_0 = id;
			Vector3 testCenter = meshPtr->getAABB().getCenter();
			float testRadius = meshPtr->getAABB().getHalfSize().length();
			uint32 size = (uint32)entitys.size();
			for (uint32 index = 0; index != size; ++index) {
				EntityObject* pNode = entitys[index];
				if (checkVisible && !pNode->getVisible()) continue;
				sph.mCenter = pNode->getOrientation() * (testCenter * pNode->getScale()) + pNode->getPosition();
				sph.mRadius = testRadius * pNode->getScale()[pNode->getScale().MainAxis() - 1];
				if (qSph.intersects(sph)) {
					pcgId.cid_1 = index;
					objIds.insert(pcgId.id);
				}
			}
		}



	}

	namespace PCG {

		struct ActorObject : public PCGGeneralObject, public ActorInfoLoadListener {
			ActorObject(PCGGeneralObjectCache* _pCache) : PCGGeneralObject(_pCache) {}
			virtual ~ActorObject() { Release(); }

			Name actorName;
			uint32 custom_type = 0;

			ActorInfoPtr actorInfo;
			std::vector<ActorPtr> actors;
			std::vector<ActorLoadListenerPCG*> actorListeners;

			void Release();
			virtual bool PreCreateObjects() override;
			virtual void CreateObjects() override;
			virtual void CollectObjects(std::map<Name, uint32>& data, uint32 mode) const override;
			virtual void CollectDrawInfo(std::map<Name, PCG::PCGDrawInfo>& data) const override;
			virtual void ChangeVisibleObjects(const std::set<uint16>& ObjectIds, bool bChange) override;
			virtual bool SetObjectsVisible(const std::pair<uint16, std::set<uint16>>& objIdIter, bool v) override;
			virtual bool GetObjectData(uint16 id0, uint16 id1, DVector3& worldPos, Quaternion& worldRot, Vector3& worldScl, Name& dataName, uint32& customType) override;
			virtual void QueryObjects(const OrientedBox& obb, std::set<uint64>& objIds, bool checkVisible) override;
			virtual void QueryObjects(const Sphere& obb, std::set<uint64>& objIds, bool checkVisible) override;


			BackgroundProcessTicket mTicket = 0;
			virtual void actorInfoResArrived(uint64 requestID, bool bFail, const Name& name, const ActorInfoPtr& actorInfo) override;

			static bool GetActorTriangleCountFun(const ActorPtr& pActor, uint32& triangleCount) {
				std::vector<EntityComponentPtr> entitys = pActor->getComponentsByType<EntityComponent, EntityComponent>(true);
				bool bDraw = false;
				triangleCount = 0;
				for (auto& entity : entitys) {
					EntityObject* pObj = entity->getEntityObject();
					if (pObj) {
						if (!pObj->getCulled()) {
							triangleCount += pObj->getResourceTriangleCount();
							bDraw = true;
						}
						pObj->setCulled(true);
					}
				}
				return bDraw;
			}
		};

		class ActorLoadListenerPCG : public ActorLoadListener {
		public:
			ActorLoadListenerPCG(uint16 _pId, uint16 _cId, PCGGeneralObjectCache* _pCache, ActorPtr& _actorPtr) : pId(_pId), cId(_cId), pCache(_pCache)
			{
				_actorPtr->addActorLoadListener(this);
			}
			virtual ~ActorLoadListenerPCG()
			{
				if (mActor) {
					mActor->removeActorLoadListener(this);
					mActor = nullptr;
				}
			}
			virtual bool OnActorCreateFinish() override {
				if (mActor == nullptr) return false;
				auto delObjsIter = pCache->mDeletedObjects.find(pId);
				if (delObjsIter != pCache->mDeletedObjects.end()) {
					if (delObjsIter->second.contains(cId)) {
						mActor->setVisiable(false);
						return true;
					}
				}
				mActor->setVisiable(true);
				mActor = nullptr;
				return true;
			}
			uint16 pId;
			uint16 cId;
			PCGGeneralObjectCache* pCache = nullptr;
		};

		void ActorObject::Release() {
			if (mTicket != 0) {
				ActorInfoLoadManager::instance()->abortRequest(mTicket);
				mTicket = 0;
			}
			for (ActorLoadListenerPCG* pLinstener : actorListeners) {
				SAFE_DELETE(pLinstener);
			}
			actorListeners.clear();
			if (!m_pCache->mRootActor.Expired()) {
				uint32 actorSize = (uint32)actors.size();
				for (uint32 index = 0; index != actorSize; ++index) {
					ActorPtr& pActor = actors[index];
					if (pActor) {
						m_pCache->mRootActor->removeActor(pActor, false);
						m_pCache->m_pActorMgr->deleteActorNew(pActor);
					}
				}
			}
			actors.clear();
			actorInfo.setNull();
			caches.clear();
		}

		void ActorObject::actorInfoResArrived(uint64 requestID, bool bFail, const Name& name, const ActorInfoPtr& _actorInfo) {
			if (_actorInfo.isNull()) return;
			if (requestID == mTicket) {
				mTicket = 0;
			}
			actorInfo = _actorInfo;
			CreateObjects();
		}

		bool ActorObject::PreCreateObjects() {
			actorInfo = ActorInfoManager::instance()->createOrRetrieve(actorName).first;
			if (!actorInfo.isNull()) {
				if (!actorInfo->isPrepared() && !actorInfo->isLoaded())
				{
					mTicket = ActorInfoLoadManager::instance()->addRequest(actorName, Echo::WorkQueue::MakePriority(0, Echo::WorkQueue::RESOURCE_TYPE_TERRAIN), this);
					return false;
				}
			}
			else {
				return false;
			}
			return true;
		}

		void ActorObject::CreateObjects() {
			if (actorInfo.isNull()) return;
			PCGID pcgId;
			pcgId.pid = m_pCache->mID;
			pcgId.cid_0 = id;
			uint32 size = (uint32)caches.size();
			actors.resize(size);
			actorListeners.resize(size);

			for (uint32 index = 0; index != size; ++index) {
				PCGGeneralObject::CacheData& cache = caches[index];
				ActorPtr& actorPtr = actors[index];
				actorPtr = m_pCache->m_pActorMgr->createActorByInfo(actorInfo, false);
				actorPtr->setPosition(cache.position);
				actorPtr->setRotation(cache.rotation);
				actorPtr->setScale(cache.scale);
				{
					pcgId.cid_1 = index;
					std::string str = "Actor_" + std::to_string(pcgId.id);
					actorPtr->setFinalActorID(str);
				}
				m_pCache->mRootActor->addActor(m_pCache->mRootActor->getRootNodeCom(), actorPtr);
				actorListeners[index] = new ActorLoadListenerPCG(id, index, m_pCache, actorPtr);
			}
			caches.resize(0);
			auto delObjsIter = m_pCache->mDeletedObjects.find(id);
			if (delObjsIter != m_pCache->mDeletedObjects.end()) {
				ChangeVisibleObjects(delObjsIter->second, false);
			}
		}

		void ActorObject::CollectObjects(std::map<Name, uint32>& data, uint32 mode) const {
			if ((mode & eGenObjectType_Actor) != 0 && !actors.empty()) {
				data[actorInfo->getName()] += (uint32)actors.size();
			}
		}
		void ActorObject::CollectDrawInfo(std::map<Name, PCG::PCGDrawInfo>& data) const {
			if (actors.empty()) return;
			auto& childData = data[actorInfo->getName()];
			childData.totalNumber += (uint32)actors.size();
			for (auto& pActor : actors) {
				if (pActor == nullptr) continue;
				uint32 triCount = 0;
				if (GetActorTriangleCountFun(pActor, triCount)) {
					childData.visibleTriangle += triCount;
					childData.visibleNumber += 1;
				}
			}
		}
		void ActorObject::ChangeVisibleObjects(const std::set<uint16>& ObjectIds, bool bChange) {
			if (actors.empty()) return;
			uint32 size = (uint32)actors.size();
			std::vector<uint8_t> viss;
			if (bChange) {
				viss.resize(size, 2);
				auto delObjsIter = m_pCache->mDeletedObjects.find(id);
				if (delObjsIter != m_pCache->mDeletedObjects.end()) {
					for (auto& index : ObjectIds) {
						if (index < size) {
							viss[index] = delObjsIter->second.contains(index) ? 0 : 1;
						}
					}
				}
				else {
					for (auto& index : ObjectIds) {
						if (index < size) {
							viss[index] = 1;
						}
					}
				}
			}
			else {
				viss.resize(size, 1);
				for (auto& index : ObjectIds) {
					if (index < size) {
						viss[index] = 0;
					}
				}
			}

			for (uint32 index = 0; index != size; ++index) {
				if (actors[index] == nullptr) continue;
				if (viss[index] == 0) {
					actors[index]->setVisiable(false);
				}
				else if (viss[index] == 1) {
					actors[index]->setVisiable(true);
				}
			}
		}
		bool ActorObject::SetObjectsVisible(const std::pair<uint16, std::set<uint16>>& objIdIter, bool v) {
			uint32 size = (uint32)actors.size();
			for (uint16 id : objIdIter.second) {
				if (id < size && actors[id]) {
					actors[id]->setVisiable(v);
				}
			}
			return false;
		}
		bool ActorObject::GetObjectData(uint16 id0, uint16 id1, DVector3& worldPos, Quaternion& worldRot, Vector3& worldScl, Name& dataName, uint32& customType) {
			uint32 size = (uint32)actors.size();
			if (id1 >= size) return false;
			ActorPtr& pNode = actors[id1];
			if (pNode == nullptr) return false;
			worldPos = pNode->getWorldPosition();
			worldRot = pNode->getWorldRotation();
			worldScl = pNode->getWorldScale();
			dataName = actorInfo->getName();
			customType = custom_type;
			return true;
		}
		void ActorObject::QueryObjects(const OrientedBox& obb, std::set<uint64>& objIds, bool checkVisible) {
			if (actors.empty()) return;
			PCGID pcgId;
			pcgId.pid = m_pCache->mID;
			Sphere sph;
			pcgId.cid_0 = id;
			uint32 size = (uint32)actors.size();
			for (uint32 index = 0; index != size; ++index) {
				ActorPtr& pNode = actors[index];
				if (pNode == nullptr) continue;
				if (checkVisible && !pNode->getVisiable()) continue;
				Vector3 testCenter = pNode->getLocalBounds().getCenter();
				float testRadius = pNode->getLocalBounds().getHalfSize().length();
				sph.mCenter = pNode->getRotation() * (testCenter * pNode->getScale()) + pNode->getPosition();
				sph.mRadius = testRadius * pNode->getScale()[pNode->getScale().MainAxis() - 1];
				if (obb.Intersect(sph)) {
					pcgId.cid_1 = index;
					objIds.insert(pcgId.id);
				}
			}
		}
		void ActorObject::QueryObjects(const Sphere& qSph, std::set<uint64>& objIds, bool checkVisible) {
			if (actors.empty()) return;
			PCGID pcgId;
			pcgId.pid = m_pCache->mID;
			Sphere sph;
			pcgId.cid_0 = id;
			uint32 size = (uint32)actors.size();
			for (uint32 index = 0; index != size; ++index) {
				ActorPtr& pNode = actors[index];
				if (pNode == nullptr) continue;
				if (checkVisible && !pNode->getVisiable()) continue;
				Vector3 testCenter = pNode->getLocalBounds().getCenter();
				float testRadius = pNode->getLocalBounds().getHalfSize().length();
				sph.mCenter = pNode->getRotation() * (testCenter * pNode->getScale()) + pNode->getPosition();
				sph.mRadius = testRadius * pNode->getScale()[pNode->getScale().MainAxis() - 1];
				if (qSph.intersects(sph)) {
					pcgId.cid_1 = index;
					objIds.insert(pcgId.id);
				}
			}
		}

	}

	namespace PCG {
		PCG::PCGGeneralObject* CreateGeneralObjectFun(const PCG::StaticMesh& _StaticMesh, PCG::PCGGeneralObjectCache* pCache) {
			switch (_StaticMesh.object_type) {
			case PCG::eGenObjectType_StaticMesh: {
				StaticMeshObject* pObj = new StaticMeshObject(_StaticMesh.component, pCache);
				pObj->meshName = _StaticMesh.mesh;
				pObj->bPhysics = _StaticMesh.physics;
				pObj->bCastShadow = _StaticMesh.cast_shadow;
				pObj->surface_type = _StaticMesh.surface_type;
				pObj->meshphyName = _StaticMesh.meshphy;
				pObj->custom_type = _StaticMesh.custom_type;
				pObj->lod1_distance = _StaticMesh.lod1_distance;
				pObj->lod2_distance = _StaticMesh.lod2_distance;
				pObj->render_distance = _StaticMesh.render_distance;
				return pObj;
			}
			case PCG::eGenObjectType_Actor: {
				ActorObject* pObj = new ActorObject(pCache);
				pObj->actorName = _StaticMesh.mesh;
				pObj->custom_type = _StaticMesh.custom_type;
				return pObj;
			}
			case PCG::eGenObjectType_EG: {
				ActorObject* pObj = new ActorObject(pCache);
				pObj->actorName = _StaticMesh.mesh;
				pObj->custom_type = _StaticMesh.custom_type;
				return pObj;
			}
			default: {
				PCG::PCGGeneralObject* pObj = new PCG::PCGGeneralObject(pCache);
				return pObj;
			}
			}
		}
		void DeleteGeneralObjectFun(PCG::PCGGeneralObject* pObj) {
			SAFE_DELETE(pObj);
		}
	}
}

namespace Echo
{
	PCGGenerateObjectManager::PCGGenerateObjectManager()
	{
		//Echo::Root::instance()->addFrameListener(this);
		PCG::PCGGeneralObjectCache::gCreateGeneralObjectFun = std::bind<PCG::PCGGeneralObject*>(PCG::CreateGeneralObjectFun, std::placeholders::_1, std::placeholders::_2);
		PCG::PCGGeneralObjectCache::gDeleteGeneralObjectFun = std::bind<void>(PCG::DeleteGeneralObjectFun, std::placeholders::_1);
	}
	PCGGenerateObjectManager::~PCGGenerateObjectManager()
	{
		//Echo::Root::instance()->removeFrameListener(this);
	}
	bool PCGGenerateObjectManager::frameStarted(const FrameEvent& evt)
	{
		return true;
	}
}
