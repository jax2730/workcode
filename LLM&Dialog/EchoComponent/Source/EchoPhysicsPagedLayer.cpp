

#include "EchoComponentStableHeaders.h"
#include "EchoPhysicsPagedLayer.h"
#include "EchoPhysicsPage.h"
#include "EchoPhysicsManager.h"
#include "EchoPhysXManager.h"
#include "Actor/EchoActorSystem.h"

namespace Echo
{
	PhysicsPagedLayer::PhysicsPagedLayer(PhysicsPagedLayerManager* pManager, int _TargetSize, int _pageSize)
		: mPageSize(_pageSize), m_pPageManger(pManager){
		mLoadTargets.resize(_TargetSize);
	}

	PhysicsPagedLayer::~PhysicsPagedLayer()
	{
		for (auto it = m_mapPhysicsPages.begin(); it != m_mapPhysicsPages.end(); ++it)
		{
			delete it->second;
		}
		m_mapPhysicsPages.clear();
		m_mapLoadPages.clear();
		mLoadTargets.clear();
	}

	void PhysicsPagedLayer::update(const std::map<uint8, Vector3>& loadTargetMap) {
		bool bChanged = false;
		for (auto& iter : loadTargetMap) {
			if (update(iter.first, iter.second)) {
				bChanged = true;
			}
		}
		if (bChanged) {
			updateLoadPageState();
		}
	}

	void PhysicsPagedLayer::updateAndSetTarget(const std::vector<std::pair<Vector3, bool>>& loadTargetVec) {
		bool bChanged = false;
		int size = (int)loadTargetVec.size();
		for (int id = 0; id != size; ++id) {
			SetTarget(id, loadTargetVec[id].second);
			if (update((uint8)id, loadTargetVec[id].first)) {
				bChanged = true;
			}
		}
		if (bChanged) {
			updateLoadPageState();
		}
	}

	bool PhysicsPagedLayer::update(uint8 id, const Vector3& pos) {
		LoadTargetData& data = mLoadTargets[id];
		bool bChanged = false;

		EchoIndex3 centerIndex = WorldPosToIndex(pos);
		if (centerIndex != data.mLastCenterIndex)
		{
			updatePage(id, centerIndex, !data.mForceAsync);
			data.mLastCenterIndex = centerIndex;
			removeExpirePage(id, centerIndex);
			bChanged = true;
		}
		EchoIndex3 nearbyIndex = WorldPosToNearbyIndex(pos);
		if (nearbyIndex != data.mLastNearbyIndex || bChanged)
		{
			updateNearbyPage(id, nearbyIndex);
			data.mLastNearbyIndex = nearbyIndex;
			bChanged = true;
		}

		return bChanged;
	}

	void PhysicsPagedLayer::unloadPage(uint8 id) {
		auto it = m_mapLoadPages.begin();
		while (it != m_mapLoadPages.end())
		{
			it->second->setState(id, PageLoadStateType_Unload);
			++it;
		}
	}

	void PhysicsPagedLayer::updateLoadPageState() {
		auto it = m_mapLoadPages.begin();
		while (it != m_mapLoadPages.end())
		{
			it->second->updateState();
			if (it->second->isUnload()) {
				it = m_mapLoadPages.erase(it);
			}
			else {
				++it;
			}
		}
	}

	void PhysicsPagedLayer::updatePage(uint8 id, const EchoIndex3& index, bool bSync)
	{
		PageLoadStateType type = bSync ? PageLoadStateType_Syncload : PageLoadStateType_Asyncload;
		for (int i = -mPageSize; i <= mPageSize; ++i)
		{
			for (int j = -mPageSize; j <= mPageSize; ++j)
			{
				for (int k = -mPageSize; k <= mPageSize; ++k)
				{
					EchoIndex3 curIndex;
					curIndex.x = index.x + i;
					curIndex.y = index.y + j;
					curIndex.z = index.z + k;

					auto it = m_mapPhysicsPages.find(curIndex);
					if (it != m_mapPhysicsPages.end())
					{
						PhysicsPage* pPage = it->second;
						if (bSync || (pPage->getState(id) != PageLoadStateType_Syncload)) {
							pPage->setState(id, type);
						}
						m_mapLoadPages[curIndex] = pPage;
					}
				}
			}
		}



	}

	void PhysicsPagedLayer::updateNearbyPage(uint8 id, const EchoIndex3& index)
	{
		if (index == EchoIndex3::ZERO) return;
		LoadTargetData& data = mLoadTargets[id];

		int mPageSizeA = mPageSize + 1;
		int xStart = -mPageSize, yStart = -mPageSize, zStart = -mPageSize, xEnd = mPageSize, yEnd = mPageSize, zEnd = mPageSize;

		if (index.x != 0) {
			if ((index.x & 1) == 1) {
				xStart = -mPageSizeA;
			}
			if ((index.x & 2) == 2) {
				xEnd = mPageSizeA;
			}
		}
		if (index.y != 0) {
			if ((index.y & 1) == 1) {
				yStart = -mPageSizeA;
			}
			if ((index.y & 2) == 2) {
				yEnd = mPageSizeA;
			}
		}
		if (index.z != 0) {
			if ((index.z & 1) == 1) {
				zStart = -mPageSizeA;
			}
			if ((index.z & 2) == 2) {
				zEnd = mPageSizeA;
			}
		}

		for (int x = xStart; x <= xEnd; ++x) {
			for (int y = yStart; y <= yEnd; ++y) {
				for (int z = zStart; z <= zEnd; ++z) {
				
					EchoIndex3 curIndex(x, y, z);
					if (abs(x) <= mPageSize && abs(y) <= mPageSize && abs(z) <= mPageSize) {
						continue;
					}
					curIndex = curIndex + data.mLastCenterIndex;

					auto it = m_mapPhysicsPages.find(curIndex);
					if (it != m_mapPhysicsPages.end())
					{
						PhysicsPage* pPage = it->second;
						pPage->setState(id, PageLoadStateType_Asyncload);
						m_mapLoadPages[curIndex] = pPage;
					}
				}
			}
		}

	}

	void PhysicsPagedLayer::removeExpirePage(uint8 id, const EchoIndex3& centerIndex)
	{
		int mPageSizeA = mPageSize + 1;
		auto it = m_mapLoadPages.begin();
		while (it != m_mapLoadPages.end())
		{
			const EchoIndex3& index = it->first;
			if (abs(index.x - centerIndex.x) > mPageSizeA || abs(index.y - centerIndex.y) > mPageSizeA || abs(index.z - centerIndex.z) > mPageSizeA)
			{
				it->second->setState(id, PageLoadStateType_Unload);
			}
			else if (abs(index.x - centerIndex.x) > mPageSize || abs(index.y - centerIndex.y) > mPageSize || abs(index.z - centerIndex.z) > mPageSize)
			{
				it->second->setState(id, PageLoadStateType_Asyncload);
			}
			++it;
		}
	}

	Echo::EchoIndex3 PhysicsPagedLayer::WorldPosToIndex(const Vector3& pos)
	{
		const Vector3& m_worldOrigin = m_pPageManger->GetPhysicsManager()->GetActorManager()->getWorldOrigin();

		Vector3 relCenterPos = pos - m_worldOrigin;

		EchoIndex3 centerIndex;

		centerIndex.x = static_cast<int>(floor(relCenterPos.x / m_cellSize));
		centerIndex.y = static_cast<int>(floor(relCenterPos.y / m_cellSize));
		centerIndex.z = static_cast<int>(floor(relCenterPos.z / m_cellSize));

		return centerIndex;
	}

	Echo::EchoIndex3 PhysicsPagedLayer::WorldPosToNearbyIndex(const Vector3& pos)
	{
		const Vector3& m_worldOrigin = m_pPageManger->GetPhysicsManager()->GetActorManager()->getWorldOrigin();


		Vector3 relCenterPos = pos - m_worldOrigin;

		float x = relCenterPos.x / m_cellSize;
		float y = relCenterPos.y / m_cellSize;
		float z = relCenterPos.z / m_cellSize;

		Vector3 dis;
		dis.x = x - static_cast<int>(floor(x));
		dis.y = y - static_cast<int>(floor(y));
		dis.z = z - static_cast<int>(floor(z));

		EchoIndex3 nearbyIndex(EchoIndex3::ZERO);

		float mAsyncLoadWidth = m_pPageManger->getAsyncLoadWidth();
		float mAsyncLoadWidthN1 = 1.0f - mAsyncLoadWidth;

		if (dis.x >= mAsyncLoadWidthN1) {
			nearbyIndex.x += 2;
		}
		if (dis.x < mAsyncLoadWidth) {
			nearbyIndex.x += 1;
		}

		if (dis.y >= mAsyncLoadWidthN1) {
			nearbyIndex.y += 2;
		}
		if (dis.y < mAsyncLoadWidth) {
			nearbyIndex.y += 1;
		}

		if (dis.z >= mAsyncLoadWidthN1) {
			nearbyIndex.z += 2;
		}
		if (dis.z < mAsyncLoadWidth) {
			nearbyIndex.z += 1;
		}
		
		return nearbyIndex;
	}

	void PhysicsPagedLayer::addEchoPhysXObject(const EchoIndex3& index, EchoPhysXObject* pEchoPhysXObject)
	{
		if (NULL == pEchoPhysXObject)
		{
			return;
		}

		PhysicsPage* pPage = NULL;
		auto it = m_mapPhysicsPages.find(index);
		if (it != m_mapPhysicsPages.end())
		{
			pPage = it->second;
		}
		else
		{
			pPage = newPage(index);
		}

		pPage->addEchoPhysXObject(pEchoPhysXObject);

	}

	PhysicsPage* PhysicsPagedLayer::newPage(const EchoIndex3& index) {
		PhysicsPage* pPage = new PhysicsPage();
		pPage->mIndex = index;
		pPage->mDepth = mDepth;
		pPage->m_pPagedLayer = this;
		m_mapPhysicsPages[index] = pPage;
		m_pPageManger->setNeedUpdate(true);

		int mPageSizeA = mPageSize + 1;

		uint8 size = (uint8)mLoadTargets.size();
		for (uint8 id = 0; id != size; ++id) {
			LoadTargetData& data = mLoadTargets[id];

			int x = abs(index.x - data.mLastCenterIndex.x);
			int y = abs(index.y - data.mLastCenterIndex.y);
			int z = abs(index.z - data.mLastCenterIndex.z);

			if (x > mPageSizeA || y > mPageSizeA || z > mPageSizeA) {
				pPage->setState(id, PageLoadStateType_Unload);
			} 
			else if (x <= mPageSize && y <= mPageSize && z <= mPageSize) {
				pPage->setState(id, data.mForceAsync ? PageLoadStateType_Asyncload : PageLoadStateType_Syncload);
			} 
			else {
				pPage->setState(id, PageLoadStateType_Asyncload);
			}
		}
		pPage->updateState();
		if (!pPage->isUnload()) {
			m_mapLoadPages[index] = pPage;
		}
		return pPage;
	}
	void PhysicsPagedLayer::SetTarget(int index, bool bForceAsync) {
		mLoadTargets[index].mForceAsync = bForceAsync;
	}
	void PhysicsPagedLayer::TargetPushBack() {
		mLoadTargets.push_back({});
	}
	void PhysicsPagedLayer::TargetPopBack() {
		unloadPage((uint8)(mLoadTargets.size() - 1));
		mLoadTargets.pop_back();
	}
	void PhysicsPagedLayer::TargetErase(int index) {
		TargetSwap(index, (int)(mLoadTargets.size() - 1));
		TargetPopBack();
	}
	void PhysicsPagedLayer::TargetSwap(int index1, int index2) {
		
		LoadTargetData temp = mLoadTargets[index1];
		mLoadTargets[index1] = mLoadTargets[index2];
		mLoadTargets[index2] = temp;

		auto it = m_mapLoadPages.begin();
		while (it != m_mapLoadPages.end())
		{
			PageLoadStateType type = it->second->getState(index1);
			it->second->setState(index1, it->second->getState(index2));
			it->second->setState(index2, type);
			++it;
		}
	}
	int PhysicsPagedLayer::TargetSize() {
		return (int)mLoadTargets.size();
	}

	void PhysicsPagedLayer::setBox(const AxisAlignedBox& box)
	{
		mBoundingBox = box;
	}

	void PhysicsPagedLayer::deletePage(PhysicsPage* pPage)
	{
		auto it = m_mapPhysicsPages.find(pPage->mIndex);
		if (it == m_mapPhysicsPages.end())
		{
			assert(false);
			return;
		}

		delete it->second;

		m_mapLoadPages.erase(it->first);
		m_mapPhysicsPages.erase(it);


	}

	bool PhysicsPagedLayer::isEmpty()
	{
		return m_mapPhysicsPages.empty();
	}

} // namespace Echo
