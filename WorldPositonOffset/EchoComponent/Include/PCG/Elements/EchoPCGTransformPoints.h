#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"
#include "EchoVector3.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGTransformPointsSettings : public PCGV2::PCGSettings
		{
		public:
			PCG_SETTINGS_BODY_H(TransformPoints)

			//bool bApplyToAttribute = false;
			//Name AttributeName;
			Vector3 OffsetMin = Vector3::ZERO;
			Vector3 OffsetMax = Vector3::ZERO;
			bool bAbsoluteOffset = false;
			Vector3 RotationMin = Vector3::ZERO;
			Vector3 RotationMax = Vector3::ZERO;
			bool bAbsoluteRotation = false;
			Vector3 ScaleMin = Vector3::UNIT_SCALE;
			Vector3 ScaleMax = Vector3::UNIT_SCALE;
			bool bAbsoluteScale = false;
			bool bUniformScale = true;
			bool bRecomputeSeed = false;
		};

		class PCGTransformPointsElement : public PCGV2::PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual bool ExecuteInternal(PCGContext* context) override;

			virtual PCGElementExecutionLoopMode ExecutionLoopMode(const PCGSettings* Settings) const override { return PCGElementExecutionLoopMode::SinglePrimaryPin; }
		};

	}
}

