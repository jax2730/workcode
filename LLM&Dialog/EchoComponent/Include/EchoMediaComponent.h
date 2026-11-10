#pragma once
#ifndef EchoMediaComponent_h__
#define EchoMediaComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoNodeActorComponent.h"

namespace Echo
{
	class _EchoComponentExport MediaComListener
	{
	public:
		virtual ~MediaComListener() {}

		virtual void MediaStartPlayCallBack() {}

		virtual void MediaFinishPlayCallBack() {}
	};
	typedef set<MediaComListener*>::type MediaComListenerSet;
	
	class HWMediaPlayer;
	class MediaComponentRenderable;
	class _EchoComponentExport MediaComponent : public NodeActorComponent
	{
		friend class MediaComponentRenderable;

		DECLAREACTORCOMTYPE_H(MediaComponent)

	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void initRenderObject(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void visibleStateChanged(bool visibleState) override;

		void openMedia(const std::string& name);
		void openUrl(const std::string& url);
		void closeCurrentMedia();

		void start();
		void pause();
		void setLoop(bool b);
		bool getLoop() { return m_loop; }
		void setAutoStart(bool b);
		bool getAutoStart() { return m_autoStart; }
		void restart();
		
		float getAspectRatio();
		double getDuration();

		void addMediaComListener(MediaComListener* listener);
		void removeMediaComListener(MediaComListener* listener);

		virtual void createComRootNodeImpl() override;

		static void startPlayCallBack(void* userdata);
		static void finishPlayCallBack(void* userdata);
	private:
		MediaComponentRenderable* m_pRenderable;

		std::string m_CurrentMedia;
		bool m_loop;
		bool m_autoStart;

		MediaComListenerSet m_setMediaComListeners;
	};

	class _EchoComponentExport MediaComponentInfo : public ActorComponentInfo
	{
	public:
		MediaComponentInfo() {}
		MediaComponentInfo(const Name& comType, ActorCOMMoveableType moveType);
		virtual ~MediaComponentInfo();
		_DECLCOMINFO_VIRFUN

#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		std::string GetMediaName() { return m_MediaName; }
		void SetMediaName(const std::string& name) { m_MediaName = name; }

		bool GetLoop() { return m_Loop; }
		void SetLoop(bool loop) { m_Loop = loop; }

		bool GetAutoStart() { return m_AutoStart; }
		void SetAutoStart(bool autostart) { m_AutoStart = autostart; }

	public:
		std::string m_MediaName;
		bool m_Loop = false;
		bool m_AutoStart = false;

		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(MediaComponent, ALL_MOVEABLE)

	class MediaComponentRenderable : public SimpleRenderable
	{
	public:
		MediaComponentRenderable(MediaComponent* pComponent, Name name);
		virtual ~MediaComponentRenderable();

		void init(const Name& mediaName);
		
		// overload from Echo::MovableObject
		virtual void updateRenderQueue(RenderQueue* pRenderQueue) override;

		virtual void setCustomParameters(const Pass* pass) override;

		virtual	void setVisible(bool b) override;

	private:
		MediaComponent* mComponent;

		SceneNode* m_pSceneNode;

		RcBuffer* mVB;
		RcBuffer* mIB;
	};

}

#endif
