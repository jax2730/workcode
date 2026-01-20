#include "EchoStableHeaders.h"
#include "EchoPlaneVegetation.h"

#include "EchoBiomeVegGroup.h"
#include "EchoVegetationOptions.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoEasyAsynHandler.h"
#include "EchoBiomeVegLayerCluster.h"
#include "EchoVegetationAreaTestHelper.h"
#include "EchoBiomeVegRandom.h"
#include "EchoBiomeVegOpt.h"
#include "EchoKarstSystem.h"
#include "EchoSphericalTerrainResource.h"
#include "EchoVegRuntimeMgr.h"

namespace Echo
{
	namespace PlaneVeg
	{
		using _VegLayerInfo			=	BiomeVegetation::VegLayerInfo;
		using _VegLayerInfoArray	=	BiomeVegetation::VegLayerInfoArray;
		using _RenderList			=	std::vector<std::pair<float, class _Render*>>;

		struct _DenseEffect
		{
			bool  isEnable = false;
			float denFactor = 1.f;
		};
		using _DenseEffectMap = std::map<int, std::vector<_DenseEffect>>;

		class _QuadTreeNode
		{
		public:
			static void	FindVis(uint32 nodeID, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const FloatRect& curRange, uint32 level = 0);
			static void	GetRandomData(const FloatRect& nodeRange, float faceSize, const Vector3& pageRootPos, const PlaneVegCreateInfo& info, const _DenseEffectMap& denEffMap, std::array<std::vector<std::vector<Vector3>>, 3>& randomDataArray);

			static bool IsLeaf(const FloatRect& r, uint32 level)
			{
				if (level >= BiomeVegOpt::GetDevideDepth() || (r.width() <= BiomeVegOpt::GetDevideSize() && r.height() <= BiomeVegOpt::GetDevideSize()))
					return true;
				return false;
			};

			static uint32 GetChildID(uint32 pID, uint32 i)
			{
				return (pID + 1u) * 4u + i;
			}
		};

		class _QuadTree
		{
		public:
			static void FindVis(const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp);
		};

		struct alignas(16) _InsData
		{
			Vector4 pos = Vector4::ZERO;	//	xyz + scale
			uint8 normal_qua[4] = {};
		};
		using _InsDataArray = std::vector<_InsData>;

		class _Render : public Renderable
		{
		public:
			_Render(const _InsDataArray& insDataArray, const BiomeVegLayerIns& vegRes, uint32 lod)
				: mLod(lod), mRes(vegRes)
			{
				mInstanceCount = (uint32)insDataArray.size();
				if (1u == mLod && vegRes.isLoaded() && vegRes.getVegTemp()->getDenseEffect())
				{
					mInstanceCount = (uint32)std::ceil(mInstanceCount / vegRes.getVegTemp()->getDenseFactor());
					mInstanceCount = std::min(mInstanceCount, (uint32)insDataArray.size());
				}
				mInsDataArray = std::vector(insDataArray.begin(), insDataArray.begin() + mInstanceCount);

				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer.resize(2);
				_Load();
			}

			virtual ~_Render() override
			{
				if (mCanDraw)
				{
					RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
					meshData.m_vertexBuffer[0] = nullptr;
					meshData.m_IndexBufferData.m_indexBuffer = nullptr;
				}
			}

			bool isCanDraw()
			{
				_Load();
				return true == mCanDraw && mInstanceCount > 0u && getMaterial() != nullptr && mBox != AxisAlignedBox::BOX_NULL;
			};

			virtual void				getWorldTransforms(const DBMatrix4** xform) const override { *xform = &mTrans; };
			virtual void				getWorldTransforms(DBMatrix4* xform)		const override { *xform = mTrans; };
			virtual RenderOperation*	getRenderOperation()			  override { return &mRenderOperation; };
			virtual const Material*		getMaterial()				const override { return mRes.getMaterial(mLod, mPreZ); };
			virtual const LightList&	getLights()					const override { return mLightList; };
			virtual uint32				getInstanceNum()				  override { return uint32(mInstanceCount * mInsDensity); };
			uint32						getLod()					const { return mLod; };
			const BiomeVegLayerIns&		getRes()					const { return mRes; }
			float						getShowDis()				const { return mRes.getVegTemp()->getRange(mLod); };
			const _InsDataArray&		getInsData()				const { return mInsDataArray; };
			AxisAlignedBox				getBox()					const { return AxisAlignedBox(mBox.getMinimum() + mSphPos, mBox.getMaximum() + mSphPos); };

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

				const _VegLayerInfo& vegLayerInfo = mRes.getInsData();

				const VegetationMeshTemplate* meshTemplate = &mRes.getVegTemp()->getMesh(mLod);
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom2, Vector4(
					meshTemplate->bounds.getMinimum().y,
					meshTemplate->bounds.getMaximum().y,
					vegLayerInfo.scaleBase,
					vegLayerInfo.scaleRange));

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

			void						setPreZ(bool v) { mPreZ = v; }
			void						setInsDensity(float v) { mInsDensity = v; };

			PassType					calPassType(const Camera* pCamera, const _Private* _dp)
			{
				return ShadowResiveInstancePass;//接受主光阴影
			}

		void						setSphPos(const Vector3& pos) { setRenderData(pos, mCamPosWorld); };
		void						setRenderData(const Vector3& sphPos, const DVector3& camPos) {
			mSphPos = sphPos;
			mCamPosWorld = camPos;
			mCamPos = camPos;

			mTrans.makeTrans(mCamPos);
		}		public:
			void collectRuntimeData()const;

		private:
			_Render(const _Render&) = delete;
			const _Render& operator=(const _Render&) = delete;

			void _Load()
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
				float mBoundPadding = furthest_corner.length() * (mRes.getInsData().scaleBase + mRes.getInsData().scaleRange);
				for (const auto& ins : mInsDataArray)
				{
					mBox.merge(Vector3(ins.pos.x - mBoundPadding, ins.pos.y - mBoundPadding, ins.pos.z - mBoundPadding));
					mBox.merge(Vector3(ins.pos.x + mBoundPadding, ins.pos.y + mBoundPadding, ins.pos.z + mBoundPadding));
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

		private:
			DBMatrix4		mTrans = DBMatrix4::IDENTITY;
			DVector3		mCamPos = DVector3::ZERO;
			DVector3		mCamPosWorld = DVector3::ZERO;

			Vector3		mSphPos = Vector3::ZERO;
			DVector3		mSphPosWorld = DVector3::ZERO;
			_InsDataArray	mInsDataArray;
			uint32			mLod = 0;
			float			mInsDensity = 1.f;
			bool			mPreZ = true;
			uint32			mInstanceCount = 0u;
			const BiomeVegLayerIns& mRes;
			AxisAlignedBox	mBox = AxisAlignedBox::BOX_NULL;
			RenderOperation mRenderOperation;
			bool			mCanDraw = false;
			LightList		mLightList;
		};
		using _RenderPtr = std::shared_ptr<_Render>;

		enum class _AsyncState { UnLoad, Loading, Loaded };
		class _RenderCache : public AsynTaskSubmitter
		{
			friend class _AsyncTask;
		public:
			_RenderCache(const FloatRect& nodeRange, const _Private* _dp);
			const std::vector<_RenderPtr>&			getHandle()		const { return mHandle; };
			float									getLastTime()	const { return mLastTime; };
			void									update(float t)		  { mLastTime = t; };
		public:
			void collectRuntimeData()const;
		private:
			void									_Recalculate(const _Private* _dp, std::map<int, std::array<std::vector<_InsData>, 3>>& hitInsDataMap);
		private:
			std::vector<_RenderPtr> mHandle;
			float mLastTime = 0.f;
			FloatRect mRange;

			_AsyncState mState = _AsyncState::UnLoad;
		};

		class _AsyncTask : public AsynTask
		{
		public:
			using AsynTask::AsynTask;	//	继承构造
			virtual bool task_executionImpl() override;
			virtual void task_finishedImpl() override;

			FloatRect range;
			float faceSize = 0.f;
			Vector3 pageRootPos = Vector3::ZERO;
			PlaneVegCreateInfo info;
			_DenseEffectMap denseEffMap;

			const _Private* _dp = nullptr;

			std::map<int, std::array<std::vector<_InsData>, 3>> hitInsDataMap;
		private:
			_RenderCache* get() { return static_cast<_RenderCache*>(mAsynTaskSubmitter); };
		};

		class _RenderCachePool
		{
		public:
			_RenderCachePool() = default;
			~_RenderCachePool() { for (auto&& it : mPool) it.second->lockAbortAllTask(); };
			void clear() { mPool.clear(); };
			void checkOverdue(float fCurTime)
			{
				for (auto it = mPool.begin(); it != mPool.end(); )
				{
					if (fCurTime - it->second->getLastTime() > mHoldTime)
						it = mPool.erase(it);
					else
						it++;
				}
			}

			void findVis(uint32 id, const FloatRect& nodeRange, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp);

		public:
			void collectRuntimeData() const;
		private:
			float mHoldTime = 0.f;
			std::map<uint32, std::unique_ptr<_RenderCache>>	mPool;
		};

		class _DyBatchDeviceMemMgr
		{
		public:
			struct DeviceMemBlockQueue;

			struct DeviceMemBlock {
				RcBuffer* buffer;
				uint32 memSize = 0;
				float lastUsedTime = 0.f;
				DeviceMemBlockQueue* parent = 0;
			};

			struct DeviceMemBlockQueue {
				uint32 allocated = 0;
				uint32 memBlockSize;
				std::deque<_DyBatchDeviceMemMgr::DeviceMemBlock> memVacant;
			};

			DeviceMemBlock requestBuffer(void* data, uint32 size);
			void returnBuffer(DeviceMemBlock & block);
			void checkOverDue(float fCurTime);

			void outputDetail()const {
				std::stringstream ss;
				for (const auto& i : mMemQueues) {
					ss << i.memBlockSize << " : " << i.allocated << " | ";
				}
				ss << "Dedicate mem total : " << totalAllocatedDedicateBufferSize << " Alive " << allocatedDedicatedBufferSize;
				LogManager::instance()->logMessage(LML_CRITICAL, ss.str());
			}

			_DyBatchDeviceMemMgr(const std::vector<uint32>&bufferSizeLists, float resHoldTime);
			~_DyBatchDeviceMemMgr();

			void collectRuntimeData()const;

		private:
			void releaseAll();
			DeviceMemBlock requestBuffer(uint32 size);
			DeviceMemBlock requestBuffer(DeviceMemBlockQueue & queue);
			DeviceMemBlock requestDedicateBuffer(uint32 memSize);
		private:
			std::vector<DeviceMemBlockQueue> mMemQueues;
			float resHoldTime = 5.f;

			//----Statistic data---//
			//Dedicated allocate IS harmful
			uint32 totalAllocatedDedicateBufferNum = 0, totalAllocatedDedicateBufferSize = 0;
			uint32 allocatedDedicatedBufferSize = 0, allocatedDedicatedBufferNum = 0;

			uint32 totalAllocatedBufferNum = 0, totalAllocatedBufferSize = 0;
			uint32 allocatedBufferSize = 0, allocatedBufferNum = 0;
		};

		class _BatchRender : public Renderable
		{
		public:
			_BatchRender(_InsDataArray& insDataArray, uint32 lod, const BiomeVegLayerIns& res, _DyBatchDeviceMemMgr* memMgr)
				:mDeviceMemPool(memMgr), mLod(lod), mInstanceCount((uint32)insDataArray.size()), mRes(res)
			{
				enableCustomParameters(true);
				enableInstanceRender(true);

				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer.resize(2);

				if (insDataArray.empty() || mRes.isCanDraw(mLod) == false)
					return;

				const BiomeVegLayerTempPtr& vegLayerTemp = mRes.getVegTemp();
				meshData.m_vertexBuffer[0] = vegLayerTemp->getVertexBuffer(mLod);
				meshData.m_vertexCount = vegLayerTemp->getVertexCount(mLod);
				meshData.m_vertexBuffSize = vegLayerTemp->getVertexBuffSize(mLod);

				meshData.m_IndexBufferData.m_indexBuffer = vegLayerTemp->getIndexBuffer(mLod);
				meshData.m_IndexBufferData.m_indexCount = vegLayerTemp->getIndexCount(mLod);;
				meshData.m_indexBufferSize = vegLayerTemp->getIndexBuffSize(mLod);

				mMemBlock = mDeviceMemPool->requestBuffer(insDataArray.data(), (uint32)insDataArray.size() * sizeof(_InsData));
				meshData.m_vertexBuffer[1] = mMemBlock.buffer;
				mRenderOperation.mPrimitiveType = ECHO_TRIANGLE_LIST;
			}

			virtual ~_BatchRender() override
			{
				RenderOperation::st_MeshData& meshData = mRenderOperation.GetMeshData();
				meshData.m_vertexBuffer[0] = nullptr;
				meshData.m_IndexBufferData.m_indexBuffer = nullptr;
				meshData.m_vertexBuffer[1] = 0;
				mDeviceMemPool->returnBuffer(mMemBlock);
			}

			virtual void getWorldTransforms(const DBMatrix4** xform)const override { *xform = &mTrans; };
			virtual void getWorldTransforms(DBMatrix4* xform)		const override { *xform = mTrans; };
			virtual RenderOperation* getRenderOperation()			  override { return &mRenderOperation; };
			virtual const Material* getMaterial()				const override { return mRes.getMaterial(mLod, mPreZ); };
			virtual const LightList& getLights()					const override { return sEmptyLightList; };
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

				const _VegLayerInfo& vegLayerInfo = mRes.getInsData();

				const VegetationMeshTemplate* meshTemplate = &mRes.getVegTemp()->getMesh(mLod);
				pRenderSystem->setUniformValue(this, pPass, U_VSCustom2, Vector4(
					meshTemplate->bounds.getMinimum().y,
					meshTemplate->bounds.getMaximum().y,
					vegLayerInfo.scaleBase,
					vegLayerInfo.scaleRange));

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

		public:
			_DyBatchDeviceMemMgr* mDeviceMemPool = 0;
			_DyBatchDeviceMemMgr::DeviceMemBlock mMemBlock;
		private:
			DBMatrix4		mTrans = DBMatrix4::IDENTITY;
			DVector3		mCamPos = DVector3::ZERO;

			const uint32 mLod = 0u;
			const uint32 mInstanceCount = 0u;
			const BiomeVegLayerIns& mRes;
			RenderOperation mRenderOperation;

			bool	mPreZ = true;
			Vector3	mSphPos = Vector3::ZERO;

			inline static LightList sEmptyLightList;
		private:
			_BatchRender(_BatchRender&) = delete;
			_BatchRender& operator=(_BatchRender&) = delete;
		};

		_DyBatchDeviceMemMgr::DeviceMemBlock _DyBatchDeviceMemMgr::requestBuffer(uint32 size)
		{
			int idx = -1;
			for (auto i = 0; i < mMemQueues.size(); ++i) {
				if (mMemQueues[i].memBlockSize > size) {
					idx = i;
					break;
				}
			}
			if (idx >= 0) {
				return requestBuffer(mMemQueues[idx]);
			}
			else
			{
				return requestDedicateBuffer(size);
			}
		}

		_DyBatchDeviceMemMgr::DeviceMemBlock _DyBatchDeviceMemMgr::requestBuffer(void* data, uint32 size)
		{
			auto ret = requestBuffer(size);
			RenderSystem* pSys = Root::instance()->getRenderSystem();
			pSys->updateBuffer(ret.buffer, data, size, false);
			return ret;
		}

		void _DyBatchDeviceMemMgr::returnBuffer(DeviceMemBlock& block)
		{
			//For Dedicate memory destroy.
			block.lastUsedTime = Root::instance()->getTimeElapsed();
			if (block.parent == 0) {
				RenderSystem* pSys = Root::instance()->getRenderSystem();
				pSys->destoryBuffer(block.buffer);

				allocatedDedicatedBufferNum--;
				allocatedDedicatedBufferSize -= block.memSize;

			}
			else {

				allocatedBufferNum--;
				allocatedBufferSize -= block.memSize;
				block.parent->memVacant.push_front(block);
			}
			block.buffer = 0;
			block.parent = 0;
		}

		void _DyBatchDeviceMemMgr::checkOverDue(float fCurTime)
		{
			RenderSystem* pSys = Root::instance()->getRenderSystem();
			for (auto&& i : this->mMemQueues) {
				for (auto&& memItor = i.memVacant.begin(); memItor != i.memVacant.end();) {
					if (fCurTime - memItor->lastUsedTime > resHoldTime) {
						pSys->destoryBuffer(memItor->buffer);
						memItor = i.memVacant.erase(memItor);
						i.allocated--;
					}
					else {
						++memItor;
					}
				}
			}
		}

		_DyBatchDeviceMemMgr::_DyBatchDeviceMemMgr(const std::vector<uint32>& bufferSizeLists, float resHoldTime)
		{
			this->resHoldTime = resHoldTime;
			auto sizeList = bufferSizeLists;
			std::sort(sizeList.begin(), sizeList.end());
			this->mMemQueues.reserve(sizeList.size());
			for (uint32 i : sizeList) {
				DeviceMemBlockQueue queue{ .memBlockSize = i };
				mMemQueues.push_back(queue);
			}
		}

		_DyBatchDeviceMemMgr::~_DyBatchDeviceMemMgr()
		{
			releaseAll();
		}

		void _DyBatchDeviceMemMgr::collectRuntimeData()const
		{
			{
				size_t cpuMemUsed = sizeof(*this);
				cpuMemUsed += mMemQueues.capacity() * sizeof(decltype(*mMemQueues.begin()));
				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::DYBATCH_CPU_MEM, cpuMemUsed);
			}

			{
				size_t gpuMemUsed = 0;
				for (const auto& i : this->mMemQueues)
					gpuMemUsed += i.memBlockSize * i.allocated;
				VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::GPU_MEM | VegRuntimeType::DYBATCH_GPU_MEM, gpuMemUsed);
			}
		}

		void _DyBatchDeviceMemMgr::releaseAll()
		{
			RenderSystem* pSys = Root::instance()->getRenderSystem();
			for (auto&& i : this->mMemQueues) {
				for (auto&& mem : i.memVacant) {
					pSys->destoryBuffer(mem.buffer);
				}
			}
		}

		_DyBatchDeviceMemMgr::DeviceMemBlock _DyBatchDeviceMemMgr::requestBuffer(DeviceMemBlockQueue& queue)
		{
			DeviceMemBlock block;
			if (queue.memVacant.size() > 0) {
				block = queue.memVacant.front();
				queue.memVacant.pop_front();
			}
			else {
				totalAllocatedBufferSize += queue.memBlockSize;
				totalAllocatedBufferNum++;

				allocatedBufferSize += queue.memBlockSize;
				allocatedBufferNum++;

				queue.allocated++;

				RenderSystem* pSys = Root::instance()->getRenderSystem();
				block.memSize = queue.memBlockSize;
				block.parent = &queue;
				block.buffer = pSys->createDynamicVB(nullptr, queue.memBlockSize);
			}
			return block;
		}

		_DyBatchDeviceMemMgr::DeviceMemBlock _DyBatchDeviceMemMgr::requestDedicateBuffer(uint32 memSize)
		{
			totalAllocatedDedicateBufferNum++;
			totalAllocatedDedicateBufferSize += memSize;

			allocatedDedicatedBufferNum++;
			allocatedBufferSize += memSize;

			DeviceMemBlock block{};
			RenderSystem* pSys = Root::instance()->getRenderSystem();
			block.buffer = pSys->createDynamicVB(nullptr, memSize);
			block.memSize = memSize;
			return block;
		}

		class _DyBatchMgr 
		{
			struct _BatchDesc
			{
				std::array<uint32, 2> lodInsCount = { 0u , 0u };
				std::vector<_Render*> renderArray;
			};

		public:
			_DyBatchMgr() {
				mDeviceMemPool = new _DyBatchDeviceMemMgr({ sizeof(_InsData) * 512,sizeof(_InsData) * 1024,sizeof(_InsData) * 2048,sizeof(_InsData) * 4096 }, 5.f);
			}

			~_DyBatchMgr() {
				mBatchRender.clear();
				SAFE_DELETE(mDeviceMemPool);
			}

			void checkOverDue(float fCurTime) {
				mDeviceMemPool->checkOverDue(fCurTime);
			}

			void add(PassType pt, _Render * render)
			{
				_BatchDesc& batchDesc = mFrameBatchPool[pt][&render->getRes()];
				batchDesc.lodInsCount[render->getLod()] += render->getInstanceNum();
				batchDesc.renderArray.push_back(render);
			}

			void clear()
			{
				//mDeviceMemPool->outputDetail();
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
			}

			void submit(RenderQueue * pQueue, const Vector3 & planetPos, const DVector3 & camPos)
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

							std::array<_InsDataArray, 2> lodInsData;
							for (uint32 i = 0; i < 2; i++)
								lodInsData[i].reserve(batchDesc.lodInsCount[i]);

							for (auto&& renderIt : batchDesc.renderArray)
							{
								_InsDataArray& _dst = lodInsData[renderIt->getLod()];
								const _InsDataArray& _src = renderIt->getInsData();

								uint32 instanceNum = renderIt->getInstanceNum();
								_dst.insert(_dst.end(), _src.begin(), _src.begin() + instanceNum);
							}

							for (uint32 i = 0; i < 2; i++)
							{
								if (!lodInsData[i].empty())
									mBatchRender.push_back({ passType , std::make_unique<_BatchRender>(lodInsData[i], i, *insPtr, mDeviceMemPool) });
							}
						}
					}
				}

				{
					for (auto&& it : mBatchRender)
					{
						it.second->setDrawData(planetPos, camPos, BiomeVegOpt::GetPreZEnabled());
						const Material* mat = it.second->getMaterial();

						if (BiomeVegOpt::GetPreZEnabled())
						{
							const PassType passType = PassType::AlphaTestNoColor;
							pQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(mat->getPass(passType), it.second.get());
							BiomeVegOpt::StatisticsDraw(it.second.get());
						}

						{
							const PassType passType = it.first;
							pQueue->getRenderQueueGroup(RenderQueue_Vegetation)->addRenderable(mat->getPass(passType), it.second.get());
							BiomeVegOpt::StatisticsDraw(it.second.get());
						}
					}
				}
			}

			void collectRuntimeData()const {
				if (mDeviceMemPool)
					mDeviceMemPool->collectRuntimeData();
			}
		private:
			std::vector <std::pair<PassType, std::unique_ptr<_BatchRender>>> mBatchRender;
			std::map<PassType, std::map<const BiomeVegLayerIns*, _BatchDesc>> mFrameBatchPool;
			_DyBatchDeviceMemMgr* mDeviceMemPool = 0;
		};

		struct _Private
		{
			BiomeVegGroup*		mGroup = nullptr;
			PlaneVegCreateInfo	mInfo;
			bool				mIsReady = false;
			float				mBoxVisDis = 128.f;
			_RenderList			mRenderList;
			std::map<int, std::vector<BiomeVegLayerIns>> mVegLayerInsMap;

			std::unique_ptr<_RenderCachePool> mRenderCachePool;

			_DyBatchMgr mDyBatchMgr;

			//	PageData
			Vector3				mPageRoot = Vector3::ZERO;
			float				mPageOff = 0.f;
			float				mFaceSize = 0.f;

			void init()
			{
				mRenderCachePool = std::make_unique<_RenderCachePool>();

				{
					const AxisAlignedBox& pageBox = mInfo.page->getPageBound();
					mPageRoot = pageBox.getCenter();
					mPageOff = pageBox.getHalfSize().y;
					mFaceSize = mInfo.page->getPageSize().x;
				}

				{
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
			}

			void shutdown()
			{
				mVegLayerInsMap.clear();
				mRenderCachePool.reset();
				mIsReady = false;
			}

			void clearRenderCache() { mRenderCachePool = std::make_unique<_RenderCachePool>(); };

			bool checkReady()
			{
				if (false == mIsReady)
				{
					mBoxVisDis = 128.f;
					for (auto&& it : mVegLayerInsMap)
					{
						for (BiomeVegLayerIns& ins : it.second)
						{
							if (false == ins.isLoaded())
								return false;

							for (int lod = 0; lod < 2; lod++)
							{
								if (ins.isCanDraw(lod))
									mBoxVisDis = std::max(mBoxVisDis, BiomeVegOpt::GetRealDis(lod, ins.getVegTemp()->getRange(lod)));
							}
						}
					}

					mBoxVisDis = std::min(mBoxVisDis, BiomeVegOpt::GetDisLimit(1));
					mIsReady = true;
				}

				return mIsReady;
			}
		};
	}

	using namespace PlaneVeg;

	PlaneVegetation::PlaneVegetation(const PlaneVegCreateInfo& info, BiomeVegGroup* groupPtr)
		:_dp(new _Private)
	{
		_dp->mGroup = groupPtr;
		_dp->mInfo = info;

		_dp->init();
	}

	PlaneVegetation::~PlaneVegetation()
	{
		_dp->shutdown();
	}

	void PlaneVegetation::findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue)
	{
		if (false == BiomeVegOpt::GetVisEnabled())
			return;
		if (false == _dp->checkReady())
			return;
		if (pCamera->getCameraUsage() != CameraUsage::SceneCamera)
			return;

		float fCurTime = Root::instance()->getTimeElapsed();

		_dp->mDyBatchMgr.clear();
		_dp->mRenderList.clear();
		_dp->mRenderList.reserve(50);

		_QuadTree::FindVis(pCamera, pQueue, fCurTime, _dp.get());

		if (BiomeVegOpt::GetDyBatchEnabled())
		{
			_dp->mDyBatchMgr.checkOverDue(fCurTime);
			for (auto&& it : _dp->mRenderList)
			{
				const PassType passType = it.second->calPassType(pCamera, _dp.get());
				_dp->mDyBatchMgr.add(passType, it.second);
			}
			_dp->mDyBatchMgr.submit(pQueue, _dp->mPageRoot, pCamera->getPosition());
		}
		else
		{
			std::sort(_dp->mRenderList.begin(), _dp->mRenderList.end(), [](auto l, auto r) {return l.first < r.first; });
			for (auto&& it : _dp->mRenderList)
			{
				it.second->setPreZ(BiomeVegOpt::GetPreZEnabled());
				const Material* mat = it.second->getMaterial();

				if (BiomeVegOpt::GetPreZEnabled())
				{
					const PassType passType = PassType::AlphaTestNoColor;
					pQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest)->addRenderable(mat->getPass(passType), it.second);
					BiomeVegOpt::StatisticsDraw(it.second);
				}

				{
					const PassType passType = it.second->calPassType(pCamera, _dp.get());
					pQueue->getRenderQueueGroup(RenderQueue_Vegetation)->addRenderable(mat->getPass(passType), it.second);
					BiomeVegOpt::StatisticsDraw(it.second);
				}
			}
		}
		_dp->mRenderCachePool->checkOverdue(fCurTime);
	}

	void PlaneVegetation::flushObjects()
	{
		_dp->clearRenderCache();
	}

	void PlaneVegetation::clearAll()
	{
		_dp->clearRenderCache();
		_dp->mIsReady = false;
	}

	void PlaneVegetation::clearRenderCache()
	{
		_dp->clearRenderCache();
	}

	void PlaneVegetation::updateLayerParam(int terIndex, const VegLayerInfo& vegLayerInfo)
	{
		_dp->mInfo.manager->updateLayerParam(terIndex, vegLayerInfo);

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

	void PlaneVegetation::updateAllVegRes(int terIndex, const VegLayerInfoArray& vegLayerInfoArray)
	{
		_dp->mInfo.manager->updateAllVegRes(terIndex, vegLayerInfoArray);

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

	void PlaneVegetation::removeAllVegRes(int terIndex)
	{
		_dp->mInfo.manager->removeAllVegRes(terIndex);

		_dp->clearRenderCache();
		_dp->mInfo.vegLayerMap.erase(terIndex);
		_dp->mVegLayerInsMap.erase(terIndex);

		_dp->mIsReady = false;
	}

	void PlaneVegetation::collectRuntimeData() const
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

		if (_dp->mRenderCachePool)
			_dp->mRenderCachePool->collectRuntimeData();

		_dp->mDyBatchMgr.collectRuntimeData();
	}

	void PlaneVegetation::onReDevide()
	{
		_dp->clearRenderCache();
	}

	void PlaneVegetation::onDenLimitChange()
	{

	}

	void PlaneVegetation::onDisLevelChange()
	{
		_dp->mIsReady = false;
	}

	namespace PlaneVeg
	{
		enum class _NodeType : uint32
		{									//	---------------
			LB = 0U,						//	|  LB  |  RB  |
			RB,								//	---------------
			RT,								//	|  LT  |  RT  |
			LT,								//	---------------

			NodeTypeMax
		};

		FloatRect _GetChildRect(_NodeType type, const FloatRect& rect)
		{
			switch (type)
			{
			case _NodeType::LB:
				return FloatRect(rect.left, (rect.bottom + rect.top) / 2.f, (rect.left + rect.right) / 2.f, rect.bottom);
				break;
			case _NodeType::RB:
				return FloatRect((rect.left + rect.right) / 2.f, (rect.bottom + rect.top) / 2.f, rect.right, rect.bottom);
				break;
			case _NodeType::RT:
				return FloatRect((rect.left + rect.right) / 2.f, rect.top, rect.right, (rect.bottom + rect.top) / 2.f);
				break;
			case _NodeType::LT:
				return FloatRect(rect.left, rect.top, (rect.left + rect.right) / 2.f, (rect.bottom + rect.top) / 2.f);
				break;
			default:
				break;
			}
			return rect;
		}

		void _QuadTree::FindVis(const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp)
		{
			const float halfSize = _dp->mFaceSize / 2.f;
			const FloatRect rectRange(-halfSize, -halfSize, halfSize, halfSize);

			for (uint32 i = 0; i < 4; i++)
			{
				uint32 curChildID = i;
				_QuadTreeNode::FindVis(curChildID, pCamera, pQueue, fCurTime, _dp, _GetChildRect(_NodeType(i), rectRange), 1);
			}
		}

		void _QuadTreeNode::FindVis(uint32 nodeID, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp, const FloatRect& curRange, uint32 level /*= 0*/)
		{
			const AxisAlignedBox worldBox(
				Vector3(curRange.left, -_dp->mPageOff, curRange.top) + _dp->mPageRoot,
				Vector3(curRange.right, _dp->mPageOff, curRange.bottom) + _dp->mPageRoot);

			const float camToBoxNearSquareDis = VegOpts::SquareDistPointToAABB(pCamera->getPosition(), worldBox);
			if (camToBoxNearSquareDis >= _dp->mBoxVisDis * _dp->mBoxVisDis)
				return;

			if (false == pCamera->isVisible(worldBox))
				return;

			/*const DVector3 camToLocPos = pCamera->getPosition() - _dp->mPageRoot;

			const AxisAlignedBox nodeLocBox(curRange.left, _dp->mPageOff.x, curRange.top, curRange.right, _dp->mPageOff.y, curRange.bottom);
			if (nodeLocBox.squaredDistance(camToLocPos) >= _dp->mBoxVisDis * _dp->mBoxVisDis)
				return;

			const AxisAlignedBox& nodeWorldBox = AxisAlignedBox(nodeLocBox.mMinimum + _dp->mPageRoot, nodeLocBox.mMaximum + _dp->mPageRoot);
			if (!pCamera->isVisible(nodeWorldBox))
				return;*/

			if (IsLeaf(curRange, level))
				_dp->mRenderCachePool->findVis(nodeID, curRange, pCamera, pQueue, fCurTime, _dp);
			else
			{
				for (uint32 i = 0; i < 4; i++)
					FindVis(GetChildID(nodeID, i), pCamera, pQueue, fCurTime, _dp, _GetChildRect(_NodeType(i), curRange), level + 1);
			}
		}

		//int _GetPlaneBiomeID(const Vector2& localPos)
		//{
		//	int terIndex = -1;
		//	uint8 typeID;
		//	if (KarstSystem::instance()->getSurfaceTypeAtWorldPos(DVector3(localPos.x, 0.0, localPos.y), typeID)) {
		//		switch (typeID) {
		//		case 11: terIndex = 0; break;
		//		case 12: terIndex = 1; break;
		//		case 21: terIndex = 2; break;
		//		case 22: terIndex = 3; break;
		//		case 23: terIndex = 4; break;
		//		case 24: terIndex = 5; break;
		//		case 25: terIndex = 6; break;
		//		case 31: terIndex = 7; break;
		//		case 32: terIndex = 8; break;
		//		case 33: terIndex = 9; break;
		//		case 41: terIndex = 10; break;
		//		case 42: terIndex = 11; break;
		//		case 43: terIndex = 12; break;
		//		case 44: terIndex = 13; break;
		//		case 45: terIndex = 14; break;
		//		case 46: terIndex = 15; break;
		//		case 51: terIndex = 16; break;
		//		case 52: terIndex = 17; break;
		//		case 53: terIndex = 18; break;
		//		case 54: terIndex = 19; break;
		//		case 61: terIndex = 20; break;
		//		case 62: terIndex = 21; break;
		//		case 63: terIndex = 22; break;
		//		case 64: terIndex = 23; break;
		//		case 65: terIndex = 24; break;
		//		case 66: terIndex = 25; break;
		//		case 67: terIndex = 26; break;
		//		case 99: terIndex = 27; break;
		//		case 255: terIndex = 28; break;
		//		}
		//	}
		//	return terIndex;
		//}

		//bool _GetPlaneHeight(const Vector2& localPos, float& height, Vector3& normal)
		//{
		//	bool res1 = KarstSystem::instance()->getHeightAtWorldPos(DVector3(localPos.x, 0.0, localPos.y), height);
		//	bool res2 = KarstSystem::instance()->getNormalAtWorldPos(DVector3(localPos.x, 0.0, localPos.y), normal);
		//	return res1 && res2;
		//}

		void _QuadTreeNode::GetRandomData(const FloatRect& nodeRange, float faceSize, const Vector3& pageRootPos, const PlaneVegCreateInfo& info, const _DenseEffectMap& denEffMap, std::array<std::vector<std::vector<Vector3>>, 3>& randomDataArray)
		{
			static const float matSize = 8.f;
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

					const std::array<int, 5> terIndexArr = {
						info.page->getManager()->getTerrainSurfaceType(sampArr[0].x + pageRootPos.x  , sampArr[0].y + pageRootPos.z),
						info.page->getManager()->getTerrainSurfaceType(sampArr[1].x + pageRootPos.x  , sampArr[1].y + pageRootPos.z),
						info.page->getManager()->getTerrainSurfaceType(sampArr[2].x + pageRootPos.x  , sampArr[2].y + pageRootPos.z),
						info.page->getManager()->getTerrainSurfaceType(sampArr[3].x + pageRootPos.x  , sampArr[3].y + pageRootPos.z),
						info.page->getManager()->getTerrainSurfaceType(sampArr[4].x + pageRootPos.x  , sampArr[4].y + pageRootPos.z),
					};

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

						if(expDen <= 0.000001f) continue;
						uint32 vegInsCount = (uint32)std::ceil(BiomeVegOpt::GetDenLimit() * expDen * matBlockWidth * matBlockHight / 16.f);
						vegInsCount = (uint32)std::ceil(vegInsCount * effDen);

						randomDataArray.at(layerIndex).resize(randomDataArray.at(layerIndex).size() + 1);
						std::vector<Vector3>& randomArray = randomDataArray.at(layerIndex).back();


						std::vector<Vector3> tempRandom;
						BiomeVegRandom::GetRandom(vegInsCount, tempRandom, Vector2((b_x + 0.5f) * (layerIndex + 1) * 11.f, (b_y + 0.5f) * (layerIndex + 1) * 17.f));
						for (auto&& random_p : tempRandom)
						{
							//	Pos from [0, 1] to [LeftTop, RightBottom]
							random_p.x = b_x + matBlockWidth * random_p.x;
							random_p.y = b_y + matBlockHight * random_p.y;
							//	Scale from [0, 1] to [-1, 1]
							random_p.z = random_p.z * 2.f - 1.f;

							const Vector2 nSamP = Vector2(random_p.x, random_p.y) + Vector2(pageRootPos.x, pageRootPos.z)
								+ (BiomeVegOpt::GetRandomOff() * Vector2(std::cos(random_p.z * Math::PI), std::sin(random_p.z * Math::PI)));

							int terIndex = info.page->getManager()->getTerrainSurfaceType(nSamP.x, nSamP.y);
							if(terIndex == -1)
								continue;

							auto randomFindIt = info.vegLayerMap.find(terIndex);
							if(randomFindIt == info.vegLayerMap.end())
								continue;

							if (layerIndex >= randomFindIt->second.size())
								continue;

							float layer_den = randomFindIt->second.at(layerIndex).density;
							auto denEffIt = denEffMap.find(terIndex);
							if (denEffIt != denEffMap.end() && layerIndex < denEffIt->second.size())
							{
								if (denEffIt->second.at(layerIndex).isEnable)
									layer_den = layer_den * denEffIt->second.at(layerIndex).denFactor;
							}

							if (layer_den < (expDen * effDen))
							{
								float ratio = layer_den / (expDen * effDen);
								if((random_p.z + 1.f) / 2.f > ratio)
									continue;
							}

							randomArray.push_back(random_p);
						}
					}
				}
			}
		}

		void _Render::collectRuntimeData()const
		{
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::RENDER_CACHE_CPU_MEM, sizeof(*this) + mInsDataArray.capacity() * sizeof(_InsData));
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::GPU_MEM | VegRuntimeType::RENDER_CACHE_GPU_MEM, mInsDataArray.capacity() * sizeof(_InsData));
		}

		_RenderCache::_RenderCache(const FloatRect& nodeRange, const _Private* _dp)
			:mRange(nodeRange)
		{
			std::shared_ptr<_AsyncTask> task = std::make_shared<_AsyncTask>(this);
			task->range = mRange;
			task->_dp = _dp;
			task->faceSize = _dp->mFaceSize;
			task->pageRootPos = _dp->mPageRoot;
			task->info = _dp->mInfo;


			for (auto&& it : task->info.vegLayerMap)
			{
				task->denseEffMap[it.first].resize(it.second.size());

				auto findIt = _dp->mVegLayerInsMap.find(it.first);
				if (findIt == _dp->mVegLayerInsMap.end())
					continue;

				for (uint32 i = 0; i < it.second.size(); i++)
				{
					if (i >= findIt->second.size())
						continue;
					if (false == findIt->second.at(i).isLoaded())
						continue;

					task->denseEffMap[it.first].at(i) = {
						findIt->second.at(i).getVegTemp()->getDenseEffect(),
						findIt->second.at(i).getVegTemp()->getDenseFactor()
					};
				}
			}

			mState = _AsyncState::Loading;
			submit(task);
		}

		void _RenderCache::collectRuntimeData()const
		{
			size_t cpuMemUsed = sizeof(*this);
			cpuMemUsed += mHandle.capacity() * sizeof(_RenderPtr);
			VegRuntimeMgr::CollectRuntimeData(VegRuntimeType::CPU_MEM | VegRuntimeType::RENDER_CACHE_CPU_MEM, cpuMemUsed);

			for (auto&& it : mHandle)
			{
				if (it)
					it->collectRuntimeData();
			}
		}

		void _RenderCache::_Recalculate(const _Private* _dp, std::map<int, std::array<std::vector<_InsData>, 3>>& hitInsDataMap)
		{
			//	clear renderer
			mHandle.clear();

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

					const std::vector<_InsData>& insDataArray = mapIt.second.at(i);
					if (insDataArray.empty())
						continue;

					mHandle.emplace_back(std::make_shared<_Render>(insDataArray, vegIns, 0));
					mHandle.back()->setSphPos(_dp->mPageRoot);
					mHandle.emplace_back(std::make_shared<_Render>(insDataArray, vegIns, 1));
					mHandle.back()->setSphPos(_dp->mPageRoot);
				}
			}
		}

		bool _AsyncTask::task_executionImpl()
		{
			std::array<std::vector<std::vector<Vector3>>, 3> tempRandom;
			_QuadTreeNode::GetRandomData(range, faceSize, pageRootPos, info, denseEffMap, tempRandom);

			//	Merge
			std::array<std::vector<Vector3>, 3> randomDataArray;
			for (uint32 i = 0; i < 3; i++)
			{
				std::vector<std::vector<Vector3>>& rArray = tempRandom.at(i);
				if (rArray.empty()) continue;
				Vector2 rdFactor(range.bottom, range.right);
				BiomeVegRandom::Shuffle(rArray, rdFactor);
				size_t maxSize = 0;
				for (auto&& it : rArray)
					maxSize = std::max(it.size(), maxSize);

				for (size_t j = 0; j < maxSize; j++)
				{
					for (auto&& it : rArray)
					{
						if (j >= it.size()) continue;
						randomDataArray.at(i).push_back(it.at(j));
					}
				}
			}

			for (int aIndex = 0; aIndex < 3; aIndex++)
			{
				std::vector<Vector3>& randomData = randomDataArray.at(aIndex);
				for (auto&& p : randomData)
				{
					const float scaleValue = p.z;
					const Vector2 point = Vector2(p.x, p.y) + Vector2(pageRootPos.x, pageRootPos.z);

					const Vector2 nSamP = point + (BiomeVegOpt::GetRandomOff() * Vector2(std::cos(scaleValue * Math::PI), std::sin(scaleValue * Math::PI)));

					Vector3 hitNormal = Vector3::UNIT_Y;
					float height = 0.0f;
					int terIndex = _dp->mInfo.page->getManager()->getTerrainSurfaceType(nSamP.x, nSamP.y);

					if (terIndex == -1
						|| !_dp->mInfo.page->getManager()->getPointIsEmpty(point.x,point.y)			//	?? 线程安全？？
						)
						continue;

					height = _dp->mInfo.page->getManager()->getTerrainHeight(point.x, point.y);
					hitNormal = _dp->mInfo.page->getManager()->getTerrainNormal(point.x, point.y);


					const Vector3 hitPos = Vector3(point.x, height, point.y);
					const Vector3 localHitPos = hitPos - pageRootPos;

					_InsData insData = { .pos = Vector4(localHitPos.x, localHitPos.y, localHitPos.z, scaleValue) };
					VegOpts::Quaternion_FromAxesToAxes(Vector3::UNIT_Y, hitNormal.GetNormalized(), insData.normal_qua);

					hitInsDataMap[terIndex].at(aIndex).emplace_back(insData);
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

		bool _CheckVisible(const Camera* pCamera, const AxisAlignedBox& worldBox, float showDis)
		{
			const float squaDis = VegOpts::SquareDistPointToAABB(pCamera->getPosition(), worldBox);
			if (squaDis >= showDis * showDis)
				return false;
			if (!pCamera->isVisible(worldBox))
				return false;
			return true;
		}

		void _RenderCachePool::findVis(uint32 id, const FloatRect& nodeRange, const Camera* pCamera, RenderQueue* pQueue, float fCurTime, _Private* _dp)
		{
			auto findIt = mPool.find(id);
			if (mPool.end() == findIt)
				findIt = mPool.emplace(id, std::make_unique<_RenderCache>(nodeRange, _dp)).first;

			findIt->second->update(fCurTime);

			for (auto&& it : findIt->second->getHandle())
			{
				if (nullptr == it) continue;
				if (false == BiomeVegOpt::GetVisLod(it->getLod())) continue;
				if (false == it->isCanDraw())continue;
				const Material* mat = it->getMaterial();
				if (nullptr == mat) continue;
				it->setRenderData(_dp->mPageRoot, pCamera->getPosition());

				const AxisAlignedBox& worldBox = it->getBox();
				if (false == _CheckVisible(pCamera, worldBox, BiomeVegOpt::GetRealDis(it->getLod(), it->getShowDis())))
					continue;

				float squaDis = (float)(pCamera->getPosition() - worldBox.GetCenter()).length();
				float insDensity = squaDis / nodeRange.width();
				insDensity = std::max(insDensity, 1.f);
				insDensity = 1.f / insDensity;
				insDensity = std::pow(insDensity, it->getRes().getVegTemp()->getDenLessFactor() * BiomeVegOpt::GetDenLessFactor());

				it->setInsDensity(insDensity);
				if (it->getInstanceNum() == 0u)
					continue;

				const auto* sceneMgr = pCamera->getSceneManager();
				if (sceneMgr->m_PortalCullFunction && BiomeVegOpt::GetRoomCullEnabled())
				{
					bool portalCull = sceneMgr->m_PortalCullFunction(worldBox);
					if (portalCull)
						continue;
				}

				_dp->mRenderList.push_back({ squaDis , it.get() });
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
	}

	PlaneVegetationManager::PlaneVegetationManager(OAE::OAEManager* pMgr) : OAE::OAEObjectManager(pMgr) {}
	PlaneVegetationManager::~PlaneVegetationManager() {}

	void PlaneVegetationManager::ImportConfig(const String& config) {
		mConfigPath = config;
	}
	void PlaneVegetationManager::ExportConfig(const String& config) {

	}

	void PlaneVegetationManager::Initialize() {

		SphericalTerrainData terrain;
		SphericalTerrainResource::loadSphericalTerrain(mConfigPath.c_str(), terrain, false);

		mLayerMap.clear();

		if (!terrain.bExistResource) {
			Uninitialize();
			return;
		}

		for (int biomeCompoIndex = 0;
			biomeCompoIndex < terrain.distribution.distRanges.size();
			biomeCompoIndex++)
		{
			if (biomeCompoIndex < terrain.composition.biomes.size() &&
				terrain.composition.biomes[biomeCompoIndex].biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
			{
				int biomeIndex = terrain.composition.getBiomeIndex(biomeCompoIndex);
				mLayerMap[biomeIndex] = terrain.composition.biomes.at(biomeCompoIndex).vegLayers;
			}
		}
		bInitialize = true;
	}
	void PlaneVegetationManager::Uninitialize() {
		mLayerMap.clear();
		bInitialize = false;
	}

	bool IsCreate(OAE::OAEPage* page)
	{
		if (nullptr == page) return false;
		//const Vector3& cPos = page->getPageBound().getCenter();
		//
		//float tempH = 0.f;
		//bool ret =  KarstSystem::instance()->getHeightAtWorldPos(DVector3(cPos.x, 0.0, cPos.z), tempH);
		return true;
	}

	OAE::OAEObject* PlaneVegetationManager::create(OAE::OAEPage* page)
	{
		if (!bInitialize) return nullptr;
		if (false == IsCreate(page))
			return nullptr;

		const PlaneVegCreateInfo cInfo = { page, this, mLayerMap };
		PlaneVegetation* pVeg = BiomeVegGroup::GetDefaultGroup()->createVeg(cInfo);
		mObjects.insert(pVeg);
		return pVeg;
	}

	bool PlaneVegetationManager::destroy(OAE::OAEObject* object)
	{
		if (nullptr == object) return false;

		PlaneVegetation* pVeg = (PlaneVegetation*)object;
		auto iter = mObjects.find(pVeg);
		if (iter == mObjects.end()) {
			return false;
		}
		mObjects.erase(iter);
		BiomeVegGroup::GetDefaultGroup()->destoryVeg(pVeg);
		return true;
	}

	void PlaneVegetationManager::collectAllResource(std::map<OAE::OAEResourceType, std::set<String>>& fileMaps) {
		fileMaps[OAE::OAEResourceType::eSph].insert(mConfigPath);
	}

	void PlaneVegetationManager::updateLayerParam(int terIndex, const PlaneVegetation::VegLayerInfo& vegLayerInfo)
	{
		auto findInfoIt = mLayerMap.find(terIndex);
		if (findInfoIt != mLayerMap.end())
		{
			int size = (int)findInfoIt->second.size();
			for (int i = 0; i != size; ++i) {
				if (findInfoIt->second.at(i).name == vegLayerInfo.name) {
					findInfoIt->second.at(i) = vegLayerInfo;
				}
			}
		}
	}

	void PlaneVegetationManager::updateAllVegRes(int terIndex, const PlaneVegetation::VegLayerInfoArray& vegLayerInfoArray)
	{
		mLayerMap[terIndex] = vegLayerInfoArray;
	}

	void PlaneVegetationManager::removeAllVegRes(int terIndex)
	{
		mLayerMap.erase(terIndex);
	}



	PlaneVegetationSystem::PlaneVegetationSystem() : OAE::OAEObjectSystem(Name("PlaneVegetationSystem")) {}
	PlaneVegetationSystem::~PlaneVegetationSystem() {}

	OAE::OAEObjectManager* PlaneVegetationSystem::create(OAE::OAEManager* _Mgr) {
		PlaneVegetationManager* pNewMgr = new PlaneVegetationManager(_Mgr);
		mManagers[_Mgr] = pNewMgr;
		return pNewMgr;
	}
	bool PlaneVegetationSystem::destroy(OAE::OAEManager* _Mgr) {
		if (nullptr == _Mgr) return false;

		auto iter = mManagers.find(_Mgr);
		if (iter == mManagers.end()) {
			return false;
		}
		SAFE_DELETE(iter->second);
		mManagers.erase(iter);
		return true;
	}
	PlaneVegetationManager* PlaneVegetationSystem::getMainManager() {
		auto iter = mManagers.find(OAE::OAESystem::instance()->getMainManager());
		if (iter == mManagers.end()) {
			return nullptr;
		}
		return iter->second;
	}
}
