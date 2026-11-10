#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		void PCG_LOG_BASE(const std::vector<const char*>& messages)
		{
			std::string logStr;
			for (const char* message : messages) {
				if (message != nullptr) {
					logStr += "[";
					logStr += message;
					logStr += "]";
				}
			}
			if (!logStr.empty()) {
				LogManager::instance()->logMessage(logStr.c_str());
			}
		}
		void PCG_LOG_BASE(const char* verbosity, PCGContext* context, const std::vector<const char*>& messages) {
			std::string logStr = verbosity;
			logStr += "\t:[";
			logStr += context->GetTaskName();
			logStr += "]";
			for (const char* message : messages) {
				if (message != nullptr) {
					logStr += "[";
					logStr += message;
					logStr += "]";
				}
			}
			if (!logStr.empty()) {
				LogManager::instance()->logMessage(logStr.c_str());
			}
		}

		PCGContext* PCGElement::Initialize(const PCGDataCollection& inputData, const PCGNode* node)
		{
			PCGContext* context = CreateContext();
			context->inputData = inputData;
			context->node = node;
			return context;
		}

		bool PCGElement::Execute(PCGContext* context)
		{
			// TODO: Add other prerequisites
			bool bExecutionPrerequisitesFulfilled = PrepareDataInternal(context);

			if (bExecutionPrerequisitesFulfilled)
			{
				return ExecuteInternal(context);
			}
			else
			{
				return false;
			}
		}

		PCGContext* PCGElement::CreateContext()
		{
			return new PCGContext();
		}

		bool PCGElement::PrepareDataInternal(PCGContext* inContext)
		{
			return true;
		}

		PCGElement* PCGEmptyElement::GetInstance()
		{
			static PCGEmptyElement* instance = new PCGEmptyElement();
			return instance;
		}

		bool PCGEmptyElement::ExecuteInternal(PCGContext* inContext)
		{
			return true;
		}
	}
}
