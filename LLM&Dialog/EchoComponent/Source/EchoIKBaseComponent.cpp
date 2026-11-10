#include "EchoComponentStableHeaders.h"
#include "EchoIKBaseComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoSkeletonInstance.h"
#include "Role/EchoRoleIKComponent.h"
#include "Actor/EchoActorManager.h"
#include "EchoRoleTagPointPosComponent.h"
#include "IK/EchoFullBodyBipedIK.h"
#include "IK/EchoIK.h"
#include "EchoRole.h"


namespace Echo{

	EDITOR_REFLECT_BEGIN(IKBaseComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RoleCompName, "RoleCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	_DEFINEINFO_VIRFUN_BASE(IKBaseComponentInfo)

	void IKBaseComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) {
		if (!pData) return;
		SetRoleCompName(pData->roleCompName, pComp);
	}

	void IKBaseComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		pData->roleCompName = GetRoleCompName();
	}
	Name IKBaseComponent::gDefaultFullBodyBipedIKName("DefaultFBBIK");
	bool IKBaseComponent::gUseCheckIKValid = false;
	IKBaseComponent::IKBaseComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : ActorComponent(pActor, moveable) { }
	IKBaseComponent::~IKBaseComponent()
	{
		RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
		if (pRoleIKComp != nullptr && !pRoleIKComp->IsEmptyPendingIKOperation())
		{
			pRoleIKComp->RemovePendingIKOperation(m_nComName);
		}
		SAFE_DELETE(m_pCompData);
	}
	void IKBaseComponent::init(ActorComponentInfo* info) {
		if (info) {
			static_cast<IKBaseComponentInfo*>(info)->CopyToCompData(m_pCompData);
			SetCompData(m_pCompData);
		}
	}
	void IKBaseComponent::unInit() {}
	void IKBaseComponent::replay(ActorComponentInfo* info) {
		if (info) {
			static_cast<IKBaseComponentInfo*>(info)->CopyToCompData(m_pCompData);
			SetCompData(m_pCompData);
		}
	}
	void IKBaseComponent::connectCom() {
		//SetCompData(m_pCompData);
	}
	void IKBaseComponent::disconnectCom() {
		DestroyIK();
		//if (m_pRoleObj) {
		//	m_pRoleObj->removeRoleObjectListener(this);
		//}
	}
	void IKBaseComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		actCom.getDynamicCastWeakPtr<IKBaseComponent>()->SetCompData(m_pCompData);
	}

	void IKBaseComponent::visibleStateChanged(bool activeState)
	{

		RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
		if (pRoleIKComp !=nullptr && pRoleIKComp->IsAsyncContext())
		{
			// Save the context and execute it in RoleIKComponent's FixedUpdate
			// after the animation thread has finished executing.
			auto ikOperation = [this, activeState]()
				{
					this->ExecuteIKOperation(activeState);
				};
			pRoleIKComp->AddPendingIKOperation(m_nComName, ikOperation);
			return;
		}

		ExecuteIKOperation(activeState);
	}

	void IKBaseComponent::ExecuteIKOperation(bool activeState)
	{
		if (!activeState)
		{
			mIsCreateIK = DestroyIK();
		}
		else
		{
			if (mIsCreateIK)
			{
				mIsCreateIK = false;
				CreateIK();
			}
		}
	}

	void IKBaseComponent::EditorPlayImp() {
		CreateIK();
	}
	void IKBaseComponent::EditorPauseImp(bool pause) {
		if (pause) {
			Disable();
		}
		else {
			Enable();
		}
	}
	void IKBaseComponent::EditorStopImp() {
		DestroyIK();
	}

	bool IKBaseComponent::OnRoleCreateFinish() {
		if (CreateIK()) {
			const std::set<EchoWeakPtr<IKEventListener>>* pLinsters = EchoIKSystem::instance()->GetIKEventListener(GetRoleComponent());
			if (pLinsters != nullptr) {
				for (auto& iter : *pLinsters) {
					if (iter != nullptr) iter->OnCreateIKAfterAsync(m_pCompData->GetIKInfo()->GetType(), this);
				}
			}
		}
		return IsExistIK();
	}
	void IKBaseComponent::OnRoleDestroy() {
		m_pIKComponent = nullptr;
	}

	bool IKBaseComponent::CreateIK(CreateIKResType* res) {
		if (m_pIKComponent) {
			if (res) *res = CreateIKResType::eIKCRT_IKEXIST;
			return false;//IK已存在
		}
		if (!getRealVisibleState()) {
			if (res) *res = CreateIKResType::eIKCRT_INVISIBLE;
			return false;//visible为false
		}
		RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
		if (!pRoleIKComp) {
			if (res) *res = CreateIKResType::eIKCRT_ROLENOTEXIST;
			return false;//获取Role失败
		}
		if (!pRoleIKComp->isCreateFinish()) {
			if (pRoleIKComp->GetRoleObject()->isCreateFinish()) {
				if (res) *res = CreateIKResType::eIKCRT_ROLENOTSKEL;//Role无骨骼
			}
			else {
				if (res) *res = CreateIKResType::eIKCRT_ROLENOTLOAD;//Role未加载完成，已注册加载回调
			}
			return false;
		}

		if (!IsCanCreate()) {
			if (res) *res = CreateIKResType::eIKCRT_INVALID;
			return false;//未达成创建条件
		}

		PreCreateIK();
		FinalIKInfoBase* pIKInfo = m_pCompData->GetIKInfo();
		Name ikName = Name(getComName());
		if (pIKInfo) {
			pIKInfo->SetName(ikName);
			m_pIKComponent = pRoleIKComp->CreateIKComponent(ikName, pIKInfo, gUseCheckIKValid);
			//这里已经在IsCanCreate里检查过有效性了，所以暂时不开启
		}
		else {
			m_pIKComponent = nullptr;
		}
		if (m_pIKComponent == nullptr) {
			if (res) *res = CreateIKResType::eIKCRT_ERROR;
			return false;//异常创建失败
		}
		PostCreateIK();
		m_pIKComponent->enable();
		const std::set<EchoWeakPtr<IKEventListener>>* pLinsters = EchoIKSystem::instance()->GetIKEventListener(GetRoleComponent());
		if (pLinsters != nullptr) {
			for (auto& iter : *pLinsters) {
				if (iter != nullptr) iter->OnCreateIK(m_pCompData->GetIKInfo()->GetType(), this);
			}
			for (auto& iter : *pLinsters) {
				if (iter != nullptr) iter->OnEnableIK(m_pCompData->GetIKInfo()->GetType(), this);
			}
		}

		if (res) *res = CreateIKResType::eIKCRT_SUCCESS;
		return true;//创建成功
	}
	bool IKBaseComponent::DestroyIK(DestroyIKResType* res) {
		if (!m_pIKComponent) {
			if (res) *res = DestroyIKResType::eIKDRT_IKNOTEXIST;
			return false;//IK不存在
		}
		RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
		if (!pRoleIKComp) {
			m_pIKComponent = nullptr;
			if (res) *res = DestroyIKResType::eIKDRT_ROLENOTEXIST;
			return false;//Role已被删除
		}
		if (!IsCanDestroy()) {
			if (res) *res = DestroyIKResType::eIKDRT_INVALID;
			return false;//未达成删除条件
		}
		const std::set<EchoWeakPtr<IKEventListener>>* pLinsters = EchoIKSystem::instance()->GetIKEventListener(GetRoleComponent());
		if (m_pIKComponent->enabled) {
			m_pIKComponent->disable();
			if (pLinsters != nullptr) {
				for (auto& iter : *pLinsters) {
					if (iter != nullptr) iter->OnDisableIK(m_pCompData->GetIKInfo()->GetType(), this);
				}
			}
		}
		PreDestroyIK();
		pRoleIKComp->destroyIK(m_pCompData->GetIKInfo()->GetName());
		pRoleIKComp->UpdateTransform_V2();
		m_pIKComponent = nullptr;
		PostDestroyIK();
		if (pLinsters != nullptr) {
			for (auto& iter : *pLinsters) {
				if (iter != nullptr) iter->OnDestroyIK(m_pCompData->GetIKInfo()->GetType(), this);
			}
		}

		if (res) *res = DestroyIKResType::eIKDRT_SUCCESS;
		return true;//成功
	}


	void IKBaseComponent::Enable() {
		if (!IsExistIK() || m_pIKComponent->enabled) return;
		m_pIKComponent->enable();
		const std::set<EchoWeakPtr<IKEventListener>>* pLinsters = EchoIKSystem::instance()->GetIKEventListener(GetRoleComponent());
		if (pLinsters != nullptr) {
			for (auto& iter : *pLinsters) {
				if (iter != nullptr) iter->OnEnableIK(m_pCompData->GetIKInfo()->GetType(), this);
			}
		}
	}
	void IKBaseComponent::Disable() {
		if (!IsExistIK() || !m_pIKComponent->enabled) return;
		m_pIKComponent->disable();
		const std::set<EchoWeakPtr<IKEventListener>>* pLinsters = EchoIKSystem::instance()->GetIKEventListener(GetRoleComponent());
		if (pLinsters != nullptr) {
			for (auto& iter : *pLinsters) {
				if (iter != nullptr) iter->OnDisableIK(m_pCompData->GetIKInfo()->GetType(), this);
			}
		}
	}
	bool IKBaseComponent::GetIKEnabled() {
		if (!IsExistIK()) return false;
		return m_pIKComponent->enabled;
	}
	const EchoWeakPtr<RoleComponent>& IKBaseComponent::GetRoleComponent() {
		if (m_pRoleComponent != nullptr) return m_pRoleComponent;
		const String& roleName = GetRoleCompName();
		if (roleName.empty() || getActor() == nullptr) {
			m_pRoleComponent = nullptr;
		}
		else {
			m_pRoleComponent = getActor()->GetComponent<RoleComponent>(roleName);
		}
		if (m_pRoleComponent != nullptr) {
			RoleObject* pRoleObj = m_pRoleComponent->GetRole();
			if (pRoleObj != nullptr) {
				pRoleObj->addRoleObjectListener(this);
			}
		}
		return m_pRoleComponent;
	}
	bool IKBaseComponent::IsExistRole() {
		return GetRoleObject() != nullptr;
	}
	void IKBaseComponent::SetRoleCompName(const String& name) {
		if (m_pCompData->roleCompName == name && m_pRoleComponent != nullptr) return;
		bool IsExist = IsExistIK();
		if (IsExist) {
			DestroyIK();
		}
		if (m_pRoleObj != nullptr) {
			m_pRoleObj->removeRoleObjectListener(this);
		}
		m_pCompData->roleCompName = name;
		m_pRoleComponent = nullptr;
		EchoWeakPtr<RoleComponent> m_pRoleComponent = GetRoleComponent();
		if (IsExist) {
			CreateIK();
		}
	}
	const String& IKBaseComponent::GetRoleCompName() {
		return m_pCompData->roleCompName;
	}
	const IKBaseComponentData* IKBaseComponent::GetCompData() {
		return m_pCompData;
	}

	void IKBaseComponent::SetCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		SetRoleCompName(pData->roleCompName);
		SetIKInfo(pData->GetIKInfo());
	}

	FinalIKInfoBase* IKBaseComponent::GetIKInfo() {
		return m_pCompData->GetIKInfo();
	}
	IKComponent* IKBaseComponent::GetIKComponent() { 
		return m_pIKComponent;
	}
	void IKBaseComponent::GetSkeletonMap(std::map<Name, Bone*>& boneMap) {
		boneMap.clear();
		if (!GetRoleObject()) return;
		Echo::RoleMovable* pSkeObj = GetRoleObject()->getRoleMovable();
		if (pSkeObj) {
			Echo::SkeletonInstance* sklIns = pSkeObj->getSkeleton();
			if (sklIns) {
				int number = sklIns->getNumBones();
				for (int i = 0; i != number; ++i) {
					Bone* pBone = sklIns->getBoneByIndex((unsigned short)i);
					boneMap.insert(std::make_pair(pBone->getName(), pBone));
				}

			}
		}
	}
	void IKBaseComponent::GetSkeletonVector(std::vector<Bone*>& boneVector) {
		boneVector.clear();
		if (!GetRoleObject()) return;
		Echo::RoleMovable* pSkeObj = GetRoleObject()->getRoleMovable();
		if (pSkeObj) {
			Echo::SkeletonInstance* sklIns = pSkeObj->getSkeleton();
			if (sklIns) {
				int number = sklIns->getNumBones();
				for (int i = 0; i != number; ++i) {
					Bone* pBone = sklIns->getBoneByIndex((unsigned short)i);
					boneVector.push_back(pBone);
				}

			}
		}
	}
	void IKBaseComponent::GetSkeletonNamePtrVector(std::vector<const char*>& boneVector) {
		boneVector.clear();
		if (!GetRoleObject()) return;
		Echo::RoleMovable* pSkeObj = GetRoleObject()->getRoleMovable();
		if (pSkeObj) {
			Echo::SkeletonInstance* sklIns = pSkeObj->getSkeleton();
			if (sklIns) {
				int number = sklIns->getNumBones();
				for (int i = 0; i != number; ++i) {
					Bone* pBone = sklIns->getBoneByIndex((unsigned short)i);
					if (pBone != nullptr && !pBone->getName().empty()) {
						boneVector.push_back(pBone->getName().c_str());
					}
				}

			}
		}
	}
	void IKBaseComponent::GetCollisionNames(std::vector<Name>& collisionNames) {
		Role* pRole = GetRoleInfo();
		if (pRole == nullptr) return;
		CollisionMap collisions;
		pRole->getAllCollisionInfo(collisions, 1, true, true);
		for (auto& collision : collisions) {
			collisionNames.push_back(collision.first);
		}
	}
	void IKBaseComponent::GetHitCollisionNames(std::vector<Name>& collisionNames) {
		Role* pRole = GetRoleInfo();
		if (pRole == nullptr) return;
		CollisionMap collisions;
		pRole->getAllCollisionInfo(collisions, 2, true, true);
		for (auto& collision : collisions) {
			collisionNames.push_back(collision.first);
		}
	}
	void IKBaseComponent::GetRoleTagPointNames(std::vector<Name>& collisionNames) {
		Role* pRole = GetRoleInfo();
		if (pRole == nullptr) return;
		LocatorStructMap locators;
		pRole->getAllLocatorInfo(locators);
		for (auto& locator : locators) {
			collisionNames.push_back(locator.first);
		}
	}
	WorldNode* IKBaseComponent::GetIKBoneByName(const Name& name) {
		RoleIKComponent* pRoleIKComp = GetRoleIKComponent();
		if (!pRoleIKComp) return nullptr;
		return pRoleIKComp->getBoneNodeByName(name);
	}
	RoleObject* IKBaseComponent::GetRoleObject() {
		if (m_pRoleObj) return m_pRoleObj;
		m_pRoleComponent = nullptr;
		GetRoleComponent();
		return m_pRoleObj;
	}
	RoleTagPointComponent* IKBaseComponent::GetRoleTagPointComponent() {
		RoleObject* pRoleObj = GetRoleObject();
		if (!pRoleObj) return nullptr;
		return pRoleObj->getTagPointComponent();
	}
	IKBaseComponentData* IKBaseComponent::ReSetCompData()
	{
		SAFE_DELETE(m_pCompData);
		m_pCompData = CreateCompData();
		return m_pCompData;
	}
	RoleIKComponent* IKBaseComponent::GetRoleIKComponent() {
		RoleObject* pRoleObj = GetRoleObject();
		if (!pRoleObj) return nullptr;
		return pRoleObj->getRoleIKComponent();
	}
	Role* IKBaseComponent::GetRoleInfo() {
		RoleObject* pRoleObj = GetRoleObject();
		if (!pRoleObj) return nullptr;
		return pRoleObj->getSkeletonComponent()->getRoleInfo().get();
	}
	bool IKBaseComponent::CheckExistFBBIK(Name& ik) {
		if (ik == Name::g_cEmptyName) {
			ik = gDefaultFullBodyBipedIKName;
		}
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return false;
		IKInfoMap ikInfoMap;
		roleInfo->getAllIKInfo(ikInfoMap, FinalIKType_FullBodyBipedIK);
		return ikInfoMap.find(ik) != ikInfoMap.end();
	}
	bool IKBaseComponent::CheckExistLimb(const Name& ik) {
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return false;
		IKInfoMap ikInfoMap;
		roleInfo->getAllIKInfo(ikInfoMap, FinalIKType_LimbIK);
		return ikInfoMap.find(ik) != ikInfoMap.end();
	}
	bool IKBaseComponent::CheckValidFBBIK(Name& ik) {
		if (ik == Name::g_cEmptyName) {
			ik = gDefaultFullBodyBipedIKName;
		}
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return false;
		IKInfoMap ikInfoMap;
		roleInfo->getAllIKInfo(ikInfoMap, FinalIKType_FullBodyBipedIK);
		auto iter = ikInfoMap.find(ik);
		if (iter == ikInfoMap.end()) {
			return false;
		}
		RoleIKComponent* pRoleIKComponent = GetRoleIKComponent();
		if (pRoleIKComponent) {
			return EchoIK::FullBodyBipedIK::CheckValid(pRoleIKComponent, iter->second);
		}
		return true;
	}
	bool IKBaseComponent::CheckValidLimb(const Name& ik) {
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return false;
		IKInfoMap ikInfoMap;
		roleInfo->getAllIKInfo(ikInfoMap, FinalIKType_LimbIK);
		auto iter = ikInfoMap.find(ik);
		if (iter == ikInfoMap.end()) {
			return false;
		}
		RoleIKComponent* pRoleIKComponent = GetRoleIKComponent();
		if (pRoleIKComponent) {
			return EchoIK::LimbIK::CheckValid(pRoleIKComponent, iter->second);
		}
		return true;
	}
	void IKBaseComponent::FindIKInfosInCurrentRole(std::vector<FinalIKInfoBase*>& infos, FinalIKTypeEnum type) {
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return;
		IKInfoMap ikInfoMap;
		roleInfo->getAllIKInfo(ikInfoMap,type);
		for (auto pairInfo : ikInfoMap) {
			infos.push_back(pairInfo.second);
		}
	}
	void IKBaseComponent::FindIKInfosInRole(std::vector<std::pair<Name, std::vector<Name>>>& roleInfos, FinalIKTypeEnum type) {
		Role* roleInfo = GetRoleInfo();
		if (!roleInfo) return;
		roleInfo->getAllRoleIKInfo(roleInfos, type);
	}
	ActorComponentPtr IKBaseComponent::FindComponent(const String& compName) {
		if (compName.empty()) return nullptr;
		if (getActor()->IsRelativeName(compName)) {
			PickActorResult res = getActor()->getActorComByRelativeName(compName);
			return res.seleteCom;
		}
		ActorComponentPtr compPtr = getActor()->getComponent(compName);
		if (compPtr != nullptr) {
			return compPtr;
		}
		PickActorResult res = getActor()->getActorManager()->GetActorComBySeletedName(compName);
		return res.seleteCom;
	}

	const Name& IKBaseComponent::getTagPointNameByTargetComponent(const EchoWeakPtr<RoleComponent>& rolePtr, const ActorComponentPtr& compPtr) {
		if (compPtr == nullptr || rolePtr == nullptr) return Name::g_cEmptyName;
		ActorComponentPtr comp = compPtr;
		ActorComponentPtr child = ActorComponent::NULLPTR;
		while (comp != nullptr) {
			if (comp == rolePtr) {
				if ((child != nullptr) && (child->getType() == TagPointPosComponent::mType)) {
					return ((TagPointPosComponent*)child.getPointer())->GetTagPointName();
				}
				break;
			}
			child = comp;
			comp = comp->getParentNodeCom();
		}
		return Name::g_cEmptyName;
	}

	const Name& IKBaseComponent::getBoneNameByTargetComponent(const EchoWeakPtr<RoleComponent>& rolePtr, const ActorComponentPtr& compPtr) {
		if (compPtr == nullptr || rolePtr == nullptr) return Name::g_cEmptyName;
		ActorComponentPtr comp = compPtr;
		ActorComponentPtr child = ActorComponent::NULLPTR;
		while (comp != nullptr) {
			if (comp == rolePtr) {
				if ((child != nullptr) && (child->getType() == TagPointPosComponent::mType)) {
					LocatorStruct* pInfo = rolePtr->GetRoleInfo()->getLocatorInfo(((TagPointPosComponent*)child.getPointer())->GetTagPointName());
					if (pInfo) {
						return pInfo->sBoneName;
					}
				}
				break;
			}
			child = comp;
			comp = comp->getParentNodeCom();
		}
		return Name::g_cEmptyName;
	}

	const Name& IKBaseComponent::getTagPointNameByTargetComponent(const ActorComponentPtr& compPtr) {
		return getTagPointNameByTargetComponent(GetRoleComponent(), compPtr);
	}

	const Name& IKBaseComponent::getBoneNameByTargetComponent(const ActorComponentPtr& compPtr) {
		return getBoneNameByTargetComponent(GetRoleComponent(), compPtr);
	}

	EchoIKManager* EchoIKManager::instance() {
		return EchoIKSystem::instance()->GetMainManager();
	}
	
	EchoIKManager::EchoIKManager(const Name& name) : ProtocolComponent(ProtocolComponentSystem::instance()->GetManager(name)) {
		registerTickTimeBegin();
	}
	EchoIKManager::EchoIKManager(ProtocolComponentManager* pMgr) : ProtocolComponent(pMgr) {
		registerTickTimeBegin();
	}
	EchoIKManager::~EchoIKManager() {

	}
	void EchoIKManager::registerTickTimeBegin() {
		m_priority = -99;
		m_eventSet[TT_PostFrame].insert(EID_EveryFrame);
		m_eventSet[TT_LateUpdate].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void EchoIKManager::AddDestroyList(const IKBaseComponentPtr& compPtr) {
		if (compPtr == nullptr) {
			return;
		}
		mDestroyIKList.push_back(compPtr);
	}

	void EchoIKManager::AddCreateList(const IKBaseComponentPtr& compPtr) {
		if (compPtr == nullptr) {
			return;
		}
		mCreateIKList.push_back(compPtr);
	}
	void EchoIKManager::AddEditorUpdateCompoennt(const IKBaseComponentPtr& compPtr) {
#ifdef ECHO_EDITOR
		if (compPtr == nullptr) {
			return;
		}
		mEditorUpdateIKList.push_back(compPtr);
#else
		//TODO
#endif // ECHO_EDITOR
	}

	//void EchoIKManager::AddTickComponent(const TagPointPosComponentPtr& compPtr) {
	//	if (compPtr == nullptr) {
	//		return;
	//	}
	//	mTagPointPosCompList.push_back(compPtr);
	//}
	//
	//void EchoIKManager::TickTagPointPosComponent() {
	//	if (mTagPointPosCompList.empty()) return;
	//	auto iter = mTagPointPosCompList.begin();
	//	while (iter != mTagPointPosCompList.end()) {
	//		if (iter->expired()) {
	//			iter = mTagPointPosCompList.erase(iter);
	//			continue;
	//		}
	//		(*iter)->RecordWorldTransform();
	//		++iter;
	//	}
	//}
	

	void EchoIKManager::postFrame(EventID, ProtocolHandle) {
		DestroyIK();
	}
	void EchoIKManager::lateUpdate(EventID, ProtocolHandle) {
		UpdateComponentInEditor();
		//TickTagPointPosComponent();
	}

	void EchoIKManager::getProtocolType(Name& type)
	{
		type = EchoIKManager::mType;
	}

	void EchoIKManager::getProtocolTypeToString(std::string& typeStr)
	{
		typeStr = EchoIKManager::mType.c_str();
	}

	void EchoIKManager::UpdateComponentInEditor() {
		if (mEditorUpdateIKList.empty()) return;
		auto iter = mEditorUpdateIKList.begin();
		while (iter != mEditorUpdateIKList.end()) {
			if (*iter == nullptr) {
				iter = mEditorUpdateIKList.erase(iter);
				continue;
			}
			(*iter)->UpdateComponentInEditor();
			++iter;
		}
	}

	void EchoIKManager::DestroyIK() {
		if (mDestroyIKList.empty()) {
			return;
		}
		std::list<IKBaseComponentPtr> tempList;
		tempList.swap(mDestroyIKList);

		for (auto& compPtr : tempList) {
			if (compPtr == nullptr) {
				continue;
			}
			compPtr->DestroyIK();
		}
	}
	void EchoIKManager::CreateIK() {
		if (mCreateIKList.empty()) {
			return;
		}
		std::list<IKBaseComponentPtr> tempList;
		tempList.swap(mCreateIKList);

		for (auto& compPtr : tempList) {
			if (compPtr == nullptr) {
				continue;
			}
			compPtr->CreateIK();
		}
	}

	void EchoIKManager::AddIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener) {
		EchoIKSystem::instance()->AddIKEventListener(role, listener);
	}
	void EchoIKManager::RemoveIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener) {
		EchoIKSystem::instance()->RemoveIKEventListener(role, listener);
	}
	const std::set<EchoWeakPtr<IKEventListener>>* EchoIKManager::GetIKEventListener(const EchoWeakPtr<RoleComponent>& role) {
		return EchoIKSystem::instance()->GetIKEventListener(role);
	}

	void EchoIKManager::ClearExpiredIKEventListener() {
		EchoIKSystem::instance()->ClearExpiredIKEventListener();
	}


	Echo::Name EchoIKManager::mType("EchoIKManager");

	EchoIKSystem::EchoIKSystem() {

	}
	EchoIKSystem::~EchoIKSystem() {

	}

	EchoIKManager* EchoIKSystem::GetManager(ActorComponent* pComp) {
		ActorManager* pActorManager = pComp->getActorManager();
		if (!pActorManager) {
			return GetMainManager();
		}
		return CreateManager(pActorManager->getName());
	}

	void EchoIKSystem::AddIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener) {
		if (role == nullptr || listener == nullptr) return;
		auto iter = mRoleIKListener.find(role);
		if (iter == mRoleIKListener.end()) {
			mRoleIKListener[role].insert(listener);
		}
		else {
			iter->second.insert(listener);
		}
	}
	void EchoIKSystem::RemoveIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener) {
		if (role == nullptr || listener == nullptr) return;
		auto iter = mRoleIKListener.find(role);
		if (iter != mRoleIKListener.end()) {
			iter->second.erase(listener);
			if (iter->second.empty()) {
				mRoleIKListener.erase(iter);
			}
		}
	}
	const std::set<EchoWeakPtr<IKEventListener>>* EchoIKSystem::GetIKEventListener(const EchoWeakPtr<RoleComponent>& role) {
		if (role == nullptr) return nullptr;
		auto iter = mRoleIKListener.find(role);
		if (iter != mRoleIKListener.end()) {
			return &(iter->second);
		}
		return nullptr;
	}

	void EchoIKSystem::ClearExpiredIKEventListener() {
		auto iter = mRoleIKListener.begin();
		while (iter != mRoleIKListener.end()) {
			if (iter->first == nullptr) {
				iter = mRoleIKListener.erase(iter);
				continue;
			}
			auto iter2 = iter->second.begin();
			while (iter2 != iter->second.end()) {
				if (*iter2 == nullptr) {
					iter2 = iter->second.erase(iter2);
					continue;
				}
				++iter2;
			}
			if (iter->second.empty()) {
				iter = mRoleIKListener.erase(iter);
				continue;
			}
			++iter;
		}
	}
}
