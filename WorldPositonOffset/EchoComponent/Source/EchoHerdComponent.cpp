#include "EchoComponentStableHeaders.h"

#include "EchoHerdComponent.h"
#include "EchoCurvedRole.h"
#include "EchoWorldSystem.h"
namespace Echo
{
#ifdef ECHO_EDITOR

	HerdComponentHelper::HerdComponentHelper(HerdComponent* pMaster)
	{
		mMaster = pMaster;
		mDrawLineObj = new HelperObject(Name(pMaster->getCombinedName() + "||LineHelper"), HelperObjType_Box,false,nullptr, pMaster->getSceneManager());
		mDrawPointObj = new HelperObject(Name(pMaster->getCombinedName()+ "||PointHelper"), HelperObjType_Box, false, nullptr, pMaster->getSceneManager());

		WorldNode* pSN = pMaster->getSceneNode();
		if (pSN)
		{
			pSN->addChild(mDrawLineObj->getSceneNode());
			pSN->addChild(mDrawPointObj->getSceneNode());
		}

		mDrawLineObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		mDrawLineObj->setHelperColorType(HelperObjColorType_Red);
		mDrawPointObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		mDrawPointObj->setHelperColorType(HelperObjColorType_Blue);
		UpdatePoints();
		Visible(false);
	}

	HerdComponentHelper::~HerdComponentHelper()
	{
		WorldNode* pSN = mMaster->getSceneNode();
		if (pSN)
		{
			pSN->removeChild(mDrawLineObj->getSceneNode());
			pSN->removeChild(mDrawPointObj->getSceneNode());
		}

		if (mDrawLineObj == nullptr)
			delete mDrawLineObj;
		if (mDrawPointObj == nullptr)
			delete mDrawPointObj;
	}

	void HerdComponentHelper::UpdatePoints()
	{
		std::vector<Vector3> vb;
		std::vector<uint16>  ib;
		std::vector<uint32>  cb;

		// line
		Curve *pCurve = mMaster->GetCurve();
		if (pCurve != nullptr)
		{
			float totalDistance = pCurve->getTotalDistance();
			if (totalDistance > 0.f)
			{
				float stepLength = std::max(0.1f, totalDistance / UINT16_MAX);

				float step = 0.f;
				while (step <= totalDistance)
				{
					Vector3 pos = pCurve->interpolate(step / totalDistance);
					vb.push_back(pos);
					ib.push_back(uint16(vb.size() - 1));
					ib.push_back(uint16(vb.size()));

					step += stepLength;
				}
				ib.pop_back();
				ib.pop_back();
			}
		}
		mDrawLineObj->changeCustomPolygon(vb, ib);
		
		// point
		float hs = mPointHalfSize;
		vb.clear(), ib.clear();
		for (uint32 i = 0; i < mMaster->GetPointCount(); ++i)
		{
			Vector3 pos = mMaster->GetPointPosition(i);
			uint16  begin = (uint16)vb.size();

			vb.push_back({ pos.x + hs, pos.y + hs, pos.z + hs });
			vb.push_back({ pos.x - hs, pos.y + hs, pos.z + hs });
			vb.push_back({ pos.x - hs, pos.y + hs, pos.z - hs });
			vb.push_back({ pos.x + hs, pos.y + hs, pos.z - hs });

			vb.push_back({ pos.x + hs, pos.y - hs, pos.z + hs });
			vb.push_back({ pos.x - hs, pos.y - hs, pos.z + hs });
			vb.push_back({ pos.x - hs, pos.y - hs, pos.z - hs });
			vb.push_back({ pos.x + hs, pos.y - hs, pos.z - hs });

			ib.push_back(begin + 0); ib.push_back(begin + 1); ib.push_back(begin + 1); ib.push_back(begin + 2); ib.push_back(begin + 2); ib.push_back(begin + 3); ib.push_back(begin + 3); ib.push_back(begin + 0);
			ib.push_back(begin + 0); ib.push_back(begin + 4); ib.push_back(begin + 1); ib.push_back(begin + 5); ib.push_back(begin + 2); ib.push_back(begin + 6); ib.push_back(begin + 3); ib.push_back(begin + 7);
			ib.push_back(begin + 4); ib.push_back(begin + 5); ib.push_back(begin + 5); ib.push_back(begin + 6); ib.push_back(begin + 6); ib.push_back(begin + 7); ib.push_back(begin + 7); ib.push_back(begin + 4);

			if (i == mSeletedPointIdx)
			{
				for (int j = 0 ; j < 8 ; ++ j)
					cb.push_back(ColorValue::Red.getAsARGB());
			}
			//else if (mSeletedPointIdx != -1 && i == mSeletedPointIdx+1)
			//{
			//	for (int j = 0; j < 8; ++j)
			//		cb.push_back(ColorValue::Green.getAsARGB());
			//}
			else
			{
				for (int j = 0; j < 8; ++j)
					cb.push_back(ColorValue::Blue.getAsARGB());
			}
		}
		mDrawPointObj->changeCustomPolygon(vb, ib,cb);
	}
	void HerdComponentHelper::SetPointHalfSize(float size)
	{
		mPointHalfSize = size;
		UpdatePoints();
	}
	float HerdComponentHelper::GetPointHalfSize()
	{
		return mPointHalfSize;
	}
	bool HerdComponentHelper::Visible()
	{
		return mVisible;
	}
	void HerdComponentHelper::Visible(bool v)
	{
		mVisible = v;
		mDrawLineObj->setVisible(v);
		mDrawPointObj->setVisible(v);
	}
	bool HerdComponentHelper::SetSeletedPointIndex(uint32 idx)
	{
		mSeletedPointIdx = idx;
		UpdatePoints();
		return idx != -1;
	}
	uint32 HerdComponentHelper::GetSeletedPointIndex()
	{
		return mSeletedPointIdx;
	}
	uint32 HerdComponentHelper::testHit(const Ray& ray)
	{
		float r = mPointHalfSize * 2;
		for (uint32 i = 0; i < mMaster->GetPointCount(); ++i)
		{
			Vector3 pos = mMaster->convertLocalToWorldPosition( mMaster->GetPointPosition(i));
			auto [hit, dis] = Math::intersects(ray, Sphere(pos, r));
			if (hit)
				return i;
		}
		return -1;
	}
#endif
	const HerdNodeStaticData HerdNodeStaticData::DefaultValue;

	// ----------------------------------------------------------------------------------------------------
	DECLAREACTORCOMTYPE_CPP(HerdComponent);
	HerdComponent::HerdComponent(ActorPtr actor, ActorCOMMoveableType movableType)
		: NodeActorComponent(actor,movableType)
	{
		
		mHerdObject = new HerdCurvedObject(getSceneManager());

	}
	HerdComponent::~HerdComponent()
	{
#ifdef ECHO_EDITOR
		SAFE_DELETE(mHelper);
#endif // ECHO_EDITOR

	}

	void HerdComponent::init(ActorComponentInfo* info)
	{
		const auto* pInfo = static_cast<HerdComponentInfo*>(info);

		// 基本属性
		this->SetNormalSpeed(pInfo->mNormalSpeed);
		this->SetEscapeSpeed(pInfo->mEscapeSpeed);
		this->SetEscapeDistance(pInfo->mEscapeRange);
		this->SetRestoreTime(pInfo->mRestoreTime);
		this->SetCloseCurve(pInfo->mCloseCurve);
		// 关键点
		this->ResetPointPosition(pInfo->mPointPosition);
		// 角色节点
		for (uint32 i = 0; i < pInfo->mHerdNodeStaticDataList.size(); ++i)
		{
			this->AddHerdNode(pInfo->mHerdNodeStaticDataList[i]);
		}
		Reset();
	}

	void HerdComponent::replay(ActorComponentInfo* info)
	{
		ClearPoint();
		RemoveAllHerdNode();

		init(info);
	}

	void HerdComponent::copyTo(const ActorComponentPtr& actCom)
	{
		const auto* other = static_cast<const HerdComponent*>(actCom.get());

		// 基本属性
		this->SetNormalSpeed(other->GetNormalSpeed());
		this->SetEscapeSpeed(other->GetEscapeSpeed());
		this->SetEscapeDistance(other->GetEscapeDistance());
		this->SetRestoreTime(other->GetRestoreTime());
		this->SetCloseCurve(other->IsCloseCurve());
		// 关键点
		ClearPoint();
		std::vector<Vector3> points;
		other->GetAllPointPosition(points);
		this->ResetPointPosition(points);

		// 角色节点
		RemoveAllHerdNode();
		for (uint32 i = 0 ; i < other->GetHerdNodeCount() ; ++ i)
		{
			HerdNode* otherNode = other->GetHerdNode(i);
			HerdNodeStaticData otherData;
			{
				otherData.mRoleName = otherNode->GetRoleResName().toString();
				otherData.mSkinName = otherNode->GetRoleSkin().toString();
				otherData.mNormalAnimationName = otherNode->GetNormalAnimation().toString();
				otherData.mEscapeAnimationName = otherNode->GetEscapeAnimation().toString();
				otherData.mRoleScale = otherNode->GetRoleScale().x;
				otherData.mAnimationSpeed = otherNode->GetAnimationSpeed();
				otherData.mRenderDistance = otherNode->GetRenderDistance();
				for (uint8 l = ROLE_LOD_LEVEL0; l < ROLE_LOD_LEVEL_CNT; ++l)
				{
					otherData.mLodDistance[l] = otherNode->GetLodDistance((ENUM_ROLE_LOD_LEVEL)l);
				}
				otherData.mStartPos = otherNode->GetRoleStartPosition();
				otherData.mAnimationLoop = otherNode->GetAnimationLoop();
			}
			this->AddHerdNode(otherData);
		}
		Reset();
	}

	void HerdComponent::bindNode()
	{
		if (mHerdObject == nullptr)
			return;

		WorldNode *compNode = getSceneNode();
		if (compNode != nullptr)
		{
			WorldNode* parnetNode = mHerdObject->GetSceneNode()->getParent();
			if (compNode == parnetNode)
				return;

			if (parnetNode != nullptr)
				parnetNode->removeChild(mHerdObject->GetSceneNode());

			compNode->addChild(mHerdObject->GetSceneNode());
		}
		if (getActorManager()->IsEditorPlay())
		{
			mHerdObject->Play();
		}
#ifdef ECHO_EDITOR
		if (mHelper == nullptr)
			mHelper = new HerdComponentHelper(this);
#endif 
	}

	void HerdComponent::EditorPlayImp()
	{
		mHerdObject->Play();
	}

	void HerdComponent::EditorPauseImp(bool pause)
	{
		if (pause)
			mHerdObject->Pause();
		else
			mHerdObject->Play();
	}

	void HerdComponent::EditorStopImp()
	{
		mHerdObject->Stop();
	}

	void HerdComponent::SetNormalSpeed(float speed)
	{
		mHerdObject->SetNormalSpeed(speed);
	}

	float HerdComponent::GetNormalSpeed() const
	{
		return mHerdObject->GetNormalSpeed();
	}

	void HerdComponent::SetEscapeSpeed(float speed)
	{
		mHerdObject->SetEscapeSpeed(speed);
	}

	float HerdComponent::GetEscapeSpeed()const
	{
		return mHerdObject->GetEscapeSpeed();
	}

	void HerdComponent::SetEscapeDistance(float dis)
	{
		mHerdObject->SetEscapeDistance(dis);
	}

	float HerdComponent::GetEscapeDistance()const
	{
		return mHerdObject->GetEscapeDistance();
	}

	void HerdComponent::SetRestoreTime(float time)
	{
		mHerdObject->SetRestoreTime(time);
	}

	float HerdComponent::GetRestoreTime()const
	{
		return mHerdObject->GetRestoreTime();
	}

	bool HerdComponent::IsCloseCurve()const
	{
		return mHerdObject->IsCloseCurve();
	}

	void HerdComponent::SetCloseCurve(bool close)
	{
		mHerdObject->SetCloseCurve(close);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	uint32 HerdComponent::GetPointCount()const
	{
		return mHerdObject->GetPointCount();
	}

	Vector3 HerdComponent::GetPointPosition(uint32 ponitIndex)const
	{
		return mHerdObject->GetPointPosition(ponitIndex);
	}

	void HerdComponent::SetPointPosition(uint32 ponitIndex, const Vector3& postsion)
	{
		mHerdObject->SetPointPosition(ponitIndex, postsion);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::InsertPoint(uint32 pointIndex, const Vector3& postsion)
	{
		mHerdObject->InsertPoint(pointIndex, postsion);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::InsertPoint(const Vector3& postsion)
	{
		mHerdObject->InsertPoint(postsion);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::ErasePoint(uint32 pointIndex)
	{
		mHerdObject->ErasePoint(pointIndex);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::ErasePoint()
	{
		mHerdObject->ErasePoint();
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::ClearPoint()
	{
		mHerdObject->ClearPoint();
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	void HerdComponent::GetAllPointPosition(std::vector<Vector3>& out)const
	{
		mHerdObject->GetAllPointPosition(out);
	}

	void HerdComponent::ResetPointPosition(const std::vector<Vector3>& in)
	{
		mHerdObject->ResetPointPosition(in);
		mHerdObject->UpdateCurve();
#ifdef ECHO_EDITOR
		if (mHelper)
			mHelper->UpdatePoints();
#endif
	}

	uint32 HerdComponent::GetHerdNodeCount()const
	{
		return	mHerdObject->GetHerdNodeCount();
	}

	HerdNode* HerdComponent::GetHerdNode(uint32 index)const
	{
		return mHerdObject->GetHerdNode(index);
	}

	HerdNode* HerdComponent::AddHerdNode(uint32 index, const HerdNodeStaticData& data)
	{
		HerdNode* newNode = mHerdObject->AddHerdNode(index);

		newNode->SetRole((Name)data.mRoleName);
		newNode->SetRoleSkin((Name)data.mSkinName);
		newNode->SetRoleStartPosition(data.mStartPos);
		newNode->SetRoleScale({ data.mRoleScale, data.mRoleScale, data.mRoleScale });
		newNode->SetRenderDistance(data.mRenderDistance);
		newNode->SetNormalAnimation((Name)data.mNormalAnimationName);
		newNode->SetEscapeAnimation((Name)data.mEscapeAnimationName);
		newNode->SetAnimationSpeed(data.mAnimationSpeed);
		newNode->SetAnimationLoop(data.mAnimationLoop);
		for (uint8 j = ROLE_LOD_LEVEL0; j < ROLE_LOD_LEVEL_CNT; ++j)
		{
			newNode->SetLodDistance((ENUM_ROLE_LOD_LEVEL)j, data.mLodDistance[j]);
		}
		return newNode;
	}

	HerdNode* HerdComponent::AddHerdNode(const HerdNodeStaticData& data)
	{
		HerdNode* newNode = mHerdObject->AddHerdNode();

		newNode->SetRoleName(this->getCombinedName() + "||" + "ROLE");
		newNode->SetRole((Name)data.mRoleName);
		newNode->SetRoleSkin((Name)data.mSkinName);
		newNode->SetRoleStartPosition(data.mStartPos);
		newNode->SetRoleScale({ data.mRoleScale, data.mRoleScale, data.mRoleScale });
		newNode->SetRenderDistance(data.mRenderDistance);
		newNode->SetNormalAnimation((Name)data.mNormalAnimationName);
		newNode->SetEscapeAnimation((Name)data.mEscapeAnimationName);
		newNode->SetAnimationSpeed(data.mAnimationSpeed);
		newNode->SetAnimationLoop(data.mAnimationLoop);
		for (uint8 j = ROLE_LOD_LEVEL0; j < ROLE_LOD_LEVEL_CNT; ++j)
		{
			newNode->SetLodDistance((ENUM_ROLE_LOD_LEVEL)j, data.mLodDistance[j]);
		}
		return newNode;
	}

	void HerdComponent::RemoveHerdNode(uint32 index)
	{
		mHerdObject->RemoveHerdNode(index);
	}

	void HerdComponent::RemoveHerdNode()
	{
		mHerdObject->RemoveHerdNode();
	}

	void HerdComponent::RemoveAllHerdNode()
	{
		mHerdObject->RemoveAllHerdNode();
	}

	void HerdComponent::Reset()
	{
		mHerdObject->Reset();
	}

	Curve* HerdComponent::GetCurve() const
	{
		return mHerdObject->getCurve();
	}
#ifdef ECHO_EDITOR
	void HerdComponent::ShowHelper(bool show)
	{
		ProtocolComponentSystem::instance()->callProtocolFunc(
			Name("PiecewiseEditModeListenerSwitches"), this, Echo::ProtocolHandleUtil::Pack(3, show));

		if (mHelper)
			mHelper->Visible(show);
	}

	bool HerdComponent::ShowHelper()
	{
		if (mHelper)
			return mHelper->Visible();
		return false;
	}
	void HerdComponent::ShowHerdNode(bool show)
	{
		mHerdObject->GetSceneNode()->setVisible(show);
	}
	bool HerdComponent::ShowHerdNode()
	{
		return 	mHerdObject->GetSceneNode()->getVisible();
	}
	void HerdComponent::SetSelectedPoint(uint32 pointIndex)
	{
		if (mHelper) mHelper->SetSeletedPointIndex(pointIndex);
	}
	uint32 HerdComponent::GetSelectedPoint()
	{
		if (mHelper)
			return mHelper->GetSeletedPointIndex();
		return -1;
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_DeletedNode(const HerdNodeStaticDataList&, uint32 delIdx)
	{
		if (delIdx >= mHerdObject->GetHerdNodeCount())
			return;
		RemoveHerdNode(delIdx);
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_CopyNode(const HerdNodeStaticDataList& data, uint32 idx, uint32 num)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)(idx+num) >= data.size())
			return;

		for (uint32 i = 0; i < num; ++i)
		{
			AddHerdNode(idx + i, data[idx + i]);
		}
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetRole(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode *pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetRoleSkin((Name)data[idx].mSkinName);
		pHerdNode->SetRole((Name)data[idx].mRoleName);
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetSkin(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetRoleSkin((Name)data[idx].mSkinName);
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetScale(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetRoleScale({ data[idx].mRoleScale, data[idx].mRoleScale, data[idx].mRoleScale});
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetLodDistance(const HerdNodeStaticDataList& data, uint32 idx, ENUM_ROLE_LOD_LEVEL level)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetLodDistance(level, data[idx].mLodDistance[level]);
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetRndDistance(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetRenderDistance(data[idx].mRenderDistance);
	}
	void HerdComponent::EditorChangeHerdNodeStaticData_SetStartPos(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetRoleStartPosition(data[idx].mStartPos);
	}

	void HerdComponent::EditorChangeHerdNodeStaticData_SetNorAnime(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetNormalAnimation((Name)data[idx].mNormalAnimationName);
	}

	void HerdComponent::EditorChangeHerdNodeStaticData_SetEscAnime(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetEscapeAnimation((Name)data[idx].mEscapeAnimationName);
	}

	void HerdComponent::EditorChangeHerdNodeStaticData_SetAnimeSpeed(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetAnimationSpeed(data[idx].mAnimationSpeed);
	}

	void HerdComponent::EditorChangeHerdNodeStaticData_SetAnimeLoop(const HerdNodeStaticDataList& data, uint32 idx)
	{
		if (idx >= mHerdObject->GetHerdNodeCount() || (size_t)idx >= data.size())
			return;
		HerdNode* pHerdNode = mHerdObject->GetHerdNode(idx);

		pHerdNode->SetAnimationLoop(data[idx].mAnimationLoop);
	}

	void HerdComponent::EditorChangeHerdNodeStaticData_AddNode(const HerdNodeStaticDataList& data)
	{
		if (data.empty())
			return;
		AddHerdNode(data.back());
	}

	
#endif

	EDITOR_REFLECT_BEGIN(HerdComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mNormalSpeed, "NormalSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEscapeSpeed, "EscapeSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEscapeRange, "EscapeRange", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mRestoreTime, "RestoreTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mCloseCurve,  "CloseCurve",  EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mPointPosition, "PointPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mHerdNodeStaticDataList, "mHerdNodeStaticDataList", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	_DEFINEINFO_VIRFUN_BASE(HerdComponentInfo);

	HerdComponentInfo::HerdComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		:ActorComponentInfo(type, moveable)
	{
	}


	void WriteJSNode(JSNODE pRoot, const char* name, const HerdNodeStaticData& data)
	{
		cJSON* rootJSON = cJSON_CreateObject();
		cJSON_AddItemToObject(pRoot, name, rootJSON);

		cJSON_AddStringToObject(rootJSON, "RoleName", data.mRoleName.c_str());
		cJSON_AddStringToObject(rootJSON, "SkinName", data.mSkinName.c_str());
		cJSON_AddStringToObject(rootJSON, "NormalAnimationName", data.mNormalAnimationName.c_str());
		cJSON_AddStringToObject(rootJSON, "EscapeAnimationName", data.mEscapeAnimationName.c_str());
		cJSON_AddNumberToObject(rootJSON, "RoleScale", data.mRoleScale);
		cJSON_AddNumberToObject(rootJSON, "AnimationSpeed", data.mAnimationSpeed);
		cJSON_AddNumberToObject(rootJSON, "RenderDistance", data.mRenderDistance);
		cJSON_AddNumberToObject(rootJSON, "StartPos", data.mStartPos);
		WriteJSNode(rootJSON, "LodDistance", data.mLodDistance, (int)ROLE_LOD_LEVEL_CNT);
		cJSON_AddBoolToObject(rootJSON, "mAnimationLoop", data.mAnimationLoop);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, HerdNodeStaticData& data)
	{
		cJSON* rootJSON = cJSON_GetObjectItem(pRoot, name);
		if (rootJSON == nullptr)
			return false;

		ReadJSNode(rootJSON, "RoleName", data.mRoleName);
		ReadJSNode(rootJSON, "SkinName", data.mSkinName);
		ReadJSNode(rootJSON, "NormalAnimationName", data.mNormalAnimationName);
		ReadJSNode(rootJSON, "EscapeAnimationName", data.mEscapeAnimationName);
		ReadJSNode(rootJSON, "RoleScale", data.mRoleScale);
		ReadJSNode(rootJSON, "AnimationSpeed", data.mAnimationSpeed);
		ReadJSNode(rootJSON, "RenderDistance", data.mRenderDistance);
		ReadJSNode(rootJSON, "StartPos", data.mStartPos);
		ReadJSNode(rootJSON, "LodDistance", data.mLodDistance, (int)ROLE_LOD_LEVEL_CNT);
		ReadJSNode(rootJSON, "mAnimationLoop", data.mAnimationLoop);

		return true;
	}

} // namespace Echo
