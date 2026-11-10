///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedRoleComponent.cpp
// @author leixu	
// @date 2024/2/21
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoCurvedRoleComponent.h"

namespace Echo
{
	EDITOR_REFLECT_BEGIN(CurvedRoleComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(FileName, "FileName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(box, "Box", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mNormalSpeed, "NormalSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mDisturbedSpeed, "DisturbedSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mEscapeDistance, "EscapeDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(roleNames, "RoleNames", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(normalAnimations, "NormalAnimations", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(escapeAnimations, "EscapeAnimations", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(skins, "Skins", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(animationSpeeds, "AnimationSpeeds", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(animationLoops, "AnimationLoops", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(startDistances, "StartDistances", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(normalAnimationTimes, "NormalAnimationTimes", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(escapeAnimationTimes, "EscapeAnimationTimes", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(renderDistances, "RenderDistances", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(animationPlayTimes, "AnimationPlayTimes", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(roleScales, "RoleScales", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(roleLODDistances, "RoleLODDistances", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mVecPoints, "Pointss", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	CurvedRoleComponentInfo::CurvedRoleComponentInfo(const Name& type, ActorCOMMoveableType moveable) : ActorComponentInfo(type, moveable)
	{

	}

	void CurvedRoleComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CurvedRoleComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CurvedRoleComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CurvedRoleComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void CurvedRoleComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		CurvedRoleComponentInfo* pInfo = static_cast<CurvedRoleComponentInfo*> (info);
		reflect::TypeResolver<CurvedRoleComponentInfo>::get()->copyTo(this, pInfo);
	}

	void CurvedRoleComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			CurvedRoleComponentInfo* m_pInfo = static_cast<CurvedRoleComponentInfo*>(mOriginComInfo);
			reflect::TypeResolver<CurvedRoleComponentInfo>::get()->overWrite(m_pInfo, this);
		}
	}

	void CurvedRoleComponentInfo::clear()
	{
		box = AxisAlignedBox::BOX_NULL;
		mNormalSpeed = 0.5f;
		mDisturbedSpeed = 1.0f;
		mEscapeDistance = 10.0f;

		roleNames.clear();
		normalAnimations.clear();
		escapeAnimations.clear();
		skins.clear();
		animationSpeeds.clear();
		animationLoops.clear();
		startDistances.clear();
		normalAnimationTimes.clear();
		escapeAnimationTimes.clear();
		animationPlayTimes.clear();
		renderDistances.clear();
		roleScales.clear();
		roleLODDistances.clear();

		mVecPoints.clear();
	}


	void CurvedRoleComponentInfo::addRoleInfo(String roleName, RoleInfo* roleInfo)
	{
		if (roleName.empty())
			return;

		RoleInfo info;
		if (roleInfo != nullptr)
		{
			info = (*roleInfo);
		}

		if (std::find(roleNames.begin(), roleNames.end(), roleName) != roleNames.end())
			return;

		roleNames.emplace_back(roleName);
		normalAnimations.emplace_back(info.mNormalAnimationName);
		escapeAnimations.emplace_back(info.mEscapeAnimationName);
		skins.emplace_back(info.mSkin);
		animationSpeeds.emplace_back(info.mAnimationSpeed);
		if (info.mAnimationLoop)
		{
			animationLoops.emplace_back(String("true"));
		}
		else
		{
			animationLoops.emplace_back(String("false"));

		}
		startDistances.emplace_back(info.mStartDistance);
		normalAnimationTimes.emplace_back(info.mNormalAnimationTime);
		escapeAnimationTimes.emplace_back(info.mEscapeAnimationTime);
		renderDistances.emplace_back(info.mRenderDistance);
		animationPlayTimes.emplace_back(info.mPlayTimes);
		roleScales.emplace_back(info.mScale);
		roleLODDistances.emplace_back(Vector3(info.mLodDistance[0],
			info.mLodDistance[1], info.mLodDistance[2]));
	}

	void CurvedRoleComponentInfo::removeRoleInfo(const String& roleName)
	{
		if (roleName.empty() || roleNames.empty())
			return;

		if (std::find(roleNames.begin(), roleNames.end(), roleName) == roleNames.end())
			return;
		size_t tag = std::find(roleNames.begin(), roleNames.end(), roleName) - roleNames.begin();

		roleNames.erase(roleNames.begin() + tag);
		normalAnimations.erase(normalAnimations.begin() + tag);
		escapeAnimations.erase(escapeAnimations.begin() + tag);
		skins.erase(skins.begin() + tag);
		animationSpeeds.erase(animationSpeeds.begin() + tag);
		animationLoops.erase(animationLoops.begin() + tag);
		startDistances.erase(startDistances.begin() + tag);
		normalAnimationTimes.erase(normalAnimationTimes.begin() + tag);
		escapeAnimationTimes.erase(escapeAnimationTimes.begin() + tag);
		renderDistances.erase(renderDistances.begin() + tag);
		animationPlayTimes.erase(animationPlayTimes.begin() + tag);
		roleScales.erase(roleScales.begin() + tag);
		roleLODDistances.erase(roleLODDistances.begin() + tag);
	}

	size_t CurvedRoleComponentInfo::getRoleIndex(const String& roleName)
	{
		if (roleName.empty())
			return -1;

		if (std::find(roleNames.begin(), roleNames.end(), roleName) == roleNames.end())
			return -1;

		return std::find(roleNames.begin(), roleNames.end(), roleName) - roleNames.begin();
	}

	void CurvedRoleComponentInfo::setAnimation(const String& role, const String& animation, bool isNormal)
	{
		size_t index = getRoleIndex(role);
		if (index >= 0)
		{
			if (isNormal)
			{
				normalAnimations[index] = animation;
			}
			else
			{
				escapeAnimations[index] = animation;
			}
		}
	}

	void CurvedRoleComponentInfo::setSkin(const String& roleName, const String& skin)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			skins[index] = skin;
		}
	}
	
	void CurvedRoleComponentInfo::setAnimationSpeed(const String& roleName, const float& speed)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			animationSpeeds[index] = speed;
		}
	}

	void CurvedRoleComponentInfo::setAnimationLoop(const String& roleName, const bool& loop)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			if (loop)
			{
				animationLoops[index] = String("true");
			}
			else
			{
				animationLoops[index] = String("false");
			}
		}
	}
	
	void CurvedRoleComponentInfo::setStartDistance(const String& roleName, const float& distance)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			startDistances[index] = distance;
		}
	}

	void CurvedRoleComponentInfo::setRenderDistance(const String& roleName, const float& renderDistance)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			renderDistances[index] = renderDistance;
		}
	}

	void CurvedRoleComponentInfo::setAnimationPlayTimes(const String& roleName, const int& playTimes)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			animationPlayTimes[index] = playTimes;
		}
	}

	void CurvedRoleComponentInfo::setRoleScale(const String& roleName, const Vector3& scale)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			roleScales[index] = scale;
		}
	}

	void CurvedRoleComponentInfo::setRoleLod(const String& roleName, const Vector3& lod)
	{
		size_t index = getRoleIndex(roleName);
		if (index >= 0)
		{
			roleLODDistances[index] = lod;
		}
	}

	DECLAREACTORCOMTYPE_CPP(CurvedRoleComponent)
		CurvedRoleComponent::CurvedRoleComponent(ActorPtr _actor,
			ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/)
		: NodeActorComponent(_actor, movableType)
	{
		mAnimationList.clear();
		mSkinList.clear();
		mCurvedRoleUtil = new CurvedRoleUtil(getSceneManager());
	
		
		
	}

	CurvedRoleComponent::~CurvedRoleComponent()
	{
		SAFE_DELETE(data);
		if (mCurve)
		{
			CurveManager* pmgr = CurvedObjectManager::instance()->getCurveManager();
			pmgr->destroyCurve(mCurve);
			mCurve = nullptr;
		}
	}


	void CurvedRoleComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;

		CurvedRoleComponentInfo* curved_info = static_cast<CurvedRoleComponentInfo*>(info);
		if (curved_info == nullptr)
			return;

		data = new CurvedRoleData();
		data->mVecPoints.clear();
		data->mPos = curved_info->m_Position;
		data->Scale = curved_info->m_Scale;
		data->mOri = transformEulerAngles(curved_info->m_EulerAngles);
		data->mName = curved_info->FileName;
		data->box = curved_info->box;
		data->mNormalSpeed = curved_info->mNormalSpeed;
		data->mDisturbedSpeed = curved_info->mDisturbedSpeed;
		data->mEscapeDistance = curved_info->mEscapeDistance;

		size_t size = curved_info->roleNames.size();

		data->roleList.clear();
		for (size_t i = 0; i < size; i++)
		{
			RoleInfo role;
			getRoleInfo(role, i);
			data->roleList.emplace_back(role);
		}

		data->mVecPoints = curved_info->mVecPoints;

		mCurvedRole = std::make_shared<CurvedRole>(data, getSceneManager());
		createCurve("curvedRoleCom");
	}

	bool CurvedRoleComponent::loadCurvedRoleFile(const std::string& filePath)
	{
		if (mCurvedRole.get() != nullptr)
		{
			CurvedObjectManager::instance()->unregisterForUpdates(mCurvedRole.get());
			mCurvedRole.reset();
			CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
			if (info != nullptr) info->clear();
			SAFE_DELETE(data);
		}

		mCurvedRole = std::make_shared<CurvedRole>(filePath, getSceneManager());
		if (mCurvedRole != nullptr)
		{
			Vector3 pos = getWorldPosition();
			mCurvedRole->setPosition(pos);

			Vector3 scale = getWorldScale();
			mCurvedRole->setCurveScale(scale);

			Quaternion q = getRotation();
			q.Normalize();
			mCurvedRole->setOrientation(q);
			getDataFromCurvedRole();
		}
		return true;
	}

	Quaternion CurvedRoleComponent::transformEulerAngles(const Vector3& EulerAngles)
	{
		Quaternion quatY, quatP, quatR;
		quatY.FromAngleAxis(Radian((EulerAngles.y /180.0f)* Math::PI), Echo::Vector3::UNIT_Y); // Y轴旋转  
		quatP.FromAngleAxis(Radian((EulerAngles.x /180.0f)* Math::PI), Echo::Vector3::UNIT_X); // X轴旋转  
		quatR.FromAngleAxis(Radian((EulerAngles.z /180.0f)* Math::PI), Echo::Vector3::UNIT_Z); // Z轴旋转  

		// 按照 Y -> X -> Z 的顺序进行四元数乘法  
		Quaternion q = quatY * quatP * quatR;
		q.normalise();
		return q;
	}

	void CurvedRoleComponent::createCurve(const String& name)
	{
		CurveManager* pmgr = CurvedObjectManager::instance()->getCurveManager();
		String curveName = name + "_" + std::to_string(pmgr->getUniqueCurveID());
		mCurve = pmgr->createCurve(curveName);
	}

	void CurvedRoleComponent::getRoleInfo(RoleInfo& role, size_t index)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (info == nullptr)
			return;
		size_t size = info->roleNames.size();
		if (size == 0 || index >= size || index < 0)
			return;

		role.mRoleName = info->roleNames[index];
		role.mNormalAnimationName = info->normalAnimations[index];
		role.mEscapeAnimationName = info->escapeAnimations[index];
		role.mSkin = info->skins[index];
		role.mAnimationSpeed = info->animationSpeeds[index];
		if (info->animationLoops[index] == String("true"))
		{
			role.mAnimationLoop = true;
		}
		else
		{
			role.mAnimationLoop = false;
		}
		role.mStartDistance = info->startDistances[index];
		role.mEscapeAnimationTime = info->escapeAnimationTimes[index];
		role.mNormalAnimationTime = info->normalAnimationTimes[index];
		role.mRenderDistance = info->renderDistances[index];
		role.mPlayTimes = info->animationPlayTimes[index];
		role.mScale = info->roleScales[index];
		role.mLodDistance[0] = info->roleLODDistances[index].x;
		role.mLodDistance[1] = info->roleLODDistances[index].y;
		role.mLodDistance[2] = info->roleLODDistances[index].z;
	}

	void CurvedRoleComponent::getDataFromCurvedRole()
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() == nullptr || info == nullptr)
			return;

		CurvedRoleData* data = mCurvedRole->getData();

		if (data == nullptr)
			return;

		if (!info->roleNames.empty() && !info->mVecPoints.empty())
			return;

		info->clear();
		info->mVecPoints = data->mVecPoints;

		for (RoleInfo& role : data->roleList)
			info->addRoleInfo(role.mRoleName, &role);

		info->box = data->box;
		info->mNormalSpeed = data->mNormalSpeed;
		info->mDisturbedSpeed = data->mDisturbedSpeed;
		info->mEscapeDistance = data->mEscapeDistance;
		info->m_Position = data->mPos;
		info->m_Scale = data->Scale;
		info->m_EulerAngles = data->mOri.ToEuler();
		
		for (RoleInfo role : data->roleList)
			info->addRoleInfo(role.mRoleName, &role);

		info->mVecPoints = data->mVecPoints;
	}

	CurvedRoleComponentInfo* CurvedRoleComponent::getInfoData()
	{
		CurvedRoleComponentInfo* info=static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		return info;
	}

	bool CurvedRoleComponent::saveCurvedRoleFile(const std::string& filePath)
	{
		if (filePath == "")
			return false;

		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);

		if (info)
		{
			cJSON* pRoot = cJSON_CreateObject();
			if (pRoot == nullptr)
				return false;
			cJSON* cvrArray = cJSON_CreateArray();
			cJSON_AddItemToObject(pRoot, "Curved Role Object", cvrArray);
			Echo::cJSON* hInfo = Echo::cJSON_CreateObject();
			if (NULL == hInfo)
				return false;

			exportJsonScene(hInfo);
			cJSON_AddItemToArray(cvrArray, hInfo);

			char* out = cJSON_Print(pRoot);
			cJSON_Delete(pRoot);
			Echo::String szFileBuf = filePath;

			FILE* fpFile = fopen(szFileBuf.c_str(), "wb");
			if (NULL == fpFile)
			{
				free(out);
				return false;
			}
			fwrite(out, sizeof(char), strlen(out), fpFile);
			fclose(fpFile);
			free(out);
			return true;
		}

		return false;
	}

	void CurvedRoleComponent::exportJsonScene(cJSON* pParent)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (info == nullptr)
			return;

		WriteJSNode(pParent,"Version", 0);
		WriteJSNode(pParent, "Name", info->FileName);
		Vector3 pos = Vector3::ZERO;
		Vector3 scale = Vector3::UNIT_SCALE;
		Quaternion q = Quaternion::IDENTITY;
		if (mCurvedRole != nullptr)
		{
			pos = mCurvedRole->getPosition();
			scale = mCurvedRole->getCurveScale();
			q = mCurvedRole->getOrientation();
		}
		WriteJSNode(pParent, "Position", pos);
		WriteJSNode(pParent, "scale", scale);
		WriteJSNode(pParent, "Orientation", q);

		WriteJSNode(pParent, "NormalSpeed", info->mNormalSpeed);
		WriteJSNode(pParent, "DisturbedSpeed", info->mDisturbedSpeed);
		WriteJSNode(pParent, "EscapeDistance", info->mEscapeDistance);

		WriteJSNode(pParent, "boxMax", info->box.getMaximum());
		WriteJSNode(pParent, "boxMin", info->box.getMinimum());

		cJSON* roles = cJSON_CreateArray();
		size_t size = info->roleNames.size();
		for (size_t i = 0; i < size; i++)
		{
			cJSON* role = cJSON_CreateObject();
			WriteJSNode(role, "Role", info->roleNames[i]);
			WriteJSNode(role, "NormalAnimation", info->normalAnimations[i]);
			WriteJSNode(role, "EscapeAnimation", info->escapeAnimations[i]);
			WriteJSNode(role, "Skin", info->skins[i]);
			WriteJSNode(role, "Speed", info->animationSpeeds[i]);
			WriteJSNode(role, "Loop", info->animationLoops[i]);
			WriteJSNode(role, "StartPosition", info->startDistances[i]);
			WriteJSNode(role, "EscapeAnimPlayTimes", info->animationPlayTimes[i]);
			WriteJSNode(role, "Scale", info->roleScales[i]);
			WriteJSNode(role, "RenderDistance", info->renderDistances[i]);
			WriteJSNode(role, "LodDistance", info->roleLODDistances[i]);
			cJSON_AddItemToArray(roles, role);
		}
		cJSON_AddItemToObject(pParent, "Roles", roles);

		cJSON* points = cJSON_CreateArray();
		auto it = info->mVecPoints.begin();
		auto end = info->mVecPoints.end();
		for (; it != end; it++)
		{
			float arr[3] = { (*it).x,(*it).y,(*it).z };
			cJSON* point = cJSON_CreateFloatArray(arr, 3);
			cJSON_AddItemToArray(points, point);
		}
		cJSON_AddItemToObject(pParent, "Points", points);
	}

	float CurvedRoleComponent::getCurveLength()
	{
		if (mCurvedRole.get() != nullptr && mCurvedRole->getCurve()!= nullptr)
		{
			return mCurvedRole->getCurve()->getTotalDistance();
		}
		
		return 0.0f;
	}

	void CurvedRoleComponent::setReset()
	{
		if (mCurvedRole.get() != nullptr)
		{
			mCurvedRole->setReset(true);
		}
	}

	void CurvedRoleComponent::updatePoints()
	{
		if (mCurvedRole.get() != nullptr && mCurvedRole->getData() != nullptr)
		{
			mCurvedRole->setPoints(mCurvedRole->getData()->mVecPoints);
			updateHelperAndCurveLine();
			bool isVisible = mCurvedRoleUtil->isVisible();
			mCurvedRoleUtil->setVisible(isVisible);
		}
	}

	void CurvedRoleComponent::getAnimationList(const std::string& roleName,std::vector<std::string>& animations)
	{
		Echo::String role = roleName;
		size_t tailPos = role.find_first_of("#");
		Echo::String removedTail = role.substr(0, tailPos);

		if (mAnimationList.find(removedTail) == mAnimationList.end())
			getRolesInfo();
		if (mAnimationList.find(removedTail) != mAnimationList.end())
		{
			animations = mAnimationList[removedTail];
		}
	}

	void CurvedRoleComponent::getSkinList(const std::string& roleName, std::vector<std::string>& skins)
	{
		getRolesInfo();

		Echo::String role = roleName;
		size_t tailPos = role.find_first_of("#");
		Echo::String removedTail = role.substr(0, tailPos);
		if (mAnimationList.find(removedTail) == mAnimationList.end())
			getRolesInfo();
		if (mAnimationList.find(removedTail) != mAnimationList.end())
		{
			skins = mSkinList[removedTail];
		}
	}

	void CurvedRoleComponent::getRolesInfo()
	{
		if (mCurvedRole.get() != nullptr && mCurvedRole->getData() != nullptr)
		{
			std::vector<RoleInfo>& roleVec = mCurvedRole->getData()->roleList;
			for (size_t i = 0; i < roleVec.size(); i++)
			{
				Echo::String role = roleVec[i].mRoleName;
				size_t tailPos = role.find_first_of("#");
				Echo::String removedTail = role.substr(0, tailPos);
				if (mSkinList.find(removedTail) != mSkinList.end() && mAnimationList.find(removedTail) != mAnimationList.end())
					continue;

				Echo::RolePtr prole = Echo::RoleManager::instance()->load(Echo::Name(removedTail));
				std::set<Name> animationList;
				prole->getAllRoleFileAnim(animationList);
				auto iter = animationList.begin(), iterend = animationList.end();
				mAnimationList[removedTail].clear();
				mAnimationList[removedTail].emplace_back("NoAnimation");
				for (; iter != iterend; ++iter)
				{
					mAnimationList[removedTail].emplace_back(iter->toString());
				}

				std::set<Echo::Name> skinNames;
				prole->getEquipInfoName(skinNames);
				mSkinList[removedTail].clear();
				mSkinList[removedTail].emplace_back("NoSkin");
				for (auto iter = skinNames.begin(); iter != skinNames.end(); ++iter)
				{
					mSkinList[removedTail].emplace_back(iter->toString());
				}
			}
		}
	}

	void CurvedRoleComponent::setAnimation(const String& role, const String& animation, bool isNormal)
	{
		if(animation == String("NoAnimation"))
			return;
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setAnimation(role, animation, isNormal);
			if (isNormal)
			{
				mCurvedRole->setListNormalAnimation(role, animation);
			}
			else
			{
				mCurvedRole->setListEscapeAnimation(role, animation);
			}

		}
	}

	void CurvedRoleComponent::setSkin(const String& role, const String& skin)
	{
		if (skin == String("NoSkin"))
			return;
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setSkin(role, skin);
			mCurvedRole->setSkin(role, skin);
		}
	}

	void CurvedRoleComponent::setAnimationSpeed(const String& role, const float& animationSpeed)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setAnimationSpeed(role, animationSpeed);
			mCurvedRole->setSpeed(role, animationSpeed);
		}
	}

	void CurvedRoleComponent::setPlayTimes(const String& role, const int& playTimes)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setAnimationPlayTimes(role, playTimes);
			mCurvedRole->setPlayTimes(role, playTimes);
		}
	}

	void CurvedRoleComponent::setStartPosition(const String& role, const float& startPos)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setStartDistance(role, startPos);
			mCurvedRole->setStartPos(role, startPos);
		}
	}

	void CurvedRoleComponent::setAnimationLoop(const String& role, const bool& loop)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setAnimationLoop(role, loop);
			mCurvedRole->setLoop(role, loop);
		}
	}

	void CurvedRoleComponent::setRoleScale(const String& role, const Vector3& scale)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setRoleScale(role, scale);
			mCurvedRole->setScale(role, scale);
		}
	}

	void CurvedRoleComponent::setRoleRenderDistance(const String& role, const float& renderDistance)
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setRenderDistance(role, renderDistance);
			mCurvedRole->setRenderDistance(role, renderDistance);
		}
	}

	void CurvedRoleComponent::setRoleLODDistance(const String& role, float lod[3])
	{
		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRole.get() != nullptr && info != nullptr)
		{
			info->setRoleLod(role, Vector3(lod[0], lod[1], lod[2]));
			mCurvedRole->setLodDistance(role,0,lod[0]);
			mCurvedRole->setLodDistance(role,1,lod[1]);
			mCurvedRole->setLodDistance(role,2,lod[2]);
		}
	}

	void CurvedRoleComponent::removeRole(const String& role)
	{
		if (mCurvedRole.get() != nullptr)
		{
			mCurvedRole->removeRole(role);
			if (mCurvedRole->getData())
			{
				mCurvedRole->getData()->removeRole(role);
				CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
				if (info)	info->removeRoleInfo(role);
			}
		}
	}

	void CurvedRoleComponent::addRole(const RoleInfo& role)
	{
		if (mCurvedRole.get() != nullptr)
		{
			mCurvedRole->addRole(role);
			if (mCurvedRole->getData())
			{
				mCurvedRole->getData()->addRole(role);
				CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
				RoleInfo temp = role;
				if (info)	info->addRoleInfo(role.mRoleName, &temp);
			}
		}
	}

	Vector3 CurvedRoleComponent::RestoreToOriginalPoint(const Vector3& pos)
	{
		if (mCurvedRole.get() != nullptr &&
			mCurvedRole->getData() != nullptr &&
			mCurvedRole->getCurve() != nullptr)
		{
			Echo::Quaternion q = mCurvedRole->getOrientation();
			Echo::Matrix3 R;
			q.ToRotationMatrix(R);
			vector<Vector3>::type points;
			mCurvedRole->getPoints(points);
			Vector3 rva_pos;
			return R.Inverse() * ((pos - points[0]) / mCurvedRole->getCurveScale()) + points[0];
		}
		return pos;
	}

	void CurvedRoleComponent::insertNullPoint(size_t index)
	{
		if (mCurvedRole.get() != nullptr && 
			mCurvedRole->getData() != nullptr &&
			mCurvedRole->getCurve() != nullptr)
		{
			auto& points = mCurvedRole->getData()->mVecPoints;
			Curve* curve = mCurvedRole->getCurve();
			Vector3 NullPos = Vector3::ZERO;
			if (index == 0)
			{
				NullPos = curve->interpolate((unsigned int)points.size() - 1, 0.5);
				NullPos = RestoreToOriginalPoint(NullPos);
				points.insert(points.begin()+index, NullPos);
			}
			else if (index == points.size())
			{
				NullPos = curve->interpolate((unsigned int)index - 1, 0.5);
				NullPos = RestoreToOriginalPoint(NullPos);
				points.emplace_back(NullPos);
			}
			else
			{
				NullPos = curve->interpolate((unsigned int)index-1, 0.5);
				NullPos = RestoreToOriginalPoint(NullPos);
				points.insert(points.begin() + index, NullPos);
			}

			CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
			if (info != nullptr)
			{
				info->mVecPoints = points;
			}
			mCurvedRole->setPoints(points);

		}
	}

	void CurvedRoleComponent::deletePoint(size_t index)
	{
		if (mCurvedRole.get() != nullptr &&
			mCurvedRole->getData() != nullptr &&
			mCurvedRole->getCurve() != nullptr)
		{
			auto& points = mCurvedRole->getData()->mVecPoints;
			if (points.size() < 3 || index >= points.size())
				return;
			points.erase(points.begin() + index);

			CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
			if (info != nullptr)
			{
				info->mVecPoints = points;
			}

			mCurvedRole->setPoints(points);
		}
	}

	bool CurvedRoleComponent::pickHelper(const Ray& ray)
	{
		bool result = false;
		size_t tempIndex = 0;
		result = mCurvedRoleUtil->PickHelper(ray, tempIndex);
		if (result)
			setPickHelperIndex(tempIndex);
		return result;
	}

	void CurvedRoleComponent::setPickHelperIndex(size_t value)
	{
		mHelperPickedIndex = value;
	}

	void CurvedRoleComponent::SetVisible(bool isVisible)
	{
		if (mCurvedRoleUtil != nullptr)
		{
			mCurvedRoleUtil->setVisible(isVisible);
		}
	}

	bool CurvedRoleComponent::getVisible()
	{
		if (mCurvedRoleUtil != nullptr)
		{
			return mCurvedRoleUtil->isVisible();
		}
		return false;
	}

	void CurvedRoleComponent::HighlightPoint(const Ray& ray)
	{
		if (mCurvedRoleUtil != nullptr)
		{
			mCurvedRoleUtil->setHighlightPoint(ray);
		}
	}

	size_t CurvedRoleComponent::getPointCount()
	{
		if (mCurvedRole != nullptr && mCurvedRole->getCurve() != nullptr)
		{
			return size_t(mCurvedRole->getCurve()->getNumPoints()-1);
		}
		return 0;
	}


	Vector3 CurvedRoleComponent::getPickHelperPoint(size_t index)
	{
		if (mCurvedRole != nullptr && mCurve != nullptr)
		{
	
			if (index == -1) index = mHelperPickedIndex;
			if(mCurve->getNumPoints()- 1 > index)
			return mCurve->getPoint((uint32)index);
		}
		return Vector3::ZERO;
	}

	void CurvedRoleComponent::update()
	{
		if (mCurvedRoleUtil != nullptr && mCurvedRole != nullptr)
		{
			mCurvedRoleUtil->DefaultHelper();

			if (mAddScene)
			{
				addChildToNode(mCurvedRoleUtil->getSceneNode());
				mAddScene = false;
				UpdateComRootNode(true, true);
			}
			
			std::vector<Vector3> points;
			mCurvedRole->getPoints(points);
			if (points.size() < 3)
				return;
			if (!points.empty())	points.emplace_back(points[0]);

			mCurve->setPoints(points);

			mCurvedRoleUtil->clearHelper();
			if (mCurve != nullptr)
			{
				Echo::uint32 size = mCurve->getNumPoints();
				for (Echo::uint32 i = 0; i < size-1; i++)
				{
					const Echo::Vector3& cpos = mCurve->getPoint(i);
					mCurvedRoleUtil->updateHelper(PiecewiseTool::ActionType::CREATE_ACTION, cpos, Vector3::ZERO, i, true);
				}
			}

			mCurvedRoleUtil->UpdateCurveLine(mCurve);
			
			CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
			if (info) info->box = mCurvedRoleUtil->getBox();
		}
	}

	void CurvedRoleComponent::updateNode()
	{
		if (mCurvedRoleUtil)
		{
			mCurvedRoleUtil->updateNode();
		}
	}

	void CurvedRoleComponent::setPickHelperPoint(Vector3 point, size_t index)
	{
		if (!mCurvedRole || !(mCurvedRole->getData()) || !mCurvedRoleUtil)
			return;

		if (index == -1) index = mHelperPickedIndex;
		mCurvedRoleUtil->setHelperPosition(index, point);
		vector<Vector3>::type points;
		mCurvedRole->getPoints(points);

		if (index > points.size() - 1)
			return;
		Vector3 pos = point;
		if (index == 0)
		{
			points[0] = pos;
		}
		else
		{
			points[index] = pos;
		}
		mCurvedRole->setPoints(points);
		
		points.clear();
		mCurvedRole->getPoints(points);
		if (!points.empty()) points.emplace_back(points[0]);

		mCurve->setPoints(points);
		mCurvedRoleUtil->UpdateCurveLine(mCurve);

		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (info) info->box = mCurvedRoleUtil->getBox();
	}


	void CurvedRoleComponent::updateHelperAndCurveLine()
	{
		if (!mCurvedRoleUtil || !mCurvedRoleUtil->getSceneNode())
			return;

		std::vector<Vector3> points;
		mCurvedRole->getPoints(points);
		if (points.empty())
		{
			return;
		}
		else
		{
			points.emplace_back(points[0]);
		}
		mCurve->setPoints(points);

		if (mCurve != nullptr)
		{
			Echo::uint32 size = mCurve->getNumPoints();
			mCurvedRoleUtil->clearHelper();
			for (Echo::uint32 i = 0; i < size - 1; i++)
			{
				const Echo::Vector3& pos = mCurve->getPoint(i);
				mCurvedRoleUtil->updateHelper(PiecewiseTool::ActionType::CREATE_ACTION, pos, Vector3::ZERO, i, true);
			}
		}
		
		mCurvedRoleUtil->UpdateCurveLine(mCurve);

		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (info) info->box = mCurvedRoleUtil->getBox();
	}

	void CurvedRoleComponent::updateCurvedRoleParams(const Vector3& pos,const Vector3& scale, const Quaternion& q)
	{
		if (mCurvedRole == nullptr)
			return;

		bool isChange = false;
		getDataFromCurvedRole();

		Vector3 cPos = mCurvedRole->getPosition();
		Vector3 cScale = mCurvedRole->getCurveScale();

		if (pos != cPos)
		{
			mCurvedRole->setPosition(pos);
			isChange = true;
		}

		if (scale != cScale)
		{
			mCurvedRole->setCurveScale(scale);
			isChange = true;
		}

		CurvedRoleComponentInfo* info = static_cast<CurvedRoleComponentInfo*>(m_pActComInfo);
		if (mCurvedRoleUtil->getSceneNode() && info)
		{
			mCurvedRole->setComOrientaion(q);
			if (mOri != q)
			{
				mCurvedRole->setComOrientaion(q);
				isChange = true;
			}
			mOri = q;
		}
		if(isChange)
		updateHelperAndCurveLine();


	}

	Vector3 CurvedRoleComponent::getOffset()
	{
		Vector3 offset = Vector3::ZERO;
		if (mCurvedRole == nullptr)
		{
			vector<Vector3>::type points;
			mCurvedRole->getPoints(points);
			if(!points.empty()) offset = points.front();
		}
		return Vector3::ZERO;
	}

	void CurvedRoleComponent::KeepScale(const Vector3& cam_pos)
	{
		if (mCurvedRoleUtil != nullptr)
			mCurvedRoleUtil->keepScale(cam_pos);
	}
}
