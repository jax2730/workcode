

#ifndef __EchoPhysicsPagedLayer_H__
#define __EchoPhysicsPagedLayer_H__

#include "EchoComponentPrerequisites.h"
#include "EchoVector2.h"
#include "EchoVector3.h"
#include "EchoPhysicsManager.h"

namespace Echo
{

	class _EchoComponentExport PhysicsPagedLayer
	{
		friend class PhysicsPagedLayerManager;
	public:
		PhysicsPagedLayer(PhysicsPagedLayerManager* pManager, int _TargetSize, int _PageSize);
		~PhysicsPagedLayer();

		void updateAndSetTarget(const std::vector<std::pair<Vector3,bool>>& loadTargetVec);

		void update(const std::map<uint8,Vector3>& loadTargetMap);
		bool update(uint8 id, const Vector3& pos);

		void updatePage(uint8 id, const EchoIndex3& index, bool bSync);
		void updateNearbyPage(uint8 id, const EchoIndex3& index);

		void removeExpirePage(uint8 id, const EchoIndex3& centerIndex);

		void unloadPage(uint8 id);
		void updateLoadPageState();

		EchoIndex3 WorldPosToIndex(const Vector3& pos);
		EchoIndex3 WorldPosToNearbyIndex(const Vector3& pos);

		void addEchoPhysXObject(const EchoIndex3& index, EchoPhysXObject* pEchoPhysXObject);

		void setBox(const AxisAlignedBox& box);

		void setCellSize(float size) { m_cellSize = size; }

		void deletePage(PhysicsPage* pPage);

		bool isEmpty();

		PhysicsPage* newPage(const EchoIndex3& index);

		void SetTarget(int index, bool bForceAsync);
		void TargetPushBack();
		void TargetPopBack();
		void TargetErase(int index);
		void TargetSwap(int index1, int index2);
		int TargetSize();
	public:
		unsigned int mDepth = 0;
		float m_cellSize = 256.0f;
		int mPageSize = 1;
		AxisAlignedBox mBoundingBox;
		PhysicsPagedLayerManager* m_pPageManger = nullptr;
		std::map<EchoIndex3, PhysicsPage*> m_mapLoadPages;
		std::map<EchoIndex3, PhysicsPage*> m_mapPhysicsPages;
	private:
		struct LoadTargetData {
			bool mForceAsync = false;
			EchoIndex3 mLastCenterIndex = EchoIndex3::MIN;
			EchoIndex3 mLastNearbyIndex = EchoIndex3::MIN;
		};
		vector<LoadTargetData>::type mLoadTargets;
	}; // class PhysicsPagedLayer
} // namespace Echo

#endif // __EchoPhysicsPagedLayer_H__
