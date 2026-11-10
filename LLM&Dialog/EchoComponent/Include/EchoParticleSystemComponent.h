//-----------------------------------------------------------------------------
// File:  EchoParticleSystemComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-8-24
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __ECHOPARTICLESYSTEMCOMPONENT_H__
#define __ECHOPARTICLESYSTEMCOMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
//#include "EchoEffectEmitter.h"
#include "EchoEnginePrerequisites.h"
#include "Effect/EchoEffectHolder.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "Actor/EchoNodeComponent.h"

#include "EchoComDefineMacro.h"
#include <random>

namespace Echo {

	class ParticleSystemComponentInfo;

	// 粒子系统组件
	class _EchoComponentExport ParticleSystemComponent : public NodeActorComponent, public EffectDeleteListen {
		DECLAREACTORCOMTYPE_H(ParticleSystemComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		void								init(ActorComponentInfo* info) override;
		void								unInit() override;
		void								bindNode() override;
		void								setActor(const ActorPtr&  pActor) override;
		void								connectCom() override;
		void								initRenderObject(ActorComponentInfo* info) override;

	/********************************************************************************************************************/
		void								setTimelineAttribute(std::vector<float>&, const Name&) override;

		void								visibleStateChanged(bool visibleState) override;
		void								updatePlaySpeed() override;
	/********************************************************************************************************************/

	// 准备的新替代接口
	// -------------------------------------------------------------------------------------------------------------------
	public:
		void								LoadParticle(const std::string& path, bool synchronous = false, SceneManager* Creator = nullptr, float priority = -9999.0f, bool UseCache = false, bool bIsPrior = false, bool bCreateByScene = false, \
			EffectVanishListen* inParticleVanishListen = NULL, bool bCreateByAnimation = false, float lifeTime = 0.0f, uint8 particleType = 0);		void								LoadParticle(const ParticleHolderCreatorDesc& desc);
		void								LoadParticle(const EffectHolder::CreateInfo& create_info);
		Name								GetResourceName() const;

		void								PlayParticle(bool ispause);
		void								ResetParticle();

		float								GetLifeTime();
		void								SetLifeTime(float val);

		float								GetPlaySpeed();
		void								SetPalySpeed(float speed);

		void								SetPalyTime(float time);
		float								GetPlayTime();

		void								SetPause(bool is_pause);
		bool								GetPause() const;

		bool								GetLoop();
		void								SetLoop(bool isLoop);

		void								SetLiveForever(bool inLiveForever);
		bool								GetLiveForever() const;

		//设置粒子链的起始点坐标 double 版
		void								SetChainBeginEnd(const DVector3& begin, const DVector3& end);
		void								SetHermitCurveTrail(std::vector<DVector3>* vertices);
		void								SetInputRoleObject(RoleObject* mRoleObject);
		void								UpdateInputRoleWithParent();
	public:
		// 主要是编辑器使用
		void								UseInfoParams();
		void								UsePEFDataUpdateInfoParams(ParticleSystemComponentInfo* ParticleInfo);
	// 不容易见名知意的接口，将来可能废弃。
	// -------------------------------------------------------------------------------------------------------------------
	public:
		// 没用的函数，为了保证客户端的编译通过。是之前客户端天气里面clientWeatherManager::setPriority调到粒子的原来基类里面去了。等之后他们改掉之后在删掉。
		void								setPriority(int prior) { (void)prior; }
		// 废弃 使用 GetResourceName
		Name								GetParticleName();
		// 废弃 使用 SetLoop
		void								LoopParticle(bool isLoop);
		void								DelParticle();

		bool								IsVanished();
		bool								isEffectCreateFinish();
		//设置相机模式
		void								SetCameraCullMode(uint8_t cullMode);
		//设置渲染距离
		void								SetRenderingDistance(float renderDis);
		void								SetVanishParticle();
		//设置优先级
		void								SetParticlePriority(float val);
		//更换贴图
		void								ChangeDiffuseTexture(const char* path, const char* name);

		void								PlayEmitterParticle(bool isPlay);
		void								StopEmitterParticle();
		void								StartEmitterParticle();
		//bool								GetEmitterSetting();

		//是否渲染到renderwindow上
		void								EnableUIRender(bool val);

		void								SetFollowPostion(bool isfollow);
		void								SetFollowOrientation(bool isfollow);
		void								SetFollowScale(bool isfollow);
		void								SetManualPosition(bool isManual);
		void								SetManualOrientation(bool isManual);
		void								SetManualScale(bool isManual);


		Vector4								GetColor();
		void								SetColor(const Vector4& _color);
		void								SetColor_RGB(float R,float G,float B,float A);
		void								SetIllusionAlpha(float val);

		void								SetBillboardSpinRot(float val);

		bool								IsVanished()const;
		void								VanishParticle();

		//设置粒子链的起始点坐标
		void								SetChainBeginEnd(const Vector3& _vBegin, const Vector3& _vEnd);

		void								addCollisionParam(int index, const Vector3& mColliderPosition, float mColliderRadius, const Vector3& mColliderVelocity);
		void								updateCollisionParam(int index, const Vector3& mColliderPosition, float mColliderRadius, const Vector3& mColliderVelocity);
		void								removeCollisionParam(int index);


		bool								idPrepared();
		void								StartPrepareRes();

		void								AddRenderableToRenderQueueGroup(SceneManager* pScene, RenderQueueGroupID id, int32 priority);
		void								SetHermitCurveTrail(std::vector<Vector3>* vertices);
		void								NotifyCurrentCamera(SceneManager* pScene);
		void								UpdateVBIB();

		bool								HaveParticle();
		ActorPtr 							GetActor() { return m_pActor; };

		/**
		 * @brief If some pointers are nullptr, then 0.0f is returned.
		 * @return Get the particle delay deletion time.
		*/
		float								GetParticleDelayDeleteTime() const noexcept;

		/**
		 * @brief If particle object are nullptr, then false is returned.
		*/
		bool								SetEffectObjectIsEnableWaitDestoryTime(bool enable) noexcept;
		bool								GetEffectObjectIsEnableWaitDestoryTime() const noexcept;

		EffectHolder*						getParticle() { return mParticleHolder; }

		void								SetClientLogMessage(const Name& str) { mClientLogMessage = str; }

		void								setFPState(Echo::ENUM_FP_STATE val);

		enum class _EchoComponentExport ErrorCode
		{
			NoError,
			TypeError,
			ParticleHolderIsNull,
			EffectObjectIsNull,
			ElementObjectIsNull,
			OtherError
		};

		std::pair<ErrorCode, Vector3>		getElementWorldPositionByIndex(unsigned index);
		std::pair<ErrorCode, Vector3>		getElementWorldPositionByName(const std::string& name);

		ErrorCode							setRingEmitterRadiusByElementIndex(unsigned index, float val);
		ErrorCode							setRingEmitterRadiusByElementName(const std::string& name, float val);
	
		ErrorCode							setRingEmitterInnerRadiusByElementIndex(unsigned index, float val);
		ErrorCode							setRingEmitterInnerRadiusByElementName(const std::string& name, float val);

		ErrorCode							setAllElementRingEmitterRadius(float val);
		ErrorCode							setAllElementRingEmitterInnerRadius(float val);

		void								UpdateParamValue(float v);

	protected:
		virtual void						EditorPlayImp() override;
		virtual void						EditorPauseImp(bool pause) override;
		virtual void						EditorStopImp() override;

	private:
		void								onParticleDelete() override;

	private:
		bool								mPlaySpeed = 1.f;	// 暗号翔记得时间轴备用的值。
		EffectHolder*						mParticleHolder = nullptr;
		Name								mClientLogMessage = Name::g_cEmptyName;
	};
	//===========================================================================================================================
	class _EchoComponentExport ParticleSystemComponentInfo : public ActorComponentInfo {
		friend class ParticleSystemComponent;
	public:
		ParticleSystemComponentInfo() {}
		ParticleSystemComponentInfo(const Name& type, ActorCOMMoveableType movable = ALL_MOVEABLE);
		~ParticleSystemComponentInfo();

		void								ExportData(cJSON* jsonNode) override;
		void								ImportData(cJSON* jsonNode) override;
		void								CopyTo(ActorComponentInfo* p_actor_com_info) override;
		virtual void						Overlay(ActorComponentInfo* comInfo) override;
		virtual void						OverWrite() override;

#ifdef ECHO_EDITOR
		bool								getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		std::string							m_path = "";

		Vector4								m_color{ 0.0f, 0.0f, 0.0f, 1.0f };
		float								m_playspeed { 1.0f };
		float								m_lifetime{ 5.0f };
		bool								m_is_forever{ false };
		bool								m_is_loop{ false };

		EDITOR_REFLECT
	};

	// 粒子系统组件工厂类
	DEFINECOMFACTORY(ParticleSystemComponent, ALL_MOVEABLE)
}

#endif

