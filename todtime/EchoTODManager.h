//-----------------------------------------------------------------------------
// File:   EchoTODManager.h
//
// Author: chenanzhi
//
// Date:   2016-6-23
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoTODManager_H__
#define __EchoTODManager_H__
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

#include "EchoPrerequisites.h"
#include "EchoFrameListener.h"
#include "EchoSingleton.h"
#include "EchoTODParamTrace.h"
#include "EchoTODVariableInfo.h"
#include "EchoWeatherManager.h"
#include "EchoTexture.h"
#include "EchoCoreSystemBase.h"

namespace Echo
{
	class Sky;
	class SceneManager;
	class TODSerializer;
	class TODVariableInfo;
	class RenderSystem;
	class TODSystem;

	typedef std::map<String, TODVariableInfo*> TODMap;

	//TOD 优先级
	enum TODPriorityLevel
	{
		e_envAreaLvl = 0,		//环境区域级别
		e_clientLogicLvl,		//客户端逻辑级别
		e_skillLvl,				//技能级别
	};

	struct TODTimeSpeedSlot
	{
		float mStart;
		float mEnd;
		float mPercent;
		double mSpeed;
		float mRealStart;
		float mRealEnd;

		TODTimeSpeedSlot()
		{
			mStart = 0.0f;
			mEnd = 0.0f;
			mPercent = 0.0f;
			mSpeed = 0.0;
			mRealStart = 0.0f;
			mRealEnd = 0.0f;
		}
	};

	class _EchoExport TODManager : public EchoAllocatedObject, public FrameListener, public RenderListener, public ResizeListener
	{
	public:
		class TODManagerListenner
		{
		public:
			virtual void onTimeUpdate(double t) = 0;
			virtual void onTodChanged(const String& todname, bool force = false) = 0;
			virtual void onTodListChanged(const TODMap& todmap) = 0;
			virtual void onTodRemove() = 0;
		};
		typedef std::list<TODManagerListenner*> TODManagerListenners;

		struct SParamValue
		{
			ColorValue mSunColor;
			ColorValue mSunEmissionColor;
			float mSunOuterRadius;
			float mSunInnerRadius;
			float mSunOpactiy;
			ColorValue mSkyTopColor;
			ColorValue mSkyBottomColor;
			float mSkyColorRatio;
			float mSkyFogHeight;
			float mStarSize;
			float mStarIntensity;
			float mStarHDRPower;
			float mVolumetricLightSize;
			float mVolumetricLightOpacity;
			ColorValue mVolumetricLightColour;
			float mLensFlareOpacity;

			float mCloudSpeed;
			float mCloudOpactiy;
			float mCloudEmendation; //补光系数
			ColorValue mCloudColor;
			ColorValue mCloudSunBackColor; //背光颜色
			String mCloudDiffuse;

			float mCloudPlaneUSpeed;
			float mCloudPlaneVSpeed;
			float mCloudPlaneUVTilling;
			float mCloudPlaneOpactiy;
			float mCloudPlaneEmendation;
			float mCloudPlaneHDRPower;
			ColorValue mCloudPlaneColor;
			ColorValue mCloudPlaneSunBackColor;

			float mCloudMeshOpactiy[3];
			float mCloudMeshEmendation[3];
			ColorValue mCloudMeshColor[3];
			ColorValue mCloudMeshSunBackColor[3];
			String	mCloudMeshDiffuse[3];

			float mMainLightYaw;
			float mMainLightPitch;
			ColorValue	mMainLightColor;
			float mAuxiliaryLightPitch;
			ColorValue	mAuxiliaryLightColor;
			ColorValue	mSkyLightColor;
			ColorValue	mGroundLightColor;

			ColorValue	mPBRMainLightColor;
			ColorValue	mPBRSkyLightColor;
			ColorValue	mPBRGroundLightColor;
			float		mPBRAmbientBlendThreshold;

			ColorValue	mRoleMainLightColor;
			ColorValue	mRoleSkyLightColor;
			ColorValue	mRoleGroundLightColor;
			float		mRoleAmbientBlendThreshold;

			ColorValue	mFogNearColor[4];
			float mFogNearStart[4];
			float mFogNearEnd[4];
			float mFogNearDensity[4];
			float mFogNearTopHeight[4];
			ColorValue	mFogFarColor[4];
			float mFogFarStart[4];
			float mFogFarEnd[4];
			float mFogFarDensity[4];
			float mFogFarTopHeight[4];

			float mShadowWeight;
			float mGlobalPointLightPower;
			float mRolePointLightPower;
			ColorValue mRolePointLightColor;

			float mGlobalSpecularRatio;
			float mGlobalReflectionRatio;
			float mGlobalEnvironmentExplosure;
			ColorValue mGlobalCorrectionColor;
			ColorValue mGlobalVegCorrectionColor;
			ColorValue mGlobalBuildCorrectionColor;
			ColorValue mGlobalTerrainCorrectionColor;

			String	mColorGradingTextureName;
			float mBloomThreshold;
			float mBloomBlurStrength;
			float mBloomBlurRadius;

			float mHDREyeParam[4];
			float mHDRBrightParam[4];
			float mHDRToneMappingParam[3];
			float mHDRToneMappingCurve[5];
			float mHDRToneMappingColor[3];

			float mVolumeLightAmount;
			float mVolumeLightAttenuation;
			float mVolumeLightColorIntensity;
			float mVolumeLightColor[3];

			float mSSAOIntensity;

			ColorValue mWeatherCoverColor;
			float mCoverColorIntensity;

			float mOceanWindPower;
			ColorValue mOceanShallowColor;
			ColorValue mOceanDeepColor;
			ColorValue mOceanFoamColor;
			ColorValue mOceanReflectColor;
			ColorValue mOceanUnderWaterFogColor;
			ColorValue mOceanUnderWaterOceanColor;
			ColorValue mOceanUnderWaterCausticColor;

			float mWindPower;
			float mWindAngle;

			float mWindTreeSwayAmp;
			float mWindTreeSwayFreq;
			float mWindTreeShakeAmp;
			float mWindTreeShakeFreq;

			float mWindVegSwayAmp;
			float mWindVegSwayFreq;
			float mWindVegShakeAmp;
			float mWindVegShakeFreq;

			float mAtmosphereHeight;
			float mAtmosphereBlendRange;
			ColorValue mAtmosphereLowerLayerColor;
			ColorValue mAtmosphereUpperLayerColor;
			float mAtmosphereTransmittanceExpTerm;
			float mAtmosphereTransmittanceExpScale;
			float mAtmosphereMaxAlpha;

			float mViewLightPercent[3];
		};

		struct VariableValue
		{
			VariableValue()
			{
				typeId = -1;
				fValue[0] = fValue[1] = fValue[2] = 0;
			}
			int typeId;
			float fValue[3];
		};
	

		~TODManager(void);

		
		static TODManager* instance();
		const Name& getName() { return mName; }
		WeatherManager* getWeatherManager();
		EnvironmentLight* getEnvironmentLight();
		ProtocolComponentManager* getProtocolComponentManager();

		void cleanup();

		String createUniqueID(const String& name);
		TODVariableInfo* prepareTod(const String& name);
		TODVariableInfo* loadTod(const String& name);
		TODVariableInfo* createTod(const String& name);
		bool saveTod(const String& name);
		bool saveTodAs(const String& name, const String&sNewPathFull);
		void removeTod(const String& name);
		void deleteTod(const String& name);
		//设置静态时间点 t（规范在0-1之间）
		void setStaticTime(double t);
		//设置当前tod文件开始时间点 t（规范在0-1之间)
		void setRangeStartTime(float t);
		//获取当前tod文件开始时间点
		float getRangeStartTime() const;
		//设置当前tod文件结束时间点 t（规范在0-1之间)
		void setRangeEndTime(float t);
		//获取当前tod文件结束时间点
		float getRangeEndTime() const;
		//每帧更新数据
		void updateData(double timeElapsed, bool force = false);
		//选择使用指定的tod文件
		bool selectTod(const String& name, bool update = true);
		//设置tod速度 （现实的一秒 = 游戏时间s秒）
		void setTimeSpeed(double s);
		//获取tod速度
		double getTimeSpeed() const;
		//设置tod更新间隔（单位：游戏时间分钟）
		void setTimeStep(double s);
		void setMainLightPitchLimit(float s);
		float getMainLightPitchLimit();
		//获取tod更新间隔
		double getTimeStep() const;
		//获取tod各个时间段所点比例信息
		const std::vector<TODTimeSpeedSlot>& getTODTimeSpeedSlots() const;
		//设置tod各个时间段所点比例信息
		void setTODTimeSpeedSlots(const std::vector<TODTimeSpeedSlot>& speedSlots);
		//设置加速多少时间单位s
		void setTimePassed(int time);
		//设置全局tod各个时间段所点比例信息
		void setGlobalTimeSpeedSlots(const std::vector<TODTimeSpeedSlot>& speedSlots);
		//使用全全局tod各个时间段所点比例信息
		void useGlobalTimeSpeedSlots();

		void setSceneManager(SceneManager* mng);
		SceneManager* getSceneManager()const { return mSceneManager; }

		void addListenner(TODManagerListenner* l);
		void removeListenner(TODManagerListenner* l);
		void clearListenner();

		//设置是否能播放tod (备注：客户端逻辑调用)
		void enablePlay(bool b);
		//设置是否能播放tod并指定静态时间点 (备注：客户端逻辑调用)
		void enablePlay(bool b, double t);

		//设置是否能播放tod (备注：从环境区域信息中指定） 
		void enableSrcPlay(bool b, double t);

		//设置服务器时间 dwSvrTime:当前服务器系统TickCount  dwDayTime:每天的时间，以秒为单位
		void setSvrTime(uint64 dwSvrTime, uint32 dwDayTime);
		//根据服务器时间和tod速度来计算当前tod的实际时间 返回值 (0到1之间)
		double calculateCurTodTime();

		//获取当前tod优先级
		TODPriorityLevel getCurTODPriorityLevel() const;
		//设置tod优先级
		void setTODPriorityLevel(TODPriorityLevel lvl);
		//获取当前tod的实际时间 (0到1之间)
		double getCurrentTime() const;
		//进入到新的tod , time：过渡时间 单位秒, lvl：指定新tod的优先级
		void enter(const String& tod, float time, TODPriorityLevel lvl);
		//使用指定信息，创建新的tod
		void createTod(const String& todName, const vector<VariableValue>::type& params);
		//void saveTod(const String& name,const AreaParameters& params);
		//获取当前使用的tod文件名 （备注：带有相对路径信息）
		String getCurrentTodName() const;
		//获取当前使用的TOD变量信息对象
		TODVariableInfo* getCurrentTODVariableInfo() const;
		void setTodFileDir(const String& dir);
		const String& getTodFileDir() const;
		const String& getRootPath() const;

		//使用自定义ColorChart 
		void setUseCustomColorChart(bool value, const String& colorChartName);
		bool isUseCustomColorChart() const;

		//是否月亮升起（TOD时间是晚上）
		bool isMoonRise() const;

		// Override from FrameListener
		bool frameStarted(const FrameEvent& frameEvent) override;

		// Override from RenderListener
		void OnInitializeDevice(void* hwnd, void* pDevice, void * pContext) override {};
		void OnUninitializeDevice() override {};
		void OnFrameEnd() override {};
		void OnDeviceLost() override {};
		void OnDeivceReset() override;
		void ResizeEvent(uint32 width, uint32 height) override;

		void OnInitRenderLister(RenderSystem * renderSys);
		void UnInitRenderLister(RenderSystem* renderSys);
		//设置资源中配置的静态时间点 t（规范在0-1之间）
		void setSrcStaticTime(double t);
		//获取资源中配置的静态时间点
		double getSrcStaticTime() const;
		//设置资源中配置的是否播放tod
		void setSrcEnablePlay(bool b);
		//获取资源中配置的是否播放tod
		bool getSrcEnablePlay() const;
		//恢复到资源配置的状态
		void restore();
		//获取当前tod状态
		void getTodStatus(String &commenName, String &rainName, double &staticTime, float &transTime, bool &bEnablePlay);
		//获取当前tod状态数据
		bool getTodStatusData(TODVariableInfo*& pData);

		Texture* getCurrentColorChart() const;
		void setCurrentColorChart(const String& name);

		void setDefaultColorChart(const String& name);

		void setCurrentCloudDiffuseByType(int type, const String& name);
		//组件用的接口
		void setCurrentCloudDiffuseNameByCom(int type, const String& name);

		void prepareTexture(TODVariableInfo* pVariableInfo);
		void loadTexture(TODVariableInfo* pVariableInfo);

		double getCurrentTotalTime() const {return m_currentTotalTime;}
		double getCurrentRealTotalTime() const {return m_currentRealTotalTime;}
		uint64 getSvrTime() const { return m_dwSvrTime; }

		void   setTranslationTime(float time_);
		static Vector3 convertDirectionFromYawPitchAngle(float yaw, float pitch);

		//是否使用下雨时或特殊闪电时更新
		void   setIsUseRainState(bool state_) { mIsUseRainState = state_; }
		double  getTimeElapsed();
		void setLocationTimeOffsetSeconds(int seconds);
		//判断是否是进场景第一次更新
		bool    judgeIsFirstUpdate();

		void    getActiveColorChartName(std::string& name_);

		void    setErrorId(int id_,const std::string& areaName_);
		const   std::multimap<std::string, int>& getErrorId() { return m_ErrorIdMap; }
	private:
		void _updateTimePeriodSpeed();
		double _getRealElapseTime(double curTime,double elapsedTime);
		int64 _getReferenceTime();
		unsigned long _getCurrentTick();

		void _onPrepareTexture(TODVariableInfo* pVariableInfo, TODVariableInfo::ETimeOfDayParamID paramId, std::map<String, TexturePtr> & texMap);
		void _onLoadTexture(TODVariableInfo* pVariableInfo, TODVariableInfo::ETimeOfDayParamID paramId, std::map<String, TexturePtr> & texMap);

		Texture * _getDefaultColorChart();
		Texture* _getColorChart(const String& name);
		//void _setColorGrading(Texture * colorChartTex);
		void _setBloomParam(float threshold, float strength, float radius);
		void _setHDRParam(const SParamValue& param);
		void _setHDRDefaultParam(SParamValue& param);
		void _updateMoonPhase(double time);

		void getParamFromTod(SParamValue & paramInfo, TODVariableInfo*pInfo, bool bLerp = true);
		inline float clamp(float dest, float minv, float maxv) { return std::max(minv, std::min(dest, maxv)); }
		ColorValue lerp(const ColorValue & clr1, const ColorValue & clr2, float factor);

		template <typename T>
		T lerp(const T &t1, const T &t2, float factor);

		TODMap mTodMap;
		TODVariableInfo *mCurrentVariableInfo;
		TODVariableInfo *mOldVariableInfo;
		double mCurrentTime;
		double mTimeSpeed;					//tod时间速度
		std::vector<TODTimeSpeedSlot> mTimeSpeedSlots; //各个时间段占得比例
		std::vector<TODTimeSpeedSlot> mGlobalTimeSpeedSlots; //全局各个时间段占得比例
		double mTimeStep;					//tod更新步长（每隔多少时间更新一次）
		float mMainLightPitchLimit;
		double mLastUpdateTime;				//上次更新的时间
		bool mPlayEnable;					//是否播放tod
		double mStaticTime;					//关闭tod使用的静态时间点
		int mTimePassed = 0;				//记录加速多少时间
		int mLocationTimeOffsetSeconds = 0;
		String mTodFileDir;
		String mRootPath;
		SceneManager * mSceneManager;
		TODManagerListenners mListenners;
		String mCommonTodName;
		String mRainTodName;

		TODPriorityLevel mTODPriorityLevel;
		bool			 mMoonRise;

		float mElapseTime;				//当前过渡时间
		float mTranslationTime;			//区域过渡总时间

		bool mSrcPlayEnable;					//资源中配置的是否播放tod
		double mSrcStaticTime;					//资源中配置的关闭tod使用的静态时间点

		uint64 m_dwSvrTime;                     //服务器操作系统当前的系统时间 与 游戏服务器程序启动时的系统时间的差值 单位秒
		unsigned long m_dwTickTime;             //设置服务器时间时的客户端tick 单位毫秒
		uint32 m_dwDayTime;                     //每天的时间，以秒为单位 比如：24*3600

		unsigned long m_dwUpdateTick;			//每帧tick 单位毫秒
		double m_currentTotalTime;				//各个时间段同速时游戏里走过的时间 单位秒
		double m_currentRealTotalTime;			//各个时间段不同速时游戏里走过的时间 单位秒

		bool mPreNeedUpdateWeatherParam;
		bool mForceUpdate;				//需要强制更新
		bool mUseCustomColorChart;
		TexturePtr mCustomColorGradingTexturePtr;
		TexturePtr mDefaultColorGradingTexturePtr;
		std::map<String, TexturePtr>  mColorGradingTextureMap;
		std::map<String, TexturePtr>  mCloudSeaTextureMap;
		String	  mCurrentDiffuseName[5];
		String    mCurrentColorChartName;
		TexturePtr mCurrentColorChartTex;
		Name mName;

		bool mIsUseRainState = false;		// GM命令todRs设置的标志位

		double mTimeElapsed = 0;

		std::multimap<std::string, int>   m_ErrorIdMap;
	private:
		TODManager();
		TODManager(const Name& name);
		friend class CoreSystemBase<TODManager>;
		friend class TODSystem;
	};

	enum class LogType
	{
		LT_CloudMesh,
		LT_CloudSphere,
		LT_CloudPlane,
		LT_End
	};

	class _EchoExport TODSystem : public Singleton<TODSystem>, public CoreSystemBase<TODManager> {
	public:
		TODSystem();
		virtual ~TODSystem() override;

		void setLogSwitch(LogType type);
		void clearLogSiwtch() { m_showLog = 0; }
		bool showLog(LogType type);

		size_t m_showLog = 0;
	};
}
//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
