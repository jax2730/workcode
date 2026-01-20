///////////////////////////////////////////////////////////////////////////////
//
// @file EchoSphericalTerrainVegetation.cpp
// @author CHENSHENG
// @date 2024/06/11 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoStableHeaders.h"
#include "EchoBiomeVegetation.h"

#include "EchoBiomeVegGroup.h"
#include "EchoVegetationOptions.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoEasyAsynHandler.h"
#include "EchoBiomeVegLayerCluster.h"
#include "EchoVegetationAreaTestHelper.h"
#include "EchoBiomeVegRandom.h"
#include "EchoBiomeVegOpt.h"
#include "EchoProtocolComponent.h"
#include "EchoVegetationAreaTestHelper.h"
#include "EchoVegRuntimeMgr.h"
#include <deque>
#include <mutex>
#include <algorithm>
#include <cstdint>

#include "EchoProfilerInfoManager.h"

namespace Echo
{
	namespace SphVeg
	{
		//	======================================================================
		using _CreateInfo = BiomeVegetation::CreateInfo;
		using _VegLayerInfo = BiomeVegetation::VegLayerInfo;
		using _VegLayerInfoArray = BiomeVegetation::VegLayerInfoArray;
		using _VegLayerInfoMap = BiomeVegetation::VegLayerInfoMap;
		using _RenderList = std::vector<std::pair<float, class _Render*>>;
		const Vector3 _invalid = Vector3(NAN);
		//	======================================================================
		namespace
		{
			struct BufferQueue;

			// 缓冲块
			struct BufferBlock {
				RcBuffer* buffer = nullptr;
				uint32_t size = 0;
				double last_used_time = 0;
				BufferQueue* parent = nullptr;
			};

			// 队列缓冲信息
			struct BufferQueue {
				uint32_t block_size = 0;
				std::deque<BufferBlock> vacant_blocks; // 可用块
				uint32_t allocated_count = 0;          // 已分配块数
			};

			// 统计信息
			struct BufferStats
			{
				uint32_t total_blocks = 0;
				uint64_t total_size = 0;
				uint32_t active_blocks = 0;
				uint64_t active_size = 0;

				uint32_t total_dedicated_blocks = 0;
				uint64_t total_dedicated_size = 0;
				uint32_t active_dedicated_blocks = 0;
				uint64_t active_dedicated_size = 0;

				uint32_t total_destroyed = 0;
				uint32_t frame_destroyed = 0;
			};
			// 动态缓冲区管理器，负责分配、回收和清理 GPU 缓冲区
			class DynamicBufferManager
			{
			public:
				DynamicBufferManager(float hold_time);
				~DynamicBufferManager();

				bool allocate_buffer(const void* data, uint32_t size, BufferBlock& out_block);
				void release_buffer(BufferBlock& block);

				void checkOverDue(double current_time);
				void collectRuntimeData();
				const BufferStats& get_stats() const { return stats_; }

			private:
				bool allocate_buffer(uint32_t size, BufferBlock& out_block);
				bool allocate_from_queue(BufferQueue& queue, BufferBlock& out_block);
				bool allocate_dedicated(uint32_t size, BufferBlock& out_block);
				void release_all();
				size_t find_queue_index(uint32_t size) const;

			private:
				std::vector<BufferQueue> queues_;
				BufferStats stats_;
				std::mutex mutex_;
				float hold_time_;                 // 空闲缓冲区保留时间（秒）
			};

			bool DynamicBufferManager::allocate_buffer(uint32_t size, BufferBlock& out_block) {
				std::lock_guard<std::mutex> lock(mutex_);
				size_t queue_idx = find_queue_index(size);
				if (queue_idx != queues_.size()) {
					return allocate_from_queue(queues_[queue_idx], out_block);
				}
				return allocate_dedicated(size, out_block);
			}

			DynamicBufferManager::DynamicBufferManager(float hold_time)
				: hold_time_(hold_time)
			{	//(uint32)mInstanceCount * sizeof(_InsData)-- 32*32,64*32,128*32,256*32 ...
				std::vector<uint32_t> sizes = {1024, 2048, 4096, 8192, 16384, 32768, 65536};;

				queues_.reserve(sizes.size());
				for (uint32_t size : sizes) {
					BufferQueue queue;
					queue.block_size = size;
					queues_.push_back(queue);
				}
			}

			DynamicBufferManager::~DynamicBufferManager()
			{
				release_all();
			}

			bool DynamicBufferManager::allocate_buffer(const void* data, uint32_t size, BufferBlock& out_block) {
				if (!allocate_buffer(size, out_block)) {
					return false;
				}

				RenderSystem* rs = Root::instance()->getRenderSystem();
				rs->updateBuffer(out_block.buffer, data, size, false, 0, false);
				return true;
			}

			void DynamicBufferManager::release_buffer(BufferBlock& block)
			{
				if (!block.buffer) {
					return;
				}

				std::lock_guard<std::mutex> lock(mutex_);
				block.last_used_time = Root::instance()->getTimeElapsed();

				RenderSystem* rs = Root::instance()->getRenderSystem();
				if (!block.parent) {
					// 专用缓冲区：直接销毁
					rs->destoryBuffer(block.buffer);
					stats_.active_dedicated_blocks--;
					stats_.active_dedicated_size -= block.size;
					stats_.frame_destroyed++;
					stats_.total_destroyed++;
				}
				else {
					// 队列缓冲区：放回队列
					stats_.active_blocks--;
					stats_.active_size -= block.size;
					block.parent->vacant_blocks.push_front(block);
				}

				block.buffer = nullptr;
				block.parent = nullptr;
				block.size = 0;
				block.last_used_time = 0;
			}

			bool DynamicBufferManager::allocate_from_queue(BufferQueue& queue, BufferBlock& out_block) {
				if (!queue.vacant_blocks.empty()) {
					out_block = queue.vacant_blocks.front();
					queue.vacant_blocks.pop_front();
					out_block.last_used_time = Root::instance()->getTimeElapsed();

					stats_.active_blocks++;
					stats_.active_size += queue.block_size;

					return true;
				}

				RenderSystem* rs = Root::instance()->getRenderSystem();
				out_block.buffer = rs->createDynamicVB(nullptr, queue.block_size);
				if (!out_block.buffer) {
					return false;
				}
				out_block.size = queue.block_size;
				out_block.last_used_time = Root::instance()->getTimeElapsed();
				out_block.parent = &queue;

				queue.allocated_count++;
				stats_.total_blocks++;
				stats_.total_size += queue.block_size;
				stats_.active_blocks++;
				stats_.active_size += queue.block_size;
				return true;
			}

			bool DynamicBufferManager::allocate_dedicated(uint32_t size, BufferBlock& out_block) {
				RenderSystem* rs = Root::instance()->getRenderSystem();
				out_block.buffer = rs->createDynamicVB(nullptr, size);
				if (!out_block.buffer) {
					return false;
				}
				out_block.size = size;
				out_block.last_used_time = Root::instance()->getTimeElapsed();
				out_block.parent = nullptr;

				stats_.total_dedicated_blocks++;
				stats_.total_dedicated_size += size;
				stats_.active_dedicated_blocks++;
				stats_.active_dedicated_size += size;
				return true;
			}

			size_t DynamicBufferManager::find_queue_index(uint32_t size) const {
				auto it = std::lower_bound(
					queues_.begin(), queues_.end(), size,
					[](const BufferQueue& queue, uint32_t s) { return queue.block_size < s; });
				if (it != queues_.end()) {
					return std::distance(queues_.begin(), it);
				}
				return queues_.size();
			}

			void DynamicBufferManager::checkOverDue(double current_time)  {
				std::lock_guard<std::mutex> lock(mutex_);
				RenderSystem* rs = Root::instance()->getRenderSystem();

				for (auto& queue : queues_) {
					auto it = queue.vacant_blocks.begin();
					while (it != queue.vacant_blocks.end()) {
						if (current_time - it->last_used_time > hold_time_) {
							rs->destoryBuffer(it->buffer);
							stats_.frame_destroyed++;
							stats_.total_destroyed++;
							stats_.total_blocks--;
							stats_.total_size -= queue.block_size;
							queue.allocated_count--; 
							it = queue.vacant_blocks.erase(it);
						}
						else {
							++it;
						}
					}
				}
			}

			void DynamicBufferManager::collectRuntimeData()
			{
				std::lock_guard<std::mutex> lock(mutex_);
				size_t cpu_memory = sizeof(*this) + queues_.capacity() * sizeof(BufferQueue);
				size_t gpu_memory = stats_.active_dedicated_size;
				for (const BufferQueue& queue : queues_) {
					gpu_memory += static_cast<size_t>(queue.block_size) * queue.allocated_count;
				}
				VegRuntimeMgr::CollectRuntimeData(
					VegRuntimeType::CPU_MEM | VegRuntimeType::DYBATCH_CPU_MEM, cpu_memory);
				VegRuntimeMgr::CollectRuntimeData(
					VegRuntimeType::GPU_MEM | VegRuntimeType::DYBATCH_GPUPOOL_SIZE, gpu_memory);
			}

			void DynamicBufferManager::release_all() {
				std::lock_guard<std::mutex> lock(mutex_);
				RenderSystem* rs = Root::instance()->getRenderSystem();

				for (auto& queue : queues_) {
					for (auto& block : queue.vacant_blocks) {
						rs->destoryBuffer(block.buffer);
						stats_.total_destroyed++;
					}
					queue.vacant_blocks.clear();
					queue.allocated_count = 0;
				}

				stats_ = {};
			}

			class _OrthoBox
			{
			public:
				explicit _OrthoBox(const Matrix3& invBase = Matrix3::IDENTITY)
					:mInvBase(invBase) {};

				const Vector3& min()					const { return mMin; };
				const Vector3& max()					const { return mMax; };
				const Matrix3& invBase()				const { return mInvBase; };
				bool		   nullopt()				const { return mNullOpt; };

				Vector3 toObSpace(const Vector3& p)		const { return mInvBase * p; };
				void	toObSpace(Plane* planes)		const {
					for (int i = 0; i < 6; i++)
						planes[i].normal = toObSpace(planes[i].normal);
				};

				void setNullopt()
				{
					mNullOpt = true;
					mMin = Vector3(_MAGIC_FLAG);
					mMax = Vector3(_MAGIC_FLAG);
				}

				void setInvBase(const Matrix3& invBase)
				{
					assert(mNullOpt);
					mInvBase = invBase;
				}

				void merge(const Vector3& p, bool isObSpace)
				{
					const Vector3 realMergeP = isObSpace ? p : toObSpace(p);

					if (mNullOpt)
					{
						mMin = realMergeP;
						mMax = realMergeP;
					}
					else
					{
						mMin.makeFloor(realMergeP);
						mMax.makeCeil(realMergeP);
					}

					mNullOpt = false;
				}

				void merge(const _OrthoBox& box)
				{
					if (box.mNullOpt)
						return;

					const std::array<Vector3, 8> posArray = box.getPosArray();
					for (auto&& p : posArray)
						merge(p, false);
				}

				float getDisSqrt(const Vector3& p, bool isObSpace) const
				{
					if (mNullOpt)
						return std::numeric_limits<float>::max();
					return VegOpts::SquareDistPointToAABB(isObSpace ? p : toObSpace(p), mMin, mMax);
				}

				bool isVisible(const Plane* planes, bool isObSpace) const
				{
					if (mNullOpt)
						return false;

					if (isObSpace)
						return VegOpts::CheckVisible(planes, mMin, mMax);

					std::array<Plane, 6> tempPlanes = { planes[0], planes[1], planes[2], planes[3], planes[4], planes[5] };
					toObSpace(tempPlanes.data());
					return VegOpts::CheckVisible(tempPlanes.data(), mMin, mMax);
				}

				std::array<Vector3, 8> getPosArray()const
				{
					const Matrix3& orthoBase = mInvBase.Inverse();
					return {
						orthoBase * Vector3(mMin.x, mMin.y, mMin.z),
						orthoBase * Vector3(mMax.x, mMin.y, mMin.z),
						orthoBase * Vector3(mMax.x, mMin.y, mMax.z),
						orthoBase * Vector3(mMin.x, mMin.y, mMax.z),

						orthoBase * Vector3(mMin.x, mMax.y, mMin.z),
						orthoBase * Vector3(mMax.x, mMax.y, mMin.z),
						orthoBase * Vector3(mMax.x, mMax.y, mMax.z),
						orthoBase * Vector3(mMin.x, mMax.y, mMax.z),
					};
				}

				AxisAlignedBox getAABB()const
				{
					AxisAlignedBox ret;
					const std::array<Vector3, 8> posArray = getPosArray();
					for (auto&& p : posArray)
						ret.merge(p);
					return ret;
				}

			private:
				bool			mNullOpt = true;
				Vector3			mMin = Vector3(_MAGIC_FLAG);
				Vector3			mMax = Vector3(_MAGIC_FLAG);
				Matrix3			mInvBase = Matrix3::IDENTITY;
			private:
				inline static const float _MAGIC_FLAG = 9999.f;
			};

			struct alignas(16) _InsData
			{
				Vector4 pos = Vector4::ZERO;	//	xyz + scale
				uint8 normal_qua[4] = {};
			};
			using _InsDataArray = std::vector<_InsData>;


			DVector3 _DyGetSurfaceWs(SphericalTerrain* sphPtr, const DVector3& worldPos, Vector3* faceNormal = nullptr)
			{
				return sphPtr->getSurfaceWs(worldPos, faceNormal);
			}
		}

		//	======================================================================
		enum class _CFaceType : uint32
		{
			PositiveX = 0,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ,

			FaceTypeMax
		};
		using _RFaceType = uint32;

		enum class _NodeType : uint32
		{									//	---------------
			LB = 0U,						//	|  LB  |  RB  |
			RB,								//	---------------
			RT,								//	|  LT  |  RT  |
			LT,								//	---------------

			NodeTypeMax
		};

		union _NodeID
		{
			struct
			{
				uint32		face  : 8;
				uint32      index : 24;
			};

			uint32 _id = 0u;
		};


		inline static void _GetChildRect(_NodeType type, const FloatRect& rect, FloatRect& childRect)
		{
			switch (type)
			{
			case _NodeType::LB:
				childRect.left = rect.left;
				childRect.top = (rect.bottom + rect.top) / 2.f;
				childRect.right = (rect.left + rect.right) / 2.f;
				childRect.bottom = rect.bottom;
				break;
			case _NodeType::RB:
				childRect.left = (rect.left + rect.right) / 2.f;
				childRect.top = (rect.bottom + rect.top) / 2.f;
				childRect.right = rect.right;
				childRect.bottom = rect.bottom;
				break;
			case _NodeType::RT:
				childRect.left = (rect.left + rect.right) / 2.f;
				childRect.top = rect.top;
				childRect.right = rect.right;
				childRect.bottom = (rect.bottom + rect.top) / 2.f;
				break;
			case _NodeType::LT:
				childRect.left = rect.left;
				childRect.top = rect.top;
				childRect.right = (rect.left + rect.right) / 2.f;
				childRect.bottom = (rect.bottom + rect.top) / 2.f;
				break;
			default:
				break;
			}
		}

		inline static FloatRect _GetChildRect(_NodeType type, const FloatRect& rect)
		{
			FloatRect childRect(0.f, 0.f, 0.f, 0.f);
			_GetChildRect(type, rect, childRect);
			return childRect;
		}

		struct _DenseEffect
		{
			float denFactor = 1.f;
			bool  isEnable = false;
		};
		using _DenseEffectMap = std::map<int, std::vector<_DenseEffect>>;
		class _QuadTreeNode
		{
		public:
			inline	static void		FindVis(const _NodeID& nodeID, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const FloatRect& curRange, uint32 level = 0);
			inline	static void		FindBox(std::vector<AxisAlignedBox>& boxVec, const _NodeID& nodeID, float fCurTime, _Private* _dp, const FloatRect& nodeRange, uint32 level, uint32 findLevel);
			static void		GetRandomData(_NodeID nodeID, const FloatRect& nodeRange, float matSize, float faceSize,
				const _CreateInfo& info, const _DenseEffectMap& denEffMap,
				std::array<std::vector<std::vector<std::pair<Vector3, int>>>, 3>& randomDataArray,
				BiomeVegLayerTempCluster* resManager = nullptr);
			inline	static _NodeID	GetChildID(_NodeID pID, uint32 i)
			{
				_NodeID curChildID;
				GetChildID(pID, i, curChildID);
				return curChildID;
			}

			inline	static void		GetChildID(_NodeID pID, uint32 i, _NodeID& childID)
			{
				childID.face = pID.face;
				childID.index = (pID.index + 1) * 4u + i;
			}

			inline static bool IsLeaf(const FloatRect& r,uint32 level)
			{
				if (level >= BiomeVegOpt::GetDevideDepth() || (r.width() <= BiomeVegOpt::GetDevideSize() && r.height() <= BiomeVegOpt::GetDevideSize()))
					return true;
				return false;
			};
		};

		class _QuadTree
		{
		public:
			static void FindVis(uint32 faceIndex, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp);
			static void FindBox(std::vector<AxisAlignedBox>& boxVec, uint32 faceIndex, float fCurTime, _Private* _dp, uint32 findLevel);
		};

		//	======================================================================

		class _BoxPool{
		public:
			_BoxPool(uint32 faceNum)
			{
				mPool.resize(faceNum);
			}

			void update(const _NodeID& id, const _OrthoBox& box, float fCurTime)
			{
				mPool[id.face][id.index] = { box, fCurTime };
			}

			void collectRuntimeData()const
			{
				size_t memUse = sizeof(*this);
				memUse += this->mPool.capacity() * sizeof(decltype(*mPool.begin()));
				for (auto&& it : mPool)
					memUse += it.size() * sizeof(decltype(*it.begin()));
				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::BOX_CACHE_CPU_MEM, memUse);
			}

			void checkOverdue(float fCurTime)
			{
				for (auto&& faceIt : mPool)
				{
					for (auto it = faceIt.begin(); it != faceIt.end(); )
					{
						if (fCurTime - it->second.lastTime > mHoldTime)
							it = faceIt.erase(it);
						else
							it++;
					}
				}
			}

			const _OrthoBox& get(const _NodeID & id, const FloatRect & range, float fCurTime, _Private * _dp);

		private:

			struct _BoxCache
			{
				_OrthoBox box;
				float	  lastTime = 0.f;
			};
			
			float mHoldTime = 4.f;
			std::vector<std::unordered_map<uint32, _BoxCache>> mPool;
		};

		//	======================================================================
		class _Render : public Renderable
		{
		public:
			_Render(const _InsDataArray& insDataArray, const BiomeVegLayerIns& vegRes, uint32 lod, const Matrix3& invBase, const _Private* _dp);

			virtual ~_Render() override
			{
				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				if(!meshData.m_vertexBuffer.empty())
					meshData.m_vertexBuffer[0] = nullptr;
				meshData.m_IndexBufferData.m_indexBuffer = nullptr;
			}

			bool isCanDraw() const
			{
				return mCanDraw;
			};

			virtual void getWorldTransforms(const DBMatrix4** xform)const override { *xform = &mTrans; };
			virtual void getWorldTransforms(DBMatrix4* xform)		const override { *xform = mTrans; };
			virtual RenderOperation*	getRenderOperation()			  override { return &mRenderOperation; };
			virtual const Material*		getMaterial()				const override { return mRes.getMaterial(mLod, mPreZ); };
			virtual const LightList&	getLights()					const override { return mLightList; };
			virtual uint32				getInstanceNum()				  override { return getInsNum(); };
			inline float				getInsNumf()				const { return float(mInstanceCount) * (mInsDensity); };
			uint32						getLod()					const { return mLod; };
			const BiomeVegLayerIns&		getRes()					const { return mRes; }
			float						getShowDis()				const { return mRes.getVegTemp()->getRange(mLod); };
			const _InsDataArray&		getInsData()				const { return mInsDataArray; };
			const _OrthoBox&			getBox()					const { return mBox; };
			virtual void				setCustomParameters(const Pass* pPass)override;
			void						setPreZ(bool v)					  { mPreZ = v; }
			void						setInsDensity(float v)			  { mInsDensity = v; };
			inline uint32				getInsNum()					const ;

			PassType					calPassType(const Camera* pCamera, const _Private* _dp);

			void						setSphPos(const Vector3& pos) { setRenderData(pos, mCamPos); };
			void						setRenderData(const Vector3& sphPos, const DVector3& camPos)
			{
				mSphPos = sphPos;
				mCamPos = camPos;

				mTrans.makeTrans(mCamPos);
			}                       

		public:
			void collectRuntimeData()const;
		private:
			_Render(const _Render&)						= delete;
			const _Render& operator=(const _Render&)	= delete;

			void _Load();

		private:
			DBMatrix4		mTrans = DBMatrix4::IDENTITY;
			DVector3		mCamPos = DVector3::ZERO;

			Vector3			mSphPos = Vector3::ZERO;
			_InsDataArray	mInsDataArray;
			uint32			mLod = 0;
			float			mInsDensity = 1.f;
			bool			mPreZ = true;
			uint32			mInstanceCount = 0u;
			const BiomeVegLayerIns& mRes;
			_OrthoBox		mBox;
			RenderOperation mRenderOperation;
			bool			mCanDraw = false;
			LightList		mLightList;

			SphericalTerrain*	mSphPtr = nullptr;
			Vector2 mIdenticalRandSeed;				//In the same quad-tree node,this is a fixed value cross multi LOD.
		};
		using _RenderPtr = std::shared_ptr<_Render>;

		struct _TempInsData
		{
			uint32 rIndex;
			_InsData insData;
		};
		enum class _AsyncState { UnLoad, Loading, Loaded };
		class _RenderCache : public AsynTaskSubmitter
		{
			friend class _AsyncTask;
		public:
			_RenderCache(_NodeID nodeID, const FloatRect& nodeRange, const _Private* _dp, const Matrix3& invBase);

			float									getDis(uint32 lod)			const { return mLodDis[lod]; };
			const _OrthoBox&						getBox(uint32 lod)			const { return mLodBox[lod]; };
			const std::vector<_RenderPtr>&			getHandle(uint32 lod)		const { return mLodHandle[lod]; };

			float									getLastTime()	const { return mLastTime; };
			void									update(float t)		  { mLastTime = t; };
			void									recalculate(const AxisAlignedBox& box, _Private* _dp);
		public:
			void collectRuntimeData()const;
		private:
			void									_Recalculate(const _Private* _dp, std::map<int, std::array<std::vector<_InsData>, 3>>& hitInsDataMap);
		private:

			float					mLodDis[2] = { 0.f, 0.f };
			_OrthoBox				mLodBox[2];
			std::vector<_RenderPtr> mLodHandle[2];

			float mLastTime = 0.f;
			_NodeID mID;
			FloatRect mRange;

			_AsyncState mState = _AsyncState::UnLoad;
		};

		class _AsyncTask : public AsynTask
		{
		public:
			using AsynTask::AsynTask;	//	继承构造
			virtual bool task_executionImpl() override;
			virtual void task_finishedImpl() override;

			_NodeID id;
			FloatRect range;
			_CreateInfo taskInfo;
			float faceSize = 0.f;
			_DenseEffectMap taskDenseEffMap;

			const _Private* _dp = nullptr;

			std::map<int, std::array<std::vector<_InsData>, 3>> hitInsDataMap;
		private:
			_RenderCache* get() { return static_cast<_RenderCache*>(mAsynTaskSubmitter); };
		};

		class _RenderCachePool
		{
		public:
			explicit _RenderCachePool(float hTime) :mHoldTime(hTime) {};
			~_RenderCachePool() { for (auto&& it : mPool) it.second->lockAbortAllTask(); };
			void clear() { mPool.clear(); };
			void checkOverdue(float fCurTime);
			void recalculate(const AxisAlignedBox& box, _Private* _dp);

			void findVis(_NodeID id, const FloatRect& nodeRange, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const Matrix3& invBase);
		public:
			void collectRuntimeData() const;
		private:
			float mHoldTime = 0.f;
			std::map<uint32, std::unique_ptr<_RenderCache>>	mPool;
		};

		class _BatchRender : public Renderable
		{
		public:
			_BatchRender()
			{
				enableCustomParameters(true);
				enableInstanceRender(true);

				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer.resize(2);
			}

			void init(_InsDataArray& insDataArray, size_t count, uint32 lod, const BiomeVegLayerIns* res, DynamicBufferManager* memMgr)
			{
				clear();
				mMemPool = memMgr;

				if (insDataArray.size() < count)
					return;

				mLod = lod;
				mInstanceCount = (uint32)count;
				mRes = res;
			
				const BiomeVegLayerTempPtr& vegLayerTemp = mRes->getVegTemp();
				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer[0] = vegLayerTemp->getVertexBuffer(mLod);
				meshData.m_vertexCount = vegLayerTemp->getVertexCount(mLod);
				meshData.m_vertexBuffSize = vegLayerTemp->getVertexBuffSize(mLod);

				meshData.m_IndexBufferData.m_indexBuffer = vegLayerTemp->getIndexBuffer(mLod);
				meshData.m_IndexBufferData.m_indexCount = vegLayerTemp->getIndexCount(mLod);;
				meshData.m_indexBufferSize = vegLayerTemp->getIndexBuffSize(mLod);

				RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
				
				if (!mMemPool)
				{
					meshData.m_vertexBuffer[1] = pRenderSystem->createStaticVB(insDataArray.data(), (uint32)mInstanceCount * sizeof(_InsData));
				}
				else
				{
					mMemPool->allocate_buffer(insDataArray.data(), (uint32)mInstanceCount * sizeof(_InsData), mMemBlock);
					meshData.m_vertexBuffer[1] = mMemBlock.buffer;
				}
				mRenderOperation.mPrimitiveType = ECHO_TRIANGLE_LIST;
			}

			void clear()
			{
				RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				if (meshData.m_vertexBuffer[1])
				{
					if (!mMemPool)
					{
						pRenderSystem->destoryBuffer(meshData.m_vertexBuffer[1]);
					}
					else
					{
						if (mMemBlock.buffer)
						{
							mMemPool->release_buffer(this->mMemBlock);
						}
					}
					meshData.m_vertexBuffer[1] = nullptr;
				}
			}

			virtual ~_BatchRender() override
			{
				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer[0] = nullptr;
				meshData.m_IndexBufferData.m_indexBuffer = nullptr;
			}

			virtual void getWorldTransforms(const DBMatrix4** xform)const override { *xform = &mTrans; };
			virtual void getWorldTransforms(DBMatrix4* xform)		const override { *xform = mTrans; };
			virtual RenderOperation*	getRenderOperation()			  override { return &mRenderOperation; };
			virtual const Material*		getMaterial()				const override { return mRes->getMaterial(mLod, mPreZ); };
			virtual const LightList&	getLights()					const override { return sEmptyLightList; };
			virtual uint32				getInstanceNum()				  override { return mInstanceCount; };

			void						setDrawData(const Vector3& planetPos, const DVector3& camPos, bool preZ) {
				mSphPos = planetPos;
				mCamPos = camPos;
				mPreZ = preZ;

				mTrans.makeTrans(mCamPos);
			};

			virtual void				setCustomParameters(const Pass* pPass)override
			{
				RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

				pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, Vector4(
					mSphPos.x, mSphPos.y, mSphPos.z, 1.f));

				if (mLod == 0)
				{
					const DVector3 roleCamSpa = VegOpts::GetRolePushPos() - mCamPos;
					pRenderSystem->setUniformValue(this, pPass, U_VSCustom1, Vector4(
						(float)roleCamSpa.x, (float)roleCamSpa.y, (float)roleCamSpa.z, 1.f));
				}

				const _VegLayerInfo& vegLayerInfo = mRes->getInsData();
				const float scaleBase = vegLayerInfo.customParam ? vegLayerInfo.scaleBase : mRes->getVegTemp()->getScaleBase();
				const float scaleRange = vegLayerInfo.customParam ? vegLayerInfo.scaleRange : mRes->getVegTemp()->getScaleRange();

				const VegetationMeshTemplate* meshTemplate = &mRes->getVegTemp()->getMesh(mLod);
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom2, Vector4(
					meshTemplate->bounds.getMinimum().y,
					meshTemplate->bounds.getMaximum().y,
					scaleBase,
					scaleRange));

				const DVector3 toCamSpaPos = mSphPos - mCamPos;
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom3, Vector4((float)toCamSpaPos.x, (float)toCamSpaPos.y, (float)toCamSpaPos.z, 0.f));

				Vector4 uvPersent(1.f, 0.f, 0.f, 0.f);
				const BiomeVegLayerTempPtr& vegLayerTemp = mRes->getVegTemp();
				if (vegLayerTemp->getMultiUV() == 1)
					uvPersent = vegLayerTemp->getUVPercent();
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom5, uvPersent);

				ColorValue clrValue[2];
				float fLerpFactor = Math::Clamp(vegLayerInfo.lerpFactor, -0.9999f, 0.9999f);
				float fSlope = 1.f, fIntercept = 0.f;
				if (fLerpFactor >= 0)
				{
					fSlope = 1.f / (1.f - fLerpFactor);
					fIntercept = 0.f;
				}
				else
				{
					fSlope = 1.f / (1.f + fLerpFactor);
					fIntercept = fLerpFactor / (1.f + fLerpFactor);
				}

				clrValue[0] = ColorValue::gammaToLinear(vegLayerInfo.topDiffuse) * vegLayerInfo.topDiffuseRatio;
				clrValue[0].a = fSlope;
				clrValue[1] = ColorValue::gammaToLinear(vegLayerInfo.bottomDiffuse) * vegLayerInfo.bottomDiffuseRatio;
				clrValue[1].a = fIntercept;
#ifdef ECHO_EDITOR
				if (BiomeVegetationSparkler::ShouldRender(&(this->mRes)))
				{
					clrValue[0] = ColorValue::White;
					clrValue[1] = ColorValue::Green;
				}
#endif // ECHO_EDITOR
				pRenderSystem->setUniformValue(this, pPass, U_PSCustom0, clrValue, sizeof(clrValue));

				if (mLod == 0)
				{
					Vector4 g4 = Vector4::ZERO;
					mRes->getVegTemp()->getFadeFactorLod0(g4);
					pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister4, g4);
				}
				else
				{
					Vector4 g4 = Vector4::ZERO, g5 = Vector4::ZERO;
					mRes->getVegTemp()->getFadeFactorLod1(g4, g5);
					pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister4, g4);
					pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister5, g5);
				}
			}


			void collectRuntimeData()const
			{
				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::DYBATCH_CPU_MEM, sizeof(*this));
				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::GPU_MEM | VegRuntimeType::DYBATCH_GPU_MEM, mInstanceCount * sizeof(_InsData));
			}

		private:
			DBMatrix4		mTrans = DBMatrix4::IDENTITY;
			DVector3		mCamPos = DVector3::ZERO;

			uint32 mLod = 0u;
			uint32 mInstanceCount = 0u;
			const BiomeVegLayerIns* mRes = nullptr;
			RenderOperation mRenderOperation;

			bool	mPreZ = true;
			Vector3	mSphPos = Vector3::ZERO;

			inline static LightList sEmptyLightList;


			DynamicBufferManager* mMemPool = nullptr;
			BufferBlock mMemBlock;
		private:
			_BatchRender(_BatchRender&) = delete;
			_BatchRender& operator=(_BatchRender&) = delete;
		};

		class _DyBatchMgr
		{
			struct _BatchDesc
			{
				std::array<uint32, 2> lodInsCount = { 0u , 0u };
				std::vector<_Render*> renderArray;
			};

		public:
			_DyBatchMgr() {
				if (BiomeVegOpt::GetBatchMemPoolEnabled())
				{
					mDeviceMemPool = new DynamicBufferManager(5.f);
				}
			}

			~_DyBatchMgr() {
				mBatchRender.clear();
				for (auto&& it : mRenderPool)
					delete it;
				SAFE_DELETE(mDeviceMemPool);
			}

			void checkOverDue(float fCurTime) {
				if (mDeviceMemPool)
				{
					mDeviceMemPool->checkOverDue(fCurTime);
				}
			}

			void add(PassType pt, _Render* render)
			{
				_BatchDesc& batchDesc = mFrameBatchPool[pt][&render->getRes()];
				batchDesc.lodInsCount[render->getLod()] += render->getInsNum();
				batchDesc.renderArray.push_back(render);
			}

			void clear()
			{
				for (auto&& i : mFrameBatchPool)
				{
					for (auto&& j : i.second)
					{
						for (auto&& k : j.second.lodInsCount)
							k = 0u;
						j.second.renderArray.clear();
					}
				}
				mBatchRender.clear();
				resetRender();
			}

			void submit(RenderQueue* pQueue, const Vector3& planetPos, const DVector3& camPos)
			{
				if (mBatchRender.empty())
				{
					for (auto&& passIt : mFrameBatchPool)
					{
						const PassType& passType = passIt.first;
						for (auto&& layerInsIt : passIt.second)
						{
							const BiomeVegLayerIns* insPtr = layerInsIt.first;
							const _BatchDesc& batchDesc = layerInsIt.second;

							uint32 dyBatchNum[2] = { 0u , 0u };
							for (auto&& renderIt : batchDesc.renderArray)
								dyBatchNum[renderIt->getLod()] += 1u;

							static std::array<_InsDataArray, 2> lodInsData;
							for (uint32 i = 0; i < 2; i++)
							{
								if (batchDesc.lodInsCount[i] > lodInsData[i].size())
									lodInsData[i].resize(batchDesc.lodInsCount[i]);
							}

							size_t insIndex[2] = { 0, 0 };
							for (auto&& renderIt : batchDesc.renderArray)
							{
								const uint32 lod = renderIt->getLod();

								_InsDataArray& _dst = lodInsData[lod];
								size_t& _dst_index = insIndex[lod];

								const _InsDataArray& _src = renderIt->getInsData();
								uint32 instanceNum = renderIt->getInsNum();

								memcpy(_dst.data() + _dst_index, _src.data(), sizeof(_InsData) * instanceNum);
								_dst_index += instanceNum;
							}

							for (uint32 i = 0; i < 2; i++)
							{
								const size_t& _dst_index = insIndex[i];

								if (_dst_index)
								{
									_BatchRender* render = getRender();
								//	render->init(lodInsData[i], _dst_index, i, insPtr);
									render->init(lodInsData[i], _dst_index, i, insPtr, mDeviceMemPool);
									mBatchRender.push_back({ passType , render });

								}
							}
						}
					}
				}

				{
					for (auto&& it : mBatchRender)
					{
						it.second->setDrawData(planetPos, camPos, BiomeVegOpt::GetPreZEnabled());
						const Material* mat = it.second->getMaterial();
						if (nullptr == mat)
							continue;

						if (BiomeVegOpt::GetPreZEnabled())
						{
							const PassType passType = PassType::AlphaTestNoColor;
							pQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(mat->getPass(passType), it.second);
							BiomeVegOpt::StatisticsDraw(it.second);
						}

						{
							const PassType passType = it.first;
							pQueue->getRenderQueueGroup(RenderQueue_Vegetation)->addRenderable(mat->getPass(passType), it.second);
							BiomeVegOpt::StatisticsDraw(it.second);
						}
					}
				}
			}

		public:
			void collectRuntimeData()const
			{
				size_t cpuMemUse = sizeof(*this);
				cpuMemUse += mBatchRender.capacity() * sizeof(decltype(*mBatchRender.begin()));
				cpuMemUse += mRenderPool.capacity() * sizeof(decltype(*mRenderPool.begin()));

				for (auto&& it : mRenderPool)
				{
					if (it)
						it->collectRuntimeData();
				}

				for (auto&& it1 : mFrameBatchPool)
				{
					cpuMemUse += sizeof(it1);
					for (auto&& it2 : it1.second)
					{
						cpuMemUse += sizeof(it2);
						cpuMemUse += it2.second.renderArray.capacity() * sizeof(decltype(*it2.second.renderArray.begin()));
					}
				}

				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::DYBATCH_CPU_MEM, cpuMemUse);

				if (mDeviceMemPool)
				{
					mDeviceMemPool->collectRuntimeData();
				}
			}

		private:
			std::vector <std::pair<PassType, _BatchRender*>> mBatchRender;
			std::map<PassType, std::map<const BiomeVegLayerIns*, _BatchDesc>> mFrameBatchPool;
			DynamicBufferManager* mDeviceMemPool = nullptr;

			_BatchRender* getRender()
			{
				if (mCurIndex == mRenderPool.size())
				{
					size_t addCount = mCurIndex / 2;
					if (addCount < 5)
						addCount = (size_t)5;
					mRenderPool.reserve(mRenderPool.size() + addCount);

					for (size_t i = 0; i < addCount; i++)
						mRenderPool.push_back(new _BatchRender);
				}

				return mRenderPool[mCurIndex++];
			}

			void resetRender()
			{
				mCurIndex = 0;
			}

			size_t mCurIndex = 0;
			std::vector<_BatchRender*> mRenderPool;
		};

		//	======================================================================
		struct _Private
		{
			_CreateInfo		mInfo;
			BiomeVegGroup*	mGroup = nullptr;
			bool mIsReady = false;

			uint32	mFaceCount = 0u;
			float	mFaceSize = 0.f;

			std::unique_ptr<_BoxPool> mBoxCachePool;
			std::unique_ptr<_RenderCachePool> mRenderCachePool;

			std::map<int, std::vector<BiomeVegLayerIns>> mVegLayerInsMap;

			LightList mQueryLight;
			_RenderList mRenderList;

			_DyBatchMgr mDyBatchMgr;

			Vector3 mSphSpaceCamPos = Vector3::ZERO;
			std::array<Plane, 6> mSphSpaceCamPlanes;

			bool mUsePointLight = true;	//是否使用点光源
			bool mReceiveShadow = true;		//是否接受阴影
			float getRadius()const {
				return mInfo.sphRadius;
			}
			void init();
			void shutdown();
			void reDevide()
			{
				if (mInfo.sphPtr->isSphere())
				{
					mFaceCount = 6u;
					const float R = getRadius();
					mFaceSize = 2.f * R;
				}
				else if(mInfo.sphPtr->isTorus())
				{
					const float r = mInfo.sphPtr->m_relativeMinorRadius;
					const uint32 faceNum = (uint32)std::round(std::clamp(1.0f / r, 1.0f, 32.0f));
					mFaceSize = (Math::TWO_PI * getRadius()) / faceNum;
					mFaceCount = faceNum;
				}
				mIsReady = false;
			}

			void clearRenderCache() { mRenderCachePool = std::make_unique<_RenderCachePool>(mInfo.infoCacheHoldTime); };

			bool checkReady()
			{
				if (false == mIsReady)
				{
					mInfo.boxVisDistance = 128.f;

					for (auto&& it : mVegLayerInsMap)
					{
						for (BiomeVegLayerIns& ins : it.second)
						{
							if (false == ins.isLoaded())
								return false;

							for (int lod = 0; lod < 2; lod++)
							{
								if (ins.isCanDraw(lod))
								{
									mInfo.boxVisDistance = std::max(mInfo.boxVisDistance, BiomeVegOpt::GetRealDis(lod, ins.getVegTemp()->getRange(lod)));
								}
							}
						}
					}

					mInfo.boxVisDistance = std::min(mInfo.boxVisDistance, BiomeVegOpt::GetDisLimit(1));

					mBoxCachePool = std::make_unique<_BoxPool>(mFaceCount);
					mIsReady = true;
				}

				return mIsReady;
			}

			void updateQueryLights(const Camera* pCamera)
			{
				mQueryLight.clear();
				if (false == mUsePointLight) return;
				const SceneManager* sceneMgr = pCamera->getSceneManager();
				if (sceneMgr)
					sceneMgr->populateLightList(pCamera->getPosition(), BiomeVegOpt::GetDisLimit(0), mQueryLight);
			}

			_DenseEffectMap getDenseEffectMap()const
			{
				_DenseEffectMap denseEffMap;

				for (auto&& it : mInfo.vegLayerMap)
				{
					denseEffMap[it.first].resize(it.second.size());

					auto findIt = mVegLayerInsMap.find(it.first);
					if (findIt == mVegLayerInsMap.end())
						continue;

					for (uint32 i = 0; i < it.second.size(); i++)
					{
						if (i >= findIt->second.size())
							continue;
						if (false == findIt->second.at(i).isLoaded())
							continue;

						denseEffMap[it.first].at(i) = {
							findIt->second.at(i).getVegTemp()->getDenseFactor(),
							findIt->second.at(i).getVegTemp()->getDenseEffect()
						};
					}
				}

				return denseEffMap;
			}
		};
		//	======================================================================

		std::set<BiomeVegetation*> sAllSphTerVegObjs;
		bool _CheckVisible(const Camera* pCamera, const AxisAlignedBox& worldBox, float showDis);
}


	using namespace SphVeg;
	BiomeVegetation::BiomeVegetation(const CreateInfo& createInfo, BiomeVegGroup* groupPtr) : _dp(new _Private)
	{

		sAllSphTerVegObjs.insert(this);

		_dp->mGroup = groupPtr;
		_dp->mInfo = createInfo;
		_dp->init();

		{
			_RegisterData regTemp = { _dp->mInfo.sceneMgrPtr, _dp->mInfo.sphPtr };
			ProtocolComponentSystem::instance()->callProtocolFunc(Name("addBiomeVegetationData"), (void*)(&regTemp), 0u);
		}
	}

	BiomeVegetation::~BiomeVegetation()
	{
		{
			_RegisterData regTemp = { _dp->mInfo.sceneMgrPtr, _dp->mInfo.sphPtr };
			ProtocolComponentSystem::instance()->callProtocolFunc(Name("delBiomeVegetationData"), (void*)(&regTemp), 0u);
		}

		_dp->shutdown();
		SAFE_DELETE(_dp);
		sAllSphTerVegObjs.erase(this);
	}

	std::string _TransPrefixName(const std::string& name)
	{
		std::string ret = name;
		std::replace(ret.begin(), ret.end(), '/', '_');
		std::replace(ret.begin(), ret.end(), '\\', '_');
		return ret;
	}

	void BiomeVegetation::collectRuntimeData() const
	{
		size_t cpuMemUse = sizeof(*this);
		cpuMemUse += sizeof(_Private);
		for (auto&& it : _dp->mInfo.vegLayerMap)
		{
			cpuMemUse += sizeof(it);
			cpuMemUse += it.second.capacity() * sizeof(decltype(*it.second.begin()));
		}
		for (auto&& it : _dp->mVegLayerInsMap)
		{
			cpuMemUse += sizeof(it);
			for (auto&& insIt : it.second)
				insIt.collectRuntimeData();
		}

		VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM, cpuMemUse);

		if (_dp->mBoxCachePool)
			_dp->mBoxCachePool->collectRuntimeData();

		if (_dp->mRenderCachePool)
			_dp->mRenderCachePool->collectRuntimeData();

		_dp->mDyBatchMgr.collectRuntimeData();
	}

	const Name& BiomeVegetation::getName() const
	{
		return _dp->mInfo.biomeName;
	}

	void BiomeVegetation::findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue)
	{
		if (false == BiomeVegOpt::GetVisEnabled())
			return;
		if(false == _dp->checkReady())
			return;
		if (pCamera->getCameraUsage() != CameraUsage::SceneCamera)
			return;
		if (_dp->mFaceSize < 64) {
			return;
		}
		float fCurTime = Root::instance()->getTimeElapsed();

		_dp->updateQueryLights(pCamera);

		_dp->mDyBatchMgr.clear();
		_dp->mRenderList.clear();

		_dp->mSphSpaceCamPos = pCamera->getPosition() - _dp->mInfo.sphPos;
		const Plane* camPlane = pCamera->getFrustumPlanes();
		for (uint32 i = 0; i < 6; i++)
		{
			_dp->mSphSpaceCamPlanes[i] = camPlane[i];
			_dp->mSphSpaceCamPlanes[i].d += _dp->mInfo.sphPos.Dot(_dp->mSphSpaceCamPlanes[i].normal);
		}

		for (uint32 faceIndex = 0u; faceIndex < _dp->mFaceCount; faceIndex++)
		{
			_QuadTree::FindVis(faceIndex, pCamera, pQueue, fCurTime, _dp);
		}

		if (BiomeVegOpt::GetDyBatchEnabled())
		{
			_RenderList newList;
			newList.reserve(_dp->mRenderList.size());

			const uint32 dybatchLimit = BiomeVegOpt::GetDyBatchLimit();

			_dp->mDyBatchMgr.checkOverDue(fCurTime);
			for (auto&& it : _dp->mRenderList)
			{
				if (it.second->getInsNum() < dybatchLimit)
				{
					const PassType passType = it.second->calPassType(pCamera, _dp);
					_dp->mDyBatchMgr.add(passType, it.second);
				}
				else
					newList.push_back(it);
			}
			_dp->mDyBatchMgr.submit(pQueue, _dp->mInfo.sphPos, pCamera->getPosition());

			_dp->mRenderList = newList;
		}
		
		{
			std::sort(_dp->mRenderList.begin(), _dp->mRenderList.end(), [](auto l, auto r) {return l.first < r.first; });
			for (auto&& it : _dp->mRenderList)
			{
				it.second->setPreZ(BiomeVegOpt::GetPreZEnabled());
				const Material* mat = it.second->getMaterial();
				if (nullptr == mat)
					continue;

				if (BiomeVegOpt::GetPreZEnabled())
				{
					const PassType passType = PassType::AlphaTestNoColor;
					pQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(mat->getPass(passType), it.second);
					BiomeVegOpt::StatisticsDraw(it.second);
				}

				{
					const PassType passType = it.second->calPassType(pCamera, _dp);
					pQueue->getRenderQueueGroup(RenderQueue_Vegetation)->addRenderable(mat->getPass(passType), it.second);
					BiomeVegOpt::StatisticsDraw(it.second);
				}
			}
		}
		
		_dp->mRenderCachePool->checkOverdue(fCurTime);
		_dp->mBoxCachePool->checkOverdue(fCurTime);
	}

	void BiomeVegetation::onReDevide()
	{
		clearAll();
	}

	void BiomeVegetation::onDenLimitChange()
	{

	}

	void BiomeVegetation::onDisLevelChange()
	{
		if (_dp->mIsReady)
		{
			_dp->mInfo.boxVisDistance = std::min(_dp->mInfo.boxVisDistance, BiomeVegOpt::GetDisLimit(1));
		}
	}

	void BiomeVegetation::onChangeTerrainPos(const Vector3& pos)
	{
		if (pos == _dp->mInfo.sphPos)
			return;

		_dp->mInfo.sphPos = pos;
		_dp->clearRenderCache();
		_dp->mIsReady = false;
	}

	void BiomeVegetation::onChangeTerrainRot(const Quaternion& rot)
	{
		if(rot == _dp->mInfo.sphRot)
			return;

		_dp->mInfo.sphRot = rot;
		_dp->clearRenderCache();
		_dp->mIsReady = false;
	}

	void BiomeVegetation::onChangeTerrainRadius(float radius)
	{
		if (radius == _dp->mInfo.sphRadius)
			return;

		_dp->mInfo.sphRadius = radius;
		clearAll();
	}

	void BiomeVegetation::onChangeTerrainMaxOff(float maxOffset)
	{
		uint16 value = (uint16)std::ceil(maxOffset);
		if(value == _dp->mInfo.sphTerMaxOff)
			return;

		_dp->mInfo.sphTerMaxOff = value;
		clearAll();
	}

	void BiomeVegetation::clearRenderCache()
	{
		_dp->clearRenderCache();
		_dp->mIsReady = false;
	}

	void BiomeVegetation::clearAll()
	{
		_dp->clearRenderCache();
		_dp->reDevide();
		_dp->mIsReady = false;
	}

	void BiomeVegetation::recheckPosition(const AxisAlignedBox& box)
	{
		_dp->mRenderCachePool->recalculate(box, _dp);
	}

	void BiomeVegetation::updateLayerParam(int terIndex, const VegLayerInfo& vegLayerInfo)
	{
		auto findIt = _dp->mVegLayerInsMap.find(terIndex);
		if (findIt == _dp->mVegLayerInsMap.end())
			return;

		for (int i = 0; i < findIt->second.size(); i++)
		{
			BiomeVegLayerIns& ins = findIt->second.at(i);
			if (ins.getInsData().name == vegLayerInfo.name)
				ins.setInsData(vegLayerInfo);

			auto findInfoIt = _dp->mInfo.vegLayerMap.find(terIndex);
			if (findInfoIt != _dp->mInfo.vegLayerMap.end() &&
				findInfoIt->second.size() < i &&
				findInfoIt->second.at(i).name == vegLayerInfo.name)
			{
				findInfoIt->second.at(i) = vegLayerInfo;
			}
		}

		_dp->mIsReady = false;
	}

	void BiomeVegetation::updateAllVegRes(int terIndex, const VegLayerInfoArray& vegLayerInfoArray)
	{
		_dp->clearRenderCache();
		_dp->mVegLayerInsMap[terIndex].clear();

		_dp->mInfo.vegLayerMap[terIndex] = vegLayerInfoArray;
		for (const _VegLayerInfo& layerInfo : vegLayerInfoArray)
		{
			auto result = _dp->mGroup->getLayerTempCluster()->createOrRetrieve(Name(BiomeVegLayerTemp::ResToPathName(layerInfo.name)));
			BiomeVegLayerTempPtr vegLayerPtr = result.first;
			_dp->mVegLayerInsMap[terIndex].emplace_back(BiomeVegLayerIns(layerInfo, vegLayerPtr));
		}

		_dp->mIsReady = false;
	}

	void BiomeVegetation::removeAllVegRes(int terIndex)
	{
		_dp->clearRenderCache();
		_dp->mInfo.vegLayerMap.erase(terIndex);
		_dp->mVegLayerInsMap.erase(terIndex);

		_dp->mIsReady = false;
	}

	std::vector<VegTestRender*> renderVec;

	void BiomeVegetation::ShowBox(uint32 args)
	{
		for (auto&& it : renderVec)
			delete it;
		renderVec.clear();

		float fCurTime = Root::instance()->getTimeElapsed();

		std::vector<AxisAlignedBox> boxVec;
		for (auto&& vegPtr : sAllSphTerVegObjs)
		{
			auto _dp = vegPtr->_dp;

			if (false == _dp->checkReady())
				return;

			for (uint32 faceIndex = 0u; faceIndex < _dp->mFaceCount; faceIndex++)
			{
				_QuadTree::FindBox(boxVec, faceIndex, fCurTime, _dp, args);
			}

			if (!boxVec.empty())
			{
				uint32 showStep = 1000u;
				for (uint32 i = 0; i < boxVec.size(); i += showStep)
				{
					auto beg = boxVec.begin() + i;
					auto end = boxVec.begin() + std::min(i + showStep, (uint32)boxVec.size());
					if (beg == end) continue;

					renderVec.push_back(new VegTestRender(std::vector<AxisAlignedBox>(beg, end), Vector4(0.f, 1.f, 0.f, 0.2f)));
				}
			}
		}
	}


	bool sRecordFlag = false;
	std::vector<Vector3> sRecordRoad;
	std::vector<std::unique_ptr<VegTestRender>> sShowRoad;

	void BiomeVegetation::ShowRoad(uint32 args)
	{
		sShowRoad.clear();
		for (uint32 i = 0; i < (uint32)sRecordRoad.size(); i += 1000u)
		{
			auto beg = sRecordRoad.begin() + i;
			auto end = sRecordRoad.begin() + std::min(i + 1000u, (uint32)sRecordRoad.size());
			if (beg != end)
				sShowRoad.push_back(std::make_unique<VegTestRender>(std::vector<Vector3>(beg, end), Vector4(0.f, 1.f, 0.f, 1.f), VegTestRender::Model::Point));
		}
	}

	void BiomeVegetation::ClearRoad(uint32 args)
	{
		sRecordRoad.clear();
	}

	void BiomeVegetation::RecordRoad(uint32 args)
	{
		sRecordFlag = !sRecordFlag;
	}

	std::set<const void*> BiomeVegetation::GetBiomeVegIns(uint32 terrIdx, const std::string& name)
	{
		std::set<const void*> ret;

		for (const auto* veg : sAllSphTerVegObjs) {
			const void* ptr = veg->getBiomeVegIns(terrIdx, name);
			if (ptr) {
				ret.insert(ptr);
			}
		}
		return ret;
	}

	const void* BiomeVegetation::getBiomeVegIns(uint32 terrIdx, const std::string& name)const
	{
		auto itor = _dp->mVegLayerInsMap.find(terrIdx);
		if (itor == _dp->mVegLayerInsMap.end()) {
			return nullptr;
		}
		for (const BiomeVegLayerIns& i : itor->second) {
			if (i.getInsData().name == name) {
				return &i;
			}
		}
		return nullptr;
	}

	namespace SphVeg
	{
		inline std::array<Vector2, 4> _FloatRectToPos(const FloatRect& rect)
		{
			return {
				Vector2(rect.left, rect.bottom),
				Vector2(rect.right, rect.bottom),
				Vector2(rect.right, rect.top),
				Vector2(rect.left, rect.top)
			};
		}

		inline Vector2 _GetRectCenter(const FloatRect& rect)
		{
			return{ rect.left + rect.width() / 2.f, rect.top + rect.height() / 2.f };
		}

		static Vector3 RingSpaceToNor(float R, float r, const Vector2& ringSpacePos, float offset = 0.f)
		{
			const float x = (1.f + (r + offset / R) * std::cos(ringSpacePos.y)) * R * std::sin(ringSpacePos.x);
			const float y = (r + offset / R) * std::sin(ringSpacePos.y) * R;
			const float z = (1.f + (r + offset / R) * std::cos(ringSpacePos.y)) * R * std::cos(ringSpacePos.x);

			return { x, y, z };
		}


		inline Vector3 _FacePosToRingNor(uint32 type, SphericalTerrain* sphPtr, const Vector2& pos, float faceSize, float offset)
		{
			// To cur face pos [-r, r] to [0, 2r]
			const Vector2 facePos = pos + Vector2(faceSize / 2.f);
			// To all face pos
			const Vector2 allFacePos = { facePos.x + type * faceSize, facePos.y };
			// To radian pos
			const Vector2 allFaceRadianPos = allFacePos / Vector2(sphPtr->m_radius, sphPtr->m_radius * sphPtr->m_relativeMinorRadius);

			return RingSpaceToNor(sphPtr->m_radius, sphPtr->m_relativeMinorRadius, allFaceRadianPos, offset) * sphPtr->m_rot;
		}

		inline std::array<Vector3, 4> _FacePosToRingNor(uint32 type, SphericalTerrain* sphPtr, const std::array<Vector2, 4>& pos, float faceSize, float offset)
		{
			return {
				_FacePosToRingNor(type, sphPtr, pos[0], faceSize, offset),
				_FacePosToRingNor(type, sphPtr, pos[1], faceSize, offset),
				_FacePosToRingNor(type, sphPtr, pos[2], faceSize, offset),
				_FacePosToRingNor(type, sphPtr, pos[3], faceSize, offset)
			};
		}

		inline Vector3 _FacePosToCube(uint32 type, float radius, const Vector2& pos)
		{
			_CFaceType cType = _CFaceType(type);
			switch (cType)
			{
			case _CFaceType::PositiveX: return { radius, pos.x, pos.y };
			case _CFaceType::NegativeX: return { -radius, pos.x, pos.y };

			case _CFaceType::PositiveY: return { pos.x,  radius, pos.y };
			case _CFaceType::NegativeY: return { pos.x, -radius, pos.y };

			case _CFaceType::PositiveZ: return { pos.x, pos.y,  radius };
			case _CFaceType::NegativeZ: return { pos.x, pos.y, -radius };
			default: break;
			}
			return Vector3::ZERO;
		}

		inline std::array<Vector3, 4> _FacePosToCube(uint32 type, float radius, const std::array<Vector2, 4>& pos)
		{
			return {
				_FacePosToCube(type, radius, pos[0]),
				_FacePosToCube(type, radius, pos[1]),
				_FacePosToCube(type, radius, pos[2]),
				_FacePosToCube(type, radius, pos[3])
			};
		}

		inline static void _GetCubePointsWorldBox(const std::array<Vector3, 4>& cubePos, const Vector3& sphPos, float sphRadius, float sphMaxOff, AxisAlignedBox& retBox)
		{
			retBox.setNull();
			Vector3 midP = Vector3::ZERO;
			for (int i = 0; i < 4; i++)
				midP += cubePos[i].GetNormalized();
			midP /= 4.f;
			float offScale = 1.f / midP.length();
			for (int i = 0; i < 4; i++)
			{
				retBox.merge(sphPos + cubePos[i].GetNormalized() * (sphRadius + sphMaxOff) * offScale);
				retBox.merge(sphPos + cubePos[i].GetNormalized() * (sphRadius - sphMaxOff));
			}
		}

		bool _CheckVisible(const Camera* pCamera, const AxisAlignedBox& worldBox, float showDis)
		{
			const float squaDis = VegOpts::SquareDistPointToAABB(pCamera->getPosition(), worldBox);
			if (squaDis >= showDis * showDis)
				return false;
			if (!pCamera->isVisible(worldBox))
				return false;
			return true;
		}
		//	======================================================================
		void _Private::init()
		{

			mQueryLight.reserve(50);
			mRenderList.reserve(50);

			mRenderCachePool = std::make_unique<_RenderCachePool>(mInfo.infoCacheHoldTime);

			reDevide();

			for (auto&& vegInfoMapIt : mInfo.vegLayerMap)
			{
				const _VegLayerInfoArray& layerArray = vegInfoMapIt.second;
				for (const _VegLayerInfo& layerInfo : layerArray)
				{
					auto result = mGroup->getLayerTempCluster()->createOrRetrieve(Name(BiomeVegLayerTemp::ResToPathName(layerInfo.name)));
					BiomeVegLayerTempPtr vegLayerPtr = result.first;
					mVegLayerInsMap[vegInfoMapIt.first].emplace_back(BiomeVegLayerIns(layerInfo, vegLayerPtr));
				}
			}

			mIsReady = false;
		}

		void _Private::shutdown()
		{
			mVegLayerInsMap.clear();
			mBoxCachePool.reset();
			mRenderCachePool.reset();

			mIsReady = false;
		}

		void _QuadTreeNode::FindVis(const _NodeID& nodeID, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const FloatRect& nodeRange, uint32 level)
		{
			Root* root = Root::instance();
			uint64 startTime = root->getWallTime();
			if (IsLeaf(nodeRange, level))
				VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::Leaf, 1u);
			else
				VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::Node, 1u);

			const _OrthoBox& nodeBox = _dp->mBoxCachePool->get(nodeID, nodeRange, fCurTime, _dp);
			const float camToBoxNearSquareDis = nodeBox.getDisSqrt(_dp->mSphSpaceCamPos, false);

			if (camToBoxNearSquareDis >= _dp->mInfo.boxVisDistance * _dp->mInfo.boxVisDistance)
				return;

			bool checkFlag = true;

			if (checkFlag)
			{
				if (false == nodeBox.isVisible(_dp->mSphSpaceCamPlanes.data(), false))
					return;
			}

			if (IsLeaf(nodeRange, level))
			{
				_dp->mRenderCachePool->findVis(nodeID, nodeRange, pCamera, pQueue, fCurTime, _dp, nodeBox.invBase());
				VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::IntoLeaf, 1u);
			}
			else
			{
				for (uint32 i = 0; i < 4; i++)
					FindVis(GetChildID(nodeID, i), pCamera, pQueue, fCurTime, _dp, _GetChildRect(_NodeType(i), nodeRange), level + 1);
			}
			{
				//ceshi
				auto profileMgr = ProfilerInfoManager::instance();
				auto& frameData = profileMgr->GetCurrentFrameData()->profilerInfo;
				uint64 deltaTime = root->getWallTime() - startTime;
				frameData.biomeVegeFindVis += deltaTime;
			}
		}

		void _QuadTreeNode::FindBox(std::vector<AxisAlignedBox>& boxVec, const _NodeID& nodeID, float fCurTime, _Private* _dp, const FloatRect& nodeRange, uint32 level, uint32 findLevel)
		{
			if (level == findLevel)
			{
				const _OrthoBox& box = _dp->mBoxCachePool->get(nodeID, nodeRange, fCurTime, _dp);
				AxisAlignedBox worldBox = box.getAABB();
				worldBox.vMax += _dp->mInfo.sphPos;
				worldBox.vMin += _dp->mInfo.sphPos;
				boxVec.push_back(worldBox);
			}

			if (IsLeaf(nodeRange, level))
				return;

			if (level < findLevel)
			{
				for (uint32 i = 0; i < 4; i++)
					FindBox(boxVec, GetChildID(nodeID, i), fCurTime, _dp, _GetChildRect(_NodeType(i), nodeRange), level + 1, findLevel);
			}
		}

		void _QuadTreeNode::GetRandomData(_NodeID nodeID, const FloatRect& nodeRange, float matSize, float faceSize, const _CreateInfo& info,
			const _DenseEffectMap& denEffMap, std::array<std::vector<std::vector<std::pair<Vector3, int>>>, 3>& randomDataArray, BiomeVegLayerTempCluster* resManager)
		{
			float geoDenFac = 1.f;
			if (BiomeVegOpt::GetGeoFactorEnabled())
			{
				const Vector2 rectCenterP = _GetRectCenter(nodeRange);
				const std::array<Vector2, 4> rectPos = _FloatRectToPos(nodeRange);

				Vector3 wCenterP = Vector3::ZERO;
				std::array<Vector3, 4>wRectP = { Vector3::ZERO , Vector3::ZERO , Vector3::ZERO , Vector3::ZERO };

				if (info.sphPtr->isSphere())
				{
					wRectP = _FacePosToCube(nodeID.face, info.sphRadius, rectPos);
					for (auto& itP : wRectP)
						itP = info.sphPos + itP.GetNormalized() * info.sphRadius;

					wCenterP = _FacePosToCube(nodeID.face, info.sphRadius, rectCenterP);
					wCenterP = info.sphPos + wCenterP.GetNormalized() * info.sphRadius;
				}
				else
				{
					wRectP = _FacePosToRingNor(nodeID.face, info.sphPtr, rectPos, faceSize, 0.f);
					for (auto& itP : wRectP)
						itP += info.sphPos;

					wCenterP = _FacePosToRingNor(nodeID.face, info.sphPtr, rectCenterP, faceSize, 0.f);
					wCenterP += info.sphPos;
				}

				wCenterP = _DyGetSurfaceWs(info.sphPtr, wCenterP);
				for (auto& itP : wRectP)
					itP = _DyGetSurfaceWs(info.sphPtr, itP);

				float tempS = 0.f;
				for (uint32 i = 0; i < 4; i++)
					tempS += (wRectP[i] - wCenterP).crossProduct(wRectP[(i + 1) % 4] - wCenterP).length() / 2.f;

				geoDenFac = tempS / (nodeRange.width() * nodeRange.height());
				geoDenFac = std::clamp(geoDenFac, 0.f, 1.f);
			}

			for (float b_x = nodeRange.left; b_x < nodeRange.right; b_x += matSize)
			{
				for (float b_y = nodeRange.top; b_y < nodeRange.bottom; b_y += matSize)
				{
					const float b_x_end = std::min(b_x + matSize, nodeRange.right);
					const float b_y_end = std::min(b_y + matSize, nodeRange.bottom);

					const std::array<Vector2, 5> sampArr = {
						Vector2(b_x, b_y),
						Vector2(b_x_end, b_y),
						Vector2(b_x_end, b_y_end),
						Vector2(b_x, b_y_end),
						Vector2((b_x + b_x_end) / 2.f, (b_y + b_y_end) / 2.f),
					};
					std::array<int, 5> terIndexArr = { 0,0,0,0,0 };
					std::array<Vector3, 5> worldPosArr = {};
					for (uint32 i = 0; i < 5; i++)
					{
						Vector3 worldSpacePos = Vector3::ZERO;
						if (info.sphPtr->isSphere())
						{
							const Vector3 cubeSpacePos = _FacePosToCube(nodeID.face, info.sphRadius, sampArr[i]);
							const Vector3 pDir = cubeSpacePos.GetNormalized();
							worldSpacePos = info.sphPos + pDir * info.sphRadius;
						}
						else
						{
							const Vector3 ringNorPos = _FacePosToRingNor(nodeID.face, info.sphPtr, sampArr[i], faceSize, 0.f);
							worldSpacePos = info.sphPos + ringNorPos;
						}

						const Vector3 hitPos = _DyGetSurfaceWs(info.sphPtr, worldSpacePos);
						worldPosArr[i] = hitPos;
						int terIndex = info.sphPtr->getSurfaceTypeWs(hitPos);
						terIndexArr[i] = terIndex;
					}

					struct _TempDen
					{
						float expDen = 0.f;
						float effDen = 0.f;
					};
					std::array<_TempDen, 3> tempDenArr;

					for (uint32 sam_i = 0; sam_i < 5; sam_i++)
					{
						const int cur_ter_index = terIndexArr[sam_i];
						auto findTerVegIt = info.vegLayerMap.find(cur_ter_index);
						if (findTerVegIt == info.vegLayerMap.end())
							continue;

						auto denEffIt = denEffMap.find(cur_ter_index);
						for (uint32 layer_i = 0; layer_i < 3; layer_i++)
						{
							if (layer_i >= findTerVegIt->second.size())
								continue;
							const BiomeVegInsData& insData = findTerVegIt->second.at(layer_i);
							float expDen = insData.density;
							if (!insData.customParam && resManager)
							{
								BiomeVegLayerTempPtr res = resManager->getByName(Name(BiomeVegLayerTemp::ResToPathName(insData.name)));
								if (!res.isNull() && res->isLoaded())
								{
									expDen = res->getDensity();
								}
							}
							float effDen = 1.f;

							if (denEffIt != denEffMap.end() && layer_i < denEffIt->second.size())
							{
								if (denEffIt->second.at(layer_i).isEnable)
									effDen = denEffIt->second.at(layer_i).denFactor;
							}

							tempDenArr[layer_i].expDen += expDen;
							tempDenArr[layer_i].effDen += effDen;
						}
					}

					for (auto&& tempDen : tempDenArr)
					{
						tempDen.expDen /= 5.f;
						tempDen.effDen /= 5.f;
					}

					const float matBlockWidth = std::min(nodeRange.right - b_x, matSize);
					const float matBlockHight = std::min(nodeRange.bottom - b_y, matSize);

					for (uint32 layerIndex = 0u; layerIndex < 3u; layerIndex++)
					{
						const float expDen = tempDenArr[layerIndex].expDen;
						const float effDen = tempDenArr[layerIndex].effDen;

						if (expDen <= 0.000001f) continue;
						float realDenLimit = BiomeVegOpt::GetDenLimit();
						float vegInsCountf = realDenLimit * expDen * geoDenFac * matBlockWidth * matBlockHight / 16.f * effDen;
						uint32 vegInsCount = 0;
						float fract = vegInsCountf - std::floor(vegInsCountf);
						Vector2 randSeed = Vector2(worldPosArr[4].x, worldPosArr[4].z);
						float rand = BiomeVegRandom::GetRandom(randSeed);
						
						if (rand < fract) {
							vegInsCount = 1;
						}
						vegInsCount += (uint32_t)std::floor(vegInsCountf);
						randomDataArray.at(layerIndex).resize(randomDataArray.at(layerIndex).size() + 1);
						std::vector<std::pair<Vector3, int>>& randomArray = randomDataArray.at(layerIndex).back();

						std::vector<Vector3> tempRandom;
						BiomeVegRandom::GetRandom(vegInsCount, tempRandom, Vector2((b_x + 0.5f) * (layerIndex + 1) * 11.f, (b_y + 0.5f)* (layerIndex + 1) * 17.f));
						for (auto&& random_p : tempRandom)
						{
							//	Pos from [0, 1] to [LeftTop, RightBottom]
							random_p.x = b_x + matBlockWidth * random_p.x;
							random_p.y = b_y + matBlockHight * random_p.y;
							//	Scale from [0, 1] to [-1, 1]
							random_p.z = random_p.z * 2.f - 1.f;

							const Vector2 nSamP = Vector2(random_p.x, random_p.y)
								+ (BiomeVegOpt::GetRandomOff() * Vector2(std::cos(random_p.z * Math::PI), std::sin(random_p.z * Math::PI)));

							Vector3 worldSpacePos = Vector3::ZERO;
							if (info.sphPtr->isSphere())
							{
								const Vector3 cubeSpacePos = _FacePosToCube(nodeID.face, info.sphRadius, nSamP);
								const Vector3 pDir = cubeSpacePos.GetNormalized();
								worldSpacePos = info.sphPos + pDir * info.sphRadius;
							}
							else
							{
								const Vector3 ringNorPos = _FacePosToRingNor(nodeID.face, info.sphPtr, nSamP, faceSize, 0.f);
								worldSpacePos = info.sphPos + ringNorPos;
							}

							const Vector3 hitPos = _DyGetSurfaceWs(info.sphPtr, worldSpacePos);
							int terIndex = info.sphPtr->getSurfaceTypeWs(hitPos);

							if (terIndex == -1)
								continue;

							auto randomFindIt = info.vegLayerMap.find(terIndex);
							if (randomFindIt == info.vegLayerMap.end())
								continue;

							if (layerIndex >= randomFindIt->second.size())
								continue;

							float layer_den = randomFindIt->second.at(layerIndex).density;
							if (!randomFindIt->second.at(layerIndex).customParam && resManager)
							{
								BiomeVegLayerTempPtr res = resManager->getByName(Name(BiomeVegLayerTemp::ResToPathName(randomFindIt->second.at(layerIndex).name)));
								if (!res.isNull() && res->isLoaded())
								{
									layer_den = res->getDensity();
								}
							}
							auto denEffIt = denEffMap.find(terIndex);
							if (denEffIt != denEffMap.end() && layerIndex < denEffIt->second.size())
							{
								if (denEffIt->second.at(layerIndex).isEnable)
									layer_den = layer_den * denEffIt->second.at(layerIndex).denFactor;

							}

							if (layer_den < (expDen * effDen))
							{
								float ratio = layer_den / (expDen * effDen);
								if ((random_p.z + 1.f) / 2.f > ratio)
									continue;
							}

							randomArray.emplace_back(random_p, terIndex);
						}
					}
				}
			}
		}

		void _QuadTree::FindVis(uint32 faceIndex, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp)
		{
			const float halfSize = _dp->mFaceSize / 2.f;
			const FloatRect rectRange(-halfSize, -halfSize, halfSize, halfSize);

			for (uint32 i = 0; i < 4; i++)
			{
				static _NodeID curChildID = {};
				curChildID.face = faceIndex;
				curChildID.index = i;

				static FloatRect curRect(0.f, 0.f, 0.f, 0.f);
				_GetChildRect(_NodeType(i), rectRange, curRect);
				_QuadTreeNode::FindVis(curChildID, pCamera, pQueue, fCurTime, _dp, curRect, 1);
			}
		}

		void _QuadTree::FindBox(std::vector<AxisAlignedBox>& boxVec, uint32 faceIndex, float fCurTime, _Private* _dp, uint32 findLevel)
		{
			const float halfSize = _dp->mFaceSize / 2.f;
			const FloatRect rectRange(-halfSize, -halfSize, halfSize, halfSize);

			for (uint32 i = 0; i < 4; i++)
			{
				static _NodeID curChildID = {};
				curChildID.face = faceIndex;
				curChildID.index = i;

				static FloatRect curRect(0.f, 0.f, 0.f, 0.f);
				_GetChildRect(_NodeType(i), rectRange, curRect);
				_QuadTreeNode::FindBox(boxVec, curChildID, fCurTime, _dp, curRect, 1, findLevel);
			}
		}

		//	======================================================================
		const _OrthoBox& _BoxPool::get(const _NodeID& id, const FloatRect& range, float fCurTime, _Private* _dp)
		{
			_BoxCache& curCache = mPool[id.face][id.index];
			curCache.lastTime = fCurTime;
			_OrthoBox& box = curCache.box;

			if (box.nullopt())
			{
				const float sphRadius = _dp->getRadius();
				const float sphMaxOff = _dp->mInfo.sphTerMaxOff;
				const std::array<Vector2, 4> rectPos = _FloatRectToPos(range);

				if (_dp->mInfo.sphPtr->isSphere())
				{
					const std::array<Vector3, 4> cubePos = _FacePosToCube(id.face, _dp->getRadius(), rectPos);

					const std::array<Vector3, 4> cubeNorPos = {
						cubePos[0].GetNormalized(),
						cubePos[1].GetNormalized(),
						cubePos[2].GetNormalized(),
						cubePos[3].GetNormalized(),
					};
					const Vector3 midNorP = (cubeNorPos[0] + cubeNorPos[1] + cubeNorPos[2] + cubeNorPos[3]).GetNormalized();

					if (BiomeVegOpt::GetOrthoBoxEnabled())
					{
						Vector3	base_y = midNorP;
						Vector3	base_x = (cubeNorPos[1] - cubeNorPos[0]).GetNormalized();
						Vector3	base_z = base_x.crossProduct(base_y);
						base_x = base_y.crossProduct(base_z);

						const Matrix3 invBase = {
							base_x.x, base_x.y, base_x.z,
							base_y.x, base_y.y, base_y.z,
							base_z.x, base_z.y, base_z.z,
						};
						box.setInvBase(invBase);
					}

					for (auto&& nroPos : cubeNorPos) {
						box.merge(nroPos * (sphRadius + sphMaxOff), false);
						box.merge(nroPos * (sphRadius - sphMaxOff), false);
					}

					box.merge(midNorP * (sphRadius + sphMaxOff), false);
					box.merge(midNorP * (sphRadius - sphMaxOff), false);
				}
				else
				{
					if (BiomeVegOpt::GetOrthoBoxEnabled())
					{
						const Vector2 rectCenterP = _GetRectCenter(range);
						const Vector3 OrchoRoot = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectCenterP,
							_dp->mFaceSize, -_dp->mInfo.sphPtr->m_relativeMinorRadius);

						const std::array<Vector3, 4> ringNorPos = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectPos, _dp->mFaceSize, 0.f);
						const Vector3 ringNorCP = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectCenterP, _dp->mFaceSize, 0.f);

						Vector3	base_y = (ringNorCP - OrchoRoot).GetNormalized();
						Vector3	base_x = (ringNorPos[1] - ringNorPos[0]).GetNormalized();
						Vector3	base_z = base_x.crossProduct(base_y);
						base_x = base_y.crossProduct(base_z);

						const Matrix3 invBase = {
							base_x.x, base_x.y, base_x.z,
							base_y.x, base_y.y, base_y.z,
							base_z.x, base_z.y, base_z.z,
						};
						box.setInvBase(invBase);
					}

					//	out points
					{
						const std::array<Vector3, 4> ringNorPos = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectPos, _dp->mFaceSize, sphMaxOff);
						for (auto&& p : ringNorPos)
							box.merge(p, false);

						const Vector2 rectCenterP = _GetRectCenter(range);
						const Vector3 ringNorCP = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectCenterP, _dp->mFaceSize, sphMaxOff);
						box.merge(ringNorCP, false);
					}

					//	in points
					{
						const std::array<Vector3, 4> ringNorPos = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectPos, _dp->mFaceSize, -sphMaxOff);
						for (auto&& p : ringNorPos)
							box.merge(p, false);

						const Vector2 rectCenterP = _GetRectCenter(range);
						const Vector3 ringNorCP = _FacePosToRingNor(id.face, _dp->mInfo.sphPtr, rectCenterP, _dp->mFaceSize, -sphMaxOff);
						box.merge(ringNorCP, false);
					}
				}
			}

			return box;
		}
		//	======================================================================
		_Render::_Render(const _InsDataArray& insDataArray, const BiomeVegLayerIns& vegRes, uint32 lod, const Matrix3& invBase, const _Private* _dp)
			: mLod(lod), mRes(vegRes), mBox(invBase)
		{
			if (mInstanceCount >= 1) {
				mIdenticalRandSeed = Vector2(insDataArray[0].pos.x, insDataArray[0].pos.y);
			}

			mInstanceCount = (uint32)insDataArray.size();
			if (1u == mLod && vegRes.isLoaded() && vegRes.getVegTemp()->getDenseEffect())
			{
				mInstanceCount = (uint32)std::ceil(mInstanceCount / vegRes.getVegTemp()->getDenseFactor());
				mInstanceCount = std::min(mInstanceCount, (uint32)insDataArray.size());
			}
			mInsDataArray = std::vector(insDataArray.begin(), insDataArray.begin() + mInstanceCount);

			RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
			meshData.m_vertexBuffer.resize(2);

			mSphPos = _dp->mInfo.sphPos;
			mSphPtr = _dp->mInfo.sphPtr;
			_Load();
		}

		void _Render::setCustomParameters(const Pass* pPass)
		{
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

			pRenderSystem->setUniformValue(this, pPass, U_VSCustom0, Vector4(
				mSphPos.x, mSphPos.y, mSphPos.z, 1.f));

			if (mLod == 0)
			{
				const DVector3 roleCamSpa = VegOpts::GetRolePushPos() - mCamPos;
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom1, Vector4(
					(float)roleCamSpa.x, (float)roleCamSpa.y, (float)roleCamSpa.z, 1.f));
			}

			const _VegLayerInfo& vegLayerInfo = mRes.getInsData();
			const float scaleBase = vegLayerInfo.customParam ? vegLayerInfo.scaleBase : mRes.getVegTemp()->getScaleBase();
			const float scaleRange = vegLayerInfo.customParam ? vegLayerInfo.scaleRange : mRes.getVegTemp()->getScaleRange();

			const VegetationMeshTemplate* meshTemplate = &mRes.getVegTemp()->getMesh(mLod);
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom2, Vector4(
				meshTemplate->bounds.getMinimum().y,
				meshTemplate->bounds.getMaximum().y,
				scaleBase,
				scaleRange));

			const DVector3 toCamSpaPos = mSphPos - mCamPos;
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom3, Vector4((float)toCamSpaPos.x, (float)toCamSpaPos.y, (float)toCamSpaPos.z, 0.f));

			Vector4 uvPersent(1.f, 0.f, 0.f, 0.f);
			const BiomeVegLayerTempPtr& vegLayerTemp = mRes.getVegTemp();
			if (vegLayerTemp->getMultiUV() == 1)
				uvPersent = vegLayerTemp->getUVPercent();
			pRenderSystem->setUniformValue(this, pPass, U_VSCustom5, uvPersent);

			ColorValue clrValue[2];
			float fLerpFactor = Math::Clamp(vegLayerInfo.lerpFactor, -0.9999f, 0.9999f);
			float fSlope = 1.f, fIntercept = 0.f;
			if (fLerpFactor >= 0)
			{
				fSlope = 1.f / (1.f - fLerpFactor);
				fIntercept = 0.f;
			}
			else
			{
				fSlope = 1.f / (1.f + fLerpFactor);
				fIntercept = fLerpFactor / (1.f + fLerpFactor);
			}

			clrValue[0] = ColorValue::gammaToLinear(vegLayerInfo.topDiffuse) * vegLayerInfo.topDiffuseRatio;
			clrValue[0].a = fSlope;
			clrValue[1] = ColorValue::gammaToLinear(vegLayerInfo.bottomDiffuse) * vegLayerInfo.bottomDiffuseRatio;
			clrValue[1].a = fIntercept;
#ifdef ECHO_EDITOR
			if (BiomeVegetationSparkler::ShouldRender(&(this->getRes())))
			{
				clrValue[0] = ColorValue::White;
				clrValue[1] = ColorValue::Green;
			}
#endif // ECHO_EDITOR
			pRenderSystem->setUniformValue(this, pPass, U_PSCustom0, clrValue, sizeof(clrValue));

			if (mLod == 0)
			{
				Vector4 g4 = Vector4::ZERO;
				mRes.getVegTemp()->getFadeFactorLod0(g4);
				pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister4, g4);
			}
			else
			{
				Vector4 g4 = Vector4::ZERO, g5 = Vector4::ZERO;
				mRes.getVegTemp()->getFadeFactorLod1(g4, g5);
				pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister4, g4);
				pRenderSystem->setUniformValue(this, pPass, U_VSGeneralRegister5, g5);
			}
		}

		void _Render::_Load()
		{
			if (mCanDraw) return;

			if (mInsDataArray.empty() || mRes.isCanDraw(mLod) == false)
				return;

			const BiomeVegLayerTempPtr& vegLayerTemp = mRes.getVegTemp();
			const AxisAlignedBox& meshTempBox = vegLayerTemp->getMeshTempBox(mLod);
			if (meshTempBox.isNull()) return;


			Vector3 furthest_corner = meshTempBox.getCorner(AxisAlignedBox::CornerEnum(0));
			for (int i = 0; i < 8; ++i)
			{
				Vector3 corner = meshTempBox.getCorner(AxisAlignedBox::CornerEnum(i));
				if (corner.squaredLength() > furthest_corner.squaredLength())
					furthest_corner = corner;
			}
			const float scaleBase = mRes.getInsData().customParam ? mRes.getInsData().scaleBase : mRes.getVegTemp()->getScaleBase();
			const float scaleRange = mRes.getInsData().customParam ? mRes.getInsData().scaleRange : mRes.getVegTemp()->getScaleRange();
			float mBoundPadding = furthest_corner.length() * (scaleBase + scaleRange);
			for (const auto& ins : mInsDataArray)
			{
				mBox.merge(Vector3(ins.pos.x - mBoundPadding, ins.pos.y - mBoundPadding, ins.pos.z - mBoundPadding), false);
				mBox.merge(Vector3(ins.pos.x + mBoundPadding, ins.pos.y + mBoundPadding, ins.pos.z + mBoundPadding), false);
			}
			enableCustomParameters(true);
			enableInstanceRender(true);

			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();

			meshData.m_vertexBuffer[0] = vegLayerTemp->getVertexBuffer(mLod);
			meshData.m_vertexCount = vegLayerTemp->getVertexCount(mLod);
			meshData.m_vertexBuffSize = vegLayerTemp->getVertexBuffSize(mLod);

			meshData.m_IndexBufferData.m_indexBuffer = vegLayerTemp->getIndexBuffer(mLod);
			meshData.m_IndexBufferData.m_indexCount = vegLayerTemp->getIndexCount(mLod);;
			meshData.m_indexBufferSize = vegLayerTemp->getIndexBuffSize(mLod);

			meshData.m_vertexBuffer[1] = pRenderSystem->createStaticVB(mInsDataArray.data(), (uint32)mInsDataArray.size() * sizeof(_InsData));
			mRenderOperation.mPrimitiveType = ECHO_TRIANGLE_LIST;

			mCanDraw = true;
		}

		inline uint32 _Render::getInsNum() const
		{
			float insNumf = getInsNumf();

			const Vector2& randSeed = mIdenticalRandSeed;
			auto rand = BiomeVegRandom::GetRandom(randSeed);
			uint32 insNum = uint32(std::floor(insNumf));
			float randBar = insNumf - insNum;
			if (randBar > 0.001f && rand < randBar) {
				insNum++;
			}
			return insNum;
		}

		PassType _Render::calPassType(const Camera* pCamera, const _Private* _dp)
		{
			if (mLod == 0 && _dp->mUsePointLight)
			{
				mLightList.clear();
				if (!_dp->mQueryLight.empty())
				{
					const SceneManager* sceneMgr = pCamera->getSceneManager();
					if (sceneMgr)
					{
						AxisAlignedBox aabb = mBox.getAABB();
						aabb.vMax += _dp->mInfo.sphPos;
						aabb.vMin += _dp->mInfo.sphPos;
						sceneMgr->populateLightList(aabb, mLightList);
					}
				}
			}

			bool usePointLightFlag = (!mLightList.empty());
			bool useDirShadowFlag = _dp->mReceiveShadow;

			if (usePointLightFlag)
			{
				if(useDirShadowFlag)
					return ShadowResivePointLightPass;//接受主光阴影，接受点光源光照
				else
					return PointLightPass;//接受点光源光照
			}
			else
			{
				if (useDirShadowFlag)
					return ShadowResiveInstancePass;//接受主光阴影
				else
					return InstancePass;
			}
		}

		void _Render::collectRuntimeData()const
		{
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::RENDER_CACHE_CPU_MEM, sizeof(*this) + mInsDataArray.capacity() * sizeof(_InsData));
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::GPU_MEM | VegRuntimeType::RENDER_CACHE_GPU_MEM, mInsDataArray.capacity() * sizeof(_InsData));
		}

		_RenderCache::_RenderCache(_NodeID nodeID, const FloatRect& nodeRange, const _Private* _dp, const Matrix3& invBase)
			:mID(nodeID), mRange(nodeRange)
		{
			mLodBox[0] = _OrthoBox(invBase);
			mLodBox[1] = mLodBox[0];

			std::shared_ptr<_AsyncTask> task = std::make_shared<_AsyncTask>(this);
			task->id = mID;
			task->range = mRange;
			task->_dp = _dp;
			task->faceSize = _dp->mFaceSize;

			task->taskInfo = _dp->mInfo;
			task->taskDenseEffMap = _dp->getDenseEffectMap();

			mState = _AsyncState::Loading;

			submit(task);
		}

		void _RenderCache::recalculate(const AxisAlignedBox& box, _Private* _dp)
		{
			if (mState == _AsyncState::UnLoad) return;

			bool needRecal = false;
			//if (mState == _AsyncState::Loading)
			//	needRecal = true;

			if (mState == _AsyncState::Loaded)
			{
				for (auto&& itBox : mLodBox)
				{
					if(itBox.nullopt())
						continue;

					AxisAlignedBox aabb = itBox.getAABB();
					aabb.vMax += _dp->mInfo.sphPos;
					aabb.vMin += _dp->mInfo.sphPos;

					if (aabb.intersects(box))
						needRecal = true;
				}
			}

			if (needRecal)
			{
				std::shared_ptr<_AsyncTask> task = std::make_shared<_AsyncTask>(this);
				task->id = mID;
				task->range = mRange;
				task->taskInfo = _dp->mInfo;
				task->_dp = _dp;
				task->faceSize = _dp->mFaceSize;

				submit(task);
			}
		}
		
		void _RenderCache::collectRuntimeData()const
		{
			size_t cpuMemUsed = sizeof(*this);
			for (auto&& handle : mLodHandle)
			{
				cpuMemUsed += handle.capacity() * sizeof(_RenderPtr);
				for (auto&& it : handle)
				{
					if (it)
					{
						it->collectRuntimeData();
					}
				}
			}
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::RENDER_CACHE_CPU_MEM, cpuMemUsed);
		}

		void _RenderCache::_Recalculate(const _Private* _dp, std::map<int, std::array<std::vector<_InsData>, 3>>& hitInsDataMap)
		{
			//	clear dirty data
			for (uint32 lod = 0u; lod < 2u; lod++)
			{
				mLodHandle	[lod].clear();
				mLodBox		[lod].setNullopt();
				mLodDis		[lod] = 0.f;
			}

			for (auto&& mapIt : hitInsDataMap)
			{
				const int terIndex = mapIt.first;
				auto findVegResIt = _dp->mVegLayerInsMap.find(terIndex);
				if (findVegResIt == _dp->mVegLayerInsMap.end() || findVegResIt->second.empty())
					continue;

				const std::vector<BiomeVegLayerIns>& vegInsKinds = findVegResIt->second;
				for (int i = 0; i < 3; i++)
				{
					if(i >= vegInsKinds.size())
						continue;

					const BiomeVegLayerIns& vegIns = vegInsKinds.at(i);
					if (false == vegIns.isLoaded())
						continue;

					const std::vector<_InsData>& insDataArray = mapIt.second.at(i);
					if (insDataArray.empty())
						continue;

					for (uint32 lod = 0u; lod < 2u; lod++)
					{
						std::shared_ptr<_Render> newRender = std::make_shared<_Render>(insDataArray, vegIns, lod, mLodBox[lod].invBase(), _dp);
						if (newRender->isCanDraw())
						{
							mLodHandle	[lod].push_back(newRender);
							mLodBox		[lod].merge(newRender->getBox());
							mLodDis		[lod] = std::max(mLodDis[lod], newRender->getShowDis());
						}
					}
				}
			}

			for (uint32 lod = 0u; lod < 2u; lod++)
			{
				if (mLodBox[lod].nullopt() == false)
				{
					_dp->mBoxCachePool->update(mID, mLodBox[lod], Root::instance()->getTimeElapsed());
					break;
				}
			}
		}

		bool _AsyncTask::task_executionImpl()
		{
			const _CreateInfo* dyInfoPtr = (&taskInfo);
			const _DenseEffectMap& denseEffMap = taskDenseEffMap;

			const float sphMatSize = BiomeVegOpt::GetMatIndexBlockSize(dyInfoPtr->sphRadius, dyInfoPtr->sphPtr->isTorus());
			std::array<std::vector<std::vector<std::pair<Vector3, int>>>, 3> tempRandom;
			BiomeVegLayerTempCluster* resManager = nullptr;
			if (_dp->mGroup)
				resManager = _dp->mGroup->getLayerTempCluster();
			_QuadTreeNode::GetRandomData(id, range, sphMatSize, faceSize, *dyInfoPtr, denseEffMap, tempRandom, resManager);

			//	Merge
			std::array<std::vector<std::pair<Vector3, int>>, 3> randomDataArray;
			for (uint32 i = 0; i < 3; i++)
			{
				auto& rArray = tempRandom[i];
				if (rArray.empty()) continue;
				Vector2 rdFactor(range.bottom, range.right);
				BiomeVegRandom::Shuffle(rArray, rdFactor);
				size_t maxSize = 0;
				for (auto&& it : rArray)
					maxSize = std::max(it.size(), maxSize);

				for (size_t j = 0; j < maxSize; j++)
				{
					for (const auto& it : rArray)
					{
						if (j >= it.size()) continue;
						randomDataArray[i].push_back(it[j]);
					}
				}
			}

			VegRoadQueryPackage roadPackage = {
				.sphPtr = dyInfoPtr->sphPtr
			};

			for (int aIndex = 0; aIndex < 3; aIndex++)
			{
				std::vector<std::pair<Vector3, int>>& randomData = randomDataArray.at(aIndex);
				roadPackage.elements.resize(1);

				for (const auto& [p, terrainIndex] : randomData)
				{
					/*const Vector2 nSamP = Vector2(p.x, p.y)
						+ (BiomeVegOpt::GetRandomOff() * Vector2(std::cos(p.z * Math::PI), std::sin(p.z * Math::PI)));*/

					const float scaleValue = p.z;
					Vector3 worldSpacePos = Vector3::ZERO;

					if (dyInfoPtr->sphPtr->isSphere())
					{
						const Vector3 cubeSpacePos = _FacePosToCube(id.face, dyInfoPtr->sphRadius, Vector2(p.x, p.y));
						const Vector3 pDir = cubeSpacePos.GetNormalized();
						worldSpacePos = dyInfoPtr->sphPos + pDir * dyInfoPtr->sphRadius;
					}
					else
					{
						const Vector3 ringNorPos = _FacePosToRingNor(id.face, dyInfoPtr->sphPtr, Vector2(p.x, p.y), faceSize, 0.f);
						worldSpacePos = dyInfoPtr->sphPos + ringNorPos;
					}

					Vector3 hitNormal = Vector3::UNIT_Y;
					const Vector3 hitPos = _DyGetSurfaceWs(dyInfoPtr->sphPtr, worldSpacePos, &hitNormal);

					{
						roadPackage.elements.front() = { hitPos, true };
						auto callRet = ProtocolComponentSystem::instance()->callProtocolFunc(Name("CheckPlanetRoadMask"), (void*)(&roadPackage), 0u);

						if (PR_OK == callRet && false == roadPackage.elements.front().second)
						{
							if (sRecordFlag)
								sRecordRoad.push_back(hitPos);
							continue;
						}
					}

					const Vector3 sphSpaceHitPos = hitPos - dyInfoPtr->sphPos;

					_InsData insData = { .pos = Vector4(sphSpaceHitPos.x, sphSpaceHitPos.y, sphSpaceHitPos.z, scaleValue) };
					VegOpts::Quaternion_FromAxesToAxes(Vector3::UNIT_Y, hitNormal.GetNormalized(), insData.normal_qua);

					/*Vector3 tempWorPos = Vector3::ZERO;
					if (dyInfoPtr->sphPtr->isSphere())
					{
						const Vector3 cubeSpacePos = _FacePosToCube(id.face, dyInfoPtr->sphRadius, nSamP);
						const Vector3 pDir = cubeSpacePos.GetNormalized();
						tempWorPos = dyInfoPtr->sphPos + pDir * dyInfoPtr->sphRadius;
					}
					else
					{
						const Vector3 ringNorPos = _FacePosToRingNor(id.face, dyInfoPtr->sphPtr, nSamP, faceSize, 0.f);
						tempWorPos = dyInfoPtr->sphPos + ringNorPos;
					}

					const Vector3 tempHitP = _DyGetSurfaceWs(mExportPhys, dyInfoPtr->sphPtr, tempWorPos);
					int terIndex = dyInfoPtr->sphPtr->getSurfaceTypeWs(tempHitP);*/

					hitInsDataMap[terrainIndex].at(aIndex).emplace_back(insData);
				}
			}

			if (dyInfoPtr->sphPtr->isVegetationCheckValid())
			{
				std::vector<Vector3> allPositions;
				std::vector<std::pair<std::vector<_InsData>*, size_t>> positionToArrayMap;

				for (auto&& mapIt : hitInsDataMap)
				{
					const int terIndex = mapIt.first;
					auto findVegResIt = _dp->mVegLayerInsMap.find(terIndex);
					if (findVegResIt == _dp->mVegLayerInsMap.end() || findVegResIt->second.empty())
						continue;

					const std::vector<BiomeVegLayerIns>& vegInsKinds = findVegResIt->second;
					for (int i = 0; i < 3; i++)
					{
						if (i >= vegInsKinds.size())
							continue;

						const BiomeVegLayerIns& vegIns = vegInsKinds.at(i);
						if (false == vegIns.isLoaded())
							continue;

						std::vector<_InsData>& insDataArray = mapIt.second.at(i);
						if (insDataArray.empty())
							continue;

						for (size_t j = 0; j < insDataArray.size(); j++)
						{
							const auto& insData = insDataArray[j];
							allPositions.emplace_back(insData.pos.x, insData.pos.y, insData.pos.z);
							positionToArrayMap.emplace_back(&insDataArray, j);
						}
					}
				}

				if (!allPositions.empty())
				{
					std::vector<bool> allOnSurface = dyInfoPtr->sphPtr->getOnSurface(allPositions);

					if (allOnSurface.size() == allPositions.size())
					{
						std::unordered_map<std::vector<_InsData>*, std::vector<bool>> retainFlags;

						for (auto&& mapIt : hitInsDataMap)
						{
							for (int i = 0; i < 3; i++)
							{
								std::vector<_InsData>& insDataArray = mapIt.second.at(i);
								if (!insDataArray.empty())
								{
									retainFlags[&insDataArray] = std::vector<bool>(insDataArray.size(), false);
								}
							}
						}

						for (size_t k = 0; k < allOnSurface.size(); k++)
						{
							if (allOnSurface[k])
							{
								auto& pair = positionToArrayMap[k];
								std::vector<_InsData>* arrayPtr = pair.first;
								size_t index = pair.second;

								if (index < retainFlags[arrayPtr].size())
								{
									retainFlags[arrayPtr][index] = true;
								}
							}
						}

						for (auto&& [arrayPtr, flags] : retainFlags)
						{
							std::vector<_InsData> newArray;
							newArray.reserve(flags.size());

							for (size_t i = 0; i < flags.size(); i++)
							{
								if (flags[i])
								{
									newArray.push_back(std::move((*arrayPtr)[i]));
								}
							}

							*arrayPtr = std::move(newArray);
						}
					}
				}
			}
			
			return true;
		}
		void _AsyncTask::task_finishedImpl()
		{
			_RenderCache* renderCache = get();
			if (renderCache)
			{
				renderCache->_Recalculate(_dp, hitInsDataMap);
				renderCache->mState = _AsyncState::Loaded;
			}
		}


		void _RenderCachePool::checkOverdue(float fCurTime)
		{
			for (auto it = mPool.begin(); it != mPool.end(); )
			{
				if (fCurTime - it->second->getLastTime() > mHoldTime)
					it = mPool.erase(it);
				else
					it++;
			}
		}

		void _RenderCachePool::recalculate(const AxisAlignedBox& box, _Private* _dp)
		{
			for (auto&& mapIt : mPool)
				mapIt.second->recalculate(box, _dp);
		}

		void  _RenderCachePool::findVis(_NodeID id, const FloatRect& nodeRange, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const Matrix3& invBase)
		{
			auto findIt = mPool.find(id._id);
			if (mPool.end() == findIt)
				findIt = mPool.emplace(id._id, std::make_unique<_RenderCache>(id, nodeRange, _dp, invBase)).first;

			findIt->second->update(fCurTime);

			for (uint32 lod = 0u; lod < 2u; lod++)
			{
				if (false == BiomeVegOpt::GetVisLod(lod))
					continue;

				VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::LodBlock, 1u);

				const _OrthoBox& lodBox = findIt->second->getBox(lod);
				if(lodBox.nullopt())
					continue;

				const float lodShowDis = BiomeVegOpt::GetRealDis(lod, findIt->second->getDis(lod));
				if (lodBox.getDisSqrt(_dp->mSphSpaceCamPos, false) >= lodShowDis * lodShowDis)
					continue;
				if (false == lodBox.isVisible(_dp->mSphSpaceCamPlanes.data(), false))
					continue;

				const auto* sceneMgr = pCamera->getSceneManager();
				if (sceneMgr->m_PortalCullFunction && BiomeVegOpt::GetRoomCullEnabled())
				{
					AxisAlignedBox worldBox = lodBox.getAABB();
					worldBox.vMin += _dp->mInfo.sphPos;
					worldBox.vMax += _dp->mInfo.sphPos;

					bool portalCull = sceneMgr->m_PortalCullFunction(worldBox);
					if (portalCull)
						continue;
				}
				VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::IntoLodBlock, 1u);

				for (auto&& render : findIt->second->getHandle(lod))
				{
					VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::Render, 1u);
					_Render* renderPtr = render.get();

					const _OrthoBox& renderBox = renderPtr->getBox();
					const float renderShowDis = BiomeVegOpt::GetRealDis(lod, renderPtr->getShowDis());
					if (renderBox.getDisSqrt(_dp->mSphSpaceCamPos, false) >= renderShowDis * renderShowDis)
						continue;
					if (false == renderBox.isVisible(_dp->mSphSpaceCamPlanes.data(), false))
						continue;

					float squaDis = (float)(renderBox.toObSpace(_dp->mSphSpaceCamPos) - (renderBox.min() + renderBox.max()) / 2.f).length();
					float insDensity = squaDis / nodeRange.width();
					insDensity = std::max(insDensity, 1.f);
					insDensity = 1.f / insDensity;
					insDensity = std::pow(insDensity, renderPtr->getRes().getVegTemp()->getDenLessFactor() * BiomeVegOpt::GetDenLessFactor());

					renderPtr->setInsDensity(insDensity);
					if (renderPtr->getInsNum() == 0u)
						continue;

					renderPtr->setRenderData(_dp->mInfo.sphPos, pCamera->getPosition());
					_dp->mRenderList.push_back({ squaDis , renderPtr });

					VegRuntimeMgr::CollectCheckBoxCount(VegRuntimeMgr::BoxType::IntoRender, 1u);
				}
			}
		}

		void _RenderCachePool::collectRuntimeData() const
		{
			size_t cpuMemUsed = sizeof(*this);
			for (auto&& it : mPool)
			{
				cpuMemUsed += sizeof(it);
				if (it.second)
					it.second->collectRuntimeData();
			}

			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::RENDER_CACHE_CPU_MEM, cpuMemUsed);
		}

		//	======================================================================
	}

#ifdef ECHO_EDITOR
	bool BiomeVegetationSparkler::ShouldRender(const void* ptr)
	{
		auto itor = s_sparkleMap.find(ptr);
		if (itor == s_sparkleMap.end())return false;
		bool ret = true;
		if (Echo::Root::instance()->getNextFrameNumber() % 5) {

			ret =  false;
		}

		if (itor->second < Echo::Root::instance()->getNextFrameNumber()) {
			s_sparkleMap.erase(itor);
		}

		return ret;
	}

	void BiomeVegetationSparkler::Mark(const void* ptr)
	{
		s_sparkleMap[ptr] = Echo::Root::instance()->getNextFrameNumber() + 60;
	}

#endif
}
