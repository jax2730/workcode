#pragma once
#include "EchoComponentPrerequisites.h"

#include <shared_mutex>

#include "PCG/Graph/EchoPCGGraphTask.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGGraph;

		using PCGGraphToTaskMap = std::unordered_map<PCGGraph*, std::vector<PCGGraphTask>>;

		class _EchoComponentExport PCGGraphCompiler
		{
		public:
			~PCGGraphCompiler();

			std::vector<PCGGraphTask> GetCompiledTasks(PCGGraph* graph, bool bIsTopGraph = true);

			void NotifyGraphChanged(PCGGraph* graph) { RemoveFromCache(graph); }

			static void OffsetNodeIds(std::vector<PCGGraphTask>& tasks, PCGTaskId offset, PCGTaskId parentId);

		private:

			void Compile(PCGGraph* graph);

			void RemoveFromCache(PCGGraph* graph);

			// void CompiledTopGraph(PCGGraph* graph);
			std::vector<PCGGraphTask> CompileGraph(PCGGraph* graph, PCGTaskId& nextId);

			PCGGraphToTaskMap m_GraphToTaskMap;

			std::shared_mutex m_RwLock;
		};
	}

}
