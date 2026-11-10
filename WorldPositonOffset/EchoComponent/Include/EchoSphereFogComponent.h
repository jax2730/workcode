#ifndef __EchoSphereFogComponent_H__
#define __EchoSphereFogComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class SphericalTerrainFog;
	class  TODComponentManager;

	static Name fogColorName_ = Name("SphereFogColor");
	static Name fogExpScaleName_ = Name("SphereFogScaleTerm");
	static Name fogExpTermName_ = Name("SphereFogExpTerm");
	static Name fogMaxAlphaName_ = Name("SphereFogMaxAlpha");
	static Name fogHorizonColorName_ = Name("SphereFogHorizonColor");
	static Name fogColorChangeGradientName_ = Name("SphereFogColorChangeGradient");
	static Name fogStartName_ = Name("SphereFogStart");
	static Name fogEndName_ = Name("SphereFogEnd");
	static Name fogSkyParamName_ = Name("SphereFogSkyParam");

	struct SphereFogComHandle {
		ColorValue				mFogColor = ColorValue(1.f);
		float					mFogExpTerm = 0.0f;
		float					mFogExpScale = 0.0f;
		float					mFogMaxAlpha = 1.f;
		ColorValue				mHorizonColor = ColorValue(1.f);
		float					mColorChangeGradient = 1.f;
		float					mFogStart = 0.0f;
		float					mFogEnd = 40000.0f;
		Vector4					mFogSkyParam = Vector4(1.f, 1.f, 0.f, 0.f);
		bool                    mEnableAtmosphere = false;

		float                   mTransFormTime = 0.f;
		int                     mPriority = 0;
	};

	class _EchoComponentExport SphereFogComponentInfo : public ActorComponentInfo
	{
	public:
		SphereFogComponentInfo() {};
		SphereFogComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual ~SphereFogComponentInfo();

		virtual void		ExportData(cJSON* jsonNode) override;
		virtual void		ImportData(cJSON* jsonNode) override;
		virtual void		CopyTo(ActorComponentInfo* comInfo) override;
		virtual void		OverWrite() override;

		SphereFogComHandle     mContext;
		EDITOR_REFLECT
	};

	class _EchoComponentExport SphereFogComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SphereFogComponent)

	public:
		//tod
		void                 use();
		void                 unuse();
		virtual  void		 setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual  void        visibleStateChanged(bool isVisible) override;
		float                getTransFormTime() { return mContext.mTransFormTime; }
		TODComponentManager* getTodComManager() { return m_tMgr; }
		void                 setPriority(int priority);
		int                  getPriority() { return mContext.mPriority; }
	private:
		bool                 mIsActiveCom = false;
		TODComponentManager* m_tMgr = nullptr;
	public:
		REGISTER_LUA_CLASS_KEY;
		void				 init(ActorComponentInfo* info) override;
		void				 unInit() override;


		// 设置星球雾效的接口
		void				SetFogColor(const ColorValue& val);
		void				SetFogExpScale(float val);
		void				SetFogExpTerm(float val);
		void				SetHorizonColor(const ColorValue& val);
		void				SetFogStart(float val);
		void				SetFogEnd(float val);
		void				SetColorChangeGradient(float value);
		void				SetMaxAlpha(float val);
		void				SetSkyParam(float val);


		// 效果开关
		void				SetEnable(bool val);


		// 往后期设置参数的接口
		void				SetFog();


		SphereFogComHandle     mContext;

	};
	DEFINECOMFACTORY(SphereFogComponent, UNMOVEABLE)
}
#endif
