#ifndef __ECHO_HDR_ADAPTIVECOMPONENT_H__
#define __ECHO_HDR_ADAPTIVECOMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoTODComponentManager.h"

namespace Echo
{
	struct HDRAdaptiveHandle
	{

		//Vector4(0.1f, 0.1f, 1.5f, 0.05f); 会有人眼自适应效果的参数
		//0.15f,0.7f,0.5f,0.05f, 不会有人眼自适应效果的参数

		float									mEyeAdaptionX = 0.1f;
		float									mEyeAdaptionY = 0.1f;
		float									mEyeAdaptionZ = 1.5f;
		float									mEyeAdaptionW = 0.05f;

		float									mTransFormTime = 0.f;
		int										mPriority = 0;
	};

	class _EchoComponentExport HDRAdaptiveComponentInfo : public ActorComponentInfo
	{
	public:
		HDRAdaptiveHandle						mHDRAdaptiveHandle;
		HDRAdaptiveComponentInfo() {};
		HDRAdaptiveComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~HDRAdaptiveComponentInfo() = default;
		virtual void							ExportData(cJSON* jsonNode) override;
		virtual void							ImportData(cJSON* jsonNode) override;
		virtual void							CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                            OverWrite() override;

#define MEMBER_OPERATION(name, type, data)	\
		void set##name(type value) { data.m##name = value; }	\
		type get##name() { return data.m##name; }

		MEMBER_OPERATION(Priority, int, mHDRAdaptiveHandle);
		MEMBER_OPERATION(EyeAdaptionX, float, mHDRAdaptiveHandle);
		MEMBER_OPERATION(EyeAdaptionY, float, mHDRAdaptiveHandle);
		MEMBER_OPERATION(EyeAdaptionZ, float, mHDRAdaptiveHandle);
		MEMBER_OPERATION(EyeAdaptionW, float, mHDRAdaptiveHandle);
#undef MEMBER_OPERATION

		EDITOR_REFLECT
	};

	class HDR;
	class _EchoComponentExport HDRAdaptiveComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(HDRAdaptiveComponent)
	public:
		static Echo::Name HDREyeAdaptionX;
		static Echo::Name HDREyeAdaptionY;
		static Echo::Name HDREyeAdaptionZ;
		static Echo::Name HDREyeAdaptionW;

	public:
		virtual void                                visibleStateChanged(bool isVisible) override;
		virtual void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void								init(ActorComponentInfo* info) override;
		virtual  void								unInit() override;

		void										use();
		void										unuse();

#define MEMBER_OPERATION(name, type, data)	\
		void set##name(type value);	\
		type get##name() { return data.m##name; }

		MEMBER_OPERATION(EyeAdaptionX, float, mContext);
		MEMBER_OPERATION(EyeAdaptionY, float, mContext);
		MEMBER_OPERATION(EyeAdaptionZ, float, mContext);
		MEMBER_OPERATION(EyeAdaptionW, float, mContext);
#undef MEMBER_OPERATION

		int											getPriority() const;
		void										setPriority(const int& val);

		float										getTransFormTime() const { return mContext.mTransFormTime; }
		void										setTransFormTime(float pTime) { mContext.mTransFormTime = pTime; }

		bool										isCurrentActiveCom();
		void										setIsUseCurrent();
		bool										isOpenHDR();

		HDR*										getHDR();
		TODComponentManager*						getTodComManager() { return m_tMgr; }
		HDRAdaptiveHandle							getContext() const { return mContext; }
	private:
		TODComponentManager*						m_tMgr = nullptr;
		HDRAdaptiveHandle							mContext;
		std::shared_ptr<HDRAdaptiveHandle>			mDefaultAdaptive;

	};
	DEFINECOMFACTORY(HDRAdaptiveComponent, UNMOVEABLE);
}

#endif // !__ECHO_HDR_ADAPTIVECOMPONENT_H__
