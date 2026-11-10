#ifndef _EchoDistantBuildingComponent_H__
#define _EchoDistantBuildingComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class ImposterComponent;
	class HLODComponent;
	class BuildingComponent;
 
	//enum class DistantBuildingFactorType
	//{
	//	ByVideoConfig = 0,
	//	ByDistance,
	//	BySmallObject,

	//	Count
	//};

	struct _db_UpdateFactorConfig
	{
		ImposterComponent* mImposter = nullptr;
		HLODComponent* mHlod = nullptr;
		BuildingComponent* mBuilding = nullptr;

		float mLod1Factor = 0.f;
		float mLod2Factor = 0.f;
		float mHlodFactor = 0.f;
		float mImposterFactor = 0.f;
		float mCullFactor = 0.f;

		uint8 mUpdateKey = 0;
	};

	class _EchoComponentExport DistantBuildingComponent : public ProtocolActorComponent
	{
		DECLAREACTORCOMTYPE_H(DistantBuildingComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		enum UpdateFactor
		{
			Building	= 0b1,
			HLOD		= 0b10,
			Imposter	= 0b100
		};

		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		//virtual void						connectActor() override;
		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;
		virtual void						EditorStopImp() override;
		virtual void						NotifiChildImp(int changeMask) override;
		virtual void						showEditorHelper(bool isShow) override;

		void								setImposterActor(ActorPtr pActor);
		void								setHlodActor(ActorPtr pActor);
		void								setBuildingActor(ActorPtr pActor);
		void								setImposterActor(const std::string& relationActor);
		void								setHlodActor(const std::string& relationActor);
		void								setBuildingActor(const std::string& relationActor);

		ActorPtr							getImposterActor();
		ImposterComponent*					getImposterComponent();
		ActorPtr							getHlodActor();
		HLODComponent*						getHLODComponent();
		ActorPtr							getBuildingActor();
		BuildingComponent*					getBuildingComponent();

		// 会在初始化时给actor刷一次; 
		void								setLodLevel1Factor(float fact);			// lod1 距离
		void								setLodLevel2Factor(float fact);			// lod2 距离
		void								setHlodFactor(float fact);				// hlod 距离
		void								setImposterFactor(float fact);			// imposter 距离
		void								setCullFactor(float fact);				// 裁剪 距离
		void								setBuildCullFactor(float fact);
		void								setHlodCullFactor(float fact);
		//void								setFactorType(int type);				// 因子类型 enum class DistantBuildingFactorType;
		void								enableFactor(bool enable);				// 启用功能 默认关闭，防止添加新组件的时候刷新
		//void								UpdateFactor();
		void								showFactor(bool visit);

		class VideoConfig*					getVideoConfig();

		float								getLodLevel1Factor(bool isReal = false);			
		float								getLodLevel2Factor(bool isReal = false);
		float								getBuildCullFactor();
		float								getHlodFactor(bool isReal = false);
		float								getHlodCullFactor();
		float								getImposterFactor(bool isReal = false);
		float								getCullFactor(bool isReal = false);
		//DistantBuildingFactorType			getFactorType();
		bool								isEnableFactor();
		bool								isShowFactor();

		// 参考距离
		//float								getLodLevel1ReferenceDistance();
		//float								getLodLevel2ReferenceDistance();
		//float								getHlodReferenceDistance();
		//float								getImposterReferenceDistance();
		//float								getCullReferenceDistance();

		//void								setFactorConfig(const _db_UpdateFactorConfig& data) { m_factorConfig = data; }
		void								setClientFactorConfig(const _db_UpdateFactorConfig& data);
#ifdef ECHO_EDITOR
		void								createHelper();
		void								updateHelper();
		void								deleteHelper();
#endif // ECHO_EDITOR


	private:
		std::string							m_ImposterActorName;
		std::string							m_HlodActorName;
		std::string							m_BuildingActorName;

		float 								m_lod1Factor			= 0.f;
		float								m_lod2Factor			= 0.f;
		float								m_BuildCullFactor		= 0.f;
		float								m_HlodFactor			= 0.f;
		float								m_HlodCullFactor		= 0.f;
		float								m_ImposterFactor		= 0.f;
		float								m_CullFactor			= 0.f;
		//DistantBuildingFactorType			m_FactorType			= DistantBuildingFactorType::ByVideoConfig;

		bool								m_Enable				= false;
		bool								m_ShowFactor			= false;

		float 								m_realLod1Factor = 0.f;
		float								m_realLod2Factor = 0.f;
		float								m_realHlodFactor = 0.f;
		float								m_realImposterFactor = 0.f;
		float								m_realCullFactor = 0.f;

		_db_UpdateFactorConfig				m_factorConfig;

#ifdef ECHO_EDITOR
	public:
		float								m_CameraFactor = 0.f;
		static bool							m_renderHelper;
	private:
		// 渲染数据
		HelperObject*						m_helperBox = nullptr;
#endif // ECHO_EDITOR
	};

	class _EchoComponentExport DistantBuildingComponentInfo : public ActorComponentInfo
	{
	public:
		DistantBuildingComponentInfo() {}
		DistantBuildingComponentInfo(const Name& type, ActorCOMMoveableType moveable = UNMOVEABLE);
		~DistantBuildingComponentInfo();

		_DECLCOMINFO_VIRFUN;
	public:
		std::string							m_ImposterActorName;
		std::string							m_HlodActorName;
		std::string							m_BuildingActorName;

		float 								m_lod1Factor			= 0.f;
		float								m_lod2Factor			= 0.f;
		float								m_BuildCullFactor		= 0.f;
		float								m_HlodFactor			= 0.f;
		float								m_HlodCullFactor		= 0.f;
		float								m_ImposterFactor		= 0.f;
		float								m_CullFactor			= 0.f;
		//int									m_FactorType			= (int)DistantBuildingFactorType::ByVideoConfig;

		bool								m_Enable = false;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(DistantBuildingComponent, UNMOVEABLE)
}

#endif
