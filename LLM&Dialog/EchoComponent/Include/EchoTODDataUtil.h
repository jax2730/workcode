#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoTODVariableInfo.h"
#include "EchoName.h"
#include "EchoSceneManager.h"

namespace Echo
{
#ifdef ECHO_EDITOR
	struct TODFrameData
	{
		String					m_strCom;
		std::vector<double>		m_vTimes;
		std::vector<Vector4>	m_vDatas;
		std::vector<String>		m_strDatas;

		std::vector<Vector4>	m_vLeftDatas;
		std::vector<Vector4>	m_vRightDatas;
	};

	struct CalcTangentData
	{
		std::vector<double>		m_vTimes;
		std::vector<Vector4>	m_vDatas;
		std::vector<Vector4>	m_vLeftHand;
		std::vector<Vector4>	m_vRightHand;
	};

	using TodDataID = TODVariableInfo::ETimeOfDayParamID;

	class _EchoComponentExport TODDataUtil
	{
	public:
		static void InitTimeData(TODVariableInfo* pTodData);
		static void InsertTimeData(int id, const TODFrameData& pData);
		static void SetTodTemplate(int id, const String& pParamStr, int pParamSize);
		static void CreateTodActor(Actor* pRoot);
		static void CreateTodComActor(Actor* pRoot);
		static void UseTODData(Actor* pRoot);

		static void ModifyTimelineFileData(Actor* act, const String& path);

		static std::vector<String> m_vActNames;
		static std::vector<struct StringToNumber> m_vDataTemplates;

		static std::vector<TODFrameData> m_vTimeData;
		static std::map<Name, CalcTangentData> m_vTangentDatas;

	private:
		/*TODDataUtil() = delete;
		~TODDataUtil() = delete;
		TODDataUtil(const TODDataUtil&) = delete;
		TODDataUtil& operator=(const TODDataUtil&) = delete;
		TODDataUtil(TODDataUtil&&) = delete;
		TODDataUtil& operator=(TODDataUtil&&) = delete;*/

		static void CheckData(TODVariableInfo* pTodData);
		static void RetrieveData(const String& pComName, int pBegin, int pEnd, TODVariableInfo* pTodData);
		
		static void CreateTodComponent(Actor* pRoot);
		static void SetTodComponentData(Actor* pRoot);
		static void SetComponentTemplate(ActorComponent*& pCom);

		static std::vector<bool> m_Registry;
	};
#endif

	class _EchoComponentExport TODUtil : public Singleton<TODUtil>
	{
	public:
		TODUtil();
		virtual ~TODUtil();

		bool GetState() { return replaceTOD; }

		void SetReplaceTODState(bool value) { replaceTOD = value; }
		void ReplaceTOD(bool useDefault = false);
		//void RestoreOp();

#ifdef ECHO_EDITOR
		void InitSTFID();
		const std::map<String, std::map<String, String>>& getSTFMap() { return mSTFMap; }

		void SelectSTFID(const String& id, const String& name);
		bool GetTodStatusData(TODVariableInfo*& data);
		bool UseSTF(Actor* pRoot);
		void applyComData(Actor* pRoot);

		void SetComponentData(ActorComponent*& pCom);
	private:
		// 分组 - 标识 - stf文件路径
		std::map<String, std::map<String, String>> mSTFMap;

		// ces
		SceneManager* mSceneManager = nullptr;
		class SceneArtTemplateManager* mSATMgr = nullptr;
		class SceneArtTemplate* mSceneArtTemplate = nullptr;
#endif
	private:

		bool			replaceTOD = false;
		AreaManager*	area = nullptr;
		int				environmentLightState = 0;
	};
}
