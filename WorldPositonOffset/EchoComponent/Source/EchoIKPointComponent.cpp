#include "EchoComponentStableHeaders.h"
#include "EchoCommonStableHeaders.h"
#include "EchoIKPointComponent.h"
#include "EchoWorldManager.h"

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(IKPointComponent)
	

	_DEFINEINFO_VIRFUN_BASE(IKPointComponentInfo)


	IKPointComponentInfo::IKPointComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType)
	{
	}
	IKPointComponentInfo::~IKPointComponentInfo()
	{
	}

	IKPointComponent::IKPointComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: NodeActorComponent(pActor, movableType){}

	IKPointComponent::~IKPointComponent(){}
	void IKPointComponent::init(ActorComponentInfo * info)
	{
	}
	void IKPointComponent::unInit()
	{
#ifdef USE_IK_HELPEROBJ
		ReleaseHelper();
#endif // USE_IK_HELPEROBJ
	}
	void IKPointComponent::replay(ActorComponentInfo * info) {


	}
	void IKPointComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		actCom.getDynamicCastWeakPtr<IKPointComponent>()->SetHelperVisible(GetHelperVisible());
	}
	void IKPointComponent::connectCom() {

	}

	void IKPointComponent::disconnectCom() {
#ifdef USE_IK_HELPEROBJ
		if (m_pHelperObj) {
			RemoveChildFromNode(m_pHelperObj->getSceneNode());
			UpdateComRootNode(true);
		}
#endif // USE_IK_HELPEROBJ
	}
	void IKPointComponent::showEditorHelper(bool isShow) {
		SetHelperVisible(isShow);
	}
	void IKPointComponent::updateCombinedName() {
#ifdef USE_IK_HELPEROBJ
		if (m_pHelperObj) {
			Name newName(getCombinedName().append("||").append("Point"));
			m_pHelperObj->setName(newName);
		}
#endif // USE_IK_HELPEROBJ
	}
	ActorComponentPtr IKPointComponent::FindComponent(const String& compName) {
		if (compName.empty()) return nullptr;
		if (getActor()->IsRelativeName(compName)) {
			PickActorResult res = getActor()->getActorComByRelativeName(compName);
			return res.seleteCom;
		}
		
		PickActorResult res = getActor()->getActorManager()->GetActorComBySeletedName(compName);
		if (res.seleteCom) {
			return res.seleteCom;
		}
		return getActor()->getComponent(compName);
	}
	
	WorldNode* IKPointComponent::GetWorldNode() {
		return getSceneNode();
		//SceneNode* pNode = nullptr;
		//ProtocolComponentSystem::instance()->callProtocolFunc(PFID_SceneNode, &pNode, (Echo::ProtocolHandle)this);
		//return (WorldNode*)pNode;
	}
	/*SceneNode* IKPointComponent::GetSceneNode() {
		return m_pSceneNode;
	}*/
	
	void IKPointComponent::SetHelperVisible(bool visible) {
#ifdef USE_IK_HELPEROBJ
		IsVisible = visible;
		if (IsVisible) CreateHelper();
		if (!m_pHelperObj) return;
		m_pHelperObj->setVisible(IsVisible);
		UpdateHelper();
#endif // USE_IK_HELPEROBJ
	}
	bool IKPointComponent::GetHelperVisible() {
#ifdef USE_IK_HELPEROBJ
		return IsVisible;
#else
		return false;
#endif // USE_IK_HELPEROBJ
	}
}

namespace Echo {

#ifdef USE_IK_HELPEROBJ
	void IKPointComponent::CreateHelper() {
		if (m_pHelperObj) return;
		Name newName(getCombinedName().append("||").append("Point"));
		m_pHelperObj = new HelperObject(newName, HelperObjType_Sphere, false, nullptr, getSceneManager());
		addChildToNode(m_pHelperObj->getSceneNode());
		m_pHelperObj->setRenderType(ECHO_LINE_LIST);
		m_pHelperObj->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj->setScale(Vector3(0.1f, 0.1f, 0.1f));
		m_pHelperObj->setVisible(IsVisible);
}
	void IKPointComponent::ReleaseHelper() {
		if (m_pHelperObj == NULL) return;
		RemoveChildFromNode(m_pHelperObj->getSceneNode());
		delete m_pHelperObj;
		m_pHelperObj = NULL;
	}
	void IKPointComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) return;
		m_pHelperObj->setScale(Vector3(0.1f, 0.1f, 0.1f));
		m_pHelperObj->setRenderType(ECHO_LINE_STRIP);
		m_pHelperObj->changeHelperType(HelperObjType_Sphere);
	}
#endif // USE_IK_HELPEROBJ
}
