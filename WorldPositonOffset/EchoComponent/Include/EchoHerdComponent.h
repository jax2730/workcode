#ifndef __EchoHerdComponent_H__
#define __EchoHerdComponent_H__ 

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"

namespace Echo
{
	class HerdCurvedObject;
	class HerdComponent;
	class HerdComponentInfo;
	class HerdNode;

#ifdef ECHO_EDITOR
	class HerdComponentHelper
	{
	public:
		HerdComponentHelper(HerdComponent* pMaster);
		~HerdComponentHelper();

		void	UpdatePoints();

		void	SetPointHalfSize(float size);
		float	GetPointHalfSize();

		bool	Visible();
		void	Visible(bool);

		bool	SetSeletedPointIndex(uint32 idx);
		uint32  GetSeletedPointIndex();

		uint32  testHit(const Ray& ray);
	private:
		bool			mVisible		= false;
		float			mPointHalfSize  = 0.2f;

		HelperObject	*mDrawLineObj	= nullptr;
		HelperObject	*mDrawPointObj  = nullptr;
		HerdComponent	*mMaster		= nullptr;

		uint32			mSeletedPointIdx = -1;
	};
#endif
	;

	struct _EchoComponentExport HerdNodeStaticData
	{
		std::string		mRoleName;
		std::string		mSkinName;
		std::string		mNormalAnimationName;
		std::string		mEscapeAnimationName;
		float			mRoleScale = 1.f;
		float			mAnimationSpeed = 1.f;
		float			mRenderDistance = 1000.f;
		float			mLodDistance[ROLE_LOD_LEVEL_CNT] = { 20.f,100.f,500.f };
		float			mStartPos = 0.f;
		bool			mAnimationLoop = true;
		static const HerdNodeStaticData DefaultValue;
	};
	typedef std::vector< HerdNodeStaticData> HerdNodeStaticDataList;
	void WriteJSNode(JSNODE pRoot, const char* name, const HerdNodeStaticData& data);
	bool ReadJSNode(JSNODE pRoot, const char* name, HerdNodeStaticData& data);


	class _EchoComponentExport HerdComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(HerdComponent)
	public:
		// override form ActorComponent
		virtual void			init(ActorComponentInfo* info)			override;
		virtual void			replay(ActorComponentInfo* info)		override;
		virtual void			copyTo(const ActorComponentPtr& actCom) override;
		virtual void			bindNode()								override;

		virtual void			EditorPlayImp()							override;
		virtual void			EditorPauseImp(bool pause)				override;
		virtual void			EditorStopImp()							override;
	public: // 组件参数调整
		void					SetNormalSpeed(float speed);									// 设置正常模式节点的移动速度
		float					GetNormalSpeed() const;											// 获取...
		void					SetEscapeSpeed(float speed);									// 设置逃离模式节点的移动速度
		float					GetEscapeSpeed() const;											// 获取...
		void					SetEscapeDistance(float dis);									// 设置节点的逃离距离
		float					GetEscapeDistance() const;										// 获取...
		void					SetRestoreTime(float time);										// 设置节点从逃离到恢复的时间
		float					GetRestoreTime()const;											// 获取...

		bool					IsCloseCurve() const;
		void					SetCloseCurve(bool close);
		uint32					GetPointCount() const;
		Vector3					GetPointPosition(uint32 ponitIndex) const;
		void					SetPointPosition(uint32 ponitIndex, const Vector3& postsion);
		void					InsertPoint(uint32 pointIndex, const Vector3& postsion);
		void					InsertPoint(const Vector3& postsion);
		void					ErasePoint(uint32 pointIndex);
		void					ErasePoint();
		void					ClearPoint();													
		void					GetAllPointPosition(std::vector<Vector3> &out) const;
		void					ResetPointPosition(const std::vector<Vector3> &in);

		uint32					GetHerdNodeCount() const;										// 获取角色数量
		HerdNode*				GetHerdNode(uint32 index) const;								// 获取角色节点
		HerdNode*				AddHerdNode(uint32 index, const HerdNodeStaticData& data = HerdNodeStaticData::DefaultValue);// 添加角色节点
		HerdNode*				AddHerdNode(const HerdNodeStaticData& data = HerdNodeStaticData::DefaultValue);				 // 在末尾添加角色节点
		void					RemoveHerdNode(uint32 index);									// 移除角色节点	
		void					RemoveHerdNode();												// 移除末尾角色节点
		void					RemoveAllHerdNode();											// 移除所有角色节点	

	public:
		void 					Reset();
		Curve*					GetCurve() const;
	private:
		HerdCurvedObject*		mHerdObject = nullptr;
	private:
		// --- BEGIN ECHO EDITOR -------------------------------------------------------------------
#ifdef ECHO_EDITOR
	public:
		void					ShowHelper(bool show);
		bool					ShowHelper();
		void					ShowHerdNode(bool show);
		bool					ShowHerdNode();
		void					SetSelectedPoint(uint32 pointIndex);
		uint32					GetSelectedPoint();

		void					EditorChangeHerdNodeStaticData_DeletedNode(const HerdNodeStaticDataList&, uint32 delIdx);
		void					EditorChangeHerdNodeStaticData_CopyNode(const HerdNodeStaticDataList& data, uint32 idx, uint32 num);
		void					EditorChangeHerdNodeStaticData_SetRole(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetSkin(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetScale(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetLodDistance(const HerdNodeStaticDataList& data, uint32 idx, ENUM_ROLE_LOD_LEVEL level);
		void					EditorChangeHerdNodeStaticData_SetRndDistance(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetStartPos(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetNorAnime(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetEscAnime(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetAnimeSpeed(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_SetAnimeLoop(const HerdNodeStaticDataList& data, uint32 idx);
		void					EditorChangeHerdNodeStaticData_AddNode(const HerdNodeStaticDataList& data);

		// for EditorPiecewiseListener
		uint32					getPointCount() { return GetPointCount(); }
		void					update() {}
		void					setPickHelperIndex(uint32 idx) { SetSelectedPoint(idx); };
		bool					pickHelper(const Ray& ray) { return mHelper->SetSeletedPointIndex(mHelper->testHit(ray)); };
	private:
		HerdComponentHelper*	mHelper = nullptr;
#endif
		// --- -END- ECHO EDITOR -------------------------------------------------------------------
	};
	class _EchoComponentExport HerdComponentInfo : public ActorComponentInfo
	{
	public:
		HerdComponentInfo() {}
		HerdComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		_DECLCOMINFO_VIRFUN
	public:
		float					mNormalSpeed = 1.0f;
		float					mEscapeSpeed = 2.5f;
		float					mEscapeRange = 10.f;
		float					mRestoreTime = 5.0f;
		bool					mCloseCurve  = false;
		std::vector<Vector3>	mPointPosition;
		HerdNodeStaticDataList	mHerdNodeStaticDataList;
		EDITOR_REFLECT
	};
	DEFINECOMFACTORY(HerdComponent, ALL_MOVEABLE);
	typedef EchoWeakPtr<HerdComponent> HerdComponentPtr;

}

#endif // #ifndef __EchoBiocoenosisComponent_H__  
