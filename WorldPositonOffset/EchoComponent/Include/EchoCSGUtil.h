#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoPlane.h"

//#include "EchoShaderContentManager.h"
#include "EchoRoot.h"
#include "EchoRenderSystem.h"
#include "EchoRenderable.h"

namespace Echo
{
	enum CSGOperation
	{
		eUnion,
		eIntersection,
		eSbstruct,
		eOperationMax,
	};

	struct _EchoComponentExport CSGVertex
	{
		Vector3 mPosition;
		Vector3 mNormal;
		int index = -1;

		CSGVertex(const Vector3& position, const Vector3& normal)
		{
			mPosition = Vector3(position);
			mNormal = Vector3(normal);
		}
	};

	struct _EchoComponentExport  CSGDrawVertex
	{
		Vector3 pos;
		Vector3 normal;
	};

	class _EchoComponentExport  CSGPolygon
	{
	public:
		CSGPolygon();
		CSGPolygon(vector<CSGVertex>::type pVertices);

		~CSGPolygon();
		void flip();
		Echo::vector<CSGVertex>::type& getVertices();
		Plane plane;

	private:
		vector<CSGVertex>::type vertices;
	};

	class _EchoComponentExport CSGPlane :public Echo::Plane
	{
	public:
		CSGPlane(Plane& other);

		void splitPolygon(CSGPolygon& polygon,
			Echo::vector<CSGPolygon>::type& coplanarFront,
			Echo::vector<CSGPolygon>::type& coplanarBack,
			Echo::vector<CSGPolygon>::type& front,
			Echo::vector<CSGPolygon>::type& back);
	};

	//---------------------------csg bsp node------------------------------------------------

	class _EchoComponentExport CSGNode
	{
	public:
		CSGNode(vector<CSGPolygon>::type& polygons);
		CSGNode();
		~CSGNode();

	private:
		CSGPlane* mPlane = nullptr;
		CSGNode* front;
		CSGNode* back;
		vector<CSGPolygon>::type polygons;

	public:
		void invert();
		vector<CSGPolygon>::type& clipPolygons(vector<CSGPolygon>::type& polygons);
		void allPolygons(vector<CSGPolygon>::type& polys);

		void clipTo(CSGNode& node);
		void build(vector<CSGPolygon>::type& polys);
		vector<CSGPolygon>::type& getpolygons();
		void deleteTree(CSGNode* node);
	};

	class _EchoComponentExport CSGBrush : public MovableObject ,public Renderable
	{
	public:
		CSGBrush(vector<CSGPolygon>::type& polygons);
		~CSGBrush();

		vector<CSGPolygon>::type mPolygons;
		static CSGBrush* buildFromPolygons(vector<CSGPolygon>::type& polygons);

		void configRenderInfo();

		CSGBrush* mergeCSGBrush(CSGBrush* brush,CSGOperation operation);
		CSGBrush* unionCSGBrush(CSGBrush& brush);
		CSGBrush* subtractCSGBrush(CSGBrush& brush);
		CSGBrush* intersectCSGBrush(CSGBrush& brush);

		void inverse();
		vector<CSGPolygon>::type& getPolygons();
		void resetRender();
		void generateDrawInfo();

		bool exportToMesh(const Echo::String& meshfilepath, const Echo::String& templateMatPath);
		bool exportToMesh(const Echo::String& meshfilepath, const Echo::String& outerTemplateMatPath,const Echo::String& innerTemplateMatPath);

		void convertToSubMesh(Echo::SubMesh* pSubMesh);

		void setNodePosition(Vector3& pos);

		void updateBoundingBox();

		//overload from Echo::MovableObject
		virtual Echo::MovableObjectType getMovableType() const override;
		virtual const Echo::AxisAlignedBox& getBoundingBox() const override;
		virtual float getBoundingRadius() const override;
		virtual void updateRenderQueue(Echo::RenderQueue* pRenderQueue) override;

		//overload from Echo::Renderable
		virtual void getWorldTransforms(Echo::DBMatrix4* xform) const override;
		virtual void getWorldTransforms(const Echo::DBMatrix4** xform) const override;
		virtual Echo::RenderOperation* getRenderOperation() override;
		// virtual const Echo::Material* getMaterial() const override;
		const MaterialWrapper& getMaterialWrapper() const override;

	private:
		Echo::RenderOperation mRenderOp;
		Echo::AxisAlignedBox mBounds;
		Echo::Material* mMaterial;
		Echo::SceneNode* mNode;

		std::vector<CSGDrawVertex> m_VB;
		std::vector<uint16> m_IB;
	};

	class _EchoComponentExport CSGShape
	{
	public:
		static void makeSphereVertex(vector<CSGVertex>::type& vertex, const Vector3& center, float r, float theta, float phi, const Matrix3& msr, const Matrix3& mr);

		static CSGBrush* sphereCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale,  float radius = 1.0f, float slices = 12.0f, float stacks = 12.0f);
		static CSGBrush* cubeCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale, const Vector3& halfExtents);
		static CSGBrush* cylinderCSG(const Vector3& center, const Vector3& eulangle, const Vector3& scale,  float radius, float halfHeight, float slices = 12.0f);
	};
}


