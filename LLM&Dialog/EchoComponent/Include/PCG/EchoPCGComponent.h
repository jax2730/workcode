#pragma once

#include "EchoComponentPrerequisites.h"

#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"

#include "PCG/EchoPCGCommon.h"
#include "PCG/EchoPCGGraph.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGManagedResource;
	}
	using PCGV2::PCGManagedResource;
	using PCGV2::PCGTaskId;

	class PCGComponentInfo;

	// Component for PCGV2
	class _EchoComponentExport PCGComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(PCGComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		PCGV2::PCGGraphPtr GetGraph() const { return m_Graph; }

		PCGV2::PCGGraph* GetRawGraph() const { return m_Graph.get(); }

		void SetGraphPath(const std::string& path);

		const std::string& GetGraphPath() { return m_GraphPath; }

		void SetGraph(PCGV2::PCGGraphPtr graph);

		const Vector3& GetBounds() const { return m_Bounds; }

		void SetBounds(const Vector3& bounds) { m_Bounds = bounds; }

		void AddToManagedResources(PCGManagedResource* resource);

		void ForEachManagedResource(std::function<void(PCGManagedResource*)> func);

		void PostProcessGraph();

		void CleanupUnusedManagedResources();

		void CleanupAllManagedResources();

		void Generate();

		bool IsGraphPrepared();

		// override functions
		virtual void init(ActorComponentInfo* info) override;

		virtual void unInit() override;

	private:

		PCGComponentInfo* GetComponentInfo() const;

		bool IsGenerating() const {return m_CurrentGenerationTask == PCGV2::c_InvalidTaskId; }

		PCGV2::PCGSystem* GetPCGSystem() const;

		PCGTaskId GenerateInternal();

		std::string m_GraphPath;

		PCGV2::PCGGraphPtr m_Graph;

		Vector3 m_Bounds;

		std::vector<PCGManagedResource*> m_GeneratedResources;

		bool m_bGenerationInProgress = false;

		PCGTaskId m_CurrentGenerationTask = PCGV2::c_InvalidTaskId;
	};

	class _EchoComponentExport PCGComponentInfo : public ActorComponentInfo
	{
	public:
		PCGComponentInfo() = default;
		PCGComponentInfo(const Name& type, ActorCOMMoveableType movable);
		virtual ~PCGComponentInfo() {};
		_DECLCOMINFO_VIRFUN;

		_DECLCOMINFO_PARAM_BEGIN(PCGComponent)
		_DECLCOMINFO_PARAM(std::string, GraphPath, std::string(""), SetGraphPath)
		_DECLCOMINFO_PARAM(Vector3, Bounds, Vector3(128.0f, 128.0f, 128.0f), SetBounds)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(PCGComponent, ALL_MOVEABLE)
}

