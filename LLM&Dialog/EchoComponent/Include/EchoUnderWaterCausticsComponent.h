#pragma once
#ifndef __ECHO_UNDERWATER_CAUSTICS_COMPONENT_H__
#define __ECHO_UNDERWATER_CAUSTICS_COMPONENT_H__
#include "EchoStringConverter.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoMovableObject.h"
#include "EchoTextureManager.h"
#include "EchoUnderWater.h"
#include "EchoRenderStrategy.h"
#include "EchoTunnel.h"
namespace Echo
{
	class TODComponentManager;
	struct CausticsHandle
	{
		float										StartDistance = 10.0f;//开始距离
		float										EndDistance = 50.0f;//截至距离
		float										Density = 0.025f;//密度
		float										DisturbStrength = 4.0f;//扰动强度
		ColorValue									Color = ColorValue(1.0f,1.0f,1.0f,1.0f);//颜色
		float										strength = 0.01f;//颜色强度
		bool										isForceUnderWater = false;
		bool										isEnable = false;

		float										mTransFormTime = 0.f;
		int											mPriority = 0;
	};
	class _EchoComponentExport UnderWaterCausticsComponentInfo : public ActorComponentInfo
	{
		public:
			void									setCausticsStartDistance(const float& start) { mCausticsHandle.StartDistance = start; };
			void									setCausticsEndDistance(const float& end) { mCausticsHandle.EndDistance = end; };
			void									setCausticsDensity(const float& density) { mCausticsHandle.Density = density; };
			void									setCausticsDisturbStrength(const float& disturbStrength) { mCausticsHandle.DisturbStrength = disturbStrength; };
			void									setCausticsColor(const ColorValue& Color) { mCausticsHandle.Color = Color; }
			void									setColorStrength(const float& strength) { mCausticsHandle.strength = strength; }
			void									setIsForceUnderWater(const bool& isUnderWater) { mCausticsHandle.isForceUnderWater = isUnderWater; }
			float									getCausticsStartDistance()const { return mCausticsHandle.StartDistance; };
			float									getCausticsEndDistance() const { return mCausticsHandle.EndDistance; };
			float									getCausticsDensity() const { return mCausticsHandle.Density; };
			float									getCausticsDisturbStrength() const { return mCausticsHandle.DisturbStrength; };
			ColorValue								getCausticsColor() const { return mCausticsHandle.Color; };
			float									getColorStrength() const { return mCausticsHandle.strength; }
			bool									getIsForceUnderWater() const { return mCausticsHandle.isForceUnderWater; }
			CausticsHandle							mCausticsHandle;
			
		public:
			UnderWaterCausticsComponentInfo() = default;
			UnderWaterCausticsComponentInfo(const Name& comType, ActorCOMMoveableType type);
			~UnderWaterCausticsComponentInfo() = default;
			virtual void							ExportData(cJSON* jsonNode) override;
			virtual void							ImportData(cJSON* jsonNode) override;
			virtual void							CopyTo(ActorComponentInfo* comInfo) override;
			virtual void                            OverWrite() override;
			EDITOR_REFLECT
	};
	class  _EchoComponentExport UnderWaterCausticsComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(UnderWaterCausticsComponent)
	public:
		void										init(ActorComponentInfo* info) override;
		virtual  void								unInit() override;
		virtual void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;

		virtual void                                visibleStateChanged(bool isVisible) override;

		int											getPriority() const;
		void										setPriority(const int& val);
		float										getTransFormTime() const { return mContext.mTransFormTime; }
		void										setTransFormTime(float time) { mContext.mTransFormTime = time; }
		void										use();
		void										unuse();
		void										setGlobalStartDistance(const float& val);
		void										setGlobalEndDistance(const float& val);
		void										setGlobalDensity(const float& val);
		void										setGlobalDisturbStrength(const float& val);
		void										setCausticsClolor(const ColorValue& Color);
		void										setColorStrength(const float& val);
		void										setIsForceUnderWater(const bool& isForceUnderWater);
		bool										IsForceUnderWater();
		void										setUnderWaterEnable(const bool& UnderWaterEnable);
		bool										getUnderWaterEnable() const;
		bool										isCurrentActiveCom();
		void										useCaustics();
		void										useColor();
		void										setIsUseUnderWater(const bool& isUse);
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
		CausticsHandle								getContext() const { return mContext; }
	public:
		void										setCausticTexture();
		String										mCausticTextureName;// = "Textures/water/caustic.dds";
		String										mTunnelTextureName;// = "Editor/TunnelNoise.png";

	private:
		CausticsHandle								mContext;
		TODComponentManager*                        m_tMgr = nullptr;
		SceneManager*                               m_sMgr = nullptr;
	};
	DEFINECOMFACTORY(UnderWaterCausticsComponent, UNMOVEABLE)
}
#endif // !__ECHO_UNDERWATER_CAUSTICS_COMPONENT_H__



