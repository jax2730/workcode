#ifndef __EchoPavementUtil_H__
#define __EchoPavementUtil_H__

#include "EchoComponentPrerequisites.h"
#include "EchoTrafficObject.h"
#include "EchoEntity.h"
#include "EchoMeshColliderComponent.h"
#include "EchoPiecewiseBezierUtil.h"
#include "EchoSubMesh.h"
#include "EchoInstanceStaticBatchObject.h"
//#include "EchoPavementObject.h"

namespace Echo
{
	namespace PavementTool
	{
		struct Vertex_ {
			Echo::Vector3 position;
			Echo::Vector3 normal;
			Vertex_() : position(Vector3::ZERO), normal(Vector3::ZERO) {};
			Vertex_(const Vector3& pos, const Vector3& nor)
			{
				position = pos;
				normal = nor;
			}
			void operator=(const Vertex_& value)
			{
				this->position = value.position;
				this->normal = value.normal;
			}
		};

		struct SubMeshData_ {
			MaterialWrapper road_material{};
			std::vector<Vertex_> road_vertexes;
			std::vector<Echo::Vector4> road_tangent;
			std::vector<Echo::Vector2> road_uv;
			std::vector<unsigned short> road_indices;

			void CopyData(SubMeshData_& other)
			{
				road_material = other.road_material;
				for (auto& v : other.road_vertexes)
					road_vertexes.push_back(v);
				for (auto& v : other.road_tangent)
					road_tangent.push_back(v);
				for (auto& v : other.road_uv)
					road_uv.push_back(v);
				for (auto& v : other.road_indices)
					road_indices.push_back(v);
			}
		};

		struct EntityData_
		{
			Vector3 point;
			Vector3 position;
			Vector3 otherValue;
			Quaternion ori;
			Matrix3 tranformMatrix;
		};

		struct AABoxData_
		{
			AxisAlignedBox AABB;
			std::vector<AxisAlignedBox> AABBs;
			std::vector<float> ABLengths;
			float ab_beginLen;
			TrafficBatchObject* obj;
			std::vector<EntityData_*> AsyncDatas;

			~AABoxData_()
			{
				obj = nullptr;
				for (auto& obj : AsyncDatas)
					delete obj;
				AsyncDatas.clear();
			}
		};

		enum SegmentAction
		{
			NullAction		= 0b0,
			SegmentIncrease = 0b1,
			SegmentDecrease = 0b10,
			NumberIncreate	= 0b100,
			NumberDecrease	= 0b1000,
		};

		using SubMeshDataCache = std::vector<PavementTool::SubMeshData_>;
		using LodMeshDataCache = std::vector<PavementTool::SubMeshDataCache>;// index -> Lod Level
		struct PavementPhyMeshData
		{
			AxisAlignedBox mBox;
			std::vector<Vector3> vertexVec;
			std::vector<uint16> indexVec;
		};
		struct PrivateMeshData
		{
			//PrivateMeshData() { SubData.resize((size_t)LOD_LEVEL_NUM); };
			MeshPtr mesh;
			size_t vertexNumMax;
			LodMeshDataCache SubData;// index -> Lod Level
			PavementPhyMeshData	PhyData;
		};

		using AsyncDataCache = std::vector<std::vector<Vector3>>;// frist vector index -> submesh (Vertex Calculation result cache)
		struct AsyncData
		{
			//AsyncData() { Points.resize((size_t)LOD_LEVEL_NUM); };
			std::shared_ptr<PrivateMeshData> only_mesh;
			std::vector<AsyncDataCache> Points;// index -> Lod Level
		};

		struct AsyncModelData
		{
			Vector3 from;
			Vector3 from_des;
			Vector3 to;
			Vector3 to_sou;
			float from_angle;
			float to_angle;
			Name mComName;

			Vector3 mWorldPosition;
			Vector3 mWorldScale;
			Quaternion mWorldOri;
		};

		struct PavementUtilMesh
		{
			std::shared_ptr<PavementTool::AsyncData> mMeshData;
			int mbUsedPlace = 0;
			float mWeighted = 1.f;
			// 1. Offset	2. Scale	3. Rotation
			std::vector<Echo::Vector3> mModelChange;
			Matrix3	mPointChangeMatrix;
			float mCreateOffset = 0.f;
			int mMaxCountInPoint = 10;
			Vector3 mModelLeftDown;
			Vector3 mModelRightUp;
			
			PavementUtilMesh() : mMeshData(new PavementTool::AsyncData()) {};
			~PavementUtilMesh() {};

			Vector3 GetOffset() { return mModelRightUp - mModelLeftDown; }
		};

		struct CalcualteNormalData_
		{
			Vector3 bf_point; 
			Vector3 bf_tangent;
			Vector3 bf_normal;

			Vector3 normal1;
			Vector3 normal2;
		};

		void DefaultSceneNodes(std::vector<std::vector<Echo::SceneNode*>>& value);
		void ReadPhyModeBufferData(std::vector<Vector3>& VertexData, std::vector<uint16>& IndexData, const Echo::MeshPtr& mesh, const Vector3& offset);
		Name GetDstPhyMeshName(const Name& srcPhyMesh);
		std::shared_ptr<PavementUtilMesh> FindPlaceUtilMeshData(size_t place, std::vector<std::shared_ptr<PavementTool::PavementUtilMesh>>& datas);

		AxisAlignedBox UpdateAsyncModelPart(size_t pick_index, float pick_length, int UpdateWay
			, std::vector<TrafficObject*>& resultdata, const PavementTool::AsyncModelData& ModelData
			, std::shared_ptr<PavementUtilMesh> MeshData, std::vector<AxisAlignedBox>& mABs
			, const CalcualteNormalData_& p_N
			, std::vector<float>& mABLength, float& mablength, int create_num, SceneManager* sceneManager);

		AxisAlignedBox CalculateAsyncPoint(size_t road_part
			, std::shared_ptr<PavementUtilMesh> mesh_da, float pick_length, size_t ai, float beginlength, float scale_size
			, const SubMeshData_& SubData, const CalcualteNormalData_& p_N
			, const std::vector<Vector3>& Points, int UpdateWay, const PavementTool::AsyncModelData& ModelData
			, TrafficObject* _object);

		void UpdateAsyncModelStep(size_t road_part, float pick_length, float step, int UpdateWay, 
			std::shared_ptr<PavementUtilMesh> MeshData, bool pickEntity, float segment_length, 
			const PavementTool::AsyncModelData& ModelData, PavementTool::AABoxData_* data, 
			const CalcualteNormalData_& p_N, int create_num, SceneManager* sceneManager);

		void AsyncPhyModel(std::unordered_map<float, float>& StoTdata
			, float NextLens, std::shared_ptr<PavementTool::PavementUtilMesh> MeshData, float part_length, int UpdataWay
			, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens
			, std::vector<std::vector<Vector3>>& VertexDatas, std::vector<std::vector<uint16>>& IndexDatas
			, const CalcualteNormalData_& p_N, std::vector<AxisAlignedBox>& oBoxs);

		void AsyncCalculatePhyStep(std::unordered_map<float, float>& StoTdata
			, float step_size, bool pickEntity, float NextLens, std::shared_ptr<PavementTool::PavementUtilMesh> mesh
			, float part_length, int UpdataWay, const Vector3& offset
			, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens
			, std::vector<PavementTool::EntityData_*>& resultData, Name& phyname, bool havePillar
			, const CalcualteNormalData_& p_N);
	}

	class PavementPhyUtil;
	class Pavement;
	class PavementWeakWrapper;
	struct RoadRangeData;
	struct BuildingRangeData;
	using PrivateMeshSharedPtr = std::shared_ptr<PavementTool::PrivateMeshData>;
	using PavementUtilMeshPtr = std::shared_ptr<PavementTool::PavementUtilMesh>;


	class _EchoComponentExport PavementModelUtil
	{
		friend PavementPhyUtil;
	public:
		PavementModelUtil(PiecewiseBezierUtil* tool = nullptr, SceneManager* scenemanager = nullptr);
		~PavementModelUtil();

		SceneNode* GetSceneNode() { return mModelNode; }
		void RemoveAllNode();
		void DeleteBezierUtil() { if (mBezierUtil != nullptr) delete mBezierUtil; }

		//初始化数据
		void DefaultAllObjects(const Vector3& offset);

		//创建当前分段道路所有渲染实例对象数据
		float UpdateIndexObject(size_t pick_index, float index_length = -1.f);//index_length当前分段贝塞尔曲线长度，不传就内部自行计算

		//按ActionType更新当前分段所有渲染实例对象数据
		std::pair<float, float> UpdateMoveObject(size_t pick_index, PiecewiseTool::ActionType action);
		std::pair<float, float> UpdateMoveObject(
			float pick_bf_length, float pick_length,
			size_t pick_index, PiecewiseTool::ActionType action, PiecewiseTool::PiecewiseBezier place);

		//更新当前分段的渲染对象数据
		bool UpdatePickModelPart(size_t pick_index, const Vector3& offset, int create_num = -1);
		bool UpdatePickModelPart(size_t pick_index, float pick_length,
			const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou,
			float from_angle, float to_angle, const Vector3& offset, int create_num = -1);
		void UpdateAsync(size_t pick_index, Vector3 offset, float pick_length
			, const std::vector<TrafficObject*>& restdata, const std::vector<AxisAlignedBox>& mABs
			, const std::vector<float>& mABLength, float mablength);

		//bool ResizeRealTimeModeSize(size_t index, int num);
		//bool ResizeRealTimeModeSize(size_t index, int num, float pick_length,
		//	const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou);

		//bool ResizeSegmentModeSize(size_t index, int num, float step, const Vector3& offset);

		void BatchRendering(int pick_index, float step, const Vector3& offset, int create_num = -1);
		void BatchRendering(int pick_index, float pick_length, float step, 
			const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, 
			float from_angle, float to_angle, const Vector3& offset, int create_num = -1);
		void UpdateAsyncStep(PavementTool::AABoxData_* data, size_t pick_index, float step, int create_num = -1);

		float CalculatePointT(float beginlength, float scale_size, size_t& part_index, const Vector3& road_point);
		float CalculatePointT(float beginlength, float scale_size, std::unordered_map<float, float>& StoTdata, size_t& part_index, const Vector3& road_point);
		float CalculatePointT(float beginlength, size_t& part_index, const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou);
		static float CalculatePointT(float beginlength, const size_t& part_index, std::unordered_map<float, float>& StoTdata,
			const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou);

		//------------------------------------------------------------------------------------------------------------
		std::shared_ptr<PavementTool::PavementUtilMesh> insertMoveObj(size_t index, float IndexLength);
		float insertMoveNode(size_t index, float IndexLength);
		void insertMovePillar(size_t index);

		float CalculateBeyondLength(size_t& road_part, float beginlength);
		PiecewiseTool::PiecewiseBezier CalculatePlace(size_t index);

		Echo::Vector3 RotateModelNormal(int pick_index, float beginLength, const Vector3& normal, const Vector3& rAxis);
		static Echo::Vector3 RotateModelNormal(float from_angle, float to_angle, int pick_index, float pick_length, float beginLength, const Vector3& normal, const Vector3& rAxis);
		
		AxisAlignedBox CalculateCoiledPoint(size_t sub_num, size_t road_part, EM_LOD_LEVEL lodlevel, float pick_length, size_t ai, float beginlength, float scale_size,
			Echo::TrafficObject* _object, std::shared_ptr<PavementTool::PavementUtilMesh> mesh,
			const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, const Vector3& offset, float from_angle = 0.f, float to_angle = 0.f);

		AxisAlignedBox InsertGround(std::shared_ptr<PavementTool::PavementUtilMesh> mesh,
			const Vector3& point, const Vector3& position, const Vector3& scale, 
			const Matrix3& tranformMatrix, const Quaternion& ori, int road_index);

		//------------------------------------------------------------------------------------------------------------
		void InitStepAndSumType(float step, int sum, int way);
		
		void InitBezierUtil(PiecewiseBezierUtil* tool);
		void InitIndexPillarHeight(size_t pillar_index, std::vector<int>& counts);

		void ClearMeshDatas();
		void AddChangeValue(const std::vector<Vector3>& changevalue);
		bool InitMeshPtr(size_t index, MeshPtr& value, const PavementTool::LodMeshDataCache& meshdata);
		void SetWeightAndPlace(size_t index, float weight, int pickplace);

		std::shared_ptr<PavementTool::PavementUtilMesh> GetPrivateMeshData(size_t index);
		bool MeshDataNotNull();

		std::pair<float, AxisAlignedBox> GetSegmentABox(size_t pick_index, size_t index, const Vector3& offset);
		AxisAlignedBox GetPickModelABox(size_t pick_index, const Vector3& offset);
		int GetIndexPillarCountSize(size_t index);

		int GetUpdateWay() { return mUpdateWay; }
		int GetMaxPillarSum() { return mMaxPillarSum; }
		float GetSpacingStep() { return mSpacingStep; }
		PiecewiseBezierUtil* GetBezierUtil() { return mBezierUtil; }

		float GetClipLength() { return mClipLength; }
		size_t GetPickModelBoxCount() { return mBoxs.size(); }
		size_t GetModeMoveCount() { return mModelMoveObjs.size(); }
		size_t GetSceneNodeCount() { return mStepBatchObjs.size(); }
		size_t GetPillarMoveCount() { return mPillarCounts.size(); }
		const std::vector<AxisAlignedBox>& GetABoxs() { return mBoxs; }
		const std::vector<AxisAlignedBox>& GetAABBs(size_t pick_index) { return mAABBs[pick_index]; }
		const std::vector<float>& GetABLengths(size_t pick_index) { return mABLengths[pick_index]; }
		size_t GetPickIndexABoxCount(size_t index) { return mAABBs[index].size(); }
		std::vector<std::vector<int>>& GetPillarCount() { return mPillarCounts; }

		void AddRequests(WorkQueue::RequestID& id, size_t pick_index, bool isStep);
		void RecodeModelPoint(size_t index);
		//void RecodeVertexData(std::vector<Vector3>& vdata, bool noRotat = false);
		static void RecodeVertex(std::vector<Vector3>& vdata, const Vector3& position, const Vector3& scale, const Matrix3& mChangeMatrix);
		Vector3 GetPoint(std::shared_ptr<PavementTool::PavementUtilMesh> mesh_d, size_t sub_num, size_t index, EM_LOD_LEVEL lodlevel);

		//------------------------------------------------------------------------------------------------------------
		void InitRotateAngle(const std::vector<float>& angle);
		float GetCreateLength() { return mSegmentLength; }
		size_t GetSegmentPick() { return mSegmentPick; }
		float GetRealLength(size_t pick_index);
		float GetRealStep(size_t pick_index);
		void SetCullDistance(float value) { mCullDistance = value; }
		void SetPickModel(bool value) { mPickModel = value; }
		bool GetPickModel() { return mPickModel; }
		float GetAngle(size_t index) { return mPointAngle[index]; }
		size_t GetMeshDataSize() { return mMeshDatas.size(); }
		void SetWorldValue(const Vector3& scale, const Quaternion& qri) { mWorldScale = scale; mWorldQri = qri; }

		String& getPathName() { return mNamePath; }

		std::vector<std::shared_ptr<PavementTool::PavementUtilMesh>>& GetMeshDatas() { return mMeshDatas; }

	public:
		// 路面 && 装饰物
		void	setPavementRenderDistance(float distance);
		void	setOrnamentRenderDistance(float distance);

		void	setRenderDistance(float distance) { mCullDistance = distance; };
		float	getRenderDistance() { return mCullDistance; };

		void	setPavementLodDistance(EM_LOD_LEVEL lodlevel, float distance);
		void	setOrnamentLodDistance(EM_LOD_LEVEL lodlevel, float distance);

		void	setLodDistance(EM_LOD_LEVEL lodlevel, float distance) { m_fLodDistance[lodlevel] = distance; };
		float	getLodDistance(EM_LOD_LEVEL lodlevel) { return m_fLodDistance[lodlevel]; };

	private:
		SceneManager* mScenemanager = nullptr;
		SceneNode* mModelNode = nullptr;

		std::vector<AxisAlignedBox>						mBoxs;//每段贝塞尔曲线上道路的包围盒
		std::vector<std::vector<float>>					mABLengths;//记录每段贝塞尔分段后的每段包围的分段距离，为所处当前贝塞尔上一点到贝塞尔顶点上沿着贝塞尔所走的距离。
		std::vector<std::vector<AxisAlignedBox>>		mAABBs;//记录每段贝塞尔生成的各个渲染对象自身的包围盒。
		std::vector<std::vector<Echo::TrafficObject*>>	mModelMoveObjs;//记录每段贝塞尔曲线，连续生成模型数据（整段模型顶点数可能会超过限制，需要拆分成多个）
		std::vector<TrafficBatchObject*>				mStepBatchObjs;//记录每段贝塞尔上合批生成对象。
		std::vector<std::vector<int>>					mPillarCounts;//记录每段贝塞尔曲线的每个路墩连续生成的数量。
		std::map<String, WorkQueue::RequestID>			mRequests;

		std::vector<std::shared_ptr<PavementTool::PavementUtilMesh>> mMeshDatas;

		std::vector<std::unordered_map<float, float>>	mIndexStoT;

		float											mCullDistance = 1000.0f;
		float											m_fLodDistance[LOD_LEVEL_NUM];

		//-----------------------------辅助分段创建-------------------------------------已废弃
		size_t											mSegmentPick = 0;		// 当前段
		float											mSegmentLength = 0.f;	// 当前段创建道路长度
		//size_t										mSegmentIndex = 0;		// 创建包围盒的中间数据
		int												mSegmentModeNumber = 0;	// 当前段分段模型生成的数量

		//----------------------------------------------------------------------

		String											mNamePath;//自身内存地址转存string
		Vector3											mWorldScale;
		Quaternion										mWorldQri;

		//----------------------------------------------------------------------
		float											mSpacingStep = 0.f;//每段贝塞尔曲线上mesh间隔距离
		float											mClipLength = 100.f;//一段贝塞尔曲线上分段的单位长度
		int												mMaxPillarSum = 0;//路墩最大延伸生成数量
		int												mUpdateWay = 0;//法线更新方式
		bool											mPickModel = false;//路墩法线反方向延伸判断方式
		String											mComName;//已废弃

		//---------------------------------------------------------------------

		//temp
		Vector3											bef_tangent = Vector3(1.f, 0.f, 0.f);
		Vector3											bef_normal = Vector3(0.f, 1.f, 0.f);
		Vector3											bef = Vector3(0.f, 0.f, 0.f);

		//std::vector<Vector3>							befs;
		//std::vector<Vector3>							bef_normals;
		//std::vector<Vector3>							bef_tangents;
		
		std::vector<float> mPointAngle;//每个路径点的绕X轴旋转角度
		
		PiecewiseBezierUtil* mBezierUtil = nullptr;
	};

	//=============================================================

	class _EchoComponentExport PavementPhyUtil
	{
	public:
		PavementPhyUtil();
		~PavementPhyUtil();

		void DeleteModelPhy(size_t phy_index, bool del = true);
		void DeleteAllModelPhy();

		void DecideCreatePhyMode(const AxisAlignedBox& role_box, const Vector3& offset);
		bool DecideDeletePhyMode(const AxisAlignedBox& role_box, const Vector3& offset);

		//------------------------------------------------------------------------------------------------------------
		void AsyncCreatePhy(const std::pair<size_t, float>& IndexLens, const std::vector<std::vector<Vector3>>& VertexDatas
			, const std::vector<std::vector<uint16>>& IndexDatas, const std::vector<AxisAlignedBox>& BoxDatas);
		void AsyncCreatePhyStep(const Vector3& offset, const std::pair<size_t, float>& IndexLens,
			const std::vector<PavementTool::EntityData_*>& resultData, const Name& phyname);

		static void InitPhyMeshName(const Name& mesh, Name& phyname);

		//------------------------------------------------------------------------------------------------------------
		void InitComponentPtr(ActorComponentPtr v) { mActorPtr = v; }
		void InitPavementModelUtil(PavementModelUtil* value) { mModelUtil = value; }

		size_t GetPhyModelCount() { return mPhyModel.size(); }
		void ResizePhyModel(size_t size) { mPhyModel.resize(size); mPhyLens.resize(size, { 0, -1.f }); }
		void InsertPhyModel() { std::vector<MeshColliderComponentPtr> v; mPhyModel.push_back(v); mPhyLens.push_back({ 0, -1.f }); }
		void InitIndexPhyData(size_t index, ActorComponentPtr v, const std::pair<size_t, float>& len) 
		{ mPhyModel[index].push_back(v.getStaticCastWeakPtr<MeshColliderComponent>()); mPhyLens.push_back(len); }
		
		//------------------------------------------------------------------------------------------------------------
		void AddRequestPhyModel(size_t pick_index, size_t ab_index, std::pair<size_t, float>& IndexLens);
		void AddRequestPhyStep(size_t pick_index, size_t ab_index, std::pair<size_t, float>& IndexLens);

	private:
		std::vector<std::vector<MeshColliderComponentPtr>> mPhyModel;
		std::vector<std::pair<size_t, float>> mPhyLens;
		std::map<String, WorkQueue::RequestID>			mRequests;

		PavementModelUtil* mModelUtil = nullptr;
		ActorComponentPtr mActorPtr;
		String mNamePath;
	};
	class RoadHelper : public WorkQueue::RequestHandler, public WorkQueue::ResponseHandler, public Singleton<RoadHelper>
	{
	public:
		RoadHelper();
		~RoadHelper();
	public:
		enum ReqType : uint16
		{
			ReqType_LoadMesh = 0,
			ReqType_LoadMeshData = 1,
			ReqType_Calculate = 1 << 1,
			ReqType_StepCalculate = 1 << 2,
			ReqType_PhyModel = 1 << 3,
			ReqType_PhyModelStep = 1 << 4,
			ReqType_ObjectMaterial = 1 << 5,
			ReqType_UpdatePavementRender = 1 << 6,
			ReqType_UpdatePavementPhy = 1 << 7,
			ReqType_UpdatePlanetPavementMask = 1 << 8,
			ReqType_UpdatePlanetBuildingMask = 1 << 9,
			ReqType_UpdatePavement = ReqType_UpdatePavementRender | ReqType_UpdatePavementPhy
		};
	public:
		static bool isVaild();

		WorkQueue::RequestID addMeshRequest(String& pavement, const Name& mesh, size_t num, size_t InfoNum, bool needData = true);
		WorkQueue::RequestID addObjectMatReq(class InstanceMaterialListener* listener, const MaterialWrapper& mat);
		WorkQueue::RequestID addCalculatePointReq(String& pavement, PavementUtilMeshPtr& MeshData, size_t pick_index, const Vector3& worldoffset, 
			int UpdateWay, float pick_length, PavementTool::AsyncModelData& Data, PavementTool::CalcualteNormalData_& p_N, SceneManager* sceneManager);
		WorkQueue::RequestID addCalculateStepReq(String& pavement, PavementUtilMeshPtr& MeshData, size_t pick_index, float pick_length, float step,
			int UpdateWay, bool pickEntity, float segment_length, const PavementTool::AsyncModelData& ModelData, PavementTool::CalcualteNormalData_& p_N, SceneManager* sceneManager);

		WorkQueue::RequestID addCalculatePhyReq(String& pavement, float NextLens, PavementUtilMeshPtr& mesh,
			float part_length, int UpdataWay, const PavementTool::AsyncModelData& ModelData, 
			std::pair<size_t, float>& IndexLens, PavementTool::CalcualteNormalData_& p_N);

		WorkQueue::RequestID addCalculatePhyStepReq(String& pavement, float step_size, bool pickEntity
			, float NextLens, PavementUtilMeshPtr& mesh, float part_length, int UpdataWay, const Vector3& offset
			, const PavementTool::AsyncModelData& ModelData, std::pair<size_t, float>& IndexLens, bool havePillar
			, PavementTool::CalcualteNormalData_& p_N);

		WorkQueue::RequestID UpdatePavement(PavementWeakWrapper& weakpavement, ReqType updateType = ReqType_UpdatePavement);
		WorkQueue::RequestID UpdatePlanetPavementMask(const RoadRangeData& data);
		WorkQueue::RequestID UpdatePlanetBuildingMask(const BuildingRangeData& data);

		//WorkQueue::RequestID addCalculateRequest(uint32 pavement_id, const Vector3& point);
		void abortRequest(unsigned long long reqID);
		void setVaild(const String& path, bool isvaild = false, PavementModelUtil* pavement = nullptr);
		void setComVaild(const String& path, bool isVaild = false, ActorComponent* pavement = nullptr);
		void setPhyVaild(const String& path, bool isVaild = false, PavementPhyUtil* pavement = nullptr);

		virtual bool canHandleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ)	override
		{
			return true;
		}

		//使用weak_ptr对计算流程阶段性做Pavement/SphericalTerrain指针有效检查，保证在指针失效时立即结束异步，减少计算浪费
		virtual WorkQueue::Response* handleRequest(const WorkQueue::Request* req, const WorkQueue* srcQ) override;

		virtual bool canHandleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override
		{
			return true;
		}

		virtual void handleResponse(const WorkQueue::Response* res, const WorkQueue* srcQ) override;

	private:
		WorkQueue* mWorkQueue;
		uint16 mWorkQueueChannel;

		std::unordered_map<String, PavementPhyUtil*> mPhyVaild;
		std::unordered_map<String, PavementModelUtil*> mVaild;
		std::unordered_map<String, ActorComponent*> mComVaild;
	};
}

#endif // !__EchoPavementUtil_H__
