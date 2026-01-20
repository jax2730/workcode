//-----------------------------------------------------------------------------
// File:   EchoGpuParamsManager.cpp
//
// Author: miao_yuzhuang
//
// Date:   2016-4-15
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoStableHeaders.h"
#include "EchoGpuParamsManager.h"
#include "EchoMaterial.h"
#include "EchoSceneManager.h"
#include "EchoRoot.h"
#include "EchoFrameListener.h"
#include "EchoShadowmapManager.h"
#include "EchoRenderSystem.h"
#include "EchoRenderStrategy.h"
#include "EchoViewport.h"
#include "EchoTextureManager.h"
#include "EchoWeatherManager.h"
#include "EchoPostProcess.h"
#include "EchoMatrix4.h"
#include "EchoMaterial.h"
#include "EchoMaterialV2.h"

#include "EchoEnvironmentLight.h"
#include "EchoClusteForwardLighting.h"
#include "EchoDeferredRenderStrategy.h"
#include "EchoDownSampleDepth.h"
#include "EchoForwardRenderStrategy.h"
#include "EchoPointLightShadowManager.h"
#include "EchoRainMaskManager.h"
#include "EchoSphericalTerrainFog.h"
#include "EchoRenderer.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	static Mat::TextureBindPoint matV1TBP2Mat2TBP(MAT_TEXTURE_TYPE type)
	{
		switch (type)
		{
		case MAT_TEXTURE_DIFFUSE: return Mat::TEXTURE_BIND_DIFFUSE;
		case MAT_TEXTURE_SPECULAR: return Mat::TEXTURE_BIND_SPECULAR;
		case MAT_TEXTURE_NORMAL: return Mat::TEXTURE_BIND_NORMAL;
		case MAT_TEXTURE_ENVIRONMENT: return Mat::TEXTURE_BIND_ENVIRONMENT;
		case MAT_TEXTURE_CUSTOMER: return Mat::TEXTURE_BIND_CUSTOMER;
		case MAT_TEXTURE_CUSTOMER2: return Mat::TEXTURE_BIND_CUSTOMER2;
		case MAT_TEXTURE_CUSTOMER3: return Mat::TEXTURE_BIND_CUSTOMER3;
		case MAT_TEXTURE_CUSTOMER4: return Mat::TEXTURE_BIND_CUSTOMER4;
		case MAT_TEXTURE_CUSTOMER5: return Mat::TEXTURE_BIND_CUSTOMER5;
		case MAT_TEXTURE_CUSTOMER6: return Mat::TEXTURE_BIND_CUSTOMER6;
		case MAT_TEXTURE_CUSTOMER7: return Mat::TEXTURE_BIND_CUSTOMER7;
		case MAT_TEXTURE_CUSTOMER8: return Mat::TEXTURE_BIND_CUSTOMER8;
		case MAT_TEXTURE_CUSTOMER9: return Mat::TEXTURE_BIND_CUSTOMER9;
		case MAT_TEXTURE_CUSTOMER10: return Mat::TEXTURE_BIND_CUSTOMER10;
		case MAT_TEXTURE_REFLECTCUBE: return Mat::TEXTURE_BIND_REFLECTION_CUBE;
		default:
			break;
		}
		return Mat::TEXTURE_BIND_COUNT;
	}

	//=============================================================================
	GpuParamsManager::GpuParamsManager()
		: //m_customSpecture(-1.0f)
		//, m_customAlpha(-1.0f)
		//, m_customOpacity(-1.0f)
		//, m_customDissovleStrength(-1.0f)
		//, m_customAnimationParams(Vector4::ZERO)
		//, m_mapUniformParams(nullptr)
		//, m_mapVsUniformParams(nullptr)
		 //m_v4CustomParam(Vector4::ZERO)
		////, m_arrCustomTexture(nullptr)
		mCustomTexKeyDiffuse("Diffuse")
		, mCustomTexKeySpecular("Specular")
		, mCustomTexKeyBump("Bump")
		, mCustomTexKeyCustomer("Customer")
		, mCustomTexKeyCustomer2("Customer2")
		, mCustomTexKeyCustomer3("Customer3")
		, mCustomTexKeyCustomer4("Customer4")
		, mCustomTexKeyCustomer5("Customer5")
		, mCustomTexKeyCustomer6("Customer6")
		, mCustomTexKeyCustomer7("Customer7")
		, mCustomTexKeyCustomer8("Customer8")
		, mCustomTexKeyCustomer9("Customer9")
		, mCustomTexKeyCustomer10("Customer10")
		, m_hasCustomMainLightParam(false)
		//, m_bUseCostomDiffuseColor(false)
		//, m_bUseLocalMatrix(false)
		, m_sceneManager(nullptr)
		, mUniformBlockCommonVSData(nullptr)
		, mUniformBlockCommonPSData(nullptr)
	{
		EchoZoneScoped;

		for (int i = 0; i < 256; ++i)
		{
			m_boneMatrix[i] = &Matrix4::IDENTITY;
		}
		m_pWorldMatrix = &DBMatrix4::IDENTITY;
		mInvWorldMatrix = &DBMatrix4::IDENTITY;
		//m_worldMatrix = DBMatrix4::IDENTITY;
		//m_localMatrix = DBMatrix4::IDENTITY;
		m_arrCustomTexture = NULL;
		//memset(m_arrCustomTexture, 0, sizeof(m_arrCustomTexture));
		//resetCustomUniform();
	}

	//=============================================================================
	GpuParamsManager::~GpuParamsManager()
	{
		EchoZoneScoped;

		cleanup();
	}


	//=============================================================================
	void GpuParamsManager::cleanup()
	{
		EchoZoneScoped;

		RenderSystem* renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem != nullptr && renderSystem->isUseSingleThreadRender()) //单线程模式下
		{
			if (mUniformBlockCommonVSData)
				free(mUniformBlockCommonVSData);

			if (mUniformBlockCommonPSData)
				free(mUniformBlockCommonPSData);
		}

		mUniformBlockCommonVSData = nullptr;
		mUniformBlockCommonPSData = nullptr;
	}

	bool GpuParamsManager::commitPublicUniformParams(RenderSystem* renderSystem)
	{
		EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool bCommitOk = false;

			RcUniformBlock* pUniformBlockVS = renderSystem->getUniformBlockCommonVS();
			if (pUniformBlockVS->pNative && pUniformBlockVS->dataSize > 0 && pUniformBlockVS->uniformInfos)
			{
				pUniformBlockVS->pData = renderSystem->getUniformData(pUniformBlockVS->dataSize);

				unsigned char* pBlockData = (unsigned char*)(pUniformBlockVS->pData);

				UniformInfoMap::const_iterator it, ite = pUniformBlockVS->uniformInfos->end();
				for (it = pUniformBlockVS->uniformInfos->begin(); it != ite; ++it)
				{
					const ECHO_UNIFORM_INFO& uniformInfo = it->second;
					void* pDataBuffer = pBlockData + uniformInfo.nOffset;
					commitUniform(uniformInfo.eNameID, pDataBuffer, uniformInfo.nDataSize);
				}
				bCommitOk = true;

				//备份一下common vs data, 后续渲FPS的队列时，会修改一部分数据
				if (renderSystem->isUseSingleThreadRender()) //单线程模式下
				{
					if (mUniformBlockCommonVSData)
					{
						memcpy(mUniformBlockCommonVSData, pUniformBlockVS->pData, pUniformBlockVS->dataSize);
					}
				}
				else
				{
					mUniformBlockCommonVSData = pUniformBlockVS->pData;
				}

				renderSystem->updateUniform(pUniformBlockVS, pUniformBlockVS->pData);
				pUniformBlockVS->pData = nullptr;
			}

			RcUniformBlock* pUniformBlockPS = renderSystem->getUniformBlockCommonPS();
			if (pUniformBlockPS->pNative && pUniformBlockPS->dataSize > 0 && pUniformBlockPS->uniformInfos)
			{
				pUniformBlockPS->pData = renderSystem->getUniformData(pUniformBlockPS->dataSize);

				unsigned char* pBlockData = (unsigned char*)(pUniformBlockPS->pData);

				UniformInfoMap::const_iterator it, ite = pUniformBlockPS->uniformInfos->end();
				for (it = pUniformBlockPS->uniformInfos->begin(); it != ite; ++it)
				{
					const ECHO_UNIFORM_INFO& uniformInfo = it->second;
					void* pDataBuffer = pBlockData + uniformInfo.nOffset;
					commitUniform(uniformInfo.eNameID, pDataBuffer, uniformInfo.nDataSize);
				}
				bCommitOk = true;

				//备份一下common ps data, 后续渲FPS的队列时，会修改一部分数据
				if (renderSystem->isUseSingleThreadRender()) //单线程模式下
				{
					if (mUniformBlockCommonPSData)
					{
						memcpy(mUniformBlockCommonPSData, pUniformBlockPS->pData, pUniformBlockPS->dataSize);
					}
				}
				else
				{
					mUniformBlockCommonPSData = pUniformBlockPS->pData;
				}

				renderSystem->updateUniform(pUniformBlockPS, pUniformBlockPS->pData);
				pUniformBlockPS->pData = nullptr;
			}

			return bCommitOk;
		}
		else
		{
			assert(false && "Unsupport gx render!");
			//renderSystem->beginUpdatePublicUniforms();
			//const EchoUniformParamsList& paramsList = renderSystem->getPublicUniformParams();
			//EchoUniformParamsList::const_iterator it, ite = paramsList.end();
			//for (it = paramsList.begin(); it != ite; ++it)
			//{
			//	EchoUniform* uniform = it->second;
			//	void* pDataBuffer = 0;
			//	if (!uniform->Map(&pDataBuffer))
			//	{
			//		assert(false && "map uniform failed.");
			//		continue;
			//	}
			//	commitUniform(it->first, pDataBuffer);
			//	uniform->Unmap();
			//}
			//renderSystem->endUpdatePublicUniforms();
			return true;
		}
	}

	//=============================================================================
	void GpuParamsManager::commitAllGpuParams(Renderable* rend, const Pass* pPass)
	{
		EchoZoneScoped;

		//-----------------------------------------------------------------------------
		commitUniformParams(rend, pPass);

		//-----------------------------------------------------------------------------
		commitSamplerParams(rend, pPass);
	}

	//=============================================================================
	void GpuParamsManager::commitPrivateParams(Renderable* rend, const Pass* pPass)
	{
		EchoZoneScoped;

		assert(false && "Unsupport gx render!");
		//rend->getWorldTransforms(&m_pWorldMatrix);
		////rend->getWorldTransforms(&m_worldMatrix);
		////m_bUseLocalMatrix = rend->getLocalTransforms(&m_localMatrix);
		//m_numBoneMatrix = rend->getNumBoneTransforms();
		//m_material = (Material*)pPass->getMaterial();
		////m_customSpecture = rend->getCustomSpecular();
		////m_customAlpha = rend->getCustomAlpha();
		////m_customDissovleStrength = rend->getCustomDissovleStrength();
		////m_customAnimationParams = rend->getCustomAnimationParams();
		////m_customOpacity = rend->getCustomOpacity();
		////m_bUseCostomDiffuseColor = rend->getCustomDiffuseColor(m_customDiffuseColor);

		//m_mapUniformParams = rend->getCustomUniformParams();
		//m_mapVsUniformParams = rend->getCustomVsUniformParams();
		//m_mapMaterialUniformParams = rend->getCustomMaterialUniformParams();
		//rend->getCustomParam(&m_v4CustomParam);

		//updateCustomUniform();
		//if (m_numBoneMatrix > 64)
		//{
		//	assert(false);
		//	m_numBoneMatrix = 64;
		//}
		//if (m_numBoneMatrix)
		//{
		//	rend->getBoneTransforms(m_boneMatrix);
		//}


		////-----------------------------------------------------------------------------
		//const EchoUniformParamsList * paramsList = pPass->getUniformParamsList();
		//if (paramsList != nullptr)
		//{
		//	EchoUniformParamsList::const_iterator it, ite = paramsList->end();
		//	for (it = paramsList->begin(); it != ite; ++it)
		//	{
		//		uint32 eNameID = it->first;
		//		if (eNameID >= UniformNameID_Count)
		//			continue;

		//		EchoUniform * pEchoUniform = it->second;

		//		switch (eNameID)
		//		{
		//		case	U_WorldViewProjectMatrix:
		//		case	U_WorldMatrix:
		//		case	U_InvWorldMatrix:
		//		case	U_InvTransposeWorldMatrix:
		//		case	U_WorldRotationMatrix:
		//		case	U_WorldViewProjectNoRotationMatrix:
		//		case	U_InvWorldViewProjectMatrix:
		//		case	U_WorldScale:
		//		case	U_BoneMatrix:
		//		case	U_MaterialDiffuse:
		//		case	U_MaterialSpecular:
		//		case	U_MaterialFresnel:
		//		case	U_MaterialEmission:
		//		case	U_MaterialCustomParam:
		//		case	U_MaterialCustomParam1:
		//		case	U_GlobalBuildingColor:
		//		case	U_ReflectCubeMapMipLevels:

		//		case	U_LightCount:
		//		case	U_LightPower:
		//		case	U_LightWorldPosiArray:
		//		case	U_LightAttenuationArray:
		//		case	U_LightDiffuseColorArray:
		//		case	U_WindParamsObjSpace:
		//		case	U_AnimationParams:

		//		case	U_VSGeneralRegister0:
		//		case	U_VSGeneralRegister1:
		//		case	U_VSGeneralRegister2:
		//		case	U_VSGeneralRegister3:
		//		case	U_VSGeneralRegister4:
		//		case	U_VSGeneralRegister5:
		//		case	U_VSGeneralRegister6:
		//		case	U_VSGeneralRegister7:

		//		case	U_PSGeneralRegister0:
		//		case	U_PSGeneralRegister1:
		//		case	U_PSGeneralRegister2:
		//		case	U_PSGeneralRegister3:
		//		case	U_PSGeneralRegister4:
		//		case	U_PSGeneralRegister5:
		//		case	U_PSGeneralRegister6:
		//		case	U_PSGeneralRegister7:
		//		case	U_PSGeneralRegister8:
		//		case	U_PSGeneralRegister9:
		//		case	U_PSGeneralRegister10:
		//		case	U_PSGeneralRegister11:
		//		case	U_PSGeneralRegister12:
		//		case	U_PSGeneralRegister13:
		//		case	U_PSGeneralRegister14:
		//		case	U_PSGeneralRegister15:
		//		case	U_PSGeneralRegister16:
		//		case	U_PSGeneralRegister17:
		//		case	U_PSGeneralRegister18:
		//		case	U_PSGeneralRegister19:
		//		case	U_PSGeneralRegister20:
		//		case	U_PSGeneralRegister21:
		//		case	U_PSGeneralRegister22:
		//		case	U_PSGeneralRegister23:
		//		case	U_PSGeneralRegister24:
		//		case	U_PSGeneralRegister25:
		//		case	U_PSGeneralRegister26:
		//		case	U_PSGeneralRegister27:
		//		case	U_PSGeneralRegister28:
		//		case	U_PSGeneralRegister29:
		//		case	U_PSGeneralRegister30:
		//		case	U_PSGeneralRegister31:
		//		case	U_PSGeneralRegister32:
		//		{
		//			void* pDataBuffer = 0;
		//			if (pEchoUniform->Map(&pDataBuffer))
		//			{
		//				commitUniform(it->first, pDataBuffer);
		//				pEchoUniform->Unmap();
		//			}
		//		}
		//		break;
		//		default:
		//			;
		//		}
		//	}
		//}
		//
		////-----------------------------------------------------------------------------
		//commitSamplerParams(rend, pPass);
	}

	void GpuParamsManager::setShaderDebugFlag(ShaderDebugFlag flag) { m_ShaderDebugFlag = flag; }
	GpuParamsManager::ShaderDebugFlag GpuParamsManager::getShaderDebugFlag()const { return m_ShaderDebugFlag; }

	bool GpuParamsManager::updateCommitPublicViewGpuParams(RenderSystem* renderSystem)
	{
		EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcUniformBlock* pUniformBlockVS = renderSystem->getUniformBlockCommonVS();
			if (pUniformBlockVS->pNative && pUniformBlockVS->dataSize > 0 && pUniformBlockVS->uniformInfos)
			{
				void* pUniformBlockVSData = renderSystem->getUniformData(pUniformBlockVS->dataSize);

				if (renderSystem->isUseSingleThreadRender()) //单线程模式下
				{
					if (!mUniformBlockCommonVSData)
					{
						mUniformBlockCommonVSData = malloc(pUniformBlockVS->dataSize);
					}
					else
					{
						memcpy(pUniformBlockVSData, mUniformBlockCommonVSData, pUniformBlockVS->dataSize);
					}
				}
				else //非单线程模式
				{
					if (mUniformBlockCommonVSData)
						memcpy(pUniformBlockVSData, mUniformBlockCommonVSData, pUniformBlockVS->dataSize);
				}

				unsigned char* pBlockData = (unsigned char*)(pUniformBlockVSData);

				UniformInfoMap::const_iterator it, ite = pUniformBlockVS->uniformInfos->end();
				for (it = pUniformBlockVS->uniformInfos->begin(); it != ite; ++it)
				{
					const ECHO_UNIFORM_INFO& uniformInfo = it->second;
					uint32 eNameID = uniformInfo.eNameID;
					switch (eNameID)
					{
					case U_ViewProjectMatrix:
					case U_ViewMatrix:
					case U_ZeroViewProjectMatrix:
					case U_VS_CameraPosition:
					case U_CameraParam:
					case U_VS_ViewParam:
					case U_ViewDirection:
					case U_ViewRightVector:
					case U_ViewUpVector:
					case U_MainCamViewMatrix:
					case U_LightVPMatrix0:
					case U_LightVPMatrix1:
					case U_LightVPMatrix2:
					case U_LightVPMatrix3:
					case U_LightVPMatrixView0:
					case U_LightVPMatrixView1:
					case U_LightVPMatrixView2:
					case U_LightVPMatrixView3:
					case U_ProjectMatrix:
					{
						void* pDataBuffer = pBlockData + uniformInfo.nOffset;
						commitUniform(uniformInfo.eNameID, pDataBuffer, uniformInfo.nDataSize);
					}
					break;
					default:
						break;
					}
				}
				renderSystem->updateUniform(pUniformBlockVS, pUniformBlockVSData);
			}

			RcUniformBlock* pUniformBlockPS = renderSystem->getUniformBlockCommonPS();
			if (pUniformBlockPS->pNative && pUniformBlockPS->dataSize > 0 && pUniformBlockPS->uniformInfos)
			{
				void* pUniformBlockPSData = renderSystem->getUniformData(pUniformBlockPS->dataSize);

				if (renderSystem->isUseSingleThreadRender()) //单线程模式下
				{
					if (!mUniformBlockCommonPSData)
					{
						mUniformBlockCommonPSData = malloc(pUniformBlockPS->dataSize);
					}
					else
					{
						memcpy(pUniformBlockPSData, mUniformBlockCommonPSData, pUniformBlockPS->dataSize);
					}
				}
				else //非单线程模式
				{
					if (mUniformBlockCommonPSData)
						memcpy(pUniformBlockPSData, mUniformBlockCommonPSData, pUniformBlockPS->dataSize);
				}

				unsigned char* pBlockData = (unsigned char*)(pUniformBlockPSData);

				UniformInfoMap::const_iterator it, ite = pUniformBlockPS->uniformInfos->end();
				for (it = pUniformBlockPS->uniformInfos->begin(); it != ite; ++it)
				{
					const ECHO_UNIFORM_INFO& uniformInfo = it->second;
					uint32 eNameID = uniformInfo.eNameID;
					switch (eNameID)
					{
					case U_ViewParam:
					case U_PS_CameraPosition:
					case U_PS_ViewDirection:
					{
						void* pDataBuffer = pBlockData + uniformInfo.nOffset;
						commitUniform(uniformInfo.eNameID, pDataBuffer, uniformInfo.nDataSize);
					}
					break;
					default:
						break;
					}
				}
				renderSystem->updateUniform(pUniformBlockPS, pUniformBlockPSData);
			}
		}

		return true;
	}

	void GpuParamsManager::commitSamplerParams(Renderable* rend, const Pass* pPass)
	{
		EchoZoneScoped;
		// m_material = rend->getMaterialWrapper().getCPtr();
		m_renderable = rend;
		const MaterialWrapper& material = m_renderable->getMaterialWrapper();

		//m_arrCustomTexture = rend->getCustomTexture();

		//memset(m_arrCustomTexture, 0, sizeof(m_arrCustomTexture));
		m_arrCustomTexture = rend->getCustomTexture();
		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pPass);
			if (!pDrawData)
				return;

			RcRenderPipeline* pRcPipeline = reinterpret_cast<RcRenderPipeline*>(pPass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->samplerDescInfos)
				return;

			static uint32 g_SamplerPtrChanged = 0xFFFF;

			SamplerDescribeInfoMap::const_iterator it, ite = pRcPipeline->samplerDescInfos->end();
			for (it = pRcPipeline->samplerDescInfos->begin(); it != ite; ++it)
			{
				uint32 eNameID = it->first;
				const ECHO_SAMPLER_DESCRIBE_INFO& desc = it->second;

				if (eNameID >= SamplerNameID_Count || desc.nIndex >= ECHO_MAX_SAMPLER_COUNT)
					continue;

				ECHO_DRAW_SAMPLER_INFO& info = pDrawData->drawSamplerInfo[desc.nIndex];
				ECHO_DRAW_SAMPLER_INFO tempInfo;

				if (material.isV1())
				{
					tempInfo.samplerState = material.getV1()->getTextureSampleState(pPass->getType(), desc.eNameID);
				}

				getSamplerInfo(desc.eNameID, tempInfo);

				if (!info.pSampler && tempInfo.pSampler)
				{
					pDrawData->pCustom = &g_SamplerPtrChanged;
				}

				if (pDrawData->pNative && info.pSampler)
				{
					if (info.pSampler != tempInfo.pSampler)
						pDrawData->pCustom = &g_SamplerPtrChanged;
					else if (tempInfo.pSampler && (tempInfo.pSampler->pNative != info.pSampler->pNative))
						pDrawData->pCustom = &g_SamplerPtrChanged;
				}

				info.pSampler = tempInfo.pSampler;
				info.samplerState = tempInfo.samplerState;
			}
		}
		else
		{
			assert(false && "Does not support GX rendering!");
			// RenderSystem *pRenderSystem = Root::instance()->getRenderSystem();
			// const EchoSamplersList * samplerList = pPass->getSamplersList();
			//
			// if (samplerList != nullptr)
			// {
			// 	EchoSamplersList::const_iterator it, ite = samplerList->end();
			// 	for (it = samplerList->begin(); it != ite; ++it)
			// 	{
			// 		uint32 eNameID = it->first;
			// 		if (eNameID >= SamplerNameID_Count)
			// 			continue;
			//
			// 		EchoSampler* pEchoSampler = it->second;
			//
			// 		ECHO_DRAW_SAMPLER_INFO info;
			// 		info.samplerState = m_material->getTextureSampleState(pPass->getType(), it->first);
			//
			// 		getSamplerInfo(it->first, info);
			//
			// 		if (info.pSampler)
			// 		{
			// 			pRenderSystem->setTextureSampleValue(pEchoSampler,info.pSampler, info.samplerState);
			// 		}
			// 	}
			// }
		}
	}

	void GpuParamsManager::commitSamplerParams(Renderable* rend, const Pass* pPass, std::vector<ECHO_DRAW_SAMPLER_COMBINED>& samplers)
	{
		EchoZoneScoped;
		// m_material = rend->getMaterialWrapper().getCPtr();
		m_renderable = rend;
		const MaterialWrapper& material = m_renderable->getMaterialWrapper();

		m_arrCustomTexture = rend->getCustomTexture();
		if (Root::instance()->isUseRenderCommand())
		{
			RcRenderPipeline* pRcPipeline = reinterpret_cast<RcRenderPipeline*>(pPass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->samplerDescInfos)
				return;

			uint32 beg = static_cast<uint32>(samplers.size());
			samplers.resize(beg + SupportArrayTextureCount);

			for (uint32 eName = S_Diffuse; eName < SamplerNameID_Count; ++eName)
			{
				//skip public texture
				if (((1 << eName) & GpuCullSupportArrayTexture_SN) == 0)
				{
					break;
				}
				ECHO_DRAW_SAMPLER_COMBINED& samplerInfo = samplers[beg + eName];
				if (material.isV1())
				{
					samplerInfo.samplerState = material.getV1()->getTextureSampleState(pPass->getType(), eName);
				}
				// MaterialV2 会在 getSamplerInfo 中获取到 samplerState

				//if (pRcPipeline->samplerDescInfos->find(eName) != pRcPipeline->samplerDescInfos->end())
				{
					/*samplerInfo.samplerState = {
						ECHO_TEX_ADDRESS_WRAP, ECHO_TEX_ADDRESS_WRAP, ECHO_TEX_ADDRESS_WRAP,
						ECHO_TEX_FILTER_LINEAR, ECHO_TEX_FILTER_LINEAR, ECHO_TEX_FILTER_LINEAR,
						ECHO_TEX_CMP_NONE, ECHO_CMP_GREATER
					};*/

					getSamplerInfo(eName, samplerInfo);
				}
			}
		}
		else
		{

		}
	}

	bool GpuParamsManager::commitUniformParams(Renderable* rend, const Pass* pPass)
	{
		EchoZoneScoped;

		m_pWorldMatrix = &DBMatrix4::IDENTITY;
		rend->getWorldTransforms(&m_pWorldMatrix);

		mInvWorldMatrix = &DBMatrix4::IDENTITY;
		rend->getInvWorldTransforms(&mInvWorldMatrix);
		//m_pWorldMatrix->decomposition(mWorldPosition, mWorldScale, mWorldOrientation);
		//mInvWorldMatrix.makeInverseTransform(mWorldPosition, mWorldScale, mWorldOrientation);
		//rend->getWorldTransforms(&m_worldMatrix);
		//m_bUseLocalMatrix = rend->getLocalTransforms(&m_localMatrix);
		m_numBoneMatrix = rend->getNumBoneTransforms();
		m_renderable = rend;
		// m_material = rend->getMaterialWrapper().getCPtr();
		//m_customSpecture = rend->getCustomSpecular();
		//m_customAlpha = rend->getCustomAlpha();
		//m_customDissovleStrength = rend->getCustomDissovleStrength();
		//m_customStippleTransparency = rend->getStippleTransparency();
		//m_customStippleSize = rend->getStippleSize();
		//m_customOpacity = rend->getCustomOpacity();
		//m_customAnimationParams = rend->getCustomAnimationParams();
		//m_bUseCostomDiffuseColor = rend->getCustomDiffuseColor(m_customDiffuseColor);

		m_hasCustomMainLightParam = rend->getCustomMainLightParam(m_CustomMainLightColor, m_CustomMainLightDir);
		m_mapUniformParams = rend->getCustomUniformParams();
		m_mapVsUniformParams = rend->getCustomVsUniformParams();
		m_mapMaterialUniformParams = rend->getCustomMaterialUniformParams();
		m_pMatCustomUniform = rend->getCustomUniformParamsByType(UNIFORM_TYPE_MAT);
		m_pVsCustomUniform = rend->getCustomUniformParamsByType(UNIFORM_TYPE_VS);
		m_pPsCustomUniform = rend->getCustomUniformParamsByType(UNIFORM_TYPE_PS);
		//updateCustomUniform(rend);

		//rend->getCustomParam(&m_v4CustomParam);

		if (m_numBoneMatrix > 64)
		{
			assert(false);
			m_numBoneMatrix = 64;
		}

		if (m_numBoneMatrix)
		{
			rend->getBoneTransforms(m_boneMatrix);
		}

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pPass);
			if (!pDrawData)
				return false;

			bool bCommitUniformOk = false;
			for (unsigned int i = ECHO_UNIFORM_BLOCK_PRIVATE_VS; i < ECHO_UNIFORM_BLOCK_USAGE_COUNT; i++)
			{
				RcUniformBlock* pUniformBlock = pDrawData->pUniformBlock[i];
				if (pUniformBlock && pUniformBlock->uniformInfos)
				{
					bCommitUniformOk = true;

					if (!pUniformBlock->pData)
					{
						continue;
					}

					unsigned char* pBlockData = (unsigned char*)(pUniformBlock->pData);

					UniformInfoMap::const_iterator it, ite = pUniformBlock->uniformInfos->end();
					for (it = pUniformBlock->uniformInfos->begin(); it != ite; ++it)
					{
						uint32 eNameID = it->first;
						if (eNameID >= UniformNameID_Count)
							continue;

						const ECHO_UNIFORM_INFO& uniformInfo = it->second;
						void* pDataBuffer = pBlockData + uniformInfo.nOffset;
						commitUniform(uniformInfo.eNameID, pDataBuffer, uniformInfo.nDataSize);
					}
				}
			}

			return bCommitUniformOk;
		}
		else
		{
			assert(false && "Unsupport gx render!");
			//const EchoUniformParamsList * paramsList = pPass->getUniformParamsList();
			//if (paramsList != nullptr)
			//{
			//	EchoUniformParamsList::const_iterator it, ite = paramsList->end();
			//	for (it = paramsList->begin(); it != ite; ++it)
			//	{
			//		uint32 eNameID = it->first;
			//		if (eNameID >= UniformNameID_Count)
			//			continue;

			//		EchoUniform * pEchoUniform = it->second;
			//		void* pDataBuffer = 0;
			//		if (!pEchoUniform->Map(&pDataBuffer))
			//		{
			//			assert(false && "map uniform failed.");
			//			continue;
			//		}
			//		commitUniform(eNameID, pDataBuffer);
			//		pEchoUniform->Unmap();
			//	}
			//}

			return true;
		}
	}

	//=============================================================================
	void GpuParamsManager::commitUniform(uint32 uniformNameID, void* pBuffer, uint32 data_size)
	{
		EchoZoneScoped;

		RenderSystem* renderSystem = Root::instance()->getRenderSystem();

		Vector4* pVector4 = (Vector4*)pBuffer;
		Matrix4* pMatrix4 = (Matrix4*)pBuffer;
		//float(*pGeneralRegister) = (float*)pBuffer;

		const unsigned int maxPointLightNum = renderSystem->getCurRenderStrategy()->getMaxPointLightNum();

		switch (uniformNameID)
		{
		case U_BoneMatrix:
		{
			if (m_numBoneMatrix)
			{
				for (size_t i = 0; i < m_numBoneMatrix; i++)
				{
					memcpy(pVector4, m_boneMatrix[i], sizeof(Vector4) * 3);
					pVector4 += 3;
				}
			}
		}
		break;
		case U_LastBoneMatrix:
		{
			const Vector4* lastBoneMatrix = NULL;
			if (m_renderable)
			{
				lastBoneMatrix = m_renderable->getLastBoneMatrix();
			}

			uint16 vecNum = m_numBoneMatrix * 3;
			if (NULL == lastBoneMatrix)
			{
				if (m_numBoneMatrix)
				{
					for (size_t i = 0; i < m_numBoneMatrix; i++)
					{
						memcpy(pVector4, m_boneMatrix[i], sizeof(Vector4) * 3);
						pVector4 += 3;
					}
				}
			}
			else
			{
				if (m_numBoneMatrix)
				{
					memcpy(pVector4, lastBoneMatrix, sizeof(Vector4) * vecNum);
				}
			}

			if (m_renderable)
			{
				m_renderable->setLastBoneMatrix(m_boneMatrix, m_numBoneMatrix);
			}

		}
		break;
		case U_LastWorldViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			const DMatrix4& viewMatrix = pCam->getViewMatrix();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			Matrix4 wvp = DBMatrix4::concatenate(projectMatrix, viewMatrix, *m_pWorldMatrix, pCam->getPosition());

			const Matrix4* lastWVP = NULL;
			if (m_renderable)
			{
				lastWVP = m_renderable->getLastWVP();
			}
			if (NULL == lastWVP)
			{
				*pMatrix4 = wvp;
			}
			else
				*pMatrix4 = *lastWVP;

			if (m_renderable)
			{
				m_renderable->setLastWVP(wvp);
			}

		}
		break;
		case U_VS_WorldViewMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			const DMatrix4& viewMatrix = pCam->getViewMatrix();
			*pMatrix4 = viewMatrix.concatenate(*m_pWorldMatrix);
		}
		break;
		case U_VS_InvWorldViewMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			const DMatrix4& viewMatrix = pCam->getViewMatrix();
			*pMatrix4 = viewMatrix.concatenate(*m_pWorldMatrix).inverse();
		}
		break;
		case U_WorldViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			const DMatrix4& viewMatrix = pCam->getViewMatrix();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			// *pMatrix4 = projectMatrix * viewMatrix * (m_worldMatrix);

			//TODO(yanghang): Optimize this through cache projection*view matrix of camera.
			*pMatrix4 = DBMatrix4::concatenate(projectMatrix, viewMatrix, *m_pWorldMatrix, pCam->getPosition());
			//if (m_bUseLocalMatrix)
			//	*pMatrix4 =pMatrix4->concatenate(m_localMatrix);
		}
		break;
		case U_ViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			*pMatrix4 = projectMatrix * pCam->getViewMatrix();
		}
		break;

		case U_InvViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			Matrix4 vpMat = projectMatrix * pCam->getViewMatrix();
			*pMatrix4 = vpMat.inverse();
		}
		break;

		case U_ViewMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			*pMatrix4 = pCam->getViewMatrix();
		}
		break;

		case U_ProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			*pMatrix4 = projectMatrix;
		}
		break;

		case U_InvProjectMatrix:
		case U_VS_InvProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			*pMatrix4 = projectMatrix.inverseWithHighPrecision();
		}
		break;

		case U_ZeroViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			*pMatrix4 = pCam->getViewMatrix();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			(*pMatrix4)[0][3] = 0.0f;
			(*pMatrix4)[1][3] = 0.0f;
			(*pMatrix4)[2][3] = 0.0f;
			(*pMatrix4)[3][3] = 1.0f;
			(*pMatrix4) = projectMatrix * (*pMatrix4);
		}
		break;

		case U_WorldMatrix: //3x4
		{
			Matrix4 tempMat = *m_pWorldMatrix;
			//if (m_bUseLocalMatrix)
			//{
			//	tempMat = tempMat.concatenate(m_localMatrix);
			//}
			for (size_t i = 0; i < 3; i++)
			{
				const float* p = tempMat[i];
				pVector4->x = *p++;
				pVector4->y = *p++;
				pVector4->z = *p++;
				pVector4->w = *p++;

				++pVector4;
			}
		}
		break;

		case U_InvWorldMatrix: //3x4
		{
			//Matrix4 tempMat = *m_pWorldMatrix;
			////if (m_bUseLocalMatrix)
			////{
			////	tempMat = tempMat.concatenate(m_localMatrix);
			////}
			//tempMat = tempMat.inverse();
			//for (size_t i = 0; i < 3; i++)
			//{
			//	const float *p = tempMat[i];
			//	pVector4->x = *p++;
			//	pVector4->y = *p++;
			//	pVector4->z = *p++;
			//	pVector4->w = *p++;

			//	++pVector4;
			//}

			constexpr const unsigned matrix_item_count = sizeof(Matrix4) / sizeof(decltype(Matrix4::_11));
			unsigned item_count = data_size / sizeof(decltype(Matrix4::_11));
			if (item_count > matrix_item_count)
				item_count = matrix_item_count;
			for (unsigned i = 0; i < item_count; ++i)
				pMatrix4->_m[i] = (float)mInvWorldMatrix->_m[i];
		}
		break;

		case U_WorldMatrixBaseView:
		{
				*pMatrix4 = *m_pWorldMatrix;
			Camera* pCam = m_sceneManager->getActiveCamera();
			const DVector3& cPos = pCam->getPosition();
			pMatrix4->m[0][3] = float(m_pWorldMatrix->m[0][3] - cPos.x);
			pMatrix4->m[1][3] = float(m_pWorldMatrix->m[1][3] - cPos.y);
			pMatrix4->m[2][3] = float(m_pWorldMatrix->m[2][3] - cPos.z);

		}
		break;

		case U_InvTransposeWorldMatrix: // 4 x 4
		{
			assert(data_size <= sizeof(Matrix4));
			if (data_size > sizeof(Matrix4))
				data_size = sizeof(Matrix4);
			Matrix4 mat_transpose = mInvWorldMatrix->transpose();
			memcpy(pBuffer, mat_transpose._m, data_size);
		}
		break;

		case U_WorldRotationMatrix: //4x3
		{
			const Quaternion* pQuat = &Quaternion::IDENTITY;
			m_renderable->getRenderableWorldOrientation(&pQuat);
			Matrix4 tempMatrix4;
			tempMatrix4.makeTransform(Vector3::ZERO, Vector3::UNIT_SCALE, *pQuat);	// only rotation

			for (size_t i = 0; i < 3; i++)
			{
				const float* p = tempMatrix4[i];
				pVector4->x = *p++;
				pVector4->y = *p++;
				pVector4->z = *p++;
				pVector4->w = *p++;

				++pVector4;
			}
		}
		break;

		case U_WorldViewProjectNoRotationMatrix:
		{
			const DBVector3* pWorldPosition = &DBVector3::ZERO;
			const Vector3* pWorldScale = &Vector3::UNIT_SCALE;
			m_renderable->getRenderableWorldPosition(&pWorldPosition);
			m_renderable->getRenderableWorldScale(&pWorldScale);
			Matrix4 matNoRotation;
			matNoRotation.makeTransform(*pWorldPosition, *pWorldScale, Echo::Quaternion::IDENTITY);		// without rotation
			Camera* pCam = m_sceneManager->getActiveCamera();
			const DMatrix4& viewMatrix = pCam->getViewMatrix();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			*pMatrix4 = projectMatrix * viewMatrix * matNoRotation;
		}
		break;

		case U_InvWorldViewProjectMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();

			const DMatrix4& viewMatrix = pCam->getViewMatrix();

			Matrix4 projectMatrix;
			if (Root::instance()->getEnableReverseDepth())
			{
				projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
			}
			else
			{
				projectMatrix = pCam->getProjectionMatrixWithRSDepth();
			}

			if (pCam->isUseJitter())
			{
				Viewport* vp = pCam->getViewport();
				if (vp)
				{
					renderSystem->jitterProjectMatrix(vp->getActualWidth(), vp->getActualHeight(), projectMatrix);
				}
			}

			Matrix4 wvpMatrix = projectMatrix * viewMatrix * (*m_pWorldMatrix);
			//if (m_bUseLocalMatrix)
			//{
			//	wvpMatrix = wvpMatrix.concatenate(m_localMatrix);
			//}
			*pMatrix4 = wvpMatrix.inverse();
		}
		break;

		case U_WorldPosition:
		{
			const DBVector3* pWorldPosition = &DBVector3::ZERO;
			m_renderable->getRenderableWorldPosition(&pWorldPosition);
			*pVector4 = *pWorldPosition;
		}
		break;
		case U_WorldScale:
		{
			const Vector3* pWorldScale = &Vector3::UNIT_SCALE;
			m_renderable->getRenderableWorldScale(&pWorldScale);
			*pVector4 = *pWorldScale;
		}
		break;
		case U_WorldOrientation:
		{
			const Quaternion* pQuat = &Quaternion::IDENTITY;
			m_renderable->getRenderableWorldOrientation(&pQuat);
			*(Quaternion*)pVector4 = *pQuat;
		}
		break;
		case U_ObjectMirrored:
		{
			const Vector3* pWorldScale = &Vector3::UNIT_SCALE;
			m_renderable->getRenderableWorldScale(&pWorldScale);
			int x_mirrored = pWorldScale->x >= 0 ? 0 : 1;
			int y_mirrored = pWorldScale->y >= 0 ? 0 : 1;
			int z_mirrored = pWorldScale->z >= 0 ? 0 : 1;
			pVector4->x = (float)x_mirrored;
			pVector4->y = (float)y_mirrored;
			pVector4->z = (float)z_mirrored;
			pVector4->w = ((x_mirrored + y_mirrored + z_mirrored) & 1) == 0 ? 1.f : -1.f;
		}
		break;

		case U_CameraPosition:
		case U_VS_CameraPosition:
		case U_PS_CameraPosition:
		{
			const Vector3& pCameraPosition = m_sceneManager->getActiveCamera()->getPosition();
			pVector4->x = pCameraPosition.x;
			pVector4->y = pCameraPosition.y;
			pVector4->z = pCameraPosition.z;
			pVector4->w = 1.0f;
		}
		break;

		case U_CameraPositionLocalSpace:
		{
			const DVector3 pCameraPosition = m_sceneManager->getActiveCamera()->getPosition();
			const DBVector3* pWorldPosition = &DBVector3::ZERO;
			m_renderable->getRenderableWorldPosition(&pWorldPosition);
			const Vector3* pWorldScale = &Vector3::UNIT_SCALE;
			m_renderable->getRenderableWorldScale(&pWorldScale);
			const Quaternion* pQuat = &Quaternion::IDENTITY;
			m_renderable->getRenderableWorldOrientation(&pQuat);
			DBMatrix4 tempInvWorldMatrix;
			tempInvWorldMatrix.makeInverseTransform(*pWorldPosition - pCameraPosition, *pWorldScale, *pQuat);
			const DVector3 CameraPositionL = tempInvWorldMatrix * DVector3::ZERO;
			pVector4->x = (float)CameraPositionL.x;
			pVector4->y = (float)CameraPositionL.y;
			pVector4->z = (float)CameraPositionL.z;
			pVector4->w = 1.0f;
		}
		break;

		case U_CameraParam:
		{
			const Camera* camera = m_sceneManager->getActiveCamera();
			pVector4->x = camera->getFarClipDistance();
			pVector4->y = camera->getNearClipDistance();
			pVector4->z = pVector4->y / pVector4->x;
			pVector4->w = 1.0f / pVector4->x;
		}
		break;

		case U_VS_ViewParam:
		case U_ViewParam:
		{
			Viewport* pVP = m_sceneManager->getCurrentViewport();

			pVector4->x = pVP->getRealWidth() * pVP->getTarget()->getRelativeWidth();
			pVector4->y = pVP->getRealHeight() * pVP->getTarget()->getRelativeHeight();
			pVector4->z = 0.5f / (float)pVP->getActualWidth();
			pVector4->w = 0.5f / (float)pVP->getActualHeight();
		}
		break;

		case U_VS_RenderWindowParam:
		{
			pVector4->x = (float)renderSystem->getCurRenderStrategy()->getRenderWindowWidth();
			pVector4->y = (float)renderSystem->getCurRenderStrategy()->getRenderWindowHeight();
			pVector4->z = 0.5f / (float)renderSystem->getCurRenderStrategy()->getRenderWindowWidth();
			pVector4->w = 0.5f / (float)renderSystem->getCurRenderStrategy()->getRenderWindowHeight();
		}
		break;

		case U_DepthRTParam:
		{
			uint32 width = 1, height = 1;

			const ECHO_DEVICE_CLASS device_class = Root::instance()->getDeviceType();
			DepthBuffer* pDepthBuffer = nullptr;
			if (ECHO_DEVICE_GLES3 == device_class ||
				//ECHO_DEVICE_METAL == device_class||
				ECHO_DEVICE_DX11 == device_class ||
				ECHO_DEVICE_VULKAN == device_class)
			{
				pDepthBuffer = renderSystem->getCurRenderStrategy()->getDepthBuffer();
			}
			else
			{
				pDepthBuffer = renderSystem->getCurrentRTDepthBuffer();
			}


			if (nullptr != pDepthBuffer)
				pDepthBuffer->getWidthAndHeight(width, height);

			pVector4->x = (float)width;
			pVector4->y = (float)height;
			pVector4->z = 1.0f / (float)width;
			pVector4->w = 1.0f / (float)height;
		}
		break;

		case U_ForwardSSREnable:
		{
			*pVector4 = m_sceneManager->isForwardSSREnable() ? Vector4(1.0f) : Vector4(-1.0f);
		}
		break;

		case U_PS_ViewLightPercent:
		{
			pVector4->x = Echo::Clamp(0.f, 1.f, m_sceneManager->getPBRViewLightPercent());
			pVector4->y = Echo::Clamp(0.f, 1.f, m_sceneManager->getRoleViewLightPercent());
			pVector4->z = 0.f;
			pVector4->w = 0.f;
		}
		break;

		case U_ViewDirection:
		case U_PS_ViewDirection:
		{
			const Camera* camera = m_sceneManager->getActiveCamera();
			*pVector4 = camera->getDerivedDirection();
		}
		break;

		case U_ViewRightVector:
		{
			const Camera* camera = m_sceneManager->getActiveCamera();
			*pVector4 = camera->getDerivedRight();
		}
		break;

		case U_ViewUpVector:
		{
			const Camera* camera = m_sceneManager->getActiveCamera();
			*pVector4 = camera->getDerivedUp();
		}
		break;

		case U_MainLightDirection:
		case U_VS_MainLightDirection:
		{
			if (m_hasCustomMainLightParam)
			{
				pVector4->x = m_CustomMainLightDir.x;
				pVector4->y = m_CustomMainLightDir.y;
				pVector4->z = m_CustomMainLightDir.z;
			}
			else
			{
				const Vector3& mainLightDir = m_sceneManager->getMainLightDirection();
				pVector4->x = mainLightDir.x;
				pVector4->y = mainLightDir.y;
				pVector4->z = mainLightDir.z;
			}
			pVector4->w = 1.0f;
		}
		break;

		case U_MainLightColor:
		case U_VS_MainLightColor:
		{
			if (m_hasCustomMainLightParam)
			{
				pVector4->x = m_CustomMainLightColor.r;
				pVector4->y = m_CustomMainLightColor.g;
				pVector4->z = m_CustomMainLightColor.b;
			}
			else
			{
				const ColorValue& colorvalue = m_sceneManager->getMainLightColor();
				pVector4->x = colorvalue.r;
				pVector4->y = colorvalue.g;
				pVector4->z = colorvalue.b;
			}

			pVector4->w = m_sceneManager->getGlobalSpecularRatio();/*colorvalue.a;*/
		}
		break;

		case U_AuxiliaryLightDirection:
		{
			const Vector3& auxiliaryLightDir = m_sceneManager->getAuxiliaryLightDirection();
			pVector4->x = auxiliaryLightDir.x;
			pVector4->y = auxiliaryLightDir.y;
			pVector4->z = auxiliaryLightDir.z;
			pVector4->w = 1.0f;
		}
		break;

		case U_AuxiliaryLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getAuxiliaryLightColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_SkyLightColor:
		case U_VS_SkyLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getSkyLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_GroundLightColor:
		case U_VS_GroundLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getGroundLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_PBR_MainLightColor:
		case U_VS_PBR_MainLightColor:
		{
			if (m_hasCustomMainLightParam)
			{
				pVector4->x = m_CustomMainLightColor.r;
				pVector4->y = m_CustomMainLightColor.g;
				pVector4->z = m_CustomMainLightColor.b;
			}
			else
			{
				const ColorValue& colorvalue = m_sceneManager->getPBRMainLightColor();
				pVector4->x = colorvalue.r;
				pVector4->y = colorvalue.g;
				pVector4->z = colorvalue.b;
			}

			//pVector4->w = colorvalue.a;
			pVector4->w = m_sceneManager->getGlobalSpecularRatio();
		}
		break;

		case U_PBR_SkyLightColor:
		case U_VS_PBR_SkyLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getPBRSkyLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_PBR_GroundLightColor:
		case U_VS_PBR_GroundLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getPBRGroundLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_PBR_RMatrix:
		{
			const Matrix4& mat = m_sceneManager->getEnvironmentLight()->getRMatrix();
			*pMatrix4 = mat;
		}
		break;
		case U_PBR_GMatrix:
		{
			const Matrix4& mat = m_sceneManager->getEnvironmentLight()->getGMatrix();
			*pMatrix4 = mat;
		}
		break;
		case U_PBR_BMatrix:
		{
			const Matrix4& mat = m_sceneManager->getEnvironmentLight()->getBMatrix();
			*pMatrix4 = mat;
		}
		break;
		case U_PBR_Environment_Explosure:
		{
			float value = m_sceneManager->getEnvironmentLight()->getEnvironmentExplosure();
			pVector4->x = value;
			pVector4->y = value;
			pVector4->z = value;
			pVector4->w = value;
		}
		break;
		case U_VS_AmbientBlendThreshold:
		case U_PS_AmbientBlendThreshold:
		{
			//float mainAmbientBlend = m_sceneManager->getAmbientBlendThreshold(); //not using for now
			float pbrAmbientBlend = m_sceneManager->getPBRAmbientBlendThreshold();
			float roleAmbientBlend = m_sceneManager->getRoleAmbientBlendThreshold();


			pVector4->y = pbrAmbientBlend;
			pVector4->z = roleAmbientBlend;
		}
		break;
		//case U_PBR_Environment_Brightness:
		//{
		//	float value = m_sceneManager->getEnvironmentMapBrightness();
		//	pVector4->x = value;
		//	pVector4->y = value;
		//	pVector4->z = value;
		//	pVector4->w = 1;
		//}
		//break;
		//case U_PBR_Environment_MaxIlluminance:
		//{
		//	float value = m_sceneManager->getEnvironmentMapMaxIlluminance();
		//	pVector4->x = value;
		//	pVector4->y = value;
		//	pVector4->z = value;
		//	pVector4->w = 1;
		//}
		//break;

		case U_RoleMainLightColor:
		{
			if (m_hasCustomMainLightParam)
			{
				pVector4->x = m_CustomMainLightColor.r;
				pVector4->y = m_CustomMainLightColor.g;
				pVector4->z = m_CustomMainLightColor.b;
			}
			else
			{
				const ColorValue& colorvalue = m_sceneManager->getRoleMainLightColor();
				pVector4->x = colorvalue.r;
				pVector4->y = colorvalue.g;
				pVector4->z = colorvalue.b;
			}

			//pVector4->w = colorvalue.a;
			pVector4->w = m_sceneManager->getGlobalSpecularRatio();
		}
		break;

		case U_RoleSkyLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getRoleSkyLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_RoleGroundLightColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getRoleGroundLightColour();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_SunColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getSunColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_FogColorNear:
		{
			const ColorValue& colorvalue = m_sceneManager->getFogColourNear();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_FogColorFar:
		{
			const ColorValue& colorvalue = m_sceneManager->getFogColourFar();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_FogParam:
		{
			*pVector4 = m_sceneManager->getFogParam();
		}
		break;
		case U_FogRampParam:
		{
			*pVector4 = m_sceneManager->getFogRampParam();
		}
		break;
		case U_SphFogLowerLayerColor:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					const ColorValue& colorValue = sphFog->GetLowerLayerColor();
					pVector4->x = colorValue.r;
					pVector4->y = colorValue.g;
					pVector4->z = colorValue.b;
					pVector4->w = colorValue.a;
				}
			}
		}
		break;
		case U_SphFogUpperLayerColor:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					const ColorValue& colorValue = sphFog->GetUpperLayerColor();
					pVector4->x = colorValue.r;
					pVector4->y = colorValue.g;
					pVector4->z = colorValue.b;
					pVector4->w = colorValue.a;
				}
			}
		}
		break;
		case U_SphFogEarthCenter:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					const Vector3& earthCenter = sphFog->GetEarthCenter();
					pVector4->x = earthCenter.x;
					pVector4->y = earthCenter.y;
					pVector4->z = earthCenter.z;
				}
			}
		}
		break;
		case U_SphFogBlendParam:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					const Vector4& blendParam = sphFog->GetBlendParam();
					Vector2 fogRampParams;
					fogRampParams.x = 1.f / (blendParam.y - blendParam.x);
					fogRampParams.y = blendParam.x * fogRampParams.x;
					pVector4->x = fogRampParams.x;
					pVector4->y = fogRampParams.y;
					pVector4->z = blendParam.z;
					pVector4->w = blendParam.w;
				}
			}
		}
		break;
		case U_SphFogParam0:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					*pVector4 = sphFog->GetFogParam0();
				}
			}
		}
		break;
		case U_SphFogParam1:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					*pVector4 = sphFog->GetFogParam1();
				}
			}
		}
		break;
		case U_SphFogSkyParam:
		{
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(pc);
					*pVector4 = sphFog->GetFogSkyParam();
				}
			}
		}
		break;
		case U_WaterShallowColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getWaterShallowColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_WaterDeepColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getWaterDeepColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_WaterFoamColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getWaterFoamColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_WaterFoamParams:
		{
			*pVector4 = m_sceneManager->getWaterFoamParam();
		}
		break;
		case U_WaterReflectColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getWaterReflectColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_ShaderDebugFlag:
		{
			pVector4->x = static_cast<float>(m_ShaderDebugFlag);
		}
		break;

		case U_UnderWaterColor:
		{
			const ColorValue& colorvalue = m_sceneManager->getUnderWaterColor();
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;
		case U_EnvironmentCubeMapMipLevels:
		{
			const TexturePtr& tex = m_sceneManager->getGlobalReflectTextrue();
			if (!tex.isNull())
			{
				pVector4->x = (float)tex->getMipLevels();
			}
			pVector4->y = m_sceneManager->getGlobalReflectionRatio(); //全局反射系数
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
		}
		break;
		case U_ReflectCubeMapMipLevels:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				TexturePtr tex;
				if (material.getV1())
				{
					tex = material.getV1()->m_textureList[MAT_TEXTURE_REFLECTCUBE];
				}
				else
				{
					tex = material.getV2()->getTexture(Mat::TEXTURE_BIND_REFLECTION_CUBE);
				}
				if (!tex.isNull())
				{
					pVector4->x = (float)tex->getMipLevels();
				}
			}
			pVector4->y = m_sceneManager->getGlobalReflectionRatio(); //全局反射系数
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
		}
		break;
		case U_GlobalEnvironmentCubeMipMapLevels:
		{
			const TexturePtr& tex = m_sceneManager->getEnvironmentLight()->getEnvironmentLightTex();
			if (!tex.isNull())
			{
				pVector4->x = (float)tex->getMipLevels();
			}
		}
		break;
		case U_MaterialDiffuse:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					const ColorValue& color = material.getV1()->getDiffuseColor();

					pVector4->x = color.r;
					pVector4->y = color.g;
					pVector4->z = color.b;
					pVector4->w = color.a;
				}
				else if (material.isV2())
				{
					const ColorValue& color = material.getV2()->getAttributes().diffuseColor;

					pVector4->x = color.r;
					pVector4->y = color.g;
					pVector4->z = color.b;
					pVector4->w = color.a;
				}
			}

			updateMaterialParam(uniformNameID, pVector4);

			/*

						if (m_bUseCostomDiffuseColor)
						{
							pVector4->x = m_customDiffuseColor.r;
							pVector4->y = m_customDiffuseColor.g;
							pVector4->z = m_customDiffuseColor.b;
						}
						else
						{
							pVector4->x = color.r;
							pVector4->y = color.g;
							pVector4->z = color.b;
						}


						if (m_customAlpha < 0.0f)
						{
							pVector4->w = color.a;
						}
						else
						{
							pVector4->w = m_customAlpha;
						}*/
		}
		break;

		case U_MaterialSpecular:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					const ColorValue& color = material.getV1()->getSpecularColor();
					pVector4->x = color.r;
					pVector4->y = color.g;
					pVector4->z = color.b;
				}
				else if (material.isV2())
				{
					const ColorValue& color = material.getV2()->getAttributes().specularColor;
					pVector4->x = color.r;
					pVector4->y = color.g;
					pVector4->z = color.b;
				}

				float f = 0.0f;

				if (material.isV1())
				{
					Material* matV1 = material.getV1();
					if (0.0f != matV1->getFrequance())
					{
						f = Root::instance()->getTimeElapsed();
						f = std::sin(matV1->getFrequance() * f + matV1->getBaseOffset());
						f = (f + 1.0f) * 0.5f;
					}
				}
				else if (material.isV2())
				{
					const Mat::Attributes& attrs = material.getV2()->getAttributes();
					if (0.0f != attrs.emissiveFrequency)
					{
						f = Root::instance()->getTimeElapsed();
						f = std::sin(attrs.emissiveFrequency * f + attrs.emissiveBaseOffset);
						f = (f + 1.0f) * 0.5f;
					}
				}

				pVector4->w = std::max(f, 0.0f);
			}
		}
		break;

		case U_MaterialFresnel:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					const Vector4& fresnel = material.getV1()->getFresnel();
					pVector4->z = fresnel.z;
					pVector4->x = 1.0f;
					pVector4->w = fresnel.w;
					pVector4->y = fresnel.y;
				}
				else if (material.isV2())
				{
					const Vector4& fresnel = material.getV2()->getAttributes().fresnel;
					pVector4->z = fresnel.z;
					pVector4->x = 1.0f;
					pVector4->w = fresnel.w;
					pVector4->y = fresnel.y;
				}
			}

			updateMaterialParam(uniformNameID, pVector4);

			/*
						if (m_customSpecture < 0.0f)
						{
							pVector4->w = fresnel.w;
						}
						else
						{
							pVector4->w = m_customSpecture;
						}
						if (m_customOpacity < 0.0f)
						{
							pVector4->x = 1.0f;
						}
						else
						{
							pVector4->x = 1.0f - m_customOpacity;
						}
						if (m_customDissovleStrength < 0.0f)
						{
							pVector4->y = fresnel.y;
						}
						else
						{
							pVector4->y = m_customDissovleStrength;
						}*/
		}
		break;

		case U_MaterialEmission:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					Material* matV1 = material.getV1();
					pVector4->x = matV1->getEmissionColor().r;
					pVector4->y = matV1->getEmissionColor().g;
					pVector4->z = matV1->getEmissionColor().b;
					pVector4->w = matV1->getEmissionColor().a;
				}
				else if (material.isV2())
				{
					const ColorValue& emissionColor = material.getV2()->getAttributes().emissionColor;
					pVector4->x = emissionColor.r;
					pVector4->y = emissionColor.g;
					pVector4->z = emissionColor.b;
					pVector4->w = emissionColor.a;
				}
			}

		}
		break;
		case U_MaterialCustomParam:
		{
			*pVector4 = Vector4::ZERO;
			updateMaterialParam(uniformNameID, pVector4);
		}
		break;
		case U_MaterialCustomParam1:
		{
			*pVector4 = Vector4::ZERO;
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					pVector4->x = material.getV1()->getHdrParam();
				}
				else if (material.isV2())
				{
					pVector4->x = material.getV2()->getAttributes().hdrParam;
				}
			}

			PostProcessChain* pPostProcessChain = renderSystem->getCurRenderStrategy()->getPostProcessChain();
			if (pPostProcessChain && !pPostProcessChain->isEnable(PostProcess::Type::HDR))
				pVector4->x = 1.0f;
			pVector4->z = 1.0f;
			updateMaterialParam(uniformNameID, pVector4);
			//pVector4->y = m_customStippleTransparency;
			//pVector4->z = m_customStippleSize;
		}
		break;

		case U_GlobalBuildingColor:
		{
			pVector4->x = m_sceneManager->getGlobalBuildCorrectionColor().r;
			pVector4->y = m_sceneManager->getGlobalBuildCorrectionColor().g;
			pVector4->z = m_sceneManager->getGlobalBuildCorrectionColor().b;
			pVector4->w = m_sceneManager->getGlobalBuildCorrectionColor().a;
		}
		break;

		case U_ShadowWeight:
		{
			pVector4->x = m_sceneManager->getShadowWeight();
			pVector4->y = m_sceneManager->getShadowProp();
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
		}
		break;
		case U_LightCount:
		{
			pVector4->x = m_sceneManager->getLightCount();
			pVector4->y = 0.0f;
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
		}
		break;
		case U_LightPower:
		{
			pVector4->x = m_sceneManager->getSeltEmissiveLightPower();//x for light power
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
			pVector4->y = 0.0f;
			updateMaterialParam(uniformNameID, pVector4);
			//if (m_customSpecture > 0.0f)
			//{
			//	pVector4->y = 1.0f;
			//}
			//else
			//{
			//	pVector4->y = 0.0f;
			//}

		}
		break;
		case U_LightWorldPosiArray:
		{
			Vector3 lightPosi;
			Vector4 lightAttenuation;
			for (size_t i = 0; i < maxPointLightNum; i++)
			{
				lightPosi = m_sceneManager->getLightPosition(i);
				lightAttenuation = m_sceneManager->getLightAttenuation(i);
				pVector4->x = lightPosi.x;
				pVector4->y = lightPosi.y;
				pVector4->z = lightPosi.z;
				pVector4->w = lightAttenuation.x;
				++pVector4;
			}
		}
		break;
		case U_LightAttenuationArray:
		{
			Vector4 lightAttenuation;
			for (size_t i = 0; i < maxPointLightNum; i++)
			{
				lightAttenuation = m_sceneManager->getLightAttenuation(i);
				pVector4->x = lightAttenuation.x;
				pVector4->y = lightAttenuation.y;
				pVector4->z = lightAttenuation.z;
				pVector4->w = lightAttenuation.w;
				++pVector4;
			}
		}
		break;
		case U_LightDiffuseColorArray:
		{
			ColorValue lightDiffuse;
			Vector4 lightAttenuation;
			for (size_t i = 0; i < maxPointLightNum; i++)
			{
				lightDiffuse = m_sceneManager->getLightDiffuseColor(i);
				lightAttenuation = m_sceneManager->getLightAttenuation(i);
				pVector4->x = lightDiffuse.r;
				pVector4->y = lightDiffuse.g;
				pVector4->z = lightDiffuse.b;
				pVector4->w = lightAttenuation.z;
				++pVector4;
			}
		}
		break;
		// TODO(yanghang): Cluste light test.
		case U_ClusterLightParam: // ScreenSize clusterBlockSize
		{
			float* clusterLightParam = (float*)pBuffer;
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			// TODO(yanghang): GPU Optimize, eliminate divide.
			*clusterLightParam++ = 1.0f / cluster_light->m_clusterBlockSizeX;
			*clusterLightParam++ = 1.0f / cluster_light->m_clusterBlockSizeY;
			*clusterLightParam++ = 1.0f / cluster_light->m_clusterBlockCountX;
			*clusterLightParam = 1.0f / cluster_light->m_clusterBlockCountY;
		}break;
		case U_ClusterLightID:// NOTE(yanghang): Use it for current light count.
		{
			uint32* clusterLightParam = (uint32*)pBuffer;
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			assert(cluster_light->m_currentLightCount <= 32 && "out of maximum light count!");
			*clusterLightParam = cluster_light->m_currentLightCount;
		}break;
		case U_ClusterLightWorldPosArray:
		{
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			Vector4* src = cluster_light->m_lightsInfo.posAndInnerRadius;
			int maxLightCount = cluster_light->m_maxLightCount;
			memcpy(pVector4, src, sizeof(Vector4) * maxLightCount);
		}break;
		case U_ClusterLightDiffuseColorArray:
		{
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			Vector4* src = cluster_light->m_lightsInfo.colorAndOuterRadius;
			int maxLightCount = cluster_light->m_maxLightCount;
			memcpy(pVector4, src, sizeof(Vector4) * maxLightCount);
		}break;
		case U_ClusterLightDirectionOuterAngleArray:
		{
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			Vector4* src = cluster_light->m_lightsInfo.directionAndCosOuterAngle;
			int maxLightCount = cluster_light->m_maxLightCount;
			memcpy(pVector4, src, sizeof(Vector4) * maxLightCount);
		}break;
		case U_ClusterLightInnerAngleArray:
		{
			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			Vector4* src = cluster_light->m_lightsInfo.cosInnerAngleAndOther;
			int maxLightCount = cluster_light->m_maxLightCount;
			memcpy(pVector4, src, sizeof(Vector4) * maxLightCount);
		}break;

		case U_VS_Time:
		case U_PS_Time:
		{
			pVector4->x = Root::instance()->getTimeElapsed();

			double time = Root::instance()->getTimeElapsed() * 0.5f;
			float factor = (float)modf(time, &time);
			pVector4->y = factor * Math::TWO_PI;

			pVector4->z = pVector4->x * 0.1f;
			pVector4->w = Root::instance()->overDrawValue;
		}
		break;
		case U_WindParams:
		case U_VS_WindParams:
		case U_PS_WindParams:
		{
			*pVector4 = m_sceneManager->getWindParams();
		}
		break;
		case U_WindParamsGravitySpace:
		{
			*pVector4 = m_sceneManager->getWindParams();
			//	风向根据场景重力方向做旋转
			if (m_sceneManager->getIsUseGravityDir())
			{
				Quaternion qua;
				qua.FromAxesToAxes(Vector3::UNIT_Y, m_sceneManager->getGravityDir());
				Vector3 windGravityDir = Vector3(pVector4->x, pVector4->y, pVector4->z) * qua;
				*pVector4 = Vector4(windGravityDir.x, windGravityDir.y, windGravityDir.z, 0.f);
			}
		}
		break;
		case U_WindParamsObjSpace:
		{
			Matrix4 tempMatrix4 = *m_pWorldMatrix;
			//if (m_bUseLocalMatrix)
			//{
			//	tempMatrix4 = tempMatrix4.concatenate(m_localMatrix);
			//}

			Matrix4 invWorldMatrix = (tempMatrix4).inverseAffine();
			Vector4 windParam = m_sceneManager->getWindParams();
			float fPower = windParam.w;
			windParam.w = 0.f;
			*pVector4 = invWorldMatrix * windParam;
			Echo::Vector3 vDir(pVector4->ptr());
			vDir.normalise();
			pVector4->x = vDir.x;
			pVector4->y = vDir.y;
			pVector4->z = vDir.z;
			pVector4->w = fPower;
		}
		break;
		case U_AnimationParams:
		{
			const Vector4& globalAni = m_sceneManager->getGlobalTreeAnimationParams();
			Vector4 materialAni;
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					materialAni = material.getV1()->getAnimationParams();
				}
				else if (material.isV2())
				{
					materialAni = material.getV2()->getAttributes().animationParams;
				}
			}
			updateMaterialParam(uniformNameID, &materialAni);


			//if (m_customAnimationParams != Vector4::ZERO)
			//{
			//	materialAni = m_customAnimationParams;
			//}
			pVector4->x = materialAni.x * globalAni.x;
			pVector4->y = materialAni.y * globalAni.y;
			pVector4->z = materialAni.z * globalAni.z;
			pVector4->w = materialAni.w * globalAni.w;
		}
		break;
		case U_GlobalAnimationParams:
		{
			*pVector4 = m_sceneManager->getGlobalVegetationAnimationParams();
		}
		break;
		case U_VS_FPS:
		{
			pVector4->x = (float)Root::instance()->GetFps();
			pVector4->y = Root::instance()->m_fExposureTime;
			pVector4->z = 0;
			pVector4->w = 0;
		}
		break;

		case U_LightVPMatrixView0:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_0);
			if (pCam)
			{
				DMatrix4 viewMatrix = pCam->getViewMatrix();
				Camera* pMainCamera = m_sceneManager->getMainCamera();

				const DVector3& cPos = pMainCamera->getPosition();
				DVector3 lightCamPos = pCam->getDerivedPosition();
				DVector3 lightRelativePos = lightCamPos - cPos;

				// 1. 获取双精度 View 矩阵并修正平移
				viewMatrix[0][3] = -(viewMatrix[0][0] * lightRelativePos.x + viewMatrix[0][1] * lightRelativePos.y + viewMatrix[0][2] * lightRelativePos.z);
				viewMatrix[1][3] = -(viewMatrix[1][0] * lightRelativePos.x + viewMatrix[1][1] * lightRelativePos.y + viewMatrix[1][2] * lightRelativePos.z);
				viewMatrix[2][3] = -(viewMatrix[2][0] * lightRelativePos.x + viewMatrix[2][1] * lightRelativePos.y + viewMatrix[2][2] * lightRelativePos.z);



				/*viewMatrix.m[0][3] = viewMatrix.m[0][3] - cPos.x;
				viewMatrix.m[1][3] = viewMatrix.m[1][3] - cPos.y;
				viewMatrix.m[2][3] = viewMatrix.m[2][3] - cPos.z;*/
				DMatrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
		}
		break;
		case U_LightVPMatrixView1:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_1);
			if (pCam)
			{
				DMatrix4 viewMatrix = pCam->getViewMatrix();
				Camera* pMainCamera = m_sceneManager->getMainCamera();

				const DVector3& cPos = pMainCamera->getPosition();
				DVector3 lightCamPos = pCam->getDerivedPosition();
				DVector3 lightRelativePos = lightCamPos - cPos;

				// 1. 获取双精度 View 矩阵并修正平移
				viewMatrix[0][3] = -(viewMatrix[0][0] * lightRelativePos.x + viewMatrix[0][1] * lightRelativePos.y + viewMatrix[0][2] * lightRelativePos.z);
				viewMatrix[1][3] = -(viewMatrix[1][0] * lightRelativePos.x + viewMatrix[1][1] * lightRelativePos.y + viewMatrix[1][2] * lightRelativePos.z);
				viewMatrix[2][3] = -(viewMatrix[2][0] * lightRelativePos.x + viewMatrix[2][1] * lightRelativePos.y + viewMatrix[2][2] * lightRelativePos.z);



				/*viewMatrix.m[0][3] = viewMatrix.m[0][3] - cPos.x;
				viewMatrix.m[1][3] = viewMatrix.m[1][3] - cPos.y;
				viewMatrix.m[2][3] = viewMatrix.m[2][3] - cPos.z;*/
				DMatrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
		}
		break;
		case U_LightVPMatrixView2:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_2);
			if (pCam)
			{
				DMatrix4 viewMatrix = pCam->getViewMatrix();
				Camera* pMainCamera = m_sceneManager->getMainCamera();

				const DVector3& cPos = pMainCamera->getPosition();
				DVector3 lightCamPos = pCam->getDerivedPosition();
				DVector3 lightRelativePos = lightCamPos - cPos;

				// 1. 获取双精度 View 矩阵并修正平移
				viewMatrix[0][3] = -(viewMatrix[0][0] * lightRelativePos.x + viewMatrix[0][1] * lightRelativePos.y + viewMatrix[0][2] * lightRelativePos.z);
				viewMatrix[1][3] = -(viewMatrix[1][0] * lightRelativePos.x + viewMatrix[1][1] * lightRelativePos.y + viewMatrix[1][2] * lightRelativePos.z);
				viewMatrix[2][3] = -(viewMatrix[2][0] * lightRelativePos.x + viewMatrix[2][1] * lightRelativePos.y + viewMatrix[2][2] * lightRelativePos.z);



				/*viewMatrix.m[0][3] = viewMatrix.m[0][3] - cPos.x;
				viewMatrix.m[1][3] = viewMatrix.m[1][3] - cPos.y;
				viewMatrix.m[2][3] = viewMatrix.m[2][3] - cPos.z;*/
				DMatrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
		}
		break;
		case U_LightVPMatrixView3:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_3);
			if (pCam)
			{
				DMatrix4 viewMatrix = pCam->getViewMatrix();
				Camera* pMainCamera = m_sceneManager->getMainCamera();

				const DVector3& cPos = pMainCamera->getPosition();
				DVector3 lightCamPos = pCam->getDerivedPosition();
				DVector3 lightRelativePos = lightCamPos - cPos;

				// 1. 获取双精度 View 矩阵并修正平移
				viewMatrix[0][3] = -(viewMatrix[0][0] * lightRelativePos.x + viewMatrix[0][1] * lightRelativePos.y + viewMatrix[0][2] * lightRelativePos.z);
				viewMatrix[1][3] = -(viewMatrix[1][0] * lightRelativePos.x + viewMatrix[1][1] * lightRelativePos.y + viewMatrix[1][2] * lightRelativePos.z);
				viewMatrix[2][3] = -(viewMatrix[2][0] * lightRelativePos.x + viewMatrix[2][1] * lightRelativePos.y + viewMatrix[2][2] * lightRelativePos.z);



				/*viewMatrix.m[0][3] = viewMatrix.m[0][3] - cPos.x;
				viewMatrix.m[1][3] = viewMatrix.m[1][3] - cPos.y;
				viewMatrix.m[2][3] = viewMatrix.m[2][3] - cPos.z;*/
				DMatrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
		}
		break;
		case U_LightVPMatrix0:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_0);
			if (pCam)
			{
				const DMatrix4& viewMatrix = pCam->getViewMatrix();

				Matrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
			else
			{
				*pMatrix4 = Matrix4::IDENTITY;
			}
		}
		break;
		case U_RainVPMatrix:
		{
			Camera* pCam = m_sceneManager->getRainMaskManager()->GetRainShadowCamera();
			if (pCam)
			{
				const DMatrix4& viewMatrix = pCam->getViewMatrix();

				Matrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
			else
			{
				*pMatrix4 = Matrix4::IDENTITY;
			}
		}
		break;
		case U_LightVPMatrix1:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_1);
			if (pCam)
			{
				const DMatrix4& viewMatrix = pCam->getViewMatrix();

				Matrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
			else
			{
				*pMatrix4 = Matrix4::IDENTITY;
			}
		}
		break;
		case U_LightVPMatrix2:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_2);
			if (pCam)
			{
				const DMatrix4& viewMatrix = pCam->getViewMatrix();

				Matrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
			else
			{
				*pMatrix4 = Matrix4::IDENTITY;
			}
		}
		break;
		case U_LightVPMatrix3:
		{
			Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_3);
			if (pCam)
			{
				const DMatrix4& viewMatrix = pCam->getViewMatrix();

				Matrix4 projectMatrix;
				if (Root::instance()->getEnableReverseDepth())
				{
					projectMatrix = pCam->getReverseDepthProjectionMatrixRSLeftHand();
				}
				else
				{
					projectMatrix = pCam->getProjectionMatrixWithRSDepth();
				}

				*pMatrix4 = projectMatrix * viewMatrix;
			}
			else
			{
				*pMatrix4 = Matrix4::IDENTITY;
			}
		}
		break;
		case U_ShadowSplit:
		case U_VS_ShadowSplit:
		{
			*pVector4 = Vector4::ZERO;
			m_sceneManager->getPsmshadowManager()->getSplitedistance((float*)pVector4);
		}
		break;
		case U_ShadowTexOffset:
		{
			float texoffset[MAXSPLITNUM];
			m_sceneManager->getPsmshadowManager()->getSplitedTexoffset(texoffset);

			(*pVector4).x = texoffset[0];
			(*pVector4).y = texoffset[1];
			(*pVector4).z = texoffset[2];
			(*pVector4).w = m_sceneManager->getShadowWeight();//m_sceneManager->getPsmshadowManager()->getShadowStrenth();

		}
		break;
		case U_ShadowEdgeFactor:
		{
			*pVector4 = Vector4::ZERO;
			pVector4->x = m_sceneManager->getPsmshadowManager()->getShadowEdgeFactor();
		}
		break;
		case U_ShadowBiasMul:
		{
			*pVector4 = Vector4::ZERO;
			for (int i = 0; i < 4; ++i)
			{
				Camera* pCam = m_sceneManager->getShadowCamera(ShadowLevel_0 + i);
				if (!pCam) continue;

				const auto& proj = pCam->getProjectionMatrix();
				auto bias = 1.0f / (static_cast<float>(m_sceneManager->getShadowMapSize(i)) * proj[0][0]);
				(*pVector4)[i] = bias;
			}
		}
		break;

		case U_ShadowLightViewMatrixArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Matrix4* val = m_sceneManager->getPointShadowManager()->GetViewArray();
			std::copy_n(val, probeCount, pMatrix4);
		}
		break;

		case U_ShadowLightWorldPosArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Vector4* val = m_sceneManager->getPointShadowManager()->GetWorldPosArray();
			std::copy_n(val, probeCount, pVector4);
		}
		break;

		case U_ShadowLightDiffuseColorArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Vector4* val = m_sceneManager->getPointShadowManager()->GetDiffuseColorArray();
			std::copy_n(val, probeCount, pVector4);
		}
		break;

		case U_ShadowLightDirectionOuterAngleArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Vector4* val = m_sceneManager->getPointShadowManager()->GetDirectionOuterAngleArray();
			std::copy_n(val, probeCount, pVector4);
		}
		break;

		case U_ShadowLightInnerAngleArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Vector4* val = m_sceneManager->getPointShadowManager()->GetInnerAngleArray();
			std::copy_n(val, probeCount, pVector4);
		}
		break;

		case U_ShadowLightMappingArray:
		{
			const int probeCount = PointLightShadowManager::GetProbeCount();
			const Vector4* val = m_sceneManager->getPointShadowManager()->GetMappingArray();
			std::copy_n(val, probeCount, pVector4);
		}
		break;

		case U_ShadowLightDepthMulAdd:
		{
			const auto cam = m_sceneManager->getActiveCamera();
			const float nearClip = cam->getNearClipDistance();
			const float farClip = cam->getFarClipDistance();
			const float cosOuterAngle = cam->getCosConeAngle();
			float depthAdd = -nearClip / (farClip - nearClip);
			//farClip / (farClip - nearClip);
			float depthMul = 1.0f / (farClip - nearClip);						// [0 near, 1 far]
			//depthAdd * -nearClip;
			if (Root::instance()->getEnableReverseDepth())						// [1 near, 0 far]
			{
				depthAdd = 1.0f - depthAdd;
				depthMul = -depthMul;
			}
			else if (Root::instance()->getDeviceType() == ECHO_DEVICE_GLES3)	// [-1 near, 1 far]
			{
				depthAdd = 2.0f * depthAdd - 1.0f;
				depthMul = 2.0f * depthMul;
			}
			const float sinOuterAngle = Math::Sqrt(1.0f - Math::Sqr(cosOuterAngle));
			const float xyMul = (1.0f + cosOuterAngle) / sinOuterAngle;

			*pVector4 = Vector4(xyMul, farClip, depthMul, depthAdd);
		}
		break;

		case U_WeatherParam0:
		{
			const ColorValue& colorvalue = m_sceneManager->getWeatherParamColor();//WeatherManager::instance()->getWeatherParams().mSnowColor;
			pVector4->x = colorvalue.r;
			pVector4->y = colorvalue.g;
			pVector4->z = colorvalue.b;
			pVector4->w = colorvalue.a;
		}
		break;

		case U_WeatherParam1:
		{
			const Vector4& params = m_sceneManager->getWeatherParam2();//WeatherManager::instance()->getWeatherParams();

			pVector4->x = params.x;
			pVector4->y = params.y;
			pVector4->z = 0.0f;
			pVector4->w = 0.0f;
		}
		break;
		case U_WeatherParam2:
		{
			const Vector4& params = m_sceneManager->getRainTestVec();//WeatherManager::instance()->getWeatherParams();

			pVector4->x = params.x;
			pVector4->y = params.y;
			pVector4->z = params.z;
			pVector4->w = params.w;
		}
		break;
		case U_VS_CloudCollision:
		case U_PS_CloudCollision:
		{
			*pVector4 = m_sceneManager->getCloudCollision();
		}
		break;
		case U_CoverColorParams:
		{
			*pVector4 = m_sceneManager->getWeatherCoverParams();
		}
		break;
		case U_CoverColorRotation:
		{
			*pVector4 = m_sceneManager->getWeatherCoverRotation();
		}
		break;
		case U_VS_GravityDirection:
		case U_PS_GravityDirection:
		{
			*pVector4 = m_sceneManager->getGravityDir();

		}
		break;

		case U_VS_GravityRotationMatrix:
		case U_PS_GravityRotationMatrix:
		{
			Matrix4 tempMatrix4 = Matrix4::IDENTITY;
			if (m_sceneManager->getIsUseGravityDirByType(Use_State_Fog))//temp only use for Fog
			{
				Vector3 gravityDirection = m_sceneManager->getGravityDir();	//get current gravity direction

				Quaternion qu;
				qu = Quaternion::FromToRotation(gravityDirection, Vector3::UNIT_Y);	//calculate rotation use default gravity direction-Vector3(0,1,0)

				tempMatrix4.makeTransform(Vector3::ZERO, Vector3::UNIT_SCALE, qu);	// only rotation
			}

			*pMatrix4 = tempMatrix4;
		}
		break;

		case U_GlobalAgingColour:
		{
			const ColorValue& params = m_sceneManager->getGlobalAgingColour();

			pVector4->x = params.r;
			pVector4->y = params.g;
			pVector4->z = params.b;
			pVector4->w = params.a;
		}
		break;

		case U_MainCamViewMatrix:
		{
			Camera* pCam = m_sceneManager->getActiveCamera();
			if (NULL == pCam)
			{
				pCam = m_sceneManager->getMainCamera();
			}
			Matrix4 viewMat = pCam->getViewMatrix();

			float* vec = viewMat[2];
			*pVector4 = Vector4(vec[0], vec[1], vec[2], vec[3]);
		}
		break;


		case U_VSGeneralRegister0:
		case U_VSGeneralRegister1:
		case U_VSGeneralRegister2:
		case U_VSGeneralRegister3:
		case U_VSGeneralRegister4:
		case U_VSGeneralRegister5:
		case U_VSGeneralRegister6:
		case U_VSGeneralRegister7:
		{
			uint32 index = uniformNameID - U_VSGeneralRegister0;

			if (index < MAX_VS_UNIFORM_COUNT)
			{
				updateVsUniformParam(index, pVector4);
				//*pVector4 = m_vsCustomUniform[index];
			}
			else
				*pVector4 = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

			//const SHADER_PARAMS& vsCustomParams = m_material->m_matInfo.vsShaderParams;
			//*pVector4 = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			//for (size_t i = 0, count = vsCustomParams.size(); i < count; ++i)
			//{
			//	const st_UniformParam& uniform = vsCustomParams[i];

			//	if (index == uniform.uniformIndex)
			//	{
			//		pGeneralRegister[uniform.paramIndex] = uniform.paramVal;
			//	}

			//	//if (uniform.uniformIndex > index)
			//	//{
			//	//	break;
			//	//}
			//}

			//std::map<uint16, float>::const_iterator it = m_mapVsUniformParams->begin();
			//for (; it != m_mapVsUniformParams->end(); ++it)
			//{
			//	uint16 temp = it->first;
			//	uint8 uniformIndex = uint8(temp >> 8);
			//	uint8 paramIndex = uint8(temp);
			//	if (index == uniformIndex)
			//	{
			//		pGeneralRegister[paramIndex] = it->second;
			//	}

			//	if (uniformIndex > index)
			//	{
			//		break;
			//	}
			//}
		}
		break;

		case U_PSGeneralRegister0:
		case U_PSGeneralRegister1:
		case U_PSGeneralRegister2:
		case U_PSGeneralRegister3:
		case U_PSGeneralRegister4:
		case U_PSGeneralRegister5:
		case U_PSGeneralRegister6:
		case U_PSGeneralRegister7:
		case U_PSGeneralRegister8:
		case U_PSGeneralRegister9:
		case U_PSGeneralRegister10:
		case U_PSGeneralRegister11:
		case U_PSGeneralRegister12:
		case U_PSGeneralRegister13:
		case U_PSGeneralRegister14:
		case U_PSGeneralRegister15:
		case U_PSGeneralRegister16:
		case U_PSGeneralRegister17:
		case U_PSGeneralRegister18:
		case U_PSGeneralRegister19:
		case U_PSGeneralRegister20:
		case U_PSGeneralRegister21:
		case U_PSGeneralRegister22:
		case U_PSGeneralRegister23:
		case U_PSGeneralRegister24:
		case U_PSGeneralRegister25:
		case U_PSGeneralRegister26:
		case U_PSGeneralRegister27:
		case U_PSGeneralRegister28:
		case U_PSGeneralRegister29:
		case U_PSGeneralRegister30:
		case U_PSGeneralRegister31:
		case U_PSGeneralRegister32:
		{
			uint32 index = uniformNameID - U_PSGeneralRegister0;

			if (index < MAX_PS_UNIFORM_COUNT)
			{
				updatePsUniformParam(index, pVector4);
				//*pVector4 = m_psCustomUniform[index];
			}

			else
				*pVector4 = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

			//const SHADER_PARAMS& psCustomParams = m_material->m_matInfo.psShaderParams;
			//*pVector4 = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			//for (size_t i = 0, count = psCustomParams.size(); i < count; ++i)
			//{
			//	const st_UniformParam& uniform = psCustomParams[i];

			//	if (index == uniform.uniformIndex)
			//	{
			//		pGeneralRegister[uniform.paramIndex] = uniform.paramVal;
			//	}

			//	//if (uniform.uniformIndex > index)
			//	//{
			//	//	break;
			//	//}
			//}

			//std::map<uint16, float>::const_iterator it = m_mapUniformParams->begin();
			//for (; it != m_mapUniformParams->end(); ++it)
			//{
			//	uint16 temp = it->first;
			//	uint8 uniformIndex = uint8(temp >> 8);
			//	uint8 paramIndex = uint8(temp);
			//	if (index == uniformIndex)
			//	{
			//		pGeneralRegister[paramIndex] = it->second;
			//	}

			//	if (uniformIndex > index)
			//	{
			//		break;
			//	}
			//}
		}
		break;

		default:
			assert(false && "unknown Uniform type");
		}
	}

	//=============================================================================
	void GpuParamsManager::getSamplerInfo(uint32 samplerNameID, ECHO_DRAW_SAMPLER_COMBINED& samplerInfo)
	{
		EchoZoneScoped;

		RenderSystem* renderSystem = Root::instance()->getRenderSystem();

		switch (samplerNameID)
		{
		case S_Diffuse:
		{
			getSamplerTexture(MAT_TEXTURE_DIFFUSE, mCustomTexKeyDiffuse, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Specular:
		{
			getSamplerTexture(MAT_TEXTURE_SPECULAR, mCustomTexKeySpecular, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Bump:
		{
			getSamplerTexture(MAT_TEXTURE_NORMAL, mCustomTexKeyBump, TEX_TYPE_2D, samplerInfo);
		}

		break;

		case S_Custom:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER, mCustomTexKeyCustomer, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom2:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER2, mCustomTexKeyCustomer2, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom3:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER3, mCustomTexKeyCustomer3, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom4:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER4, mCustomTexKeyCustomer4, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom5:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER5, mCustomTexKeyCustomer5, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom6:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER6, mCustomTexKeyCustomer6, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom7:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER7, mCustomTexKeyCustomer7, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom8:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER8, mCustomTexKeyCustomer8, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom9:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER9, mCustomTexKeyCustomer9, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Custom10:
		{
			getSamplerTexture(MAT_TEXTURE_CUSTOMER10, mCustomTexKeyCustomer10, TEX_TYPE_2D, samplerInfo);
		}
		break;

		case S_Environment:
		{
			RcTexture* pRcTex = nullptr;

			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					Material* matV1 = material.getV1();
					if (!matV1->m_textureList[MAT_TEXTURE_ENVIRONMENT].isNull())
					{
						pRcTex = matV1->m_textureList[MAT_TEXTURE_ENVIRONMENT]->getRcTex();
					}
					else
					{
						pRcTex = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
					}
				}
				else if (material.isV2())
				{
					Mat::BindTextureInfo bindSamplerInfo = material.getV2()->getBindTextureInfo(Mat::TEXTURE_BIND_ENVIRONMENT);
					if (!bindSamplerInfo.texture.isNull())
					{
						pRcTex = bindSamplerInfo.texture->getRcTex();
					}
					else
					{
						pRcTex = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
					}
					samplerInfo.samplerState = bindSamplerInfo.samplerState;
				}
			}
			else
			{
				pRcTex = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
			}

			samplerInfo.pSampler = pRcTex;
		}
		break;
		case S_ShadowMap0:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			if (Root::instance()->getEnableReverseDepth())
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_GREATEREQUAL;
			}
			else
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_LESSEQUAL;
			}
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			RcSampler* pRcTex = m_sceneManager->getShadowmapTex(ShadowLevel_0);
			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;
		case S_ShadowMap1:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			if (Root::instance()->getEnableReverseDepth())
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_GREATEREQUAL;
			}
			else
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_LESSEQUAL;
			}
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			RcSampler* pRcTex = m_sceneManager->getShadowmapTex(ShadowLevel_1);
			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;
		case S_ShadowMap2:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			if (Root::instance()->getEnableReverseDepth())
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_GREATEREQUAL;
			}
			else
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_LESSEQUAL;
			}
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			RcSampler* pRcTex = m_sceneManager->getShadowmapTex(ShadowLevel_2);
			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;

		case S_ShadowMap3:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			if (Root::instance()->getEnableReverseDepth())
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_GREATEREQUAL;
			}
			else
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_LESSEQUAL;
			}
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			RcSampler* pRcTex = m_sceneManager->getShadowmapTex(ShadowLevel_3);
			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;

		case S_Dpsm0:
		case S_Dpsm1:
		case S_Dpsm2:
		case S_Dpsm3:
		case S_Dpsm4:
		case S_Dpsm5:
		case S_Dpsm6:
		case S_Dpsm7:
		{
			samplerInfo.samplerState.AddressU =
				samplerInfo.samplerState.AddressV =
				samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.TexCompareFunc = Root::instance()->getEnableReverseDepth() ? ECHO_CMP_GREATEREQUAL : ECHO_CMP_LESSEQUAL;
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_BILINEAR;

			RcSampler* pRcTex = m_sceneManager->getPointShadowManager()->GetShadowTexture(samplerNameID - S_Dpsm0);
			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;

		case S_GBuffer0:
		case S_GBuffer1:
		case S_GBuffer2:
		case S_DeferredSC:
		{
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			assert(pRenderSystem->getCurrentRenderStrategyType() == RS_Deferred);
			DeferredRenderStrategy* pRS = dynamic_cast<DeferredRenderStrategy*>(pRenderSystem->getRenderStrategy(RS_Deferred));
			assert(pRS != nullptr);

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_POINT;
			samplerInfo.pSampler = pRS->getGBufferColor((ECHO_MRT_COLOR)(samplerNameID - S_GBuffer0));
			assert(samplerInfo.pSampler != nullptr);
		}
		break;

		case S_Depth:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_POINT;

			RcSampler* pRcSampler = nullptr;
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

			DepthBuffer* pDepthBuffer = pRenderSystem->getCurRenderStrategy()->getDepthBuffer();
			assert(nullptr != pDepthBuffer);

			if (nullptr != pDepthBuffer)
				pRcSampler = pDepthBuffer->getDepthTex();

			samplerInfo.pSampler = pRcSampler;
		}
		break;
		case S_DepthLoZ:
		{
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

			getSamplerInfo(S_Depth, samplerInfo);

			auto* chain = pRenderSystem->getCurRenderStrategy()->getPostProcessChain();
			if (!chain->isEnable(PostProcess::DepthDownSample)) break;
			auto* downSample = dynamic_cast<DownSampleDepth*>(chain->getPostProcess(PostProcess::DepthDownSample));
			if (!downSample) break;
			auto* loZ = downSample->getOutput();
			if (!loZ) break;
			samplerInfo.pSampler = loZ;
		}
		break;
		case S_BackbufferTex:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			RenderTarget* pRenderTarget = pRenderSystem->getCurRenderStrategy()->getRefractMap();

			if (nullptr != pRenderTarget)
			{
				samplerInfo.pSampler = pRenderTarget->getRenderTexture();
			}
		}
		break;

		case S_NoiseTex:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_POINT;

			if (m_sceneManager->getPsmshadowManager())
			{
				const TexturePtr& noiseTex = m_sceneManager->getPsmshadowManager()->getNoiseTex();
				if (!noiseTex.isNull())
					samplerInfo.pSampler = noiseTex->getRcTex();
				else
					samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
			}
		}
		break;

		case S_Noise3DTex:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			samplerInfo.pSampler = TextureManager::instance()->getNosie3DRcTexture();
		}
		break;

		case S_Noise3DTex128:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			samplerInfo.pSampler = TextureManager::instance()->getNosie3DRcTexture128();
		}
		break;

		case S_CausticTex:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			const TexturePtr& causticTexture = m_sceneManager->getGlobalCausticTexture();

			if (!causticTexture.isNull())
				samplerInfo.pSampler = causticTexture->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;

		case S_AgingTex:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			const TexturePtr& agingTex = m_sceneManager->getGlobalAgingTexture();
			if (!agingTex.isNull())
				samplerInfo.pSampler = agingTex->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;

		case S_ReflectCubeMap:
		{
			if (m_renderable != nullptr)
			{
				const MaterialWrapper& material = m_renderable->getMaterialWrapper();
				if (material.isV1())
				{
					const TexturePtr& textureCube = material.getV1()->m_textureList[MAT_TEXTURE_REFLECTCUBE];
					if (!textureCube.isNull() && textureCube->getRcTex())
						samplerInfo.pSampler = textureCube->getRcTex();
					else
						samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_CUBE_MAP);
				}
				else if (material.isV2())
				{
					Mat::BindTextureInfo bindSamplerInfo = material.getV2()->getBindTextureInfo(Mat::TEXTURE_BIND_REFLECTION_CUBE);
					if (!bindSamplerInfo.texture.isNull() && bindSamplerInfo.texture->getRcTex())
					{
						samplerInfo.pSampler = bindSamplerInfo.texture->getRcTex();
					}
					else
					{
						samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_CUBE_MAP);
					}
					samplerInfo.samplerState = bindSamplerInfo.samplerState;
				}
			}
			else
			{
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_CUBE_MAP);
			}
		}
		break;

		case S_EnvironmentCubeMap:
		{
			samplerInfo.samplerState = MaterialV2::getDefaultSamplerState();

			const TexturePtr& textureCube = m_sceneManager->getGlobalReflectTextrue();
			if (!textureCube.isNull())
				samplerInfo.pSampler = textureCube->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_CUBE_MAP);
		}
		break;

		case S_GlobalEnvironmentCubeMap:
		{
			samplerInfo.samplerState = MaterialV2::getDefaultSamplerState();

			const TexturePtr& textureCube = m_sceneManager->getEnvironmentLight()->getEnvironmentLightTex();
			if (!textureCube.isNull())
				samplerInfo.pSampler = textureCube->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_CUBE_MAP);
		}
		break;
		case S_Ripple:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;

			const TexturePtr& rippleTexture = m_sceneManager->getGlobalRippleTexture();

			if (!rippleTexture.isNull())
				samplerInfo.pSampler = rippleTexture->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;
#if defined(_WIN32)
		case S_RippleFlow:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;

			const TexturePtr& rippleFlowTexture = m_sceneManager->getGlobalRippleFlowTexture();

			if (!rippleFlowTexture.isNull())
				samplerInfo.pSampler = rippleFlowTexture->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;

		case S_RippleDrops:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;

			const TexturePtr& rippleDropsTexture = m_sceneManager->getGlobalRippleDropsTexture();

			if (!rippleDropsTexture.isNull())
				samplerInfo.pSampler = rippleDropsTexture->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;
		case S_RippleNoise:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_WRAP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_WRAP;

			const TexturePtr& rippleNoiseTexture = m_sceneManager->getGlobalRippleNoiseTexture();

			if (!rippleNoiseTexture.isNull())
				samplerInfo.pSampler = rippleNoiseTexture->getRcTex();
			else
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
		}
		break;
#endif
		//NOTE(yanghang):Different camera use different pixel mask texture.
		case S_ClusterLightMaskTex:
		{
			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_POINT;

			auto cluster_light = m_sceneManager->getClusterForwardLighting();
			auto pointLightMask = cluster_light->m_maskMap;
			if (!pointLightMask.isNull())
			{
				samplerInfo.pSampler = pointLightMask->getRcTex();
			}
			else
			{
				samplerInfo.pSampler = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
			}
		}break;

		case S_RainEffectMap:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			if (Root::instance()->getEnableReverseDepth())
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_GREATEREQUAL;
			}
			else
			{
				samplerInfo.samplerState.TexCompareFunc = ECHO_CMP_LESSEQUAL;
			}
			samplerInfo.samplerState.TexCompareMode = ECHO_TEX_CMP_REF_TO_TEXTURE;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_POINT;

			RcSampler* pRcTex = nullptr;

			const RainMaskManager* rainMaskManager = m_sceneManager->getRainMaskManager();
			if (rainMaskManager)
			{
				RenderTarget* rt = rainMaskManager->getRainEffectRT();
				if (rt)
				{
					pRcTex = rt->getDepthBuffer()->getDepthTex();
				}
			}

			if (pRcTex == nullptr)
			{
				pRcTex = renderSystem->getDefaultDepthBuffer()->getDepthTex();
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;
		case S_PreComputeTransmittanceMap:
		{
			samplerInfo.samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
			samplerInfo.samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

			samplerInfo.samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;

			RcSampler* pRcTex = nullptr;
			RenderStrategy* renderStrategy = renderSystem->getCurRenderStrategy();
			if (renderStrategy != nullptr) {
				PostProcess* pc = renderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = dynamic_cast<SphericalTerrainFog*>(pc);
					if (sphFog != nullptr) {
						RenderTarget* rt = sphFog->GetPreComputedTransmittance();
						if (rt != nullptr) {
							pRcTex = rt->getRenderTexture();
						}
					}
				}
			}
			if (pRcTex == nullptr)
			{
				pRcTex = TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D);
			}
			samplerInfo.pSampler = pRcTex;
		}
		break;
		default:
			assert(false && "unknown sampler type");
		}
	}

	void GpuParamsManager::getSamplerTexture(MAT_TEXTURE_TYPE type, const Name& customTexKey, TextureType texType, ECHO_DRAW_SAMPLER_COMBINED& samplerCombined)
	{
		EchoZoneScoped;

		RcTexture* pRcTexture = nullptr;

		const MaterialWrapper* material = m_renderable != nullptr ? &m_renderable->getMaterialWrapper() : nullptr;

		if (NULL != m_arrCustomTexture && !m_arrCustomTexture[type].isNull())
		{
			pRcTexture = m_arrCustomTexture[type]->getRcTex();
			if (material != nullptr && material->isV2())
			{
				samplerCombined.samplerState = material->getV2()->getSamplerState(matV1TBP2Mat2TBP(type));
			}
		}
		else
		{
			if (material != nullptr && material->isV1())
			{
				Material* matV1 = material->getV1();
				if (!matV1->m_textureList[type].isNull())
				{
					pRcTexture = matV1->m_textureList[type]->getRcTex();
				}
			}
			else if (material != nullptr && material->isV2())
			{
				Mat::BindTextureInfo bindSamplerInfo = material->getV2()->getBindTextureInfo(matV1TBP2Mat2TBP(type));
				if (!bindSamplerInfo.texture.isNull())
				{
					pRcTexture = bindSamplerInfo.texture->getRcTex();
					samplerCombined.samplerState = bindSamplerInfo.samplerState;
				}
			}
		}

		if (pRcTexture == nullptr)
		{
			samplerCombined.pSampler = TextureManager::instance()->getDefaultRcTexture(texType);
			if (material != nullptr && material->isV2())
			{
				samplerCombined.samplerState = MaterialV2::getDefaultSamplerState();
			}
			return;
		}

		bool bUseTexDensity = Root::instance()->getTextureDensityEnable();
		if (MAT_TEXTURE_DIFFUSE == type && bUseTexDensity)
		{
			pRcTexture = TextureManager::instance()->getTessellateRcTexture(pRcTexture->texDesc.width, pRcTexture->texDesc.height);
		}

		samplerCombined.pSampler = pRcTexture;
		if (material != nullptr && material->isV2())
		{
			samplerCombined.samplerState = MaterialV2::getDefaultSamplerState();
		}
	}


	void GpuParamsManager::updateMaterialParam(uint32 uniformNameID, Vector4* pVector4)
	{
		EchoZoneScoped;

		uint32 index = uniformNameID - U_MaterialDiffuse;
		if (index >= g_UniformMaxCnt[UNIFORM_TYPE_MAT])
		{
			return;
		}
		if (m_pMatCustomUniform)
		{
			*pVector4 = m_pMatCustomUniform[index];
		}
		else if (m_mapMaterialUniformParams)
		{
			auto it = m_mapMaterialUniformParams->find(index);
			if (it != m_mapMaterialUniformParams->end())
			{
				auto it0 = it->second.begin();
				for (; it0 != it->second.end(); ++it0)
				{
					if (it0->first < 4)
					{
						(*pVector4)[it0->first] = it0->second;
					}
				}
			}
		}
	}

	void GpuParamsManager::updateVsUniformParam(uint32 index, Vector4* pVector4)
	{
		const MaterialWrapper* material = m_renderable != nullptr ? &m_renderable->getMaterialWrapper() : nullptr;

		if (index >= g_UniformMaxCnt[UNIFORM_TYPE_VS])
		{
			return;
		}
		if (m_pVsCustomUniform)
		{
			*pVector4 = m_pVsCustomUniform[index];
		}
		else
		{
			if (material != nullptr && material->isValid())
			{
				if (material->isV1())
				{
					*pVector4 = material->getV1()->m_matInfo.m_vsCustomUniform[index];
				}
				else if (material->isV2())
				{
					*pVector4 = material->getV2()->getUniformValuesData(Mat::ShaderStage::VS)[index];
				}
			}
			if (m_mapVsUniformParams)
			{
				auto it = m_mapVsUniformParams->find(index);
				if (it != m_mapVsUniformParams->end())
				{
					auto it0 = it->second.begin();
					for (; it0 != it->second.end(); ++it0)
					{
						if (it0->first < 4)
						{
							(*pVector4)[it0->first] = it0->second;
						}
					}
				}
			}
		}
	}

	void GpuParamsManager::updatePsUniformParam(uint32 index, Vector4* pVector4)
	{
		const MaterialWrapper* material = m_renderable != nullptr ? &m_renderable->getMaterialWrapper() : nullptr;

		if (index >= g_UniformMaxCnt[UNIFORM_TYPE_PS])
		{
			return;
		}
		if (m_pPsCustomUniform)
		{
			*pVector4 = m_pPsCustomUniform[index];
		}
		else
		{
			if (material != nullptr && material->isValid())
			{
				if (material->isV1())
				{
					*pVector4 = material->getV1()->m_matInfo.m_psCustomUniform[index];
				}
				else if (material->isV2())
				{
					*pVector4 = material->getV2()->getUniformValuesData(Mat::ShaderStage::PS)[index];
				}
			}
			if (m_mapUniformParams)
			{
				auto it = m_mapUniformParams->find(index);
				if (it != m_mapUniformParams->end())
				{
					auto it0 = it->second.begin();
					for (; it0 != it->second.end(); ++it0)
					{
						if (it0->first < 4)
						{
							(*pVector4)[it0->first] = it0->second;
						}
					}
				}
			}
		}
	}
	//
	//
	//	void GpuParamsManager::updateCustomUniform(Renderable* rend)
	//	{
	//EchoZoneScoped;
	//
	//		if (!m_material.isValid())
	//		{
	//			return;
	//		}
	//		if (NULL == rend)
	//		{
	//			return;
	//		}
	//
	//		{
	//			if (m_material.isV1())
	//			{
	//				m_matCustomUniform[U_MaterialDiffuse - U_MaterialDiffuse] = m_material.getV1()->getDiffuseColor();
	//				m_matCustomUniform[U_MaterialSpecular - U_MaterialDiffuse] = m_material.getV1()->getSpecularColor();
	//				m_matCustomUniform[U_MaterialFresnel - U_MaterialDiffuse] = m_material.getV1()->getFresnel();
	//				m_matCustomUniform[U_MaterialEmission - U_MaterialDiffuse] = m_material.getV1()->getEmissionColor();
	//				m_matCustomUniform[U_MaterialCustomParam1 - U_MaterialDiffuse].x = m_material.getV1()->getHdrParam();
	//				m_matCustomUniform[U_AnimationParams - U_MaterialDiffuse] = m_material.getV1()->getAnimationParams();
	//			}
	//			else if (m_material.isV2())
	//			{
	//				memcpy(m_matCustomUniform, &m_material.getV2()->getAttributes().diffuseColor, sizeof(Vector4) * 4);
	//				m_matCustomUniform[U_MaterialCustomParam1 - U_MaterialDiffuse].x = m_material.getV2()->getAttributes().hdrParam;
	//				m_matCustomUniform[U_AnimationParams - U_MaterialDiffuse] = m_material.getV2()->getAttributes().animationParams;
	//			}
	//			m_matCustomUniform[U_MaterialFresnel - U_MaterialDiffuse].x = 1.0f;
	//			m_matCustomUniform[U_MaterialCustomParam - U_MaterialDiffuse] = Vector4::ZERO;
	//			m_matCustomUniform[U_MaterialCustomParam1 - U_MaterialDiffuse].z = 1.0f;
	//			m_matCustomUniform[U_LightPower - U_MaterialDiffuse] = Vector4(m_sceneManager->getSeltEmissiveLightPower(), 0, 0, 0);
	//
	//			const UNIFORM_MAP* m_mapMaterialUniformParams = rend->getCustomMaterialUniformParams();
	//
	//			if (NULL != m_mapMaterialUniformParams)
	//			{
	//				auto it = m_mapMaterialUniformParams->begin();
	//				for (; it != m_mapMaterialUniformParams->end(); ++it)
	//				{
	//					uint16 uniformIndex = it->first;
	//
	//					for (auto it0 = it->second.begin(); it0 != it->second.end(); ++it0)
	//					{
	//						uint16 paramIndex = it0->first;
	//						if (uniformIndex < MAX_MAT_UNIFORM_COUNT && paramIndex < 4)
	//						{
	//							m_matCustomUniform[uniformIndex][paramIndex] = it0->second;
	//						}
	//					}
	//				}
	//			}
	//
	//		}
	//
	//		{
	//
	//			if (m_material.isV1())
	//			{
	//				memcpy(m_vsCustomUniform, m_material.getV1()->m_matInfo.m_vsCustomUniform, sizeof(m_vsCustomUniform));
	//			}
	//			else if (m_material.isV2())
	//			{
	//				memcpy(m_vsCustomUniform, m_material.getV2()->getUniformValuesData(Mat::ShaderStage::VS), sizeof(m_vsCustomUniform));
	//			}
	//
	//			if (NULL != m_mapVsUniformParams)
	//			{
	//				auto it = m_mapVsUniformParams->begin();
	//				for (; it != m_mapVsUniformParams->end(); ++it)
	//				{
	//					uint16 uniformIndex = it->first;
	//
	//					for (auto it0 = it->second.begin(); it0 != it->second.end(); ++it0)
	//					{
	//						uint16 paramIndex = it0->first;
	//						if (uniformIndex < MAX_VS_UNIFORM_COUNT && paramIndex < 4)
	//						{
	//							m_vsCustomUniform[uniformIndex][paramIndex] = it0->second;
	//						}
	//					}
	//				}
	//			}
	//		}
	//
	//		{
	//			//const SHADER_PARAMS& psCustomParams = m_material->m_matInfo.psShaderParams;
	//			//for (size_t i = 0, count = psCustomParams.size(); i < count; ++i)
	//			//{
	//			//	const st_UniformParam& uniform = psCustomParams[i];
	//
	//			//	if (uniform.uniformIndex < MAX_PS_UNIFORM_COUNT && uniform.paramIndex < 4)
	//			//	{
	//			//		m_psCustomUniform[uniform.uniformIndex][uniform.paramIndex] = uniform.paramVal;
	//			//	}
	//			//}
	//
	//			if (m_material.isV1())
	//			{
	//				memcpy(m_psCustomUniform, m_material.getV1()->m_matInfo.m_psCustomUniform, sizeof(m_psCustomUniform));
	//			}
	//			else if (m_material.isV2())
	//			{
	//				memcpy(m_psCustomUniform, m_material.getV2()->getUniformValuesData(Mat::ShaderStage::PS), sizeof(m_psCustomUniform));
	//			}
	//
	//			const UNIFORM_MAP* m_mapUniformParams = rend->getCustomUniformParams();
	//			if (NULL != m_mapUniformParams)
	//			{
	//				auto it = m_mapUniformParams->begin();
	//				for (; it != m_mapUniformParams->end(); ++it)
	//				{
	//
	//					uint16 uniformIndex = it->first;
	//
	//					for (auto it0 = it->second.begin();it0!=it->second.end();++it0)
	//					{
	//						uint16 paramIndex = it0->first;
	//						if (uniformIndex < MAX_PS_UNIFORM_COUNT && paramIndex < 4)
	//						{
	//							m_psCustomUniform[uniformIndex][paramIndex] = it0->second;
	//						}
	//					}
	//				}
	//			}
	//
	//		}
	//	}

		//void GpuParamsManager::resetCustomUniform()
		//{
		//	EchoZoneScoped;
		//	memset(m_matCustomUniform, 0, sizeof(m_matCustomUniform));
		//	memset(m_psCustomUniform, 0, sizeof(m_psCustomUniform));
		//	memset(m_vsCustomUniform, 0, sizeof(m_vsCustomUniform));

		//}
} // namespace Echo

//-----------------------------------------------------------------------------
//	CPP File End
//-----------------------------------------------------------------------------
