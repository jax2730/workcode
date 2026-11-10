///////////////////////////////////////////////////////////////////////////////
//
// @file EchoPiecewiseBezierComponent.cpp
// @author luoshuquan
// @date 2022/07/19 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoProtocolComponent.h"
#include "EchoTerrainHeightUtil.h"
#include "EchoRoadManager.h"
#include <numeric>

#define MAX_CONNECT_DIS 25.f 

#define CHANGE_LENGTHS(pickIndex, lengthsVec, Tool, typeaction)			   		  	   \
switch (typeaction)																  	   \
{																				  	   \
case PiecewiseTool::CREATE_ACTION:													   \
	if(pickIndex > 0)																   \
		lengthsVec.push_back(Tool.LengthFromTime(1.f, pickIndex - 1));		   		   \
	break;																			   \
case PiecewiseTool::DELETE_ACTION:													   \
	for (size_t i = pickIndex; i < lengthsVec.size() - 1; ++i)					  	   \
		lengthsVec[i] = lengthsVec[i + 1];										  	   \
	lengthsVec.pop_back();														  	   \
case PiecewiseTool::REMOVE_ACTION:													   \
	if((int)pickIndex - 1 >= 0)														   \
		lengthsVec[(int)pickIndex - 1] = Tool.LengthFromTime(1.f, pickIndex - 1); 	   \
	if (pickIndex < lengthsVec.size())												   \
		lengthsVec[(int)pickIndex] = Tool.LengthFromTime(1.f, pickIndex);		  	   \
	break;																			   \
default: break;																		   \
}

namespace Echo
{
	///////////////////////////////////////////////////////////////////////////////
	// PiecewiseBezierComponentInfo::BezierInfoPoint

	void WriteJSNode(JSNODE pRoot, const char* name, const PiecewiseBezierComponentInfo::BezierInfoPoint& value) {
		cJSON* jsonNode = cJSON_CreateObject();
		value.ExportJson(jsonNode);
		cJSON_AddItemToObject(pRoot, name, jsonNode);
	}
	bool ReadJSNode(JSNODE pRoot, const char* name, PiecewiseBezierComponentInfo::BezierInfoPoint& value) {
		auto jsonNode = Echo::cJSON_GetObjectItem(pRoot, name);
		if (jsonNode)
		{
			value.ImportJson(jsonNode);
			return true;
		}
		return false;
	}

	void PiecewiseBezierComponentInfo::BezierInfoPoint::ExportJson(cJSON* jsonNode) const
	{
		cJSON_AddBoolToObject(jsonNode, "lock_control", lock_control);
		cJSON_AddNumberToObject(jsonNode, "axis_x_angle", AxisXAngle);

		cJSON* _js_point = cJSON_CreateObject();
		WriteJSNode(_js_point, "point", point);
		WriteJSNode(_js_point, "source_control", source_control);
		WriteJSNode(_js_point, "destination_control", destination_control);
		WriteJSNode(_js_point, "center normal", normal);
		WriteJSNode(_js_point, "FitGorundLevel", FitGorundLevel);
		cJSON_AddItemToObject(jsonNode, "bezier_point", _js_point);
	}

	bool PiecewiseBezierComponentInfo::BezierInfoPoint::ImportJson(cJSON* jsonNode)
	{
		ReadJSNode(jsonNode, "lock_control", lock_control);
		ReadJSNode(jsonNode, "axis_x_angle", AxisXAngle);

		cJSON* _js_bezier_point = cJSON_GetObjectItem(jsonNode, "bezier_point");
		ReadJSNode(_js_bezier_point, "point", point);
		ReadJSNode(_js_bezier_point, "source_control", source_control);
		ReadJSNode(_js_bezier_point, "destination_control", destination_control);
		ReadJSNode(_js_bezier_point, "center normal", normal);
		ReadJSNode(_js_bezier_point, "FitGorundLevel", FitGorundLevel);

		if (normal == Vector3::ZERO)
			normal = Vector3::UNIT_Y;

		return true;
	}

	bool PiecewiseBezierComponentInfo::BezierInfoPoint::operator!=(const BezierInfoPoint& data)
	{
		return (
			data.AxisXAngle != this->AxisXAngle ||
			data.point != this->point ||
			data.destination_control != this->destination_control ||
			data.source_control != this->source_control ||
			data.normal != this->normal ||
			data.lock_control != this->lock_control ||
			data.FitGorundLevel != this->FitGorundLevel
			);
	}

	BezierFallbackAction::OneAction BezierFallbackAction::Recode(BezierOperation act, size_t index, PiecewiseBezierPoint& new_d)
	{
		OneAction pAct;
		pAct.mAction = NullAction;

		switch (act)
		{
		case Echo::NullAction:	break;
		case Echo::RemoveControlPoint: 
		{
			if (index == mRecodeIndex &&
				new_d != mRecodePoint )
			{
				pAct.mAction = act;
				pAct.mOldPosData = mRecodePoint;
				pAct.mPiecewiseIndex = index;
				pAct.mNewPosData = new_d;
			}

			mRecodePoint = new_d;
			mRecodeIndex = index;
		}	break;
		case Echo::DeleteControlPoint:
		case Echo::CreateControlPoint:
		{
			pAct.mAction = act;
			pAct.mPiecewiseIndex = index;
			pAct.mNewPosData = new_d;

			mRecodePoint = new_d;
			mRecodeIndex = index;

		}	break;
		default:	break;
		}
		return pAct;
	}

	// PiecewiseBezierComponentInfo
	EDITOR_REFLECT_BEGIN(PiecewiseBezierComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mPoints, "Points", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(AdsorbTerrain, "AdsorbTerrain", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(PullTerrain, "PullTerrain", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mixPoint, "mixPoint", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(maxPoint, "maxPoint", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mLineType, "PiecewiseType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	PiecewiseBezierComponentInfo::PiecewiseBezierComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{
		addCustomParam("mPoints");
	}

	void PiecewiseBezierComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<PiecewiseBezierComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void PiecewiseBezierComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<PiecewiseBezierComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void PiecewiseBezierComponentInfo::CopyTo(ActorComponentInfo * info)
	{
		if (info == nullptr)
			return;

		PiecewiseBezierComponentInfo* pInfo = static_cast<PiecewiseBezierComponentInfo*> (info);
		reflect::TypeResolver<PiecewiseBezierComponentInfo>::get()->copyTo(this, pInfo);
	}

	void PiecewiseBezierComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			PiecewiseBezierComponentInfo* m_pInfo = static_cast<PiecewiseBezierComponentInfo*>(mOriginComInfo);
			reflect::TypeResolver<PiecewiseBezierComponentInfo>::get()->overWrite(m_pInfo, this);
		}
	}

	void PiecewiseBezierComponentInfo::setPoints(const std::vector<float>& liffts)
	{
		for (int i = 0; i < liffts.size(); ++i)
			mPoints[i].point.y += liffts[i];
	}

	void PiecewiseBezierComponentInfo::ClearData()
	{
		mPoints.clear();
		AdsorbTerrain = false;
		PullTerrain = false;
		mixPoint = Vector3::ZERO;
		maxPoint = Vector3::ZERO;
		mLineType = 1;
	}

	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	// PiecewiseBezierComponent
	DECLAREACTORCOMTYPE_CPP(PiecewiseBezierComponent)
	PiecewiseBezierComponent::PiecewiseBezierComponent(ActorPtr _actor,
			ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/)
		: NodeActorComponent(_actor, movableType)
	{
		mBezierUtil = new PiecewiseBezierUtil(getSceneManager());
		RoadManager::instance()->SetBezierVec(this);

#ifdef ECHO_EDITOR
		stringstream ss;
		ss << this;
		std::shared_ptr<BezierData> mPoints(new BezierData());
		mPoints->m_pBezier = this;
		RoadManager::instance()->mBezierPoints[ss.str()] = mPoints;
		mBezierUtil->m_pBezierPoint = &RoadManager::instance()->mBezierPoints[ss.str()]->mPoints;
#else
		mBezierUtil->m_pBezierPoint = new std::vector<std::vector<Vector3>>;
#endif // ECHO_EDITOR
	}

	PiecewiseBezierComponent::~PiecewiseBezierComponent()
	{
		if (ProtocolComponentSystem::instance() != nullptr)
		{
			ProtocolComponentSystem::instance()->callProtocolFunc(
				Name("PiecewiseBezierEditModeListenerSwitches"), this, Echo::ProtocolHandleUtil::Pack(0, 0));
		}
		RoadManager::instance()->SetBezierVec(this);

#ifdef ECHO_EDITOR
		stringstream ss;
		ss << this;
		auto iter = RoadManager::instance()->mBezierPoints.find(ss.str());
		if (iter != RoadManager::instance()->mBezierPoints.end())
			RoadManager::instance()->mBezierPoints.erase(iter);
#endif
	}

	void PiecewiseBezierComponent::init(ActorComponentInfo* info)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mLineType = PiecewiseTool::PiecewiseType(mInfo->mLineType);
			updateToolPoints();
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			{
				update();
			}
		}
	}

	void PiecewiseBezierComponent::unInit()
	{
		mBezierUtil->DefaultHelper(Vector3::ZERO);
		if (mRoads.size() == 0)
		{
			mBezierUtil->Clear();
			delete mBezierUtil;
		}
	}
	
	void PiecewiseBezierComponent::showEditorHelper(bool isShow)
	{
		if (!mBezierUtil)
			return;
		mBezierUtil->SetHelperVisible(isShow);
	}

	void PiecewiseBezierComponent::frameWork()
	{
		if (!RoadManager::instance()->m_pBezierEditor.expired() &&
			RoadManager::instance()->m_pBezierEditor != this &&
			m_pActComInfo)
		{
			PiecewiseBezierComponentPtr other_bezier = RoadManager::instance()->m_pBezierEditor;
			PiecewiseBezierUtil* other_tool = other_bezier->getBezierTool();
			size_t pick_index = other_bezier->getPickHelperIndex();
			BoxHelper* pick_box = other_tool->GetPickBoxHelepr(pick_index);
			if (pick_box == nullptr) return;
			Vector3 pick_point = pick_box->getWorldPosition();
			Vector3 control_change;
			int result = isConnectOtherBezier(pick_point, control_change);
			Matrix3 mat1, mat2, mat;
			other_bezier->getWorldMatrix().extract3x3Matrix(mat1);
			this->getWorldMatrix().extract3x3Matrix(mat2);
			mat = mat1 * mat2.Inverse();
			if (result != -1)
			{
				auto cpoint = other_bezier->getPoint(pick_index);
				cpoint->point = mat * pick_point +
					mat1.Inverse() * (this->getSceneNode()->_getDerivedPosition() - other_bezier->getSceneNode()->_getDerivedPosition());
					
				auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
				cpoint->AxisXAngle = mInfo->mPoints[result].AxisXAngle;
				if (pick_index == 0)
					cpoint->destination_control = mat * control_change * cpoint->destination_control.length();
				else if (pick_index > 0)
					cpoint->source_control = mat * control_change * cpoint->source_control.length();
				other_bezier->setPoint(pick_index, *cpoint);
				//RoadManager::instance()->m_bUpdateBezier = true;
			}
		}
	}

	void PiecewiseBezierComponent::updateToolPoints()
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			std::vector<Vector3> mPoint;
			std::vector<Vector3> mSourceControl;
			std::vector<Vector3> mDestinationControl;
			std::vector<Vector3> mCenterNormal;
			std::vector<bool> mLockControl;
			for (int i = 0; i < mInfo->mPoints.size(); ++i)
			{
				PiecewiseBezierPoint* pt = getPoint(i);
				mPoint.push_back(pt->point);
				mSourceControl.push_back(pt->source_control);
				mDestinationControl.push_back(pt->destination_control);
				mCenterNormal.push_back(pt->normal);
				mLockControl.push_back(pt->lock_control);
			}
			mBezierUtil->SetPoints(mPoint, mSourceControl, mDestinationControl, mCenterNormal, mLockControl);

			if (mLineType & PiecewiseTool::StraightLine)
				mBezierUtil->SetControlToLine();
		}
	}

	void PiecewiseBezierComponent::update()
	{
#ifdef ECHO_EDITOR
		mBezierUtil->DefaultHelper(this->getWorldPosition());
		mLiffts.clear();
		//mPartLength.clear();

		//Vector3 post = getActor()->getWorldPosition();
		setActionType(PiecewiseTool::CREATE_ACTION);
		for (size_t i = 0; i < getPointCount(); ++i)
		{
			setPickHelperIndex(i);
			updatePart(i);
		}
		if (mAddScene)
		{
			addChildToNode(mBezierUtil->GetSceneNode());
			mAddScene = false;
			UpdateComRootNode(true, true);
		}
#endif
	}

	void PiecewiseBezierComponent::updatePart(size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);

			mInfo->addCustomParam("mPoints");
			mInfo->addCustomParam("mixPoint");
			mInfo->addCustomParam("maxPoint");

			if (index == -1)
				index = getPickHelperIndex();
			//Vector3 post = getActor()->getWorldPosition();

			mBezierUtil->UpdateHelper(index, getAdTerrain(), Vector3::ZERO, mAction, mLineType);
			if (!(mAction ^ PiecewiseTool::DELETE_ACTION) && index == mInfo->mPoints.size())
			{
				setPickHelperIndex(index - 1);
				index = index - 1;
			}

			mBezierUtil->UpdatePartLengths(mAction, index);

			if (getPointCount() != 0)
			{
				if (index == 0)
					mBezierUtil->UpdateBezierLine(index, getAdTerrain(), this->getWorldPosition());
				else
				{
					mBezierUtil->UpdateBezierLine(index - 1, getAdTerrain(), this->getWorldPosition());
					if (index < mInfo->mPoints.size())
						mBezierUtil->UpdateBezierLine(index, getAdTerrain(), this->getWorldPosition());
				}
			}

			/*if (getPointCount() > 0)
				CHANGE_LENGTHS(index, getLengthVec(), mBezierUtil, mAction);*/

			AxisAlignedBox ab;
			for (auto& a : mBezierUtil->GetBezierAABB())
				ab.merge(a);
			setAxisAlignedBox(ab);
			UpdateComRootNode(true);
		}
	}

	void PiecewiseBezierComponent::updateControlColor(size_t index)
	{
		if (index == -1) index = mHelperPickedType;
		if (index == -1) return;

		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);

			if (index >= mInfo->mPoints.size()) return;

			mBezierUtil->SetSourceColor(index, mInfo->mPoints[index].lock_control);
		}
	}

	void PiecewiseBezierComponent::addRoadIns(ActorComponentPtr value)
	{
		auto pavement = value.getDynamicCastWeakPtr<PavementComponent>();

		auto p = [&](PavementComponentPtr& comPtr, bool* HaveRoad)->void {
			if (!*HaveRoad && pavement->getComName() == comPtr->getComName())
				*HaveRoad |= true;
			else
				*HaveRoad |= false;
		};

		bool flag = false;
		std::for_each(mRoads.begin(), mRoads.end(), std::bind(p, std::placeholders::_1, &flag));

		if (!flag)
			mRoads.push_back(pavement);
	}

	void PiecewiseBezierComponent::clearRoadIns(ActorComponentPtr value)
	{
		if (mRoads.size() == 0) return;

		auto iter = std::find(mRoads.begin(), mRoads.end(), value);

		while (iter != mRoads.end())
		{
			mRoads.erase(iter);
			iter = std::find(mRoads.begin(), mRoads.end(), value);
		}
	}

	void PiecewiseBezierComponent::RoadUpdate(bool sync /*= false*/)
{
		if (mRoads.size() > 0)
		{
			for (auto& road : mRoads)
			{
				if (!Root::instance()->UsePavementTestNewMode)
					road->update();
				else
				{
					road->updateAllPavement(PavementComponent::UpdateRender, sync);
				}
			}
		}

		if (m_arrCurvedCom.size() > 0)
		{
			for (auto& com : m_arrCurvedCom)
				com->update();
		}
	}

	void PiecewiseBezierComponent::RoadPartUpdate(bool sync /*= false*/)
{
		if (mRoads.size() > 0 && getPointCount() > 1)
		{
			for (auto& road : mRoads)
			{
				if (road && road->GetPavementInfo())
				{
					if (road->needUpdateAll())
					{
						if (!Root::instance()->UsePavementTestNewMode)
							road->update();
						else
						{
							road->updateAllPavement(PavementComponent::UpdateRender, sync);
						}
					}
					else
					{
						if (!Root::instance()->UsePavementTestNewMode)
							road->updatePiecewiseRoad(mHelperPickedIndex);
						else
						{
							road->updateIndexPavement(mHelperPickedIndex == 0 ? 0 : (mHelperPickedIndex - 1), PavementComponent::UpdateRender, sync);
							road->updateIndexPavement(mHelperPickedIndex, PavementComponent::UpdateRender, sync);
						}
						if (!(mLineType & PiecewiseTool::Curve) && int(mHelperPickedIndex) - 1 >= 0)
						{
							setActionType(PiecewiseTool::REMOVE_ACTION);
							//road->updatePiecewiseRoad(mHelperPickedIndex - 1);
							if (!Root::instance()->UsePavementTestNewMode)
								road->updatePiecewiseRoad(mHelperPickedIndex - 1);
							else
							{
								road->updateIndexPavement(mHelperPickedIndex == 0 ? 0 : (mHelperPickedIndex - 1), PavementComponent::UpdateRender, sync);
								road->updateIndexPavement(mHelperPickedIndex, PavementComponent::UpdateRender, sync);
							}
						}
					}
				}
			}
		}
		else if (mRoads.size() > 0)
		{
			for (auto& road : mRoads)
			{
				if (!Root::instance()->UsePavementTestNewMode)
					road->update();
				else
				{
					road->updateAllPavement(PavementComponent::UpdateRender, sync);
				}
			}
		}

		if (m_arrCurvedCom.size() > 0)
		{
			for (auto& com : m_arrCurvedCom)
				com->update();
		}
	}

	void PiecewiseBezierComponent::addCurvedComponent(ActorComponentPtr pCom)
	{
		for (auto& com : m_arrCurvedCom)
		{
			if (com->getComName() == pCom->getComName())
			{
				return;
			}
		}
		m_arrCurvedCom.push_back(pCom.getStaticCastWeakPtr<CurvedMeshComponent>());
	}

	bool PiecewiseBezierComponent::deleteCurvedComponent(ActorComponentPtr pCom)
	{
		if (pCom)
		{
			if (!m_arrCurvedCom.empty() && !pCom.expired())
			{
				for (auto iter = m_arrCurvedCom.begin(); iter != m_arrCurvedCom.end(); ++iter)
				{
					if ((*iter)->getComName() == pCom->getComName())
					{
						iter = m_arrCurvedCom.erase(iter);
						return true;
					}
				}
			}
		}
		else
		{
			if (!m_arrCurvedCom.empty())
			{
				for (auto iter = m_arrCurvedCom.begin(); iter != m_arrCurvedCom.end();)
				{
					if ((*iter).expired())
					{
						iter = m_arrCurvedCom.erase(iter);
					}
					else
					{
						++iter;
					}
				}
				return true;
			}
		}
		return false;
	}

	void PiecewiseBezierComponent::ChangeToControlPoint(
		Vector3& pick_point, Vector3& other_point, const Vector3& move_diff)
	{
		pick_point = pick_point + move_diff;
		if (getPoint(getPickHelperIndex())->lock_control)
			other_point = -pick_point;
	}

	Vector3 PiecewiseBezierComponent::onChangedPoint(const Vector3& move_diff)
	{
		size_t IndexInPosition = mHelperPickedIndex;
		PiecewiseBezierPoint* Position = getPoint(IndexInPosition);

		PiecewiseTool::ControlType TypeInPosition = mHelperPickedType;
		PiecewiseBezierPointVec* value = getPoints();
		if (IndexInPosition < getPointCount())
		{
			PiecewiseBezierPoint ptlast = (*value)[IndexInPosition];
			if (TypeInPosition == PiecewiseTool::BoxControl)
				ptlast.point = ptlast.point + move_diff;
			else if (TypeInPosition == PiecewiseTool::DestinationControl)
			{
				ChangeToControlPoint(
					ptlast.destination_control,
					ptlast.source_control,
					move_diff);
			}
			else if (TypeInPosition == PiecewiseTool::SourceControl)
			{
				ChangeToControlPoint(
					ptlast.source_control,
					ptlast.destination_control,
					move_diff);
			}
			ptlast.lock_control = (*value)[IndexInPosition].lock_control;
			//value[IndexInPosition] = ptlast;
			setPoint(IndexInPosition, ptlast);
		}
		return Position->point;
	}

	size_t PiecewiseBezierComponent::getPointCount() const
	{

		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			return mInfo->mPoints.size();
		}
		return 0;
	}

	PiecewiseBezierPointVec* PiecewiseBezierComponent::getPoints() const
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			return &mInfo->mPoints;
		}
		return nullptr;
	}

	PiecewiseBezierComponentInfo* PiecewiseBezierComponent::getInfo()
	{
		if (m_pActComInfo)
		{
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
		}
		return nullptr;
	}

	void PiecewiseBezierComponent::changePoints(const PiecewiseBezierPointVec& points)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mInfo->mPoints = points;

			updateToolPoints();
		}
	}

	void PiecewiseBezierComponent::addPoint(const PiecewiseBezierPoint& point)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mInfo->mPoints.push_back(point);
			mBezierUtil->AddPoint(point.point, point.source_control, point.destination_control, point.normal);
			if (!(mLineType & PiecewiseTool::Curve))
			{
				mBezierUtil->SetControlToLine(mBezierUtil->GetPointCount() - 1);
				mBezierUtil->SetControlToLine(mBezierUtil->GetPointCount() - 2);
			}
			//for (auto& road : mRoads)
			//	road->pushAngle(point.AxisXAngle);
		}
	}

	void PiecewiseBezierComponent::insertNullPoint(size_t index)
	{
		if (!m_pActComInfo)
			return;
		auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
		PiecewiseBezierPoint InsertPoint;
		if (index > 0 && index < mInfo->mPoints.size())
		{			
			float length = getPiecewiseLength(index - 1, false);
			float t = mBezierUtil->LengthsTot(length / 2.f, index - 1);
			
			InsertPoint.point = mBezierUtil->Interpolate((unsigned int)index - 1, t);
			InsertPoint.normal = Vector3::UNIT_Y;

			PiecewiseBezierPoint* ptbef = getPoint(index - 1);
			Vector3 l = InsertPoint.point - (ptbef->destination_control + ptbef->point);
			InsertPoint.source_control = -l / 3.f;
			InsertPoint.destination_control = l / 3.f;

			InsertPoint.destination_control.y = 0.f;
			InsertPoint.source_control.y = 0.f;
			
		}
		else if (index >= mInfo->mPoints.size())
		{
			InsertPoint = mInfo->mPoints.back();
			index = mInfo->mPoints.size();
		}
		else if (index == 0)
		{
			InsertPoint = mInfo->mPoints[0];
		}
		insertPoint(index, InsertPoint);

		NotificationInsertOp(index);
	}

	void PiecewiseBezierComponent::NotificationInsertOp(size_t index)
	{
		//for (auto& road : mRoads)
		//{
		//	if (road && road->GetPavementInfo())
		//	{
		//		road->AddAngleInIndex(index, 0.f);
		//	}
		//}
	}

	void PiecewiseBezierComponent::insertPoint(size_t index, const PiecewiseBezierPoint& point)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if(index < mInfo->mPoints.size())
			{
				auto pos = mInfo->mPoints.begin();
				std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
				index = pos - mInfo->mPoints.begin();

				mInfo->mPoints.insert(pos, point);
			}
			else
			{
				index = mInfo->mPoints.size();
				mInfo->mPoints.push_back(point);
			}
			mBezierUtil->InsertPoint(index, point.point, point.source_control, point.destination_control, point.normal);
			mBezierUtil->SetSourceColor(index, point.lock_control);
			if (!(mLineType & PiecewiseTool::Curve))
				mBezierUtil->SetControlToLine(index);
			//for (auto& road : mRoads)
			//	road->insertAngle(index, point.AxisXAngle);
		}
	}

	void PiecewiseBezierComponent::removePoint(size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			auto pos = mInfo->mPoints.begin();
			std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
			index = pos - mInfo->mPoints.begin();

			mInfo->mPoints.erase(pos);
			mBezierUtil->DeletePoint(index);
			//for (auto& road : mRoads)
			//	road->deleteAngle(index);
		}
	}

	void PiecewiseBezierComponent::setPoint(size_t index, const PiecewiseBezierPoint& point)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			auto pos = mInfo->mPoints.begin();
			std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
			index = pos - mInfo->mPoints.begin();

			mInfo->mPoints[index] = point;
			mBezierUtil->SetPoint(index,
				mInfo->mPoints[index].point,
				mInfo->mPoints[index].source_control,
				mInfo->mPoints[index].destination_control,
				mInfo->mPoints[index].normal);
			if (!(mLineType & PiecewiseTool::Curve))
				mBezierUtil->SetControlToLine(index);

			//for (auto& road : mRoads)
			//	road->setAngle(index, point.AxisXAngle);
		}
	}

	void PiecewiseBezierComponent::setPointsOffset(const std::vector<float>& liffts)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mInfo->setPoints(liffts);
			mBezierUtil->SetPointOffset(liffts);
		}
	}

	void PiecewiseBezierComponent::setBoxPoint(const Vector3& point, size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index == -1)
				index = mHelperPickedIndex;

			auto pos = mInfo->mPoints.begin();
			std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
			index = pos - mInfo->mPoints.begin();

			mInfo->mPoints[index].point = point;
			mBezierUtil->SetPoint(index,
				mInfo->mPoints[index].point,
				mInfo->mPoints[index].source_control,
				mInfo->mPoints[index].destination_control,
				mInfo->mPoints[index].normal);
			if (!(mLineType & PiecewiseTool::Curve))
				mBezierUtil->SetControlToLine(index);
		}
	}

	void PiecewiseBezierComponent::setSourcePoint(const Vector3& point, size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index == -1)
				index = mHelperPickedIndex;

			auto pos = mInfo->mPoints.begin();
			std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
			index = pos - mInfo->mPoints.begin();

			mInfo->mPoints[index].source_control = point;
			mBezierUtil->SetPoint(index,
				mInfo->mPoints[index].point,
				mInfo->mPoints[index].source_control,
				mInfo->mPoints[index].destination_control,
				mInfo->mPoints[index].normal);
			if (!(mLineType & PiecewiseTool::Curve))
				mBezierUtil->SetControlToLine(index);
		}
	}

	void PiecewiseBezierComponent::setDestinationPoint(const Vector3& point, size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index == -1)
				index = mHelperPickedIndex;

			auto pos = mInfo->mPoints.begin();
			std::advance(pos, index < mInfo->mPoints.size() ? index : mInfo->mPoints.size());
			index = pos - mInfo->mPoints.begin();

			mInfo->mPoints[index].destination_control = point;
			mBezierUtil->SetPoint(index,
				mInfo->mPoints[index].point,
				mInfo->mPoints[index].source_control,
				mInfo->mPoints[index].destination_control,
				mInfo->mPoints[index].normal);
			if (!(mLineType & PiecewiseTool::Curve))
				mBezierUtil->SetControlToLine(index);
		}
	}

	void PiecewiseBezierComponent::setPickHelperPoint(Vector3 point, size_t index)
	{
		if (index == -1) index = mHelperPickedIndex;
		switch (mHelperPickedType)
		{
		case PiecewiseTool::CenterNormal:
		case PiecewiseTool::BoxControl:
			setBoxPoint(point, index); break;
		case PiecewiseTool::DestinationControl:
			point = point - getPickHelperPoint(index, PiecewiseTool::BoxControl);
			setDestinationPoint(point, index); break;
		case PiecewiseTool::SourceControl:
			point = point - getPickHelperPoint(index, PiecewiseTool::BoxControl);
			setSourcePoint(point, index); break;
		default: break;
		}
	}

	Vector3 PiecewiseBezierComponent::getPickHelperPoint(size_t index, PiecewiseTool::ControlType control)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index == -1) index = mHelperPickedIndex;
			if (control == PiecewiseTool::NonControl) control = mHelperPickedType;
			Vector3 pos = mInfo->mPoints[index].point;
			switch (control)
			{
			case PiecewiseTool::CenterNormal:
			case PiecewiseTool::BoxControl:
				return mInfo->mPoints[index].point;
			case PiecewiseTool::DestinationControl:
				return mInfo->mPoints[index].destination_control + pos;
			case PiecewiseTool::SourceControl:
				return mInfo->mPoints[index].source_control + pos;
			default:
				return Vector3::ZERO;
			}
		}
		return Vector3::ZERO;
	}

	PiecewiseBezierPoint* PiecewiseBezierComponent::getPoint(size_t index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index < mInfo->mPoints.size())
				return &mInfo->mPoints[index];
			return nullptr;
		}
		return nullptr;
	}

	std::vector<float>& PiecewiseBezierComponent::getLengthVec()
	{
		return mBezierUtil->GetLengths();
	}

	float PiecewiseBezierComponent::getPiecewiseLength(size_t Index, bool changeTer)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			bool adTerrain = false;
			if (changeTer)
				adTerrain = mInfo->AdsorbTerrain;

			if (!adTerrain)
			{
				if (Index < mBezierUtil->GetLengthCount())
					return mBezierUtil->GetPartLength(Index);
			}
			else
				if (Index < mInfo->mPoints.size() - 1)
					return mInfo->mPoints[Index].point.distance(mInfo->mPoints[Index + 1].point);
			return -1.f;
		}
		return -1.f;
	}

	float PiecewiseBezierComponent::getSegmentLengths(bool changeTer, size_t Index)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (Index == -1) Index = mBezierUtil->GetLengthCount();

			bool adTerrain = false;
			if (changeTer)
				adTerrain = mInfo->AdsorbTerrain;

			float lengths = 0.f;
			if (!adTerrain)
				for (size_t i = 0; i < Index; ++i)
					lengths += mBezierUtil->GetPartLength(i);
			else
			{
				//index = mInfo->mPoints.size();
				for (size_t i = 1; i < Index; ++i)
					lengths += mInfo->mPoints[i - 1].point.distance(mInfo->mPoints[i].point);
			}
			return lengths;
		}
		return -1.f;
	}

	bool PiecewiseBezierComponent::pickHelper(const Ray& ray)
	{
		bool result = false;
		size_t tempIndex = 0;
		result = mBezierUtil->PickHelper(ray, tempIndex, mHelperPickedType);
		if (result)
			setPickHelperIndex(tempIndex);
		return result;
	}

#ifdef ECHO_EDITOR


	void* pickEntityDataRequest(void* da)
	{
		String* key = (String*)da;
		auto iter = RoadManager::instance()->mBezierPoints.find(*key);
		if (iter != RoadManager::instance()->mBezierPoints.end())
		{
			std::shared_ptr<BezierData> mBezier = iter->second;
			std::vector<std::vector<Vector3>>& PointData = mBezier->mPoints;
			DBMatrix4 mat = mBezier->m_pBezier->getWorldMatrix();

			mBezier->mOffsetY.clear();
			for (auto& IndexBezier : PointData)
			{
				float MaxHeight = 0.f;
				for (Vector3& point : IndexBezier)
				{
					Echo::Vector3 position = mat * point;
					Ray ray(position, Vector3::UNIT_Y);
					float maxlength = PiecewiseTool::calculateMinTerrainLength(ray, Vector3::ZERO, Matrix3::IDENTITY, true);
					float maxL = (maxlength - position.y > 0 ? maxlength - position.y : 0.f);
					MaxHeight = MaxHeight < maxL ? maxL : MaxHeight;
					if (MaxHeight > 0.f)
						MaxHeight = MaxHeight;
				}
				mBezier->mOffsetY.push_back(MaxHeight);
			}
			mBezier->mOffsetY.push_back(0.f);
		}

		return da;
	}

	void pickEntityDataResponce(void* da)
	{
		String* key = (String*)da;
		auto iter = RoadManager::instance()->mBezierPoints.find(*key);
		if (iter != RoadManager::instance()->mBezierPoints.end())
		{
			std::shared_ptr<BezierData> mBezier = iter->second;
			mBezier->m_pBezier->rebalanceLifft(mBezier->mOffsetY);
			mBezier->m_pBezier->setActionType(PiecewiseTool::REMOVE_ACTION);
			mBezier->m_pBezier->update();
		}

		delete key;
	}

	void PiecewiseBezierComponent::rebalanceLifft()
	{
		stringstream ss;
		ss << this;
		String* name = new String();
		*name = ss.str();
		pickEntityDataRequest((void*)name);
		pickEntityDataResponce((void*)name);

		//RoadManager::instance()->addRegFuncInf(ss.str(), pickEntityDataRequest, pickEntityDataResponce);

		//String* name = new String();
		//*name = ss.str();
		//RoadManager::instance()->addRequest(ss.str(), (void*)name);
	}
#else
	void PiecewiseBezierComponent::rebalanceLifft() {};
#endif // ECHO_EDITOR


	void PiecewiseBezierComponent::rebalanceLifft(std::vector<float>& pLiffts)
	{
		if (m_pActComInfo)
		{
			std::adjacent_difference(pLiffts.begin(), pLiffts.end(),
				pLiffts.begin(), [](float a, float b)-> float {
						return a > b ? a : b;
					});
			setPointsOffset(pLiffts);

		}
	}

	bool PiecewiseBezierComponent::getAdTerrain() 
	{ 
		if (m_pActComInfo)
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->AdsorbTerrain;
		else
			return false;
	}

	bool* PiecewiseBezierComponent::getAdTerrainIns() 
	{ 
		if (m_pActComInfo)
			return &static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->AdsorbTerrain;
		else
			return nullptr;
	}

	bool PiecewiseBezierComponent::getPullTerrain()
	{
		if (m_pActComInfo)
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->PullTerrain;
		else
			return false;
	}

	bool* PiecewiseBezierComponent::getPullTerrainIns()
	{
		if (m_pActComInfo)
			return &static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->PullTerrain;
		else
			return nullptr;
	}

	float* PiecewiseBezierComponent::getXAxisAngleIns(int pointIndex) 
	{ 
		if (m_pActComInfo)
			return &static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->mPoints[pointIndex].AxisXAngle;
		return nullptr;
	}

	float PiecewiseBezierComponent::getRotateAngle(int pointIndex) 
	{ 
		if (m_pActComInfo)
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->mPoints[pointIndex].AxisXAngle;
		return 0.f;
	}

	void PiecewiseBezierComponent::setAxisAlignedBox(const AxisAlignedBox& value) 
	{ 
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mInfo->mixPoint = value.vMin; 
			mInfo->maxPoint = value.vMax; 
		}
		
	}
	Vector3 PiecewiseBezierComponent::getAABBMin() 
	{ 
		if (m_pActComInfo)
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->mixPoint;
		return Vector3::ZERO;
	}

	Vector3 PiecewiseBezierComponent::getAABBMax() 
	{ 
		if (m_pActComInfo)
			return static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo)->maxPoint;
		return Vector3::ZERO;
	}

	void PiecewiseBezierComponent::setLineType(PiecewiseTool::PiecewiseType value)
	{ 
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mLineType = value;
			mInfo->mLineType = mLineType;
		}
	}

	void PiecewiseBezierComponent::insertRoadAngle(size_t index)
	{
		//float v = getRotateAngle((int)index);
		//for (auto& road : mRoads)
		//	road->insertAngle(index, v);
	}

	void PiecewiseBezierComponent::setPickHelperIndex(size_t value)
	{
		mHelperPickedIndex = value;
		mTrackChange = true;
	}

	void PiecewiseBezierComponent::getIndexNormal(size_t value, Vector3& v)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (mInfo->mPoints.size() <= value) return ;
			v = mInfo->mPoints[value].normal;
		}
	}

	Vector4& PiecewiseBezierComponent::getSourceColor()
	{
		return mBezierUtil->m_oSourceColor;
	}

	void PiecewiseBezierComponent::setSourceColor()
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			std::vector<bool> isGray;
			for (auto& point : mInfo->mPoints)
			{
				isGray.push_back(point.lock_control);
			}
			mBezierUtil->SetSourceDefaultColor(isGray);
		}
	}

	std::pair<cJSON*, cJSON*> PiecewiseBezierComponent::readbezierFile(const std::string& filePath, PiecewiseBezierPointVec& pReadData)
	{
		std::pair<cJSON*, cJSON*> result = { nullptr, nullptr };
		if (filePath == "")
			return result;

		Echo::DataStreamPtr pDataStream(Echo::Root::instance()->GetFileDataStream(filePath.c_str(), false, "Bezier"));
		if (!pDataStream.isNull())
		{
			size_t nSize = pDataStream->size();
			char* pData = new char[nSize + 1];
			memset(pData, 0, nSize + 1);
			pDataStream->read(pData, nSize);

			JSNODE hRoot = cJSON_Parse(pData);
			delete[] pData;

			if (0 != hRoot)
			{
				result.first = hRoot;
				pReadData.clear();

				cJSON* _js_array = cJSON_GetObjectItem(hRoot, "BezierCurve");
				if (!_js_array || _js_array->type != cJSON_Array) return result;
				int count = cJSON_GetArraySize(_js_array);
				pReadData.reserve(count);

				if (_js_array->child)
					for (cJSON* _child = _js_array->child; _child != nullptr; _child = _child->next)
					{
						PiecewiseBezierComponentInfo::BezierInfoPoint data;
						ReadJSNode(_child, "control_point", data.point);
						ReadJSNode(_child, "source_control", data.source_control);
						ReadJSNode(_child, "destination_control", data.destination_control);
						ReadJSNode(_child, "lock_control", data.lock_control);
						ReadJSNode(_child, "axis_x_angle", data.AxisXAngle);
						ReadJSNode(_child, "center_normal", data.normal);
						if (data.normal == Vector3::ZERO)
							data.normal = Vector3::UNIT_Y;
						//data.ImportJson(_child);
						pReadData.push_back(data);
						if (pReadData.size() == count - 1) break;
					}
				result.second = _js_array;
				return result;
			}
		}

		return result;
	}

	bool PiecewiseBezierComponent::saveBezierRoadFile(const std::string& filePath)
	{
		if (filePath == "")
			return false;

		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);

			cJSON* pRoot = cJSON_CreateObject();
			cJSON* _js_point_array = cJSON_CreateArray();
			cJSON_AddItemToObject(pRoot, "BezierCurve", _js_point_array);
			for (const PiecewiseBezierPoint& pt : mInfo->mPoints)
			{
				cJSON* _js_point = cJSON_CreateObject();
				cJSON_AddItemToArray(_js_point_array, _js_point);

				WriteJSNode(_js_point, "control_point", pt.point);
				WriteJSNode(_js_point, "source_control", pt.source_control);
				WriteJSNode(_js_point, "destination_control", pt.destination_control);
				WriteJSNode(_js_point, "lock_control", pt.lock_control);
				WriteJSNode(_js_point, "axis_x_angle", pt.AxisXAngle);
				WriteJSNode(_js_point, "center_normal", pt.normal);
			}

			cJSON* _js_other = cJSON_CreateObject();
			cJSON_AddItemToArray(_js_point_array, _js_other);
			cJSON_AddBoolToObject(_js_other, "AdsorbTerrain", mInfo->AdsorbTerrain);
			cJSON_AddBoolToObject(_js_other, "PullTerrain", mInfo->PullTerrain);
			cJSON_AddNumberToObject(_js_other, "LineType", mInfo->mLineType);
			WriteJSNode(_js_other, "AABBmin", mInfo->mixPoint);
			WriteJSNode(_js_other, "AABBman", mInfo->maxPoint);

			//cJSON* _js_Road = cJSON_CreateArray();
			//cJSON_AddItemToObject(pRoot, "RoadModel", _js_Road);
			for (auto& road : mRoads)
				road->saveRoadFile(pRoot);

			char* out = cJSON_Print(pRoot);
			cJSON_Delete(pRoot);

			DataStreamPtr pDataStream(Echo::Root::instance()->CreateDataStream(filePath.data(), "Bezier"));
			if (pDataStream.isNull())
			{
				free(out);
				return false;
			}
			pDataStream->writeData(out, strlen(out));
			pDataStream->close();
			free(out);

			return true;
		}

		return false;
	}

	bool PiecewiseBezierComponent::loadBezierFile(const std::string& filePath)
	{
		bool loadSuccess = false;
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);

			std::pair<cJSON*, cJSON*> RootFile = readbezierFile(filePath, mInfo->mPoints);
			cJSON* _js_array = RootFile.second;

			if (_js_array != nullptr)
			{
				int count = cJSON_GetArraySize(_js_array);
				cJSON* _js_data = cJSON_GetArrayItem(_js_array, count - 1);
				ReadJSNode(_js_data, "AdsorbTerrain", mInfo->AdsorbTerrain);
				ReadJSNode(_js_data, "PullTerrain", mInfo->PullTerrain);
				ReadJSNode(_js_data, "LineType", mInfo->mLineType);
				ReadJSNode(_js_data, "AABBmin", mInfo->mixPoint);
				ReadJSNode(_js_data, "AABBman", mInfo->maxPoint);

				for (size_t i = 0; i < mInfo->mPoints.size(); ++i)
					insertRoadAngle(i);

				setAdTerrain(false);
				mLineType = PiecewiseTool::PiecewiseType(mInfo->mLineType);
				updateToolPoints();
				update();
				RoadUpdate(true);
				loadSuccess = true;
			}
			if (RootFile.first)
				cJSON_Delete(RootFile.first);
		}
		return loadSuccess;
	}

	void swapControl(PiecewiseBezierPointVec& vec)
	{
		for (auto& vPoint : vec)
		{
			Vector3 temp = vPoint.destination_control;
			vPoint.destination_control = vPoint.source_control;
			vPoint.source_control = temp;
		}
	}

	bool PiecewiseBezierComponent::connectOtherBezier(const std::string& pPath1, const std::string& pPath2
		, const Vector3& pOffset, const Vector3& pRotation, bool one, bool second, bool isRetention)
	{
		bool loadSuccess = false;
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			mInfo->mPoints.clear();
			PiecewiseBezierPointVec vPoints1, vPoints2;
			cJSON *_js_array1, *_js_array2;
			int key1 = 0, key2 = 0;
			std::pair<cJSON*, cJSON*> RootFile1 = readbezierFile(pPath1, vPoints1);
			std::pair<cJSON*, cJSON*> RootFile2 = readbezierFile(pPath2, vPoints2);
			_js_array1 = RootFile1.second;
			_js_array2 = RootFile2.second;
			if (!(vPoints1.empty() && vPoints2.empty()))
			{
				Vector3 endPos = Vector3::ZERO;
				if (!one)
				{
					mInfo->mPoints.insert(mInfo->mPoints.end(), vPoints1.begin(), vPoints1.end());
					if (!vPoints1.empty())
						endPos = vPoints1[vPoints1.size() - 1].point;
				}
				else
				{
					if (isRetention)	swapControl(vPoints1);
					mInfo->mPoints.insert(mInfo->mPoints.end(), vPoints1.rbegin(), vPoints1.rend());
					if (!vPoints1.empty())
						endPos = vPoints1[0].point;
				}

				if (_js_array1)
				{
					int count = cJSON_GetArraySize(_js_array1);
					cJSON* _js_data = cJSON_GetArrayItem(_js_array1, count - 1);
					ReadJSNode(_js_data, "LineType", key1);
					ReadJSNode(_js_data, "AABBmin", mInfo->mixPoint);
					ReadJSNode(_js_data, "AABBman", mInfo->maxPoint);
				}

				AxisAlignedBox CollectionBox;
				CollectionBox.merge(mInfo->mixPoint);
				CollectionBox.merge(mInfo->maxPoint);

				Quaternion qu;
				qu.FromEulerAnglesZXY(pRotation.z, pRotation.x, pRotation.y);
				Matrix4 mat;
				mat.makeTransform(endPos + pOffset, Vector3::UNIT_SCALE, qu);
				for (auto& ipos : vPoints2)
					ipos.point = mat * ipos.point;

				if(second)
					mInfo->mPoints.insert(mInfo->mPoints.end(), vPoints2.begin(), vPoints2.end());
				else
				{
					if (isRetention)	swapControl(vPoints2);
					mInfo->mPoints.insert(mInfo->mPoints.end(), vPoints2.rbegin(), vPoints2.rend());
				}

				if (_js_array2)
				{
					int count = cJSON_GetArraySize(_js_array2);
					cJSON* _js_data = cJSON_GetArrayItem(_js_array2, count - 1);
					ReadJSNode(_js_data, "LineType", key2);
					ReadJSNode(_js_data, "AABBmin", mInfo->mixPoint);
					ReadJSNode(_js_data, "AABBman", mInfo->maxPoint);
				}

				if ((key1 || key2) && !(key1 ^ key2))
				{
					mInfo->mLineType = key1;
					for (size_t i = 0; i < mInfo->mPoints.size(); ++i)
						insertRoadAngle(i);

					setAdTerrain(false);
					mLineType = PiecewiseTool::PiecewiseType(mInfo->mLineType);
					updateToolPoints();
					update();
					RoadUpdate(true);
					loadSuccess = true;
				}
			}

			if (RootFile1.first)
				cJSON_Delete(RootFile1.first);
			if (RootFile2.first)
				cJSON_Delete(RootFile2.first);
		}

		return loadSuccess;
	}

	int PiecewiseBezierComponent::isConnectOtherBezier(Vector3& inout_WorldPosition, Vector3& out_ControlPoint)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (mInfo->mPoints.empty()) return -1;

			size_t point_size = mInfo->mPoints.size() - 1;
			auto& point0 = mInfo->mPoints[0];
			auto& point1 = mInfo->mPoints[point_size];

			Matrix3 mat;
			this->getWorldMatrix().extract3x3Matrix(mat);
			Vector3 vec0 = point0.point + mat * this->getWorldPosition();
			Vector3 vec1 = point1.point + mat * this->getWorldPosition();

			float dis0 = inout_WorldPosition.distance(mat * vec0);
			float dis1 = inout_WorldPosition.distance(mat * vec1);

			if (dis0 > MAX_CONNECT_DIS && dis1 > MAX_CONNECT_DIS) return -1;
			if (dis0 < 1e-4 || dis1 < 1e-4) return -1;

			int index = (dis0 > dis1 ? (int)point_size : 0);
			float dis = (dis0 > dis1 ? dis1 : dis0);

			if (dis <= MAX_CONNECT_DIS)
			{
				inout_WorldPosition = mInfo->mPoints[index].point;
				if (index == 0)
				{
					float len = mInfo->mPoints[index].destination_control.length();
					out_ControlPoint = -mInfo->mPoints[index].destination_control / len;
				}
				else if (index == point_size)
				{
					float len = mInfo->mPoints[index].source_control.length();
					out_ControlPoint = -mInfo->mPoints[index].source_control / len;
				}
				return index;
			}
		}

		return -1;
	}

	void PiecewiseBezierComponent::AddBezierControlPoint(size_t index, float t)
	{
		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (!mInfo->mPoints.empty() && index < mInfo->mPoints.size() - 1)
			{
				//PiecewiseBezierUtil::BezierControlPoint PiecewiseBezierUtil::CalculateSplittingBezierCurvesFormT

				PiecewiseBezierComponentInfo::BezierInfoPoint newPoint1;
				PiecewiseBezierComponentInfo::BezierInfoPoint newPoint2;
				PiecewiseBezierComponentInfo::BezierInfoPoint newPoint3;
				const PiecewiseBezierComponentInfo::BezierInfoPoint& point1 = mInfo->mPoints[index];
				const PiecewiseBezierComponentInfo::BezierInfoPoint& point2 = mInfo->mPoints[index + 1];

				PiecewiseBezierUtil::BezierControlPoint CotrolPoint1;
				PiecewiseBezierUtil::BezierControlPoint CotrolPoint2;

				CotrolPoint1.point = point1.point;
				CotrolPoint1.source_control = point1.source_control;
				CotrolPoint1.destination_control = point1.destination_control;
				CotrolPoint2.point = point2.point;
				CotrolPoint2.source_control = point2.source_control;
				CotrolPoint2.destination_control = point2.destination_control;

				PiecewiseBezierUtil::BezierControlPoint newCotrolPoint = PiecewiseBezierUtil::CalculateSplittingBezierCurvesFormT(CotrolPoint1, CotrolPoint2, t);

				newPoint1.point = CotrolPoint1.point;
				newPoint1.source_control = CotrolPoint1.source_control;
				newPoint1.destination_control = CotrolPoint1.destination_control;
				newPoint1.lock_control = false;
				newPoint1.normal = point1.normal;
				newPoint1.FitGorundLevel = point1.FitGorundLevel;
				newPoint1.AxisXAngle = point1.AxisXAngle;

				newPoint2.point = newCotrolPoint.point;
				newPoint2.source_control = newCotrolPoint.source_control;
				newPoint2.destination_control = newCotrolPoint.destination_control;
				newPoint2.lock_control = false;
				newPoint2.normal = Vector3::lerp(point1.normal, point2.normal, t);
				newPoint2.FitGorundLevel = point1.FitGorundLevel + (point2.FitGorundLevel - point1.FitGorundLevel) / 2;
				newPoint2.AxisXAngle = Math::lerp(point1.AxisXAngle, point2.AxisXAngle, t);

				newPoint3.point = CotrolPoint2.point;
				newPoint3.source_control = CotrolPoint2.source_control;
				newPoint3.destination_control = CotrolPoint2.destination_control;
				newPoint3.lock_control = false;
				newPoint3.normal = point2.normal;
				newPoint3.FitGorundLevel = point2.FitGorundLevel;
				newPoint3.AxisXAngle = point2.AxisXAngle;

				setPoint(index, newPoint1);
				setPoint(index + 1, newPoint3);

				insertPoint(index + 1, newPoint2);
			}
		}
	}

	std::pair<Echo::Vector3, Echo::Matrix3> PiecewiseBezierComponent::getBezierVector3AndMatrix3FromT(size_t index, const Transform& transform, float t, bool isFitGround,int CalculateType /*= 2*/,PavementFitTerrain* terrain /*= nullptr*/)
{
		std::pair<Vector3, Matrix3> data;

		if (m_pActComInfo)
		{
			PiecewiseBezierComponentInfo* mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (mInfo->mPoints.size() == 0 || index >= mInfo->mPoints.size() - 1)
				return data;
			const PiecewiseBezierComponentInfo::BezierInfoPoint& point1 = mInfo->mPoints[index];
			const PiecewiseBezierComponentInfo::BezierInfoPoint& point2 = mInfo->mPoints[index + 1];
			Vector3 p1 = point1.point;
			Vector3 p2 = point2.point;
			if(isFitGround && terrain)
			{
				if (point1.FitGorundLevel > -1)
				{
					PavementFitTerrain::TerrainData data;
					terrain->getTerrainData(convertLocalToWorldPosition(p1), data);
					p1 = data.terrainPos + data.terrainNormal * (point1.FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
					p1 = convertWorldToLocalPosition(p1);
				}
				if (point2.FitGorundLevel > -1)
				{
					PavementFitTerrain::TerrainData data;
					terrain->getTerrainData(convertLocalToWorldPosition(p2), data);
					p2 = data.terrainPos + data.terrainNormal * (point2.FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
					p2 = convertWorldToLocalPosition(p2);
				}
			}
			PavementComponent::PavementFitPlanetTerrain temp(weak_from_this());
			data = PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromT(p1, point1.destination_control, point2.source_control, p2, t,
				point1.normal, point2.normal, transform, CalculateType,
				isFitGround ? weak_from_this() : nullptr, point1.FitGorundLevel, point2.FitGorundLevel, terrain == nullptr ? &temp : terrain);
		}

		return data;
	}

	std::pair<Echo::Vector3, Echo::Matrix3> PiecewiseBezierComponent::getBezierVector3AndMatrix3FromLength(size_t index, const Transform& transform, float length, bool isFitGround,int CalculateType /*= 2*/, PavementFitTerrain* terrain /*= nullptr*/)
{
		std::pair<Vector3, Matrix3> data;

		if (m_pActComInfo)
		{
			PiecewiseBezierComponentInfo* mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (mInfo->mPoints.size() == 0 || index >= mInfo->mPoints.size() - 1)
				return data;
			const PiecewiseBezierComponentInfo::BezierInfoPoint& point1 = mInfo->mPoints[index];
			const PiecewiseBezierComponentInfo::BezierInfoPoint& point2 = mInfo->mPoints[index + 1];
			Vector3 p1 = point1.point;
			Vector3 p2 = point2.point;
			if (isFitGround && terrain)
			{
				if (point1.FitGorundLevel > -1)
				{
					PavementFitTerrain::TerrainData data;
					terrain->getTerrainData(convertLocalToWorldPosition(p1), data);
					p1 = data.terrainPos + data.terrainNormal * (point1.FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
					p1 = convertWorldToLocalPosition(p1);
				}
				if (point2.FitGorundLevel > -1)
				{
					PavementFitTerrain::TerrainData data;
					terrain->getTerrainData(convertLocalToWorldPosition(p2), data);
					p2 = data.terrainPos + data.terrainNormal * (point2.FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
					p2 = convertWorldToLocalPosition(p2);
				}
			}
			PavementComponent::PavementFitPlanetTerrain temp(weak_from_this());
			data = PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(p1, point1.destination_control, point2.source_control, p2, length,
				point1.normal, point2.normal, transform, CalculateType,
				isFitGround ? weak_from_this() : nullptr, point1.FitGorundLevel, point2.FitGorundLevel, terrain == nullptr ? &temp : terrain);
		}

		return data;
	}

#ifdef ECHO_EDITOR
	BezierFallbackAction::OneAction PiecewiseBezierComponent::RecordThisOperation(BezierOperation action, size_t index)
	{
		BezierFallbackAction::OneAction fallback;
		if (index == -1) index = mHelperPickedIndex;
		if (index == -1) return fallback;

		if (m_pActComInfo)
		{
			auto mInfo = static_cast<PiecewiseBezierComponentInfo*>(m_pActComInfo);
			if (index >= mInfo->mPoints.size()) return fallback;

			fallback = m_pFallbackAction.Recode(action, index, mInfo->mPoints[index]);
		}
		return fallback;
	}
	void PiecewiseBezierComponent::UndoCommand(const BezierFallbackAction::OneAction& pRecodeAction)
	{
		// 撤销操作
		if (pRecodeAction.mAction == NullAction) return;
		setPickHelperType(PiecewiseTool::BoxControl);
		setPickHelperIndex(pRecodeAction.mPiecewiseIndex);
		switch (pRecodeAction.mAction)
		{
		case RemoveControlPoint:
		{
			setActionType(PiecewiseTool::REMOVE_ACTION);
			setPoint(pRecodeAction.mPiecewiseIndex, pRecodeAction.mOldPosData);
			updatePart();
		}	break;
		case DeleteControlPoint:
		{
			setActionType(PiecewiseTool::INSERT_ACTION);
			insertPoint(pRecodeAction.mPiecewiseIndex, pRecodeAction.mNewPosData);
			update();
		}	break;
		case CreateControlPoint:
		{
			setActionType(PiecewiseTool::DELETE_ACTION);
			removePoint(pRecodeAction.mPiecewiseIndex);
			updatePart();
		}	break;
		default:
			break;
		}
	}

	void PiecewiseBezierComponent::RedoCommand(const BezierFallbackAction::OneAction& pRecodeAction)
	{
		if (pRecodeAction.mAction == NullAction) return;

		setPickHelperType(PiecewiseTool::BoxControl);
		setPickHelperIndex(pRecodeAction.mPiecewiseIndex);
		switch (pRecodeAction.mAction)
		{
		case RemoveControlPoint:
		{
			setActionType(PiecewiseTool::REMOVE_ACTION);
			setPoint(pRecodeAction.mPiecewiseIndex, pRecodeAction.mNewPosData);
			updatePart();
		}	break;
		case DeleteControlPoint:
		{
			setActionType(PiecewiseTool::DELETE_ACTION);
			removePoint(pRecodeAction.mPiecewiseIndex);
			updatePart();
		}	break;
		case CreateControlPoint:
		{
			setActionType(PiecewiseTool::INSERT_ACTION);
			insertPoint(pRecodeAction.mPiecewiseIndex, pRecodeAction.mNewPosData);
			update();
		}	break;
		default:
			break;
		}
	}

#else
	BezierFallbackAction::OneAction PiecewiseBezierComponent::RecordThisOperation(BezierOperation action, size_t index) 
			{ return BezierFallbackAction::OneAction(); };
	void PiecewiseBezierComponent::UndoCommand(const BezierFallbackAction::OneAction& pRecodeAction) {};
	void PiecewiseBezierComponent::RedoCommand(const BezierFallbackAction::OneAction& pRecodeAction) {};
#endif

	/*void PiecewiseBezierComponent::loadBezierFile(const std::string& filePath)
	{

	}*/

	///////////////////////////////////////////////////////////////////////////////
	// PiecewiseBezierComProFunc
	//void PiecewiseBezierComProFunc::initProtocolFunction()
	//{
	//	//auto p = ProtocolComponentManager::instance();
	//	//p->registerFactory(Name("PiecewiseBezierComponent"), new PiecewiseBezierComFactory());
	//	//REGISTEROMFACTORY(PiecewiseBezierComponent);
	//	
	//	//p->registerProtocolFunc(PFID_Trajectory_Show, &EchoTrajectoryProFuncImp::ProCom_ShowTrajectory);
	//}

	//void PiecewiseBezierComProFunc::uninitProtocolFunction()
	//{
	//}

	// static ProtocolResult ProCom_ShowTrajectory(void* isShow, ProtocolHandle handle);
	///////////////////////////////////////////////////////////////////////////////


}
