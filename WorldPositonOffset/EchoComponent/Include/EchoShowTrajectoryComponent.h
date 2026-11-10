// Author: anhaoxiang

#ifndef __EchoShowTrajectoryComponent_h__ 
#define __EchoShowTrajectoryComponent_h__ 
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoMovableObject.h"

#include "EchoComDefineMacro.h"
#include "EchoManualObject.h"
#include "EchoComDefineMacro.h"

namespace Echo {

	class TimelineComponent;

	class _EchoComponentExport ShowTrajectoryComponentInfo : public ActorComponentInfo {
	public:
		ShowTrajectoryComponentInfo(const Name& name = COMPONENTTYPE(ShowTrajectoryComponent), ActorCOMMoveableType moveable = EDITOR_MOVEABLE);
		virtual ~ShowTrajectoryComponentInfo();
		
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* info) override;
		virtual void OverWrite() override;
	public:
		std::string              selectTimelineName;
		bool                     isShow = false;
		int                      startCnt = 0;

	};
   
	class _EchoComponentExport ShowTrajectoryComponent :public ProtocolNodeActorComponent {
		DECLAREACTORCOMTYPE_H(ShowTrajectoryComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
	/*	ShowTrajectoryComponent(ActorPtr  pActor, ActorCOMMoveableType isMoveable = EDITOR_MOVEABLE);
		~ShowTrajectoryComponent();*/
		virtual void                init(ActorComponentInfo* info) override;
		virtual void                replay(ActorComponentInfo* info) override {}
		virtual void			    copyTo(const ActorComponentPtr&  actCom) override;
		virtual	void				setActor(const ActorPtr&  pActor) override;
		virtual void                bindNode() override;
		virtual void				beginFrame(EventID, ProtocolHandle handle) override;
		virtual void				endFrame(EventID, ProtocolHandle handle)override {};
		virtual void				preUpdateAnimation(EventID, ProtocolHandle handle) override {}
		virtual void				postUpdateAnimation(EventID, ProtocolHandle handle) override {}

		virtual void				preUpdateScene(EventID, ProtocolHandle handle) override {}
		virtual void				postUpdateScene(EventID, ProtocolHandle handle) override {}
		//ImGui Set Value
		virtual void				preRenderScene(EventID, ProtocolHandle handle) override {};
		//ImGui Get Value
		virtual void				postRenderScene(EventID, ProtocolHandle handle) override {};

		void						registerTickTimeBegin() override;
		void                        getTimelineComponent();
		void                        getTrajectory();
		void                        showTrajectory();

		bool                        getIsSameSource();
		ActorPtr                       getActor();
		std::string                 getAssociatedName();
		virtual void                                    createComRootNodeImpl() override;
	public:
		ShowTrajectoryComponentInfo* TrajectoryInfo=nullptr;
	private:
		TimelineComponent*          selectTimeline=nullptr;
		ManualObject*               mHelperObj=nullptr;
		bool                        isSameSource = true;
		std::string                 associatedName;
	};

	DEFINECOMFACTORY(ShowTrajectoryComponent, EDITOR_MOVEABLE)

	class EchoShowTrajectoryScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static ShowTrajectoryComponent*                  Script_GetShowTrajectoryComponent(ActorPtr  p_Actor, const std::string name);
	};
}



#endif
