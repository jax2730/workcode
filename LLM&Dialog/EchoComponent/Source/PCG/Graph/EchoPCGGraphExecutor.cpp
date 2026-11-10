#include "EchoComponentStableHeaders.h"
#include "PCG/Graph/EchoPCGGraphExecutor.h"

#include "PCG/EchoPCGComponent.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGSettings.h"

namespace Echo
{
	namespace PCGV2
	{
		void PCGGraphExecutor::Schedule(PCGGraph* graph)
		{
			std::vector<PCGGraphTask> compiledTasks = m_GraphCompiler.GetCompiledTasks(graph);

			// TODO: Schedule to ready tasks
			m_ReadyTasks.reserve(m_ReadyTasks.size() + compiledTasks.size());
			m_ReadyTasks.insert(m_ReadyTasks.end(), std::make_move_iterator(compiledTasks.begin()),
				std::make_move_iterator(compiledTasks.end()));
		}

		void PCGGraphExecutor::Schedule(EchoWeakPtr<PCGComponent> inPCGComponent)
		{
			std::vector<PCGGraphTask> compiledTasks = m_GraphCompiler.GetCompiledTasks(inPCGComponent->GetRawGraph());

			// TODO: Schedule to ready tasks
			m_ReadyTasks.reserve(m_ReadyTasks.size() + compiledTasks.size());
			m_ReadyTasks.insert(m_ReadyTasks.end(), std::make_move_iterator(compiledTasks.begin()),
				std::make_move_iterator(compiledTasks.end()));

			for (auto& task : m_ReadyTasks)
			{
				task.sourceComponent = inPCGComponent;
			}
		}

		void PCGGraphExecutor::Execute()
		{
			for (int32 readyTaskIndex = static_cast<int32>(m_ReadyTasks.size()) - 1; readyTaskIndex >= 0; --readyTaskIndex)
			{
				PCGGraphTask& task = m_ReadyTasks[readyTaskIndex];
				// Build input
				PCGDataCollection taskInput;
				// BuildTaskInput(task, taskInput);

				// Initialize the element
				if (!task.element)
				{
					const PCGSettings* settings = task.node->GetSettings();
					if (settings)
					{
						task.element = settings->GetElement();
					}
				}

				// Drop task without element
				if (!task.element)
				{
					std::swap(task, m_ReadyTasks.back());
					m_ReadyTasks.pop_back();
					continue;
				}

				// Allocate context if not previous done
				if (!task.context)
				{
					task.context = task.element->Initialize(taskInput, task.node);
					task.context->InitializeSettings();
					task.context->taskId = task.nodeId;
					task.context->compiledTaskId = task.compiledTaskId;
					task.context->sourceComponent = task.sourceComponent;
				}

				// Prepare active tasks
				m_ActiveTasks.emplace_back();
				PCGGraphActiveTask& activeTask = m_ActiveTasks.back();
				activeTask.inputs = std::move(task.inputs);
				activeTask.element = task.element;
				activeTask.nodeId = task.nodeId;
				activeTask.context = std::unique_ptr<PCGContext>(task.context);

				std::swap(task, m_ReadyTasks.back());
				m_ReadyTasks.pop_back();
			}

			for (int32 activeTaskIndex = static_cast<int32>(m_ActiveTasks.size()) - 1; activeTaskIndex >= 0; --activeTaskIndex)
			{
				PCGGraphActiveTask& activeTask = m_ActiveTasks[activeTaskIndex];
				// Execute the task
				if (activeTask.element && activeTask.context)
				{
					// Prepare the inputs
					// TODO: remove this, prepare inputs before bushed into ready tasks
					for (const PCGGraphTaskInput& input : activeTask.inputs)
					{
						if (!input.bProvideData)
						{
							continue;
						}

						if (m_OutputData.contains(input.taskId))
						{
							const PCGDataCollection& cachedInputCollection = m_OutputData[input.taskId];
							for (const PCGTaggedData& cachedInputData : cachedInputCollection.taggedDataList)
							{
								PCGTaggedData& inputData = activeTask.context->inputData.AddData(cachedInputData);
								inputData.pinLabel = input.outPin->m_Properties.label;
							}
						}
					}

					// Execute
					bool bTaskDone = activeTask.element->Execute(activeTask.context.get());
					if (bTaskDone)
					{
						StoreResults(activeTask.nodeId, activeTask.context->outputData);
					}
					else
					{
						break;
					}
				}
			}

			m_ActiveTasks.clear();
		}

		void PCGGraphExecutor::BuildTaskInput(const PCGGraphTask& task, PCGDataCollection& outTaskInput)
		{
			// Build input data for the task
			for (const PCGGraphTaskInput& input : task.inputs)
			{
				// assert(m_OutputData.find(input.taskId) != m_OutputData.end());

				if (!input.bProvideData)
				{
					continue;
				}

				if (m_OutputData.contains(input.taskId))
				{
					const PCGDataCollection& inputCollection = m_OutputData[input.taskId];
					outTaskInput.AddData(inputCollection.taggedDataList);
				}
			}

		}

		void PCGGraphExecutor::StoreResults(PCGTaskId taskId, const PCGDataCollection& taskOutput)
		{
			m_OutputData[taskId] = taskOutput;
		}

		void PCGGraphExecutor::ClearResults()
		{
			m_OutputData.clear();
		}
	}
}

