#include "EchoComponentStableHeaders.h"
#include "PCG/Graph/EchoPCGGraphCompiler.h"

#include "PCG/EchoPCGGraph.h"
#include "PCG/EchoPCGPin.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGGraphCompiler::~PCGGraphCompiler()
		{

		}

		std::vector<PCGGraphTask> PCGGraphCompiler::GetCompiledTasks(PCGGraph* graph, bool bIsTopGraph/* = true*/)
		{
			std::vector<PCGGraphTask> compiledTasks;

			Compile(graph);

			std::shared_lock<std::shared_mutex> _(m_RwLock);
			if (m_GraphToTaskMap.contains(graph))
			{
				compiledTasks = m_GraphToTaskMap[graph];
			}

			return compiledTasks;
		}

		void PCGGraphCompiler::RemoveFromCache(PCGGraph* graph)
		{
			std::unique_lock<std::shared_mutex> _(m_RwLock);
			m_GraphToTaskMap.erase(graph);
		}

		void PCGGraphCompiler::OffsetNodeIds(std::vector<PCGGraphTask>& tasks, PCGTaskId offset, PCGTaskId parentId)
		{
			for (PCGGraphTask& task : tasks)
			{
				task.nodeId += offset;

				if (task.parentId == c_InvalidTaskId)
				{
					task.parentId = parentId;
				}
				else
				{
					task.parentId += offset;
				}

				for (PCGGraphTaskInput& input : task.inputs)
				{
					input.taskId += offset;
				}

			}
		}

		void PCGGraphCompiler::Compile(PCGGraph* graph)
		{
			// Check if the graph is already compiled and cached
			bool bAlreadyCached = false;
			{
				std::shared_lock<std::shared_mutex> _(m_RwLock);
				bAlreadyCached = m_GraphToTaskMap.contains(graph);
			}
			if (bAlreadyCached)
			{
				return;
			}

			PCGTaskId firstId = 0;
			std::vector<PCGGraphTask> compiledTasks = CompileGraph(graph, firstId);

			if (!compiledTasks.empty())
			{
				std::unique_lock<std::shared_mutex> _(m_RwLock);
				if (!m_GraphToTaskMap.contains(graph)) // Double check
				{
					m_GraphToTaskMap.emplace(graph, std::move(compiledTasks));
				}
			}
		}

		std::vector<PCGGraphTask> PCGGraphCompiler::CompileGraph(PCGGraph* graph, PCGTaskId& nextId)
		{
			if (!graph)
			{
				return std::vector<PCGGraphTask>();
			}

			std::vector<PCGGraphTask> compiledTasks;

			// Prime the node queue with all nodes that have no inbound edges
			std::vector<const PCGNode*> nodeStack;
			std::unordered_map<const PCGNode*, PCGTaskId> idMapping;

			for (const UniquePCGNodePtr& node : graph->GetNodes())
			{
				if (node->GetInputPins().empty())
				{
					nodeStack.push_back(node.get());
				}
			}

			nodeStack.push_back(graph->GetInputNode());

			while (!nodeStack.empty())
			{
				const PCGNode* node = nodeStack.back();
				nodeStack.pop_back();

				const PCGTaskId nodeId = nextId++;
				compiledTasks.emplace_back();
				PCGGraphTask& task = compiledTasks.back();

				task.node = node;
				task.nodeId = nodeId;

				for (const UniquePCGPinPtr& inputPin : node->GetInputPins())
				{
					for (const PCGEdgePtr& inboundEdge : inputPin->m_Edges)
					{
						if (!inboundEdge->IsValid())
						{
							continue;
						}

						// Make sure the previous node is compiled
						auto it = idMapping.find(inboundEdge->m_InputPin->m_Node);
						if (it != idMapping.end())
						{
							PCGTaskId inputId = it->second;
							task.inputs.emplace_back(inputId, inboundEdge->m_InputPin, inboundEdge->m_OutputPin);
						}
						else
						{
							return std::vector<PCGGraphTask>();
						}
					}
				}

				assert(!idMapping.contains(node));
				idMapping.emplace(node, nodeId);

				// Push next ready nodes on the stack
				for (const UniquePCGPinPtr& outputPin : node->GetOutputPins())
				{
					for (const PCGEdgePtr& outboundEdge : outputPin->m_Edges)
					{
						if (!outboundEdge->IsValid())
						{
							continue;
						}

						const PCGNode* outboundNode = outboundEdge->m_OutputPin->m_Node;

						if (std::find(nodeStack.begin(), nodeStack.end(), outboundNode) != nodeStack.end())
						{
							// The node is already in the stack
							continue;
						}

						// Check the nodes connected to the outputPins of current node
						// If the prerequisite conditions are met, push them into the stack
						bool bAllPrerequisitesMet = true;
						for (const UniquePCGPinPtr& outboundNodeInputPin : outboundNode->GetInputPins())
						{
							for (const PCGEdgePtr& outboundNodeInboundEdge : outboundNodeInputPin->m_Edges)
							{
								if (outboundNodeInboundEdge->IsValid())
								{
									bAllPrerequisitesMet &= idMapping.contains(outboundNodeInboundEdge->m_InputPin->m_Node);
								}
							}
						}
						if (bAllPrerequisitesMet)
						{
							nodeStack.push_back(outboundNode);
						}
					}
				}
			}

			return compiledTasks;
		}

	}

}
