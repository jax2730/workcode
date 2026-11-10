#include "EchoComponentStableHeaders.h"
#include "SUToEchoCom/EchoExcelDataHelper.h"
#include "EchoSqliteMgr.h"
#include "EchoSqlite.h"
#include "EchoSqliteObject.h"
#include "Actor/EchoActorSystem.h"
#include "Actor/EchoActorManager.h"

namespace Echo
{

#define GET_INT_VECTOR(strstr)\
	std::vector<int> tmpVec;\
	{\
		if(strstr.empty()) return {};\
		size_t prepos = 0;\
		for (size_t i = 0; i < strstr.size(); ++i)\
		{\
			if (strstr[i] == '*')\
			{\
				tmpVec.push_back(std::stoi(strstr.substr(prepos, i - prepos)));\
				prepos = i + 1;\
			}\
		}\
		tmpVec.push_back(std::stoi(strstr.substr(prepos, strstr.size() - prepos)));\
	}\
	
#define GET_FLOAT_VECTOR(strstr)\
		std::vector<float> tmpVec;\
		{\
			if(strstr.empty()) return {};\
			size_t prepos = 0;\
			for (size_t i = 0; i < strstr.size(); ++i)\
			{\
				if (strstr[i] == '*')\
				{\
					tmpVec.push_back(std::stof(strstr.substr(prepos, i - prepos)));\
					prepos = i + 1;\
				}\
			}\
				tmpVec.push_back(std::stof(strstr.substr(prepos, strstr.size() - prepos)));\
		}\

#define GET_STRING_VECTOR(strstr)\
		std::vector<std::string> tmpVec;\
		{\
			if(strstr.empty()) return {};\
			size_t prepos = 0;\
			for (size_t i = 0; i < strstr.size(); ++i)\
			{\
				if (strstr[i] == '*')\
				{\
					tmpVec.push_back(strstr.substr(prepos, i - prepos));\
					prepos = i + 1;\
				}\
			}\
			tmpVec.push_back(strstr.substr(prepos, strstr.size() - prepos));\
		}\

#define GET_FLOAT_VEC_VEC(id)\
		std::vector<std::vector<float>>  result;\
	if (!m_curline->_data.empty())\
	{\
		std::string coll = m_curline->_data[id];\
		if (coll.empty()) return {};\
		size_t nextPos = 0;\
		for (size_t i = 0; i < coll.size(); ++i)\
		{\
			if (coll[i] == '|')\
			{\
				std::string tmpData = coll.substr(nextPos, i - nextPos);\
				nextPos = i + 1;\
				GET_FLOAT_VECTOR(tmpData);\
				result.push_back(tmpVec);\
			}\
		}\
		std::string tmpData = coll.substr(nextPos, coll.size() - nextPos);\
		GET_FLOAT_VECTOR(tmpData);\
		result.push_back(tmpVec);\
	}\
	return result;\

	//********************************asset_pic********************************************************

	lines_data::~lines_data()
	{
		
	}

	lines_data::lines_data(bool isInitAll/*= false*/) :m_bIsInitAllExcelData(isInitAll)
	{

	}

	void lines_data::setID(int id)
	{
		m_id = id; 
		getData();
	}

	ExcelDataManager::ExcelDataManager(bool initAll/* = false*/)
	{
		m_excels.emplace("asset_pic_block", new asset_pic_block(initAll));	//构造函数为true, 可在引擎初始化时全部读取数据, 默认设为false
		m_excels.emplace("wrap_entity", new wrap_entity(initAll));
		m_excels.emplace("actorblock_list", new actorblock_list(initAll));
		m_excels.emplace("newblock_list", new newblock_list(initAll));
		m_excels.emplace("particle_list", new particle_list(initAll));
		m_excels.emplace("building_list", new building_list(initAll));
		m_excels.emplace("asset_pic_building", new asset_pic_building(initAll));
		m_excels.emplace("mesh_group_state_list", new mesh_group_state_list(initAll));
		m_excels.emplace("building_info", new building_info(initAll));
		m_excels.emplace("physics_filter_groups", new physics_filter_groups(initAll));
	}

	ExcelDataManager::~ExcelDataManager()
	{
		for (auto&& iter : m_excels)
		{
			SAFE_DELETE(iter.second);
		}
		m_excels.clear();
	}

	lines_data* ExcelDataManager::getExcelByName(const String& name)
	{
		auto iter = m_excels.find(name);
		if (iter == m_excels.end())
			return nullptr;

		return iter->second;
	}

	void ExcelDataManager::getResSet(std::set<std::string>& ResSet)
	{
		//m_resSet.clear();
		ResSet.clear();
		for (auto&& m : m_excels)
		{
			if (m.first == "wrap_entity")
			{
				m.second->initAllData();
				auto ptr = static_cast<wrap_entity*>(m.second);
				for (auto&& line : ptr->getAllLine())
				{
					auto path = line.second->mesh_path();
					if(!ResSet.count(path))
						ResSet.emplace(path);
				}
			}
			else if (m.first == "asset_pic_building")
			{
				m.second->initAllData();
				auto ptr = static_cast<asset_pic_building*>(m.second);
				for (auto&& line : ptr->getAllLine())
				{
					auto path = line.second->actor_path();
					if(!ResSet.count(path))
						ResSet.emplace(path);
				}
			}
			else if (m.first == "building_list")
			{
				m.second->initAllData();
				auto ptr = static_cast<building_list*>(m.second);
				for (auto&& line : ptr->getAllLine())
				{
					auto path = line.second->hlod_path();
					if(!ResSet.count(path))
						ResSet.emplace(path);
				}
			}
		}
	}

	void ExcelDataManager::getResFromActor(const String& path, std::set<String>& resources)
	{
		mbCollectRes = true;
		auto actor = ActorSystem::instance()->getMainActorManager()->createActor(Name(path), "", true);
			resources.insert(m_resSet.begin(), m_resSet.end());
		ActorSystem::instance()->getMainActorManager()->deleteActorImpl(actor);
		mbCollectRes = false;
	}

	void ExcelDataManager::collectRes(const String& res)
	{
		if (!mbCollectRes)
			return;
		if(!m_resSet.count(res))
			m_resSet.insert(res);
	}


	one_line_data::one_line_data(_Select_Result* ptr) :m_curline(ptr) {}

	asset_pic_block::asset_pic_block(bool isInitAll/* = false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("asset_pic_block"));
		if(isInitAll)
			m_table->selectAllData(Name("asset_pic_block"), m_line_data);
	}

	asset_pic_line* asset_pic_block::getLine(int id)
	{
		auto iter = m_asset_pic_lines.find(id);
		if (iter != m_asset_pic_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new asset_pic_line(&m_line_data[id]);
		m_asset_pic_lines.emplace(id, pic);
		return pic;
	}

	void asset_pic_block::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("asset_pic_block"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	void asset_pic_block::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (!m_bIsInitAllExcelData && iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("asset_pic_block"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else if(iter != m_line_data.end())
		{
			m_curline = iter->second;
		}
		
	}

	asset_pic_line::asset_pic_line(_Select_Result* ptr) :one_line_data(ptr)
	{
	}

	int asset_pic_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string asset_pic_line::actor_path()
	{
		return m_curline->_data[1];
	}
	int asset_pic_line::tpp_wrap_role()
	{
		if (m_curline->_data[2].empty())
			return -1;
		return std::stoi(m_curline->_data[2]);
	}
	int asset_pic_line::fpp_wrap_role()
	{
		if (m_curline->_data[3].empty())
			return -1;
		return std::stoi(m_curline->_data[3]);
	}
	std::vector<int> asset_pic_line::mesh_group_state_id()
	{
		if (m_curline->_data[4].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[4]);
		return tmpVec;
	}
	std::vector<float> asset_pic_line::boxsize()
	{
		if (m_curline->_data[5].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[5]);
		return tmpVec;
	}
	std::vector<int> asset_pic_line::wrap_entity_id()
	{
		if (m_curline->_data[6].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[6]);
		return tmpVec;
	}

	std::vector<float> asset_pic_line::extra_transfrom()
	{
		if (m_curline->_data[7].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[7]);
		return tmpVec;
	}
	int asset_pic_line::fpp_ui_path()
	{
		if (m_curline->_data[8].empty())
			return -1;
		return std::stoi(m_curline->_data[8]);
	}
	int asset_pic_line::fpp_ui_id()
	{
		if (m_curline->_data[9].empty())
			return -1;
		return std::stoi(m_curline->_data[9]);
	}
	int asset_pic_line::tpp_ui_path()
	{
		if (m_curline->_data[10].empty())
			return -1;
		return std::stoi(m_curline->_data[10]);
	}
	int asset_pic_line::tpp_ui_id()
	{
		if (m_curline->_data[11].empty())
			return -1;
		return std::stoi(m_curline->_data[11]);
	}
	std::vector<int> asset_pic_line::fsm_ids()
	{
		if (m_curline->_data[12].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[12]);
		return tmpVec;

	}
	std::vector<int> asset_pic_line::dynamic_fsm_id()
	{
		if (m_curline->_data[13].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[13]);
		return tmpVec;
	}
	int	asset_pic_line::grade_list()
	{
		if (m_curline->_data[14].empty())
			return -1;
		return std::stoi(m_curline->_data[14]);
	}

	//********************************wrap_entity********************************************************

	wrap_entity::wrap_entity(bool isInitAll/* = false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("wrap_entity"));
		if(isInitAll)
			m_table->selectAllData(Name("wrap_entity"), m_line_data);
	}

	void wrap_entity::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("wrap_entity"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	void wrap_entity::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("wrap_entity"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	wrap_entity_line* wrap_entity::getLine(int id)
	{
		auto iter = m_wrap_entity_lines.find(id);
		if (iter != m_wrap_entity_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new wrap_entity_line(&m_line_data[id]);
		m_wrap_entity_lines.emplace(id, pic);
		return pic;
	}

	wrap_entity_line::wrap_entity_line(_Select_Result* ptr) :one_line_data(ptr) {}

	int wrap_entity_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string wrap_entity_line::mesh_path()
	{
		return m_curline->_data[1];
	}
	int wrap_entity_line::sight_dis_type()
	{
		if (m_curline->_data[2].empty())
			return 0;
		return std::stoi(m_curline->_data[2]);
	}
	float wrap_entity_line::sight_dis_param()
	{
		if (m_curline->_data[3].empty())
			return .0f;
		return stof(m_curline->_data[3]);
	}
	std::vector<float> wrap_entity_line::lod_cut()
	{
		if (m_curline->_data[4].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[4]);
		return tmpVec;
	}
	int wrap_entity_line::surface_type()
	{
		if (m_curline->_data[5].empty())
			return -1;
		return std::stoi(m_curline->_data[5]);
	}
	int wrap_entity_line::collider_type()
	{
		if (m_curline->_data[6].empty())
			return -1;
		return std::stoi(m_curline->_data[6]);
	}
	int wrap_entity_line::convex_type()
	{
		if (m_curline->_data[7].empty())
			return 0;
		return std::stoi(m_curline->_data[7]);
	}

	std::string wrap_entity_line::meshphy_path()
	{
		return m_curline->_data[8];
	}
	std::vector<float> wrap_entity_line::collider_box()
	{
		if (m_curline->_data[9].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[9]);
		return tmpVec;

	}
	int wrap_entity_line::fps_state()
	{
		if (m_curline->_data[10].empty())
			return 0;
		return std::stoi(m_curline->_data[10]);
	}
	int wrap_entity_line::cast_shadow()
	{
		if (m_curline->_data[11].empty())
			return 0;
		return std::stoi(m_curline->_data[11]);
	}

	std::vector<int> wrap_entity_line::hit_geom()
	{
		if (m_curline->_data[12].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[12]);
		return tmpVec;
	}
	std::vector<int> wrap_entity_line::svr_hit_geom()
	{
		if (m_curline->_data[13].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[13]);
		return tmpVec;
	}
	std::vector<std::vector<float>>	wrap_entity_line::extra_transfrom()
	{
		GET_FLOAT_VEC_VEC(14)
	}
	std::vector<int> wrap_entity_line::target_point_offset()
	{
		if (m_curline->_data[15].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[15]);
		return tmpVec;
	}
	std::vector<float> wrap_entity_line::axis_reverse()
	{
		if (m_curline->_data[16].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[16]);
		return tmpVec;
	}

	//********************************actorblock_list********************************************************

	actorblock_list::actorblock_list(bool isInitAll/* = false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("actorblock_list")); 
		if (isInitAll)
			m_table->selectAllData(Name("actorblock_list"), m_line_data);
	}

	void actorblock_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("actorblock_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	actorblock_list_line* actorblock_list::getLine(int id)
	{
		auto iter = m_actorblock_list_lines.find(id);
		if (iter != m_actorblock_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new actorblock_list_line(&m_line_data[id]);
		m_actorblock_list_lines.emplace(id, pic);
		return pic;
	}

	void actorblock_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("actorblock_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	actorblock_list_line::actorblock_list_line(_Select_Result* ptr) :one_line_data(ptr) {}

	int actorblock_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string actorblock_list_line::fathertype()
	{
		return m_curline->_data[1];
	}
	std::string actorblock_list_line::editor_type()
	{
		return m_curline->_data[2];
	}
	std::string actorblock_list_line::editorname()
	{
		return m_curline->_data[3];
	}
	int actorblock_list_line::asset_pic()
	{
		if (m_curline->_data[4].empty())
			return -1;
		return std::stoi(m_curline->_data[4]);
	}
	std::vector<float> actorblock_list_line::boxSize()
	{
		if (m_curline->_data[5].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[5]);
		return tmpVec;

	}
	int	actorblock_list_line::weight_value()
	{
		if (m_curline->_data[6].empty())
			return -1;
		return std::stoi(m_curline->_data[6]);
	}
	int	actorblock_list_line::gravity_value()
	{
		if (m_curline->_data[7].empty())
			return -1;
		return std::stoi(m_curline->_data[7]);
	}
	int	actorblock_list_line::pulling_value()
	{
		if (m_curline->_data[8].empty())
			return -1;
		return std::stoi(m_curline->_data[8]);
	}

	std::vector<int> actorblock_list_line::slot_pool_id()
	{
		if (m_curline->_data[9].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[9]);
		return tmpVec;

	}
	std::vector<std::vector<int>> actorblock_list_line::slot_part()
	{
		//GETVECTOR(int, 10)

		std::vector<std::vector<int>> result;
		return result;
	}
	int actorblock_list_line::atb_template()
	{
		if (m_curline->_data[11].empty())
			return -1;
		return std::stoi(m_curline->_data[11]);
	}
	std::vector<int> actorblock_list_line::trait()
	{
		if (m_curline->_data[12].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[12]);
		return tmpVec;
	}
	std::vector<int> actorblock_list_line::actor_enum_list()
	{
		if (m_curline->_data[13].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[13]);
		return tmpVec;
	}
	std::vector<int> actorblock_list_line::msg_face()
	{
		if (m_curline->_data[14].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[14]);
		return tmpVec;
	}
	std::vector<std::string> actorblock_list_line::msg_info()
	{
		if (m_curline->_data[15].empty())
			return {};

		GET_STRING_VECTOR(m_curline->_data[15]);
		return tmpVec;
	}
	int	actorblock_list_line::death_logic()
	{
		if (m_curline->_data[16].empty())
			return -1;
		return std::stoi(m_curline->_data[16]);
	}

	//***********************************particle_list********************************************************************

	particle_list::particle_list(bool isInitAll/* = false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("particle_list"));
		if (isInitAll)
			m_table->selectAllData(Name("particle_list"), m_line_data);
	}

	void particle_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("particle_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	void particle_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("particle_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	particle_list_line* particle_list::getLine(int id)
	{
		auto iter = m_particle_list_lines.find(id);
		if (iter != m_particle_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new particle_list_line(&m_line_data[id]);
		m_particle_list_lines.emplace(id, pic);
		return pic;
	}

	particle_list_line::particle_list_line(_Select_Result* ptr) :one_line_data(ptr) {}

	int particle_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string particle_list_line::fathertype()
	{
		return m_curline->_data[1];
	}
	std::string particle_list_line::editor_type()
	{
		return m_curline->_data[2];
	}
	std::string particle_list_line::editor_name()
	{
		return m_curline->_data[3];
	}
	std::string particle_list_line::filename()
	{
		return m_curline->_data[4];;
	}
	bool particle_list_line::bIsPrior()
	{
		int result = std::stoi(m_curline->_data[5]);
		return result == 1;
	}
	float particle_list_line::time()
	{
		return std::stof(m_curline->_data[6]);;
	}

	//******************************newblock_list*****************************************

	newblock_list::newblock_list(bool isInitAll/* = false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("newblock_list"));
		if (isInitAll)
			m_table->selectAllData(Name("newblock_list"), m_line_data);
	}

	void newblock_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("newblock_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	void newblock_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("newblock_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	newblock_list_line::newblock_list_line(_Select_Result* ptr) :one_line_data(ptr) {}

	newblock_list_line* newblock_list::getLine(int id)
	{
		auto iter = m_newblock_list_lines.find(id);
		if (iter != m_newblock_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new newblock_list_line(&m_line_data[id]);
		m_newblock_list_lines.emplace(id, pic);
		return pic;
	}

	int newblock_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string newblock_list_line::fatherType()
	{
		return m_curline->_data[1];
	}
	std::string newblock_list_line::type()
	{
		return m_curline->_data[2];
	}
	std::string newblock_list_line::name()
	{
		return m_curline->_data[3];
	}
	int	newblock_list_line::pic_id()
	{
		if (m_curline->_data[4].empty())
			return -1;
		return std::stoi(m_curline->_data[4]);
	}
	int	newblock_list_line::cha_pic_id()
	{
		if (m_curline->_data[5].empty())
			return -1;
		return std::stoi(m_curline->_data[5]);
	}
	int	newblock_list_line::cha_skin_id()
	{
		if (m_curline->_data[6].empty())
			return -1;
		return std::stoi(m_curline->_data[6]);
	}
	float newblock_list_line::scale()
	{
		if (m_curline->_data[7].empty())
			return -1;
		return std::stof(m_curline->_data[7]);
	}
	std::string newblock_list_line::mesh()
	{
		return m_curline->_data[8];
	}
	bool newblock_list_line::collider_type()
	{
		if (m_curline->_data[9].empty())
			return false;
		int result = std::stoi(m_curline->_data[9]);
		return result == 1;
	}
	std::string newblock_list_line::collider_id()
	{
		return m_curline->_data[10];
	}
	std::string newblock_list_line::collision()
	{
		return m_curline->_data[11];
	}
	std::vector<float> newblock_list_line::collider_box()
	{
		if (m_curline->_data[12].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[12]);
		return tmpVec;
	}
	std::vector<float> newblock_list_line::boxSize()
	{
		if (m_curline->_data[13].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[13]);
		return tmpVec;
	}
	int	newblock_list_line::place_sound()
	{
		if (m_curline->_data[14].empty())
			return -1;
		return std::stoi(m_curline->_data[14]);
	}
	int	newblock_list_line::occlusion()
	{
		if (m_curline->_data[15].empty())
			return -1;
		return std::stoi(m_curline->_data[15]);
	}

	//***************************************cha_fsm_list****************************************

	cha_fsm_list_line::cha_fsm_list_line(_Select_Result* ptr) :one_line_data(ptr) {}

	int cha_fsm_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string cha_fsm_list_line::name()
	{
		return m_curline->_data[1];
	}
	std::string cha_fsm_list_line::is_custom()
	{
		return m_curline->_data[2];
	}
	int cha_fsm_list_line::fsm_basic()
	{
		if (m_curline->_data[3].empty())
			return -1;
		return std::stoi(m_curline->_data[3]);
	}
	int cha_fsm_list_line::fpe_basic()
	{
		if (m_curline->_data[4].empty())
			return -1;
		return std::stoi(m_curline->_data[4]);
	}
	int cha_fsm_list_line::fsm_micro()
	{
		if (m_curline->_data[5].empty())
			return -1;
		return std::stoi(m_curline->_data[5]);
	}

	int cha_fsm_list_line::fsm_small()
	{
		if (m_curline->_data[6].empty())
			return -1;
		return std::stoi(m_curline->_data[6]);
	}
	int cha_fsm_list_line::fsm_cover()
	{
		if (m_curline->_data[7].empty())
			return -1;
		return std::stoi(m_curline->_data[7]);
	}
	int cha_fsm_list_line::spg_basic()
	{
		if (m_curline->_data[8].empty())
			return -1;
		return std::stoi(m_curline->_data[8]);
	}
	int cha_fsm_list_line::dpp_basic() 
	{
		if (m_curline->_data[9].empty())
			return -1;
		return std::stoi(m_curline->_data[9]);
	}
	std::string cha_fsm_list_line::cover_sole()
	{
		return m_curline->_data[10];
	}
	int cha_fsm_list_line::time_line()
	{
		if (m_curline->_data[11].empty())
			return -1;
		return std::stoi(m_curline->_data[11]);
	}

	cha_fsm_list::cha_fsm_list(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("cha_fsm_list"));
		if (isInitAll)
			m_table->selectAllData(Name("cha_fsm_list"), m_line_data);
	}

	cha_fsm_list_line* cha_fsm_list::getLine(int id)
	{
		auto iter = m_cha_fsm_list_lines.find(id);
		if (iter != m_cha_fsm_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new cha_fsm_list_line(&m_line_data[id]);
		m_cha_fsm_list_lines.emplace(id, pic);
		return pic;
	}

	void cha_fsm_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("cha_fsm_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void cha_fsm_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("cha_fsm_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//************************************************building_list_line******************************************************************

	building_list_line::building_list_line(_Select_Result* ptr): one_line_data(ptr) {}
	int building_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string building_list_line::fatherType()
	{
		return m_curline->_data[1];
	}
	std::string building_list_line::editor_type()
	{
		return m_curline->_data[2];
	}
	std::string building_list_line::editorname()
	{
		return m_curline->_data[3];
	}
	int building_list_line::asset_id()
	{
		if (m_curline->_data[4].empty())
			return -1;
		return std::stoi(m_curline->_data[4]);
	}
	int building_list_line::blueprint_id()
	{
		if (m_curline->_data[5].empty())
			return -1;
		return std::stoi(m_curline->_data[5]);
	}
	std::string building_list_line::actorphy_path()
	{
		return m_curline->_data[6];
	}
	std::vector<int> building_list_line::slot_pool_id()
	{
		if (m_curline->_data[7].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[7]);
		return tmpVec;
	}
	std::vector<std::vector<int>> building_list_line::slot_part()
	{
		std::vector<std::vector<int>> result;
		return result;
	}
	int building_list_line::atb_template()
	{
		if (m_curline->_data[9].empty())
			return -1;
		return std::stoi(m_curline->_data[9]);
	}
	std::vector<int> building_list_line::trait()
	{
		if (m_curline->_data[10].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[10]);
		return tmpVec;
	}
	int building_list_line::actor_enum_list()
	{
		if (m_curline->_data[11].empty())
			return -1;
		return std::stoi(m_curline->_data[11]);
	}
	int building_list_line::intera_id()
	{
		if (m_curline->_data[12].empty())
			return -1;
		return std::stoi(m_curline->_data[12]);
	}
	int building_list_line::area_id()
	{
		if (m_curline->_data[13].empty())
			return -1;
		return std::stoi(m_curline->_data[13]);
	}
	int building_list_line::collider_id()
	{
		if (m_curline->_data[14].empty())
			return -1;
		return std::stoi(m_curline->_data[14]);
	}
	int building_list_line::dynamic_id()
	{
		if (m_curline->_data[15].empty())
			return -1;
		return std::stoi(m_curline->_data[15]);
	}
	int building_list_line::dynamic_trig()
	{
		if (m_curline->_data[16].empty())
			return -1;
		return std::stoi(m_curline->_data[16]);
	}
	std::vector<int> building_list_line::joint()
	{
		if (m_curline->_data[17].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[17]);
		return tmpVec;
	}
	int building_list_line::transfer_id()
	{
		if (m_curline->_data[18].empty())
			return -1;
		return std::stoi(m_curline->_data[18]);
	}
	int building_list_line::range()
	{
		if (m_curline->_data[19].empty())
			return -1;
		return std::stoi(m_curline->_data[19]);
	}
	float building_list_line::imposter_render_dis()
	{
		if (m_curline->_data[20].empty())
			return .0f;
		return std::stof(m_curline->_data[20]);
	}
	
	std::vector<float> building_list_line::hlod_factor()
	{
		if (m_curline->_data[21].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[21]);
		return tmpVec;
	}
	float building_list_line::Imposter_scale()
	{
		if (m_curline->_data[22].empty())
			return .0f;
		return std::stof(m_curline->_data[22]);
	}
	
	float building_list_line::hlod_scale()
	{
		if (m_curline->_data[23].empty())
			return .0f;
		return std::stof(m_curline->_data[23]);
	}
	float building_list_line::building_scale()
	{
		if (m_curline->_data[24].empty())
			return .0f;
		return std::stof(m_curline->_data[24]);
	}
	std::string building_list_line::imposter_path()
	{
		return m_curline->_data[25];
	}
	std::string building_list_line::hlod_path()
	{
		return m_curline->_data[26];
	}
	std::vector<float> building_list_line::Hlod_settings()
	{
		if (m_curline->_data[27].empty())
			return {};

		GET_FLOAT_VECTOR(m_curline->_data[27]);
		return tmpVec;
	}
	int building_list_line::dynamic_resource()
	{
		if (m_curline->_data[28].empty())
			return -1;
		return std::stoi(m_curline->_data[28]);
	}
	bool building_list_line::generate_imposter()
	{
		if (m_curline->_data[29].empty())
			return false;
		return std::stoi(m_curline->_data[29]);
	}
	bool building_list_line::generate_hlod()
	{
		if (m_curline->_data[30].empty())
			return false;
		return std::stoi(m_curline->_data[30]);
	}
	bool building_list_line::is_inner()
	{
		if (m_curline->_data[31].empty())
			return false;
		return std::stoi(m_curline->_data[31]);
	}
	std::vector<std::vector<float>>	building_list_line::inner_box_size()
	{
		GET_FLOAT_VEC_VEC(32)
	}
	int	building_list_line::is_export()
	{
		if (m_curline->_data[33].empty())
			return 0;
		return std::stoi(m_curline->_data[33]);
	}

	building_list::building_list(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("building_list"));
		if (isInitAll)
			m_table->selectAllData(Name("building_list"), m_line_data);
	}

	building_list_line* building_list::getLine(int id)
	{
		auto iter = m_building_list_lines.find(id);
		if (iter != m_building_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new building_list_line(&m_line_data[id]);
		m_building_list_lines.emplace(id, pic);
		return pic;
	}

	void building_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("building_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void building_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("building_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//*******************************************actorblock_material********************************************************
	actorblock_material_line::actorblock_material_line(_Select_Result* ptr) :one_line_data(ptr){}
	int actorblock_material_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string actorblock_material_line::type()
	{
		return m_curline->_data[1];
	}
	std::string actorblock_material_line::name()
	{
		return m_curline->_data[2];
	}
	std::string actorblock_material_line::material()
	{
		return m_curline->_data[3];
	}
	std::vector<int> actorblock_material_line::material_Type()
	{
		if (m_curline->_data[4].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[4]);
		return tmpVec;
	}
	std::vector<int> actorblock_material_line::material_enum_list()
	{
		if (m_curline->_data[5].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[5]);
		return tmpVec;
	}


	actorblock_material::actorblock_material(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("actorblock_material"));
		if (isInitAll)
			m_table->selectAllData(Name("actorblock_material"), m_line_data);
	}

	actorblock_material_line* actorblock_material::getLine(int id)
	{
		auto iter = m_actorblock_material_lines.find(id);
		if (iter != m_actorblock_material_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new actorblock_material_line(&m_line_data[id]);
		m_actorblock_material_lines.emplace(id, pic);
		return pic;
	}

	void actorblock_material::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("actorblock_material"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}


	void actorblock_material::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("actorblock_material"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//******************************************mesh_group_state_list***********************************************
	mesh_group_state_list_line::mesh_group_state_list_line(_Select_Result* ptr) : one_line_data(ptr) {}
	int mesh_group_state_list_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::vector<int> mesh_group_state_list_line::tpp_state_group_id()
	{			   
		if (m_curline->_data[6].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[6]);
		return tmpVec;
	}			   
	std::vector<int> mesh_group_state_list_line::fpp_state_group_id()
	{
		if (m_curline->_data[7].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[7]);
		return tmpVec;
	}
	std::vector<int> mesh_group_state_list_line::fpp_tpp_sign_id()
	{
		if (m_curline->_data[8].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[8]);
		return tmpVec;
	}
	std::vector<int> mesh_group_state_list_line::fpp_seat_state_group_id()
	{
		if (m_curline->_data[9].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[9]);
		return tmpVec;
	}
	std::vector<int> mesh_group_state_list_line::fpp_tpp_seat_sign_id()
	{
		if (m_curline->_data[10].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[10]);
		return tmpVec;
	}

	mesh_group_state_list::mesh_group_state_list(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("mesh_group_state_list"));
		if (isInitAll)
			m_table->selectAllData(Name("mesh_group_state_list"), m_line_data);
	}

	mesh_group_state_list_line* mesh_group_state_list::getLine(int id)
	{
		auto iter = m_mesh_group_state_list_lines.find(id);
		if (iter != m_mesh_group_state_list_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new mesh_group_state_list_line(&m_line_data[id]);
		m_mesh_group_state_list_lines.emplace(id, pic);
		return pic;
	}

	void mesh_group_state_list::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("mesh_group_state_list"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void mesh_group_state_list::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("mesh_group_state_list"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//*************************************asset_pic_building*****************************************************

	asset_pic_building_line::asset_pic_building_line(_Select_Result* ptr) :one_line_data(ptr){}
	int	asset_pic_building_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}
	std::string asset_pic_building_line::actor_path()
	{
		return m_curline->_data[1];
	}
	int asset_pic_building_line::tpp_wrap_role()
	{
		if (m_curline->_data[2].empty())
			return 0;
		return std::stoi(m_curline->_data[2]);
	}
	int asset_pic_building_line::fpp_wrap_role()
	{
		if (m_curline->_data[3].empty())
			return 0;
		return std::stoi(m_curline->_data[3]);
	}
	std::vector<int> asset_pic_building_line::mesh_group_state_id()
	{
		if (m_curline->_data[4].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[4]);
		return tmpVec;
	}
	std::vector<int> asset_pic_building_line::boxsize()
	{
		if (m_curline->_data[5].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[5]);
		return tmpVec;
	}
	std::vector<int> asset_pic_building_line::wrap_entity_id()
	{
		if (m_curline->_data[6].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[6]);
		return tmpVec;
	}
	std::vector<std::vector<float>> asset_pic_building_line::extra_transfrom()
	{
		GET_FLOAT_VEC_VEC(7)
	}
	std::vector<int> asset_pic_building_line::fpp_ui_path()
	{
		if (m_curline->_data[8].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[8]);
		return tmpVec;
	}
	int asset_pic_building_line::fpp_ui_id()
	{
		if (m_curline->_data[9].empty())
			return 0;
		return std::stoi(m_curline->_data[9]);
	}
	int asset_pic_building_line::tpp_ui_path()
	{
		if (m_curline->_data[10].empty())
			return 0;
		return std::stoi(m_curline->_data[10]);
	}
	int asset_pic_building_line::tpp_ui_id()
	{
		if (m_curline->_data[11].empty())
			return 0;
		return std::stoi(m_curline->_data[11]);
	}
	std::vector<int> asset_pic_building_line::fsm_ids()
	{
		if (m_curline->_data[12].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[12]);
		return tmpVec;
	}

	std::vector<int> asset_pic_building_line::dynamic_fsm_id()
	{
		if (m_curline->_data[13].empty())
			return {};

		GET_INT_VECTOR(m_curline->_data[13]);
		return tmpVec;
	}
	int	asset_pic_building_line::grade_list()
	{
		if (m_curline->_data[14].empty())
			return 0;
		return std::stoi(m_curline->_data[14]);
	}

	asset_pic_building::asset_pic_building(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("asset_pic_building"));
		if (isInitAll)
			m_table->selectAllData(Name("asset_pic_building"), m_line_data);
	}

	asset_pic_building_line* asset_pic_building::getLine(int id)
	{
		auto iter = m_asset_pic_building_lines.find(id);
		if (iter != m_asset_pic_building_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new asset_pic_building_line(&m_line_data[id]);
		m_asset_pic_building_lines.emplace(id, pic);
		return pic;
	}

	void asset_pic_building::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("asset_pic_building"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void asset_pic_building::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("asset_pic_building"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//*************************************building_info*****************************************************

	building_info_line::building_info_line(_Select_Result* ptr) :one_line_data(ptr) {}

	int	building_info_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}

	std::vector<std::vector<float>> building_info_line::boxSize()
	{
		GET_FLOAT_VEC_VEC(1)
	}

	building_info::building_info(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("building_info"));
		if (isInitAll)
			m_table->selectAllData(Name("building_info"), m_line_data);
	}

	building_info_line* building_info::getLine(int id)
	{
		auto iter = m_building_info_lines.find(id);
		if (iter != m_building_info_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new building_info_line(&m_line_data[id]);
		m_building_info_lines.emplace(id, pic);
		return pic;
	}

	void building_info::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("building_info"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void building_info::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("building_info"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}

	//************************************************physics_filter_groups*****************************************************

	physics_filter_groups_line::physics_filter_groups_line(_Select_Result* ptr) :one_line_data(ptr) {}
	int physics_filter_groups_line::id()
	{
		if (m_curline->_data[0].empty())
			return -1;
		return std::stoi(m_curline->_data[0]);
	}

	std::string physics_filter_groups_line::name()
	{
		return m_curline->_data[1];
	}

	int physics_filter_groups_line::bit(int idx)
	{
		int tmp = 2 + idx;
		if (m_curline->_data[tmp].empty())
			return -1;
		return std::stoi(m_curline->_data[tmp]);
	}

	physics_filter_groups::physics_filter_groups(bool isInitAll /*= false*/) :lines_data(isInitAll)
	{
		m_table = SqliteMgr::instance()->getSqliteByTableName(Name("physics_filter_groups"));
		if (isInitAll)
			m_table->selectAllData(Name("physics_filter_groups"), m_line_data);
	}

	physics_filter_groups_line* physics_filter_groups::getLine(int id)
	{
		auto iter = m_physics_filter_groups_lines.find(id);
		if (iter != m_physics_filter_groups_lines.end())
			return iter->second;

		setID(id);
		if (m_curline._data.empty())
			return nullptr;
		auto pic = new physics_filter_groups_line(&m_line_data[id]);
		m_physics_filter_groups_lines.emplace(id, pic);
		return pic;
	}

	void physics_filter_groups::getData()
	{
		auto iter = m_line_data.find(m_id);
		if (iter == m_line_data.end())
		{
			if (m_table != nullptr)
			{
				m_curline = m_table->selectDatasById(Name("physics_filter_groups"), Name("id"), m_id);
				m_line_data.insert({ m_id, m_curline });
			}
		}
		else
		{
			m_curline = iter->second;
		}
	}

	void physics_filter_groups::initAllData()
	{
		if (!m_bIsInitAllExcelData)
			m_table->selectAllData(Name("physics_filter_groups"), m_line_data);

		for (auto&& data : m_line_data)
		{
			getLine(data.first);
		}
	}
}
