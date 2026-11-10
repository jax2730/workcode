#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"

namespace Echo
{
	class PCGComponent;

	namespace PCGV2
	{
		class PCGGraphExecutor;
		class PCGGraphCompiler;

		class PCGSystem : public FrameListener
		{
		public:
			static PCGSystem* GetInstance();

			void Tick();

			PCGTaskId ScheduleComponent(PCGComponentPtr pcgComponent);

			PCGGraphCompiler& GetGraphCompiler();

			PCGBaseManager* GetPCGBaseManager() { return m_PCGBaseManager; }
		private:
			PCGSystem();

			~PCGSystem();

			virtual bool frameEnded(const FrameEvent& evt);

			PCGGraphExecutor* m_GraphExecutor = nullptr;
			PCGBaseManager* m_PCGBaseManager = nullptr;
		};
	}
}
