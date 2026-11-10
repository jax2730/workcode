//-----------------------------------------------------------------------------
// File:   EchoPlanetRoadNetworkManager.h
//
// Author: zhouguangyuan
//
// Date:   2026-1-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include "EchoComponentPrerequisites.h"

#include "EchoTerrainModifyData.h"

namespace Echo
{
	class SphericalTerrainComponent;
	class PavementComponent;
	struct PlanetRoadGroup;
	struct PlanetRoadData;

	// Planet Road Network Manager
	// All road network related management for planet terrain is handled here
	// Now only used as a interface placeholder, more features will be added later
	class _EchoComponentExport PlanetRoadNetworkManager
	{
	public:
		PlanetRoadNetworkManager(SphericalTerrainComponent* ownerPlanetComponent);

		void CreateRoadNetwork(bool sync = false);

		void ClearRoadNetwork();

		void CreateRoad(int32 roadId, bool sync = false);

		void RebuildRoad(int32 roadId, bool sync = false);

		void DestroyRoad(int32 roadId);

		void RefreshRoad(int32 roadId, bool sync = false);

		ActorPtr GetRoadActor(int32 roadId);

		EchoWeakPtr<PavementComponent> GetPavementComponent(int32 roadId);

		void CollectTerrainModifiers(std::vector<TerrainModify::Modifier>& outModifiers, float trans = 1.0f);

	private:
		const PlanetRoadGroup& GetPlanetRoadGroup() const;

		const PlanetRoadData* GetRoadDataById(int32 roadId) const;

		void AttachRoadPavementComponent(const ActorPtr& roadActor, ActorComponentPtr bezierComponent, const PlanetRoadData& roadData, bool sync = false);

		ActorPtr CreateRoadActor(const PlanetRoadData& roadData, bool sync = false);

		SphericalTerrainComponent* m_PlanetComponent;

		// TODO: store created road actors for management
	};
}
