#ifndef __EchoLightFlareComponentManager_h__ 
#define __EchoLightFlareComponentManager_h__
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

namespace Echo {


	class LightFlareComponent;
	struct LightFlareData {
		int                   priority = 0;
		float                 distance = 0.f;
		LightFlareComponent*  lightFlareCom = nullptr;
	};

	class _EchoComponentExport LightFlareComponentManager : public ProtocolComponent
	{
		friend class  LightFlareComSystem;
		friend class  LightFlareComponent;
	public:
		LightFlareComponentManager(const Name& name,ProtocolComponentManager* pMgr);
		~LightFlareComponentManager();

		virtual void						            registerTickTimeBegin() override;
		virtual void                                    beginFrame(EventID, ProtocolHandle) override;
		

		virtual void                                    getProtocolType(Name& type) override;
		virtual void                                    getProtocolTypeToString(std::string& typeStr) override;

		void                                            addLightFlareCom(LightFlareComponent*);
		void                                            deleteLightDlareCom(LightFlareComponent*);
	
		void                                            getTextureByName(TexturePtr& texPtr_, const std::string& texName_);
		void                                            setTextureByName(const TexturePtr& texPtr_, const std::string& texName_);
	private:
		void                                            _updateLightFlare();
		void                                            _initLightArrayDpData();
		float                                           _computeArchArea(const Vector2& point_a, const Vector2& point_b);
	private:
		std::set<LightFlareComponent*>                  mLightFlareSet;
		std::vector<LightFlareData>                     mLightFlareCullVec;
		
		//多场景相关
		Name                                            mName;
		               
		static float                                    unit_half_trans_30[16][2];
		static float  	                                unit_trans_16[16][2];
			
		std::vector<std::map<int,float>>                mAreaPercentDp_16;
		std::vector<std::map<int,float>>                mAreaPercrntDp_8;
		//贴图的缓存容器
		std::map<std::string, TexturePtr>               m_TextureMap;
	public:
		const  size_t MAX_LIGHRFLARE_COUNT = 20;
	};



	static const Name g_MainLightFlareComponentName = Name("Main");

	class _EchoComponentExport LightFlareComSystem : public Singleton<LightFlareComSystem>
	{
	public:
		LightFlareComSystem();
		~LightFlareComSystem();						

		LightFlareComponentManager*						createLightFlareComponentManager(const Name& name, ProtocolComponentManager* lm);
		void											destroyLightFlareComponentManager(LightFlareComponentManager* lm);
		void											destroyLightFlareComponentManager(const Name& name);
		LightFlareComponentManager*						getLightFlareComponentManager(const Name& name);
		LightFlareComponentManager*						getMainLightFlareComponentManager();

	private:
		std::map<Name, LightFlareComponentManager*>		m_mapLightFlareManager;
	};

}

#endif
