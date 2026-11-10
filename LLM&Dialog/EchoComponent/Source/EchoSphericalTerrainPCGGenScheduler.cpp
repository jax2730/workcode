#include "EchoComponentStableHeaders.h"
#include "EchoSphericalTerrainPCGGenScheduler.h"
#include "EchoPCGGraphComponent.h"
#include "EchoPCG.h"
#include "EchoPhysXManager.h"
#include "EchoPCGObject.h"
#include "EchoSphericalTerrainManager.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoPCGBaseElement.h"

namespace Echo {

	namespace PCG
	{
		struct _EchoComponentExport SphericalTerrainSampler : public PCG::SurfaceSampler {
		public:
			SphericalTerrainSampler(const SphericalTerrainSampler& other);
			SphericalTerrainSampler& operator=(const SphericalTerrainSampler& other);
			SphericalTerrainSampler(PlanetGenObjectScheduler* _data, const std::set<EchoIndex2>& _BiomeGroup, bool _bCheckRoad, bool _bSurfaceTypeExclude) : data(_data), mSameBiomes(_BiomeGroup), bCheckRoad(_bCheckRoad), bSurfaceTypeExclude(_bSurfaceTypeExclude){}
			~SphericalTerrainSampler();

			virtual bool Sampler(const SamplerParam& param, std::shared_ptr<PCG::PCGDataSet>& pointSet) override;
			bool isSameBiomeGroup(int type0, int type1) const;

			PlanetGenObjectScheduler* data = nullptr;
			std::set<EchoIndex2> mSameBiomes;
			bool bCheckRoad = true;
			bool bSurfaceTypeExclude = true;
			bool bInit = false;
			OBBOctree* mRoadOctree = nullptr;
			static Name gType;
		};

		SphericalTerrainSampler::~SphericalTerrainSampler() {
			SAFE_DELETE(mRoadOctree);
		}
		SphericalTerrainSampler::SphericalTerrainSampler(const SphericalTerrainSampler& other) {
			*this = other;
		}
		SphericalTerrainSampler& SphericalTerrainSampler::operator=(const SphericalTerrainSampler& other) {
			if (this == &other) return *this;
			data = other.data;
			bCheckRoad = other.bCheckRoad;
			bInit = other.bInit;
			if (other.mRoadOctree) {
				mRoadOctree = new PCG::OBBOctree(*other.mRoadOctree);
			}
			else {
				mRoadOctree = nullptr;
			}
			mSameBiomes = other.mSameBiomes;
			return *this;
		}
		bool SphericalTerrainSampler::Sampler(const SamplerParam& param, std::shared_ptr<PCG::PCGDataSet>& pcgDataSet) {
			PCG::PCGPointSet pointSet(*pcgDataSet);
			if (pointSet.getSize() == 0) {
				return false;
			}

			float* pSlope = param.Slope ? pcgDataSet->addData<float>(PCGStaticAttribute::gSlope) : nullptr;
			Vector3 curNormal = Vector3::UP;
			std::vector<OrientedBox> pointBounds(pointSet.getSize());
			bool bExclude = data->existStaticAvoidArea() || (bCheckRoad && data->getSphericalTerrain()->getPlanetRoad() != nullptr);
			DVector3 sideVerts[4];
			DVector3 surSideVerts[4];
			for (uint32 index = 0; index != pointSet.getSize(); ++index) {
				DVector3 worldPos = param.pOriginSpace->ToWorldPosition(pointSet.getWorldPosition(index));
				worldPos = data->getSurfaceFinestWs(worldPos, &curNormal);
				int newSurfaceType = data->getSurfaceTypeWs(worldPos);
				if (!isSameBiomeGroup(pointSet.getSurfaceType(index), newSurfaceType)) {
					pointSet.getSurfaceType(index) = -1;
					if (bSurfaceTypeExclude) {
						continue;
					}
				}
				pointSet.getSurfaceType(index) = newSurfaceType;
				Quaternion quat;
				Vector3 upDir = -data->getGravityWs(worldPos);
				if (param.ApplySlope) {
					quat.RotationAxisToAxis(Vector3::UP, curNormal);
					pointSet.setWorldRotation(index, param.pOriginSpace->ToLocalRotation(quat));
				}
				else if (param.ApplyGravity) {
					quat.RotationAxisToAxis(Vector3::UP, upDir);
					pointSet.setWorldRotation(index, param.pOriginSpace->ToLocalRotation(quat));
				}
				else {
					quat = pointSet.getWorldRotation(index);
				}
				if (pSlope || param.VertexGround) {
					sideVerts[0] = worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMin(index).z);
					sideVerts[1] = worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMax(index).z);
					sideVerts[2] = worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMin(index).z);
					sideVerts[3] = worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMax(index).z);
					surSideVerts[0] = data->getSurfaceFinestWs(sideVerts[0]);
					surSideVerts[1] = data->getSurfaceFinestWs(sideVerts[1]);
					surSideVerts[2] = data->getSurfaceFinestWs(sideVerts[2]);
					surSideVerts[3] = data->getSurfaceFinestWs(sideVerts[3]);
				}
				if (pSlope) {
					pSlope[index] = Vector3::Angle(upDir, Vector3::VCross(surSideVerts[3] - surSideVerts[0], (surSideVerts[2] - surSideVerts[1])).GetNormalized() + curNormal);
				}
				if (param.VertexGround) {
					float maxHeight = -FLT_MAX;
					for (int verIndex = 0; verIndex < 4; ++verIndex) {
						Vector3 groundToVert = sideVerts[verIndex] - surSideVerts[verIndex];
						float gtvDotDir = groundToVert.Dot(upDir);
						maxHeight = std::max(maxHeight, gtvDotDir);
					}
					if (maxHeight > 0.0f) {
						worldPos -= maxHeight * upDir;
					}
					pointSet.setWorldPosition(index, param.pOriginSpace->ToLocalPosition(worldPos));
				}
				else {
					pointSet.setWorldPosition(index, param.pOriginSpace->ToLocalPosition(worldPos));
				}
				if (bExclude) {
					pointBounds[index] = pointSet.getWorldOBB(index);
				}
			}
			if (bExclude) {
				if (!bInit)
				{
					bInit = true;
					DVector3 centerPos = data->getSurfaceFinestWs(param.pOriginSpace->mPosition);
					Vector3 gravity = -data->getGravityWs(centerPos);
					Quaternion quat;
					quat.RotationAxisToAxis(Vector3::UP, gravity);
					AxisAlignedBox pointsBound = param.pOriginSpace->mLocalBound;
					pointsBound.mMinimum = pointsBound.mMinimum - 100.0f;
					pointsBound.mMaximum = pointsBound.mMaximum + 100.0f;
					PCG::PCGOriginSpace::GetWorldBound(pointsBound, centerPos, quat, Vector3::UNIT_SCALE);
					pointsBound.mMinimum = pointsBound.mMinimum - data->getPosition();
					pointsBound.mMaximum = pointsBound.mMaximum - data->getPosition();
					std::vector<PlanetRoadData> resultRoad;
					if (data->getPlanetRoad() && bCheckRoad) {
						data->getPlanetRoad()->getRoadBeziers(pointsBound, resultRoad);
					}
					std::vector<OrientedBox> roadAreas;
					if (!resultRoad.empty()) {
						for (auto& road : resultRoad) {
							road.width = std::max(road.width * 2.0f, 20.0f);
							std::vector<Vector3> temp = PlanetRoad::getRoadSubdivideAABB(road, road.width);
							int tempSize = (int)temp.size();
							for (int i = 0; i < tempSize; ++i) {
								OrientedBox& obb = roadAreas.emplace_back();
								obb.center = temp[i] + data->getPosition();
								obb.extent = Vector3(road.width);
								obb.orientation = quat;
								obb = param.pOriginSpace->ToLocalOBB(obb);
							}
						}
					}
					
					OrientedBox pointsObb;
					pointsObb.extent = param.pOriginSpace->mLocalBound.getHalfSize() + 100.0f;
					pointsObb.orientation = quat;
					pointsObb.center = centerPos - data->getPosition();
					std::vector<OrientedBox> avoidAreas;
					if (data->getStaticAvoidArea(pointsObb, avoidAreas)) {
						for (auto& obb : avoidAreas) {
							obb.center += data->getPosition();
							obb = param.pOriginSpace->ToLocalOBB(obb);
						}
					}

					if (!roadAreas.empty() || !avoidAreas.empty()) {
						pointsObb = param.pOriginSpace->ToLocalOBB(pointsObb);
						float octreeExtent = pointsObb.extent[pointsObb.extent.MainAxis() - 1];
						mRoadOctree = new PCG::OBBOctree(pointsObb.center, octreeExtent);
						for (auto& obb : roadAreas) {
							mRoadOctree->AddElement(obb);
						}
						for (auto& obb : avoidAreas) {
							mRoadOctree->AddElement(obb);
						}
					}
					else {
						SAFE_DELETE(mRoadOctree);
					}
				}

				if (mRoadOctree) {
					for (uint32 index = 0; index != pointSet.getSize(); ++index) {
						if (pointSet.getSurfaceType(index) < 0) continue;
						if (PCG::OBBOctreeSemantics::intersect(mRoadOctree, pointBounds[index])) {
							pointSet.getSurfaceType(index) = -2;
						}
					}
				}
			}
			if (bSurfaceTypeExclude) {
				std::vector<uint32> mFilterDataVector;
				mFilterDataVector.reserve(pointSet.getSize());
				for (uint32 index = 0; index != pointSet.getSize(); ++index) {
					if (pointSet.getSurfaceType(index) < 0) continue;
					mFilterDataVector.emplace_back(index);
				}
				if (mFilterDataVector.empty()) return false;
				pcgDataSet = std::make_shared<PCGDataSet>(*pcgDataSet, mFilterDataVector);
			}
			return true;
		}
		bool SphericalTerrainSampler::isSameBiomeGroup(int type0, int type1) const {
			if (type0 == type1) return true;
			if (type0 < type1) {
				return mSameBiomes.contains(EchoIndex2(type0, type1));
			}
			else {
				return mSameBiomes.contains(EchoIndex2(type1, type0));
			}
		}
	}


	namespace PCG {

		class SphericalTerrainRoadSplineSampler : public PCG::SplineSampler {
		public:
			SphericalTerrainRoadSplineSampler(PlanetGenObjectScheduler* _data) : data(_data) {}
			virtual ~SphericalTerrainRoadSplineSampler() {}
			virtual bool GetSpline(const PCGOriginSpace* pOriginSpace, std::vector<PCG::SplineSampler::Spline>& splines) override {
				std::vector<PlanetRoadData> resultRoad;
				if (!data->getPlanetRoad()) {
					data->getSphericalTerrain()->BuildPlanetRoad(true);
				}
				PCGOriginSpace sphSpace;
				sphSpace.mPosition = data->getPosition();
				sphSpace.mRotation = data->getRotation();
				sphSpace.mScale = data->getScale();
				if (data->getPlanetRoad()) {
					AxisAlignedBox pointsBound = pOriginSpace->GetWorldBound();
					pointsBound = sphSpace.ToLocalBound(pointsBound);
					data->getPlanetRoad()->getRoadBeziers(pointsBound, resultRoad);
				}
				if (!resultRoad.empty()) {
					for (auto& road : resultRoad) {
						std::vector<Vector3> temp = PlanetRoad::getRoadSubdivideAABB(road, 20.0f);
						int tempSize = (int)temp.size();
						
						if (tempSize > 1) {
							PCG::SplineSampler::Spline& spline = splines.emplace_back();
							spline.bClosed = false;
							spline.points.reserve(tempSize);
							DVector3 lastPos(DBL_MAX);
							for (int tempIndex = 0; tempIndex < tempSize; ++tempIndex) {
								//DVector3 pos = data->getSurfaceFinestWs(sphSpace.ToWorldPosition(temp[tempIndex]));
								DVector3 pos = sphSpace.ToWorldPosition(temp[tempIndex]);
								if (lastPos.isEqual(pos, 0.1)) continue;
								lastPos = pos;
								auto& splinePoint = spline.points.emplace_back();
								splinePoint.position = pOriginSpace->ToLocalPosition(pos);
								splinePoint.up = pOriginSpace->mRotation.Inverse() * -data->getGravityWs(pos);
							}

						}
					}
					return true;
				}
				return false;
			}
			PlanetGenObjectScheduler* data = nullptr;
		};

	}
}

namespace Echo {
	class SphericalTerrainPCGGenSchedulerPrivate {
	public:
		SphericalTerrainPCGGenSchedulerPrivate() {}
		~SphericalTerrainPCGGenSchedulerPrivate() { clear(); }
		void clear() {
			mRootActorCompPtr = nullptr;
			for (int level = 0; level < 3; ++level) {
				mSameBiomes[level].clear();
				for (auto& iter : mBiomeInputs[level]) {
					SAFE_DELETE(iter.second);
				}
				mBiomeInputs[level].clear();
			}
		}

		ActorComponentPtr mRootActorCompPtr;
		std::array<std::set<EchoIndex2>, 3> mSameBiomes;
		std::array<std::map<int, PCG::PCGAnyMap*>, 3> mBiomeInputs;
	};
}

namespace Echo {

	PCG::PlanetGenObjectScheduler* SphericalTerrainPCGGenScheduler::gCreateFunction(SphericalTerrain* pSph) {
		return new SphericalTerrainPCGGenScheduler(pSph);
	}

	SphericalTerrainPCGGenScheduler::SphericalTerrainPCGGenScheduler(SphericalTerrain* pSph) : PCG::PlanetGenObjectScheduler(pSph) {
		m_pSTPrivate = new SphericalTerrainPCGGenSchedulerPrivate();
	}
	SphericalTerrainPCGGenScheduler::~SphericalTerrainPCGGenScheduler() {
		Clear();
		SAFE_DELETE(m_pRoadPcg);
		PCGObjectManager::GetInstance()->CancelOrWairWaitDestroyJobs();
		SAFE_DELETE(m_pSTPrivate);
	}
	void SphericalTerrainPCGGenScheduler::Clear() {
		for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
			if (it->second) {
				it->second->Destroy();
				it->second = nullptr;
			}
		}
		m_PcgMap.clear();
	}
	EchoPCGObject* SphericalTerrainPCGGenScheduler::Add(const GenSpace& _obb, uint32 id, int level, EchoPCGObject* parentPcg, bool bSync) {
		
		if (mPlanetGenObject.path[level].empty()) return nullptr;

		DVector3 worldPosition = getRotation() * (_obb.position * getScale()) + getPosition();
		Quaternion worldRotation = getRotation() * _obb.rotation;

		EchoPCGObjectDesc desc;
		desc.mSeed = getRandomSeed(id);
		desc.mPosition = _obb.position;
		desc.mRotation = _obb.rotation;
		desc.mScale = Vector3::UNIT_SCALE;
		desc.mBound.setExtents(-_obb.halfSize, _obb.halfSize);
		desc.mBasePosition = getPosition();
		desc.mBaseRotation = getRotation();
		desc.mBaseScale = getScale();
		desc.mID = id;
		desc.mRootActorCompPtr = m_pSTPrivate->mRootActorCompPtr;

		std::vector<std::pair<Name, int>> biomeGraphs = PCG::PCGBiomeGraphGenerate(_obb.halfSize, [&](const DVector3& pos) -> int {
			return m_pSphericalTerrain->getSurfaceTypeWs(worldRotation * pos + worldPosition);
			}, mPlanetGenObject.path[level]);

		if (biomeGraphs.empty()) return nullptr;
		uint32 biomeGraphSize = (uint32)biomeGraphs.size();

		desc.mPCGGraphs.resize(biomeGraphs.size());
		for (uint32 biomeGraphIndex = 0; biomeGraphIndex < biomeGraphSize; ++biomeGraphIndex) {
			desc.mPCGGraphs[biomeGraphIndex].mGraphName = biomeGraphs[biomeGraphIndex].first;
			desc.mPCGGraphs[biomeGraphIndex].mSurfaceType = biomeGraphs[biomeGraphIndex].second;
		}
		switch (level) {
		case 0: {
			desc.mLevel = PCG::GenObjectRuleLevel::eHigh;
			break;
		}
		case 1: {
			desc.mLevel = PCG::GenObjectRuleLevel::eMiddle;
			break;
		}
		case 2: {
			desc.mLevel = PCG::GenObjectRuleLevel::eLow;
			break;
		}
		}

		EchoPCGObject* newPcg = new EchoPCGObject(getSceneName());
		newPcg->SetPCGObjectDesc(desc);
		newPcg->SetParent(parentPcg);
		newPcg->Prepare();
		{
			newPcg->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SurfaceSampler>>(PCG::SurfaceSampler::gType, new PCG::SphericalTerrainSampler(this, m_pSTPrivate->mSameBiomes[level], true, true));
			PCG::PCGGeneralObjectCache* pStaticMeshCache = newPcg->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
			if (pStaticMeshCache) {
				pStaticMeshCache->mLibMeshMap = mPlanetGenObject.meshs[level];
				pStaticMeshCache->SetRenderDistance(getRenderDistance(level));
				pStaticMeshCache->mLibMeshOffset = desc.mSeed;

				auto delObjIter = mDeletedObjects.find(id);
				if (delObjIter != mDeletedObjects.end()) {
					pStaticMeshCache->mDeletedObjects = delObjIter->second;
				}
				else {
					pStaticMeshCache->mDeletedObjects = {};
				}
			}
		}
		for (uint32 biomeGraphIndex = 0; biomeGraphIndex < biomeGraphSize; ++biomeGraphIndex) {
			PCG::PCGAnyMap* pAttrSet = newPcg->GetInputAttributeSet(biomeGraphIndex);
			auto biomeInputIter = m_pSTPrivate->mBiomeInputs[level].find(biomeGraphs[biomeGraphIndex].second);
			if (biomeInputIter != m_pSTPrivate->mBiomeInputs[level].end()) {
				*pAttrSet = *biomeInputIter->second;
			}
		}

		if (bSync) {
			newPcg->Execute();
		}
		else {
			newPcg->Schedule();
		}
		return newPcg;
	}
	using _NodeID = PCG::NodeID;
	void SphericalTerrainPCGGenScheduler::updateGenSpace(const std::map<int, GenSpaceList>& genSpaceListMap) {

		std::map<uint32, EchoPCGObject*> newPcgMap2;
		for (auto& genSpaceListPair : genSpaceListMap) {
			const std::unordered_set<uint32>& keepSpaces = genSpaceListPair.second.keepSpaces;
			for (uint32 id : keepSpaces) {
				auto iter = m_PcgMap.find(id);
				if (iter != m_PcgMap.end()) {
					newPcgMap2.insert(std::make_pair(iter->first, iter->second));
					m_PcgMap.erase(iter);
				}
			}
		}
		{
			for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
				if (it->second) {
					it->second->Destroy();
					it->second = nullptr;
				}
			}
			m_PcgMap.clear();
			m_PcgMap.swap(newPcgMap2);
		}
		for (auto& genSpaceListPair : genSpaceListMap) {
			const std::unordered_map<uint32, GenSpace>& newSpaces = genSpaceListPair.second.newSpaces;
			for (auto& iter : newSpaces) {
				EchoPCGObject* pParObj = nullptr;
				int j = genSpaceListPair.first;
				_NodeID nodeId(iter.first);
				while (j > 0) {
					_NodeID parentId;
					if (nodeId.GetParentID(parentId)) {
						auto parIt = m_PcgMap.find(parentId._id);
						if (parIt != m_PcgMap.end()) {
							if (parIt->second) {
								pParObj = parIt->second;
								break;
							}
						}
					}
					nodeId = parentId;
					--j;
				}
				m_PcgMap[iter.first] = Add(iter.second, iter.first, genSpaceListPair.first, pParObj, false);
			}
		}

	}

	void SphericalTerrainPCGGenScheduler::ExportGenObjects(const std::map<int, GenSpaceMap>& genSpaceListMap, std::map<Name, std::vector<PCG::ExportPoint>>& exportPoints) {
		if (!m_PcgMap.empty()) {
			for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
				if (it->second) {
					it->second->Destroy();
					it->second = nullptr;
				}
			}
			m_PcgMap.clear();
		}
		for (auto& genSpaceListPair : genSpaceListMap) {
			const std::unordered_map<uint32, GenSpace>& newSpaces = genSpaceListPair.second;
			for (auto& iter : newSpaces) {
				EchoPCGObject* pParObj = nullptr;
				int j = genSpaceListPair.first;
				_NodeID nodeId(iter.first);
				while (j > 0) {
					_NodeID parentId;
					if (nodeId.GetParentID(parentId)) {
						auto parIt = m_PcgMap.find(parentId._id);
						if (parIt != m_PcgMap.end()) {
							if (parIt->second) {
								pParObj = parIt->second;
								break;
							}
						}
					}
					nodeId = parentId;
					--j;
				}
				m_PcgMap[iter.first] = Add(iter.second, iter.first, genSpaceListPair.first, pParObj, true);
			}
		}
		for (auto& [id,pPcg] : m_PcgMap) {
			if (pPcg) pPcg->ExportPhysicsObjects(exportPoints);
		}
		for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
			SAFE_DELETE(it->second);
		}
		m_PcgMap.clear();
	}
	void SphericalTerrainPCGGenScheduler::collectGenObjects(std::map<Name, uint32>& data, uint32 type) {
		for (const auto& iter : m_PcgMap) {
			if (iter.second) {
				iter.second->CollectObjects(data, type);
			}
		}
	}
	void SphericalTerrainPCGGenScheduler::collectDrawInfo(PCG::PCGDrawInfoMap& data) {
		for (const auto& iter : m_PcgMap) {
			if (iter.second) {
				iter.second->CollectDrawInfo(data);
			}
		}
	}

	void SphericalTerrainPCGGenScheduler::initImpl() {
		m_pSTPrivate->clear();
		{
			ActorManager* pActorMgr = ActorSystem::instance()->getActorManager(getSceneName());
			pActorMgr->createMainActor();
			m_pSTPrivate->mRootActorCompPtr = pActorMgr->mMainActor->getRootNodeCom();
		}
		PCG::PlanetGenObjectToPreDatas(mPlanetGenObject, m_pSTPrivate->mSameBiomes, m_pSTPrivate->mBiomeInputs);
		if (m_pRoadPcg) {
			m_pRoadPcg->Create(false);
		}
	}
	void SphericalTerrainPCGGenScheduler::updateDeletedObjects() {
		if (!mStateChangeObjects.empty()) {
			for (const auto& pidIter : mStateChangeObjects) {
				auto pcgIter = m_PcgMap.find(pidIter.first);
				if (pcgIter == m_PcgMap.end()) continue;
				EchoPCGObject* pPCGObj = pcgIter->second;
				if (pPCGObj && pPCGObj->IsExistPCG()) {
					PCG::PCGGeneralObjectCache* pStaticMeshCache = pPCGObj->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
					if (pStaticMeshCache) {
						auto delObjIter = mDeletedObjects.find(pidIter.first);
						if (delObjIter != mDeletedObjects.end()) {
							pStaticMeshCache->mDeletedObjects = delObjIter->second;
						}
						else {
							pStaticMeshCache->mDeletedObjects = {};
						}
						pStaticMeshCache->ChangeVisibleObjects(pidIter.second);
					}
				}
			}
			mStateChangeObjects.clear();
		}
	}
	bool SphericalTerrainPCGGenScheduler::queryGenObjects(const DVector3& worldPos, const Quaternion& worldRot, const Vector3& extent, std::set<uint64>& objIds, bool checkVisible) {
		for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
			EchoPCGObject* pPCGObj = it->second;
			if (pPCGObj && pPCGObj->IsExistPCG()) {
				PCG::PCGGeneralObjectCache* pStaticMeshCache = pPCGObj->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
				if (pStaticMeshCache == nullptr) continue;
				const AxisAlignedBox& cacheBound = pStaticMeshCache->mRootActor->getLocalBounds();
				if (cacheBound.isNull()) continue;
				OrientedBox tObb;
				tObb.center = pStaticMeshCache->mOriginSpace.ToLocalPosition(worldPos);
				tObb.orientation = pStaticMeshCache->mOriginSpace.ToLocalRotation(worldRot);
				tObb.extent = extent;
				if (tObb.Intersect(cacheBound)) {
					pStaticMeshCache->QueryObjects(tObb, objIds, checkVisible);
				}
			}
		}
		return true;
	}
	bool SphericalTerrainPCGGenScheduler::queryGenObjects(const DVector3& worldPos, float radius, std::set<uint64>& objIds, bool checkVisible) {
		for (auto it = m_PcgMap.rbegin(); it != m_PcgMap.rend(); ++it) {
			EchoPCGObject* pPCGObj = it->second;
			if (pPCGObj && pPCGObj->IsExistPCG()) {
				PCG::PCGGeneralObjectCache* pStaticMeshCache = pPCGObj->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
				if (pStaticMeshCache == nullptr) continue;
				const AxisAlignedBox& cacheBound = pStaticMeshCache->mRootActor->getLocalBounds();
				if (cacheBound.isNull()) continue;
				Sphere sph;


				sph.mCenter = pStaticMeshCache->mOriginSpace.ToLocalPosition(worldPos);
				sph.mRadius = radius;
				if (cacheBound.Intersect(sph)) {
					pStaticMeshCache->QueryObjects(sph, objIds, checkVisible);
				}
			}
		}
		return true;
	}
	bool SphericalTerrainPCGGenScheduler::setGenObjectsVisible(const std::set<uint64>& objIds, bool v) {
		std::map<uint32, std::map<uint16, std::set<uint16>>> objectIdMap;
		PCG::PCGID pcgId;
		for (uint64 objId : objIds) {
			pcgId.id = objId;
			objectIdMap[pcgId.pid][pcgId.cid_0].insert(pcgId.cid_1);
		}
		for (auto& iter1 : objectIdMap) {
			auto iter = m_PcgMap.find(iter1.first);
			if (iter == m_PcgMap.end()) continue;
			EchoPCGObject* pPCGObj = iter->second;
			if (pPCGObj && pPCGObj->IsExistPCG()) {
				PCG::PCGGeneralObjectCache* pStaticMeshCache = pPCGObj->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
				if (pStaticMeshCache) {
					pStaticMeshCache->SetObjectsVisible(iter1.second, v);
				}
			}
		}
		return true;
	}
	bool SphericalTerrainPCGGenScheduler::getGenObject(uint64 id, GenObjectInfo& info) {
		PCG::PCGID pcgId;
		pcgId.id = id;
		auto iter = m_PcgMap.find(pcgId.pid);
		if (iter == m_PcgMap.end()) return false;
		EchoPCGObject* pPCGObj = iter->second;
		if (pPCGObj && pPCGObj->IsExistPCG()) {
			PCG::PCGGeneralObjectCache* pStaticMeshCache = pPCGObj->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
			if (pStaticMeshCache) {
				return pStaticMeshCache->GetObjectData(pcgId.cid_0, pcgId.cid_1, info.position, info.rotation, info.scale, info.data_name, info.custom_type);
			}
		}
		return false;

	}
}

namespace Echo
{
	RoadPCGGenScheduler* SphericalTerrainPCGGenScheduler::getRoadPCGGenScheduler() {
		if (m_pRoadPcg == nullptr) {
			m_pRoadPcg = new RoadPCGGenScheduler(this);
		}
		return m_pRoadPcg;
	}

	bool SphericalTerrainPCGGenScheduler::ExportRoadGenObjects(const std::string& folderPath, const std::map<int, PCG::BiomeGenObjectLayerInfo>& info, uint32 id) {
		if (!m_pSphericalTerrain) return false;
		m_pSphericalTerrain->BuildPlanetRoad(true);
		if (!m_pSphericalTerrain->getPlanetRoad()) return false;
		SAFE_DELETE(m_pRoadPcg);
		m_pRoadPcg = new RoadPCGGenScheduler(this);
		m_pRoadPcg->SetGenObjectInfo(info, id);
		m_pRoadPcg->Create(true, false);
		std::string filePath = m_pSphericalTerrain->m_terrainData->getName().toString();
		{
			auto sufPox = filePath.find_last_of(".");
			if (sufPox != filePath.npos) {
				filePath = filePath.substr(0, sufPox);
			}
			std::replace(filePath.begin(), filePath.end(), '\\', '/');
			std::replace(filePath.begin(), filePath.end(), '/', '_');
			filePath.append("_road.golist");
		}
		if (!folderPath.empty()) {
			if (folderPath.back() == '/' || folderPath.back() == '\\') {
				filePath = folderPath + filePath;
			}
			else {
				filePath = folderPath + "\\" + filePath;
			}
		}
		bool res = m_pRoadPcg->ExportGenObjects(filePath, "");
		SAFE_DELETE(m_pRoadPcg);
		return res;
	}

	RoadPCGGenScheduler::RoadPCGGenScheduler(SphericalTerrainPCGGenScheduler* pScheduler) : m_pScheduler(pScheduler) {}
	RoadPCGGenScheduler::~RoadPCGGenScheduler() {
		Clear();
		PCGObjectManager::GetInstance()->CancelOrWairWaitDestroyJobs();
	}

	void RoadPCGGenScheduler::Clear()
	{
		if (m_pPCG) {
			m_pPCG->Destroy();
			m_pPCG = nullptr;
		}
	}
	
	bool RoadPCGGenScheduler::ExportGenObjects(const String& file, const char* folderType) {
		std::map<Name, std::vector<PCG::ExportPoint>> allObjects;
		ExportGenObjects(allObjects);
		if (allObjects.empty()) { return false; }

		std::map<Name, uint32> meshToCustomTypeMap;
		{
			for (const auto& iter : PCG::BiomeGenObjectLibraryManager::instance()->mLibraryMap) {
				meshToCustomTypeMap[iter.second.mesh] = iter.second.custom_type;
			}
		}

		PCG::ExportPointData epData;
		epData.reserve(allObjects.size());

		for (const auto& objPair : allObjects) {
			auto& iter = epData.emplace_back();
			iter.name = objPair.first.toString();
			iter.custom = meshToCustomTypeMap[objPair.first];
			iter.points = objPair.second;
		}
		return epData.Export(file, folderType);
	}
	void RoadPCGGenScheduler::ExportGenObjects(std::map<Name, std::vector<PCG::ExportPoint>>& data) {
		if (m_pPCG) {
			m_pPCG->ExportPhysicsObjects(data);
		}
	}
	void RoadPCGGenScheduler::CreateFromConfig(bool bSync) {
		std::map<int, PCG::BiomeGenObjectLayerInfo> roadBiomeLayer;
		SphericalTerrain* pSphTer = m_pScheduler->getSphericalTerrain();
		for (int biomeCompoIndex = 0;
			biomeCompoIndex < pSphTer->m_terrainData->distribution.biomeSlots.size();
			biomeCompoIndex++)
		{
			if (biomeCompoIndex < pSphTer->m_terrainData->composition.biomes.size() &&
				pSphTer->m_terrainData->composition.biomes[biomeCompoIndex].biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
			{
				int biomeIndex = pSphTer->m_terrainData->composition.getBiomeIndex(biomeCompoIndex);
				int biomeId = pSphTer->m_terrainData->composition.biomes.at(biomeIndex).biomeTemp.biomeID;
				if (PCG::GlobalBiomeGenObjectConfig::instance().pcg_link_road.contains(biomeId)) {
					String biomePath = PCG::GlobalBiomeGenObjectConfig::instance().pcg_link_road.at(biomeId);
					const PCG::BiomeGenObjectLayerInfo& biomeLayerInfo = PCG::GlobalBiomeGenObjectConfig::instance().pcg_template_road.at(biomePath);
					roadBiomeLayer[biomeIndex] = biomeLayerInfo;
				}
				else {
					roadBiomeLayer[biomeIndex] = {};
				}
			}
		}
		SetGenObjectInfo(roadBiomeLayer, PCG::GlobalBiomeGenObjectConfig::instance().randomSeed);
		Create(true);
	}
	void RoadPCGGenScheduler::SetGenObjectInfo(const std::map<int, PCG::BiomeGenObjectLayerInfo>& info, uint32 id) {
		mInfo = info;
		mId = id;
	}
	void RoadPCGGenScheduler::Create(bool mSync, bool bCreate) {
		Clear();
		std::map<int, PCG::BiomeGenObjectLayer> mData;
		for (const auto& iter : mInfo) {
			PCG::BiomeGenObjectLayer tempStaticMesh;
			if (PCG::BiomeGenObjectLibraryManager::instance()->Generate(3, iter.second, tempStaticMesh)) {
				mData[iter.first] = tempStaticMesh;
			}
		}
		std::set<Name> sameBiomeMaps;
		std::vector<std::pair<Name, int>> biomeGraphs;
		for (const auto& iter : mData) {
			if (!sameBiomeMaps.contains(iter.second.path)) {
				biomeGraphs.emplace_back(iter.second.path, iter.first);
				sameBiomeMaps.insert(iter.second.path);
			}
		}
		if (biomeGraphs.empty()) return;

		std::set<EchoIndex2> mSameBiomes;
		std::map<int, PCG::PCGAnyMap*> mBiomeInputs;

		PCG::PlanetGenObjectToPreDatas(mData, mSameBiomes, mBiomeInputs);

		Vector3 halfSize(m_pScheduler->getSphericalTerrain()->getMaxRealRadius() + 500.0f);

		EchoPCGObjectDesc desc;
		desc.mSeed = m_pScheduler->getRandomSeed(mId);
		desc.mPosition = DVector3::ZERO;
		desc.mRotation = Quaternion::IDENTITY;
		desc.mScale = Vector3::UNIT_SCALE;
		desc.mBound.setExtents(-halfSize, halfSize);
		desc.mBasePosition = m_pScheduler->getPosition();
		desc.mBaseRotation = m_pScheduler->getRotation();
		desc.mBaseScale = m_pScheduler->getScale();
		desc.mID = mId;
		desc.mRootActorCompPtr = nullptr;

		uint32 biomeGraphSize = (uint32)biomeGraphs.size();

		desc.mPCGGraphs.resize(biomeGraphs.size());
		for (uint32 biomeGraphIndex = 0; biomeGraphIndex < biomeGraphSize; ++biomeGraphIndex) {
			desc.mPCGGraphs[biomeGraphIndex].mGraphName = biomeGraphs[biomeGraphIndex].first;
			desc.mPCGGraphs[biomeGraphIndex].mSurfaceType = biomeGraphs[biomeGraphIndex].second;
		}
		desc.mLevel = PCG::GenObjectRuleLevel::eRoad;

		EchoPCGObject* newPcg = new EchoPCGObject(m_pScheduler->getSceneName());
		newPcg->SetPCGObjectDesc(desc);
		newPcg->SetParent(nullptr);
		newPcg->Prepare();
		{
			newPcg->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SurfaceSampler>>(PCG::SurfaceSampler::gType, new PCG::SphericalTerrainSampler(m_pScheduler, mSameBiomes, false, false));
			PCG::PCGGeneralObjectCache* pStaticMeshCache = newPcg->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
			if (pStaticMeshCache) {
				for (const auto& iter : mData) {
					pStaticMeshCache->mLibMeshMap[iter.first] = iter.second.meshs;
				}
				pStaticMeshCache->SetRenderDistance(m_pScheduler->getRenderDistance(0) * 2.0f);
				pStaticMeshCache->mLibMeshOffset = desc.mSeed;
				pStaticMeshCache->mDeletedObjects = {};
				//auto delObjIter = m_pScheduler->mDeletedObjects.find(id);
				//if (delObjIter != m_pScheduler->mDeletedObjects.end()) {
				//	pStaticMeshCache->mDeletedObjects = delObjIter->second;
				//}
				//else {
				//	pStaticMeshCache->mDeletedObjects = {};
				//}
			}
			newPcg->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SplineSampler>>(PCG::SplineSampler::gType, new PCG::SphericalTerrainRoadSplineSampler(m_pScheduler));
		}
		for (uint32 biomeGraphIndex = 0; biomeGraphIndex < biomeGraphSize; ++biomeGraphIndex) {
			PCG::PCGAnyMap* pAttrSet = newPcg->GetInputAttributeSet(biomeGraphIndex);
			auto biomeInputIter = mBiomeInputs.find(biomeGraphs[biomeGraphIndex].second);
			if (biomeInputIter != mBiomeInputs.end()) {
				*pAttrSet = *biomeInputIter->second;
			}
		}

		mSameBiomes.clear();
		for (auto& iter : mBiomeInputs) {
			SAFE_DELETE(iter.second);
		}
		mBiomeInputs.clear();

		if (mSync) {
			newPcg->Execute();
			if (bCreate) {
				newPcg->PostExecute();
			}
		}
		else {
			newPcg->Schedule();
		}
		m_pPCG = newPcg;
	}

}
