#include "EchoComponentStableHeaders.h"
#include "EchoPCGGraphComponent.h"

#include "EchoPCGGraphComponentManager.h"
#include "EchoOAESystem.h"
#include "EchoPCGObject.h"
#include "EchoPCGBaseElement.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoPCGClutter.h"

namespace Echo {
	struct NotTerrainSampler : public PCG::SurfaceSampler {
		NotTerrainSampler() {}
		~NotTerrainSampler() {}
		virtual bool Sampler(const SamplerParam& param, std::shared_ptr<PCG::PCGDataSet>& pcgDataSet) override {
			PCG::PCGPointSet pointSet(*pcgDataSet);
			if (pointSet.getSize() == 0) {
				return false;
			}
			return true;
		}
	};

	class KarstTerrainSampler : public PCG::SurfaceSampler {
	public:
		KarstTerrainSampler(OAE::OAEManager* _data) : data(_data) {}
		~KarstTerrainSampler() {}
		virtual bool Sampler(const SamplerParam& param, std::shared_ptr<PCG::PCGDataSet>& pcgDataSet) override {
			PCG::PCGPointSet pointSet(*pcgDataSet);
			if (pointSet.getSize() == 0) {
				return false;
			}
			float* pSlope = param.Slope ? pcgDataSet->addData<float>(PCGStaticAttribute::gSlope) : nullptr;
			Vector3 curNormal = Vector3::UP;
			for (uint32 index = 0; index != pointSet.getSize(); ++index) {
				DVector3 worldPos = param.pOriginSpace->ToWorldPosition(pointSet.getWorldPosition(index));
				worldPos.y = data->getTerrainHeight(worldPos.x, worldPos.z);
				pointSet.getSurfaceType(index) = data->getTerrainSurfaceType(worldPos.x, worldPos.z);
				Quaternion quat = Quaternion::IDENTITY;
				if (param.ApplySlope) {
					curNormal = data->getTerrainNormal(worldPos.x, worldPos.z);
					quat.RotationAxisToAxis(Vector3::UP, curNormal);
					pointSet.setWorldRotation(index, param.pOriginSpace->ToLocalRotation(quat));
				}
				else if (param.ApplyGravity) {
					curNormal = Vector3::UP;
					pointSet.setWorldRotation(index, param.pOriginSpace->ToLocalRotation(quat));
				}
				if (pSlope) {
					pSlope[index] = Vector3::Angle(Vector3::UP, curNormal);
				}
				if (param.VertexGround) {
					pointSet.getBoundMin(index);
					DVector3 verts[4] = {
						worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMin(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMax(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMin(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMax(index).z)
					};

					float maxHeight = -FLT_MAX;
					for (int verIndex = 0; verIndex < 4; ++verIndex) {
						float gtvDotDir = (float)(verts[verIndex].y - data->getTerrainHeight(verts[verIndex].x, verts[verIndex].z));
						maxHeight = std::max(maxHeight, gtvDotDir);
					}
					if (maxHeight > 0.0f) {
						worldPos -= maxHeight * Vector3::UP;
					}
					pointSet.setWorldPosition(index, param.pOriginSpace->ToLocalPosition(worldPos));
				}
				else {
					pointSet.setWorldPosition(index, param.pOriginSpace->ToLocalPosition(worldPos));
				}
			}
			return true;
		}
		OAE::OAEManager* data = nullptr;
	};

	struct SphericalTerrainSampler : public PCG::SurfaceSampler {
	public:
		SphericalTerrainSampler(const SphericalTerrainSampler& other) {
			*this = other;
		}
		SphericalTerrainSampler(SphericalTerrain* _data) : data(_data) {}
		~SphericalTerrainSampler() {}

		virtual bool Sampler(const SamplerParam& param, std::shared_ptr<PCG::PCGDataSet>& pcgDataSet) override {
			PCG::PCGPointSet pointSet(*pcgDataSet);
			if (pointSet.getSize() == 0) {
				return false;
			}
			float* pSlope = param.Slope ? pcgDataSet->addData<float>(PCGStaticAttribute::gSlope) : nullptr;
			Vector3 curNormal = Vector3::UP;
			for (uint32 index = 0; index != pointSet.getSize(); ++index) {
				DVector3 worldPos = param.pOriginSpace->ToWorldPosition(pointSet.getWorldPosition(index));
				worldPos = data->getSurfaceFinestWs(worldPos, &curNormal);
				//pointSet.getSurfaceType(index) = data->getSurfaceTypeWs(worldPos);
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
				if (pSlope) {
					pSlope[index] = Vector3::Angle(upDir, curNormal);
				}
				if (param.VertexGround) {
					pointSet.getBoundMin(index);
					DVector3 verts[4] = {
						worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMin(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMin(index).x, 0.0f, pointSet.getBoundMax(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMin(index).z),
						worldPos + quat * Vector3(pointSet.getBoundMax(index).x, 0.0f, pointSet.getBoundMax(index).z)
					};
					float maxHeight = -FLT_MAX;
					for (int verIndex = 0; verIndex < 4; ++verIndex) {
						Vector3 groundToVert = verts[verIndex] - data->getSurfaceFinestWs(verts[verIndex]);
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
			}
			return true;
		}

		SphericalTerrain* data = nullptr;
	};

	class SphericalTerrainSplineSampler : public PCG::SplineSampler {
	public:
		SphericalTerrainSplineSampler(SphericalTerrain* _data) : data(_data) {}
		virtual ~SphericalTerrainSplineSampler() {}
		virtual bool GetSpline(const PCG::PCGOriginSpace* pOriginSpace, std::vector<PCG::SplineSampler::Spline>& splines) override {
			std::vector<PlanetRoadData> resultRoad;
			if (!data->getPlanetRoad()) {
				data->BuildPlanetRoad(true);
			}
			if (data->getPlanetRoad()) {
				data->getPlanetRoad()->getRoadBeziers(pOriginSpace->GetWorldBound(), resultRoad);
			}
			if (!resultRoad.empty()) {
				for (auto& road : resultRoad) {
					std::vector<Vector3> temp = PlanetRoad::getRoadSubdivideAABB(road, 20.0f);
					int tempSize = (int)temp.size();
					if (tempSize > 1) {
						PCG::SplineSampler::Spline& spline = splines.emplace_back();
						spline.bClosed = false;
						spline.points.resize(tempSize);
						for (int tempIndex = 0; tempIndex < tempSize; ++tempIndex) {
							DVector3 pos = data->getSurfaceFinestWs(temp[tempIndex] + data->m_pos);
							spline.points[tempIndex].position = pOriginSpace->ToLocalPosition(pos);
							spline.points[tempIndex].up = pOriginSpace->mRotation.Inverse() * -data->getGravityWs(pos);
						}

					}
				}
				return true;
			}
			return false;
		}
		SphericalTerrain* data = nullptr;
	};

}

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(PCGGraphComponent)
	EDITOR_REFLECT_BEGIN(PCGGraphComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_PCGGraphPath, "PCGGraphPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BoundSize, "BoundSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Seed, "Seed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_PCGInfo, "PCGInfo", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(PCGGraphComponentInfo)

	PCGGraphComponent::PCGGraphComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:NodeActorComponent(pActor, type)
	{
	}

	PCGGraphComponent::~PCGGraphComponent() {}

	void PCGGraphComponent::init(ActorComponentInfo* info) {

		if (info) {
			PCGGraphComponentInfo* pInfo = static_cast<PCGGraphComponentInfo*>(info);
			mPCGGraphPath = pInfo->GetPCGGraphPath();
			mBoundSize = pInfo->GetBoundSize();
			mSeed = pInfo->GetSeed();
		}
		else {
			mPCGGraphPath = "";
			mBoundSize = Vector3::UNIT_SCALE * 256.0f;
			mSeed = 0;
		}
		bInit = true;
		SetNotifiedUpdate(true);
		PCGSystem::instance()->getComponentManager()->AddComponent(this);
	}
	void PCGGraphComponent::unInit() {
		PCGSystem::instance()->getComponentManager()->RemoveComponent(this);
		SetNotifiedUpdate(false);
		ClearPCGObject();
		bInit = false;
	}
	void PCGGraphComponent::replay(ActorComponentInfo* info) {

	}
	void PCGGraphComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		PCGGraphComponent* other = static_cast<PCGGraphComponent*>(actCom.getPointer());
		other->SetPCGGraphPath(GetPCGGraphPath());
		other->SetBoundSize(GetBoundSize());
		other->SetSeed(GetSeed());
	}
	void PCGGraphComponent::connectCom() {
		MarkNeedExcutePCGGraph();
	}
	void PCGGraphComponent::disconnectCom() {

	}
	void PCGGraphComponent::NotifiChildImp(int changeMask) {
		if ((uint8)changeMask == WorldNode::TSC_NONE) return;
		MarkNeedExcutePCGGraph();
	}
	void PCGGraphComponent::visibleStateChanged(bool activeState) {

	}
	AxisAlignedBox PCGGraphComponent::getLoadBoundingBox() {
		return AxisAlignedBox(-mBoundSize, mBoundSize);
	}
	void PCGGraphComponent::updateCombinedName()
	{
		MarkNeedExcutePCGGraph();
	}

	void PCGGraphComponent::SetPCGGraphPath(const String& file)
	{
		mPCGGraphPath = file;
		MarkNeedExcutePCGGraph();
	}
	void PCGGraphComponent::SetBoundSize(const Vector3& size) {
		mBoundSize = size;
		MarkNeedExcutePCGGraph();
		UpdateHelperVisible();
	}
	void PCGGraphComponent::SetSeed(uint32 seed) {
		mSeed = seed;
		MarkNeedExcutePCGGraph();
	}
	void PCGGraphComponent::SetPCGInfo(const PCG::BiomeGenObjectLayerInfo& info) {
		mPCGInfo = info;
		MarkNeedExcutePCGGraph();
	}

	void PCGGraphComponent::ExcutePCGGraph() {
		if (!mbNeedExcutePCGGraph || !bInit) return;
		mbNeedExcutePCGGraph = false;
		ClearPCGObject();
		PCG::BiomeGenObjectLayer mLayer;
		PCG::BiomeGenObjectLibraryManager::instance()->Generate(0, mPCGInfo, mLayer);
		if (mLayer.path.empty()) {
			mLayer.path = mPCGGraphPath;
		}
		if (!mLayer.path.empty()) {
			m_pEchoPCGObject = new EchoPCGObject(getActorManager()->getName());
			DVector3 worldPos = getWorldPosition();
			EchoPCGObjectDesc desc;
			desc.mPCGGraphs = { {mLayer.path,0} };
			desc.mPosition = worldPos;
			desc.mScale = getWorldScale();
			desc.mRotation = getWorldRotation();
			desc.mBound.setExtents(-mBoundSize * 0.5f, mBoundSize * 0.5f);
			desc.mSeed = mSeed;
			desc.mID = mSeed;
			desc.mRootActorCompPtr = getWeakPtr<ActorComponent>();
			m_pEchoPCGObject->SetPCGObjectDesc(desc);
			m_pEchoPCGObject->Prepare();
			SphericalTerrainComponentPtr pComp = getActor()->GetFirstComByType<SphericalTerrainComponent>();
			if (pComp) {
				SphericalTerrain* pSphTer = pComp->GetSphericalTerrain();
				m_pEchoPCGObject->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SurfaceSampler>>(PCG::SurfaceSampler::gType, new SphericalTerrainSampler(pSphTer));
				m_pEchoPCGObject->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SplineSampler>>(PCG::SplineSampler::gType, new SphericalTerrainSplineSampler(pSphTer));
			}
			else {
				OAE::OAEManager* pOAEMgr = getSceneManager()->getOAEManager();
				if (pOAEMgr) {
					m_pEchoPCGObject->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SurfaceSampler>>(PCG::SurfaceSampler::gType, new KarstTerrainSampler(pOAEMgr));
				}
				else {
					m_pEchoPCGObject->GetRootAttributeSet()->addAttribute<std::shared_ptr<PCG::SurfaceSampler>>(PCG::SurfaceSampler::gType, new NotTerrainSampler());
				}
			}
			PCG::PCGGeneralObjectCache* pStaticMeshCache = m_pEchoPCGObject->GetRootAttributeSet()->getAttribute<PCG::PCGGeneralObjectCache>();
			if (pStaticMeshCache) {
				pStaticMeshCache->mLibMeshMap[0] = mLayer.meshs;
				pStaticMeshCache->mLibMeshOffset = desc.mID;
				pStaticMeshCache->SetRenderDistance(4096.f);
				pStaticMeshCache->mDeletedObjects = {};
			}
			PCG::PCGAnyMap* pInputAttrSet = m_pEchoPCGObject->GetInputAttributeSet(0);
			if (pInputAttrSet) {
				PCG::PCGParamMapToAttrSet(mLayer.params, pInputAttrSet);
				PCG::PCGParamMapToAttrSet(mLayer.pcg_params, pInputAttrSet);
			}
			if (mPreExecuteCallback) {
				mPreExecuteCallback(m_pEchoPCGObject);
			}
			m_pEchoPCGObject->Schedule();
		}
	}

	void PCGGraphComponent::ClearPCGObject()
	{
		if (m_pEchoPCGObject)
		{
			m_pEchoPCGObject->Destroy();
			m_pEchoPCGObject = nullptr;
		}
	}

	void PCGGraphComponent::showEditorHelper(bool isShow) {
		SetHelperVisible(isShow);
	}

	void PCGGraphComponent::SetHelperVisible(bool value) {
		m_IsHelperVisible = value;
		UpdateHelperVisible();
	}

	void PCGGraphComponent::UpdateHelperVisible() {
		if (m_IsHelperVisible) {
			if (!m_pHelperObj) {
				CreateHelper();
			}
			if (m_pHelperObj) {
				UpdateHelper();
			}
		}
		else if (m_pHelperObj) {
			ReleaseHelper();
		}
	}

	void PCGGraphComponent::CreateHelper() {
		if (m_pHelperObj) return;
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager());
		WorldNode* pSN = getSceneNode();
		if (pSN) {
			pSN->addChild(m_pHelperObj->getSceneNode());
		}
		m_pHelperObj->setVisible(m_IsHelperVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
		m_pHelperObj->SetRanderingDistance(4096.0f);
	}
	void PCGGraphComponent::ReleaseHelper() {
		if (m_pHelperObj == NULL) return;
		WorldNode* pSN = getSceneNode();
		if (pSN) {
			pSN->removeChild(m_pHelperObj->getSceneNode());
		}
		delete m_pHelperObj;
		m_pHelperObj = NULL;
	}
	void PCGGraphComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) {
			return;
		}
		std::vector<Vector3> vbVec = {
			Vector3(1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, 1.0f, -1.0f),
			Vector3(1.0f, 1.0f, -1.0f),
			Vector3(1.0f, -1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f)
		};
		std::vector<uint16> ibVec = { 0,1,1,2,2,3,3,0,4,5,5,6,6,7,7,4,0,4,1,5,2,6,3,7 };

		Vector3 halfSize = GetBoundSize() * 0.5f;
		for (Vector3& vb : vbVec) {
			vb *= halfSize;
		}
		m_pHelperObj->changeCustomPolygon(vbVec, ibVec);
	}
}
