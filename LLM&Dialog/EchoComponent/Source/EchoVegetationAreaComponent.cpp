///////////////////////////////////////////////////////////////////////////////
//
// @file EchoVegetationAreaComponent.cpp
// @author chensheng
// @date 2024/02/26 Monday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "EchoVegetationAreaComponent.h"

#include "EchoVegetationGroup.h"
#include "EchoVegetationOptions.h"
#include "EchoVegetationArea.h"
#include "EchoVegetationAreaResource.h"
#include "EchoVegetationAreaResourceManager.h"
#include "EchoSubVegetationAreaResource.h"
#include "EchoEasyAsynHandler.h"
#include "EchoVegetationAreaComponentTestHelper.h"

#include "EchoLogManager.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"

#include "Actor/EchoActor.h"


namespace Echo
{
	namespace 
	{
		using	_SubVegParam	= VegAreaCompInfo::SubVegParam;
		using	_SubVegParamVec = VegAreaCompInfo::SubVegParamVec;
		using	_AreaInfo		= VegAreaCompInfo::AreaInfo;
		using	_MeshInfo		= VegAreaCompInfo::MeshInfo;
		using	_MeshInfoVec	= VegAreaCompInfo::MeshInfoVec;

		const std::string _LOG_HEAD("[VegAreaCom]:");
#define LOG_N(message)														\
	LogManager::instance()->logMessage(LML_NORMAL, _LOG_HEAD + message)		
#define LOG_C(message)														\
	LogManager::instance()->logMessage(LML_CRITICAL, _LOG_HEAD + message)	

		bool	_VegGroupIsVaild() { return VegOpts::GetVegetationGroup(); };
		Matrix4 _GetWorldMatrix(const ActorComponent* comp) { return comp->getActor()->getWorldMatrix(); };
		Matrix3 _GetNormalMat(const Matrix4& mat);
	}

	///////////////////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////VegetationAreaComponentInfo////////////////////////
	EDITOR_REFLECT_BEGIN(VegetationAreaComponentInfo)
	EDITOR_REFLECT_END();

	VegetationAreaComponentInfo::VegetationAreaComponentInfo(const Name& type,
		ActorCOMMoveableType movable) :ActorComponentInfo(type, movable) {};

	void VegetationAreaComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<VegetationAreaComponentInfo>::get()->ExportJson(this, jsonNode, "");
		//	Area Info
		{
			WriteJSNode(jsonNode, "AreaPoints", mAreaInfo.points);
			WriteJSNode(jsonNode, "AreaHitDir", mAreaInfo.dir);
			WriteJSNode(jsonNode, "AreaRange", mAreaInfo.range);
		}
		//	Mesh Info Vector
		{
			cJSON* meshInfoArray = cJSON_CreateArray();
			cJSON_AddItemToObject(jsonNode, "MeshInfoArray", meshInfoArray);
			for (const MeshInfo& meshInfo : mMeshInfoVec)
			{
				cJSON* meshInfoItem = cJSON_CreateObject();
				WriteJSNode(meshInfoItem, "name", meshInfo.name);
				WriteJSNode(meshInfoItem, "mat", meshInfo.mat._m, 16);
				WriteJSNode(meshInfoItem, "min", meshInfo.min);
				WriteJSNode(meshInfoItem, "max", meshInfo.max);
				cJSON_AddItemToArray(meshInfoArray, meshInfoItem);
			}
		}
		//	SubVegParamVec
		{
			cJSON* subVegParamArray = cJSON_CreateArray();
			cJSON_AddItemToObject(jsonNode, "SubVegParamArray", subVegParamArray);
			for (const SubVegParam& subVegParam : mSubVegParamVec)
			{
				cJSON* subVegParamItem = cJSON_CreateObject();
				WriteJSNode(subVegParamItem, "layer_name", subVegParam.layer_name);
				WriteJSNode(subVegParamItem, "density", subVegParam.density);
				WriteJSNode(subVegParamItem, "scale_base", subVegParam.scale_base);
				WriteJSNode(subVegParamItem, "scale_range", subVegParam.scale_range);
				WriteJSNode(subVegParamItem, "top_diffuse", subVegParam.top_diffuse);
				WriteJSNode(subVegParamItem, "top_diffuse_ratio", subVegParam.top_diffuse_ratio);
				WriteJSNode(subVegParamItem, "bottom_diffuse", subVegParam.bottom_diffuse);
				WriteJSNode(subVegParamItem, "bottom_diffuse_ratio", subVegParam.bottom_diffuse_ratio);
				WriteJSNode(subVegParamItem, "top_emission", subVegParam.top_emission);
				WriteJSNode(subVegParamItem, "top_emission_ratio", subVegParam.top_emission_ratio);
				WriteJSNode(subVegParamItem, "bottom_emission", subVegParam.bottom_emission);
				WriteJSNode(subVegParamItem, "bottom_emission_ratio", subVegParam.bottom_emission_ratio);
				WriteJSNode(subVegParamItem, "refresh_distribution", subVegParam.needRefreshDistribution);
				union {
					uint64 seed;
					double d;
				}temp;
				temp.seed = subVegParam.distributionSeed;
				WriteJSNode(subVegParamItem, "distribution_seed", temp.d);

				cJSON_AddItemToArray(subVegParamArray, subVegParamItem);
			}
		}
	}

	void VegetationAreaComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<VegetationAreaComponentInfo>::get()->ImportJson(this, jsonNode, "");
		//	Area Info
		{
			ReadJSNode(jsonNode, "AreaPoints", mAreaInfo.points);
			ReadJSNode(jsonNode, "AreaHitDir", mAreaInfo.dir);
			ReadJSNode(jsonNode, "AreaRange", mAreaInfo.range);
		}
		//	Mesh Info Vector
		{
			cJSON* meshInfoArray = cJSON_GetObjectItem(jsonNode, "MeshInfoArray");
			if (meshInfoArray)
			{
				int meshInfoArraySize = cJSON_GetArraySize(meshInfoArray);
				mMeshInfoVec.resize(meshInfoArraySize);
				for (int index = 0; index < meshInfoArraySize; index++)
				{
					cJSON* meshInfoItem = cJSON_GetArrayItem(meshInfoArray, index);
					MeshInfo& meshInfo = mMeshInfoVec.at(index);

					ReadJSNode(meshInfoItem, "name", meshInfo.name);
					ReadJSNode(meshInfoItem, "mat", meshInfo.mat._m, 16);
					ReadJSNode(meshInfoItem, "min", meshInfo.min);
					ReadJSNode(meshInfoItem, "max", meshInfo.max);
				}
			}
		}
		//	SubVegParamVec
		{
			cJSON* subVegParamArray = cJSON_GetObjectItem(jsonNode, "SubVegParamArray");
			if (subVegParamArray)
			{
				int subVegParamArraySize = cJSON_GetArraySize(subVegParamArray);
				mSubVegParamVec.resize(subVegParamArraySize);
				for (int index = 0; index < subVegParamArraySize; index++)
				{
					cJSON* subVegParamItem = cJSON_GetArrayItem(subVegParamArray, index);
					SubVegParam& subVegParam = mSubVegParamVec.at(index);

					ReadJSNode(subVegParamItem, "layer_name", subVegParam.layer_name);
					ReadJSNode(subVegParamItem, "density", subVegParam.density);
					ReadJSNode(subVegParamItem, "scale_base", subVegParam.scale_base);
					ReadJSNode(subVegParamItem, "scale_range", subVegParam.scale_range);
					ReadJSNode(subVegParamItem, "top_diffuse", subVegParam.top_diffuse);
					ReadJSNode(subVegParamItem, "top_diffuse_ratio", subVegParam.top_diffuse_ratio);
					ReadJSNode(subVegParamItem, "bottom_diffuse", subVegParam.bottom_diffuse);
					ReadJSNode(subVegParamItem, "bottom_diffuse_ratio", subVegParam.bottom_diffuse_ratio);
					ReadJSNode(subVegParamItem, "top_emission", subVegParam.top_emission);
					ReadJSNode(subVegParamItem, "top_emission_ratio", subVegParam.top_emission_ratio);
					ReadJSNode(subVegParamItem, "bottom_emission", subVegParam.bottom_emission);
					ReadJSNode(subVegParamItem, "bottom_emission_ratio", subVegParam.bottom_emission_ratio);
					ReadJSNode(subVegParamItem, "refresh_distribution", subVegParam.needRefreshDistribution);
					union{
						uint64 seed;
						double d;
					}temp;
					ReadJSNode(subVegParamItem, "distribution_seed", temp.d);
					subVegParam.distributionSeed = temp.seed;
				}
			}
		}
	}

	void VegetationAreaComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		VegetationAreaComponentInfo* pInfo = static_cast<VegetationAreaComponentInfo*>(comInfo);
		reflect::TypeResolver<VegetationAreaComponentInfo>::get()->copyTo(this, pInfo);

		pInfo->mAreaInfo = mAreaInfo;
		pInfo->mMeshInfoVec = mMeshInfoVec;
		pInfo->mSubVegParamVec = mSubVegParamVec;
	}

	void VegetationAreaComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			VegetationAreaComponentInfo* oriInfo = static_cast<VegetationAreaComponentInfo*> (mOriginComInfo);
			mAreaInfo = oriInfo->mAreaInfo;
			mMeshInfoVec = oriInfo->mMeshInfoVec;
			mSubVegParamVec = oriInfo->mSubVegParamVec;
		}
	}
	///////////////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////VegetationAreaComponent//////////////////////////
	struct VegetationAreaComponent::_Private : public AsynTaskSubmitter
	{
		//	Async task
		struct _AsynTask : public AsynTask
		{
			using AsynTask::AsynTask;	//	继承构造

			virtual bool task_executionImpl() override
			{
				if (res_ptr.isNull())
					return false;
				res_ptr->prepare();
				return true;
			}

			virtual void task_finishedImpl() override
			{
				VegetationGroup* pGroup = VegOpts::GetVegetationGroup();
				if (!pGroup) 
					return;
				res_ptr->load();
				pGroup->needUpdate();
				_Private* dp = get();
				if (dp && dp->mHandle)
					dp->mHandle->updateBox();
			}
		public:
			VegetationAreaResourcePtr res_ptr;
		private:
			_Private* get() { return static_cast<_Private*>(mAsynTaskSubmitter); };
		};

#define CHECK_IF_CHANGE(cls,member,newval,changeNotify) if(cls->get##member() != newval){cls->set##member(newval),changeNotify=true;}

		void _addSubVegArea(VegetationAreaResourcePtr ptrVegArea, const _SubVegParam& param, bool needLoad = false)
		{
			if (ptrVegArea.isNull()) return;

			SubVegetationAreaResource* pSubVegAreaRes =
				ptrVegArea->getSubVegArea(Name(param.layer_name));
			bool forceRefresh = false;
			if (pSubVegAreaRes == nullptr)
			{
				pSubVegAreaRes = ptrVegArea->createSubVegArea(Name(param.layer_name));
				forceRefresh = true;
			}

			pSubVegAreaRes->setTopDiffuse(param.top_diffuse);
			pSubVegAreaRes->setTopDiffuseRatio(param.top_diffuse_ratio);
			pSubVegAreaRes->setBottomDiffuse(param.bottom_diffuse);
			pSubVegAreaRes->setBottomDiffuseRatio(param.bottom_diffuse_ratio);
			pSubVegAreaRes->setTopEmission(param.top_emission);
			pSubVegAreaRes->setTopEmissionRatio(param.top_emission_ratio);
			pSubVegAreaRes->setBottomEmission(param.bottom_emission);
			pSubVegAreaRes->setBottomEmissionRatio(param.bottom_emission_ratio);
			pSubVegAreaRes->setScaleBase(param.scale_base);
			pSubVegAreaRes->setScaleRange(param.scale_range);

			pSubVegAreaRes->setDistributionSeed(param.distributionSeed);
			CHECK_IF_CHANGE(pSubVegAreaRes, LayerName, Echo::Name(param.layer_name), forceRefresh);
			CHECK_IF_CHANGE(pSubVegAreaRes, Density, param.density, forceRefresh);


			if (needLoad && forceRefresh)
			{
				pSubVegAreaRes->load();
				pSubVegAreaRes->generateObjects();
			}
		}

		void _updateSubVegArea(const _SubVegParam& param)
		{
			if (nullptr == mHandle)
				return;

			VegetationAreaResourcePtr ptrVegArea = mHandle->getResource();
			if (ptrVegArea.isNull())
				return;

			SubVegetationAreaResource* pSubVegAreaRes =
			ptrVegArea->getSubVegArea(Name(param.layer_name));
			if (pSubVegAreaRes == nullptr)
				return;

			pSubVegAreaRes->setLayerName(Echo::Name(param.layer_name));
			pSubVegAreaRes->setDensity(param.density);
			pSubVegAreaRes->setScaleBase(param.scale_base);
			pSubVegAreaRes->setScaleRange(param.scale_range);
			pSubVegAreaRes->setTopDiffuse(param.top_diffuse);
			pSubVegAreaRes->setTopDiffuseRatio(param.top_diffuse_ratio);
			pSubVegAreaRes->setBottomDiffuse(param.bottom_diffuse);
			pSubVegAreaRes->setBottomDiffuseRatio(param.bottom_diffuse_ratio);
			pSubVegAreaRes->setTopEmission(param.top_emission);
			pSubVegAreaRes->setTopEmissionRatio(param.top_emission_ratio);
			pSubVegAreaRes->setBottomEmission(param.bottom_emission);
			pSubVegAreaRes->setBottomEmissionRatio(param.bottom_emission_ratio);
			pSubVegAreaRes->setDistributionSeed(param.distributionSeed);
		}

		void _removeSubVegArea(const Name& layerName)
		{
			if (nullptr == mHandle)
				return;

			VegetationAreaResourcePtr ptrVegArea = mHandle->getResource();
			if (ptrVegArea.isNull())
				return;

			ptrVegArea->removeSubVegArea(layerName);
		}

		void _createVegAreaHandle(const Name& name,
			const VegAreaCompInfo* info, const Matrix4& buildingToWorldSpaceMat)
		{
			if (info->mAreaInfo.points.size() < 3)
			{
				LOG_C(std::string("A valud area contains "
					"3 points at least! Name : ") + name.toString());
				return;
			}

			if (false == _VegGroupIsVaild()) return;

			if (mHandle)
			{
				LOG_N(std::string("Handle ptr is not null, Name : ") + name.toString());
				_destoryVegAreaHandle();
			}

			const Name createName(name.toString() + std::to_string(sVegCrtIndex++));


			VegAreaResCreateInfo createInfo;
			for (const _MeshInfo& meshInfo_build : info->mMeshInfoVec)
			{
				VegHitCalculator::MeshInfo vegMeshInfo = {
					.name = meshInfo_build.name,
					.mat = meshInfo_build.mat,
					.min = meshInfo_build.min,
					.max = meshInfo_build.max
				};
				createInfo.meshInfoVec.emplace_back(vegMeshInfo);
			}

			createInfo.points = info->mAreaInfo.points;
			createInfo.range = info->mAreaInfo.range;
			createInfo.dir = info->mAreaInfo.dir;

			VegetationAreaResourcePtr ptrVegArea =
				VegetationAreaResourceManager::instance()->createOrRetrieve(createName, false, 0, (NameValuePairList*)(&createInfo)).first;

			for (const VegetationAreaComponentInfo::SubVegParam& subParam : info->mSubVegParamVec)
				_addSubVegArea(ptrVegArea, subParam, Root::instance()->getRuntimeMode() == Root::eRM_Editor);

			mHandle = VegOpts::GetVegetationGroup()->createVegetationArea(ptrVegArea);
			mHandle->setTransform(buildingToWorldSpaceMat);

			std::shared_ptr<_AsynTask> task = std::make_shared<_AsynTask>(this);
			task->res_ptr = ptrVegArea;
			submit(task);
		}

		void _destoryVegAreaHandle()
		{
			if (nullptr == mHandle || false == _VegGroupIsVaild())
				return;
			VegOpts::GetVegetationGroup()->destroyVegetationArea(mHandle);
			mHandle = nullptr;
		}

		~_Private()
		{
			if (mHandle) 
				_destoryVegAreaHandle();
		}

		VegetationArea* mHandle = nullptr;
		VegAreaCompTestHelper mTestHelper;

		inline static uint64 sVegCrtIndex = 0ul;
	};

	DECLAREACTORCOMTYPE_CPP(VegetationAreaComponent)

		VegetationAreaComponent::VegetationAreaComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type)
		, _dp(new _Private)
	{ 
		_dp->mTestHelper.setHandle(this);
	};

	VegetationAreaComponent::~VegetationAreaComponent()
	{

	};

	void VegetationAreaComponent::init(ActorComponentInfo* info)
	{
		SetNotifiedUpdate(true);	
		m_pActComInfo = info;
	}

	void VegetationAreaComponent::unInit()
	{
		_dp->_destoryVegAreaHandle();
	}

	void VegetationAreaComponent::connectActor()
	{
		const VegAreaCompInfo* vegAreaCompInfo = static_cast<VegetationAreaComponentInfo*>(m_pActComInfo);
		if (vegAreaCompInfo == nullptr)
		{
			LOG_C(std::string("Info is null! Name:") + getCombinedName());
			return;
		}

		if (vegAreaCompInfo->mAreaInfo.points.size() < 3)
		{
			LOG_C(std::string("A valud area contains "
				"3 points at least!, Comp Name : ") + getCombinedName());
			return;
		}

		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject)) return;

		if (_dp->mHandle == nullptr)
			_dp->_createVegAreaHandle(Name(getCombinedName()), vegAreaCompInfo, _GetWorldMatrix(this));
	}

	VegetationArea* VegetationAreaComponent::getVegHandleForUI()
	{
		return _dp->mHandle;
	}

	void VegetationAreaComponent::changeParamForUI(const _SubVegParam& param)
	{
		if (_dp->mHandle == nullptr || _dp->mHandle->getResource().isNull())
			return;

		_dp->_addSubVegArea(_dp->mHandle->getResource(), param,true);
		_dp->mHandle->updateBox();
		VegOpts::GetVegetationGroup()->needUpdate();
	}

	void VegetationAreaComponent::addParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, size_t index)
	{
		changeParamForUI(subVegParamVec.at(index));
	}

	void VegetationAreaComponent::removeParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, const std::string& layerName)
	{
		_dp->_removeSubVegArea(Name(layerName));
		_dp->mHandle->updateBox();
		VegOpts::GetVegetationGroup()->needUpdate();
	}

	void VegetationAreaComponent::updateParamForUI(const std::vector<VegAreaCompInfo::SubVegParam>& subVegParamVec, const std::string& layerName)
	{
		if (_dp->mHandle == nullptr || _dp->mHandle->getResource().isNull())
			return;

		VegAreaCompInfo::SubVegParam param;
		for (auto& subParam : subVegParamVec)
		{
			if (subParam.layer_name == layerName)
			{
				param = subParam;
				break;
			}
		}
		if (param.layer_name.empty())
			return;

		_dp->_updateSubVegArea(param);
		_dp->mHandle->updateBox();
		VegOpts::GetVegetationGroup()->needUpdate();
	}

	void VegetationAreaComponent::updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask)
	{
		if (inChangeMask & ActorComActiveBits::SHOWENTITY_BIT) {
			auto change = inVal & ActorComActiveBits::SHOWENTITY_BIT;
			setVisible(change);
		}
	}

	void VegetationAreaComponent::visibleStateChanged(bool visibleState)
	{
		if (_dp->mHandle) {
			_dp->mHandle->setEnable(visibleState);
			auto pGroup = VegOpts::GetVegetationGroup();
			if (pGroup)
				pGroup->needUpdate();
		}

	}

	void VegetationAreaComponent::setAreaInfoForUI(const VegAreaCompInfo::AreaInfo& areaInfo)
	{
		VegAreaCompInfo* vegAreaCompInfo = static_cast<VegetationAreaComponentInfo*>(m_pActComInfo);
		if (vegAreaCompInfo == nullptr)
		{
			LOG_C(std::string("Info is null! Name:") + getCombinedName());
			return;
		}
		vegAreaCompInfo->mAreaInfo = areaInfo;

		_dp->_destoryVegAreaHandle();
		if (vegAreaCompInfo->mAreaInfo.points.size() < 3)
			return;

		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject))
			return;

		_dp->_createVegAreaHandle(Name(getCombinedName()), vegAreaCompInfo, _GetWorldMatrix(this));
	}

	void VegetationAreaComponent::setMeshInfoForUI(const VegAreaCompInfo::MeshInfoVec& meshInfoVec)
	{
		VegAreaCompInfo* vegAreaCompInfo = static_cast<VegetationAreaComponentInfo*>(m_pActComInfo);
		if (vegAreaCompInfo == nullptr)
		{
			LOG_C(std::string("Info is null! Name:") + getCombinedName());
			return;
		}
		vegAreaCompInfo->mMeshInfoVec = meshInfoVec;

		_dp->_destoryVegAreaHandle();
		if (vegAreaCompInfo->mAreaInfo.points.size() < 3)
			return;

		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject)) return;
		_dp->_createVegAreaHandle(Name(getCombinedName()), vegAreaCompInfo, _GetWorldMatrix(this));
	}

	void VegetationAreaComponent::NotifiChildImp(int changeMask)
	{
		if (nullptr == _dp || nullptr == _dp->mHandle || nullptr == m_pActComInfo)
			return;

		if (isActComNotInitMaskEnable(ActComNotInitMask_RenderObject))
			return;

		const VegAreaCompInfo* vegAreaCompInfo = static_cast<VegetationAreaComponentInfo*>(m_pActComInfo);
		if (vegAreaCompInfo == nullptr)  return;

		_dp->mHandle->setTransform(_GetWorldMatrix(this));
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			_dp->mHandle->clear(false);

		_dp->mTestHelper.update();
	}

	bool VegetationAreaComponent::getAreaDesc(std::vector<Vector3>& out_area, Vector3& out_dir, Vector2& out_range) const
	{
		const VegAreaCompInfo* vegAreaCompInfo = static_cast<VegetationAreaComponentInfo*>(m_pActComInfo);
		if (vegAreaCompInfo == nullptr) return false;
		if (vegAreaCompInfo->mAreaInfo.points.size() < 3) return false;

		out_area = vegAreaCompInfo->mAreaInfo.points;
		const Matrix4 worldMat = _GetWorldMatrix(this);

		for (Vector3& p : out_area)
			p = worldMat * p;
		out_dir = (_GetNormalMat(worldMat) * vegAreaCompInfo->mAreaInfo.dir).GetNormalized();
		out_range = vegAreaCompInfo->mAreaInfo.range;

		return true;
	}

	VegAreaCompTestHelper& VegetationAreaComponent::getTestHelper()const
	{
		return _dp->mTestHelper;
	}

	///////////////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////VegetationAreaComponentFactory///////////////////////
	VegetationAreaComponentFactory::VegetationAreaComponentFactory()
		:ActorComFactory()
	{
		mType = Echo::Name("VegetationAreaComponent");
		mMoveableType = UNMOVEABLE;
		bIsEditorCom = true;
		mPathGizmoIcon = Echo::Name("VegetationAreaComponent");


		auto&& pcm = ProtocolComponentSystem::instance();
		if (pcm)
			pcm->registerProtocolExtFunc(Name("EnableVegetationAreaCom"),
				ProCom_EnableImposter);
	}

	VegetationAreaComponentFactory::~VegetationAreaComponentFactory()
	{
		ProtocolComponentSystem::instance()
			->unregisterProtocolExtFunc(Name("EnableVegetationAreaCom"));
	}

	Echo::ProtocolResult VegetationAreaComponentFactory::
		ProCom_EnableImposter(void* enable, Echo::ProtocolHandle handle)
	{
		return PR_OK;
	}
	///////////////////////////////////////////////////////////////////////////////////

	namespace
	{
		Matrix3 _GetNormalMat(const Matrix4& mat)
		{
			Matrix3 ret = Matrix3::IDENTITY;
			mat.inverse().transpose().extract3x3Matrix(ret);
			return ret;
		}
	}
}
