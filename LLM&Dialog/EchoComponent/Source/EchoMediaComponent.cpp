
#include "EchoComponentStableHeaders.h"
#include "EchoMediaComponent.h"
#include "EchoHWMediaPlayer.h"
#include "EchoShaderContentManager.h"
#include "EchoTextureManager.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(MediaComponent)
	MediaComponent::MediaComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: NodeActorComponent(pActor, type)
		, m_pRenderable(nullptr)
		, m_loop(false)
		, m_autoStart(false)
	{
	}

	MediaComponent::~MediaComponent()
	{
	}

	void MediaComponent::init(ActorComponentInfo* info)
	{
	}
	
	void MediaComponent::initRenderObject(ActorComponentInfo* info)
	{
		m_pActComInfo = info;
		MediaComponentInfo* mInfo = static_cast<MediaComponentInfo*>(m_pActComInfo);
		m_loop = mInfo->GetLoop();
		m_autoStart = mInfo->GetAutoStart();
		HWMediaPlayer::instance()->SetLooping(m_loop);
		HWMediaPlayer::instance()->SetAutoStart(m_autoStart);
		if (!HWMediaPlayer::instance()->isInit() && !mInfo->m_MediaName.empty())
		{
			if (mInfo->m_MediaName.starts_with("http"))
			{
				openUrl(mInfo->m_MediaName);
			}
			else
			{
				openMedia(mInfo->m_MediaName);
			}
		}
	}
	
	void MediaComponent::unInit()
	{
		AttachNode* pSceneNode = dynamic_cast<AttachNode*>(mComRootNode);
		if (nullptr != pSceneNode)
		{
			pSceneNode->detachAllObject();
		}
		closeCurrentMedia();
		if (m_pRenderable)
		{
			delete m_pRenderable;
			m_pRenderable = nullptr;
		}
	}
	
	void MediaComponent::visibleStateChanged(bool visibleState)
	{
		if (m_pRenderable)
		{
			m_pRenderable->setVisible(visibleState);
		}
		UpdateComRootNode(true);
	}

	void MediaComponent::openMedia(const std::string& name)
	{
		if (HWMediaPlayer::instance()->isInit())
		{
			HWMediaPlayer::instance()->Close();
			if (m_pRenderable)
			{
				delete m_pRenderable;
				m_pRenderable = nullptr;
			}
		}

		if (HWMediaPlayer::instance()->OpenFile(name, this, &startPlayCallBack, &finishPlayCallBack))
		{
			m_CurrentMedia = name;

			if (!m_pRenderable)
			{
				MediaComponentInfo* mInfo = static_cast<MediaComponentInfo*>(m_pActComInfo);
				m_pRenderable = new MediaComponentRenderable(this, Name(mInfo->m_MediaName));
				m_pRenderable->init(Name(mInfo->GetMediaName()));
				UpdateComRootNode(true);
			}
		}
	}

	void MediaComponent::openUrl(const std::string& url)
	{
		if (HWMediaPlayer::instance()->isInit())
		{
			HWMediaPlayer::instance()->Close();
			if (m_pRenderable)
			{
				delete m_pRenderable;
				m_pRenderable = nullptr;
			}
		}

		if (HWMediaPlayer::instance()->OpenUrl(url, this, &startPlayCallBack, &finishPlayCallBack))
		{
			m_CurrentMedia = url;

			if (!m_pRenderable)
			{
				MediaComponentInfo* mInfo = static_cast<MediaComponentInfo*>(m_pActComInfo);
				m_pRenderable = new MediaComponentRenderable(this, Name(mInfo->m_MediaName));
				m_pRenderable->init(Name(mInfo->GetMediaName()));
				UpdateComRootNode(true);
			}
		}
	}

	void MediaComponent::closeCurrentMedia()
	{
		HWMediaPlayer::instance()->Close();
	}

	void MediaComponent::start()
	{
		if ((!HWMediaPlayer::instance()->isInit() && !m_CurrentMedia.empty()) || HWMediaPlayer::instance()->GetCurrentMediaName() != m_CurrentMedia)
		{
			HWMediaPlayer::instance()->SetLooping(m_loop);
			HWMediaPlayer::instance()->SetAutoStart(m_autoStart);

			if (HWMediaPlayer::instance()->isInit())
			{
				HWMediaPlayer::instance()->Close();
			}

			if (m_CurrentMedia.starts_with("http"))
			{
				openUrl(m_CurrentMedia);
			}
			else
			{
				openMedia(m_CurrentMedia);
			}
		}
		HWMediaPlayer::instance()->Start();
	}

	void MediaComponent::pause()
	{
		HWMediaPlayer::instance()->Pause();
	}

	void MediaComponent::setLoop(bool b)
	{
		HWMediaPlayer::instance()->SetLooping(b);
		m_loop = b;
	}

	void MediaComponent::setAutoStart(bool b)
	{
		HWMediaPlayer::instance()->SetAutoStart(b);
		m_autoStart = b;
	}

	void MediaComponent::restart()
	{
		pause();
		HWMediaPlayer::instance()->SetCurrentPos(0.0);
		start();
	}

	float MediaComponent::getAspectRatio()
	{
		return HWMediaPlayer::instance()->GetAspectRatio();
	}

	double MediaComponent::getDuration()
	{
		return HWMediaPlayer::instance()->GetDuration();
	}

	void MediaComponent::addMediaComListener(MediaComListener* listener)
	{
		if (listener)
		{
			m_setMediaComListeners.emplace(listener);
		}
	}

	void MediaComponent::removeMediaComListener(MediaComListener* listener)
	{
		if (listener)
		{
			m_setMediaComListeners.erase(listener);
		}
	}

	void MediaComponent::createComRootNodeImpl()
	{
		mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
	}

	void MediaComponent::startPlayCallBack(void* userdata)
	{
		MediaComponent* pCom = reinterpret_cast<MediaComponent*>(userdata);

		if (!pCom->m_setMediaComListeners.empty())
		{
			for (auto& listener : pCom->m_setMediaComListeners)
			{
				listener->MediaStartPlayCallBack();
			}
		}
	}

	void MediaComponent::finishPlayCallBack(void* userdata)
	{
		MediaComponent* pCom = reinterpret_cast<MediaComponent*>(userdata);

		if (!pCom->m_setMediaComListeners.empty())
		{
			for (auto& listener : pCom->m_setMediaComListeners)
			{
				listener->MediaFinishPlayCallBack();
			}
		}
	}

	//================================================================

	EDITOR_REFLECT_BEGIN(MediaComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MediaName, "MediaName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Loop, "Loop", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_AutoStart, "AutoStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	MediaComponentInfo::MediaComponentInfo(const Name& comType, ActorCOMMoveableType moveType)
		: ActorComponentInfo(comType, moveType)
	{
	}

	MediaComponentInfo::~MediaComponentInfo()
	{
	}

	_DEFINEINFO_VIRFUN_BASE(MediaComponentInfo)

#ifdef ECHO_EDITOR
	bool MediaComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_MediaName.empty() || m_MediaName.starts_with("http"))
		{
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::NeedCopyRes, m_MediaName));
		return true;
	}
#endif

	//================================================================

	struct MediaVertex
	{
		Vector3 pos;
		Vector2 UV;
	};
	/*
	* 0---1
	* |	\ |
	* 3---2
	*/
	const MediaVertex vertex[4] =
	{
		{Vector3(0.0f, 1.0f, 0.f), Vector2(0.f, 0.f)},
		{Vector3(1.0f, 1.0f, 0.f), Vector2(1.f, 0.f)},
		{Vector3(1.0f, 0.0f, 0.f), Vector2(1.f, 1.f)},
		{Vector3(0.0f, 0.0f, 0.f), Vector2(0.f, 1.f)}
	};

	const uint16 index[6] =
	{
		0, 3, 2, 0, 2, 1
	};

	MediaComponentRenderable::MediaComponentRenderable(MediaComponent* pComponent, Name name)
		: SimpleRenderable(name)
		, mComponent(pComponent)
		, mVB(nullptr)
		, mIB(nullptr)
	{
		enableCustomParameters(true);
		m_pSceneNode = ((SceneNode*)mComponent->getSceneNode())->createChildSceneNode();
		m_pSceneNode->attachObject(this);
	}

	MediaComponentRenderable::~MediaComponentRenderable()
	{
		if (mMaterial)
		{
			delete mMaterial;
			mMaterial = nullptr;
		}
		m_pSceneNode->detachAllObject();
		mComponent->getSceneNode()->removeChild(m_pSceneNode);
		if (mVB)
		{
			mVB = nullptr;
		}
		if (mIB)
		{
			mIB = nullptr;
		}
	}

	void MediaComponentRenderable::init(const Name& mediaName)
	{
		mMaterial = new Material(NULL, "MediaMaterial_" + mediaName);

		uint32 n = 0;
		ECHO_IA_DESC& IA_desc = mMaterial->m_desc.IADesc;
		IA_desc.ElementArray[n].nStream = 0;
		IA_desc.ElementArray[n].nOffset = offsetof(MediaVertex, pos);
		IA_desc.ElementArray[n].eType = ECHO_FLOAT3;
		IA_desc.ElementArray[n].Semantics = ECHO_POSITION0;
		++n;
		IA_desc.ElementArray[n].nStream = 0;
		IA_desc.ElementArray[n].nOffset = offsetof(MediaVertex, UV);
		IA_desc.ElementArray[n].eType = ECHO_FLOAT2;
		IA_desc.ElementArray[n].Semantics = ECHO_TEXCOORD0;
		++n;

		IA_desc.nElementCount = n;

		IA_desc.StreamArray[0].bPreInstance = ECHO_FALSE;
		IA_desc.StreamArray[0].nRate = 1;
		IA_desc.StreamArray[0].nStride = sizeof(MediaVertex);
		IA_desc.nStreamCount = 1;

		std::string vsShader, psShader, vsMarco, psMarco;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Media_VS.txt"), vsShader);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Media_PS.txt"), psShader);

		mMaterial->setShaderName("Media_VS.txt", "Media_PS.txt");
		mMaterial->setShaderSrcCode(vsShader, psShader);
		mMaterial->setMacroShaderSrcCode(vsMarco, psMarco);
		mMaterial->createPass(NormalPass);
		mMaterial->loadInfo();

		mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
		mRenderOp.GetMeshData().m_vertexCount = sizeof(vertex) / sizeof(vertex[0]);
		mVB = Root::instance()->getRenderSystem()->createStaticVB(vertex, sizeof(vertex));
		mRenderOp.GetMeshData().m_vertexBuffer.push_back(mVB);

		mRenderOp.GetMeshData().m_IndexBufferData.m_indexCount = sizeof(index) / sizeof(index[0]);
		mIB = Root::instance()->getRenderSystem()->createStaticIB(index, sizeof(index));
		mRenderOp.GetMeshData().m_IndexBufferData.m_indexBuffer = mIB;

		mBox.setExtents(0.0f, 0.0f, -0.5f, 1.0f, 1.0f, 0.5f);
	}

	void MediaComponentRenderable::updateRenderQueue(RenderQueue* pRenderQueue)
	{
		if (mVB && mIB)
		{
			SimpleRenderable::updateRenderQueue(pRenderQueue);
		}
	}

	void MediaComponentRenderable::setCustomParameters(const Pass* pass)
	{
		ECHO_SAMPLER_STATE samplerState;
		samplerState.AddressU = ECHO_TEX_ADDRESS_CLAMP;
		samplerState.AddressV = ECHO_TEX_ADDRESS_CLAMP;
		samplerState.AddressW = ECHO_TEX_ADDRESS_CLAMP;

		samplerState.Filter = ECHO_TEX_FILTER_TRILINEAR;
		if (HWMediaPlayer::instance()->GetCurrentState() == HWMediaPlayer::Playing)
		{
			Root::instance()->getRenderSystem()->setTextureSampleValue(this, pass, S_Diffuse, HWMediaPlayer::instance()->GetMediaTarget(), samplerState);
		}
		else
		{
			Root::instance()->getRenderSystem()->setTextureSampleValue(this, pass, S_Diffuse, TextureManager::instance()->getDefaultRcTexture(TEX_TYPE_2D), samplerState);
		}
	}

	void MediaComponentRenderable::setVisible(bool b)
	{
		m_pSceneNode->setVisible(b);
		SimpleRenderable::setVisible(b);
	}

}
