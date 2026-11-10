#include "EchoComponentStableHeaders.h"
#include "EchoPhysicsManager.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "EchoRigidBodyComponent.h"
//#include "EchoRigidBodyGroupComponent.h"
#include "EchoColliderComponent.h"
#include "EchoJointComponent.h"
#include <functional>
#include "EchoPhysXAsset.h"
#include "EchoPhysXSystem.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXCallback.h"
#include "EchoPhysXVehicleNoDrive.h"
//#include "EchoWheelColliderComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoCharactorControllerComponent.h"
#include "EchoJointComponent.h"
#include "EchoConstantForceComponent.h"
#include "Actor/EchoActorManager.h"
#include "EchoRagdollComponent.h"
#include "EchoPhysXCharacterController.h"
#include "EchoPhysXJoint.h"
#include "EchoKeyBoard.h"
#include "EchoSphereColliderComponent.h"
#include "EchoMeshColliderComponent.h"
#include "EchoTerrainColliderComponent.h"
#include "EchoPhysXManager.h"
#include "EchoPhysicsPage.h"
#include "EchoPhysicsPagedLayer.h"
#include "EchoAggregateComponent.h"
#include "EchoActorEntityComponent.h"
#include "EchoTimer.h"
#include "EchoMeshColliderComponent.h"
#include "EchoAuxiliaryManager.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoGeneratedTerrainPhysics.h"
#include "EchoPhysXCharacterController.h"
#include "EchoActorRoleComponent.h"
#include "EchoIKPointComponent.h"
#include "EchoPhysXManager.h"
#include "EchoPhysXAggregate.h"

#ifdef USE_PHYSICS_PAGE
namespace Echo {

	namespace EchoPhysics {
		class UnionFind {
		public:
			UnionFind(int numVertices) {
				mParent.resize(numVertices);
				mRank.resize(numVertices);
				for (int i = 0; i < numVertices; i++) {
					mParent[i] = i;
					mRank[i] = 0;
				}
			}
			int find(int x) {
				if (mParent[x] != x) {
					mParent[x] = find(mParent[x]);
				}
				return mParent[x];
			}
			void unite(int x, int y) {
				int root1 = find(x);
				int root2 = find(y);

				if (root1 != root2) {
					if (mRank[root1] < mRank[root2]) {
						mParent[root1] = root2;
					}
					else if (mRank[root1] > mRank[root2]) {
						mParent[root2] = root1;
					}
					else {
						mParent[root2] = root1;
						mRank[root1]++;
					}
				}
			}
			bool isConnected(int x, int y) {
				return find(x) == find(y);
			}
			int getGroup(std::vector<int>& groups) {
				int size = (int)mParent.size();
				int group = 0;
				int n = 0;
				groups.resize(size, -1);
				while (n != size) {
					int curIndex = 0;
					while (curIndex != size) {
						if (groups[curIndex] != -1) {
							++curIndex;
						}
						else {
							break;
						}
					}
					if (curIndex == size) {
						break;
					}
					groups[curIndex] = group;
					++n;
					for (int i = 0; i != size; ++i) {
						if (groups[i] != -1) {
							continue;
						}
						if (isConnected(curIndex, i)) {
							groups[i] = group;
							++n;
						}
					}
					++group;
				}
				return group;
			}
		private:
			std::vector<int> mParent;
			std::vector<int> mRank;
		};
	}

	namespace EchoPhysics {

		template<typename GraphNodeT>
		Graph<GraphNodeT>::Graph(Graph<GraphNodeT>& graph) {
			mData.swap(graph.mData);
		}

		template<typename GraphNodeT>
		Graph<GraphNodeT>::Graph(const std::vector<GraphNodeT*>& nodes) {
			for (GraphNodeT* pNode : nodes) {
				mData.insert(std::make_pair(pNode, std::list<GraphNodeT*>()));
			}
		}

		template<typename GraphNodeT>
		Graph<GraphNodeT>::Graph(const std::vector<GraphNodeT*>& nodes, const std::vector<std::pair<GraphNodeT*, GraphNodeT*>>& edges, bool bTwoWay) {
			for (GraphNodeT* pNode : nodes) {
				mData.insert(std::make_pair(pNode, std::list<GraphNodeT*>()));
			}
			for (auto& edge : edges) {
				auto iter1 = mData.find(edge.first);
				iter1->second.push_back(edge.second);
				if (bTwoWay) {
					auto iter2 = mData.find(edge.second);
					iter2->second.push_back(edge.first);
				}
			}
		}

		template<typename GraphNodeT>
		Graph<GraphNodeT>::Graph(const std::set<GraphNodeT*>& nodes, const std::vector<std::pair<GraphNodeT*, GraphNodeT*>>& edges) {
			for (GraphNodeT* pNode : nodes) {
				mData.insert(std::make_pair(pNode, std::list<GraphNodeT*>()));
			}
			for (auto& edge : edges) {
				auto iter1 = mData.find(edge.first);
				iter1->second.push_back(edge.second);
			}
		}

		template<typename GraphNodeT>
		void Graph<GraphNodeT>::merge(Graph& graph) {
			if (graph.empty()) return;
			for (auto& data : graph.mData) {
				auto iter = mData.find(data.first);
				if (iter == mData.end()) {
					mData.insert(data);
				}
				else {
					iter->second.splice(iter->second.begin(), data.second);
				}
			}
			graph.clear();
		}

		template<typename GraphNodeT>
		void Graph<GraphNodeT>::clear() {
			for (auto& iter : mData) {
				iter.second.clear();
			}
			mData.clear();
		}

		template<typename GraphNodeT>
		void Graph<GraphNodeT>::AddNode(GraphNodeT* pNode) {
			auto iter = mData.find(pNode);
			if (iter != mData.end()) {
				return;
			}
			mData.insert(std::make_pair(pNode, std::list<GraphNodeT*>()));
		}
		template<typename GraphNodeT>
		bool Graph<GraphNodeT>::RemoveNode(GraphNodeT* pNode) {
			auto iter = mData.find(pNode);
			if (iter == mData.end()) {
				return false;
			}
			auto& edges = iter->second;
			for (GraphNodeT* pOtherNode : edges) {
				mData.find(pOtherNode)->second.remove(pNode);
			}
			bool res = edges.size() > 1;
			mData.erase(iter);
			return res;
		}
		template<typename GraphNodeT>
		void Graph<GraphNodeT>::AddEdge(GraphNodeT* pNode1, GraphNodeT* pNode2) {
			AddNode(pNode1);
			AddNode(pNode2);
			auto iter1 = mData.find(pNode1);
			auto iter2 = mData.find(pNode2);
			iter1->second.push_back(pNode2);
			iter2->second.push_back(pNode1);
		}
		template<typename GraphNodeT>
		bool Graph<GraphNodeT>::RemoveEdge(GraphNodeT* pNode1, GraphNodeT* pNode2) {
			auto iter1 = mData.find(pNode1);
			auto iter2 = mData.find(pNode2);
			if (iter1 == mData.end() || iter2 == mData.end()) {
				return false;
			}
			iter1->second.remove(pNode2);
			iter2->second.remove(pNode1);
			return true;
		}

		template<typename GraphNodeT>
		bool Graph<GraphNodeT>::Reset(std::vector<Graph*>& newGraph, std::vector<GraphNodeT*>& newNode) {
			if (mData.empty()) {
				return true;
			}
			std::unordered_map<GraphNodeT*, int> indexMap;
			std::vector<GraphNodeT*> nodeVec;
			int size = 0;
			bool bUnionFind = false;
			for (auto& iter : mData) {
				indexMap.insert(std::make_pair(iter.first, size++));
				nodeVec.push_back(iter.first);
				if (!iter.second.empty()) {
					bUnionFind = true;
				}
			}
			if (!bUnionFind) {
				newNode.clear();
				newNode.swap(nodeVec);
				clear();
				return true;
			}
			UnionFind uf(size);
			for (auto& node : mData) {
				int fIndex = indexMap[node.first];
				for (auto& iter : node.second) {
					int sIndex = indexMap[iter];
					if (fIndex < sIndex) {
						uf.unite(fIndex, sIndex);
					}
				}
			}

			std::vector<int> groups;
			int group = uf.getGroup(groups);
			if (group == 1) {
				return false;
			}
			std::vector<std::vector<int>> groupNbs(group);
			for (int i = 0; i != size; ++i) {
				groupNbs[groups[i]].push_back(i);
			}
			for (int i = 0; i != group; ++i) {
				if (groupNbs[i].size() == 1) {
					newNode.push_back(nodeVec[groupNbs[i][0]]);
				}
				else {
					int curNewGraphSize = (int)groupNbs[i].size();
					std::map<GraphNodeT*, std::list<GraphNodeT*>> newGraphData;
					for (int j = 0; j != curNewGraphSize; ++j) {
						GraphNodeT* pNode = nodeVec[groupNbs[i][j]];
						auto nodeEdges = mData.find(pNode);
						newGraphData.insert(std::make_pair(pNode, nodeEdges->second));
					}
					Graph* pGraph = new Graph(newGraphData);
					newGraph.push_back(pGraph);
				}
			}
			clear();
			return true;
		}

	}

	class EchoPhysicsPageGraph {
	public:
		void Clear() {
			for (ColliderComponent* pComp : mColliderSet) {
				if (pComp) {
					pComp->m_bWaitCreateGroup = false;
				}
			}
			mColliderSet.clear();
			for (RigidBodyComponent* pComp : mRigidBodySet) {
				if (pComp) {
					pComp->m_bWaitCreateGroup = false;
				}
			}
			mRigidBodySet.clear();
			for (JointComponent* pComp : mJointSet) {
				if(pComp) {
					pComp->m_bWaitCreateGroup = false;
				}
			}
			mJointSet.clear();
			for (EchoPhysXGroupObject* pGroup : mGroupSet) {
				SAFE_DELETE(pGroup);
			}
			mGroupSet.clear();
			bCreateGroup = false;
		}
		void AddObject(ColliderComponent* compPtr) {
			if (compPtr == nullptr || compPtr->m_bWaitCreateGroup) return;
			mColliderSet.push_back(compPtr);
			compPtr->m_bWaitCreateGroup = true;
			if (!bCreateGroup) bCreateGroup = true;
		}
		void AddObject(RigidBodyComponent* compPtr) {
			if (compPtr == nullptr || compPtr->m_bWaitCreateGroup) return;
			mRigidBodySet.push_back(compPtr);
			compPtr->m_bWaitCreateGroup = true;
			if (!bCreateGroup) bCreateGroup = true;
		}
		void AddObject(JointComponent* compPtr) {
			if (compPtr == nullptr || compPtr->m_bWaitCreateGroup) return;
			mJointSet.push_back(compPtr);
			compPtr->m_bWaitCreateGroup = true;
			if (!bCreateGroup) bCreateGroup = true;
		}
		void RemoveObject(ColliderComponent* compPtr) {
			if (compPtr == nullptr) return;

			if (compPtr->m_bWaitCreateGroup) {
				for (auto& comp : mColliderSet) {
					if (comp == compPtr) {
						comp = nullptr;
						break;
					}
				}
				compPtr->m_bWaitCreateGroup = false;
			}

			EchoPhysXActorObject* ptr = compPtr->GetEchoPhysXObject();
			if (ptr != nullptr) {
				if (ptr->isStatic()) {
					EchoPhysXStaticObject* pObject = (EchoPhysXStaticObject*)compPtr->mPhysXObject;
					EchoPhysXGroupObject* pGroup = pObject->GetGroup();
					if (pGroup != nullptr) {
						std::vector<EchoPhysXObjectGraph*> newGraphs;
						std::vector<EchoPhysXObject*> newNodes;
						if (pGroup->RemoveNode(pObject, newGraphs, newNodes)) {
							for (auto& iter : newGraphs) {
								AddPhysXGroup((EchoPhysXObjectGraph*)iter);
							}
							for (auto& iter : newNodes) {
								AddPhysXObject(iter);
							}
							RemovePhysXGroup(pGroup);
						}
					}
				}
			}
			
			m_pManager->removeEchoPhysXObject(compPtr->mPhysXObject);
			SAFE_DELETE(compPtr->mPhysXObject);
			compPtr->SetIsUsePhysics(false);
		}
		void RemoveObject(RigidBodyComponent* compPtr) {
			if (compPtr == nullptr) return;

			if (compPtr->m_bWaitCreateGroup) {
				for (auto& comp : mRigidBodySet) {
					if (comp == compPtr) {
						comp = nullptr;
						break;
					}
				}
				compPtr->m_bWaitCreateGroup = false;
			}

			if (compPtr->mPhysXObject != nullptr) {
				EchoPhysXDynamicObject* pObject = (EchoPhysXDynamicObject*)compPtr->mPhysXObject;
				EchoPhysXGroupObject* pGroup = pObject->GetGroup();
				if (pGroup != nullptr) {
					std::vector<EchoPhysXObjectGraph*> newGraphs;
					std::vector<EchoPhysXObject*> newNodes;
					if (pGroup->RemoveNode(pObject, newGraphs, newNodes)) {
						for (auto& iter : newGraphs) {
							AddPhysXGroup((EchoPhysXObjectGraph*)iter);
						}
						for (auto& iter : newNodes) {
							AddPhysXObject(iter);
						}
						RemovePhysXGroup(pGroup);
					}
				}
			}
			
			m_pManager->removeEchoPhysXObject(compPtr->mPhysXObject);
			SAFE_DELETE(compPtr->mPhysXObject);
			compPtr->SetIsUsePhysics(false);
		}
		void RemoveObject(JointComponent* compPtr) {
			if (compPtr == nullptr) return;

			if (compPtr->m_bWaitCreateGroup) {
				for (auto& comp : mJointSet) {
					if (comp == compPtr) {
						comp = nullptr;
						break;
					}
				}
				compPtr->m_bWaitCreateGroup = false;
			}

			EchoPhysXActorObject* ptr = compPtr->GetEchoPhysXObject();
			if (ptr != nullptr) {
				EchoPhysXGroupObject* pGroup = ptr->GetGroup();
				if (pGroup != nullptr) {
					std::vector<EchoPhysXObjectGraph*> newGraphs;
					std::vector<EchoPhysXObject*> newNodes;
					if (pGroup->RemoveEdge(compPtr, newGraphs, newNodes)) {
						for (auto& iter : newGraphs) {
							AddPhysXGroup((EchoPhysXObjectGraph*)iter);
						}
						for (auto& iter : newNodes) {
							AddPhysXObject(iter);
						}
						RemovePhysXGroup(pGroup);
					}
				}
			}
			compPtr->SetIsUsePhysics(false);
		}
		void CreateGroup() {
			if (!bCreateGroup) return;
			bCreateGroup = false;
			std::set<EchoPhysXObject*> tempNodes;
			std::vector<std::pair<EchoPhysXObject*, EchoPhysXObject*>> tempEdges;

			for (RigidBodyComponent* rigidbody : mRigidBodySet) {
				if (rigidbody == nullptr) {
					continue;
				}
				rigidbody->m_bWaitCreateGroup = false;
				if (rigidbody->mPhysXObject == nullptr) {
					rigidbody->mPhysXObject = new EchoPhysXDynamicObject(rigidbody);
				}
				tempNodes.insert(rigidbody->mPhysXObject);

				std::vector<JointComponent*> joints;
				rigidbody->GetJointCompList(joints);
				for (auto& iter : joints) {
					if (!iter->m_bWaitCreateGroup) {
						mJointSet.push_back(iter);
						iter->m_bWaitCreateGroup = true;
					}
				}
			}
			mRigidBodySet.clear();

			for (ColliderComponent* collider : mColliderSet) {
				if (collider == nullptr) {
					continue;
				}
				collider->m_bWaitCreateGroup = false;
				if (collider->GetRigidBody() == nullptr) {
					if (collider->mPhysXObject == nullptr) {
						collider->mPhysXObject = new EchoPhysXStaticObject(collider);
					}
					tempNodes.insert(collider->mPhysXObject);

					std::vector<JointComponent*> joints;
					collider->GetJointCompList(joints);
					for (auto& iter : joints) {
						if (!iter->m_bWaitCreateGroup) {
							mJointSet.push_back(iter);
							iter->m_bWaitCreateGroup = true;
						}
					}
				}
				else {
					if (collider->mPhysXObject != nullptr) {
						LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- Internal processing error of physical paging.");
						RemoveObject_Error(collider);
					}
					collider->SetIsUsePhysics(collider->GetRigidBody()->GetIsUsePhysics());
				}
			}
			mColliderSet.clear();

			for (JointComponent* joint : mJointSet) {
				if (joint == nullptr) {
					continue;
				}
				joint->m_bWaitCreateGroup = false;
				joint->SetIsUsePhysics(true);
				auto& ptr1 = joint->GetSelfRigidBodyComp();
				auto& ptr2 = joint->GetConnectedRigidBodyComp();
				if (ptr1 == nullptr || ptr2 == nullptr) {
					continue;
				}
				EchoPhysXActorObject* p1 = ptr1->mPhysXObject;
				EchoPhysXActorObject* p2 = ptr2->mPhysXObject;
				if (p1 != nullptr && p2 != nullptr) {
					if (p1->GetGroup() != nullptr) {
						EchoPhysXGroupObject* pGroup = p1->GetGroup();
						if (p2->GetGroup() != nullptr && pGroup == p2->GetGroup()) {
							pGroup->AddEdge(joint);
							continue;
						}
						for (auto& iter : pGroup->m_pGraph->mData) {
							tempNodes.insert(iter.first);
							for (auto& iter2 : iter.second) {
								tempEdges.push_back(std::make_pair(iter.first, iter2));
							}
						}
						RemovePhysXGroup(pGroup);
					}
					else {
						tempNodes.insert(p1);
					}
					if (p2->GetGroup() != nullptr) {
						EchoPhysXGroupObject* pGroup = p2->GetGroup();
						for (auto& iter : pGroup->m_pGraph->mData) {
							tempNodes.insert(iter.first);
							for (auto& iter2 : iter.second) {
								tempEdges.push_back(std::make_pair(iter.first, iter2));
							}
						}
						RemovePhysXGroup(pGroup);
					}
					else {
						tempNodes.insert(p2);
					}
					tempEdges.push_back(std::make_pair(p1, p2));
					tempEdges.push_back(std::make_pair(p2, p1));
				}
			}
			mJointSet.clear();

			EchoPhysXObjectGraph mGraph(tempNodes, tempEdges);
			std::vector<EchoPhysXObjectGraph*> newGraphs;
			std::vector<EchoPhysXObject*> newNodes;
			if (mGraph.Reset(newGraphs, newNodes)) {
				for (auto& iter : newGraphs) {
					AddPhysXGroup((EchoPhysXObjectGraph*)iter);
				}
				for (auto& iter : newNodes) {
					AddPhysXObject(iter);
				}
			}
			else {
				EchoPhysXObjectGraph* pGraph = new EchoPhysXObjectGraph(mGraph);
				AddPhysXGroup((EchoPhysXObjectGraph*)pGraph);
			}
		}
		void AddPhysXGroup(EchoPhysXObjectGraph* pGraph) {
			if (pGraph == nullptr) return;
			EchoPhysXGroupObject* ptr = new EchoPhysXGroupObject(pGraph);
			mGroupSet.insert(ptr);
			m_pManager->addEchoPhysXObject(ptr);
		}
		void AddPhysXObject(EchoPhysXObject* pObject) {
			if (pObject == nullptr) return;
			((EchoPhysXActorObject*)pObject)->SetGroup(nullptr);
			m_pManager->addEchoPhysXObject(pObject);
		}
		void RemovePhysXGroup(EchoPhysXGroupObject* pGroup) {
			if (pGroup == nullptr) return;
			m_pManager->removeEchoPhysXObject(pGroup);
			mGroupSet.erase(pGroup);
			SAFE_DELETE(pGroup);
		}
		void RemoveObject_Error(PhysicsActorComponent* compPtr) {
			if (compPtr == nullptr) return;

			if (compPtr->m_bWaitCreateGroup) {
				for (auto& comp : mColliderSet) {
					if (comp == compPtr) {
						comp = nullptr;
						break;
					}
				}
				compPtr->m_bWaitCreateGroup = false;
			}

			if (compPtr->mPhysXObject != nullptr) {
				EchoPhysXActorObject* pObject = (EchoPhysXActorObject*)compPtr->mPhysXObject;
				EchoPhysXGroupObject* pGroup = pObject->GetGroup();
				if (pGroup != nullptr) {
					RemovePhysXGroup(pGroup);
				}
				m_pManager->removeEchoPhysXObject(compPtr->mPhysXObject);
				SAFE_DELETE(compPtr->mPhysXObject);
			}
		}
	public:
		bool bCreateGroup = false;
		EchoPhysicsManager* m_pManager = nullptr;
		std::vector<ColliderComponent*> mColliderSet;
		std::vector<RigidBodyComponent*> mRigidBodySet;
		std::vector<JointComponent*> mJointSet;
		std::unordered_set<EchoPhysXGroupObject*> mGroupSet;
	};
}

#endif // USE_PHYSICS_PAGE

namespace Echo {

	static Name gRigidBodyName = COMPONENTTYPE(RigidBodyComponent);
	static Name gColliderName = COMPONENTTYPE(ColliderComponent);
	static Name gJointName = COMPONENTTYPE(JointComponent);
	static Name gConstantForceName = COMPONENTTYPE(ConstantForceComponent);
	static Name gCharatorControllerName = COMPONENTTYPE(CharatorControllerComponent);
	static Name gAggregateName = COMPONENTTYPE(AggregateComponent);
	static Name gTerrainColliderName = COMPONENTTYPE(TerrainColliderComponent);

}

namespace Echo {
	
	void PhysXSceneQueryHitToEchoSceneQueryHit(PhysXSceneQueryHit& physXhit, EchoSceneQueryHit& hit) {
		EchoPhysicsManager::PhysXPtrToEchoPtr(physXhit.actor, physXhit.shape, hit.rigidbody, hit.collider);
	}
	void PhysXRaycastHitToEchoRaycastHit(PhysXRaycastHit& physXhit, EchoRaycastHit& hit, EchoSceneQueryFlags flags) {
		hit.distance = physXhit.distance;
		hit.point = physXhit.position;
		hit.normal = physXhit.normal;
		PhysXSceneQueryHitToEchoSceneQueryHit(physXhit, hit);
		if ((flags & EchoSceneQueryFlag::eSURFACE_TYPE) && hit.collider != nullptr) {
			hit.surfaceType = hit.collider->GetSurfaceType(hit.point);
		}
		else {
			hit.surfaceType = 0;
		}
	}
	bool EchoPhysXSceneQueryFilterCallBack(const EchoSceneQueryFilter& filter, PhysicsActor* actor, void* shapeUserData) {
		EchoSceneQueryHit hit;
		PhysXSceneQueryHit physXHit;
		physXHit.actor = actor;
		physXHit.shape = (PhysXShape*)shapeUserData;
		PhysXSceneQueryHitToEchoSceneQueryHit(physXHit, hit);
		if (hit.rigidbody != nullptr || hit.collider != nullptr) {
			return filter(hit.rigidbody, hit.collider);
		}
		return false;
	}

	EchoHitFlag EchoSceneQueryFlagToEchoHitFlag(EchoSceneQueryFlags flag) {
		return (EchoHitFlag)(uint16)((uint32)flag & 0xFFFF);
	}

	EchoQueryFlag EchoSceneQueryFlagToEchoQueryFlag(EchoSceneQueryFlags flag) {
		return (EchoQueryFlag)(uint16)((uint32)((uint32)flag >> 16) & 0x00FF);
	}

	bool EchoPhysicsQuery::Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		PhysXRaycastHit physXhit;
		return GetPhysXSceneQuery()->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
	}
	bool EchoPhysicsQuery::Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		PhysXRaycastHit physXhit;
		bool res = GetPhysXSceneQuery()->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		PhysXRaycastHitToEchoRaycastHit(physXhit, hit, _flag);
		return res;
	}
	bool EchoPhysicsQuery::Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		std::vector<PhysXRaycastHit> physXhits;
		bool res = GetPhysXSceneQuery()->Raycast(origin, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		int size = (int)physXhits.size();
		hits.resize(size);
		for (int i = 0; i != size; ++i) {
			PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], _flag);
		}
		return res;
	}
	bool EchoPhysicsQuery::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		PhysXRaycastHit physXhit;
		return GetPhysXSceneQuery()->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
	}
	bool EchoPhysicsQuery::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		PhysXRaycastHit physXhit;
		bool res = GetPhysXSceneQuery()->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		PhysXRaycastHitToEchoRaycastHit(physXhit, hit, _flag);
		return res;
	}
	bool EchoPhysicsQuery::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		std::vector<PhysXRaycastHit> physXhits;
		bool res = GetPhysXSceneQuery()->Sweep(*geometry, pose, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		int size = (int)physXhits.size();
		hits.resize(size);
		for (int i = 0; i != size; ++i) {
			PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], _flag);
		}
		return res;
	}
	bool EchoPhysicsQuery::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		PhysXSceneQueryHit physXHit;
		return GetPhysXSceneQuery()->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
	}
	bool EchoPhysicsQuery::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		PhysXSceneQueryHit physXHit;
		bool res = GetPhysXSceneQuery()->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		PhysXSceneQueryHitToEchoSceneQueryHit(physXHit, hit);
		return res;
	}
	bool EchoPhysicsQuery::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry) return false;
		std::vector<PhysXSceneQueryHit> physXhits;
		bool res = GetPhysXSceneQuery()->Overlap(*geometry, pose, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr), EchoSceneQueryFlagToEchoHitFlag(_flag), EchoSceneQueryFlagToEchoQueryFlag(_flag));
		int size = (int)physXhits.size();
		hits.resize(size);
		for (int i = 0; i != size; ++i) {
			PhysXSceneQueryHitToEchoSceneQueryHit(physXhits[i], hits[i]);
		}
		return res;
	}

	bool EchoPhysicsQuery::SphereCast(const DTransform& pose, const Vector3& direction, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return GeometryCast(&geom, pose, direction, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::SphereCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return GeometryCast(&geom, pose, direction, hit, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::SphereCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return GeometryCast(&geom, pose, direction, hits, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::BoxCast(const DTransform& pose, const Vector3& direction, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return GeometryCast(&geom, pose, direction, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::BoxCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return GeometryCast(&geom, pose, direction, hit, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::BoxCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return GeometryCast(&geom, pose, direction, hits, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::CapsuleCast(const DTransform& pose, const Vector3& direction, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return GeometryCast(&geom, pose, direction, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::CapsuleCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return GeometryCast(&geom, pose, direction, hit, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::CapsuleCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return GeometryCast(&geom, pose, direction, hits, maxDistance, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::ColliderCast(ColliderComponent* collider, const Vector3& direction, float maxDistance, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		bool res = GeometryCast(pGeom, pose, direction, maxDistance, collider->GetCanInteractFilterGroupType(), filter, _flag);
		return res;
	}
	bool EchoPhysicsQuery::ColliderCast(ColliderComponent* collider, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		bool res = GeometryCast(pGeom, pose, direction, hit, maxDistance, collider->GetCanInteractFilterGroupType(), filter, _flag);
		return res;
	}
	bool EchoPhysicsQuery::ColliderCast(ColliderComponent* collider, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		bool res = GeometryCast(pGeom, pose, direction, hits, maxDistance, collider->GetCanInteractFilterGroupType(), filter, _flag);
		return res;
	}

	bool EchoPhysicsQuery::OverlapSphere(const DTransform& pose, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return OverlapGeometry(&geom, pose, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapSphere(const DTransform& pose, EchoSceneQueryHit& hit, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return OverlapGeometry(&geom, pose, hit, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapSphere(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoSphereGeometry geom(radius);
		return OverlapGeometry(&geom, pose, hits, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapBox(const DTransform& pose, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return OverlapGeometry(&geom, pose, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapBox(const DTransform& pose, EchoSceneQueryHit& hit, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return OverlapGeometry(&geom, pose, hit, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapBox(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoBoxGeometry geom(halfSize);
		return OverlapGeometry(&geom, pose, hits, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCapsule(const DTransform& pose, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return OverlapGeometry(&geom, pose, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCapsule(const DTransform& pose, EchoSceneQueryHit& hit, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return OverlapGeometry(&geom, pose, hit, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCapsule(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		EchoCapsuleGeometry geom(radius, middleHeight * 0.5f);
		return OverlapGeometry(&geom, pose, hits, layerMask, filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCollider(ColliderComponent* collider, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		return OverlapGeometry(pGeom, pose, collider->GetCanInteractFilterGroupType(), filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCollider(ColliderComponent* collider, EchoSceneQueryHit& hit, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		return OverlapGeometry(pGeom, pose, hit, collider->GetCanInteractFilterGroupType(), filter, _flag);
	}
	bool EchoPhysicsQuery::OverlapCollider(ColliderComponent* collider, std::vector<EchoSceneQueryHit>& hits, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!collider) return false;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* pGeom = collider->GenerateGeometry(buffer);
		if (!pGeom) return false;
		DTransform pose(collider->getWorldPosition(), collider->getWorldRotation());
		return OverlapGeometry(pGeom, pose, hits, collider->GetCanInteractFilterGroupType(), filter, _flag);
	}


}

namespace Echo {

	class CommonQuery {
	public:
		enum Enum : uint16 {
			eRAYCAST = 0,
			eSWEEP = 1,
			eOVERLAP = 2,
		};
	public:
		virtual ~CommonQuery() {}

		bool mResult = false;
		Enum mType;
		PhysXGroup mLayerMask;
		const EchoSceneQueryFilter& mFilter;
		EchoSceneQueryFlags mFlags;

		EchoCommonQuery::Enum mHitType = EchoCommonQuery::eNONE;

		const EchoGeometry* mGeometry = nullptr;
		DTransform mPose;
		Vector3 mDirection;
		float mDistance;
		std::vector<EchoRaycastHit> mHits;

		CommonQuery(EchoCommonQuery::Enum hitType, const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags)
			: mType(eRAYCAST), mLayerMask(layerMask), mFilter(filter), mFlags(flags),
			mHitType(hitType), mPose(origin), mDirection(direction), mDistance(maxDistance) {
			if (mHitType == EchoCommonQuery::eSINGLE) {
				mHits.resize(1);
			}
			if (mHitType != EchoCommonQuery::eNONE) {
				mFlags |= EchoSceneQueryFlag::eALLHITDATA;
			}
		}

		CommonQuery(EchoCommonQuery::Enum hitType, const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags)
			: mType(eSWEEP), mLayerMask(layerMask), mFilter(filter), mFlags(flags),
			mHitType(hitType), mGeometry(geometry), mPose(pose), mDirection(direction), mDistance(maxDistance) {
			if (mHitType == EchoCommonQuery::eSINGLE) {
				mHits.resize(1);
			}
			if (mHitType != EchoCommonQuery::eNONE) {
				mFlags |= EchoSceneQueryFlag::eALLHITDATA;
			}
		}

		CommonQuery(EchoCommonQuery::Enum hitType, const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags)
			: mType(eOVERLAP), mLayerMask(layerMask), mFilter(filter), mFlags(flags),
			mHitType(hitType), mGeometry(geometry), mPose(pose) {
			if (mHitType == EchoCommonQuery::eSINGLE) {
				mHits.resize(1);
			}
			if (mHitType != EchoCommonQuery::eNONE) {
				mFlags |= EchoSceneQueryFlag::eALLHITDATA;
			}
		}

		void Execute(EchoPhysicsQuery* mQuery) {
			mResult = false;
			switch (mType) {
			case eRAYCAST: {
				switch (mHitType) {
				case EchoCommonQuery::eNONE: {
					mResult = mQuery->Raycast(mPose.translation, mDirection, mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eSINGLE: {
					mResult = mQuery->Raycast(mPose.translation, mDirection, mHits[0], mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eMULTIPLE: {
					mResult = mQuery->Raycast(mPose.translation, mDirection, mHits, mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				}
				break;
			}
			case eSWEEP: {
				switch (mHitType) {
				case EchoCommonQuery::eNONE: {
					mResult = mQuery->GeometryCast(mGeometry, mPose, mDirection, mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eSINGLE: {
					mResult = mQuery->GeometryCast(mGeometry, mPose, mDirection, mHits[0], mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eMULTIPLE: {
					mResult = mQuery->GeometryCast(mGeometry, mPose, mDirection, mHits, mDistance, mLayerMask, mFilter, mFlags);
					break;
				}
				}
				break;
			}
			case eOVERLAP: {
				switch (mHitType) {
				case EchoCommonQuery::eNONE: {
					mResult = mQuery->OverlapGeometry(mGeometry, mPose, mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eSINGLE: {
					mResult = mQuery->OverlapGeometry(mGeometry, mPose, mHits[0], mLayerMask, mFilter, mFlags);
					break;
				}
				case EchoCommonQuery::eMULTIPLE: {
					std::vector<EchoSceneQueryHit> _Hits;
					mResult = mQuery->OverlapGeometry(mGeometry, mPose, _Hits, mLayerMask, mFilter, mFlags);
					uint32 size = (uint32)_Hits.size();
					if (size != 0) {
						mHits.reserve(size);
						for (uint32 i = 0; i != size; ++i) {
							mHits.emplace_back(_Hits[i]);
						}
					}
					break;
				}
				}
				break;
			}
			}
		}

		bool GetResult() { return mResult; }
		const std::vector<EchoRaycastHit>& GetResults() { return mHits; }
	};

	EchoCommonQuery::EchoCommonQuery(EchoCommonQuery::Enum hitType, const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags) {
		m_private = new CommonQuery(hitType, origin, direction, maxDistance, layerMask, filter, flags);
	}
	EchoCommonQuery::EchoCommonQuery(EchoCommonQuery::Enum hitType, const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags) {
		m_private = new CommonQuery(hitType, geometry, pose, direction, maxDistance, layerMask, filter, flags);
	}
	EchoCommonQuery::EchoCommonQuery(EchoCommonQuery::Enum hitType, const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags flags) {
		m_private = new CommonQuery(hitType, geometry, pose, layerMask, filter, flags);
	}
	EchoCommonQuery::~EchoCommonQuery() {
		SAFE_DELETE(m_private);
	}
	void EchoCommonQuery::Execute(EchoPhysicsQuery* pQuery) {
		m_private->Execute(pQuery);
	}
	bool EchoCommonQuery::GetResult() {
		return m_private->GetResult();
	}
	const std::vector<EchoRaycastHit>& EchoCommonQuery::GetResults() {
		return m_private->GetResults();
	}

	class PhysXQueryTask : public PhysXTask {
	public:
		PhysXQueryTask(EchoPhysicsQuery* _Query) : mQuery(_Query) {}
		virtual ~PhysXQueryTask() {}
	public:
		virtual void TaskExecute() {
			if (mQuery == nullptr) return;
			for (CommonQuery* pCommand : mCommands) {
				pCommand->Execute(mQuery);
			}
		}
		virtual void TaskFinish() {}
		int PushBack(CommonQuery* pCommand) {
			if (pCommand == nullptr || IsPrepare()) return -1;
			int index = (int)mCommands.size();
			mCommands.push_back(pCommand);
			return index;
		}
	protected:
		std::vector<CommonQuery*> mCommands;
		EchoPhysicsQuery* mQuery = nullptr;
	};

	EchoQueryTask::EchoQueryTask(EchoPhysicsQuery* pQuery, const std::vector<EchoCommonQuery*>& commands) {
		m_private = new PhysXQueryTask(pQuery);
		for (EchoCommonQuery* command : commands) {
			m_private->PushBack(command->m_private);
		}
	}
	EchoQueryTask::~EchoQueryTask() {
		SAFE_DELETE(m_private);
	}
	void EchoQueryTask::Execute() {
		m_private->PrepareTask();
	}

	void EchoQueryTask::Wait() {
		m_private->SynchroTask();
	}

	class EchoBatchQueryPrivate {
	public:
		EchoBatchQueryPrivate(EchoPhysicsQuery* pQuery) {
			for (int i = 0; i != 4; ++i) {
				mTasks[i] = new PhysXQueryTask(pQuery);
			}
		}
		virtual ~EchoBatchQueryPrivate() {
			for (int i = 0; i != 4; ++i) {
				SAFE_DELETE(mTasks[i]);
			}
			Clear();
		}
		void PrepareTask() {
			mStopPushBack = true;
			for (int i = 0; i != 4; ++i) {
				mTasks[i]->PrepareTask();
			}
		}
		void SynchroTask() {
			for (int i = 0; i != 4; ++i) {
				mTasks[i]->SynchroTask();
			}
			Swap();
			mStopPushBack = false;
		}
		void CancelTask() {
			for (int i = 0; i != 4; ++i) {
				mTasks[i]->CancelTask();
			}
		}
		int PushBack(CommonQuery* pCommand) {
			if (pCommand == nullptr) return -1;
			int index = (int)mCommands.size();
			mCommands.push_back(pCommand);
			mTasks[index % 4]->PushBack(pCommand);
			return index;
		}
		void Clear() {
			for (CommonQuery* pCommand : mCommands) {
				delete pCommand;
			}
			mCommands.clear();
			for (CommonQuery* pCommand : mResults) {
				delete pCommand;
			}
			mResults.clear();
		}
		void Swap() {
			for (CommonQuery* pCommand : mResults) {
				delete pCommand;
			}
			mResults.clear();
			mCommands.swap(mResults);
		}
		int GetResultNumber() {
			return (int)mResults.size();
		}
		bool GetResult(int index) {
			if (index < 0 || index >= mResults.size()) return false;
			return mResults[index]->GetResult();
		}
		const std::vector<EchoRaycastHit>& GetResults(int index) {
			if (index < 0 || index >= mResults.size()) return mTempHit;
			return mResults[index]->GetResults();
		}
	public:
		bool mStopPushBack = false;
		PhysXQueryTask* mTasks[4] = { 0,0,0,0 };
		std::vector<CommonQuery*> mCommands;
		std::vector<CommonQuery*> mResults;
		std::vector<EchoRaycastHit> mTempHit;
	};

	EchoBatchQuery::EchoBatchQuery(EchoPhysicsQuery* pQuery) {
		m_private = new EchoBatchQueryPrivate(pQuery);
	}
	EchoBatchQuery::~EchoBatchQuery() {
		SAFE_DELETE(m_private);
	}

	int EchoBatchQuery::Raycast(EchoCommonQuery::Enum type, const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (m_private->mStopPushBack) return -1;
		return m_private->PushBack(new CommonQuery(type, origin, direction, maxDistance, layerMask, filter, _flag));
	}
	int EchoBatchQuery::GeometryCast(EchoCommonQuery::Enum type, const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry || m_private->mStopPushBack) return -1;
		return m_private->PushBack(new CommonQuery(type, geometry, pose, direction, maxDistance, layerMask, filter, _flag));
	}
	int EchoBatchQuery::OverlapGeometry(EchoCommonQuery::Enum type, const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter, EchoSceneQueryFlags _flag) {
		if (!geometry || m_private->mStopPushBack) return -1;
		return m_private->PushBack(new CommonQuery(type, geometry, pose, layerMask, filter, _flag));
	}
	
	void EchoBatchQuery::Execute() {
		m_private->PrepareTask();
	}
	void EchoBatchQuery::Wait() {
		m_private->SynchroTask();
	}
	int EchoBatchQuery::GetResultNumber() {
		return m_private->GetResultNumber();
	}
	bool EchoBatchQuery::GetResult(int index) {
		return m_private->GetResult(index);
	}
	const std::vector<EchoRaycastHit>& EchoBatchQuery::GetResults(int index) {
		return m_private->GetResults(index);
	}
}

namespace Echo {

	PhysicsBaseComponent::PhysicsBaseComponent(ActorPtr pActor, ActorCOMMoveableType type) : ActorComponent(pActor, type)
	{
		setActorComActiveType(ActorComActiveType::PHYSICS_BIT);
	}
	PhysicsBaseComponent::~PhysicsBaseComponent() {
	
	}
	void PhysicsBaseComponent::connectCom() {
		if (GetPhysicsManager()->PhysicsIsRunning()) AddToPhysicsUpdateQueue();
	}
	void PhysicsBaseComponent::disconnectCom() {}
	void PhysicsBaseComponent::init(ActorComponentInfo* info) {
		m_pPhysicsManager = EchoPhysicsSystem::instance()->GetPhysicsManager(this);
		GetPhysicsManager()->AddPhysicsComponent(this);
	}
	void PhysicsBaseComponent::unInit() {
		RemoveFromPhysicsUpdateQueue();
		RemoveFromEditorUpdateQueue();
		GetPhysicsManager()->RemovePhysicsComponent(this);
		m_pPhysicsManager = nullptr;
	}
	void PhysicsBaseComponent::visibleStateChanged(bool activeState) {
		if (!GetPhysicsManager()) return;
		if (!activeState) {
			ReleasePhysics();
		}
		if (activeState) {
			AddToPhysicsUpdateQueue();
		}
		else {
			RemoveFromPhysicsUpdateQueue();
		}
	}

	void PhysicsBaseComponent::AddToPhysicsUpdateQueue() {
		GetPhysicsManager()->AddToPhysicsUpdateQueue(this);
	}
	void PhysicsBaseComponent::RemoveFromPhysicsUpdateQueue() {
		GetPhysicsManager()->RemoveFromPhysicsUpdateQueue(this);
	}
	void PhysicsBaseComponent::AddToEditorUpdateQueue() {
#ifdef USE_PHYSICS_HELPEROBJ
		GetPhysicsManager()->AddToEditorUpdateQueue(this);
#endif // USE_PHYSICS_HELPEROBJ
	}
	void PhysicsBaseComponent::RemoveFromEditorUpdateQueue() {
#ifdef USE_PHYSICS_HELPEROBJ
		GetPhysicsManager()->RemoveFromEditorUpdateQueue(this);
#endif // USE_PHYSICS_HELPEROBJ
	}
	EchoPhysicsManager* PhysicsBaseComponent::GetPhysicsManager() {
		if (!m_pPhysicsManager) {
			//if (m_bLoad) {
			//	LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- The Expired physical component is called.");
			//}
			m_pPhysicsManager = EchoPhysicsSystem::instance()->GetPhysicsManager(this);
		}
		return m_pPhysicsManager;
	}

	ActorComponentPtr PhysicsBaseComponent::FindComponent(const String& compName) {
		if (compName.empty()) return nullptr;
		if (getActor()->IsRelativeName(compName)) {
			PickActorResult res = getActor()->getActorComByRelativeName(compName);
			return res.seleteCom;
		}
		ActorComponentPtr compPtr = getActor()->getComponent(compName);
		if (compPtr != nullptr) {
			return compPtr;
		}
		PickActorResult res = getActor()->getActorManager()->GetActorComBySeletedName(compName);
		return res.seleteCom;
	}

	void PhysicsActorComponent::AttachSelfLink() {
		PhysicsLinkComponentPtrList* m_LinkCompList = nullptr;
		GetPhysicsManager()->GetSelfLinkCompList(this, m_LinkCompList);
		if (m_LinkCompList != nullptr && !m_LinkCompList->empty()) {
			for (auto& pJoint : *m_LinkCompList) {
				if (pJoint != nullptr) {
					pJoint->_actorNotifyPhysX(this, PhysXNotify_Create);
				}
			}
		}
		if (mAggregatePtr) {
			mAggregatePtr->_actorNotifyPhysX(this, PhysXNotify_Create);
		}
	}
	void PhysicsActorComponent::DetachSelfLink() {
		PhysicsLinkComponentPtrList* m_LinkCompList = nullptr;
		GetPhysicsManager()->GetSelfLinkCompList(this, m_LinkCompList);
		if (m_LinkCompList != nullptr && !m_LinkCompList->empty()) {
			for (auto& pJoint : *m_LinkCompList) {
				if (pJoint != nullptr) {
					pJoint->_actorNotifyPhysX(this, PhysXNotify_Destroy);
				}
			}
		}
		if (mAggregatePtr) {
			mAggregatePtr->_actorNotifyPhysX(this, PhysXNotify_Destroy);
		}
	}
	void PhysicsActorComponent::RemoveSelfLink() {
		PhysicsLinkComponentPtrList* m_LinkCompList = nullptr;
		GetPhysicsManager()->GetSelfLinkCompList(this, m_LinkCompList);
		if (m_LinkCompList != nullptr && !m_LinkCompList->empty()) {
			std::vector<PhysicsLinkComponentPtr> tempVec(m_LinkCompList->begin(), m_LinkCompList->end());
			for (auto& pJoint : tempVec) {
				if (pJoint != nullptr) {
					pJoint->_actorNotifyPhysX(this, PhysXNotify_Break);
				}
			}
			tempVec.clear();
			GetPhysicsManager()->ClearSelfLinkCompList(this);
		}
		if (mAggregatePtr) {
			mAggregatePtr->_actorNotifyPhysX(this, PhysXNotify_Break);
		}
	}
	PhysicsActorComponent::~PhysicsActorComponent() {
		if (mPhysXObject != nullptr) {
			if (GetPhysicsManager() != nullptr) {
				GetPhysicsManager()->m_pGraph->RemoveObject_Error(this);
				LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- External processing error of physical paging.");
			}
			else {
				if (mPhysXObject->m_pPhysicsPage) {
					mPhysXObject->m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->m_pGraph->RemoveObject_Error(this);
				}
				else {
					SAFE_DELETE(mPhysXObject);
				}
				LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- Internal processing error of physical paging.");
			}
		}
	}
	void PhysicsActorComponent::GetJointCompList(std::vector<JointComponent*>& joints) {
		PhysicsLinkComponentPtrList* m_LinkCompList = nullptr;
		GetPhysicsManager()->GetSelfLinkCompList(this, m_LinkCompList);
		if (m_LinkCompList == nullptr || m_LinkCompList->empty()) {
			return;
		}
		for (auto& iter : *m_LinkCompList) {
			if (iter->GetPhysicsType() == EchoPhysicsComponentType::eJOINT) {
				joints.push_back((JointComponent*)iter.getPointer());
			}
		}
	}

	AggregateComponentPtr PhysicsActorComponent::GetAggregate() {
		return mAggregatePtr;
	}

	void PhysicsActorComponent::_removePhysX(PhysicsLinkComponentPtr& pComp) {
		if (!pComp) return;
		if (pComp->GetPhysicsType() == EchoPhysicsComponentType::eAGGREGATE) {
			if (mAggregatePtr == pComp) {
				mAggregatePtr = nullptr;
			}
		}
		else {
			PhysicsLinkComponentPtrList& m_LinkCompList = GetPhysicsManager()->GetSelfLinkCompList(this);
			auto iter = std::find(m_LinkCompList.begin(), m_LinkCompList.end(), pComp);
			if (iter != m_LinkCompList.end()) {
				m_LinkCompList.erase(iter);
				if (m_LinkCompList.empty()) {
					GetPhysicsManager()->ClearSelfLinkCompList(this);
				}
			}
		}
	}
	void PhysicsActorComponent::_addPhysX(PhysicsLinkComponentPtr& pComp) {
		if (!pComp) return;
		if (pComp->GetPhysicsType() == EchoPhysicsComponentType::eAGGREGATE) {
			mAggregatePtr = pComp;
		}
		else {
			PhysicsLinkComponentPtrList& m_LinkCompList = GetPhysicsManager()->GetSelfLinkCompList(this);
			m_LinkCompList.push_back(pComp);
		}
	}

	DVector3 EchoContactSet::getPoint(uint32 i) const { return m_pContactSet->getPoint(i); }
	void EchoContactSet::setPoint(uint32 i, const DVector3& p) const { m_pContactSet->setPoint(i, p); }
	Vector3 EchoContactSet::getNormal(uint32 i) const { return bInver ? -m_pContactSet->getNormal(i) : m_pContactSet->getNormal(i); }
	void EchoContactSet::setNormal(uint32 i, const Vector3& n) const { m_pContactSet->setNormal(i, bInver ? -n : n); }
	float EchoContactSet::getSeparation(uint32 i) const { return m_pContactSet->getSeparation(i); }
	void EchoContactSet::setSeparation(uint32 i, float s) const { m_pContactSet->setSeparation(i, s); }
	Vector3 EchoContactSet::getTargetVelocity(uint32 i) const { return bInver ? -m_pContactSet->getTargetVelocity(i) : m_pContactSet->getTargetVelocity(i); }
	void EchoContactSet::setTargetVelocity(uint32 i, const Vector3& v) const { m_pContactSet->setTargetVelocity(i, bInver ? -v : v); }
	float EchoContactSet::getMaxImpulse(uint32 i) const { return m_pContactSet->getMaxImpulse(i); }
	void EchoContactSet::setMaxImpulse(uint32 i, float s) const { m_pContactSet->setMaxImpulse(i, s); }
	float EchoContactSet::getRestitution(uint32 i) const { return m_pContactSet->getRestitution(i); }
	void EchoContactSet::setRestitution(uint32 i, float r) const { m_pContactSet->setRestitution(i, r); }
	float EchoContactSet::getStaticFriction(uint32 i) const { return m_pContactSet->getStaticFriction(i); }
	void EchoContactSet::setStaticFriction(uint32 i, float f) const { m_pContactSet->setStaticFriction(i, f); }
	float EchoContactSet::getDynamicFriction(uint32 i) const { return m_pContactSet->getDynamicFriction(i); }
	void EchoContactSet::setDynamicFriction(uint32 i, float f) const { m_pContactSet->setDynamicFriction(i, f); }
	void EchoContactSet::ignore(uint32 i) const { m_pContactSet->ignore(i); }
	uint32 EchoContactSet::size() const { return m_pContactSet->size(); }

	class EchoPhysicsSceneCallBack : public PhysXSceneCallBack
	{
	public:
		enum CollisionEventType {
			eEnter,
			eStay,
			eExit,
		};
		struct EchoTriggerPairParam {
			RigidBodyComponent* rigidbody0 = nullptr;
			ColliderComponent* collider0 = nullptr;
			RigidBodyComponent* rigidbody1 = nullptr;
			ColliderComponent* collider1 = nullptr;
		};
		struct EchoCollisionPairParam {

			RigidBodyComponent* rigidbody0 = nullptr;
			ColliderComponent* collider0 = nullptr;
			RigidBodyComponent* rigidbody1 = nullptr;
			ColliderComponent* collider1 = nullptr;
			std::vector<EchoContactPointParam> points;
		};
		struct EchoContactModifyPairParam {
			RigidBodyComponent* actor[2] = { nullptr,nullptr };
			ColliderComponent* shape[2] = { nullptr,nullptr };
		};
	public:
		EchoPhysicsSceneCallBack() {}
		virtual ~EchoPhysicsSceneCallBack() {}

		virtual void OnContactEnter(ContactParam& contact) override {
			EchoCollisionPairParam param;
			if (extractCollisionParam(contact, param)) {
				mCurrentColliderCollisionQueue.push_back(std::make_pair(eEnter, param));
			}
		}

		virtual void OnContactStay(ContactParam& contact) override {
			EchoCollisionPairParam param;
			if (extractCollisionParam(contact, param)) {
				mCurrentColliderCollisionQueue.push_back(std::make_pair(eStay, param));
			}
		}

		virtual void OnContactExit(ContactParam& contact) override {
			EchoCollisionPairParam param;
			if (extractCollisionParam(contact, param)) {
				mCurrentColliderCollisionQueue.push_back(std::make_pair(eExit, param));
			}
		}

		virtual void OnTriggerEnter(TriggerParam& contact) override {
			EchoTriggerPairParam param;
			if (extractTriggerParam(contact, param)) {
				mCurrentColliderTriggerQueue.push_back(std::make_pair(eEnter, param));
			}
		}

		virtual void OnTriggerExit(TriggerParam& contact) override {
			EchoTriggerPairParam param;
			if (extractTriggerParam(contact, param)) {
				mCurrentColliderTriggerQueue.push_back(std::make_pair(eExit, param));
			}

		}

		virtual void OnTriggerStay(TriggerParam& contact) override {
			EchoTriggerPairParam param;
			if (extractTriggerParam(contact, param)) {
				mCurrentColliderTriggerQueue.push_back(std::make_pair(eStay, param));
			}
		}

		virtual void OnJointBreak(PhysXJoint* pJoint) override {
			if (!pJoint) return;
			if (pJoint->GetUserType() != PhysXUserTypeEnum::ActorComponent) return;
			JointComponent* pComp = (JointComponent*)pJoint->GetUserData();
			if (!pComp) return;
			mCurrentJointBreakQueue.push_back(pComp);
		}

		virtual void OnRigidBodyMove(const std::vector<RigidBodyTransform>& rigids) override {
			if (rigids.empty()) return;
			mCurrentMoveQueue.reserve(rigids.size());
			for (auto& rigid : rigids) {
				if (rigid.pActor->GetUserType() != PhysXUserTypeEnum::ActorComponent) {
					continue;
				}
				RigidBodyComponent* pComp = (RigidBodyComponent*)rigid.pActor->GetUserData();
				mCurrentMoveQueue.push_back(std::make_pair(pComp, rigid.trans));
			}
		}

		virtual void OnRigidBodyWake(const std::vector<PhysXRigidDynamic*>& rigids) override {
			if (rigids.empty()) return;
			mCurrentWakeQueue.reserve(rigids.size());
			for (auto& rigid : rigids) {
				if (rigid->GetUserType() != PhysXUserTypeEnum::ActorComponent) {
					continue;
				}
				RigidBodyComponent* pComp = (RigidBodyComponent*)rigid->GetUserData();
				if (pComp == nullptr) continue;
				mCurrentWakeQueue.push_back(pComp);
			}
		}

		virtual void OnRigidBodySleep(const std::vector<PhysXRigidDynamic*>& rigids) override {
			if (rigids.empty()) return;
			mCurrentSleepQueue.reserve(rigids.size());
			for (auto& rigid : rigids) {
				if (rigid->GetUserType() != PhysXUserTypeEnum::ActorComponent) {
					continue;
				}
				RigidBodyComponent* pComp = (RigidBodyComponent*)rigid->GetUserData();
				if (pComp == nullptr) continue;
				mCurrentSleepQueue.push_back(pComp);
			}
		}

		virtual void OnSubstep(float dt) override {}

		virtual void OnContactModify(const std::vector<ContactModifyPair>& pairs) override {
			std::unordered_map<RigidBodyComponent*, std::vector<EchoContactModifyPair>> pairMap;
			exportContactModifyPairMap(pairs, pairMap);
			for (const auto& iter : pairMap) {
				iter.first->SendContactModify(iter.second);
			}
		};

		virtual void OnCCDContactModify(const std::vector<ContactModifyPair>& pairs) override {
			std::unordered_map<RigidBodyComponent*, std::vector<EchoContactModifyPair>> pairMap;
			exportContactModifyPairMap(pairs, pairMap);
			for (const auto& iter : pairMap) {
				iter.first->SendCCDContactModify(iter.second);
			}
		};
	public:
		//int UpdateRigidBodyMove() {
		//	int size = (int)mCurrentMoveQueue.size();
		//	std::multimap<uint32, std::pair<RigidBodyComponent*, DTransform>, std::greater<uint32>> sortRigids;
		//	for (auto& iter : mCurrentMoveQueue) {
		//		sortRigids.insert(std::make_pair(iter.first->GetRigidBodyTreeLevel(), iter));
		//	}
		//	for (auto& iter : sortRigids) {
		//		iter.second.first->SetWorldTransformFromPhysX(iter.second.second);
		//	}
		//	mCurrentMoveQueue.clear();
		//	return size;
		//}
		int UpdateRigidBodyMove() {
			int size = (int)mCurrentMoveQueue.size();
			for (auto& iter : mCurrentMoveQueue) {
				const auto& pParentNode = iter.first->getParentNodeCom()->getParentNodeCom();
				if (pParentNode) {
					iter.second.translation = pParentNode->convertWorldToLocalPosition(iter.second.translation);
					iter.second.rotation = pParentNode->convertWorldToLocalRotation(iter.second.rotation);
				}
			}
			for (auto& iter : mCurrentMoveQueue) {
				iter.first->SetLocalTransformFromPhysX(iter.second);
			}
			mCurrentMoveQueue.clear();
			return size;
		}
		int UpdateRigidBodyWake() {
			int size = (int)mCurrentWakeQueue.size();
			for (auto& iter : mCurrentWakeQueue) {
				iter->SendWakeEvent();
			}
			mCurrentWakeQueue.clear();
			return size;
		}
		int UpdateRigidBodySleep() {
			int size = (int)mCurrentSleepQueue.size();
			for (auto& iter : mCurrentSleepQueue) {
				iter->SendSleepEvent();
			}
			mCurrentSleepQueue.clear();
			return size;
		}
		int UpdateColliderCollision() {
			int size = (int)mCurrentColliderCollisionQueue.size();
			for (auto& iter : mCurrentColliderCollisionQueue) {
				EchoCollisionParam cp0, cp1;
				AssignParamPair(cp0, cp1, iter.second);
				switch (iter.first) {
				case eEnter: {
					cp0.self_collider->SendCollisionEnter(cp0);
					cp1.self_collider->SendCollisionEnter(cp1);
					break;
				}
				case eStay: {
					cp0.self_collider->SendCollisionStay(cp0);
					cp1.self_collider->SendCollisionStay(cp1);
					break;
				}
				case eExit: {
					cp0.self_collider->SendCollisionExit(cp0);
					cp1.self_collider->SendCollisionExit(cp1);
					break;
				}
				default: {
					break;
				}
				}
			}
			mCurrentColliderCollisionQueue.clear();
			return size;
		}
		int UpdateColliderTrigger() {
			int size = (int)mCurrentColliderTriggerQueue.size();
			for (auto& iter : mCurrentColliderTriggerQueue) {
				EchoTriggerParam cp0, cp1;
				AssignParamPair(cp0, cp1, iter.second);
				switch (iter.first) {
				case eEnter: {
					cp0.self_collider->SendTriggerEnter(cp0);
					cp1.self_collider->SendTriggerEnter(cp1);
					break;
				}
				case eStay: {
					cp0.self_collider->SendTriggerStay(cp0);
					cp1.self_collider->SendTriggerStay(cp1);
					break;
				}
				case eExit: {
					cp0.self_collider->SendTriggerExit(cp0);
					cp1.self_collider->SendTriggerExit(cp1);
					break;
				}
				default: {
					break;
				}
				}
			}
			mCurrentColliderTriggerQueue.clear();
			return size;
		}
		void UpdateJointEvent() {
			if (!mCurrentJointBreakQueue.empty()) {
				for (auto& iter : mCurrentJointBreakQueue) {
					iter->SendJointBreak();
				}
				mCurrentJointBreakQueue.clear();
			}
		}
	public:
		void AssignParamPair(EchoCollisionParam& cp0, EchoCollisionParam& cp1, EchoCollisionPairParam& param) {
			cp0.self_rigidbody = param.rigidbody0;
			cp0.self_collider = param.collider0;
			cp0.rigidbody = param.rigidbody1;
			cp0.collider = param.collider1;

			cp1.self_rigidbody = param.rigidbody1;
			cp1.self_collider = param.collider1;
			cp1.rigidbody = param.rigidbody0;
			cp1.collider = param.collider0;

			size_t size = param.points.size();
			if (size == 0) return;
			if (cp0.self_collider->ExistColliderCallBack()) {
				cp0.points.resize(size);
				memcpy(cp0.points.data(), param.points.data(), size * sizeof(EchoContactPointParam));
			}
			if (cp1.self_collider->ExistColliderCallBack()) {
				cp1.points.resize(size);
				memcpy(cp1.points.data(), param.points.data(), size * sizeof(EchoContactPointParam));
				for (EchoContactPointParam& point : cp1.points) {
					point.normal.Neg();
					point.impulse.Neg();
				}
			}
		}
		void AssignParamPair(EchoTriggerParam& cp0, EchoTriggerParam& cp1, EchoTriggerPairParam& param) {
			cp0.self_rigidbody = param.rigidbody0;
			cp0.self_collider = param.collider0;
			cp0.rigidbody = param.rigidbody1;
			cp0.collider = param.collider1;
			cp1.self_rigidbody = param.rigidbody1;
			cp1.self_collider = param.collider1;
			cp1.rigidbody = param.rigidbody0;
			cp1.collider = param.collider0;
		}
		bool extractCollisionParam(ContactParam& contact, EchoCollisionPairParam& param) {
			if (!EchoPhysicsManager::PhysXPtrToEchoPtr(contact.a0, contact.s0, param.rigidbody0, param.collider0)) return false;
			if (!EchoPhysicsManager::PhysXPtrToEchoPtr(contact.a1, contact.s1, param.rigidbody1, param.collider1)) return false;
			copyContactPointParam(contact.points, param.points);
			return true;
		}
		bool extractTriggerParam(TriggerParam& contact, EchoTriggerPairParam& param) {
			if (!EchoPhysicsManager::PhysXPtrToEchoPtr(contact.triggerActor, contact.triggerShape, param.rigidbody0, param.collider0)) return false;
			if (!EchoPhysicsManager::PhysXPtrToEchoPtr(contact.otherActor, contact.otherShape, param.rigidbody1, param.collider1)) return false;
			return true;
		}
		void copyContactPointParam(std::vector<EchoContactPoint>& contactPoint, std::vector<EchoContactPointParam>& param) {
			size_t size = contactPoint.size();
			param.resize(size);
			memcpy(param.data(), contactPoint.data(), size * sizeof(EchoContactPointParam));
		}
		void exportContactModifyPairMap(const std::vector<ContactModifyPair>& pairs, std::unordered_map<RigidBodyComponent*, std::vector<EchoContactModifyPair>>& pairMap) {
			int size = (int)pairs.size();
			for (int i = 0; i != size; ++i) {
				const ContactModifyPair& pair = pairs[i];
				EchoContactModifyPairParam pairParam;
				EchoPhysicsManager::PhysXPtrToEchoPtr(pair.actor[0], pair.shape[0], pairParam.actor[0], pairParam.shape[0]);
				EchoPhysicsManager::PhysXPtrToEchoPtr(pair.actor[1], pair.shape[1], pairParam.actor[1], pairParam.shape[1]);
				if (pairParam.actor[0] && pairParam.actor[0]->ExistRigidBodyCallBack()) {
					auto& ePair = pairMap[pairParam.actor[0]].emplace_back();
					ePair.rigidbody[0] = pairParam.actor[0];
					ePair.rigidbody[1] = pairParam.actor[1];
					ePair.collider[0] = pairParam.shape[0];
					ePair.collider[1] = pairParam.shape[1];
					ePair.transform[0] = pair.transform[0];
					ePair.transform[1] = pair.transform[1];
					ePair.contacts.bInver = false;
					ePair.contacts.m_pContactSet = (PhysXContactSet*)(&pair.contacts);
				}
				if (pairParam.actor[1] && pairParam.actor[1]->ExistRigidBodyCallBack()) {
					auto& ePair = pairMap[pairParam.actor[1]].emplace_back();
					ePair.rigidbody[0] = pairParam.actor[1];
					ePair.rigidbody[1] = pairParam.actor[0];
					ePair.collider[0] = pairParam.shape[1];
					ePair.collider[1] = pairParam.shape[0];
					ePair.transform[0] = pair.transform[1];
					ePair.transform[1] = pair.transform[0];
					ePair.contacts.bInver = true;
					ePair.contacts.m_pContactSet = (PhysXContactSet*)(&pair.contacts);
				}
			}
		}
	public:
		std::vector<std::pair<RigidBodyComponent*, DTransform>> mCurrentMoveQueue;
		std::vector<RigidBodyComponent*> mCurrentWakeQueue;
		std::vector<RigidBodyComponent*> mCurrentSleepQueue;
		//std::map<RigidBodyComponent*, DTransform> mCurrentPostMoveQueue;
		std::vector<std::pair<CollisionEventType, EchoCollisionPairParam>> mCurrentColliderCollisionQueue;
		std::vector<std::pair<CollisionEventType, EchoTriggerPairParam>> mCurrentColliderTriggerQueue;
		std::vector<JointComponent*> mCurrentJointBreakQueue;
	};

	class EchoPhysicsControllerCallBack : public PhysXControllerCallBack {
	public:
		virtual bool OnShapeHit(const PhysXControllerShapeHit& shapeHit) override {
			CharatorControllerComponent* pComp = (CharatorControllerComponent*)shapeHit.controller->GetUserData();
			EchoControllerShapeHitParam param;
			copyControllerHitParam(shapeHit, param);
			param.controller = GetComponent(shapeHit.controller);
			EchoPhysicsManager::PhysXPtrToEchoPtr(shapeHit.pActor, shapeHit.pShape, param.actor, param.shape);
			if (param.shape) {
				param.shape->SendCollisionCCT(param);
			}
			if (pComp->GetControllerCallBack()) {
				return pComp->GetControllerCallBack()->OnShapeHit(param);
			}
			return false;
		};
		virtual bool OnControllerHit(const PhysXControllersHit& controllerHit) override {
			CharatorControllerComponent* pComp = (CharatorControllerComponent*)controllerHit.controller->GetUserData();
			if (pComp->GetControllerCallBack()) {
				EchoControllersHitParam param;
				copyControllerHitParam(controllerHit, param);
				param.controller = GetComponent(controllerHit.controller);
				param.other = GetComponent(controllerHit.other);
				return pComp->GetControllerCallBack()->OnControllerHit(param);
			}
			return false;
		};

		virtual ControllerBehaviorFlag GetBehaviorFlag(PhysXCharacterController* pSelf, PhysicsActor* pActor, PhysXShape* pShape)  override {
			CharatorControllerComponent* pComp = (CharatorControllerComponent*)pSelf->GetUserData();
			if (pComp->GetControllerCallBack()) {
				EchoControllerShapeBehaviorParam param;
				param.controller = pComp;
				EchoPhysicsManager::PhysXPtrToEchoPtr(pActor, pShape, param.actor, param.shape);
				return (ControllerBehaviorFlag)(uint32)pComp->GetControllerCallBack()->GetBehaviorFlag(param);
			}
			return mCCTBehaviorFlag;
		}

		virtual ControllerBehaviorFlag GetBehaviorFlag(PhysXCharacterController* pSelf, PhysXCharacterController* controller) override {

			CharatorControllerComponent* pComp = (CharatorControllerComponent*)pSelf->GetUserData();
			if (pComp->GetControllerCallBack()) {
				EchoControllersBehaviorParam param;
				param.controller = pComp;
				param.other = GetComponent(controller);
				return (ControllerBehaviorFlag)(uint32)pComp->GetControllerCallBack()->GetBehaviorFlag(param);
			}
			return mCCTBehaviorFlag;

		}
	public:
		static void copyControllerHitParam(const PhysXControllerHit& hit, EchoControllerHitParam& param) {
			param.IsStanding = hit.IsStanding;
			param.motionLen = hit.motionLen;
			param.pos = hit.pos;
			param.normal = hit.normal;
			param.motionDir = hit.motionDir;
		}
		static CharatorControllerComponent* GetComponent(PhysXCharacterController* pController) {
			if (!pController || pController->GetUserType() != PhysXUserTypeEnum::ActorComponent || pController->GetType() != PhysicsType::CharacterController) return nullptr;
			return (CharatorControllerComponent*)pController->GetUserData();
		}
		ControllerBehaviorFlag mCCTBehaviorFlag = ControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE;
	};

	class EchoMathPhysicsControllerCallBack : public PhysXMathControllerCallBack {
	public:
		virtual bool FilterShape(const PhysXMathControllerShape& data) override { 
			MathCharacterController* pComp = (MathCharacterController*)data.controller->GetUserData();
			EchoMathControllerShapeParam param;
			param.controller = pComp;
			EchoPhysicsManager::PhysXPtrToEchoPtr(data.pActor, data.pShape, param.actor, param.shape);
			if (pComp->GetControllerCallBack()) {
				return pComp->GetControllerCallBack()->OnFilterShape(param);
			}
			else {
				return true;
			}
		}

		virtual void OnShapeHit(const PhysXMathControllerShapeHit& shapeHit) override {
			MathCharacterController* pComp = (MathCharacterController*)shapeHit.controller->GetUserData();
			EchoMathControllerShapeHitParam param;
			EchoPhysicsControllerCallBack::copyControllerHitParam(shapeHit, param);
			param.controller = pComp;
			EchoPhysicsManager::PhysXPtrToEchoPtr(shapeHit.pActor, shapeHit.pShape, param.actor, param.shape);
			if (param.shape) {
				param.shape->SendCollisionCCT(param);
			}
			if (pComp->GetControllerCallBack()) {
				pComp->GetControllerCallBack()->OnShapeHit(param);
			}
		};
		virtual ControllerBehaviorFlag GetBehaviorFlag(PhysXMathCharacterController* pSelf, PhysicsActor* pActor, PhysXShape* pShape) override { 

			MathCharacterController* pComp = (MathCharacterController*)pSelf->GetUserData();
			if (pComp->GetControllerCallBack()) {
				EchoMathControllerShapeBehaviorParam param;
				param.controller = pComp;
				EchoPhysicsManager::PhysXPtrToEchoPtr(pActor, pShape, param.actor, param.shape);
				return (ControllerBehaviorFlag)(uint32)pComp->GetControllerCallBack()->GetBehaviorFlag(param);
			}
			return ControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE;
		}

	public:
	};

	class EchoPhysXWorldSystemCallBack_EchoPhysicsSystem : public EchoPhysXWorldSystemCallBack {
	public:
		virtual void OnCreateScene(const Name& name) override {
			//EchoPhysicsSystem::instance()->CreateManager(name);
		}
		virtual void OnReleaseScene(const Name& name) override {
			//EchoPhysicsSystem::instance()->DestroyManager(name);
		}
		virtual void OnTerrainCreateAll(const Name& name) override {
			EchoPhysicsManager* pMgr = EchoPhysicsSystem::instance()->GetManager(name);
			if(pMgr) pMgr->CreateTerrainColliderComponent();
		}
		virtual void OnTerrainReleaseAll(const Name& name) override {
			EchoPhysicsManager* pMgr = EchoPhysicsSystem::instance()->GetManager(name);
			if (pMgr) pMgr->DestroyTerrainColliderComponent();
		}
		virtual void OnTerrainCreate(const Name& name, Index index) override {
			EchoPhysicsManager* pMgr = EchoPhysicsSystem::instance()->GetManager(name);
			if (pMgr) pMgr->AddTerrainColliderComponent(index);
		}
		virtual void OnTerrainRelease(const Name& name, Index index) override {
			EchoPhysicsManager* pMgr = EchoPhysicsSystem::instance()->GetManager(name);
			if (pMgr) pMgr->RemoveTerrainColliderComponent(index);
		}
	};

}

namespace Echo{

	Echo::Name EchoPhysicsSystem::mType("EchoPhysicsSystem");
	const std::set<Name> EchoPhysicsManager::gColliderTypes = {
		COMPONENTTYPE(BoxColliderComponent),
		COMPONENTTYPE(SphereColliderComponent),
		COMPONENTTYPE(CapsuleColliderComponent),
		COMPONENTTYPE(MeshColliderComponent),
		COMPONENTTYPE(MeshColliderUNMOVEABLEComponent)
	};
	const std::vector<Name>	EchoPhysicsManager::gColliderTypesVec = {
		COMPONENTTYPE(BoxColliderComponent),
		COMPONENTTYPE(SphereColliderComponent),
		COMPONENTTYPE(CapsuleColliderComponent),
		COMPONENTTYPE(MeshColliderComponent),
		COMPONENTTYPE(MeshColliderUNMOVEABLEComponent)
	};

	bool EchoPhysicsManager::PhysXPtrToEchoPtr(PhysicsActor* actor, PhysXShape* shape, RigidBodyComponent*& rigidbody, ColliderComponent*& collider) {
		if (actor == nullptr || shape == nullptr) {
			rigidbody = nullptr;
			collider = nullptr;
			return false;
		}
		if (actor->GetUserType() == PhysXUserTypeEnum::WaitDelete || shape->GetUserType() == PhysXUserTypeEnum::WaitDelete) {
			rigidbody = nullptr;
			collider = nullptr;
			return false;
		}

		if (actor->GetUserType() == PhysXUserTypeEnum::ActorComponent) {
			if (actor->GetType() == PhysicsType::RigidDynamic) {
				if (shape->GetUserType() == PhysXUserTypeEnum::ActorComponent) {
					rigidbody = (RigidBodyComponent*)actor->GetUserData();
					collider = (ColliderComponent*)shape->GetUserData();
				}
				else {
					rigidbody = nullptr;
					collider = nullptr;
					return false;
				}
			}
			else if (actor->GetType() == PhysicsType::CharacterController) {
				rigidbody = nullptr;
				collider = (CharatorControllerComponent*)actor->GetUserData();
			}
			else {
				rigidbody = nullptr;
				collider = nullptr;
				return false;
			}
		}
		else {
			rigidbody = nullptr;
			if (shape->GetUserType() == PhysXUserTypeEnum::ActorComponent) {
				collider = (ColliderComponent*)shape->GetUserData();
			}
			else {
				collider = nullptr;
				return false;
			}
		}
		return true;
	}

	EchoPhysicsManager* EchoPhysicsSystem::GetPhysicsManager(ActorManager* pActorMgr) {
		if (pActorMgr != nullptr) {
			EchoPhysicsManager* res = GetManager(pActorMgr->getName());
			if (res != nullptr) {
				return res;
			}
		}
		return GetMainPhysicsManager();
	}
	EchoPhysicsManager* EchoPhysicsSystem::GetPhysicsManager(ActorComponent* pComp) {
		if (pComp != nullptr) {
			return GetPhysicsManager(pComp->getActorManager());
		}
		return GetMainPhysicsManager();
	}
	EchoPhysicsManager* EchoPhysicsSystem::GetPhysicsManager(WorldManager* pWorldMgr) {
		if (pWorldMgr != nullptr) {
			EchoPhysicsManager* res = GetManager(pWorldMgr->getName());
			if (res != nullptr) {
				return res;
			}
		}
		return GetMainPhysicsManager();
	}
	EchoPhysicsManager* EchoPhysicsSystem::GetPhysicsManager(SceneManager* pSceneMgr) {
		if (pSceneMgr != nullptr) {
			EchoPhysicsManager* res = GetManager(pSceneMgr->getName());
			if (res != nullptr) {
				return res;
			}
		}
		return GetMainPhysicsManager();
	}

	EchoPhysicsManager* EchoPhysicsSystem::GetActivePhysicsManager() {
		return GetManager(WorldSystem::instance()->GetActiveWorld());
	}

	EchoPhysicsSystem::EchoPhysicsSystem() {
		mPhysXWorldCallBack = new EchoPhysXWorldSystemCallBack_EchoPhysicsSystem();
		PhysXWorldSystem::instance()->m_pCallBack = mPhysXWorldCallBack;

		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("Play")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->EditorPlay();
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("Stop")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->EditorStop();
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("StartSim")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetUpdatePhysics(true);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("StopSim")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetUpdatePhysics(false);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ShowC")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetColliderHelperVisible(true);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("HideC")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetColliderHelperVisible(false);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ShowJ")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetJointHelperVisible(true);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("HideJ")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetJointHelperVisible(false);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ShowT")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetCCTHelperVisible(true);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("HideT")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetCCTHelperVisible(false);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("Memory")] = []() { 
			LogManager::instance()->logMessage("PhysXMemory : " + std::to_string(PhysXSystem::instance()->GetPhysicsMemory()));
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("LoadSync")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetPhysXPageLoadMainActor(0, pActiveMgr->GetPhysXPageLoadMainActor(), false);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("LoadAsync")] = [this]() {
			EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
			if (pActiveMgr) pActiveMgr->SetPhysXPageLoadMainActor(0, pActiveMgr->GetPhysXPageLoadMainActor(), true);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("Memory")] = std::bind<void>(&EchoPhysicsSystem::PrintMemoryS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("ResetCellSize")] = std::bind<void>(&EchoPhysicsSystem::ResetCellSizeS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("SetUsePhysXPage")] = std::bind<void>(&EchoPhysicsSystem::SetUsePhysXPageS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("PhysXPageLoadActorComponent")] = std::bind<void>(&EchoPhysicsSystem::SetPhysXPageLoadMainActorS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("AddPhysXPageLoadActorComponent")] = std::bind<void>(&EchoPhysicsSystem::AddPhysXPageLoadMainActorS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("DebugLines")] = std::bind<void>(&EchoPhysicsSystem::SetPhysXDebugLinesS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("dls")] = std::bind<void>(&EchoPhysicsSystem::SetPhysXDebugLinesS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("LogData")] = std::bind<void>(&EchoPhysicsSystem::ShowPhysicsData, this);

		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ClearDynamic")] = std::bind<void>(&EchoPhysicsSystem::ClearShootCollider, this);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CreateDynamic")] = std::bind<void>(&EchoPhysicsSystem::ShootDynamicSphereColliderS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("DynamicSpeed")] = std::bind<void>(&EchoPhysicsSystem::SetSpeedS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("DynamicMode")] = std::bind<void>(&EchoPhysicsSystem::SetCollisionDetectionModeS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("DynamicRandom")] = std::bind<void>(&EchoPhysicsSystem::SetRandomModeS, this, std::placeholders::_1);
		

		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CreateCCT")] = std::bind<void>(&EchoPhysicsSystem::CreateAutoMoveCCTS, this, std::placeholders::_1);
		//PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ClearCCT")] = std::bind<void>(&EchoPhysicsSystem::ClearAutoMoveCCT, this);
		//PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("CCTLogOpen")] = [&]() { mShowCCTTestTime = true; };
		//PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("CCTLogClose")] = [&]() { mShowCCTTestTime = false; };
		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CCTGravityMode")] = std::bind<void>(&EchoPhysicsSystem::SetGravityModeS, this, std::placeholders::_1);
		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CCTOrigin")] = std::bind<void>(&EchoPhysicsSystem::SetCreateCCTOriginS, this, std::placeholders::_1);
		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CCTGravitySource")] = std::bind<void>(&EchoPhysicsSystem::SetGravitySourceS, this, std::placeholders::_1);
		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CCTMaxSpeed")] = std::bind<void>(&EchoPhysicsSystem::SetCreateCCTMaxSpeedS, this, std::placeholders::_1);
		//PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CCTGravityConstant")] = std::bind<void>(&EchoPhysicsSystem::SetCreateCCTGravityConstantS, this, std::placeholders::_1);

		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticPositionMin")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticPositionMinS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticPositionMax")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticPositionMaxS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticScale")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticRandomScaleS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticOrigin")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticOriginS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticRadius")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticRadiusS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("StaticMode")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticModeS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CreateStatic")] = std::bind<void>(&EchoPhysicsSystem::CreateStaticTestPhysicsS, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ClearStatic")] = std::bind<void>(&EchoPhysicsSystem::ClearStaticTestPhysics, this);
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("CreateStaticParentActor")] = std::bind<void>(&EchoPhysicsSystem::SetCreateStaticParentActorS, this, std::placeholders::_1);

		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("Test")] = std::bind<void>(&EchoPhysicsSystem::TestS, this, std::placeholders::_1);

		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("Help")] = std::bind<void>(&EchoPhysicsSystem::ShowHelp, this);
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("tg")] = [&]() {
			EchoPhysicsManager* pActiveMgr = GetMainManager();
			if (pActiveMgr) pActiveMgr->TestGeometryQueryTime();
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("tmg")] = [&]() {
			EchoPhysicsManager* pActiveMgr = GetMainManager();
			if (pActiveMgr) pActiveMgr->TestMeshGeometryQueryTime();
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ts")] = [&]() {
			EchoPhysicsManager* pActiveMgr = GetMainManager();
			if (pActiveMgr) pActiveMgr->TestSceneQueryTime();
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("tt")] = [&]() {
			EchoPhysicsManager* pActiveMgr = GetMainManager();
			if (pActiveMgr) {
				pActiveMgr->TestGeometryQueryTime();
			}
			std::vector<String> strVec = { "ta","20" };
			TestSceneQueryTime(strVec);
			if (pActiveMgr) {
				pActiveMgr->TestSceneQueryTime();
			}
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("ta")] = [&]() {
			std::vector<String> strVec = { "ta","20" };
			TestSceneQueryTime(strVec);
		};
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap[String("ta")] = std::bind<void>(&EchoPhysicsSystem::TestSceneQueryTime, this, std::placeholders::_1);
		PhysXWorldSystem::instance()->mConsoleCommandFunMap[String("DebugLinesHelp")] = std::bind<void>(&EchoPhysicsSystem::ShowDebugLinesFlagsData, this);
	}
	EchoPhysicsSystem::~EchoPhysicsSystem() {
		PhysXWorldSystem::instance()->m_pCallBack = nullptr;
		PhysXWorldSystem::instance()->mConsoleCommandFunMap.clear();
		PhysXWorldSystem::instance()->mConsoleCommandFunPMap.clear();
		SAFE_DELETE(mPhysXWorldCallBack);
		//DestroyAllPhysicsManager();
	}

	void EchoPhysicsSystem::EditorPlay() {
		if (GetMainPhysicsManager()) GetMainPhysicsManager()->EditorPlay();
	}
	void EchoPhysicsSystem::EditorPause(bool pause) {
		if (GetMainPhysicsManager()) GetMainPhysicsManager()->EditorPause(pause);
	}
	void EchoPhysicsSystem::EditorStop() {
		if (GetMainPhysicsManager()) GetMainPhysicsManager()->EditorStop();
	}

	void EchoPhysicsSystem::SetRunPhysics(bool IsRun) {
		if (GetMainPhysicsManager()) GetMainPhysicsManager()->SetRunPhysics(IsRun);
	}

	void EchoPhysicsSystem::SetUpdatePhysics(bool IsUpdate) {
		if (GetMainPhysicsManager()) GetMainPhysicsManager()->SetUpdatePhysics(IsUpdate);
	}
	bool EchoPhysicsSystem::PhysicsIsRunning() const {
		if (GetMainPhysicsManager()) {
			return GetMainPhysicsManager()->PhysicsIsRunning();
		}
		else {
			return false;
		}
	}
	bool EchoPhysicsSystem::PhysicsIsUpdating() const {
		if (GetMainPhysicsManager()) {
			return GetMainPhysicsManager()->PhysicsIsUpdating();
		}
		else {
			return false;
		}
	}
	void EchoPhysicsSystem::UpdateComponentInEditor() {
		if (GetMainPhysicsManager()) {
			GetMainPhysicsManager()->UpdateComponentInEditor();
		}
	}

	void EchoPhysicsSystem::SetCurrentCharactorController(CharatorControllerComponent* pComp) {
		if (m_pCurrentCharatorControllerComponent) m_pCurrentCharatorControllerComponent->m_IsCurrentController = false;
		m_pCurrentCharatorControllerComponent = pComp;
		if (m_pCurrentCharatorControllerComponent) m_pCurrentCharatorControllerComponent->m_IsCurrentController = true;
	}
	CharatorControllerComponent* EchoPhysicsSystem::GetCurrentCharactorController() {
		return m_pCurrentCharatorControllerComponent;
	}

	void EchoPhysicsManager::convertWorldToLocal(const DTransform& t1, const DTransform& t2, Transform& res) {
		res.translation = t1.rotation.Inverse() * (t2.translation - t1.translation);
		res.rotation = t1.rotation.Inverse() * t2.rotation;
	}
	void EchoPhysicsManager::convertLocalToWorld(const DTransform& t1, const Transform& t2, DTransform& res) {

		res.translation = t1.rotation * t2.translation + t1.translation;
		res.rotation = t1.rotation * t2.rotation;
	}

	void EchoPhysicsSystem::SetAllJointIsUpdateHelper(bool value) {
		for (auto& iter : mMgrMap) {
			iter.second->SetAllJointIsUpdateHelper(value);
		}
	}

	void EchoPhysicsSystem::ResetAllColliderColor() {
		for (auto& iter : mMgrMap) {
			iter.second->ResetAllColliderColor();
		}
	}

	bool EchoPhysicsManager::ExportStaticPhysicsJson(const ActorPtr& actor, cJSON* root) {
		if (actor == nullptr || root == nullptr) {
			return false;
		}

		const ActorComponentPtr& rootComp = actor->getRootNodeCom();
		std::vector<ColliderComponent*> colliders;
		FindColliderComponent(actor, colliders, true);
		if (colliders.empty()) {
			return false;
		}

		cJSON* colliderCollection = cJSON_CreateObject();
		cJSON_AddItemToObject(root, "vecCollider", colliderCollection);
		cJSON* colliderVec_box = nullptr;
		cJSON* colliderVec_sphere = nullptr;
		cJSON* colliderVec_capsule = nullptr;
		cJSON* colliderVec_convexMesh = nullptr;
		cJSON* colliderVec_triangleMesh = nullptr;

		bool bPosition = false, bRotation = false, bScale = false;
		Vector3 rootPosition = rootComp->getWorldPosition();
		Vector3 rootScale = rootComp->getWorldScale();
		Quaternion rootRotation = rootComp->getWorldRotation();

		if (!rootPosition.isEqual(Vector3::ZERO)) {
			bPosition = true;
		}

		if (!rootRotation.isEqual(Quaternion::IDENTITY)) {
			bRotation = true;
		}

		if (!rootScale.isEqual(Vector3::UNIT_SCALE, 0.00001f)) {
			bScale = true;
		}

		for (ColliderComponent* comp : colliders) {

			cJSON* colliderNode = cJSON_CreateObject();

			WriteJSNode(colliderNode, "name", comp->getCombinedName());
			WriteJSNode(colliderNode, "surfaceType", comp->GetSurfaceType());
			if (bPosition) {
				WriteJSNode(colliderNode, "position", Vector3(rootComp->convertWorldToLocalPosition(comp->getWorldPosition())));
			}
			else {
				WriteJSNode(colliderNode, "position", Vector3(comp->getWorldPosition()));
			}

			if (bRotation) {
				WriteJSNode(colliderNode, "rotation", rootComp->convertWorldToLocalRotation(comp->getWorldRotation()));
			}
			else {
				WriteJSNode(colliderNode, "rotation", comp->getWorldRotation());
			}

			if (bScale) {
				WriteJSNode(colliderNode, "scale", Vector3(rootComp->convertWorldToLocalScale(comp->getWorldScale())));
			}
			else {
				WriteJSNode(colliderNode, "scale", comp->getWorldScale());
			}

			const Name& type = comp->getType();
			if (type == gColliderTypesVec[0]) {
				if (colliderVec_box == nullptr) {
					colliderVec_box = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Box", colliderVec_box);
				}
				BoxColliderComponent* collider = (BoxColliderComponent*)comp;
				WriteJSNode(colliderNode, "halfSize", collider->GetHalfSize());
				cJSON_AddItemToArray(colliderVec_box, colliderNode);
			}
			else if (type == gColliderTypesVec[3] || type == gColliderTypesVec[4]) {
				MeshColliderComponent* collider = (MeshColliderComponent*)comp;
				WriteJSNode(colliderNode, "file", collider->GetMeshName());
				if (collider->GetIsConvex()) {
					if (colliderVec_convexMesh == nullptr) {
						colliderVec_convexMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "ConvexMesh", colliderVec_convexMesh);
					}
					cJSON_AddItemToArray(colliderVec_convexMesh, colliderNode);
				}
				else {
					if (colliderVec_triangleMesh == nullptr) {
						colliderVec_triangleMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "TriangleMesh", colliderVec_triangleMesh);
					}
					cJSON_AddItemToArray(colliderVec_triangleMesh, colliderNode);
				}
			}
			else if (type == gColliderTypesVec[1]) {
				if (colliderVec_sphere == nullptr) {
					colliderVec_sphere = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Sphere", colliderVec_sphere);
				}
				SphereColliderComponent* collider = (SphereColliderComponent*)comp;
				WriteJSNode(colliderNode, "radius", collider->GetRadius());
				cJSON_AddItemToArray(colliderVec_sphere, colliderNode);
			}
			else if (type == gColliderTypesVec[2]) {
				if (colliderVec_capsule == nullptr) {
					colliderVec_capsule = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Capsule", colliderVec_capsule);
				}
				CapsuleColliderComponent* collider = (CapsuleColliderComponent*)comp;
				float radius = collider->GetRadius(), halfHeight = collider->GetHeight();
				halfHeight = Math::Max(halfHeight * 0.5f - radius, 0.0001f);
				WriteJSNode(colliderNode, "radius", radius);
				WriteJSNode(colliderNode, "halfHeight", halfHeight);
				cJSON_AddItemToArray(colliderVec_capsule, colliderNode);
			}
		}

		return true;
	}

	struct CompInfoTree {
		ActorComponentInfo* pRootInfo = nullptr;
		WorldNode* pRootNode = nullptr;
		std::map<ActorComponentInfo*, WorldNode*> data;

		CompInfoTree(ActorComponentInfo* _pRootInfo, bool _bUseRoot) : pRootInfo(_pRootInfo) {
			if (_bUseRoot) {
				Quaternion quat;
				quat.FromEuler(pRootInfo->GetEulerAngles());
				pRootNode = new WorldNode(Name(pRootInfo->m_ComName), pRootInfo->GetPosition(), quat, pRootInfo->GetScale());
			}
			else {
				pRootNode = new WorldNode(Name(pRootInfo->m_ComName));
			}
		};
		~CompInfoTree() {
			SAFE_DELETE(pRootNode);

			for (auto& iter : data) {
				SAFE_DELETE(iter.second);
			}
			data.clear();
		};
		WorldNode* Add(ActorComponentInfo* pInfo) {
			if (pInfo == nullptr) {
				return nullptr;
			}
			if (pInfo == pRootInfo) {
				return pRootNode;
			}
			auto iter = data.find(pInfo);
			if (iter == data.end()) {
				ActorComponentInfo* pParentInfo = pInfo->getParentComInfo();
				WorldNode* pParentNode = Add(pParentInfo);
				if (pParentNode == nullptr) {
					return nullptr;
				}
				Quaternion quat;
				quat.FromEuler(pInfo->GetEulerAngles());
				WorldNode* pNode = pParentNode->createChild(Name(pInfo->m_ComName), pInfo->GetPosition(), quat, pInfo->GetScale());
				data.insert(std::make_pair(pInfo, pNode));
				return pNode;
			}
			else {
				return iter->second;
			}
		}
		WorldNode* Get(ActorComponentInfo* pInfo) {
			auto iter = data.find(pInfo);
			if (iter == data.end()) {
				return nullptr;
			}
			return iter->second;
		}
	};

	bool EchoPhysicsManager::ExportStaticPhysicsJson(const ActorInfoPtr& actorInfo, cJSON* root) {
		
		std::vector<ColliderComponentInfo*> colliders;

		std::vector<ActorInfoPtr> childActors;
		childActors.push_back(actorInfo);
		actorInfo->getAllSubActorInfos(childActors);

		ActorComponentInfo* rootNodeInfo = actorInfo->getRootComInfo();
		if (rootNodeInfo == nullptr) {
			return false;
		}
		CompInfoTree mp(rootNodeInfo, false);

		for (auto& childActor : childActors) {
			
			const std::vector<ActorComponentInfo*>& components = childActor->getComponentInfos();
			for (auto& pComp : components) {
				if (pComp == nullptr) {
					continue;
				}
				if (gColliderTypes.find(pComp->m_Type) != gColliderTypes.end()) {
					if (mp.Add(pComp)) {
						colliders.push_back((ColliderComponentInfo*)pComp);
					}
				}
			}

		}

		if (colliders.empty()) {
			return false;
		}

		cJSON* colliderCollection = cJSON_CreateObject();
		cJSON_AddItemToObject(root, "vecCollider", colliderCollection);
		cJSON* colliderVec_box = nullptr;
		cJSON* colliderVec_sphere = nullptr;
		cJSON* colliderVec_capsule = nullptr;
		cJSON* colliderVec_convexMesh = nullptr;
		cJSON* colliderVec_triangleMesh = nullptr;

		for (ColliderComponentInfo* comp : colliders) {

			cJSON* colliderNode = cJSON_CreateObject();

			WorldNode* pNode = mp.Get(comp);

			WriteJSNode(colliderNode, "name", comp->m_ComName);
			WriteJSNode(colliderNode, "surfaceType", comp->GetSurfaceType());
			WriteJSNode(colliderNode, "position", Vector3(pNode->_getDerivedPosition()));
			WriteJSNode(colliderNode, "rotation", pNode->_getDerivedOrientation());
			WriteJSNode(colliderNode, "scale", pNode->_getDerivedScale());

			Name type = comp->m_Type;
			if (type == gColliderTypesVec[0]) {
				if (colliderVec_box == nullptr) {
					colliderVec_box = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Box", colliderVec_box);
				}
				BoxColliderComponentInfo* collider = (BoxColliderComponentInfo*)comp;
				WriteJSNode(colliderNode, "halfSize", collider->GetHalfSize());
				cJSON_AddItemToArray(colliderVec_box, colliderNode);
			}
			else if (type == gColliderTypesVec[3] || type == gColliderTypesVec[4]) {
				MeshColliderComponentInfo* collider = (MeshColliderComponentInfo*)comp;
				WriteJSNode(colliderNode, "file", collider->GetMeshName());
				if (collider->GetIsConvex()) {
					if (colliderVec_convexMesh == nullptr) {
						colliderVec_convexMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "ConvexMesh", colliderVec_convexMesh);
					}
					cJSON_AddItemToArray(colliderVec_convexMesh, colliderNode);
				}
				else {
					if (colliderVec_triangleMesh == nullptr) {
						colliderVec_triangleMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "TriangleMesh", colliderVec_triangleMesh);
					}
					cJSON_AddItemToArray(colliderVec_triangleMesh, colliderNode);
				}
			}
			else if (type == gColliderTypesVec[1]) {
				if (colliderVec_sphere == nullptr) {
					colliderVec_sphere = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Sphere", colliderVec_sphere);
				}
				SphereColliderComponentInfo* collider = (SphereColliderComponentInfo*)comp;
				WriteJSNode(colliderNode, "radius", collider->GetRadius());
				cJSON_AddItemToArray(colliderVec_sphere, colliderNode);
			}
			else if (type == gColliderTypesVec[2]) {
				if (colliderVec_capsule == nullptr) {
					colliderVec_capsule = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Capsule", colliderVec_capsule);
				}
				CapsuleColliderComponentInfo* collider = (CapsuleColliderComponentInfo*)comp;
				float radius = collider->GetRadius(), halfHeight = collider->GetHeight();
				halfHeight = Math::Max(halfHeight * 0.5f - radius, 0.0001f);
				WriteJSNode(colliderNode, "radius", radius);
				WriteJSNode(colliderNode, "halfHeight", halfHeight);
				cJSON_AddItemToArray(colliderVec_capsule, colliderNode);
			}
		}

		return true;
		
		

	}

	bool EchoPhysicsManager::ExportStaticPhysicsJson(const ActorInfoPtr& actorInfo, const Name& actorFilePath) {
		JSNODE actorphy = cJSON_CreateObject();
		if (!ExportStaticPhysicsJson(actorInfo, actorphy)) {
			cJSON_Delete(actorphy);
			return false;
		}
		char* phyout = cJSON_Print(actorphy);
		cJSON_Delete(actorphy);

		if (0 == phyout) {
			return false;
		}

		String phyActorName = actorFilePath.toString() + "phy";
		//路径拼出文件名
		std::replace(phyActorName.begin(), phyActorName.end(), '\\', '/');
		std::replace(phyActorName.begin(), phyActorName.end(), '/', '_');

		phyActorName.insert(0, "collision/actor/");
		Echo::DataStreamPtr pStream(Echo::Root::instance()->CreateDataStream(phyActorName.c_str(), "Physics"));
		if (pStream.isNull())
		{
			free(phyout);
			return false;
		}
		pStream->writeData(phyout, strlen(phyout));
		pStream->close();
		free(phyout);
		return true;
	}

	bool EchoPhysicsManager::CheckStaticPhysics(const ActorInfoPtr& actorInfo) {

		std::vector<ActorInfoPtr> childActors;
		childActors.push_back(actorInfo);
		actorInfo->getAllSubActorInfos(childActors);
		ActorComponentInfo* rootNodeInfo = actorInfo->getRootComInfo();
		if (rootNodeInfo == nullptr) {
			return false;
		}
		for (auto& childActor : childActors) {
			const std::vector<ActorComponentInfo*>& components = childActor->getComponentInfos();
			for (auto& pComp : components) {
				if (pComp == nullptr) {
					continue;
				}
				if (gColliderTypes.find(pComp->m_Type) != gColliderTypes.end()) {
					return true;
				}
			}

		}
		return false;


	}

	void EchoPhysicsManager::FindColliderComponent(const ActorPtr& actor, std::vector<ColliderComponent*>& colliders, bool IsSearchChildActor) {
		if (actor == nullptr) {
			return;
		}
		const EchoTMap<String, ActorComponentPtr>& components = actor->getComponents();
		for (auto& pComp : components) {
			if (pComp.second == nullptr) {
				continue;
			}
			if (gColliderTypes.find(pComp.second->getType()) != gColliderTypes.end()) {
				colliders.push_back((ColliderComponent*)pComp.second.getPointer());
			}
		}
		if (IsSearchChildActor) {
			const EchoTMap<String, ActorPtr>& childActors = actor->getChildActors();
			for (auto& childActor : childActors) {
				FindColliderComponent(childActor.second, colliders, true);
			}
		}
	}
	void EchoPhysicsManager::FindColliderComponentPtr(const ActorPtr& actor, std::vector<ColliderComponentPtr>& colliders, bool IsSearchChildActor) {
		if (actor == nullptr) {
			return;
		}
		const EchoTMap<String, ActorComponentPtr>& components = actor->getComponents();
		for (auto& pComp : components) {
			if (pComp.second == nullptr) {
				continue;
			}
			if (gColliderTypes.find(pComp.second->getType()) != gColliderTypes.end()) {
				colliders.push_back(pComp.second.getStaticCastWeakPtr<ColliderComponent>());
			}
		}
		if (IsSearchChildActor) {
			const EchoTMap<String, ActorPtr>& childActors = actor->getChildActors();
			for (auto& childActor : childActors) {
				FindColliderComponentPtr(childActor.second, colliders, true);
			}
		}
	}
	RigidBodyComponentPtr EchoPhysicsManager::FindNewRigidBodyComp(const ActorComponentPtr& pNodeComp) {
		ActorComponentPtr rigid = nullptr;
		ActorComponentPtr ptr = pNodeComp;
		while (ptr != nullptr) {
			rigid = ptr->getFirstChildComByType(gRigidBodyName);
			if (rigid != nullptr && rigid->getRealVisibleState()) {
				return rigid.getStaticCastWeakPtr<RigidBodyComponent>();
			}
			else {
				ptr = ptr->getParentNodeCom();
			}
		}
		return nullptr;
	}
	RigidBodyComponentPtr EchoPhysicsManager::FindNewRigidBodyComp(const ActorComponentPtr& pNodeComp, RigidBodyComponent* exclude) {
		if (exclude == nullptr) return FindNewRigidBodyComp(pNodeComp);
		ActorComponentPtr rigid = nullptr;
		ActorComponentPtr ptr = pNodeComp;
		ActorComponentPtr excludeParent = exclude->getParentNodeCom();
		while (ptr != nullptr) {
			rigid = ptr->getFirstChildComByType(gRigidBodyName);
			if (rigid != nullptr && rigid->getRealVisibleState() && (rigid != exclude) && (ptr != excludeParent)) {
				return rigid.getStaticCastWeakPtr<RigidBodyComponent>();
			}
			else {
				ptr = ptr->getParentNodeCom();
			}
		}
		return nullptr;
	}

	void EchoPhysicsManager::getProtocolType(Name& type)
	{
		type = EchoPhysicsSystem::mType;
	}

	void EchoPhysicsManager::getProtocolTypeToString(std::string& typeStr)
	{
		typeStr = EchoPhysicsSystem::mType.c_str();
	}

	void PhysicsPagedLayerManager::SetPageSize(float cellSize, unsigned int _FrameAsyncLoadNumber, float _AsyncLoadWidth, int _PageSize) {
		if (!m_mapPagedLayers.empty()) return;
		m_cellSize = cellSize;
		mFrameAsyncLoadNumber = _FrameAsyncLoadNumber;
		mAsyncLoadWidth = Math::Clamp(_AsyncLoadWidth, 0.05f, 0.95f);
		float minMoveDistance = m_cellSize * 0.02f;
		m_fMinMoveDisSqure = floor(minMoveDistance * minMoveDistance);
		mPageSize = _PageSize;
	}
	void PhysicsPagedLayerManager::addTarget(const Vector3& position, bool bForceAsync) {
		mLoadTargets.push_back(std::make_pair(position, bForceAsync));
		uint8 id = (uint8)(mLoadTargets.size() - 1);
		for (auto it = m_mapPagedLayers.begin(); it != m_mapPagedLayers.end(); ++it)
		{
			it->second->TargetPushBack();
			it->second->SetTarget(id, bForceAsync);
			it->second->update(id, position);
			it->second->updateLoadPageState();
		}
		m_bNeedUpdate = true;
	}
	void PhysicsPagedLayerManager::setTarget(int index, const Vector3& position, bool bForceAsync) {
		if (index < 0 || index >= mLoadTargets.size()) return;
		mLoadTargets[index].first = position;
		mLoadTargets[index].second = bForceAsync;
		for (auto it = m_mapPagedLayers.begin(); it != m_mapPagedLayers.end(); ++it)
		{
			it->second->SetTarget(index, bForceAsync);
			it->second->update(index, position);
			it->second->updateLoadPageState();
		}
		m_bNeedUpdate = true;
	}
	void PhysicsPagedLayerManager::removeTarget(int index) {
		if (mLoadTargets.empty() || index < 1) return;
		int end = (int)mLoadTargets.size() - 1;
		if (index == end) {
			mLoadTargets.pop_back();
			for (auto it = m_mapPagedLayers.begin(); it != m_mapPagedLayers.end(); ++it)
			{
				it->second->TargetPopBack();
				it->second->updateLoadPageState();
			}
		}
		else if (index < mLoadTargets.size()) {
			mLoadTargets[index] = mLoadTargets[end];
			mLoadTargets.pop_back();
			for (auto it = m_mapPagedLayers.begin(); it != m_mapPagedLayers.end(); ++it)
			{
				it->second->TargetErase(index);
				it->second->updateLoadPageState();
			}
		}
		m_bNeedUpdate = true;
	}
	PhysicsPagedLayerManager::PhysicsPagedLayerManager() {
		mLoadTargets.resize(1);
	}
	PhysicsPagedLayerManager::~PhysicsPagedLayerManager() {
		mLoadTargets.clear();
	}

	void PhysicsPagedLayerManager::setCellSize(float cellSize) {
		if (!m_mapPagedLayers.empty()) return;
		m_cellSize = cellSize;
	}
	float PhysicsPagedLayerManager::getCellSize() {
		return m_cellSize;
	}
	void PhysicsPagedLayerManager::setAsyncLoadWidth(float width) {
		mAsyncLoadWidth = Math::Clamp(width, 0.05f, 0.95f);
	}
	float PhysicsPagedLayerManager::getAsyncLoadWidth() {
		return mAsyncLoadWidth;
	}

	const std::map<uint16, PhysicsPagedLayer*>& PhysicsPagedLayerManager::getMapPagedLayers() {
		return m_mapPagedLayers;
	}
	void PhysicsPagedLayerManager::update(const std::vector<PhysXPageLoadPointData>& targets) {
		std::map<uint8, Vector3> loadTargetMap;

		uint8 size = (uint8)mLoadTargets.size();
		for (uint8 i = 0; i != size; ++i) {
			if ((mLoadTargets[i].first.squaredDistance(targets[i].m_vLastPosition) < m_fMinMoveDisSqure) && !m_bNeedUpdate) {
				continue;
			}
			mLoadTargets[i].first = targets[i].m_vLastPosition;
			loadTargetMap[i] = targets[i].m_vLastPosition;
		}
		if (loadTargetMap.empty()) return;
		m_bNeedUpdate = false;

		for (auto it = m_mapPagedLayers.begin(); it != m_mapPagedLayers.end(); ++it)
		{
			it->second->update(loadTargetMap);
		}
	}

	PhysicsPagedLayer* PhysicsPagedLayerManager::createPagedLayer(uint16 depth) {
		int TargetSize = (int)mLoadTargets.size();
		PhysicsPagedLayer* pLayer = new PhysicsPagedLayer(this, TargetSize, mPageSize);
		pLayer->mDepth = depth;
		float cellSize = (1 << depth) * m_cellSize;
		pLayer->m_cellSize = cellSize;
		m_mapPagedLayers[depth] = pLayer;
		pLayer->updateAndSetTarget(mLoadTargets);
		return pLayer;
	}

	void PhysicsPagedLayerManager::deletePagedLayer(PhysicsPagedLayer* pLayer)
	{
		if (NULL == pLayer)
		{
			return;
		}

		uint16 depth = pLayer->mDepth;
		
		auto it = m_mapPagedLayers.find(depth);
		if (it == m_mapPagedLayers.end())
		{
			assert(false);
			return;
		}
		else
		{
			delete it->second;
			m_mapPagedLayers.erase(it);
		}
	}

	PhysicsPagedLayer* PhysicsPagedLayerManager::getPagedLayer(uint16 depth) {

		PhysicsPagedLayer* pPagedLayer = nullptr;

		auto it = m_mapPagedLayers.find(depth);
		if (it == m_mapPagedLayers.end())
		{
			pPagedLayer = createPagedLayer(depth);
		}
		else
		{
			pPagedLayer = it->second;
		}
		return pPagedLayer;
	}

	void PhysicsPagedLayerManager::addEchoPhysXObject(uint16 depth, const EchoIndex3& index, EchoPhysXObject* pEchoPhysXObject)
	{
		if (index == EchoIndex3::MIN)
		{
			return;
		}
		PhysicsPagedLayer* pPagedLayer = getPagedLayer(depth);
		pPagedLayer->addEchoPhysXObject(index, pEchoPhysXObject);
	}

	Echo::EchoIndex3 PhysicsPagedLayerManager::worldAABBToIndex(const AxisAlignedBox& box, int& depth)
	{
		if (box.IsEmpty())
		{
			//assert(false);
			return EchoIndex3::MIN;
		}
		
		const Vector3& m_worldOrigin = GetPhysicsManager()->GetActorManager()->getWorldOrigin();


		Vector3 boxSize = box.getSize();

		float maxLength = std::max(boxSize.x, boxSize.y);
		maxLength = std::max(maxLength, boxSize.z);


		float layer = maxLength / m_cellSize;
		if (layer > 1.0f)
		{
			layer = Math::Log2(layer);
			depth = (int)ceilf(layer);
		}

		float layerCellSize = (1 << depth) * m_cellSize;

		Vector3 centerPos = box.getCenter();
		Vector3 relCenterPos = centerPos - m_worldOrigin;

		EchoIndex3 centerIndex;

		centerIndex.x = static_cast<int>(floor(relCenterPos.x / layerCellSize));
		centerIndex.y = static_cast<int>(floor(relCenterPos.y / layerCellSize));
		centerIndex.z = static_cast<int>(floor(relCenterPos.z / layerCellSize));


		return centerIndex;
	}

	void PhysicsPagedLayerManager::updateEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject)
	{

		if (NULL == pEchoPhysXObject)
		{
			return;
		}
		const AxisAlignedBox& box = pEchoPhysXObject->getPhysXWorldAABB();
		int depth = 0;
		EchoIndex3 index = worldAABBToIndex(box, depth);


		if (pEchoPhysXObject->m_pPhysicsPage)
		{
			if (index == pEchoPhysXObject->m_pPhysicsPage->mIndex && pEchoPhysXObject->m_pPhysicsPage->mDepth == depth)
			{
				return;
			}
			removeEchoPhysXObject(pEchoPhysXObject);
		}

		addEchoPhysXObject(depth, index, pEchoPhysXObject);
	}

	void PhysicsPagedLayerManager::addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject)
	{

		if (NULL == pEchoPhysXObject)
		{
			assert(false);
			return;
		}
		const AxisAlignedBox& box = pEchoPhysXObject->getPhysXWorldAABB();

		int depth = 0;
		EchoIndex3 index = worldAABBToIndex(box, depth);

		addEchoPhysXObject(depth, index, pEchoPhysXObject);
	}

	void PhysicsPagedLayerManager::removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject)
	{

		if (NULL == pEchoPhysXObject)
		{
			assert(false);
			return;
		}
		PhysicsPage* pPage = pEchoPhysXObject->m_pPhysicsPage;
		if (pPage)
		{
			pPage->removeEchoPhysXObject(pEchoPhysXObject);

			PhysicsPagedLayer* pLayer = pPage->m_pPagedLayer;
			if (pPage->isEmpty())
			{
				pLayer->deletePage(pPage);
			}
			if (pLayer->isEmpty())
			{
				deletePagedLayer(pLayer);
			}

		}

	}

	void PhysicsPagedLayerManager::clear() {
		auto it = m_mapPagedLayers.begin();
		for (; it != m_mapPagedLayers.end(); ++it)
		{
			delete it->second;
		}
		m_mapPagedLayers.clear();
		mAsyncLoadPhysicsObjectQueue.clear();
	}
	void PhysicsPagedLayerManager::clearAsyncLoadPhysXObject() {
		for (auto& iter : mAsyncLoadPhysicsObjectQueue) {
			iter->setIsWaitAsyncLoad(false);
		}
		mAsyncLoadPhysicsObjectQueue.clear();
 	}
	void PhysicsPagedLayerManager::addAsyncLoadPhysXObject(EchoPhysXObject* ptr) {
		if (ptr == nullptr || ptr->getIsWaitAsyncLoad()) return;
		ptr->setIsWaitAsyncLoad(true);
		mAsyncLoadPhysicsObjectQueue.push_back(ptr);
	}
	void PhysicsPagedLayerManager::removeAsyncLoadPhysXObject(EchoPhysXObject* ptr) {
		if (ptr == nullptr || !ptr->getIsWaitAsyncLoad()) return;
		ptr->setIsWaitAsyncLoad(false);
		auto iter = std::find(mAsyncLoadPhysicsObjectQueue.begin(), mAsyncLoadPhysicsObjectQueue.end(), ptr);
		if (iter != mAsyncLoadPhysicsObjectQueue.end()) {
			mAsyncLoadPhysicsObjectQueue.erase(iter);
		}
	}
	int PhysicsPagedLayerManager::updateAsyncQueue(int maxLoadNumber) {
		if (mAsyncLoadPhysicsObjectQueue.empty()) return 0;
		int maxNumber = std::min(mFrameAsyncLoadNumber, maxLoadNumber);
		int n = maxNumber;
		while ((n-- != 0) && !mAsyncLoadPhysicsObjectQueue.empty()) {
			EchoPhysXObject* pComp = mAsyncLoadPhysicsObjectQueue.front();
			mAsyncLoadPhysicsObjectQueue.pop_front();
			pComp->setIsWaitAsyncLoad(false);
			if (!pComp->m_pPhysicsPage->isUnload()) {
				pComp->PhysxStateChanged(PageLoadStateType_Syncload);
			}
			else {
				++n;
			}
		}
		return maxNumber - n - 1;
	}

	void EchoPhysicsManager::AxisAlignedBoxLocalToWorld(AxisAlignedBox& box, const Vector3& worldPosition, const Quaternion& worldRotation) {
		if (worldRotation.isEqual(Quaternion::IDENTITY)) {
			box.SetMinMax(box.getMinimum() + worldPosition, box.getMaximum() + worldPosition);
			return;
		}
		Matrix4 matri(worldRotation);
		matri.setTrans(worldPosition);
		box.transform(matri);
	}
	void EchoPhysicsManager::computePhysicsWorldAABB(const std::vector<ColliderComponent*>& collider, AxisAlignedBox& box) {
		if (collider.empty()) return;
		int size = (int)collider.size();
		box.setNull();
		for (int i = 0; i != size; ++i) {
			if (collider[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsWorldAABB(collider[i], bound);
			box.Add(bound);
		}
	}
	void EchoPhysicsManager::computePhysicsWorldAABB(const std::vector<ColliderComponentPtr>& collider, AxisAlignedBox& box) {
		if (collider.empty()) return;
		int size = (int)collider.size();
		box.setNull();
		for (int i = 0; i != size; ++i) {
			if (collider[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsWorldAABB(collider[i].get(), bound);
			box.Add(bound);
		}
	}
	void EchoPhysicsManager::computePhysicsLocalAABB(const std::vector<ColliderComponent*>& colliders, const std::vector<Transform>& transforms, AxisAlignedBox& box) {
		if (colliders.empty() || (colliders.size() != transforms.size())) return;
		box.setNull();
		int size = (int)colliders.size();
		for (int i = 0; i != size; ++i) {
			if (colliders[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsLocalAABB(colliders[i], bound);
			AxisAlignedBoxLocalToWorld(bound, transforms[i].translation, transforms[i].rotation);
			box.Add(bound);
		}

	}
	void EchoPhysicsManager::computePhysicsLocalAABB(const std::vector<ColliderComponentPtr>& colliders, const std::vector<Transform>& transforms, AxisAlignedBox& box) {
		if (colliders.empty() || (colliders.size() != transforms.size())) return;
		box.setNull();
		int size = (int)colliders.size();
		for (int i = 0; i != size; ++i) {
			if (colliders[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsLocalAABB(colliders[i].getPointer(), bound);
			AxisAlignedBoxLocalToWorld(bound, transforms[i].translation, transforms[i].rotation);
			box.Add(bound);
		}
	}
	void EchoPhysicsManager::computePhysicsLocalAABB(const std::vector<ColliderComponent*>& colliders, const DTransform& attT, AxisAlignedBox& box) {
		if (colliders.empty()) return;
		box.setNull();
		int size = (int)colliders.size();
		for (int i = 0; i != size; ++i) {
			if (colliders[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsLocalAABB(colliders[i], bound);
			DTransform myT = { colliders[i]->getWorldPosition(), colliders[i]->getWorldRotation() };
			Transform resT;
			convertWorldToLocal(attT, myT, resT);
			AxisAlignedBoxLocalToWorld(bound, resT.translation, resT.rotation);
			box.Add(bound);
		}
	}
	void EchoPhysicsManager::computePhysicsLocalAABB(const std::vector<ColliderComponentPtr>& colliders, const DTransform& attT, AxisAlignedBox& box) {
		if (colliders.empty()) return;
		box.setNull();
		int size = (int)colliders.size();
		for (int i = 0; i != size; ++i) {
			if (colliders[i] == nullptr) continue;
			AxisAlignedBox bound;
			computePhysicsLocalAABB(colliders[i].getPointer(), bound);
			DTransform myT = { colliders[i]->getWorldPosition(), colliders[i]->getWorldRotation() };
			Transform resT;
			convertWorldToLocal(attT, myT, resT);
			AxisAlignedBoxLocalToWorld(bound, resT.translation, resT.rotation);
			box.Add(bound);
		}
	}
	void EchoPhysicsManager::computePhysicsWorldAABB(ColliderComponent* _collider, AxisAlignedBox& box) {
		if (_collider == nullptr) return;
		const Name& type = _collider->getType();
		Vector3 worldPosition = _collider->getWorldPosition();
		Quaternion worldRotation = _collider->getWorldRotation();
		if (type == gColliderTypesVec[1]) {
			worldRotation = Quaternion::IDENTITY;
		}
		computePhysicsLocalAABB(_collider, box);
		AxisAlignedBoxLocalToWorld(box, worldPosition, worldRotation);
	}
	void EchoPhysicsManager::computePhysicsLocalAABB(ColliderComponent* _collider, AxisAlignedBox& box) {
		if (_collider == nullptr) return;
		const Name& type = _collider->getType();

		if (type == gColliderTypesVec[0]) {
			BoxColliderComponent* collider = (BoxColliderComponent*)_collider;
			Vector3 extent = collider->GetRealHalfSize();
			box.setExtents(-extent, extent);
		}
		else if (type == gColliderTypesVec[1]) {
			SphereColliderComponent* collider = (SphereColliderComponent*)_collider;
			Vector3 extent(collider->GetRealRadius());
			box.setExtents(-extent, extent);
		}
		else if (type == gColliderTypesVec[2]) {
			CapsuleColliderComponent* collider = (CapsuleColliderComponent*)_collider;
			float radius, halfHeight;
			collider->GetRealRadiusHalfHeight(radius, halfHeight);
			Vector3 extent(halfHeight + radius, radius, radius);
			box.setExtents(-extent, extent);
		}
		else if (type == gColliderTypesVec[3]) {
			MeshColliderComponent* collider = (MeshColliderComponent*)_collider;
			AxisAlignedBox nsbox = collider->GetPhysicsLocalAABB();
			Vector3 worldScale = collider->getWorldScale();
			Vector3 min, max;
			min.x = (worldScale.x > 0.0f ? nsbox.mMinimum.x : nsbox.mMaximum.x) * worldScale.x;
			min.y = (worldScale.y > 0.0f ? nsbox.mMinimum.y : nsbox.mMaximum.y) * worldScale.y;
			min.z = (worldScale.z > 0.0f ? nsbox.mMinimum.z : nsbox.mMaximum.z) * worldScale.z;
			max.x = (worldScale.x > 0.0f ? nsbox.mMaximum.x : nsbox.mMinimum.x) * worldScale.x;
			max.y = (worldScale.y > 0.0f ? nsbox.mMaximum.y : nsbox.mMinimum.y) * worldScale.y;
			max.z = (worldScale.z > 0.0f ? nsbox.mMaximum.z : nsbox.mMinimum.z) * worldScale.z;
			box.setExtents(min, max);
		}
		else if (type == gColliderTypesVec[4]) {
			MeshColliderUNMOVEABLEComponent* collider = (MeshColliderUNMOVEABLEComponent*)_collider;
			AxisAlignedBox nsbox = collider->GetPhysicsLocalAABB();
			Vector3 worldScale = collider->getWorldScale();
			Vector3 min, max;
			min.x = (worldScale.x > 0.0f ? nsbox.mMinimum.x : nsbox.mMaximum.x) * worldScale.x;
			min.y = (worldScale.y > 0.0f ? nsbox.mMinimum.y : nsbox.mMaximum.y) * worldScale.y;
			min.z = (worldScale.z > 0.0f ? nsbox.mMinimum.z : nsbox.mMaximum.z) * worldScale.z;
			max.x = (worldScale.x > 0.0f ? nsbox.mMaximum.x : nsbox.mMinimum.x) * worldScale.x;
			max.y = (worldScale.y > 0.0f ? nsbox.mMaximum.y : nsbox.mMinimum.y) * worldScale.y;
			max.z = (worldScale.z > 0.0f ? nsbox.mMaximum.z : nsbox.mMinimum.z) * worldScale.z;
			box.setExtents(min, max);
		}
		else {
			box.Empty();
		}
	}

}

#ifdef USE_PHYSICS_PAGE

namespace Echo {

#ifndef PHYSICS_RUNTIME
	std::unordered_set<EchoPhysXGroupObject*> EchoPhysXGroupObject::gAllObject;
	std::unordered_set<EchoPhysXStaticObject*> EchoPhysXStaticObject::gAllObject;
	std::unordered_set<EchoPhysXDynamicObject*> EchoPhysXDynamicObject::gAllObject;
#endif // PHYSICS_RUNTIME

	EchoPhysXActorObject::EchoPhysXActorObject(EchoPhysXObjectType _type) : EchoPhysXObject(_type) {}
	void EchoPhysXActorObject::SetGroup(EchoPhysXGroupObject* _pGroup) {
		if (_pGroup != nullptr) {
			pGroup = _pGroup;
			if (m_pPhysicsPage != nullptr) {
				m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->removeEchoPhysXObject(this);
			}
		}
		else {
			pGroup = nullptr;
		}
	}

	EchoPhysXStaticObject::EchoPhysXStaticObject(ColliderComponent* _pComp) : EchoPhysXActorObject(ePhysXStaticObjectType), m_pComp(_pComp) {
#ifndef PHYSICS_RUNTIME
		gAllObject.insert(this);
#endif // PHYSICS_RUNTIME
	}
	EchoPhysXStaticObject::~EchoPhysXStaticObject() {
#ifndef PHYSICS_RUNTIME
		gAllObject.erase(this);
#endif // PHYSICS_RUNTIME
	}
	void EchoPhysXStaticObject::PhysxStateChanged(PageLoadStateType type) {
		if (type != PageLoadStateType_Asyncload && getIsWaitAsyncLoad()) {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXStaticObjectType)->removeAsyncLoadPhysXObject(this);
		}
		switch (type) {
		case PageLoadStateType_Syncload: {
			m_pComp->SetIsUsePhysics(true);
			break;
		}
		case PageLoadStateType_Asyncload: {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXStaticObjectType)->addAsyncLoadPhysXObject(this);
			break;
		}
		case PageLoadStateType_Unload: {
			m_pComp->SetIsUsePhysics(false);
			break;
		}
		}
	}
	AxisAlignedBox EchoPhysXStaticObject::getPhysXWorldAABB() {
		AxisAlignedBox mBox;
		EchoPhysicsManager::computePhysicsWorldAABB(m_pComp, mBox);
		if (mBox.isNull() || mBox.isInfinite()) {
			Vector3 halfSize(0.5f, 0.5f, 0.5f);
			Vector3 worldPosition = m_pComp->getWorldPosition();
			mBox = AxisAlignedBox(worldPosition - halfSize, worldPosition + halfSize);
		}
		return mBox;
	}

	EchoPhysXDynamicObject::EchoPhysXDynamicObject(RigidBodyComponent* _pComp) : EchoPhysXActorObject(ePhysXDynamicObjectType), m_pComp(_pComp) {
#ifndef PHYSICS_RUNTIME
		gAllObject.insert(this);
#endif // PHYSICS_RUNTIME
	}
	EchoPhysXDynamicObject::~EchoPhysXDynamicObject() {
#ifndef PHYSICS_RUNTIME
		gAllObject.erase(this);
#endif // PHYSICS_RUNTIME
	}
	void EchoPhysXDynamicObject::PhysxStateChanged(PageLoadStateType type) {
		if (type != PageLoadStateType_Asyncload && getIsWaitAsyncLoad()) {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXDynamicObjectType)->removeAsyncLoadPhysXObject(this);
		}
		switch (type) {
		case PageLoadStateType_Syncload: {
			auto iter = m_pComp->m_ColliderList.begin();
			while (iter != m_pComp->m_ColliderList.end()) {
				if (*iter == nullptr) {
					iter = m_pComp->m_ColliderList.erase(iter);
					continue;
				}
				(*iter)->SetIsUsePhysics(true);
				++iter;
			}
			m_pComp->SetIsUsePhysics(true);
			break;
		}
		case PageLoadStateType_Asyncload: {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXDynamicObjectType)->addAsyncLoadPhysXObject(this);
			break;
		}
		case PageLoadStateType_Unload: {
			auto iter = m_pComp->m_ColliderList.begin();
			while (iter != m_pComp->m_ColliderList.end()) {
				if (*iter == nullptr) {
					iter = m_pComp->m_ColliderList.erase(iter);
					continue;
				}
				(*iter)->SetIsUsePhysics(false);
				++iter;
			}
			m_pComp->SetIsUsePhysics(false);
			break;
		}
		}
	}
	AxisAlignedBox EchoPhysXDynamicObject::getPhysXWorldAABB() {
		AxisAlignedBox mBox = m_pComp->GetPhysicsWorldAABB();
		if (mBox.isNull() || mBox.isInfinite()) {
			Vector3 halfSize(0.5f, 0.5f, 0.5f);
			Vector3 worldPosition = m_pComp->getWorldPosition();
			mBox = AxisAlignedBox(worldPosition - halfSize, worldPosition + halfSize);
		}
		return mBox;
	}

	EchoPhysXGroupObject::EchoPhysXGroupObject(EchoPhysXObjectGraph* graph) : EchoPhysXObject(ePhysXGroupObjectType), m_pGraph(graph) {
		if (m_pGraph != nullptr) {
			for (auto& iter : m_pGraph->mData) {
				((EchoPhysXActorObject*)iter.first)->SetGroup(this);
			}
		}
#ifndef PHYSICS_RUNTIME
		gAllObject.insert(this);
#endif // PHYSICS_RUNTIME
	}

	EchoPhysXGroupObject::~EchoPhysXGroupObject() {
		if (m_pGraph) {
			for (auto& iter : m_pGraph->mData) {
				((EchoPhysXActorObject*)iter.first)->SetGroup(nullptr);
			}
			delete m_pGraph;
		}
#ifndef PHYSICS_RUNTIME
		gAllObject.erase(this);
#endif // PHYSICS_RUNTIME
	}

	void EchoPhysXGroupObject::PhysxStateChanged(PageLoadStateType type) {
		if (m_pGraph == nullptr) return;
		if (type != PageLoadStateType_Asyncload && getIsWaitAsyncLoad()) {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXGroupObjectType)->removeAsyncLoadPhysXObject(this);
		}
		switch (type) {
		case PageLoadStateType_Syncload: {
			for (auto& iter : m_pGraph->mData) {
				iter.first->PhysxStateChanged(type);
			}
			break;
		}
		case PageLoadStateType_Asyncload: {
			m_pPhysicsPage->m_pPagedLayer->m_pPageManger->GetPhysicsManager()->getMapPagedLayers(ePhysXGroupObjectType)->addAsyncLoadPhysXObject(this);
			break;
		}
		case PageLoadStateType_Unload: {
			for (auto& iter : m_pGraph->mData) {
				iter.first->PhysxStateChanged(type);
			}
			break;
		}
		}
	}

	AxisAlignedBox EchoPhysXGroupObject::getPhysXWorldAABB() {
		AxisAlignedBox mBox;
		if (m_pGraph != nullptr) {
			for (auto& iter : m_pGraph->mData) {
				mBox.Add(iter.first->getPhysXWorldAABB());
			}
		}
		return mBox;
	}

	void EchoPhysXGroupObject::AddEdge(JointComponent* pJoint) {
		if (m_pGraph == nullptr) return;
		auto& ptr1 = pJoint->GetSelfRigidBodyComp();
		auto& ptr2 = pJoint->GetConnectedRigidBodyComp();
		if (ptr1 == nullptr || ptr2 == nullptr) {
			return;
		}
		m_pGraph->AddEdge(ptr1->GetPhysXObject(), ptr2->GetPhysXObject());
	}

	bool EchoPhysXGroupObject::RemoveEdge(JointComponent* pJoint, std::vector<EchoPhysXObjectGraph*>& newGraphs, std::vector<EchoPhysXObject*>& newNodes) {
		if (m_pGraph == nullptr) return false;
		auto& ptr1 = pJoint->GetSelfRigidBodyComp();
		auto& ptr2 = pJoint->GetConnectedRigidBodyComp();
		if (ptr1 == nullptr || ptr2 == nullptr) {
			return false;
		}
		if (m_pGraph->RemoveEdge(ptr1->GetPhysXObject(), ptr2->GetPhysXObject())) {
			if (m_pGraph->Reset(newGraphs, newNodes)) {
				return true;
			}
		}
		return false;
	}

	void EchoPhysXGroupObject::AddNode(EchoPhysXObject* pObj) {
		if (m_pGraph == nullptr) return;
		m_pGraph->AddNode(pObj);
	}
	bool EchoPhysXGroupObject::RemoveNode(EchoPhysXObject* pObj, std::vector<EchoPhysXObjectGraph*>& newGraphs, std::vector<EchoPhysXObject*>& newNodes) {
		if (m_pGraph == nullptr) return false;
		if (m_pGraph->RemoveNode(pObj)) {
			if (m_pGraph->Reset(newGraphs, newNodes)) {
				return true;
			}
		}
		return m_pGraph->empty();
	}
}

#endif //USE_PHYSICS_PAGE

namespace Echo {

	class TestEchoColliderCallBack : public EchoColliderCallBack, public EchoRigidBodyCallBack {
	public:
		TestEchoColliderCallBack() {}
		virtual void OnCreate(RigidBodyComponent* rigid) override {
			LogManager::instance()->logMessage("Create:" + rigid->getCombinedName());
		}
		virtual void OnRelease(RigidBodyComponent* rigid) override {
			LogManager::instance()->logMessage("Release:" + rigid->getCombinedName());
		}
		virtual void OnWake(RigidBodyComponent* rigid) override {
			LogManager::instance()->logMessage("Wake:" + rigid->getCombinedName());
		}
		virtual void OnSleep(RigidBodyComponent* rigid) override {
			LogManager::instance()->logMessage("Sleep:" + rigid->getCombinedName());
		}
		virtual void OnContactModify(const std::vector<EchoContactModifyPair>& pair) override {
			for (const auto& iter : pair) {
				if (iter.rigidbody[0] != nullptr) {
					LogManager::instance()->logMessage("ContactModify:" + iter.rigidbody[0]->getCombinedName());
				}
			}
			//LogManager::instance()->logMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			//for (const EchoContactModifyPair& pair : pairs) {
			//	if (pair.contacts.isInverse()) continue;
			//	uint32 size = pair.contacts.size();
			//	for (uint32 i = 0; i != size; ++i) {
			//		String log = "--------------------------------------------------\n(" + pair.rigidbody[0]->getCombinedName() + ")\n";
			//		log += "Point:" + pair.contacts.getPoint(i).ToString() + "\n";
			//		log += "Normal:" + pair.contacts.getNormal(i).ToString() + "\n";
			//		log += "TargetVelocity:" + pair.contacts.getTargetVelocity(i).ToString() + "\n";
			//		log += "Separation:" + std::to_string(pair.contacts.getSeparation(i)) + "\n";
			//		log += "MaxImpulse:" + std::to_string(pair.contacts.getMaxImpulse(i)) + "\n";
			//		log += "Restitution:" + std::to_string(pair.contacts.getRestitution(i)) + "\n";
			//		log += "StaticFriction:" + std::to_string(pair.contacts.getStaticFriction(i)) + "\n";
			//		log += "DynamicFriction:" + std::to_string(pair.contacts.getDynamicFriction(i)) + "\n--------------------------------------------------";
			//		LogManager::instance()->logMessage(log);
			//	}
			//}
			//LogManager::instance()->logMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		}
		virtual void OnCCDContactModify(const std::vector<EchoContactModifyPair>& pair) override {
			for (const auto& iter : pair) {
				if (iter.rigidbody[0] != nullptr) {
					LogManager::instance()->logMessage("CCDContactModify:" + iter.rigidbody[0]->getCombinedName());
				}
			}
		}
		virtual void OnCollisionEnter(const EchoCollisionParam & param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("CollisionEnter:" + param.self_collider->getCombinedName());
			}
		}
		virtual void OnCollisionExit(const EchoCollisionParam& param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("CollisionExit:" + param.self_collider->getCombinedName());
			}
		}
		virtual void OnCollisionStay(const EchoCollisionParam& param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("CollisionStay:" + param.self_collider->getCombinedName());
			}
		}

		virtual void OnTriggerEnter(const EchoTriggerParam& param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("TriggerEnter:" + param.self_collider->getCombinedName());
			}
		}

		virtual void OnTriggerExit(const EchoTriggerParam& param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("TriggerExit:" + param.self_collider->getCombinedName());
			}
		}

		virtual void OnTriggerStay(const EchoTriggerParam& param) override {
			if (param.self_collider != nullptr) {
				LogManager::instance()->logMessage("TriggerStay:" + param.self_collider->getCombinedName());
			}
		}

		virtual void OnCollisionCCT(const EchoControllerShapeHitParam& param) override {
			if (param.shape != nullptr) {
				LogManager::instance()->logMessage("CollisionCCT:" + param.shape->getCombinedName());
			}
		}
		virtual void OnCollisionCCT(const EchoMathControllerShapeHitParam& param) override {
			if (param.shape != nullptr) {
				LogManager::instance()->logMessage("CollisionCCT:" + param.shape->getCombinedName());
			}
		}

		static EchoColliderCallBackPtr GetEchoColliderCallBackPtr() {
			static TestEchoColliderCallBack gPtr;
			return &gPtr;
		}
		static EchoRigidBodyCallBackPtr GetEchoRigidBodyCallBackPtr() {
			static TestEchoColliderCallBack gPtr;
			return &gPtr;
		}
	};

	void EchoPhysicsSystem::SetColliderHelperVisible(bool value) {
		if (GetMainPhysicsManager()) {
			GetMainPhysicsManager()->SetColliderHelperVisible(value);
		}
	}
	void EchoPhysicsSystem::SetJointHelperVisible(bool value) {
		if (GetMainPhysicsManager()) {
			GetMainPhysicsManager()->SetJointHelperVisible(value);
		}
	}
	void EchoPhysicsSystem::SetCCTHelperVisible(bool value) {
		if (GetMainPhysicsManager()) {
			GetMainPhysicsManager()->SetCCTHelperVisible(value);
		}
	}

	void EchoPhysicsSystem::SetCollisionDetectionModeS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		int mode = std::atoi(value[1].c_str());
		mCollisionDetectionMode = Math::Clamp(mode, 0, 3);
	}

	void EchoPhysicsSystem::ShootDynamicSphereColliderS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		int number = std::atoi(value[1].c_str());
		ShootDynamicSphereCollider(number);
	}

	void EchoPhysicsSystem::SetSpeedS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		float speed = (float)std::atof(value[1].c_str());
		mSpeed = Math::Max(speed, 0.0f);
	}

	void EchoPhysicsSystem::SetRandomModeS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		mRandomMode = std::atoi(value[1].c_str());
	}

	void EchoPhysicsSystem::ShootDynamicSphereCollider(int number) {

		if (number < 1 || number > 1000) return;
		number = (int)Math::Pow((float)number, 0.33f);
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			pActiveMgr->GetActorManager()->createMainActor();
			mainActor = pActiveMgr->GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestShootCollider");
		if (pAllRootActor == nullptr) {
			pAllRootActor = pActiveMgr->GetActorManager()->createActor();
			pAllRootActor->setActorID("PhysicsTestShootCollider");
			mainActor->addActor(mainActor->getRootNodeCom(), pAllRootActor);
		}

	
		ActorPtr pRootActor = pActiveMgr->GetActorManager()->createActor();
		pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pRootActor);

		if (mRandomMode == 0) {
			
			Vector3 camPos = pActiveMgr->GetCamera()->getPosition();
			camPos.y += 6.0f;
			Quaternion camRot = pActiveMgr->GetCamera()->getOrientation();
			Vector3 velDir = camRot * (-Vector3::UNIT_Z) * mSpeed;
			pRootActor->setWorldPosition(camPos);
			pRootActor->setWorldRotation(camRot);

			float offset = 1.0f;
			float lower = (number - 1) * -0.5f;
			for (int i = 0; i != number; ++i) {
				for (int j = 0; j != number; ++j) {
					for (int k = 0; k != number; ++k) {
						ActorPtr pActor = pActiveMgr->GetActorManager()->createActor();
						Vector3 pos(lower + i * offset, lower + j * offset, lower + k * offset);
						pActor->setPosition(pos);
						auto pComp = pActor->AddComponent<RigidBodyComponent>(String("RigidBody"), pActor->getRootNodeCom());
						pComp->SetCollisionDetection(mCollisionDetectionMode);
						//pComp->SetConstraints(EchoPhysics::FreezeRotation);
						//pComp->SetUseGravity(true);
						pComp->SetVelocity(velDir);
						auto pComp2 = pActor->AddComponent<SphereColliderComponent>(String("Collider"), pActor->getRootNodeCom());
						pComp2->SetRadius(0.5f);
						auto pComp3 = pActor->AddComponent<EntityComponent>(String("Entity"), pActor->getRootNodeCom());
						pComp3->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
						pComp3->setRenderDistance(5000.0f);
						pRootActor->addActor(pRootActor->getRootNodeCom(), pActor);
					}
				}
			}

		}
		else {

			Vector3 camPos = pActiveMgr->GetCamera()->getPosition();
			Quaternion camRot = pActiveMgr->GetCamera()->getOrientation();
			pRootActor->setWorldPosition(camPos);
			pRootActor->setWorldRotation(camRot);

			Vector3 offsetVec(0.0f, 0.0f, -1.0f);
			float offset = 9.0f;
			float lower = (number - 1) * -0.5f * offset;
			for (int i = 0; i != number; ++i) {
				for (int j = 0; j != number; ++j) {
					for (int k = 0; k != number; ++k) {
						ActorPtr pActor = pActiveMgr->GetActorManager()->createActor();
						Quaternion rot;
						rot.FromEuler(Vector3(Math::RangeRandom(-180.f, 180.f), Math::RangeRandom(-180.f, 180.f), Math::RangeRandom(-180.f, 180.f)));
						Vector3 pos(lower + i * offset, lower + j * offset, lower + k * offset);
						Vector3 vel = Vector3::UNIT_Z * rot;
						pos += vel;
						pActor->setPosition(pos);
						auto pComp = pActor->AddComponent<RigidBodyComponent>(String("RigidBody"), pActor->getRootNodeCom());
						pComp->SetCollisionDetection(mCollisionDetectionMode);
						//pComp->SetConstraints(EchoPhysics::FreezeRotation);
						//pComp->SetUseGravity(true);
						pComp->SetVelocity(vel * mSpeed);
						auto pComp2 = pActor->AddComponent<SphereColliderComponent>(String("Collider"), pActor->getRootNodeCom());
						pComp2->SetRadius(0.5f);
						auto pComp3 = pActor->AddComponent<EntityComponent>(String("Entity"), pActor->getRootNodeCom());
						pComp3->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
						pComp3->setRenderDistance(5000.0f);
						pRootActor->addActor(pRootActor->getRootNodeCom(), pActor);
					}
				}
			}

		}
		


	}

	void EchoPhysicsSystem::ClearShootCollider() {
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			return;
		}
		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestShootCollider");
		if (pAllRootActor == nullptr) {
			return;
		}
		mainActor->delActor(pAllRootActor);
	}

	void EchoPhysicsSystem::CreateStaticTestPhysics(int _number) {
		if (_number < 1) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		int number = Math::Max(1, _number / 2);
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			pActiveMgr->GetActorManager()->createMainActor();
			mainActor = pActiveMgr->GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestStatic");
		if (pAllRootActor == nullptr) {
			pAllRootActor = pActiveMgr->GetActorManager()->createActor();
			pAllRootActor->setActorID("PhysicsTestStatic");
			mainActor->addActor(mainActor->getRootNodeCom(), pAllRootActor);
		}
		if (mCreateStaticParentActor) {
			pAllRootActor = mCreateStaticParentActor;
		}

		AxisAlignedBox box;
		Vector3 extent(0.5f, 0.5f, 0.5f);
		box.setExtents(-extent, extent);
		if (mCreateStaticMode == 0) {

			for (int i = 0; i != number; ++i) {
				Vector3 pos = Vector3(
					Math::RangeRandom(mCreateStaticPositionMin.x, mCreateStaticPositionMax.x),
					Math::RangeRandom(mCreateStaticPositionMin.y, mCreateStaticPositionMax.y),
					Math::RangeRandom(mCreateStaticPositionMin.z, mCreateStaticPositionMax.z)
				) + mCreateStaticOrigin;
				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(Name("collision/mesh/model_helperobj_sphere.meshphy"));
				pComp1->SetMeshName(Name("collision/mesh/model_helperobj_sphere.meshphy"));
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/sphere.mesh");
				pComp2->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);
			}

			for (int i = 0; i != number; ++i) {
				Vector3 pos = Vector3(
					Math::RangeRandom(mCreateStaticPositionMin.x, mCreateStaticPositionMax.x),
					Math::RangeRandom(mCreateStaticPositionMin.y, mCreateStaticPositionMax.y),
					Math::RangeRandom(mCreateStaticPositionMin.z, mCreateStaticPositionMax.z)
				) + mCreateStaticOrigin;

				Vector3 euler = Vector3(
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f)
				);
				Quaternion rot;
				rot.FromEuler(euler);
				Vector3 scl = Vector3(Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax));
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setRotation(rot);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetEulerAngles(euler);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(Name("collision/mesh/model_helperobj_cube.meshphy"));
				pComp1->SetMeshName(Name("collision/mesh/model_helperobj_cube.meshphy"));
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/cube.mesh");
				pComp2->loadEntity(Name("echo/model/helperobj/cube.mesh"));
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}
		else if(mCreateStaticMode == 1) {
			
			for (int i = 0; i != number; ++i) {

				Vector3 euler1 = Vector3(
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f)
				);
				Quaternion quat1;
				quat1.FromEuler(euler1);
				Vector3 pos = Vector3::UNIT_Z * Math::RangeRandom(mCreateStaticRadiusMin, mCreateStaticRadiusMax) * quat1 + mCreateStaticOrigin;
				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor();
				pActor->setPosition(pos);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(Name("collision/mesh/model_helperobj_sphere.meshphy"));
				pComp1->SetMeshName(Name("collision/mesh/model_helperobj_sphere.meshphy"));
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/sphere.mesh");
				pComp2->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);
			}

			for (int i = 0; i != number; ++i) {
				Vector3 euler1 = Vector3(
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f)
				);
				Quaternion quat1;
				quat1.FromEuler(euler1);
				Vector3 pos = Vector3::UNIT_Z * Math::RangeRandom(mCreateStaticRadiusMin, mCreateStaticRadiusMax) * quat1 + mCreateStaticOrigin;

				Vector3 euler = Vector3(
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f)
				);
				Quaternion rot;
				rot.FromEuler(euler);
				Vector3 scl = Vector3(Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax));
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor();
				pActor->setPosition(pos);
				pActor->setRotation(rot);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetEulerAngles(euler);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(Name("collision/mesh/model_helperobj_cube.meshphy"));
				pComp1->SetMeshName(Name("collision/mesh/model_helperobj_cube.meshphy"));
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/cube.mesh");
				pComp2->loadEntity(Name("echo/model/helperobj/cube.mesh"));
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}

	}

	void EchoPhysicsSystem::CreateStaticTestPhysicsBox(int number) {
		if (number < 1) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			pActiveMgr->GetActorManager()->createMainActor();
			mainActor = pActiveMgr->GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestStatic");
		if (pAllRootActor == nullptr) {
			pAllRootActor = pActiveMgr->GetActorManager()->createActor();
			pAllRootActor->setActorID("PhysicsTestStatic");
			mainActor->addActor(mainActor->getRootNodeCom(), pAllRootActor);
		}

		if (mCreateStaticParentActor) {
			pAllRootActor = mCreateStaticParentActor;
		}

		if (mCreateStaticMode == 0) {

			for (int i = 0; i != number; ++i) {
				Vector3 pos = Vector3(
					Math::RangeRandom(mCreateStaticPositionMin.x, mCreateStaticPositionMax.x),
					Math::RangeRandom(mCreateStaticPositionMin.y, mCreateStaticPositionMax.y),
					Math::RangeRandom(mCreateStaticPositionMin.z, mCreateStaticPositionMax.z)
				) + mCreateStaticOrigin;

				Vector3 euler = Vector3(
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f)
				);
				Quaternion rot;
				rot.FromEuler(euler);
				Vector3 scl = Vector3(Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax));
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setRotation(rot);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetEulerAngles(euler);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<BoxColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp2->loadEntity(Name("echo/model/helperobj/cube.mesh"));
				pComp2->setRenderDistance(5000.0f);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/cube.mesh");
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}
		else if (mCreateStaticMode == 1) {

			for (int i = 0; i != number; ++i) {
				Vector3 euler1 = Vector3(
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f)
				);
				Quaternion quat1;
				quat1.FromEuler(euler1);
				Vector3 pos = Vector3::UNIT_Z * Math::RangeRandom(mCreateStaticRadiusMin, mCreateStaticRadiusMax) * quat1 + mCreateStaticOrigin;

				Vector3 euler = Vector3(
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f),
					Math::RangeRandom(-90.f, 90.f)
				);
				Quaternion rot;
				rot.FromEuler(euler);
				Vector3 scl = Vector3(Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax), Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax));
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setRotation(rot);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetEulerAngles(euler);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<BoxColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp2->loadEntity(Name("echo/model/helperobj/cube.mesh"));
				pComp2->setRenderDistance(5000.0f);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/cube.mesh");
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}

	}

	void EchoPhysicsSystem::CreateStaticTestPhysicsSphere(int number) {
		if (number < 1) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			pActiveMgr->GetActorManager()->createMainActor();
			mainActor = pActiveMgr->GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestStatic");
		if (pAllRootActor == nullptr) {
			pAllRootActor = pActiveMgr->GetActorManager()->createActor();
			pAllRootActor->setActorID("PhysicsTestStatic");
			mainActor->addActor(mainActor->getRootNodeCom(), pAllRootActor);
		}

		if (mCreateStaticParentActor) {
			pAllRootActor = mCreateStaticParentActor;
		}

		if (mCreateStaticMode == 0) {

			for (int i = 0; i != number; ++i) {
				Vector3 pos = Vector3(
					Math::RangeRandom(mCreateStaticPositionMin.x, mCreateStaticPositionMax.x),
					Math::RangeRandom(mCreateStaticPositionMin.y, mCreateStaticPositionMax.y),
					Math::RangeRandom(mCreateStaticPositionMin.z, mCreateStaticPositionMax.z)
				) + mCreateStaticOrigin;
				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<SphereColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp1->SetRadius(0.5f);
				((SphereColliderComponentInfo*)pComp1->m_pActComInfo)->SetRadius(0.5f);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp2->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
				pComp2->setRenderDistance(5000.0f);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/sphere.mesh");
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);
			}


		}
		else if (mCreateStaticMode == 1) {

			for (int i = 0; i != number; ++i) {

				Vector3 euler1 = Vector3(
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f)
				);
				Quaternion quat1;
				quat1.FromEuler(euler1);
				Vector3 pos = Vector3::UNIT_Z * Math::RangeRandom(mCreateStaticRadiusMin, mCreateStaticRadiusMax) * quat1 + mCreateStaticOrigin;
				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<SphereColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp1->SetRadius(0.5f);
				((SphereColliderComponentInfo*)pComp1->m_pActComInfo)->SetRadius(0.5f);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				pComp2->loadEntity(Name("echo/model/helperobj/sphere.mesh"));
				pComp2->setRenderDistance(5000.0f);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName("echo/model/helperobj/sphere.mesh");
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);
			}


		}

	}

	void EchoPhysicsSystem::CreateStaticTestPhysicsMesh(const String& meshName, int number) {

		if (number < 1) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			pActiveMgr->GetActorManager()->createMainActor();
			mainActor = pActiveMgr->GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestStatic");
		if (pAllRootActor == nullptr) {
			pAllRootActor = pActiveMgr->GetActorManager()->createActor();
			pAllRootActor->setActorID("PhysicsTestStatic");
			mainActor->addActor(mainActor->getRootNodeCom(), pAllRootActor);
		}
		if (mCreateStaticParentActor) {
			pAllRootActor = mCreateStaticParentActor;
		}

		String entityMeshPath = meshName;
		std::replace(entityMeshPath.begin(), entityMeshPath.end(), '\\', '/');
		Name entityMeshName(entityMeshPath.c_str());
		String str = entityMeshPath + "phy";
		std::replace(str.begin(), str.end(), '/', '_');
		str.insert(0, "collision/mesh/");
		Name colliderMeshName(str.c_str());

		AxisAlignedBox box;
		PhysXTriangleMeshResourcePtr res = PhysXTriangleMeshResourceManager::instance()->getByName(colliderMeshName);
		if (res.isNull() || !res->isLoaded())
		{
			res = PhysXTriangleMeshResourceManager::instance()->load(colliderMeshName);
		}
		if (!res.isNull() && res->isLoaded() && res->m_mesh)
		{
			box = res->m_mesh->getLocalBounds();
		}


		if (mCreateStaticMode == 0) {

			for (int i = 0; i != number; ++i) {
				Vector3 pos = Vector3(
					Math::RangeRandom(mCreateStaticPositionMin.x, mCreateStaticPositionMax.x),
					Math::RangeRandom(mCreateStaticPositionMin.y, mCreateStaticPositionMax.y),
					Math::RangeRandom(mCreateStaticPositionMin.z, mCreateStaticPositionMax.z)
				) + mCreateStaticOrigin;

				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor(Name::g_cEmptyName, "Object", true);
				pActor->setPosition(pos);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(colliderMeshName);
				pComp1->SetMeshName(colliderMeshName);
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName(entityMeshPath);
				pComp2->loadEntity(entityMeshName);
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}
		else if (mCreateStaticMode == 1) {

			for (int i = 0; i != number; ++i) {
				Vector3 euler1 = Vector3(
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f),
					Math::RangeRandom(-180.f, 180.f)
				);
				Quaternion quat1;
				quat1.FromEuler(euler1);
				Vector3 pos = Vector3::UNIT_Z * Math::RangeRandom(mCreateStaticRadiusMin, mCreateStaticRadiusMax) * quat1 + mCreateStaticOrigin;

				Quaternion rot;
				rot.FromAxesToAxes(Vector3::UP, Vector3::UNIT_Z * quat1);
				Vector3 euler = rot.ToEulerPositive();
				Vector3 scl = Math::RangeRandom(mCreateStaticRandomScaleMin, mCreateStaticRandomScaleMax) * Vector3::UNIT_SCALE;
				ActorPtr pActor = pActiveMgr->GetActorManager()->createActor();
				pActor->setPosition(pos);
				pActor->setRotation(rot);
				pActor->setScale(scl);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetPosition(pos);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetEulerAngles(euler);
				(pActor->getRootNodeCom()->m_pActComInfo)->SetScale(scl);
				auto pComp1 = pActor->AddComponent<MeshColliderComponent>(String("collider"), pActor->getRootNodeCom(), nullptr, 0, false);
				((MeshColliderComponentInfo*)pComp1->m_pActComInfo)->SetMeshName(colliderMeshName);
				pComp1->SetMeshName(colliderMeshName);
				pComp1->SetPhysicsLocalAABB(box);
				auto pComp2 = pActor->AddComponent<EntityComponent>(String("entity"), pActor->getRootNodeCom(), nullptr, 0, false);
				((EntityComponentInfo*)pComp2->m_pActComInfo)->SetMeshName(entityMeshPath);
				pComp2->loadEntity(entityMeshName);
				pComp2->setRenderDistance(5000.0f);
				pAllRootActor->addActor(pAllRootActor->getRootNodeCom(), pActor);
				pAllRootActor->addActorInfo(pAllRootActor->getRootNodeCom(), pActor);

			}

		}

	}

	void EchoPhysicsSystem::SetCreateStaticPositionMinS(const vector<String>::type& value) {
		if (value.size() != 4) return;
		mCreateStaticPositionMin = Vector3((float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()), (float)std::atof(value[3].c_str()));
	}
	void EchoPhysicsSystem::SetCreateStaticPositionMaxS(const vector<String>::type& value) {
		if (value.size() != 4) return;
		mCreateStaticPositionMax = Vector3((float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()), (float)std::atof(value[3].c_str()));
	}
	void EchoPhysicsSystem::SetCreateStaticOriginS(const vector<String>::type& value) {
		if (value.size() != 4) return;
		mCreateStaticOrigin = Vector3((float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()), (float)std::atof(value[3].c_str()));
	}
	void EchoPhysicsSystem::SetCreateStaticParentActorS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		mCreateStaticParentActor = pActiveMgr->GetActorManager()->getActor(value[1]);
	}
	void EchoPhysicsSystem::CreateStaticTestPhysicsS(const vector<String>::type& value) {
		if (value.size() == 2) {
			int number = std::atoi(value[1].c_str());
			CreateStaticTestPhysics(number);
		}
		else if(value.size() == 3) {
			int number = std::atoi(value[2].c_str());
			if (value[1] == "Box") {
				CreateStaticTestPhysicsBox(number);
			}
			else if (value[1] == "Sphere") {
				CreateStaticTestPhysicsSphere(number);
			}
		}
		else if (value.size() == 4 && value[1] == "Mesh") {
			int number = std::atoi(value[3].c_str());
			CreateStaticTestPhysicsMesh(value[2], number);
		}
	}

	void EchoPhysicsSystem::SetCreateStaticRandomScaleS(const vector<String>::type& value) {
		if (value.size() != 3) return;
		mCreateStaticRandomScaleMin = Math::Max(1.0f, (float)std::atof(value[1].c_str()));
		mCreateStaticRandomScaleMax = Math::Max(mCreateStaticRandomScaleMin, (float)std::atof(value[2].c_str()));
	}

	void EchoPhysicsSystem::ClearStaticTestPhysics() {
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr mainActor = pActiveMgr->GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			return;
		}
		ActorPtr pAllRootActor = mainActor->getChildActor("PhysicsTestStatic");
		if (pAllRootActor == nullptr) {
			return;
		}
		mainActor->delActor(pAllRootActor);
	}

	void EchoPhysicsSystem::SetCreateStaticRadiusS(const vector<String>::type& value) {
		if (value.size() != 3) return;
		mCreateStaticRadiusMin = Math::Max(1.0f, (float)std::atof(value[1].c_str()));
		mCreateStaticRadiusMax = Math::Max(1.0f, (float)std::atof(value[2].c_str()));
	}
	void EchoPhysicsSystem::SetCreateStaticModeS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		mCreateStaticMode = std::atoi(value[1].c_str());
	}

	void EchoPhysicsSystem::TestS(const vector<String>::type& value) {
		if (value.size() < 3) return;
		/*if (value[1] == "CCT") {
			mCCTMaxSpeed = 1.0f;
			int number = (int)std::atoi(value[2].c_str());
			if (number == 0) {
				ClearAutoMoveCCT();
			}
			else if (number > 0) {
				mShowCCTTestTime = true;
				CreateAutoMoveCCT(number);
			}
			else {
				mShowCCTTestTime = false;
				CreateAutoMoveCCT(-number);
			}
		}
		else */if (value[1] == "Static") {
			int number = Math::Max(12, (int)std::atoi(value[2].c_str())) / 6;
			float radiusScale = (value.size() == 4) ? (Math::Abs((float)std::atof(value[3].c_str()))) : 1.0f;
			mCreateStaticOrigin = Vector3::ZERO;
			mCreateStaticMode = 1;
			mCreateStaticRadiusMin = 1990.f * radiusScale;
			mCreateStaticRadiusMax = 2000.f * radiusScale;
			mCreateStaticRandomScaleMin = 50.0f * radiusScale;
			mCreateStaticRandomScaleMax = 60.0f * radiusScale;
			CreateStaticTestPhysicsBox(number);
			CreateStaticTestPhysicsSphere(number);
			mCreateStaticRadiusMin = 2050.f * radiusScale;
			mCreateStaticRadiusMax = 2100.f * radiusScale;
			mCreateStaticRandomScaleMin = 5.0f * radiusScale;
			mCreateStaticRandomScaleMax = 10.0f * radiusScale;
			CreateStaticTestPhysics(2 * number);
			mCreateStaticRadiusMin = 2000.f * radiusScale;
			mCreateStaticRadiusMax = 2100.f * radiusScale;
			mCreateStaticRandomScaleMin = 1.0f * radiusScale;
			mCreateStaticRandomScaleMax = 5.0f * radiusScale;
			CreateStaticTestPhysics(2 * number);
		}
		else if (value[1] == "Dynamic") {
			mSpeed = 100.0f;
			mCollisionDetectionMode = 2;
			mRandomMode = 0;
			int number = (int)std::atoi(value[2].c_str());
			ShootDynamicSphereCollider(number);
		}
		else if (value[1] == "STS") {
			int number = Math::Max(3, (int)std::atoi(value[2].c_str())) / 3;
			float radiusScale = (value.size() == 4) ? (Math::Abs((float)std::atof(value[3].c_str()))) : 1.0f;
			mCreateStaticOrigin = Vector3(0.0f, 22000.0f, 0.0f);
			mCreateStaticMode = 1;
			mCreateStaticRadiusMin = 20000.f * radiusScale;
			mCreateStaticRadiusMax = 20600.f * radiusScale;
			mCreateStaticRandomScaleMin = 100.0f * radiusScale;
			mCreateStaticRandomScaleMax = 300.0f * radiusScale;
			CreateStaticTestPhysicsBox(number);
			CreateStaticTestPhysicsSphere(number);
			CreateStaticTestPhysics(number);
		}
	}
	void EchoPhysicsSystem::SetUsePhysXPageS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		bool bv = std::atoi(value[1].c_str()) == 1;
		pActiveMgr->setUsePhysXPage(bv);
	}
	void EchoPhysicsSystem::ResetCellSizeS(const vector<String>::type& value) {
		if (value.size() != 2) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		pActiveMgr->resetCellSize((float)std::atof(value[1].c_str()));
	}
	void EchoPhysicsSystem::SetPhysXPageLoadMainActorS(const vector<String>::type& value) {
		if (value.size() != 3) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr selectAct = pActiveMgr->GetActorManager()->getActor(value[1]);
		if (selectAct) {
			pActiveMgr->SetPhysXPageLoadMainActor(selectAct->getComponent(value[2]));
		}
		else {
			pActiveMgr->SetPhysXPageLoadMainActor(ActorComponent::NULLPTR);
		}
	}
	void EchoPhysicsSystem::AddPhysXPageLoadMainActorS(const vector<String>::type& value) {
		if (value.size() != 3) return;
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr == nullptr) return;
		ActorPtr selectAct = pActiveMgr->GetActorManager()->getActor(value[1]);
		if (selectAct) {
			if (pActiveMgr) pActiveMgr->AddPhysXPageLoadMainActor(selectAct->getComponent(value[2]));
		}
	}

	void EchoPhysicsSystem::PrintMemoryS(const vector<String>::type& value) {
		if (value.size() == 2) {
			size_t size = PhysXSystem::instance()->GetPhysicsMemory();
			if (value[1] == "G") {
				double data = (double)size / (double)(1024 * 1024 * 1024);
				LogManager::instance()->logMessage("PhysXMemory : " + std::to_string(data) + " G");

			}
			else if (value[1] == "M") {
				double data = (double)size / (double)(1024 * 1024);
				LogManager::instance()->logMessage("PhysXMemory : " + std::to_string(data) + " M");
			}
			else if (value[1] == "K") {
				double data = (double)size / (double)(1024);
				LogManager::instance()->logMessage("PhysXMemory : " + std::to_string(data) + " K");
			}
			else if (value[1] == "B") {
				LogManager::instance()->logMessage("PhysXMemory : " + std::to_string(size) + " B");
			}
		}
		else if (value.size() == 3) {
			size_t size = PhysXSystem::instance()->GetPhysicsMemory();
			const auto& mp = PhysXSystem::instance()->GetPhysicsMemoryType();
			std::multimap<double, std::string> logOuputs;
			if (value[1] == "G") {
				LogManager::instance()->logMessage("==========PhysXMemory==========");
				double data = (double)size / (double)(1024 * 1024 * 1024);
				LogManager::instance()->logMessage("All : " + std::to_string(data) + " G");
				for (auto& iter : mp) {
					data = (double)(iter.second.size) / (double)(1024 * 1024 * 1024);
					logOuputs.insert(std::make_pair(data, iter.second.fileName + " " + std::to_string(iter.second.line) + " : " + std::to_string(data) + " G"));
				}
			}
			else if (value[1] == "M") {
				LogManager::instance()->logMessage("==========PhysXMemory==========");
				double data = (double)size / (double)(1024 * 1024);
				LogManager::instance()->logMessage("All : " + std::to_string(data) + " M");
				for (auto& iter : mp) {
					data = (double)(iter.second.size) / (double)(1024 * 1024);
					logOuputs.insert(std::make_pair(data, iter.second.fileName + " " + std::to_string(iter.second.line) + " : " + std::to_string(data) + " M"));
				}
			}
			else if (value[1] == "K") {
				LogManager::instance()->logMessage("==========PhysXMemory==========");
				double data = (double)size / (double)(1024);
				LogManager::instance()->logMessage("All : " + std::to_string(data) + " K");
				for (auto& iter : mp) {
					data = (double)(iter.second.size) / (double)(1024);
					logOuputs.insert(std::make_pair(data, iter.second.fileName + " " + std::to_string(iter.second.line) + " : " + std::to_string(data) + " K"));
				}
			}
			else if (value[1] == "B") {
				LogManager::instance()->logMessage("==========PhysXMemory==========");
				LogManager::instance()->logMessage("All : " + std::to_string(size) + " B");
				for (auto& iter : mp) {
					double data = (double)(iter.second.size);
					logOuputs.insert(std::make_pair(data, iter.second.fileName + " " + std::to_string(iter.second.line) + " : " + std::to_string(data) + " B"));
				}
			}
			for (const auto& logOutput : logOuputs) {
				LogManager::instance()->logMessage(logOutput.second);
			}
			LogManager::instance()->logMessage("==========PhysXMemory==========");
		}
	}

	void EchoPhysicsSystem::SetPhysXDebugLinesS(const vector<String>::type& _value) {
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (!pActiveMgr) return;
		if (_value.size() < 3) {
			if (_value.size() == 2) {
				int value = (int)std::atoi(_value[1].c_str());
				pActiveMgr->GetPhysXWorldManager()->SetShowDebugVisualization(value != 0);
				pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_SHAPES, 0.0f);
				switch (value) {
				case 1: {//显示碰撞体
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
					break;
				}
				case 2: {//显示碰撞体包围盒
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_AABBS, 1.0f);
					break;
				}
				case 3: {//显示碰撞体及其包围盒
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_AABBS, 1.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_AXES, 10.0f);
					break;
				}
				case 4: {//显示动静态刚体坐标轴
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eACTOR_AXES, 10.0f);
					break;
				}
				case 5: {//显示动态刚体参数
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eBODY_AXES, 10.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eBODY_MASS_AXES, 1.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eBODY_LIN_VELOCITY, 10.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eBODY_ANG_VELOCITY, 10.0f);
					break;
				}
				case 6: {//显示碰撞信息
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCONTACT_POINT, 10.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCONTACT_NORMAL, 10.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCONTACT_FORCE, 10.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCONTACT_ERROR, 10.0f);
					break;
				}
				case 7: {//显示物理世界中心
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eWORLD_AXES, 2000.0f);
					break;
				}
				case 8: {//显示物理世界中心及静态剪枝
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eWORLD_AXES, 100.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_STATIC, 1.0f);
					break;
				}
				case 9: {//显示物理世界中心及动态剪枝
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eWORLD_AXES, 100.0f);
					pActiveMgr->GetScene()->SetVisualizationParameter(EchoVisualizationParameter::eCOLLISION_DYNAMIC, 1.0f);
					break;
				}
				default: {
					break;
				}
				}
			}
			return;
		}
		if (_value[1] == "Cull") {
			if (_value.size() == 5) {
				//AuxiliaryManager::instance()->SetCullingBoxHalfExtents(Vector3(Math::Abs((float)std::atof(_value[2].c_str())), Math::Abs((float)std::atof(_value[3].c_str())), Math::Abs((float)std::atof(_value[4].c_str()))));
				pActiveMgr->GetPhysXWorldManager()->SetCullingBoxHalfExtents(Vector3(Math::Abs((float)std::atof(_value[2].c_str())), Math::Abs((float)std::atof(_value[3].c_str())), Math::Abs((float)std::atof(_value[4].c_str()))));
			}
			else if (_value.size() == 4 && _value[2] == "All") {
				int type = (int)std::atoi(_value[3].c_str());
				//AuxiliaryManager::instance()->SetIsManualCulling((bool)(type != 0));
				pActiveMgr->GetPhysXWorldManager()->SetIsManualCulling((bool)(type != 0));
			}
		}
		else if (_value[1] == "Scene") {
			if (_value.size() != 4) return;
			int type = (int)std::atoi(_value[2].c_str());
			if (type < 1 || type >= (int)EchoVisualizationParameter::eNUM_VALUES) return;
			int value = (int)std::atoi(_value[3].c_str());
			pActiveMgr->GetScene()->SetVisualizationParameter((EchoVisualizationParameter)type, (float)value);
		}
		else if (_value[1] == "CCT") {
			if (_value.size() != 3) return;
			int type = (int)std::atoi(_value[2].c_str());
			pActiveMgr->GetScene()->SetCCTVisualizationFlags((EchoControllerDebugRenderFlags)type);
		}
		
	}

	void EchoPhysicsSystem::ShowPhysicsData() {
		EchoPhysicsManager* pActiveMgr = GetActivePhysicsManager();
		if (pActiveMgr) {
			LogManager::instance()->logMessage("==========Physics==========");
			LogManager::instance()->logMessage("RigidBody : " + std::to_string(pActiveMgr->GetRigidBody_Exist()) + "/" + std::to_string(pActiveMgr->m_RigidBodyList.size()));
			LogManager::instance()->logMessage("Collider : " + std::to_string(pActiveMgr->GetCollider_Exist()) + "/" + std::to_string(pActiveMgr->m_ColliderList.size()));
			LogManager::instance()->logMessage("Controller : " + std::to_string(pActiveMgr->GetController_Exist()) + "/" + std::to_string(pActiveMgr->m_ControllerList.size()));
			LogManager::instance()->logMessage("Aggregate : " + std::to_string(pActiveMgr->GetAggregate_Exist()) + "/" + std::to_string(pActiveMgr->m_AggregateList.size()));
			LogManager::instance()->logMessage("Joint : " + std::to_string(pActiveMgr->GetJoint_Exist()) + "/" + std::to_string(pActiveMgr->m_JointList.size()));
			LogManager::instance()->logMessage("Terrain : " + std::to_string(pActiveMgr->GetTerrain_Exist()) + "/" + std::to_string(pActiveMgr->m_TerrainColliderList.size()));
			LogManager::instance()->logMessage("MoveRigidBody : " + std::to_string(pActiveMgr->mCurMoveNumber));
			LogManager::instance()->logMessage("Collision : " + std::to_string(pActiveMgr->mCurCollisionNumber));
			LogManager::instance()->logMessage("Trigger : " + std::to_string(pActiveMgr->mCurTriggerNumber));
			LogManager::instance()->logMessage("==========Physics==========");
		}
	}
	void EchoPhysicsSystem::ShowHelp() {
		LogManager::instance()->logMessage("-----------------------------------------");
		LogManager::instance()->logMessage("StartSim - Start Physics Simulate.");
		LogManager::instance()->logMessage("StopSim - Stop Physics Simulate.");
		LogManager::instance()->logMessage("ShowC - Show Collider HelperObject.");
		LogManager::instance()->logMessage("HideC - Hide Collider HelperObject.");
		LogManager::instance()->logMessage("ShowJ - Show Joint HelperObject.");
		LogManager::instance()->logMessage("HideJ - Hide Joint HelperObject.");
		LogManager::instance()->logMessage("ShowT - Show CCT HelperObject.");
		LogManager::instance()->logMessage("HideT - Hide CCT HelperObject.");
		LogManager::instance()->logMessage("Memory - Log Print Physics Memory Data");
		LogManager::instance()->logMessage("ResetCellSize N - Reset Physics Page CellSize.");
		LogManager::instance()->logMessage("PhysXPageLoadActorComponent ActorName ActorComponentName");
		LogManager::instance()->logMessage("DebugLines N - Set PhysX DebugLines.(0/1)");
		LogManager::instance()->logMessage("LogData - Show Physics Component Number.");
		LogManager::instance()->logMessage("DebugLinesHelp - Show PhysX DebugLines Flags Meaning.");
		LogManager::instance()->logMessage("-----------------------------------------");
		LogManager::instance()->logMessage("CreateDynamic N - Create N*N*N Dynamic Sphere.(1-10)");
		LogManager::instance()->logMessage("ClearDynamic - Delete All Dynamic Sphere.");
		LogManager::instance()->logMessage("DynamicSpeed N - Set RigidBody Speed.(>=0)");
		LogManager::instance()->logMessage("DynamicMode N - Set RigidBody Collision Detection Mode.(0-3)");
		LogManager::instance()->logMessage("DynamicRandom N - Set RigidBody Random Position/Velocity.(0/1)");
		//LogManager::instance()->logMessage("-----------------------------------------");
		//LogManager::instance()->logMessage("CreateCCT N - Create N CCT.");
		//LogManager::instance()->logMessage("ClearCCT - Delete All Create CCT.");
		//LogManager::instance()->logMessage("CCTOrigin X Y Z");
		//LogManager::instance()->logMessage("CCTMaxSpeed N");
		//LogManager::instance()->logMessage("CCTGravityConstant N");
		//LogManager::instance()->logMessage("CCTGravityMode N - Set CCT Gravity Mode.(0 - null, 1 - gravity, 2 - source)");
		//LogManager::instance()->logMessage("CCTGravitySource X Y Z - Set CCT GravitySource Position.");
		LogManager::instance()->logMessage("-----------------------------------------");
		LogManager::instance()->logMessage("CreateStatic N");
		LogManager::instance()->logMessage("CreateStatic (Box/Sphere) N");
		LogManager::instance()->logMessage("CreateStatic Mesh (MeshName) N");
		LogManager::instance()->logMessage("ClearStatic");
		LogManager::instance()->logMessage("StaticScale Min Max");
		LogManager::instance()->logMessage("StaticRadius Min Max");
		LogManager::instance()->logMessage("StaticMode N(0 - random, 1 - Sphere)");
		LogManager::instance()->logMessage("StaticPositionMin X Y Z");
		LogManager::instance()->logMessage("StaticPositionMax X Y Z");
		LogManager::instance()->logMessage("StaticOrigin X Y Z");
		LogManager::instance()->logMessage("CreateStaticParentActor ActorName");
		LogManager::instance()->logMessage("-----------------------------------------");
	}

	void EchoPhysicsSystem::TestSceneQueryTime(const vector<String>::type& value) {
		EchoPhysicsManager* pMgr = GetMainPhysicsManager();
		if (pMgr == nullptr) return;
		int size = (int)value.size();
		if (size == 2) {
			Vector3 center = pMgr->mPhysXPageLoadMainActors[0].m_vLastPosition;
			float halfSize = (float)std::atof(value[1].c_str());
			AxisAlignedBox bound(center - halfSize, center + halfSize);
			pMgr->TestSceneQueryTime(bound);
		}
		else if (size == 4) {
			Vector3 center = pMgr->mPhysXPageLoadMainActors[0].m_vLastPosition;
			Vector3 halfSize = Vector3((float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()), (float)std::atof(value[3].c_str()));
			AxisAlignedBox bound(center - halfSize, center + halfSize);
			pMgr->TestSceneQueryTime(bound);
		}
		else if (size == 7) {
			Vector3 min = Vector3((float)std::atof(value[1].c_str()), (float)std::atof(value[2].c_str()), (float)std::atof(value[3].c_str()));
			Vector3 max = Vector3((float)std::atof(value[4].c_str()), (float)std::atof(value[5].c_str()), (float)std::atof(value[6].c_str()));
			AxisAlignedBox bound(min, max);
			pMgr->TestSceneQueryTime(bound);
		}
	}

	void EchoPhysicsSystem::ShowDebugLinesFlagsData() {
		LogManager::instance()->logMessage("-----------------------------------------");
		LogManager::instance()->logMessage("Scene 1	(0/1)	: eWORLD_AXES");
		LogManager::instance()->logMessage("Scene 2	(0/1)	: eBODY_AXES");
		LogManager::instance()->logMessage("Scene 3	(0/1)	: eBODY_MASS_AXES");
		LogManager::instance()->logMessage("Scene 4	(0/1)	: eBODY_LIN_VELOCITY");
		LogManager::instance()->logMessage("Scene 5	(0/1)	: eBODY_ANG_VELOCITY");
		LogManager::instance()->logMessage("Scene 6	(0/1)	: eCONTACT_POINT");
		LogManager::instance()->logMessage("Scene 7	(0/1)	: eCONTACT_NORMAL");
		LogManager::instance()->logMessage("Scene 8	(0/1)	: eCONTACT_ERROR");
		LogManager::instance()->logMessage("Scene 9	(0/1)	: eCONTACT_FORCE");
		LogManager::instance()->logMessage("Scene 10 (0/1)	: eACTOR_AXES");
		LogManager::instance()->logMessage("Scene 11 (0/1)	: eCOLLISION_AABBS");
		LogManager::instance()->logMessage("Scene 12 (0/1)	: eCOLLISION_SHAPES");
		LogManager::instance()->logMessage("Scene 13 (0/1)	: eCOLLISION_AXES");
		LogManager::instance()->logMessage("Scene 14 (0/1)	: eCOLLISION_COMPOUNDS");
		LogManager::instance()->logMessage("Scene 15 (0/1)	: eCOLLISION_FNORMALS");
		LogManager::instance()->logMessage("Scene 16 (0/1)	: eCOLLISION_EDGES");
		LogManager::instance()->logMessage("Scene 17 (0/1)	: eCOLLISION_STATIC");
		LogManager::instance()->logMessage("Scene 18 (0/1)	: eCOLLISION_DYNAMIC");
		LogManager::instance()->logMessage("Scene 20 (0/1)	: eJOINT_LOCAL_FRAMES");
		LogManager::instance()->logMessage("Scene 21 (0/1)	: eJOINT_LIMITS");
		LogManager::instance()->logMessage("Scene 22 (0/1)	: eCULL_BOX");
		LogManager::instance()->logMessage("CCT 1 (0/1)		: eTEMPORAL_BV");
		LogManager::instance()->logMessage("CCT 2 (0/1)		: eCACHED_BV");
		LogManager::instance()->logMessage("CCT 8 (0/1)		: eCACHEDGEOM_BV");
		LogManager::instance()->logMessage("Cull X Y Z");
		LogManager::instance()->logMessage("Cull All (0/1)");
		LogManager::instance()->logMessage("-----------------------------------------");
	}
}

//============================================================================================================================================
//============================================================================================================================================
//============================================================================================================================================

namespace Echo {

	EchoPhysicsManager::EchoPhysicsManager(const Name& name)
		: ProtocolComponent(ProtocolComponentSystem::instance()->GetManager(name)), mName(name){

		m_pPhysXWorldManager = PhysXWorldSystem::instance()->CreateManager(name);
		mPhysXSceneCallBack = new EchoPhysicsSceneCallBack();
		mPhysXCCTCallBack = new EchoPhysicsControllerCallBack();
		mPhysXMathCCTCallBack = new EchoMathPhysicsControllerCallBack();

		m_pPhysXWorldManager->OnPreUpdate = std::bind<void>(&EchoPhysicsManager::_prePhysicsUpdate, this);
		m_pPhysXWorldManager->OnPostUpdate = std::bind<void>(&EchoPhysicsManager::_postPhysicsUpdate, this);

		m_pPhysXWorldManager->OnPreTick = std::bind<void>(&EchoPhysicsManager::_fixedUpdate, this);
		m_pPhysXWorldManager->OnPostTick = std::bind<void>(&EchoPhysicsManager::_latePhysicsUpdate, this);

		mPagedLayers[ePhysXDynamicObjectType].mType = ePhysXDynamicObjectType;
		mPagedLayers[ePhysXGroupObjectType].mType = ePhysXGroupObjectType;
		mPagedLayers[ePhysXStaticObjectType].mType = ePhysXStaticObjectType;

		mPagedLayers[ePhysXDynamicObjectType].SetPageSize(256.f, 500, 0.3f, 1);
		mPagedLayers[ePhysXGroupObjectType].SetPageSize(256.f, 500, 0.3f, 1);
		mPagedLayers[ePhysXStaticObjectType].SetPageSize(256.f, 500, 0.8f, 1);

		mAsyncUpdateNumber = 1500;

		m_pGraph = new EchoPhysicsPageGraph();
		m_pGraph->m_pManager = this;
		mPagedLayers[ePhysXDynamicObjectType].SetPhysicsManager(this);
		mPagedLayers[ePhysXGroupObjectType].SetPhysicsManager(this);
		mPagedLayers[ePhysXStaticObjectType].SetPhysicsManager(this);

		mPhysXPageLoadMainActors.push_back(PhysXPageLoadPointData());

		registerTickTimeBegin();
	}
	EchoPhysicsManager::~EchoPhysicsManager() {
		SetRunPhysics(false);
		mPagedLayers.clear();
		m_RigidBodyList.clear();
		m_ColliderList.clear();
		m_JointList.clear();
		m_ForceList.clear();
		m_ControllerList.clear();
		SAFE_DELETE(mPhysXSceneCallBack);
		SAFE_DELETE(mPhysXCCTCallBack);
		SAFE_DELETE(mPhysXMathCCTCallBack);
		SAFE_DELETE(m_pGraph);
		m_CallBackList.clear();
		m_SurfaceTypeColliderPtrMap.clear();
	}
	
	void EchoPhysicsManager::registerTickTimeBegin() {
		m_priority = INT_MAX;
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		m_eventSet[TT_EndFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}
	void EchoPhysicsManager::beginFrame(EventID, ProtocolHandle) {
		if (!PhysicsIsRunning()) return;

		UpdatePhysXPage();
		UpdateSphericalTerrain();
	}
	void EchoPhysicsManager::endFrame(EventID, ProtocolHandle) {
		if (instance() != this) return;
		PhysXWorldSystem::instance()->FlushPhysXJob();
	}

	void EchoPhysicsManager::_prePhysicsUpdate() {
		CreateSurfaceTypeCollider();
		if (!PhysicsIsRunning()) return;
		mIsPhysicsUpdating = true;
		mCurMoveNumber = 0;
		mCurCollisionNumber = 0;
		mCurTriggerNumber = 0;
		FlushPhysicsSceneImpl();
		UpdatePhysXWorldOrigin();
		SyncAllCctPhysXGlobalPose();
		AsyncUpdatePhysics();
		UpdatePhysics();
		UpdatePhysics();
		UpdateRigidBodyMAITQueue();
		mIsPhysicsUpdating = false;
	}
	void EchoPhysicsManager::_fixedUpdate() {
		if (!PhysicsIsRunning()) return;
		FlushPhysicsSceneImpl();
		ApplyPhysics();
	}
	void EchoPhysicsManager::_latePhysicsUpdate() {
		if (!PhysicsIsRunning()) return;
		UpdateRigidBodyMove();
		UpdateSceneEvent();
		FlushPhysicsSceneImpl();
	}
	void EchoPhysicsManager::_postPhysicsUpdate() {
		UpdateComponentInEditor();
		if (!PhysicsIsRunning()) return;
		FlushPhysicsSceneImpl();
		GetScene()->FlushQueryUpdates();
		FlushSceneSimulationCache();
	}


	void EchoPhysicsManager::EditorPlay() {
		SetRunPhysics(true);
		SetUpdatePhysics(true);
	}
	void EchoPhysicsManager::EditorPause(bool pause) {
		SetUpdatePhysics(!pause);
	}
	void EchoPhysicsManager::EditorStop() {
		SetUpdatePhysics(false);
		SetRunPhysics(false);
		UpdateComponentInEditor();
	}

	void EchoPhysicsManager::SetRunPhysics(bool IsRun) {
		if (!IsExistScene()) return;
		if (mIsRunPhysics == IsRun) {
			return;
		}
		if (IsRun) {//开始运行
			StartPhysics();
		}
		else {//结束运行
			StopPhysics();
		}
		mIsRunPhysics = IsRun;
	}

	void EchoPhysicsManager::SetUpdatePhysics(bool IsUpdate) {
		if (!IsExistScene()) return;
		GetScene()->SetIsSimulation(IsUpdate);
	}
	bool EchoPhysicsManager::PhysicsIsRunning() const {
		return mIsRunPhysics;
	}
	bool EchoPhysicsManager::PhysicsIsUpdating() const {
		if (GetScene() == nullptr) return false;
		return GetScene()->GetIsSimulation();
	}
	void EchoPhysicsManager::StartPhysics() {
		if (!IsExistScene()) return;

#ifdef USE_PHYSICS_PAGE
		m_pGraph->Clear();
		for (auto& iter : mPagedLayers) {
			iter.second.clear();
		}
#endif // USE_PHYSICS_PAGE

		ClearAllUpdateCacheQueue();
		UpdatePhysXPage();
		UpdatePhysXWorldOrigin();

		for (auto pComp : m_ColliderList) {
			pComp->CreatePhysXObject();
		}

		for (auto pComp : m_RigidBodyList) {
			pComp->CreatePhysXObject();
		}

		for (auto pComp : m_JointList) {
			pComp->CreatePhysXObject();
		}

		for (auto pComp : m_AggregateList) {
			pComp->AddToPhysicsUpdateQueue();
		}
		for (auto pComp : m_ControllerList) {
			pComp->AddToPhysicsUpdateQueue();
		}
		for (auto pComp : m_ForceList) {
			pComp->AddToPhysicsUpdateQueue();
		}
		for (auto pComp : m_TerrainColliderList) {
			pComp->AddToPhysicsUpdateQueue();
		}

		UpdatePhysXPage();
		UpdateSphericalTerrain();

		GetScene()->AddCallback(mPhysXSceneCallBack);
	}
	void EchoPhysicsManager::StopPhysics() {
		if (!IsExistScene()) return;

		ClearSphericalTerrainPhysics();

		for (int i = 0; i < (int)mPhysicsUpdateQueue.size(); ++i) {
			mPhysicsUpdateQueue[i].resize(0);
		}

#ifdef USE_PHYSICS_PAGE
		m_pGraph->Clear();
		for (auto& iter : mPagedLayers) {
			iter.second.clearAsyncLoadPhysXObject();
		}
#endif // USE_PHYSICS_PAGE

		for (auto pComp : m_ColliderList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
#ifdef USE_PHYSICS_PAGE
			SAFE_DELETE(pComp->mPhysXObject);
#endif // USE_PHYSICS_PAGE
			pComp->SetIsUsePhysics(false);
		}

		for (auto pComp : m_AggregateList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
		}

		//销毁全部动态刚体
		for (auto pComp : m_RigidBodyList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
#ifdef USE_PHYSICS_PAGE
			SAFE_DELETE(pComp->mPhysXObject);
#endif // USE_PHYSICS_PAGE
			pComp->SetIsUsePhysics(false);
		}

		//销毁全部Joint
		for (auto pComp : m_JointList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
			pComp->SetIsUsePhysics(false);
		}

		//销毁全部Force
		for (auto pComp : m_ForceList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
		}

		for (auto pComp : m_TerrainColliderList) {
			pComp->m_bUpdatePhysics = false;
			pComp->ReleasePhysics();
		}

		for (auto pComp : m_ControllerList) {
			pComp->m_bUpdatePhysics = false;
		}

#ifdef USE_PHYSICS_PAGE
		ClearPhysXObjectBoundQueue();
		for (auto& iter : mPagedLayers) {
			iter.second.clear();
		}
#endif // USE_PHYSICS_PAGE

		ClearAllUpdateCacheQueue();
		GetScene()->RemoveCallback(mPhysXSceneCallBack);
	}

	void EchoPhysicsManager::UpdateComponentInEditor() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (mEditorUpdateQueue.empty()) return;

		const size_t frameMaxSize = 2500;

		//复制更新队列
		size_t curSize = mEditorUpdateQueue.size();
		std::vector<PhysicsBaseComponent*> curUpdateQueue;

		if (curSize <= frameMaxSize) {
			curUpdateQueue.reserve(curSize);
			curUpdateQueue.swap(mEditorUpdateQueue);
		}
		else {
			curUpdateQueue.assign(mEditorUpdateQueue.begin() + frameMaxSize, mEditorUpdateQueue.end());
			mEditorUpdateQueue.resize(frameMaxSize);
			curUpdateQueue.swap(mEditorUpdateQueue);
		}
		for (PhysicsBaseComponent* pComp : curUpdateQueue) {
			if (pComp == nullptr) {
				continue;
			}
			pComp->m_bUpdateEditor = false;
			//组件可能会在接口内把自己移入队列
			pComp->UpdateComponentInEditor();
		}
		curUpdateQueue.clear();
#endif // USE_PHYSICS_HELPEROBJ
	}
	void EchoPhysicsManager::UpdatePhysics() {

		for (int i = 0; i < (int)mPhysicsUpdateQueue.size(); ++i) {
			if (mPhysicsUpdateQueue[i].empty()) continue;
			std::vector<PhysicsBaseComponent*> curUpdateQueue;
			curUpdateQueue.swap(mPhysicsUpdateQueue[i]);
			for (PhysicsBaseComponent* pComp : curUpdateQueue) {
				if (pComp == nullptr) {
					continue;
				}
				pComp->m_bUpdatePhysics = false;

				if (pComp->IsExistPhysics()) {
					//LogManager::instance()->logMessage("-error-\t" + pComp->getCombinedName() + "StateError in EchoPhysicsSystem::UpdatePhysics");
				}
				else {
					if (!pComp->getRealVisibleState() || !pComp->IsCanPhysics()) {
						continue;
					}
					pComp->CreatePhysics();
					//assert(pComp->IsExistPhysics());
				}
			}
		}
	}

	void EchoPhysicsManager::UpdatePhysicsImpl(EchoPhysicsComponentType type) {
		int typeIndex = (int)type;
		if (typeIndex < 0 || typeIndex >= (int)EchoPhysicsComponentType::eNUMBER) return;
		if (mPhysicsUpdateQueue[typeIndex].empty()) return;
		std::vector<PhysicsBaseComponent*> curUpdateQueue;
		curUpdateQueue.swap(mPhysicsUpdateQueue[typeIndex]);
		for (PhysicsBaseComponent* pComp : curUpdateQueue) {
			if (pComp == nullptr) {
				continue;
			}
			pComp->m_bUpdatePhysics = false;

			if (pComp->IsExistPhysics()) {
				//LogManager::instance()->logMessage("-error-\t" + pComp->getCombinedName() + "StateError in EchoPhysicsSystem::UpdatePhysics");
			}
			else {
				if (!pComp->getRealVisibleState() || !pComp->IsCanPhysics()) {
					continue;
				}
				pComp->CreatePhysics();
				//assert(pComp->IsExistPhysics());
			}
		}
	}

	void EchoPhysicsManager::UpdateRigidBodyMove() {
		mIsRigidBodyMoveing = true;
		mCurMoveNumber += (int)mPhysXSceneCallBack->UpdateRigidBodyMove();
		mIsRigidBodyMoveing = false;
	}

	//void EchoPhysicsManager::PostUpdateRigidBodyMove() {
	//	mIsRigidBodyMoveing = true;
	//	mCurMoveNumber += (int)mPhysXSceneCallBack->PostUpdateRigidBodyMove();
	//	mIsRigidBodyMoveing = false;
	//}

	void EchoPhysicsManager::UpdateSceneEvent() {
		mCurCollisionNumber += mPhysXSceneCallBack->UpdateColliderCollision();
		mCurTriggerNumber += mPhysXSceneCallBack->UpdateColliderTrigger();
		mPhysXSceneCallBack->UpdateJointEvent();
		mPhysXSceneCallBack->UpdateRigidBodyWake();
		mPhysXSceneCallBack->UpdateRigidBodySleep();
	}

	void EchoPhysicsManager::AsyncUpdatePhysics() {

		int n = mAsyncUpdateNumber;

		for (auto& iter : mPagedLayers) {
			n -= iter.second.updateAsyncQueue(n);
			if (n < 1) {
				break;
			}
		}
	}

	void EchoPhysicsManager::ApplyPhysics() {
		if (!PhysicsIsUpdating()) return;

		for (auto pComp : m_ForceList) {
			if (!pComp->getRealVisibleState()) continue;
			if (!pComp->IsExistPhysics()) {
				pComp->CreatePhysics();
				continue;
			}
			pComp->ApplyPhysics();
		}

	}

	bool EchoPhysicsManager::IsExistScene() const {
		return (GetScene() != nullptr) && (GetScene()->GetPxScene() != nullptr);
	}
	const Name& EchoPhysicsManager::GetName() {
		return mName;
	}
	Camera* EchoPhysicsManager::GetCamera() {
		return GetPhysXWorldManager()->GetCamera();
	}
	SceneManager* EchoPhysicsManager::GetSceneManager() {
		return GetPhysXWorldManager()->GetSceneManager();
	}
	WorldManager* EchoPhysicsManager::GetWorldManager() {
		return WorldSystem::instance()->GetWorldManager(GetName());
	}
	ActorManager* EchoPhysicsManager::GetActorManager() {
		if (m_pActorManager == nullptr) {
			m_pActorManager = ActorSystem::instance()->getActorManager(GetName());
		}
		return m_pActorManager;
	}
	PhysXWorldManager* EchoPhysicsManager::GetPhysXWorldManager() {
		return m_pPhysXWorldManager;
	}
	PhysXSceneImpl* EchoPhysicsManager::GetScene() const {
		return m_pPhysXWorldManager != nullptr ? m_pPhysXWorldManager->GetPhysXScene() : nullptr;
	}
	PhysXSceneQuery* EchoPhysicsManager::GetPhysXSceneQuery() const {
		return GetScene();
	}

	Vector3 EchoPhysicsManager::GetGravity() {
		return GetScene()->GetGravity();
	}
	void EchoPhysicsManager::SetGravity(const Vector3& vec) {
		GetScene()->SetGravity(vec);
	}
	float EchoPhysicsManager::GetSimulationTime() {
		return GetScene()->GetSimulationTime();
	}

	int EchoPhysicsManager::GetCurrentMoveNumber() {
		return mCurMoveNumber;
	}

	int EchoPhysicsManager::GetCurrentCollisionNumber() {
		return mCurCollisionNumber;
	}
	int EchoPhysicsManager::GetCurrentTriggerNumber() {
		return mCurTriggerNumber;
	}

	void EchoPhysicsManager::SetAsyncLoadNumber(int num) {
		mAsyncUpdateNumber = Math::Clamp(num, 0, 10000);
	}
	int EchoPhysicsManager::GetAsyncLoadNumber() {
		return mAsyncUpdateNumber;
	}

	void EchoPhysicsManager::SetDefaultControllerBehavior(uint32 flag) {
		((EchoPhysicsControllerCallBack*)mPhysXCCTCallBack)->mCCTBehaviorFlag = (ControllerBehaviorFlag)flag;
	}
	uint32 EchoPhysicsManager::GetDefaultControllerBehavior() {
		return (uint32)((EchoPhysicsControllerCallBack*)mPhysXCCTCallBack)->mCCTBehaviorFlag;
	}

	//bool EchoPhysicsManager::GetIsResetCCT() {
	//	return mIsResetCCT;
	//}
	//void EchoPhysicsManager::SetIsResetCCT(bool value) {
	//	mIsResetCCT = value;
	//}

	void EchoPhysicsManager::SetControllerOverlapRecoveryModule(bool value) {
		GetScene()->SetControllerOverlapRecoveryModule(value);
	}
	void EchoPhysicsManager::SetControllerTessellation(bool flag, float maxEdgeLength) {
		GetScene()->SetControllerTessellation(flag, maxEdgeLength);
	}
	bool EchoPhysicsManager::GetControllerOverlapRecoveryModule() {
		return GetScene()->GetControllerOverlapRecoveryModule();
	}
	std::pair<bool, float> EchoPhysicsManager::GetControllerTessellation() {
		return GetScene()->GetControllerTessellation();
	}

	void EchoPhysicsManager::SetControllerPreciseSweeps(bool flag) {
		GetScene()->SetControllerPreciseSweeps(flag);
	}
	bool EchoPhysicsManager::GetControllerPreciseSweeps() {
		return GetScene()->GetControllerPreciseSweeps();
	}
	void EchoPhysicsManager::SetControllerPreventVerticalSlidingAgainstCeiling(bool flag) {
		GetScene()->SetControllerPreventVerticalSlidingAgainstCeiling(flag);
	}
	bool EchoPhysicsManager::GetControllerPreventVerticalSlidingAgainstCeiling() {
		return GetScene()->GetControllerPreventVerticalSlidingAgainstCeiling();
	}

	void EchoPhysicsManager::SetMeshDoubleSided(bool flag) {
		GetScene()->SetMeshDoubleSided(flag);
	}
	bool EchoPhysicsManager::GetMeshDoubleSided() {
		return GetScene()->GetMeshDoubleSided();
	}

	void EchoPhysicsManager::SetControllerIsSyncGlobalPose(bool value) {
		GetScene()->SetControllerIsSyncGlobalPose(value);
	}
	bool EchoPhysicsManager::GetControllerIsSyncGlobalPose() {
		return GetScene()->GetControllerIsSyncGlobalPose();
	}
	void EchoPhysicsManager::SetControllerShapeScale(float value) {
		mCCTShapeScale = value;
	}
	float EchoPhysicsManager::GetControllerShapeScale() {
		return mCCTShapeScale;
	}

	bool EchoPhysicsManager::AddToPhysicsUpdateQueue(PhysicsBaseComponent* pComp) {
		if (pComp == nullptr || pComp->m_bUpdatePhysics) return false;
		int type = (int)pComp->GetPhysicsType();
		mPhysicsUpdateQueue[type].push_back(pComp);
		pComp->m_bUpdatePhysics = true;
		return true;
	}
	bool EchoPhysicsManager::RemoveFromPhysicsUpdateQueue(PhysicsBaseComponent* pComp) {
		if (pComp == nullptr || !pComp->m_bUpdatePhysics) return false;
		int type = (int)pComp->GetPhysicsType();
		for (auto& comp : mPhysicsUpdateQueue[type]) {
			if (comp == pComp) {
				comp = nullptr;
				break;
			}
		}
		pComp->m_bUpdatePhysics = false;
		return true;
	}
#ifdef USE_PHYSICS_HELPEROBJ
	bool EchoPhysicsManager::AddToEditorUpdateQueue(PhysicsBaseComponent* pComp) {
		if (pComp == nullptr || pComp->m_bUpdateEditor) return false;
		mEditorUpdateQueue.push_back(pComp);
		pComp->m_bUpdateEditor = true;
		return true;
	}
	bool EchoPhysicsManager::RemoveFromEditorUpdateQueue(PhysicsBaseComponent* pComp) {
		if (pComp == nullptr || !pComp->m_bUpdateEditor) return false;
		for (auto& comp : mEditorUpdateQueue) {
			if (comp == pComp) {
				comp = nullptr;
				break;
			}
		}
		pComp->m_bUpdateEditor = false;
		return true;
	}
#endif // USE_PHYSICS_HELPEROBJ

	bool EchoPhysicsManager::AddPhysicsComponent(PhysicsBaseComponent* pComponent) {
		if (!pComponent) return false;
		EchoPhysicsComponentType physicsType = pComponent->GetPhysicsType();

		switch (physicsType) {
		case EchoPhysicsComponentType::eRIGIDBODY: { 
			RigidBodyComponent* pRigid = (RigidBodyComponent*)pComponent;
			m_RigidBodyList.insert(pRigid);
			break; 
		}
		case EchoPhysicsComponentType::eCOLLIDER: { 
			ColliderComponent* pCollider = (ColliderComponent*)pComponent;
			m_ColliderList.insert(pCollider);
#ifndef ECHO_EDITOR
			if (mIsShowCollider) {
				pCollider->SetHelperVisible(true);
			}
#endif // !ECHO_EDITOR
			break; 
		}
		case EchoPhysicsComponentType::eJOINT: { 
			JointComponent* pJoint = (JointComponent*)pComponent;
			m_JointList.insert(pJoint);
#ifndef ECHO_EDITOR
			if (mIsShowJoint) {
				pJoint->SetHelperVisible(true);
			}
#endif // !ECHO_EDITOR
			break; 
		}
		case EchoPhysicsComponentType::eFORCE: { 
			m_ForceList.insert((ConstantForceComponent*)pComponent);
			break; 
		}
		case EchoPhysicsComponentType::eCONTROLLER: { 
			CharatorControllerComponent* pCCT = (CharatorControllerComponent*)pComponent;
			m_ControllerList.insert(pCCT);
#ifndef ECHO_EDITOR
			if (mIsShowCCT) {
				pCCT->SetHelperVisible(true);
			}
#endif // !ECHO_EDITOR
			break; 
		}
		case EchoPhysicsComponentType::eAGGREGATE: { 
			m_AggregateList.insert((AggregateComponent*)pComponent);
			break; 
		}
		case EchoPhysicsComponentType::eTERRAIN: { 
			m_TerrainColliderList.insert((TerrainColliderComponent*)pComponent);
			break; 
		}
		case EchoPhysicsComponentType::eNUMBER: { 
			LogManager::instance()->logMessage("-error-\tPhysicsComponent InvalidType [" + pComponent->getCombinedName() + "]");
			assert(false);
			break;
		}
		}

		return true;
	}
	bool EchoPhysicsManager::RemovePhysicsComponent(PhysicsBaseComponent* pComponent) {
		if (!pComponent) return false;
		EchoPhysicsComponentType physicsType = pComponent->GetPhysicsType();

		switch (physicsType) {
			case EchoPhysicsComponentType::eRIGIDBODY: { m_RigidBodyList.erase((RigidBodyComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eCOLLIDER: { m_ColliderList.erase((ColliderComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eJOINT: { m_JointList.erase((JointComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eFORCE: { m_ForceList.erase((ConstantForceComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eCONTROLLER: { m_ControllerList.erase((CharatorControllerComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eAGGREGATE: { m_AggregateList.erase((AggregateComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eTERRAIN: { m_TerrainColliderList.erase((TerrainColliderComponent*)pComponent); break; }
			case EchoPhysicsComponentType::eNUMBER: { 
				LogManager::instance()->logMessage("-error-\tPhysicsComponent InvalidType [" + pComponent->getCombinedName() + "]");
				assert(false);
				break; 
			}
		}

		if (PhysicsIsRunning()) {
			pComponent->ReleasePhysics();
		}
		return true;
	}

	bool EchoPhysicsManager::AddSphericalTerrain(GeneratedTerrainPhysics* pComp) {
		if (pComp == nullptr) return false;
		m_SphericalTerrainList.insert(pComp);
		return true;
	}
	bool EchoPhysicsManager::RemoveSphericalTerrain(GeneratedTerrainPhysics* pComp) {
		if (pComp == nullptr) return false;
		m_SphericalTerrainList.erase(pComp);
		return true;
	}
	bool EchoPhysicsManager::AddMathCharacterController(MathCharacterController* pComp) {
		if (pComp == nullptr) return false;
		m_MathControllerList.insert(pComp);
		pComp->SetHelperVisible(mIsShowCCT);
		return true;
	}
	bool EchoPhysicsManager::RemoveMathCharacterController(MathCharacterController* pComp) {
		if (pComp == nullptr) return false;
		m_MathControllerList.erase(pComp);
		return true;
	}
	void EchoPhysicsManager::SetAllJointIsUpdateHelper(bool value) {
#ifdef USE_PHYSICS_HELPEROBJ
		if (value) {
			for (auto pComp : m_JointList) {
				pComp->AddToEditorUpdateQueue();
			}
		}
		else {
			for (auto pComp : m_JointList) {
				pComp->RemoveFromEditorUpdateQueue();
			}
		}
#endif // USE_PHYSICS_HELPEROBJ
	}

	void EchoPhysicsManager::ResetAllColliderColor() {
		for (auto pComp : m_ColliderList) {
			pComp->ResetHelperColor();
		}
		for (auto pComp : m_TerrainColliderList) {
			pComp->ResetHelperColor();
		}
		for (auto pComp : m_ControllerList) {
			pComp->ResetHelperColor();
		}
		for (auto pComp : m_MathControllerList) {
			pComp->ResetHelperColor();
		}
	}

	//void EchoPhysicsManager::ResetAllCCTWorldPosition() {
	//	if (m_ControllerList.empty() || !mIsResetCCT) return;
	//
	//	for (auto pComp : m_ControllerList) {
	//		pComp->ResetCCTWorldPosition();
	//	}
	//}

	void EchoPhysicsManager::GetSelfLinkCompList(PhysicsActorComponent* pComp, PhysicsLinkComponentPtrList*& pJoints) {
		auto iter = mAllActorLinkCompListMap.find(pComp);
		if (iter != mAllActorLinkCompListMap.end()) {
			pJoints = &(iter->second);
		}
		else {
			pJoints = nullptr;
		}
	}

	PhysicsLinkComponentPtrList& EchoPhysicsManager::GetSelfLinkCompList(PhysicsActorComponent* pComp) {
		return mAllActorLinkCompListMap[pComp];
	}

	void EchoPhysicsManager::ClearSelfLinkCompList(PhysicsActorComponent* pComp) {
		auto iter = mAllActorLinkCompListMap.find(pComp);
		if (iter != mAllActorLinkCompListMap.end()) {
			iter->second.clear();
			mAllActorLinkCompListMap.erase(iter);
		}
	}

	void EchoPhysicsManager::AddTerrainColliderComponent(Index index) {
		RemoveTerrainColliderComponent(index);
		ActorPtr mainActor = GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			GetActorManager()->createMainActor();
			mainActor = GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}

		std::ostringstream oss;
		oss << index.x << "x" << index.z;
		String name(oss.str());
		TerrainColliderComponentPtr comp = mainActor->AddComponent<TerrainColliderComponent>(name, mainActor->getRootNodeCom());
		comp->SetBlockTerrainIndex(index);
		m_TerrainColliderPtrMap.insert(std::make_pair(index, comp));
	}
	void EchoPhysicsManager::RemoveTerrainColliderComponent(Index index) {
		auto iter = m_TerrainColliderPtrMap.find(index);
		if (iter == m_TerrainColliderPtrMap.end()) {
			return;
		}
		auto& pComp = iter->second;
		ActorPtr mAct = pComp->getActor();
		if (pComp && mAct)
		{
			mAct->delComponent(pComp->weak_from_this());
		}
		m_TerrainColliderPtrMap.erase(iter);
	}

	void EchoPhysicsManager::CreateTerrainColliderComponent() {
		DestroyTerrainColliderComponent();
		ActorPtr mainActor = GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			GetActorManager()->createMainActor();
			mainActor = GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}
		std::vector<Index> indexs;
		m_pPhysXWorldManager->GetAllBlockTerrainIndex(indexs);
		if (indexs.empty() || !m_TerrainColliderPtrMap.empty()) {
			return;
		}
		for (auto& index : indexs) {
			std::ostringstream oss;
			oss << index.x << "x" << index.z;
			String name(oss.str());
			TerrainColliderComponentPtr comp = mainActor->AddComponent<TerrainColliderComponent>(name, mainActor->getRootNodeCom());
			comp->SetBlockTerrainIndex(index);
			m_TerrainColliderPtrMap.insert(std::make_pair(index, comp));
		}
	}
	void EchoPhysicsManager::DestroyTerrainColliderComponent() {
		for (auto& iter : m_TerrainColliderPtrMap) {
			auto& pComp = iter.second;
			ActorPtr mAct = pComp->getActor();
			if (pComp && mAct)
			{
				mAct->delComponent(pComp->weak_from_this());
			}
		}
		m_TerrainColliderPtrMap.clear();
	}
	void EchoPhysicsManager::CreateSurfaceTypeCollider() {
		if (!m_SurfaceTypeColliderPtrMap.empty()) return;
		ActorPtr mainActor = GetActorManager()->mMainActor;
		if (mainActor == nullptr) {
			GetActorManager()->createMainActor();
			mainActor = GetActorManager()->mMainActor;
		}
		if (mainActor == nullptr) {
			return;
		}
		for (uint32 i = 0; i < (uint32)SurfaceType::Count; ++i) {
			std::ostringstream oss;
			oss << "SurfaceType" << i;
			String name(oss.str());
			TerrainColliderComponentPtr comp = mainActor->AddComponent<TerrainColliderComponent>(name, mainActor->getRootNodeCom());
			comp->SetSurfaceType(i);
			m_SurfaceTypeColliderPtrMap[i] = comp;
		}
		m_pPhysXWorldManager->SetInternalPhysicsColliders(m_SurfaceTypeColliderPtrMap);
	}
	void EchoPhysicsManager::AddCallBack(const EchoPhysicsManagerCallBackPtr& callback) {
		m_CallBackList.push_back(callback);
	}
	void EchoPhysicsManager::RemoveCallBack(const EchoPhysicsManagerCallBackPtr& callback) {
		auto iter = std::find(m_CallBackList.begin(), m_CallBackList.end(), callback);
		if (iter != m_CallBackList.end()) {
			m_CallBackList.erase(iter);
		}
	}

	void EchoPhysicsManager::SendRigidBodyCreate(const RigidBodyComponentPtr& _compPtr) {
		for (auto& callback : m_CallBackList) {
			if (callback != nullptr) {
				callback->OnCreate(_compPtr);
			}
		}
	}
	void EchoPhysicsManager::SendRigidBodyRelease(const RigidBodyComponentPtr& _compPtr) {
		for (auto& callback : m_CallBackList) {
			if (callback != nullptr) {
				callback->OnRelease(_compPtr);
			}
		}
	}

	void EchoPhysicsManager::SendRigidBodyWake(const RigidBodyComponentPtr& _compPtr) {
		for (auto& callback : m_CallBackList) {
			if (callback != nullptr) {
				callback->OnWake(_compPtr);
			}
		}
	}
	void EchoPhysicsManager::SendRigidBodySleep(const RigidBodyComponentPtr& _compPtr) {
		for (auto& callback : m_CallBackList) {
			if (callback != nullptr) {
				callback->OnSleep(_compPtr);
			}
		}
	}

	bool EchoPhysicsManager::ExportStaticPhysics(cJSON* root) {
		if (m_ColliderList.empty()) {
			return false;
		}
		cJSON* colliderCollection = cJSON_CreateObject();
		cJSON_AddItemToObject(root, "vecCollider", colliderCollection);

		cJSON* colliderVec_box = nullptr;
		cJSON* colliderVec_sphere = nullptr;
		cJSON* colliderVec_capsule = nullptr;
		cJSON* colliderVec_convexMesh = nullptr;
		cJSON* colliderVec_triangleMesh = nullptr;

		for (ColliderComponent* comp : m_ColliderList) {

			cJSON* colliderNode = cJSON_CreateObject();

			WriteJSNode(colliderNode, "name", comp->getCombinedName());
			WriteJSNode(colliderNode, "surfaceType", comp->GetSurfaceType());
			WriteJSNode(colliderNode, "position", Vector3(comp->getWorldPosition()));
			WriteJSNode(colliderNode, "rotation", comp->getWorldRotation());

			const Name& type = comp->getType();
			if (type == EchoPhysicsManager::gColliderTypesVec[0]) {
				if (colliderVec_box == nullptr) {
					colliderVec_box = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Box", colliderVec_box);
				}
				BoxColliderComponent* collider = (BoxColliderComponent*)comp;
				WriteJSNode(colliderNode, "halfSize", collider->GetRealHalfSize());
				cJSON_AddItemToArray(colliderVec_box, colliderNode);
			}
			else if (type == EchoPhysicsManager::gColliderTypesVec[3] || type == EchoPhysicsManager::gColliderTypesVec[4]) {
				MeshColliderComponent* collider = (MeshColliderComponent*)comp;
				WriteJSNode(colliderNode, "scale", comp->getWorldScale());
				WriteJSNode(colliderNode, "file", collider->GetMeshName());
				if (collider->GetIsConvex()) {
					if (colliderVec_convexMesh == nullptr) {
						colliderVec_convexMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "ConvexMesh", colliderVec_convexMesh);
					}
					cJSON_AddItemToArray(colliderVec_convexMesh, colliderNode);
				}
				else {
					if (colliderVec_triangleMesh == nullptr) {
						colliderVec_triangleMesh = cJSON_CreateArray();
						cJSON_AddItemToObject(colliderCollection, "TriangleMesh", colliderVec_triangleMesh);
					}
					cJSON_AddItemToArray(colliderVec_triangleMesh, colliderNode);
				}
			}
			else if (type == EchoPhysicsManager::gColliderTypesVec[1]) {
				if (colliderVec_sphere == nullptr) {
					colliderVec_sphere = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Sphere", colliderVec_sphere);
				}
				SphereColliderComponent* collider = (SphereColliderComponent*)comp;
				WriteJSNode(colliderNode, "radius", collider->GetRealRadius());
				cJSON_AddItemToArray(colliderVec_sphere, colliderNode);
			}
			else if (type == EchoPhysicsManager::gColliderTypesVec[2]) {
				if (colliderVec_capsule == nullptr) {
					colliderVec_capsule = cJSON_CreateArray();
					cJSON_AddItemToObject(colliderCollection, "Capsule", colliderVec_capsule);
				}
				CapsuleColliderComponent* collider = (CapsuleColliderComponent*)comp;
				float radius, halfHeight;
				collider->GetRealRadiusHalfHeight(radius, halfHeight);
				WriteJSNode(colliderNode, "radius", radius);
				WriteJSNode(colliderNode, "halfHeight", halfHeight);
				cJSON_AddItemToArray(colliderVec_capsule, colliderNode);
			}
		}

		return true;
	}

	void EchoPhysicsManager::SetPhysXPageLoadMainActor(const ActorComponentPtr& _compPtr, bool bForceAsync) {
		SetPhysXPageLoadMainActor(0, _compPtr, bForceAsync);
	}

	const ActorComponentPtr& EchoPhysicsManager::GetPhysXPageLoadMainActor() {
		return mPhysXPageLoadMainActors[0].m_pCompPtr;
	}
	int EchoPhysicsManager::GetPhysXPageLoadMainActorNumber() {
		return (int)mPhysXPageLoadMainActors.size();
	}
	PhysXPageLoadPointData EchoPhysicsManager::GetPhysXPageLoadMainActor(int index) {
		if (index > -1 && index < (int)mPhysXPageLoadMainActors.size()) {
			return mPhysXPageLoadMainActors[index];
		}
		else {
			return PhysXPageLoadPointData();
		}
	}
	void EchoPhysicsManager::SetPhysXPageLoadMainActor(int index, const ActorComponentPtr& _compPtr, bool bForceAsync) {
		if (index > -1 && index < (int)mPhysXPageLoadMainActors.size()) {
			Vector3 position = (_compPtr == nullptr) ? (Vector3)GetCamera()->getPosition() : (Vector3)_compPtr->getWorldPosition();
			mPhysXPageLoadMainActors[index].m_pCompPtr = _compPtr;
			mPhysXPageLoadMainActors[index].m_bForceAsync = bForceAsync;
			mPhysXPageLoadMainActors[index].m_vLastPosition = position;
			for (auto& iter : mPagedLayers) {
				iter.second.setTarget(index, position, bForceAsync);
			}
		}
	}
	void EchoPhysicsManager::AddPhysXPageLoadMainActor(const ActorComponentPtr& _compPtr, bool bForceAsync) {
		Vector3 pos = (_compPtr == nullptr) ? (Vector3)GetCamera()->getPosition() : (Vector3)_compPtr->getWorldPosition();
		mPhysXPageLoadMainActors.push_back(PhysXPageLoadPointData(_compPtr, pos, bForceAsync));
		for (auto& iter : mPagedLayers) {
			iter.second.addTarget(pos, bForceAsync);
		}
	}
	void EchoPhysicsManager::RemovePhysXPageLoadMainActor(const ActorComponentPtr& _compPtr) {
		int size = (int)mPhysXPageLoadMainActors.size();
		if (size < 2) return;
		int index = -1;
		for (int i = 1; i != size; ++i) {
			if (mPhysXPageLoadMainActors[i].m_pCompPtr == _compPtr) {
				index = i;
			}
		}
		if (index != -1) {
			mPhysXPageLoadMainActors[index] = mPhysXPageLoadMainActors[size - 1];
			mPhysXPageLoadMainActors.pop_back();
			for (auto& iter : mPagedLayers) {
				iter.second.removeTarget(index);
			}
		}
	}
	void EchoPhysicsManager::RemovePhysXPageLoadMainActor(int index) {
		int size = (int)mPhysXPageLoadMainActors.size();
		if (size < 2 || index < 1 || index >= size) return;
		mPhysXPageLoadMainActors[index] = mPhysXPageLoadMainActors[size - 1];
		mPhysXPageLoadMainActors.pop_back();
		for (auto& iter : mPagedLayers) {
			iter.second.removeTarget(index);
		}
	}

	void EchoPhysicsManager::updateEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject) {
		if (pEchoPhysXObject == nullptr) return;
		mPagedLayers[pEchoPhysXObject->getPhysXType()].updateEchoPhysXObject(pEchoPhysXObject);
	}
	void EchoPhysicsManager::addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject) {
		if (pEchoPhysXObject == nullptr) return;
		mPagedLayers[pEchoPhysXObject->getPhysXType()].addEchoPhysXObject(pEchoPhysXObject);
	}
	void EchoPhysicsManager::removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject) {
		if (pEchoPhysXObject == nullptr) return;
		RemovePhysXObjectFromUpdateQueue(pEchoPhysXObject);
		PhysicsPagedLayerManager& mLMgr = mPagedLayers[pEchoPhysXObject->getPhysXType()];
		mLMgr.removeAsyncLoadPhysXObject(pEchoPhysXObject);
		mLMgr.removeEchoPhysXObject(pEchoPhysXObject);
	}

	void EchoPhysicsManager::UpdatePhysXPage() {
#ifdef USE_PHYSICS_PAGE
		if (mIsUsePhysXPage) {
			m_pGraph->CreateGroup();
			UpdatePhysXObjectBoundQueue();

			for (int i = 0; i != mPhysXPageLoadMainActors.size(); ++i) {
				mPhysXPageLoadMainActors[i].m_vLastPosition = (mPhysXPageLoadMainActors[i].m_pCompPtr == nullptr) ? (Vector3)GetCamera()->getPosition() : (Vector3)mPhysXPageLoadMainActors[i].m_pCompPtr->getWorldPosition();
			}
			for (auto& iter : mPagedLayers) {
				iter.second.update(mPhysXPageLoadMainActors);
			}
		}
#endif //USE_PHYSICS_PAGE
	}

	void EchoPhysicsManager::UpdateSphericalTerrain() {
		if (m_SphericalTerrainList.empty()) return;

		if (mIsUsePhysXPage) {
			Vector3 position = mPhysXPageLoadMainActors[0].m_vLastPosition;
			UpdatePredictVelocity(position);
			Vector3 predictDireciton = m_PredictVelocity.GetNormalized();
			for (auto pComp : m_SphericalTerrainList) {
				pComp->UpdatePhysics(position, predictDireciton, m_PredictSpeed, mPhysXPageLoadMainActors[0].m_bForceAsync);
			}
		}
		else {
			for (auto pComp : m_SphericalTerrainList) {
				if (!pComp->IsExistPhysics()) {
					pComp->CreatePhysics();
				}
				pComp->UpdatePhysics();
			}
		}
	}

	void EchoPhysicsManager::ClearSphericalTerrainPhysics() {
		if (m_SphericalTerrainList.empty()) return;
		for (auto pComp : m_SphericalTerrainList) {
			pComp->ClearPhysics();
		}
	}

	void EchoPhysicsManager::UpdatePredictVelocity(const Vector3& position) {
		Vector3 offset = position - m_PredictLastPosition;
		m_PredictLastPosition = position;
		float offsetLength = offset.Length();
		if (offsetLength > 300.0f) {
			m_SumOffset = Vector3::ZERO;
			m_PredictVelocity = Vector3::ZERO;
			m_PredictSpeed = 0.0f;
			m_OffsetCaches[(m_OffsetCacheIndex + 1) % m_OffsetCaches.size()] = Vector3::ZERO;
		}
		else {
			m_OffsetCacheIndex = (m_OffsetCacheIndex + 1) % m_OffsetCaches.size();			m_SumOffset -= m_OffsetCaches[m_OffsetCacheIndex];			m_OffsetCaches[m_OffsetCacheIndex] = offset;			m_SumOffset += m_OffsetCaches[m_OffsetCacheIndex];			m_PredictVelocity = m_SumOffset.GetNormalized();			m_PredictSpeed = (offsetLength / EchoIK::Time::deltaTime) * 0.1f + m_PredictSpeed * 0.8f;
		}
	}

	void EchoPhysicsManager::UpdatePhysXWorldOrigin() {
#ifdef PHYSX_USE_WORLDORIGIN
		DVector3 newOrigin = mPhysXPageLoadMainActors[0].m_vLastPosition;
		if (newOrigin.squaredDistance(GetScene()->GetPhysXWorldOrigin()) > 10000000.0f) {
			GetScene()->SetPhysXWorldOrigin(DVector3(floor(newOrigin.x), floor(newOrigin.y), floor(newOrigin.z)));
		}
#endif // PHYSX_USE_WORLDORIGIN
	}

	void EchoPhysicsManager::resetCellSize(float cellSize) {
		if (!PhysicsIsRunning()) {
			for (auto& iter : mPagedLayers) {
				iter.second.setCellSize(cellSize);
			}
		}
		else {
			StopPhysics();
			for (auto& iter : mPagedLayers) {
				iter.second.setCellSize(cellSize);
			}
			StartPhysics();
		}
	}
	void EchoPhysicsManager::setUsePhysXPage(bool value) {
		if (mIsUsePhysXPage == value) return;
		if (!PhysicsIsRunning()) {
			mIsUsePhysXPage = value;
		}
		else {
			StopPhysics();
			mIsUsePhysXPage = value;
			StartPhysics();
		}
	}
	void EchoPhysicsManager::setCellSize(float cellSize, EchoPhysXObjectType type) {
		if (PhysicsIsRunning()) return;
		auto iter = mPagedLayers.find(type);
		if (iter != mPagedLayers.end()) {
			iter->second.setCellSize(cellSize);
		}
	}
	float EchoPhysicsManager::getCellSize(EchoPhysXObjectType type) {
		auto iter = mPagedLayers.find(type);
		if (iter != mPagedLayers.end()) {
			return iter->second.getCellSize();
		}
		return 0.0f;
	}

	PhysicsPagedLayerManager* EchoPhysicsManager::getMapPagedLayers(EchoPhysXObjectType type) {
		auto iter = mPagedLayers.find(type);
		if (iter != mPagedLayers.end()) {
			return &(iter->second);
		}
		return nullptr;
	}

	void EchoPhysicsManager::calculatePagedLayersData(std::map<float, uint32>& datas, EchoPhysXObjectType type) {
		datas.clear();
		auto pageIter = mPagedLayers.find(type);
		if (pageIter == mPagedLayers.end()) {
			return;
		}
		const std::map<uint16, PhysicsPagedLayer*>& m_mapPagedLayers = pageIter->second.getMapPagedLayers();
		for (auto& iter : m_mapPagedLayers) {
			uint32 count = 0;
			for (auto& iter2 : iter.second->m_mapPhysicsPages) {
				count += (uint32)iter2.second->m_setEchoPhysXObjects.size();
			}

			datas[iter.second->m_cellSize] = count;
		}
	}

	void EchoPhysicsManager::SetColliderHelperVisible(bool value) {
#ifndef ECHO_EDITOR
		if (mIsShowCollider == value) return;
		for (auto pComp : m_ColliderList) {
			pComp->SetHelperVisible(value);
		}
#else
		for (const auto& type : gColliderTypesVec) {
			auto pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(type);
			pFactory->EditorShowHelper(value);
		}
#endif // !ECHO_EDITOR
		mIsShowCollider = value;
	}
	void EchoPhysicsManager::SetJointHelperVisible(bool value) {
#ifndef ECHO_EDITOR
		if (mIsShowJoint == value) return;
		for (auto pComp : m_JointList) {
			pComp->SetHelperVisible(value);
		}
#else
		auto pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(ConfigurableJointComponent));
		pFactory->EditorShowHelper(value);
		pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(FixedJointComponent));
		pFactory->EditorShowHelper(value);
#endif // !ECHO_EDITOR
		mIsShowJoint = value;
	}
	void EchoPhysicsManager::SetCCTHelperVisible(bool value) {
#ifndef ECHO_EDITOR
		if (mIsShowCCT == value) return;
		for (auto pComp : m_ControllerList) {
			pComp->SetHelperVisible(value);
		}
#else
		auto pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(CharatorControllerComponent));
		pFactory->EditorShowHelper(value);
#endif // !ECHO_EDITOR
		mIsShowCCT = value;
		for (auto pComp : m_MathControllerList) {
			pComp->SetHelperVisible(value);
		}
	}

	int EchoPhysicsManager::GetRigidBody_Exist() { 
		int size = 0;
		for (auto pComp : m_RigidBodyList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetCollider_Exist() { 
		int size = 0;
		for (auto pComp : m_ColliderList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetTerrain_Exist() { 
		int size = 0;
		for (auto pComp : m_TerrainColliderList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetJoint_Exist() { 
		int size = 0;
		for (auto pComp : m_JointList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetForce_Exist() { 
		int size = 0;
		for (auto pComp : m_ForceList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetController_Exist() { 
		int size = 0;
		for (auto pComp : m_ControllerList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetAggregate_Exist() { 
		int size = 0;
		for (auto pComp : m_AggregateList) {
			if (pComp->IsExistPhysics()) {
				++size;
			}
		}
		return size;
	}
	int EchoPhysicsManager::GetMathController_Exist() {
		return (int)m_MathControllerList.size();
	}
	void EchoPhysicsManager::FlushSceneSimulationCache() {
		if ((++mFlushSimulationCycleCount & 255U) != 0U) return;
		GetScene()->FlushSimulation();
		mFlushSimulationCycleCount = 0U;
	}

	void EchoPhysicsManager::ClearAllUpdateCacheQueue() {
		mHasUncommittedChanges = false;
		bool isLock = PhysXWorldSystem::instance()->PhysXJobLock(true);
		ClearRigidBodyMAITQueue();
		ClearColliderLocalPoseQueue();
		ClearRigidBodyPhysXTransformQueue();
		ClearColliderGeometryQueue();
		UpdatePhysXWaitDeleteQueue();
		if (isLock) PhysXWorldSystem::instance()->PhysXJobUnlock();
	}

	void EchoPhysicsManager::AddRigidBodyToUpdateMAITQueue(RigidBodyComponent* pComp) {
		if (pComp == nullptr || pComp->m_IsUpdateMAIT) return;
		pComp->m_IsUpdateMAIT = true;
		mUpdateMAITQueue.insert(pComp);
	}
	void EchoPhysicsManager::RemoveRigidBodyFromUpdateMAITQueue(RigidBodyComponent* pComp) {
		if (pComp == nullptr || !pComp->m_IsUpdateMAIT) return;
		mUpdateMAITQueue.erase(pComp);
		pComp->m_IsUpdateMAIT = false;
	}
	void EchoPhysicsManager::UpdateRigidBodyMAITQueue() {
		if ((++mUpdateMAITCycleCount & 15U) != 0U) return;
		if (mUpdateMAITQueue.empty()) return;
		for (RigidBodyComponent* ptr : mUpdateMAITQueue) {
			if (ptr != nullptr) {
				ptr->ResetMassAndInertiaTensorImpl();
				ptr->m_IsUpdateMAIT = false;
			}
		}
		mUpdateMAITQueue.clear();
		mUpdateMAITCycleCount = 0U;
	}

	void EchoPhysicsManager::ClearRigidBodyMAITQueue() {
		if (mUpdateMAITQueue.empty()) return;
		for (RigidBodyComponent* ptr : mUpdateMAITQueue) {
			if (ptr != nullptr) {
				ptr->m_IsUpdateMAIT = false;
			}
		}
		mUpdateMAITQueue.clear();
		mUpdateMAITCycleCount = 0U;
	}

	void EchoPhysicsManager::AddColliderToUpdateLocalPoseQueue(ColliderComponent* pComp) {
		if (pComp == nullptr || pComp->m_bPhysXTransformChanged) return;
		pComp->m_bPhysXTransformChanged = true;
		mUpdateLocalPoseQueue.insert(pComp);
		AppearUncommittedChanges();
	}
	void EchoPhysicsManager::RemoveColliderFromUpdateLocalPoseQueue(ColliderComponent* pComp) {
		if (pComp == nullptr || !pComp->m_bPhysXTransformChanged) return;
		mUpdateLocalPoseQueue.erase(pComp);
		pComp->m_bPhysXTransformChanged = false;
	}
	void EchoPhysicsManager::UpdateColliderLocalPoseQueue() {
		if (mUpdateLocalPoseQueue.empty()) return;
		for (ColliderComponent* ptr : mUpdateLocalPoseQueue) {
			if (ptr != nullptr) {
				ptr->ChangeLocalPoseImpl();//change mUpdateMAITQueue
				ptr->m_bPhysXTransformChanged = false;
			}
		}
		mUpdateLocalPoseQueue.clear();
	}
	void EchoPhysicsManager::ClearColliderLocalPoseQueue() {
		if (mUpdateLocalPoseQueue.empty()) return;
		for (ColliderComponent* ptr : mUpdateLocalPoseQueue) {
			if (ptr != nullptr) {
				ptr->m_bPhysXTransformChanged = false;
			}
		}
		mUpdateLocalPoseQueue.clear();
	}

	void EchoPhysicsManager::AddRigidBodyToUpdatePhysXTransformQueue(RigidBodyComponent* pComp) {
		if (pComp == nullptr || pComp->m_bPhysXTransformChanged) return;
		pComp->m_bPhysXTransformChanged = true;
		mUpdatePhysXTransformQueue.insert(pComp);
		AppearUncommittedChanges();
	}
	void EchoPhysicsManager::RemoveRigidBodyFromUpdatePhysXTransformQueue(RigidBodyComponent* pComp) {
		if (pComp == nullptr || !pComp->m_bPhysXTransformChanged) return;
		mUpdatePhysXTransformQueue.erase(pComp);
		pComp->m_bPhysXTransformChanged = false;
	}
	void EchoPhysicsManager::UpdateRigidBodyPhysXTransformQueue() {
		if (mUpdatePhysXTransformQueue.empty()) return;
		for (RigidBodyComponent* ptr : mUpdatePhysXTransformQueue) {
			if (ptr != nullptr) {
				ptr->SyncWorldTransformToPhysX();
				ptr->m_bPhysXTransformChanged = false;
			}
		}
		mUpdatePhysXTransformQueue.clear();
	}
	void EchoPhysicsManager::ClearRigidBodyPhysXTransformQueue() {
		if (mUpdatePhysXTransformQueue.empty()) return;
		for (RigidBodyComponent* ptr : mUpdatePhysXTransformQueue) {
			if (ptr != nullptr) {
				ptr->m_bPhysXTransformChanged = false;
			}
		}
		mUpdatePhysXTransformQueue.clear();
	}
	void EchoPhysicsManager::AddColliderToUpdateGeometry(ColliderComponent* pComp) {
		if (pComp == nullptr || pComp->m_bUpdateGeometry) return;
		pComp->m_bUpdateGeometry = true;
		mUpdateColliderGeometryQueue.insert(pComp);
		AppearUncommittedChanges();
	}
	void EchoPhysicsManager::RemoveColliderFromUpdateGeometry(ColliderComponent* pComp) {
		if (pComp == nullptr || !pComp->m_bUpdateGeometry) return;
		mUpdateColliderGeometryQueue.erase(pComp);
		pComp->m_bUpdateGeometry = false;
	}
	void EchoPhysicsManager::UpdateColliderGeometryQueue() {
		if (mUpdateColliderGeometryQueue.empty()) return;
		for (ColliderComponent* ptr : mUpdateColliderGeometryQueue) {
			if (ptr != nullptr) {
				ptr->ChangeGeometryImpl();
				ptr->m_bUpdateGeometry = false;
			}
		}
		mUpdateColliderGeometryQueue.clear();
	}
	void EchoPhysicsManager::ClearColliderGeometryQueue() {
		if (mUpdateColliderGeometryQueue.empty()) return;
		for (ColliderComponent* ptr : mUpdateColliderGeometryQueue) {
			if (ptr != nullptr) {
				ptr->m_bUpdateGeometry = false;
			}
		}
		mUpdateColliderGeometryQueue.clear();
	}
	PhysXShape* EchoPhysicsManager::CreatePhysXShape(EchoGeometry* geom, int matID) {
		return PhysXSystem::instance()->MakePhysXShape(geom, false, matID);
	}
	void EchoPhysicsManager::ReleasePhysXShape(PhysXShape* pShape) {
		if (pShape == nullptr) return;
		mPhysXShapeWaitDeleteQueue.push_back(pShape);
		pShape->SetUserType(PhysXUserTypeEnum::WaitDelete);
		pShape->SetFilterData(ePhysXGroup_None, ePhysXGroup_None, 0);
		AppearUncommittedChanges();
	}

	void EchoPhysicsManager::RemovePhysXActor(PhysXCharacterController* pRigid) {
		if (pRigid == nullptr) return;
		mPhysXActorWaitDeleteQueue.push_back(pRigid);
		pRigid->SetUserType(PhysXUserTypeEnum::WaitDelete);
		PhysXShape* pShape = pRigid->GetPhysXShape();
		if (pShape) {
			pShape->SetUserType(PhysXUserTypeEnum::WaitDelete);
			pShape->SetFilterData(ePhysXGroup_None, ePhysXGroup_None, 0);
		}
		AppearUncommittedChanges();
	}

	void EchoPhysicsManager::RemovePhysXActor(PhysXRigidDynamic* pRigid) {
		if (pRigid == nullptr) return;
		mPhysXActorWaitDeleteQueue.push_back(pRigid);
		pRigid->SetUserType(PhysXUserTypeEnum::WaitDelete);
		AppearUncommittedChanges();
	}
	void EchoPhysicsManager::RemovePhysXActor(PhysXStatic* pRigid) {
		if (pRigid == nullptr) return;
		mPhysXActorWaitDeleteQueue.push_back(pRigid);
		pRigid->SetUserType(PhysXUserTypeEnum::WaitDelete);
		AppearUncommittedChanges();
	}

	void EchoPhysicsManager::RemovePhysXAggregate(PhysXAggregate* pAggre) {
		if (pAggre == nullptr) return;
		mPhysXAggregateWaitDeleteQueue.push_back(pAggre);
		AppearUncommittedChanges();
	}

	void EchoPhysicsManager::UpdatePhysXWaitDeleteQueue() {
		bool bInvalidate = false;
		PhysXSceneImpl* pScene = GetScene();
		if (!mPhysXAggregateWaitDeleteQueue.empty()) {
			for (PhysXAggregate* pAggre : mPhysXAggregateWaitDeleteQueue) {
				pScene->RemoveAggregate(pAggre);
			}
			mPhysXAggregateWaitDeleteQueue.clear();
			bInvalidate = true;
		}

		if (!mPhysXActorWaitDeleteQueue.empty()) {
			pScene->RemovePhysicsActors(mPhysXActorWaitDeleteQueue);
			mPhysXActorWaitDeleteQueue.clear();
			bInvalidate = true;
		}

		if (!mPhysXShapeWaitDeleteQueue.empty()) {
			for (PhysXShape* pShape : mPhysXShapeWaitDeleteQueue) {
				pShape->DetachActor();
				PhysXSystem::instance()->ReleasePhysXShape(pShape);
			}
			mPhysXShapeWaitDeleteQueue.clear();
			bInvalidate = true;
		}
		if (bInvalidate) {
			pScene->InvalidateCache();
		}
	}

	void EchoPhysicsManager::AppearUncommittedChanges() {
		if (mHasUncommittedChanges) return;
		mHasUncommittedChanges = true;
	}
	void EchoPhysicsManager::FlushPhysicsScene() {
		if (!mHasUncommittedChanges) return;
		bool isLock = PhysXWorldSystem::instance()->PhysXJobLock(true);
		FlushPhysicsSceneImpl();
		UpdatePhysicsImpl(EchoPhysicsComponentType::eAGGREGATE);
		GetScene()->FlushQueryUpdates();
		if (isLock) PhysXWorldSystem::instance()->PhysXJobUnlock();
	}
	void EchoPhysicsManager::FlushPhysicsSceneImpl() {
		if (!mHasUncommittedChanges) return;
		UpdateRigidBodyPhysXTransformQueue();
		UpdateColliderLocalPoseQueue();
		UpdateColliderGeometryQueue();
		UpdatePhysXWaitDeleteQueue();
		mHasUncommittedChanges = false;
	}
	void EchoPhysicsManager::SyncAllCctPhysXGlobalPose() {
		if (!GetControllerIsSyncGlobalPose()) return;
		for (CharatorControllerComponent* pCct : m_ControllerList) {
			pCct->SyncPhysXGlobalPose();
		}
	}
	EchoPhysicsManager* EchoPhysicsManager::instance() {
		return EchoPhysicsSystem::instance()->GetMainManager();
	}

#ifdef USE_PHYSICS_PAGE
#define CRPHYSXOBJECT(T) \
	void EchoPhysicsManager::CreatePhysXObject(T* pComp) {\
		if (mIsUsePhysXPage) {\
			if (!pComp->getRealVisibleState()) return;\
			m_pGraph->AddObject(pComp);\
		}\
		else {\
			pComp->SetIsUsePhysics(true);\
		}\
	}\
	void EchoPhysicsManager::ReleasePhysXObject(T* pComp) {\
		if (mIsUsePhysXPage) {\
			m_pGraph->RemoveObject(pComp);\
		}\
		else {\
			pComp->SetIsUsePhysics(false);\
		}\
	}\

#else
#define CRPHYSXOBJECT(T) \
	void EchoPhysicsManager::CreatePhysXObject(T* pComp) {\
		pComp->SetIsUsePhysics(true);\
	}\
	template<typename T>\
	void EchoPhysicsManager::ReleasePhysXObject(T* pComp) {\
		pComp->SetIsUsePhysics(false);\
	}\

#endif

	CRPHYSXOBJECT(RigidBodyComponent);
	CRPHYSXOBJECT(JointComponent);
	CRPHYSXOBJECT(ColliderComponent);

}

#ifdef USE_PHYSICS_PAGE

namespace Echo {
	
	void EchoPhysicsManager::RemovePhysXObjectFromUpdateQueue(EchoPhysXObject* ptr)
	{
		if (ptr == nullptr || !ptr->getIsUpdateWorldAABB()) return;
		ptr->setIsUpdateWorldAABB(false);
		mUpdateBoundQueue.erase(ptr);
	}

	void EchoPhysicsManager::AddPhysXObjectToUpdateQueue(EchoPhysXObject* ptr) 
	{
		if (ptr == nullptr || ptr->getIsUpdateWorldAABB()) return;
		ptr->setIsUpdateWorldAABB(true);
		mUpdateBoundQueue.insert(ptr);
	}
	void EchoPhysicsManager::UpdatePhysXObjectBoundQueue() {
		if (mUpdateBoundQueue.empty()) return;
		
		for (EchoPhysXObject* ptr : mUpdateBoundQueue) {
			if (ptr != nullptr) {
				updateEchoPhysXObject(ptr);
				ptr->setIsUpdateWorldAABB(false);
			}
		}
		mUpdateBoundQueue.clear();
	}
	void EchoPhysicsManager::ClearPhysXObjectBoundQueue() {
		if (mUpdateBoundQueue.empty()) return;
		mUpdateBoundQueue.clear();
	}

	void EchoPhysicsManager::TestMeshGeometryQueryTime() {
		LogManager::instance()->logMessage("----------------------------------------MeshGeometryQuery----------------------------------------");
		LogManager::instance()->logMessage("TestResultUnit:  microseconds(0.000001s)");
		std::vector<std::pair<String, EchoTriangleMeshGeometry>> triangles;
		std::vector< std::pair<String, EchoConvexMeshGeometry>> convexs;

		EchoBoxGeometry box(Vector3(0.5f));
		EchoSphereGeometry sphere(0.5f);
		EchoCapsuleGeometry capsule(0.4f, 0.6f);
		EchoCylinderGeometry cylinder(0.4f, 1.0f);

		{
			auto iter = PhysXTriangleMeshResourceManager::instance()->getResources().begin();
			auto endIter = PhysXTriangleMeshResourceManager::instance()->getResources().end();
			while (iter != endIter) {
				PhysXTriangleMeshResource* pRes = (PhysXTriangleMeshResource*)iter->second;
				if (pRes->m_mesh) {
					triangles.push_back(std::make_pair("[" + pRes->getName().toString() + "]: ", EchoTriangleMeshGeometry(pRes->m_mesh)));
				}
				++iter;
			}
		}
		{
			auto iter = PhysXConvexMeshResourceManager::instance()->getResources().begin();
			auto endIter = PhysXConvexMeshResourceManager::instance()->getResources().end();
			while (iter != endIter) {
				PhysXConvexMeshResource* pRes = (PhysXConvexMeshResource*)iter->second;
				if (pRes->m_mesh) {
					convexs.push_back(std::make_pair("[" + pRes->getName().toString() + "]: ", EchoConvexMeshGeometry(pRes->m_mesh)));
				}
				++iter;
			}
		}

		auto testRaycast = [&](String txt) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometryRaycast(iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometryRaycast(iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};
		auto testSweep = [&](String txt, EchoGeometry& geom) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};
		auto testOverlap = [&](String txt, EchoGeometry& geom) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};

		testRaycast("-----------------Raycast-------------------");

		testOverlap("---------------BoxOverlap------------------", box);
		testSweep("---------------BoxSweep--------------------", box);

		testOverlap("--------------SphereOverlap----------------", sphere);
		testSweep("--------------SphereSweep------------------", sphere);

		testOverlap("--------------CapsuleOverlap---------------", capsule);
		testSweep("--------------CapsuleSweep-----------------", capsule);

		testOverlap("--------------CylinderOverlap--------------", cylinder);
		testSweep("--------------CylinderSweep----------------", cylinder);
		LogManager::instance()->logMessage("----------------------------------------MeshGeometryQuery----------------------------------------");
	}

	void EchoPhysicsManager::TestGeometryQueryTime() {
		LogManager::instance()->logMessage("----------------------------------------GeometryQuery----------------------------------------");
		LogManager::instance()->logMessage("TestResultUnit:  microseconds(0.000001s)");
		String boxStr		= "Box:             ";
		String sphereStr	= "Sphere:          ";
		String capsuleStr	= "Capsule:         ";
		String cylinderStr	= "Cylinder:        ";
		EchoBoxGeometry box(Vector3(0.5f));
		EchoSphereGeometry sphere(0.5f);
		EchoCapsuleGeometry capsule(0.4f, 0.6f);
		EchoCylinderGeometry cylinder(0.4f, 1.0f);

		std::vector<std::pair<String, EchoTriangleMeshGeometry>> triangles;
		std::vector< std::pair<String, EchoConvexMeshGeometry>> convexs;

		EchoConvexMeshGeometryDEL convex100(PhysXSystem::instance()->createFanConvexMesh(3.0f, 0.f, 350.0f, 2.0f, 100));
		EchoConvexMeshGeometryDEL convex500(PhysXSystem::instance()->createFanConvexMesh(3.0f, 0.f, 350.0f, 2.0f, 500));
		EchoConvexMeshGeometryDEL convex1000(PhysXSystem::instance()->createFanConvexMesh(3.0f, 0.f, 350.0f, 2.0f, 1000));

		convexs.push_back(std::make_pair("Convex:          ", EchoConvexMeshGeometry(convex100.convex)));
		convexs.push_back(std::make_pair("Convex:          ", EchoConvexMeshGeometry(convex500.convex)));
		convexs.push_back(std::make_pair("Convex:          ", EchoConvexMeshGeometry(convex1000.convex)));

		EchoTriangleMeshGeometryDEL triangle200(PhysXSystem::instance()->createFanTriangleMesh(3.0f, 0.1f, 350.0f, 2.0f, 200));
		EchoTriangleMeshGeometryDEL triangle500(PhysXSystem::instance()->createFanTriangleMesh(3.0f, 0.1f, 350.0f, 2.0f, 500));
		EchoTriangleMeshGeometryDEL triangle1000(PhysXSystem::instance()->createFanTriangleMesh(3.0f, 0.1f, 350.0f, 2.0f, 1000));

		triangles.push_back(std::make_pair("Triangle:        ", EchoTriangleMeshGeometry(triangle200.triangle)));
		triangles.push_back(std::make_pair("Triangle:        ", EchoTriangleMeshGeometry(triangle500.triangle)));
		triangles.push_back(std::make_pair("Triangle:        ", EchoTriangleMeshGeometry(triangle1000.triangle)));

		auto testRaycast = [&](String txt) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			time = PhysXSystem::instance()->EvaluateGeometryRaycast(box);
			LogManager::instance()->logMessage(boxStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryRaycast(sphere);
			LogManager::instance()->logMessage(sphereStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryRaycast(capsule);
			LogManager::instance()->logMessage(capsuleStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryRaycast(cylinder);
			LogManager::instance()->logMessage(cylinderStr + std::to_string(time));

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometryRaycast(iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometryRaycast(iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};
		auto testSweep = [&](String txt, EchoGeometry& geom) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, box);
			LogManager::instance()->logMessage(boxStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, sphere);
			LogManager::instance()->logMessage(sphereStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, capsule);
			LogManager::instance()->logMessage(capsuleStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, cylinder);
			LogManager::instance()->logMessage(cylinderStr + std::to_string(time));

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometrySweep(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};
		auto testOverlap = [&](String txt, EchoGeometry& geom) {
			LogManager::instance()->logMessage(txt.c_str());
			double time = 0.0;

			time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, box);
			LogManager::instance()->logMessage(boxStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, sphere);
			LogManager::instance()->logMessage(sphereStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, capsule);
			LogManager::instance()->logMessage(capsuleStr + std::to_string(time));
			time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, cylinder);
			LogManager::instance()->logMessage(cylinderStr + std::to_string(time));

			for (auto& iter : convexs) {
				time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

			for (auto& iter : triangles) {
				time = PhysXSystem::instance()->EvaluateGeometryOverlap(geom, iter.second);
				LogManager::instance()->logMessage(iter.first + std::to_string(time));
			}

		};

		testRaycast("-----------------Raycast-------------------");

		testOverlap("---------------BoxOverlap------------------", box);
		testSweep("---------------BoxSweep--------------------", box);

		testOverlap("--------------SphereOverlap----------------", sphere);
		testSweep("--------------SphereSweep------------------", sphere);

		testOverlap("--------------CapsuleOverlap---------------", capsule);
		testSweep("--------------CapsuleSweep-----------------", capsule);

		testOverlap("--------------CylinderOverlap--------------", cylinder);
		testSweep("--------------CylinderSweep----------------", cylinder);

		LogManager::instance()->logMessage("----------------------------------------GeometryQuery----------------------------------------");
	}

	const std::vector<std::pair<Vector3, Vector3>>& __getTestData_Ray(int level) {
		static auto genFun = [](int n) -> std::vector<std::pair<Vector3, Vector3>> {
			std::vector<std::pair<Vector3, Vector3>> res;
			int begin = -n;
			int end = n + 1;
			float inl = 0.5f / n;
			for (int i = begin; i != end; ++i) {
				for (int j = begin; j != end; ++j) {
					for (int k = begin; k != end; ++k) {
						Vector3 ori = Vector3((float)i* inl, (float)j* inl, (float)k* inl);
						Vector3 dir = Vector3((float)i, (float)j, (float)k).GetNormalized();
						res.push_back(std::make_pair(ori, dir));
						res.push_back(std::make_pair(ori, -dir));
					}
				}
			}
			return res;
		};
		static std::unordered_map<int, std::vector<std::pair<Vector3, Vector3>>> gTestDataMap;
		auto iter = gTestDataMap.find(level);
		if (iter != gTestDataMap.end()) {
			return iter->second;
		}
		else {
			gTestDataMap[level] = genFun(level);
			return gTestDataMap[level];
		}
	}

	const std::vector<Vector3>& __getBoxTestData_Overlap(int level) {
		static auto genFun = [](int n) -> std::vector<Vector3> {
			std::vector<Vector3> res;
			int begin = -n;
			int end = n + 1;
			float inl = 0.5f / n;
			for (int i = begin; i != end; ++i) {
				for (int j = begin; j != end; ++j) {
					for (int k = begin; k != end; ++k) {
						res.push_back(Vector3((float)i * inl, (float)j * inl, (float)k * inl));
					}
				}
			}
			return res;
		};
		static std::unordered_map<int, std::vector<Vector3>> gTestDataMap;
		auto iter = gTestDataMap.find(level);
		if (iter != gTestDataMap.end()) {
			return iter->second;
		}
		else {
			gTestDataMap[level] = genFun(level);
			return gTestDataMap[level];
		}
	}


	void EchoPhysicsManager::TestSceneQueryTime(const AxisAlignedBox& bound) {
		if (bound.IsEmpty()) return;

		LogManager::instance()->logMessage("----------------------------------------SceneQuery----------------------------------------");

		LogManager::instance()->logMessage("SceneAABB: Min" + bound.getMinimum().ToString() + " Max" + bound.getMaximum().ToString() + "    ResultUnit: microseconds(0.000001s)");
		
		EchoBoxGeometry box(Vector3(0.5f));
		EchoSphereGeometry sphere(0.5f);
		EchoCapsuleGeometry capsule(0.4f, 0.6f);
		EchoCylinderGeometry cylinder(0.4f, 1.0f);
		EchoBoxGeometry boxo(Vector3(1.5f));
		EchoSphereGeometry sphereo(1.5f);
		EchoCapsuleGeometry capsuleo(1.2f, 1.8f);
		EchoCylinderGeometry cylindero(1.2f, 3.0f);

		Vector3 boxSize = bound.getSize();
		float maxLength = std::max(boxSize.x, boxSize.y);
		maxLength = std::max(maxLength, boxSize.z);

		int level = 7;
		float layer = maxLength / 10;
		if (layer > 1.0f)
		{
			layer = Math::Log2(layer);
			level += (int)ceilf(layer) * 2;
		}
		std::multimap<double, String> outputMap;

		auto testRaycast = [&](String txt) {
			Vector3 center = bound.GetCenter();
			Vector3 halfSize = bound.getHalfSize();
			float dis = Math::Max(Math::Max(halfSize.x, halfSize.y), halfSize.z) + 0.1f;
			float maxDis = dis * 2.0f;
			const auto& testData = __getTestData_Ray(3);
			int allSize = 0;

			std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();
			{
				int size = (int)testData.size();
				for (int i = 0; i != size; ++i) {
					Raycast(center + testData[i].first * halfSize, testData[i].second, maxDis, ePhysXGroup_All);
				}
				allSize += size;
			}
			std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;
			double time = (double)(std::chrono::duration_cast<std::chrono::microseconds>(_time).count()) / (double)allSize;
			//LogManager::instance()->logMessage(txt + std::to_string(time));
			outputMap.insert(std::make_pair(time, txt + std::to_string(time)));
		};
		auto testSweep = [&](String txt, EchoGeometry& geom, const Quaternion& quat = Quaternion::IDENTITY) {
			Vector3 center = bound.GetCenter();
			Vector3 halfSize = bound.getHalfSize();
			float maxDis = halfSize.length() * 3.0f;
			EchoLocationHit hit;
			halfSize += geom.getLocalBounds().getHalfSize().length();

			const auto& testData = __getTestData_Ray(level);
			int allSize = 0;
			DTransform trans;
			trans.rotation = quat;
			std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();
			{
				int size = (int)testData.size();
				for (int i = 0; i != size; ++i) {
					trans.translation = center + testData[i].first * halfSize;
					GeometryCast(&geom, trans, testData[i].second, maxDis, ePhysXGroup_All);
				}
				allSize += size;
			}
			std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;
			double time = (double)(std::chrono::duration_cast<std::chrono::microseconds>(_time).count()) / (double)allSize;
			//LogManager::instance()->logMessage(txt + std::to_string(time));
			outputMap.insert(std::make_pair(time, txt + std::to_string(time)));
		};
		auto testOverlap = [&](String txt, EchoGeometry& geom, const Quaternion& quat = Quaternion::IDENTITY) {
			Vector3 center = bound.GetCenter();
			Vector3 halfSize = bound.getHalfSize();
			const std::vector<Vector3>& testData = __getBoxTestData_Overlap(level);
			halfSize += geom.getLocalBounds().getHalfSize().length();
			int allSize = 0;
			DTransform trans;
			trans.rotation = quat;
			std::chrono::steady_clock::time_point tickBegin = std::chrono::steady_clock::now();
			{
				int size = (int)testData.size();
				for (int i = 0; i != size; ++i) {
					trans.translation = center + testData[i] * halfSize;
					OverlapGeometry(&geom, trans, ePhysXGroup_All);
				}
				allSize += size;
			}
			std::chrono::nanoseconds _time = std::chrono::steady_clock::now() - tickBegin;
			double time = (double)(std::chrono::duration_cast<std::chrono::microseconds>(_time).count()) / (double)allSize;
			//LogManager::instance()->logMessage(txt + std::to_string(time));
			outputMap.insert(std::make_pair(time, txt + std::to_string(time)));
		};
		Quaternion quat = GetCamera()->getOrientation();
		testRaycast("Raycast:         ");

		testOverlap("AABBOverlap:     ", boxo);
		testOverlap("BoxOverlap:      ", boxo, quat);

		testSweep("BoxSweep:        ", box);

		testOverlap("SphereOverlap:   ", sphereo);

		testSweep("SphereSweep:     ", sphere);

		testOverlap("CapsuleOverlap:  ", capsuleo);

		testSweep("CapsuleSweep:    ", capsule);

		testOverlap("CylinderOverlap: ", cylindero);

		testSweep("CylinderSweep:   ", cylinder);

		for (const auto& iter : outputMap) {
			LogManager::instance()->logMessage(iter.second);
		}

		LogManager::instance()->logMessage("----------------------------------------SceneQuery----------------------------------------");

	}

	void EchoPhysicsManager::TestSceneQueryTime() {
		
		AxisAlignedBox bound;
		for (const auto& collider : m_ColliderList) {
			if (!collider->IsExistPhysics()) continue;
			AxisAlignedBox _bound;
			computePhysicsWorldAABB(collider, _bound);
			bound.merge(_bound);
		}
		TestSceneQueryTime(bound);
	}
}


#endif //USE_PHYSICS_PAGE

