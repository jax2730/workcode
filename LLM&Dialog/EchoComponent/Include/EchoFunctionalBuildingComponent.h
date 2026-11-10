#ifndef _EchoFunctionalBuildingComponent_H
#define _EchoFunctionalBuildingComponent_H

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoComDefineMacro.h"

namespace Echo
{

	class _EchoComponentExport FunctionalBuildingComponentInfo : public ActorComponentInfo
	{
	public:
		FunctionalBuildingComponentInfo()                                    = default;
		FunctionalBuildingComponentInfo(const FunctionalBuildingComponentInfo&)            = delete;
		FunctionalBuildingComponentInfo& operator=(const FunctionalBuildingComponentInfo&) = delete;
		FunctionalBuildingComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~FunctionalBuildingComponentInfo() override = default;
		void ExportData(cJSON* jsonNode) override;
		void ImportData(cJSON* jsonNode) override;
		void CopyTo(ActorComponentInfo* comInfo) override;
		void Overlay(ActorComponentInfo* srcInfo) override;
		void OverWrite() override;

		//FuncBuildingType m_type              = FuncBuildingType::FBTMacdonal;
		int m_type = 2001;

		EDITOR_REFLECT
	};

	class _EchoComponentExport FunctionalBuildingComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(FunctionalBuildingComponent)

	public:
		void init(ActorComponentInfo* info) override;
		void copyTo(const ActorComponentPtr&) override;
		void connectCom() override;
		void disconnectCom() override;
		//static void SetType(int type);

	protected:
		FunctionalBuildingComponentInfo* m_Info = nullptr;
	};

	DEFINECOMFACTORY(FunctionalBuildingComponent, UNMOVEABLE)
}

#endif
