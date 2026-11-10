#pragma once
#include "Actor/EchoActorComponent.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class SheetComponentInfo;
	class SqliteObject;
	struct _Select_Result;

	class _EchoComponentExport SheetComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SheetComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							copyTo(const ActorComponentPtr& newComp) override;
		virtual void							replay(ActorComponentInfo* info) override;

		//加载数据库读取配置文件
		void									loadRuleFile(const String& path);
		//生成数据库读取配置文件
		void									genRuleFile();

		//从文件加载数据库的方式改变数据库
		void									changeDataBase(const Name& fileName,bool synchronous = false);
		// 设置数据库对象来改变读取的数据库
		void									setDataBase(SqliteObject* obj);
		//通过表名来设置已经加载到内存的数据库
		void									setDataBaseByTableName(const Name& rhs);
		//设置当前读取的数据库表名
		void									setDBTable(const Name& name);

		/** 读取表的某一行某一列
			@param
			[IN] colName: 要读取的列名
			[IN] rowID: 要读取的行id
			[OUT] result: 读取出的值
		*/
		void									readDB(const Name& colName, int rowID, std::string& result);
		void									readDB(const Name& colName, int rowID, int& result);
		void									readDB(const Name& colName, int rowID, float& result);


		/** 读取表的某一列
			@param
			[IN] ColName: 要读取的列名
			[OUT] result: 读取出的值
		*/
		void									readDBByCol(const Name& colName, std::vector<String>& result);
		void									readDBByCol(const Name& colName, std::vector<float>& result);
		void									readDBByCol(const Name& colName, std::vector<int>& result);

		//判断列是否等于某个特定值的条件查询
		void									selectDataByCondition(const Name& table_name, const Name& column_name, const Name& condition, std::vector<_Select_Result>& result);

		//测试函数
		//void									testDBFunction();
		std::vector<Name>&						getDBTableList() { return mTableList; }
		void									updateDBTableList();
		/** 读取表的某一行
			@param
			[IN] RowID: 要读取的行id
		*/
		//std::vector<char*>						readDBByRow(int RowID);
		
	public:

		SheetComponentInfo*						mInfo = nullptr;
	private:
		SqliteObject*							mSqliteObject = nullptr;
		std::vector<Name>						mTableList;
		Name									mDBTableName;
	};



	class _EchoComponentExport SheetComponentInfo :public ActorComponentInfo
	{
	public:
		SheetComponentInfo(){}
		SheetComponentInfo(const Name& comType, ActorCOMMoveableType moveType);
		virtual ~SheetComponentInfo() override;

		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void									Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;

		//void											AnalyzeRuleFile();

	public:
		//String									mPath;		//数据库读取配置文件路径
		//std::vector<Name>						mAffectComs;	//修改字段影响到的组件集合
		Echo::Name									mDBPath;	//数据库路径
		Echo::Name									mDBTableName;	//数据库表名
		EDITOR_REFLECT
	private:
		//Name									mComponentType;
		//String								mDataBaseName;
		//int									mRowID = 0;		//行ID
		//std::vector<String>					mColIDs;	//列名
		//std::unordered_map<String, String>	mPropertyData;	//属性值
		//std::vector<ActorComponentPtr>		mAffectComs;	//修改字段影响到的组件集合
	};






	DEFINECOMFACTORY(SheetComponent, UNMOVEABLE)


	class EchoSheetComponentScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoSheetComponentScript() = default;
		EchoSheetComponentScript(const EchoActorScript& pActor, const std::string& name) :
		EchoActorComponentScript(pActor, name)
		{
		}

	public:
		static EchoSheetComponentScript			Script_GetSheetComponent(const EchoActorScript& pActor, const std::string& name);

		//从文件加载数据库的方式改变数据库
		void									changeDataBase(const Name& fileName, bool synchronous = false);
		// 设置数据库对象来改变读取的数据库
		void									setDataBase(SqliteObject* obj);
		//通过表名来设置已经加载到内存的数据库
		void									setDataBaseByTableName(const Name& rhs);
		//设置当前读取的数据库表名
		void									setDBTable(const Name& name);

		/** 读取表的某一行某一列
			@param
			[IN] colName: 要读取的列名
			[IN] rowID: 要读取的行id
			[OUT] result: 读取出的值
		*/
		std::string								readDB_string(const std::string& colName, int rowID);
		int										readDB_int(const std::string& colName, int rowID);
		float									readDB_float(const std::string& colName, int rowID);


		/** 读取表的某一列
			@param
			[IN] ColName: 要读取的列名
			[OUT] result: 读取出的值
		*/
		//std::vector<String>					readDBByCol_string(const String& colName);
		//std::vector<float>					readDBByCol_float(const String& colName);
		//std::vector<int>						readDBByCol_int(const String& colName);
		//测试函数
		//void									testDBFunction();
	};
}
