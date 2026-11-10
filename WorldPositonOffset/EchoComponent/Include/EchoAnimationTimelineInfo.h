#ifndef _EchoAnimation_Timeline_Info_h__
#define _EchoAnimation_Timeline_Info_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "EchoComDefineMacro.h"
#include "EchoResourceBase.h"
#include "EchoResourceManagerBase.h"



namespace Echo {

	class EchoHermite;
	struct AnimationTimelineInfoHandle {
		std::vector<EchoHermite*>                mAnimationVec;
		std::vector<std::pair<int,std::string>>  mClipType;
		float                                    mStartTime = 0.f;
		float                                    mEndTime = 2.f;
		float                                    mDeltaTime = 30.f;

#ifdef ECHO_EDITOR
		std::vector<std::string>         mClipName;
#endif

		AnimationTimelineInfoHandle() = default;

		void clearAnimationVec();

		void copyAnimationVec(const AnimationTimelineInfoHandle& rhs);

		AnimationTimelineInfoHandle(const AnimationTimelineInfoHandle& rhs);

		AnimationTimelineInfoHandle& operator=(const AnimationTimelineInfoHandle& rhs);
       
		virtual ~AnimationTimelineInfoHandle();
	};

	class AnimationTimeLineInfo;
	using  AnimationTimelineInfoPtr = SmartPtr<AnimationTimeLineInfo>;

	class _EchoComponentExport AnimationTimeLineInfoManager : public ResourceManagerBase, public Singleton<AnimationTimeLineInfoManager>
	{
	public:
		AnimationTimeLineInfoManager();
		~AnimationTimeLineInfoManager();

		AnimationTimelineInfoPtr               createManual(const Name& name, ManualResourceLoader* loader = 0);
	protected:
		virtual                                ResourceBase* createImpl(const Name& name, bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams) override;
	};

	class AnimationTimelineComponent;

	class _EchoComponentExport AnimationTimeLineInfo : public ResourceBase {
		friend class AnimationTimeLineInfoManager;
		friend class AnimationTimelineComponent;
	public:
		AnimationTimeLineInfo(ResourceManagerBase* creator, const Name& actFileName, bool isManual, ManualResourceLoader* loader);
		~AnimationTimeLineInfo();

		void    loadFile(const std::string& filePath);
    
		void    saveFile(const std::string& filePath);

	private:
		void    _loadFile(cJSON* pRoot);
		void    _saveFile(cJSON* pRoot);
	protected:
		virtual void prepareImpl(void) override;
		virtual void unprepareImpl(void) override {};
		virtual void loadImpl(void) override {};
		virtual void unloadImpl(void)override {};
		size_t calcMemUsage(void) const override;
	private:
	    
		AnimationTimelineInfoHandle      mDataHandle;

		std::string                      mVer = "1.2";
	};

}



#endif
