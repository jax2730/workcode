#include "EchoComponentStableHeaders.h"
#include "EchoRagdollBehaviorComponentBase.h"
#include "EchoRagdollComponent.h"
#include "EchoPhysicsManager.h"

#define GET_RAGDOLL() static_cast<RagdollComponent*>(ragdollCom.get())

namespace Echo
{
    static Vector3 ComponentUP(const ActorComponentPtr comp)
    {
        if (comp.expired())
            return Vector3::UP;

        return comp->getWorldRotation() * Vector3::UP;
    }

    static Name g_ragdollBehaviorBaseType = Name("BehaviourBase");
    const Name& Echo::RagdollBehaviorComponentBase::GetTypeSpring()
    {
        return g_ragdollBehaviorBaseType;
    }

    void RagdollBehaviorComponentBase::OnMuscleAdded(RagdollMuscle* m)
    {
        if (OnHierarchyChanged) OnHierarchyChanged();
    }

    void RagdollBehaviorComponentBase::OnMuscleRemoved(RagdollMuscle* m)
    {
        if (OnHierarchyChanged) OnHierarchyChanged();
    }

    void RagdollBehaviorComponentBase::OnMuscleHit(const MuscleHit& hit)
    {
        if (!initiated) return;
        if (OnPreMuscleHit) OnPreMuscleHit(hit);

        OnMuscleHitBehaviour(hit);

        if (OnPostMuscleHit) OnPostMuscleHit(hit);
    }

    void RagdollBehaviorComponentBase::OnMuscleCollision(const MuscleCollision& collision)
    {
        if (!initiated) return;
        if (OnPreMuscleCollision) OnPreMuscleCollision(collision);

        OnMuscleCollisionBehaviour(collision);

        if (OnPostMuscleCollision) OnPostMuscleCollision(collision);
    }

    void RagdollBehaviorComponentBase::OnMuscleCollisionExit(const MuscleCollision& collision)
    {
        if (!initiated) return;
        if (OnPreMuscleCollisionExit) OnPreMuscleCollisionExit(collision);

        OnMuscleCollisionExitBehaviour(collision);

        if (OnPostMuscleCollisionExit) OnPostMuscleCollisionExit(collision);
    }

    void RagdollBehaviorComponentBase::Initiate()
    {
        initiated = true;

        if (OnPreInitiate) OnPreInitiate();

        OnInitiate();

        if (OnPostInitiate) OnPostInitiate();
    }

    void RagdollBehaviorComponentBase::OnFixTransforms()
    {
        if (!initiated) return;
        if (!getVisible()) return;

        if (OnPreFixTransforms) OnPreFixTransforms();

        OnFixTransformsBehaviour();

        if (OnPostFixTransforms) OnPostFixTransforms();
    }

    void RagdollBehaviorComponentBase::OnRead(float deltaTime)
    {
        if (!initiated) return;
        if (!getVisible()) return;

        if (OnPreRead) OnPreRead(deltaTime);

        OnReadBehaviour(deltaTime);

        if (OnPostRead) OnPostRead(deltaTime);
    }

    void RagdollBehaviorComponentBase::OnWrite(float deltaTime)
    {
        if (!initiated) return;
        if (!getVisible()) return;

        if (OnPreWrite) OnPreWrite(deltaTime);

        OnWriteBehaviour(deltaTime);

        if (OnPostWrite) OnPostWrite(deltaTime);
    }

    void RagdollBehaviorComponentBase::Activate()
    {
        RagdollComponent* pMaster = GET_RAGDOLL();
        if (pMaster) 
        {
            pMaster->BehavioursForeach([this](RagdollBehaviorComponentBase* b)
            {
                b->setVisible(b == this);
            });
        }

        if (OnPreActivate) OnPreActivate();

        OnActivate();

        if (OnPostActivate) OnPostActivate();
    }

    void RagdollBehaviorComponentBase::FixedUpdateB(float deltaTime)
    {
        if (!initiated) return;
        if (!getVisible()) return;
        RagdollComponent* pMaster = GET_RAGDOLL();
        if (pMaster == nullptr || pMaster->muscles.size() <= 0) return;

        if (OnPreFixedUpdate) OnPreFixedUpdate(deltaTime);

        OnFixedUpdate(deltaTime);

        if (OnPostFixedUpdate) OnPostFixedUpdate(deltaTime);
    }

    void RagdollBehaviorComponentBase::UpdateB(float deltaTime)
    {
        if (!initiated) return;
        if (!getVisible()) return;
        RagdollComponent* pMaster = GET_RAGDOLL();
        if (pMaster == nullptr || pMaster->muscles.size() <= 0) return;

        if (OnPreUpdate) OnPreUpdate(deltaTime);

        OnUpdate(deltaTime);

        if (OnPostUpdate) OnPostUpdate(deltaTime);
    }

    void RagdollBehaviorComponentBase::LateUpdateB(float deltaTime)
    {
        if (!initiated) return;
        if (!getVisible()) return;
        RagdollComponent* pMaster = GET_RAGDOLL();
        if (pMaster == nullptr || pMaster->muscles.size() <= 0) return;

        if (OnPreLateUpdate) OnPreLateUpdate(deltaTime);

        OnLateUpdate(deltaTime);

        if (OnPostLateUpdate) OnPostLateUpdate(deltaTime);
    }

    void RagdollBehaviorComponentBase::UpdateHelperB()
    {
        if (!initiated) return;
        if (!getVisible()) return;
        OnUpdateHelperBehaviour();
    }

    void RagdollBehaviorComponentBase::RotateTargetToRootMuscle()
    {
        RagdollComponent* pRagdoll = GET_RAGDOLL();
        if (pRagdoll == nullptr || pRagdoll->muscles.size() == 0 )
            return;

        if (pRagdoll->muscles[0]->target == nullptr || pRagdoll->GetTargetRoot() == nullptr || pRagdoll->muscles[0]->rigidbody == nullptr)
            assert(false && "pRagdoll data lose !!!");

        Vector3 hipsForward = pRagdoll->muscles[0]->target->_getDerivedOrientation().Inverse() * ComponentUP(pRagdoll->GetTargetRoot());
        Vector3 forward = pRagdoll->muscles[0]->rigidbody->getRotation() * hipsForward;

        forward.y = 0.f;
        pRagdoll->GetTargetRoot().getDynamicCastWeakPtr<ComponentNode>()->setWorldRotation(Quaternion::LookRotation(forward, Vector3::UP));

    }

    void RagdollBehaviorComponentBase::TranslateTargetToRootMuscle(float tx, float ty, float tz)
    {
        RagdollComponent* pRagdoll = GET_RAGDOLL();
        if (pRagdoll == nullptr || pRagdoll->muscles.size() == 0)
            return;

        if (pRagdoll->muscles[0]->target == nullptr || pRagdoll->muscles[0]->GetTransform() == nullptr)
            assert(false && "pRagdoll data lose !!!");
        
        Vector3 targetPos = pRagdoll->muscles[0]->target->_getDerivedPosition();
        Vector3 transfPos = pRagdoll->muscles[0]->GetTransform()->getWorldPosition();
        float x = Math::lerp(transfPos.x, targetPos.x, tx);
        float y = Math::lerp(transfPos.y, targetPos.y, ty);
        float z = Math::lerp(transfPos.z, targetPos.x, tz);
        
        pRagdoll->muscles[0]->target->_setDerivedPosition(Vector3(x, y, z));
    }

    void RagdollBehaviorComponentBase::GroundTarget(int layers)
    {
        RagdollComponent* pRagdoll = GET_RAGDOLL();

        if (pRagdoll == nullptr || !pRagdoll->GetTargetRoot())
            return;
        Vector3 rolePostion = pRagdoll->GetTargetRoot()->getWorldPosition();

        EchoRaycastHit hit;
        if (EchoPhysicsSystem::instance()->GetPhysicsManager(this)->Raycast(rolePostion + ComponentUP(pRagdoll->GetTargetRoot()), -ComponentUP(pRagdoll->GetTargetRoot()), hit, 4.f, layers))
        {
            pRagdoll->GetTargetRoot().getDynamicCastWeakPtr<ComponentNode>()->setWorldPosition(hit.point);
        }
    }

    void RagdollBehaviorComponentBase::visibleStateChanged(bool visibleState)
    {
        if (visibleState)
            OnEnable();
        else
            OnDisable();
    }

    void RagdollBehaviorComponentBase::OnEnable()
    {
        if (!initiated) return;
        Activate();
    }

    void RagdollBehaviorComponentBase::OnDisable()
    {
        if (!initiated) return;
        if (OnPreDeactivate) OnPreDeactivate();

        OnDeactivate();

        if (OnPostDeactivate) OnPostDeactivate();
    }

    void RagdollBehavior::RagdollEvent::Trigger(RagdollComponent* ragdollcom)
    {
        animationEvent->Activate();

        if (switchToBehaviour.empty())
            return;

        bool found = false;
        
        ragdollcom->BehavioursForeach([&found,this](RagdollBehaviorComponentBase* b) 
            {
                if (b->GetTypeSpring() == switchToBehaviour)
                {
                    found = true;
                    b->setVisible(true);
                }
            });

        if (!found)
        {
            // TODO log
        }
    }

};
