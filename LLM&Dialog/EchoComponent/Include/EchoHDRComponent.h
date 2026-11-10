#pragma once
#ifndef __ECHO_HDR_COMPONENT_H__
#define	__ECHO_HDR_COMPONENT_H__
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoSubEntity.h"
#include "EchoHDR.h"
#include "EchoPostProcess.h"
namespace Echo
{
	class TODComponentManager;
	struct HDRHandle
	{
		Vector4									EyeAdaption = Vector4(0.15f,0.7f,0.5f,0.05f); // Vector4(0.1f, 0.1f, 1.5f, 0.05f);
		Vector4									Bright = Vector4(0.7f,0.5f,1.0f,3.3f);
		Vector4									EyeToneMapping = Vector4(0.65f,0.5f,1.0f,0.0f);
		std::vector<float>						EyeToneMappingCurve{3.3f,0.08f,2.43f,0.59f,0.12f};
		ColorValue								EyeToneMappingColor;

		float									mTransFormTime = 0.f;
		int										mPriority = 0;

		bool                                    mIsUseHDR = false;

		void Default();
	};
	class _EchoComponentExport HDRComponentInfo : public ActorComponentInfo
	{
	public:
		HDRHandle								mHDRHandle;
		HDRComponentInfo() {};
		HDRComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~HDRComponentInfo() = default;
		virtual void							ExportData(cJSON* jsonNode) override;
		virtual void							ImportData(cJSON* jsonNode) override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                            OverWrite() override;
		int										getPriority() const { return mHDRHandle.mPriority; };
		void									setPriority(const int& _priority) { mHDRHandle.mPriority = _priority; }
		Vector4									getEyeAdaption() const { return mHDRHandle.EyeAdaption; }
		void									setEyeAdaption(const Vector4& EyeAdaption) { mHDRHandle.EyeAdaption = EyeAdaption; }
		Vector4									getBright() const { return mHDRHandle.Bright; }
		void									setBright(const Vector4& Bright) { mHDRHandle.Bright = Bright; }
		Vector4									getEyeToneMapping() const { return mHDRHandle.EyeToneMapping; }
		void									setEyeToneMapping(const Vector4& EyeToneMapping) { mHDRHandle.EyeToneMapping = EyeToneMapping; }
		std::vector<float>						getEyeToneMappingCurve() const { return mHDRHandle.EyeToneMappingCurve; }
		void									setEyeToneMappingCurve(const std::vector<float>& EyeToneMappingCurve) { mHDRHandle.EyeToneMappingCurve = EyeToneMappingCurve; }
		ColorValue								getEyeToneMappingColor() const { return mHDRHandle.EyeToneMappingColor; }
		void									setEyeToneMappingColor(const ColorValue& color) { mHDRHandle.EyeToneMappingColor = color; }
		
	public:
		EDITOR_REFLECT
	};
	class  _EchoComponentExport HDRComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(HDRComponent)
	/*	RenderSystem*								pRenderSystem = nullptr;
		RenderStrategy*								pStrategy = nullptr;
		PostProcessChain*							pProcessChain = nullptr;*/
		HDR*										mHDR = nullptr;
		std::shared_ptr<HDRHandle>					mDefaultHDR;
	public:
		virtual void                                visibleStateChanged(bool isVisible) override;

		HDRHandle									getContext() const { return mContext; }
		void										init(ActorComponentInfo* info) override;
		virtual  void								unInit() override;
		bool										isCurrentActiveCom();
		int											getPriority() const ;
		void										setPriority(const int& val);
		float										getTransFormTime() const { return mContext.mTransFormTime; }
		void										setTransFormTime(float time) { mContext.mTransFormTime = time; }
		void										setEyeAdaption(const Vector4& EyeAdaption);
		void										setBright(const Vector4& Bright);
		void										setEyeToneMapping(const Vector4& EyeToneMapping);
		void										setEyeToneMappingCurve(const std::vector<float>& EyeToneMappingCurve);
		void										setEyeToneMappingColor(const ColorValue& EyeToneMappingColor);
		void										setIsUseCurrent();
		static void									setIsOpenHDR(bool IsOpen);
		void										use();
		void										unuse();
		virtual void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		static HDR*									getHDR();
		//void										getDefaultHDR();
		TODComponentManager*                        getTodComManager() { return m_tMgr; }
	private:
		TODComponentManager*                        m_tMgr = nullptr;
		HDRHandle								    mContext;
	};
	DEFINECOMFACTORY(HDRComponent, UNMOVEABLE);
}
#endif // !__ECHO_HDR_COMPONENT_H__
