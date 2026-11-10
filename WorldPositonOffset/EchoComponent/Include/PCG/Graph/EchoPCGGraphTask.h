#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"

namespace Echo
{
	namespace PCGV2
	{
		struct _EchoComponentExport PCGGraphTaskInput
		{
			PCGGraphTaskInput(PCGTaskId taskId, PCGPin* inPin, PCGPin* outPin, bool bprovidedData = true)
				: taskId(taskId)
				, inPin(inPin)
				, outPin(outPin)
				, bProvideData(bprovidedData)
			{
			}

			PCGTaskId taskId;

			const PCGPin* inPin;

			const PCGPin* outPin;

			bool bProvideData;
		};

		struct _EchoComponentExport PCGGraphTask
		{
			std::vector<PCGGraphTaskInput> inputs;

			const PCGNode* node = nullptr;

			EchoWeakPtr<PCGComponent> sourceComponent = nullptr;

			PCGElement* element = nullptr;

			PCGContext* context = nullptr;

			PCGTaskId nodeId = c_InvalidTaskId; // task id can be offset during scheduling

			PCGTaskId compiledTaskId = c_InvalidTaskId; // The task id as it exists when compiled

			PCGTaskId parentId = c_InvalidTaskId; // The parent task id, if this is a subgraph task

			PCGStackId stackIndex = c_InvalidStackId;

			// stackContext
		};
	}
}
