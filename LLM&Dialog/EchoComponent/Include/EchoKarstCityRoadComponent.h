#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoPavementComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoKarstSystem.h"
#include "EchoPiecewiseBezierComponent.h"

namespace Echo
{
	class KarstCityRoadComponent;
	class KarstCityRoadComponentInfo;

	class _EchoComponentExport KarstCityTerrain : public PavementFitTerrain {
	public:
		
		KarstCityTerrain() = default;
		~KarstCityTerrain() = default;
		virtual void getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data) override;
	};

	class _EchoComponentExport KarstCityRoadComponent :public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(KarstCityRoadComponent)
	public:

		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		void createRoads(KarstGroup*);

	private:
		KarstCityTerrain mTerrain;
		
	};

	class _EchoComponentExport KarstCityRoadComponentInfo :public ActorComponentInfo {
	public:


		KarstCityRoadComponentInfo() {};
		KarstCityRoadComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual ~KarstCityRoadComponentInfo() {};

		virtual void					ExportData(cJSON* jsonNode) override;
		virtual void					ImportData(cJSON* jsonNode) override;
		virtual void					CopyTo(ActorComponentInfo* info) override;
		virtual void					OverWrite() override;

		Vector3 cityCenter = Vector3(0.0f, 0.0f, 0.0f);
		//std::tuple<Vector2, Vector2> cityAabb;
		std::vector< std::vector<PiecewiseBezierComponentInfo::BezierInfoPoint>> beziers;
		std::vector<PavementMeshBaseData> pavements;

		bool fitTerrain = false;
		EDITOR_REFLECT
	};
	DEFINECOMFACTORY(KarstCityRoadComponent, ALL_MOVEABLE)
}//namespace Echo
