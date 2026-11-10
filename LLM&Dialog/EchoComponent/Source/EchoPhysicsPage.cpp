

#include "EchoComponentStableHeaders.h"
#include "EchoPhysicsPage.h"
#include "EchoPhysicsPagedLayer.h"

namespace Echo
{
	//uint32 PagdLoadState::gNUMBER = 2;
	std::array<uint32, 32> PagdLoadState::gBitToValue = {
		1U << 0,
		1U << 1,
		1U << 2,
		1U << 3,
		1U << 4,
		1U << 5,
		1U << 6,
		1U << 7,
		1U << 8,
		1U << 9,
		1U << 10,
		1U << 11,
		1U << 12,
		1U << 13,
		1U << 14,
		1U << 15,
		1U << 16,
		1U << 17,
		1U << 18,
		1U << 19,
		1U << 20,
		1U << 21,
		1U << 22,
		1U << 23,
		1U << 24,
		1U << 25,
		1U << 26,
		1U << 27,
		1U << 28,
		1U << 29,
		1U << 30,
		1U << 31
	};
	bool PagdLoadState::get(uint8 index) { return (mV & gBitToValue[index]) != 0; }
	void PagdLoadState::set(uint8 index, bool value) {
		if (value) {
			mV = mV | gBitToValue[index];
		}
		else {
			mV = mV & (~gBitToValue[index]);
		}
	}
	PagdLoadState::operator bool() { 
		//return (mV & gBelowBitToValue[gNUMBER]) != 0;
		return mV != 0U;
	}
	PagdLoadState& PagdLoadState::operator=(const PagdLoadState& other) {
		if (this == &other) return *this;
		mV = other.mV;
		return *this;
	}
	PagdLoadState::PagdLoadState(uint32 _V) : mV(_V) {}
	PagdLoadState::PagdLoadState(bool value) { 
		//mV = value ? gBelowBitToValue[gNUMBER] : (uint32)0; 
		mV = value ? 0xFFFFFFFF : 0U;
	}

	PhysicsPage::PhysicsPage()
	{
		
	}

	PhysicsPage::~PhysicsPage()
	{

	}

	void PhysicsPage::addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject)
	{
		if (NULL == pEchoPhysXObject)
		{
			return;
		}
		if (pEchoPhysXObject->m_pPhysicsPage)
		{
			pEchoPhysXObject->m_pPhysicsPage->removeEchoPhysXObject(pEchoPhysXObject);
		}
		m_setEchoPhysXObjects.insert(pEchoPhysXObject);
		pEchoPhysXObject->m_pPhysicsPage = this;
		pEchoPhysXObject->PhysxStateChanged(m_lastState);
	}


	void PhysicsPage::removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject)
	{
		if (NULL == pEchoPhysXObject)
		{
			return;
		}
		m_setEchoPhysXObjects.erase(pEchoPhysXObject);
		pEchoPhysXObject->m_pPhysicsPage = nullptr;
	}

	bool PhysicsPage::isEmpty()
	{
		return m_setEchoPhysXObjects.empty();
	}

	bool PhysicsPage::isUnload() {
		return getState() == PageLoadStateType_Unload;
	}

	bool PhysicsPage::isAsync() {
		return getState() == PageLoadStateType_Asyncload;
	}

	bool PhysicsPage::isSync() {
		return getState() == PageLoadStateType_Syncload;
	}

	bool PhysicsPage::isUnload(uint8 id) {
		return getState(id) == PageLoadStateType_Unload;
	}

	bool PhysicsPage::isAsync(uint8 id) {
		return getState(id) == PageLoadStateType_Asyncload;
	}

	bool PhysicsPage::isSync(uint8 id) {
		return getState(id) == PageLoadStateType_Syncload;
	}

	PageLoadStateType PhysicsPage::getState() {
		return m_bSyncLoad ? PageLoadStateType_Syncload : (m_bAsyncLoad ? PageLoadStateType_Asyncload : PageLoadStateType_Unload);
	}
	PageLoadStateType PhysicsPage::getState(uint8 id) {
		return m_bSyncLoad.get(id) ? PageLoadStateType_Syncload : (m_bAsyncLoad.get(id) ? PageLoadStateType_Asyncload : PageLoadStateType_Unload);
	}
	void PhysicsPage::setState(uint8 id, PageLoadStateType type) {
		switch (type) {
		case PageLoadStateType_Unload: {
			m_bSyncLoad.set(id, false);
			m_bAsyncLoad.set(id, false);
			break;
		}
		case PageLoadStateType_Syncload: {
			m_bSyncLoad.set(id, true);
			m_bAsyncLoad.set(id, true);
			break;
		}
		case PageLoadStateType_Asyncload: {
			m_bSyncLoad.set(id, false);
			m_bAsyncLoad.set(id, true);
			break;
		}
		default: {
			break;
		}
		}
	}

	void PhysicsPage::updateState() {
		PageLoadStateType newType = getState();
		if (m_lastState == newType) {
			return;
		}
		for (auto iter = m_setEchoPhysXObjects.begin(); iter != m_setEchoPhysXObjects.end(); ++iter)
		{
			(*iter)->PhysxStateChanged(newType);
		}
		m_lastState = newType;
	}
	EchoPhysXObject::EchoPhysXObject(EchoPhysXObjectType _type) 
		: m_bUpdateWorldAABB(false), 
		m_bWaitAsyncLoad(false), 
		m_bPhysXStaticObjectType(_type == ePhysXStaticObjectType),
		m_bPhysXDynamicObjectType(_type == ePhysXDynamicObjectType),
		m_bPhysXGroupObjectType(_type == ePhysXGroupObjectType)
	{
		
	}
	EchoPhysXObject::~EchoPhysXObject() 
	{
	}
	void EchoPhysXObject::ResetState() {
		if (m_pPhysicsPage != nullptr) {
			PhysxStateChanged(m_pPhysicsPage->getLastState());
		}
	}

	EchoPhysXObjectType EchoPhysXObject::getPhysXType() {
		return (EchoPhysXObjectType)(m_bPhysXStaticObjectType ? ePhysXStaticObjectType : (m_bPhysXDynamicObjectType ? ePhysXDynamicObjectType : (m_bPhysXGroupObjectType ? ePhysXGroupObjectType : ePhysXINVALIDObjectType)));
	}

} // namespace Echo
