#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGHelper.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGHelpers
		{
			int ComputeSeed(int A)
			{
				return (A * 196314165U) + 907633515U;
			}

			int ComputeSeed(int A, int B)
			{
				return ((A * 196314165U) + 907633515U) ^ ((B * 73148459U) + 453816763U);
			}

			int ComputeSeed(int A, int B, int C)
			{
				return ((A * 196314165U) + 907633515U) ^ ((B * 73148459U) + 453816763U) ^ ((C * 34731343U) + 453816743U);
			}

			int ComputeSeedFromPosition(const Vector3& InPosition)
			{
				return ComputeSeed(static_cast<int>(InPosition.x * 100.0f), static_cast<int>(InPosition.y * 100.0f), static_cast<int>(InPosition.z * 100.0f));
			}



			FBox GetActorLocalBounds(const ActorPtr& InActor, bool bIgnorePCGCreatedComponents)
			{
				FBox Box;

				if (InActor)
				{
					Box = InActor->getLocalBounds();
				}

				return Box;
			}
		}

	}
}
