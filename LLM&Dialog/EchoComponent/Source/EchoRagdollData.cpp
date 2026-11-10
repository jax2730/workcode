#include "EchoComponentStableHeaders.h"
#include "EchoRagdollData.h"
#include "EchoResourceManagerFactory.h"

namespace Echo
{
	///// class RagdollData /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	RagdollData::RagdollData(ResourceManagerBase* creator, const Name& fileName, bool isManual, ManualResourceLoader* loader)
		:ResourceBase(creator,fileName,isManual,loader)
	{
	}
	Echo::RagdollData::~RagdollData()
	{
		unload();

		if (mpData != nullptr)
			cJSON_Delete(mpData);
	}
	void RagdollData::prepareImpl(void)
	{
		DataStreamPtr stream = DataStreamPtr(Root::instance()->GetFileDataStream(mName.c_str()));
		if (stream.isNull() || stream->size() == 0)
		{
			ECHO_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
				"Cannot locate resource " + mName.toString(),
				" RagdollData::prepareImpl");
			return;
		}

		size_t	dataSize = stream->size();
		char* dataBuffer = new char[dataSize];
		memset(dataBuffer, 0, dataSize);
		stream->read(dataBuffer, dataSize);

		mpData = cJSON_Parse(dataBuffer);

		delete[] dataBuffer;
		stream->close();
	}

	size_t RagdollData::calcMemUsage(void) const
	{
		return 0u;
	}

	///// class RagdollDataManager ////////////////////////////////////////////////////////////////////////////////////////////
	RagdollDataManager::RagdollDataManager()
	{
		mResourceType = "RagdollData";
		ResourceManagerFactory::instance()->registerResourceManager(mResourceType, this);
	}
	RagdollDataManager::~RagdollDataManager()
	{
		ResourceManagerFactory::instance()->unregisterResourceManager(mResourceType);
	}
	RagdollDataPtr RagdollDataManager::createManual(const Name& name, ManualResourceLoader* loader)
	{
		return createResource(name, true, loader);
	}
	ResourceBase* RagdollDataManager::createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams)
	{
		return new RagdollData(this,name,isManual,loader);
	}
}; // namespace Echo
