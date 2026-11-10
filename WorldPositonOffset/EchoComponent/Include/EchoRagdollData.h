#pragma once 

#include "EchoComponentPrerequisites.h"
#include "EchoResourceBase.h"
#include "EchoResourceManagerBase.h"
#include "EchoSingleton.h"
namespace Echo
{
	struct cJSON;

	class _EchoComponentExport RagdollData : public ResourceBase
	{
		friend class RagdollDataManager;
		friend class RagdollComponentInitData;
	private:
		RagdollData(ResourceManagerBase* creator, const Name& fileName, bool isManual, ManualResourceLoader* loader);
		~RagdollData();

		virtual void prepareImpl(void) override;
		virtual void unprepareImpl(void) override {};
		virtual void loadImpl(void) override {};
		virtual void unloadImpl(void)override {};
		size_t calcMemUsage(void) const override;
	private:
		cJSON* mpData = nullptr;
	};
	typedef SmartPtr<RagdollData> RagdollDataPtr;
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class _EchoComponentExport RagdollDataManager : public ResourceManagerBase ,  public Singleton<RagdollDataManager>
	{
	public:
		RagdollDataManager();
		~RagdollDataManager();

		RagdollDataPtr createManual(const Name& name, ManualResourceLoader* loader = 0);
	protected:
		virtual ResourceBase* createImpl(const Name& name, bool isManual, ManualResourceLoader* loader,const NameValuePairList* createParams) override;

	};

}; // namespace Echo
