//-----------------------------------------------------------------------------
// File:   EchoRenderSystem.cpp
//
// Author: miao_yuzhuang
//
// Date:   2016-5-16
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoStableHeaders.h"
#include "EchoRenderSystem.h"
#include "EchoRenderStrategy.h"
#include "EchoPass.h"
#include "EchoCamera.h"
#include "EchoGpuParamsManager.h"
#include "EchoForwardRenderStrategy.h"
#include "EchoDeferredRenderStrategy.h"
#include "EchoOcclusionPlaneManager.h"
#include "EchoMaterialBuildHelper.h"
#include "EchoRenderer.h"
#include "EchoMediaManager.h"
#include "EchoProtocolComponent.h"
#include "EchoComputable.h"
#include "EchoGpuPoolManagerImpl.h"
#include "EchoTextureManager.h"
#include "EchoBufferManager.h"

#include "EchoProfilerInfoManager.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <thread>
#endif



//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{
	//extern char TimerTempBuffer[512];

	uint32 RenderSystem::m_staticVBIBTotalSize = 0;
	uint32 RenderSystem::m_dynamicVBIBTotalSize = 0;
	uint32 RenderSystem::m_uniformBufferTotalSize = 0;
	uint32 RenderSystem::m_structBufferTotalSize = 0;
	//uint32 RenderSystem::m_gpuPoolBufferTotalSize = 0;
	static Renderer::ResourceType s_WorkType = Renderer::RT_RenderPipeline;
	static Renderer::ResourceType s_ResType = Renderer::RT_RenderPipelineRes;
	static bool s_CreateInResourceThread = true;
	static bool s_CreateInWorkThread = false;

	//=============================================================================
	RenderSystem::RenderSystem(Renderer* pRenderer, const ECHO_GRAPHIC_PARAM* pGraphicParam, const char* hookPath)
		: m_renderWindow(nullptr)
		, m_curStrategyIndex(0)
		, m_view(nullptr)
		, m_device(nullptr)
		, m_MainRenderPass(nullptr)
		, m_currentRenderPass(nullptr)
		, m_currentRenderTarget(nullptr)
		, m_currentRTDepthBuffer(nullptr)
		, m_ActiveViewport(nullptr)
		, m_bInitUniformParams(false)
		, m_ClearColor(0.0f)
		, m_DefaultDepthBuffer(nullptr)
		, m_pRenderer(pRenderer)
		, m_pRcDevice(nullptr)
		, m_pUniformBlockCommonVS(nullptr)
		, m_pUniformBlockCommonPS(nullptr)
		, m_workThreadID(0)
	{
EchoZoneScoped;

		for (uint32 i = 0; i < RENDER_STRATEGY_CNT; i++)
		{
			m_renderStrategy[i] = nullptr;
		}

		m_GraphicParam = *pGraphicParam;

		m_workThreadID = _getCurrentThreadID();

		m_renderStrategy[0] = new ForwardRenderStrategy(this, pGraphicParam); // ForwardRenderStrategy();
		m_renderStrategy[1] = new DeferredRenderStrategy(this, pGraphicParam); // DeferredRenderStrategy();
	}

	//=============================================================================
	RenderSystem::~RenderSystem()
	{
EchoZoneScoped;

		for (uint32 i = 0; i < RENDER_STRATEGY_CNT; i++)
		{
			SAFE_DELETE(m_renderStrategy[i]);
		}

		destroyRenderWindow();
		SAFE_DELETE(m_renderWindow);

		m_currentRenderPass = nullptr;
		m_currentRenderTarget = nullptr;
		m_currentRTDepthBuffer = nullptr;
		m_staticVBIBTotalSize = 0;
		m_dynamicVBIBTotalSize = 0;
		m_structBufferTotalSize = 0;
		//m_gpuPoolBufferTotalSize = 0;

		if (Root::instance()->isUseRenderCommand())
		{
			if (m_pUniformBlockCommonVS)
			{
				m_pRenderer->destroyBuffer(m_pUniformBlockCommonVS);
				m_pUniformBlockCommonVS = nullptr;
			}

			if (m_pUniformBlockCommonPS)
			{
				m_pRenderer->destroyBuffer(m_pUniformBlockCommonPS);
				m_pUniformBlockCommonPS = nullptr;
			}

			std::vector<RcRenderPipeline*>::iterator it, ite = m_destoryRcPipelineList.end();
			for (it = m_destoryRcPipelineList.begin(); it != ite; ++it)
			{
				RcRenderPipeline * pPipeline = *it;
				pPipeline->pDevice = &s_WorkType;
				if (pPipeline->type == ECHO_PIPELINE_TYPE_RENDER)
				{
					m_pRenderer->destroyRenderPipeline(pPipeline);
				}
				else
				{
					RcComputePipeline* pCPipeline = static_cast<RcComputePipeline*>(pPipeline);
					m_pRenderer->destroyComputePipeline(pCPipeline);
				}
			}
			m_destoryRcPipelineList.clear();

			ite = m_delayedDestoryRcPipelineList.end();
			for (it = m_delayedDestoryRcPipelineList.begin(); it != ite; ++it)
			{
				RcRenderPipeline * pPipeline = *it;
				pPipeline->pDevice = &s_WorkType;
				if (pPipeline->type == ECHO_PIPELINE_TYPE_RENDER)
				{
					m_pRenderer->destroyRenderPipeline(pPipeline);
				}
				else
				{
					RcComputePipeline* pCPipeline = static_cast<RcComputePipeline*>(pPipeline);
					m_pRenderer->destroyComputePipeline(pCPipeline);
				}
			}
			m_delayedDestoryRcPipelineList.clear();

			for (auto it = m_destoryRcTextureList.begin(); it != m_destoryRcTextureList.end(); ++it)
			{
				m_pRenderer->destroyTexture(*it);
			}
			m_destoryRcTextureList.clear();

			for (auto it = m_delayedDestoryRcTextureList.begin(); it != m_delayedDestoryRcTextureList.end(); ++it)
			{
				m_pRenderer->destroyTexture(*it);
			}
			m_delayedDestoryRcTextureList.clear();

			for (auto it = m_destoryRcBufferList.begin(); it != m_destoryRcBufferList.end(); ++it)
			{
				m_pRenderer->destroyBuffer(*it);
			}
			m_destoryRcBufferList.clear();

			for (auto it = m_delayedDestoryRcBufferList.begin(); it != m_delayedDestoryRcBufferList.end(); ++it)
			{
				m_pRenderer->destroyBuffer(*it);
			}
			m_delayedDestoryRcBufferList.clear();
		}

		if (m_DefaultDepthBuffer)
		{
			destoryDepthBuffer(m_DefaultDepthBuffer);
			m_DefaultDepthBuffer = nullptr;
		}

		m_pRenderer = nullptr;
		m_pRcDevice = nullptr;
	}

	//=============================================================================
	bool RenderSystem::beginFranme()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			m_pRenderer->beginFrame();
			return true;
		}
		else
		{
			bool bResult = m_view->BeginFrame();
			bResult &= m_view->Begin();
			return bResult;
		}
	}

	//=============================================================================
	void RenderSystem::endFrame()
	{
EchoZoneScoped;

#ifdef _DEBUG
		assert(m_unsubmittedStagingBufferCount == 0 && "There is staging buffer not submit!");
#endif // _DEBUG

		if (Root::instance()->isUseRenderCommand())
		{
			m_pRenderer->endFrame();
		}
		else
		{
			m_view->End();
			m_view->EndFrame();
		}
	}

	//=============================================================================
	void RenderSystem::submit()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			m_pRenderer->submit();
		}
	}

	//=============================================================================
	void RenderSystem::_convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest, bool forGpuProgram /* = false */)
	{
EchoZoneScoped;

		dest = matrix;

		//-----------------------------------------------------------------------------
		//IMPORTANT(yanghang): When gl_clip_control extension is enable. we need convert z
		// range of clip space from [-1,1] to [0,1].
		if (ECHO_DEVICE_GLES3 == m_GraphicParam.eClass && !Root::instance()->m_enableGLDepthZeroToOne)
			return;

		//IMPORTANT(yanghang):Depth range has been in[0~1].
		bool enableReverseDepth = Root::instance()->getEnableReverseDepth();
		if (!enableReverseDepth)
		{
			// Convert depth range from [-1,+1] to [0,1]
			// A = dest[2][2]
			// B = dest[2][3]
			// 透视投影下: -A - (B / z_view) = z_NDC:([-1, 1])
			// --> ((-A - (B / z_view)) + 1) / 2 = z_NDC:([0, 1])
			// dest[2][2] = (A - 1) / 2
			// dest[2][3] = B / 2
			// 正交投影下: A * z_view + B = z_NDC:([-1, 1])
			// --> (A * z_view + B + 1) / 2 = z_NDC:([0, 1])
			// dest[2][2] = A / 2
			// dest[2][3] = (B + 1) / 2
			dest[2][2] = (dest[2][2] + dest[3][2]) / 2;
			dest[2][3] = (dest[2][3] + dest[3][3]) / 2;
		}

		if (ECHO_DEVICE_VULKAN == m_GraphicParam.eClass)
		{
			dest[1][0] = -dest[1][0];
			dest[1][1] = -dest[1][1];
			dest[1][2] = -dest[1][2];
			dest[1][3] = -dest[1][3];
		}

		if (!forGpuProgram)
		{
			// Convert right-handed to left-handed
			dest[0][2] = -dest[0][2];
			dest[1][2] = -dest[1][2];
			dest[2][2] = -dest[2][2];
			dest[3][2] = -dest[3][2];
		}

		//-----------------------------------------------------------------------------
	}

	//=============================================================================
	void RenderSystem::copyRenderTarget(RenderTarget* pDest, RenderTarget* pSource, unsigned int mask, ECHO_RECT* inSourceRect, ECHO_RECT* inDestRect)
	{
EchoZoneScoped;

		////修改多场景时发现SceneBox偶现断言，暂时增加
		if (pDest == nullptr || pSource == nullptr)
			return;
		////修改多场景时发现SceneBox偶现断言，暂时增加
		RcRenderTarget* pDestRT = pDest->getRenderTexture();
		RcRenderTarget* pSoureRT = pSource->getRenderTexture();
		if (nullptr == pSoureRT)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			RcCopyRenderTarget *pRcParam = NEW_RcCopyRenderTarget;
			pRcParam->pDest = pDestRT;
			pRcParam->pSource = pSoureRT;
			pRcParam->copyMask = mask;
			if (inSourceRect)
			{
				pRcParam->sourceRectUse = true;
				pRcParam->sourceRect = *inSourceRect;
			}
			else
			{
				pRcParam->sourceRectUse = false;
			}
			if (inDestRect)
			{
				pRcParam->destRectUse = true;
				pRcParam->destRect = *inDestRect;
			}
			else
			{
				pRcParam->destRectUse = false;
			}
			m_pRenderer->copyRenderTarget(pRcParam);
		}
		else
		{
			IGXRenderTarget* pGxDestRT = nullptr;
			IGXRenderTarget* pGxSrcRT = nullptr;
			if (pDestRT)
				pGxDestRT = reinterpret_cast<IGXRenderTarget*>(pDestRT->pNative);

			pGxSrcRT = reinterpret_cast<IGXRenderTarget*>(pSoureRT->pNative);
			if (nullptr == pGxSrcRT)
				return;
			GX_RECT sourceRect;
			GX_RECT* pSourceRect = &sourceRect;
			if (inSourceRect)
			{
				pSourceRect->x = inSourceRect->x;
				pSourceRect->dx = inSourceRect->dx;
				pSourceRect->y = inSourceRect->y;
				pSourceRect->dy = inSourceRect->dy;
			}
			else
			{
				pSourceRect = NULL;
			}

			GX_RECT destRect;
			GX_RECT* pDestRect = &destRect;
			if (inDestRect)
			{
				pDestRect->x = inDestRect->x;
				pDestRect->dx = inDestRect->dx;
				pDestRect->y = inDestRect->y;
				pDestRect->dy = inDestRect->dy;
			}
			else
			{
				pDestRect = NULL;
			}
			m_device->CopyRenderTarget(pGxDestRT, pGxSrcRT, mask, pSourceRect, pDestRect);
		}
	}

	void RenderSystem::copyMRT2RT(RenderTarget* pDest, RenderTarget* pSource, uint32_t mask, ECHO_MRT_COLOR colorIndex)
	{
		if (pDest == nullptr || pSource == nullptr)
			return;
		RcRenderTarget* pDestRT = pDest->getRenderTexture();
		RcMultiRenderTarget* pSourceRT = pSource->getRcMultiRenderTarget();

		assert(pSourceRT != nullptr);
		if (nullptr == pSourceRT)
			return;

		RcCopyMRTToRT *pRcParam = NEW_RcCopyMRTToRT;
		pRcParam->pDest = pDestRT;
		pRcParam->pSource = pSourceRT;
		pRcParam->copyMask = mask;
		pRcParam->colorIndex = colorIndex;

		m_pRenderer->copyMRTToRT(pRcParam);
	}

	bool RenderSystem::copyRenderTargetContentsToMemory(RenderTarget* pRenderTarget,RcCaptureScreen &pInfo)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcCaptureScreen *pRcParam = NEW_RcCaptureScreen;

			pRcParam->pDestRT = pRenderTarget->m_rcRenderTarget;
			pRcParam->pCustom = pInfo.pCustom;
			pRcParam->pNative = pInfo.pNative;
			memcpy(pRcParam->filePath,pInfo.filePath, 1024);
			pRcParam->mode = pInfo.mode;
			pRcParam->type = pInfo.type;
			pRcParam->useCustomRes = pInfo.useCustomRes;
			pRcParam->customWidth = pInfo.customWidth;
			pRcParam->customHeight = pInfo.customHeight;

			m_pRenderer->captureScreen(pRcParam);

			//编辑器中直接用的该个接口，在单线程模式下需要将数据立即传递过去
			if (nullptr == pInfo.pCustom && ECHO_THREAD_ST == m_GraphicParam.threadMode &&
				ECHO_DEVICE_METAL != m_GraphicParam.eClass ) //如果使用单线程渲染，除了Metal外，其他API可以直接拿数据
			{
				if (pRcParam->dataSize > 0 && pRcParam->pData)
				{
					pInfo.realWidth = pRcParam->realWidth;
					pInfo.realHeight = pRcParam->realHeight;
					pInfo.dataSize = pRcParam->dataSize;
					pInfo.pData = pRcParam->pData;  ///@NOTE pRcParam->pData 底层不会释放,这里移交给pInfo.pData在调用函数成后释放
					pInfo.dataFormat = pRcParam->dataFormat;

					pRcParam->pData = nullptr;
					pRcParam->dataSize = 0;
					return true;
				}
				else
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			EchoRenderPass* pEchoRenderPass = pRenderTarget->getEchoRenderPass();
			if (!pEchoRenderPass || !pEchoRenderPass->pNative)
				return false;

			IGXRenderPass* pGxRenderPass = reinterpret_cast<IGXRenderPass*>(pEchoRenderPass->pNative);
			if (!pGxRenderPass)
				return false;

			IGXRTOutput* pGxOutput = pGxRenderPass->ReadOutRT(0);
			if (!pGxOutput)
				return false;

			pInfo.realWidth = pGxOutput->GetWidth();
			pInfo.realHeight = pGxOutput->GetHeight();
			pInfo.dataSize = pGxOutput->GetBufferSize();
			pInfo.pData = ::malloc(pInfo.dataSize);
			memcpy(pInfo.pData, pGxOutput->GetDataPtr(), pInfo.dataSize);
			pInfo.dataFormat = (ECHO_FMT)pGxOutput->GetFormat();

			pGxOutput->Release(); //数据已拷贝至 pInfo，故此释放

			return true;
		}
	}

	IGXDeviceHook* RenderSystem::getDeviceHook()
	{
		EchoZoneScoped;

		IGXDeviceHook* hook = nullptr;
		if (m_device)
			hook = m_device->GetHook();

		return hook;
	}

	void RenderSystem::onPrecompileShader(const char* vsName, const char* psName, const char* csName, const char* vs, const char* ps, const char* cs)
	{
EchoZoneScoped;
		if (Root::instance()->isUseRenderCommand() && Root::instance()->getDeviceType() == ECHO_DEVICE_GLES3)
		{
			RcGpuProgram* pRcProgram = NEW_RcGpuProgram;
			pRcProgram->pVSName = vsName;
			pRcProgram->pPSName = psName;
			pRcProgram->pCSName = csName;
			pRcProgram->pVS = vs;
			pRcProgram->pPS = ps;
			pRcProgram->pCS = cs;
			pRcProgram->immediateCompile = true;

			m_pRenderer->createGpuProgram(pRcProgram);

			m_pRenderer->destroyGpuProgram(pRcProgram);
		}
	}

	void RenderSystem::setThreadShareContext(bool value)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcContext* rcContext = Root::instance()->getRcContext();
			if (value)
				m_pRenderer->initThreadContext(rcContext);
			else
				m_pRenderer->uninitThreadContext(rcContext);
		}
		else
		{
			if (m_view)
				m_view->SetShareContext(value);
		}
	}


	void RenderSystem::shutDownNew()
	{
		for (size_t i = 0; i < RENDER_STRATEGY_CNT; i++)
		{
			if (m_renderStrategy[i])
			{
				m_renderStrategy[i]->shutdown();
			}
		}
	}

//	void RenderSystem::shutdown(void)
//	{
//EchoZoneScoped;
//
//		clearupDepthBuffers();
//		clearupRenderTargets();
//
//		for (size_t i = 0; i < m_renderListeners.size(); ++i)
//		{
//			m_renderListeners[i]->OnUninitializeDevice();
//		}
//	}

	void RenderSystem::convertColourValue(const ColorValue& colour, uint32* pDest)
	{
EchoZoneScoped;

		if (ECHO_DEVICE_DX9C == m_GraphicParam.eClass)
			*pDest = colour.getAsARGB();
		else
			*pDest = colour.getAsABGR();
	}

	void RenderSystem::_setViewport(Viewport *vp)
	{
EchoZoneScoped;

		if (!vp)
		{
			m_ActiveViewport = NULL;
		}
		else
		{
			m_ActiveViewport = vp;
		}
	}

	Viewport* RenderSystem::_getViewport(void)
	{
EchoZoneScoped;

		return m_ActiveViewport;
	}

	void RenderSystem::_notifyCameraRemoved(const Camera* cam)
	{
EchoZoneScoped;

		RenderTargetMap::iterator itb, ite;
		itb = m_renderTargets.begin();
		ite = m_renderTargets.end();
		for (; itb != ite; ++itb)
		{
			itb->second->_notifyCameraRemoved(cam);
		}
	}

	bool RenderSystem::createPassEffect(const Pass* pass, const ECHO_EFFECT_DESC& effectDesc, bool bPreCompile)
	{
EchoZoneScoped;

		bool bCanCompileShaderInResourceThread = Root::instance()->isCompileShaderInResourceThread();
		unsigned long curThreadID = _getCurrentThreadID();

		if (!bCanCompileShaderInResourceThread && m_workThreadID != curThreadID)
		{
			assert(false && "Warning: Can not compile shader in the resource thread!");
		}

		if (pass->mDest != nullptr)
		{
			assert(false && "Warning: The pass create effect duplicate!");
		}

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(m_renderPipelineThreadCheck);

			size_t nVsTextLen = strlen(effectDesc.szVertexShaderText);
			size_t nPsTextLen = strlen(effectDesc.szPixelShaderText);

			if (nVsTextLen == 0 || nPsTextLen == 0)
			{
				return false;
			}

			RcRenderPipeline *pRcRenderPipeline = nullptr;

			if (m_workThreadID != curThreadID)
			{
				pRcRenderPipeline = NEW_RcRenderPipelineRT;
				pRcRenderPipeline->pCustom = &s_ResType;
			}
			else
			{
				pRcRenderPipeline = NEW_RcRenderPipeline;
				pRcRenderPipeline->pCustom = &s_WorkType;
			}

			pRcRenderPipeline->desc.deferredCompile = effectDesc.deferredCompile;

			pRcRenderPipeline->pDevice = m_pRcDevice;

			if (effectDesc.RODesc.RenderTargetFormatArray[0] == ECHO_FMT_INVALID && effectDesc.RODesc.DepthStencilFormat == ECHO_FMT_INVALID)
			{
				assert(false && "Effect RODesc invalid!");
			}

			pRcRenderPipeline->bPreCompile = bPreCompile;
			pRcRenderPipeline->bHDREnable = getCurRenderStrategy()->isSupport(DL_HDR);

			if (nVsTextLen > 0)
			{
				pRcRenderPipeline->szVSData = (char*)malloc(nVsTextLen + 1);
				pRcRenderPipeline->szVSData[nVsTextLen] = '\0';
				memcpy(pRcRenderPipeline->szVSData, effectDesc.szVertexShaderText, nVsTextLen);
				pRcRenderPipeline->desc.szVertexShaderText = pRcRenderPipeline->szVSData;
				pRcRenderPipeline->desc.strVsName = Name(effectDesc.strVsName).c_str();
			}

			if (nPsTextLen > 0)
			{
				pRcRenderPipeline->szPSData = (char*)malloc(nPsTextLen + 1);
				pRcRenderPipeline->szPSData[nPsTextLen] = '\0';
				memcpy(pRcRenderPipeline->szPSData, effectDesc.szPixelShaderText, nPsTextLen);
				pRcRenderPipeline->desc.szPixelShaderText = pRcRenderPipeline->szPSData;
				pRcRenderPipeline->desc.strPsName = Name(effectDesc.strPsName).c_str();
			}

			if (effectDesc.nShaderCacheSizeVS > 0)
			{
				pRcRenderPipeline->szShaderCacheDataVS = (char*)malloc(effectDesc.nShaderCacheSizeVS + 1);
				pRcRenderPipeline->szShaderCacheDataVS[effectDesc.nShaderCacheSizeVS] = '\0';
				memcpy(pRcRenderPipeline->szShaderCacheDataVS, effectDesc.pShaderCacheVS, effectDesc.nShaderCacheSizeVS);
				pRcRenderPipeline->desc.pShaderCacheVS = pRcRenderPipeline->szShaderCacheDataVS;
				pRcRenderPipeline->desc.nShaderCacheSizeVS = effectDesc.nShaderCacheSizeVS;
			}
			if (effectDesc.nShaderCacheSizePS > 0)
			{
				pRcRenderPipeline->szShaderCacheDataPS = (char*)malloc(effectDesc.nShaderCacheSizePS + 1);
				pRcRenderPipeline->szShaderCacheDataPS[effectDesc.nShaderCacheSizePS] = '\0';
				memcpy(pRcRenderPipeline->szShaderCacheDataPS, effectDesc.pShaderCachePS, effectDesc.nShaderCacheSizePS);
				pRcRenderPipeline->desc.pShaderCachePS = pRcRenderPipeline->szShaderCacheDataPS;
				pRcRenderPipeline->desc.nShaderCacheSizePS = effectDesc.nShaderCacheSizePS;
			}

			pRcRenderPipeline->desc.IADesc = effectDesc.IADesc;
			pRcRenderPipeline->desc.RenderState = effectDesc.RenderState;
			pRcRenderPipeline->desc.RODesc = effectDesc.RODesc;
			pRcRenderPipeline->desc.glShaderVersion = effectDesc.glShaderVersion;

			if(Root::instance()->getEnableReverseDepth())
			{
				if(pRcRenderPipeline->desc.RenderState.DepthFunc == ECHO_CMP_LESS)
				{
					pRcRenderPipeline->desc.RenderState.DepthFunc =  ECHO_CMP_GREATER;
				}
				else if(pRcRenderPipeline->desc.RenderState.DepthFunc == ECHO_CMP_LESSEQUAL)
				{
					pRcRenderPipeline->desc.RenderState.DepthFunc = ECHO_CMP_GREATEREQUAL;
				}
				else if (pRcRenderPipeline->desc.RenderState.DepthFunc == ECHO_CMP_GREATEREQUAL)
				{
					pRcRenderPipeline->desc.RenderState.DepthFunc = ECHO_CMP_LESSEQUAL;
				}
				else if (pRcRenderPipeline->desc.RenderState.DepthFunc == ECHO_CMP_GREATER)
				{
					pRcRenderPipeline->desc.RenderState.DepthFunc = ECHO_CMP_LESS;
				}


			}

			m_pRenderer->createRenderPipeline(pRcRenderPipeline);

			pass->mDest = pRcRenderPipeline;

			return true;
		}
		else
		{
			GX_EFFECT_DESC gxEffectDesc;
			gxEffectDesc.szVertexShaderText = effectDesc.szVertexShaderText;
			gxEffectDesc.szPixelShaderText = effectDesc.szPixelShaderText;
			gxEffectDesc.szShaderCacheTextVS = effectDesc.pShaderCacheVS;
			gxEffectDesc.nShaderCacheSizeVS = effectDesc.nShaderCacheSizeVS;
			gxEffectDesc.szShaderCacheTextPS = effectDesc.pShaderCachePS;
			gxEffectDesc.nShaderCacheSizePS = effectDesc.nShaderCacheSizePS;
			gxEffectDesc.RenderState.BlendDst = (GX_BLEND)effectDesc.RenderState.BlendDst;
			gxEffectDesc.RenderState.BlendEnable = (GX_BOOL)effectDesc.RenderState.BlendEnable;
			gxEffectDesc.RenderState.BlendOp = (GX_BLEND_OP)effectDesc.RenderState.BlendOp;
			gxEffectDesc.RenderState.BlendSrc = (GX_BLEND)effectDesc.RenderState.BlendSrc;
			gxEffectDesc.RenderState.CCWStencilFail = (GX_STENCIL_OP)effectDesc.RenderState.CCWStencilFail;
			gxEffectDesc.RenderState.CCWStencilFunc = (GX_CMP_FUNC)effectDesc.RenderState.CCWStencilFunc;
			gxEffectDesc.RenderState.CCWStencilPass = (GX_STENCIL_OP)effectDesc.RenderState.CCWStencilPass;
			gxEffectDesc.RenderState.CCWStencilZfail = (GX_STENCIL_OP)effectDesc.RenderState.CCWStencilZfail;
			gxEffectDesc.RenderState.ColorWriteAlphaEnable = (GX_BOOL)!!(effectDesc.RenderState.ColorWriteFlags[0] & ECHO_COLOR_WRITE_ALPHA);
			gxEffectDesc.RenderState.ColorWriteBlueEnable = (GX_BOOL)!!(effectDesc.RenderState.ColorWriteFlags[0] & ECHO_COLOR_WRITE_BLUE);
			gxEffectDesc.RenderState.ColorWriteGreenEnable = (GX_BOOL)!!(effectDesc.RenderState.ColorWriteFlags[0] & ECHO_COLOR_WRITE_GREEN);
			gxEffectDesc.RenderState.ColorWriteRedEnable = (GX_BOOL)!!(effectDesc.RenderState.ColorWriteFlags[0] & ECHO_COLOR_WRITE_RED);
			gxEffectDesc.RenderState.ConstantBias = effectDesc.RenderState.ConstantBias;
			gxEffectDesc.RenderState.CullMode = (GX_CULL_MODE)effectDesc.RenderState.CullMode;
			gxEffectDesc.RenderState.DepthFunc = (GX_CMP_FUNC)effectDesc.RenderState.DepthFunc;
			gxEffectDesc.RenderState.DepthTestEnable = (GX_BOOL)effectDesc.RenderState.DepthTestEnable;
			gxEffectDesc.RenderState.DepthWriteEnable = (GX_BOOL)effectDesc.RenderState.DepthWriteEnable;
			gxEffectDesc.RenderState.PolygonOffsetFill = (GX_BOOL)effectDesc.RenderState.PolygonOffsetFill;
			//gxEffectDesc.RenderState.ScissorEnable = (GX_BOOL)effectDesc.RenderState.ScissorEnable;
			gxEffectDesc.RenderState.SlopeScaleBias = effectDesc.RenderState.SlopeScaleBias;
			gxEffectDesc.RenderState.StencilEnable = (GX_BOOL)effectDesc.RenderState.StencilEnable;
			gxEffectDesc.RenderState.StencilFail = (GX_STENCIL_OP)effectDesc.RenderState.StencilFail;
			gxEffectDesc.RenderState.StencilFunc = (GX_CMP_FUNC)effectDesc.RenderState.StencilFunc;
			gxEffectDesc.RenderState.StencilMask = effectDesc.RenderState.StencilMask;
			gxEffectDesc.RenderState.StencilPass = (GX_STENCIL_OP)effectDesc.RenderState.StencilPass;
			gxEffectDesc.RenderState.StencilWriteMask = effectDesc.RenderState.StencilWriteMask;
			gxEffectDesc.RenderState.StencilRef = effectDesc.RenderState.StencilRef;
			gxEffectDesc.RenderState.StencilZfail = (GX_STENCIL_OP)effectDesc.RenderState.StencilZfail;
			gxEffectDesc.RenderState.TwoSidedStencilEnable = (GX_BOOL)effectDesc.RenderState.TwoSidedStencilEnable;
			gxEffectDesc.RenderState.WindingMode = (GX_WINDING_MODE)effectDesc.RenderState.WindingMode;
			gxEffectDesc.RenderState.ScissorEnable = (GX_BOOL)effectDesc.RenderState.ScissorEnable;
			gxEffectDesc.RenderState.ScissorValue.Left = effectDesc.RenderState.ScissorValue.Left;
			gxEffectDesc.RenderState.ScissorValue.Top = effectDesc.RenderState.ScissorValue.Top;
			gxEffectDesc.RenderState.ScissorValue.Right = effectDesc.RenderState.ScissorValue.Right;
			gxEffectDesc.RenderState.ScissorValue.Bottom = effectDesc.RenderState.ScissorValue.Bottom;
			gxEffectDesc.RODesc.nRenderTargetCount = effectDesc.RODesc.nRenderTargetCount;
			gxEffectDesc.RODesc.RenderTargetFormatArray[0] = (GX_FMT)effectDesc.RODesc.RenderTargetFormatArray[0];
			gxEffectDesc.IADesc.nElementCount = effectDesc.IADesc.nElementCount;
			gxEffectDesc.IADesc.nStreamCount = effectDesc.IADesc.nStreamCount;
			unsigned int elementCount = gxEffectDesc.IADesc.nElementCount;
			unsigned int streamCount = gxEffectDesc.IADesc.nStreamCount;
			for (unsigned int i = 0; i < elementCount; i++)
			{
				gxEffectDesc.IADesc.ElementArray[i].eType = (GX_VERTEX_TYPE)effectDesc.IADesc.ElementArray[i].eType;
				gxEffectDesc.IADesc.ElementArray[i].nOffset = effectDesc.IADesc.ElementArray[i].nOffset;
				gxEffectDesc.IADesc.ElementArray[i].nStream = effectDesc.IADesc.ElementArray[i].nStream;
				gxEffectDesc.IADesc.ElementArray[i].Semantics = (GX_SEMANTICS)effectDesc.IADesc.ElementArray[i].Semantics;
			}

			for (unsigned int j = 0; j < streamCount; j++)
			{
				gxEffectDesc.IADesc.StreamArray[j].bPreInstance = (GX_BOOL)effectDesc.IADesc.StreamArray[j].bPreInstance;
				gxEffectDesc.IADesc.StreamArray[j].nRate = effectDesc.IADesc.StreamArray[j].nRate;
				gxEffectDesc.IADesc.StreamArray[j].nStride = effectDesc.IADesc.StreamArray[j].nStride;
			}
			IGXEffect* igxeffect = m_device->CreateEffect(&gxEffectDesc, pass->getHash());
			pass->mDest = igxeffect;
			if (igxeffect == NULL)
			{
				return false;
			}

			pass->mVertexInputSemantics = igxeffect->GetVertexInputSemantics();

			return true;
		}
	}

	bool RenderSystem::createComputePassEffect(const Pass* pass, const ECHO_EFFECT_DESC& effectDesc, bool bPreCompile)
	{
EchoZoneScoped;

		bool bCanCompileShaderInResourceThread = Root::instance()->isCompileShaderInResourceThread();
		unsigned long curThreadID = _getCurrentThreadID();

		if (!bCanCompileShaderInResourceThread && m_workThreadID != curThreadID)
		{
			assert(false && "Warning: Can not compile shader in the resource thread!");
		}

		if (pass->mDest != nullptr)
		{
			assert(false && "Warning: The pass create effect duplicate!");
		}

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(3);

			size_t nCsTextLen = strlen(effectDesc.szComputeShaderText);

			if (nCsTextLen == 0)
			{
				return false;
			}

			RcComputePipeline* pRcComputePipeline = nullptr;

			if (m_workThreadID != curThreadID)
			{
				pRcComputePipeline = NEW_RcComputePipelineRT;
				pRcComputePipeline->pCustom = &s_ResType;
			}
			else
			{
				pRcComputePipeline = NEW_RcComputePipeline;
				pRcComputePipeline->pCustom = &s_WorkType;
			}

			pRcComputePipeline->desc.deferredCompile = effectDesc.deferredCompile;

			pRcComputePipeline->pDevice = m_pRcDevice;

			pRcComputePipeline->bPreCompile = bPreCompile;
			pRcComputePipeline->bHDREnable = getCurRenderStrategy()->isSupport(DL_HDR);

			if (nCsTextLen > 0)
			{
				pRcComputePipeline->szCSData = (char*)malloc(nCsTextLen + 1);
				pRcComputePipeline->szCSData[nCsTextLen] = '\0';
				memcpy(pRcComputePipeline->szCSData, effectDesc.szComputeShaderText, nCsTextLen);
				pRcComputePipeline->desc.szComputeShaderText = pRcComputePipeline->szCSData;
				pRcComputePipeline->desc.strCsName = Name(effectDesc.strCsName).c_str();
			}

			if (effectDesc.nShaderCacheSizeCS > 0)
			{
				pRcComputePipeline->szShaderCacheDataCS = (char*)malloc(effectDesc.nShaderCacheSizeCS + 1);
				pRcComputePipeline->szShaderCacheDataCS[effectDesc.nShaderCacheSizeCS] = '\0';
				memcpy(pRcComputePipeline->szShaderCacheDataCS, effectDesc.pShaderCacheCS, effectDesc.nShaderCacheSizeCS);
				pRcComputePipeline->desc.pShaderCacheCS = pRcComputePipeline->szShaderCacheDataCS;
				pRcComputePipeline->desc.nShaderCacheSizeCS = effectDesc.nShaderCacheSizeCS;
			}

			pRcComputePipeline->desc.glShaderVersion = effectDesc.glShaderVersion;

			//pRcRenderPipeline->desc.strVsName = effectDesc.strVsName;
			//pRcRenderPipeline->desc.strPsName = effectDesc.strPsName;
			//pRcRenderPipeline->desc.strVsMacro = effectDesc.strVsMacro;
			//pRcRenderPipeline->desc.strPsMacro = effectDesc.strPsMacro;

			m_pRenderer->createComputePipeline(pRcComputePipeline);

			pass->mDest = pRcComputePipeline;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool RenderSystem::loadPassEffect(const Pass* pass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			if (!pass->mDest)
				return false;

			initPublicUniformParamList(pass->mDest);

			return true;
		}
		else
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return false;
			}

			bool bRes = effect->Load();

			if (bRes)
			{
				initPublicUniformParamList(nullptr);
			}

			return bRes;
		}
	}

	void RenderSystem::beginPassEffect(const Pass* pass) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->Begin();
		}
	}

	void RenderSystem::endPassEffect(const Pass* pass) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->End();
		}
	}

	void RenderSystem::beginUpdatePublicUniforms()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			if (m_device)
				m_device->BeginUpdatePublicUniforms();
		}
	}

	void RenderSystem::endUpdatePublicUniforms()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			if (m_device)
				m_device->EndUpdatePublicUniforms();
		}
	}

	void RenderSystem::beginUpdateGpuParameters(const Pass* pass) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->BeginUpdateGpuParameters();
		}
	}

	void RenderSystem::endUpdateGpuParameters(const Pass* pass) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->EndUpdateGpuParameters();
		}
	}

	void RenderSystem::setPolygonOffset(const Pass* pass, float constantBias, float slopeScaleBias) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			///@TODO
		}
		else
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->SetPolygonOffset(constantBias, slopeScaleBias);
		}
	}


	void RenderSystem::setEnableScissor(const Pass* pass, bool value, int left, int top, int right, int bottom) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->SetEnableScissor(value, left, top, right, bottom);
		}
	}

	void RenderSystem::releasePassEffect(const Pass* pass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(3);

			RcRenderPipeline* pPipeline = reinterpret_cast<RcRenderPipeline*>(pass->mDest);
			if (!pPipeline)
				return;

			Renderer::ResourceType poolType = s_WorkType;
			if (pPipeline->pCustom)
			{
				poolType = *(Renderer::ResourceType*)(pPipeline->pCustom);
			}

			unsigned long curThreadID = _getCurrentThreadID();
			if (m_workThreadID == curThreadID)
			{
				//原本在工作线程中创建的，还在工作线程中销毁
				//原本在资源线程中创建的，且pNative不为空的，再次投递到资源线程中销毁
				if (pPipeline->type == ECHO_PIPELINE_TYPE_RENDER)
				{
					if (s_WorkType == poolType)
					{
						pPipeline->pDevice = &s_WorkType;
						m_pRenderer->destroyRenderPipeline(pPipeline);
					}
					else
					{
						if (pPipeline->pNative)
						{
							m_delayedDestoryRcPipelineList.push_back(pPipeline);
						}
						else
						{
							pPipeline->pDevice = &s_WorkType;
							m_pRenderer->destroyRenderPipeline(pPipeline);
						}
					}
				}
				else
				{
					RcComputePipeline* pCPipeline = static_cast<RcComputePipeline*>(pPipeline);
					if (s_WorkType == poolType)
					{
						pCPipeline->pDevice = &s_WorkType;
						m_pRenderer->destroyComputePipeline(pCPipeline);
					}
					else
					{
						if (pCPipeline->pNative)
						{
							m_delayedDestoryRcPipelineList.push_back(pCPipeline);
						}
						else
						{
							pCPipeline->pDevice = &s_WorkType;
							m_pRenderer->destroyComputePipeline(pCPipeline);
						}
					}
				}

			}
			else
			{
				if (s_WorkType == poolType)
				{
					assert(false && "RenderSystem::releasePassEffect");
					LogManager::getSingleton().logMessage("-warn-\tThere's an anomaly at RenderSystem::releasePassEffect", LML_CRITICAL);
				}

				//在资源线程中调用过来的释放，直接销毁，无需再投递到资源线程
				pPipeline->pDevice = &s_ResType;
				if (pPipeline->type == ECHO_PIPELINE_TYPE_RENDER)
				{
					m_pRenderer->destroyRenderPipeline(pPipeline);
				}
				else
				{
					RcComputePipeline* pCPipeline = static_cast<RcComputePipeline*>(pPipeline);
					m_pRenderer->destroyComputePipeline(pCPipeline);
				}
			}
		}
		else
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			if (effect == NULL)
			{
				return;
			}
			effect->Release();
		}
	}


	void RenderSystem::destroyRcRenderPipeline(RcRenderPipeline* pPipeline)
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			return;

		if (!pPipeline)
			return;

		unsigned long curThreadID = _getCurrentThreadID();
		if (m_workThreadID == curThreadID)
		{
			assert(false && "RenderSystem::destroyRcRenderPipeline");
			LogManager::getSingleton().logMessage("-warn-\tThere's an anomaly at RenderSystem::destroyRcRenderPipeline", LML_CRITICAL);

			pPipeline->pDevice = &s_WorkType;
		}
		else
		{
			pPipeline->pDevice = &s_ResType;
		}

		if (pPipeline->type == ECHO_PIPELINE_TYPE_RENDER)
		{
			m_pRenderer->destroyRenderPipeline(pPipeline);
		}
		else
		{
			RcComputePipeline* pCPipeline = static_cast<RcComputePipeline*>(pPipeline);
			m_pRenderer->destroyComputePipeline(pCPipeline);
		}
	}


	void RenderSystem::destroyRcRenderPipelines()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			return;

		//std::vector<RcRenderPipeline*>::iterator it, ite = m_destoryRcPipelineList.end();
		//for (it = m_destoryRcPipelineList.begin(); it != ite; ++it)
		//{
		//	MaterialBuildHelper::instance()->addRequest(*it);
		//}
		if (!m_destoryRcPipelineList.empty())
		{
			MaterialBuildHelper::instance()->addRequest(m_destoryRcPipelineList);
		}
		m_destoryRcPipelineList.clear();

		m_destoryRcPipelineList.swap(m_delayedDestoryRcPipelineList);
	}

	EchoUniform* RenderSystem::getEchoEffectUniform(Pass* pass, const String & inName)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			return nullptr;
		}
		else
		{
			IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
			IGXUniform* pUnified = effect->GetUniform(inName);
			if (pUnified != 0)
			{
				EchoUniform* echoUniform = new EchoUniform();
				echoUniform->pNative = pUnified;
				return echoUniform;
			}
			return nullptr;
		}
	}

	EchoSampler* RenderSystem::getEchoEffectSample(Pass* pass, const String & inName, ECHO_TEX_TARGET type)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			return nullptr;
		}
		else
		{
			if (ECHO_TEX_TARGET_2D == type)
			{
				IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
				IGXSampler* pSample = effect->GetSampler(inName);
				if (pSample != 0)
				{
					EchoSampler* echoSample = new EchoSampler(type);
					echoSample->pNative = pSample;
					return echoSample;
				}
				return nullptr;
			}
			else if (ECHO_TEX_TARGET_CUBE_MAP == type)
			{
				IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
				IGXSamplerCube* pSampleCube = effect->GetSamplerCube(inName);
				if (pSampleCube != 0)
				{
					EchoSampler* echoSampleCube = new EchoSampler(type);
					echoSampleCube->pNative = pSampleCube;
					return echoSampleCube;
				}
				return nullptr;
			}
			else if (ECHO_TEX_TARGET_2D_ARRAY == type)
			{
				IGXEffect* effect = reinterpret_cast<IGXEffect*>(pass->mDest);
				IGXSampler2DArray* pSample = effect->GetSampler2DArray(inName);
				if (pSample != 0)
				{
					EchoSampler* echoSample = new EchoSampler(type);
					echoSample->pNative = pSample;
					return echoSample;
				}
				return nullptr;
			}

			return nullptr;
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const ColorValue& value) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			_setUniformValue(pDrawData, eNameID, &value, sizeof(float) * 4);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			ColorValue* pData = 0;
			if (pUniform != 0 && pUniform->Map((void**)&pData))
			{
				*pData = value;
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const Vector4& value) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			_setUniformValue(pDrawData, eNameID, &value, sizeof(float) * 4);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			Vector4* pData = 0;
			if (pUniform != 0 && pUniform->Map((void**)&pData))
			{
				*pData = value;
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const Vector3& value) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			Vector4 dataVec4(value.x, value.y, value.z, 0.0f);
			_setUniformValue(pDrawData, eNameID, &dataVec4, sizeof(float) * 4);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			Vector4* pData = 0;
			if (pUniform != 0 && pUniform->Map((void**)&pData))
			{
				pData->x = value.x;
				pData->y = value.y;
				pData->z = value.z;
				pData->w = 0.0f;
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const Vector2& value) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			Vector4 dataVec4(value.x, value.y, 0.0f, 0.0f);
			_setUniformValue(pDrawData, eNameID, &dataVec4, sizeof(float) * 4);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			Vector4* pData = 0;
			if (pUniform != 0 && pUniform->Map((void**)&pData))
			{
				pData->x = value.x;
				pData->y = value.y;
				pData->z = 0.0f;
				pData->w = 0.0f;
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const float& value) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			Vector4 dataVec4(value, 0.0f, 0.0f, 0.0f);
			_setUniformValue(pDrawData, eNameID, &dataVec4, sizeof(float) * 4);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			Vector4* pData = 0;
			if (pUniform != 0 && pUniform->Map((void**)&pData))
			{
				pData->x = value;
				pData->y = 0.0f;
				pData->z = 0.0f;
				pData->w = 0.0f;
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const void* inValuePtr, uint32 inValueSize) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			_setUniformValue(pDrawData, eNameID, inValuePtr, inValueSize);
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			void* dest = 0;
			if (pUniform != 0 && pUniform->Map(&dest))
			{
				memcpy(dest, inValuePtr, inValueSize);
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::setUniformValue(const Computable* comp, const Pass* pass, uint32 eNameID, const void* inValuePtr, uint32 inValueSize) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcComputeData* pComputeData = comp->getComputeData(pass);
			if (!pComputeData)
				return;

			_setUniformValue(pComputeData, eNameID, inValuePtr, inValueSize);
		}
		else
		{
			/*EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			void* dest = 0;
			if (pUniform != 0 && pUniform->Map(&dest))
			{
				memcpy(dest, inValuePtr, inValueSize);
				pUniform->Unmap();
			}*/
		}
	}

	bool RenderSystem::mapUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID, void** inDataPtr) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return false;

			for (unsigned int i = ECHO_UNIFORM_BLOCK_PRIVATE_VS; i < ECHO_UNIFORM_BLOCK_USAGE_COUNT; i++)
			{
				RcUniformBlock* pUniformBlock = pDrawData->pUniformBlock[i];
				if (pUniformBlock && pUniformBlock->pData && pUniformBlock->uniformInfos)
				{
					unsigned char * pBlockData = (unsigned char *)(pUniformBlock->pData);
					UniformInfoMap::const_iterator it = pUniformBlock->uniformInfos->find(eNameID);
					if (it != pUniformBlock->uniformInfos->end())
					{
						const ECHO_UNIFORM_INFO & uniformInfo = it->second;
						void * pDataBuffer = pBlockData + uniformInfo.nOffset;
						*inDataPtr = pDataBuffer;

						return true;
					}
				}
			}
			return false;
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			if (pUniform != 0 && pUniform->Map(inDataPtr))
			{
				return true;
			}
			return false;
		}
	}

	void RenderSystem::unmapUniformValue(const Renderable* rend, const Pass* pass, uint32 eNameID) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			// don't have to do anything.
		}
		else
		{
			EchoUniform* pUniform = pass->getEchoUniformByID(eNameID);
			if (pUniform != 0)
			{
				pUniform->Unmap();
			}
		}
	}

	void RenderSystem::_setUniformValue(RcDrawData* pDrawData, unsigned int eNameID,const void* _pData, uint32 byteSize) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			return;

		if (!pDrawData)
			return;

		for (unsigned int i = ECHO_UNIFORM_BLOCK_PRIVATE_VS; i < ECHO_UNIFORM_BLOCK_USAGE_COUNT; i++)
		{
			RcUniformBlock* pUniformBlock = pDrawData->pUniformBlock[i];
			if (pUniformBlock && pUniformBlock->uniformInfos)
			{
				if (!pUniformBlock->pData)
				{
					continue;
				}

				unsigned char * pBlockData = (unsigned char *)(pUniformBlock->pData);

				UniformInfoMap::const_iterator it = pUniformBlock->uniformInfos->find(eNameID);
				if (it != pUniformBlock->uniformInfos->end())
				{
					const ECHO_UNIFORM_INFO & uniformInfo = it->second;
					void * pDataBuffer = pBlockData + uniformInfo.nOffset;
					if (byteSize > uniformInfo.nDataSize)
					{
						byteSize = uniformInfo.nDataSize;
					}

					memcpy(pDataBuffer, _pData, byteSize);

					return;
				}
			}
		}
	}

	void RenderSystem::setTextureSampleValue(const Renderable* rend, const Pass* pass, uint32 eNameID, const TexturePtr & inTexture, const ECHO_SAMPLER_STATE& sampleState) const
	{
EchoZoneScoped;

		setTextureSampleValue(rend, pass, eNameID, inTexture->getRcTex(), sampleState);
	}

	void RenderSystem::setTextureSampleValue(const Renderable* rend, const Pass* pPass, uint32 eNameID, RcSampler* inTexture, const ECHO_SAMPLER_STATE& sampleState) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pPass);
			if (!pDrawData)
				return;

			RcRenderPipeline* pRcPipeline = reinterpret_cast<RcRenderPipeline*>(pPass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->samplerDescInfos)
				return;

			static uint32 g_SamplerPtrChanged = 0xFFFF;

			SamplerDescribeInfoMap::const_iterator it = pRcPipeline->samplerDescInfos->find(eNameID);
			if (it != pRcPipeline->samplerDescInfos->end())
			{
				const ECHO_SAMPLER_DESCRIBE_INFO & desc = it->second;

				ECHO_DRAW_SAMPLER_INFO &info = pDrawData->drawSamplerInfo[desc.nIndex];

				if (!info.pSampler && inTexture)
				{
					pDrawData->pCustom = &g_SamplerPtrChanged;
				}

				if (pDrawData->pNative && info.pSampler)
				{
					if (info.pSampler != inTexture)
						pDrawData->pCustom = &g_SamplerPtrChanged;
					else if (inTexture && (inTexture->pNative != info.pSampler->pNative))
						pDrawData->pCustom = &g_SamplerPtrChanged;
				}

				info.pSampler = inTexture;
				info.samplerState = sampleState;
				info.level = -1;
			}
		}
		else
		{
			EchoSampler* pEchoSampler = pPass->getEchoSamplerByID(eNameID);
			if (nullptr == pEchoSampler)
				return;

			//setTextureSampleValue(pEchoSampler, inTexture, sampleState);
		}
	}

	void RenderSystem::setTextureSampleValue(const Renderable* rend, const Pass* pPass, uint32 eNameID, const ECHO_DRAW_SAMPLER_COMBINED* pSamplerCombined, size_t count) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pPass);
			if (!pDrawData)
				return;

			RcRenderPipeline* pRcPipeline = reinterpret_cast<RcRenderPipeline*>(pPass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->samplerDescInfos)
				return;

			static uint32 g_SamplerPtrChanged = 0xFFFF;

			SamplerDescribeInfoMap::const_iterator it = pRcPipeline->samplerDescInfos->find(eNameID);
			if (it != pRcPipeline->samplerDescInfos->end())
			{
				const ECHO_SAMPLER_DESCRIBE_INFO& desc = it->second;

				if (desc.nArraySize < count)
				{
					assert("");
				}
				else if (desc.nArraySize == 1)
				{
					setTextureSampleValue(rend, pPass, eNameID, pSamplerCombined->pSampler, pSamplerCombined->samplerState);
				}
				else
				{
					ECHO_DRAW_SAMPLER_INFO& info = pDrawData->drawSamplerInfo[desc.nIndex];

					size_t dataSize = sizeof(pSamplerCombined[0]) * count;
					void* pSamplerData = Rc_Malloc(dataSize);
					memcpy(pSamplerData, pSamplerCombined, dataSize);

					info.pInfos = (ECHO_DRAW_SAMPLER_COMBINED*)pSamplerData;
					info.nCount = static_cast<unsigned int>(count);
					pDrawData->pCustom = &g_SamplerPtrChanged;
				}
			}
		}
		else
		{
		}
	}

	void RenderSystem::setTextureSampleValue(const Computable* rend, const Pass* pPass, uint32 eNameID, RcSampler* inTexture, const ECHO_SAMPLER_STATE& sampleState) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcComputeData* pComputeData = rend->getComputeData(pPass);
			if (!pComputeData)
				return;

			RcComputePipeline* pRcPipeline = reinterpret_cast<RcComputePipeline*>(pPass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->samplerDescInfos)
				return;

			static uint32 g_SamplerPtrChanged = 0xFFFF;

			SamplerDescribeInfoMap::const_iterator it = pRcPipeline->samplerDescInfos->find(eNameID);
			if (it != pRcPipeline->samplerDescInfos->end())
			{
				const ECHO_SAMPLER_DESCRIBE_INFO& desc = it->second;

				ECHO_DRAW_SAMPLER_INFO& info = pComputeData->drawSamplerInfo[desc.nIndex];

				if (!info.pSampler && inTexture)
				{
					pComputeData->pCustom = &g_SamplerPtrChanged;
				}

				if (pComputeData->pNative && info.pSampler)
				{
					if (info.pSampler != inTexture)
						pComputeData->pCustom = &g_SamplerPtrChanged;
					else if (inTexture && (inTexture->pNative != info.pSampler->pNative))
						pComputeData->pCustom = &g_SamplerPtrChanged;
					/*else if(-1 != info.level)
						pComputeData->pCustom = &g_SamplerPtrChanged;*/
				}

				info.pSampler = inTexture;
				info.level = -1;
				info.samplerState = sampleState;
			}
		}
		else
		{
			/*EchoSampler* pEchoSampler = pPass->getEchoSamplerByID(eNameID);
			if (nullptr == pEchoSampler)
				return;

			setTextureSampleValue(pEchoSampler, inTexture, sampleState);*/
		}
	}

#ifdef __APPLE__
	void RenderSystem::setImageValue(const Computable* rend, const Pass* pPass, uint32 eNameID, RcSampler* inTexture, uint32 level) const
#else
	void RenderSystem::setImageValue(const Computable* rend, const Pass* pPass, uint32 eNameID, RcSampler* inTexture, uint32 level) const
#endif
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcComputeData* pComputeData = rend->getComputeData(pPass);
			if (!pComputeData)
				return;

			RcComputePipeline* pRcPipeline = reinterpret_cast<RcComputePipeline*>(pPass->mDest);
			if (!pRcPipeline)
				return;

#ifdef __APPLE__
			SamplerDescribeInfoMap* pDescInfos = pRcPipeline->samplerDescInfos;
			ECHO_DRAW_SAMPLER_INFO* pDrawImageInfo = pComputeData->drawSamplerInfo;
#else
			SamplerDescribeInfoMap* pDescInfos = pRcPipeline->imageDescInfos;
			ECHO_DRAW_SAMPLER_INFO* pDrawImageInfo = pComputeData->drawImageInfo;
#endif
			if (nullptr == pDescInfos)
				return;

			static uint32 g_SamplerPtrChanged = 0xFFFF;

			SamplerDescribeInfoMap::const_iterator it = pDescInfos->find(eNameID);
			if (it != pDescInfos->end())
			{
				const ECHO_SAMPLER_DESCRIBE_INFO& desc = it->second;

				ECHO_DRAW_SAMPLER_INFO& info = pDrawImageInfo[desc.nIndex];

				if (!info.pSampler && inTexture)
				{
					pComputeData->pCustom = &g_SamplerPtrChanged;
				}

				if (pComputeData->pNative && info.pSampler)
				{
					if (info.pSampler != inTexture)
						pComputeData->pCustom = &g_SamplerPtrChanged;
					else if (inTexture && (inTexture->pNative != info.pSampler->pNative))
						pComputeData->pCustom = &g_SamplerPtrChanged;
#if !defined(__APPLE__)
					else if (level != info.level)
						pComputeData->pCustom = &g_SamplerPtrChanged;
#endif
				}

				info.pSampler = inTexture;
#ifdef __APPLE__
				//Note(WangPeng): level idx must be specified by uniform in Metal
				assert(level == -1);
				info.level = 0;
#else
				info.level = level;
#endif
			}
		}
		else
		{
			/*EchoSampler* pEchoSampler = pPass->getEchoSamplerByID(eNameID);
			if (nullptr == pEchoSampler)
				return;

			setTextureSampleValue(pEchoSampler, inTexture, sampleState);*/
		}
	}

//	void RenderSystem::setTextureSampleValue(EchoSampler* pEchoSampler, RcSampler* inTexture, const ECHO_SAMPLER_STATE& sampleState) const
//	{
//EchoZoneScoped;
//
//		if (Root::instance()->isUseRenderCommand())
//			return;
//
//		GX_SAMPLER_STATE sampleStateTemp;
//		sampleStateTemp.TexCompareMode = (GX_TEX_CMP_MODE)sampleState.TexCompareMode;
//		sampleStateTemp.TexCompareFunc = (GX_CMP_FUNC)sampleState.TexCompareFunc;
//		sampleStateTemp.AddressU = (GX_TEX_ADDRESS)sampleState.AddressU;
//		sampleStateTemp.AddressV = (GX_TEX_ADDRESS)sampleState.AddressV;
//		sampleStateTemp.AddressW = (GX_TEX_ADDRESS)sampleState.AddressW;
//		sampleStateTemp.MagFilter = (GX_TEX_FILTER)sampleState.MagFilter;
//		sampleStateTemp.MinFilter = (GX_TEX_FILTER)sampleState.MinFilter;
//		sampleStateTemp.MipFilter = (GX_TEX_FILTER)sampleState.MipFilter;
//
//		switch (pEchoSampler->m_type)
//		{
//		case ECHO_TEX_TARGET_2D:
//		{
//			IGXTex* pGxTex = nullptr;
//			switch (inTexture->type)
//			{
//			case ECHO_SAMPLER_TEXTURE:
//				pGxTex = reinterpret_cast<IGXTex*>(inTexture->pNative);
//				break;
//			case ECHO_SAMPLER_DEPTH_STENCIL:
//			{
//				IGXDepthStencil * pGXDepthStencil = reinterpret_cast<IGXDepthStencil*>(inTexture->pNative);
//				if (pGXDepthStencil)
//					pGxTex = pGXDepthStencil->GetDepthTex();
//			}
//			break;
//			case ECHO_SAMPLER_RENDER_TARGET:
//			case ECHO_SAMPLER_RENDER_WINDOW:
//			{
//				IGXRenderTarget * pGXRenderTarget = reinterpret_cast<IGXRenderTarget*>(inTexture->pNative);
//				if (pGXRenderTarget)
//					pGxTex = pGXRenderTarget->GetColorTex();
//			}
//			break;
//			default:
//				break;
//			}
//
//			if (!pGxTex)
//				return;
//
//			IGXSampler *pGxSamper = reinterpret_cast<IGXSampler*>(pEchoSampler->pNative);
//			if (!pGxSamper)
//				return;
//
//			pGxSamper->SetTexture(pGxTex, &sampleStateTemp);
//
//			break;
//		}
//		case ECHO_TEX_TARGET_CUBE_MAP:
//		{
//			IGXCube* pGxCube = nullptr;
//			if (ECHO_SAMPLER_TEXTURE == inTexture->type)
//			{
//				pGxCube = reinterpret_cast<IGXCube*>(inTexture->pNative);
//			}
//
//			if (!pGxCube)
//				return;
//
//			IGXSamplerCube * pGxSamplerCube = reinterpret_cast<IGXSamplerCube*>(pEchoSampler->pNative);
//			if (!pGxSamplerCube)
//				return;
//
//			pGxSamplerCube->SetTexture(pGxCube, &sampleStateTemp);
//
//			break;
//		}
//		case ECHO_TEX_TARGET_2D_ARRAY:
//		{
//			IGXTex2DArray* pGx2DArrayTex = nullptr;
//			if (ECHO_SAMPLER_TEXTURE == inTexture->type)
//			{
//				pGx2DArrayTex = reinterpret_cast<IGXTex2DArray*>(inTexture->pNative);
//			}
//
//			if (!pGx2DArrayTex)
//				return;
//
//			IGXSampler2DArray * pGXSampler2DArray = reinterpret_cast<IGXSampler2DArray*>(pEchoSampler->pNative);
//			if (!pGXSampler2DArray)
//				return;
//
//			pGXSampler2DArray->SetTexture(pGx2DArrayTex, &sampleStateTemp);
//
//			break;
//		}
//		default:
//			break;
//		}
//	}

	void RenderSystem::setBufferValue(const Computable* comp, const Pass* pass, uint32 eNameID, RcBuffer* inBuffer)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcComputeData* pComputeData = comp->getComputeData(pass);
			if (!pComputeData)
				return;

			RcComputePipeline* pRcPipeline = reinterpret_cast<RcComputePipeline*>(pass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->structBufferInfos)
				return;


			StructBufferInfoMap::const_iterator it = pRcPipeline->structBufferInfos->find(eNameID);
			if (it != pRcPipeline->structBufferInfos->end())
			{
				const ECHO_STRUCT_BUFFER_INFO& desc = it->second;

				RcBuffer*& structBuffer = pComputeData->structBufferInfo[desc.nIndex];

				structBuffer = inBuffer;

			}
		}
		else
		{
		}
	}

	void RenderSystem::setBufferValue(const Renderable* rend, const Pass* pass, uint32 eNameID, RcBuffer* inBuffer)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcDrawData* pDrawData = rend->getDrawData(pass);
			if (!pDrawData)
				return;

			RcRenderPipeline* pRcPipeline = reinterpret_cast<RcRenderPipeline*>(pass->mDest);
			if (!pRcPipeline || nullptr == pRcPipeline->structBufferInfos)
				return;

			//static uint32 g_SamplerPtrChanged = 0xFFFF;

			StructBufferInfoMap::const_iterator it = pRcPipeline->structBufferInfos->find(eNameID);
			if (it != pRcPipeline->structBufferInfos->end())
			{
				const ECHO_STRUCT_BUFFER_INFO& desc = it->second;

				RcBuffer*& structBuffer = pDrawData->structBufferInfo[desc.nIndex];
				structBuffer = inBuffer;
			}
		}
		else
		{
		}
	}

	void RenderSystem::setViewport(const ECHO_VIEWPORT* viewport) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcUpdateViewport *pRcUpdateViewport = NEW_RcUpdateViewport;
			pRcUpdateViewport->pDevice = m_pRcDevice;
			pRcUpdateViewport->veiwport = *viewport;

			m_pRenderer->updateViewport(pRcUpdateViewport);
		}
		else
		{
			assert(nullptr != viewport);

			GX_VIEWPORT viewportTemp;
			viewportTemp.Height = viewport->Height;
			viewportTemp.MaxDepth = viewport->MaxDepth;
			viewportTemp.MinDepth = viewport->MinDepth;
			viewportTemp.Width = viewport->Width;
			viewportTemp.X = viewport->X;
			viewportTemp.Y = viewport->Y;

			m_device->SetViewPort(&viewportTemp);
		}
	}


	GX_TEXTURE_TYPE RenderSystem::getTextureType(const void* _pData, uint32	_ByteSize)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			///#TODO
			return GX_TEXTURE_TYPE_2D;
		}
		else
		{
			return m_device->GetTextureType(_pData, _ByteSize);
		}
	}

	void RenderSystem::resizeWindow(uint32 width, uint32 height)
	{
EchoZoneScoped;

		uint32 oldWidth = m_renderWindow->getTargetWidth();
		uint32 oldHeight = m_renderWindow->getTargetHeight();

		if (width > 4 && height > 4 && (width != oldWidth || height != oldHeight || Root::instance()->getDeviceType() == ECHO_DEVICE_VULKAN))
		{
			createRenderWindow(width, height, Root::instance()->getRcContext()->swapchainColorFormat, true);

			fireResizeEvent(width, height);
		}
	}

	//=============================================================================
	RenderTarget* RenderSystem::createRenderWindow(uint32 width, uint32 height, ECHO_FMT format, bool bResize)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (m_renderWindow)
			{
				RcRenderWindow* pRcRenderWindow = (RcRenderWindow*)(m_renderWindow->m_rcRenderTarget);
				m_pRenderer->destroyRenderWindow(pRcRenderWindow);
			}

			RcRenderWindow* pRcRenderWindow = NEW_RcRenderWindow;

			pRcRenderWindow->format = format;
			pRcRenderWindow->pDepthStencil = nullptr;
			pRcRenderWindow->deviceIndex = 0;
			pRcRenderWindow->connection = m_GraphicParam.pInstance;
			pRcRenderWindow->window = m_GraphicParam.pHandle;
			//NOTE(yanghang):Only work on ios metal,using the as flag to create depth-stencil.
			pRcRenderWindow->depthStencilFormat = Root::instance()->m_depthFormat; //d3d11 and gles don't support create depthstencil for renderwindow.

			pRcRenderWindow->width = width;
			pRcRenderWindow->height = height;

			pRcRenderWindow->pName = "RenderWindow";

			m_pRenderer->createRenderWindow(pRcRenderWindow);

			m_pRcDevice = pRcRenderWindow->pDevice;

			if (nullptr == m_renderWindow)
			{
				m_renderWindow = new RenderTarget(width, height, format);
			}
			m_renderWindow->m_rcRenderTarget = pRcRenderWindow;
		}
		else
		{
			m_view->ReSize(width, height);
			if (nullptr == m_renderWindow)
			{
				m_renderWindow = new RenderTarget(width, height, format);
			}
		}

		OcclusionPlaneManager::instance()->InitRenderTarget(width, height);

		if (bResize)
		{
			m_renderWindow->setTargetWidthAndHeight(width, height);
			for (size_t i = 0; i < RENDER_STRATEGY_CNT; i++)
			{
				if (m_renderStrategy[i])
				{
					m_renderStrategy[i]->resize(width, height);
				}
			}
		}
		else
		{
			for (size_t i = 0; i < RENDER_STRATEGY_CNT; i++)
			{
				if (m_renderStrategy[i])
				{
					m_renderStrategy[i]->setRenderWindow(m_renderWindow);
				}
			}
		}

		if (m_selectedDLSSQueryQuality != ECHO_DLSS_PERF_QUALITY_DISABLE)
		{
			resetDLSSPerfQuality(m_selectedDLSSQueryQuality);
		}

		return m_renderWindow;
	}

	void RenderSystem::destroyRenderWindow()
	{
EchoZoneScoped;

		RcRenderWindow* pRcRenderWindow = (RcRenderWindow*)(m_renderWindow->m_rcRenderTarget);
		m_pRenderer->destroyRenderWindow(pRcRenderWindow);
		m_renderWindow->m_rcRenderTarget = nullptr;
	}

	//=============================================================================
	RenderTarget* RenderSystem::createSeperateRenderWindow(void* OSWindowHandle, const Echo::Name& rtName,uint32 width, uint32 height, ECHO_FMT format, DepthBuffer* pDepthBuffer, bool autoUpdate)
	{
EchoZoneScoped;

		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

		RcRenderWindow* pRcRenderWindow = NEW_RcRenderWindow;

		pRcRenderWindow->format = format;
		pRcRenderWindow->pDepthStencil = nullptr;
		pRcRenderWindow->deviceIndex = 0;
		pRcRenderWindow->connection = 0;
		pRcRenderWindow->window = OSWindowHandle;
		pRcRenderWindow->depthStencilFormat = Root::instance()->m_depthFormat;

		if(pDepthBuffer)
		{
			pRcRenderWindow->pDepthStencil = pDepthBuffer->getDepthTex();
		}

		pRcRenderWindow->width = width;
		pRcRenderWindow->height = height;

		pRcRenderWindow->pName = rtName.c_str();

		m_pRenderer->createSeperateRenderWindow(pRcRenderWindow);

		RenderTarget* resultRenderWindow = new RenderTarget(width, height, format);

		if (resultRenderWindow)
		{
			resultRenderWindow->m_rcRenderTarget = pRcRenderWindow;
			resultRenderWindow->m_depthBuffer = pDepthBuffer;
			resultRenderWindow->m_name = rtName;
			resultRenderWindow->m_format = format;
			resultRenderWindow->m_bAutoUpdate = autoUpdate;

			m_renderTargets.insert(std::make_pair(rtName, resultRenderWindow));
		}
		else
		{
			assert(false && "RT with the name already exists!");
		}
		return resultRenderWindow;
	}

	//=============================================================================
	RenderTarget* RenderSystem::createRenderTarget(const Name& name, uint32 width, uint32 height, ECHO_FMT format, DepthBuffer* pDepthBuffer, bool bAutoUpdate/* = true*/)
	{
EchoZoneScoped;

		RenderTarget* pRenderTarget = getRenderTarget(name);
		if (!pRenderTarget)
		{
			pRenderTarget = new RenderTarget(name, width, height, format, pDepthBuffer, bAutoUpdate);
			m_renderTargets.insert(std::make_pair(name, pRenderTarget));
		}
		else
		{
			assert(false && "RT with the name already exists!");
		}
		return pRenderTarget;
	}

	RenderTarget* RenderSystem::createRenderTarget(const Name& name, uint32 width, uint32 height,
												   const std::vector<ECHO_FMT>& formats, DepthBuffer* pDepthBuffer,
												   bool bAutoUpdate)
	{
		RenderTarget* pRenderTarget = getRenderTarget(name);
		if (!pRenderTarget)
		{
			if (formats.size() > ECHO_MRT_COLOR_COUNT)
			{
				assert(false && "The number of color attachments for RT exceeds the limit!");
				return nullptr;
			}

			pRenderTarget = new RenderTarget(name, width, height, formats, pDepthBuffer, bAutoUpdate);
			m_renderTargets.insert(std::make_pair(name, pRenderTarget));
		}
		else
		{
			assert(false && "MRT with the name already exists!");
		}
		return pRenderTarget;
	}

	EchoRenderPass* RenderSystem::createRenderPass(uint32 _nRT, RcRenderTarget* _pRT, RcDepthStencil* _pDS)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			return nullptr;
		}
		else
		{
			EchoRenderPass *echoRenderPass = new EchoRenderPass();

			if (_nRT == 0)
			{
				echoRenderPass->pNative = m_device->CreateRenderPass(_nRT, 0, (IGXDepthStencil*)(nullptr == _pDS ? nullptr : _pDS->pNative));
			}
			else
			{
				IGXRenderTarget* renderTarget = (IGXRenderTarget*)(_pRT->pNative);
				IGXRenderPass* renderPass = m_device->CreateRenderPass(_nRT, &renderTarget, (IGXDepthStencil*)(nullptr == _pDS ? nullptr : _pDS->pNative));
				echoRenderPass->pNative = renderPass;
				renderTarget->pRenderPass = renderPass;
			}
			return echoRenderPass;
		}
	}

	void RenderSystem::destoryRenderPass(EchoRenderPass* renderPass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return;

		IGXRenderPass* gxRenderPass = (IGXRenderPass*)(renderPass->pNative);
		if (gxRenderPass)
		{
			gxRenderPass->Release();
		}
		delete renderPass;
	}

	bool RenderSystem::beginRenderPass(EchoRenderPass* renderPass, uint32 _nWidth, uint32 _nHeight, const ECHO_CLEAR* _pClear)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return false;

		if (_pClear)
		{
			GX_CLEAR gxclear;
			gxclear.eClearColor[0] = (GX_LOAD_ACTION)_pClear->eClearColor[0];
			gxclear.eClearDepth = (GX_LOAD_ACTION)_pClear->eClearDepth;
			gxclear.eClearStencil = (GX_LOAD_ACTION)_pClear->eClearStencil;
			gxclear.fDepth = _pClear->fDepth;
			gxclear.nColor = _pClear->nColor;
			gxclear.nStencil = _pClear->nStencil;
			gxclear.vColor[0].x = _pClear->vColor[0];
			gxclear.vColor[0].y = _pClear->vColor[0];
			gxclear.vColor[0].z = _pClear->vColor[0];
			gxclear.vColor[0].w = _pClear->vColor[0];

			return ((IGXRenderPass*)(renderPass->pNative))->Begin(_nWidth, _nHeight, &gxclear);
		}
		else
		{
			return ((IGXRenderPass*)(renderPass->pNative))->Begin(_nWidth, _nHeight, NULL);
		}
	}

	void RenderSystem::endRenderPass(EchoRenderPass* renderPass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return;

		((IGXRenderPass*)(renderPass->pNative))->End();
	}


	void RenderSystem::setPolygonMode(ECHO_POLYGON_MODE level)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			///@TODO
		}
		else
		{
			m_device->SetPolygonMode((GX_POLYGON_MODE)level);
		}
	}

	void RenderSystem::setUseOverDraw(bool value)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return;

		if (value)
		{
			m_device->UseOverDrawRenderState();
		}
	}

	void RenderSystem::updateOverDrawState(bool value)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcUpdateOverDrawState *pUpdateOverDrawState = NEW_RcUpdateOverDrawState;
			pUpdateOverDrawState->bEnabled = value;
			m_pRenderer->updateOverDrawState(pUpdateOverDrawState);
		}
	}

	bool RenderSystem::setVertexBuffer(const Renderable* rend, const Pass* pass, uint32 _nStream, RcBuffer* _pVertexBuffer) const
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			return false;
		}
		else
		{
			if (nullptr == _pVertexBuffer)
				return false;

			IGXEffect* gxEffect = reinterpret_cast<IGXEffect*>(pass->mDest);
			IGXBaseVB* gxVB = reinterpret_cast<IGXBaseVB*>(_pVertexBuffer->pNative);
			return gxEffect->SetVertexBuffer(_nStream, gxVB);
		}
	}

	void RenderSystem::drawIndexed(const Pass* pass, ECHO_DRAW _DrawMode, RcBuffer* _pIB, uint32 _NumIndexes, uint32 _NumVertices) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* gxEffect = reinterpret_cast<IGXEffect*>(pass->mDest);
			IGXBaseIB* gxIB = reinterpret_cast<IGXBaseIB*>(_pIB->pNative);
			gxEffect->DrawIndexed((GX_DRAW)_DrawMode, gxIB, _NumIndexes, _NumVertices);
		}

	}

	void RenderSystem::drawInstanced(const Pass* pass, ECHO_DRAW _DrawMode, RcBuffer* _pIB, uint32 _NumPrimitive, uint32 _NumVertices, uint32 _NumInstance) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* gxEffect = reinterpret_cast<IGXEffect*>(pass->mDest);
			IGXBaseIB* gxIB = reinterpret_cast<IGXBaseIB*>(_pIB->pNative);
			gxEffect->DrawInstanced((GX_DRAW)_DrawMode, gxIB, _NumPrimitive, _NumVertices, _NumInstance);
		}
	}

	void RenderSystem::draw(const Pass* pass, ECHO_DRAW _DrawMode, uint32 _NumVertices) const
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* gxEffect = reinterpret_cast<IGXEffect*>(pass->mDest);
			gxEffect->Draw((GX_DRAW)_DrawMode, _NumVertices);
		}
	}

	void RenderSystem::updateMedia()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcUpdateMedia *pUpdateMedia = NEW_RcUpdateMedia;
			pUpdateMedia->pCustom = MediaManager::instance()->mRendeFun;
			m_pRenderer->updateMedia(pUpdateMedia);
		}
	}

	void RenderSystem::drawOffset(const Pass* pass, ECHO_DRAW _DrawMode, uint32 _StartVertices, uint32 _NumVertices)
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
		{
			IGXEffect* gxEffect = reinterpret_cast<IGXEffect*>(pass->mDest);
			gxEffect->DrawOffset((GX_DRAW)_DrawMode, _StartVertices, _NumVertices);
		}
	}

	void RenderSystem::draw(RcUpdateDrawData* pUpdateDrawData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			pUpdateDrawData->pUpdateRenderTarget = m_currentRenderTarget->m_rcRenderTarget;
			m_pRenderer->draw(pUpdateDrawData);
		}
	}

	void RenderSystem::dispatch(RcUpdateComputeData* pComputeData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			//pComputeData->pUpdateRenderTarget = m_currentRenderTarget->m_rcRenderTarget;
			m_pRenderer->dispatch(pComputeData);
		}
	}

	bool RenderSystem::hasCapability(const ECHO_CAPABILITIES c)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return m_pRenderer->hasCapability(c);
		else
			return m_device->HasCapability((GX_CAPABILITIES)c);
	}

	uint64_t RenderSystem::getDependentLimit(const ECHO_DEPENDENT_LIMITS _limit)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return m_pRenderer->getDependentLimit(_limit);
		else
			return m_device->GetDependentLimit((GX_DEPENDENT_LIMITS)_limit);
	}

	void RenderSystem::getVendorAndRenderer(std::string& vendor, std::string& renderer)
	{
EchoZoneScoped;

		vendor = m_GraphicParam.vendor;
		renderer = m_GraphicParam.renderer;
	}

	bool RenderSystem::isUseSingleThreadRender()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
			return m_GraphicParam.threadMode == ECHO_THREAD_ST;

		return true;
	}

	void RenderSystem::addResizeListener(ResizeListener* pListener)
	{
EchoZoneScoped;

		if (pListener)
		{
			m_ResizeListener.push_back(pListener);
		}
	}

	void RenderSystem::removeResizeListener(ResizeListener* pListener)
	{
EchoZoneScoped;

		if (pListener)
		{
			vector<ResizeListener*>::iterator iter = std::find(m_ResizeListener.begin(), m_ResizeListener.end(), pListener);
			if (iter != m_ResizeListener.end())
			{
				m_ResizeListener.erase(iter);
			}
		}
	}


	void RenderSystem::fireResizeEvent(uint32 width, uint32 height)
	{
EchoZoneScoped;

		for (size_t i = 0; i < m_ResizeListener.size(); ++i)
		{
			m_ResizeListener[i]->ResizeEvent(width, height);
		}
	}

	DepthBuffer* RenderSystem::getDefaultDepthBuffer()
	{
		if (m_DefaultDepthBuffer == nullptr)
		{
			m_DefaultDepthBuffer = createDepthBuffer(Name("Default_Depth"), 1, 1, ECHO_FMT_D16_UNORM);
		}
		return m_DefaultDepthBuffer;
	}

	void* RenderSystem::mallocRcUpdateStagingBuffer(unsigned int size)
	{
		if (size == 0)
			return nullptr;
#ifdef _DEBUG
		m_unsubmittedStagingBufferCount++;
#endif // _DEBUG
		return Rc_Malloc(size);
	}

	void RenderSystem::updateTextureWithStaging(RcTexture* pRcTex, void* _pData, uint32 _byteSize, const ECHO_RECT& _pRect, uint32 _layerIndex, uint32 _layerCount, bool _isCompressed, uint32 srcLevelOffset, uint32 dstLevelOffset, uint32 numUpdateLevels)
	{
		if (!pRcTex || !_pData || _byteSize == 0)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcUpdateTexture* pRcParam = NEW_RcUpdateTexture;
			pRcParam->pTexture = pRcTex;
			pRcParam->pUpdateData = _pData;
			pRcParam->updateDataSize = _byteSize;
			pRcParam->updateDataFreeMethod = ECHO_MEMORY_RC_FREE;
			pRcParam->rect = _pRect;
			pRcParam->layerStart = _layerIndex;
			pRcParam->layerCount = _layerCount;
			pRcParam->isCompressed = _isCompressed;
			pRcParam->srcLevelOffset = srcLevelOffset;
			pRcParam->dstLevelOffset = dstLevelOffset;
			pRcParam->numUpdateLevels = numUpdateLevels;

			m_pRenderer->updateTexture(pRcParam);
		}
#ifdef _DEBUG
		m_unsubmittedStagingBufferCount--;
#endif // _DEBUG
	}

	void RenderSystem::updateBufferWithStaging(RcBuffer* pRcBuffer, void* _pData, uint32 _byteSize, uint32 _offset, bool updateInCQueue)
	{
		if (!pRcBuffer || !_pData || _byteSize == 0)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcUpdateBuffer* pRcParam = NEW_RcUpdateBuffer;
			if (updateInCQueue)
			{
				pRcParam->pCustom = (void*)s_UpdateInComputeQueue;
			}
			pRcParam->pBuffer = pRcBuffer;
			pRcParam->pUpdateData = _pData;
			pRcParam->updateDataSize = _byteSize;
			pRcParam->updateDataFreeMethod = ECHO_MEMORY_RC_FREE;
			pRcParam->updateOffset = _offset;

			m_pRenderer->updateBuffer(pRcParam);
		}
#ifdef _DEBUG
		m_unsubmittedStagingBufferCount--;
#endif // _DEBUG
	}

	void RenderSystem::finishCommand()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			m_device->FinishCommand();
	}

	void RenderSystem::initPublicUniformParamList(void *pNative)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			if (m_bInitUniformParams)
				return;

			RcRenderPipeline* pPipeline = reinterpret_cast<RcRenderPipeline*>(pNative);
			if (!pPipeline )
				return;

			if (!pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_VS] ||
				!pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_PS])
			{
				return;
			}

			if (pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_VS]->empty() ||
				pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_PS]->empty())
			{
				return;
			}

			RcUniformBlock* uniformBlockVS = getUniformBlockCommonVS();
			//uniformBlockVS->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_VS];
			uniformBlockVS->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_COMMON_VS];


			RcUniformBlock* uniformBlockPS = getUniformBlockCommonPS();
			//uniformBlockPS->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_COMMON_PS];
			uniformBlockPS->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_COMMON_PS];

			m_bInitUniformParams = true;
		}
		else
		{
			if (m_bInitUniformParams)
				return;

			bool bReady = false;
			for (uint32 i = 0, uCount = UniformNameID_Count; i < uCount; ++i)
			{
				const String & paramName = UniformNameIDUtil::toName(i);
				assert(!paramName.empty());

				IGXUniform* pUnified = m_device->GetPublicUniform(paramName, bReady);
				if (!bReady)
					return;

				if (nullptr == pUnified)
					continue;

				EchoUniform* echoUniform = new EchoUniform();
				echoUniform->pNative = pUnified;

				m_publicUniformParamsList.insert(std::make_pair(i, echoUniform));
			}

			m_bInitUniformParams = true;
		}
	}


	unsigned long RenderSystem::_getCurrentThreadID()
	{
EchoZoneScoped;

		unsigned long threadID = 0;
#ifdef _WIN32
		threadID = GetCurrentThreadId();
#else
		std::thread::id curID = std::this_thread::get_id();
		threadID = std::hash<std::thread::id>()(curID);
#endif
		return threadID;
	}

	RenderStrategy*	RenderSystem::getCurRenderStrategy() const
	{
EchoZoneScoped;

		return getRenderStrategy(m_curStrategyIndex);
	}

	RenderStrategy*	RenderSystem::getRenderStrategy(uint32 index) const
	{
EchoZoneScoped;

		if (index >= RENDER_STRATEGY_CNT)
			return nullptr;

		return m_renderStrategy[index];
	}


	void RenderSystem::useRenderStrategy(uint32 index)
	{
EchoZoneScoped;

		if (m_renderStrategy[index])
		{
			uint32 oldIndex = m_curStrategyIndex;
			if (m_renderStrategy[oldIndex] && m_renderStrategy[oldIndex]->isEnable())
			{
				m_renderStrategy[oldIndex]->cleanup(true);
				m_renderStrategy[oldIndex]->setEnable(false);
			}

			m_curStrategyIndex = index;
			if (!m_renderStrategy[index]->isEnable())
			{
				m_renderStrategy[index]->setEnable(true);

				if (isEnableDLSS())
				{
					resetDLSSPerfQuality(m_selectedDLSSQueryQuality);
				}
				else
				{
					m_renderStrategy[index]->setMainRenderTargetScale(m_mainRTScale);
					m_renderStrategy[index]->setViewportScale(m_viewportScale);
				}

				if (m_renderWindow)
				{
					uint32_t width, height;
					m_renderWindow->getTargetWidthAndHeight(width, height);
					m_renderStrategy[index]->resize(width, height);
				}
			}
		}
	}

	bool RenderSystem::getEnableVisibilityBuffer() const
	{
		return m_curStrategyIndex == RS_Deferred && Root::instance()->getEnableVisibilityBuffer();
	}

	VisRenderPrimitiveManager* RenderSystem::getVisRenderPrimitiveManager() const
	{
		if (m_curStrategyIndex == RS_Deferred)
		{
			return dynamic_cast<DeferredRenderStrategy*>(m_renderStrategy[m_curStrategyIndex])->getVisibilityRenderPrimitiveManager();
		}
		return nullptr;
	}

	RenderStrategyType RenderSystem::getCurrentRenderStrategyType()
	{
		return (RenderStrategyType)m_curStrategyIndex;
	}

	bool RenderSystem::getMainViewPortSize(uint32 & width, uint32 & height)
	{
EchoZoneScoped;

		if(! m_renderStrategy[m_curStrategyIndex])
			return false;

		m_renderStrategy[m_curStrategyIndex]->getMainViewPortSize(width, height);
		return true;
	}

	void RenderSystem::getWindowSize(uint32& width, uint32& height) const
	{
EchoZoneScoped;

		width = m_renderWindow->getTargetWidth();
		height = m_renderWindow->getTargetHeight();
	}

	//=============================================================================
	void RenderSystem::destoryRenderTarget(RenderTarget* pRenderTarget)
	{
EchoZoneScoped;

		RenderTargetMap::iterator itor = m_renderTargets.find(pRenderTarget->getName());
		if (m_renderTargets.end() == itor)
		{
			assert("RenderSystem::destoryRenderTarget");
			return;
		}

		m_currentRTDepthBuffer = nullptr;

		if (m_currentRenderTarget == pRenderTarget)
		{
			m_currentRenderTarget = nullptr;
			m_currentRenderPass = nullptr;
		}

		delete pRenderTarget;
		m_renderTargets.erase(itor);
	}

	//=============================================================================
	void RenderSystem::destroySeperateRenderWindow(RenderTarget* pRenderTarget)
	{
EchoZoneScoped;

		if (pRenderTarget == nullptr)
			return;
		m_currentRTDepthBuffer = nullptr;

		if (m_currentRenderTarget == pRenderTarget)
		{
			m_currentRenderTarget = nullptr;
			m_currentRenderPass = nullptr;
		}

		RcRenderWindow* pRcRenderWindow = (RcRenderWindow*)(pRenderTarget->m_rcRenderTarget);
		//IMPORTANT(yanghang):Not use common RenderTarget Destructor to free low-level RcRenderWindow.
		// Actually this is not only a simple RenderTarget. So set null to the low-level pointer.
		pRenderTarget->m_rcRenderTarget = 0;

		if (pRcRenderWindow)
		{
			const Echo::Name& rtName = pRenderTarget->getName();
			if (m_renderTargets.find(rtName) != m_renderTargets.end())
				m_renderTargets.erase(rtName);
			m_pRenderer->destroySeperateRenderWindow(pRcRenderWindow);
		}

		delete pRenderTarget;
	}

	//=============================================================================
	void RenderSystem::setRenderTarget(RenderTarget* pRenderTarget, const ECHO_CLEAR& clear)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (m_currentRenderTarget)
				m_pRenderer->endRenderTarget(m_currentRenderTarget->m_rcRenderTarget);

			if (!pRenderTarget)
			{
				m_currentRenderTarget = nullptr;
				m_currentRTDepthBuffer = nullptr;
				return;
			}


			RcBeginRenderTarget * pParam = NEW_RcBeginRenderTarget;
			pParam->pRenderTarget = pRenderTarget->m_rcRenderTarget;
			pParam->clear = clear;

			if (Root::instance()->getEnableReverseDepth())
			{
				pParam->clear.fDepth = 1.0f - pParam->clear.fDepth;
			}

			m_pRenderer->beginRenderTarget(pParam);

			m_currentRTDepthBuffer = pRenderTarget->getDepthBuffer();
			m_currentRenderTarget = pRenderTarget;
		}
		else
		{
			if (m_currentRenderPass)
				endRenderPass(m_currentRenderPass);

			if (0 == pRenderTarget)
			{
				m_currentRenderPass = 0;
				m_currentRTDepthBuffer = 0;
				return;
			}

			unsigned int width, height;
			pRenderTarget->getTargetWidthAndHeight(width, height);

			m_currentRTDepthBuffer = pRenderTarget->getDepthBuffer();

			m_currentRenderPass = pRenderTarget->m_gxRenderPass;

			beginRenderPass(m_currentRenderPass, width, height, &clear);
		}
	}

	void RenderSystem::clearupRenderTargets()
	{
EchoZoneScoped;

		RenderTargetMap::iterator itb, ite;
		itb = m_renderTargets.begin();
		ite = m_renderTargets.end();
		for (; itb != ite; ++itb)
		{
			delete itb->second;
		}
		m_renderTargets.clear();
	}


	RenderTarget* RenderSystem::getRenderTarget(const Name& name)
	{
EchoZoneScoped;

		RenderTargetMap::iterator it = m_renderTargets.find(name);
		if (it != m_renderTargets.end())
		{
			return it->second;
		}

		return nullptr;
	}

	void RenderSystem::resetRTDepthBuffer(RenderTarget* pRT, DepthBuffer* pDepthBuffer) const
	{
EchoZoneScoped;

		if (!pRT)
		{
			return;
		}

		RcResetRTDepthStencil *pParam = NEW_RcResetRTDepthStencil;
		pParam->pRT = pRT->m_rcRenderTarget;
		if (pDepthBuffer != nullptr)
		{
			pParam->pNewDepthStencil = pDepthBuffer->m_rcDepthStencil;
		}
		else
		{
			pParam->pNewDepthStencil = nullptr;
		}

		m_pRenderer->resetRTDepthStencil(pParam);
		pRT->resetDepthBuffer(pDepthBuffer);
		pRT->setSharedDepth(true);
	}

	//=============================================================================
	DepthBuffer* RenderSystem::createDepthBuffer(const Name& name, uint32 width, uint32 height, ECHO_FMT format)
	{
EchoZoneScoped;

		DepthBuffer* pDepthBuffer = new DepthBuffer(name, width, height, format);

		m_depthBuffers.push_back(pDepthBuffer);

		return pDepthBuffer;
	}

	//=============================================================================
	void RenderSystem::destoryDepthBuffer(DepthBuffer* pDepthBuffer)
	{
EchoZoneScoped;

		if (!pDepthBuffer)
			return;

		std::vector<DepthBuffer*>::iterator itor = std::find(m_depthBuffers.begin(), m_depthBuffers.end(), pDepthBuffer);
		if (m_depthBuffers.end() == itor)
		{
			assert("RenderSystem::destoryDepthBuffer");
			return;
		}

		delete pDepthBuffer;
		m_depthBuffers.erase(itor);
	}

	void RenderSystem::clearupDepthBuffers()
	{
EchoZoneScoped;

		std::vector<DepthBuffer*>::iterator itb, ite;
		itb = m_depthBuffers.begin();
		ite = m_depthBuffers.end();
		for (; itb != ite; ++itb)
		{
			delete *itb;
		}
		m_depthBuffers.clear();
	}

	void RenderSystem::setMainRenderTargetScale(float scale)
	{
		EchoZoneScoped;

		if (fabs(m_mainRTScale - scale) <= 0.000001f)
			return;

		m_mainRTScale = scale;

		if (m_selectedDLSSQueryQuality != ECHO_DLSS_PERF_QUALITY_DISABLE)
		{
			queryDLSSOptimal();
			resetDLSSPerfQuality(m_selectedDLSSQueryQuality);
			return;
		}

		if (m_renderStrategy[m_curStrategyIndex])
		{
			m_renderStrategy[m_curStrategyIndex]->setMainRenderTargetScale(scale);
		}
	}

	float RenderSystem::getMainRenderTargetScale() const
	{
		EchoZoneScoped;

		return m_mainRTScale;
	}

	void RenderSystem::setViewportScale(float scale)
	{
		EchoZoneScoped;

		if (fabs(m_viewportScale - scale) <= 0.000001f)
			return;

		m_viewportScale = scale;

		if (m_renderStrategy[m_curStrategyIndex])
		{
			m_renderStrategy[m_curStrategyIndex]->setViewportScale(scale);
		}
	}

	float RenderSystem::getViewportScale() const
	{
		EchoZoneScoped;

		return m_viewportScale;
	}

	uint32_t RenderSystem::getRenderWindowHeight() const
	{
		EchoZoneScoped;
		if (m_renderStrategy[m_curStrategyIndex])
		{
			return m_renderStrategy[m_curStrategyIndex]->getRenderWindowHeight();
		}
		return 0;
	}

	namespace
	{
		struct RenderCameraInfo
		{
			ProjectionType projType;
			Vector3 pos;
			Quaternion qua;
			Radian fOVy;
			float cullingFactor;
			float farDist;
			float nearDist;
			float aspect;
			float viewPortRangeX;
			float viewPortRangeY;
		};

		void onBackupCameraInfo(const Camera * pCamera, RenderCameraInfo & cameraInfo)
		{
EchoZoneScoped;

			cameraInfo.projType = pCamera->getProjectionType();
			cameraInfo.pos = pCamera->getPosition();
			cameraInfo.qua = pCamera->getOrientation();
			cameraInfo.fOVy = pCamera->getFOVy();
			cameraInfo.cullingFactor = pCamera->getCullingFactor();
			cameraInfo.farDist = pCamera->getFarClipDistance();
			cameraInfo.nearDist = pCamera->getNearClipDistance();
			cameraInfo.aspect = pCamera->getAspectRatio();
		}

		void onCopyCameraInfo(const Camera * pSrcCamera, Camera * pDesCamera)
		{
EchoZoneScoped;

			pDesCamera->setProjectionType(pSrcCamera->getProjectionType());
			pDesCamera->setPosition(pSrcCamera->getPosition());
			pDesCamera->setOrientation(pSrcCamera->getOrientation());
			pDesCamera->setFOVy(pSrcCamera->getFOVy());
			pDesCamera->setCullingFactor(pSrcCamera->getCullingFactor());
			pDesCamera->setFarClipDistance(pSrcCamera->getFarClipDistance());
			pDesCamera->setNearClipDistance(pSrcCamera->getNearClipDistance());
			pDesCamera->setAspectRatio(pSrcCamera->getAspectRatio());
		}

		void onCopyCameraInfo(const RenderCameraInfo & cameraInfo, Camera * pDesCamera)
		{
EchoZoneScoped;

			pDesCamera->setProjectionType(cameraInfo.projType);
			pDesCamera->setPosition(cameraInfo.pos);
			pDesCamera->setOrientation(cameraInfo.qua);
			pDesCamera->setFOVy(cameraInfo.fOVy);
			pDesCamera->setCullingFactor(cameraInfo.cullingFactor);
			pDesCamera->setFarClipDistance(cameraInfo.farDist);
			pDesCamera->setNearClipDistance(cameraInfo.nearDist);
			pDesCamera->setAspectRatio(cameraInfo.aspect);
		}
	}
	//=============================================================================
	void RenderSystem::_updateAllRenderTargets()
	{
EchoZoneScoped;
		Root* root = Root::instance();
		//std::unordered_map<std::string, std::tuple<uint64, uint32, uint32>> RenderTargetSet;
		m_updatedRenderTargetSet.clear();
		if (beginFranme())
		{
			Root * pRoot = Root::instance();

			if (pRoot->m_bActiveRT)
			{
				RenderTargetMap::iterator itor = m_renderTargets.begin();
				RenderTargetMap::iterator end = m_renderTargets.end();

				for (; itor != end; ++itor)
				{
					RenderTarget* pRenderTarget = itor->second;
					std::string RenderTarName = pRenderTarget->getName().toString();
					if (pRenderTarget->isAutoUpdate() && pRenderTarget->isActive())
					{
						uint32 BeforeCurRTDrawcall = root->getDrawCount();
						uint32 BeforeCurRTTriangle = root->getTriangleCount();
						uint64 CurrentRenderTarTime = root->getWallTime();
						//std::string RenderTarName = pRenderTarget->getName().toString();
						pRenderTarget->new_update();
						uint64 deltaTime = root->getWallTime() - CurrentRenderTarTime;
						uint32 deltaDC = root->getDrawCount() - BeforeCurRTDrawcall;
						uint32 deltaTC= root->getTriangleCount() - BeforeCurRTTriangle;
						m_updatedRenderTargetSet.insert({ RenderTarName, std::make_tuple(deltaTime, deltaDC, deltaTC) });
					}
					/*else {
						RenderTargetSet.insert({ RenderTarName, 0 });
					}*/
				}
			}
			//{
			//	//ceshi
			//	auto profileMgr = ProfilerInfoManager::instance();
			//	auto& frameData = profileMgr->GetCurrentFrameData()->profilerInfo;
			//	std::stringstream ss;
			//	bool first = true;

			//	for (const auto& [key, value] : m_updatedRenderTargetSet) {
			//		if (!first) ss << ","; // Separate entries with commas
			//		first = false;

			//		// Access the tuple
			//		uint64_t a;
			//		uint32_t b, c;
			//		std::tie(a, b, c) = value;

			//		// Append to string stream
			//		ss << key << "T:" << a << " D:" << b << " C:" << c;
			//	}

			//	std::string result = ss.str();
			//	frameData.AllRenderTargetList = result;
			//	//uint64 deltaTime = root->getWallTime() - CurrentPrenderTarTime;
			//	//frameData.RootRenderOneFrameStartTime += deltaTime;
			//}


			Camera * pMainCamera = pRoot->getMainSceneManager()->getMainCamera();
			if (pRoot->m_bMultiCameraRendering && Root::eRM_Editor == pRoot->getRuntimeMode() )
			{
				Camera * pSecondMainCamera = pRoot->getMainSceneManager()->getSecondMainCamera();

				//先备份主相机参数&viewPortRange
				RenderCameraInfo cameraInfo;
				onBackupCameraInfo(pMainCamera, cameraInfo);
				m_renderStrategy[m_curStrategyIndex]->getMainViewPortRange(cameraInfo.viewPortRangeX, cameraInfo.viewPortRangeY);

				//把secondMainCamera 参数设置给 mainCamera
				onCopyCameraInfo(pSecondMainCamera, pMainCamera);
				pMainCamera->setScenodCameraRendering(true);

				bool viewPortRangeChg = false;
				FloatRect secondWinRect;
				if (pSecondMainCamera->getWindow(secondWinRect.left, secondWinRect.top, secondWinRect.right, secondWinRect.bottom))
				{
					float newRangeX, newRangeY;
					uint32 curWinWidth,curWinHeight;
					getWindowSize(curWinWidth, curWinHeight);
					newRangeX = Math::Clamp(secondWinRect.width() / curWinWidth, 0.0f, 1.0f);
					newRangeY =  Math::Clamp(secondWinRect.height() / curWinHeight, 0.0f, 1.0f);
					m_renderStrategy[m_curStrategyIndex]->setMainViewPortRange(newRangeX, newRangeY);
					viewPortRangeChg = true;
				}

				pMainCamera->setUseJitter(false);
				m_beginRenderStrategy = true;
				m_renderStrategy[m_curStrategyIndex]->new_tick(RT_SecondMain);
				m_beginRenderStrategy = false;

				//还原mainCamera
				onCopyCameraInfo(cameraInfo, pMainCamera);
				pMainCamera->setScenodCameraRendering(false);

				//还原viewportRange
				if (viewPortRangeChg)
				{
					m_renderStrategy[m_curStrategyIndex]->setMainViewPortRange(cameraInfo.viewPortRangeX, cameraInfo.viewPortRangeY);
				}
			}
			//GpuPoolManagerImpl::instance()->updateBuffers();


			//ceshi
			uint32 BeforeCurRTMainDrawcall = root->getDrawCount();
			uint32 BeforeCurRTMaintriangle = root->getTriangleCount();
			uint64 CurrentRTMainTime = root->getWallTime();

			m_beginRenderStrategy = true;
			m_renderStrategy[m_curStrategyIndex]->new_tick(RT_Main);
			m_beginRenderStrategy = false;

			uint64 deltaTime = root->getWallTime() - CurrentRTMainTime;
			uint32 deltaDC = root->getDrawCount() - BeforeCurRTMainDrawcall;
			uint32 deltaTC = root->getTriangleCount() - BeforeCurRTMaintriangle;
			m_updatedRenderTargetSet.insert({ "RT_Main", std::make_tuple(deltaTime, deltaDC, deltaTC) });


			uint32 BeforeCurRTDrawcall = root->getDrawCount();
			uint32 BeforeCurRTTriangle = root->getTriangleCount();
			uint64 CurrentRenderTarTime = root->getWallTime();
			m_renderStrategy[m_curStrategyIndex]->new_renderUI();
			deltaTime = root->getWallTime() - CurrentRenderTarTime;
			deltaDC = root->getDrawCount() - BeforeCurRTDrawcall;
			deltaTC = root->getTriangleCount() - BeforeCurRTTriangle;
			m_updatedRenderTargetSet.insert({ "RT_UI", std::make_tuple(deltaTime, deltaDC, deltaTC) });


			if (m_renderListeners.size() > 0)
			{
				ECHO_CLEAR clear;
				clear.vColor[0] = getClearColor();
				clear.eClearStencil = ECHO_LOAD_KEEP;
				clear.eClearDepth = ECHO_LOAD_KEEP;
				clear.eClearColor[0] = ECHO_LOAD_KEEP;
				setRenderTarget(m_renderWindow, clear);

				if (isUseSingleThreadRender())
				{
					_fireEndFrame();
					setRenderTarget(nullptr, clear);
				}
			}

			endFrame();

			updateJitter();
		}
	}

	void RenderSystem::_fireEndFrame(void)
	{
EchoZoneScoped;

		for (size_t i = 0; i < m_renderListeners.size(); ++i)
		{
			m_renderListeners[i]->OnFrameEnd();
		}
	}

	//=============================================================================
	void RenderSystem::addRenderListener(RenderListener* listener)
	{
EchoZoneScoped;

		// only support d3d9
		if (ECHO_DEVICE_DX9C != m_GraphicParam.eClass && ECHO_DEVICE_DX11 != m_GraphicParam.eClass)
			return;

		if (std::find(m_renderListeners.begin(), m_renderListeners.end(), listener) != m_renderListeners.end())
			return;

		m_renderListeners.push_back(listener);

		if (Root::instance()->isUseRenderCommand() )
		{
			void *pWinHand = m_pRenderer->getRenderLayerUsageInfo(ECHO_WINDOW_HAND);
			if (pWinHand)
			{
				listener->OnInitializeDevice(pWinHand,
					m_pRenderer->getRenderLayerUsageInfo(ECHO_RENDER_DEVICE),
					m_pRenderer->getRenderLayerUsageInfo(ECHO_RENDER_CONTEXT) );
			}
		}
		else
		{
			listener->OnInitializeDevice(m_view->getHandle(), m_device->getDevice(), m_device->getContext());
		}
	}

	//=============================================================================
	void RenderSystem::removeRenderListener(RenderListener* listener)
	{
EchoZoneScoped;

		std::vector<RenderListener*>::iterator itFind = std::find(m_renderListeners.begin(), m_renderListeners.end(), listener);
		if (itFind != m_renderListeners.end())
		{
			m_renderListeners.erase(itFind);
		}
	}
	//=============================================================================
	RcDepthStencil* RenderSystem::createRcDepthStencil(const Name& name, uint32 width, uint32 height, ECHO_FMT _eFmt)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcDepthStencil* pRcDepthStencil = NEW_RcDepthStencil;
			pRcDepthStencil->pDevice = m_pRcDevice;
			pRcDepthStencil->width = width;
			pRcDepthStencil->height = height;
			pRcDepthStencil->format = _eFmt;
			pRcDepthStencil->pName = name.c_str();

			m_pRenderer->createDepthStencil(pRcDepthStencil);
			return pRcDepthStencil;
		}
		else
		{
			RcDepthStencil* pRcDepthStencil = new RcDepthStencil();
			pRcDepthStencil->pNative = m_device->CreateDepthStencil((GX_FMT)_eFmt, width, height);
			return pRcDepthStencil;
		}
	}

	void RenderSystem::destoryRcDepthStencil(RcDepthStencil* pRcDepthStencil)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			m_pRenderer->destroyDepthStencil(pRcDepthStencil);
		}
		else
		{
			IGXDepthStencil* gxDepthStencil = reinterpret_cast<IGXDepthStencil*>(pRcDepthStencil->pNative);
			gxDepthStencil->Release();
			delete pRcDepthStencil;
		}
	}

	RcRenderTarget*	RenderSystem::createRcRenderTarget(const Name& name, uint32 width, uint32 height, ECHO_FMT _eFmt, RcDepthStencil* depthStencil)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcRenderTarget *pRcRenderTarget = NEW_RcRenderTarget;
			pRcRenderTarget->pName = name.c_str();
			pRcRenderTarget->pDevice = m_pRcDevice;
			pRcRenderTarget->width = width;
			pRcRenderTarget->height = height;
			pRcRenderTarget->format = _eFmt;
			pRcRenderTarget->pDepthStencil = depthStencil;

			m_pRenderer->createRenderTarget(pRcRenderTarget);
			return pRcRenderTarget;
		}
		else
		{
			RcRenderTarget* pRcRenderTarget = new RcRenderTarget();
			pRcRenderTarget->pNative = m_device->CreateRenderTarget((GX_FMT)_eFmt, width, height);
			return pRcRenderTarget;
		}
	}

	RcMultiRenderTarget* RenderSystem::createRcMultiRenderTarget(const Name& name, uint32 width, uint32 height,
																 const std::vector<ECHO_FMT>& _eFmts,
																 RcDepthStencil* depthStencil)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

		if (_eFmts.size() > ECHO_MRT_COLOR_COUNT)
		{
			assert(false && "The number of color attachments for RT exceeds the limit!");
			return nullptr;
		}

		RcMultiRenderTarget* pRcRenderTarget = NEW_RcMultiRenderTarget;
		pRcRenderTarget->pName = name.c_str();
		pRcRenderTarget->pDevice = m_pRcDevice;
		pRcRenderTarget->width = width;
		pRcRenderTarget->height = height;
		pRcRenderTarget->pDepthStencil = depthStencil;
		pRcRenderTarget->colorCount = static_cast<uint32_t>(_eFmts.size());
		for (uint32 i = 0; i < pRcRenderTarget->colorCount; i++)
		{
			pRcRenderTarget->colors[i].format = _eFmts[i];
		}

		m_pRenderer->createMultiRenderTarget(pRcRenderTarget);
		return pRcRenderTarget;
	}

	void RenderSystem::destoryRcRenderTarget(RcRenderTarget* pRcRenderTarget)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (ECHO_SAMPLER_RENDER_WINDOW == pRcRenderTarget->type)
			{
				RcRenderWindow * pRcWindow = reinterpret_cast<RcRenderWindow*>(pRcRenderTarget);
				m_pRenderer->destroyRenderWindow(pRcWindow);
			}
			else
			{
				m_pRenderer->destroyRenderTarget(pRcRenderTarget);
			}
		}
		else
		{
			IGXRenderTarget* gxRenderTarget = reinterpret_cast<IGXRenderTarget*>(pRcRenderTarget->pNative);
			gxRenderTarget->Release();
			delete pRcRenderTarget;
		}
	}

	RcComputeTarget* RenderSystem::createRcComputeTarget(uint32 width, uint32 height, ECHO_FMT _eFmt, uint32 mipmap)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcComputeTarget* pRcComputeTarget = NEW_RcComputeTarget;
			pRcComputeTarget->pDevice = m_pRcDevice;
			pRcComputeTarget->width = width;
			pRcComputeTarget->height = height;
			pRcComputeTarget->format = _eFmt;
			pRcComputeTarget->mipmap = mipmap;

			m_pRenderer->createComputeTarget(pRcComputeTarget);
			return pRcComputeTarget;
		}
		else
		{
			/*RcRenderTarget* pRcRenderTarget = new RcRenderTarget();
			pRcRenderTarget->pNative = m_device->CreateRenderTarget((GX_FMT)_eFmt, width, height);
			return pRcRenderTarget;*/
			return nullptr;
		}
	}

	void RenderSystem::destoryRcComputeTarget(RcComputeTarget* pRcComputeTarget)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			m_pRenderer->destroyComputeTarget(pRcComputeTarget);
		}
		else
		{
			/*IGXRenderTarget* gxRenderTarget = reinterpret_cast<IGXRenderTarget*>(pRcRenderTarget->pNative);
			gxRenderTarget->Release();
			delete pRcRenderTarget;*/
		}
	}

	RcTexture* RenderSystem::createTextureRaw(const Name & name, ECHO_FMT _Fmt, uint32 _width, uint32 _height, const void* _pData, uint32 _byteSize, uint32 mipmapLevel, uint32 skipLevel)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}
			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_2D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = skipLevel;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _width;
			pRcTexture->texDesc.height = _height;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = mipmapLevel;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);

			return pRcTexture;
		}
		else
		{
			IGXTex* gxTex = m_device->CreateTextureRaw((GX_FMT)_Fmt, _pData, _width, _height);

			if (nullptr == gxTex)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTextureRaw failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_2D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _width;
			pRcTexture->texDesc.height = _height;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = 1;

			pRcTexture->pNative = gxTex;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createTextureDyn(const Name & name, ECHO_TEX_TARGET textureType, ECHO_FMT _Fmt, uint32 _width, uint32 _height, const void* _pData, uint32 _byteSize, uint32 mip)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = textureType;

			//IMPORTANT(yanghang):D3D11 Texture2D USAGE_DYNAMIC Resource must have MipLevels equal to 1.
			if (mip > 1)
			{
				pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			}
			else
			{
				pRcTexture->usage = ECHO_TEX_USAGE_DYNAMIC;
			}
			
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _width;
			pRcTexture->texDesc.height = _height;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = mip;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXTex* gxTex = m_device->CreateTextureDyn((GX_FMT)_Fmt, _width, _height);
			if (nullptr == gxTex)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTextureDyn failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			if (_pData)
				gxTex->SetData(_pData);

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_2D;
			pRcTexture->usage = ECHO_TEX_USAGE_DYNAMIC;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _width;
			pRcTexture->texDesc.height = _height;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = 1;

			pRcTexture->pNative = gxTex;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createTextureCompressed(const Name & name, const void* _pData, uint32 _byteSize, uint32 _texLevel)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_2D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXTex* pGxTex = m_device->CreateTextureCompressed(_pData, _byteSize, _texLevel);

			if (nullptr == pGxTex)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTextureCompressed failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			const GX_TEX_DESC* pGxdesc = pGxTex->GetDesc();

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_2D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->texDesc.format = (ECHO_FMT)pGxdesc->eFmt;
			pRcTexture->texDesc.width = pGxdesc->nSizeX;
			pRcTexture->texDesc.height = pGxdesc->nSizeY;
			pRcTexture->texDesc.depth = pGxdesc->nSizeZ;
			pRcTexture->texDesc.mipmapLevel = pGxdesc->nMipMap;
			pRcTexture->texDesc.useTextureLvl = pGxdesc->useTextureLvl;

			pRcTexture->pNative = pGxTex;

			return pRcTexture;
		}
	}

	RcTexture *	RenderSystem::createTexture2DArrayRaw(const Name & name, ECHO_FMT _Fmt, uint32 _width, uint32 _height, uint32 _depth, uint32 _mipmapLvl,
		const void* _pData, uint32 _byteSize, uint32 _texLevel)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_2D_ARRAY;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _width;
			pRcTexture->texDesc.height = _height;
			pRcTexture->texDesc.depth = _depth;
			pRcTexture->texDesc.mipmapLevel = _mipmapLvl;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXTex2DArray * pGxTex2DArray = m_device->CreateTexture2DArrayRaw(_pData, _byteSize, (GX_FMT)_Fmt, _width, _height, _depth, _mipmapLvl);
			if (nullptr == pGxTex2DArray)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTexture2DArrayRaw failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			const GX_TEX_DESC * pGxdesc = pGxTex2DArray->GetDesc();

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_2D_ARRAY;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->texDesc.format = (ECHO_FMT)pGxdesc->eFmt;
			pRcTexture->texDesc.width = pGxdesc->nSizeX;
			pRcTexture->texDesc.height = pGxdesc->nSizeY;
			pRcTexture->texDesc.depth = pGxdesc->nSizeZ;
			pRcTexture->texDesc.mipmapLevel = pGxdesc->nMipMap;
			pRcTexture->texDesc.useTextureLvl = pGxdesc->useTextureLvl;

			pRcTexture->pNative = pGxTex2DArray;

			return pRcTexture;
		}
	}

	RcTexture *	RenderSystem::createTexture2DArrayCompressed(const Name & name, const void * _pData, uint32 _byteSize, uint32 _texLevel)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_2D_ARRAY;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXTex2DArray * pGxTex2DArray = m_device->CreateTexture2DArrayCompressed(_pData, _byteSize, _texLevel);

			if (nullptr == pGxTex2DArray)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTexture2DArrayCompressed failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			const GX_TEX_DESC * pGxdesc = pGxTex2DArray->GetDesc();

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_2D_ARRAY;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->texDesc.format = (ECHO_FMT)pGxdesc->eFmt;
			pRcTexture->texDesc.width = pGxdesc->nSizeX;
			pRcTexture->texDesc.height = pGxdesc->nSizeY;
			pRcTexture->texDesc.depth = pGxdesc->nSizeZ;
			pRcTexture->texDesc.mipmapLevel = pGxdesc->nMipMap;
			pRcTexture->texDesc.useTextureLvl = pGxdesc->useTextureLvl;

			pRcTexture->pNative = pGxTex2DArray;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createCubeRaw(const Name & name, ECHO_FMT _Fmt, uint32 _widthHeight, const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = 6;
			pRcTexture->texDesc.mipmapLevel = 1;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXCube* pGxCube = m_device->CreateCubeRaw((GX_FMT)_Fmt, _pData, _widthHeight);

			if (nullptr == pGxCube)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createCubeRaw failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = 1;

			pRcTexture->pNative = pGxCube;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createCubeDyn(const Name & name, ECHO_FMT _Fmt, uint32 _widthHeight, int mip, const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}
			
			RcTexture* pRcTexture = nullptr;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_DYNAMIC;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = 6;
			pRcTexture->texDesc.mipmapLevel = mip;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXCube* pGxCube = m_device->CreateCubeDyn((GX_FMT)_Fmt, _widthHeight);
			if (nullptr == pGxCube)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createCubeDyn failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_DYNAMIC;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = 1;
			pRcTexture->texDesc.mipmapLevel = 1;

			pRcTexture->pNative = pGxCube;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createCubeCompressed(const Name&name, const void* _pData, uint32 _byteSize, uint32 _texLevel)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXCube* pGxCubeTex = m_device->CreateCubeCompressed(_pData, _byteSize, _texLevel);

			if (nullptr == pGxCubeTex)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createCubeCompressed failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			const GX_TEX_DESC * pGxdesc = pGxCubeTex->GetDesc();

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_CUBE_MAP;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_COMPRESSED;
			pRcTexture->texLevel = _texLevel;
			pRcTexture->texDesc.format = (ECHO_FMT)pGxdesc->eFmt;
			pRcTexture->texDesc.width = pGxdesc->nSizeX;
			pRcTexture->texDesc.height = pGxdesc->nSizeY;
			pRcTexture->texDesc.depth = pGxdesc->nSizeZ;
			pRcTexture->texDesc.mipmapLevel = pGxdesc->nMipMap;
			pRcTexture->texDesc.useTextureLvl = pGxdesc->useTextureLvl;

			pRcTexture->pNative = pGxCubeTex;

			return pRcTexture;
		}
	}

	RcTexture* RenderSystem::createTex3DRaw(const Name & name, ECHO_FMT _Fmt, uint32 _widthHeight, const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcTexture* pRcTexture = nullptr;
			if (createInResThread)
			{
				pRcTexture = NEW_RcTextureRT;
				pRcTexture->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcTexture = NEW_RcTexture;
				pRcTexture->pCustom = &s_CreateInWorkThread;
			}

			pRcTexture->pDevice = m_pRcDevice;
			pRcTexture->target = ECHO_TEX_TARGET_3D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = _widthHeight;
			pRcTexture->texDesc.mipmapLevel = 1;
			pRcTexture->pData = pTexData;
			pRcTexture->dataSize = _byteSize;
			pRcTexture->dataFreeMethod = method;
			pRcTexture->pName = name.c_str();

			m_pRenderer->createTexture(pRcTexture);
			return pRcTexture;
		}
		else
		{
			IGXTex3D* pGxTex3D = m_device->CreateTex3DRaw((GX_FMT)_Fmt, _pData, _widthHeight);

			if (nullptr == pGxTex3D)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createTex3DRaw failed! " + name.toString(), LML_CRITICAL);
				return nullptr;
			}

			RcTexture* pRcTexture = new RcTexture();
			pRcTexture->target = ECHO_TEX_TARGET_3D;
			pRcTexture->usage = ECHO_TEX_USAGE_STATIC_RAW;
			pRcTexture->texLevel = 0;
			pRcTexture->texDesc.format = _Fmt;
			pRcTexture->texDesc.width = _widthHeight;
			pRcTexture->texDesc.height = _widthHeight;
			pRcTexture->texDesc.depth = _widthHeight;
			pRcTexture->texDesc.mipmapLevel = 1;

			pRcTexture->pNative = pGxTex3D;

			return pRcTexture;
		}
	}

	void RenderSystem::destoryTexture(RcTexture* pRcTex)
	{
EchoZoneScoped;

		if (!pRcTex)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN | DEF_THREAD_TYPE_RESOUCE);

			bool bCreateInResourceThread = false;
			if (pRcTex->pCustom)
				bCreateInResourceThread = *reinterpret_cast<bool*>(pRcTex->pCustom);
			if (m_workThreadID == _getCurrentThreadID())
			{
				if (bCreateInResourceThread)
				{
					if (pRcTex->pNative)
					{
						// 投递到资源线程中销毁
						m_delayedDestoryRcTextureList.push_back(pRcTex);
					}
					else
					{
						m_pRenderer->destroyTexture(pRcTex);
					}
				}
				else
				{
					m_pRenderer->destroyTexture(pRcTex);
				}
			}
			else
			{
				m_pRenderer->destroyTexture(pRcTex);
			}
		}
		else
		{
			switch (pRcTex->target)
			{
			case ECHO_TEX_TARGET_2D:
			{
				IGXTex* pGxTex = reinterpret_cast<IGXTex*>(pRcTex->pNative);
				if (pGxTex)
					pGxTex->Release();
			}
			break;
			case ECHO_TEX_TARGET_2D_ARRAY:
			{
				IGXTex2DArray * pGxTex = reinterpret_cast<IGXTex2DArray*>(pRcTex->pNative);
				if (pGxTex)
					pGxTex->Release();
			}
			break;
			case ECHO_TEX_TARGET_CUBE_MAP:
			{
				IGXCube* pGxCube = reinterpret_cast<IGXCube*>(pRcTex->pNative);
				if (pGxCube)
					pGxCube->Release();
			}
			break;
			case ECHO_TEX_TARGET_3D:
			{
				IGXTex3D* pGxTex = reinterpret_cast<IGXTex3D*>(pRcTex->pNative);
				if (pGxTex)
					pGxTex->Release();
			}
			break;
			default:
				break;
			}

			delete pRcTex;
		}
	}

	void RenderSystem::destoryRcTextures()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			return;

		if (!m_destoryRcTextureList.empty())
		{
			TextureManager::instance()->addRequest(m_destoryRcTextureList);
		}
		m_destoryRcTextureList.clear();

		m_destoryRcTextureList.swap(m_delayedDestoryRcTextureList);
	}

	void RenderSystem::updateTexture(RcTexture* pRcTex, const void* _pData, uint32 _byteSize, const ECHO_RECT& _pRect, uint32 _layerIndex, uint32 _layerCount, bool isCompressed,
		bool _dataSupportAsyn, uint32 srcLevelOffset, uint32 dstLevelOffset, uint32 numUpdateLevels)
	{
EchoZoneScoped;

		if (!pRcTex || !_pData || _byteSize == 0)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void * pTexData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0) //if (!_dataSupportAsyn) //data不支持异步时，需要重新申请内存
			{
				pTexData = malloc(_byteSize);
				memcpy(pTexData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcUpdateTexture * pRcParam = NEW_RcUpdateTexture;
			pRcParam->pTexture = pRcTex;
			pRcParam->pUpdateData = pTexData;
			pRcParam->updateDataSize = _byteSize;
			pRcParam->updateDataFreeMethod = method;
			pRcParam->rect = _pRect;
			pRcParam->layerStart = _layerIndex;
			pRcParam->layerCount = _layerCount;
			pRcParam->isCompressed = isCompressed;
			pRcParam->srcLevelOffset = srcLevelOffset;
			pRcParam->dstLevelOffset = dstLevelOffset;
			pRcParam->numUpdateLevels = numUpdateLevels;

			m_pRenderer->updateTexture(pRcParam);
		}
		else
		{
			switch (pRcTex->target)
			{
			case ECHO_TEX_TARGET_2D:
			{
				GX_RECT gxRect(_pRect.x, _pRect.y, _pRect.dx, _pRect.dy);
				IGXTex* pGxTex = reinterpret_cast<IGXTex*>(pRcTex->pNative);
				if (pGxTex)
					pGxTex->SetSubData(_pData, &gxRect);
			}
			break;
			case ECHO_TEX_TARGET_2D_ARRAY:
			{
				IGXTex2DArray * pGxTex = reinterpret_cast<IGXTex2DArray*>(pRcTex->pNative);
				if (!pGxTex)
					return;

				if (isCompressed)
					pGxTex->setData(_pData, _byteSize, _layerIndex);
				else
					pGxTex->setRawData(_pData, _layerIndex);
			}
			break;
			case ECHO_TEX_TARGET_CUBE_MAP:
			{
				IGXCube* pGxCube = reinterpret_cast<IGXCube*>(pRcTex->pNative);
				if (pGxCube)
					pGxCube->SetData(_pData);
			}
			break;
			default:
				break;
			}
		}
	}

	void RenderSystem::copyTexture(RcTexture* pSrcTex, RcTexture* pDstTex, const ECHO_EXTENT3D& extent, uint32 srcMipLevel, uint32 dstMipLevel, uint32 mipLevelCount, uint32 srcBaseArrayLayer, uint32 dstBaseArrayLayer, uint32 layerCount)
	{
EchoZoneScoped;

		if (!pSrcTex || !pDstTex)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcCopyTexture* pRcParam = NEW_RcCopyTexture;
			pRcParam->pSrcTexture = pSrcTex;
			pRcParam->pDstTexture = pDstTex;
			pRcParam->srcMipLevel = srcMipLevel;
			pRcParam->dstMipLevel = dstMipLevel;
			pRcParam->mipLevelCount = mipLevelCount;
			pRcParam->srcBaseArrayLayer = srcBaseArrayLayer;
			pRcParam->dstBaseArrayLayer = dstBaseArrayLayer;
			pRcParam->layerCount = layerCount;
			pRcParam->extent = extent;

			m_pRenderer->copyTexture(pRcParam);
		}
	}

	RcBuffer* RenderSystem::createStaticVB(const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (0 == _byteSize)
			return nullptr;

		m_staticVBIBTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = nullptr;
			if (createInResThread)
			{
				pRcBuffer = NEW_RcBufferRT;
				pRcBuffer->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcBuffer = NEW_RcBuffer;
				pRcBuffer->pCustom = &s_CreateInWorkThread;
			}

			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			IGXStaticVB *pGxVB = m_device->CreateStaticVB(_pData, _byteSize);

			if (nullptr == pGxVB)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createStaticVB failed! ", LML_CRITICAL);
				return nullptr;
			}

			RcBuffer * pRcBuffer = new RcBuffer();
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->pNative = pGxVB;

			return pRcBuffer;
		}
	}

	RcBuffer* RenderSystem::createDynamicVB(const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (0 == _byteSize)
			return nullptr;

		m_dynamicVBIBTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void * pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer * pRcBuffer = NEW_RcBuffer;
			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			IGXDynamicVB *pGxVB = m_device->CreateDynamicVB(_byteSize);

			if (nullptr == pGxVB)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createDynamicVB failed! ", LML_CRITICAL);
				return nullptr;
			}

			if (_pData)
				pGxVB->SetData(_pData, _byteSize);

			RcBuffer * pRcBuffer = new RcBuffer();
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->pNative = pGxVB;

			return pRcBuffer;
		}
	}

	RcBuffer* RenderSystem::createStaticIB(const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (0 == _byteSize)
			return nullptr;

		m_staticVBIBTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void * pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = nullptr;
			if (createInResThread)
			{
				pRcBuffer = NEW_RcBufferRT;
				pRcBuffer->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcBuffer = NEW_RcBuffer;
				pRcBuffer->pCustom = &s_CreateInWorkThread;
			}

			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_INDEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			IGXStaticIB *pGxIB = m_device->CreateStaticIB((unsigned short*)_pData, _byteSize);

			if (nullptr == pGxIB)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createStaticIB failed! ", LML_CRITICAL);
				return nullptr;
			}

			RcBuffer * pRcBuffer = new RcBuffer();
			pRcBuffer->type = ECHO_BUFFER_INDEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->pNative = pGxIB;

			return pRcBuffer;
		}
	}


	RcBuffer* RenderSystem::createDynamicIB(const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		if (0 == _byteSize)
			return nullptr;

		m_dynamicVBIBTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void * pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer * pRcBuffer = NEW_RcBuffer;
			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_INDEX;
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			IGXDynamicIB *pGxIB = m_device->CreateDynamicIB(_byteSize);

			if (nullptr == pGxIB)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createDynamicIB failed! ", LML_CRITICAL);
				return nullptr;
			}

			if (_pData)
				pGxIB->SetData(_pData, _byteSize);

			RcBuffer * pRcBuffer = new RcBuffer();
			pRcBuffer->type = ECHO_BUFFER_INDEX;
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->pNative = pGxIB;

			return pRcBuffer;
		}
	}

	RcBuffer* RenderSystem::createDynamicSB(const void* _pData, uint32 _stride, uint32 _byteSize, bool forGpuPool)
	{
EchoZoneScoped;

		if (0 == _byteSize)
			return nullptr;

		m_structBufferTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void* pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = nullptr;
			if (createInResThread)
			{
				pRcBuffer = NEW_RcBufferRT;
				pRcBuffer->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcBuffer = NEW_RcBuffer;
				pRcBuffer->pCustom = &s_CreateInWorkThread;
			}

			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_STRUCTED;
			if (forGpuPool && Root::instance()->getDeviceType() == ECHO_DEVICE_VULKAN)
			{
				pRcBuffer->type = ECHO_BUFFER_STRUCTED_GPU_POOL;
			}
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->stride = _stride;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			return nullptr;
		}
	}

	RcBuffer* RenderSystem::createDefaultSB(const void* _pData, uint32 _stride, uint32 _byteSize)
	{
EchoZoneScoped;

		m_structBufferTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void* pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = NEW_RcBuffer;
			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_STRUCTED;
			pRcBuffer->usage = ECHO_BUFFER_AUTO;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->stride = _stride;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			return nullptr;
		}
	}

	/*RcBuffer* RenderSystem::createDynamicRB(const void* _pData, uint32 _byteSize)
	{
		if (0 == _byteSize)
			return nullptr;

		m_structReadBufferTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void* pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = NEW_RcBuffer;
			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_RAW;
			pRcBuffer->usage = ECHO_BUFFER_DYNAMIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->stride = 0;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			return nullptr;
		}
	}*/

	RcBuffer* RenderSystem::createDefaultRB(const void* _pData, uint32 _byteSize)
	{
EchoZoneScoped;

		m_structBufferTotalSize += _byteSize;
		if (Root::instance()->isUseRenderCommand())
		{
			bool createInResThread = false;
			if (m_workThreadID != _getCurrentThreadID() && Root::instance()->m_bCreateBufferInResourceThread)
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_RESOUCE);
				createInResThread = true;
			}
			else
			{
				DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);
			}

			void* pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && !createInResThread && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer;
			if (createInResThread)
			{
				pRcBuffer = NEW_RcBufferRT;
				pRcBuffer->pCustom = &s_CreateInResourceThread;
			}
			else
			{
				pRcBuffer = NEW_RcBuffer;
				pRcBuffer->pCustom = &s_CreateInWorkThread;
			}

			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_RAW;
			pRcBuffer->usage = ECHO_BUFFER_AUTO;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->stride = 0;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			return nullptr;
		}
	}

	void RenderSystem::destoryBuffer(RcBuffer* pRcBuffer)
	{
EchoZoneScoped;

		if (!pRcBuffer)
			return;

		if (ECHO_BUFFER_VERTEX == pRcBuffer->type || ECHO_BUFFER_INDEX == pRcBuffer->type)
		{
			if (pRcBuffer->usage == ECHO_BUFFER_STATIC)
				m_staticVBIBTotalSize -= pRcBuffer->dataSize;
			else if (pRcBuffer->usage == ECHO_BUFFER_DYNAMIC)
				m_dynamicVBIBTotalSize -= pRcBuffer->dataSize;
		}
		else if (ECHO_BUFFER_UNIFORM == pRcBuffer->type)
		{
			m_uniformBufferTotalSize -= pRcBuffer->dataSize;
		}
		else if (ECHO_BUFFER_STRUCTED == pRcBuffer->type)
		{
			m_structBufferTotalSize -= pRcBuffer->dataSize;
		}
		else if (ECHO_BUFFER_RAW == pRcBuffer->type)
		{
			m_structBufferTotalSize -= pRcBuffer->dataSize;
		}

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN | DEF_THREAD_TYPE_RESOUCE);

			bool bCreateInResourceThread = false;
			if (pRcBuffer->pCustom)
				bCreateInResourceThread = *reinterpret_cast<bool*>(pRcBuffer->pCustom);

			if (m_workThreadID == _getCurrentThreadID())
			{
				if (bCreateInResourceThread)
				{
					if (pRcBuffer->pNative)
					{
						// 投递到资源线程中销毁
						m_delayedDestoryRcBufferList.push_back(pRcBuffer);
					}
					else
					{
						m_pRenderer->destroyBuffer(pRcBuffer);
					}
				}
				else
				{
					m_pRenderer->destroyBuffer(pRcBuffer);
				}
			}
			else
			{
				if (!bCreateInResourceThread)
				{
					assert(false && "RenderSystem::destoryBuffer");
					LogManager::getSingleton().logMessage("-warn-\tThere's an anomaly at RenderSystem::destoryBuffer", LML_CRITICAL);
				}
				m_pRenderer->destroyBuffer(pRcBuffer);
			}
		}
		else
		{
			switch (pRcBuffer->type)
			{
			case ECHO_BUFFER_VERTEX:
			{
				IGXBaseVB* pGxVB = reinterpret_cast<IGXBaseVB*>(pRcBuffer->pNative);
				if (pGxVB)
					pGxVB->Release();
			}
			break;
			case ECHO_BUFFER_INDEX:
			{
				IGXBaseIB * pGxIB = reinterpret_cast<IGXBaseIB*>(pRcBuffer->pNative);
				if (pGxIB)
					pGxIB->Release();
			}
			break;
			default:
				break;
			}

			delete pRcBuffer;
		}
	}

	void RenderSystem::destoryRcBuffers()
	{
EchoZoneScoped;

		if (!Root::instance()->isUseRenderCommand())
			return;

		if (!m_destoryRcBufferList.empty())
		{
			BufferManager::instance()->addRequest(m_destoryRcBufferList);
		}
		m_destoryRcBufferList.clear();
		m_destoryRcBufferList.swap(m_delayedDestoryRcBufferList);
	}

	void RenderSystem::updateBuffer(RcBuffer* pRcBuffer, const void* _pData, uint32 _byteSize, bool _dataSupportAsyn, uint32 _offset, bool updateInCQueue)
	{
EchoZoneScoped;

		if (!pRcBuffer || !_pData || _byteSize == 0)
			return;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void * pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = Rc_Malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_RC_FREE;
			}

			RcUpdateBuffer * pRcParam = NEW_RcUpdateBuffer;
			if (updateInCQueue)
			{
				pRcParam->pCustom = (void*)s_UpdateInComputeQueue;
			}
			pRcParam->pBuffer = pRcBuffer;
			pRcParam->pUpdateData = pBufferData;
			pRcParam->updateDataSize = _byteSize;
			pRcParam->updateDataFreeMethod = method;
			pRcParam->updateOffset = _offset;

			m_pRenderer->updateBuffer(pRcParam);
		}
		else
		{
			if (pRcBuffer->type == ECHO_BUFFER_VERTEX &&
				pRcBuffer->usage == ECHO_BUFFER_DYNAMIC )
			{
				IGXDynamicVB* pGxVB = reinterpret_cast<IGXDynamicVB*>(pRcBuffer->pNative);
				if (pGxVB)
					pGxVB->SetData(_pData, _byteSize);
			}
			else if ( pRcBuffer->type == ECHO_BUFFER_INDEX &&
					  pRcBuffer->usage == ECHO_BUFFER_DYNAMIC )
			{
				IGXDynamicIB* pGxIB = reinterpret_cast<IGXDynamicIB*>(pRcBuffer->pNative);
				if (pGxIB)
					pGxIB->SetData(_pData, _byteSize);
			}
		}
	}

	RcDrawData*	RenderSystem::createDrawData(Renderable* rend, const Pass* pass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcRenderPipeline* pPipeline = reinterpret_cast<RcRenderPipeline*>(pass->mDest);
			if (!pPipeline)
				return nullptr;

			RcUniformBlock* pUniformDefault = NEW_RcUniformBlock;
			pUniformDefault->pDevice = m_pRcDevice;
			pUniformDefault->type = ECHO_BUFFER_UNIFORM;
			pUniformDefault->usage = ECHO_BUFFER_DYNAMIC;
			pUniformDefault->blockUsage = ECHO_UNIFORM_BLOCK_DEFAULT;
			pUniformDefault->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_DEFAULT];
			pUniformDefault->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_DEFAULT];
			pUniformDefault->pData = nullptr;


			RcUniformBlock* pUniformPrivateVs = NEW_RcUniformBlock;
			pUniformPrivateVs->pDevice = m_pRcDevice;
			pUniformPrivateVs->type = ECHO_BUFFER_UNIFORM;
			pUniformPrivateVs->usage = ECHO_BUFFER_DYNAMIC;
			pUniformPrivateVs->blockUsage = ECHO_UNIFORM_BLOCK_PRIVATE_VS;
			pUniformPrivateVs->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_PRIVATE_VS];
			pUniformPrivateVs->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_PRIVATE_VS];
			pUniformPrivateVs->pData = nullptr;


			RcUniformBlock* pUniformPrivatePs = NEW_RcUniformBlock;
			pUniformPrivatePs->pDevice = m_pRcDevice;
			pUniformPrivatePs->type = ECHO_BUFFER_UNIFORM;
			pUniformPrivatePs->usage = ECHO_BUFFER_DYNAMIC;
			pUniformPrivatePs->blockUsage = ECHO_UNIFORM_BLOCK_PRIVATE_PS;
			pUniformPrivatePs->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_PRIVATE_PS];
			pUniformPrivatePs->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_PRIVATE_PS];
			pUniformPrivatePs->pData = nullptr;


			RcDrawData* pDrawData = NEW_RcDrawData;

			pDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_COMMON_VS] = getUniformBlockCommonVS();
			pDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_COMMON_PS] = getUniformBlockCommonPS();
			pDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_VS] = pUniformPrivateVs;
			pDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_PS] = pUniformPrivatePs;
			pDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_DEFAULT] = pUniformDefault;
			pDrawData->pRenderPipeline = pPipeline;

			RenderOperation* op = rend->getRenderOperation();
			pDrawData->drawMode = op->mPrimitiveType;

			rend->setDrawData(pass, pDrawData);

			m_pRenderer->bindDrawData(pDrawData);

// 			bool bCommitOk = GpuParamsManager::instance()->commitUniformParams(rend, pass);
//
// 			if (!bCommitOk)
// 				return nullptr;
//
// 			GpuParamsManager::instance()->commitSamplerParams(rend, pass);
//
// 			if (rend->hasCustomParameters())
// 			{
// 				rend->setCustomParameters(pass);
// 			}
//
// 			if (pass->hasCustomParam())
// 			{
// 				Vector4 param;
// 				uint32 index = 0;
// 				pass->getCustomParam(param, index);
// 				setUniformValue(rend, pass, index, param);
// 			}

			return pDrawData;
		}

		return nullptr;
	}

	RcUpdateDrawData* RenderSystem::getUpdateDrawData(Renderable* rend, const Pass* pass, RcDrawData* pRcDrawData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcUpdateDrawData *pUpdateDrawData = NEW_RcUpdateDrawData;

			pUpdateDrawData->pDrawData = pRcDrawData;

			RcRenderPipeline* pRenderPipeline = reinterpret_cast<RcRenderPipeline*>(pass->mDest);
			const ECHO_IA_DESC& IADesc = pRenderPipeline->desc.IADesc;

			RenderOperation* op = rend->getRenderOperation();
			IndexBuffInfo indexInfo;
			rend->getIndexBuffInfo(indexInfo);
			const RenderOperation::st_MeshData& opMeshData = op->GetCurMeshData();
			uint32 bufferCnt = (uint32)opMeshData.m_vertexBuffer.size();
			pUpdateDrawData->vertexBufferCount = 0;
			for (uint32 i = 0, j = 0; i < bufferCnt && i < ECHO_IA_MAX_STREAM; ++i)
			{
				if (opMeshData.m_vertexBuffer[i])
				{
					pUpdateDrawData->pVertexBuffer[j++] = opMeshData.m_vertexBuffer[i];
					pUpdateDrawData->vertexBufferCount += 1;
				}
			}

			//纠正一下vertexBufferCount
			if (pUpdateDrawData->vertexBufferCount >= IADesc.nStreamCount)
			{
				pUpdateDrawData->vertexBufferCount = IADesc.nStreamCount;
			}
			else
			{
				assert(pUpdateDrawData->vertexBufferCount < IADesc.nStreamCount);
				return nullptr;
			}

			//copy SamplerInfo
			for (uint32 j = 0; j < ECHO_MAX_SAMPLER_COUNT; j++)
			{
				pUpdateDrawData->updateSamplerInfo[j] = pRcDrawData->drawSamplerInfo[j];
				if (pRcDrawData->drawSamplerInfo[j].nCount > 1)
				{
					if (!pUpdateDrawData->pUpdateData)
					{
						pUpdateDrawData->pUpdateData = pUpdateDrawData->updateSamplerInfo[j].pInfos;
						pUpdateDrawData->updateDataFreeMethod = ECHO_MEMORY_RC_FREE;
					}
					else
					{
						assert("");
					}

					pRcDrawData->drawSamplerInfo[j].pInfos = nullptr;
					pRcDrawData->drawSamplerInfo[j].nCount = 1;
				}
			}

			//copy ImageInfo
			for (uint32 j = 0; j < ECHO_MAX_IMAGE_COUNT; j++)
			{
				pUpdateDrawData->updateImageInfo[j] = pRcDrawData->drawImageInfo[j];
			}

			//copy BufferInfo
			for (uint32 j = 0; j < ECHO_MAX_STRUCT_BUFFER_COUNT; j++)
			{
				pUpdateDrawData->updateBufferInfo[j] = pRcDrawData->structBufferInfo[j];
			}

			float uiscale = getCurRenderStrategy()->getUIRTScale();
			//scissor info
			pUpdateDrawData->scissorEnable = op->m_bScissorEnable ? ECHO_TRUE : ECHO_FALSE;
			pUpdateDrawData->scissorValue.Left = int(op->m_ScissorLeft * uiscale);
			pUpdateDrawData->scissorValue.Top = int(op->m_ScissorTop * uiscale);
			pUpdateDrawData->scissorValue.Right = int(op->m_ScissorRight * uiscale);
			pUpdateDrawData->scissorValue.Bottom = int(op->m_ScissorBottom * uiscale);

			pUpdateDrawData->drawInfo.indexType = indexInfo.m_indexType;

			if (rend->useInstanceRender())
			{
				if (op->useIndexes)
				{
					assert(indexInfo.m_indexCount != 0);
					pUpdateDrawData->pIndexBuffer = indexInfo.m_indexBuffer;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_INSTEANCED;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = indexInfo.m_indexCount;
					pUpdateDrawData->drawInfo.vertexNumber = opMeshData.m_vertexCount;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = rend->getInstanceNum();
				}
				else
				{
					pUpdateDrawData->pIndexBuffer = indexInfo.m_indexBuffer;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_VERTEX_INSTEANCED;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = indexInfo.m_indexCount;
					pUpdateDrawData->drawInfo.vertexNumber = opMeshData.m_vertexCount;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = rend->getInstanceNum();
				}
			}
			else if (rend->useOffsetDraw())
			{
				RenderOffset offset;
				offset.StartVert = 0;
				offset.VertNum = 0;

				rend->getRenderOffset(offset);

				pUpdateDrawData->pIndexBuffer = nullptr;
				pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_OFFSET;
				pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
				pUpdateDrawData->drawInfo.primitiveNumber = 0;
				pUpdateDrawData->drawInfo.vertexNumber = offset.VertNum;
				pUpdateDrawData->drawInfo.startVertex = offset.StartVert;
				pUpdateDrawData->drawInfo.instanceNumber = 0;
			}
			else if (rend->isIndirectDraw())
			{
				if (op->useIndexes && !op->isManualVertexFetch)
				{
					assert(indexInfo.m_indexCount != 0);
					pUpdateDrawData->pIndexBuffer = indexInfo.m_indexBuffer;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_INDIRECT;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = indexInfo.m_indexCount;
					pUpdateDrawData->drawInfo.vertexNumber = opMeshData.m_vertexCount;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = 0;
					pUpdateDrawData->drawInfo.indexType = ECHO_INDEX_TYPE_UINT32;
				}
				else if(op->isManualVertexFetch)
				{
					pUpdateDrawData->pIndexBuffer = nullptr;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_INDIRECT_MANUAL;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = 0;
					pUpdateDrawData->drawInfo.vertexNumber = 0;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = 0;
				}
			}
			else
			{
				if (op->useIndexes)
				{
					assert(indexInfo.m_indexCount != 0);
					pUpdateDrawData->pIndexBuffer = indexInfo.m_indexBuffer;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_INDEXED;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = indexInfo.m_indexCount;
					pUpdateDrawData->drawInfo.vertexNumber = opMeshData.m_vertexCount;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = 0;
				}
				else
				{
					pUpdateDrawData->pIndexBuffer = nullptr;
					pUpdateDrawData->drawInfo.drawType = ECHO_DRAW_VERTEX;
					pUpdateDrawData->drawInfo.drawMode = op->mPrimitiveType;
					pUpdateDrawData->drawInfo.primitiveNumber = 0;
					pUpdateDrawData->drawInfo.vertexNumber = opMeshData.m_vertexCount;
					pUpdateDrawData->drawInfo.startVertex = 0;
					pUpdateDrawData->drawInfo.instanceNumber = 0;
				}
			}

			return pUpdateDrawData;
		}

		return nullptr;
	}

	void RenderSystem::destoryDrawData(RcDrawData* pRcDrawData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (!pRcDrawData)
				return;

			RcUniformBlock* pUniformPrivateVs = pRcDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_VS];
			RcUniformBlock* pUniformPrivatePs = pRcDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_PS];
			RcUniformBlock* pUniformDefault   = pRcDrawData->pUniformBlock[ECHO_UNIFORM_BLOCK_DEFAULT];

			if (pUniformPrivateVs)
			{
				m_uniformBufferTotalSize -= pUniformPrivateVs->dataSize;
				pUniformPrivateVs->pData = nullptr;
				m_pRenderer->destroyBuffer(pUniformPrivateVs);
			}

			if (pUniformPrivatePs)
			{
				m_uniformBufferTotalSize -= pUniformPrivatePs->dataSize;
				pUniformPrivatePs->pData = nullptr;
				m_pRenderer->destroyBuffer(pUniformPrivatePs);
			}

			if (pUniformDefault)
			{
				m_uniformBufferTotalSize -= pUniformDefault->dataSize;
				pUniformDefault->pData = nullptr;
				m_pRenderer->destroyBuffer(pUniformDefault);
			}

			m_pRenderer->unbindDrawData(pRcDrawData);
		}
	}

	RcComputeData* RenderSystem::createComputeData(Computable* comp, const Pass* pass)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			RcComputePipeline* pPipeline = reinterpret_cast<RcComputePipeline*>(pass->mDest);
			if (!pPipeline)
				return nullptr;

			RcUniformBlock* pUniformDefault = NEW_RcUniformBlock;
			pUniformDefault->pDevice = m_pRcDevice;
			pUniformDefault->type = ECHO_BUFFER_UNIFORM;
			pUniformDefault->usage = ECHO_BUFFER_DYNAMIC;
			pUniformDefault->blockUsage = ECHO_UNIFORM_BLOCK_DEFAULT;
			pUniformDefault->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_DEFAULT];
			pUniformDefault->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_DEFAULT];
			pUniformDefault->pData = nullptr;

			RcUniformBlock* pUniformPrivateCs = NEW_RcUniformBlock;
			pUniformPrivateCs->pDevice = m_pRcDevice;
			pUniformPrivateCs->type = ECHO_BUFFER_UNIFORM;
			pUniformPrivateCs->usage = ECHO_BUFFER_DYNAMIC;
			pUniformPrivateCs->blockUsage = ECHO_UNIFORM_BLOCK_PRIVATE_CS;
			pUniformPrivateCs->uniformInfos = pPipeline->uniformInfos[ECHO_UNIFORM_BLOCK_PRIVATE_CS];
			pUniformPrivateCs->dataSize = pPipeline->uniformBlockDataSize[ECHO_UNIFORM_BLOCK_PRIVATE_CS];
			pUniformPrivateCs->pData = nullptr;

			RcComputeData* pComputeData = NEW_RcComputeData;

			pComputeData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_CS] = pUniformPrivateCs;
			pComputeData->pUniformBlock[ECHO_UNIFORM_BLOCK_DEFAULT] = pUniformDefault;
			pComputeData->pComputePipeline = pPipeline;
			//pComputeData->pComputePipeline->pRenderTarget = m_currentRenderTarget->m_rcRenderTarget;

			//RenderOperation* op = rend->getRenderOperation();
			//pComputeData->drawMode = op->mPrimitiveType;

			comp->setComputeData(pass, pComputeData);

			m_pRenderer->bindComputeData(pComputeData);

			// 			bool bCommitOk = GpuParamsManager::instance()->commitUniformParams(rend, pass);
			//
			// 			if (!bCommitOk)
			// 				return nullptr;
			//
			// 			GpuParamsManager::instance()->commitSamplerParams(rend, pass);
			//
			// 			if (rend->hasCustomParameters())
			// 			{
			// 				rend->setCustomParameters(pass);
			// 			}
			//
			// 			if (pass->hasCustomParam())
			// 			{
			// 				Vector4 param;
			// 				uint32 index = 0;
			// 				pass->getCustomParam(param, index);
			// 				setUniformValue(rend, pass, index, param);
			// 			}

			return pComputeData;
		}

		return nullptr;
	}

	RcUpdateComputeData* RenderSystem::getUpdateComputeData(Computable* comp, const Pass* pass, RcComputeData* pRcComputeData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			RcUpdateComputeData* pUpdateComputeData = NEW_RcUpdateComputeData;

			pUpdateComputeData->pComputeData = pRcComputeData;

			//copy SamplerInfo
			for (uint32 j = 0; j < ECHO_MAX_SAMPLER_COUNT; j++)
			{
				pUpdateComputeData->updateSamplerInfo[j] = pRcComputeData->drawSamplerInfo[j];
			}

			//copy ImageInfo
			for (uint32 j = 0; j < ECHO_MAX_IMAGE_COUNT; j++)
			{
				pUpdateComputeData->updateImageInfo[j] = pRcComputeData->drawImageInfo[j];
			}

			for (uint32 j = 0; j < ECHO_MAX_STRUCT_BUFFER_COUNT; j++)
			{
				pUpdateComputeData->updateBufferInfo[j] = pRcComputeData->structBufferInfo[j];
			}

			return pUpdateComputeData;
		}

		return nullptr;
	}

	void RenderSystem::destoryComputeData(RcComputeData* pRcComputeData)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (!pRcComputeData)
				return;

			RcUniformBlock* pUniformPrivateCs = pRcComputeData->pUniformBlock[ECHO_UNIFORM_BLOCK_PRIVATE_CS];
			RcUniformBlock* pUniformDefault = pRcComputeData->pUniformBlock[ECHO_UNIFORM_BLOCK_DEFAULT];

			if (pUniformPrivateCs)
			{
				m_uniformBufferTotalSize -= pUniformPrivateCs->dataSize;
				pUniformPrivateCs->pData = nullptr;
				m_pRenderer->destroyBuffer(pUniformPrivateCs);
			}

			if (pUniformDefault)
			{
				m_uniformBufferTotalSize -= pUniformDefault->dataSize;
				pUniformDefault->pData = nullptr;
				m_pRenderer->destroyBuffer(pUniformDefault);
			}

			m_pRenderer->unbindComputeData(pRcComputeData);
		}
	}

	RcUniformBlock* RenderSystem::getUniformBlockCommonVS()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			if (m_pUniformBlockCommonVS)
			{
				return m_pUniformBlockCommonVS;
			}

			m_pUniformBlockCommonVS = NEW_RcUniformBlock;
			m_pUniformBlockCommonVS->pDevice = m_pRcDevice;
			m_pUniformBlockCommonVS->type = ECHO_BUFFER_UNIFORM;
			m_pUniformBlockCommonVS->usage = ECHO_BUFFER_DYNAMIC;
			m_pUniformBlockCommonVS->blockUsage = ECHO_UNIFORM_BLOCK_COMMON_VS;
			m_pUniformBlockCommonVS->pData = nullptr;
			m_pUniformBlockCommonVS->dataSize = 0;

			return m_pUniformBlockCommonVS;
		}

		return nullptr;
	}

	RcUniformBlock* RenderSystem::getUniformBlockCommonPS()
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			if (m_pUniformBlockCommonPS)
			{
				return m_pUniformBlockCommonPS;
			}

			m_pUniformBlockCommonPS = NEW_RcUniformBlock;
			m_pUniformBlockCommonPS->pDevice = m_pRcDevice;
			m_pUniformBlockCommonPS->type = ECHO_BUFFER_UNIFORM;
			m_pUniformBlockCommonPS->usage = ECHO_BUFFER_DYNAMIC;
			m_pUniformBlockCommonPS->blockUsage = ECHO_UNIFORM_BLOCK_COMMON_PS;
			m_pUniformBlockCommonPS->pData = nullptr;
			m_pUniformBlockCommonPS->dataSize = 0;

			return m_pUniformBlockCommonPS;
		}

		return nullptr;
	}

	void* RenderSystem::getUniformData(unsigned int dataSize)
	{
EchoZoneScoped;

		if(dataSize == 0)
			return nullptr;

		m_uniformBufferTotalSize += dataSize;

		return Rc_Malloc(dataSize);
	}

	void RenderSystem::updateUniform(RcUniformBlock* pRcBlock, void* uniformData, bool updateInCQueue, bool isAsync)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			if (pRcBlock->dataSize == 0)
				return;

			RcUpdateUniform * pRcParam = NEW_RcUpdateUniform;
			if (updateInCQueue)
			{
				if (isAsync)
				{
					pRcParam->pCustom = (void*)s_AsyncComputeQueue;
				}
				else
				{
					pRcParam->pCustom = (void*)s_UpdateInComputeQueue;
				}
			}
			pRcParam->pUniformBlock = pRcBlock;
			pRcParam->updateDataSize = pRcBlock->dataSize;
			pRcParam->pUpdateData = uniformData;
			pRcParam->updateDataFreeMethod = ECHO_MEMORY_RC_FREE;

			m_pRenderer->updateUniform(pRcParam);
		}
	}

	//=============================================================================
	void RenderSystem::OnDeviceInit(void* hwnd, void* pDevice, void * pContext)
	{
EchoZoneScoped;

		if (Root::instance()->isUseRenderCommand() && !isUseSingleThreadRender())
		{
			for (size_t i = 0; i < m_renderListeners.size(); ++i)
			{
				m_renderListeners[i]->OnInitializeDevice(hwnd, pDevice, pContext);
			}
		}
	}

	//=============================================================================
	void RenderSystem::OnDeviceLost()
	{
EchoZoneScoped;

		for (size_t i = 0; i < m_renderListeners.size(); ++i)
		{
			m_renderListeners[i]->OnDeviceLost();
		}
	}

	//=============================================================================
	void RenderSystem::OnDeviceReset()
	{
EchoZoneScoped;

		for (size_t i = 0; i < m_renderListeners.size(); ++i)
		{
			m_renderListeners[i]->OnDeivceReset();
		}
	}

	/*void RenderSystem::getTimerStr(std::string& result)
	{*/
		/*if (Root::instance()->isUseRenderCommand())
		{
			DEF_THREAD_CHEEK(DEF_THREAD_TYPE_MAIN);

			void* pBufferData = const_cast<void*>(_pData);
			ECHO_MEMORY_DESTROY_METHOD method = ECHO_MEMORY_NO_DESTROY;
			if (ECHO_THREAD_ST != m_GraphicParam.threadMode && _pData && _byteSize > 0)
			{
				pBufferData = malloc(_byteSize);
				memcpy(pBufferData, _pData, _byteSize);
				method = ECHO_MEMORY_FREE;
			}

			RcBuffer* pRcBuffer = NEW_RcBuffer;
			pRcBuffer->pDevice = m_pRcDevice;
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->pData = pBufferData;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->dataFreeMethod = method;

			m_pRenderer->createBuffer(pRcBuffer);

			return pRcBuffer;
		}
		else
		{
			IGXStaticVB* pGxVB = m_device->CreateStaticVB(_pData, _byteSize);

			if (nullptr == pGxVB)
			{
				LogManager::getSingleton().logMessage("-error-\tdevice createStaticVB failed! ", LML_CRITICAL);
				return nullptr;
			}

			RcBuffer* pRcBuffer = new RcBuffer();
			pRcBuffer->type = ECHO_BUFFER_VERTEX;
			pRcBuffer->usage = ECHO_BUFFER_STATIC;
			pRcBuffer->dataSize = _byteSize;
			pRcBuffer->pNative = pGxVB;

			return pRcBuffer;
		}*/
		//result = TimerTempBuffer;
	//}

	EchoRenderPass::EchoRenderPass()
		: pNative(nullptr)
	{
EchoZoneScoped;

	}

	EchoUniform::EchoUniform()
		: pNative(nullptr)
	{
EchoZoneScoped;

	}

	bool EchoUniform::Map(void** ppMem_)
	{
EchoZoneScoped;

		if (!pNative)
			return false;

		return ((IGXUniform*)(pNative))->Map(ppMem_);
	}

	void EchoUniform::Unmap()
	{
EchoZoneScoped;

		if (!pNative)
			return;

		((IGXUniform*)(pNative))->Unmap();
	}

	EchoSampler::EchoSampler(ECHO_TEX_TARGET type)
		: m_type(type)
		, pNative(nullptr)
	{
EchoZoneScoped;

	}

	bool
	RenderSystem::copyComputeTargetContentsToMemory(RcComputeTarget* pTarget,
													RcReadComputeTarget &pInfo)
	{
		EchoZoneScoped;

		bool result = true;
		RcReadComputeTarget *pRcParam = NEW_RcReadComputeTarget;

		pRcParam->pDestCT = pTarget;
		pRcParam->pCustom = pInfo.pCustom;
		pRcParam->pNative = pInfo.pNative;

		m_pRenderer->readComputeTarget(pRcParam);

		//编辑器中直接用的该个接口，在单线程模式下需要将数据立即传递过去
		if (nullptr == pInfo.pCustom && ECHO_THREAD_ST == m_GraphicParam.threadMode &&
			ECHO_DEVICE_METAL != m_GraphicParam.eClass ) //如果使用单线程渲染，除了Metal外，其他API可以直接拿数据
		{
			if (pRcParam->dataSize > 0 && pRcParam->pData)
			{
				pInfo.realWidth = pRcParam->realWidth;
				pInfo.realHeight = pRcParam->realHeight;
				pInfo.dataSize = pRcParam->dataSize;
				pInfo.pData = pRcParam->pData;  ///@NOTE pRcParam->pData 底层不会释放,这里移交给pInfo.pData在调用函数成后释放
				pInfo.dataFormat = pRcParam->dataFormat;

				pRcParam->pData = nullptr;
				pRcParam->dataSize = 0;
			}
			else
			{
				result = false;
			}
		}

		return (result);
	}

	bool
	RenderSystem::copyBufferContentsToMemory(RcBuffer* pTarget,
											 RcReadBuffer &pInfo)
	{
		EchoZoneScoped;

		bool result = true;
		RcReadBuffer *pRcParam = NEW_RcReadBuffer;

		pRcParam->pDestBuffer = pTarget;
		pRcParam->pCustom = pInfo.pCustom;
		pRcParam->pNative = pInfo.pNative;

		m_pRenderer->readBuffer(pRcParam);

		//编辑器中直接用的该个接口，在单线程模式下需要将数据立即传递过去
		if (nullptr == pInfo.pCustom && ECHO_THREAD_ST == m_GraphicParam.threadMode &&
			ECHO_DEVICE_METAL != m_GraphicParam.eClass ) //如果使用单线程渲染，除了Metal外，其他API可以直接拿数据
		{
			if (pRcParam->dataSize > 0 && pRcParam->pData)
			{
				pInfo.dataSize = pRcParam->dataSize;
				pInfo.pData = pRcParam->pData;  ///@NOTE pRcParam->pData 底层不会释放,这里移交给pInfo.pData在调用函数成后释放

				pRcParam->pData = nullptr;
				pRcParam->dataSize = 0;
			}
			else
			{
				result = false;
			}
		}

		return (result);
	}

	void RenderSystem::copyBufferToBuffer(RcBuffer* pSrc, RcBuffer* pDst, uint32_t byteSize, uint32_t srcOffset, uint32_t dstOffset)
	{
		EchoZoneScoped;

		RcCopyBuffer* pRcParam = NEW_RcCopyBuffer;

		pRcParam->pSrcBuffer = pSrc;
		pRcParam->pDstBuffer = pDst;
		pRcParam->srcOffset = srcOffset;
		pRcParam->dstOffset = dstOffset;
		pRcParam->byteSize = byteSize;

		m_pRenderer->copyBuffer(pRcParam);
	}

	bool RenderSystem::isSupportDLSS() const
	{
		EchoZoneScoped;

		return m_GraphicParam.bSupportDLSS;
	}

	bool RenderSystem::isEnableDLSS() const
	{
		EchoZoneScoped;

		return m_selectedDLSSQueryQuality != ECHO_DLSS_PERF_QUALITY_DISABLE;
	}

	bool RenderSystem::isEnableJitter() const
	{
		float xRange;
		float yRange;
		getCurRenderStrategy()->getMainViewPortRange(xRange, yRange);
		if (isEnableDLSS() && (fabs(1.0f - xRange) > 0.00001f || fabs(1.0f - yRange) > 0.00001f))
		{
			return false;
		}
		return isEnableDLSS() || getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::TAA);
	}

	ECHO_DLSS_PERF_QUALITY RenderSystem::getSelectedDLSSQuality() const
	{
		EchoZoneScoped;

		return m_selectedDLSSQueryQuality;
	}

	const RcDLSSQueryOptimal &RenderSystem::queryDLSSOptimal() const
	{
		EchoZoneScoped;

		if (m_renderWindow == nullptr)
		{
			return m_DLSSQueryOptimal;
		}

		const uint32_t windowWidth = m_renderWindow->getTargetWidth();
		const uint32_t windowHeight = m_renderWindow->getTargetHeight();

		const uint32_t srcRtWidth = static_cast<uint32_t>(windowWidth * m_mainRTScale);
		const uint32_t srcRtHeight = static_cast<uint32_t>(windowHeight * m_mainRTScale);
		if (m_DLSSQueryOptimal.displayWidth != srcRtWidth || m_DLSSQueryOptimal.displayHeight != srcRtHeight)
		{
			// Query Optimal
			RcDLSSQueryOptimal *pParam = NEW_RcDLSSQueryOptimal;
			pParam->displayWidth = srcRtWidth;
			pParam->displayHeight = srcRtHeight;

			m_pRenderer->dlssQueryOptimal(pParam);

			m_DLSSQueryOptimal.displayWidth = pParam->displayWidth;
			m_DLSSQueryOptimal.displayHeight = pParam->displayHeight;
			for (uint32_t i = 0; i < ECHO_DLSS_PERF_QUALITY_COUNT; i++)
			{
				m_DLSSQueryOptimal.recommendedSetting[i] = pParam->recommendedSetting[i];
			}
		}
		return m_DLSSQueryOptimal;
	}

	bool RenderSystem::resetDLSSPerfQuality(ECHO_DLSS_PERF_QUALITY quality)
	{
		EchoZoneScoped;

		if (!isSupportDLSS())
		{
			return false;
		}

		m_selectedDLSSQueryQuality = quality;

		if (!m_renderWindow)
		{
			return false;
		}

		if (!m_GraphicParam.bSupportDLSS)
		{
			m_selectedDLSSQueryQuality = ECHO_DLSS_PERF_QUALITY_DISABLE;
			return false;
		}

		const RcDLSSQueryOptimal &dlssOptimal = queryDLSSOptimal();
		const auto& setting = m_DLSSQueryOptimal.recommendedSetting[m_selectedDLSSQueryQuality];
		if (!setting.available)
		{
			m_selectedDLSSQueryQuality = ECHO_DLSS_PERF_QUALITY_DISABLE;
		}

		RcDLSSUpdateFeatures* pParam = NEW_RcDLSSUpdateFeatures;
		pParam->perfQuality = m_selectedDLSSQueryQuality;
		if (m_selectedDLSSQueryQuality != ECHO_DLSS_PERF_QUALITY_DISABLE)
		{
			pParam->depthInverted = Root::instance()->getEnableReverseDepth();
			pParam->optimalRenderWidth = setting.optimalRenderWidth;
			pParam->optimalRenderHeight = setting.optimalRenderHeight;
			pParam->displayOutWidth = dlssOptimal.displayWidth;
			pParam->displayOutHeight = dlssOptimal.displayHeight;
		}
		m_pRenderer->dlssUpdateFeatures(pParam);

		// Change Global MipLodBias
		RcSetGlobalSamplerInfo* info = NEW_RcSetGlobalSamplerInfo;
		if (m_selectedDLSSQueryQuality != ECHO_DLSS_PERF_QUALITY_DISABLE)
		{
			const float lodBias = std::log2f(static_cast<float>(setting.optimalRenderWidth) / static_cast<float>(dlssOptimal.displayWidth)) - 1.0f;
			info->mipLodBias = lodBias;
		}
		else
		{
			info->mipLodBias = 0;
		}
		m_pRenderer->setGlobalSamplerInfo(info);

		if (m_selectedDLSSQueryQuality == ECHO_DLSS_PERF_QUALITY_DISABLE)
		{
			m_renderStrategy[m_curStrategyIndex]->setMainRenderTargetScale(m_mainRTScale);
		}
		else
		{
			float scale = (float)(setting.optimalRenderWidth + 2) / (float)m_renderWindow->getTargetWidth();
			scale = std::clamp(scale, 0.01f, 1.0f);
			m_renderStrategy[m_curStrategyIndex]->setMainRenderTargetScale(scale);
		}
		m_renderStrategy[m_curStrategyIndex]->setViewportScale(m_viewportScale);

		return true;
	}

	void RenderSystem::evaluateDLSSSuperSampling(RenderTarget* inputRT,
												 RenderTarget* outputRT, RenderTarget* motionVectorsRT, DepthBuffer* depth,
												 bool resetAccumulation,
												 uint32_t renderingOffsetX, uint32_t renderingOffsetY, uint32_t renderingWidth,
												 uint32_t renderingHeight)
	{
		EchoZoneScoped;

		RcDLSSEvaluateSuperSampling* pParam = NEW_RcDLSSEvaluateSuperSampling;

		pParam->inputRT = inputRT->getRenderTexture();
		pParam->outputRT = outputRT->getRenderTexture();
		pParam->depth = depth->getDepthTex();
		pParam->motionVectorsRT = motionVectorsRT->getRenderTexture();
		pParam->renderingOffsetX = renderingOffsetX;
		pParam->renderingOffsetY = renderingOffsetY;
		pParam->renderingWidth = renderingWidth;
		pParam->renderingHeight = renderingHeight;
		pParam->resetAccumulation = resetAccumulation;

		const Vector2 jitter = resetAccumulation ? Vector2(0, 0) : getJitterOffset();
		pParam->jitterOffsetX = jitter.x;
		pParam->jitterOffsetY = jitter.y;

		m_pRenderer->dlssEvaluateSuperSampling(pParam);
	}

	Vector2 RenderSystem::getJitterOffset() const
	{
		EchoZoneScoped;

		if (isEnableJitter())
		{
			return m_jitterOffset;
		}
		return Vector2{0.0f, 0.0f};
	}

	void RenderSystem::jitterProjectMatrix(uint32_t vpWidth, uint32_t vpHeight, Matrix4& mat) const
	{
		EchoZoneScoped;

		Vector2 jitter = getJitterOffset();
		jitter.y = -jitter.y;

		const Vector2 jitterInClipSpace = jitter * (2.0f / Vector2{(float)vpWidth, (float)vpHeight});

		mat[0][2] -= jitterInClipSpace.x;
		mat[1][2] -= jitterInClipSpace.y;
	}

	constexpr static float _halton(uint32_t i,uint32_t b) noexcept
	{
		i += 409;
		float f = 1.0f;
		float r = 0.0f;
		while (i > 0u)
		{
			f /= float(b);
			r += f * float(i % b);
			i /= b;
		}
		return r;
	}

	template <uint32_t COUNT>
	constexpr auto halton()
	{
		std::array<Vector2, COUNT> h{};
		for (uint32_t i = 0; i < COUNT; i++)
		{
			h[i] = Vector2{
				_halton(i, 2),
				_halton(i, 3)
			};
		}
		return h;
	}

	template <size_t SIZE>
	struct JitterSequence
	{
		auto operator()(size_t i) const noexcept
		{
			return positions[i % SIZE] - 0.5f;
		}

		const std::array<Vector2, SIZE> positions{};
	};

	void RenderSystem::updateJitter()
	{
		EchoZoneScoped;

		const static JitterSequence<32> sHaltonSamples = { halton<32>()};

		m_frameIndex = m_frameIndex + 1;
		m_jitterOffset = sHaltonSamples(m_frameIndex % 16);
	}
} // namespace Echo

//-----------------------------------------------------------------------------
//	CPP File End
//-----------------------------------------------------------------------------
