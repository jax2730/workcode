#pragma once
#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoSqlite.h"
#include "EchoSingleton.h"

namespace Echo
{

	/*这个头文件的函数返回值根据需求更改*/
	/*此文件定义读取表中某一行的值的函数，一个类的构造是以要读取的行id为参数进行构造的，类中所包含的成员是要查询的那一行的所包含的所有字段的值*/

	/*客户端里的数据在引擎编辑器中是通过转化成sqlite数据库然后再以读取数据库的方式进行查询的，
	可以直接通过调用SqliteObject中的函数直接读取数据库中的数据，
	也可以通过在这个文件中创建类似于客户端的相应的类去读取可能会方便一点，例子如下所示*/

	/*为了方便将要读取的表的名字写成与文件名字相对应*/

	/*引擎中的数据库可以通过任意种类的(分隔符标记的文件,如txt,csv等)加载转换成sqlite数据表*/
	/*SU组件转到引擎工具*/

	//获取某个表格某一行的所有列
#define  GetExcelLineData(tableName, id)  ExcelDataManager::instance()->getExcelByName(#tableName) == nullptr ? nullptr : \
	static_cast<tableName*>(ExcelDataManager::instance()->getExcelByName(#tableName))->getLine(id);

	enum eActorType
	{
		eActorTypeNone = 0,
		eActorTypeItem = 1,
		eActorTypeCha = 2,
		eActorTypeSeat = 3,
		eActorTypeVehicle = 4,
		eActorType3DUI = 5,
		eActorTypeBlock = 6,
		eActorTypeBuilding = 7,
		eActorTypeParticle = 8,
		eActorTypeWeather = 9,
		eActorTypeCount,
	};

	struct _Select_Result;
	class SqliteObject;

	class _EchoComponentExport lines_data		//一张表中的所有列数据集合
	{
	public:
		virtual ~lines_data();
		lines_data(bool isInitAll = false);
		void					setID(int id);
		virtual void			initAllData() { }
	protected:
		virtual void			getData() { };
	protected:
		std::map<int, _Select_Result>		m_line_data;
		int									m_id = -1;
		SqliteObject*						m_table = nullptr;
		_Select_Result						m_curline {};
		bool								m_bIsInitAllExcelData = false;
	};

	class _EchoComponentExport ExcelDataManager :public Singleton<ExcelDataManager>
	{
	public:
		ExcelDataManager(bool initAll = false);
		~ExcelDataManager();
		lines_data*						getExcelByName(const String& name);
		void							getResSet(std::set<std::string>& ResSet);
		void							getResFromActor(const String& path, std::set<String>& resources);
		void							collectRes(const String& res);
	private:
		std::map<String, lines_data*>		m_excels;
		std::set<std::string>				m_resSet;
		bool								mbCollectRes = false;
	};

	class _EchoComponentExport one_line_data		//表中所有列一行的数据，继承这个基类
	{
	public:
		one_line_data(_Select_Result* ptr);
		bool					isNull() { return m_curline->_data.empty(); }
	protected:
		_Select_Result*		m_curline = nullptr;
	};

	//-------------------------asset_pic----------------------
	class _EchoComponentExport asset_pic_line :public one_line_data
	{
	public:
		asset_pic_line(_Select_Result* ptr);
		int						id();
		std::string				actor_path();
		int						tpp_wrap_role();
		int						fpp_wrap_role();
		std::vector<int>		mesh_group_state_id();
		std::vector<float>		boxsize();
		std::vector<int>		wrap_entity_id();
		std::vector<float>		extra_transfrom();
		int						fpp_ui_path();
		int						fpp_ui_id();
		int						tpp_ui_path();
		int						tpp_ui_id();
		std::vector<int>		fsm_ids();
		std::vector<int>		dynamic_fsm_id();
		int						grade_list();
	};

	class _EchoComponentExport asset_pic_block :public lines_data
	{
	public:
		asset_pic_block(bool isInitAll = false);
		//asset_pic(int id);
		virtual void			initAllData() override;
		asset_pic_line*			getLine(int id);
		std::map<int, asset_pic_line*>& getAllLine() { return m_asset_pic_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, asset_pic_line*>	m_asset_pic_lines;
	};

	//-----------------actorblock_list---------------------------------

	class _EchoComponentExport actorblock_list_line :public one_line_data
	{
	public:
		actorblock_list_line(_Select_Result* ptr);
		int						id();
		std::string				fathertype();
		std::string				editor_type();
		std::string				editorname();
		int						asset_pic();
		std::vector<float>		boxSize();
		int						weight_value();
		int						gravity_value();
		int						pulling_value();
		std::vector<int>		slot_pool_id();
		std::vector<std::vector<int>>	slot_part();
		int						atb_template();
		std::vector<int>		trait();
		std::vector<int>		actor_enum_list();
		std::vector<int>		msg_face();
		std::vector<std::string>		msg_info();
		int						death_logic();
	};

	class _EchoComponentExport actorblock_list :public lines_data
	{
	public:
		actorblock_list(bool isInitAll = false);
		//actorblock_list(int id);
		virtual void			initAllData() override;
		actorblock_list_line*	getLine(int id);
		std::map<int, actorblock_list_line*>& getAllLine() { return m_actorblock_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, actorblock_list_line*>	m_actorblock_list_lines;
	};

	//------------------------newblock_list----------------------------------------
	class _EchoComponentExport newblock_list_line :public one_line_data
	{
	public:
		newblock_list_line(_Select_Result* ptr);
		int						id();
		std::string				fatherType();
		std::string				type();
		std::string				name();
		int						pic_id();
		int						cha_pic_id();
		int						cha_skin_id();
		float					scale();
		std::string				mesh();
		bool					collider_type();
		std::string				collider_id();
		std::string				collision();
		std::vector<float>		collider_box();
		std::vector<float>		boxSize();
		int						place_sound();
		int						occlusion();
	};

	class _EchoComponentExport newblock_list :public lines_data
	{
	public:
		newblock_list(bool isInitAll = false);
		//newblock_list(int id); 
		virtual void			initAllData() override;
		newblock_list_line*		getLine(int id);
		std::map<int, newblock_list_line*>& getAllLine() { return m_newblock_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, newblock_list_line*>	m_newblock_list_lines;
	};

	//--------------------------wrap_entity----------------------------------------

	class _EchoComponentExport wrap_entity_line :public one_line_data
	{
	public:
		wrap_entity_line(_Select_Result* ptr);
		int						id();
		std::string				mesh_path();
		int						sight_dis_type();
		float					sight_dis_param();
		std::vector<float>		lod_cut();
		int						surface_type();
		int						collider_type();
		int						convex_type();
		std::string				meshphy_path();
		std::vector<float>		collider_box();
		int						fps_state();
		int						cast_shadow();
		std::vector<int>		hit_geom();
		std::vector<int>		svr_hit_geom();
		std::vector<std::vector<float>>	extra_transfrom();
		std::vector<int>		target_point_offset();
		std::vector<float>		axis_reverse();

	};

	class _EchoComponentExport wrap_entity :public lines_data
	{
	public:
		wrap_entity(bool isInitAll = false);
		virtual void			initAllData() override;
		wrap_entity_line*		getLine(int id);
		std::map<int, wrap_entity_line*>& getAllLine() { return m_wrap_entity_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, wrap_entity_line*>	m_wrap_entity_lines;
	};

	//------------------------particle_list-------------------------------------

	class _EchoComponentExport particle_list_line :public one_line_data
	{
	public:
		particle_list_line(_Select_Result* ptr);
		int						id();
		std::string				fathertype();
		std::string				editor_type();
		std::string				editor_name();
		std::string				filename();
		bool					bIsPrior();
		float					time();
	};

	class _EchoComponentExport particle_list :public lines_data
	{
	public:
		particle_list(bool isInitAll = false);
		virtual void			initAllData() override;
		//particle_list(int id);
		particle_list_line*		getLine(int id);
		std::map<int, particle_list_line*>& getAllLine() { return m_particle_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, particle_list_line*>	m_particle_list_lines;
	};

	//-----------------------cha_fsm_list----------------------------------------
	class _EchoComponentExport cha_fsm_list_line : public one_line_data
	{
	public:
		cha_fsm_list_line(_Select_Result* ptr);
		int						id();
		std::string				name();
		std::string				is_custom();
		int						fsm_basic();
		int						fpe_basic();
		int						fsm_micro();
		int						fsm_small();
		int						fsm_cover();
		int						spg_basic();
		int						dpp_basic();
		std::string				cover_sole();
		int						time_line();
	};

	class _EchoComponentExport cha_fsm_list :public lines_data
	{
	public:
		cha_fsm_list(bool isInitAll = false);
		virtual void			initAllData() override;
		cha_fsm_list_line*		getLine(int id);
		std::map<int, cha_fsm_list_line*>& getAllLine() { return m_cha_fsm_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, cha_fsm_list_line*>	m_cha_fsm_list_lines;
	};

	//---------------------------------building_list----------------------------------------

	class _EchoComponentExport building_list_line :public one_line_data
	{
	public:
		building_list_line(_Select_Result* ptr);
		int						id();
		std::string				fatherType();
		std::string				editor_type();
		std::string				editorname();
		int						asset_id();
		int						blueprint_id();
		std::string				actorphy_path();
		std::vector<int>		slot_pool_id();
		std::vector<std::vector<int>>	slot_part();
		int						atb_template();
		std::vector<int>		trait();
		int						actor_enum_list();
		int						intera_id();
		int						area_id();
		int						collider_id();
		int						dynamic_id();
		int						dynamic_trig();
		std::vector<int>		joint();
		int						transfer_id();
		int						range();
		float					imposter_render_dis();
		std::vector<float>		hlod_factor();
		float					Imposter_scale();
		float					hlod_scale();
		float					building_scale();
		std::string				imposter_path();
		std::string				hlod_path();
		std::vector<float>		Hlod_settings();
		int						dynamic_resource();
		bool					generate_imposter();
		bool					generate_hlod();
		bool					is_inner();
		std::vector<std::vector<float>>	inner_box_size();
		int						is_export();
	};

	class _EchoComponentExport building_list :public lines_data
	{
	public:
		building_list(bool isInitAll = false);
		virtual void			initAllData() override;
		building_list_line* getLine(int id);
		std::map<int, building_list_line*>& getAllLine() { return m_building_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, building_list_line*>	m_building_list_lines;
	};

	//--------------------------------actorblock_material--------------------------------------

	class _EchoComponentExport actorblock_material_line :public one_line_data
	{
	public:
		actorblock_material_line(_Select_Result* ptr);
		int						id();
		std::string				type();
		std::string				name();
		std::string				material();
		std::vector<int>		material_Type();
		std::vector<int>		material_enum_list();
	};


	class _EchoComponentExport actorblock_material :public lines_data
	{
	public:
		actorblock_material(bool isInitAll = false);
		virtual void			initAllData() override;
		actorblock_material_line* getLine(int id);
		std::map<int, actorblock_material_line*>& getAllLine() { return m_actorblock_material_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, actorblock_material_line*>	m_actorblock_material_lines;
	};

	//-------------------------------------mesh_group_state_list--------------------------------------

	class _EchoComponentExport mesh_group_state_list_line :public one_line_data
	{
	public:
		mesh_group_state_list_line(_Select_Result* ptr);
		int						id();
		std::vector<int>		tpp_state_group_id();
		std::vector<int>		fpp_state_group_id();
		std::vector<int>		fpp_tpp_sign_id();
		std::vector<int>		fpp_seat_state_group_id();
		std::vector<int>		fpp_tpp_seat_sign_id();
	};

	class _EchoComponentExport mesh_group_state_list :public lines_data
	{
	public:
		mesh_group_state_list(bool isInitAll = false);
		virtual void			initAllData() override;
		mesh_group_state_list_line* getLine(int id);
		std::map<int, mesh_group_state_list_line*>& getAllLine() { return m_mesh_group_state_list_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, mesh_group_state_list_line*>	m_mesh_group_state_list_lines;
	};

	//-----------------------------asset_pic_building-------------------------------------

	class _EchoComponentExport asset_pic_building_line :public one_line_data
	{
	public:
		asset_pic_building_line(_Select_Result* ptr);
		int						id();
		std::string				actor_path();
		int						tpp_wrap_role();
		int						fpp_wrap_role();
		std::vector<int>		mesh_group_state_id();
		std::vector<int>		boxsize();
		std::vector<int>		wrap_entity_id();
		std::vector<std::vector<float>> extra_transfrom();
		std::vector<int>		fpp_ui_path();
		int						fpp_ui_id();
		int						tpp_ui_path();
		int						tpp_ui_id();
		std::vector<int>		fsm_ids();
		std::vector<int>		dynamic_fsm_id();
		int						grade_list();
	};

	class _EchoComponentExport asset_pic_building :public lines_data
	{
	public:
		asset_pic_building(bool isInitAll = false);
		virtual void			initAllData() override;
		asset_pic_building_line* getLine(int id);
		std::map<int, asset_pic_building_line*>& getAllLine() { return m_asset_pic_building_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, asset_pic_building_line*>	m_asset_pic_building_lines;
	};

	//-----------------------------building_info-------------------------------------

	class _EchoComponentExport building_info_line :public one_line_data
	{
	public:
		building_info_line(_Select_Result* ptr);
		int									id();
		std::vector<std::vector<float>>		boxSize();
	};

	class _EchoComponentExport building_info :public lines_data
	{
	public:
		building_info(bool isInitAll = false);
		virtual void			initAllData() override;
		building_info_line*		getLine(int id);
		std::map<int, building_info_line*>& getAllLine() { return m_building_info_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, building_info_line*>	m_building_info_lines;
	};

	//-----------------------------physics_filter_groups-------------------------------------

	class _EchoComponentExport physics_filter_groups_line :public one_line_data
	{
	public:
		physics_filter_groups_line(_Select_Result* ptr);
		int									id();
		std::string							name();
		int									bit(int idx);

	};

	class _EchoComponentExport physics_filter_groups :public lines_data
	{
	public:
		physics_filter_groups(bool isInitAll = false);
		virtual void				initAllData() override;
		physics_filter_groups_line* getLine(int id);
		std::map<int, physics_filter_groups_line*>& getAllLine() { return m_physics_filter_groups_lines; }
	protected:
		virtual void			getData() override;
	protected:
		std::map<int, physics_filter_groups_line*>	m_physics_filter_groups_lines;
	};

	//todo
	//class ...
}


