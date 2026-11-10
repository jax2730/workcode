///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedMeshComponent.h
// @author changziheng
// @date 2023/1/16
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __EchoCurvedMeshComponent_H__
#define __EchoCurvedMeshComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "EchoRenderOperation.h"
#include "EchoMeshMgr.h"
#include "EchoTrafficObject.h"
#include "EchoSubMesh.h"
namespace Echo
{
	class PiecewiseBezierComponent;
	
	typedef EchoWeakPtr<PiecewiseBezierComponent> PiecewiseBezierComponentPtr;

	class _EchoComponentExport CurvedMeshComponentInfo : public ActorComponentInfo
	{
	public:
		CurvedMeshComponentInfo() {}
		CurvedMeshComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* info) override;
		virtual void OverWrite() override;

		void setMeshPath(String path) { strMeshPath = path; }
		String getMeshPath() { return strMeshPath; }

		void setComponentBindName(String name) { componentBindName = name; }
		String getComponentBindName() { return componentBindName; }
	public:
		String strMeshPath;
		String componentBindName;

		EDITOR_REFLECT
	};

	class _EchoComponentExport CurvedMeshComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(CurvedMeshComponent)

	public:
		void init(ActorComponentInfo* info) override;
		void connectCom() override;
		void visibleStateChanged(bool visibleState) override;

		void setMeshData(String mesh);

		bool setComponentBindName(String name);

		void update();
		void updatePickRoad(size_t subNum, size_t road_part);
		void calculateCoiledTPoint(size_t subNum, size_t road_part
			, size_t ai, float beginlength, float scale_size
			, Echo::Vector3& point0_normal, Echo::Vector3& point0_tangent
			, Echo::Vector3& point0, Echo::TrafficObject* _object);
		float calculatePointT(float beginlength
			, float scale_size, size_t& part_index, Echo::Vector3 point);
		float calculateBeyondLength(size_t& road_part, float beginlength);

		void calculateNormal(Vector3 Tangent, Vector3& Normal, Vector3& ParNormal);

		void clearCurvedMesh();

		bool initByUI = false;
		virtual void                                    createComRootNodeImpl() override;
	private:
		Vector3 getPoint(MeshVertexIndexData& data, size_t subNum, size_t i, bool isChange = true)
		{
			Vector3 point = data.vertices[i];
			return point;
		}

		PiecewiseBezierComponentPtr mBezier;
		bool bBezierReady;

		MeshPtr mMesh;

		Vector3 mMeshSize;
		Vector3 mRoadNormal;
		Vector3 mRoadTangent;

		std::vector<TrafficObject*> mMeshObjects;
		std::map<float, float> mStoT;
	};

	typedef EchoWeakPtr<CurvedMeshComponent> CurvedMeshComponentPtr;

	class _EchoComponentExport CurvedMeshComponentProFunc
	{
	public:
		static void initProtocolFunction();
		static void uninitProtocolFunction();

		// static ProtocolResult ProCom_ShowTrajectory(void* isShow, ProtocolHandle handle);
	};

	DEFINECOMFACTORY(CurvedMeshComponent, ALL_MOVEABLE)
}

#endif
