
#ifndef _EchoPolylineComponent_h__
#define _EchoPolylineComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoSceneNode.h"

#include "Actor/EchoActor.h"
#include "Actor/EchoActorComponent.h"

#include "EchoMovableObject.h"
#include "EchoManualObject.h"
#include "EchoComDefineMacro.h"
#include "ControlHelperRenderable.h"
#include "EchoTrafficObject.h"
#include "EchoRay.h"

#include "EchoParticleSystemComponent.h"


namespace Echo
{
	using namespace EchoBox;

	class _EchoComponentExport PolylineComponentInfo : public ActorComponentInfo
	{
	public:
		PolylineComponentInfo() {};
		PolylineComponentInfo(const Name& comType, ActorCOMMoveableType MoveType = ALL_MOVEABLE);
		virtual ~PolylineComponentInfo();

		_DECLCOMINFO_VIRFUN
	public:
		std::vector<Vector3>				Points; // 点位置列表
		int									PointCount = 0;
		std::string							BindPtclSysName = ""; // 绑定的粒子系统名称
		bool								IsWorldPos = false;
		//EDITOR_REFLECT
	};

	class _EchoComponentExport PolylineComponent : public NodeActorComponent
	{

		//friend class EchoPolylineComponentInfo;
		DECLAREACTORCOMTYPE_H(PolylineComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		//virtual void						UpdateComRender();
		virtual void						init(ActorComponentInfo* info) override;
		virtual void                        unInit() override;
		bool								GetHelperVisiblity() const { return m_IsHelperVisible; } // 获取显示状态
		void								SetHelperVisibility(bool isShow); // 设置显示隐藏

		void								SetLineHelperVisiblity(bool isShow); // 设置是否显示辅助线
		void								SetPointHelperVisiblity(bool isShow); // 设置是否显示控制点辅助显示对象

		void								CreatePoint(); // 创建点
		void								SetPoint(int index, const Vector3 pos); // 设置点位置
		void								RemovePoint(int index); // 移除点
		void 								RemoveAllPoints(); // 移除所有点
		Vector3                             RelativeToWorldPos(const Vector3& pos); // 单个点坐标相对位置转世界位置

		void								CreateLineHelper(); // 创建辅助线
		void								UpdateLineHelper(); // 更新辅助线
							
		void								UpdateAllPointHelpers(const std::vector<Vector3>& points); // 更新所有辅助点
		void                                AddPointHelper(const Vector3 pos); // 增加单个辅助点
		void                                UpdatePointHelper(const Vector3& pos, const int index); // 更新单个辅助点位置
		void                                RemovePointHelper(const int index); // 移除单个辅助点
		void								RemoveAllPointHelpers(); // 清除辅助点	

		std::vector<ActorComponentPtr>		FindAllParticleSystem(); // 查找所属actor所有粒子系统组件
		void								ExportDataToParticleSystem(); // 导出数据到粒子系统组件

		std::string							GetBindPtclSysName() const { return m_Info->BindPtclSysName; }
		void                                SetBindPtclSysName(const std::string& name) { m_Info->BindPtclSysName = name; }
		std::vector<Vector3>				GetPoints() const { return m_Info->Points; }
		int									GetPointCount() const { return m_Info->PointCount; }

		size_t								GetPickedHelperIndex() const { return m_PickedHelperIndex; }
		SphereHelper*						GetPickedHelper();
		Vector3								GetPickedHelperPos();
	
		virtual	void						setActor(const ActorPtr& pActor) override;
		virtual void                        createComRootNodeImpl() override;
		virtual void						NotifiChildImp(int changeMask) override;

		bool								PickHelper(const Echo::Ray& ray); // 拾取辅助对象，返回拾取到的点索引
		bool								GetIsEditing() const { return m_IsEditing; }
		void 								SetIsEditing(bool isEditing) { m_IsEditing = isEditing; }
		
	public:
		int                                 m_BindPtclSysIndex = -1;	// 绑定的粒子组件索引
		std::vector<ActorComponentPtr>		m_PtclSysComps;			// 粒子组件列表
		size_t								m_PtclSysCount = 0;			// comp所属Actor内的粒子系统数量

		std::vector<Vector3>				m_Points_Temp;				// 点世界坐标列表

		
		
	private:
		DBMatrix4							m_actorTransform;			// comp所属Actor的变换矩阵
		bool								m_IsHelperVisible = false;	// 是否显示辅助线
		bool								m_IsEditing = false;		// 是否正在编辑	
		Echo::ManualObject*					m_LineHelper = nullptr;	
		std::vector<SphereHelper*>			m_PointHelpers;				// 控制点辅助显示对象列表
		size_t 								m_PickedHelperIndex = -1;	// 当前拾取到的辅助对象索引
		//SphereHelper*						m_PickedHelper = nullptr;	// 当前拾取到的辅助对象
		
		Echo::PolylineComponentInfo*		m_Info = nullptr;
		Echo::SceneManager*					mSceneManager = nullptr;
		
	};

	class _EchoComponentExport PolylineComponentFactory : public Echo::ActorComFactory
	{
		static_assert(std::is_same<decltype(ALL_MOVEABLE), Echo::ActorCOMMoveableType>::value, "MOVEABLE must be ActorCOMMoveableType type!");
	public:
		PolylineComponentFactory();
		virtual ~PolylineComponentFactory();
		virtual EchoSharedPtr<ActorComponent> create(const Echo::ActorPtr& pActor) override
		{
			return EchoSharedPtr<ActorComponent>(new PolylineComponent(pActor, ALL_MOVEABLE), Echo::ActorComponentPtrDeleter);
		}
		virtual ActorComponentInfo* createComponentInfo() override
		{
			return new PolylineComponentInfo(mType, ALL_MOVEABLE);
		}

	};

	class _EchoComponentExport EchoPolylineProFuncImp : public ProtocolComponent
	{

	};

	class _EchoComponentExport EchoPolylineScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoPolylineScript() = default;
		EchoPolylineScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {
		}
	public:
		static EchoPolylineScript Script_GetPolylineComponent(const EchoActorScript& pActor, const std::string& name) {
			return EchoPolylineScript(pActor, name);
		}
	};

	
}


#endif
