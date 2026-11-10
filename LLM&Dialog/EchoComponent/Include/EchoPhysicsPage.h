

#ifndef __EchoPhysicsPage_H__
#define __EchoPhysicsPage_H__

#include "EchoComponentPrerequisites.h"
#include "EchoVector2.h"
#include "EchoVector3.h"

namespace Echo
{
	class EchoPhysicsManager;
	class PhysicsPage;

	enum EchoPhysXObjectType : uint8 {
		ePhysXINVALIDObjectType = 0,
		ePhysXStaticObjectType = 1 << 0,
		ePhysXDynamicObjectType = 1 << 1,
		ePhysXActorObjectType = ePhysXStaticObjectType | ePhysXDynamicObjectType,
		ePhysXGroupObjectType = 1<< 2,
	};

	enum PageLoadStateType : uint8 {
		PageLoadStateType_Unload = 0,
		PageLoadStateType_Syncload = 1,
		PageLoadStateType_Asyncload = 2
	};

	class _EchoComponentExport EchoPhysXObject {
	public:
		EchoPhysXObject(EchoPhysXObjectType _type);
		virtual ~EchoPhysXObject();
		void ResetState();
		virtual void PhysxStateChanged(PageLoadStateType type) = 0;
		virtual AxisAlignedBox getPhysXWorldAABB() = 0;

		bool getIsUpdateWorldAABB() const { return m_bUpdateWorldAABB; }
		void setIsUpdateWorldAABB(bool value) { m_bUpdateWorldAABB = value; }
		bool getIsWaitAsyncLoad() const { return m_bWaitAsyncLoad; }
		void setIsWaitAsyncLoad(bool value) { m_bWaitAsyncLoad = value; }
		bool isStatic() { return m_bPhysXStaticObjectType; }
		bool isDynamic() { return m_bPhysXDynamicObjectType; }
		bool isGroup() { return m_bPhysXGroupObjectType; }
		EchoPhysXObjectType getPhysXType();
	public:
		PhysicsPage* m_pPhysicsPage = nullptr;
	private:
		uint8 m_bUpdateWorldAABB : 1;
		uint8 m_bWaitAsyncLoad : 1;
		uint8 m_bPhysXStaticObjectType : 1;
		uint8 m_bPhysXDynamicObjectType : 1;
		uint8 m_bPhysXGroupObjectType : 1;
	};

	class _EchoComponentExport PagdLoadState {
	public:
		PagdLoadState() {}
		PagdLoadState(uint32 _V);
		PagdLoadState(bool _V);
		virtual ~PagdLoadState() {}
		bool get(uint8 index);
		void set(uint8 index, bool value);
		operator bool();
		PagdLoadState& operator=(const PagdLoadState& other);
	private:
		uint32 mV = 0U;
		static std::array<uint32, 32> gBitToValue;
	};

	class _EchoComponentExport PhysicsPage
	{
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
	public:
		PhysicsPage();
		~PhysicsPage();

		void addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);

		bool isEmpty();
		bool isUnload();
		bool isAsync();
		bool isSync();
		bool isUnload(uint8 id);
		bool isAsync(uint8 id);
		bool isSync(uint8 id);
		void setState(uint8 id, PageLoadStateType type);
		PageLoadStateType getState();
		PageLoadStateType getState(uint8 id);
		void updateState();
		PageLoadStateType getLastState() { return m_lastState; }
	public:
		unsigned int mDepth = 0;
		EchoIndex3 mIndex;
		PhysicsPagedLayer* m_pPagedLayer = nullptr;
		std::unordered_set<EchoPhysXObject*> m_setEchoPhysXObjects;
	private:
		PagdLoadState m_bSyncLoad = false;
		PagdLoadState m_bAsyncLoad = false;
		PageLoadStateType m_lastState = PageLoadStateType_Unload;
	}; // class PhysicsPage
} // namespace Echo

#endif // __EchoPhysicsPage_H__
