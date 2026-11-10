#pragma once

#include "EchoComponentPrerequisites.h"

#include "EchoResourceBase.h"
#include "EchoResourceManagerBase.h"
#include "PCG/EchoPCGCommon.h"
#include "PCG/EchoPCGSettings.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGObject;
		class PCGProperty;

		class _EchoComponentExport PCGEdge
		{
		public:
			PCGEdge();

			~PCGEdge();

			PCGPin* GetOtherPin(PCGPin* pin) const;

			bool IsValid() const;

			PCGPin* m_InputPin = nullptr;

			PCGPin* m_OutputPin = nullptr;
		};

		class _EchoComponentExport PCGNode
		{
		public:
			PCGNode() = default;

			PCGNode(PCGGraph* graph);

			~PCGNode();

			PCGNode(const PCGNode&) = delete;

			PCGNode& operator=(const PCGNode&) = delete;

			PCGGraph* GetGraph() const;

			void SetGraph(PCGGraph* graph) { m_Graph = graph; }

			PCGPin* GetOutputPin(const Name& label);

			PCGPin* GetInputPin(const Name& label);

			const PCGPin* GetOutputPin(const Name& label) const;

			const PCGPin* GetInputPin(const Name& label) const;

			Name GetNodeTitle() const;

			void SetNodeTitle(const Name& title);

			Name GetDefaultNodeTitle() const;

			const std::vector<UniquePCGPinPtr>& GetInputPins() const;

			const std::vector<UniquePCGPinPtr>& GetOutputPins() const;

			bool IsInputPinConnected(const Name& Label) const;

			bool IsOutputPinConnected(const Name& Label) const;

			bool HasInboundEdges();

			void SetPosition(int positionX, int positionY);

			void SetSettings(std::unique_ptr<PCGSettings> settings, bool bUpdatePins = true);

			PCGSettings* GetSettings() const;

			void MarkModified();

			template<typename T>
			T* GetSettings() const
			{
				return dynamic_cast<T*>(m_Settings.get());
			}

			// Editor data
			Name m_NodeTitle;

			int32 m_PositionX = 0;

			int32 m_PositionY = 0;

		protected:
			void UpdatePins();

			PCGGraph* m_Graph = nullptr;

			std::vector<UniquePCGPinPtr> m_InputPins;

			std::vector<UniquePCGPinPtr> m_OutputPins;

			std::unique_ptr<PCGSettings> m_Settings;

			friend class PCGGraphSerializer;
		};

		class _EchoComponentExport PCGGraph : public ResourceBase
		{
		public:
			PCGGraph(ResourceManagerBase* creator, const Name& name, bool isManual, ManualResourceLoader* loader);

			~PCGGraph();

			PCGGraph(const PCGGraph&) = delete;

			PCGGraph& operator=(const PCGGraph&) = delete;

			PCGNode* AddNode(std::unique_ptr<PCGNode> node);

			PCGNode* AddNode(std::unique_ptr<PCGSettings> settings);

			PCGNode* CreateAndAddNode(const Name& settingsClassName);

			void RemoveNode(PCGNode* inNode);

			void RemoveNodes(const std::vector<PCGNode*>& inNodes);

			bool AddEdge(PCGNode* from, const Name& fromPinLabel, PCGNode* to, const Name& toPinLabel);

			bool RemoveEdge(PCGNode* from, const Name& fromPinLabel, PCGNode* to, const Name& toPinLabel);

			const std::vector<UniquePCGNodePtr>& GetNodes() const { return m_Nodes; }

			PCGNode* GetInputNode() const;

			PCGNode* GetOutputNode() const;

			void MarkModified(bool bNeedRecompile = false);

			bool IsModified();

			bool ExportToFile(const String& filepath);

			bool ExportToFile();

			void Clear();

		public:
			virtual void prepareImpl(void) override;

			virtual void unprepareImpl(void) override;

			virtual void loadImpl(void) override;

			virtual void unloadImpl(void) override;

			virtual size_t calcMemUsage(void) const override;

		private:
			// Clear cached tasks in the compiler
			void ClearCompiledTasks();

			void InitializeDefaultInputOutput();

			std::vector<UniquePCGNodePtr> m_Nodes;

			UniquePCGNodePtr m_InputNode;

			UniquePCGNodePtr m_OutputNode;

			bool m_bModified = true;

			friend class PCGGraphJsonSerializer;
		};

		class PCGGraphJsonSerializer
		{
		public:
			static bool Export(const PCGGraph& graph, const std::string& filepath);

			static bool Import(PCGGraph& outGraph, const std::string& filepath);

			static cJSON* SerializeGraph(const PCGGraph& graph);

			static bool DeserializeGraph(cJSON* jGraph, PCGGraph& outGraph);

			static cJSON* SerializeGraphNode(const PCGNode& node);

			static std::vector<PCGNode*> BuildNodesFromJson(PCGGraph& graph, cJSON* jNodes);

			static bool BuildEdgesFromJson(PCGGraph& graph, cJSON* jEdges, const std::vector<PCGNode*>& nodeList);
		};

		using PCGGraphPtr = SmartPtr<PCGGraph>;

		class _EchoComponentExport PCGGraphManager : public ResourceManagerBase, public Singleton<PCGGraphManager>
		{
		public:
			PCGGraphManager();
			virtual ~PCGGraphManager();
		protected:
			ResourceBase* createImpl(const Name& name,
				bool isManual, ManualResourceLoader* loader,
				const NameValuePairList* createParames);
		};

		class _EchoComponentExport PCGExtraEditorNode
		{
		public:
			PCGExtraEditorNode() = default;
			virtual ~PCGExtraEditorNode() = default;

			virtual void ToJson(cJSON* jExtraEditorNode) const {}
			virtual bool FromJson(cJSON* jExtraEditorNode) { return true; }
		};

		class _EchoComponentExport PCGCommentNode : public PCGExtraEditorNode
		{

		};

		class _EchoComponentExport PCGExtraEditorNodeFactory
		{
			using PCGExtraEditorNodeCreateFunc = std::function<std::shared_ptr<PCGExtraEditorNode>()>;
			using PCGExtraEditorNodeCreateFuncMap = std::unordered_map<Name, PCGExtraEditorNodeCreateFunc, NameIDHasher>;

		public:
			static std::shared_ptr<PCGExtraEditorNode> CreateFromName(const String& nodeType);

			static std::shared_ptr<PCGExtraEditorNode> CreateFromName(const Name& nodeType);

			static void RegisterCreateFunc(const Name& settingsType, PCGExtraEditorNodeCreateFunc creator);

		private:
			static PCGExtraEditorNodeCreateFuncMap& GetCreators();
		};
	}
}

#define REGISTER_PCG_EXTRA_EDITOR_NODE(ClassName) \
namespace \
{ \
	struct ClassName##AutoRegister \
	{ \
		ClassName##AutoRegister() \
		{ \
			PCGV2::PCGExtraEditorNodeFactory::RegisterCreateFunc( \
				Name(#ClassName), \
				[]() -> std::shared_ptr<PCGV2::PCGExtraEditorNode> { return std::make_shared<ClassName>(); } \
			); \
		} \
	}; \
	static ClassName##AutoRegister ClassName##AutoRegistrar; \
}
