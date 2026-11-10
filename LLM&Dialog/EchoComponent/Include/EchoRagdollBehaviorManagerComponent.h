#ifndef __EchoRagdollBehaviorManagerComponent_H__
#define __EchoRagdollBehaviorManagerComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class RagdollBehaviorComponentBase;
	class RagdollComponent;
}

namespace Echo
{
	class _EchoComponentExport RagdollBehaviorManagerComponentInfo : public ActorComponentInfo
	{
	public:
		RagdollBehaviorManagerComponentInfo();
		RagdollBehaviorManagerComponentInfo(const Name& mtype, ActorCOMMoveableType movableType);
		virtual ~RagdollBehaviorManagerComponentInfo();
	public:
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void						OverWrite()	override;
	public:
		void								SetDefaultBehaviorName(const Name& bhvr);
		const Name&							GetDefaultBehaviorName();
	private:
		Name								defaultBehaviorName = Name::g_cEmptyName;
	};

	class _EchoComponentExport RagdollBehaviorManagerComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(RagdollBehaviorManagerComponent)
	public:
		void								GetAllBehavior(std::vector<RagdollBehaviorComponentBase*>& dst);
		void								SetDefaultBehavior(RagdollBehaviorComponentBase* bhvr);
		RagdollBehaviorComponentBase*		GetDefaultBehavior();
	public:
		void								BehavioursForeach(RagdollComponent* master,const std::function<void(RagdollBehaviorComponentBase*)>& impl);
	private:
		ActorComponentPtr					defaultBehavior;
	};

	DEFINECOMFACTORY(RagdollBehaviorManagerComponent, ALL_MOVEABLE);

}

#endif // !__EchoRagdollBehaviorManangerComponent_H__
