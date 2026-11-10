#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/Data/EchoPCGDataCommon.h"
#include "PCG/EchoPCGGraph.h"
#include "PCG/EchoPCGSettings.h"

namespace Echo
{
	class PCGComponent;

	namespace PCGV2
	{
		class PCGNode;

		struct _EchoComponentExport PCGContext
		{
		public:
			template<typename T, typename... Args>
			static T* NewObject(PCGContext* Context, Args&&... args) {
				static_assert(std::is_base_of_v<PCGBase, T>, "PCGContext::NewObject - T must be derived from PCGBase.");
				return new T(std::forward<Args>(args)...);
			}

		public:

			PCGDataCollection inputData;

			PCGDataCollection outputData;

			EchoWeakPtr<PCGComponent> sourceComponent;

			PCGTaskId taskId = c_InvalidTaskId;

			PCGTaskId compiledTaskId = c_InvalidTaskId;

			const PCGNode* node = nullptr;

			template<typename SettingsType>
			const SettingsType* GetInputSettings() const
			{
				return static_cast<const SettingsType*>(m_SettingsWithOverride.get());
			}

			int GetSeed() const;

			void InitializeSettings();

			PCGContext() = default;

			String GetTaskName() const;
			String GetComponentName() const;
		private:
			std::unique_ptr<PCGSettings> m_SettingsWithOverride = nullptr;
		};
	}
}
