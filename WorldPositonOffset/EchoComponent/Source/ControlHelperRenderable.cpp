///////////////////////////////////////////////////////////////////////////////
//
// @file ControlHelperRenderable.cpp
// @author luoshuquan
// @date 2022/07/22 Friday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
#include "EchoComponentStableHeaders.h"
#include "ControlHelperRenderable.h"

#include "EchoMaterial.h"
#include "EchoShaderContentManager.h"
#include "EchoSceneManager.h"

namespace EchoBox
{
	using namespace Echo;

	class HelperRenderable : public Echo::MovableObject, public Echo::Renderable
	{
	public:
		HelperRenderable(Echo::SceneManager* sMgr);
		virtual ~HelperRenderable();

		SceneNode* GetSceneNode() { return mNode; }
		//void setPosition(const Echo::Vector3& pos);
		//const Echo::Vector3& getPosition() const
		//{ return mPosition; }

		//void setScale(float scale);

		//void setHightlight(bool hightlight);

		//std::pair<bool, float> hitTest(const Echo::Ray& ray);
		//
		void setRenderOperation(const RenderOperation & render_op)
		{ mRenderOp = render_op; }
		void setBounds(const AxisAlignedBox & aabb)
		{ mBounds = aabb; }

		// overload from Echo::MovableObject
		virtual Echo::MovableObjectType getMovableType() const override;
		virtual const Echo::AxisAlignedBox& getBoundingBox() const override;
		virtual float getBoundingRadius() const override;
		virtual void updateRenderQueue(Echo::RenderQueue* pRenderQueue) override;

		// overload from Echo::Renderable
		virtual void getWorldTransforms(Echo::DBMatrix4* xform) const override;
		virtual void getWorldTransforms(const Echo::DBMatrix4** xform) const override;
		virtual Echo::RenderOperation* getRenderOperation() override;
		// virtual const Echo::Material* getMaterial() const override;
		const MaterialWrapper& getMaterialWrapper() const override;

	public:
		bool mIsVisible;
		Echo::RenderOperation mRenderOp;
		Echo::AxisAlignedBox mBounds;
		Echo::Material* mMaterial;
		Echo::SceneNode* mNode;
		Echo::Vector3 mPosition;
		//bool mHightlight;
		static const Echo::Vector3 CubeVertex[8];
		static const Echo::uint16 CubeIndex[36];
	};

	HelperRenderable::HelperRenderable(Echo::SceneManager* sMgr) : mIsVisible(true)
	{
		mMaterial = new Echo::Material(NULL, Echo::Name(""));
		ECHO_IA_DESC& iaDesc = mMaterial->m_desc.IADesc;
		iaDesc.ElementArray[0].nStream = 0;
		iaDesc.ElementArray[0].nOffset = 0;
		iaDesc.ElementArray[0].eType = ECHO_FLOAT3;
		iaDesc.nElementCount = 1;

		iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		iaDesc.StreamArray[0].nRate = 1;
		iaDesc.StreamArray[0].nStride = sizeof(float) * 3;
		iaDesc.nStreamCount = 1;

		mMaterial->getDiffuseColor() = Echo::ColorValue::White;
		mMaterial->m_desc.RenderState.DepthTestEnable = ECHO_FALSE;
		mMaterial->m_desc.RenderState.BlendEnable = ECHO_TRUE;
		mMaterial->m_desc.RenderState.BlendSrc = ECHO_BLEND_SRCALPHA;
		mMaterial->m_desc.RenderState.BlendDst = ECHO_BLEND_INVSRCALPHA;
		mMaterial->m_desc.RenderState.BlendOp = ECHO_BLEND_OP_ADD;

		std::string vsShader, psShader;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Helper_VS.txt"), vsShader);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("Helper_PS.txt"), psShader);
		mMaterial->setShaderName("Helper_VS.txt", "Helper_PS.txt");

		psShader.insert(0, "#define MATERIAL_DIFFUSE\n\n");

		mMaterial->setShaderSrcCode(vsShader, psShader);
		mMaterial->createPass(NormalPass);
		mMaterial->loadInfo();

		mBounds.setNull();

		char szNodeName[64] = { 0 };
		snprintf(szNodeName, 64, "HelperRenderable/%p", this);
		mNode = sMgr->createSceneNode(Name(szNodeName));;
		mNode->attachObject(this);
	}

	HelperRenderable::~HelperRenderable()
	{
		mNode->detachAllObject();
		//mNode->getParentSceneNode()->destroyChild(mNode);
		mNode->getCreator()->destroySceneNode(mNode);
		mNode = 0;

		delete mMaterial;
		mMaterial = 0;
	}

	Echo::MovableObjectType HelperRenderable::getMovableType() const
	{
		return Echo::MovableObjectType_SimpleRenderable;
	}

	const Echo::AxisAlignedBox& HelperRenderable::getBoundingBox() const
	{
		return mBounds;
	}

	float HelperRenderable::getBoundingRadius() const
	{
		return Math::boundingRadiusFromAABB(mBounds);
	}

	void HelperRenderable::updateRenderQueue(Echo::RenderQueue* pRenderQueue)
	{
		if (mRenderOp.GetMeshData().m_vertexCount != 0 && mIsVisible)
		{
			RenderQueueGroup* pGroup = pRenderQueue->getRenderQeueuGroup(RenderQueue_EditorHelp);
			pGroup->addRenderable(mMaterial->getPass(Echo::NormalPass), this);
		}
	}

	void HelperRenderable::getWorldTransforms(Echo::DBMatrix4* xform) const
	{
		*xform = *getWorldMatrix();
	}

	void HelperRenderable::getWorldTransforms(const Echo::DBMatrix4** xform) const
	{
		*xform = getWorldMatrix();
	}

	Echo::RenderOperation* HelperRenderable::getRenderOperation()
	{
		return &mRenderOp;
	}

	// const Echo::Material* HelperRenderable::getMaterial() const
	// {
	// 	return mMaterial;
	// }

	const MaterialWrapper& HelperRenderable::getMaterialWrapper() const
	{
		if (m_materialWrapper.getV1() != mMaterial)
		{
			m_materialWrapper = MaterialWrapper::fromPointer(mMaterial);
		}
		return m_materialWrapper;
	}

	///////////////////////////////////////////////////////////////////////////////

	ControlHelperRenderable::ControlHelperRenderable(Echo::SceneManager* sMgr)
	{
		mRenderable = new HelperRenderable(sMgr);
	}

	ControlHelperRenderable::~ControlHelperRenderable()
	{
		delete mRenderable;
		mRenderable = nullptr;
	}

	SceneNode* ControlHelperRenderable::GetSceneNode()
	{
		return mRenderable->GetSceneNode();
	}

	const Echo::DVector3& ControlHelperRenderable::getWorldPosition()
	{
		return mRenderable->mNode->_getDerivedPosition();
	}

	void ControlHelperRenderable::setPosition(const Echo::Vector3 & position)
	{
		mPosition = position;
		if (mRenderable->mNode != nullptr) {
			mRenderable->mNode->setPosition(position);
		}
	}

	void ControlHelperRenderable::setScale(float scale)
	{
		mScale = scale;
		if (mRenderable->mNode != nullptr) {
			mRenderable->mNode->setScale(scale, scale, scale);
		}
	}

	void ControlHelperRenderable::setColor(const Echo::ColorValue & color)
	{
		mColor = color;
		if (mRenderable->mMaterial != nullptr) {
			mRenderable->mMaterial->getDiffuseColor() = color;
		}
	}

	void ControlHelperRenderable::setHighlight(bool highlight)
	{
		mHighlight = highlight;
		if (mRenderable->mMaterial != nullptr)
		{
			if (mHighlight)
			{
				Echo::ColorValue highlight_color = Echo::ColorValue(0.4f, 0.4f, 0.f, 0.f) + mColor * 0.6f;
				highlight_color.a = mColor.a;
				mRenderable->mMaterial->getDiffuseColor() = highlight_color;
			}
			else {
				mRenderable->mMaterial->getDiffuseColor() = mColor;
			}
		}
	}

	void ControlHelperRenderable::setVisible(bool value)
	{
		mRenderable->mIsVisible = value;
	}

	bool ControlHelperRenderable::getVisible()
	{
		return mRenderable->mIsVisible;
	}

	///////////////////////////////////////////////////////////////////////////////
	// BoxHelper
	BoxHelper::BoxHelper(Echo::SceneManager* sMgr)
		: ControlHelperRenderable(sMgr)
	{
	}

	BoxHelper::~BoxHelper()
	{
	}

	namespace
	{
		/*
			   4-----7
			  /|    /|
			 5-----6 |
			 | |   | |
			 | 0---|-3
			 |/    |/
			 1-----2
		*/
		const uint16 BoxIndex[36] =
		{
			0, 3, 2, 0, 2, 1,	// bottom
			4, 5, 6, 4, 6, 7,	// top
			0, 1, 5, 0, 5, 4,	// left
			1, 2, 6, 1, 6, 5,	// front
			2, 3, 7, 2, 7, 6,	// right
			3, 0, 4, 3, 4, 7	// back
		};
	}

	std::pair<bool, float> BoxHelper::hitTest(const Echo::Ray & ray) const
	{
		Ray local_ray = ray;
		if (mRenderable->mNode != nullptr)
		{
			const DBVector3 & position = mRenderable->mNode->_getDerivedPosition();
			const Vector3 & scale = mRenderable->mNode->getScale();
			const Quaternion & orientation = mRenderable->mNode->getOrientation();

			DMatrix4 inv_trans;
			inv_trans.makeInverseTransform(position, scale, orientation);

			local_ray.setOrigin(inv_trans * local_ray.getOrigin());

			Vector4 dir(local_ray.getDirection());
			dir.w = 0.f;
			dir = inv_trans * dir;
			local_ray.setDirection(Vector3(dir.x, dir.y, dir.z));
		}

		bool hit = false;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (int i=0; i<36; i+=3)
		{
			std::pair<bool, float> res = Math::intersects(local_ray,
					mVertices[BoxIndex[i]], mVertices[BoxIndex[i+1]], mVertices[BoxIndex[i+2]], true, false);
			if (res.first && closest_distance > res.second)
			{
				closest_distance = res.second;
				hit = true;
			}
		}

		return std::make_pair(hit, closest_distance);
	}

	void BoxHelper::setBox(const Echo::Vector3 box[8])
	{
		memcpy(mVertices, box, sizeof(mVertices));

		mRenderable->mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
		mRenderable->mRenderOp.GetMeshData().m_vertexCount = 8;
		RcBuffer* pVB = Root::instance()->getRenderSystem()->
			createStaticVB(mVertices, sizeof(mVertices));
		mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.push_back(pVB);

		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexCount = sizeof(BoxIndex) / sizeof(BoxIndex[0]);
		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexBuffer = Root::instance()->getRenderSystem()->
			createStaticIB(BoxIndex, sizeof(BoxIndex));

		mRenderable->mBounds.setNull();
		for (int i=0; i<8; ++i)
			mRenderable->mBounds.merge(mVertices[i]);
	}

	void BoxHelper::setBox(float x, float y, float z,
			const Echo::Vector3 & center /* = Echo::Vector3::ZERO */)
	{
		float half_x = 0.5f * x,
			  half_y = 0.5f * y,
			  half_z = 0.5f * z;
		Vector3 box[] =
		{
			center + Vector3(-half_x, -half_y, -half_z),
			center + Vector3(-half_x, -half_y,  half_z),
			center + Vector3( half_x, -half_y,  half_z),
			center + Vector3( half_x, -half_y, -half_z),
			center + Vector3(-half_x,  half_y, -half_z),
			center + Vector3(-half_x,  half_y,  half_z),
			center + Vector3( half_x,  half_y,  half_z),
			center + Vector3( half_x,  half_y, -half_z),
		};
		setBox(box);
	}

	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	//
	SphereHelper::SphereHelper(Echo::SceneManager* sMgr)
		: ControlHelperRenderable(sMgr)
	{
	}

	SphereHelper::~SphereHelper()
	{
	}

	bool SphereHelper::isInitData()
	{
		bool result = true;
		result &= (mPosition == Vector3::ZERO);
		result &= (mColor == ColorValue::White);
		//result &= (mHighlight == false);
		result &= (mRadius == 0.5f);
		return result;
	}

	void SphereHelper::setRadius(float radius)
	{
		mRadius = radius;

		int tell = (int)std::ceil(std::powf(radius, 1.f / 3.f) * 8.f);
		tell = Math::Clamp(tell, 1, 32);
		int longitude_tell = tell * 4,
			latitude_tell = tell * 2;

		float longitude_div = Math::TWO_PI / longitude_tell;
		float latitude_div = Math::PI / latitude_tell;
		float latitude_step = Math::HALF_PI;
		std::vector<Vector3> vertices;
		vertices.reserve((longitude_tell + 1) * (latitude_tell + 1));
		for (int la=0; la<=latitude_tell; ++la, latitude_step-=latitude_div)
		{
			Vector3 vertex(0.f, Math::Sin(Radian(latitude_step)) * radius, 0.f);
			float longitude_step = 0.f;
			float latitude_radius = Math::Cos(Radian(latitude_step)) * radius;
			for (int lo=0; lo<=longitude_tell; ++lo, longitude_step+=longitude_div)
			{
				vertex.x = Math::Cos(Radian(longitude_step)) * latitude_radius;
				vertex.z = Math::Sin(Radian(longitude_step)) * latitude_radius;
				vertices.push_back(vertex);
			}
		}

		std::vector<unsigned short> indices;
		indices.reserve(longitude_tell * latitude_tell * 2 * 3);
		for (int la=0; la<latitude_tell; ++la)
		{
			for (int lo=0; lo<longitude_tell; ++lo)
			{
				/*
					0 - 3
					| \ |
					1 - 2
				*/
				int net_idxs[] = {
					la * (longitude_tell + 1) + lo,
					(la + 1) * (longitude_tell + 1) + lo,
					(la + 1) * (longitude_tell + 1) + lo + 1,
					la * (longitude_tell + 1) + lo + 1,
				};
				indices.push_back(net_idxs[0]);
				indices.push_back(net_idxs[1]);
				indices.push_back(net_idxs[2]);
				indices.push_back(net_idxs[0]);
				indices.push_back(net_idxs[2]);
				indices.push_back(net_idxs[3]);
			}
		}

		mRenderable->mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
		mRenderable->mRenderOp.GetMeshData().m_vertexCount = (unsigned)vertices.size();
		RcBuffer * _vb = Root::instance()->getRenderSystem()->
			createStaticVB(vertices.data(), (unsigned)(vertices.size() * sizeof(Vector3)));
		mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.push_back(_vb);

		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexCount = (unsigned)indices.size();
		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexBuffer =
			Root::instance()->getRenderSystem()->createStaticIB(indices.data(),
			(unsigned)(indices.size() * sizeof(unsigned short)));

		mRenderable->mBounds.setNull();
		mRenderable->mBounds.setExtents(Vector3(-radius, -radius, -radius),
				Vector3(radius, radius, radius));
	}

	std::pair<bool, float> SphereHelper::hitTest(const Ray & ray) const
	{
		Ray local_ray = ray;
		if (mRenderable->mNode != nullptr)
		{
			const DBVector3 & position = mRenderable->mNode->_getDerivedPosition();
			const Vector3 & scale = mRenderable->mNode->getScale();
			const Quaternion & orientation = mRenderable->mNode->getOrientation();

			DMatrix4 inv_trans;
			inv_trans.makeInverseTransform(position, scale, orientation);

			local_ray.setOrigin(inv_trans * local_ray.getOrigin());

			Vector4 dir(local_ray.getDirection());
			dir.w = 0.f;
			dir = inv_trans * dir;
			local_ray.setDirection(Vector3(dir.x, dir.y, dir.z));
		}

		return Math::intersects(local_ray, Sphere(Vector3::ZERO, mRadius));
	}

	ArrowHelper::ArrowHelper(Echo::SceneManager* sMgr)
		: ControlHelperRenderable(sMgr)
	{
		setVisible(false);
	}

	ArrowHelper::~ArrowHelper()
	{

	}

	bool ArrowHelper::isInitData()
	{
		bool result = true;
		result &= (mPosition == Vector3::ZERO);
		result &= (mColor == ColorValue::White);
		//result &= (mHighlight == false);
		result &= (mRadius == 0.5f);
		return result;
	}

	void ArrowHelper::setRadius(float radius)
	{
		mRadius = radius;

		int LineVertexCount = 26;
		int LineIndexCount = 132;
		float LineWidth = 0.05f;
		int VertexCount = LineVertexCount;
		int IndexCount = LineIndexCount;
		float LineLength = radius * 3.f;
		float beginLen = 0.25f;
		int vtxCount = 0, idxCount = 0;
		float MPI_2 = Math::TWO_PI;
		int CircleDivide = 16;

		Matrix3 mat;
		mat.SetFromToRotation(Vector3::UNIT_X, mNormal);

		mVertices[vtxCount++] = mat * Vector3(beginLen, -LineWidth, -LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen, -LineWidth, LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen, LineWidth, LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen, LineWidth, -LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, -LineWidth, -LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, -LineWidth, LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, LineWidth, LineWidth);
		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, LineWidth, -LineWidth);

		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, 0.f, 0.f);		// 前端点
		for (float theta = 0.f; theta < MPI_2; theta += MPI_2 / CircleDivide) {
			mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength, radius * cos(theta), radius * sin(theta));
		}
		mVertices[vtxCount++] = mat * Vector3(beginLen + LineLength + radius, 0.f, 0.f);

		assert(VertexCount == vtxCount);

		std::vector<uint16> indices(IndexCount, 0);

		// 坐标轴直线的索引
		unsigned short indexArray[36] = {
			0u, 1u, 2u, 0u, 2u, 3u,	// left
			4u, 6u, 5u, 2u, 7u, 6u,	// right
			2u, 6u, 7u, 2u, 7u, 3u,	// top
			0u, 4u, 1u, 4u, 5u, 1u,	// bottom
			1u, 5u, 6u, 1u, 6u, 2u,	// front
			0u, 3u, 7u, 0u, 7u, 4u	// back
		};
		memcpy(mIndexArray, indexArray, sizeof(unsigned short) * 36);

		idxCount = 36;

		// 前端的圆
		for (int i = 1; i < CircleDivide; ++i)
		{
			mIndexArray[idxCount++] = 8;
			mIndexArray[idxCount++] = 8 + i + 1;
			mIndexArray[idxCount++] = 8 + i;
		}
		mIndexArray[idxCount++] = 8;
		mIndexArray[idxCount++] = 8 + 1;
		mIndexArray[idxCount++] = 8 + CircleDivide;

		for (int i = 1; i < CircleDivide; ++i)
		{
			mIndexArray[idxCount++] = 8 + CircleDivide + 1;
			mIndexArray[idxCount++] = 8 + i;
			mIndexArray[idxCount++] = 8 + i + 1;
		}
		mIndexArray[idxCount++] = 8 + CircleDivide + 1;
		mIndexArray[idxCount++] = 8 + CircleDivide;
		mIndexArray[idxCount++] = 8 + 1;

		assert(IndexCount == idxCount);

		for (size_t i = 0; i < LineIndexCount; ++i)
			indices.push_back(mIndexArray[i]);

		mRenderable->mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
		mRenderable->mRenderOp.GetMeshData().m_vertexCount = (unsigned)26;
		RcBuffer* _vb = Root::instance()->getRenderSystem()->
			createStaticVB(mVertices, sizeof(mVertices));
		mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.push_back(_vb);

		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexCount = (unsigned)indices.size();
		mRenderable->mRenderOp.GetMeshData().m_IndexBufferData.m_indexBuffer =
			Root::instance()->getRenderSystem()->createStaticIB(indices.data(),
				(unsigned)(indices.size() * sizeof(unsigned short)));

		mRenderable->mBounds.setNull();
		for (size_t i = 0; i < 26; ++i)
			mRenderable->mBounds.merge(mVertices[i]);
	}

	std::pair<bool, float> ArrowHelper::hitTest(const Echo::Ray& ray) const
	{
		Ray local_ray = ray;

		if (mRenderable->mNode != nullptr)
		{
			const DBVector3& position = mRenderable->mNode->_getDerivedPosition();
			const Vector3& scale = mRenderable->mNode->getScale();
			const Quaternion& orientation = mRenderable->mNode->getOrientation();

			DMatrix4 inv_trans;
			inv_trans.makeInverseTransform(position, scale, orientation);

			local_ray.setOrigin(inv_trans * local_ray.getOrigin());

			Vector4 dir(local_ray.getDirection());
			dir.w = 0.f;
			dir = inv_trans * dir;
			local_ray.setDirection(Vector3(dir.x, dir.y, dir.z));
		}

		bool hit = false;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (int i = 0; i < 132; i += 3)
		{
			std::pair<bool, float> res = Math::intersects(local_ray,
				mVertices[mIndexArray[i]], mVertices[mIndexArray[i + 1]], mVertices[mIndexArray[i + 2]], true, false);
			if (res.first && closest_distance > res.second)
			{
				closest_distance = res.second;
				hit = true;
			}
		}

		return std::make_pair(hit, closest_distance);
	}

	void ArrowHelper::SetNormal(const Echo::Vector3& v)
	{
		if (mRenderable->mNode != nullptr)
		{
			Matrix3 mat;
			if (v + mNormal != Vector3::ZERO)
			{
				Quaternion qu;
				qu.FromAxesToAxes(mNormal, v.IsZero() ? Vector3::UNIT_Y : v);
				qu.ToRotationMatrix(mat);
			}
			else
				mat.SetFromToRotation(mNormal, v);

			for (int i = 0; i < sizeof(mVertices) / sizeof(mVertices[0]); ++i)
			{
				mVertices[i] = mat * mVertices[i];
			}
			mNormal = v.IsZero() ? Vector3::UNIT_Y : v;

			if (!mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.empty())
			{
				Root::instance()->getRenderSystem()->destoryBuffer(mRenderable->mRenderOp.GetMeshData().m_vertexBuffer[0]);
				mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.clear();
			}

			mRenderable->mRenderOp.mPrimitiveType = ECHO_TRIANGLE_LIST;
			mRenderable->mRenderOp.GetMeshData().m_vertexCount = (unsigned)26;
			RcBuffer* _vb = Root::instance()->getRenderSystem()->
				createStaticVB(mVertices, sizeof(mVertices));
			mRenderable->mRenderOp.GetMeshData().m_vertexBuffer.push_back(_vb);

			mRenderable->mBounds.setNull();
			for (size_t i = 0; i < 26; ++i)
				mRenderable->mBounds.merge(mVertices[i]);
		}
	}

	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////

}
