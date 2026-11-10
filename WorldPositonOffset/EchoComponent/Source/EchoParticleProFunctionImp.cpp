//-----------------------------------------------------------------------------
// File:  EchoParticleProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-14
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoParticleProFunctionImp.h"
#include "EchoAutoObjectManager.h"
#include "EchoSingleton.h"
#include "Effect/EchoEffectHolderManager.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void ParticleProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(PFID_Particle_Create, &ParticleProFunctionImp::ProCom_CreateParticle);
		p->registerProtocolFunc(PFID_Particle_Delete, &ParticleProFunctionImp::ProCom_DeleteParticle);
		p->registerProtocolFunc(PFID_Particle_SetPosition, &ParticleProFunctionImp::ProCom_SetParticlePos);
		p->registerProtocolFunc(PFID_Particle_GetPosition, &ParticleProFunctionImp::ProCom_GetParticlePos);
		p->registerProtocolFunc(PFID_Particle_SetRotation, &ParticleProFunctionImp::ProCom_SetParticleRot);
		p->registerProtocolFunc(PFID_Particle_GetRotation, &ParticleProFunctionImp::ProCom_GetParticleRot);
		p->registerProtocolFunc(PFID_Particle_SetScale, &ParticleProFunctionImp::ProCom_SetParticleScale);
		p->registerProtocolFunc(PFID_Particle_GetScale, &ParticleProFunctionImp::ProCom_GetParticleScale);
	}

	//=============================================================================
	void ParticleProFunctionImp::uninitProtocolFunction()
	{
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult ParticleProFunctionImp::ProCom_CreateParticle(void * handle, ProtocolHandle descPtr)
	{
		ParticleHolderDesc desc = *(ParticleHolderDesc*)descPtr;
		EffectHolder* particleHolder = new EffectHolder(desc.resName, desc.pSceneMgr, desc.priority, desc.UseCache, desc.synch, desc.bIsPrior, desc.bNoLimit, desc.pParticleVanishListen, desc.byBatchAnim, desc.lifeTime);
		*(ProtocolHandle*)handle = (ProtocolHandle)particleHolder;
		return PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_DeleteParticle(void * data, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		if (particleHolder == NULL)
			return PR_OK;
		SAFE_DELETE(particleHolder);
		return PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_SetParticlePos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		if (particleHolder == NULL)
			return PR_OK;
		particleHolder->setPosition(*(Echo::Vector3*)pos);
		return Echo::PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_GetParticlePos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		if (particleHolder != NULL)
			*(Echo::Vector3*) pos = particleHolder->getPosition();
		return Echo::PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_SetParticleRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		if (particleHolder == NULL)
			return PR_OK;
		particleHolder->setOrientation(*(Quaternion*)qua);
		return PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_GetParticleRot(void * qua, ProtocolHandle handle)
	{
		//Not find function
		/*if (handle == 0)
			*(Echo::Quaternion*) qua = Echo::Quaternion::ZERO;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		if (particleHolder != NULL)
			*(Echo::Quaternion*) qua = particleHolder->getp();*/
		return Echo::PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_SetParticleScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		if (particleHolder == NULL)
			return PR_OK;
		particleHolder->setScale(*(Vector3*)scale);
		return PR_OK;
	}

	ProtocolResult ParticleProFunctionImp::ProCom_GetParticleScale(void * scale, ProtocolHandle handle)
	{
		//Not find function
		/*if (handle == 0)
			*(Echo::Vector3*) scale = Echo::Vector3::UNIT_SCALE;
		EffectHolder* particleHolder = (EffectHolder*)handle;
		*(Echo::Vector3*) scale = Echo::Vector3::UNIT_SCALE;
		if (particleHolder != NULL)
			*(Echo::Vector3*) scale = particleHolder->getScale();*/
		return Echo::PR_OK;
	}
	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
