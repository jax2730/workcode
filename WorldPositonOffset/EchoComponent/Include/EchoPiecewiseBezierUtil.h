#ifndef __EchoPiecewiseBezierUtil_H__
#define __EchoPiecewiseBezierUtil_H__

#include "EchoComponentPrerequisites.h"
#include "EchoName.h"
#include "ControlHelperRenderable.h"
#include "EchoMatrix3.h"
#include "EchoSceneNode.h"
#include "EchoTrafficObject.h"
#include "EchoRay.h"
#include "EchoVector3.h"

namespace Echo
{
	class PavementFitTerrain;
	using namespace EchoBox;

	namespace PiecewiseTool
	{
		enum PiecewiseBezier
		{
			Begin = 0b1,
			Middle = 0b10,
			End = 0b100,
		};

		enum ControlType
		{
			NonControl = 0b0,
			BoxControl = 0b1,
			SourceControl = 0b10,
			DestinationControl = 0b100,
			CenterNormal = 0b1000,
			AllControl = BoxControl | SourceControl | DestinationControl | CenterNormal,
		};

		enum ActionType
		{
			CREATE_ACTION = 0b1,
			REMOVE_ACTION = 0b10,
			DELETE_ACTION = 0b100,
			INSERT_ACTION = 0b1000,
		};

		enum PiecewiseType
		{
			Curve = 0b1,
			StraightLine = 0b10,
		};

		float calculateMinTerrainLength(Ray& position, const Vector3& ModelSize, const Matrix3& tranformMatrix, bool pickEntity);

		bool PickEntity(Echo::Ray& ray, Echo::Entity** result, Echo::Vector3& hitpoint, Echo::uint32 uQueryMask, float max_distance = -1.0f, bool keyval = false);

		std::pair<Vector3, bool> ToolGetHeight(Ray& ray, bool pickEntity = false);
	}

	class _EchoComponentExport PiecewiseBezierUtil
	{
	public:
		PiecewiseBezierUtil(SceneManager* sceneManager);
		~PiecewiseBezierUtil();

		SceneNode*							GetSceneNode() { return mNode; }

		float								GetPickHeight(const Vector3& point);
		Vector3								GetPickPos(Ray& ray);

		void								DefaultHelper(const Vector3& offset);
		void								ClearHelper(const Vector3& offset);

		void								UpdateHelper(size_t pickIndex, bool AdTerrain, const Vector3& offset, PiecewiseTool::ActionType action, PiecewiseTool::PiecewiseType linetype);
		void								UpdateHelper(PiecewiseTool::ActionType action,
															const Vector3& point, const Vector3& destination, const Vector3& source, const Vector3& normal, 
															const Vector3& offset, const Vector3& point_bf, const Vector3& destination_bf, const Vector3& source_bf,
															PiecewiseTool::PiecewiseBezier place, size_t index, bool AdTerrain, bool isVisiable = true);
		void								UpdateHelper(PiecewiseTool::ActionType action, const Vector3& point_bf, const Vector3& point,
															PiecewiseTool::PiecewiseBezier place, size_t index, bool AdTerrain, const Vector3& offset);

		void								UpdateBezierLine(size_t pickIndex, bool AdTerrain, const Vector3& offset);
		void								UpdateBezierLine(const Vector3& from, const Vector3& from_des, const Vector3& from_sou,
																const Vector3& to, const Vector3& to_sou, size_t index, bool AdTerrain,
																PiecewiseTool::PiecewiseBezier place, const Vector3& offset);

		void								UpdatePartLengths(PiecewiseTool::ActionType action, size_t pick_index, size_t other_index = -1, bool flag = true);

		//---------------------------------------------------------------
		void								HighlightPoint(const Echo::Ray& ray);

		void								KeepScale(const Vector3& cam_pos);

		bool								PickHelper(const Ray& ray, size_t& pickIndex, PiecewiseTool::ControlType& controlType);

		//---------------------------------------------------------------
		float								LengthsTot(float s, size_t road_part, float t = 0.f) const;
		static float						LengthsTot(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float s, float t = 0.f);

		float								LengthFromTime(float time, size_t piece_index) const;
		static float						LengthFromTime(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float time);

		Vector3								Interpolate(float all_t) const;
		Vector3								Interpolate(unsigned int piece_index, float t) const;
		static Vector3						CInterpolate(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t);
		static Vector3						SInterpolate(const Vector3& from, const Vector3& to, float t);

		Vector3								Tangent(float t) const;
		Vector3								Tangent(unsigned int piece_index, float t) const;
		static Vector3						CTangent(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t);
		static Vector3						STangent(const Vector3& from, const Vector3& from_des, const Vector3& to, const Vector3& to_sou, float t, PiecewiseTool::PiecewiseBezier place);

		void								CalculateControlNormal();

		void								CalculateNormal(int UpdateWay, size_t index, float length, const Vector3& point,
																Vector3& tangent, Vector3& normal, Vector3& paranormal);
		static Vector3						CalcualteCenterNormal(float length, float all_length, Vector3 normal1, Vector3 normal2);
		static void							GetNormalInTangent(int UpdateWay, const Vector3& point,
																Vector3& tangent, Vector3& normal, Vector3& paranormal,
																Vector3& bef, Vector3& bef_tangent, Vector3& bef_normal, 
																const Vector3& world_n = Vector3::UNIT_Y);
		static void							CalculateTangent(const Vector3& tangent, const Vector3& normal, Vector3& T, Vector3& N);
		static void							CalculateTangent(Echo::Vector3& Tangent, Echo::Vector3& Normal, Echo::Vector3& ParNormal);
		static void							CalculateNormal(Echo::Vector3 Tangent, Echo::Vector3& Normal, Echo::Vector3& ParNormal, const Vector3& pNormal = Vector3(0.f, 1.f, 0.f));
		static void							CalculateNormal(const Vector3& PointNow, Vector3& TangentNow, Vector3& ParNormal, Vector3& NormalNow,
															const Vector3& FrontPoint, const Vector3& FrontTangent, const Vector3& FrontNormal);

		void								CalculateCurve(const Vector3& from, const Vector3& from_des, const Vector3& from_sou,
															const Vector3& to, const Vector3& to_sou, size_t index, bool AdTerrain,
															PiecewiseTool::PiecewiseBezier place, const Vector3& offset, TrafficObject* _obj);

		void								CalculateLine(const Vector3& from, const Vector3& to, size_t index, bool AdTerrain,
															PiecewiseTool::PiecewiseBezier place, const Vector3& offset, TrafficObject* _obj);
		//------------------------------------------------------------------
		PiecewiseTool::PiecewiseBezier		CalculatePlace(size_t pick_index);

		void								SetPoints(const std::vector<Vector3>& point, const std::vector<Vector3>& source, 
														const std::vector<Vector3>& destination, const std::vector<Vector3>& normal, const std::vector<bool>& pGray);
		void								SetPoint(size_t index, const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal);
		void								SetPointOffset(const std::vector<float>& liffts);
		void								SetControlToLine(size_t index = -1);
		void								AddPoint(const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal);
		size_t								GetPointCount();
		Vector3								GetPoint(size_t index) const;
		Vector3								GetSource(size_t index) const;
		Vector3								GetDestination(size_t index) const;
		Vector3								GetNormal(size_t index) const;
		void								InsertPoint(size_t index, const Vector3& point, const Vector3& source, const Vector3& destination, const Vector3& normal);
		void								DeletePoint(size_t index);
		void								ClearPoint();
		void								Clear();

		std::vector<AxisAlignedBox>&		GetBezierAABB() { return mAABBs; }
		size_t								GetLengthCount() { return mPartLengths.size(); }
		std::vector<float>&					GetLengths() { return mPartLengths; }
		float								GetPartLength(size_t part) { return mPartLengths[part]; }

		BoxHelper*							GetPickBoxHelepr(size_t pick);
		SphereHelper*						GetControlHelper(size_t pick, PiecewiseTool::ControlType type);
		void								SetControlPoint(size_t pick, PiecewiseTool::ControlType type, const Vector3& vec);
		void								SetArrowVisitable(bool visit);
		void								SetSourceColor(size_t index, bool pGray);
		void								SetSourceDefaultColor(const std::vector<bool> pGray);

		void								SetTrajectoryData(const std::vector<Vector3>& points);
		void								AddTrajectoryData(const Vector3& point);
		void								CalculateControlPosition(int index);

		//-------------------------------道路最小旋转轴计算时需要的接口-----------------------------------
		void								CalculateBfData(std::vector<Vector3>& Tangents, std::vector<Vector3>& Normals);
		void								ClearBfData();
		void								SetBfData(size_t index, const Vector3& point, const Vector3& tangent, const Vector3& normal);
		void								GetBfData(size_t index, Vector3& point, Vector3& tangent, Vector3& normal);

		//----------------------------------------------------------------------
		void								SetHelperVisible(bool visible);
		bool								GetHelperVisible() { return mNeedVisit; };

		static std::pair<Vector3, Matrix3>	GetBezierPositionAndMatrix3FromLength(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float length,
			const Vector3& normal1, const Vector3& normal2, const Transform& transform,
			int CalculateType = 2, const ActorComponentPtr& com = nullptr, int fitGroundLevel1 = 0, int fitGroundLevel2 = 0, PavementFitTerrain* terrain = nullptr);
		static std::pair<Vector3, Matrix3>	GetBezierPositionAndMatrix3FromT(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t,
			const Vector3& normal1, const Vector3& normal2, const Transform& transform,
			int CalculateType = 2, const ActorComponentPtr& com = nullptr, int fitGroundLevel1 = 0, int fitGroundLevel2 = 0, PavementFitTerrain* terrain = nullptr);

		static Vector3 GetBezierPositionAndMatrix3FromLength(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float length,
			const Vector3& normal1, const Vector3& normal2, const Matrix4& loacalTransform, const Matrix4& parementTransform, int fitGroundLevel1 = 0, int fitGroundLevel2 = 0, PavementFitTerrain* terrain = nullptr);
		static Vector3 GetBezierPositionAndMatrix3FromT(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t,
			const Vector3& normal1, const Vector3& normal2, const Matrix4& loacalTransform, const Matrix4& parementTransform, int fitGroundLevel1 = 0, int fitGroundLevel2 = 0, PavementFitTerrain* terrain = nullptr);

	public:
		struct BezierControlPoint
		{
			Vector3 point = Vector3::ZERO;
			Vector3	source_control = Vector3::ZERO;
			Vector3	destination_control = Vector3::ZERO;
		};
		//根据t值分割贝塞尔曲线，返回新增控制点数据，并且会修改原曲线控制点数据，以保证曲率不变
		static PiecewiseBezierUtil::BezierControlPoint CalculateSplittingBezierCurvesFormT(BezierControlPoint& point1, BezierControlPoint& point2, float t);
	public:
		std::vector<std::vector<Vector3>>*	m_pBezierPoint;
		Vector4								m_oSourceColor = Vector4(127.f, 127.f, 127.f, 255.f);;

	private:
		Echo::SceneManager*					mSceneManager = nullptr;
		Echo::SceneNode*					mNode = nullptr;
		std::vector<BoxHelper*>				mPointHelpers;
		std::vector<SphereHelper*>			mSourceControlHelpers;
		std::vector<SphereHelper*>			mDestinationControlHelpers;
		std::vector<ArrowHelper*>			mCenterNormalHelpers;
		std::vector<Echo::TrafficObject*>	mObjects;

		std::vector<AxisAlignedBox>			mAABBs;
		std::vector<float>					mPartLengths;
		//std::vector<float> mLiffts;

		//----------------------------------计算贝塞尔法线-----------------------------
		std::vector<Vector3>				mBfTangents;	// 记录每一段贝塞尔第一个控制点的切线方向
		std::vector<Vector3>				mBfNormals;		// 记录每一段贝塞尔第一个控制点的法线方向
		std::vector<Vector3>				mBfPoints;		// 记录每一段贝塞尔第一个控制点的顶点坐标

		//----------------------------------------------------------------------
		bool								mHaveValue = false;
		std::vector<Vector3>				mPoint;						// 相对BezierComponent坐标
		std::vector<Vector3>				mSourceControl;				// 相对mPoint坐标
		std::vector<Vector3>				mDestinationControl;		// 相对mPoint坐标
		std::vector<Vector3>				mCenterNormal;
		std::vector<ColorValue>				mSourceColors;

		float								mWeight = 0.5f;

		//---------------------------------默认贝塞尔控制点数据-------------------------------
		bool								mNeedVisit = false;
		ColorValue							mDestinationColor = Echo::ColorValue(0.8f, 0.3f, 0.7f, 0.6f);
		ColorValue							mSourceColor = Echo::ColorValue(0.5f, 0.0, 0.8f, 0.6f);
		ColorValue							mSourceGrayColor = Echo::ColorValue(0.501f, 0.501f, 0.501f, 1.f);
	};
}

#endif
