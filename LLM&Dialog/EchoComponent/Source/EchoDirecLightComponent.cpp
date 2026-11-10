#include "EchoComponentStableHeaders.h"
#include "EchoDirecLightComponent.h"

#include "EchoActorProFuncImp.h"
#include "EchoTODComponentManager.h"
#include "EchoTODManager.h"
#include "EchoSun.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo
{
	Name direcLightPBRMainLightColorName_ = Name("DirecLightPBRMainLightColor");
	Name direcLightPBRSkyLightColorName_ = Name("DirecLightPBRSkyLightColor");
	Name direcLightPBRGroundLightColorName_ = Name("DirecLightPBRGroundLightColor");
	Name direcLightRoleMainLightColorName_ = Name("DirecLightRoleMainLightColor");
	Name direcLightRoleSkyLightColorName_ = Name("DirecLightRoleSkyLightColor");
	Name direcLightRoleGroundLightColorName_ = Name("DirecLightRoleGroundLightColor");
	Name direcLightPBRMainLightPowerName_ = Name("DirecLightPBRMainLightPower");
	Name direcLightPBRSkyLightPowerName_ = Name("DirecLightPBRSkyLightPower");
	Name direcLightPBRGroundLightPowerName_ = Name("DirecLightPBRGroundLightPower");
	Name direcLightRoleMainLightPowerName_ = Name("DirecLightRoleMainLightPower");
	Name direcLightRoleSkyLightPowerName_ = Name("DirecLightRoleSkyLightPower");
	Name direcLightRoleGroundLightPowerName_ = Name("DirecLightRoleGroundLightPower");
	Name direcLightMainLightPitchAngleName_ = Name("DirecLightMainLightPitchAngle");
	Name direcLightMainLightYawAngleName_ = Name("DirecLightMainLightYawAngle");
	Name direcLightMainLightColorName_ = Name("MainLightColor");
	Name direcLightMainLightColorPowerName_ = Name("MainLightColorPower");
	Name direcLightMainSkyLightColorName_ = Name("MainSkyLightColor");
	Name direcLightMainSkyLightColorPowerName_ = Name("MainSkyLightColorPower");
	Name direcLightMainGroundLightColorName_ = Name("MainGroundLightColor");
	Name direcLightMainGroundLightColorPowerName_ = Name("MainGroundLightColorPower");
	Name direcLightMainLightHDRPowerName_ = Name("MainLightHDRPower");
	Name direcLightMainSkyLightHDRPowerName_ = Name("MainSkyLightHDRPower");
	Name direcLightMainGroundLightHDRPowerName_ = Name("MainGroundLightHDRPower");
	Name direcLightRoleMainLightHDRPowerName_ = Name("RoleMainLightHDRPower");
	Name direcLightRoleSkyLightHDRPowerName_ = Name("RoleSkyLightHDRPower");
	Name direcLightRoleGroundLightHDRPowerName_ = Name("RoleGroundLightHDRPower");
	Name direcLightPBRMainLightHDRPowerName_ = Name("PBRMainLightHDRPower");
	Name direcLightPBRSkyLightHDRPowerName_ = Name("PBRSkyLightHDRPower");
	Name direcLightPBRGroundLightHDRPowerName_ = Name("PBRGroundLightHDRPower");
	Name direcLightPBRAmbientBlendThresholdName_ = Name("PBRAmbientBlendThreshold");
	Name direcLightRoleAmbientBlendThresholdName_ = Name("RoleAmbientBlendThreshold");
	Name direcLightViewLightPercent = Name("ViewLightPercent");

	Name direcLightViewLightPercent1 = Name("LViewLightPercent");
	Name direcLightViewLightPercent2 = Name("LPBRViewLightPercent");
	Name direcLightViewLightPercent3 = Name("LRoleViewLightPercent");

	DECLAREACTORCOMTYPE_CPP(DirecLightComponent)

		EDITOR_REFLECT_BEGIN(DirecLightComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRMainLightColor, "PBRMainLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRSkyLightColor, "PBRSkyLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRGroundLightColor, "PBRGroundLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleMainLightColor, "RoleMainLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleSkyLightColor, "RoleSkyLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleGroundLightColor, "RoleGroundLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRMainLightPower, "PBRMainLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRSkyLightPower, "PBRSkyLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRGroundLightPower, "PBRGroundLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRAmbientBlendThreshold, "PBRAmbientBlendThreshold", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleMainLightPower, "RoleMainLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleSkyLightPower, "RoleSkyLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleGroundLightPower, "RoleGroundLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleAmbientBlendThreshold, "RoleAmbientBlendThreshold", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainLightYawAngle, "MainLightYawAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainLightPitchAngle, "MainLightPitchAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainLightColor, "MainLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainLightColorPower, "MainLightColorPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainSkyLightColor, "MainSkyLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainSkyLightPower, "MainSkyLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainGroundLightColor, "MainGroundLightColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainGroundLightPower, "MainGroundLightPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	    
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainLightHDRPower, "MainLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainSkyLightHDRPower, "MainSkyLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMainGroundLightHDRPower, "MainGroundLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRMainLightHDRPower, "PBRMainLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRSkyLightHDRPower, "PBRSkyLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPBRGroundLightHDRPower, "PBRGroundLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleMainLightHDRPower, "RoleMainLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleSkyLightHDRPower, "mRoleSkyLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRoleGroundLightHDRPower, "mRoleGroundLightHDRPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_ViewLightPercent, "ViewLightPercent", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_PBRViewLightPercent, "PBRViewLightPercent", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_RoleViewLightPercent, "RoleViewLightPercent", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	DirecLightComponent::DirecLightComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ActorComponent(pActor,type)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_MAIN_LIGHT_YAW,			direcLightMainLightYawAngleName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_MAIN_LIGHT_PITCH,			direcLightMainLightPitchAngleName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_MAIN_LIGHT_COLOR,			direcLightMainLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_MAIN_LIGHT_COLOR_POWER,	direcLightMainLightColorPowerName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_MAIN_LIGHT_HDR_POWER,		direcLightMainLightHDRPowerName_.toString(),	1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_LIGHT_COLOR,			direcLightMainSkyLightColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_LIGHT_COLOR_POWER,		direcLightMainSkyLightColorPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_LIGHT_HDR_POWER,		direcLightMainSkyLightHDRPowerName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GROUND_LIGHT_COLOR,		direcLightMainGroundLightColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GROUND_LIGHT_COLOR_POWER,	direcLightMainGroundLightColorPowerName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_GROUND_LIGHT_HDR_POWER,	direcLightMainGroundLightHDRPowerName_.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_MAIN_LIGHT_COLOR,			direcLightPBRMainLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_MAIN_LIGHT_COLOR_POWER,	direcLightPBRMainLightPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_MAIN_LIGHT_HDR_POWER,		direcLightPBRMainLightHDRPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_AMBIENT_BLEND_THRESHOLD,	direcLightPBRAmbientBlendThresholdName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_SKY_LIGHT_COLOR,			direcLightPBRSkyLightColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_SKY_LIGHT_COLOR_POWER,		direcLightPBRSkyLightPowerName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_SKY_LIGHT_HDR_POWER,		direcLightPBRSkyLightHDRPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_GROUND_LIGHT_COLOR,		direcLightPBRGroundLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_GROUND_LIGHT_COLOR_POWER,	direcLightPBRGroundLightPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_GROUND_LIGHT_HDR_POWER,	direcLightPBRGroundLightHDRPowerName_.toString(),	1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_MAIN_LIGHT_COLOR,			direcLightRoleMainLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_MAIN_LIGHT_COLOR_POWER,	direcLightRoleMainLightPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_MAIN_LIGHT_HDR_POWER,		direcLightRoleMainLightHDRPowerName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_AMBIENT_BLEND_THRESHOLD,	direcLightRoleAmbientBlendThresholdName_.toString(),1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_SKY_LIGHT_COLOR,			direcLightRoleSkyLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_SKY_LIGHT_COLOR_POWER,	direcLightRoleSkyLightPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_SKY_LIGHT_HDR_POWER,		direcLightRoleSkyLightHDRPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_GROUND_LIGHT_COLOR,		direcLightRoleGroundLightColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_GROUND_LIGHT_COLOR_POWER,	direcLightRoleGroundLightPowerName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_GROUND_LIGHT_HDR_POWER,	direcLightRoleGroundLightHDRPowerName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_VIEW_LIGHT_PERCENT,			direcLightViewLightPercent1.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_PBR_VIEW_LIGHT_PERCENT,		direcLightViewLightPercent2.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_ROLE_VIEW_LIGHT_PERCENT,		direcLightViewLightPercent3.toString(),				1);
#endif // ECHO_EDITOR
	}

	DirecLightComponent::~DirecLightComponent()
	{

	}

	void DirecLightComponent::init(ActorComponentInfo * info)
	{
		Name sceneName = g_MainActorManagerName;
		if (m_pActor)
		{
			SceneManager* sMgr = m_pActor->GetSceneManager();
			if (sMgr)
				sceneName = sMgr->getName();
		}
		m_tMgr = TODComponentSystem::instance()->getTODComponentManager(sceneName);
		if(m_tMgr && getRealVisibleState())
			m_tMgr->addDirecLight(this);

		DirecLightComponentInfo* mDirecLightInfo = static_cast<DirecLightComponentInfo*>(info);
		if (mDirecLightInfo == nullptr)
			return;
		mContext = mDirecLightInfo->mContext;
	}

	void DirecLightComponent::unInit()
	{
		if(m_tMgr)
		{
			m_tMgr->deleteDireclight(this);
			m_tMgr->clearComInFinalComVec(this, T_DirecLight);
		}
		m_tMgr = nullptr;
	}
	

	void DirecLightComponent::copyTo(const ActorComponentPtr&  pcomp)
	{
	}

	void DirecLightComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (!m_tMgr)
			return;
		if (m_tMgr->getDirecLightIsUse() == false)
			return;
		if (m_tMgr->getCurrActiveDirecLightCom() != this)
			return;
		m_tMgr->setTodComChangeState(T_DirecLight);
		bool flag = false;

		if (name == direcLightPBRMainLightColorName_)
		{ 
			mContext.mPBRMainLightColor = Vector4(data[0], data[1], data[2], data[3]);
			setPBRMainLightColor();
		}
		else if (name == direcLightPBRSkyLightColorName_)
		{
			mContext.mPBRSkyLightColor = Vector4(data[0], data[1], data[2], data[3]);
			setPBRSkyLightColor();
		}	
		else if (name == direcLightPBRGroundLightColorName_)
		{
			mContext.mPBRGroundLightColor = Vector4(data[0], data[1], data[2], data[3]);
		    setPBRGroundLightColor();
		}	
		else if (name == direcLightRoleMainLightColorName_)
		{
			mContext.mRoleMainLightColor = Vector4(data[0], data[1], data[2], data[3]);
			setRoleMainLightColor();
		}
		else if (name == direcLightRoleSkyLightColorName_)
		{
			mContext.mRoleSkyLightColor = Vector4(data[0], data[1], data[2], data[3]);
			setRoleSkyLightColor();
		}
		else if (name == direcLightRoleGroundLightColorName_)
		{
			mContext.mRoleGroundLightColor = Vector4(data[0], data[1], data[2], data[3]);
			setRoleGroundLightColor();
		}
		else if (name == direcLightPBRMainLightPowerName_)
		{
			mContext.mPBRMainLightPower = data[0];
			setPBRMainLightColor();
		}
		else if (name  == direcLightPBRSkyLightPowerName_)
		{
			mContext.mPBRSkyLightPower = data[0];
			setPBRSkyLightColor();
		}
		else if (name == direcLightPBRGroundLightPowerName_)
		{
			mContext.mPBRGroundLightPower = data[0];
			setPBRGroundLightColor();
		}
		else if (name == direcLightRoleMainLightPowerName_)
		{
			mContext.mRoleMainLightPower = data[0];
			setRoleMainLightColor();
		}
		else if (name == direcLightRoleSkyLightPowerName_)
		{
			mContext.mRoleSkyLightPower = data[0];
			setRoleSkyLightColor();
		}
		else if (name == direcLightRoleGroundLightPowerName_)
		{
			mContext.mRoleGroundLightPower = data[0];
			setRoleGroundLightColor();
		}
		else if (name == direcLightMainLightPitchAngleName_)
		{
			data[0] = std::max(-90.f, data[0]);
			data[0] = std::min(0.f, data[0]);
			mContext.mMainLightPitchAngle = data[0];
			flag = true;
		}
		else if (name == direcLightMainLightYawAngleName_)
		{
			data[0] = std::min(360.f, data[0]);
			data[0] = std::max(0.f, data[0]);
			mContext.mMainLightYawAngle = data[0];
			flag = true;
		}
		else if (name == direcLightMainLightColorName_)
		{
			mContext.mMainLightColor = ColorValue(data[0], data[1], data[2], data[3]);
			setMainLightColor();
		}
		else if (name == direcLightMainLightColorPowerName_)
		{
			mContext.mMainLightColorPower = data[0];
			setMainLightColor();
		}
		else if (name == direcLightMainSkyLightColorName_)
		{
			mContext.mMainSkyLightColor = ColorValue(data[0], data[1], data[2], data[3]);
			setMainSkyLightColor();
		}
		else if (name == direcLightMainSkyLightColorPowerName_)
		{
			mContext.mMainSkyLightPower = data[0];
			setMainSkyLightColor();
		}
		else if (name == direcLightMainGroundLightColorName_)
		{
			mContext.mMainGroundLightColor = ColorValue(data[0], data[1], data[2], data[3]);
			setMainGroundLightColor();
		}
		else if (name == direcLightMainGroundLightColorPowerName_)
		{
			mContext.mMainGroundLightPower = data[0];
			setMainGroundLightColor();
		}
		else if (name == direcLightMainLightHDRPowerName_)
		{
			setMainLightHDRPower(data[0]);
		}
		else if (name == direcLightMainSkyLightHDRPowerName_)
		{
			setMainSkyLightHDRPower(data[0]);
		}
		else if (name == direcLightMainGroundLightHDRPowerName_)
		{
			setMainGroundLightHDRPower(data[0]);
		}
		else if (name == direcLightRoleMainLightHDRPowerName_)
		{
			setRoleMainLightHDRPower(data[0]);
		}
		else if (name == direcLightRoleSkyLightHDRPowerName_)
		{
			setRoleSkyLightHDRPower(data[0]);
		}
		else if (name == direcLightRoleGroundLightHDRPowerName_)
		{
			setRoleGroundLightHDRPower(data[0]);
		}
		else if (name == direcLightPBRMainLightHDRPowerName_)
		{
			setPBRMainLightHDRPower(data[0]);
        }
		else if (name == direcLightPBRSkyLightHDRPowerName_)
		{
			setPBRSkyLightHDRPower(data[0]);
		}
		else if (name == direcLightPBRGroundLightHDRPowerName_)
		{
			setPBRGroundLightHDRPower(data[0]);
		}
		else if (name == direcLightPBRAmbientBlendThresholdName_)
		{
			setPBRAmbientBlendThreshold(data[0]);
		}
		else if (name == direcLightRoleAmbientBlendThresholdName_)
		{
			setRoleAmbientBlendThreshold(data[0]);
		}
		else if (name == direcLightViewLightPercent)
		{
			setViewLightPercent(data[0], true);
		}
		else if (name == direcLightViewLightPercent1)
		{
			setViewLightPercent(data[0]);
		}
		else if (name == direcLightViewLightPercent2)
		{
			setPBRViewLightPercent(data[0]);
		}
		else if (name == direcLightViewLightPercent3)
		{
			setRoleViewLightPercent(data[0]);
		}

		if (flag)
		{
			setMainLightDirection();
		}

	}

	void DirecLightComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->addDirecLight(this);
		else
			m_tMgr->deleteDireclight(this);

	}

	void DirecLightComponent::setMainLightYawAngle(float yawAngle_)
	{
		mContext.mMainLightYawAngle = yawAngle_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainLightDirection();
	}

	void DirecLightComponent::setMainLightPitchAngle(float pitchAngle_)
	{
		mContext.mMainLightPitchAngle = pitchAngle_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainLightDirection();
	}

	void DirecLightComponent::setInitPBRMainLight(const Vector4& color_)
	{
		mContext.mPBRMainLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRMainLightColor();
	}

	void DirecLightComponent::setInitPBRSkyLight(const Vector4& color_)
	{
		mContext.mPBRSkyLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRSkyLightColor();
	}

	void DirecLightComponent::setInitPBRGroundLight(const Vector4& color_)
	{
		mContext.mPBRGroundLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRGroundLightColor();
	}

	void DirecLightComponent::setInitRoleMainLight(const Vector4& color_)
	{
		mContext.mRoleMainLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleMainLightColor();
	}

	void DirecLightComponent::setInitRoleSkyLight(const Vector4& color_)
	{
		mContext.mRoleSkyLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleSkyLightColor();
	}

	void DirecLightComponent::setInitRoleGroundLight(const Vector4& color_)
	{
		mContext.mRoleGroundLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleGroundLightColor();
	}

	void DirecLightComponent::setInitMainLightColor(const ColorValue& color_)
	{
		mContext.mMainLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainLightColor();
	}

	void DirecLightComponent::setInitMainSkyLightColor(const ColorValue& color_)
	{
		mContext.mMainSkyLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainSkyLightColor();
	}

	void DirecLightComponent::setInitMainGroundLightColor(const ColorValue& color_)
	{
		mContext.mMainGroundLightColor = color_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainGroundLightColor();
	}

	void DirecLightComponent::setPBRMainLightPower(float power_)
	{
		mContext.mPBRMainLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRMainLightColor();
	}

	void DirecLightComponent::setPBRSkyLightPower(float power_)
	{
		mContext.mPBRSkyLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRSkyLightColor();
	}

	void DirecLightComponent::setPBRGroundLightPower(float power_)
	{
		mContext.mPBRGroundLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setPBRGroundLightColor();
	}

	void DirecLightComponent::setRoleMainLightPower(float power_)
	{
		mContext.mRoleMainLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleMainLightColor();
	}

	void DirecLightComponent::setRoleSkyLightPower(float power_)
	{
		mContext.mRoleSkyLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleSkyLightColor();
	}

	void DirecLightComponent::setRoleGroundLightPower(float power_)
	{
		mContext.mRoleGroundLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setRoleGroundLightColor();
	}

	void DirecLightComponent::setMainSkyLightPower(float power_)
	{
		mContext.mMainSkyLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainSkyLightColor();
	}

	void DirecLightComponent::setMainLightPower(float power_)
	{
		mContext.mMainLightColorPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainLightColor();
	}

	void DirecLightComponent::setMainGroundLightPower(float power_)
	{
		mContext.mMainGroundLightPower = power_;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			setMainGroundLightColor();
	}

	void DirecLightComponent::setviewLightPercent()
	{
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
			{
				std::array<float, 3> percent;
				percent[0] = (mContext.m_ViewLightPercent);
				percent[1] = (mContext.m_PBRViewLightPercent);
				percent[2] = (mContext.m_RoleViewLightPercent);
				sMgr->setViewLightPercent(percent);
			}
		}
	}

	void DirecLightComponent::setViewLightPercent(float percent_, bool isOld)
	{
		mContext.m_ViewLightPercent = percent_;
		if (isOld)
		{
			mContext.m_PBRViewLightPercent = percent_;
			mContext.m_RoleViewLightPercent = percent_;
			if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
			{
				SceneManager* sMgr = m_tMgr->GetSceneManager();
				if (sMgr)
					sMgr->setViewLightPercent(percent_);
			}
		}
		else
			setviewLightPercent();
	}

	void DirecLightComponent::setPBRViewLightPercent(float percent_)
	{
		mContext.m_PBRViewLightPercent= percent_;
		setviewLightPercent();
	}

	void DirecLightComponent::setRoleViewLightPercent(float percent_)
	{
		mContext.m_RoleViewLightPercent = percent_;
		setviewLightPercent();
	}

	void DirecLightComponent::setPBRAmbientBlendThreshold(float value)
	{
		mContext.mPBRAmbientBlendThreshold = value;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setPBRAmbientBlendThreshold(value);
		}
	}

	void DirecLightComponent::setRoleAmbientBlendThreshold(float value)
	{
		mContext.mRoleAmbientBlendThreshold = value;
		if (m_tMgr && m_tMgr->getCurrActiveDirecLightCom() == this)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setRoleAmbientBlendThreshold(value);
		}
	}
	float DirecLightComponent::getPBRAmbientBlendThreshold()
	{
		return mContext.mPBRAmbientBlendThreshold;
	}
	float DirecLightComponent::getRoleAmbientBlendThreshold()
	{
		return mContext.mRoleAmbientBlendThreshold;
	}
	Vector4 DirecLightComponent::getInitPBRMainLight() 
	{
		return mContext.mPBRMainLightColor;
	}
	Vector4	DirecLightComponent::getInitPBRSkyLight()
	{
		return mContext.mPBRSkyLightColor;
	}
	Vector4 DirecLightComponent::getInitPBRGroundLight()
	{
		return mContext.mPBRGroundLightColor;
	}
	Vector4	DirecLightComponent::getInitRoleMainLight()
	{
		return mContext.mRoleMainLightColor;
	}
	Vector4	DirecLightComponent::getInitRoleSkyLight()
	{
		return mContext.mRoleSkyLightColor;
	}
	Vector4 DirecLightComponent::getInitRoleGroundLight()
	{
		return mContext.mRoleGroundLightColor;
	}

	const ColorValue& DirecLightComponent::getInitMainLightColor()
	{
		return mContext.mMainLightColor;
	}

	float DirecLightComponent::getMainLightPower()
	{
		return mContext.mMainLightColorPower;
	}

	const ColorValue& DirecLightComponent::getInitMainSkyLightColor()
	{
		return mContext.mMainSkyLightColor;
	}

	float DirecLightComponent::getMainSkyLightPower()
	{
		return mContext.mMainSkyLightPower;
	}

	const ColorValue& DirecLightComponent::getInitMainGroundLightColor()
	{
		return mContext.mMainGroundLightColor;
	}

	float DirecLightComponent::getMainGroundLightPower()
	{
		return mContext.mMainGroundLightPower;
	}

	void DirecLightComponent::setMainLightHDRPower(float power_)
	{
		mContext.mMainLightHDRPower = power_;
		setMainLightColor();
	}

	float DirecLightComponent::getMainLightHDRPower()
	{
		return mContext.mMainLightHDRPower;
	}

	void DirecLightComponent::setMainSkyLightHDRPower(float power_)
	{
		mContext.mMainSkyLightHDRPower = power_;
		setMainSkyLightColor();
	}

	float DirecLightComponent::getMainSkyLightHDRPower()
	{
		return mContext.mMainSkyLightHDRPower;
	}

	void DirecLightComponent::setMainGroundLightHDRPower(float power_)
	{
		mContext.mMainGroundLightHDRPower = power_;
		setMainGroundLightColor();
	}

	float DirecLightComponent::getMainGroundLightHDRPower()
	{
		return mContext.mMainGroundLightHDRPower;
	}

	void DirecLightComponent::setPBRMainLightHDRPower(float power_)
	{
		mContext.mPBRMainLightHDRPower = power_;
		setPBRMainLightColor();
	}

	float DirecLightComponent::getPBRMainLightHDRPower()
	{
		return mContext.mPBRMainLightHDRPower;
	}

	void DirecLightComponent::setPBRSkyLightHDRPower(float power_)
	{
		mContext.mPBRSkyLightHDRPower = power_;
		setPBRSkyLightColor();
	}

	float DirecLightComponent::getPBRSkyLightHDRPower()
	{
		return mContext.mPBRSkyLightHDRPower;
	}

	void DirecLightComponent::setPBRGroundLightHDRPower(float power_)
	{
		mContext.mPBRGroundLightHDRPower = power_;
		setPBRGroundLightColor();
	}

	float DirecLightComponent::getPBRGroundLightHDRPower()
	{
		return mContext.mPBRGroundLightHDRPower;
	}

	void DirecLightComponent::setRoleMainLightHDRPower(float power_)
	{
		mContext.mRoleMainLightHDRPower = power_;
		setRoleMainLightColor();
	}

	float DirecLightComponent::getRoleMainLightHDRPower()
	{
		return mContext.mRoleMainLightHDRPower;
	}

	void DirecLightComponent::setRoleSkyLightHDRPower(float power_)
	{
		mContext.mRoleSkyLightHDRPower = power_;
		setRoleSkyLightColor();
	}

	float DirecLightComponent::getRoleSkyLightHDRPower()
	{
		return mContext.mRoleSkyLightHDRPower;
	}

	void DirecLightComponent::setRoleGroundLightHDRPower(float power_)
	{
		mContext.mRoleGroundLightHDRPower = power_;
		setRoleGroundLightColor();
	}

	float DirecLightComponent::getRoleGroundLightHDRPower()
	{
		return mContext.mRoleGroundLightHDRPower;
	}

	void DirecLightComponent::setMainLightColor()
	{
		ColorValue tempColor = ColorValue::gammaToLinear(mContext.mMainLightColor) * mContext.mMainLightColorPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			tempColor *= mContext.mMainLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setMainLightColor(tempColor);
		}
	}

	void DirecLightComponent::setMainSkyLightColor()
	{
		ColorValue tempColor = ColorValue::gammaToLinear(mContext.mMainSkyLightColor) * mContext.mMainSkyLightPower;

		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			tempColor *= mContext.mMainSkyLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setSkyLightColour(tempColor);
		}
	}

	void DirecLightComponent::setMainGroundLightColor()
	{
		ColorValue tempColor = ColorValue::gammaToLinear(mContext.mMainGroundLightColor) * mContext.mMainGroundLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			tempColor *= mContext.mMainGroundLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setGroundLightColour(tempColor);
		}
	}

	//set and get from MainSceneManager
	void DirecLightComponent::setPBRMainLightColor()
	{
		const Vector4& tempColor = getInitPBRMainLight();
		ColorValue color_(tempColor.x, tempColor.y, tempColor.z, tempColor.w);
		ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mPBRMainLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mPBRMainLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setPBRMainLightColor(temp);
		}
	}

	

	void DirecLightComponent::setPBRSkyLightColor()
	{
		const Vector4& tempColor = getInitPBRSkyLight();
		ColorValue color_(tempColor.x, tempColor.y, tempColor.z, tempColor.w);
		ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mPBRSkyLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mPBRSkyLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setPBRSkyLightColour(temp);
		}

	}

	

	void DirecLightComponent::setPBRGroundLightColor()
	{
		const Vector4& tempColor = getInitPBRGroundLight();
		ColorValue color_(tempColor.x, tempColor.y, tempColor.z, tempColor.w);
		ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mPBRGroundLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mPBRGroundLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setPBRGroundLightColour(temp);
		}

	}


	void DirecLightComponent::setRoleMainLightColor()
	{
		const Vector4& tempColor = getInitRoleMainLight();
		ColorValue color_(tempColor.x, tempColor.y, tempColor.z, tempColor.w);
		ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mRoleMainLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mRoleMainLightHDRPower;
	
		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setRoleMainLightColor(temp);
		
		}
	}


	void DirecLightComponent::setRoleSkyLightColor()
	{
		ColorValue color_(mContext.mRoleSkyLightColor.x, mContext.mRoleSkyLightColor.y, mContext.mRoleSkyLightColor.z, mContext.mRoleSkyLightColor.w);
	    ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mRoleSkyLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mRoleSkyLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setRoleSkyLightColour(temp);
		}
	}


	void DirecLightComponent::setRoleGroundLightColor()
	{
		const Vector4& tempColor = getInitRoleGroundLight();
		ColorValue color_(tempColor.x, tempColor.y, tempColor.z, tempColor.w);
		ColorValue temp = ColorValue::gammaToLinear(color_) * mContext.mRoleGroundLightPower;
		bool bEnableHDR = false;
		RenderStrategy* pStrategy = Root::instance()->getRenderSystem()->getCurRenderStrategy();
		if (pStrategy && pStrategy->getPostProcessChain())
		{
			bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
		}
		if (bEnableHDR)
			temp *= mContext.mRoleGroundLightHDRPower;

		if (m_tMgr)
		{
			SceneManager* sMgr = m_tMgr->GetSceneManager();
			if (sMgr)
				sMgr->setRoleGroundLightColour(temp);
		}

	} 

	float DirecLightComponent::getPBRMainLightPower() 
	{
		return mContext.mPBRMainLightPower;
	}

	float DirecLightComponent::getPBRSkyLightPower()
	{
		return mContext.mPBRSkyLightPower;
	}

	float DirecLightComponent::getPBRGroundLightPower()
	{
		return mContext.mPBRGroundLightPower;
	}

	float DirecLightComponent::getRoleMainLightPower() {
		return mContext.mRoleMainLightPower;
	}

	float DirecLightComponent::getRoleSkyLightPower()
	{
		return mContext.mRoleSkyLightPower;
	}

	float DirecLightComponent::getRoleGroundLightPower()
	{
		return mContext.mRoleGroundLightPower;
	}

	void DirecLightComponent::setMainLightDirection()
	{
		if (!m_tMgr ||  !m_tMgr->getDirecLightIsUse())
			return;

		if (m_tMgr == nullptr)
			return;
		TODManager* todMgr = m_tMgr->GetTODManager();

		float mainLightPitchLimit = 0.f;
		if(todMgr)
			mainLightPitchLimit = todMgr->getMainLightPitchLimit();
		float mainYaw = mContext.mMainLightYawAngle;
		float mainPitch = mContext.mMainLightPitchAngle;
		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr)
		{
			sMgr->setMainLightDirection(TODManager::convertDirectionFromYawPitchAngle(mainYaw, mainPitch > mainLightPitchLimit ? mainLightPitchLimit : mainPitch));
			sMgr->setMainLightShadowDirection(TODManager::convertDirectionFromYawPitchAngle(mainYaw, mainPitch > mainLightPitchLimit ? mainLightPitchLimit : mainPitch));
		}
		

		 const Vector3& dir = TODManager::convertDirectionFromYawPitchAngle(mainYaw, mainPitch);
		 m_tMgr->setCurrSunDirection(dir);
	}

	void DirecLightComponent::setRealTime(bool flag)
	{
		if(m_tMgr)
		   m_tMgr->setDirecLightRealTime(flag, this);
	}
	bool DirecLightComponent::getRealTime()
	{
		if (!m_tMgr)
			return false;
		return m_tMgr->getDirecLightRealTime(this);
	}

	void DirecLightComponent::setScreenAttribute()
	{
		if (!m_tMgr || !m_tMgr->getDirecLightIsUse())
			return;

		setPBRMainLightColor();
	    setPBRSkyLightColor(); 
		setPBRGroundLightColor();
		setRoleMainLightColor();
	    setRoleSkyLightColor();
		setRoleGroundLightColor();
		setMainLightDirection();
		setMainLightColor();
		setMainSkyLightColor();
		setMainGroundLightColor();
		//setMainAmbientBlendThreshold();
		setPBRAmbientBlendThreshold(mContext.mPBRAmbientBlendThreshold);
		setRoleAmbientBlendThreshold(mContext.mRoleAmbientBlendThreshold);
		setViewLightPercent(mContext.m_ViewLightPercent);
		setPBRViewLightPercent(mContext.m_PBRViewLightPercent);
		setRoleViewLightPercent(mContext.m_RoleViewLightPercent);
	}

	void DirecLightComponent::setPriority(int prior) {
		if (prior > 255) prior = 255;
		else if (prior < 0) prior = 0;
		mContext.mPriority  = prior;
		if(m_tMgr)
			m_tMgr->DirecLightPriorityChanged(this);
	}
	int DirecLightComponent::getPriority()
	{
		return mContext.mPriority;
	}

	DirecLightComponentInfo::DirecLightComponentInfo(const Name& comType,ActorCOMMoveableType type):ActorComponentInfo(comType,type)
	{
	}
	DirecLightComponentInfo::~DirecLightComponentInfo()
	{
	}
	void DirecLightComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<DirecLightComponentInfo>::get()->ExportJson(this, jsonNode, "");

		if (jsonNode)
			cJSON_AddNumberToObject(jsonNode, "version", mVersion);
	}

	void DirecLightComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<DirecLightComponentInfo>::get()->ImportJson(this, jsonNode, "");

		if (jsonNode)
		{
			int version = 0;
			ReadJSNode(jsonNode, "version", version);
			if (version == 0)
			{
				mContext.m_PBRViewLightPercent = mContext.m_ViewLightPercent;
				mContext.m_RoleViewLightPercent = mContext.m_ViewLightPercent;
			}
			mVersion = 1;
		}
	}
	void DirecLightComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		DirecLightComponentInfo* dLComInfo = static_cast<DirecLightComponentInfo*>(comInfo);
		if (dLComInfo == nullptr)
			return;
		reflect::TypeResolver<DirecLightComponentInfo>::get()->copyTo(this, dLComInfo);
	}
	void DirecLightComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}
	void DirecLightComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<DirecLightComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
}
