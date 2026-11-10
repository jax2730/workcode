// Author: anhaoxiang

#ifndef __EchoLightComponent_h__ 
#define __EchoLightComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"
#include "EchoLight.h"

#include "EchoComInfoDefineMacro.h"
#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"
#include "EchoSubEntity.h"

namespace Echo {

	class LightComponentInfo;

	struct lightComponentDataHandle
	{
		Vector4      mOriginColor = Vector4(1.f,1.f,1.f,1.f);
		uint8        mPriority = 1;
		float        mInRadius = 0.f;
		float        mOutRadius = 1.f;
		float        mIntensity = 1.f;
		bool         mIsUseTod = true;
		bool         mIsProduceShadow = false;
	};

	class _EchoComponentExport LightComponent : public NodeActorComponent {
		DECLAREACTORCOMTYPE_H(LightComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		void					   init(ActorComponentInfo* info) override;
		void                       unInit() override;
		void                       bindNode() override;
		
		virtual void               initRenderObject(ActorComponentInfo* info) override;
		virtual void               initHelperObject(ActorComponentInfo* info) override;
		virtual void               setTimelineAttribute(std::vector<float>&,const Name&) override;

		virtual void			   copyTo(const ActorComponentPtr& actCom) override;
		virtual void               showEditorHelper(bool isShow = true)override;
		virtual void               updateCombinedName() override;
		virtual void               visibleStateChanged(bool isVisible) override;
		virtual void			   NotifiChildImp(int changeMask) override;
		void                       setPointLightColor(const Vector4& color);     //设置颜色
	    const Vector4&             getPointLightColor();                         //获取颜色
		void                       setLightIntensity(float intensity_);          //设置光照强度
		float                      getLightIntensity() ;                         //获取光源强度
		void                       setRadius(const float& in_,const float& out_); //设置光源内外半径，最好不要同时使用该函数和设置光源内半径，外半径函数，他们设置的是同一份值
		void                       setInRadius(const float& in_);                 //设置光源内半径  
		void                       setOutRadius(const float& out_);               //设置光源外半径
		float                      getInRadius();                                 //获取光源内半径
		float                      getOutRadius();                                //获取光源外半径
		void                       setPriority(LightIlluminatePriority);          //设置光源优先级
		LightIlluminatePriority    getPriority();                                 //获取光源优先级 
		    
		void                       setIsUseTod(bool isUse_);
		bool                       getIsUseTod() { return mContext.mIsUseTod; }

		void                       setIsProduceShadow(bool isUse_);
		bool                       getIsProduceShadow() { return mContext.mIsProduceShadow; }

#ifdef ECHO_EDITOR
		void                       showHelper(bool isshow_);
		void                       showLightEnity(bool isshow_);
		void                       CreateHelper();
		void                       destroyHelperAndLightEntity();
		void                       setHelperRadius();
#endif
		virtual void               createComRootNodeImpl() override;

#ifdef ECHO_EDITOR
		void                       setPointLightSelectedColor();
		void                       restorePointLightHelperColor();
#endif 

	private:
		Light*                     mPointLight = nullptr;  //点光源的实例
#ifdef ECHO_EDITOR
		HelperObject*              mHelperObj = nullptr;   //辅助物体的实例
		Entity*                    mEntity = nullptr;
		SceneNode*                 childNode_ = nullptr;
#endif
		lightComponentDataHandle   mContext;

		static String				POINTLIGHTMESH;
	};

	class _EchoComponentExport LightComponentInfo : public ActorComponentInfo {
	public:
		LightComponentInfo() {}
		LightComponentInfo(const Name& name , ActorCOMMoveableType isMoveable );
		virtual ~LightComponentInfo();
		//导出json
		virtual void  ExportData(cJSON* jsonNode) override;
		//读取json
		virtual void  ImportData(cJSON* jsonNode) override;
		virtual void  Overlay(ActorComponentInfo* srcInfo) override;
		virtual void  CopyTo(ActorComponentInfo * info) override;
		virtual void  OverWrite() override;
	public:
		lightComponentDataHandle mContext;
		EDITOR_REFLECT
	
	};

	DEFINECOMFACTORY(LightComponent, ALL_MOVEABLE);
	//脚本使用函数
	class _EchoComponentExport EchoLightScript : public EchoActorComponentScript {
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoLightScript() = default;
		EchoLightScript(const EchoActorScript& pActor, const std::string& name);
		EchoLightScript(const ActorComponentPtr& scriptCom);

		static EchoLightScript                 Script_GetLightComponent(ActorPtr pActor, const std::string& name);
	};
}

#endif
