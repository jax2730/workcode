//-----------------------------------------------------------------------------
// File:   EchoIKBaseComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKBaseComponent_h__ 
#define __EchoIKBaseComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoRole.h"
#include "EchoActorRoleComponent.h"

#ifdef ECHO_EDITOR
#define USE_IK_HELPEROBJ
#endif // ECHO_EDITOR

namespace Echo {


	class IKBaseComponent; typedef EchoWeakPtr<IKBaseComponent> IKBaseComponentPtr;
	class IKAimIKComponent; typedef EchoWeakPtr<IKAimIKComponent> IKAimIKComponentPtr;
	class IKBodyTiltComponent; typedef EchoWeakPtr<IKBodyTiltComponent> IKBodyTiltComponentPtr;
	class IKCCDIKComponent; typedef EchoWeakPtr<IKCCDIKComponent> IKCCDIKComponentPtr;
	class IKEffectorOffsetComponent; typedef EchoWeakPtr<IKEffectorOffsetComponent> IKEffectorOffsetComponentPtr;
	class IKFBBIKHeadEffectorComponent; typedef EchoWeakPtr<IKFBBIKHeadEffectorComponent> IKFBBIKHeadEffectorComponentPtr;
	class IKGrounderFBBIKComponent; typedef EchoWeakPtr<IKGrounderFBBIKComponent> IKGrounderFBBIKComponentPtr;
	class IKGrounderIKComponent; typedef EchoWeakPtr<IKGrounderIKComponent> IKGrounderIKComponentPtr;
	class IKGrounderQuadrupedComponent; typedef EchoWeakPtr<IKGrounderQuadrupedComponent> IKGrounderQuadrupedComponentPtr;
	class IKHitReactionComponent; typedef EchoWeakPtr<IKHitReactionComponent> IKHitReactionComponentPtr;
	class IKInertiaComponent; typedef EchoWeakPtr<IKInertiaComponent> IKInertiaComponentPtr;
	class IKLimbIKComponent; typedef EchoWeakPtr<IKLimbIKComponent> IKLimbIKComponentPtr;
	class IKLookAtIKComponent; typedef EchoWeakPtr<IKLookAtIKComponent> IKLookAtIKComponentPtr;
	class IKPenetrationAvoidanceComponent; typedef EchoWeakPtr<IKPenetrationAvoidanceComponent> IKPenetrationAvoidanceComponentPtr;
	class IKRecoilComponent; typedef EchoWeakPtr<IKRecoilComponent> IKRecoilComponentPtr;
	class IKRotationLimitAngleComponent; typedef EchoWeakPtr<IKRotationLimitAngleComponent> IKRotationLimitAngleComponentPtr;
	class IKRotationLimitHingeComponent; typedef EchoWeakPtr<IKRotationLimitHingeComponent> IKRotationLimitHingeComponentPtr;
	class IKTargetFBBIKComponent; typedef EchoWeakPtr<IKTargetFBBIKComponent> IKTargetFBBIKComponentPtr;
	class TagPointPosComponent; typedef EchoWeakPtr<TagPointPosComponent> TagPointPosComponentPtr;
	class IKEventListener; typedef EchoWeakPtr<IKEventListener> IKEventListenerPtr;

	enum CreateIKResType : uint16 {
		eIKCRT_SUCCESS = 1 << 0, //创建成功
		eIKCRT_IKEXIST = 1 << 1, //IK已经存在
		eIKCRT_INVISIBLE = 1 << 2, //visible是false
		eIKCRT_ROLENOTEXIST = 1 << 3, //Role不存在
		eIKCRT_ROLENOTLOAD = 1 << 4, //Role未加载完成，已打开自动创建
		eIKCRT_INVALID = 1 << 5, //未达成创建条件
		eIKCRT_ERROR = 1 << 6, //异常失败
		eIKCRT_ROLENOTSKEL = 1 << 7, //Role无骨骼
	};

	enum DestroyIKResType : uint16 {
		eIKDRT_SUCCESS = 1 << 0, //删除成功
		eIKDRT_IKNOTEXIST = 1 << 1, //IK不存在或是Role未加载完成，已关闭自动创建
		eIKDRT_ROLENOTEXIST = 1 << 2, //Role不存在或是已被删除
		eIKDRT_INVALID = 1 << 3 //未达成删除条件
	};

	class _EchoComponentExport IKEventListener {
	public:
		virtual void OnCreateIKAfterAsync(FinalIKTypeEnum type, IKBaseComponent* pIK) {}
		virtual void OnCreateIK(FinalIKTypeEnum type, IKBaseComponent* pIK) {}
		virtual void OnDestroyIK(FinalIKTypeEnum type, IKBaseComponent* pIK) {}
		virtual void OnEnableIK(FinalIKTypeEnum type, IKBaseComponent* pIK) {}
		virtual void OnDisableIK(FinalIKTypeEnum type, IKBaseComponent* pIK) {}
	};

	struct IKBaseComponentData {
		String roleCompName = String("");
		virtual FinalIKInfoBase* GetIKInfo() = 0;
		virtual FinalIKTypeEnum GetIKType() { return GetIKInfo()->GetType(); }
		IKBaseComponentData() {};
		virtual ~IKBaseComponentData(){}
	};

	//IK组件基类
	class _EchoComponentExport IKBaseComponent : public ActorComponent, public RoleObjectListener {
		REGISTER_LUA_CLASS_KEY;
	public:
		IKBaseComponent(ActorPtr pActor, ActorCOMMoveableType moveable);
		virtual ~IKBaseComponent();
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void visibleStateChanged(bool activeState) override final;

		virtual void EditorPlayImp() override;
		virtual void EditorPauseImp(bool pause) override;
		virtual void EditorStopImp() override;
		virtual bool OnRoleCreateFinish() override;
		virtual void OnRoleDestroy() override;
		virtual bool IsCanCreate() { return true; }
		virtual bool IsCanDestroy() { return true; }
	public:
		bool CreateIK(CreateIKResType* res = nullptr);
		bool DestroyIK(DestroyIKResType* res = nullptr);
		void Enable();
		void Disable();
		bool GetIKEnabled();
	public:
		inline bool IsExistIK() { return m_pIKComponent != nullptr; }
		bool IsExistRole();
		void SetRoleCompName(const String& name);
		const String& GetRoleCompName();
		virtual void SetCompData(IKBaseComponentData* pData);
		const IKBaseComponentData* GetCompData();
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) = 0;
		FinalIKInfoBase* GetIKInfo();
		virtual void UseDefaultInitialize() {}
		IKComponent* GetIKComponent();
		virtual IKBaseComponentData* CreateCompData() = 0;
	public:
		WorldNode* GetIKBoneByName(const Name& name);
		RoleIKComponent* GetRoleIKComponent();
		const EchoWeakPtr<RoleComponent>& GetRoleComponent();
		RoleObject* GetRoleObject();
		Role* GetRoleInfo();
		RoleTagPointComponent* GetRoleTagPointComponent();
		void SetRoleObject(RoleObject* pRoleObj) {}
		IKBaseComponentData* ReSetCompData();
	protected:
		virtual void PreCreateIK() {}
		virtual void PostCreateIK() {}
		virtual void PreDestroyIK() {}
		virtual void PostDestroyIK() {}
		virtual void UpdateComponentInEditor() {}
	public:
		bool CheckExistFBBIK(Name& ik);
		bool CheckExistLimb(const Name& ik);
		bool CheckValidFBBIK(Name& ik);
		bool CheckValidLimb(const Name& ik);
		void FindIKInfosInCurrentRole(std::vector<FinalIKInfoBase*>& infos, FinalIKTypeEnum type);
		void FindIKInfosInRole(std::vector<std::pair<Name, std::vector<Name>>>& infos, FinalIKTypeEnum type);
		void GetSkeletonMap(std::map<Name, Bone*>& boneMap);
		void GetSkeletonVector(std::vector<Bone*>& boneVector);
		void GetSkeletonNamePtrVector(std::vector<const char*>& boneVector);
		void GetCollisionNames(std::vector<Name>& names);
		void GetHitCollisionNames(std::vector<Name>& names);
		void GetRoleTagPointNames(std::vector<Name>& names);
	public:
		static Name gDefaultFullBodyBipedIKName;
		static bool gUseCheckIKValid;
	public:
		ActorComponentPtr FindComponent(const String& compName);
		template<typename T>
		EchoWeakPtr<T> FindComponent(const String& compName) {
			return FindComponent(compName).getDynamicCastWeakPtr<T>();
		}
		const Name& getTagPointNameByTargetComponent(const ActorComponentPtr& compPtr);
		const Name& getBoneNameByTargetComponent(const ActorComponentPtr& compPtr);
		static const Name& getTagPointNameByTargetComponent(const EchoWeakPtr<RoleComponent>& rolePtr, const ActorComponentPtr& compPtr);
		static const Name& getBoneNameByTargetComponent(const EchoWeakPtr<RoleComponent>& rolePtr, const ActorComponentPtr& compPtr);
	private:
		void ExecuteIKOperation(bool activeState);

	protected:
		bool mIsCreateIK = false;
		EchoWeakPtr<RoleComponent> m_pRoleComponent = nullptr;
		IKBaseComponentData* m_pCompData = nullptr;
		IKComponent*		m_pIKComponent = nullptr;
		friend class EchoIKManager;
	};


	class _EchoComponentExport IKBaseComponentInfo : public ActorComponentInfo {
	public:
		IKBaseComponentInfo() {}
		IKBaseComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : ActorComponentInfo(comType, moveable) {}
		virtual ~IKBaseComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData);
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp);
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKBaseComponent)
		_DECLCOMINFO_PARAM(String, RoleCompName, String(""), SetRoleCompName)
		_DECLCOMINFO_PARAM_END()
	};

	class _EchoComponentExport EchoIKManager : public ProtocolComponent {
	public:
		static	Echo::Name			mType;
		const Echo::Name& getType() { return mType; }
		static EchoIKManager* instance();
	public:
		REGISTER_LUA_CLASS_KEY

		virtual ~EchoIKManager();
		virtual void registerTickTimeBegin() override;
		virtual void postFrame(EventID, ProtocolHandle) override;
		virtual void lateUpdate(EventID, ProtocolHandle) override;


		virtual void getProtocolType(Name& type) override;
		virtual void getProtocolTypeToString(std::string& typeStr) override;

	public:
		void AddIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener);
		void RemoveIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener);
		const std::set<EchoWeakPtr<IKEventListener>>* GetIKEventListener(const EchoWeakPtr<RoleComponent>& role);
		void ClearExpiredIKEventListener();
	public:
		void AddDestroyList(const IKBaseComponentPtr& compPtr);
		void AddCreateList(const IKBaseComponentPtr& compPtr);
		void AddEditorUpdateCompoennt(const IKBaseComponentPtr& compPtr);
		//void AddTickComponent(const TagPointPosComponentPtr& compPtr);
	private:
		void DestroyIK();
		void CreateIK();
		void UpdateComponentInEditor();
		//void TickTagPointPosComponent();

		std::list<IKBaseComponentPtr> mDestroyIKList;
		std::list<IKBaseComponentPtr> mCreateIKList;
		std::list<IKBaseComponentPtr> mEditorUpdateIKList;
	private:
		EchoIKManager(const Name& name);
		EchoIKManager(ProtocolComponentManager*);
		friend class CoreSystemBase<EchoIKManager>;
		friend class EchoIKSystem;
		friend class IKBaseComponent;
	};

	class _EchoComponentExport EchoIKSystem : public Singleton<EchoIKSystem>, public CoreSystemBase<EchoIKManager> {
	public:
		EchoIKSystem();
		virtual ~EchoIKSystem();
		EchoIKManager* GetManager(ActorComponent* pComp);
	public:
		void AddIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener);
		void RemoveIKEventListener(const EchoWeakPtr<RoleComponent>& role, const EchoWeakPtr<IKEventListener>& listener);
		const std::set<EchoWeakPtr<IKEventListener>>* GetIKEventListener(const EchoWeakPtr<RoleComponent>& role);
		void ClearExpiredIKEventListener();
	private:
		std::unordered_map<EchoWeakPtr<RoleComponent>, std::set<EchoWeakPtr<IKEventListener>>> mRoleIKListener;
	};

	template<typename T>
	void SaveObjectArrayJSON(cJSON* father, const char* label, const std::vector<T>& arr) {
		cJSON* pArr = cJSON_CreateArray();
		for (int i = 0; i != arr.size(); ++i) {
			cJSON_AddItemToArray(pArr, arr[i].SaveInfo());
		}
		cJSON_AddItemToObject(father, label, pArr);
	}
	template<typename T>
	void LoadObjectArrayJSON(cJSON* father, const char* label, std::vector<T>& arr) {
		cJSON* pArr = cJSON_GetObjectItem(father, label);
		if (!pArr || pArr->type != cJSON_Array) {
			return;
		}
		arr.clear();
		T temp;
		for (JSNODE ChildNode = pArr->child; ChildNode; ChildNode = ChildNode->next) {
			temp.LoadInfo(ChildNode);
			arr.push_back(temp);
		}
	}



	inline void WriteJSNode(JSNODE pRoot, const char * name, const GrounderFBBIKInfo& value)
	{
		value.Save(pRoot);
	}
	inline bool ReadJSNode(JSNODE pRoot, const char * name, GrounderFBBIKInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const GroundingInfo& value)
	{
		cJSON_AddItemToObject(pRoot, name, value.SaveInfo());
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, GroundingInfo& value)
	{
		value.LoadInfo(cJSON_GetObjectItem(pRoot, name));
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const std::vector<SpineEffectorInfo>& value)
	{
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, std::vector<SpineEffectorInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const TargetFBBIKInfo& value)
	{
		value.Save(pRoot);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, TargetFBBIKInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const AimIKInfo& value)
	{
		value.Save(pRoot);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, AimIKInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const std::vector<NodeWeightInfo>& value)
	{
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, std::vector<NodeWeightInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}
	inline void WriteJSNode(JSNODE pRoot, const char * name, const FBBIKHeadEffectorInfo& value)
	{
		value.Save(pRoot);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, FBBIKHeadEffectorInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const GrounderIKInfo& value)
	{
		value.Save(pRoot);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, GrounderIKInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char * name, const GrounderQuadrupedInfo& value)
	{
		value.Save(pRoot);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char * name, GrounderQuadrupedInfo& value)
	{
		value.Load(pRoot);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<IKRecoilInfo::Offsets>& value)
	{
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<IKRecoilInfo::Offsets>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}


	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<InertiaBodyInfo>& value)
	{
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<InertiaBodyInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<InertiaOffsetLimitsInfo>& value)
	{
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<InertiaOffsetLimitsInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<HitReaHitPointEffectorInfo>& value) {
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<HitReaHitPointEffectorInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<HitReaHitPointBoneInfo>& value) {
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<HitReaHitPointBoneInfo>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<BTEffectorLink>& value) {
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<BTEffectorLink>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

	inline void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<Avoider>& value) {
		SaveObjectArrayJSON(pRoot, name, value);
	}

	inline bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<Avoider>& value)
	{
		LoadObjectArrayJSON(pRoot, name, value);
		return true;
	}

}

#endif /*__EchoIKBaseComponent_h__*/
