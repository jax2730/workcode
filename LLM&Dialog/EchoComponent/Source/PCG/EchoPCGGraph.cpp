#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGGraph.h"

#include "EchoResourceManagerFactory.h"
#include "EchoDataStreamManager.h"

#include "PCG/EchoPCGPin.h"
#include "PCG/Elements/EchoPCGInputOutputSettings.h"
#include "PCG/EchoPCGSystem.h"
#include "PCG/Graph/EchoPCGGraphCompiler.h"
#include "PCG/Reflection/EchoPCGSerializer.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGEdge::PCGEdge()
		{

		}

		PCGEdge::~PCGEdge()
		{

		}

		PCGPin* PCGEdge::GetOtherPin(PCGPin* pin) const
		{
			assert(pin == m_InputPin || pin == m_OutputPin);
			return pin == m_InputPin ? m_OutputPin : m_InputPin;
		}

		bool PCGEdge::IsValid() const
		{
			return m_InputPin && m_OutputPin;
		}

		PCGNode::PCGNode(PCGGraph* graph)
			: m_Graph(graph)
		{

		}

		PCGNode::~PCGNode()
		{
		}

		PCGGraph* PCGNode::GetGraph() const
		{
			return m_Graph;
		}

		PCGPin* PCGNode::GetOutputPin(const Name& label)
		{
			for (auto& pin : m_OutputPins)
			{
				if (pin && pin->m_Properties.label == label)
				{
					return pin.get();
				}
			}
			return nullptr;
		}

		PCGPin* PCGNode::GetInputPin(const Name& label)
		{
			for (auto& pin : m_InputPins)
			{
				if (pin && pin->m_Properties.label == label)
				{
					return pin.get();
				}
			}
			return nullptr;
		}

		const PCGPin* PCGNode::GetOutputPin(const Name& label) const
		{
			for (auto& pin : m_OutputPins)
			{
				if (pin && pin->m_Properties.label == label)
				{
					return pin.get();
				}
			}
			return nullptr;
		}

		const PCGPin* PCGNode::GetInputPin(const Name& label) const
		{
			for (auto& pin : m_InputPins)
			{
				if (pin && pin->m_Properties.label == label)
				{
					return pin.get();
				}
			}
			return nullptr;
		}

		Name PCGNode::GetNodeTitle() const
		{
			if (!m_NodeTitle.empty())
			{
				return m_NodeTitle;
			}

			return GetDefaultNodeTitle();
		}

		bool PCGNode::IsInputPinConnected(const Name& Label) const
		{
			if (const PCGPin* InputPin = GetInputPin(Label))
			{
				return InputPin->IsConnected();
			}
			else
			{
				return false;
			}
		}

		bool PCGNode::IsOutputPinConnected(const Name& Label) const
		{
			if (const PCGPin* OutputPin = GetOutputPin(Label))
			{
				return OutputPin->IsConnected();
			}
			else
			{
				return false;
			}
		}

		void PCGNode::SetNodeTitle(const Name& title)
		{
			if (title != m_NodeTitle)
			{
				m_NodeTitle = title;
			}
		}

		Name PCGNode::GetDefaultNodeTitle() const
		{
			if (m_Settings)
			{
				return m_Settings->GetDefaultNodeTitle();
			}

			return Name("Node without Settings");
		}

		const std::vector<UniquePCGPinPtr>& PCGNode::GetInputPins() const
		{
			return m_InputPins;
		}

		const std::vector<UniquePCGPinPtr>& PCGNode::GetOutputPins() const
		{
			return m_OutputPins;
		}

		bool PCGNode::HasInboundEdges()
		{
			for (const UniquePCGPinPtr& InputPin : m_InputPins)
			{
				for (const PCGEdgePtr& InboundEdge : InputPin->m_Edges)
				{
					if (InboundEdge->IsValid())
					{
						return true;
					}
				}
			}

			return false;
		}

		void PCGNode::SetPosition(int positionX, int positionY)
		{
			m_PositionX = positionX;
			m_PositionY = positionY;
		}

		void PCGNode::SetSettings(std::unique_ptr<PCGSettings> settings, bool bUpdatePins /*= true*/)
		{
			m_Settings = std::move(settings);
			if (bUpdatePins)
			{
				UpdatePins();
			}
		}

		PCGSettings* PCGNode::GetSettings() const
		{
			return m_Settings.get();
		}

		void PCGNode::MarkModified()
		{
			m_Graph->MarkModified();
		}

		void PCGNode::UpdatePins()
		{
			const PCGSettings* settings = GetSettings();

			if (!settings)
			{
				if (!m_InputPins.empty() || !m_OutputPins.empty())
				{
					for (auto& Pin : m_InputPins)
					{
						if (Pin)
						{
							Pin->BreakAllEdges();
						}
					}

					for (auto& Pin : m_OutputPins)
					{
						if (Pin)
						{
							Pin->BreakAllEdges();
						}
					}

					m_InputPins.clear();
					m_OutputPins.clear();
				}

				return;
			}

			std::vector<PCGPinProperties> InboundPinProperties = m_Settings->AllInputPinProperties();
			std::vector<PCGPinProperties> OutboundPinProperties = m_Settings->AllOutputPinProperties();

			auto UpdatePins = [this](std::vector<UniquePCGPinPtr>& pins, std::vector<PCGPinProperties>& PinProperties)
			{
				bool bAppliedEdgeChanges = false;
				bool bChangedPins = false;
				bool bChangedTooltips = false;

				// Find unmatched pins vs. properties on a name basis
				std::vector<PCGPin*> unmatchedPins;

				for (auto& pin : pins)
				{
					auto matchingPropsIter = std::find_if(PinProperties.begin(), PinProperties.end(), [&pin](const PCGPinProperties& prop)
					{
						return prop.label == pin->m_Properties.label;
					});

					if (matchingPropsIter != PinProperties.end())
					{
						const PCGPinProperties& matchingProperties = *matchingPropsIter;

						if (pin->m_Properties != matchingProperties)
						{
							pin->m_Properties = matchingProperties;
							bAppliedEdgeChanges |= pin->BreakAllIncompatibleEdges();
							bChangedPins = true;
						}
						else if (pin->m_Properties.tooltip != matchingProperties.tooltip)
						{
							pin->SetTooltip(matchingProperties.tooltip);
							bChangedTooltips = true;
						}
					}
					else
					{
						unmatchedPins.push_back(pin.get());
					}
				}

				// Remove unmatched properties vs. pins on a name basis
				std::vector<PCGPinProperties> unmatchedProperties;
				for (auto& prop : PinProperties)
				{
					auto matchingPinIter = std::find_if(pins.begin(), pins.end(), [&prop](const UniquePCGPinPtr& pin)
					{
						return pin->m_Properties.label == prop.label;
					});

					if (matchingPinIter == pins.end())
					{
						unmatchedProperties.push_back(prop);
					}
				}

				auto RemovePins = [&pins, &bAppliedEdgeChanges](std::vector<PCGPin*>& pinsToRemove, bool bRemoveFromAllNames)
				{
					for (int32 removedPinIndex = static_cast<int32>(pinsToRemove.size()) - 1; removedPinIndex >= 0; --removedPinIndex)
					{
						PCGPin* pinToRemove = pinsToRemove[removedPinIndex];
						auto it = std::find_if(pins.begin(), pins.end(), [pinToRemove](const UniquePCGPinPtr& pin)
						{
							return pin.get() == pinToRemove;
						});

						if (it != pins.end())
						{
							bAppliedEdgeChanges |= pinToRemove->BreakAllEdges();
							pins.erase(it);
						}
					}
				};
				RemovePins(unmatchedPins, true);

				// Add new pins
				for (const PCGPinProperties& prop : unmatchedProperties)
				{
					auto matchingPinIter = std::find_if(pins.begin(), pins.end(), [&prop](const UniquePCGPinPtr& pin)
					{
						return pin->m_Properties.label == prop.label;
					});

					if (matchingPinIter == pins.end())
					{
						UniquePCGPinPtr newPin = std::make_unique<PCGPin>(this);
						newPin->m_Properties = prop;
						pins.push_back(std::move(newPin));
						bChangedPins = true;
					}
				}
			};

			UpdatePins(m_InputPins, InboundPinProperties);
			UpdatePins(m_OutputPins, OutboundPinProperties);
		}

		PCGGraph::PCGGraph(ResourceManagerBase* creator, const Name& name, bool isManual, ManualResourceLoader* loader)
			: ResourceBase(creator, name, isManual, loader)
		{
			InitializeDefaultInputOutput();
		}

		PCGGraph::~PCGGraph()
		{
			Clear();
		}

		PCGNode* PCGGraph::AddNode(std::unique_ptr<PCGNode> node)
		{
			m_Nodes.push_back(std::move(node));
			return m_Nodes.back().get();
		}

		PCGNode* PCGGraph::AddNode(std::unique_ptr<PCGSettings> settings)
		{
			assert(settings && "PCGGraph::AddNode: settings is null");
			std::unique_ptr<PCGNode> newNode = settings->CreateNode();
			newNode->SetGraph(this);
			newNode->SetSettings(std::move(settings));

			m_Nodes.push_back(std::move(newNode));
			return m_Nodes.back().get();
		}

		PCGNode* PCGGraph::CreateAndAddNode(const Name& settingsClassName)
		{
			return AddNode(CreatePCGSettings(Name(settingsClassName)));
		}

		void PCGGraph::RemoveNode(PCGNode* inNode)
		{
			RemoveNodes({ inNode });
		}

		void PCGGraph::RemoveNodes(const std::vector<PCGNode*>& inNodes)
		{
			if (inNodes.empty())
			{
				return;
			}

			for (PCGNode* nodeToRemove: inNodes)
			{
				assert(nodeToRemove);

				for (const UniquePCGPinPtr& inputPin : nodeToRemove->GetInputPins())
				{
					inputPin->BreakAllEdges();
				}

				for (const UniquePCGPinPtr& outputPin : nodeToRemove->GetOutputPins())
				{
					outputPin->BreakAllEdges();
				}

				auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeToRemove](UniquePCGNodePtr& node)
					{
						return node.get() == nodeToRemove;
					});

				if (it != m_Nodes.end())
				{
					m_Nodes.erase(it);
				}
			}
		}

		bool PCGGraph::AddEdge(PCGNode* from, const Name& fromPinLabel, PCGNode* to, const Name& toPinLabel)
		{
			if (!from || !to)
			{
				return false;
			}
			PCGPin* fromPin = from->GetOutputPin(fromPinLabel);
			PCGPin* toPin = to->GetInputPin(toPinLabel);

			if (!fromPin || !toPin)
			{
				return false;
			}

			fromPin->AddEdgeTo(toPin);

			return true;
		}

		bool PCGGraph::RemoveEdge(PCGNode* from, const Name& fromPinLabel, PCGNode* to, const Name& toPinLabel)
		{
			if (!from || !to)
			{
				return false;
			}
			PCGPin* fromPin = from->GetOutputPin(fromPinLabel);
			PCGPin* toPin = to->GetInputPin(toPinLabel);

			bool bChanged = fromPin->BreakEdgeTo(toPin);

			return bChanged;
		}

		PCGNode* PCGGraph::GetInputNode() const
		{
			return m_InputNode.get();
		}

		PCGNode* PCGGraph::GetOutputNode() const
		{
			return m_OutputNode.get();
		}

		void PCGGraph::MarkModified(bool bNeedRecompile)
		{
			if (bNeedRecompile)
			{
				ClearCompiledTasks();
			}
			m_bModified = true;
		}

		bool PCGGraph::IsModified()
		{
			return m_bModified;
		}

		bool PCGGraph::ExportToFile(const String& filepath)
		{
			bool bSuccess = PCGGraphJsonSerializer::Export(*this, filepath);
			if (bSuccess)
			{
				m_bModified = false;
			}

			return bSuccess;
		}

		bool PCGGraph::ExportToFile()
		{
			return ExportToFile(mName.c_str());
		}

		void PCGGraph::Clear()
		{
			ClearCompiledTasks();
			m_Nodes.clear();
			m_InputNode = nullptr;
			m_OutputNode = nullptr;
		}

		void PCGGraph::prepareImpl(void)
		{
			Clear();
			PCGGraphJsonSerializer::Import(*this, mName.c_str());
			m_bModified = false;
		}

		void PCGGraph::unprepareImpl(void)
		{
			Clear();
		}

		void PCGGraph::loadImpl(void)
		{

		}

		void PCGGraph::unloadImpl(void)
		{

		}

		size_t PCGGraph::calcMemUsage(void) const
		{
			return 0;
		}

		void PCGGraph::ClearCompiledTasks()
		{
			PCGGraphCompiler& compiler = PCGSystem::GetInstance()->GetGraphCompiler();
			compiler.NotifyGraphChanged(this);
		}

		void PCGGraph::InitializeDefaultInputOutput()
		{
			Name inputOutputSettingsName = Name("PCGInputOutputSettings");
			m_InputNode = std::make_unique<PCGNode>(this);
			m_InputNode->SetSettings(CreatePCGSettings(inputOutputSettingsName));
			m_InputNode->GetSettings<PCGInputOutputSettings>()->UseAsInput();

			m_OutputNode = std::make_unique<PCGNode>(this);
			m_OutputNode->SetSettings(CreatePCGSettings(inputOutputSettingsName));
			m_OutputNode->GetSettings<PCGInputOutputSettings>()->UseAsOutput();
		}

		bool PCGGraphJsonSerializer::Export(const PCGGraph& graph, const std::string& filepath)
		{
			DataStreamPtr pDataStream(Root::instance()->CreateDataStream(filepath.c_str()));
			if (pDataStream.isNull())
			{
				Echo::LogManager::instance()->logMessage("[" + filepath + "] export failed.");
				return false;
			}

			cJSON* jGraph = SerializeGraph(graph);
			char* data = cJSON_Print(jGraph);
			pDataStream->write(data, strlen(data));
			free(data);
			cJSON_Delete(jGraph);

			return true;
		}

		bool PCGGraphJsonSerializer::Import(PCGGraph& outGraph, const std::string& filepath)
		{
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(filepath.c_str()));
			if (pDataStream.isNull())
			{
				ECHO_EXCEPT(Exception::ERR_FILE_NOT_FOUND, std::string("Cannot locate resource ") + filepath.c_str(), "PCGGraphJsonSerializer::Import");
				return false;
			}

			size_t bufferSize = pDataStream->size();
			char* jsonContentBuffer = new char[bufferSize];
			memset(jsonContentBuffer, 0, bufferSize);
			pDataStream->read(jsonContentBuffer, bufferSize);

			cJSON* jGraph = cJSON_Parse(jsonContentBuffer);

			bool bSuccess = DeserializeGraph(jGraph, outGraph);

			cJSON_Delete(jGraph);
			delete[] jsonContentBuffer;

			return bSuccess;
		}

		cJSON* PCGGraphJsonSerializer::SerializeGraph(const PCGGraph& graph)
		{
			cJSON* jGraph = cJSON_CreateObject();

			// Scan the graph and build necessary data structures
			std::vector<PCGNode*> nodeList;
			std::unordered_map<PCGNode*, uint32> nodeToIndexMap;

			std::vector<PCGEdge*> edgeList;
			std::unordered_set<PCGEdge*> edgeSet;

			auto RegisterEdge = [&edgeList, &edgeSet](PCGEdge* edge)
				{
					if (!edge || edgeSet.find(edge) != edgeSet.end())
					{
						return;
					}

					edgeList.push_back(edge);

					edgeSet.emplace(edge);
				};

			auto RegisterNode = [&nodeList, &nodeToIndexMap, &RegisterEdge](PCGNode* currentNode)
				{
					if (!currentNode || nodeToIndexMap.find(currentNode) != nodeToIndexMap.end())
					{
						return;
					}

					uint32 nodeIndex = (uint32)nodeList.size();
					nodeList.push_back(currentNode);
					nodeToIndexMap[currentNode] = nodeIndex;

					for (const UniquePCGPinPtr& Pin : currentNode->GetInputPins())
					{
						for (const PCGEdgePtr& InboundEdge : Pin->m_Edges)
						{
							if (InboundEdge->IsValid())
							{
								RegisterEdge(InboundEdge.get());
							}
						}
					}
				};

			// Register all nodes and edges
			RegisterNode(graph.GetInputNode());
			RegisterNode(graph.GetOutputNode());
			for (const UniquePCGNodePtr& Node : graph.GetNodes())
			{
				RegisterNode(Node.get());
			}

			// PCG graph info
			WriteJSNode(jGraph, "Type", "PCG_GRAPH");
			WriteJSNode(jGraph, "Version", "1.0.0");

			// Nodes
			cJSON* jNodes = cJSON_CreateArray();
			for (const PCGNode* pcgNode : nodeList)
			{
				cJSON* jPcgNode = SerializeGraphNode(*pcgNode);
				cJSON_AddItemToArray(jNodes, jPcgNode);
			}

			cJSON_AddItemToObject(jGraph, "Nodes", jNodes);

			// Edges
			cJSON* jEdges = cJSON_CreateArray();
			for (const PCGEdge* edge : edgeList)
			{
				cJSON* jEdge = cJSON_CreateObject();
				if (edge->m_InputPin && edge->m_OutputPin)
				{
					auto itFromNode = nodeToIndexMap.find(edge->m_InputPin->m_Node);
					auto itToNode = nodeToIndexMap.find(edge->m_OutputPin->m_Node);
					if (itFromNode != nodeToIndexMap.end() && itToNode != nodeToIndexMap.end())
					{
						WriteJSNode(jEdge, "FromNode", itFromNode->second);
						WriteJSNode(jEdge, "FromPin", edge->m_InputPin->m_Properties.label);
						WriteJSNode(jEdge, "ToNode", itToNode->second);
						WriteJSNode(jEdge, "ToPin", edge->m_OutputPin->m_Properties.label);
					}
				}
				cJSON_AddItemToArray(jEdges, jEdge);
			}
			cJSON_AddItemToObject(jGraph, "Edges", jEdges);

			return jGraph;
		}

		bool PCGGraphJsonSerializer::DeserializeGraph(cJSON* jGraph, PCGGraph& outGraph)
		{
			if (!jGraph)
			{
				return false;
			}

			// Type check
			std::string type;
			ReadJSNode(jGraph, "Type", type);
			if (type != "PCG_GRAPH")
			{
				return false;
			}

			std::string version;
			ReadJSNode(jGraph, "Version", version);

			bool bSuccess = false;

			if (version == "1.0.0")
			{
				// Build nodes
				cJSON* jNodes = cJSON_GetObjectItem(jGraph, "Nodes");
				std::vector<PCGNode*> nodes = BuildNodesFromJson(outGraph, jNodes);

				// Add edges
				cJSON* jEdges = cJSON_GetObjectItem(jGraph, "Edges");
				bSuccess = BuildEdgesFromJson(outGraph, jEdges, nodes);
			}

			return bSuccess;
		}

		cJSON* PCGGraphJsonSerializer::SerializeGraphNode(const PCGNode& pcgNode)
		{
			cJSON* jPcgNode = cJSON_CreateObject();

			if (!pcgNode.m_NodeTitle.empty())
			{
				WriteJSNode(jPcgNode, "NodeTitle", pcgNode.m_NodeTitle);
			}
			WriteJSNode(jPcgNode, "PositionX", pcgNode.m_PositionX);
			WriteJSNode(jPcgNode, "PositionY", pcgNode.m_PositionY);

			// Settings
			const PCGSettings* settings = pcgNode.GetSettings();
			if (settings)
			{
				cJSON* jSettings = PCGJsonSerializer::SerializePCGObject(settings);
				cJSON_AddItemToObject(jPcgNode, "Settings", jSettings);
			}

			return jPcgNode;
		}

		std::vector<PCGNode*> PCGGraphJsonSerializer::BuildNodesFromJson(PCGGraph& graph, cJSON* jNodes)
		{
			std::vector<PCGNode*> nodeList;

			if (!jNodes || jNodes->type != cJSON_Array)
			{
				return nodeList;  // Empty
			}

			int nodeCount = cJSON_GetArraySize(jNodes);
			if (nodeCount < 2)
			{
				return nodeList;  // Invalid
			}

			for (int i = 0; i < nodeCount; ++i)
			{
				cJSON* jNode = cJSON_GetArrayItem(jNodes, i);
				if (!jNode || jNode->type != cJSON_Object)
				{
					continue;
				}

				cJSON* jSettings = cJSON_GetObjectItem(jNode, "Settings");

				PCGSettings* settings = dynamic_cast<PCGSettings*>(PCGJsonSerializer::NewPCGObject(jSettings));

				if (settings)
				{
					// Build node
					std::unique_ptr<PCGNode> newNode = settings->CreateNode();
					newNode->SetGraph(&graph);
					newNode->SetSettings(std::unique_ptr<PCGSettings>(settings), true);
					nodeList.push_back(newNode.get());

					// Set node properties
					Name nodeTitle;
					if (ReadJSNode(jNode, "NodeTitle", nodeTitle))
					{
						newNode->SetNodeTitle(nodeTitle);
					}
					ReadJSNode(jNode, "PositionX", newNode->m_PositionX);
					ReadJSNode(jNode, "PositionY", newNode->m_PositionY);

					if (i == 0)
					{
						graph.m_InputNode = std::move(newNode);
					}
					else if (i == 1)
					{
						graph.m_OutputNode = std::move(newNode);
					}
					else
					{
						graph.AddNode(std::move(newNode));
					}
				}
				else
				{
					LogManager::instance()->logMessage("PCGGraphJsonSerializer::BuildNodesFromJson: failed to create settings for node " + std::to_string(i));
				}
			}

			return nodeList;
		}

		bool PCGGraphJsonSerializer::BuildEdgesFromJson(PCGGraph& graph, cJSON* jEdges, const std::vector<PCGNode*>& nodeList)
		{
			if (!jEdges || jEdges->type != cJSON_Array)
			{
				return false;
			}

			int edgeCount = cJSON_GetArraySize(jEdges);
			for (int i = 0; i < edgeCount; ++i)
			{
				cJSON* jEdge = cJSON_GetArrayItem(jEdges, i);

				if (!jEdge || jEdge->type != cJSON_Object)
				{
					continue;
				}

				int fromNodeIndex = -1;
				int toNodeIndex = -1;
				Name fromPinLabel;
				Name toPinLabel;

				bool bValid = true;
				bValid &= ReadJSNode(jEdge, "FromNode", fromNodeIndex);
				bValid &= ReadJSNode(jEdge, "FromPin", fromPinLabel);
				bValid &= ReadJSNode(jEdge, "ToNode", toNodeIndex);
				bValid &= ReadJSNode(jEdge, "ToPin", toPinLabel);
				if (!bValid || fromNodeIndex < 0 || fromNodeIndex >= (int)nodeList.size() || toNodeIndex < 0 || toNodeIndex >= (int)nodeList.size())
				{
					continue;
				}

				graph.AddEdge(nodeList[fromNodeIndex], fromPinLabel, nodeList[toNodeIndex], toPinLabel);
			}

			return true;
		}

		PCGGraphManager::PCGGraphManager()
		{
			mLoadOrder = 100.0f;
			mResourceType = "PCGGraphV2";
			ResourceManagerFactory::instance()->registerResourceManager(mResourceType, this);
		}

		PCGGraphManager::~PCGGraphManager()
		{
			ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);
		}

		Echo::ResourceBase* PCGGraphManager::createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParames)
		{
			return new PCGGraph(this, name, isManual, loader);
		}

		REGISTER_PCG_EXTRA_EDITOR_NODE(PCGCommentNode);

		std::shared_ptr<PCGExtraEditorNode> PCGExtraEditorNodeFactory::CreateFromName(const String& nodeType)
		{
			return CreateFromName(Name(nodeType));
		}

		std::shared_ptr<PCGExtraEditorNode> PCGExtraEditorNodeFactory::CreateFromName(const Name& nodeType)
		{
			auto& creators = GetCreators();
			auto it = creators.find(nodeType);
			if (it != creators.end())
			{
				auto& createFunc = it->second;
				return createFunc();
			}

			return nullptr;
		}

		void PCGExtraEditorNodeFactory::RegisterCreateFunc(const Name& settingsType, PCGExtraEditorNodeCreateFunc creator)
		{
			auto& createFuncMap = GetCreators();
			createFuncMap.insert(std::make_pair(settingsType, creator));
		}

		PCGExtraEditorNodeFactory::PCGExtraEditorNodeCreateFuncMap& PCGExtraEditorNodeFactory::GetCreators()
		{
			static PCGExtraEditorNodeCreateFuncMap createFuncMap;
			return createFuncMap;
		}

	}
}
