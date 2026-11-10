#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"
#include "PCG/Data/EchoPCGDataCommon.h"
#include "PCG/EchoPCGContext.h"
#include "PCG/Graph/EchoPCGGraphTask.h"
#include "PCG/Graph/EchoPCGGraphCompiler.h"

namespace Echo
{
	class PCGComponent;

	namespace PCGV2
	{

		struct _EchoComponentExport PCGGraphActiveTask
		{
			std::vector<PCGGraphTaskInput> inputs;

			PCGElement* element = nullptr;

			std::unique_ptr<PCGContext> context;

			PCGTaskId nodeId = c_InvalidTaskId;
		};

		class _EchoComponentExport PCGGraphExecutor
		{
		public:
			PCGGraphExecutor() = default;

			~PCGGraphExecutor() = default;

			void Schedule(PCGGraph* graph);

			void Schedule(EchoWeakPtr<PCGComponent> inPCGComponent);

			PCGGraphCompiler& GetGraphCompiler() { return m_GraphCompiler; }

			// Tick
			void Execute();

		private:
			PCGGraphExecutor(const PCGGraphExecutor&) = delete;

			PCGGraphExecutor& operator=(const PCGGraphExecutor&) = delete;

			void BuildTaskInput(const PCGGraphTask& task, PCGDataCollection& outTaskInput);

			void StoreResults(PCGTaskId taskId, const PCGDataCollection& taskOutput);

			void ClearResults();

			PCGGraphCompiler m_GraphCompiler;

			std::vector<PCGGraphTask> m_ReadyTasks;

			std::vector<PCGGraphActiveTask> m_ActiveTasks;

			std::unordered_map<PCGTaskId, PCGDataCollection> m_OutputData;
		};

	}
}
