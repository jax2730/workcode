//-----------------------------------------------------------------------------
// File:  EchoPavementObject.cpp
//
// Author: LangGuangyue
//
// Date:   2024-04-01
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoPavementObject.h"
#include "EchoVertexIndexData.h"
#include "EchoStableHeaders.h"
//#include "EchoShaderContentManager.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoPavementUtil.h"
#include "EchoRoadManager.h"
#include "EchoMeshSerializer.h"
#include "EchoWorldManager.h"

namespace Echo
{
	//String			mVSDXGenerateMask = "#define DIFFUSE_TEX\n";
	//String			mPSDXGenerateMask = "#define DIFFUSE_TEX\n";

	uint32			vertexSize = 12 * sizeof(float);	// 12 = 3(positon->Vector3) + 3(normal->Vector3) + 4(tangent->Vector4) + 2(uv->Vector2)

	PartPavement::PartPavement(PavementObject* pavementObj)
	{
		mParent = pavementObj;

		mRenderOperation = new RenderOperation();
		mRenderOperation->useIndexes = true;
		mRenderOperation->mPrimitiveType = Echo::ECHO_TRIANGLE_LIST;

		//mMaterial = new Material(NULL, Name::g_cEmptyName);
		//mMaterial->m_desc.RenderState.CullMode = mCullMode;
		//mMaterial->m_desc.RenderState.BlendEnable = ECHO_TRUE;
		//mMaterial->m_desc.RenderState.BlendDst = ECHO_BLEND_INVSRCALPHA;
		//mMaterial->m_desc.RenderState.BlendSrc = ECHO_BLEND_SRCALPHA;
		//mMaterial->m_desc.RenderState.BlendOp = ECHO_BLEND_OP_ADD;
		//mMaterial->m_desc.RenderState.DepthWriteEnable = ECHO_TRUE;
		//mMaterial->m_desc.RenderState.DrawMode = Echo::ECHO_TRIANGLE_LIST;
		//mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_TRUE;
	}

	PartPavement::~PartPavement()
	{
		//SAFE_DELETE(mMaterial);
		SAFE_DELETE(mRenderOperation);

		SAFE_DELETE(m_ptrCustomUniformParams);
	}

	RenderOperation* PartPavement::getRenderOperation()
	{
		return mRenderOperation;
	}

	const Material* PartPavement::getMaterial(void) const
	{
		assert(mMaterial.isV1());
		return mMaterial.getV1();
	}

	const MaterialWrapper& PartPavement::getMaterialWrapper() const
	{
		return mMaterial;
	}

	void PartPavement::getWorldTransforms(Echo::DBMatrix4* xform) const
	{
		// 构建包含 mLocalOrigin 偏移的世界变换矩阵
		// 顶点已经是相对于 mLocalOrigin 的局部坐标，这里需要把 mLocalOrigin 加回去
		const DBMatrix4& parentTransform = mParent->_getParentNodeFullTransform();
		
		// 创建平移矩阵：将局部坐标平移到世界空间
		DBMatrix4 localOriginTransform = DBMatrix4::IDENTITY;
		localOriginTransform.setTrans(mLocalOrigin);
		
		// 最终变换 = 父节点变换 * 局部原点平移
		xform[0] = parentTransform * localOriginTransform;
	}

	void PartPavement::getInvWorldTransforms(const DBMatrix4** xform) const
	{
		if (mParent && mParent->getParentNode())
		{
			*xform = &(mParent->getParentNode()->getInvWorldMatrix());
		}
		else
			*xform = &DBMatrix4::IDENTITY;
	}



	void PartPavement::getWorldTransforms(const Echo::DBMatrix4** xform) const
	{
		// 构建包含 mLocalOrigin 偏移的世界变换矩阵
		const DBMatrix4* parentTransform = mParent->getWorldMatrix();
		
		// 创建平移矩阵：将局部坐标平移到世界空间
		DBMatrix4 localOriginTransform = DBMatrix4::IDENTITY;
		localOriginTransform.setTrans(mLocalOrigin);
		
		// 最终变换 = 父节点变换 * 局部原点平移
		mWorldTransformCache = (*parentTransform) * localOriginTransform;
		xform[0] = &mWorldTransformCache;
	}





	void PartPavement::getRenderableWorldPosition(const DBVector3** pos) const
	{
		if (mParent && mParent->getParentNode())
		{
			*pos = &(mParent->getParentNode()->_getDerivedPosition());
		}
		else
			*pos = &DBVector3::ZERO;
	}

	void PartPavement::getRenderableWorldScale(const Vector3** scale) const
	{
		if (mParent && mParent->getParentNode())
		{
			*scale = &(mParent->getParentNode()->_getDerivedScale());
		}
		else
			*scale = &Vector3::UNIT_SCALE;
	}

	void PartPavement::getRenderableWorldOrientation(const Quaternion** ori) const
	{
		if (mParent && mParent->getParentNode())
		{
			*ori = &(mParent->getParentNode()->_getDerivedOrientation());
		}
		else
			*ori = &Quaternion::IDENTITY;
	}

	void PartPavement::setCustomMaterialUniformParams(uint16 uniformIndex, const Echo::Vector4& val)
	{
		if (m_ptrCustomUniformParams == nullptr)
			m_ptrCustomUniformParams = new UNIFORM_MAP();

		(*m_ptrCustomUniformParams)[uniformIndex][0] = val[0];
		(*m_ptrCustomUniformParams)[uniformIndex][1] = val[1];
		(*m_ptrCustomUniformParams)[uniformIndex][2] = val[2];
		(*m_ptrCustomUniformParams)[uniformIndex][3] = val[3];
	}

	void PartPavement::updateRender(const PartPavementRenderData& data, const DBVector3& localOrigin)
	{
		if (data.material.isNull() || data.index.empty() || data.vertex.empty())
		{
			/*ECHO_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"You must use effective data to update renderdata for the PartPavement",
				"PartPavement::updateRender");*/
			//assert(false && "PartPavement::updateRender -> material or index/vertex is NULL/empty.");
			LogManager::instance()->logMessage("PartPavement::updateRender -> material or index/vertex is NULL/empty.");
			return;
		}
		if (data.vertex.size() > 65535)
		{
			//assert(false && "PartPavement vertex count more than 65535.");
			LogManager::instance()->logMessage("PartPavement vertex count more than 65535.");
			return;
		}

		// 保存局部原点，用于 getWorldTransforms 中构建正确的世界变换矩阵
		mLocalOrigin = localOrigin;

		//setCullMode(data.cullMode);
		//setMaterial(data.material);
		mMaterial = data.material;
		if (mMaterial.isV1())
		{
			if (!mMaterial.getV1()->isLoaded())
				mMaterial.getV1()->load();
			updateDescHash(mMaterial.getV1()->m_desc.IADesc);
		}
		else if (mMaterial.isV2())
		{
			if (!mMaterial.getV2()->isLoaded())
				mMaterial.getV2()->load();
			updateDescHash(mMaterial.getV2()->getVertexInputAttributeDesc());
		}

		size_t VertexBufferSize, IndexBufferSize;
		VertexBufferSize = data.vertex.size() * vertexSize;
		IndexBufferSize = data.index.size() * sizeof(uint16);

		// 顶点位置转换为相对于 localOrigin 的局部坐标（小数值）
		Vector3 localOriginF((float)localOrigin.x, (float)localOrigin.y, (float)localOrigin.z);

		//Vertex
		MemoryDataStream mTempVertexBuffer(VertexBufferSize);
		float* pVertexBuff = static_cast<float*>(static_cast<void*>((unsigned char*)(mTempVertexBuffer.getPtr())));
		for (auto& _vertex : data.vertex)
		{
			Vector3 localPos = _vertex.position - localOriginF;
			*pVertexBuff++ = localPos.x;
			*pVertexBuff++ = localPos.y;
			*pVertexBuff++ = localPos.z;

			*pVertexBuff++ = _vertex.normal.x;
			*pVertexBuff++ = _vertex.normal.y;
			*pVertexBuff++ = _vertex.normal.z;

			*pVertexBuff++ = _vertex.tangent.x;
			*pVertexBuff++ = _vertex.tangent.y;
			*pVertexBuff++ = _vertex.tangent.z;
			*pVertexBuff++ = _vertex.tangent.w;

			*pVertexBuff++ = _vertex.uv.x;
			*pVertexBuff++ = _vertex.uv.y;
		}

		//Index
		MemoryDataStream mTempIndexBuffer(IndexBufferSize);
		uint16* pIndexBuff = (uint16*)mTempIndexBuffer.getPtr();
		for (auto& _index : data.index)
		{
			*pIndexBuff++ = _index;
		}

		//RenderData
		RenderSystem* renderSystem = Root::instance()->getRenderSystem();

		mRenderOperation->GetMeshData().Release();

		mRenderOperation->GetMeshData().m_vertexCount = (uint32)data.vertex.size();
		mRenderOperation->GetMeshData().m_vertexBuffer.push_back(renderSystem->createStaticVB(mTempVertexBuffer.getPtr(), (uint32)VertexBufferSize));

		mRenderOperation->GetMeshData().m_IndexBufferData.m_indexCount = (uint32)data.index.size();
		mRenderOperation->GetMeshData().m_IndexBufferData.m_indexBuffer = renderSystem->createStaticIB(mTempIndexBuffer.getPtr(), (uint32)IndexBufferSize);
		

		//touchMaterial();
	}

	//void PartPavement::setCullMode(ECHO_CULL_MODE mode)
	//{
	//	mCullMode = mode;
	//	mMaterial->m_desc.RenderState.CullMode = mCullMode;
	//}

	//void PartPavement::setMaterial(Echo::Material* material)
	//{
	//	if (!material)
	//		return;

	//	mMaterial->clone(material, true, false);

	//	memcpy(&mMaterial->m_desc.RenderState, &material->m_desc.RenderState, sizeof(material->m_desc.RenderState));

	//	uint32 elementSize = 0;
	//	mMaterial->m_desc.IADesc.ElementArray[0].nStream = 0;
	//	mMaterial->m_desc.IADesc.ElementArray[0].eType = ECHO_FLOAT3;
	//	mMaterial->m_desc.IADesc.ElementArray[0].Semantics = ECHO_POSITION0;
	//	mMaterial->m_desc.IADesc.ElementArray[0].nOffset = elementSize;
	//	elementSize += 3 * sizeof(float);
	//	mMaterial->m_desc.IADesc.ElementArray[1].nStream = 0;
	//	mMaterial->m_desc.IADesc.ElementArray[1].eType = ECHO_FLOAT3;
	//	mMaterial->m_desc.IADesc.ElementArray[1].Semantics = ECHO_NORMAL0;
	//	mMaterial->m_desc.IADesc.ElementArray[1].nOffset = elementSize;
	//	elementSize += 3 * sizeof(float);
	//	mMaterial->m_desc.IADesc.ElementArray[3].nStream = 0;
	//	mMaterial->m_desc.IADesc.ElementArray[3].eType = ECHO_FLOAT4;
	//	mMaterial->m_desc.IADesc.ElementArray[3].Semantics = ECHO_TANGENT0;
	//	mMaterial->m_desc.IADesc.ElementArray[3].nOffset = elementSize;
	//	elementSize += 4 * sizeof(float);
	//	mMaterial->m_desc.IADesc.ElementArray[2].nStream = 0;
	//	mMaterial->m_desc.IADesc.ElementArray[2].eType = ECHO_FLOAT2;
	//	mMaterial->m_desc.IADesc.ElementArray[2].Semantics = ECHO_TEXCOORD0;
	//	mMaterial->m_desc.IADesc.ElementArray[2].nOffset = elementSize;
	//	elementSize += 2 * sizeof(float);

	//	mMaterial->m_desc.IADesc.nElementCount = 4;
	//	mMaterial->m_desc.IADesc.nStreamCount = 1;
	//	mMaterial->m_desc.IADesc.StreamArray[0].nStride = elementSize;//vertexSize

	//	mMaterial->setShaderName(material->mVSName, material->mPSName);
	//	ShaderContentManager::instance()->GetOrLoadShaderContent(Name(material->mVSName), mVSDX);
	//	ShaderContentManager::instance()->GetOrLoadShaderContent(Name(material->mPSName), mPSDX);
	//	mMaterial->setShaderSrcCode(mVSDX, mPSDX);
	//	mMaterial->setMacroShaderSrcCode(mVSDXGenerateMask + material->mMaterialMacro, mPSDXGenerateMask + material->mMaterialMacro);

	//	mMaterial->setMaterialPassMask(material->getMaterialPassMask());
	//	//mMaterial->setMaterialPointShadowPassMask(material->getMaterialPointShadowPassMask());
	//	mMaterial->setMaterialRipplePassMask(material->getMaterialRipplePassMask());
	//	mMaterial->setMaterialSoftPassMask(material->getMaterialSoftPassMask());

	//	// 设置显示LOD颜色宏
	//	mMaterial->mMacroPsSrcCode.append("#define SHOW_LODCOLOR\n");
	//}

	//void PartPavement::touchMaterial()
	//{
	//	updateDescHash(mMaterial->m_desc.IADesc);

	//	mMaterial->createPass(NormalPass);
	//	mMaterial->loadInfo();
	//}

	PavementObject::PavementObject(Pavement* pavement)
		: MovableObject()
	{
		mPavement = pavement;
		if (mPavement->getPavementComponent())
		{
			mName = mPavement->getPavementComponent()->getCombinedName() + "_PavementObject";
		}
		mPavementLodVec.resize(LOD_LEVEL_NUM);

		for (int i = LOD_LEVEL_0; i < LOD_LEVEL_NUM; ++i)
		{
			m_fLodDistance[i] = i * 100.0f;
		}
	}

	PavementObject::~PavementObject()
	{
		//SAFE_DELETE(mBezierData);
		for (WholePavement& _obj : mPavementLodVec)
		{
			for (PartPavement* partRender : _obj)
			{
				SAFE_DELETE(partRender);
			}
		}
		mPavementLodVec.clear();
	}

	void PavementObject::update(const PavementLodRenderData& data)
	{
		mBounds.setNull();

		size_t LodLevelMax = data.size();
		for (size_t lod = LOD_LEVEL_0; lod < LodLevelMax; lod++)
		{
			size_t newsize = data[lod].size();
			size_t oldsize = mPavementLodVec[lod].size();

			//重新组织渲染对象Renderable的数量,暂时先采用每次更新 new/delete 的策略
			while (newsize != oldsize)
			{
				if (newsize > oldsize)
				{
					PartPavement* pavement = new PartPavement(this);
					mPavementLodVec[lod].push_back(pavement);
					++oldsize;
				}
				else
				{
					SAFE_DELETE(mPavementLodVec[lod][oldsize - 1]);
					mPavementLodVec[lod].pop_back();
					--oldsize;
				}
			}
			if (newsize == 0)
				continue;
			//重新渲染数据
			for (size_t index = 0; index < newsize; ++index)
			{
				// 使用第一个顶点位置作为局部原点，使顶点坐标变成小数值
				DBVector3 localOrigin = DBVector3::ZERO;
				if (!data[lod][index].vertex.empty())
				{
					const Vector3& firstPos = data[lod][index].vertex[0].position;
					localOrigin = DBVector3(firstPos.x, firstPos.y, firstPos.z);
				}
				mPavementLodVec[lod][index]->updateRender(data[lod][index], localOrigin);
				for (auto& iter : data[lod][index].vertex)
				{
					mBounds.merge(iter.position);
				}
			}
		}

		if (mParentNode)
		{
			mParentNode->needUpdate();
		}
	}

	void PavementObject::setRenderDistance(float distance)
	{
		mRenderDistance = distance;
	}

	void PavementObject::setLodDistance(EM_LOD_LEVEL lodlevel, float distance)
	{
		m_fLodDistance[lodlevel] = distance;
	}

	MovableObjectType PavementObject::getMovableType() const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
		return MovableObjectType_Custom;
	}

	const AxisAlignedBox& PavementObject::getBoundingBox() const
	{
		// TODO: 在此处插入 return 语句
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
		return mBounds;
	}

	float PavementObject::getBoundingRadius(void) const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
		return 0.0f;
	}

	void PavementObject::_notifyCurrentCamera(Camera* cam)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)

		getWorldBoundingBox(true);

		float distanceSquared = mWorldAABB.SquareDistPointToAABB(cam->getPosition());
		distanceSquared = std::max(0.000001f, distanceSquared);

		float tempRenderDistence = mRenderDistance;

		mCulled = (tempRenderDistence * tempRenderDistence) < distanceSquared;

		if (mCulled)
		{
			return;
		}

		if (cam->getCameraUsage() != SceneCamera)
			return;

		int lodLevel = LOD_LEVEL_NUM - 1;

		float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
		fLodDisScale *= m_fGlobalLodScale;
		for (; lodLevel > LOD_LEVEL_0; --lodLevel)
		{
			float lodDistance = m_fLodDistance[lodLevel] * fLodDisScale;
			if (distanceSquared >= lodDistance * lodDistance)
				break;

			if (lodLevel != mCurrLodLevel)
			{
				if (lodLevel >= LOD_LEVEL_NUM)
				{
					lodLevel = LOD_LEVEL_NUM - 1;
				}
			}
		}

		if (!mPavementLodVec.empty())
		{
			if (lodLevel >= mPavementLodVec.size())
				lodLevel = (int)mPavementLodVec.size() - 1;
			while (lodLevel >= 0 && mPavementLodVec[lodLevel].empty())
			{
				lodLevel = lodLevel - 1;
			}
		}
		else
			lodLevel = 0;

		if (lodLevel != mCurrLodLevel)
		{
			mCurrLodLevel = (EM_LOD_LEVEL)lodLevel;
		}
	}

	void PavementObject::updateRenderQueue(RenderQueue* pRenderQueue)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		if (mPavementLodVec.empty())
			return;
		if (mCurrLodLevel >= mPavementLodVec.size())
			mCurrLodLevel = (EM_LOD_LEVEL)(mPavementLodVec.size() - 1);

		bool forwardSSR = Root::instance()->getActiveSceneManager()->isForwardSSREnable() &&
			Root::instance()->getRenderSystem()->getCurrentRenderStrategyType() == RS_Forward;

		WholePavement& renderlist = mPavementLodVec[mCurrLodLevel];
		for (auto& tos : renderlist)
		{
			// 设置道路显示LOD颜色
			if (Root::instance()->getShowLodColor())
			{
				Vector4 v4Param = Vector4::ZERO;
				v4Param.w = Root::instance()->getShowLodColorW();
				switch (mCurrLodLevel)
				{
				case LOD_LEVEL_0: v4Param.x = 1.f; break;											// 红色
				case LOD_LEVEL_1: v4Param.x = 1.f; v4Param.y = 1.f; break;							// 黄色
				case LOD_LEVEL_2: v4Param.y = 1.f; break;											// 绿色
				default: break;
				}

				tos->setCustomMaterialUniformParams(4, v4Param);
			}
			else
				tos->setCustomMaterialUniformParams(4, Vector4::ZERO);

			RenderOperation* rop = tos->getRenderOperation();
			if (rop->GetMeshData().m_vertexCount == 0 || rop->GetMeshData().m_IndexBufferData.m_indexCount == 0)
				continue;
			{
				RoadManager::instance()->m_pavementDrawCount += 1;
				RoadManager::instance()->m_pavementDrawCountLOD[mCurrLodLevel] += 1;

				RoadManager::instance()->m_pavementTriangleCount += (rop->GetMeshData().m_IndexBufferData.m_indexCount / 3);
				RoadManager::instance()->m_pavementTriangleCountLOD[mCurrLodLevel] += (rop->GetMeshData().m_IndexBufferData.m_indexCount / 3);
			}
			const MaterialWrapper& matWrapper = tos->getMaterialWrapper();

			if (matWrapper.getV1())
			{
				const Material* pMat = matWrapper.getV1();
				forwardSSR = forwardSSR && matWrapper.getV1()->m_matInfo.bForwardSSR;
				if (pMat->m_desc.RenderState.BlendEnable)
					pRenderQueue->getRenderQueueGroup(RenderQueue_Solid_Transparent)->addRenderable(pMat->getPass(NormalPass), tos, 0);
				else if (forwardSSR)
					pRenderQueue->getRenderQueueGroup(RenderQueue_Skies)->addRenderable(pMat->getPass(NormalPass), tos, 0);
				else if (pMat->m_matInfo.alphaTestEnable)
					pRenderQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(pMat->getPass(NormalPass), tos, 0);
				else
					pRenderQueue->getRenderQueueGroup(RenderQueue_Solid)->addRenderable(pMat->getPass(NormalPass), tos, 0);
			}
			else
			{
				MaterialV2* pMat = matWrapper.getV2();
				PassMaskFlags postMaskFlags;
				postMaskFlags |= PassMaskFlag::UseLight;
				postMaskFlags |= PassMaskFlag::UseDpsm;
				postMaskFlags |= PassMaskFlag::ReceiveShadow;
				PostRenderableParams postParams{
					.material = pMat,
					.renderable = tos,
					.renderQueue = pRenderQueue,
					.passMaskFlags = postMaskFlags,
					.isAfterWater = true
				};
				MaterialShaderPassManager::fastPostRenderable(postParams);
			}
		}
	}
#ifndef RequestTaskCheck
#define RequestTaskCheck\
		if(!m_setRenderTickets.empty() || !m_setPhysicsTickets.empty())\
		{\
			LogManager::instance()->logMessage("Pavement in asynchronous computation of data.");\
			return false;\
		}
#endif
	
	Pavement::Pavement(const PavementDesc& desc)
	{
		setPavementDesc(desc);
		update(desc.synchronous, desc.updateType);
	}

	Pavement::~Pavement()
	{
		for (auto& ticket : m_setRenderTickets)
		{
			if (ticket != 0)
			{
				RoadHelper::instance()->abortRequest(ticket);
				if (mParent->getWorldManager())
					mParent->getWorldManager()->removePavementWorkObject(ticket, "Pavement::~Pavement()");
			}
		}
		m_setRenderTickets.clear();
		
		for (auto& ticket : m_setPhysicsTickets)
		{
			if (ticket != 0)
			{
				RoadHelper::instance()->abortRequest(ticket);
				if (mParent->getWorldManager())
					mParent->getWorldManager()->removePavementWorkObject(ticket, "Pavement::~Pavement()");
			}
		}
		m_setPhysicsTickets.clear();

		SAFE_DELETE(mPlanetListener);

		for (auto& pavement : mPavementVec)
		{
			SAFE_DELETE(pavement);
		}
		mPavementVec.clear();

		SAFE_DELETE(mBatchingObject);
	}

	void Pavement::setName(const Name& name)
	{
		if (!name.empty() && mName != name)
			mName = name;
	}

	void Pavement::update(bool sync, PavementComponent::UpdateType updateType/* = PavementComponent::UpdateRender*/)
	{
		if (mBaseMeshName.empty())
		{
			LogManager::instance()->logMessage("Pavement::update() base mesh name is empty!");
			return;
		}
		PavementFitTerrain* terrain = mParent->getPavementFitTerrain();
		//SphericalTerrain* mPlanet = nullptr;
		if (terrain && terrain->mType == PavementFitTerrain::planetTerrain)
		{
			PavementComponent::PavementFitPlanetTerrain* planetTerrain = (PavementComponent::PavementFitPlanetTerrain*)terrain;
			if (mPlanetListener && mPlanetListener->mPavement == this)
			{
				if (!mPlanetListener->mPlanet || !mPlanetListener->mPlanet->isCreateFinish())
				{
					LogManager::instance()->logMessage("Pavement::update() listener's planet is empty or not create finished!");
					return;
				}
				mPlanetListener->mPavement = nullptr;
				mPlanetListener->mPlanet = nullptr;
			}
			else if (mFitGroundModel)
			{
				//assert(planetTerrain->getTerrainPtr() && "Pavement FitGroundModel:: There is no planet.");
				if (!planetTerrain->getTerrainPtr())
				{
					LogManager::instance()->logMessage("Pavement FitGroundModel:: There is no planet.");
					//return;//暂时不返回，以不贴地模式更新
				}
				else if (!planetTerrain->getTerrainPtr()->isCreateFinish())
				{
					LogManager::instance()->logMessage("Pavement::update:: planet is not create finished");

					if (!mPlanetListener)
						mPlanetListener = new PlanetLoadListener(this);
					else
						mPlanetListener->mPavement = this;
					mPlanetListener->mPlanet = planetTerrain->getTerrainPtr();
					mPlanetListener->mUpdateType = updateType;
					mPlanetListener->sync = sync;
					planetTerrain->getTerrainPtr()->addLoadListener(mPlanetListener);
					return;
				}
			}
		}
		if (mFitGroundModel && terrain)
		{
			if (mBezierData.mControllerFitGroundLevel[0] > -1)
			{
				PavementFitTerrain::TerrainData data;
				terrain->getTerrainData(mParent->convertLocalToWorldPosition(mBezierData.mControllerPoint[0]), data);
				mBezierData.mControllerPoint[0] = data.terrainPos + data.terrainNormal * (mBezierData.mControllerFitGroundLevel[0] * RoadManager::instance()->m_pavementFitGroundLevelHight);
				mBezierData.mControllerPoint[0] = mParent->convertWorldToLocalPosition(mBezierData.mControllerPoint[0]);
			}
			if (mBezierData.mControllerFitGroundLevel[1] > -1)
			{
				PavementFitTerrain::TerrainData data;
				terrain->getTerrainData(mParent->convertLocalToWorldPosition(mBezierData.mControllerPoint[3]), data);
				mBezierData.mControllerPoint[3] = data.terrainPos + data.terrainNormal * (mBezierData.mControllerFitGroundLevel[1] * RoadManager::instance()->m_pavementFitGroundLevelHight);
				mBezierData.mControllerPoint[3] = mParent->convertWorldToLocalPosition(mBezierData.mControllerPoint[3]);
			}
			mBezierData.mLength = PiecewiseBezierUtil::LengthFromTime(mBezierData.mControllerPoint[0], mBezierData.mControllerPoint[1], mBezierData.mControllerPoint[3], mBezierData.mControllerPoint[2], 1.0f);
			
		}

		if (sync)
		{
			std::shared_ptr<PavementTool::PrivateMeshData> it_data = RoadManager::instance()->prepareMeshData(mBaseMeshName);
			if (it_data != nullptr)
			{
				if (!mCurveModel && m_spacingStep > 1e-6f)
				{
					if (updateType & PavementComponent::UpdateRender)
						ResetBacthObject(it_data->mesh);
					CalculateObjectTransform();//不随曲线扭曲模型，计算间隔生成在曲线上的位置
				}
				if (updateType & PavementComponent::UpdateRender)
				{
					if (mCurveModel || Math::FloatEqual(m_spacingStep, 0.0f))//随曲线扭曲模型计算vbib
						CalculateRenderVBIB(it_data);
					this->updateObject();
				}
				if (updateType & PavementComponent::UpdatePhysics)
				{
					if (mCurveModel || Math::FloatEqual(m_spacingStep, 0.0f))//随曲线扭曲模型计算物理vbib
						CalculatePhyVBIB(it_data);
					mParent->updatePavementPhyObj(this);
				}
			}
		}
		else
		{
			//异步更新暂时只保留最后一个请求进行计算
			//这样会导致数帧多次进行异步更新请求的时候不会有中间变化的过程，会直接变化到最后一次更新请求的结果
			//目前不考虑上述情况（运行时动态编辑异步更新）,但是暂时保留set的数据结构，后续如需修改把下面特殊处理的内容注释了就行
			{
				if ((updateType & PavementComponent::UpdateRender) && !m_setRenderTickets.empty())
				{
					for (auto& id : m_setRenderTickets)
					{
						RoadHelper::instance()->abortRequest(id);
						if (mParent->getWorldManager())
							mParent->getWorldManager()->removePavementWorkObject(id, "in update Remove intermediate requests");
					}
					m_setRenderTickets.clear();
				}
				if ((updateType & PavementComponent::UpdatePhysics) && !m_setPhysicsTickets.empty())
				{
					for (auto& id : m_setPhysicsTickets)
					{
						RoadHelper::instance()->abortRequest(id);
						if (mParent->getWorldManager())
							mParent->getWorldManager()->removePavementWorkObject(id, "in update Remove intermediate requests");
					}
					m_setPhysicsTickets.clear();
				}
			}

			PavementRequestID ticket = RoadHelper::instance()->UpdatePavement(this, (RoadHelper::ReqType)(updateType << 6));

			if (mParent->getWorldManager())
				mParent->getWorldManager()->addPavementWorkObject(ticket);
			if (updateType & PavementComponent::UpdateRender)
				m_setRenderTickets.insert(ticket);
			if (updateType & PavementComponent::UpdatePhysics)
				m_setPhysicsTickets.insert(ticket);
		}
	}
	bool Pavement::setPavementDesc(const PavementDesc& desc)
	{
		RequestTaskCheck

		mName = desc.pavementName;

		if (mName.empty())
		{
			LogManager::instance()->logMessage("Pavement::setPavementDesc() error pavement Name. can not empty.");
			return false;
		}

		mParent = desc.parent;

		mBezierData = desc.bezierdata;

		mBaseMeshName = desc.meshName;

		mBaseMeshTransform.position = desc.position;
		mBaseMeshTransform.rotation = desc.rotation;
		mBaseMeshTransform.scale = desc.scale;

		mRenderDistance = desc.renderDistance;
		m_fLodDistance[LOD_LEVEL_1] = desc.lod1_distance;
		m_fLodDistance[LOD_LEVEL_2] = desc.lod2_distance;

		m_partitionDistance = desc.segmentLength;
		m_spacingStep = desc.spacingStep;

		mCurveModel = desc.m_IsCurveModel;
		mPierModel = desc.m_IsPierModel;

		mFitGroundModel = desc.isFitGround;

		mPhySurfaceType = desc.m_surfaceType;

		return true;
	}

	bool Pavement::setBaseMesh(const Name& meshName)
	{
		if (mBaseMeshName == meshName)
			return false;
		RequestTaskCheck
		mBaseMeshName = meshName;
		return true;
	}

	bool Pavement::setMeshOffset(const Vector3& pos)
	{
		RequestTaskCheck
		mBaseMeshTransform.position = pos;
		return true;
	}

	bool Pavement::setMeshQuat(const Quaternion& quat)
	{
		RequestTaskCheck
		mBaseMeshTransform.rotation = quat;
		return true;
	}

	bool Pavement::setMeshScale(const Vector3& scale)
	{
		RequestTaskCheck
		mBaseMeshTransform.scale = scale;
		return true;
	}

	bool Pavement::setBaseMeshTransform(const Pavement::Transform& transform)
	{
		RequestTaskCheck
		mBaseMeshTransform = transform;
		return true;
	}

	bool Pavement::setBezierData(const BezierDataCache& data)
	{
		RequestTaskCheck
		mBezierData = data;
		return true;
	}

	bool Pavement::setBezierUpdateType(int updateType)
	{
		RequestTaskCheck
		if (updateType < 0 || updateType >2)
		{
			LogManager::instance()->logMessage("Pavement::setBezierUpdateType() error bezier update type!");
			return false;
		}
		mBezierData.mType = updateType;
		return true;
	}

	bool Pavement::setPartitionDistance(float distance)
	{
		if (m_partitionDistance == distance)
			return false;
		RequestTaskCheck
		m_partitionDistance = distance;
		return true;
	}

	bool Pavement::setStepLength(float distance)
	{
		if (m_spacingStep == distance)
			return false;
		RequestTaskCheck
		m_spacingStep = distance;
		if (Math::FloatEqual(distance, 0.0f))
		{
			mCurveModel = true;
			mPierModel = false;
		}
		return true;
	}

	bool Pavement::setCurveModel(bool isCurveModel)
	{
		if (m_spacingStep < 1e-6f || mCurveModel == isCurveModel)
			return false;
		RequestTaskCheck
		mCurveModel = isCurveModel;
		return true;
	}

	bool Pavement::setPierModel(bool isPierModel)
	{
		if (m_spacingStep < 1e-6f || mPierModel == isPierModel || mCurveModel)
			return false;
		RequestTaskCheck
		mPierModel = isPierModel;
		return true;
	}

	bool Pavement::setFitGroundModel(bool isFitGroundModel)
	{
		if (mFitGroundModel == isFitGroundModel)
			return false;
		RequestTaskCheck
		mFitGroundModel = isFitGroundModel;
		return true;
	}

	bool Pavement::setPartLength(float distance)
	{
		RequestTaskCheck
		m_partitionDistance = distance;
		return true;
	}

	void Pavement::setRenderDistance(float distance)
	{
		mRenderDistance = distance;
		for (auto& pavement : mPavementVec)
		{
			pavement->setRenderDistance(mRenderDistance);
		}
	}

	void Pavement::setLodDistance(EM_LOD_LEVEL lodlevel, float distance)
	{
		m_fLodDistance[lodlevel] = distance;
		for (auto& pavement : mPavementVec)
		{
			pavement->setLodDistance(lodlevel, distance);
		}
	}

	void Pavement::addPavementListener(PavementListener* listener)
	{
		if (listener != nullptr)
		{
			if (listener->mPavement == nullptr)
			{
				listener->mPavement = this;
				mListeners.insert(listener);
			}
			else
				LogManager::instance()->logMessage("Pavement::addPavementListener error : Pavement listener is registered");
		}
	}

	void Pavement::removePavementListener(PavementListener* listener)
	{
		if (listener != nullptr)
		{
			listener->mPavement = nullptr;
			mListeners.erase(listener);
		}
	}

	void Pavement::CalculateRenderVBIB(const std::shared_ptr<PavementTool::PrivateMeshData>& data)
	{
		if (data == nullptr || data->mesh.isNull() || data->SubData.empty())
			return;
		BezierDataCache TempData = mBezierData;

		MeshPtr mesh = data->mesh;
		const PavementTool::LodMeshDataCache& dataVec = data->SubData;

		float partLength = m_partitionDistance;
		int partNum = (int)(TempData.mLength / partLength) +1;//按固定距离所分段数，取整，多余部分单独算作一段

		//单个moveable先改为不再进行按固定距离分段，一个movable就是一整段，如果需要固定距离分段再把下面判断内容注的true释掉
		if(TempData.mLength <= partLength/* || true*/)
		{
			partNum = 1;
			partLength = TempData.mLength;
		}


		AxisAlignedBox box = mesh->getAABB();
		Matrix4 meshtransform;
		//默认以X轴正方向进行变换平铺，所以可能有部分模型需要调整自身的旋转
		meshtransform.makeTransform(mBaseMeshTransform.position, mBaseMeshTransform.scale, mBaseMeshTransform.rotation);
		box.transformAffine(meshtransform);

		float meshOffset_X = box.vMin.x;//基础模型在自身局部空间下X轴的偏移量，用于偏移超过贝塞尔曲线起始位置的部分

		float meshLength = box.vMax.x - box.vMin.x + m_spacingStep;//基础模型的长度（局部空间X轴方向下）
		
		int meshNum = (int)(partLength / meshLength) + 1;//单位分段上平铺的mesh的数量,向上取整，非整部分由整段所有mesh整体缩放对齐

		float meshrealLength = partLength / meshNum;//单位分段上实际每个mesh的长度

		float meshscale = meshrealLength / meshLength;//单位分段上所有mesh的整体缩放系数

		int renderNumMax = 65535 / (int)data->vertexNumMax;//单个Renderable最大可以平铺的mesh中顶点数最多的submesh数量

		int meshRenderNum = meshNum / renderNumMax + 1;//单位分段需要创建的mesh数量

		mRenderData.clear();

		std::vector<MeshPtr> LodMesh = mesh->m_vtLodMesh;
		LodMesh.insert(LodMesh.begin(), mesh);
		size_t lodlevel = LodMesh.size();

		std::unordered_map<float, float> tempdata;//用与贝塞尔曲线上长度对应t值的数据缓存

		//按分段顺序组织数据
		for (int index = 0; index < partNum; ++index)
		{
			PavementLodRenderData pavementVec;

			float beginLenght = index * partLength;//每段的起始长度

			if (index >= partNum - 2 && partNum != 1)//最后两段单独处理，判断是否算作一段（计算完全一样，只是分段长度可能不同）
			{
				float length = TempData.mLength - (index * partLength);
				if (index == partNum - 1 && length < 0.01f)//如果最后一段距离剩余的太短就不铺了
					continue;
				else if (index == partNum - 2 && (int)(length / meshLength) + 1 <= renderNumMax && length > meshLength * 0.5f)
					--partNum;

				//重新计算最后一段的数据
				if((int)(length / meshLength) + 1 <= renderNumMax)
				{
					meshNum = (int)(length / meshLength) + 1;
					meshrealLength = length / meshNum;
					meshscale = meshrealLength / meshLength;
					meshRenderNum = meshNum / renderNumMax + 1;
				}
			}
			
			//每段按lod模型级别组织数据
			for (size_t lod = 0; lod < lodlevel; ++lod)
			{
				WholePavementRenderData partPavementVec;

				uint32 subNum = LodMesh[lod]->getNumSubMesh();
				for (uint32 subindex = 0; subindex < subNum; ++subindex)
				{
					//std::vector<Vector3> roadFitGorundCache;
					for (int rendernum = 0; rendernum < meshRenderNum; ++rendernum)
					{
						//subdata.material = LodMesh[lod]->getSubMesh(subindex)->getMaterial();
						if (dataVec[lod][subindex].road_material.isValid())
						{
							PartPavementRenderData subdata;
							subdata.material = dataVec[lod][subindex].road_material;

							for (int num = 0; num < meshNum; ++num)
							{
								//Vertex
								size_t vertexSize = dataVec[lod][subindex].road_vertexes.size();
								for (size_t i = 0; i < vertexSize; ++i)
								{
									PartPavementRenderData::Vertex vertexTemp;
									Vector3 position = dataVec[lod][subindex].road_vertexes[i].position;
									Vector3 normal = dataVec[lod][subindex].road_vertexes[i].normal;
									Vector4 tangent = dataVec[lod][subindex].road_tangent[i];
									Vector2 uv = dataVec[lod][subindex].road_uv[i];


									//Vertex适配mesh的局部变换
									{
										position = mBaseMeshTransform.rotation * (mBaseMeshTransform.scale * position);
										position += mBaseMeshTransform.position;
										position.x -= meshOffset_X;
										//if (lod == 0 && mFitGroundModel && mPlanet && TempData.mControllerFitGroundLevel[0] == 0 && TempData.mControllerFitGroundLevel[1] == 0)
										//	roadFitGorundCache.push_back(mPlanet->ToLocalSpace(mParent->convertLocalToWorldPosition(point.first)));
									}


									float length = beginLenght + num * meshrealLength + position.x * meshscale;
									if (Math::FloatEqual(length, 0.0f, 1.0e-5f))
										length = 0.0f;
									{
										Echo::Transform transform;
										transform.translation = position;
										transform.translation.x = 0.0f;
										std::pair<Vector3, Matrix3> point = PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(TempData.mControllerPoint[0], TempData.mControllerPoint[1], TempData.mControllerPoint[2], TempData.mControllerPoint[3],
											length, TempData.mControllerNormal[0], TempData.mControllerNormal[1], transform, TempData.mType,
											mFitGroundModel ? mParent->weak_from_this() : nullptr, TempData.mControllerFitGroundLevel[0], TempData.mControllerFitGroundLevel[1], mParent->getPavementFitTerrain());

										// position
										{
											vertexTemp.position = point.first;
											//if (lod == 0 && mFitGroundModel && mPlanet && TempData.mControllerFitGroundLevel[0] == 0 && TempData.mControllerFitGroundLevel[1] == 0)
											//	roadFitGorundCache.push_back(mPlanet->ToLocalSpace(mParent->convertLocalToWorldPosition(point.first)));
										}

										// normal and tangent
										{
											vertexTemp.normal = point.second * normal;
											vertexTemp.normal = mBaseMeshTransform.rotation * vertexTemp.normal;

											Vector3 temptangent = Vector3(tangent.x, tangent.y, tangent.z);
											temptangent = point.second * temptangent;
											temptangent = mBaseMeshTransform.rotation * temptangent;
											vertexTemp.tangent = Vector4(temptangent.x, temptangent.y, temptangent.z, tangent.w);
										}

										// uv
										{
											vertexTemp.uv = uv;
										}


										subdata.vertex.push_back(vertexTemp);
										continue;
									}
								}

								//Index
								for (auto& _index : dataVec[lod][subindex].road_indices)
								{
									subdata.index.push_back(_index + (uint16)dataVec[lod][subindex].road_vertexes.size() * num);
								}
							}
							//if (lod == 0 && mFitGroundModel && mPlanet && TempData.mControllerFitGroundLevel[0] == 0 && TempData.mControllerFitGroundLevel[1] == 0)
							//{
							//	RoadManager::instance()->UpdatePlanetGitGroundRoadMaskMap(mPlanet, roadFitGorundCache, subdata.index);
							//}
							partPavementVec.push_back(subdata);
						}
					}
				}
				pavementVec.push_back(partPavementVec);
			}
			mRenderData.push_back(pavementVec);
		}

	}

	void Pavement::CalculatePhyVBIB(const std::shared_ptr<PavementTool::PrivateMeshData>& data)
	{
		//以下计算物理模型的算法与渲染模型的算法完全相同
		if (data == nullptr || data->mesh.isNull() || data->PhyData.vertexVec.empty() || data->PhyData.indexVec.empty())
			return;
		BezierDataCache TempData = mBezierData;
		
		//为了保证物理模型和渲染模型在平铺到一定数量时，因为分段距离不同导致的单位模型的缩放系数不同，导致物理与渲染无法完全对应的问题
		//这里物理顶点计算时取与渲染分段完全相同的分段距离
		//计算时进行数据二次处理，以减少物理创建数量
		// -------------------------------------------------------------------------------------------------------------------
		float partPhyLength = m_partitionDistance;
		int partNum = (int)(TempData.mLength / partPhyLength) + 1;

		if(TempData.mLength < partPhyLength)
		{
			partNum = 1;
			partPhyLength = TempData.mLength;
		}

		//物理模型在局部空间下的包围盒与偏移量，这里取原模型的数据代替
		MeshPtr mesh = data->mesh;
		AxisAlignedBox box = mesh->getAABB();
		Matrix4 meshtransform;
		//默认以X轴正方向进行变换平铺，所以可能有部分模型需要调整自身的旋转
		meshtransform.makeTransform(mBaseMeshTransform.position, mBaseMeshTransform.scale, mBaseMeshTransform.rotation);
		box.transformAffine(meshtransform);

		float phymeshOffset_X = box.vMin.x;//基础模型在自身局部空间下X轴的偏移量，用于偏移超过贝塞尔曲线起始位置的部分

		float phymeshLength = box.vMax.x - box.vMin.x + m_spacingStep;//基础模型的长度（局部空间X轴方向下）

		int phymeshNum = (int)(partPhyLength / phymeshLength) + 1;//单位分段上平铺的mesh的数量,向上取整，非整部分由整段所有mesh整体缩放对齐

		float phyrealLength = partPhyLength / phymeshNum;//单位分段上实际每个mesh的长度

		float phymeshscale = phyrealLength / phymeshLength;//单位分段上所有mesh的整体缩放系数

		int phyNumMax = 65535 / (int)data->PhyData.vertexVec.size();//单个Renderable最大可以平铺的mesh中顶点数最多的submesh数量

		int PhyNum = phymeshNum / phyNumMax + 1;//单位分段需要创建的mesh数量

		mPhysicsData.clear();

		std::unordered_map<float, float> tempdata;//用与贝塞尔曲线上长度对应t值的数据缓存

		PavementTool::PavementPhyMeshData phyTempData;;//用来二次规划物理数据，防止单个物理过大或者物理数量过多

		//按分段顺序组织数据,算法与渲染数据的算法完全相同
		for (int index = 0; index < partNum; ++index)
		{
			float beginLenght = index * partPhyLength;//每段的起始长度

			if (index >= partNum - 2 && partNum != 1)//最后两段单独处理，判断是否算作一段（计算完全一样，只是分段长度可能不同）
			{
				float length = TempData.mLength - (index * partPhyLength);
				if (index == partNum - 1 && length < 0.01f)//如果最后一段距离剩余的太短就不铺了
					continue;
				else if (index == partNum - 2 && (int)(length / phymeshLength) + 1 <= phyNumMax && length > phymeshLength * 0.5f)
					--partNum;

				//重新计算最后一段的数据
				if ((int)(length / phymeshLength) + 1 <= phyNumMax)
				{
					phymeshNum = (int)(length / phymeshLength) + 1;
					phyrealLength = length / phymeshNum;
					phymeshscale = phyrealLength / phymeshLength;
					PhyNum = phymeshNum / phyNumMax + 1;
				}
			}
			
			for (int phynum = 0; phynum < PhyNum; ++phynum)
			{
				for (int num = 0; num < phymeshNum; ++num)
				{
					//Index
					for (auto& _index : data->PhyData.indexVec)
					{
						//phyTempData.indexVec.push_back(_index + (uint16)vertexSize * num);
						phyTempData.indexVec.push_back(_index + (uint16)phyTempData.vertexVec.size());
					}

					//Vertex
					size_t vertexSize = data->PhyData.vertexVec.size();
					for (size_t i = 0; i < vertexSize; ++i)
					{
						Vector3 phyposition = data->PhyData.vertexVec[i];

						//Vertex适配mesh的局部变换
						{
							phyposition = mBaseMeshTransform.rotation * (mBaseMeshTransform.scale * phyposition);
							phyposition += mBaseMeshTransform.position;
							phyposition.x -= phymeshOffset_X;
						}


						float length = beginLenght + num * phyrealLength + phyposition.x * phymeshscale;
						{
							Echo::Transform transform;
							transform.translation = phyposition;
							transform.translation.x = 0.0f;
							std::pair<Vector3, Matrix3> point = PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(TempData.mControllerPoint[0], TempData.mControllerPoint[1], TempData.mControllerPoint[2], TempData.mControllerPoint[3],
								length, TempData.mControllerNormal[0], TempData.mControllerNormal[1], transform, TempData.mType,
								mFitGroundModel ? mParent->weak_from_this() : nullptr, TempData.mControllerFitGroundLevel[0], TempData.mControllerFitGroundLevel[1], mParent->getPavementFitTerrain());

							phyTempData.vertexVec.push_back(point.first);

							phyTempData.mBox.merge(point.first);
						}
					}

					//为了减少物理创建的数量，这里进行数据的二次处理
					if (phyTempData.mBox.getSize().length() > 240)
					{
						mPhysicsData.push_back(phyTempData);
						
						phyTempData.mBox.setNull();
						phyTempData.vertexVec.clear();
						phyTempData.indexVec.clear();
					}
				}
			}
			
		}
		if (!phyTempData.mBox.isNull() && !phyTempData.vertexVec.empty() && !phyTempData.indexVec.empty())
			mPhysicsData.push_back(phyTempData);
	}

	void Pavement::ResetBacthObject(const MeshPtr& mesh)
	{
		if (mCurveModel || Math::FloatEqual(m_spacingStep, 0.0f))
			return;
		SAFE_DELETE(mBatchingObject);
		mBatchingObject = new TrafficBatchObject(mParent->getSceneManager());
		mBatchingObject->init(mesh, mBaseMeshTransform.position, mBaseMeshTransform.scale, mBaseMeshTransform.rotation);
		mBatchingObject->mRenderDistance = mRenderDistance;
		mBatchingObject->m_fLodDistance[LOD_LEVEL_1] = m_fLodDistance[LOD_LEVEL_1];
		mBatchingObject->m_fLodDistance[LOD_LEVEL_2] = m_fLodDistance[LOD_LEVEL_2];

	}

	void Pavement::CalculateObjectTransform()
	{
		if (Math::FloatEqual(m_spacingStep, 0.0f) || m_spacingStep < 0.0f)
			return;
		BezierDataCache TempData = mBezierData;

		int objectNum = (int)(TempData.mLength / m_spacingStep) + 1;
		mTransformData.clear();
		mTransformData.resize(objectNum);

		std::unordered_map<float, float> tempdata;//用与贝塞尔曲线上长度对应t值的数据缓存

		for (int index = 0; index < objectNum; ++index)
		{
			float length = index * m_spacingStep;
			{
				Echo::Transform transform;
				transform.translation = mBaseMeshTransform.position;
				transform.translation.x = 0.0f;
				std::pair<Vector3, Matrix3> point = PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(TempData.mControllerPoint[0], TempData.mControllerPoint[1], TempData.mControllerPoint[2], TempData.mControllerPoint[3],
					length, TempData.mControllerNormal[0], TempData.mControllerNormal[1], transform, TempData.mType,
					mFitGroundModel ? mParent->weak_from_this() : nullptr, TempData.mControllerFitGroundLevel[0], TempData.mControllerFitGroundLevel[1], mParent->getPavementFitTerrain());

				mTransformData[index].position = point.first;

				Matrix3 quatMatrix;
				mBaseMeshTransform.rotation.ToRotationMatrix(quatMatrix);
				mTransformData[index].rotation.FromRotationMatrix(point.second * quatMatrix);

				mTransformData[index].scale = mBaseMeshTransform.scale;
			}
		}
	}

	void Pavement::updateObject()
	{
		if (mCurveModel || Math::FloatEqual(m_spacingStep, 0.0f))
		{
			if (mRenderData.empty())
				return;

			if (mBatchingObject)
			{
				SAFE_DELETE(mBatchingObject);
				mBatchingObject = nullptr;
			}

			size_t newsize = mRenderData.size();
			size_t oldsize = mPavementVec.size();

			//重新组织实例对象PavementObject的数量,暂时先采用每次更新 new/delete 的策略
			while (newsize != oldsize && oldsize >= 0)
			{
				if (newsize > oldsize)
				{
					PavementObject* pavement = new PavementObject(this);
					pavement->setRenderDistance(mRenderDistance);
					pavement->setLodDistance(LOD_LEVEL_1, m_fLodDistance[LOD_LEVEL_1]);
					pavement->setLodDistance(LOD_LEVEL_2, m_fLodDistance[LOD_LEVEL_2]);

					mParent->attachToNode(pavement);
					mPavementVec.push_back(pavement);
					++oldsize;
				}
				else
				{
					mParent->detachFromNode(mPavementVec[oldsize - 1]);
					SAFE_DELETE(mPavementVec[oldsize - 1]);
					mPavementVec.pop_back();
					--oldsize;
				}
			}
			//mParent->getSceneNode()->_update(true, false);

			if (newsize == 0)
				return;
			//更新实例数据
			for (size_t index = 0; index < newsize; ++index)
			{
				mPavementVec[index]->update(mRenderData[index]);
			}
			//临时数据，用完即清
			mRenderData.clear();
		}
		else
		{
			if (!mPavementVec.empty())
			{
				for (auto& pavement : mPavementVec)
				{
					SAFE_DELETE(pavement);
				}
				mPavementVec.clear();
			}

			if (!mPierModel)
			{
				ObjectInstanceDataVec BatchDatas;
				for (auto& data : mTransformData)
				{
					BatchDatas.push_back(ObjectInstanceData{ data.position, data.scale, data.rotation });
				}
				mParent->attachToNode(mBatchingObject);
				mBatchingObject->addInstanceData(BatchDatas);
				mBatchingObject->mMesh->load();
				mBatchingObject->asyncLastInit();
			}
			else
			{

			}
		}
		mParent->getSceneNode()->_update(true, false);

		//if (RoadManager::instance()->usePlanetRoadMask && (mFitGroundModel || mParent->getVegetationCollection()) &&
		//	(mBezierData.mControllerFitGroundLevel[0] == 0 || mBezierData.mControllerFitGroundLevel[1] == 0) &&
		//	mParent && mParent->getPavementFitTerrain() && mParent->getPavementFitTerrain()->mType == PavementFitTerrain::planetTerrain)
		//{
		//	SphericalTerrain* mPlanet = ((PavementComponent::PavementFitPlanetTerrain*)mParent->getPavementFitTerrain())->getTerrainPtr();
		//	/*for (auto& planet : mParent->getSceneManager()->getPlanetManager()->m_SphericalTerrainSet)
		//	{
		//		if (!mPlanet)
		//		{
		//			mPlanet = planet;
		//		}
		//		else
		//		{
		//			DVector3 pos = mParent->getWorldPosition();
		//			if (pos.squaredDistance(mPlanet->m_pos) > pos.squaredDistance(planet->m_pos))
		//				mPlanet = planet;
		//		}
		//	}*/
		//	if (!mPlanet)
		//		return;
		//	RoadManager::instance()->addPlanetFitGroundPavementAreaCache(mParent->weak_from_this(), mPlanet, mParent->getWorldAABB());
		//}
	}

	void Pavement::CalculateOperationCompleted(PavementRequestID tickID, uint16 updateType)
	{
		m_setRenderTickets.erase(tickID);
		m_setPhysicsTickets.erase(tickID);
		if (mParent->getWorldManager())
			mParent->getWorldManager()->removePavementWorkObject(tickID, mBaseMeshName.toString() + " in Pavement CalculateOperationCompleted");

		//等待最后一个异步更新请求的回调再更新
		if (!m_setRenderTickets.empty() || !m_setPhysicsTickets.empty())
		{
			LogManager::instance()->logMessage("Pavement::CalculateOperationCompleted : Waiting for the final result");
			return;
		}

		if (updateType & RoadHelper::ReqType_UpdatePavementRender)
			updateObject();
		if (updateType & RoadHelper::ReqType_UpdatePavementPhy)
		{
			if (mParent)
				mParent->updatePavementPhyObj(this);
			mPhysicsData.clear();
		}

		for (auto& listener : mListeners)
		{
			listener->OnPavementCreateFinish();
		}
	}

	//void Pavement::CalculateFitGroundVBIB(SphericalTerrain* mPlanet, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	//{
	//	if (!mPlanet || !mFitGroundModel || mBezierData.mControllerFitGroundLevel[0] != 0 || mBezierData.mControllerFitGroundLevel[1] != 0)
	//		return;
	//	else
	//	{
	//		const std::set<SphericalTerrain*>& planetSet = mParent->getSceneManager()->getPlanetManager()->m_SphericalTerrainSet;
	//		if (planetSet.find(mPlanet) == planetSet.end())
	//			return;
	//	}
	//	if (!mParent->getPavementFitTerrain() || mParent->getPavementFitTerrain()->mType != PavementFitTerrain::planetTerrain)
	//		return;
	//	//这里直接用物理顶点
	//	std::shared_ptr<PavementTool::PrivateMeshData> it_data = RoadManager::instance()->prepareMeshData(mBaseMeshName);
	//	CalculatePhyVBIB(it_data);
	//	//update(true, PavementComponent::UpdatePhysics);
	//	for (auto& data : mPhysicsData)
	//	{
	//		for (auto& i : data.indexVec)
	//		{
	//			ibVec.push_back(i + (uint16)vbVec.size());
	//		}
	//		for (auto& v : data.vertexVec)
	//		{
	//			//vbVec.push_back(mPlanet->ToLocalSpace(mParent->convertLocalToWorldPosition(v)));
	//			vbVec.push_back(mParent->convertLocalToWorldPosition(v));
	//		}
	//	}
	//	mPhysicsData.clear();
	//}

	PavementListener::~PavementListener()
	{
		if (mPavement != nullptr)
		{
			mPavement->removePavementListener(this);
		}
	}

	PlanetLoadListener::PlanetLoadListener(Pavement* pavement)
	{
		assert(pavement && "PlanetLoadListener:: Pavement is NULL.");
		mPavement = pavement;
	}

	PlanetLoadListener::~PlanetLoadListener()
	{
		if (mPlanet)
			mPlanet->removeLoadListener(this);
	}

	void PlanetLoadListener::OnCreateFinish()
	{
		if (mPlanet && mPavement)
		{
			mPlanet->removeLoadListener(this);
			mPavement->update(sync, mUpdateType);
			mPlanet = nullptr;
		}
	}

	void PlanetLoadListener::OnDestroy()
	{
		mPlanet = nullptr;
		mPavement = nullptr;
	}
}
