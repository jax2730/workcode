#include "EchoComponentStableHeaders.h"
#include "EchoPolylineComponent.h"

#include "EchoActorProFuncImp.h"
#include "EchoLogManager.h"
#include "EchoResourceFolderManager.h"
#include "Actor/EchoActorHelper.h"
#include "EchoDataStream.h"

namespace Echo
{
	PolylineComponentInfo::PolylineComponentInfo(const Name& comType, ActorCOMMoveableType MoveType)
		: ActorComponentInfo(comType, MoveType)
	{

	}

	PolylineComponentInfo::~PolylineComponentInfo()
	{
		Points.clear();
	}

	void PolylineComponentInfo::ExportData(cJSON* jsonNode) {
		cJSON* jArr = cJSON_CreateArray();
		for (const auto& v : Points)
		{
			cJSON* jVec = cJSON_CreateArray();        // [x,y,z]
			cJSON_AddItemToArray(jVec, cJSON_CreateNumber(v.x));
			cJSON_AddItemToArray(jVec, cJSON_CreateNumber(v.y));
			cJSON_AddItemToArray(jVec, cJSON_CreateNumber(v.z));
			cJSON_AddItemToArray(jArr, jVec);
		}
		cJSON_AddItemToObject(jsonNode, "PolylinePoints", jArr);
		cJSON_AddNumberToObject(jsonNode, "PolylineCount", PointCount);
		cJSON_AddStringToObject(jsonNode, "BindParticleSystemName", BindPtclSysName.c_str());
	}

	void PolylineComponentInfo::ImportData(cJSON* jsonNode) {
		cJSON* temp = cJSON_GetObjectItem(jsonNode, "BindParticleSystemName");
		if (temp != nullptr) {
			BindPtclSysName = temp->valuestring;
		}
		
		cJSON* jPoints = cJSON_GetObjectItem(jsonNode, "PolylinePoints");
		if (jPoints != nullptr) {
			PointCount = cJSON_GetArraySize(jPoints);
			for (int i = 0; i < PointCount; ++i)
			{
				cJSON* jVec = cJSON_GetArrayItem(jPoints, i);
				if (jVec != nullptr && cJSON_GetArraySize(jVec) == 3) {
					Vector3 v;
					cJSON* jX = cJSON_GetArrayItem(jVec, 0);
					cJSON* jY = cJSON_GetArrayItem(jVec, 1);
					cJSON* jZ = cJSON_GetArrayItem(jVec, 2);
					if (jX != nullptr) v.x = (float)jX->valuedouble;
					if (jY != nullptr) v.y = (float)jY->valuedouble;
					if (jZ != nullptr) v.z = (float)jZ->valuedouble;
					Points.push_back(v);
				}
			}
		}
	}

	void PolylineComponentInfo::OverWrite() {

	}

	void PolylineComponentInfo::CopyTo(ActorComponentInfo* info) {

	}

	void PolylineComponentInfo::Overlay(ActorComponentInfo* info) {

	}

	DECLAREACTORCOMTYPE_CPP(PolylineComponent)
	PolylineComponent::PolylineComponent(ActorPtr pActor, ActorCOMMoveableType moveType)
		:NodeActorComponent(pActor, moveType)
	{
		
	}

	PolylineComponent::~PolylineComponent()
	{

	}

	void PolylineComponent::init(ActorComponentInfo* info)
	{
		mSceneManager = Root::instance()->getMainSceneManager();
		m_Info = static_cast<PolylineComponentInfo*>(info);

		m_PtclSysComps = m_pActor->getRootNodeCom()->getComponentsByType(Echo::Name("ParticleSystemComponent"), true);
		m_PtclSysCount = m_PtclSysComps.size();
		for(int i = 0; i < m_PtclSysCount; ++i)
		{
			if (m_PtclSysComps[i].get()->getComNameToString() == m_Info->BindPtclSysName) {
				m_BindPtclSysIndex = i;
				break;
			}
		}

		UpdateAllPointHelpers(m_Info->Points);
		SetHelperVisibility(m_IsHelperVisible);
		ExportDataToParticleSystem();

		SetNotifiedUpdate(true); // 通知更新
	}

	void PolylineComponent::unInit()
	{
		SetNotifiedUpdate(false); // 通知更新
		if (m_LineHelper != nullptr) {
			//mComRootNode->removeChild(m_LineHelper->GetSceneNode());
			detachFromNode(m_LineHelper);
		}
		RemoveAllPointHelpers();
		m_Points_Temp.clear();
		m_PtclSysComps.clear();
		mSceneManager = nullptr;
	}

	void PolylineComponent::SetHelperVisibility(bool isShow)
	{
		UpdateLineHelper();
		SetLineHelperVisiblity(isShow);
		SetPointHelperVisiblity(isShow);
		m_IsHelperVisible = isShow;
		mComRootNode->_update();
	}

	void PolylineComponent::SetLineHelperVisiblity(bool isShow)
	{
		CreateLineHelper();
		m_LineHelper->setVisible(isShow);
	}

	void PolylineComponent::SetPointHelperVisiblity(bool isShow)
	{
		for(auto& p : m_PointHelpers)
		{
			if (p != nullptr) {
				p->setVisible(isShow);
			}
		}
	}

	void PolylineComponent::CreatePoint()
	{
		
		Vector3 newpoint = Vector3::ZERO;
		m_Info->Points.push_back(newpoint);
		m_Info->PointCount++;
		if (m_IsHelperVisible) {
			UpdateLineHelper();
		}
		AddPointHelper(Vector3::ZERO);
		SetNotifiedUpdate(true);
		//SaveCompInfo();
	}

	void PolylineComponent::SetPoint(int index, const Vector3 pos)
	{
		if (index < 0 || index >= m_Info->PointCount) {
			return;
		}
		m_Info->Points[index] = pos;

		if (m_IsHelperVisible) {
			UpdateLineHelper();
		}
		UpdatePointHelper(pos, index);
		//SaveCompInfo();
	}

	void PolylineComponent::RemovePoint(int index)
	{
		if (index < 0 || index >= m_Info->PointCount)
			return;
		m_Info->Points.erase(m_Info->Points.begin() + index);
		m_Info->PointCount--;
		if (m_IsHelperVisible) {
			UpdateLineHelper();
		}
		RemovePointHelper(index);
	}

	Vector3 PolylineComponent::RelativeToWorldPos(const Vector3& pos)
	{
		Vector4 point_WPos = m_actorTransform * Vector4(pos.x, pos.y, pos.z, 1.0f);
		return Vector3(point_WPos.x, point_WPos.y, point_WPos.z);
	}

	void PolylineComponent::CreateLineHelper()
	{
		if (m_LineHelper != NULL || mSceneManager == NULL) {
			return;
		}

		std::string manualObjName_ = getCombinedName() + "|| PolylineHelper";
		char idx = '0';
		while (Echo::Root::instance()->getMainSceneManager()->getManualObject(Name(manualObjName_.data())) != nullptr)
		{
			idx = idx + 1;
			manualObjName_ = manualObjName_.append(&idx, 1);
		}
		m_LineHelper = mSceneManager->createManualObject(Name(manualObjName_.data()));
		//m_LineHelper->setBoundingBox(AxisAlignedBox(-0.1f, -0.1f, -0.1f, 0.1f, 0.1f, 0.1f));

		//mComRootNode->addChild(m_LineHelper->GetSceneNode());

		attachToNode(m_LineHelper);
		
		getSceneNode()->needUpdateBounds();
		getSceneNode()->_updateBounds();
	}

	void PolylineComponent::UpdateLineHelper()
	{
		if (m_pActor == nullptr) {
			return;
		}
		CreateLineHelper();
		m_LineHelper->clear();
		m_LineHelper->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_STRIP);
		m_LineHelper->colour(0.f, 0.5f, 1.f);
				
		for (auto pos: m_Info->Points)
		{
			m_LineHelper->position(pos);
		}

		m_LineHelper->end();
		m_LineHelper->setVisible(m_IsHelperVisible);
		//UpdateComRootNode();
	}

	void PolylineComponent::RemoveAllPoints()
	{
		m_Info->Points.clear();
		m_Info->PointCount = 0;

		UpdateLineHelper();
		RemoveAllPointHelpers();
	}

	void PolylineComponent::UpdateAllPointHelpers(const std::vector<Vector3>& points)
	{
		RemoveAllPointHelpers();

		for (const auto& pos : points)
		{
			SphereHelper* _helper = new SphereHelper(mSceneManager);
			_helper->setRadius(0.2f);
			_helper->setPosition(pos);
			_helper->setColor(Echo::ColorValue(0.0f, 0.0f, 0.8f, 0.8f));
			mComRootNode->addChild(_helper->GetSceneNode());
			mComRootNode->_update();
			m_PointHelpers.push_back(_helper);
		}
	}

	void PolylineComponent::AddPointHelper(const Vector3 pos)
	{
		SphereHelper* _helper = new SphereHelper(mSceneManager);
		_helper->setRadius(0.2f);
		_helper->setPosition(pos);
		_helper->setColor(Echo::ColorValue(0.0f, 0.0f, 0.8f, 0.8f));
		_helper->setVisible(m_IsHelperVisible);
		mComRootNode->addChild(_helper->GetSceneNode());
		if (m_IsHelperVisible) {
			mComRootNode->_update();
		}
		m_PointHelpers.push_back(_helper);
	}

	void PolylineComponent::UpdatePointHelper(const Vector3& pos, const int index)
	{
		if (index < 0 || index >= m_PointHelpers.size()) {
			return;
		}
		m_PointHelpers[index]->setPosition(pos);
	}

	void PolylineComponent::RemovePointHelper(const int index)
	{
		if (index < 0 || index >= static_cast<int>(m_PointHelpers.size()))
			return;

		SphereHelper* _helper = m_PointHelpers[index];
		mComRootNode->removeChild(_helper->GetSceneNode());

		delete _helper;
		m_PointHelpers.erase(m_PointHelpers.begin() + index);
	}

	void PolylineComponent::RemoveAllPointHelpers()
	{
		for (auto& helper : m_PointHelpers)
		{
			mComRootNode->removeChild(helper->GetSceneNode());
			delete helper;
		}
		m_PointHelpers.clear();
	}

	std::vector<ActorComponentPtr> PolylineComponent::FindAllParticleSystem()
	{
		m_PtclSysComps = m_pActor->getRootNodeCom()->getComponentsByType(Echo::Name("ParticleSystemComponent"),false);
		m_PtclSysCount = m_PtclSysComps.size();
		return m_PtclSysComps;
	}

	

	void PolylineComponent::ExportDataToParticleSystem()
	{
		if (m_Info->BindPtclSysName.empty() || m_PtclSysCount == 0) {
			return;
		}
		if (m_BindPtclSysIndex < 0 || m_BindPtclSysIndex > m_PtclSysComps.size() - 1) {
			return;
		}

		ParticleSystemComponent* ptrlSysComp = static_cast<ParticleSystemComponent*>(m_PtclSysComps[m_BindPtclSysIndex].get());

		if (m_Info->PointCount < 2)
		{
			return;
		}

		if (ptrlSysComp->getParticle() != nullptr) {
				m_Points_Temp.clear();
				Vector3 compPos = getPosition();
				for (int i = 0; i < m_Info->PointCount; ++i)
				{
					Vector3 point_WPos = RelativeToWorldPos(m_Info->Points[i]+ compPos); // 转换为世界坐标
					m_Points_Temp.push_back(point_WPos);
				}
				ptrlSysComp->getParticle()->SetHermitCurveTrail(&m_Points_Temp);
			//}
		}
	}


	SphereHelper* PolylineComponent::GetPickedHelper()
	{
		if( m_PickedHelperIndex >= 0 && m_PickedHelperIndex < static_cast<int>(m_PointHelpers.size()))
			return m_PointHelpers[m_PickedHelperIndex];
		else
			return nullptr;
	}

	Vector3 PolylineComponent::GetPickedHelperPos()
	{
		if (m_PickedHelperIndex >= 0 && m_PickedHelperIndex < m_PointHelpers.size())
			return m_Info->Points[m_PickedHelperIndex];
		else
			return Vector3(0.f, 0.f, 0.f);
	}

	void PolylineComponent::setActor(const ActorPtr& pActor)
	{
		//if (pActor == m_pActor)
		//	return;
		//if (pActor == nullptr && m_HelperLine != NULL)
		//	detachFromNode(m_HelperLine);
		//m_pActor = pActor;
		//if (m_pActor == nullptr)
		//	return;
		//if (m_HelperLine != NULL)
		//{
		//	attachToNode(m_HelperLine);
		//	UpdateComRootNode();
		//}
	}

	void PolylineComponent::createComRootNodeImpl()
	{
		mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
	}

	void PolylineComponent::NotifiChildImp(int changeMask)
	{
		m_actorTransform = m_pActor->getWorldMatrix();
		ExportDataToParticleSystem();
	}

	bool PolylineComponent::PickHelper(const Echo::Ray& ray)
	{
		if (GetPickedHelper()) {
			GetPickedHelper()->setColor(Echo::ColorValue(0.0f, 0.0f, 0.8f, 0.8f));
		}

		m_PickedHelperIndex = -1;   // 默认未选中

		if (m_PointHelpers.empty())
			return false;

		float closest_distance = std::numeric_limits<float>::infinity();

		for (int i = 0; i < static_cast<int>(m_PointHelpers.size()); ++i)
		{
			auto _helper = m_PointHelpers[i];
			auto [hit, dist] = _helper->hitTest(ray);
			if (hit && dist < closest_distance)
			{
				closest_distance = dist;
				m_PickedHelperIndex = i;   // 记录索引
			}
		}
		if (GetPickedHelper()) {
			GetPickedHelper()->setColor(Echo::ColorValue(0.f, 1.0f, 0.0f, 0.8f));
		}

		return m_PickedHelperIndex != -1;
	}

	PolylineComponentFactory::PolylineComponentFactory() {
		mType = Echo::Name("PolylineComponent");
		mMoveableType = ALL_MOVEABLE;
		bIsEditorCom = true;
		mPathGizmoIcon = mType;
	}

	PolylineComponentFactory::~PolylineComponentFactory() {

	}

}//namespace Echo


