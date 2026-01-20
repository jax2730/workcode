#include "EchoStableHeaders.h"
#include "EchoRoleBatchEntity.h"
#include "EchoRoot.h"
#include "EchoRenderSystem.h"
#include "EchoShadowmapManager.h"
#include "EchoEntity.h"
#include "EchoSubMesh.h"
#include "EchoMaterialManager.h"
#include "../EchoInstanceMaterialHelper_p.h"
#include "EchoMaterialV2Manager.h"
#include "EchoPointLightShadowManager.h"
#include "EchoSkeletonInstance.h"
#include "EchoRoleBatchAnimationTexture.h"
#include "EchoInstanceBatchEntity.h"


#define	NoZore(flt)								(-0.00001f < flt && flt < 0.00001f ? flt < 0 ? -0.00001f : 0.00001f : flt)
#define	uniform_f4_assign_pos(v1,v2)			do { v1.x = v2.x, v1.y = v2.y, v1.z = v2.z; } while(false)
#define	uniform_f4_assign_scale(v1,v2)			do { v1.x = v2.x, v1.y = v2.y, v1.z = v2.z;} while(false)
#define uniform_f4_assign_orientation(v1,v2)	do { v1.x = v2.x, v1.y = v2.y, v1.z = v2.z, v1.w = v2.w;} while(false)
namespace Echo
{
	namespace
	{
		void updateWorldTransform(const SubEntityVec& vecInst, uint32 iUniformCount,
			std::vector<Vector4>& vecData)
		{
			EchoZoneScoped;
			// copy form echoinstanceEntity.cpp func_updateWorldTransform
			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount);
			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				Vector4* _inst_buff = &vecData[i * iUniformCount];
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)_world_matrix->m[i][0];
					_inst_buff[i].y = (float)_world_matrix->m[i][1];
					_inst_buff[i].z = (float)_world_matrix->m[i][2];
					_inst_buff[i].w = (float)_world_matrix->m[i][3];
				}

				//Vector3 position, scale;
				//Quaternion orientation;
				//_world_matrix->decomposition(position, scale, orientation);
				DBMatrix4 inv_transpose_world_matrix;
				const DBMatrix4* inv_world_matrix = nullptr;
				pSubEntity->getInvWorldTransforms(&inv_world_matrix);
				//inv_transpose_world_matrix.makeInverseTransform(position, scale, orientation);
				inv_transpose_world_matrix = inv_world_matrix->transpose();
				_inst_buff += 3;
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)inv_transpose_world_matrix.m[i][0];
					_inst_buff[i].y = (float)inv_transpose_world_matrix.m[i][1];
					_inst_buff[i].z = (float)inv_transpose_world_matrix.m[i][2];
					_inst_buff[i].w = (float)inv_transpose_world_matrix.m[i][3];
				}
			}
		}
		void updateWorldViewTransform(const SubEntityVec& vecInst, uint32 iUniformCount,
			std::vector<Vector4>& vecData)
		{
			EchoZoneScoped;
			// copy form echoinstanceEntity.cpp func_updateWorldTransform
			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount);

			DVector3 camPos = DVector3::ZERO;
			if (iInstNum != 0)
			{
				Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
				camPos = pCam->getDerivedPosition();
			}

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DBMatrix4 world_matrix_not_cam = *_world_matrix;
				world_matrix_not_cam[0][3] -= camPos[0];
				world_matrix_not_cam[1][3] -= camPos[1];
				world_matrix_not_cam[2][3] -= camPos[2];
				Vector4* _inst_buff = &vecData[i * iUniformCount];
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)world_matrix_not_cam.m[i][0];
					_inst_buff[i].y = (float)world_matrix_not_cam.m[i][1];
					_inst_buff[i].z = (float)world_matrix_not_cam.m[i][2];
					_inst_buff[i].w = (float)world_matrix_not_cam.m[i][3];
				}

				//Vector3 position, scale;
				//Quaternion orientation;
				//_world_matrix->decomposition(position, scale, orientation);
				DBMatrix4 inv_transpose_world_matrix;
				const DBMatrix4* inv_world_matrix = nullptr;
				pSubEntity->getInvWorldTransforms(&inv_world_matrix);
				//inv_transpose_world_matrix.makeInverseTransform(position, scale, orientation);
				inv_transpose_world_matrix = inv_world_matrix->transpose();
				_inst_buff += 3;
				for (int i = 0; i < 3; ++i)
				{
					_inst_buff[i].x = (float)inv_transpose_world_matrix.m[i][0];
					_inst_buff[i].y = (float)inv_transpose_world_matrix.m[i][1];
					_inst_buff[i].z = (float)inv_transpose_world_matrix.m[i][2];
					_inst_buff[i].w = (float)inv_transpose_world_matrix.m[i][3];
				}
			}
		}
		void updateWorldPositionScaleOrientation(const SubEntityVec& vecInst, uint32 iUniformCount,
			std::vector<Vector4>& vecData)
		{
			EchoZoneScoped;

			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount);

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DVector3 position;
				Vector3 scale;
				Quaternion orientation;
				_world_matrix->decomposition(position, scale, orientation);
				scale.x = NoZore(scale.x);
				scale.y = NoZore(scale.y);
				scale.z = NoZore(scale.z);
				Vector4* _inst_buff = &vecData[i * iUniformCount];

				Vector3 tempPos = position;
				uniform_f4_assign_pos(_inst_buff[0], tempPos);
				uniform_f4_assign_scale(_inst_buff[1], scale);
				uniform_f4_assign_orientation(_inst_buff[2], orientation);
			}
		}

		void updateWorldViewPositionScaleOrientation(const SubEntityVec& vecInst, uint32 iUniformCount,
			std::vector<Vector4>& vecData)
		{
			EchoZoneScoped;

			size_t iInstNum = vecInst.size();
			vecData.resize(iInstNum * iUniformCount);

			DVector3 camPos = DVector3::ZERO;
			if (iInstNum != 0)
			{
				Camera* pCam = vecInst[0]->getParent()->_getManager()->getActiveCamera();
				camPos = pCam->getDerivedPosition();
			}

			for (size_t i = 0u; i < iInstNum; ++i)
			{
				SubEntity* pSubEntity = vecInst[i];
				if (nullptr == pSubEntity)
					continue;

				const DBMatrix4* _world_matrix = nullptr;
				pSubEntity->getWorldTransforms(&_world_matrix);
				DVector3 position;
				Vector3 scale;
				Quaternion orientation;
				_world_matrix->decomposition(position, scale, orientation);
				position -= camPos;
				scale.x = NoZore(scale.x);
				scale.y = NoZore(scale.y);
				scale.z = NoZore(scale.z);
				Vector4* _inst_buff = &vecData[i * iUniformCount];

				Vector3 tempPos = position;
				uniform_f4_assign_pos(_inst_buff[0], tempPos);
				uniform_f4_assign_scale(_inst_buff[1], scale);
				uniform_f4_assign_orientation(_inst_buff[2], orientation);
			}
		}

	}
	
	struct RoleInstanceUniformInfo
	{
		typedef std::function<void(const SubEntityVec& vecInst, uint32 iUniformCount, std::vector<Vector4>& vecData)> UniformFunc;
		uint32			uniformTrunkLength = 0;
		UniformFunc		uniformConstructor;
	};
	static const std::unordered_map<std::string , RoleInstanceUniformInfo> gShaderSupported =
	{
		{ "Illum",				{6 , updateWorldViewTransform}},
		{ "IllumPBR",			{6 , updateWorldViewTransform}},
		{ "IllumPBR2022",		{6 , updateWorldViewTransform}},
		{ "IllumPBR2023",		{6 , updateWorldViewTransform}},
		{ "BillboardLeaf",		{3 , updateWorldViewPositionScaleOrientation}},
		{ "BillboardLeafPBR",	{3 , updateWorldViewPositionScaleOrientation}},
		//{ "SpecialIllumPBR",	{6 , updateWorldViewTransform}},
		{ "Trunk",				{3 , updateWorldViewPositionScaleOrientation}},
		{ "TrunkPBR",			{3 , updateWorldViewPositionScaleOrientation}},
	};

	class RoleBatchEntityRender : public Renderable
	{
	public:
		RoleBatchEntityRender()
		{
			EchoZoneScoped;

			enableCustomParameters(true);
			enableInstanceRender(true);
		}

		virtual RenderOperation* getRenderOperation() override
		{
			EchoZoneScoped;
			return &mBatch->mRenderOperation;
		}

		virtual const LightList& getLights() const override
		{
			EchoZoneScoped;

			return RoleBatchEntity::GetLightList();
		}

		virtual const Material* getMaterial() const override
		{
			EchoZoneScoped;
			return mBatch->mMaterial.getV1();
		}

		virtual const MaterialWrapper& getMaterialWrapper(void) const override
		{
			EchoZoneScoped;
			return mBatch->mMaterial;
		}

		virtual uint32 getInstanceNum() override
		{
			EchoZoneScoped;
			return mInstNum;
		}

		virtual void setCustomParameters(const Pass* pPass) override
		{
			EchoZoneScoped;
			mBatch->updatePassParam(this, pPass, mInstPartIdx, mInstIdxStart, mInstNum);
		}

		virtual void getIndexBuffInfo(IndexBuffInfo& indexInfo) override
		{
			EchoZoneScoped;
			indexInfo = mBatch->mIndexBuffInfo;
		}

		uint32 mInstPartIdx = 0u;
		uint32 mInstIdxStart = 0u;
		uint32 mInstNum = 0u;
		RoleBatchEntity* mBatch = nullptr;
	}; 
	


	RcBuffer * RoleBatchEntity::mBuffInstID = nullptr;
	LightList RoleBatchEntity::mLightList;
	
	RoleBatchEntity::RoleBatchEntity(RoleInstanceKey key)
		: mKey(key)
	{
EchoZoneScoped;

		mInstances.reserve(32);
	}

	RoleBatchEntity::~RoleBatchEntity()
	{
EchoZoneScoped;

		if (mMatTick != 0)
			InstanceMaterialHelper::instance()->abortRequest(mMatTick);
		mMatTick = 0;
		auto & data = mRenderOperation.m_stMeshData;
		data.m_vertexBuffer.clear();
		data.m_vertexCount = data.m_vertexBuffSize = 0;
		data.m_IndexBufferData.m_indexBuffer = nullptr;
		data.m_IndexBufferData.m_indexCount = data.m_indexBufferSize = 0;

		for (auto i : mRenderPool)
			delete i;
		mRenderPool.clear();
	}

	RoleInstanceKey RoleBatchEntity::GenBatchKey(SubEntity* pSubEntity)
	{
		RoleInstanceKey key = InstanceBatchEntity::GenBatchKey(pSubEntity);

		Entity* pParEntity = pSubEntity->getParent();
		if (pParEntity)
		{
			key.m |= pParEntity->getEntityType() == RoleEntity ? (RoleInstanceKey::emUseRoleStencil) : 0;
		}

		return key;
	}

	bool RoleBatchEntity::init(SubEntity *pSubEntity)
	{
EchoZoneScoped;

		MaterialWrapper baseMat;
		if (pSubEntity != nullptr)
			baseMat = pSubEntity->getRealMaterialWrapper();
		if (baseMat.isNull())
			return false;

		SubMesh *pSubMesh = pSubEntity->getSubMesh();
		RenderOperation *pOp = pSubMesh->_getRenderOperation();
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
		if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass)
		{
			mRenderOperation.m_stMeshData.m_vertexBuffer.reserve(
				pOp->m_stMeshData.m_vertexBuffer.size() + 1);
		}
		mRenderOperation.m_stMeshData = pOp->m_stMeshData;
		if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass) {
			mRenderOperation.m_stMeshData.m_vertexBuffer.push_back(mBuffInstID);
		}

		pSubEntity->getIndexBuffInfo(mIndexBuffInfo);
		m_pSubMesh = pSubMesh;

		if (baseMat.isV1())
		{
			mUInfo = &gShaderSupported.at(baseMat.getV1()->m_matInfo.shadername);
		}
		else if (baseMat.isV2())
		{
			mUInfo = &gShaderSupported.at(baseMat.getV2()->getShaderPassName().toString());
		}
		if (!initCaps())
			return false;
		// custom config
		mUseRoleStencil = (mKey.m & RoleInstanceKey::emUseRoleStencil) == RoleInstanceKey::emUseRoleStencil;

		// create batch material, include point light, shader resive
		SceneManager *pSceneMgr = pSubEntity->getParent()->_getManager();
		bool bShadowEnable = pSceneMgr->getPsmshadowManager()->getShadowEnable();
		const bool pointShadowEnable = pSceneMgr->getPointShadowManager()->IsReceivingShadow(AxisAlignedBox::BOX_INFINITE);
		initMaterial(baseMat, bShadowEnable, pointShadowEnable);
		return true;
	}

	bool RoleBatchEntity::initCaps()
	{
EchoZoneScoped;

		uint16 boneCnt = (uint16)m_pSubMesh->getIndexToBone().size();
		if (boneCnt == 0)
		{
			return false;
		}

		mInstanceCount = MaxRoleBatchCount;

		mInstanceMacros.clear();
		mInstanceMacros.resize(4);
		mInstanceMacros[0] = ShaderMacrosItem("SKINNEDINSTANCE");
		mInstanceMacros[1] = ShaderMacrosItem("InstanceCount", (int)mInstanceCount);
		mInstanceMacros[3] = ShaderMacrosItem("UniformCount", (int)mUInfo->uniformTrunkLength);

		return true;
	}

	void RoleBatchEntity::initMaterial(const MaterialWrapper& baseMaterial, bool bShadowEnable, bool pointShadowEnable)
	{
		EchoZoneScoped;

		if (baseMaterial.isV1())
		{
			Material* pBaseMat = baseMaterial.getV1();

			MaterialPtr ptrMat = MaterialManager::instance()->createManual();
			ptrMat->m_desc = pBaseMat->m_desc;
			ptrMat->m_matInfo = pBaseMat->m_matInfo;

			if (Root::instance()->getDeviceType() == ECHO_DEVICE_DX9C)
			{
				// Add an extra instance id buffer
				uint32 idxInst = ptrMat->m_desc.IADesc.nStreamCount;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].bPreInstance = ECHO_TRUE;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nRate = 1;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nStride = 4;
				++ptrMat->m_desc.IADesc.nStreamCount;

				uint32 idxInstElem = ptrMat->m_desc.IADesc.nElementCount;
				ECHO_IA_ELEMENT& refElem = ptrMat->m_desc.IADesc.ElementArray[idxInstElem];
				refElem.nStream = idxInst;
				refElem.nIndex = 0u;
				refElem.eType = ECHO_UBYTE4;
				refElem.nOffset = 0u;
				refElem.Semantics = ECHO_BLENDINDICES1;
				++ptrMat->m_desc.IADesc.nElementCount;
			}

			String strVS, strPS, strVSMacro, strPSMacro;
			strVSMacro.reserve(128);
			strPSMacro.reserve(128);

			MaterialManager::instance()->getShader(ptrMat.get(), strVS, strPS, strVSMacro, strPSMacro);
			for (const ShaderMacrosItem& macro : mInstanceMacros)
			{
				strVSMacro.append(macro.toString());
			}
			ptrMat->setShaderSrcCode(strVS, strPS);
			ptrMat->setMacroShaderSrcCode(strVSMacro, strPSMacro);

			if (mUseRoleStencil)
			{
				ptrMat->m_desc.RenderState.StencilEnable = ECHO_TRUE;
				ptrMat->m_desc.RenderState.StencilRef = HAIR_STENCIL_MASK;
				ptrMat->m_desc.RenderState.StencilMask = HAIR_STENCIL_MASK | FIRST_PERSON_STENCIL_MASK;
				ptrMat->m_desc.RenderState.StencilWriteMask = 0xff ^ FIRST_PERSON_STENCIL_MASK;
				ptrMat->m_desc.RenderState.StencilFunc = ECHO_CMP_GREATEREQUAL;
				ptrMat->m_desc.RenderState.StencilPass = ECHO_STENCIL_OP_REPLACE;
			}

			ptrMat->createPass(InstancePass);
			ptrMat->createPass(InstanceLightPass);
			ptrMat->createPass(InstanceRipplePass);
			ptrMat->createPass(InstanceLightRipplePass);
			if (bShadowEnable)
			{
				ptrMat->createPass(ShadowResiveInstanceRipplePass);
				ptrMat->createPass(ShadowResiveInstanceLightRipplePass);
				ptrMat->createPass(ShadowResiveInstancePass);
				ptrMat->createPass(ShadowResiveInstanceLightPass);
				ptrMat->createPass(ShadowCasterInstancePass);
			}
			if (pointShadowEnable)
			{
				ptrMat->createPass(PointShadowCasterInstance);
				ptrMat->createPass(PointShadowReceiverInstance);
				ptrMat->createPass(PointShadowReceiverRippleInstance);
			}
			if (bShadowEnable && pointShadowEnable)
			{
				ptrMat->createPass(ShadowReceiverPointShadowReceiverInstance);
				ptrMat->createPass(ShadowReceiverPointShadowReceiverRippleInstance);
			}

			mMatTick = InstanceMaterialHelper::instance()->addEntityMatReq(this, ptrMat);
		}
		else if (baseMaterial.isV2())
		{
			MaterialV2* pBaseMat = baseMaterial.getV2();

			MaterialV2Ptr ptrMat = MaterialV2Manager::instance()->createManual();

			Mat::MaterialInfo matInfo = pBaseMat->getMaterialInfo();
			ECHO_IA_DESC iaDesc = pBaseMat->getVertexInputAttributeDesc();

			PassMaskFlags basePassMaskFlags = pBaseMat->getBasePassMaskFlags();
			basePassMaskFlags |= PassMaskFlag::UseInstance;
			if (bShadowEnable)
			{
				basePassMaskFlags |= PassMaskFlag::ReceiveShadow;
			}
			if (pointShadowEnable)
			{
				basePassMaskFlags |= PassMaskFlag::UseDpsm;
			}
			if (mUseRoleStencil)
			{
				basePassMaskFlags |= PassMaskFlag::UseRoleStencil;
			}

			ptrMat->setBasePassMaskFlags(basePassMaskFlags);
			ptrMat->createFromMaterialInfo(matInfo, false);
			ptrMat->setVertexInputAttributeDesc(iaDesc);
			ptrMat->setCustomVSMacros(mInstanceMacros);

			mMatTick = InstanceMaterialHelper::instance()->addEntityMatReq(this, ptrMat);
		}
	}

	void RoleBatchEntity::materialLoaded(const MaterialWrapper &mat,uint64 requstID)
	{
EchoZoneScoped;

		mMaterial = mat;
		mMatTick = 0;
	}

	bool RoleBatchEntity::addInstance(SubEntity *pSubEntity)
	{
EchoZoneScoped;

#ifdef _DEBUG
		auto it = std::find(mInstances.begin(), mInstances.end(), pSubEntity);
		if (it != mInstances.end())
		{
			assert(false && "The instance has been added!");
			return false;
		}
#endif
		if (mMaterial.isNull())
			return false;
		uint32 idx = RoleBatchAnimationTextureManager::instance()->AllocTextureBuffer(pSubEntity);
		if (idx == -1)
			return false;
		mInstances.push_back(pSubEntity);
		mAnimationData.push_back(idx);
		return true;
	}

//	bool RoleBatchEntity::addSelfInstance(SubEntity* pSubEntity)
//	{
//#ifdef _DEBUG
//		auto it = std::find(mSelfInstances.begin(), mSelfInstances.end(), pSubEntity);
//		if (it != mSelfInstances.end())
//		{
//			assert(false && "The instance has been added!");
//			return false;
//		}
//#endif
//		mSelfInstances.push_back(pSubEntity);
//		return true;
//	}

//	bool RoleBatchEntity::preaddInstance(SubEntity* pSubEntity)
//	{
//EchoZoneScoped;
//
//#ifdef _DEBUG
//		auto it = std::find(mPerInstances.begin(), mPerInstances.end(), pSubEntity);
//		if (it != mPerInstances.end())
//		{
//			assert(false && "The instance has been added!");
//			return false;
//		}
//#endif
//		if (mMaterial.isNull())
//			return false;
//		mPerInstances.push_back(pSubEntity);
//		return true;
//	}
	
	void RoleBatchEntity::clearInstance()
	{
EchoZoneScoped;

		mInstances.clear();
		//mSelfInstances.clear();
		//mPerInstances.clear();
		mInstanceData.clear();
		mAnimationData.clear();
	}

	bool RoleBatchEntity::hasInstance() const
	{
EchoZoneScoped;

		return !mInstances.empty();
	}

	uint32 RoleBatchEntity::getInstanceCount() const
	{
		return (uint32)mInstances.size();
	}

	//bool RoleBatchEntity::hasPerinstance() const
	//{
	//	return !mPerInstances.empty();
	//}

	//uint32 RoleBatchEntity::getPerinstanceCount() const
	//{
	//	return uint32(mPerInstances.size());
	//}

	//SubEntity* RoleBatchEntity::getPerinstance(uint32 idx) const
	//{
	//	return mPerInstances[idx];
	//}

	std::tuple<uint32, uint32> RoleBatchEntity::updateRenderQueue(PassType typePass, RenderQueue *pRenderQueue)
	{
EchoZoneScoped;

		std::tuple<uint32, uint32> tpNum(0u, 0u);
		if (!mMaterial.isV1())
			return tpNum;

		Pass *pPass = mMaterial.getV1()->getPass(typePass);
		if (nullptr == pPass)
			return tpNum;

		mUInfo->uniformConstructor(mInstances, mUInfo->uniformTrunkLength, mInstanceData);

		RenderQueueGroup *pGroup = nullptr;
		if (mMaterial.getV1()->m_matInfo.alphaTestEnable)
			pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest);
		else
			pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_Solid);

		uint32 iTotalInstNum = (uint32)mInstances.size();
		uint32 iRenderIdx = 0;
		uint32 iInstIdx = 0;
		for (; iInstIdx < iTotalInstNum; iInstIdx += mInstanceCount)
		{
			// 准备资源
			if (iRenderIdx >= mRenderPool.size())
				mRenderPool.push_back(new RoleBatchEntityRender);
			RoleBatchEntityRender* pRender = mRenderPool[iRenderIdx];
			// 添加到渲染
			pRender->clearDrawData();
			pRender->mInstPartIdx = iRenderIdx;
			pRender->mInstIdxStart = iInstIdx;
			pRender->mInstNum = mInstanceCount;
			pRender->mBatch = this;
			if (pRender->mInstIdxStart + pRender->mInstNum >= iTotalInstNum)
				pRender->mInstNum = iTotalInstNum - pRender->mInstIdxStart;
			pGroup->addRenderable(pPass, pRender);
			++iRenderIdx;
		}
		//for (SubEntity* pEntity : mSelfInstances)
		//{
		//	pGroup->addRenderable(pPass, pEntity);
		//}
		Root::instance()->addBatchDrawCount(iTotalInstNum - iRenderIdx);
		return std::tuple<uint32, uint32>(iTotalInstNum, iRenderIdx);
	}

	std::tuple<uint32, uint32> RoleBatchEntity::updateRenderQueueV2(bool shadowPass, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue)
	{
		EchoZoneScoped;

		std::tuple<uint32, uint32> tpNum(0u, 0u);
		if (!mMaterial.isV2())
			return tpNum;

		MaterialV2* mat = mMaterial.getV2();

		mUInfo->uniformConstructor(mInstances, mUInfo->uniformTrunkLength, mInstanceData);

		uint32 iTotalInstNum = (uint32)mInstances.size();
		uint32 iRenderIdx = 0;
		uint32 iInstIdx = 0;
		for (; iInstIdx < iTotalInstNum; iInstIdx += mInstanceCount)
		{
			// 准备资源
			if (iRenderIdx >= mRenderPool.size())
				mRenderPool.push_back(new RoleBatchEntityRender);
			RoleBatchEntityRender* pRender = mRenderPool[iRenderIdx];
			// 添加到渲染
			pRender->clearDrawData();
			pRender->mInstPartIdx = iRenderIdx;
			pRender->mInstIdxStart = iInstIdx;
			pRender->mInstNum = mInstanceCount;
			pRender->mBatch = this;
			if (pRender->mInstIdxStart + pRender->mInstNum >= iTotalInstNum)
				pRender->mInstNum = iTotalInstNum - pRender->mInstIdxStart;

			PostRenderableParams postParams{
				.material = mat,
				.renderable = pRender,
				.renderQueue = pRenderQueue,
				.passMaskFlags = passMaskFlags,
				.basePriority = DEFAULT_PRIORITY,
				.isShadowPass = shadowPass
			};
			MaterialShaderPassManager::fastPostRenderable(postParams);

			++iRenderIdx;
		}
		//for (SubEntity* pEntity : mSelfInstances)
		//{
		//	MaterialShaderPassManager::postRenderableToQueue(mat, pEntity, pRenderQueue, stageType, passMaskFlags, DEFAULT_PRIORITY);
		//}
		Root::instance()->addBatchDrawCount(iTotalInstNum - iRenderIdx);
		return std::tuple<uint32, uint32>(iTotalInstNum, iRenderIdx);
	}

	void RoleBatchEntity::updatePassParam(const Renderable *pRend,
	                                      const Pass *pPass, uint32 iInstPartIdx ,uint32 iInstIdxStart, uint32 iInstNum)
	{
EchoZoneScoped;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		// 世界矩阵
		pRenderSystem->setUniformValue(pRend, pPass, U_VSCustom0,
			mInstanceData.data() + iInstIdxStart * mUInfo->uniformTrunkLength,
			iInstNum * mUInfo->uniformTrunkLength * sizeof(mInstanceData[0]));
		// 动画信息
		pRenderSystem->setUniformValue(pRend, pPass, U_VSCustom1,
			mAnimationData.data() + iInstIdxStart,
			iInstNum  * sizeof(mAnimationData[0]));
		// 动画纹理
		pRenderSystem->setTextureSampleValue(pRend, pPass, S_2DExt0,
			RoleBatchAnimationTextureManager::instance()->mAnimationTexture,
			RoleBatchAnimationTextureManager::instance()->mAnimationSmapler);
	}


	void RoleBatchEntity::SetLightList(const LightList &lights)
	{
EchoZoneScoped;

		mLightList = lights;
	}

	const Echo::LightList & RoleBatchEntity::GetLightList(void)
	{
EchoZoneScoped;

		return mLightList;
	}

	void RoleBatchEntity::Init()
	{
EchoZoneScoped;

		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();

		if (eClass == ECHO_DEVICE_DX9C)
		{
			uint8 iInstID[MaxRoleBatchCount * 4] = { 0u, };
			for (uint32 i=0; i<MaxRoleBatchCount; ++i)
			{
				iInstID[i * 4 + 0] = (uint8)i;
				iInstID[i * 4 + 1] = 0;
				iInstID[i * 4 + 2] = 0;
				iInstID[i * 4 + 3] = 0;
			}
			mBuffInstID = Root::instance()->getRenderSystem()->createStaticVB(iInstID, MaxRoleBatchCount * 4);
		}
	}

	void RoleBatchEntity::Destroy()
	{
EchoZoneScoped;

		if (mBuffInstID != nullptr)
		{
			Root::instance()->getRenderSystem()->destoryBuffer(mBuffInstID);
			mBuffInstID = nullptr;
		}
	}

	bool RoleBatchEntity::IsShaderSupported( const String & sShader)
	{
EchoZoneScoped;
		if (gShaderSupported.find(sShader) != gShaderSupported.end())
			return true;
		return false;
	}
	void RoleBatchEntity::getAllSupprtedShader( std::vector<String>& shaders)
	{
		shaders.clear();

		for (const auto& el : gShaderSupported)
		{
			shaders.push_back(el.first);
		}
	}
	uint32 RoleBatchEntity::GetInstancesMaxCount()
	{
		return MaxRoleBatchCount;
	}
	// InstnaceBatchEntity
	//--------------------------------------------------------------------------

} // namespace Echo
