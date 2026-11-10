#include "EchoComponentStableHeaders.h"

#include "PCG/EchoPCGSystem.h"
#include "PCG/EchoPCGComponent.h"
#include "PCG/Graph/EchoPCGGraphExecutor.h"
namespace Echo
{
	namespace PCGV2
	{
		PCGSystem* PCGSystem::GetInstance()
		{
			static PCGSystem* instance = new PCGSystem();
			return instance;
		}

		void PCGSystem::Tick()
		{
			m_GraphExecutor->Execute();
			if (m_PCGBaseManager) m_PCGBaseManager->DestoryUnreferenced();
		}

		PCGTaskId PCGSystem::ScheduleComponent(PCGComponentPtr pcgComponent)
		{
			//TODO Use executer->ScheduleGeneric
			m_GraphExecutor->Schedule(pcgComponent);
			return c_InvalidTaskId;
		}

		PCGGraphCompiler& PCGSystem::GetGraphCompiler()
		{
			return m_GraphExecutor->GetGraphCompiler();
		}

		bool PCGSystem::frameEnded(const FrameEvent& evt)
		{
			Tick();
			return true;
		}

		PCGSystem::PCGSystem()
		{
			m_GraphExecutor = new PCGGraphExecutor();
			m_PCGBaseManager = new PCGBaseManager();
			Echo::Root::instance()->addFrameListener(this);
		}

		PCGSystem::~PCGSystem()
		{
			Echo::Root::instance()->removeFrameListener(this);
			SAFE_DELETE(m_PCGBaseManager);
			delete m_GraphExecutor;
		}

	}
}

