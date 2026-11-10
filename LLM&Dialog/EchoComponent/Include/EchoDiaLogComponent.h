#ifndef _EchoDialogComponent_H
#define _EchoDialogComponent_H

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class _EchoComponentExport DialogComponentInfo : public ActorComponentInfo
	{
	public:
		DialogComponentInfo() = default;
		DialogComponentInfo(const DialogComponentInfo&) = delete;
		DialogComponentInfo& operator = (const DialogComponentInfo&) = delete;
		DialogComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~DialogComponentInfo() override = default;
		void ExportData(cJSON* jsonNode) override;
		void ImportData(cJSON* jsonNode) override;
		void CopyTo(ActorComponentInfo* comInfo) override;
		void Overlay(ActorComponentInfo* srcInfo) override;
		void OverWrite() override;
		int m_type = 2001;

		EDITOR_REFLECT
	};

	class _EchoComponentExport DialogComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(DialogComponent)

	public:
	

		void init(ActorComponentInfo* info) override;
		void copyTo(const ActorComponentPtr&) override;
		void connectCom() override;
		void disconnectCom() override;

	protected:
		DialogComponentInfo* m_info = nullptr;


	};
	DEFINECOMFACTORY(DialogComponent, UNMOVEABLE)

}

#endif
