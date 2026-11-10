#pragma once
#ifndef __ECHO_LIGHTNING_COMPONENT_H__
#define	__ECHO_LIGHTNING_COMPONENT_H__
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoMovableObject.h"
#include "EchoFrameListener.h"
#include "EchoWeatherManager.h"
#include "EchoStringTable.h"
namespace Echo{
	
	class TODComponentManager;
	 

	struct  LightningHandle
	{
		uint32								version;
		bool								mNormalLightningVisible = false;		//常规闪电可见

		//特殊闪电而非下雨闪电
		bool								mLightningVisible = false;				//特殊闪电可见
		float								mLightningLife = 1.0f;					//闪电存活时间
		bool								mLightningDelayed = false;				//闪电是否延后渲染

		Vector2								mLightningInterval{1.5f,10.0f};		    //打闪电的间隔
		float								mLightningLimitYawAngle = 25.0f;		//闪电限制的偏航角度

		Vector2								mLightningLimitPitchRange{-55.0f,-25.0f};//闪电限制的俯仰角度范围，limitPitch
		bool								mLightningUseBeam = false;				 //闪电是否发光

		ColorValue							mLightningColor;				         //闪电颜色
		float								mLightningColorPower = 0.0f;			 //闪电颜色曝光系数，闪电颜色强度
		bool								mUseSunColor = false;					 //闪电时使用太阳颜色（使用海洋高光颜色）

		ColorValue							mSunColor;						         //闪电时的太阳颜色（用于海洋高光颜色）
		bool								mUseSunBackColor = false;				 //闪电时使用背光颜色
		

		ColorValue							mCloudSunBackColour;			        //闪电时的球体云和云海背光颜色
		float								mLightningSunBackColourPower = 3.5f;	//闪电时的球体云和云海背光颜色曝光系数(背光颜色强度) 注：调用setCloudSunBackColour(type,mCloudSunBackColour*mLightningSunBackColourPower)
		//std::vector<String>					mLightningTexture{"Textures/sky/lightning.dds","","","","",""};//闪电形状纹理
		std::vector<String>					mLightningTexture{ "","","","","","" };//闪电形状纹理
		bool								mLightningUseParticle = false;			//闪电形状使用粒子
		bool								mLightningParticleIsLoop = false;		//闪电模型使用的粒子是否循环

		Vector2								mLightningParticleOffsetRange{ 50.0f,100.0f };	//闪电粒子偏移范围
		Vector2								mLightningParticleScaleRange{5.0f,10.0f};	    //闪电粒子比例范围
		std::vector<String>					mLightningParticle{"", "", "", "", "", ""};		//闪电形状粒子
		std::vector<String>					mLightningSound{"", "", "", "", "", ""};		//闪电时要播放的音效
		int									mPriority = 0;
		//没找到对应用途
		Vector2								mLightningRotate;								//闪电旋转范围
		Vector4								mLightningDirection;							//闪电朝向

		std::vector<WeatherBellData>        m_BellVecs;

		LightningHandle()
		{
			m_BellVecs.reserve(6);
			m_BellVecs.resize(6);
			if (StringTable::Alive())
				mLightningTexture[0] = StringTable::instance()->getString("Light", "DefaultLightningTexture");
		}
		
	};
	class _EchoComponentExport LightningComponentInfo : public ActorComponentInfo
	{
	public:
		bool								getNormalLightningVisible() const { return mLightningHandle.mNormalLightningVisible; }
		void								setNormalLightningVisible(const bool& visible) { mLightningHandle.mNormalLightningVisible = visible; }
		bool								getSpecialLightningVisible() const { return mLightningHandle.mLightningVisible; }
		void								setSpecialLightningVisible(const bool& visible) { mLightningHandle.mLightningVisible = visible; }
		float								getLightningLife() const { return mLightningHandle.mLightningLife; }
		void								setLightningLife(const float& LightningLife) { mLightningHandle.mLightningLife = LightningLife; }
		bool								getLightningDelayed() const { return mLightningHandle.mLightningDelayed; }
		void								setLightningDelayed(const bool& lightningDelayed) { mLightningHandle.mLightningDelayed = lightningDelayed; }
		Vector2								getLightningInterval() { return mLightningHandle.mLightningInterval; }
		void								setLightningInterval(const Vector2& Interval) { mLightningHandle.mLightningInterval = Interval; }
		float								getLightningLimitYawAngle() const { return mLightningHandle.mLightningLimitYawAngle; };
		void								setLightningLimitYawAngle(const float& LightningLimitYawAngle) { mLightningHandle.mLightningLimitYawAngle = LightningLimitYawAngle; }
		Vector2								getLightningLimitPitchRange() const { return mLightningHandle.mLightningLimitPitchRange; }
		void								setLightningLimitPitchRange(const Vector2& Range) { mLightningHandle.mLightningLimitPitchRange = Range; }
		bool								getLightningUseBeam() const { return mLightningHandle.mLightningUseBeam; }
		void								setLightningUseBeam(const bool& isUseBeam) { mLightningHandle.mLightningUseBeam = isUseBeam; }
		ColorValue							getLightningColor() const { return mLightningHandle.mLightningColor; }
		void								setLightningColor(const ColorValue& color) { mLightningHandle.mLightningColor = color; }
		float								getLightningColorPower() const { return mLightningHandle.mLightningColorPower; }
		void								setLightningColorPower(const float& lightningColorPower) { mLightningHandle.mLightningColorPower = lightningColorPower; }
		bool								getUseSunColor() const { return mLightningHandle.mUseSunColor; }
		void								setUseSunColor(const bool& useSunColor) { mLightningHandle.mUseSunColor = useSunColor; }
		ColorValue							getSunColor() const { return mLightningHandle.mSunColor; }
		void								setSunColor(const ColorValue& color) { mLightningHandle.mSunColor = color; }
		bool								getUseSunBackColor() const { return mLightningHandle.mUseSunBackColor; }
		void								setUseSunBackColor(const bool& isUseSunBackColor) { mLightningHandle.mUseSunBackColor = isUseSunBackColor; }
		ColorValue							getSunBackColor() const { return mLightningHandle.mCloudSunBackColour; }
		void								setSunBackColor(const ColorValue& color) { mLightningHandle.mCloudSunBackColour = color; }
		float								getSunBackColorPower() const { return mLightningHandle.mLightningSunBackColourPower; }
		void								setSunBackColorPower(const float& power) { mLightningHandle.mLightningSunBackColourPower = power; }
		std::vector<String>					getLightningTexturesName() const { return mLightningHandle.mLightningTexture; }
		void								setLightningTexturesName(const std::vector<String>& names) { mLightningHandle.mLightningTexture = names; }
		bool								getIsUseParticle() const { return mLightningHandle.mLightningUseParticle; }
		void								setIsUseParticle(const bool& isUse) { mLightningHandle.mLightningUseParticle = isUse; }
		bool								getIsParticleLoop() const { return mLightningHandle.mLightningParticleIsLoop; }
		void								setIsParticleLoop(const bool& isLoop) { mLightningHandle.mLightningParticleIsLoop = isLoop; }
		Vector2								getParticleOffsetRange() const { return mLightningHandle.mLightningParticleOffsetRange; }
		void								setParticleOffsetRange(const Vector2& range) { mLightningHandle.mLightningParticleOffsetRange = range; }
		Vector2								getParticleScaleRange() const { return mLightningHandle.mLightningParticleScaleRange; }
		void								setParticleScaleRange(const Vector2& ScaleRange) { mLightningHandle.mLightningParticleScaleRange = ScaleRange; }
		std::vector<String>					getParticlesName() const { return mLightningHandle.mLightningParticle; }
		void								setParticlesName(const std::vector<String>& ParticlesName) { mLightningHandle.mLightningParticle = ParticlesName; }
		std::vector<String>					getLightningSoundName() const { return mLightningHandle.mLightningSound; }
		void								setLightningSoundName(const std::vector<String>& SoundName) { mLightningHandle.mLightningSound = SoundName; }
		int									getPriority() const { return mLightningHandle.mPriority; }
		void								setPriority(const int& _priority) { mLightningHandle.mPriority = _priority; }

		LightningComponentInfo() = default;
		LightningComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~LightningComponentInfo() = default;
		LightningHandle						mLightningHandle;
		virtual void						ExportData(cJSON* jsonNode) override;
		virtual void						ImportData(cJSON* jsonNode) override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                        OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool						getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		EDITOR_REFLECT
	};
	class  _EchoComponentExport LightningComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(LightningComponent)
	public:
		void										init(ActorComponentInfo* info) override;
		virtual  void								unInit() override;
		virtual  void                               visibleStateChanged(bool isVisible) override;

		//void										resetLightning();
		bool										isCurrentActiveCom();
		int											getPriority() const ;
		void										setPriority(const int& val);
		void										setIsUseNormalLightning(const bool& visible);
		void										setIsUseSpecialLightning(const bool& visible);
		void										setLightningLifeTime(const float& LightningLife);
		void										setLightningDelayed(const bool& lightningDelayed);
		void										setLightningInterval(const Vector2& Interval);
		void										setLightningLimitYawAngle(const float& LightningLimitYawAngle);
		void										setLightningLimitPitchRange(const Vector2& Range);
		void										setLightningUseBeam(const bool& isUseBeam);
		void										setLightningColor(const ColorValue& color);
		void										setLightningColorPower(const float& lightningColorPower);
		void										setUseSunColor(const bool& isUseSunColor);
		void										setSunColor(const ColorValue& color);
		void										setUseSunBackColor(const bool& useSunBackColor);
		void										setSunBackColor(const ColorValue& color);
		void										setSunBackColorPower(const float& power);
		void										setIsUseParticle(const bool& isUse);
		void										setIsParticleLoop(const bool& isLoop);
		void										setParticleOffsetRange(const Vector2& range);
		void										setParticleScaleRange(const Vector2& ScaleRange);
		void										setParticlesName(const std::vector<String>& ParticlesName);
		void										setLightningSoundName(const std::vector<String>& SoundName);

		void                                        setLightningBells(const std::vector<WeatherBellData>& bellVec_);
		const std::vector<WeatherBellData>&         getLightningBells() { return mContext.m_BellVecs; }

		void										use();
		void										unuse();
		virtual void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		String										getGlobalLightningTextureName();
		void										setGlobalLightningTextureName(const std::vector<String>& names);
		std::vector<String>							clearEmptyName(const std::vector<String>& names);
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
		LightningHandle								getContext() const { return mContext; }

	protected:
		void                                        _getResBellsData(std::vector<WeatherBellData>& bellData_);
	public:
		String										mGlobalEnvironmentTexName; //= Echo::String("Textures/sky/lightning.dds");
		int											RenderNameSize = 6;
		LightningType								type = LT_SPECIAL;
		LightningHandle								mContext;

	private:
		TODComponentManager*                        m_tMgr = nullptr;
	};

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, const WeatherBellData& bellData);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, WeatherBellData& bellData);


	DEFINECOMFACTORY(LightningComponent, UNMOVEABLE)
}
#endif // !__ECHO_LIGHTNING_COMPONENT_H__
