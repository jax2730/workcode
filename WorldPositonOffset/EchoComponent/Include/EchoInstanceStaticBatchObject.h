#ifndef __EchoInstanceStaticBatchObject_H__
#define __EchoInstanceStaticBatchObject_H__

#include "EchoPrerequisites.h"
#include "EchoMesh.h"
#include "EchoSceneManager.h"

namespace Echo
{
	struct ObjectInstanceData
	{
		Vector3 position;
		Vector3 scale;
		Quaternion orientation;
	}; // struct EntityInstanceData

	using ObjectInstanceDataVec = std::vector<ObjectInstanceData>;

	class InstanceStaticBatchObject : public EchoAllocatedObject
	{
	public:
		InstanceStaticBatchObject(SceneManager* pSceneManager);
		~InstanceStaticBatchObject();

		bool init(const MeshPtr& mesh, const Vector3& offset, const Vector3& scale, const Quaternion& ori);
		void initVertexAndIndexSubData();
		MeshPtr& getMesh();
		void addInstance(const ObjectInstanceData& data);
		void addInstance(const ObjectInstanceDataVec& datas);
		void clearInstance();
		void onlyclearInstance();
		void setUpdateState(bool isupdate);
		void setWorldSeting(const Vector3& offset, const Vector3& scale, const Quaternion& ori);
		const AxisAlignedBox& getWorldAABB() const;
		Vector3& getWorldPosition();
		Vector3& getWorldScale();
		Quaternion& getQuaternion();

		void updateRenderQueue(PassType passType, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue);

		static void Init();
		static void Destroy();

		static bool TestSupported(SubMesh* pSubMesh);
	protected:
		class InstanceStaticBatchObjectPrivate* dp = nullptr;
		static bool m_bInitUtil;
	};
	
	class TrafficBatchObject : public MovableObject
	{
	public:
		TrafficBatchObject(SceneManager* pSceneManager);
		virtual ~TrafficBatchObject() { delete mStaticBatch; };

		/// override from SceneManager::Listener
		//virtual void findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue) override;
		//------------------------------------------------------------------
		// Override from MovableObject
		virtual Echo::MovableObjectType getMovableType() const override;
		virtual const Echo::AxisAlignedBox& getBoundingBox() const override;
		virtual float getBoundingRadius() const override;
		virtual void _notifyCurrentCamera(Camera* cam) override;
		virtual void updateRenderQueue(Echo::RenderQueue* pRenderQueue) override;
		virtual bool getCulled() override { return mCulled; };
		//------------------------------------------------------------------
		void init(const MeshPtr& mesh, const Vector3& offset, const Vector3& scale, const Quaternion& ori);
		void initInstanceData(const ObjectInstanceDataVec& data);
		void asyncLastInit();
		void addInstanceData(ObjectInstanceDataVec& data);
		void initWorldSet(const Vector3& scale, const Quaternion& ori);
		bool initInstanceStaticBatchObject(MeshPtr& mesh);
		ObjectInstanceDataVec& getData() { return mBatchChangeData; }

		int mLastRenderLevel = -1;
		InstanceStaticBatchObject* mStaticBatch;
		SceneManager* mSceneMgr;
		ObjectInstanceDataVec mBatchChangeData;
		MeshPtr mMesh;
		Vector3 mWorldOffset = Vector3(0.f, 0.f, 0.f);
		Vector3 mWorldScale = Vector3(1.f, 1.f, 1.f);
		Quaternion mWorldOri;
		float			mRenderDistance = 0.f;
		float			m_fLodDistance[LOD_LEVEL_NUM];
		bool			mCulled = false;
		EM_LOD_LEVEL	mCurrLodLevel = LOD_LEVEL_0;

		Echo::AxisAlignedBox mBounds;
		float mRadius;
	};
}

#endif
