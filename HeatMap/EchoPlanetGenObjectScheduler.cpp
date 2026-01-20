#include "EchoStableHeaders.h"
#include "EchoPlanetGenObjectScheduler.h"
#include "EchoSphericalTerrainManager.h"

namespace Echo {

	namespace PCG
	{
		float _SquareDistPointToAABB(const Vector3& vPos, const Vector3& vMin, const Vector3& vMax)
		{
			float dx = std::max(vMin.x - vPos.x, std::max(vPos.x - vMax.x, 0.0f));
			float dy = std::max(vMin.y - vPos.y, std::max(vPos.y - vMax.y, 0.0f));
			float dz = std::max(vMin.z - vPos.z, std::max(vPos.z - vMax.z, 0.0f));
			return dx * dx + dy * dy + dz * dz;
		}

		float _SquareDistPointToAABB(const Vector3& vPos, const AxisAlignedBox& aabb)
		{
			return _SquareDistPointToAABB(vPos, aabb.vMin, aabb.vMax);
		}

		//	======================================================================
		using _CreateInfo = PlanetGenObjectScheduler::CreateInfo;
		using _GenSpace = PlanetGenObjectScheduler::GenSpace;
		using _GenSpaceList = PlanetGenObjectScheduler::GenSpaceList;
		using _GenSpaceMap = PlanetGenObjectScheduler::GenSpaceMap;
		class _OrthoBox
		{
		public:
			explicit _OrthoBox(const Matrix3& invBase = Matrix3::IDENTITY)
				:mInvBase(invBase) {
			};

			const Vector3& min()					const { return mMin; };
			const Vector3& max()					const { return mMax; };
			bool		   nullopt()				const { return mNullOpt; };

			Vector3 toObSpace(const Vector3& p)		const { return mInvBase * p; };

			void setNullopt()
			{
				mNullOpt = true;
				mMin = Vector3(_MAGIC_FLAG);
				mMax = Vector3(_MAGIC_FLAG);
			}

			void setInvBase(const Matrix3& invBase)
			{
				assert(mNullOpt);
				mInvBase = Quaternion(invBase);
			}

			void setOrthoBox(const Quaternion& invBase, const Vector3& min, const Vector3& max)
			{
				assert(mNullOpt);
				mInvBase = invBase;
				mMin = min;
				mMax = max;
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
				return _SquareDistPointToAABB(isObSpace ? p : toObSpace(p), mMin, mMax);
			}

			std::array<Vector3, 8> getPosArray()const
			{
				const Quaternion& orthoBase = mInvBase.Inverse();
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
			void getGenSpace(_GenSpace& space) const
			{
				const Quaternion& orthoBase = mInvBase.Inverse();
				space.position = orthoBase * ((mMin + mMax) * 0.5f);
				space.rotation = orthoBase;
				space.halfSize = (mMax - mMin) * 0.5f;
			}
		private:
			bool			mNullOpt = true;
			Vector3 mMin = Vector3(_MAGIC_FLAG);
			Vector3 mMax = Vector3(_MAGIC_FLAG);
			Quaternion mInvBase = Quaternion::IDENTITY;
		private:
			inline static const float _MAGIC_FLAG = 9999.f;
		};

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
		using _NodeID = NodeID;
		NodeID NodeID::GetChildID(uint32 i) const
		{
			return NodeID(face, (index + 1) * 4u + i);
		}
		bool NodeID::GetParentID(NodeID& parentID) const
		{
			if (index < 4) {
				return false;
			}
			else {
				parentID.face = face;
				parentID.index = (uint32)std::floor((uint32)index / 4u) - 1u;
				return true;
			}
		}
		std::array<uint32, 10> _NodeIDCalLevelMaxs() {
			std::array<uint32, 10> maxs;
			maxs[0] = 0;
			for (int i = 1; i < maxs.size(); ++i) {
				maxs[i] = (maxs[i - 1] + 1u) * 4u;
			}
			return maxs;
		}
		static std::array<uint32, 10> _gNodeIdLevelMaxs = _NodeIDCalLevelMaxs();
		uint32 NodeID::GetLevel() const {
			uint32 curId = index;
			uint32 maxLevel = (uint32)_gNodeIdLevelMaxs.size();
			for (uint32 i = 0; i != maxLevel; ++i) {
				if (curId < _gNodeIdLevelMaxs[i]) {
					return i;
				}
			}
			return 10;
		}
		int NodeID::GetLevelIndex(uint32 level_start) const {
			uint32 curId = index;
			int res = -1;
			uint32 maxLevel = (uint32)_gNodeIdLevelMaxs.size();
			for (uint32 i = level_start; i < maxLevel; ++i) {
				if (curId < _gNodeIdLevelMaxs[i]) {
					res = i - level_start;
				}
				else {
					break;
				}
			}
			return res;
		}
		bool NodeID::IsLessLevel(uint32 level) const {
			uint32 maxLevel = (uint32)_gNodeIdLevelMaxs.size();
			if (level >= maxLevel) {
				return true;
			}
			else if (level < 2) {
				return false;
			}
			else {
				return (((uint32)index) < _gNodeIdLevelMaxs[level - 1]);
			}
		}

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

		class _QuadTreeNode
		{
		public:
			static void FindVis(const _NodeID& nodeID, const Camera* pCamera, PlanetGenObjectSchedulerPrivate* _dp, const FloatRect& curRange, uint32 level = 0);
			static void Collect(const _NodeID& nodeID, PlanetGenObjectSchedulerPrivate* _dp, const FloatRect& curRange, uint32 level, uint32 rootId);
		};

		class _QuadTree
		{
		public:
			static void FindVis(uint32 faceIndex, const Camera* pCamera, PlanetGenObjectSchedulerPrivate* _dp);
			static void Collect(uint32 faceIndex, PlanetGenObjectSchedulerPrivate* _dp);
		};

		//	======================================================================

		class _BoxPool {
		public:
			_BoxPool() {}

			void unload() {
				mPool.clear();
				mVisSpaces.clear();
				mLastVisSpaces.clear();
				mVisBoxs.clear();
				mCollectSpaces.clear();
			}

			const _OrthoBox& get(const _NodeID& id, const FloatRect& range);
			void update(std::map<int, _GenSpaceList>& genSpaceListMap);
			int getLevelIndex(uint32 id);
			void CalGenSpace(const FloatRect& range, uint32 id, _GenSpace& genSpace);

			void collect(std::vector<std::map<int, _GenSpaceMap>>& genList);

			std::map<uint32, _OrthoBox> mPool;
			std::set<uint32> mVisSpaces;
			std::set<uint32> mLastVisSpaces;
			std::set<uint32> mVisBoxs;
			std::map<uint32, FloatRect> mVisSpaceBoxs;
			PlanetGenObjectSchedulerPrivate* _dp = nullptr;
			std::map<uint32, std::map<int, std::unordered_set<uint32>>> mCollectSpaces;
		};
		
		//	======================================================================
		class PlanetGenObjectSchedulerPrivate
		{
		public:
			PlanetGenObjectSchedulerPrivate() {
			}
			~PlanetGenObjectSchedulerPrivate() {
				shutdown();
			}

			float getRadius()const {
				return mInfo.sphRadius;
			}
			void init()
			{
				shutdown();

				if (!GenObjectScheduler::GetIsUse()) return;

				if (mInfo.sphPtr->isSphere())
				{
					mFaceCount = 6u;
					const float R = getRadius();
					mFaceSize = 2.f * R;
				}
				else if (mInfo.sphPtr->isTorus())
				{
					const float r = mInfo.sphPtr->m_relativeMinorRadius;
					const uint32 faceNum = (uint32)std::round(std::clamp(1.0f / r, 1.0f, 32.0f));
					mFaceSize = (Math::TWO_PI * getRadius()) / faceNum;
					mFaceCount = faceNum;
				}

				{
					mMaxDepth = 0;
					float rootBoxSize = mFaceSize;
					uint32 level = 0;
					while (rootBoxSize >= 256.f) {
						mMaxDepth = level;
						rootBoxSize *= 0.5f;
						++level;
					}
				}
				if (mMaxDepth < 1) {
					return;
				}
				else if (mMaxDepth < 3) {
					mLevelNumber = mMaxDepth;
					mMinDepth = 1;
					mMinDepthIndex = 3 - mLevelNumber;
				}
				else {
					mMinDepth = mMaxDepth - 2;
					mLevelNumber = 3;
					mMinDepthIndex = 0;
				}

				mRenderDistance.resize(mLevelNumber);
				mRenderDistance[mLevelNumber - 1] = GenObjectScheduler::GetMinRenderDistance();
				for (int i = mLevelNumber - 2; i >= 0; --i) {
					mRenderDistance[i] = mRenderDistance[i + 1] * GenObjectScheduler::GetLevelStepMul();
				}

				mLoadDisSqu.resize(mLevelNumber);
				for (uint32 i = 0; i != mLevelNumber; ++i) {
					mLoadDisSqu[i] = mRenderDistance[i] * GenObjectScheduler::GetLoadMul() + GenObjectScheduler::GetLoadAdd();
				}

				mUnloadDisSqu.resize(mLevelNumber);
				for (uint32 i = 0; i != mLevelNumber; ++i) {
					mUnloadDisSqu[i] = mRenderDistance[i] * GenObjectScheduler::GetUnloadMul() + GenObjectScheduler::GetUnloadAdd();
				}

				m_NearEarthHeightSqr = mInfo.maxRealRadius + mUnloadDisSqu[0];
				m_NearEarthHeightSqr = Math::Floor(m_NearEarthHeightSqr * m_NearEarthHeightSqr);
				m_fUpdateDistanceSqu = GenObjectScheduler::GetMinRenderDistance() * 0.08f;
				m_fUpdateDistanceSqu = Math::Floor(m_fUpdateDistanceSqu * m_fUpdateDistanceSqu);

				for (uint32 i = 0; i < mLevelNumber; ++i) {
					mLoadDisSqu[i] = mLoadDisSqu[i] * mLoadDisSqu[i];
					mUnloadDisSqu[i] = mUnloadDisSqu[i] * mUnloadDisSqu[i];
				}
				mSphSpaceCamPos = Vector3::MIN;
				m_bIsNearby = false;

				mBoxCachePool = std::make_unique<_BoxPool>();
				mBoxCachePool->_dp = this;
				bInitialization = true;
			}

			void shutdown()
			{
				mBoxCachePool.reset();
				bInitialization = false;
			}
			void clear()
			{
				if (mBoxCachePool) {
					mBoxCachePool->unload();
				}
				mSphSpaceCamPos = Vector3::MIN;
			}
			float getLoadDisSqu(uint32 level) {
				if (level < mMinDepth) {
					return mLoadDisSqu[0];
				}
				uint32 index = level - mMinDepth;
				if (index >= mLevelNumber) {
					return mLoadDisSqu[mLevelNumber - 1];
				}
				return mLoadDisSqu[index];
			}
			float getUnloadDisSqu(uint32 level) {
				if (level < mMinDepth) {
					return mUnloadDisSqu[0];
				}
				uint32 index = level - mMinDepth;
				if (index >= mLevelNumber) {
					return mUnloadDisSqu[mLevelNumber - 1];
				}
				return mUnloadDisSqu[index];
			}

			_CreateInfo mInfo;
			bool bInitialization = false;
			uint32	mFaceCount = 0u;
			float	mFaceSize = 0.f;
			uint32 mMinDepth = 4;
			uint32 mMaxDepth = 6;
			int mMinDepthIndex = 0;
			uint32 mLevelNumber = 3;
			std::vector<float> mLoadDisSqu;
			std::vector<float> mUnloadDisSqu;
			std::vector<float> mRenderDistance;
			std::unique_ptr<_BoxPool> mBoxCachePool;
			Vector3 mSphSpaceCamPos = Vector3::ZERO;

			bool update(const Camera* pCamera, std::map<int, _GenSpaceList>& genSpaceListMap) {
				if (!bInitialization) return false;
				DVector3 worldPos = pCamera->getDerivedPosition();
				Vector3 localPos = mInfo.sphRotInv * (worldPos - mInfo.sphPos) / mInfo.sphScl;
				bool isNearby = localPos.squaredLength() < m_NearEarthHeightSqr;
				bool bChanged = false;
				if (m_bIsNearby != isNearby) {
					if (m_bIsNearby) {
						clear();
						bChanged = true;
					}
					m_bIsNearby = isNearby;
				}
				if (!m_bIsNearby) {
					return bChanged;
				}
				if ((localPos - mSphSpaceCamPos).LengthSq() < m_fUpdateDistanceSqu) {
					return false;
				}
				mSphSpaceCamPos = localPos;

				for (uint32 faceIndex = 0u; faceIndex < mFaceCount; ++faceIndex) {
					_QuadTree::FindVis(faceIndex, pCamera, this);
				}
				mBoxCachePool->update(genSpaceListMap);
				return true;
			}

			bool m_bIsNearby = false;
			float m_fUpdateDistanceSqu = 6000.0f;
			float m_NearEarthHeightSqr = FLT_MAX;

			bool collectGenSpaceList(std::vector<std::map<int, _GenSpaceMap>>& genList) {
				if (mBoxCachePool == nullptr) {
					return false;
				}
				mBoxCachePool->unload();
				for (uint32 faceIndex = 0u; faceIndex < mFaceCount; ++faceIndex) {
					_QuadTree::Collect(faceIndex, this);
				}
				mBoxCachePool->collect(genList);
				mBoxCachePool->unload();
				return true;
			}
		};
		//	======================================================================

		bool _CheckVisible(const Camera* pCamera, const AxisAlignedBox& worldBox, float showDis);
	}

	namespace PCG
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


		inline Vector3 _FacePosToRingNor(uint32 type, ProceduralSphere* sphPtr, const Vector2& pos, float faceSize, float offset)
		{
			// To cur face pos [-r, r] to [0, 2r]
			const Vector2 facePos = pos + Vector2(faceSize / 2.f);
			// To all face pos
			const Vector2 allFacePos = { facePos.x + type * faceSize, facePos.y };
			// To radian pos
			const Vector2 allFaceRadianPos = allFacePos / Vector2(sphPtr->m_radius, sphPtr->m_radius * sphPtr->m_relativeMinorRadius);

			return RingSpaceToNor(sphPtr->m_radius, sphPtr->m_relativeMinorRadius, allFaceRadianPos, offset);
		}

		inline std::array<Vector3, 4> _FacePosToRingNor(uint32 type, ProceduralSphere* sphPtr, const std::array<Vector2, 4>& pos, float faceSize, float offset)
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

		bool _CheckVisible(const Camera* pCamera, const AxisAlignedBox& worldBox, float showDis)
		{
			const float squaDis = _SquareDistPointToAABB(pCamera->getPosition(), worldBox);
			if (squaDis >= showDis * showDis)
				return false;
			if (!pCamera->isVisible(worldBox))
				return false;
			return true;
		}

		//	======================================================================
		void _QuadTreeNode::FindVis(const _NodeID& nodeID, const Camera* pCamera, PlanetGenObjectSchedulerPrivate* _dp, const FloatRect& nodeRange, uint32 level)
		{

			const _OrthoBox& nodeBox = _dp->mBoxCachePool->get(nodeID, nodeRange);
			const float camToBoxNearSquareDis = nodeBox.getDisSqrt(_dp->mSphSpaceCamPos, false);

			if (camToBoxNearSquareDis >= _dp->getLoadDisSqu(level)) {
				_dp->mBoxCachePool->mVisBoxs.insert(nodeID._id);
				return;
			}

			if (level >= _dp->mMinDepth) {
				if (_dp->mInfo.sphPtr->isSphere())
				{
					_dp->mBoxCachePool->mVisSpaceBoxs[nodeID._id] = nodeRange;
				}
				_dp->mBoxCachePool->mVisSpaces.insert(nodeID._id);
			}
			else {
				_dp->mBoxCachePool->mVisBoxs.insert(nodeID._id);
			}
			if (level < _dp->mMaxDepth)
			{
				for (uint32 i = 0; i < 4; i++)
					FindVis(nodeID.GetChildID(i), pCamera, _dp, _GetChildRect(_NodeType(i), nodeRange), level + 1);
			}
		}
		void _QuadTreeNode::Collect(const _NodeID& nodeID, PlanetGenObjectSchedulerPrivate* _dp, const FloatRect& nodeRange, uint32 level, uint32 rootId)
		{
			_dp->mBoxCachePool->get(nodeID, nodeRange);

			if (level == _dp->mMinDepth) {
				rootId = nodeID._id;
			}
			
			if (level >= _dp->mMinDepth) {
				if (_dp->mInfo.sphPtr->isSphere())
				{
					_dp->mBoxCachePool->mVisSpaceBoxs[nodeID._id] = nodeRange;
				}
				int levelIndex = (int)(level - _dp->mMinDepth) + _dp->mMinDepthIndex;
				_dp->mBoxCachePool->mCollectSpaces[rootId][levelIndex].insert(nodeID._id);
			}
			if (level < _dp->mMaxDepth)
			{
				for (uint32 i = 0; i < 4; i++)
					Collect(nodeID.GetChildID(i), _dp, _GetChildRect(_NodeType(i), nodeRange), level + 1, rootId);
			}
		}

		void _QuadTree::FindVis(uint32 faceIndex, const Camera* pCamera, PlanetGenObjectSchedulerPrivate* _dp)
		{
			const float halfSize = _dp->mFaceSize / 2.f;
			const FloatRect rectRange(-halfSize, -halfSize, halfSize, halfSize);

			for (uint32 i = 0; i < 4; i++)
			{
				static _NodeID curChildID;
				curChildID.face = faceIndex;
				curChildID.index = i;

				static FloatRect curRect(0.f, 0.f, 0.f, 0.f);
				_GetChildRect(_NodeType(i), rectRange, curRect);
				_QuadTreeNode::FindVis(curChildID, pCamera, _dp, curRect, 1);
			}
		}
		void _QuadTree::Collect(uint32 faceIndex, PlanetGenObjectSchedulerPrivate* _dp) {
			const float halfSize = _dp->mFaceSize / 2.f;
			const FloatRect rectRange(-halfSize, -halfSize, halfSize, halfSize);

			for (uint32 i = 0; i < 4; i++)
			{
				static _NodeID curChildID;
				curChildID.face = faceIndex;
				curChildID.index = i;

				static FloatRect curRect(0.f, 0.f, 0.f, 0.f);
				_GetChildRect(_NodeType(i), rectRange, curRect);
				_QuadTreeNode::Collect(curChildID, _dp, curRect, 1, 0);
			}
		}

		const _OrthoBox& _BoxPool::get(const _NodeID& id, const FloatRect& range)
		{
			_OrthoBox& box = mPool[id._id];

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

		void _BoxPool::update(std::map<int, _GenSpaceList>& genSpaceListMap) {
			for (auto&& spaceId : mVisSpaces) {
				int levelIndex = getLevelIndex(spaceId);
				auto it = mLastVisSpaces.find(spaceId);
				if (it != mLastVisSpaces.end()) {
					genSpaceListMap[levelIndex].keepSpaces.insert(spaceId);
					mLastVisSpaces.erase(it);
				}
				else {
					_NodeID nodeId(spaceId);
					_GenSpace& genSpace = genSpaceListMap[levelIndex].newSpaces[spaceId];
					auto gsIter = mVisSpaceBoxs.find(spaceId);
					if (gsIter == mVisSpaceBoxs.end()) {
						auto& box = mPool[spaceId];
						box.getGenSpace(genSpace);
					}
					else {
						CalGenSpace(gsIter->second, gsIter->first, genSpace);
					}
				}
			}
			auto getParentExist = [&](uint32 spaceId)->bool {
				_NodeID nodeId(spaceId);
				_NodeID parentId;
				if (!nodeId.GetParentID(parentId)) {
					return false;
				}
				int levelIndex = getLevelIndex(parentId._id);
				if (levelIndex < 0) {
					return false;
				}
				auto& genSpaceList = genSpaceListMap[levelIndex];
				return genSpaceList.keepSpaces.contains(parentId._id) || genSpaceList.newSpaces.contains(parentId._id);
			};
			for (auto&& spaceId : mLastVisSpaces) {
				if (getParentExist(spaceId)) {
					auto& box = mPool[spaceId];
					float toCamDisSqu = box.getDisSqrt(_dp->mSphSpaceCamPos, false);
					int levelIndex = getLevelIndex(spaceId);
					if (toCamDisSqu < _dp->mUnloadDisSqu[levelIndex]) {
						genSpaceListMap[levelIndex].keepSpaces.insert(spaceId);
						mVisSpaces.insert(spaceId);
						continue;
					}
				}
			}
			mLastVisSpaces.clear();
			mLastVisSpaces.swap(mVisSpaces);
			for (auto it = mPool.begin(); it != mPool.end(); )
			{
				_NodeID nodeId(it->first);
				if (nodeId.IsLessLevel(_dp->mMinDepth))
				{
					if (mVisBoxs.contains(it->first)) {
						it++;
					}
					else {
						it = mPool.erase(it);
					}
				}
				else {
					if (mLastVisSpaces.contains(it->first)) {
						it++;
					}
					else {
						it = mPool.erase(it);
					}
				}
			}
			mVisBoxs.clear();
			mVisSpaceBoxs.clear();
		}

		void _BoxPool::collect(std::vector<std::map<int, _GenSpaceMap>>& genList) {
			genList.reserve(mCollectSpaces.size());
			for (const auto& iter1 : mCollectSpaces) {
				auto& curGenSpaceMap = genList.emplace_back();
				for (const auto& iter2 : iter1.second) {
					_GenSpaceMap& curGsM = curGenSpaceMap[iter2.first];
					for (uint32 spaceId : iter2.second) {
						_NodeID nodeId(spaceId);
						_GenSpace& genSpace = curGsM[spaceId];
						auto gsIter = mVisSpaceBoxs.find(spaceId);
						if (gsIter == mVisSpaceBoxs.end()) {
							auto& box = mPool[spaceId];
							box.getGenSpace(genSpace);
						}
						else {
							CalGenSpace(gsIter->second, gsIter->first, genSpace);
						}
					}
				}
			}
		}

		int _BoxPool::getLevelIndex(uint32 id) {
			_NodeID nodeId(id);
			uint32 level = nodeId.GetLevel();
			if (level < _dp->mMinDepth) {
				return -1;
			}
			return (int)(level - _dp->mMinDepth) + _dp->mMinDepthIndex;
		}
		void _BoxPool::CalGenSpace(const FloatRect& range, uint32 id, _GenSpace& genSpace) {
			static std::array<Quaternion, 6> _gRotation = {
				Quaternion(Vector3(0, 0, -1),Vector3(1, 0, 0), Vector3(0, -1, 0)),
				Quaternion(Vector3(0, 0, 1), Vector3(-1, 0, 0),Vector3(0, -1, 0)),
				Quaternion(Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)),
				Quaternion(Vector3(1, 0, 0), Vector3(0, -1, 0),Vector3(0, 0, -1)),
				Quaternion(Vector3(1, 0, 0), Vector3(0, 0, 1), Vector3(0, -1, 0)),
				Quaternion(Vector3(-1, 0, 0),Vector3(0, 0, -1),Vector3(0, -1, 0))
			};
			const std::array<Vector2, 4> rectPos = _FloatRectToPos(range);
			Vector2 _max = rectPos[0], _min = rectPos[0];
			for (int i = 1; i < 4; ++i) {
				_max.makeCeil(rectPos[i]);
				_min.makeFloor(rectPos[i]);
			}
			Vector2 center = (_min + _max) * 0.5f;
			Vector2 halfSize = (_max - _min) * 0.5f;
			_NodeID nodeId(id);
			genSpace.rotation = _gRotation[nodeId.face];
			genSpace.position = _FacePosToCube(nodeId.face, _dp->getRadius(), center);
			genSpace.halfSize = Vector3(halfSize.x, _dp->mInfo.sphTerMaxOff, halfSize.y);
		}
		//	======================================================================
	}

	namespace PCG {

		PlanetGenObjectScheduler::PlanetGenObjectScheduler(SphericalTerrain* pSph) : m_pSphericalTerrain(pSph) {
			m_pPrivate = new PlanetGenObjectSchedulerPrivate();
		}
		PlanetGenObjectScheduler::~PlanetGenObjectScheduler() {
			SAFE_DELETE(m_pPrivate);
		}
		void PlanetGenObjectScheduler::Init(const CreateInfo& info, const PlanetGenObject& _PlanetGenObject) {
			m_pPrivate->mInfo = info;
			m_pPrivate->init();
			mPlanetGenObject = _PlanetGenObject;
			std::vector<OrientedBox> _PlanetGenObjAvoidArea = m_pSphericalTerrain->getFlatAreas();
			mPlanetStaticAvoidArea.resize(_PlanetGenObjAvoidArea.size());
			int areaSize = (int)_PlanetGenObjAvoidArea.size();
			for (int areaIndex = 0; areaIndex != areaSize; ++areaIndex) {
				mPlanetStaticAvoidArea[areaIndex].first = _PlanetGenObjAvoidArea[areaIndex];
				mPlanetStaticAvoidArea[areaIndex].second = mPlanetStaticAvoidArea[areaIndex].first.extent.LengthSq();
			}
			initImpl();
		}
		void PlanetGenObjectScheduler::setWorldTrans(const DTransform& trans, const Vector3& scale) {
			if (m_pPrivate->mInfo.sphPos.isEqual(trans.translation, 0.001) && m_pPrivate->mInfo.sphRot.isEqual(trans.rotation, 0.001f) && m_pPrivate->mInfo.sphScl.isEqual(scale, 0.001f)) return;
			clearGenObjects();
			m_pPrivate->mInfo.sphPos = trans.translation;
			m_pPrivate->mInfo.sphRot = trans.rotation;
			m_pPrivate->mInfo.sphRotInv = trans.rotation.Inverse();
			m_pPrivate->mInfo.sphScl = scale;
			m_pPrivate->init();
			initImpl();
		}
		void PlanetGenObjectScheduler::findVisibleObjects(const Camera* pCamera) {
			std::map<int, GenSpaceList> genSpaceListMap;
			if (m_pPrivate->update(pCamera, genSpaceListMap)) {
				updateGenSpace(genSpaceListMap);
			}
		}
		bool ExportDataParse(const std::string& file, const char* folderType, const std::map<Name, std::vector<std::tuple<Vector3, Quaternion, Vector3, uint64>>>& allObjects) {
			auto _GetPhysXMeshPath = [](const Name& file) ->std::string {
				String phyMeshName = file.toString() + "phy";

				std::replace(phyMeshName.begin(), phyMeshName.end(), '\\', '/');
				std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');

				phyMeshName.insert(0, "collision/mesh/");
				return phyMeshName;
				};

			DataStreamPtr stream(Root::instance()->CreateDataStream(file.c_str(), folderType));
			if (stream.isNull()) return false;

			std::map<Name, uint32> meshToCustomTypeMap;
			{
				for (const auto& iter : BiomeGenObjectLibraryManager::instance()->mLibraryMap) {
					meshToCustomTypeMap[iter.second.mesh] = iter.second.custom_type;
				}
			}

			uint32 version = 0;
			stream->writeData(&version, 1);

			uint32 objSize = (uint32)allObjects.size();
			stream->writeData(&objSize, 1);


			for (const auto& objPair : allObjects) {
				std::string objFileStr = _GetPhysXMeshPath(objPair.first);
				stream->writeString(objFileStr);

				uint32 custom_enum = meshToCustomTypeMap[objPair.first];
				stream->writeData(&custom_enum, 1);

				uint32 pointSize = (uint32)objPair.second.size();
				stream->writeData(&pointSize, 1);

				for (uint32 pointIndex = 0; pointIndex != pointSize; ++pointIndex) {
					auto& tp = objPair.second[pointIndex];
					stream->writeData(&std::get<0>(tp), 1);
					stream->writeData(&std::get<1>(tp), 1);
					stream->writeData(&std::get<2>(tp), 1);
					stream->writeData(&std::get<3>(tp), 1);
				}
			}
			stream->close();

			return true;
		}

		bool PlanetGenObjectScheduler::ExportGenObjects(const String& file, const char* folderType) {
			std::map<Name, std::vector<ExportPoint>> allObjects;
			if (!ExportGenObjects(allObjects)) {
				return false;
			}
			return ExportDataParse(file, folderType, allObjects);
		}
		bool PlanetGenObjectScheduler::ExportGenObjects(std::map<Name, std::vector<ExportPoint>>& allObjects) {
			clearGenObjects();
			std::vector<std::map<int, _GenSpaceMap>> _AllGenList;
			if (!m_pPrivate->collectGenSpaceList(_AllGenList)) return false;
			size_t genSize = _AllGenList.size();
			for (size_t genIndex = 0; genIndex != genSize; ++genIndex) {
				std::map<Name, std::vector<std::tuple<Vector3, Quaternion, Vector3, uint64>>> objects;
				ExportGenObjects(_AllGenList[genIndex], objects);
				for (auto& objPair : objects) {
					auto& allObjectPair = allObjects[objPair.first];
					for (auto& obj : objPair.second) {
						std::get<0>(obj) = m_pPrivate->mInfo.sphRotInv * (std::get<0>(obj) - m_pPrivate->mInfo.sphPos) / m_pPrivate->mInfo.sphScl;
						std::get<1>(obj) = m_pPrivate->mInfo.sphRotInv * std::get<1>(obj);
					}
					allObjectPair.insert(allObjectPair.end(), objPair.second.begin(), objPair.second.end());
				}
			}
			return true;
		}
		void PlanetGenObjectScheduler::clearGenObjects() {
			m_pPrivate->clear();
			updateGenSpace({});
		}
		void PlanetGenObjectScheduler::resetGenObjects() {
			clearGenObjects();
			m_pPrivate->init();
		}
		float PlanetGenObjectScheduler::getRenderDistance(int level) const {
			return m_pPrivate->mRenderDistance[level];
		}
		uint32 PlanetGenObjectScheduler::getRandomSeed(uint32 id) const {
			return m_pPrivate->mInfo.randomSeed + id;
		}
		const Name& PlanetGenObjectScheduler::getName() {
			return m_pSphericalTerrain->m_terrainData->getName();
		}
		const DVector3& PlanetGenObjectScheduler::getPosition() const {
			return m_pPrivate->mInfo.sphPos;
		}
		const Quaternion& PlanetGenObjectScheduler::getRotation() const {
			return m_pPrivate->mInfo.sphRot;
		}
		const Vector3& PlanetGenObjectScheduler::getScale() const {
			return m_pPrivate->mInfo.sphScl;
		}
		const Name& PlanetGenObjectScheduler::getSceneName() const {
			return m_pPrivate->mInfo.sceneName;
		}
		Vector3 PlanetGenObjectScheduler::convertWorldToLocalPosition(const DVector3& pos) const {
			return getRotation().Inverse() * (pos - getPosition()) / getScale();
		}
		Quaternion PlanetGenObjectScheduler::convertWorldToLocalRotation(const Quaternion& rot) const {
			return getRotation().Inverse() * rot;
		}
		DVector3 PlanetGenObjectScheduler::convertLocalToWorldPosition(const Vector3& pos) const {
			return (getRotation() * (pos * getScale())) + getPosition();
		}
		Quaternion PlanetGenObjectScheduler::convertLocalToWorldRotation(const Quaternion& rot) const {
			return getRotation() * rot;
		}

		bool PlanetGenObjectScheduler::existStaticArea() const {
			return (m_pSphericalTerrain->getPlanetRoad() != nullptr) || existStaticAvoidArea();
		}
		DVector3 PlanetGenObjectScheduler::getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal) const {
			return m_pSphericalTerrain->getSurfaceFinestWs(worldPos, faceNormal);
		}
		int PlanetGenObjectScheduler::getSurfaceTypeWs(const DVector3& worldPos, bool* underwater) const {
			return m_pSphericalTerrain->getSurfaceTypeWs(worldPos, underwater);
		}
		bool PlanetGenObjectScheduler::isUnderOcean(const DVector3& worldPos, double* depth) const {
			return m_pSphericalTerrain->isUnderOcean(worldPos, depth);
		}
		Vector3 PlanetGenObjectScheduler::getGravityWs(const Vector3& worldPos) const {
			return m_pSphericalTerrain->getGravityWs(worldPos);
		}
		class PlanetRoad* PlanetGenObjectScheduler::getPlanetRoad() const {
			return m_pSphericalTerrain->getPlanetRoad();
		}

		bool PlanetGenObjectScheduler::getStaticAvoidArea(const AxisAlignedBox& bound, std::vector<OrientedBox>& obbs) {
			if (mPlanetStaticAvoidArea.empty()) return false;
			for (auto& obb : mPlanetStaticAvoidArea) {
				if (bound.squaredDistance(obb.first.center) > obb.second) continue;
				if (!obb.first.Intersect(bound)) continue;
				obbs.push_back(obb.first);
			}
			return true;
		}
		bool PlanetGenObjectScheduler::getStaticAvoidArea(const OrientedBox& bound, std::vector<OrientedBox>& obbs) {
			if (mPlanetStaticAvoidArea.empty()) return false;
			for (auto& obb : mPlanetStaticAvoidArea) {
				if (bound.DistanceSqr(obb.first.center) > obb.second) continue;
				if (!obb.first.Intersect(bound)) continue;
				obbs.push_back(obb.first);
			}
			return true;
		}
	}

}
