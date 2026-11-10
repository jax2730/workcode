#pragma once
//-----------------------------------------------------------------------------
// File:  EchoPavementObject.h
//
// Author: LangGuangyue
//
// Date:   2024-04-01
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoPavementObject_H_
#define _EchoPavementObject_H_

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoMaterial.h"
#include "EchoMesh.h"
#include "EchoMovableObject.h"
#include "EchoRenderable.h"
#include "EchoPavementComponent.h"
#include "EchoSphericalTerrainManager.h"

namespace Echo
{
	class PavementComponent;
	class PavementListener;
	class PartPavement;
	class Pavement;
	class PavementObject;
	namespace PavementTool
	{
		struct PrivateMeshData;
		struct PavementPhyMeshData;
	};

	//===============================================================================================================================

	class PlanetLoadListener : public SphericalTerrain::LoadListener
	{
	public:
		PlanetLoadListener(std::weak_ptr<Pavement> weakpavement);
		virtual ~PlanetLoadListener();
	public:
		virtual void OnCreateFinish() override;
		void OnDestroy() override;
	public:
		std::weak_ptr<Pavement> mWeakPavement;
		bool sync = false;
		SphericalTerrain* mPlanet = nullptr;
		PavementComponent::UpdateType mUpdateType = PavementComponent::UpdateRender;
	};
	struct BezierDataCache
	{
		/*	Cubic Bezier curves
		*
		*		  1    2
		*		 /      \
		*		0	     3
		*/
		Vector3		mControllerPoint[4];
		Vector3		mControllerNormal[2];//only point 0 and 3 have normal
		int			mControllerFitGroundLevel[2];//only point 0 and 3 have fit ground model level
		float		AxisXAngle[2];//only point 0 and 3 rotation angle around the X-axis
		float		mLength = 0.0f;//length of the bezier curves

		int			mType = 2;	// 2 依赖曲线的法线方向朝上
								// 1 只依赖世界坐标的y轴朝上
								// 0 根据曲线坐标进行平铺
	};

	struct PavementDesc
	{
		Name pavementName;
		PavementComponent* parent = nullptr;
		BezierDataCache bezierdata;

		Name meshName;

		Vector3 position = Vector3::ZERO;//模型生成相对于曲线本身在局部空间下的位置偏移（由于生成方向默认为X轴正方向，这里一般只在Y轴与Z轴方向进行偏移）
		Quaternion rotation = Quaternion::IDENTITY;//由于道路默认生成方向为局部空间下X轴的正方向，因此有部分模型需要调整自身旋转来适应平铺方向
		Vector3 scale = Vector3::UNIT_SCALE;//模型基础缩放（可能会影响平铺效果，一般不做调整）

		bool synchronous = false;
		PavementComponent::UpdateType updateType = PavementComponent::UpdateRender;

		float renderDistance = 1000.0f;
		float lod1_distance = 200.0f;
		float lod2_distance = 400.0f;

		float segmentLength = 200.0f;//单位分段长度
		float spacingStep = 0.0f;//模型间隔生成距离

		bool m_IsCurveModel = true;//间隔生成模型是否随曲线扭曲，不扭曲时使用静态合批
		bool m_IsPierModel = false;//间隔生成模型不随曲线扭曲时，是否按路墩的形式动态计算向 法线/局部空间Y轴 负方向延伸生成

		bool isFitGround = false;//贴地模式

		uint32  m_surfaceType = 0;//物理表面类型
	};

	struct PartPavementRenderData
	{
		struct Vertex
		{
			Vector3 position;
			Vector3 normal;
			Vector4 tangent;
			Vector2 uv;
		};
		std::vector<Vertex>		vertex;
		std::vector<uint16>		index;

		//ECHO_CULL_MODE		cullMode = Echo::ECHO_CULL_BACK;
		MaterialWrapper			material;
	};

	//===============================================================================================================================

	//实例对象
	using WholePavement = std::vector<PartPavement*>;//单个分段模型，WholePavement对应Mesh，PartPavement对应SubMesh
	using PavementLodVec = std::vector<WholePavement>;//分段lod.  std::Vector<> Index for Lod level
	using PavementVec = std::vector<PavementObject*>;//按固定距离一条贝塞尔曲线划分多段路面

	//组织渲染数据
	//vector层级以及下标(索引)关系同上，为减少内存占用，实例不存具体数据，在需要更新的时候临时创建，用完即销毁
	using WholePavementRenderData = std::vector<PartPavementRenderData>;
	using PavementLodRenderData = std::vector<WholePavementRenderData>;
	using PavementRenderData = std::vector<PavementLodRenderData>;

	//物理对象顶点数据
	using PavementPhyData = std::vector<PavementTool::PavementPhyMeshData>;

	//异步请求ID
	using PavementRequestID = unsigned long long int;

	//===============================================================================================================================

	class _EchoComponentExport PavementListener
	{
		friend class Pavement;
	public:
		virtual ~PavementListener();

		virtual void OnPavementCreateFinish() {};
	protected:
		std::weak_ptr<Pavement> mWeakPavement;
	};

	//===============================================================================================================================
	class _EchoComponentExport PartPavement : public Renderable
	{
		friend class PavementObject;
	protected:
		PartPavement(PavementObject* pavementObj);
		~PartPavement();
	public:
		virtual RenderOperation*	getRenderOperation() override;
		// virtual const Material*		getMaterial(void) const override;
		virtual const MaterialWrapper&     getMaterialWrapper() const override;

		virtual void	getWorldTransforms(Echo::DBMatrix4* xform) const override;
		virtual void    getInvWorldTransforms(const DBMatrix4** xform) const override;
		virtual void	getWorldTransforms(const Echo::DBMatrix4** xform) const override;

		virtual void getRenderableWorldPosition(const DBVector3** pos) const override;
		virtual void getRenderableWorldScale(const Vector3** scale) const override;
		virtual void getRenderableWorldOrientation(const Quaternion** ori) const override;

		virtual const UNIFORM_MAP* getCustomMaterialUniformParams() override { return m_ptrCustomUniformParams; };

		void    setCustomMaterialUniformParams(uint16 uniformIndex, const Echo::Vector4& val);

	protected:
		void		updateRender(const PartPavementRenderData& data, const DBVector3& localOrigin);

		//void		setCullMode(ECHO_CULL_MODE mode);
		//void		setMaterial(Echo::Material* material);

		//void		touchMaterial();
	protected:
		PavementObject*		mParent = nullptr;

		RenderOperation*	mRenderOperation = nullptr;
		MaterialWrapper		mMaterial;

		ECHO_CULL_MODE		mCullMode = Echo::ECHO_CULL_BACK;

		String			mVSDX;
		String			mPSDX;

		UNIFORM_MAP* m_ptrCustomUniformParams = nullptr;
		DBVector3		mLocalOrigin = DBVector3::ZERO;		// 局部坐标系原点（世界空间）
		mutable DBMatrix4	mWorldTransformCache;
	};

	//===============================================================================================================================
	class _EchoComponentExport PavementObject : public MovableObject
	{
	public:
		PavementObject(std::weak_ptr<Pavement> weakpavement);
		~PavementObject();
	public:
		void	update(const PavementLodRenderData& data);

		void	setRenderDistance(float distance);
		void	setLodDistance(EM_LOD_LEVEL lodlevel, float distance);

	public:
		virtual MovableObjectType		getMovableType() const override;
		virtual const AxisAlignedBox&	getBoundingBox() const override;
		virtual float	getBoundingRadius(void) const override;
		virtual void	_notifyCurrentCamera(Camera* cam) override;
		virtual void	updateRenderQueue(RenderQueue* pRenderQueue) override;

		virtual bool	getCulled() override { return mCulled; };
	protected:
		std::weak_ptr<Pavement> mWeakPavement;
		PavementLodVec	mPavementLodVec;

		bool			mCulled = false;
		float			mRenderDistance = 1000.0f;
		float           m_fLodDistance[LOD_LEVEL_NUM];
		EM_LOD_LEVEL	mCurrLodLevel = LOD_LEVEL_0;

		AxisAlignedBox	mBounds;
	};

	//===============================================================================================================================
	class _EchoComponentExport PavementWeakWrapper
	{
	public:
		std::weak_ptr<Pavement> mWeakPavement;

		PavementWeakWrapper() = default;
		explicit PavementWeakWrapper(std::weak_ptr<Pavement> w)
			:mWeakPavement(std::move(w)) {}

		friend std::ostream& operator<<(std::ostream& os, const PavementWeakWrapper&)
		{
			return os << "[PavementWeakWrapper]";
		}
	};

	class _EchoComponentExport Pavement : public std::enable_shared_from_this<Pavement>
	{
		friend class RoadHelper;
		friend class PlanetLoadListener;
	public:
		Pavement(const PavementDesc& desc);
		~Pavement();

		struct Transform
		{
			Vector3 position = Vector3::ZERO;
			Quaternion rotation = Quaternion::IDENTITY;
			Vector3 scale = Vector3::UNIT_SCALE;
		};
		using TransformVec = std::vector<Pavement::Transform>;
	public:
		void	setName(const Name& name);
		//道路更新接口
		void	update(bool sync = false, PavementComponent::UpdateType updateType = PavementComponent::UpdateRender);

		//影响渲染/物理生成计算的参数修改
		//数据设置接口不会自动更新道路，需要手动调用上面的update接口更新
		//返回true为设置成功，false为当前异步线程有任务需要处理，需要访问数据，不可修改
		bool	setPavementDesc(const PavementDesc& desc);
		bool	setBaseMesh(const Name& meshName);

		bool	setMeshOffset(const Vector3& pos);
		bool	setMeshQuat(const Quaternion& quat);
		bool	setMeshScale(const Vector3& scale);
		bool	setBaseMeshTransform(const Pavement::Transform& transform);

		bool	setBezierData(const BezierDataCache& data);
		bool	setBezierUpdateType(int updateType);
		bool	setPartLength(float distance);
		bool	setPartitionDistance(float distance);

		bool	setStepLength(float distance);
		bool	setCurveModel(bool isCurveModel);
		bool	setPierModel(bool isPierModel);

		//贴地测试，贴合地形星球
		bool	setFitGroundModel(bool isFitGroundModel);

	//实例数据参数修改
	public:
		void	setRenderDistance(float distance);
		void	setLodDistance(EM_LOD_LEVEL lodlevel, float distance);
	public:
		void	addPavementListener(PavementListener* listener);
		void	removePavementListener(PavementListener* listener);

	//扭曲模型
	protected:
		//创建渲染VBIB的计算缓存数据
		void	CalculateRenderVBIB(const std::shared_ptr<PavementTool::PrivateMeshData>& data);
		//创建物理VBIB的计算缓存数据
		void	CalculatePhyVBIB(const std::shared_ptr<PavementTool::PrivateMeshData>& data);

	//非扭曲模型
	protected:
		void	ResetBacthObject(const MeshPtr& mesh);
		//计算模型在曲线上间隔生成的位置
		void	CalculateObjectTransform();

	protected:
		//根据计算的VBIB缓存数据更新渲染对象。结束时会清除临时缓存的渲染和物理数据。
		//非扭曲计算模型则创建/更新合批渲染对象
		void	updateObject();
		//异步更新回调
		void	CalculateOperationCompleted(PavementRequestID tickID, uint16 updateType);

		//用于异步计算收集贴地道路顶点，更新星球贴地掩码供植被使用
		//void	CalculateFitGroundVBIB(SphericalTerrain* mPlanet, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);
	public:
		const Name&			getName() { return mName; };
		PavementComponent*	getPavementComponent() { return mParent; };

		const Name&			getMeshName() { return mBaseMeshName; };
		const Vector3&		getMeshOffset() { return mBaseMeshTransform.position; };
		const Quaternion&	getMeshQuat() { return mBaseMeshTransform.rotation; };
		const Vector3&		getMeshScale() { return mBaseMeshTransform.scale; };

		float				getRenderDistance() { return mRenderDistance; };
		float				getLodDistance(EM_LOD_LEVEL lodlevel) { return m_fLodDistance[lodlevel]; };


		float				getPartLength() { return m_partitionDistance; };
		float				getSpacingStep() { return m_spacingStep; };

		bool				getIsCurveModel() { return mCurveModel; };
		bool				getIsPierModel() { return mPierModel; };

		bool				getIsFitGroundModel() { return mFitGroundModel; };

		//渲染数据缓存会在更新完渲染实例后清空
		const PavementRenderData&	getPavementRenderDataCache() { assert(m_setRenderTickets.empty()); return mRenderData; };
		const PavementPhyData&		getPavementPhyDataCache() { assert(m_setPhysicsTickets.empty()); return mPhysicsData; };
		const TransformVec&			getPavementTransformDataCache() { assert(m_setPhysicsTickets.empty() && m_setRenderTickets.empty()); return mTransformData; };
		bool						getIsCalculating() { return !m_setRenderTickets.empty() || !m_setPhysicsTickets.empty(); };
		bool						getIsRenderCalculating() { return !m_setRenderTickets.empty(); };
		bool						getIsPhysicCalculating() { return !m_setPhysicsTickets.empty(); };
		void						updatePavement();	//手动更新pavement,避免在构造阶段更新导致提交异步的weak_ptr绑定不上
		void						abortAllRequests();
	protected:
		Name						mName = Name::g_cEmptyName;
		PavementComponent*			mParent = nullptr;
		BezierDataCache				mBezierData;

		Name						mBaseMeshName;
		Pavement::Transform			mBaseMeshTransform;

		float						mRenderDistance = 1000.0f;
		float						m_fLodDistance[LOD_LEVEL_NUM];

		float						m_partitionDistance = 200.0f;//分段渲染单位长度
		float						m_spacingStep = 0.0f;//模型间隔生成距离

		bool						mCurveModel = true;//间隔生成模型是否随曲线扭曲，不扭曲时使用静态合批
		bool						mPierModel = false;//间隔生成模型不随曲线扭曲时，是否按路墩的形式动态计算向 法线/局部空间Y轴 负方向延伸生成

		bool						mFitGroundModel = false;//地形星球贴地模式，会自动寻找最近的星球贴合地面

		//最后的保险机制！！！未创建完成的星球有无效数据，上层创建理应保证贴地的星球已经创建完成
		PlanetLoadListener*			mPlanetListener = nullptr;//星球加载回调，防止依据星球贴地时，星球未创建完

		//道路模型实例
		PavementVec					mPavementVec;//扭曲模型
		TrafficBatchObject*			mBatchingObject = nullptr;//间隔生成不随曲线扭曲时使用静态合批

		//随曲线扭曲
		PavementRenderData			mRenderData;//创建渲染实例使用的数据，用完即清
		PavementPhyData				mPhysicsData;//创建物理组件使用的数据，用完即清

		//不随曲线扭曲
		TransformVec				mTransformData;//创建渲染/物理实例对应在曲线上的位置

		std::set<PavementRequestID>	m_setRenderTickets;
		std::set<PavementRequestID>	m_setPhysicsTickets;

		std::set<PavementListener*>	mListeners;

		bool						isNeedUpdate = false;//只有在影响渲染/物理更新的参数被修改时才会更新，减少不必要的更新计算
	public:
		uint32						mPhySurfaceType = 0;//物理表面类型
		PavementDesc				mUpdateDescCache;	//缓存一次数据保证updatePavement用到
	};
}
#endif
