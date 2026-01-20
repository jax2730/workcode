///////////////////////////////////////////////////////////////////////////////
//
// @file EchoKarstRenderNode.cpp
// @author liyaming
// @date 2024/07/26
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoStableHeaders.h"
#include "EchoKarstRenderNode.h"
#include "EchoKarstPage.h"
#include "EchoKarstGroup.h"
#include "EchoSceneManager.h"
#include "EchoUberNoiseFBM2D.h"
#include "EchoShaderContentManager.h"

namespace Echo
{
	const static ECHO_SAMPLER_STATE clampLinearSamplerState(
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_FILTER_TRILINEAR,
		ECHO_TEX_CMP_NONE,
		ECHO_CMP_GREATER
	);

	const static ECHO_SAMPLER_STATE clampPointSamplerState(
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_ADDRESS_CLAMP,
		ECHO_TEX_FILTER_POINT,
		ECHO_TEX_CMP_NONE,
		ECHO_CMP_GREATER
	);

	KarstBatchRender::KarstBatchRender(uint32 maxInstanceNum, const MaterialWrapper& material, uint32 meshType, KarstMesh* karstMesh, const KarstGroup* karstGroup)
		: mInstanceNum(0)
		, mMaxInstanceNum(maxInstanceNum)
		, mMaterial(material)
		, mKarstMesh(karstMesh)
		, mMeshType(meshType)
		, mKarstGroup(karstGroup)
	{
		initMesh();
		reserveInfoBuffer();
		enableCustomParameters(true);
		enableInstanceRender(true);
	};

	KarstBatchRender::~KarstBatchRender()
	{
		auto& meshData = mRenderOp.GetMeshData();
		meshData.m_vertexBuffer[0] = nullptr;
	}

	void KarstBatchRender::initMesh()
	{
		mRenderOp.GetMeshData().m_vertexBuffer.resize(2); 

		auto& meshData = mRenderOp.GetMeshData();

		meshData.m_vertexCount = mKarstMesh->getVertexCount();
		meshData.m_vertexBuffer[0] = mKarstMesh->getVertexBuffer();
		meshData.m_IndexBufferData.m_indexBuffer = mKarstMesh->getIndexBuffer(mMeshType);
		meshData.m_IndexBufferData.m_indexCount = mKarstMesh->getIndexCount(mMeshType);

		mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
	}

	void KarstBatchRender::reserveInfoBuffer()
	{
		auto& meshData = mRenderOp.GetMeshData();
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		mRenderBatchInfo.resize(mMaxInstanceNum, {});
		meshData.m_vertexBuffer[1] = pRenderSystem->createDynamicVB(mRenderBatchInfo.data(), mMaxInstanceNum * (uint32)sizeof(KarstElementInfo));
	}

	void KarstBatchRender::addRenderNode(KarstRenderable* pNode)
	{
		auto options = mKarstGroup->getOptions();

		KarstElementInfo info;
		
		info.nodeUV = Vector2((float)pNode->mQuadtreeNode->mXOffset, (float)pNode->mQuadtreeNode->mYOffset) / (float)options->mGridNum;
		info.karstSize = options->mKarstSize;
		info.nodeScale = (float)pow(2, pNode->mQuadtreeNode->mDepth);
		info.basePostion = pNode->mKarstPage->getBasePosition() - mKarstGroup->getSceneManager()->getActiveCamera()->getPosition();
		info.levelInfo.displayArea = pNode->mKarstPage->getKarstGroup()->getDisplayArea() ? 1 : 0;
		info.levelInfo.shadingMode = pNode->mKarstPage->getKarstGroup()->getShadingMode();

		KarstVTCoord vtCoord = {};
		pNode->mKarstPage->getNodeTextureCoord(vtCoord, pNode, KarstResourceDataType::HeightNormalMap, true);
		auto vtOptions = pNode->mKarstPage->getKarstGroup()->getDataVTOptions();
		info.levelInfo.level = vtCoord.level;
		info.tileCoord = vtCoord.coord + vtOptions->mPixelOffset;
		info.tileScale = vtCoord.scale * vtOptions->mVTCacheScale;

		vtCoord = {};
		pNode->mKarstPage->getNodeTextureCoord(vtCoord, pNode, KarstResourceDataType::TextureMap, true);
		vtOptions = pNode->mKarstPage->getKarstGroup()->getTexVTOptions();
		info.levelInfo.texLevel = vtCoord.level;
		info.texTileCoord = vtCoord.coord + vtOptions->mPixelOffset;
		info.texTileScale = vtCoord.scale * vtOptions->mVTCacheScale;

		mRenderBatchInfo.push_back(info);
	}

	bool KarstBatchRender::updateRenderData(SceneManager* sm)
	{
		mInstanceNum = (uint32)mRenderBatchInfo.size();
		if (mInstanceNum) {
			auto& meshData = mRenderOp.GetMeshData();
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			pRenderSystem->updateBuffer(meshData.m_vertexBuffer[1], mRenderBatchInfo.data(), mInstanceNum * sizeof(KarstElementInfo), false);
			return true;
		}
		else {
			return false;
		}
	}

	void KarstBatchRender::setCustomParameters(const Pass* pPass)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt0, mKarstGroup->getTileHeightTexture(), clampPointSamplerState);
		//pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt1, mKarstGroup->getTileNormalTexture(), clampLinearSamplerState);
		pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt2, mKarstGroup->getTileTextureTexture(), clampLinearSamplerState);
		pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt3, mKarstGroup->getAreaTexture(), clampPointSamplerState);
		//pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt7, mKarstGroup->getRoadMaskTexture(), samplerState);
		auto dataVTOptions = mKarstGroup->getDataVTOptions();
		auto texVTOptions = mKarstGroup->getTexVTOptions();
		KarstVTSizeInfo sizeInfo{
			dataVTOptions->mVTCacheWidth,
			dataVTOptions->mVTCacheHeight,
			texVTOptions->mVTCacheWidth,
			texVTOptions->mVTCacheHeight };
		pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, &sizeInfo, sizeof(sizeInfo));

		ECHO_SAMPLER_STATE pointSampler;
		pRenderSystem->setTextureSampleValue(this, pPass, S_Custom5, mKarstGroup->getTileMaterialTexture(), pointSampler);

		const auto& PSCustomParams = mKarstGroup->getPSCustomParams();

		pRenderSystem->setUniformValue(this, pPass, U_PSCustom1, &PSCustomParams.PSCustom1, sizeof(PSCustomParams.PSCustom1));
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom2, &PSCustomParams.PSCustom2, sizeof(PSCustomParams.PSCustom2));
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom3, &PSCustomParams.PSCustom3, sizeof(PSCustomParams.PSCustom3));
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom5, &PSCustomParams.PSCustom5, sizeof(PSCustomParams.PSCustom5));
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom6, &PSCustomParams.PSCustom6, sizeof(PSCustomParams.PSCustom6));
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom10, &PSCustomParams.PSCustom10, sizeof(PSCustomParams.PSCustom10));
		uint32 displayAreaBorder = 0;
		if (mKarstGroup->getAreaIDVersion() > 0) {
			displayAreaBorder = mKarstGroup->getDisplayAreaBorder();
		}
		pRenderSystem->setUniformValue(this, pPass, U_PSCustom7, &displayAreaBorder, sizeof(uint32));
		pRenderSystem->setBufferValue(this, pPass, B_CSCustom0, mKarstGroup->getAreaFogBuffer());
	}

	KarstVTViewer::KarstVTViewer(RcTexture* tileTexture, float widthProp, float heightProp)
		:mTileTexture(tileTexture)
	{
		enableCustomParameters(true);

		uint32 width, height;
		Root::instance()->getRenderSystem()->getRenderWindow()->getWidthAndHeight(width, height);
		mAspectRatio = float(width) / height;
		mWidthProp = widthProp;
		mHeightProp = heightProp == 0.0f ? widthProp * mAspectRatio : heightProp;

		initQuadMesh();
		initMaterial();
	}

	void KarstVTViewer::setCustomParameters(const Pass* pPass)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt0, mTileTexture, clampLinearSamplerState);
	}

	void KarstVTViewer::initQuadMesh()
	{
		Vertex vtx[] = {
			{ { -0.9f, -0.9f + mHeightProp * 2.0f, 1.0f }, { 0.f, 0.f } },							//	0 - 3
			{ { -0.9f, -0.9f, 1.0f }, { 0.f, 1.f } },												//	| \ |
			{ { -0.9f + mWidthProp * 2.0f, -0.9f, 1.0f }, { 1.f, 1.f } },							//	1 - 2
			{ { -0.9f + mWidthProp * 2.0f, -0.9f + mHeightProp * 2.0f, 1.0f }, { 1.f, 0.f } }
		};
		unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };

		mRenderOp.GetMeshData().m_vertexBuffer.resize(1);

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		RenderOperation::st_MeshData& meshData = mRenderOp.GetMeshData();
		meshData.m_vertexBuffer[0] = pRenderSystem->createStaticVB(vtx, sizeof(vtx));
		meshData.m_vertexCount = 4u;
		meshData.m_IndexBufferData.m_indexBuffer = pRenderSystem->createStaticIB(idx, sizeof(idx));
		meshData.m_IndexBufferData.m_indexCount = 6u;

		mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
	}

	void KarstVTViewer::initMaterial()
	{
		mMaterial = new Material(NULL, Name::g_cEmptyName);

		int offset = 0;
		ECHO_IA_DESC& iaDesc = mMaterial->m_desc.IADesc;
		iaDesc.ElementArray[0].nStream = 0;
		iaDesc.ElementArray[0].nOffset = offset;
		iaDesc.ElementArray[0].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[0].Semantics = ECHO_POSITION0;
		offset += VertexType::typeSize(ECHO_FLOAT3);

		iaDesc.ElementArray[1].nStream = 0;
		iaDesc.ElementArray[1].nOffset = offset;
		iaDesc.ElementArray[1].eType = ECHO_FLOAT2;
		iaDesc.ElementArray[1].Semantics = ECHO_TEXCOORD0;
		offset += VertexType::typeSize(ECHO_FLOAT2);

		iaDesc.nElementCount = 2;

		iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		iaDesc.StreamArray[0].nRate = 1;
		iaDesc.StreamArray[0].nStride = sizeof(Vertex);

		iaDesc.nStreamCount = 1;

		mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.DepthWriteEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.CullMode = ECHO_CULL_NONE;
		mMaterial->m_desc.RenderState.StencilEnable = ECHO_FALSE;
		mMaterial->m_desc.RenderState.DrawMode = ECHO_TRIANGLE_LIST;

		String vsSource, psSource;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("KarstVTViewer_VS.txt"), vsSource);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("KarstVTViewer_PS.txt"), psSource);

		mMaterial->setShaderName("KarstVTViewer_VS", "KarstVTViewer_PS");
		mMaterial->setShaderSrcCode(vsSource, psSource);
		mMaterial->createPass(NormalPass);
		mMaterial->loadInfo();
	}

	void KarstVTViewer::addRenderable(RenderQueueGroup* pqg)
	{
		pqg->addRenderable(mMaterial->getPass(NormalPass), this);
	}

	EchoKarst::KeyPack	KarstRenderable::getGlobalKey() const 
	{
		return mQuadtreeNode->mNodeOffset.key + mKarstPage->mPageOffset.key * ((uint64)1 << mQuadtreeNode->mDepth);
	}

	bool KarstRenderable::checkUpdateMap() const
	{
		//return mNoiseHashValue != mKarstPage->mKarstGroup->getNoiseHashValue();
		return false;
	}

	KarstRenderable::KarstRenderable()
		:mKarstPage(nullptr)
		, mQuadtreeNode(nullptr)
		, mLodParams({})
		, mDist(0.0)
	{
		mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
		mRenderOp.GetMeshData().m_vertexBuffer.resize(3);
		enableCustomParameters(true);
	}

	void KarstRenderable::setCustomParameters(const Pass* pPass)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		const KarstOptions* options = mKarstPage->getOptions();
		// VS
		{
			Vector4 patchInfo;
			patchInfo.x = (float)mQuadtreeNode->mXOffset / options->mGridNum;
			patchInfo.y = (float)mQuadtreeNode->mYOffset / options->mGridNum;
			patchInfo.z = options->mKarstSize;
			patchInfo.w = (float)pow(2, mQuadtreeNode->mDepth);
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, patchInfo);
			Vector3 basePosition = mKarstPage->getBasePosition();
			Vector4 patchPosition = Vector4(basePosition.x, basePosition.y, basePosition.z, 0.0);
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom1, patchPosition);

			auto vtOptions = mKarstPage->getKarstGroup()->getDataVTOptions();
			Vector2 tileCoord = mVTCoord.coord + vtOptions->mPixelOffset;
			Vector2 tileScale = mVTCoord.scale * vtOptions->mVTCacheScale;
			Vector4 tileInfo = Vector4(tileCoord.x, tileCoord.y, tileScale.x, tileScale.y);
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom2, tileInfo);
		}
		// PS
		{
			pRenderSystem->setUniformValue(this, pPass, U_PSCustom0, Vector4(1.0f));
			//pRenderSystem->setTextureSampleValue(this, pPass, S_2DExt0, mKarstPage->getKarstGroup()->getTileNormalTexture(), clampLinearSamplerState);
		}
	}

	void KarstRenderable::renderUpdate()
	{
		RenderOperation::st_MeshData& meshData = mRenderOp.GetMeshData();
		auto karstMesh = mKarstPage->getKarstGroup()->getKarstMesh();
		meshData.m_vertexCount = karstMesh->getVertexCount();
		meshData.m_vertexBuffer[0] = karstMesh->getVertexBuffer();
	
		mKarstPage->getNodeTextureCoord(mVTCoord, this, KarstResourceDataType::HeightNormalMap, true);
		mKarstPage->getNodeHeightRcBuffer(&meshData.m_vertexBuffer[1], mVTCoord.level, mVTCoord.tileID);

		updateLodParams();
		meshData.m_IndexBufferData.m_indexBuffer = karstMesh->getIndexBuffer(mLodParams);
		meshData.m_IndexBufferData.m_indexCount = karstMesh->getIndexCount(mLodParams);

		mKarstPage->getKarstGroup()->getSceneManager()->populateLightList(mWorldAABB, mLightList);
	}

	void KarstRenderable::updateLodParams()
	{
		auto pageNodeOffset = mKarstPage->getPageNodeOffset();
		auto depth = mQuadtreeNode->mDepth;
		auto calculateEdgeType = [this, pageNodeOffset, depth](int xOffset, int yOffset) {
			return mKarstPage->getKarstGroup()->getKarstRenderNodeLod(depth, pageNodeOffset, mQuadtreeNode->mNodeOffset, xOffset, yOffset) ? 0 : 1;
		};
		mLodParams.core = 0;
		mLodParams.left = calculateEdgeType(-1, 0);
		mLodParams.right = calculateEdgeType(1, 0);
		mLodParams.top = calculateEdgeType(0, 1);
		mLodParams.bottom = calculateEdgeType(0, -1);
	}

	const Material* KarstRenderable::getMaterial(void) const
	{
		assert(mKarstPage->getKarstGroup()->getMaterials()->mNodeBaseMaterial.isV1());
		return mKarstPage->getKarstGroup()->getMaterials()->mNodeBaseMaterial.getV1();
	}

	const MaterialWrapper& KarstRenderable::getMaterialWrapper() const
	{
		return mKarstPage->getKarstGroup()->getMaterials()->mNodeBaseMaterial;
	}
}
