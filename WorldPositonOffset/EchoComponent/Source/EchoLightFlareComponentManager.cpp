#include "EchoComponentStableHeaders.h"
#include "EchoLightFlareComponentManager.h"
#include "EchoLightFlareComponent.h"
//#include "EchoLensFlare.h"

namespace Echo{

	Name LightFlareManagerType = Name("LightFlareManager");


	float LightFlareComponentManager::unit_half_trans_30[16][2] = { {-0.5f,0.f},{-0.489f,0.104f}, {-0.457f,0.2035f},{-0.4045f,0.294f},{-0.3345f,0.3715f},{-0.25f,0.433f},{-0.1545f,0.4755f},{-0.0525f,0.4975f },
				                                               {0.0525f,0.4975f},{0.1545f,0.4755f},{0.25f,0.433f},{0.3345f,0.3715f},{0.4045f,0.294f},{0.457f,0.2035f},{0.489f,0.104f},{0.5f,0.f} };

	float LightFlareComponentManager::unit_trans_16[16][2] = { {-0.5f,0.f},{-0.4619f,0.1913f},{-0.3536f,0.3536f},{-0.1913f,0.4619f},{0.f,0.5f} , {0.1913f,0.4619f},{0.3536f,0.3536f},{0.4619f,0.1913f},
								   {0.5f,0.f},{0.4619f,-0.1913f}, {0.3536f,-0.3536f},{0.1913f,-0.4619f},{0.f,-0.5f},{-0.1913f,-0.4619f},{-0.3536f,-0.3536f},{-0.4619f,-0.1913f} };
	enum LightFlare_LIMIT_COUNT {
		One_Level = 2,
		Second_Level = 6,
	    Third_Level = 14,
	    Forth_Level = 20
	};

	LightFlareComponentManager::LightFlareComponentManager(const Name& name, ProtocolComponentManager* pMgr)
		:ProtocolComponent(pMgr)
		,mName(name)
	{
		registerTickTimeBegin();
		_initLightArrayDpData();
	}
	LightFlareComponentManager::~LightFlareComponentManager()
	{
		m_TextureMap.clear();
	}
	void LightFlareComponentManager::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}


	void LightFlareComponentManager::beginFrame(EventID eventID, ProtocolHandle)
	{
		switch (eventID)
		{
		case EID_EveryFrame:
			_updateLightFlare();
			break;
		default:
			break;
		}
	}

	void LightFlareComponentManager::getProtocolType(Name& type)
	{
		type = LightFlareManagerType;
	}

	void LightFlareComponentManager::getProtocolTypeToString(std::string& typeStr)
	{
		typeStr = LightFlareManagerType.c_str();
	}

	void LightFlareComponentManager::addLightFlareCom(LightFlareComponent*  lightFlareCom)
	{
		if (lightFlareCom == nullptr)
			return;
		mLightFlareSet.insert(lightFlareCom);
		lightFlareCom->setLightFlareComMgr(this);
	}
	void LightFlareComponentManager::deleteLightDlareCom(LightFlareComponent* lightFlareCom)
	{
		if (lightFlareCom == nullptr)
			return;
		if (mLightFlareSet.count(lightFlareCom) != 0)
		{
			lightFlareCom->setLightFlareComMgr(nullptr);
			mLightFlareSet.erase(lightFlareCom);
		}
	}
	void LightFlareComponentManager::getTextureByName(TexturePtr& texPtr_, const std::string& texName_)
	{
		if (m_TextureMap.contains(texName_))
			texPtr_ = m_TextureMap[texName_];
	}
	void LightFlareComponentManager::setTextureByName(const TexturePtr& texPtr_, const std::string& texName_)
	{
		if (!m_TextureMap.contains(texName_))
		{
			m_TextureMap.insert(std::make_pair(texName_, texPtr_));
		}
	}
	void LightFlareComponentManager::_updateLightFlare()
	{
		/*auto begin =   std::chrono::high_resolution_clock::now();*/
		mLightFlareCullVec.clear();
		for (const auto& iter : mLightFlareSet)
		{
			if (iter == nullptr)
				continue;
			if (!iter->_getLightFlareState())
				continue;

			iter->_computeLightFlareData();

			iter->_updateLightFlareOpacity();

			bool isAtConeOfVision_ = iter->_getLightFlareIsAtConeOfVision();
			if (!isAtConeOfVision_)
			{
				iter->mLensFlare->setComLensFlareVisible(false);
				continue;
			}

			LightFlareData tempLightFlareData_;
			tempLightFlareData_.distance = iter->mLightDistance;
			tempLightFlareData_.priority = iter->mContext.mPriority;
			tempLightFlareData_.lightFlareCom = iter;

			mLightFlareCullVec.emplace_back(tempLightFlareData_);
		}

		std::sort(mLightFlareCullVec.begin(), mLightFlareCullVec.end(), [&](const LightFlareData& lhs, const LightFlareData& rhs) {
			if (lhs.priority == rhs.priority)
				return lhs.distance < rhs.distance;
			return lhs.priority > rhs.priority;
			}
		);

		//auto one  = std::chrono::high_resolution_clock::now();

		size_t resultSz = std::min(mLightFlareCullVec.size(), MAX_LIGHRFLARE_COUNT);

		int flare_count = 0;
		for (size_t i = 0; i < resultSz; ++i)
		{
			bool isUpdate_ = false;

			if (mLightFlareCullVec[i].distance < mLightFlareCullVec[i].lightFlareCom->getFirstLevelDistance())
			{
				isUpdate_ = mLightFlareCullVec[i].lightFlareCom->_getLightFlareIsVisible();

				if (isUpdate_)
				{
					if (flare_count < One_Level)
					{
						mLightFlareCullVec[i].lightFlareCom->_computeLightFlareDegreeofOcclusion(16);
					}
					else if (flare_count < Second_Level)
					{
						mLightFlareCullVec[i].lightFlareCom->_computeLightFlareDegreeofOcclusion(8);
					}
					else if (flare_count < Third_Level)
					{
						mLightFlareCullVec[i].lightFlareCom->_computeLightFlareDegreeofOcclusion(4);
					}
					else if (flare_count < Forth_Level)
					{
						mLightFlareCullVec[i].lightFlareCom->_computeLightFlareDegreeofOcclusion(1);
					}
				}

			}
			else if (mLightFlareCullVec[i].distance < mLightFlareCullVec[i].lightFlareCom->getSecondLevelDistance())
			{
				isUpdate_ = mLightFlareCullVec[i].lightFlareCom->_getLightFlareIsVisible();
			}
		
			    
			if (isUpdate_)
			{
				mLightFlareCullVec[i].lightFlareCom->_updateLightFlare();
				++flare_count;
			}
			else
				mLightFlareCullVec[i].lightFlareCom->mLensFlare->setComLensFlareVisible(false);
		}

		//auto second = std::chrono::high_resolution_clock::now();
		
		/*oneTime_ = one - begin;
		secTime_ = second - begin;*/
		
	}
	void LightFlareComponentManager::_initLightArrayDpData()
	{
		mAreaPercentDp_16.resize(16);
		mAreaPercrntDp_8.resize(8);
		float roundArea = Math::PI * 0.125f;
		for(int i = 0; i < 15 ; ++ i)
			for(int j = i+1 ; j < 16 ; ++ j)
				mAreaPercentDp_16[i][j] = _computeArchArea(Vector2(unit_half_trans_30[i][0], unit_half_trans_30[i][1]), Vector2(unit_half_trans_30[j][0], unit_half_trans_30[j][1])) / roundArea;
	
		for (int i = 0; i < 7; ++i)
			for (int j = i + 1; j < 8; ++j)
		        mAreaPercrntDp_8[i][j] = _computeArchArea(Vector2(unit_trans_16[i][0], unit_trans_16[i][1]), Vector2(unit_trans_16[j][0], unit_trans_16[j][1])) / roundArea;
	}

	float LightFlareComponentManager::_computeArchArea(const Vector2& point_a, const Vector2& point_b)
	{
		Vector2 ab = point_b - point_a;
		Vector2 ac = -point_a;

		float d = std::abs(ab.crossProduct(ac));
		d /= abs(ab.length());

		float degree = 2 * acos(d * 2);

		return 0.5f * 0.25f * degree - d * sqrtf(0.25f - d * d);
	}
	LightFlareComSystem::LightFlareComSystem()
	{
	}
	LightFlareComSystem::~LightFlareComSystem()
	{
		for (auto& iter : m_mapLightFlareManager)
		{
			LightFlareComponentManager* lMgr = m_mapLightFlareManager[iter.first];
			if (lMgr)
			{
				delete lMgr;
				lMgr = nullptr;
			}
			m_mapLightFlareManager[iter.first] = nullptr;
		}
	}
	LightFlareComponentManager* LightFlareComSystem::createLightFlareComponentManager(const Name& name, ProtocolComponentManager* pm)
	{
		if (m_mapLightFlareManager.count(name) != 0)
		{
			if (m_mapLightFlareManager[name]->m_pProtocolComponentManager == nullptr)
				m_mapLightFlareManager[name]->m_pProtocolComponentManager = pm;
			return m_mapLightFlareManager[name];
		}
		LightFlareComponentManager* lMgr = new LightFlareComponentManager(name, pm);
		m_mapLightFlareManager.emplace(name, lMgr);

		return lMgr;
	}
	void LightFlareComSystem::destroyLightFlareComponentManager(LightFlareComponentManager* lm)
	{
		if (lm == nullptr)
			return;

		if (m_mapLightFlareManager.count(lm->mName) != 0)
		{
			m_mapLightFlareManager.erase(lm->mName);
		}

		delete lm;
		lm = nullptr;
	}
	void LightFlareComSystem::destroyLightFlareComponentManager(const Name& name)
	{
		if (m_mapLightFlareManager.count(name) != 0)
		{
			LightFlareComponentManager* lMgr = m_mapLightFlareManager[name];
			m_mapLightFlareManager[name] = nullptr;
			if (lMgr)
			{
				delete lMgr;
				lMgr = nullptr;
			}
			m_mapLightFlareManager.erase(name);
		}
	}
	LightFlareComponentManager* LightFlareComSystem::getLightFlareComponentManager(const Name& name)
	{
		if (m_mapLightFlareManager.count(name) == 0)
			return nullptr;
		return m_mapLightFlareManager[name];
	}
	LightFlareComponentManager* LightFlareComSystem::getMainLightFlareComponentManager()
	{
		if (m_mapLightFlareManager.count(g_MainLightFlareComponentName) == 0)
			createLightFlareComponentManager(g_MainLightFlareComponentName, nullptr);
		return m_mapLightFlareManager[g_MainLightFlareComponentName];
	}
}
