///////////////////////////////////////////////////////////////////////////////
//
// @file PavementComponent.h
// @author wangyuxuan
// @date 2022/11/11 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __EchoPavementComponent_H__
#define __EchoPavementComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoProtocolComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoPavementUtil.h"
#include "EchoSubMesh.h"

namespace Echo
{
	class PiecewiseBezierComponent;
	class PavementComponent;
	class Pavement;
	class PavementWeakWrapper;

	using PiecewiseBezierComponentPtr = EchoWeakPtr<PiecewiseBezierComponent>;

	class _EchoComponentExport PavementFitTerrain
	{
	public:
		PavementFitTerrain() {};
		virtual~PavementFitTerrain() {};
	public:
		enum TerrainType : uint8
		{
			nullTerrain		= 0,
			planetTerrain	= 1 << 0,

			customTerrain1 = 1 << 1,
			customTerrain2 = 1 << 2,
			customTerrain3 = 1 << 3,
			customTerrain4 = 1 << 4,
			customTerrain5 = 1 << 5,
			customTerrain6 = 1 << 6,
			customTerrain7 = 1 << 7
		};
		struct TerrainData
		{
			float	terrainHeight = 0.0f;
			Vector3 terrainPos = Vector3::ZERO;
			Vector3 terrainNormal = Vector3::UNIT_Y;
		};
		virtual void	getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data) = 0;

		TerrainType mType = nullTerrain;
	};

	
	struct PavementMeshBaseData
	{
		Name groupName = Name::g_cEmptyName;
		Name meshName = Name::g_cEmptyName;

		int partIndex = -1;//可以单独设置在创建在某一段，默认0时默认应用到整条贝塞尔曲线（每一段）

		int updateType = 2;	// 2 依赖曲线的法线方向朝上，
							// 1 依赖组件局部空间下的y轴方向
							// 0 根据曲线坐标进行平铺，已废弃，此计算方式均会转换成依赖法线方向计算

		float	partLength = 200.f;//连续生成时为单位分段长度

		float	m_SpacingStep = 0.0f;//模型间隔生成距离，连续平铺时也可使用

		bool	m_IsCurveModel = true;//间隔生成模型是否随曲线扭曲，不扭曲时使用静态合批
		bool	m_IsPierModel = false;//间隔生成模型不随曲线扭曲时，是否按路墩的形式动态计算向 法线/局部空间Y轴 负方向延伸生成

		float	m_RenderDistance = 1000.f;
		float	m_Lod1Distance = 100.f;
		float	m_Lod2Distance = 200.f;

		Vector3 offset = Vector3::ZERO;
		Quaternion quat = Quaternion::IDENTITY;
		Vector3 scale = Vector3::UNIT_SCALE;

		uint32  m_surfaceType = 0;//物理表面类型

#ifdef ECHO_EDITOR
		bool	keepScale = false;
#endif // ECHO_EDITOR

	};

	class _EchoComponentExport PavementComponentInfo : public ActorComponentInfo
	{
	public:
		struct MeshDatas_
		{
			Name						m_MeshName;//所用mesh路径
			Echo::Vector3				m_OffsetPoint = Vector3::ZERO;
			Echo::Vector3				m_ScalePoint = Vector3(1.f, 1.f, 1.f);
			Echo::Vector3				m_RotatePoint = Vector3::ZERO;
			float						m_Weighted = 1.f;
			int							m_PickPlace = 0;
			int							m_PickType = 0;
		};

		class LayerDatas_
		{
		public:
			static int					reflectFlag;

			//装饰物相关
			float						m_SpacingStep = 0.f;
			Vector3						m_MaxRotation = Vector3::ZERO;
			Vector3						m_MinRotation = Vector3::ZERO;

			//路墩相关
			int							m_SortNumber = 0;
			int							m_MaxPillarSum = 0;
			bool						m_PickModel = false;
			bool						m_NewStoT = true;
			std::vector<std::vector<int>> m_PillarCount;

			//路面相关
			
			float						m_RenderDistance = 1000.f;
			float						m_Lod1Distance = 100.f;
			float						m_Lod2Distance = 200.f;
			int							m_UpdateWay = 2;	// 2 依赖曲线的法线方向朝上
															// 1 只依赖世界坐标的y轴朝上
															// 0 根据曲线坐标进行平铺，已废弃，此计算方式均会转换成依赖法线方向计算

			float						m_PartLength = 200.0f;//单位分段长度，当此长度大于整段曲线长度时会取曲线长度作计算
			std::vector<MeshDatas_*>	m_Meshdatas;//此段道路没有路墩和装饰物的话，这里vector里就只有一个路面所需的数据，这里下标没有特殊意义，只是记录所有的路面、装饰物、路墩数据

			LayerDatas_() {};
			~LayerDatas_()
			{
				for (auto& v : m_Meshdatas)
					delete v;
				m_Meshdatas.clear();
			}
			
			void						ExportJson(cJSON* jsonNode) const;
			bool						ImportJson(cJSON* jsonNode);
			bool						OldDataRead(cJSON* jsonNode);
		};

		PavementComponentInfo() {};
		PavementComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		~PavementComponentInfo();
		virtual void					ExportData(cJSON* jsonNode) override;
		virtual void					ImportData(cJSON* jsonNode) override;
		virtual void					CopyTo(ActorComponentInfo* info) override;
		virtual void					OverWrite() override;
#ifdef ECHO_EDITOR
		virtual bool					getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
	public:
		std::vector<PavementMeshBaseData> mPavementData;
		bool		isFitGround = false;
		bool		mVegetationCollection = false;
	protected:
		void	DataToNewModel();//导入导出时内部调用，为了将旧的数据结构转换成新的数据结构

	//----------------------------------------------------旧版数据接口------------------------------------------------------------
	public:
		//	针对UI使用
		const String&					getBindComName() { return mBindComName; }
		void							setBindComName(const String& name) { mBindComName = name; }
		void							CopyDataTo(const std::vector<LayerDatas_*>& data, std::vector<LayerDatas_*>& layers);
		void							SetPillarHeight(size_t layer_index, size_t index, const std::vector<int>& Counts);

	public:
		std::vector<LayerDatas_*>		mLayerDatas;//vector下标代表一整条贝塞尔曲线上，所分各段贝塞尔曲线的顺序
		String							mBindComName;//绑定的贝塞尔组件名（同一个actor下的）
		Name							mPavementName;
		int								RoadIndex = 0;
		String							mVersion = "1.0";
		bool							isCreatePhysics = true;

	public:
		EDITOR_REFLECT
	};

	void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<PavementComponentInfo::LayerDatas_*>& value);
	bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<PavementComponentInfo::LayerDatas_*>& value);
	
	void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<PavementMeshBaseData>& dataVec);
	bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<PavementMeshBaseData>& dataVec);

	//using namespace EchoBox;
	using PavementInfoData = PavementComponentInfo::LayerDatas_;
	using PavementInfoLayerData = PavementComponentInfo::MeshDatas_;
	//using SubMeshDataVec = std::vector<PavementTool::SubMeshData_>;

	class _EchoComponentExport PavementComponent : public NodeActorComponent
	{
		friend class RoadHelper;
		DECLAREACTORCOMTYPE_H(PavementComponent)

	public:
		//----------------------------------------------------------------------------

		void										initBezier(ActorComponentPtr mbez);
		bool										initPavementUtil(size_t num, size_t InfoNum);
		void										tryInitPavementUtil(size_t num, size_t InfoNum);
		virtual void								copyTo(const ActorComponentPtr& actCom) override;
		

		Name										getErrorInfo(size_t i);
		void										delErrorInfo(size_t i);
		int											findErrorIndex(Name value);
		int											getPartNumber(size_t part_index);

		int											getSegmentCount();
		bool										isNewData() { return mNewData; }
		bool*										getUpdate() { return &mNeedUpdate; }
		int											getCreatePart() { return mNowCreateSegment; }
		size_t										getMeshNumber() { return mRoadUtil.size(); }
		size_t										getErrorInfoCount() { return mErrorMeshs.size(); }
		size_t										getInfoDataCount();
		String										getBindComName();
		//PiecewiseBezierComponentPtr&				getBezierIns() { return mBezier; }
		Name										PavementName(size_t meshNum);

		void										setDefaultSegment();
		void										setNewData(bool value) { mNewData = value; }
		void										setRoadIndex(int value) { if(m_pActComInfo) GetPavementInfo()->RoadIndex = value; }
		void										pushAngle(float value) { mPickAngles.push_back(value); }
		void										setAngle(size_t index, float value) { mPickAngles[index] = value; }
		void										setRoadVisible(int meshNum, bool value) { mRoadVisible[meshNum] = value; }
		void										setUpdateType(size_t meshNum, int type) { if (m_pActComInfo) GetPavementInfo()->mLayerDatas[meshNum]->m_UpdateWay = type; }
		void										setSpacingStep(size_t meshNum, float step) { if (m_pActComInfo) GetPavementInfo()->mLayerDatas[meshNum]->m_SpacingStep = step; }
		void										setPickModel(size_t meshNum, bool value) { mRoadUtil[meshNum]->SetPickModel(value); }
		void										deleteAngle(size_t index) { mPickAngles.erase(std::next(mPickAngles.begin(), index)); }
		//void										setCullDistance(size_t index, float value) { mRoadUtil[index]->SetCullDistance(value); }
		void										insertAngle(size_t index, float value);
		void										AddAngleInIndex(size_t index, float value);
		void										changeAngle(std::vector<float>& vec);

		//----------------------------------------------------------------------------
		virtual void								bindActor() override;
		
		virtual AxisAlignedBox						getLoadBoundingBox() override;
		void										frameWork();
		void										needUpdateBezier();
		bool										needUpdateAll();

		//----------------------------------------------------------------------------
		void										update(size_t num = -1);
		void										updatePiecewiseRoad(size_t pick_index, size_t mesh_num = -1);
		void										updateRealTime(int pick_index, int create_num);
		void										updateModelPhy(size_t num, const Vector3& offset);
		void										LoadedMeshAndUpdate(std::vector<MeshPtr>& mesh, size_t num, size_t InfoNum, const std::vector<PavementTool::LodMeshDataCache>& data);

		//----------------------------------------------------------------------------
		void										bezierAddRoadIns();
		void										bezierCleaRoadIns();
		void										deleteRoadDataUtil(size_t num);
		void										deleteRoadModel(size_t num);
		void										removeAllRoadModel();
		size_t										addRoadUtilSize();

		//----------------------------------------------------------------------------
		void										addRequestModelPick(size_t index, size_t pick_index, const Vector3& offset);
		void										addRequestModelStep(size_t index, size_t pick_index, float step, const Vector3& offset);
		//bool										saveRoadFile(cJSON* root);
		void										randomCreateRoadUtil(size_t index);

		PavementComponentInfo*						GetPavementInfo();
		std::deque<bool>							mNoReBuild;

		//---------------------------------------------新版数据结构所需核心接口---------------------------------------------------------
	public:
		virtual void				init(ActorComponentInfo* info) override;
		virtual void				unInit() override;
		virtual void				connectCom() override;
		virtual void				createComRootNodeImpl() override;
		virtual void				dealComponentChange(const ActorComponentPtr& com) override;
		virtual void				dealComponentDelete(const ActorComponentPtr& com) override;

		void						setWaitDelete(bool val) { isWaitDel = val; };
		bool						isWaitDelete() { return isWaitDel; };
		void						clearAllPavements();

		void						setBezierCom(const ActorComponentPtr& bezier);
		void						setBezierComByName(const Name& bezierName);
		PiecewiseBezierComponent*	getBezierCom() { if (mBezier.expired()) return nullptr; return mBezier.get(); };

		bool						IsPavementGroup(const Name& group);
		void						ChangePavementGroupName(const Name& oldName, const Name& newName);

		//-----------------------------------------------------按分组名设置--------------------------------------------------------------
		bool						InitializeFromTemplate(int32 templateID);
		bool						ReplaceTemplate(int32 newTemplateID);
		//设置计算使用的基础模型
		void						SetBaseMesh(const Name& meshName, const Name& group);
		//设置基础模型的偏移
		void						SetMeshOffset(const Vector3& pos, const Name& group);
		void						SetMeshQuat(const Quaternion& quat, const Name& group);
		void						SetMeshScale(const Vector3& scale, const Name& group);
		void						SetBaseMeshTransform(const Vector3& pos, const Quaternion& quat, const Vector3& scale, const Name& group);
		//设置道路分组创建生成的段数索引（-1为全部生成，超过最大段数时默认取最后一段）
		void						SetGroupPartIndex(int partIndex, const Name& group);
		//设置依据贝塞尔曲线计算的方式
		void						SetBezierUpdateType(int type, const Name& group);
		//设置单位渲染分段长度
		void						SetPartLength(float distance, const Name& group);
		//设置模型间隔生成距离
		void						SetStepLength(float distance, const Name& group);
		//设置间隔生成模型是否延曲线扭曲
		void						SetCurveModel(bool isCurveModel, const Name& group);
		//设置间隔生成模型是否采用生成路墩的计算方式
		void						SetPierModel(bool isPierModel, const Name& group);
		//设置模型渲染距离
		void						SetRenderDistance(float distance, const Name& group);
		//设置模型Lod切换距离
		void						SetLod1Distance(float distance, const Name& group);
		void						SetLod2Distance(float distance, const Name& group);
		//设置物理表面类型
		void						SetSurfaceType(uint32 type, const Name& group);

		//-----------------------------------------------------实例数据获取---------------------------------------------------------------
		//获取实例使用的基础模型
		const Name&					GetBaseMeshName(const Name& group);
		//获取实例基础模型的偏移属性
		const Vector3&				GetBaseMeshOffset(const Name& group);
		const Quaternion&			GetBaseMeshQuat(const Name& group);
		const Vector3&				GetBaseMeshScale(const Name& group);
		//获取实例创建所在的段数索引
		int							GetGroupPartIndex(const Name& group);
		//获取实例依据贝塞尔曲线计算的方式
		int							GetBezierUpdatType(const Name& group);
		//获取实例渲染分段的单位长度
		float						GetPartLength(const Name& group);
		//获取实例间隔生成模型的单位距离
		float						GetStepLength(const Name& group);
		//获取实例间隔生成模型是否随曲线扭曲
		bool						GetIsCurveModel(const Name& group);
		//获取实例间隔生成模型是否使用路墩生成的计算方式
		bool						GetIdPierModel(const Name& group);
		//获取实例模型的渲染距离
		float						GetRenderDistance(const Name& group);
		//获取实例模型的Lod切换距离
		float						GetLod1Distance(const Name& group);
		float						GetLod2Distance(const Name& group);
		//获取物理表面类型
		uint32						GetSurfaceType(const Name& group);

	//PavementTestNewMode
	//创建以及更新，目前只拆分出了路面部分（连续模型）,装饰物/路墩后续拆分
	//因为最小旋转角的计算方式会出现模型异常拼接的问题，新模式暂时取消了这个计算方式。
	//新模式的最小旋转角计算方式，目前都会被视为依赖法线的计算方式。
	public:
		enum UpdateType
		{
			UpdateRender = 1,
			UpdatePhysics = 1 << 1,
			UpdateAll = UpdateRender | UpdatePhysics
		};
		void	createPavement(const PavementMeshBaseData& data, UpdateType type = UpdateRender, bool sync = false);
		void	deletePavement(const Name& group);

		//按更新类型进行更新
		void	updateAllPavement(UpdateType type = UpdateRender, bool sync = false);
		void	updateGroupPavement(const Name& group, UpdateType type = UpdateRender, bool sync = false);
		void	updateIndexPavement(size_t index, UpdateType type = UpdateRender, bool sync = false);
		void	updatePavement(size_t index, const Name& group, PavementComponent::UpdateType type = UpdateRender, bool sync = false);
		void	updatePavementPhyObj(std::shared_ptr<Pavement> sharedpavement);

	public:
		bool saveRoadFile(cJSON* root);

		class _EchoComponentExport PavementFitPlanetTerrain : public PavementFitTerrain
		{
		public:
			PavementFitPlanetTerrain(ActorComponentPtr pavement) { com = pavement; mType = planetTerrain; };
			virtual~PavementFitPlanetTerrain() {};
		public:

			virtual void	getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data);

			//void	setTerrainPtr(SphericalTerrain* planet);
			virtual SphericalTerrain* getTerrainPtr() { return nullptr; };
		protected:
			ActorComponentPtr com = nullptr;
		};

		//贴地模式测试（整个道路贴地）
		void setFitGroundModel(bool isFitGround);
		bool getFitGroundModel() { return fitGround; };

		//不设置默认使用defult（星球贴地）
		void				setPavementFitTerrain(PavementFitTerrain* terrain);
		PavementFitTerrain* getPavementFitTerrain() { return mFitTerrain; };

		using PavementGroup = std::map<Name, std::pair<PavementMeshBaseData, std::vector<std::shared_ptr<Pavement>>>>;
		const PavementGroup& getPavementGroups() { return mPavements; };

		//植被剔除收集
		void setVegetationCollection(bool isCollection);
		bool getVegetationCollection() { return mVegetationCollection; };

		bool isPavementsCreateFinish();

		void setRoadID(int32 roadID) { mRoadID = roadID; }

		int32 getRoadID() const { return mRoadID; }

		bool isUsingTemplate() const { return mCurrentTemplateID != -1; }

		int32 getCurrentTemplateIndex() const { return mCurrentTemplateID; }
	protected:
		//关联的贝塞尔组件
		PiecewiseBezierComponentPtr	mBezier = nullptr;

		// 路网RoadID，如果不由路网生成则为-1
		int32 mRoadID = -1;

		// 当前使用的模板ID，如果没有使用模板则为-1
		int32 mCurrentTemplateID = -1;

		//按名称分组
		PavementGroup		mPavements;

		//道路实例对应创建的物理组件
		std::map<
			std::weak_ptr<Pavement>,
			std::vector<MeshColliderComponent*>,
			std::owner_less<std::weak_ptr<Pavement>>>				mPavementPhys;

		bool		connectFirst = true;

		bool		fitGround = false;
		PavementFitTerrain* mFitTerrain = nullptr;
		//PavementFitTerrain* mDefaultFitTerrain = nullptr;

		bool		mVegetationCollection = false;

	//----------------------------------------旧版数据，新版本稳定后废弃删除-----------------------------------------------
	private:
		std::vector<PavementModelUtil*>				mRoadUtil;
		std::vector<PavementPhyUtil*>				mRoadPhyUtil;
		std::vector<bool>							mMeshAddScene;

		std::vector<Name>							mErrorMeshs;
		std::vector<float>							mPickAngles;

		std::vector<bool>							mRoadVisible;		// 记录每个mesh是否勾选更新信息	
		std::vector<int>							mTerrainValue;		// 记录道路包围盒四角的地形等级

		//bool										mAllUpdate = true;	// 是否更新所有submesh
		bool										mNewData = true;	// 是否为第一次加载数据
		bool										mNeedUpdate = true;	// 是否需要更新道路
		//bool										createPhy = false;	// 是否创建物理模型
		//bool										connectFirst = true;// 是否为第一次调用connectActor

		int											mRealCreateNumber = -1;
		int											mRealCreateSegment = -1;

		int											mNowCreateNumber = -1;
		int											mNowCreateSegment = -1;

		float										mRealCreateLength = 0.f;
		std::vector<int>							mPhyCreateNumberInPart;
		String										mNamePath;
		//std::vector<PavementTool::AsyncData>		mMeshDatas;

		bool										isWaitDel = false;
	};
	using PavementComponentPtr = EchoWeakPtr<PavementComponent>;


	DEFINECOMFACTORY(PavementComponent, ALL_MOVEABLE)
}

#endif // __EchoPavementComponent_H__
