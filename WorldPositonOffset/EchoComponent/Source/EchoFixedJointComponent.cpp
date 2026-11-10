#include "EchoComponentStableHeaders.h"
#include "EchoFixedJointComponent.h"
#include "EchocJsonHelper.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(FixedJointComponent)

	FixedJointComponentInfo::FixedJointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : JointComponentInfo(comType, MoveType) {}
	FixedJointComponentInfo::~FixedJointComponentInfo() {}

	FixedJointComponent::FixedJointComponent(ActorPtr actor, ActorCOMMoveableType type) : JointComponent(actor, type)
	{ 
		m_jointType = JointCompType::FIXED_JOINT;
		m_pCompData = new JointComponentData();
	}
	FixedJointComponent::~FixedJointComponent() {};

}
