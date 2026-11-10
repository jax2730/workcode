///////////////////////////////////////////////////////////////////////////////
//
// @file EchoPiecewiseBezierComponent.h
// @author luoshuquan
// @date 2022/07/19 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __EchoPiecewiseBezierComponent_H__
#define __EchoPiecewiseBezierComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoPavementComponent.h"
#include "EchoCurvedMeshComponent.h"

#include "EchoPiecewiseBezierUtil.h"

namespace Echo
{
	//=============================================================================
	struct FilePath
	{
		int s_id;
		String s_path;
		bool s_isVaild = false;
	};

	enum BezierOperation
	{
		NullAction			= 0b0,
		RemoveControlPoint	= 0b1,
		DeleteControlPoint	= 0b10,
		CreateControlPoint	= 0b100,
	};

	class _EchoComponentExport PiecewiseBezierComponentInfo : public ActorComponentInfo
	{
		//friend PiecewiseBezierComponent;
	public:
		struct BezierInfoPoint
		{
			static int					reflectFlag;
			Vector3						point;//贝塞尔曲线分段控制点的位置
			Vector3						source_control;//贝塞尔曲线曲率控制点，是上边point的前一个点
			Vector3						destination_control;//贝塞尔曲线曲率控制点，是上边point的后一个点
			Vector3						normal;//贝塞尔曲线分段控制点的法线方向
			bool						lock_control = true;//该贝塞尔控制点point所连接控制的两段贝塞尔曲线，在该point是否做平滑处理
			float						AxisXAngle = 0.f;//该控制点在组件模型空间下X轴正方向的偏移角度

			int							FitGorundLevel = 0;//贴地等级

			void						ExportJson(cJSON* jsonNode) const;
			bool						ImportJson(cJSON* jsonNode);
			bool						operator!=(const BezierInfoPoint& data);
		};
		PiecewiseBezierComponentInfo() {};
		PiecewiseBezierComponentInfo(const Name& type, ActorCOMMoveableType moveable);

		virtual void					ExportData(cJSON* jsonNode) override;
		virtual void					ImportData(cJSON* jsonNode) override;
		virtual void					CopyTo(ActorComponentInfo* info) override;
		virtual void					OverWrite() override;

		void							setPoints(const std::vector<float>& liffts);
		void							ClearData();
		//bool							createDataToFile(const String& file_path);
	public:
		std::vector<BezierInfoPoint>	mPoints;//贝塞尔控制点，由多段贝塞尔曲线组成的一整条贝塞尔曲线，控制点顺序按下Vector标排序

		bool							AdsorbTerrain = false;//吸附地面
		bool							PullTerrain = false;

		Vector3							mixPoint;//包围盒aabb
		Vector3							maxPoint;

		int								mLineType = 1;
		//int							m_Iid = -1;
		//std::vector<std::vector<Vector3>> minBox;

		EDITOR_REFLECT
	};

	void WriteJSNode(JSNODE pRoot, const char* name, const PiecewiseBezierComponentInfo::BezierInfoPoint& value);
	bool ReadJSNode(JSNODE pRoot, const char* name, PiecewiseBezierComponentInfo::BezierInfoPoint& value);

	using PiecewiseBezierPointVec = std::vector<PiecewiseBezierComponentInfo::BezierInfoPoint>;
	using PiecewiseBezierPoint = PiecewiseBezierComponentInfo::BezierInfoPoint;
	using namespace EchoBox;

	struct _EchoComponentExport BezierFallbackAction
	{
		struct OneAction
		{
			BezierOperation			mAction = NullAction;
			size_t					mPiecewiseIndex;
			PiecewiseBezierPoint	mOldPosData;
			PiecewiseBezierPoint	mNewPosData;
		};
		size_t					mRecodeIndex = -1;
		PiecewiseBezierPoint	mRecodePoint;

		OneAction Recode(BezierOperation act, size_t index, PiecewiseBezierPoint& new_d);
	};

	class _EchoComponentExport PiecewiseBezierComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(PiecewiseBezierComponent)

	public:
		virtual void					init(ActorComponentInfo* info) override;
		virtual void					unInit() override;
		virtual void					showEditorHelper(bool isShow) override;
		void							updateToolPoints();

		void							update();
		void							updatePart(size_t index = -1);
		void							frameWork();
		void							updateControlColor(size_t index = -1);

		//======================================================================

		void							addRoadIns(ActorComponentPtr value);
		void							clearRoadIns(ActorComponentPtr value = nullptr);

		void RoadUpdate(bool sync = false);
		void RoadPartUpdate(bool sync = false);

		void							addCurvedComponent(ActorComponentPtr pCom);
		bool							deleteCurvedComponent(ActorComponentPtr pCom);

		void							ChangeToControlPoint(Vector3& pick_point, Vector3& other_point, const Vector3& move_diff);
		Vector3							onChangedPoint(const Vector3& move_diff);

		size_t							getPointCount() const;
		PiecewiseBezierPointVec*		getPoints() const;
		PiecewiseBezierComponentInfo*	getInfo();
		void							changePoints(const PiecewiseBezierPointVec& points);

		void							insertNullPoint(size_t index);
		void							NotificationInsertOp(size_t index);
		void							insertPoint(size_t index, const PiecewiseBezierPoint& point);
		void							addPoint(const PiecewiseBezierPoint& point);

		void							removePoint(size_t index);

		void							setPoint(size_t index, const PiecewiseBezierPoint& point);
		void							setPointsOffset(const std::vector<float>& liffts);
		void							setBoxPoint(const Vector3& point, size_t index = -1);
		void							setSourcePoint(const Vector3& point, size_t index = -1);
		void							setDestinationPoint(const Vector3& point, size_t index = -1);
		void							setPickHelperPoint(Vector3 value, size_t index = -1);

		Vector3							getPickHelperPoint(size_t index = -1, PiecewiseTool::ControlType control = PiecewiseTool::NonControl);
		PiecewiseBezierPoint*			getPoint(size_t index);

		std::vector<float>&				getLengthVec();
		float							getPiecewiseLength(size_t Index, bool changeTer = true);
		float							getSegmentLengths(bool changeTer = true, size_t index = -1);

		PiecewiseBezierUtil*			getBezierTool() { return mBezierUtil; }

		bool							pickHelper(const Ray& ray);

		//======================================================================
		void							rebalanceLifft();
		void							rebalanceLifft(std::vector<float>& pLiffts);

		void							setAdTerrain(bool value) { if(m_pActComInfo) static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->AdsorbTerrain = value; }
		bool							getAdTerrain();
		bool*							getAdTerrainIns();

		void							setPullTerrain(bool value) { if (m_pActComInfo) static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->PullTerrain = value; }
		bool							getPullTerrain();
		bool*							getPullTerrainIns();

		bool							getEditing() { return isEditing; }
		void							setEditing(bool value) { isEditing = value; }
		bool*							getEditingIns() { return &isEditing; }

		void							setTrackChange(bool value) { mTrackChange = value; }
		bool							getTrackChange() { return mTrackChange; }

		float*							getXAxisAngleIns(int pointIndex);
		float							getRotateAngle(int pointIndex);

		bool							getVisitArrow() { return mVisitArrow; }
		bool*							getVisitArrowIns() { return &mVisitArrow; }

		void							setAxisAlignedBox(const AxisAlignedBox& value);
		Vector3							getAABBMin();
		Vector3							getAABBMax();

		PiecewiseTool::ActionType		getActionType() { return mAction; }
		void							setActionType(PiecewiseTool::ActionType value) { mAction = value; }

		PiecewiseTool::ControlType		getPickHelperType() { return mHelperPickedType; }
		void							setPickHelperType(PiecewiseTool::ControlType value) { mHelperPickedType = value; }

		size_t							getPickHelperIndex() const { return mHelperPickedIndex; }
		void							setPickHelperIndex(size_t value);
		void							getIndexNormal(size_t value, Vector3& v);

		Vector4&						getSourceColor();
		void							setSourceColor();

		PiecewiseTool::PiecewiseType	getLineType() { return mLineType; }
		void							setLineType(PiecewiseTool::PiecewiseType value);
		void							insertRoadAngle(size_t index);

		std::pair<cJSON*, cJSON*>		readbezierFile(const std::string& filepath, PiecewiseBezierPointVec& pReadData);

		bool							saveBezierRoadFile(const std::string& filePath);
		bool							loadBezierFile(const std::string& filePath);
		bool							connectOtherBezier(const std::string& pPath1, const std::string& pPath2, 
															const Vector3& pOffset, const Vector3& pRotation, bool one, bool second, bool isRetention = true);
		int								isConnectOtherBezier(Vector3& inout_WorldPosition, Vector3& out_ControlPoint);

		BezierFallbackAction::OneAction	RecordThisOperation(BezierOperation action = RemoveControlPoint, size_t index = -1);
		void							UndoCommand(const BezierFallbackAction::OneAction& pRecodeAction);
		void							RedoCommand(const BezierFallbackAction::OneAction& pRecodeAction);
		//------------------------------------------------------------------------------------------------------------------

		//保证原曲线曲率不变，在t值位置处分割曲线
		void											AddBezierControlPoint(size_t index, float t);

		//---------------------贴地测试接口------------------
		std::pair<Vector3, Matrix3> getBezierVector3AndMatrix3FromT(size_t index, const Transform& transform, float t, bool isFitGround, int CalculateType = 2, PavementFitTerrain* terrain = nullptr);
		std::pair<Vector3, Matrix3> getBezierVector3AndMatrix3FromLength(size_t index, const Transform& transform, float length, bool isFitGround,int CalculateType = 2, PavementFitTerrain* terrain = nullptr);

	private:
		PiecewiseBezierUtil*					mBezierUtil = nullptr;

		std::vector<float>						mLiffts;

		std::vector<PavementComponentPtr>		mRoads;
		std::vector<CurvedMeshComponentPtr>		m_arrCurvedCom;

		bool									isEditing = false;
		bool									mTrackChange = false;
		bool									mVisitArrow = true;

		size_t									mHelperPickedIndex = -1;						// 选中控制点的索引
		PiecewiseTool::ControlType				mHelperPickedType = PiecewiseTool::AllControl;	// 选中控制点的类型
		PiecewiseTool::ActionType				mAction = PiecewiseTool::CREATE_ACTION;			// 操作的类型
		PiecewiseTool::PiecewiseType			mLineType = PiecewiseTool::Curve;				// 创建线条类型

	public:
#ifdef ECHO_EDITOR
		BezierFallbackAction					m_pFallbackAction;
		bool									mAddScene = true;
#endif
	};

	using PiecewiseBezierComponentPtr = EchoWeakPtr<PiecewiseBezierComponent>;

	//class _EchoComponentExport PiecewiseBezierComProFunc
	//{
	//public:
	//	static void initProtocolFunction();
	//	static void uninitProtocolFunction();

	//	// static ProtocolResult ProCom_ShowTrajectory(void* isShow, ProtocolHandle handle);
	//};

	DEFINECOMFACTORY(PiecewiseBezierComponent, ALL_MOVEABLE)
}

#endif // __EchoPiecewiseBezierComponent_H__
