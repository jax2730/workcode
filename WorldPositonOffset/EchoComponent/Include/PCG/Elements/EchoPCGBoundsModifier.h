#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		enum class PCGBoundsModifierMode : uint8 {
			Set,
			Intersect,
			Include,
			Translate,
			Scale
		};
		ENUM_CLASS_FLAGS(PCGBoundsModifierMode)

		class PCGBoundsModifierSettings : public PCGV2::PCGSettings
		{
		public:
			PCG_SETTINGS_BODY_H(BoundsModifier)

			PCGBoundsModifierMode Mode = PCGBoundsModifierMode::Scale;
			Vector3 BoundsMin = Vector3::UNIT_SCALE;
			Vector3 BoundsMax = Vector3::UNIT_SCALE;
			bool bAffectSteepness = false;
			float Steepness = 1.0f;
		};

		class PCGBoundsModifierElement : public PCGV2::PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual bool ExecuteInternal(PCGContext* context) override;
		};
	}
}

