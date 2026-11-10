#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGSpatialData;
		class PCGPointData;
		class PCGSurfaceSamplerSettings;

		namespace PCGSurfaceSamplerConstants
		{
			const Name SurfaceLabel = Name("Surface");
			const Name BoundingShapeLabel = Name("Bounding Shape");
		}

		namespace PCGSurfaceSampler
		{
			struct SurfaceSamplerParams
			{
				float pointsPerSquaredMeter = 1.0f;
				Vector3 pointExtents = Vector3::UNIT_SCALE * 0.5f;
				float looseness = 0.0f;
				bool bApplyDensityToPoints = false;
				float pointSteepness = 0.0f;

				bool Initialize(const PCGSurfaceSamplerSettings* settings, const PCGContext* context, const AxisAlignedBox& inputBounds, const PCGTransform& inputTransform);

				EchoIndex2 ComputeCellIndices(int32 index) const;

				/** Computed values **/
				Vector3 interstitialDistance;
				Vector3 innerCellSize;
				Vector3 cellSize;

				int32 cellMinX;
				int32 cellMaxX;
				int32 cellMinZ;
				int32 cellMaxZ;
				int32 cellCount;
				float ratio;
				int seed;

				float inputBoundsMinY;
				float inputBoundsMaxY;

				PCGTransform PreProjectionTransform = PCGTransform::IDENTITY;
			};

			PCGPointData* SampleSurface(PCGContext* context, const SurfaceSamplerParams& params, const PCGSpatialData* inSurface, const PCGSpatialData* inBoundingShape);
		}


		class PCGSurfaceSamplerSettings : public PCGSettings
		{
		public:
			PCGSurfaceSamplerSettings();

			PCG_OBJECT_BODY(PCGSurfaceSamplerSettings)

			virtual PCGElement* GetElement() const override;

			virtual Name GetDefaultNodeTitle() const override { return Name("Surface Sampler"); }

			virtual PCGSettings* Clone() const override;

		protected:
			virtual std::vector<PCGPinProperties> InputPinProperties() const override;

			virtual std::vector<PCGPinProperties> OutputPinProperties() const override;

		public:
			float m_PointsPerSquaredMeter = 1.0f;

			Vector3 m_PointExtents = Vector3{ 1.0f };

			float m_Looseness = 1.0f;

			bool m_bApplyDensityToPoints = true;

			float m_PointSteepness = 0.5f;

			bool m_bUnbounded = false;
		};

		class PCGSurfaceSamplerElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();

		protected:
			virtual PCGContext* CreateContext() override;

			bool PrepareDataInternal(PCGContext* inContext) override;

			bool ExecuteInternal(PCGContext* inContext) override;
		};

		struct PCGSurfaceSamplerContext : public PCGContext
		{
			std::vector<PCGSurfaceSampler::SurfaceSamplerParams> samplerParamsArray;
		};
	}
}

