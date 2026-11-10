#pragma once

#include "EchoComponentPrerequisites.h"
#include "PCG/EchoPCGCommon.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGHelpers
		{
			_EchoComponentExport int ComputeSeed(int A);
			_EchoComponentExport int ComputeSeed(int A, int B);
			_EchoComponentExport int ComputeSeed(int A, int B, int C);
			_EchoComponentExport int ComputeSeedFromPosition(const Vector3& InPosition);


			_EchoComponentExport FBox GetActorLocalBounds(const ActorPtr& InActor, bool bIgnorePCGCreatedComponents = true);
		}
	}
}
