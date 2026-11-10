#include "EchoComponentStableHeaders.h"
#include "EchoSheetComponent.h"
#include "EchoActorEntityComponent.h"
#include "EchoSheetManager.h"
#include "EchoSqliteObject.h"
#include "EchoResourceFolderManager.h"
#include "EchoSqliteMgr.h"


namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(SheetComponent)

	EDITOR_REFLECT_BEGIN(SheetComponentInfo)
	//EDITOR_REFLECT_MEMBER_UPDATE(mPath, "DBRuleFilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mDBPath, "DBFilePath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mDBTableName, "DBTableName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	//EDITOR_REFLECT_MEMBER_UPDATE(mAffectComs, "AffectComponents", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	SheetComponent::SheetComponent(ActorPtr pActor, ActorCOMMoveableType type) : ActorComponent(pActor, type)
	{
		//mSqliteObject = new SqliteObject(Name(fileName));
		//updateDBTableList();
	}

	SheetComponent::~SheetComponent()
	{
		SAFE_DELETE(mSqliteObject);
	}

	void SheetComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;

		mInfo = static_cast<SheetComponentInfo*>(info);
		mDBTableName = mInfo->mDBTableName;
		//mInfo->AnalyzeRuleFile();
		changeDataBase(mInfo->mDBPath);
	}

	void SheetComponent::copyTo(const ActorComponentPtr& newComp)
	{
	}

	void SheetComponent::replay(ActorComponentInfo* info)
	{
		mInfo = static_cast<SheetComponentInfo*>(info);
		//mInfo->AnalyzeRuleFile();
	}


	void SheetComponent::loadRuleFile(const String& path)
	{
		//mInfo->AnalyzeRuleFile();
	}

	void SheetComponent::genRuleFile()
	{
		//if (mInfo->mPath.empty())
			//return;

	}

	void SheetComponent::changeDataBase(const Name& fileName, bool synchronous)
	{
		if (fileName.empty() || fileName.size() == 0)
			return;

		mInfo->mDBPath = fileName;
		Name name = Name(Root::instance()->getRootResourcePath() + fileName.toString());
		if (mSqliteObject == nullptr)
		{
			mSqliteObject = new SqliteObject(name);
		}

		mSqliteObject->changeDataBase(name, synchronous);
			
	}

	void SheetComponent::setDataBase(SqliteObject* obj)
	{
		if (obj == nullptr)
			return;

		mSqliteObject = obj;
		if(mInfo)
			mInfo->mDBPath = mSqliteObject->getDBFile();
		//mInfo->mDBTableName = "";
	}

	void SheetComponent::setDataBaseByTableName(const Name& rhs)
	{
		auto obj = SqliteMgr::instance()->getSqliteByTableName(rhs);
		if (!obj)
			return;

		mSqliteObject = obj;
		if(mInfo)
			mInfo->mDBPath = mSqliteObject->getDBFile();
		//mInfo->mDBTableName = "";
	}

	void SheetComponent::updateDBTableList()
	{
		SqliteMgr::instance()->reInit("config/SqliteConfig.cfg");
		SqliteMgr::instance()->getDBTableList(mTableList);
	}

	void SheetComponent::readDB(const Name& colName, int rowID, std::string& result)
	{
		if (!mSqliteObject)
			return;
		
		mSqliteObject->readDB(mDBTableName, colName, rowID, result);
	}

	void SheetComponent::readDB(const Name& colName, int rowID, int& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->readDB(mDBTableName, colName, rowID, result);

	}

	void SheetComponent::readDB(const Name& colName, int rowID, float& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->readDB(mDBTableName, colName, rowID, result);

	}

	void SheetComponent::readDBByCol(const Name& colName, std::vector<String>& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->readDBByCol(mDBTableName,colName, result);
	}

	void SheetComponent::readDBByCol(const Name& colName, std::vector<float>& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->readDBByCol(mDBTableName, colName, result);
	}

	void SheetComponent::readDBByCol(const Name& colName, std::vector<int>& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->readDBByCol(mDBTableName, colName, result);
	}

	void SheetComponent::selectDataByCondition(const Name& table_name, const Name& column_name, const Name& condition, std::vector<_Select_Result>& result)
	{
		if (!mSqliteObject)
			return;

		mSqliteObject->selectDataByCondition(table_name, column_name, condition, result);
	}

	void SheetComponent::setDBTable(const Name& name)
	{
		mDBTableName = name;
		if(mInfo)
			mInfo->mDBTableName = name;
	}


	//void SheetComponent::testDBFunction()
	//{
	//	std::vector<String> res;
	//	std::vector<float>  pos_x, pos_y, pos_z;
	//	readDBByCol(Name("meshName"), res);
	//	readDBByCol(Name("Position_x"), pos_x);
	//	readDBByCol(Name("Position_y"), pos_y);
	//	readDBByCol(Name("Position_z"), pos_z);

	//	assert(res.size() == pos_x.size());

	//	for (int i = 0; i < res.size(); ++i)
	//	{
	//		auto pCom =  getActor()->addComponent(Name("EntityComponent"), Name("test_0"), getActor()->getRootNodeCom());
	//		EntityComponent* pEntity = static_cast<EntityComponent*>(pCom.get());
	//		pEntity->changeEntity(res[i]);
	//		pEntity->setPosition(Vector3(pos_x[i], pos_y[i], pos_z[i]));
	//		
	//	}
	//}

	SheetComponentInfo::SheetComponentInfo(const Name& comType, ActorCOMMoveableType moveType)
		:ActorComponentInfo(comType, moveType)
	{
	}

	SheetComponentInfo::~SheetComponentInfo()
	{
	}

	void SheetComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SheetComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void SheetComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SheetComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void SheetComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		SheetComponentInfo* pInfo = static_cast<SheetComponentInfo*>(comInfo);
		reflect::TypeResolver<SheetComponentInfo>::get()->copyTo(this, pInfo);
	}

	void SheetComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}


	void SheetComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<SheetComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}


	//void SheetComponentInfo::AnalyzeRuleFile()
	//{
	//	if (mPath.empty())
	//		return;

	//	DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(mPath.c_str()));
	//	if (pDataStream.isNull() || pDataStream->size() == 0)
	//	{
	//		ECHO_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
	//			"Cannot locate resource " + mPath,
	//			"ActorInfo::prepareImpl");
	//		return;
	//	}
	//	size_t nSize = pDataStream->size();
	//	char* pData = nullptr;

	//	pData = new char[nSize + 1];
	//	memset(pData, 0, nSize + 1);
	//	pDataStream->seek(0);
	//	pDataStream->read(pData, nSize);

	//	char* out = nullptr;
	//	out = pData;
	//	cJSON* node = cJSON_Parse(out);
	//	free(out);

	//	if (node == nullptr)
	//		return;

	//	ReadJSNode(node, "ComponentType", mComponentType);
	//	ReadJSNode(node, "DBName", mDataBaseName);
	//	ReadJSNode(node, "DBTableName", mDBTableName);
	//	ReadJSNode(node, "RowID", mRowID);
	//	
	//	cJSON* child = cJSON_GetObjectItem(node, "Property");

	//	for (auto it = child; it->next != nullptr; it = it->next)
	//	{
	//		mPropertyData[it->string] = it->valuestring;
	//	}

	//	cJSON_Delete(node);
	//}

	EchoSheetComponentScript EchoSheetComponentScript::Script_GetSheetComponent(const EchoActorScript& pActor, const std::string& name)
	{
		return EchoSheetComponentScript(pActor, name);
	}

	void EchoSheetComponentScript::changeDataBase(const Name& fileName, bool synchronous/* = false*/)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		if (sheetCom)
			sheetCom->changeDataBase(fileName, synchronous);

	}

	void EchoSheetComponentScript::setDataBase(SqliteObject* obj)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		if (sheetCom)
			sheetCom->setDataBase(obj);
	}

	void EchoSheetComponentScript::setDataBaseByTableName(const Name& rhs)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		if (sheetCom)
			sheetCom->setDataBaseByTableName(rhs);
	}

	void EchoSheetComponentScript::setDBTable(const Name& name)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		if (sheetCom)
			sheetCom->setDBTable(name);
	}

	std::string EchoSheetComponentScript::readDB_string(const std::string& colName, int rowID)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		String result;
		if (sheetCom)
		{
			sheetCom->readDB(Name(colName), rowID, result);
		}
		return result;
	}

	int EchoSheetComponentScript::readDB_int(const std::string& colName, int rowID)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		int result = 0;
		if (sheetCom)
		{
			sheetCom->readDB(Name(colName), rowID, result);
		}
		return result;
	}

	float EchoSheetComponentScript::readDB_float(const std::string& colName, int rowID)
	{
		SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
		float result = .0f;
		if (sheetCom)
			sheetCom->readDB(Name(colName), rowID, result);
		return result;
	}

	//std::vector<String> EchoSheetComponentScript::readDBByCol_string(const String& colName)
	//{
	//	SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
	//	std::vector<String> result;
	//	if (sheetCom)
	//		sheetCom->readDBByCol(Name(colName), result);
	//	return result;
	//}

	//std::vector<float> EchoSheetComponentScript::readDBByCol_float(const String& colName)
	//{
	//	SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
	//	std::vector<float> result;
	//	if (sheetCom)
	//		sheetCom->readDBByCol(Name(colName), result);
	//	return result;
	//}

	//std::vector<int> EchoSheetComponentScript::readDBByCol_int(const String& colName)
	//{
	//	SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
	//	std::vector<int> result;
	//	if (sheetCom)
	//		sheetCom->readDBByCol(Name(colName), result);
	//	return result;
	//}

	//void EchoSheetComponentScript::testDBFunction()
	//{
	//	SheetComponent* sheetCom = static_cast<SheetComponent*>(m_Ptr.get());
	//	if (sheetCom)
	//		sheetCom->testDBFunction();
	//}
}
