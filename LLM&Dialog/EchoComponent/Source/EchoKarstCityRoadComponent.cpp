#include "EchoComponentStableHeaders.h"
#include "EchoKarstCityRoadComponent.h"
#include "EchoKarstTerrainComponent.h"
#include "EchoKarstSystem.h"
#include "EchoKarstGroup.h"
namespace Echo
{
	KarstCityRoadComponentInfo::KarstCityRoadComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{
		addCustomParam("cityCenter");
	}
	void KarstCityRoadComponentInfo::ExportData(cJSON* jsonNode)
	{
		cJSON_AddBoolToObject(jsonNode, "fitTerrain", fitTerrain);
		WriteJSNode(jsonNode, "cityCenter", cityCenter);

		cJSON* beziersJS = cJSON_CreateArray();
		for (auto& bezier : beziers)
		{
			cJSON* bezierNode = cJSON_CreateArray();
			for (auto& point : bezier) {
				cJSON* pointNode = cJSON_CreateObject();
				point.ExportJson(pointNode);
				cJSON_AddItemToArray(bezierNode, pointNode);
			}
			cJSON_AddItemToArray(beziersJS, bezierNode);
		}
		cJSON_AddItemToObject(jsonNode, "beziers", beziersJS);

		cJSON* pavementsJS = cJSON_CreateObject();
		WriteJSNode(pavementsJS, "pavements", pavements);
		cJSON_AddItemToObject(jsonNode, "pavements", pavementsJS);

	}

	void KarstCityRoadComponentInfo::ImportData(cJSON* jsonNode)
	{
		ReadJSNode(jsonNode, "fitTerrain", fitTerrain);
		ReadJSNode(jsonNode, "cityCenter", cityCenter);

		Vector2 min, max;
		//cJSON* cityAabbJS = cJSON_GetObjectItem(jsonNode, "cityAabb");
		//if (cityAabbJS) {
		//	ReadJSNode(cityAabbJS, "min", min);
		//	ReadJSNode(cityAabbJS, "max", max);
		//	//cityAabb = std::make_tuple(min, max);
		//}

		cJSON* beziersJS = cJSON_GetObjectItem(jsonNode, "beziers");
		std::vector< std::vector<PiecewiseBezierComponentInfo::BezierInfoPoint>>().swap(beziers);
		if (beziersJS) {
			for (int i = 0; i < cJSON_GetArraySize(beziersJS); i++) {
				cJSON* bezierNode = cJSON_GetArrayItem(beziersJS, i);
				std::vector<PiecewiseBezierComponentInfo::BezierInfoPoint> bezier;
				for (int j = 0; j < cJSON_GetArraySize(bezierNode); j++) {
					cJSON* pointNode = cJSON_GetArrayItem(bezierNode, j);
					PiecewiseBezierComponentInfo::BezierInfoPoint point;
					point.ImportJson(pointNode);
					bezier.push_back(point);
				}
				beziers.push_back(bezier);
			}
		}
		cJSON* pavementsJS = cJSON_GetObjectItem(jsonNode, "pavements");
		std::vector<PavementMeshBaseData>().swap(pavements);
		if (pavementsJS) {
			ReadJSNode(pavementsJS, "pavements", pavements);
		}
	}

	EDITOR_REFLECT_BEGIN(KarstCityRoadComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(fitTerrain, "fitTerrain", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(cityCenter, "cityCenter", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(pavements, "pavements", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(beziers, "beziers", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	void KarstCityRoadComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;

		KarstCityRoadComponentInfo* pInfo = static_cast<KarstCityRoadComponentInfo*>(info);
		reflect::TypeResolver<KarstCityRoadComponentInfo>::get()->copyTo(this, pInfo);
		//pInfo->cityAabb = cityAabb;
		return;
	}

	void KarstCityRoadComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			KarstCityRoadComponentInfo* m_pInfo = static_cast<KarstCityRoadComponentInfo*>(mOriginComInfo);
			reflect::TypeResolver<KarstCityRoadComponentInfo>::get()->overWrite(m_pInfo, this);
		}
	}

	DECLAREACTORCOMTYPE_CPP(KarstCityRoadComponent)
	KarstCityRoadComponent::KarstCityRoadComponent(ActorPtr _actor,
		ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/): NodeActorComponent(_actor, movableType) {}
	KarstCityRoadComponent::~KarstCityRoadComponent() {}

	void KarstCityRoadComponent::init(ActorComponentInfo* info)
	{
		if(info == nullptr)
			return;
		this->m_pActComInfo = info;
		
	}

	void KarstCityRoadComponent::unInit()
	{

	}

	void KarstCityRoadComponent::replay(ActorComponentInfo* info)
	{
	}

	void KarstCityRoadComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if(!actCom) return;
		this->m_pActComInfo = actCom->m_pActComInfo;
	}

	void KarstCityRoadComponent::connectCom()
	{
		//auto cityAabb = static_cast<KarstCityRoadComponentInfo*>(m_pActComInfo)->cityAabb;
		createRoads(nullptr);
	}

	void KarstCityRoadComponent::disconnectCom()
	{
		mTerrain = KarstCityTerrain();
	}

	void KarstCityRoadComponent::createRoads(KarstGroup*)
	{
		ActorComFactory* bezierFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(PiecewiseBezierComponent));
		KarstCityRoadComponentInfo* pInfo = static_cast<KarstCityRoadComponentInfo*>(this->m_pActComInfo);
		for (int i = 0; i < pInfo->beziers.size(); ++i) {
			const auto& beziers = pInfo->beziers[i];
			const auto& pavement = pInfo->pavements[i];
			PiecewiseBezierComponentInfo* bezier_info = static_cast<PiecewiseBezierComponentInfo*>(bezierFactory->createComponentInfo());
			bezier_info->mPoints = beziers;
			bezier_info->AdsorbTerrain = true;
			ActorComponentPtr bezierCom = this->getActorPtr()->addComponent(COMPONENTTYPE(PiecewiseBezierComponent), "Bezier", static_cast<ActorComponentPtr>(this), bezier_info);
			PavementComponent* pavementCom = static_cast<PavementComponent*>(this->getActorPtr()->addComponent(COMPONENTTYPE(PavementComponent), "Road", static_cast<ActorComponentPtr>(this), nullptr).get());
			pavementCom->setBezierCom(bezierCom);
			pavementCom->setPavementFitTerrain(&mTerrain);
			pavementCom->setFitGroundModel(true);
			pavementCom->setVegetationCollection(false);
			pavementCom->createPavement(pavement, PavementComponent::UpdateAll, false);
			pavementCom->setFitGroundModel(true);
			pavementCom->SetRenderDistance(2000.0f, Name(pavement.groupName));
		}
	}

	void KarstCityTerrain::getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data)
	{
		Vector2 worldPos2d = Vector2(worldPos.x, worldPos.z);
		float height = 0.0f;
		KarstGroup* karstGroup = KarstSystem::instance()->getKarstGroup();
		if (!karstGroup) karstGroup = KarstSystem::instance()->getVoxelKarstGroup();
		if (!karstGroup) {
			data.terrainHeight = height;
			data.terrainNormal = Vector3::UP;
			data.terrainPos = Vector3(worldPos.x, height, worldPos.z);
			return;
		}
		bool&& res = karstGroup->computeHeightAtPos(worldPos2d, height, true, false);
		if (!res) height = 0.0f;
		data.terrainHeight = height;
		data.terrainNormal = Vector3::UP;
		data.terrainPos = Vector3(worldPos.x, height, worldPos.z);

	}

}
