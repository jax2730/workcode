#include "EchoComponentStableHeaders.h"
#include "EchoShaderContentManager.h"
#include "EchoSubMesh.h"
#include "EchoShadowmapManager.h"
#include "EchoEntityBatchOptions.h"
#include "EchoInstanceStaticBatchObject.h"
#include "EchoInstanceMaterialListener.h"
#include "EchoMaterialManager.h"
#include "EchoTerrainSystem.h"

#include "EchoInstanceStaticBatchEntity.h"
#include "EchoPavementUtil.h"
#include "EchoPointLightShadowManager.h"
#include "EchoMaterialV2Manager.h"
#include "EchoTRectF.h"

namespace Echo
{
	namespace
	{
		float _SquareDistPointToAABB_Near(const Vector3& vPos, const AxisAlignedBox& aabb)
		{
			const Vector3& aabbMin = aabb.getMinimum();
			const Vector3& aabbMax = aabb.getMaximum();

			if (vPos.x >= aabbMin.x && vPos.x <= aabbMax.x)
				if (vPos.y >= aabbMin.y && vPos.y <= aabbMax.y)
					if (vPos.z >= aabbMin.z && vPos.z <= aabbMax.z)
						return 0; // inside

			float dy;
			if (vPos.y < aabbMin.y)
				dy = aabbMin.y - vPos.y;
			else if (vPos.y > aabbMax.y)
				dy = vPos.y - aabbMax.y;
			else
				dy = 0;

			float dx;
			if (vPos.x < aabbMin.x)
				dx = aabbMin.x - vPos.x;
			else if (vPos.x > aabbMax.x)
				dx = vPos.x - aabbMax.x;
			else
				dx = 0;

			float dz;
			if (vPos.z < aabbMin.z)
				dz = aabbMin.z - vPos.z;
			else if (vPos.z > aabbMax.z)
				dz = vPos.z - aabbMax.z;
			else
				dz = 0;

			return dx * dx + dy * dy + dz * dz;
		}

		float _SquareDistPointToAABB_Far(const Vector3& vPos, const AxisAlignedBox& aabb)
		{
			const Vector3& aabbMin = aabb.getMinimum();
			const Vector3& aabbMax = aabb.getMaximum();

			float dx0 = fabs(vPos.x - aabbMin.x);
			float dx1 = fabs(vPos.x - aabbMax.x);
			float dx = dx0 > dx1 ? dx0 : dx1;


			float dy0 = fabs(vPos.y - aabbMin.y);
			float dy1 = fabs(vPos.y - aabbMax.y);
			float dy = dy0 > dy1 ? dy0 : dy1;

			float dz0 = fabs(vPos.z - aabbMin.z);
			float dz1 = fabs(vPos.z - aabbMax.z);
			float dz = dz0 > dz1 ? dz0 : dz1;

			return dx * dx + dy * dy + dz * dz;
		}
		float _SquareDistPointToAABB(const Vector3& vPos, const AxisAlignedBox& aabb)
		{
			const Vector3& aabbMin = aabb.getMinimum();
			const Vector3& aabbMax = aabb.getMaximum();

			if (vPos.x >= aabbMin.x && vPos.x <= aabbMax.x)
				if (vPos.y >= aabbMin.y && vPos.y <= aabbMax.y)
					if (vPos.z >= aabbMin.z && vPos.z <= aabbMax.z)
						return 0; // inside

			float dy;
			if (vPos.y < aabbMin.y)
				dy = aabbMin.y - vPos.y;
			else if (vPos.y > aabbMax.y)
				dy = vPos.y - aabbMax.y;
			else
				dy = 0;

			float dx;
			if (vPos.x < aabbMin.x)
				dx = aabbMin.x - vPos.x;
			else if (vPos.x > aabbMax.x)
				dx = vPos.x - aabbMax.x;
			else
				dx = 0;

			float dz;
			if (vPos.z < aabbMin.z)
				dz = aabbMin.z - vPos.z;
			else if (vPos.z > aabbMax.z)
				dz = vPos.z - aabbMax.z;
			else
				dz = 0;

			return dx * dx + dy * dy + dz * dz;
		}

		enum InstanceType
		{
			eIT_None,
			eIT_Illum,
			eIT_IllumPBR,
			eIT_IllumPBR2022,
			eIT_IllumPBR2023,
			eIT_SpecialIllumPBR,
			eIT_BillboardLeaf,
			eIT_BillboardLeafPBR,
			eIT_Trunk,
			eIT_TrunkPBR,
			eIT_SimpleTreePBR,
			eIT_BaseWhiteNoLight,

			eIT_MaxCount,
		};

		enum InstanceDataType
		{
			eIDD_WorldTransform,
		};

		union uniform_f4
		{
			struct
			{
				float x, y, z, w;
			};
			struct
			{
				float r, g, b, a;
			};
		};

		using Uniformf4Vec = std::vector<uniform_f4>;

		void uniform_f4_assign_pos(uniform_f4& uniform, const Vector3& pos)
		{
			uniform.x = pos.x;
			uniform.y = pos.y;
			uniform.z = pos.z;
		}

		void uniform_f4_assign_scale(uniform_f4& uniform, const Vector3& fScale)
		{
			uniform.x = fScale.x;
			uniform.y = fScale.y;
			uniform.z = fScale.z;
		}

		void uniform_f4_assign_orientation(uniform_f4& uniform, const Quaternion& qua)
		{
			uniform.x = qua.x;
			uniform.y = qua.y;
			uniform.z = qua.z;
			uniform.w = qua.w;
		}

		void updateWorldPositionScaleOrientation(const SubMesh* pSubMesh,
			const ObjectInstanceData& data,
			const Vector3& offset, const Vector3& scale, const Quaternion& ori,
			Uniformf4Vec& vecData, size_t pos_start/*const SubEntityVec& vecInst, uint32 iUniformCount,
			Uniformf4Vec& vecData*/)
		{
			(void)pSubMesh;
			assert(pos_start < vecData.size());
			DBMatrix4 postran;
			postran.makeTransform(offset, scale, ori);
			uniform_f4_assign_pos(vecData[pos_start], postran * data.position);
			uniform_f4_assign_scale(vecData[pos_start + 1], data.scale);
			uniform_f4_assign_orientation(vecData[pos_start + 2], ori * data.orientation);
		}

		void InstanceData_WorldTransform(const SubMesh* pSubMesh,
			const ObjectInstanceData& data,
			const Vector3& offset, const Vector3& scale, const Quaternion& ori,
			Uniformf4Vec& vecData, size_t pos_start)
		{
			(void)pSubMesh;
			assert(pos_start < vecData.size());

			DBMatrix4 postran;
			postran.makeTransform(offset, scale, ori);

			DBMatrix4 matrix4, InvMatrix4;

			matrix4.makeTransform(postran * data.position, data.scale, ori * data.orientation);
			InvMatrix4.makeInverseTransform(postran * data.position, data.scale, ori * data.orientation);
			uniform_f4* _inst_buff = &vecData[pos_start];
			for (int i = 0; i < 3; ++i)
			{
				_inst_buff[i].x = (float)matrix4.m[i][0];
				_inst_buff[i].y = (float)matrix4.m[i][1];
				_inst_buff[i].z = (float)matrix4.m[i][2];
				_inst_buff[i].w = (float)matrix4.m[i][3];
			}
			for (int i = 0; i < 3; ++i)
			{
				_inst_buff[i + 3].x = (float)InvMatrix4.m[i][0];
				_inst_buff[i + 3].y = (float)InvMatrix4.m[i][1];
				_inst_buff[i + 3].z = (float)InvMatrix4.m[i][2];
				_inst_buff[i + 3].w = (float)InvMatrix4.m[i][3];
			}
		}

		using InstanceDataSetFunc = void(*)(const SubMesh*,
			const ObjectInstanceData&, const Vector3&, const Vector3&, const Quaternion&,
			Uniformf4Vec&, size_t);

		/** 某种可以合批的 shader 的一些信息，包括一个批次可以支持
			多少个，instance data 的组织方式等等
		*/
		struct InstanceTypeInfo
		{
			InstanceType type = eIT_None;
			// 每个实例的按照 4 个 float（float4 in HLSL, vec4 in OpenGL ES SL）
			// 组织后占用的 uniform 的个数。
			int instance_uniform_count = 0;

			// 保留的 uniform 数量，shader 用来传递一些全局或材质共用的参数
			int rev_uniform_count = 0;

			InstanceDataSetFunc func = nullptr;
		};

		using InstanceTypeInfoMap = std::unordered_map<int, InstanceTypeInfo>;
		using TypeShaderMap = std::unordered_map<int, String>;
		using ShaderTypeMap = std::unordered_map<String, int>;


		template<class T>
		void __merge_to_rect(TRectF<T>& rect, T x, T y)
		{
			if (rect.left > x)		rect.left = x;
			if (rect.right < x) 	rect.right = x;
			if (rect.top > y)		rect.top = y;
			if (rect.bottom < y)	rect.bottom = y;
		}

	} // namespace

	class StaticBatchObjectUtil : public InstanceMaterialListener,
		public Singleton<StaticBatchObjectUtil>
	{
	public:
		InstanceTypeInfoMap mInstanceTypeInfos;
		TypeShaderMap mTypeShaderMap;
		ShaderTypeMap mShaderTypeMap;

		std::map<Name, MaterialWrapper> mMaterialPool;
		std::set<Name> mMaterialWaiting;
		MaterialWrapper mEmptyMaterial;

		///	In DirectX3D 9, there isn't an 'InstanceID' like OpenGL ES 3.0
		/// when draw an object in instance, so we use an extra vertex buffer
		/// as the 'InstanceID'.
		RcBuffer* mBuffInstID = nullptr;
		static const uint32 MaxInstancePerBatch_ = 256;

		void init()
		{
			const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();
			if (eClass == ECHO_DEVICE_DX9C)
			{
				uint8 iInstID[MaxInstancePerBatch_ * 4] = { 0u, };
				for (uint32 i = 0; i < MaxInstancePerBatch_; ++i)
				{
					iInstID[i * 4 + 0] = (uint8)i;
					iInstID[i * 4 + 1] = 0;
					iInstID[i * 4 + 2] = 0;
					iInstID[i * 4 + 3] = 0;
				}
				mBuffInstID = Root::instance()->getRenderSystem()->
					createStaticVB(iInstID, MaxInstancePerBatch_ * 4);
			}
		}

		void destroy()
		{
			if (mBuffInstID != nullptr)
			{
				Root::instance()->getRenderSystem()->destoryBuffer(mBuffInstID);
				mBuffInstID = nullptr;
			}

			mInstanceTypeInfos.clear();
			mTypeShaderMap.clear();
			mShaderTypeMap.clear();
			//mMaterialPool.clear();
			for (auto& v : mMaterialPool)
				v.second.setNull();
		}

		void registerShaderType(InstanceType type, const char* shader,
			int inst_uniform_count, int rev_uniform_count, InstanceDataSetFunc _func)
		{
			InstanceTypeInfo& info = mInstanceTypeInfos[type];
			info.type = type;
			info.instance_uniform_count = inst_uniform_count;
			info.rev_uniform_count = rev_uniform_count;
			info.func = _func;
			mTypeShaderMap[type] = shader;
			mShaderTypeMap[shader] = type;
		};


		InstanceType getInstanceType(const String& sShader) const
		{
			auto it = mShaderTypeMap.find(sShader);
			if (it != mShaderTypeMap.end())
				return (InstanceType)it->second;
			return eIT_None;
		}

		InstanceTypeInfo getInstanceInfo(const String& sShader) const
		{
			return getInstanceInfo(getInstanceType(sShader));
		}

		InstanceTypeInfo getInstanceInfo(InstanceType type) const
		{
			auto it = mInstanceTypeInfos.find(type);
			if (it != mInstanceTypeInfos.end())
				return it->second;

			return InstanceTypeInfo();
		}

		const MaterialWrapper& getMaterial(const Name& nmName)
		{
			const auto& it = mMaterialPool.find(nmName);
			if (it != mMaterialPool.end())
				return it->second;

			return mEmptyMaterial;
		}

		bool isMaterialWaiting(const Name& nmName)
		{
			return mMaterialWaiting.find(nmName) != mMaterialWaiting.end();
		}

		void requestMaterial(const MaterialWrapper& ptrMat)
		{
			if (mMaterialWaiting.find(ptrMat.getName()) == mMaterialWaiting.end())
			{
				// AddRequest TODO
				//InstanceMaterialHelper::instance()->addEntityMatReq(this, ptrMat);
				RoadHelper::instance()->addObjectMatReq(this, ptrMat);
				//assert(false && "Material is not prepare");
				mMaterialWaiting.insert(ptrMat.getName());
				// String sLog;
				// sLog.reserve(256);
				// sLog.append(__FUNCTION__).append(" ").append(ptrMat->getName().c_str());
				// LogManager::instance()->logMessage(LML_NORMAL, sLog);
			}
		}

		virtual void materialLoaded(const MaterialWrapper& ptrMat, uint64 requestID) override
		{
			// String sLog;
			// sLog.reserve(256);
			// sLog.append(__FUNCTION__).append(" ").append(ptrMat->getName().c_str());
			// LogManager::instance()->logMessage(LML_NORMAL, sLog);

			const Name name = ptrMat.getName();
			mMaterialPool[name] = ptrMat;
			mMaterialWaiting.erase(name);
		}

	}; // class StaticBatchEntityUtil

	class InstanceStaticBatchObjectPrivate;

	class InstanceSubStaticBatchObject
	{
		class Render : public Renderable
		{
		public:
			Render()
			{
				enableCustomParameters(true);
				enableInstanceRender(true);
			}

			virtual RenderOperation* getRenderOperation() override
			{
				return &mSubBatch->mRenderOperation;
			}
			// virtual const Material* getMaterial() const override
			// {
			// 	return mSubBatch->mMaterial.getV1();
			// }
			const MaterialWrapper& getMaterialWrapper() const override
			{
				return mSubBatch->mMaterial;
			}
			virtual uint32 getInstanceNum() override
			{
				return mInstNum;
			}
			virtual void setCustomParameters(const Pass* pPass) override
			{
				mSubBatch->setPassParam(this, pPass, mInstIdxStart, mInstNum);
			}
			virtual void getIndexBuffInfo(IndexBuffInfo& indexInfo) override
			{
				indexInfo.m_indexBuffer = mSubBatch->mRenderOperation.GetCurMeshData().m_IndexBufferData.m_indexBuffer;
				indexInfo.m_indexCount = mSubBatch->mRenderOperation.GetCurMeshData().m_IndexBufferData.m_indexCount;
			}

			uint32 mInstIdxStart = 0u;
			uint32 mInstNum = 0u;
			InstanceSubStaticBatchObject* mSubBatch = nullptr;
			SubMesh* mSubMesh = nullptr;
		}; // class Render

		using RenderVec = std::vector<Render*>;

	public:
		InstanceSubStaticBatchObject(InstanceStaticBatchObjectPrivate* parent)
			: mParent(parent)
		{
			mRenderPool.reserve(2);
		}

		~InstanceSubStaticBatchObject()
		{
			//for (int i = LOD_LEVEL_0; i < LOD_LEVEL_NUM; ++i)
			{
				auto& data = mRenderOperation.m_stMeshData;
				data.m_vertexBuffer.clear();
				data.m_vertexCount = data.m_vertexBuffSize = 0;
				data.m_IndexBufferData.m_indexBuffer = nullptr;
				data.m_IndexBufferData.m_indexCount = data.m_indexBufferSize = 0;
			}

			for (auto& it : mRenderPool)
				delete it;
			mRenderPool.clear();
		}

		bool init(SubMesh* pSubMesh, SceneManager* pSceneManager);
		void initSubMeshData();
		bool initCaps(const String& ssShaderName);
		void initMaterial(const MaterialWrapper& ptrBaseMat, bool bShadowEnable, bool pointShadowEnable);

		void addInstanceData(const ObjectInstanceData* datas, size_t num);
		void updateInstanceData(const ObjectInstanceData* datas,
			size_t start, size_t num, const Vector3& offset, const Vector3& scale, const Quaternion& ori);
		void clearInstanceData();

		std::pair<uint32, uint32> updateRenderQueue(PassType passType, PassMaskFlags passMaskFlags,
			RenderQueue* pRenderQueue, uint32 iInstNum);

		void setPassParam(const Renderable* pRend, const Pass* pPass,
			uint32 iInstIdxStart, uint32 iInstNum);

	public:
		InstanceStaticBatchObjectPrivate* mParent = nullptr;
		Uniformf4Vec mInstanceData;
		InstanceTypeInfo mInfo;
		SubMesh* mSubMesh = nullptr;
		RenderOperation mRenderOperation;
		IndexBuffInfo mIndexBuffInfo;
		int mInstanceCountPerBatch = 0;
		String mInstanceMacros;
		Name mMaterialName;
		MaterialWrapper mMaterial;

		RenderVec mRenderPool;
	}; // class InstanceSubStaticBatchEntity

	using InstanceSubStaticBatchObjectVec = std::vector<InstanceSubStaticBatchObject*>;

	void InstanceSubStaticBatchObject::initSubMeshData()
	{
		RenderOperation* pOp = mSubMesh->_getRenderOperation();
		const ECHO_DEVICE_CLASS eClass = Root::instance()->getDeviceType();

		//for (int i = LOD_LEVEL_0; i < LOD_LEVEL_NUM; ++i)
		{
			if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass)
			{
				mRenderOperation.m_stMeshData.m_vertexBuffer.reserve(
					pOp->m_stMeshData.m_vertexBuffer.size() + 1);
			}

			mRenderOperation.m_stMeshData = pOp->m_stMeshData;
			/*if (m_pIndexData && m_pVertexData)
			{
				m_renderOperation->GetMeshData().Release();

				RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();

				m_renderOperation->GetMeshData().m_IndexBufferData.m_indexCount = m_pIndexData->m_indexCount;

				if (m_pIndexData->m_indexCount > 0)
				{
					uint32 dataSize = sizeof(uint16) * m_pIndexData->m_indexCount;
					uint16* pIdxBuf = (uint16*)m_pIndexData->mp_cpu_stream_data->m_start_buffer;
					m_renderOperation->GetMeshData().m_IndexBufferData.m_indexBuffer = pRenderSystem->createStaticIB(pIdxBuf, dataSize);
					m_renderOperation->GetMeshData().m_indexBufferSize = dataSize;
				}

				m_renderOperation->GetMeshData().m_vertexCount = m_pVertexData->m_vertexCount;

				unsigned int bindingCound = mSubMesh->m_desc.IADesc.nStreamCount;
				for (unsigned int i = 0; i < bindingCound; ++i)
				{
					VertexCpuStreamData* pData = m_pVertexData->GetVertexCpuData(i);

					uint32 vtxBuffSize = pData->m_vertex_size * m_pVertexData->m_vertexCount;

					if (0 == vtxBuffSize)
					{
						char errMsg[512] = { 0 };
						snprintf(errMsg, 512, "-error-\tThe SubMesh vertex size is zero!!! meshname:%s", mSubMesh->mParent->getName().c_str());
						LogManager::instance()->logMessage(errMsg, LML_CRITICAL);
						continue;
					}

					m_renderOperation->GetMeshData().m_vertexBuffSize += vtxBuffSize;
					m_renderOperation->GetMeshData().m_vertexBuffer.push_back(pRenderSystem->createStaticVB(pData->mp_start_memory, vtxBuffSize));
				}
			}*/

			if (!pOp->m_stMeshData.m_vertexBuffer.empty() && ECHO_DEVICE_DX9C == eClass) {
				mRenderOperation.m_stMeshData.m_vertexBuffer.push_back(
					StaticBatchObjectUtil::instance()->mBuffInstID);
			}
		}
	}

	bool InstanceSubStaticBatchObject::init(SubMesh* pSubMesh, SceneManager* pSceneManager)
	{
		mSubMesh = pSubMesh;

		assert(pSubMesh != nullptr);
		if (nullptr == pSubMesh) return false;

		const MaterialWrapper& baseMat = pSubMesh->getMaterialWrapper();
		if (baseMat.isNull()) return false;

		String shaderName = baseMat.isV1() ? baseMat.getV1()->m_matInfo.shadername : baseMat.getV2()->getShaderPassName().toString();
		if (!initCaps(shaderName)) return false;

		bool bShadowEnable = pSceneManager->getPsmshadowManager()->getShadowEnable();
		initMaterial(baseMat, bShadowEnable, pSceneManager->getPointShadowManager()->IsEnabled());

		return true;
	}

	bool InstanceSubStaticBatchObject::initCaps(const String& ssShaderName)
	{
		mInfo = StaticBatchObjectUtil::instance()->getInstanceInfo(ssShaderName);
		if (eIT_None == mInfo.type) return false;

		uint64_t iMaxVtxUniformCount = Root::instance()->getRenderSystem()->
			getDependentLimit(ECHO_MAX_VERTEX_UNIFORM_VECTORS_NUMBER);
		if (0 == iMaxVtxUniformCount)
			return false;

		mInstanceCountPerBatch = int(std::min(iMaxVtxUniformCount - mInfo.rev_uniform_count, uint64_t(std::numeric_limits<int>::max())));
		mInstanceCountPerBatch /= mInfo.instance_uniform_count;
		if (mInstanceCountPerBatch <= 0)
			return false;

		char szBuf[512] = { '\0' };
		snprintf(szBuf, sizeof(szBuf) - 1u, "#define InstanceCount\t%d\n"
			"#define UniformCount\t%d\n", mInstanceCountPerBatch,
			mInfo.instance_uniform_count);

		mInstanceMacros = szBuf;

		return true;
	}

	void InstanceSubStaticBatchObject::initMaterial(
		const MaterialWrapper& baseMat, bool bShadowEnable, bool pointShadowEnable)
	{
		String ssTmpName;
		ssTmpName.reserve(baseMat.getName().size() + 32);
		ssTmpName.append(baseMat.getName().c_str()).
			append("#InstanceStaticBatch");
		mMaterialName = ssTmpName;

		mMaterial = StaticBatchObjectUtil::instance()->getMaterial(mMaterialName);
		if (!mMaterial.isNull())
			return;

		if (StaticBatchObjectUtil::instance()->isMaterialWaiting(mMaterialName))
			return;

		if (baseMat.isV2())
		{
			MaterialV2* pBaseMat = baseMat.getV2();

			MaterialV2Ptr ptrMat = MaterialV2Manager::instance()->createManual(mMaterialName);
			ptrMat->cloneFrom(pBaseMat, false);
			ptrMat->setBasePassMaskFlags(PassMaskFlags(PassMaskFlag::UseInstance));

			static bool synchronization = true;
			if (synchronization) {
				StaticBatchObjectUtil::instance()->requestMaterial(ptrMat);
			}
			else
			{
				ptrMat->prepareShaderPass();
				ptrMat->load();
				mMaterial = ptrMat;
			}
		}
		else if (baseMat.isV1())
		{
			Material* pBaseMat = baseMat.getV1();

			MaterialPtr ptrMat = MaterialManager::instance()->createManual(mMaterialName);
			ptrMat->m_desc = pBaseMat->m_desc;
			ptrMat->m_matInfo = pBaseMat->m_matInfo;

			if (Root::instance()->getDeviceType() == ECHO_DEVICE_DX9C)
			{
				// Add an extra instance id buffer
				uint32 idxInst = ptrMat->m_desc.IADesc.nStreamCount;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].bPreInstance = ECHO_TRUE;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nRate = 1;
				ptrMat->m_desc.IADesc.StreamArray[idxInst].nStride = 4;
				++ptrMat->m_desc.IADesc.nStreamCount;

				uint32 idxInstElem = ptrMat->m_desc.IADesc.nElementCount;
				ECHO_IA_ELEMENT& refElem = ptrMat->m_desc.IADesc.ElementArray[idxInstElem];
				refElem.nStream = idxInst;
				refElem.nIndex = 0u;
				refElem.eType = ECHO_UBYTE4;
				refElem.nOffset = 0u;
				refElem.Semantics = ECHO_BLENDINDICES1;
				++ptrMat->m_desc.IADesc.nElementCount;
			}

			String strVS, strPS, strVSMacro, strPSMacro;
			strVSMacro.reserve(128);	strPSMacro.reserve(128);
			MaterialManager::instance()->getShader(ptrMat.get(), strVS, strPS,
				strVSMacro, strPSMacro);
			strVSMacro.append(mInstanceMacros);
			ptrMat->setShaderSrcCode(strVS, strPS);
			ptrMat->setMacroShaderSrcCode(strVSMacro, strPSMacro);
			ptrMat->createPass(InstancePass);
			if (bShadowEnable)
			{
				ptrMat->createPass(ShadowCasterInstancePass);
				ptrMat->createPass(ShadowResiveInstancePass);
			}
			if (pointShadowEnable)
			{
				ptrMat->createPass(PointShadowCasterInstance);
				ptrMat->createPass(PointShadowReceiverInstance);
			}
			if (bShadowEnable && pointShadowEnable)
			{
				ptrMat->createPass(ShadowReceiverPointShadowReceiverInstance);
			}

			ptrMat->setMaterialPassMask(1ull << MPO_ShadowCasterInstancePass |
				1ull << MPO_ShadowResiveInstancePass);

			ptrMat->setMaterialPointShadowPassMask(
				MPSPO_PointShadowCasterInstance,
				MPSPO_PointShadowReceiverInstance,
				MPSPO_ShadowReceiverPointShadowReceiverInstance);

			static bool synchronization = true;
			if (synchronization) {
				StaticBatchObjectUtil::instance()->requestMaterial(ptrMat);
			}
			else
			{
				ptrMat->prepareInfo();
				ptrMat->loadInfo();
				mMaterial = ptrMat;
			}
		}
	}

	void InstanceSubStaticBatchObject::addInstanceData(
		const ObjectInstanceData* datas, size_t num)
	{
		assert(datas != nullptr);

		size_t iUniformNum = mInfo.instance_uniform_count * num;
		if (mInstanceData.capacity() == 0) {
			mInstanceData.reserve(iUniformNum > 64 ? iUniformNum : 64);
		}

		mInstanceData.insert(mInstanceData.end(), iUniformNum,
			{ { 0.f, 0.f, 0.f, 0.f } });
	}

	void InstanceSubStaticBatchObject::updateInstanceData(
		const ObjectInstanceData* datas, size_t start, size_t num,
		const Vector3& offset, const Vector3& scale, const Quaternion& ori)
	{
		assert(datas != nullptr);

		size_t iInstNum = mInstanceData.size() / mInfo.instance_uniform_count;
		if (start >= iInstNum || (start + num) > iInstNum)
		{
			assert(false);
			return;
		}

		for (size_t i = 0u; i < num; ++i)
		{
			mInfo.func(mSubMesh, datas[i], offset, scale, ori, mInstanceData,
				(start + i) * mInfo.instance_uniform_count);
		}
	}

	void InstanceSubStaticBatchObject::clearInstanceData()
	{
		mInstanceData.clear();
		mInstanceData.shrink_to_fit();
	}

	std::pair<uint32, uint32> InstanceSubStaticBatchObject::updateRenderQueue(PassType passType, PassMaskFlags passMaskFlags,
	                                                                          RenderQueue* pRenderQueue, uint32 iInstNum)
	{
		assert(mSubMesh != nullptr && !mMaterialName.empty() && !mInstanceData.empty());

		std::pair<uint32, uint32> pairInstBatch(0u, 0u);
		if (mMaterial.isNull())
		{
			mMaterial = StaticBatchObjectUtil::instance()->getMaterial(mMaterialName);
			if (mMaterial.isNull()) {
				return pairInstBatch;
			}
		}

		if (mMaterial.isV1())
		{
			Pass* pPass = mMaterial.getV1()->getPass(passType);
			if (nullptr == pPass) {
				return pairInstBatch;
			}

			// add render object to render queue
			RenderQueueGroup* pGroup = nullptr;
			if (mMaterial.getV1()->m_matInfo.alphaTestEnable)
				pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_SolidAlphaTest);
			else
				pGroup = pRenderQueue->getRenderQueueGroup(RenderQueue_Solid);

			uint32 iRenderIdx = 0;
			for (uint32 i = 0u; i < iInstNum; i += mInstanceCountPerBatch)
			{
				if (iRenderIdx >= mRenderPool.size())
					mRenderPool.push_back(new Render);
				Render* pRender = mRenderPool[iRenderIdx];
				pRender->mInstIdxStart = i;
				pRender->mInstNum = mInstanceCountPerBatch;
				pRender->mSubBatch = this;
				pRender->mSubMesh = mSubMesh;
				if (pRender->mInstIdxStart + pRender->mInstNum >= iInstNum)
					pRender->mInstNum = iInstNum - pRender->mInstIdxStart;
				pGroup->addRenderable(pPass, pRender);
				++iRenderIdx;
			}

			return std::pair<uint32, uint32>(iInstNum, iRenderIdx);
		}
		if (mMaterial.isV2())
		{
			uint32 iRenderIdx = 0;
			for (uint32 i = 0u; i < iInstNum; i += mInstanceCountPerBatch)
			{
				if (iRenderIdx >= mRenderPool.size())
					mRenderPool.push_back(new Render);
				Render* pRender = mRenderPool[iRenderIdx];
				pRender->mInstIdxStart = i;
				pRender->mInstNum = mInstanceCountPerBatch;
				pRender->mSubBatch = this;
				pRender->mSubMesh = mSubMesh;
				if (pRender->mInstIdxStart + pRender->mInstNum >= iInstNum)
					pRender->mInstNum = iInstNum - pRender->mInstIdxStart;
				PostRenderableParams postParams{
					.material = mMaterial.getV2(),
					.renderable = pRender,
					.renderQueue = pRenderQueue,
					.passMaskFlags = passMaskFlags,
					.basePriority = DEFAULT_PRIORITY,
					.isShadowPass = false
				};
				MaterialShaderPassManager::fastPostRenderable(postParams);
				++iRenderIdx;
			}

			return std::pair<uint32, uint32>(iInstNum, iRenderIdx);
		}

		return pairInstBatch;
	}

	void InstanceSubStaticBatchObject::setPassParam(const Renderable* pRend,
		const Pass* pPass, uint32 iInstIdxStart, uint32 iInstNum)
	{
		uint32 iUniformStart = iInstIdxStart * mInfo.instance_uniform_count;
		uint32 iUniformNum = iInstNum * mInfo.instance_uniform_count;

		assert(iUniformStart < mInstanceData.size() &&
			(iUniformStart + iUniformNum) <= mInstanceData.size());

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		pRenderSystem->setUniformValue(pRend, pPass, U_VSCustom0,
			mInstanceData.data() + iUniformStart,
			iUniformNum * sizeof(mInstanceData[0]));
	}

	class InstanceStaticBatchObjectPrivate
	{
	public:
		~InstanceStaticBatchObjectPrivate()
		{
			clearBatches();
		}

		void buildBatches(const MeshPtr& mesh);

		void clearBatches();

		MeshPtr& getMesh() { return mMesh; }

		void addInstance(const ObjectInstanceData* datas, size_t num);

		void updateRenderQueue(PassType passType, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue);

		Vector3 getWorldOffset() { return mWorldOffset; }

	public:
		SceneManager* mSceneManager = nullptr;
		MeshPtr mMesh;
		AxisAlignedBox mWorldAABB;
		Vector3 mWorldOffset = { 0.f, 0.f, 0.f };
		Vector3 mWorldScale = { 1.f,1.f,1.f };
		Quaternion mWorldOri;
		/// Mesh state count, used to detect differences
		size_t mMeshStateCount = 0u;
		ObjectInstanceDataVec mInstances;
		InstanceSubStaticBatchObjectVec mSubBatches;

		int mHeightMipLevel[4] = { -1, -1, -1, -1 };
		FloatRect mRange = FloatRect(
			std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity()
		);

		bool mUpdateLastRender = false;
	}; // class InstanceStaticBatchEntityPrivate

	void InstanceStaticBatchObjectPrivate::buildBatches(const MeshPtr& mesh)
	{
		assert(mSubBatches.empty() && "");
		if (!mSubBatches.empty())
			return;

		mMesh = mesh;
		uint32 num = mesh->getNumSubMesh();
		mSubBatches.resize(num, nullptr);
		for (uint32 i = 0u; i < num; ++i)
		{
			SubMesh* pSubMesh = mMesh->getSubMesh(i);
			if (!InstanceStaticBatchObject::TestSupported(pSubMesh)) {
				LogManager::instance()->logMessage(LML_CRITICAL, "The InstanceStaticBatchObject Mesh Test Failed.");
				continue;
			}

			auto* pSubBatch = new InstanceSubStaticBatchObject(this);
			if (!pSubBatch->init(pSubMesh, mSceneManager))
			{
				delete pSubBatch;
				continue;
			}

			mSubBatches[i] = pSubBatch;
		}

		mMeshStateCount = mMesh->getStateCount();
	}

	void InstanceStaticBatchObjectPrivate::clearBatches()
	{
		for (auto& it : mSubBatches) {
			delete it;
		}

		mSubBatches.clear();
	}

	void InstanceStaticBatchObjectPrivate::addInstance(
		const ObjectInstanceData* datas, size_t num)
	{
		if (mInstances.capacity() == 0)
			mInstances.reserve(num > 32 ? num : 32);

		size_t oldSize = mInstances.size();
		mInstances.insert(mInstances.end(), datas, datas + num);
		size_t newSize = mInstances.size();

		const AxisAlignedBox& aabb = mMesh->getAABB();
		Matrix4 matWorld;

		for (size_t i = oldSize; i < newSize; ++i)
		{
			auto& data = mInstances[i];
			/*if (1 == data.snap_mode)
			{
				float fHeight = 0.f;
				if (TerrainUtil::GetHeight(data.position, fHeight) == eTR_OK)
					data.position.y = fHeight + data.snap_offset;
			}*/

			Matrix4 mat;
			mat.makeTransform(mWorldOffset, mWorldScale, mWorldOri);

			matWorld.makeTransform(mat * data.position,
				data.scale, data.orientation);
			AxisAlignedBox aabb_obj = aabb;
			aabb_obj.transformAffine(matWorld);
			mWorldAABB.merge(aabb_obj);

			__merge_to_rect(mRange, data.position.x, data.position.z);
		}

		for (auto& it : mSubBatches)
		{
			if (it != nullptr)
				it->addInstanceData(mInstances.data() + oldSize, num);
		}
	}

	void InstanceStaticBatchObjectPrivate::updateRenderQueue(
		PassType passType, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue)
	{
		if (mMeshStateCount != mMesh->getStateCount())
		{
			clearBatches();
			buildBatches(mMesh);
			for (auto& it : mSubBatches)
			{
				if (it != nullptr)
				{
					it->addInstanceData(mInstances.data(), mInstances.size());
					it->initSubMeshData();
				}
			}

		}

		int iCurMipLevel[4] = { -1, -1, -1, -1, };
		TerrainSystem::instance()->getHeightMipLevel(mRange.left, mRange.top, iCurMipLevel[0]);
		TerrainSystem::instance()->getHeightMipLevel(mRange.left, mRange.bottom, iCurMipLevel[1]);
		TerrainSystem::instance()->getHeightMipLevel(mRange.right, mRange.bottom, iCurMipLevel[2]);
		TerrainSystem::instance()->getHeightMipLevel(mRange.right, mRange.top, iCurMipLevel[3]);
		if (memcmp(mHeightMipLevel, iCurMipLevel, sizeof(mHeightMipLevel)) != 0)
		{
			const AxisAlignedBox& aabb = mMesh->getAABB();
			Matrix4 mat;
			mat.makeTransform(mWorldOffset, mWorldScale, mWorldOri);
			Matrix4 matWorld;
			for (auto& data : mInstances)
			{
				matWorld.makeTransform(mat * data.position,
					data.scale, data.orientation);

				AxisAlignedBox aabb_obj = aabb;
				aabb_obj.transformAffine(matWorld);
				mWorldAABB.merge(aabb_obj);
			}
			memcpy(mHeightMipLevel, iCurMipLevel, sizeof(mHeightMipLevel));
		}

		std::pair<uint32, uint32> pairInstBatch(0u, 0u);
		for (auto& it : mSubBatches)
		{
			if (nullptr == it)
			{
				continue;
			}

			it->updateInstanceData(mInstances.data(), 0u, mInstances.size(), mWorldOffset, mWorldScale, mWorldOri);

			auto res = it->updateRenderQueue(passType, passMaskFlags, pRenderQueue, (uint32)mInstances.size());
			pairInstBatch.first += res.first;
			pairInstBatch.second += res.second;
		}

		Root::instance()->addBatchDrawCount(pairInstBatch.first - pairInstBatch.second);
	}

	bool InstanceStaticBatchObject::m_bInitUtil = false;

	InstanceStaticBatchObject::InstanceStaticBatchObject(SceneManager* pSceneManager)
		: dp(new InstanceStaticBatchObjectPrivate)
	{
		dp->mSceneManager = pSceneManager;
	}

	InstanceStaticBatchObject::~InstanceStaticBatchObject()
	{
		clearInstance();
		delete dp; dp = nullptr;
	}

	bool InstanceStaticBatchObject::init(const MeshPtr& mesh,
		const Vector3& offset, const Vector3& scale, const Quaternion& ori)
	{
		assert(!mesh.isNull());
		if (mesh.isNull())
			return false;

		setWorldSeting(offset, scale, ori);
		dp->buildBatches(mesh);
		return true;
	}

	void InstanceStaticBatchObject::initVertexAndIndexSubData()
	{
		for (auto& obj : dp->mSubBatches)
		{
			if (obj)
				obj->initSubMeshData();
		}
	}

	MeshPtr& InstanceStaticBatchObject::getMesh()
	{
		return dp->getMesh();
	}

	void InstanceStaticBatchObject::addInstance(const ObjectInstanceData& data)
	{
		dp->addInstance(&data, 1);
	}

	void InstanceStaticBatchObject::addInstance(const ObjectInstanceDataVec& datas)
	{
		dp->addInstance(datas.data(), datas.size());
	}

	void InstanceStaticBatchObject::clearInstance()
	{
		onlyclearInstance();
		dp->mMesh = nullptr;
	}

	void InstanceStaticBatchObject::onlyclearInstance()
	{
		dp->mInstances.clear();
		for (auto& it : dp->mSubBatches)
		{
			if (it != nullptr)
				it->clearInstanceData();
		}

		dp->mInstances.shrink_to_fit();

		dp->mWorldAABB.setNull();
		dp->mHeightMipLevel[0] = -1;
		dp->mHeightMipLevel[1] = -1;
		dp->mHeightMipLevel[2] = -1;
		dp->mHeightMipLevel[3] = -1;
		dp->mRange = FloatRect(std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity());
	}

	void InstanceStaticBatchObject::setUpdateState(bool isupdate)
	{
		dp->mUpdateLastRender = isupdate;
	}

	void InstanceStaticBatchObject::setWorldSeting(const Vector3& offset, const Vector3& scale, const Quaternion& ori)
	{
		dp->mWorldOffset = offset;
		dp->mWorldScale = scale;
		dp->mWorldOri = ori;
	}

	const AxisAlignedBox& InstanceStaticBatchObject::getWorldAABB() const
	{
		return dp->mWorldAABB;
	}

	Vector3& InstanceStaticBatchObject::getWorldPosition()
	{
		return dp->mWorldOffset;
	}

	Vector3& InstanceStaticBatchObject::getWorldScale()
	{
		return dp->mWorldScale;
	}

	Quaternion& InstanceStaticBatchObject::getQuaternion()
	{
		return dp->mWorldOri;
	}

	void InstanceStaticBatchObject::updateRenderQueue(PassType passType, PassMaskFlags passMaskFlags, RenderQueue* pRenderQueue)
	{
		dp->updateRenderQueue(passType, passMaskFlags, pRenderQueue);
	}

	void InstanceStaticBatchObject::Init()
	{
		if (m_bInitUtil)
			return;

		m_bInitUtil = true;

		StaticBatchObjectUtil *pUtil = new StaticBatchObjectUtil();
		pUtil->init();

#define shader_type_register(util, type, inst_uni_count, rev_uni_count, _func) \
		util->registerShaderType(eIT_##type, #type, (inst_uni_count), (rev_uni_count), _func);

		shader_type_register(pUtil, Illum, 6, 58, InstanceData_WorldTransform);
		shader_type_register(pUtil, IllumPBR, 6, 58, InstanceData_WorldTransform);
		shader_type_register(pUtil, IllumPBR2022, 6, 58, InstanceData_WorldTransform);
		shader_type_register(pUtil, IllumPBR2023, 6, 58, InstanceData_WorldTransform);
		shader_type_register(pUtil, SpecialIllumPBR, 6, 58, InstanceData_WorldTransform);
		shader_type_register(pUtil, BillboardLeaf, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(pUtil, BillboardLeafPBR, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(pUtil, Trunk, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(pUtil, TrunkPBR, 3, 58, updateWorldPositionScaleOrientation);
		shader_type_register(pUtil, SimpleTreePBR, 6, 58, InstanceData_WorldTransform);
		//shader_type_register(pUtil, BaseWhiteNoLight, 3, 58, updateWorldPositionScaleOrientation);
#undef shader_type_register

		LogManager::instance()->logMessage(LML_NORMAL, "InstanceStaticBatchObject init successfully.");
	}

	void InstanceStaticBatchObject::Destroy()
	{
		if (!m_bInitUtil)
			return;

		m_bInitUtil = false;

		if (StaticBatchObjectUtil::instance() != nullptr)
		{
			StaticBatchObjectUtil::instance()->destroy();
			delete StaticBatchObjectUtil::instance();
			LogManager::instance()->logMessage(LML_NORMAL, "InstanceStaticBatchObject destroyed.");
		}
	}

	bool InstanceStaticBatchObject::TestSupported(SubMesh* pSubMesh)
	{
		StaticBatchObjectUtil* pUtil = StaticBatchObjectUtil::instance();
		if (nullptr == pUtil)
		{
			LogManager::instance()->logMessage(LML_CRITICAL, "The InstanceStaticBatchObject hasn't init.");
			return false;
		}

		const MaterialWrapper& ptrMat = pSubMesh->getMaterialWrapper();
		if (ptrMat.isNull()) {
			return false;
		}

		if (ptrMat.isV1())
		{
			Material* matV1 = ptrMat.getV1();
			return !matV1->m_matInfo.transparentEnable &&
				matV1->m_matInfo.genmask.find("%HWSKINNED") == String::npos &&
				pUtil->getInstanceType(matV1->m_matInfo.shadername) != eIT_None;
		}
		if (ptrMat.isV2())
		{
			MaterialV2* matV2 = ptrMat.getV2();
			return !matV2->isTransparent() &&
				matV2->isEnableHwSkinned() &&
				pUtil->getInstanceType(matV2->getShaderPassName().toString()) != eIT_None;
		}
		return false;
	}

	TrafficBatchObject::TrafficBatchObject(SceneManager* pSceneManager)
	{
		mSceneMgr = pSceneManager;
		mStaticBatch = new InstanceStaticBatchObject(pSceneManager);
		for (int i = 0; i < LOD_LEVEL_NUM; ++i)
		{
			float d = i * 100.f;
			m_fLodDistance[i] = d;
		}
	}

	bool TrafficBatchObject::initInstanceStaticBatchObject(MeshPtr& mesh)
	{
		if (mesh.isNull()) return false;
		if (mBatchChangeData.size() == 0) return false;

		if (mStaticBatch->getMesh().isNull() ||
			mStaticBatch->getMesh() != mesh ||
			mStaticBatch->getWorldPosition() != mWorldOffset ||
			mStaticBatch->getWorldScale() != mWorldScale ||
			mStaticBatch->getQuaternion() != mWorldOri)
		{
			//mStaticBatch->clearInstance();
			delete mStaticBatch;
			mStaticBatch = new InstanceStaticBatchObject(mSceneMgr);

			mStaticBatch->init(mesh, mWorldOffset, mWorldScale, mWorldOri);
			mStaticBatch->addInstance(mBatchChangeData);
			mStaticBatch->initVertexAndIndexSubData();

			return false;
		}

		return true;
	}

	/*void TrafficBatchObject::findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue)
	{
		float fShowDist = EntityBatchOptions::GetShowDistance();
		if (mRenderDistance != 0.f)
			fShowDist = mRenderDistance;
		const Camera* pMainCamera = mSceneMgr->getMainCamera();
		const Vector3 vCamPos = pMainCamera->getDerivedPosition();
		assert(pMainCamera != nullptr);


		AxisAlignedBox aabbWorld = mStaticBatch->getWorldAABB();
		float fSquareDistNear = _SquareDistPointToAABB_Near(vCamPos, aabbWorld);

		PassType passType = InstancePass;

		if (!mMesh->m_bUseLod)
		{
			initInstanceStaticBatchObject(mMesh);
			mStaticBatch->updateRenderQueue(passType, pQueue);
		}
		else
		{
			size_t LodSize = mMesh->m_vtLodMesh.size();
			if (fSquareDistNear < POW2(fShowDist / (LodSize + 1)))
			{
				initInstanceStaticBatchObject(mMesh);
				mStaticBatch->updateRenderQueue(passType, pQueue);
			}
			else
				for (size_t i = LodSize; i >= 1; --i)
				{
					if (fSquareDistNear < POW2(fShowDist / i))
					{
						size_t lodLevel = LodSize - i;
						initInstanceStaticBatchObject(mMesh->m_vtLodMesh[lodLevel]);
						mStaticBatch->updateRenderQueue(passType, pQueue);
						break;
					}
				}
		}
	}*/

	Echo::MovableObjectType TrafficBatchObject::getMovableType() const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return MovableObjectType_ManualObject;
	}

	const Echo::AxisAlignedBox& TrafficBatchObject::getBoundingBox() const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return mBounds;
	}

	float TrafficBatchObject::getBoundingRadius() const
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL)
			return mRadius;
	}

	void TrafficBatchObject::_notifyCurrentCamera(Camera* pMainCamera)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);
		//float fShowDist = EntityBatchOptions::GetShowDistance();
		//if (mRenderDistance != 0.f)
		//	fShowDist = mRenderDistance;

		//const Camera* pMainCamera = mSceneMgr->getMainCamera();
		Vector3 vCamPos = pMainCamera->getDerivedPosition();
		assert(pMainCamera != nullptr);

		AxisAlignedBox aabbWorld = mStaticBatch->getWorldAABB();
		float fSquareDistNear = _SquareDistPointToAABB_Near(vCamPos, aabbWorld);

		if (fSquareDistNear < POW2(mRenderDistance))
		{
			if (fSquareDistNear < POW2(m_fLodDistance[LOD_LEVEL_1]))
				mCurrLodLevel = LOD_LEVEL_0;
			else if (fSquareDistNear < POW2(m_fLodDistance[LOD_LEVEL_2]))
				mCurrLodLevel = LOD_LEVEL_1;
			else
				mCurrLodLevel = LOD_LEVEL_2;
			mCulled = false;
		}
		else
			mCulled = true;
	}

	void TrafficBatchObject::updateRenderQueue(Echo::RenderQueue* pRenderQueue)
	{
		DEF_THREAD_CHEEK(DEF_THREAD_TYPE_ALL);

		PassType passType = InstancePass; // 这个给 Material V1
		PassMaskFlags passMaskFlags = PassMaskFlag::UseInstance;	// 这个给 V2

		mWorldOffset = mParentNode->_getDerivedPosition();
		mWorldScale = mParentNode->_getDerivedScale();
		mWorldOri = mParentNode->_getDerivedOrientation();

		if (!mMesh->m_bUseLod)
		{
			if (initInstanceStaticBatchObject(mMesh))
				mStaticBatch->setWorldSeting(mWorldOffset, mWorldScale, mWorldOri);
			mStaticBatch->updateRenderQueue(passType, passMaskFlags, pRenderQueue);
		}
		else
		{
			size_t LodSize = mMesh->m_vtLodMesh.size();
			mCurrLodLevel = mCurrLodLevel >= LodSize ? ((EM_LOD_LEVEL)(LodSize - 1)) : mCurrLodLevel;
			if (initInstanceStaticBatchObject(mCurrLodLevel == LOD_LEVEL_0 ? mMesh : mMesh->m_vtLodMesh[mCurrLodLevel - 1]))
				mStaticBatch->setWorldSeting(mWorldOffset, mWorldScale, mWorldOri);
			mStaticBatch->updateRenderQueue(passType, passMaskFlags, pRenderQueue);
		}
	}

	void TrafficBatchObject::init(const MeshPtr& mesh, const Vector3& offset, const Vector3& scale, const Quaternion& ori)
	{
		mMesh = mesh;
		//mWorldOffset = offset;
		mStaticBatch->init(mesh, offset, scale, ori);
	}

	void TrafficBatchObject::initInstanceData(const ObjectInstanceDataVec& data)
	{
		mBatchChangeData = data;
	}

	void TrafficBatchObject::asyncLastInit()
	{
		mStaticBatch->initVertexAndIndexSubData();
	}

	void TrafficBatchObject::addInstanceData(ObjectInstanceDataVec& data)
	{
		if (data.size() == 0) return;

		Matrix3 matri;
		mWorldOri.ToRotationMatrix(matri);
		AxisAlignedBox ab = mStaticBatch->getMesh()->getAABB();

		for (auto& batchData : data)
		{
			//batchData.scale *= mWorldScale;
			//batchData.orientation = batchData.orientation * mWorldOri;
			//batchData.position = matri * batchData.position;

			Matrix4 mat;
			mat.makeTransform(batchData.position, batchData.scale, batchData.orientation);
			AxisAlignedBox re = ab;
			re.transformAffine(mat);
			mBounds.merge(re);
			mRadius = std::max(re.vMax.length(), re.vMin.length());
		}
		mStaticBatch->addInstance(data);


		mBatchChangeData.insert(mBatchChangeData.end(), data.begin(), data.end());
	}

	void TrafficBatchObject::initWorldSet(const Vector3& scale, const Quaternion& ori)
	{
		//mWorldScale = scale;
		//mWorldOri = ori;
	}

}
