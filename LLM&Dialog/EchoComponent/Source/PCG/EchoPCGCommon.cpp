#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGCommon.h"
#include "PCG/EchoPCGSystem.h"

namespace Echo
{
	namespace PCGV2
	{
		DEFINE_PCG_ENUM(PCGDataType,
			{ "None", PCGDataType::None},
			{ "Point", PCGDataType::Point },
			{ "Landscape", PCGDataType::Landscape },
			{ "Spatial", PCGDataType::Spatial },
			{ "Any", PCGDataType::Any },
		)

		void PCGBaseManager::DestroyImpl(const PCGBase* ptr) {
			ECHO_LOCK_AUTO_MUTEX
				gWaitDestroys.push_back(ptr);
		}
		void PCGBaseManager::DestoryUnreferenced() {
			if (gWaitDestroys.empty()) return;
			std::list<const PCGBase*> _WaitDestroys;
			{
				ECHO_LOCK_AUTO_MUTEX
					_WaitDestroys.swap(gWaitDestroys);
			}
			for (const PCGBase* ptr : _WaitDestroys) {
				SAFE_DELETE(ptr)
			}
		}

		void PCGBase::DestroyImpl()
		{
			PCGSystem::GetInstance()->GetPCGBaseManager()->DestroyImpl(this);
		}
	}
}
