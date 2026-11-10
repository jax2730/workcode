//-----------------------------------------------------------------------------
// File:   EchoSphericalTerrainPCGGenScheduler.h
//
// Author: qiao_yuzhi
//
// Date:   2025-6-3
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoSphericalTerrainPCGGenScheduler_h__
#define EchoSphericalTerrainPCGGenScheduler_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoPlanetGenObjectScheduler.h"

namespace Echo
{
	class EchoPCGObject;
	class SphericalTerrainPCGGenScheduler;
	class _EchoComponentExport RoadPCGGenScheduler {
	public:
		RoadPCGGenScheduler(SphericalTerrainPCGGenScheduler*);
		~RoadPCGGenScheduler();

		void CreateFromConfig(bool bSync);
		void SetGenObjectInfo(const std::map<int, PCG::BiomeGenObjectLayerInfo>& info, uint32 id);
		void Create(bool bSync, bool bCreate = true);
		void Clear();
		bool ExportGenObjects(const String& file, const char* folderType);
		void ExportGenObjects(std::map<Name, std::vector<PCG::ExportPoint>>&);
	private:
		uint32 mId = 0;
		std::map<int, PCG::BiomeGenObjectLayerInfo> mInfo;
		SphericalTerrainPCGGenScheduler* m_pScheduler = nullptr;
		EchoPCGObject* m_pPCG = nullptr;
	};

	class SphericalTerrainPCGGenSchedulerPrivate;
	class _EchoComponentExport SphericalTerrainPCGGenScheduler : public PCG::PlanetGenObjectScheduler
	{
	public:
		static PCG::PlanetGenObjectScheduler* gCreateFunction(SphericalTerrain* pSph);
	public:
		SphericalTerrainPCGGenScheduler(SphericalTerrain* pSph);
		virtual ~SphericalTerrainPCGGenScheduler();

		virtual void updateGenSpace(const std::map<int, GenSpaceList>&) override;
		virtual void ExportGenObjects(const std::map<int, GenSpaceMap>&, std::map<Name, std::vector<PCG::ExportPoint>>&) override;
		virtual void collectGenObjects(std::map<Name, uint32>& data, uint32 type) override;
		virtual void collectDrawInfo(PCG::PCGDrawInfoMap& data) override;
		virtual void updateDeletedObjects() override;
		virtual void initImpl() override;
		virtual bool queryGenObjects(const DVector3& worldPos, const Quaternion& worldRot, const Vector3& halfSize, std::set<uint64>& objIds, bool checkVisible) override;
		virtual bool queryGenObjects(const DVector3& worldPos, float radius, std::set<uint64>& objIds, bool checkVisible) override;
		virtual bool setGenObjectsVisible(const std::set<uint64>& objIds, bool v) override;
		virtual bool getGenObject(uint64 id, Echo::GenObjectInfo& info) override;
		void Clear();
	public:
		RoadPCGGenScheduler* getRoadPCGGenScheduler();
		bool ExportRoadGenObjects(const std::string& folder, const std::map<int, PCG::BiomeGenObjectLayerInfo>& info, uint32 id);
	private:
		EchoPCGObject* Add(const GenSpace& _obb, uint32 seed, int level, EchoPCGObject* parentPcg, bool bSync);
		std::map<uint32, EchoPCGObject*> m_PcgMap;
		SphericalTerrainPCGGenSchedulerPrivate* m_pSTPrivate = nullptr;
		RoadPCGGenScheduler* m_pRoadPcg = nullptr;
		friend class SphericalTerrainComponent;
	};

};

#endif // EchoSphericalTerrainPCGGenScheduler_h__
