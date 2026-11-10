// Author: anhaoxiang

#ifndef __EchoSpotSpotLightComponent_h__ 
#define __EchoSpotSpotLightComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"
#include "EchoMovableObject.h"
#include "EchoManualObject.h"
#include "EchoLight.h"
#include "EchoProtocolComponent.h"

namespace Echo {

	struct SpotLightComponentHandle {
		ColorValue   mColor = ColorValue::White;
		int          mPriority = 1;
		float        mInRadius = 0.f;
		float        mOutRadius = 1.f;
		float        mIntensity = 1.f;
		float        mInnerAngle = 20.f;
		float        mOuterAngle = 30.f;
		Vector3      mDirection = Vector3(0.f, 0.f, 1.f);
		bool         mIsUseTodChange = true;
		bool         mIsProduceShadow = false;
	};

	class SpotLightComponentInfo;
	class _EchoComponentExport SpotLightComponent : public NodeActorComponent {
		DECLAREACTORCOMTYPE_H(SpotLightComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		virtual void			  init(ActorComponentInfo* info) override;
		virtual void              unInit() override;
		virtual void			  copyTo(const ActorComponentPtr& actCom) override;
		virtual void              showEditorHelper(bool isShow = true)override;
		virtual void              updateCombinedName() override;
		virtual void              visibleStateChanged(bool isVisible) override;
		virtual void			  NotifiChildImp(int changeMask) override;

		virtual void              initRenderObject(ActorComponentInfo* info) override;
		virtual void              initHelperObject(ActorComponentInfo* info) override;

		void                      setSpotLightColor(const ColorValue& color);    //设置颜色
		const ColorValue&         getSpotLightColor();                           //获取颜色
		void                      setIntensity(float intensity_);                //设置光照强度
		float                     getIntensity();                                //获取光源强度
		void                      setInRadius(float in_);                        //设置光源内半径  
		void                      setOutRadius(float out_);                      //设置光源外半径
		float                     getInRadius();                                 //获取光源内半径
		float                     getOutRadius();                                //获取光源外半径
		void                      setPriority(int priority_);                    //设置光源优先级
		int                       getPriority();                                 //获取光源优先级 
		void                      setInnerAngle(float in_);
		float                     getInnerAngle();
		void                      setOuterAngle(float out_);
		float                     getOuterAngle();
		void                      setDirection(const Vector3& direction_);
		Vector3                   getDirection();
		void                      showHelper(bool isshow_);
		void                      setIsUseTodChange(bool isChange_);
		bool                      getIsUseTodChange() { return mContext.mIsUseTodChange; }
		void                      setIsProduceShadow(bool isUse_);
		bool                      getIsProduceShadow() { return mContext.mIsProduceShadow; }

		void                      remakeSate();
		virtual void              createComRootNodeImpl() override;     

		virtual void              setTimelineAttribute(std::vector<float>&,const Name&) override;

		void                      drawSpotLight();
#ifdef ECHO_EDITOR
		void                      createHelper();
		void                      destroyHelper();
		void                      drawCircle(const float radius, const float zOffset, const Echo::ColorValue& colour);
		void                      setSpotLightSelectedColor();
		void                      restoreSpotLightHelperColor();
#endif 

	private:
		Light*                    mSpotLight = nullptr;

#ifdef ECHO_EDITOR
		ManualObject*             mHelperObj = nullptr;
		SceneNode*				  mHelperNode = nullptr;


		const unsigned int        gCircleVertexCount = 16;
		float                     mOutRadius_;
		Radian                    mInnerAngle_ = Degree(20.f);
		Radian                    mOuterAngle_ = Degree(30.f);
		ColorValue                mColour1 = ColorValue::White;
		ColorValue                mColour2 = ColorValue::Black;
#endif
		SpotLightComponentHandle  mContext;

		static String				POINTLIGHTMESH;
	};

	class _EchoComponentExport SpotLightComponentInfo : public ActorComponentInfo {
	public:
		SpotLightComponentInfo() {}
		SpotLightComponentInfo(const Name& name, ActorCOMMoveableType isMoveable);
		virtual ~SpotLightComponentInfo();
		//导出json
		virtual void  ExportData(cJSON* jsonNode) override;
		//读取json
		virtual void  ImportData(cJSON* jsonNode) override;
		virtual void  CopyTo(ActorComponentInfo* info) override;
		virtual void  OverWrite() override;
	public:
		SpotLightComponentHandle  mContext;
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(SpotLightComponent, ALL_MOVEABLE)
	//脚本使用函数
	class _EchoComponentExport EchoSpotLightScript : public EchoActorComponentScript {
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoSpotLightScript() = default;
		EchoSpotLightScript(const EchoActorScript& pActor, const std::string& name);

		static EchoSpotLightScript                 Script_GetSpotLightComponent(ActorPtr pActor, const std::string& name);
	};

	class _EchoComponentExport EchoSpotLightComponentProFuncImp
	{
	public:
		static void initProtocolFunction();
		static void uninitProtocolFunction();

		//static Echo::ProtocolResult  ProCom_QuaternionToSpotDirection(void* quaternion_, ProtocolHandle comHandle);
	};
}

#endif
