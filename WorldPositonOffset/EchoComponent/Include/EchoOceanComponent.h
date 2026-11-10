#ifndef _OceanComponent_H_
#define _OceanComponent_H_

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class TODComponentManager;
	struct OceanComponentContext
	{
		float									getOcenScale()const							{ return mOceanScale; }

		Echo::String							getMeshFile()const							{ return mMeshFile; }
		Echo::String							getNormalTexName()const						{ return mNormalTexName; }
		Echo::String							getFoamTexName()const						{ return mFoamTexName; }

		float									getFresnelBias()const						{ return mFresnelBias; }
		float									getFresnelIntensity()const					{ return mFresnelIntensity; }
		float									getFresnelRatio()const						{ return mFresnelRatio; }

		Vector2									getTextureScale()const						{ return mTextureScale; }
		float									getReflectionAmount()const					{ return mReflectionAmount; }
		float									getWaveAmount()const						{ return mWaveAmount; }
		int										getLayer()const								{ return mLayer; }
		bool									getVisible()const							{ return mVisible; }
		int										getPriority()const							{ return mPriority; }
		float									getTransFormTime()const						{ return mTransFormTime; }

		float									getOceanHeight()const						{ return mOceanHeight; }
		float									getWaveFrequency()const						{ return mWaveFrequency; }
		float									getWaveAmplitude()const						{ return mWaveAmplitude; }
		float									getBumpScale()const							{ return mBumpScale; }
		float									getRefractAmount()const						{ return mRefractAmount; }
		float									getBorderSoft()const						{ return mBorderSoft; }
		Echo::String							getReflectTextureName()const				{ return mReflectTextureName; }

		float									getSpecularPower()const						{ return mSpecularPower; }
		float									getSpecularStrength()const					{ return mSpecularStrength; }

		float									getFoamBias()const							{ return mFoamBias; }
		float									getFoamSoft()const							{ return mFoamSoft; }
		float									getFoamTilling()const						{ return mFoamTilling; }

		float									getUnderWaterFogDensity()const				{ return mUnderWaterFogDensity; }
		float									getUnderWaterFogStart()const				{ return mUnderWaterFogStart; }
		float									getUnderWaterFogEnd()const					{ return mUnderWaterFogEnd; }

		//TOD属性
		ColorValue								getWaterShallowColor()const					{ return mWaterShallowColor; }
		ColorValue								getWaterDeepColor()const					{ return mWaterDeepColor; }
		ColorValue								getWaterFoamColor()const					{ return mWaterFoamColor; }
		ColorValue								getWaterReflectColor()const					{ return mWaterReflectColor; }
		ColorValue								getUnderWaterColor()const					{ return mUnderWaterColor; }
		ColorValue								getUnderWaterFogColor()const				{ return mUnderWaterFogColor; }

		
		void						 			setOcenScale(float val)						{ mOceanScale = val; }

		void						 			setMeshFile(const String& val)				{ mMeshFile = val; }
		void						 			setNormalTexName(const String& val)			{ mNormalTexName = val; }
		void						 			setFoamTexName(const String& val)			{ mFoamTexName = val; }

		void						 			setFresnelBias(float val)					{ mFresnelBias = val; }
		void						 			setFresnelIntensity(float val)				{ mFresnelIntensity = val; }
		void						 			setFresnelRatio(float val)					{ mFresnelRatio = val; }

		void						 			setTextureScale(const Vector2& val)			{ mTextureScale = val; }
		void						 			setReflectionAmount(float val)				{ mReflectionAmount = val; }
		void						 			setWaveAmount(float val)					{ mWaveAmount = val; }
		void									setLayer(int val) { mLayer = val; }
		void									setVisible(bool val)						{ mVisible = val; }
		void									setPriority(int val)						{ mPriority = val; }

		void									setOceanHeight(float val)					{ mOceanHeight = val; }
		void									setWaveFrequency(float val)					{ mWaveFrequency = val; }
		void									setWaveAmplitude(float val)					{ mWaveAmplitude = val; }
		void									setBumpScale(float val)						{ mBumpScale = val; }
		void									setRefractAmount(float val)					{ mRefractAmount = val; }
		void									setBorderSoft(float val)					{ mBorderSoft = val; }
		void						 			setReflectTextureName(const String& val)	{ mReflectTextureName = val; }

		void									setSpecularPower(float val)					{ mSpecularPower = val; }
		void									setSpecularStrength(float val)				{ mSpecularStrength = val; }

		void									setFoamBias(float val)						{ mFoamBias = val; }
		void									setFoamSoft(float val)						{ mFoamSoft = val; }
		void									setFoamTilling(float val)					{ mFoamTilling = val; }

		void									setUnderWaterFogDensity(float val)			{ mUnderWaterFogDensity = val; }
		void									setUnderWaterFogStart(float val)			{ mUnderWaterFogStart = val; }
		void									setUnderWaterFogEnd(float val)				{ mUnderWaterFogEnd = val; }

		//TOD属性
		void									setWaterShallowColor(const ColorValue& val)	{ mWaterShallowColor = val; }
		void									setWaterDeepColor(const ColorValue& val)	{ mWaterDeepColor = val; }
		void									setWaterFoamColor(const ColorValue& val)	{ mWaterFoamColor = val; }
		void									setWaterReflectColor(const ColorValue& val)	{ mWaterReflectColor = val; }
		void									setUnderWaterColor(const ColorValue& val)	{ mUnderWaterColor = val; }
		void									setUnderWaterFogColor(const ColorValue& val){ mUnderWaterFogColor = val; }


		float									mOceanScale = 100.f;					//海面缩放

		Echo::String							mMeshFile;// = "model/common/gx_haimian_001.mesh";
		Echo::String							mNormalTexName;// = "Textures/water/waves.dds";
		Echo::String							mFoamTexName;// = "Textures/water/foam.dds";

		float									mFresnelBias = 0.15f;					//菲涅尔基础值
		float									mFresnelIntensity = 5.0f;				//菲涅尔效应的变化强度
		float									mFresnelRatio = 0.5f;					//菲涅尔系数

		Echo::Vector2							mTextureScale = Echo::Vector2(10,10);	//纹理比例
		float									mReflectionAmount = 1.0f;				//反射数量
		float									mWaveAmount = 0.3f;						//水量
		int										mLayer = 0;
		bool									mVisible = true;

		float									mTransFormTime = 0.f;
		int										mPriority = 0;
		//模板
		float									mOceanHeight = 0.f;						//海面高度
		float									mWaveFrequency = 3.f;					//海浪频率
		float									mWaveAmplitude = 0.007f;				//海浪振幅
		float									mBumpScale = 1.5f;						//微波占比
		float									mRefractAmount = 2.0f;					//折射扭曲强度
		float									mBorderSoft = 30.0f;					//海岸柔边范围
		String									mReflectTextureName;// = "Textures/water/gx_huanjing_001.dds";	//反射贴图

		//高光
		float									mSpecularPower = 300.0f;					//高光能量
		float									mSpecularStrength = 15.0f;				//高光强度

		//泡沫									
		float									mFoamBias = 7.0f;						//泡沫范围
		float									mFoamSoft = 0.35f;						//泡沫柔边
		float									mFoamTilling = 0.35f;					//泡沫大小（UV扩展系数）

		//水下雾效
		float									mUnderWaterFogDensity = 100.0f;			//水下雾效浓度
		float									mUnderWaterFogStart = 0.0f;				//水下雾开始距离
		float									mUnderWaterFogEnd = 50.0f;				//水下雾结束距离

		//TOD属性
		ColorValue								mWaterShallowColor = ColorValue(0.0549f,0.6745f,1.0f,1.0f);		//浅海颜色
		ColorValue								mWaterDeepColor = ColorValue(0.0f, 0.298f, 0.498f, 1.0f);		//深海颜色
		ColorValue								mWaterFoamColor = ColorValue(1.0f, 1.0f, 1.0f, 1.0f);			//泡沫颜色
		ColorValue								mWaterReflectColor = ColorValue(0.192f,0.651f, 0.667f, 1.f);	//水反射颜色
		ColorValue								mUnderWaterColor = ColorValue(0.514f,0.749f, 0.839f, 1.f);		//在水下时，水的颜色
		ColorValue								mUnderWaterFogColor = ColorValue(0.031f, 0.467f, 0.529f, 1.f);	//水下雾颜色

		OceanComponentContext();
	};

	class _EchoComponentExport OceanComponentInfo : public ActorComponentInfo
	{
	public:
		OceanComponentInfo();
		OceanComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~OceanComponentInfo();
#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		virtual void							ExportData(cJSON* jsonNode) override;
		virtual void							ImportData(cJSON* jsonNode) override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                            Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                            OverWrite() override;

	public:
		EDITOR_REFLECT


	public:
		ColorValue								getWaterShallowColor()const { return mContext.getWaterShallowColor(); }
		ColorValue								getWaterDeepColor()const { return mContext.getWaterDeepColor(); }
		ColorValue								getWaterFoamColor()const { return mContext.getWaterFoamColor(); }
		ColorValue								getWaterReflectColor()const { return mContext.getWaterReflectColor(); }
		ColorValue								getUnderWaterColor()const { return mContext.getUnderWaterColor(); }
		ColorValue								getUnderWaterFogColor()const { return mContext.getUnderWaterFogColor(); }

		//TOD属性
		void									setWaterShallowColor(const ColorValue& val) { mContext.setWaterShallowColor(val); }
		void									setWaterDeepColor(const ColorValue& val) { mContext.setWaterDeepColor(val); }
		void									setWaterFoamColor(const ColorValue& val) { mContext.setWaterFoamColor(val); }
		void									setWaterReflectColor(const ColorValue& val) { mContext.setWaterReflectColor(val); }
		void									setUnderWaterColor(const ColorValue& val) { mContext.setUnderWaterColor(val); }
		void									setUnderWaterFogColor(const ColorValue& val) { mContext.setUnderWaterFogColor(val); }

	public:
		OceanComponentContext			mContext;
	};

	class _EchoComponentExport OceanComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(OceanComponent)
	public:
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							unInit() override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		virtual  void                           visibleStateChanged(bool isVisible) override;
		void setTimelineAttribute(std::vector<float>& data,const Name& name) override;

		//OceanComponentInfo*				mOceanComponentInfo = nullptr;

		
		void									use();
		void									unuse();
		void									setbFFTOcean(bool& value);

		void									setVisibleImpl(bool& value);

		void									setOceanScale(float& value);
		void									setMeshFile(Echo::String& value);
		void									setNormalTexName(Echo::String& value);
		void									setFoamTexName(Echo::String& value);
		void									setFresnelBias(float& value);
		void									setFresnelIntensity(float& value);
		void									setFresnelRatio(float& value);
		void									setTextureScale(Echo::Vector2& value);
		void									setReflectionAmount(float& value);
		void									setWaveAmount(float& value);
		void									setLayer(int& layer);

		void									setPriority(int val);
		void									setTransFormTime(float pTime) { mContext.mTransFormTime = pTime; }
		//模板
		void									setOceanHeight(float& value);
		void									setWaveFrequency(float& value);
		void									setWaveAmplitude(float& value);
		void									setBumpScale(float& value);
		void									setRefractAmount(float& value);
		void									setBorderSoft(float& value);
		void						 			setReflectTextureName(const String& value);

		void									setSpecularPower(float& value);
		void									setSpecularStrength(float& value);

		void									setFoamBias(float& value);
		void									setFoamSoft(float& value);
		void									setFoamTilling(float& value);

		void									setUnderWaterFogDensity(float& value);
		void									setUnderWaterFogStart(float& value);
		void									setUnderWaterFogEnd(float& value);

		//TOD属性
		void									setWaterShallowColor(const ColorValue& val);
		void									setWaterDeepColor(const ColorValue& val);
		void									setWaterFoamColor(const ColorValue& val);
		void									setWaterReflectColor(const ColorValue& val);
		void									setUnderWaterColor(const ColorValue& val);
		void									setUnderWaterFogColor(const ColorValue& val);

		float									getOcenScale()const				{ return mContext.getOcenScale(); }

		Echo::String							getMeshFile()const				{ return mContext.getMeshFile(); }
		Echo::String							getNormalTexName()const			{ return mContext.getNormalTexName(); }
		Echo::String							getFoamTexName()const			{ return mContext.getFoamTexName(); }

		float									getFresnelBias()const			{ return mContext.getFresnelBias(); }
		float									getFresnelIntensity()const		{ return mContext.getFresnelIntensity(); }
		float									getFresnelRatio()const			{ return mContext.getFresnelRatio(); }

		Vector2									getTextureScale()const			{ return mContext.getTextureScale(); }
		float									getReflectionAmount()const		{ return mContext.getReflectionAmount(); }
		float									getWaveAmount()const			{ return mContext.getWaveAmount(); }
		int										getLayer()const					{ return mContext.getLayer(); }
		bool									getVisible()const				{ return mContext.getVisible(); }
		int										getPriority() const				{ return mContext.getPriority(); }
		float									getTransFormTime() const		{ return mContext.getTransFormTime(); }
		//模板
		float									getOceanHeight()const			{ return mContext.getOceanHeight(); }
		float									getWaveFrequency()const			{ return mContext.getWaveFrequency(); }
		float									getWaveAmplitude()const			{ return mContext.getWaveAmplitude(); }
		float									getBumpScale()const				{ return mContext.getBumpScale(); }
		float									getRefractAmount()const			{ return mContext.getRefractAmount(); }
		float									getBorderSoft()const			{ return mContext.getBorderSoft(); }
		Echo::String							getReflectTextureName()const	{ return mContext.getReflectTextureName(); }

		float									getSpecularPower()const			{ return mContext.getSpecularPower(); }
		float									getSpecularStrength()const		{ return mContext.getSpecularStrength(); }

		float									getFoamBias()const				{ return mContext.getFoamBias(); }
		float									getFoamSoft()const				{ return mContext.getFoamSoft(); }
		float									getFoamTilling()const			{ return mContext.getFoamTilling(); }

		float									getUnderWaterFogDensity()const	{ return mContext.getUnderWaterFogDensity(); }
		float									getUnderWaterFogStart()const	{ return mContext.getUnderWaterFogStart(); }
		float									getUnderWaterFogEnd()const		{ return mContext.getUnderWaterFogEnd(); }

		//TOD属性
		ColorValue								getWaterShallowColor()const		{ return mContext.getWaterShallowColor(); }
		ColorValue								getWaterDeepColor()const		{ return mContext.getWaterDeepColor(); }
		ColorValue								getWaterFoamColor()const		{ return mContext.getWaterFoamColor(); }
		ColorValue								getWaterReflectColor()const		{ return mContext.getWaterReflectColor(); }
		ColorValue								getUnderWaterColor()const		{ return mContext.getUnderWaterColor(); }
		ColorValue								getUnderWaterFogColor()const	{ return mContext.getUnderWaterFogColor(); }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
		OceanComponentContext&					getContext() {return mContext;}
	private:
		OceanComponentContext					mContext;
		SceneManager*							mSceneManager = nullptr;
		Ocean*                                  mOceanHandle = nullptr;
		TODComponentManager*                    m_tMgr = nullptr;
	};
	DEFINECOMFACTORY(OceanComponent, UNMOVEABLE)
}
#endif // !_OceanComponent_H_
