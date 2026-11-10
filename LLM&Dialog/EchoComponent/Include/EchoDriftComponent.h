#ifndef EchoDriftComponent_h__
#define EchoDriftComponent_h__
#include "EchoComponentPrerequisites.h" 
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoDriftManager.h"

namespace Echo
{
	class DriftComponentInfo;
	class TODComponentManager;
	struct DriftPropData
	{
		DriftGeometryType m_DriftGeometryType = DriftGeometryType_Cube;
		int						m_DriftCount = 1000;
		float					m_MinScale = 0.1f;
		float					m_MaxScale = 1.0f;
		Vector3				m_MinVelocity = Vector3(-1.0f);
		Vector3				m_MaxVelocity = Vector3(1.0f);
		Vector3				m_MinRotateAxis = Vector3(-1.0f);
		Vector3				m_MaxRotateAxis = Vector3(1.0f);
		float					m_MinRotateSpeed = -1.0f;
		float					m_MaxRotateSpeed = 1.0f;
		float					m_AreaRange = 1000.0f;
		float					m_InnerRadius = 0.0f;
		float					m_OuterRadius = 1000.0f;
		float					m_FadeInTimeOffset = 1.0f;
		bool					m_ShadowEnable = false;
	};
	class _EchoComponentExport DriftComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(DriftComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							unInit() override;
		virtual void							copyTo(const ActorComponentPtr& newComp)override;
		virtual void							visibleStateChanged(bool visibleState)override;

		void										use();
		void										unuse();
		int											getPriority() const { return m_nPriority; };
		void										setPriority(const int& val);
		void										createDrifts(const Name& meshName, const DriftPropData& data);
		void										createDriftsImpl(const Name& meshName, const int& count);
		void										removeDrifts(const Name& meshName);
		void										addDriftsCount(const Name& meshName, const int& count);
		void										removeDriftsCount(const Name& meshName, const int& count);
		void										changeDriftsGeometryType(const Name& meshName, const DriftGeometryType& type);
		void										changeDriftsMesh(const Name& oldMeshName, const Name& newMeshName);
		void										changeDriftsCount(const Name& mesh, const int& count);
		void										changeDriftsAreaRange(const Name& mesh, const float& areaRange);
		void										changeDriftsAreaInnerRadius(const Name& mesh, const float& outerRadius);
		void										changeDriftsAreaOuterRadius(const Name& mesh, const float& outerRadius);
		void										changeDriftsVelocity(const Name& mesh, const Vector3& min, const Vector3& max);
		void										changeDriftsRotateAxis(const Name& mesh, const Vector3& min, const Vector3& max);
		void										changeDriftsRotateSpeed(const Name& mesh, const float& min, const float& max);
		void										changeDriftsSize(const Name& meshName, const float& min, const float& max);
		void										changeDriftsFadeInOffsetTime(const Name& meshName, const float& range);
		void										enableDriftsShadow(const Name& meshName, const bool& bShadowEnable);
		void										setFadeInTime(const float& time);
		const std::map<Name, DriftPropData>&        getAllDriftPropData()const { return m_mapDriftProp; };
		void										setDriftsPropData(const std::map<Name, DriftPropData>& dataMap);
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
	private:
		int m_nPriority = 0;
		bool m_bEnable = false;
		std::map<Name, DriftPropData> m_mapDriftProp;

		TODComponentManager*          m_tMgr = nullptr;
	};

	class _EchoComponentExport DriftComponentInfo : public ActorComponentInfo
	{
	public:
		DriftComponentInfo() {}
		DriftComponentInfo(const Name& comType, ActorCOMMoveableType moveType);
		virtual ~DriftComponentInfo();
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* comInfo) override;
		virtual void Overlay(ActorComponentInfo* comInfo) override;
		virtual void OverWrite() override; 

#ifdef ECHO_EDITOR
		virtual bool										getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		std::map<Name, DriftPropData> m_DriftPropMap;
		int m_Priority = 0;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(DriftComponent, UNMOVEABLE)
}
#endif
