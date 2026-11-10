#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGSurfaceSampler.h"

#include <random>

#include "PCG/EchoPCGHelper.h"
#include "PCG/EchoPCGComponent.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGSurfaceSampler
		{
			PCGPointData* SampleSurface(PCGContext* context, const SurfaceSamplerParams& params, const PCGSpatialData* inSurface, const PCGSpatialData* inBoundingShape)
			{
				PCGPointData* sampledData = PCGContext::NewObject<PCGPointData>(context);
				sampledData->InitializeFromData(inSurface);
				std::vector<PCGPoint>& sampledPoints = sampledData->GetMutablePoints();

				// Try to use a multiplier instead of a simply offset to combat loss of precision in floats. However if MaxZ is very small,
				// then multiplier will not work, so just use an offset.
				constexpr double yMultiplier = 1.0 - 1e-6;
				double sampleY = (Math::Abs(params.inputBoundsMaxY) > ECHO_EPSILON) ? params.inputBoundsMaxY * yMultiplier : -ECHO_EPSILON;

				for (int32 cellIndex = 0; cellIndex < params.cellCount; ++cellIndex)
				{
					const EchoIndex2 indices = params.ComputeCellIndices(cellIndex);
					const double currentX = indices.x * params.cellSize.x;
					const double currentZ = indices.z * params.cellSize.z;
					const Vector3 innerCellSize = params.innerCellSize;

					int currentSeed = PCGHelpers::ComputeSeed(params.seed, indices.x, indices.z);

					std::mt19937 randomEngine(currentSeed);
					std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

					const float chance = distribution(randomEngine);
					const float ratio = params.ratio;

					if (chance >= ratio)
					{
						continue;
					}

					const float randX = distribution(randomEngine);
					const float randZ = distribution(randomEngine);

					DVector3 tentativeLocation = DVector3{
						currentX + randX * innerCellSize.x,
						sampleY,
						currentZ + randZ * innerCellSize.z
					};
					tentativeLocation = params.PreProjectionTransform.TransformPosition(tentativeLocation);

					PCGPoint& point = sampledPoints.emplace_back();

					point.SetExtents(params.pointExtents);
					point.steepness = params.pointSteepness;

					// TODO: Handle inBoundingShape
					inSurface->ProjectPoint(tentativeLocation, AxisAlignedBox{}, PCGProjectionParams{}, point, nullptr);
					//inSurface->ProjectPoint(tentativeTransform, inBoundingShape->GetBounds(), PCGProjectionParams{}, point);

					point.density = (ratio - chance) / ratio;
					point.seed = currentSeed;
				}

				return sampledData;
			}

			bool SurfaceSamplerParams::Initialize(const PCGSurfaceSamplerSettings* settings, const PCGContext* context, const AxisAlignedBox& inputBounds, const PCGTransform& inputTransforms)
			{
				if (!context)
				{
					return false;
				}

				if (settings)
				{
					pointsPerSquaredMeter = settings->m_PointsPerSquaredMeter;
					pointExtents = settings->m_PointExtents;
					looseness = settings->m_Looseness;
					bApplyDensityToPoints = settings->m_bApplyDensityToPoints;
					pointSteepness = settings->m_PointSteepness;

					seed = context->GetSeed();

					// break down the surface bounds in a N x M grid
					interstitialDistance = pointExtents * 2;
					innerCellSize = interstitialDistance * looseness;
					cellSize = innerCellSize + interstitialDistance;

					if (cellSize.x <= 0 || cellSize.z <= 0)
					{
						return false;
					}

					{
						PreProjectionTransform.SetTranslation(inputTransforms.GetTranslation());
						PreProjectionTransform.SetRotation(inputTransforms.GetRotation());
						PreProjectionTransform.SetScale(Vector3::UNIT_SCALE);
					}

					cellMinX = Math::CeilToTnt((inputBounds.getMinimum().x / cellSize.x));
					cellMaxX = Math::FloorToTnt((inputBounds.getMaximum().x / cellSize.x));
					cellMinZ = Math::CeilToTnt((inputBounds.getMinimum().z / cellSize.z));
					cellMaxZ = Math::FloorToTnt((inputBounds.getMaximum().z / cellSize.z));

					const int32 cellCountX = cellMaxX - cellMinX + 1;
					const int32 cellCountY = cellMaxZ - cellMinZ + 1;

					if (cellCountX <= 0 || cellCountY <= 0)
					{
						return false;
					}

					cellCount = cellCountX * cellCountY;
					assert(cellCount > 0);

					Vector3 boundSize = inputBounds.getSize();
					const float targetPointCount = (boundSize.x * boundSize.z) * pointsPerSquaredMeter;

					ratio = Math::Clamp(targetPointCount / cellCount, 0.0f, 1.0f);

					if (ratio < ECHO_EPSILON)
					{
						return false;
					}

					inputBoundsMinY = inputBounds.getMinimum().y;
					inputBoundsMaxY = inputBounds.getMaximum().y;

					return true;
				}

				return false;
			}

			EchoIndex2 SurfaceSamplerParams::ComputeCellIndices(int32 index) const
			{
				assert(index >= 0 && index < cellCount);
				const int32 cellCountX = 1 + cellMaxX - cellMinX;

				return EchoIndex2(cellMinX + (index % cellCountX), cellMinZ + (index / cellCountX));
			}
		}

		PCGSurfaceSamplerSettings::PCGSurfaceSamplerSettings()
			: PCGSettings()
		{
		}

		PCGElement* PCGSurfaceSamplerSettings::GetElement() const
		{
			return PCGSurfaceSamplerElement::GetInstance();
		}

		PCGSettings* PCGSurfaceSamplerSettings::Clone() const
		{
			return CloneT<PCGSurfaceSamplerSettings>();
		}

		REGISTER_PCG_CLASS(PCGSurfaceSamplerSettings)
		PCGClass* PCGSurfaceSamplerSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGSurfaceSamplerSettings)
				Class.AddProperty("PointsPerSquaredMeter", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_PointsPerSquaredMeter));
				Class.AddProperty("PointExtents", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_PointExtents));
				Class.AddProperty("Looseness", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_Looseness));
				Class.AddProperty("bApplyDensityToPoints", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_bApplyDensityToPoints));
				Class.AddProperty("PointSteepness", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_PointSteepness));
				Class.AddProperty("bUnbounded", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGSurfaceSamplerSettings, m_bUnbounded));

				PCGSettings::AppendPublicPCGSettingsProperties<PCGSurfaceSamplerSettings>(Class);
				RegisterPCGSettings("PCGSurfaceSamplerSettings");
			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGSurfaceSamplerSettings::InputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGSurfaceSamplerConstants::SurfaceLabel, PCGDataType::Surface).SetRequiredPin();
			properties.emplace_back(PCGSurfaceSamplerConstants::BoundingShapeLabel, PCGDataType::Spatial);
#ifdef ECHO_EDITOR
			properties[0].tooltip = "SurfaceSamplerSurfacePinTooltip\n" \
				"The surface to sample with points. Points will be generated in the two dimensional footprint of the combined bounds of the Surface and the Bounding Shape (if any) \n" \
				"and then projected onto this surface. If this input is omitted then the network of shapes connected to the Bounding Shape pin will be inspected for a surface \n" \
				"shape to use to project the points onto.";
			properties[1].tooltip = "SurfaceSamplerBoundingShapePinTooltip\n"\
				"All sampled points must be contained within this shape. If this input is omitted then bounds will be taken from the actor so that points are contained within actor bounds. \n"\
				"The Unbounded property disables this and instead generates over the entire bounds of Surface.";
#endif // ECHO_EDITOR
			return properties;
		}

		std::vector<PCGPinProperties> PCGSurfaceSamplerSettings::OutputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultOutputLabel, PCGDataType::Point);
			return properties;
		}

		PCGElement* PCGSurfaceSamplerElement::GetInstance()
		{
			static PCGSurfaceSamplerElement* instance = new PCGSurfaceSamplerElement();
			return instance;
		}

		PCGContext* PCGSurfaceSamplerElement::CreateContext()
		{
			return new PCGSurfaceSamplerContext();
		}

		bool PCGSurfaceSamplerElement::PrepareDataInternal(PCGContext* inContext)
		{
			assert(inContext);
			PCGSurfaceSamplerContext* context = static_cast<PCGSurfaceSamplerContext*>(inContext);
			const PCGSurfaceSamplerSettings* settings = context->GetInputSettings<PCGSurfaceSamplerSettings>();
			const std::vector<PCGTaggedData> surfaceInputs = context->inputData.GetInputsByPin(PCGSurfaceSamplerConstants::BoundingShapeLabel);
			if (surfaceInputs.empty()) {
				auto pinnedPCGComponent = context->sourceComponent.lock();
				if (pinnedPCGComponent)
				{
					PCGTransform trans;
					trans.SetTranslation(pinnedPCGComponent->getActorPtr()->getWorldPosition());
					trans.SetRotation(pinnedPCGComponent->getActorPtr()->getWorldRotation());
					trans.SetScale(Vector3::UNIT_SCALE);
					Vector3 bounds = pinnedPCGComponent->GetBounds();
					AxisAlignedBox box(-bounds, bounds);
					context->samplerParamsArray.resize(1);
					context->samplerParamsArray[0].Initialize(settings, inContext, box, trans);
				}
			}
			else {
				for (const auto& input : surfaceInputs)
				{
					const PCGPointData* pointData = input.data->Cast<PCGPointData>();
					const std::vector<PCGPoint>& Points = pointData->GetPoints();
					for (const auto& point : Points) {
						auto& samplerParams = context->samplerParamsArray.emplace_back();
						samplerParams.Initialize(settings, inContext, point.GetLocalBounds(), point.transform);
					}
				}
			}

			return true;
		}

		bool PCGSurfaceSamplerElement::ExecuteInternal(PCGContext* inContext)
		{
			assert(inContext);
			PCGSurfaceSamplerContext* context = static_cast<PCGSurfaceSamplerContext*>(inContext);
			const PCGSurfaceSamplerSettings* settings = context->GetInputSettings<PCGSurfaceSamplerSettings>();
			assert(settings);

			const std::vector<PCGTaggedData> surfaceInputs = context->inputData.GetInputsByPin(PCGSurfaceSamplerConstants::SurfaceLabel);
			std::vector<PCGTaggedData>& outputs = context->outputData.taggedDataList;
			if (surfaceInputs.empty())
			{
				return false;
			}

			for (const auto& input : surfaceInputs)
			{
				const PCGSpatialData* surface = input.data->Cast<PCGSpatialData>();
				for (const auto& samplerParams : context->samplerParamsArray) {
					PCGPointData* pointData = PCGSurfaceSampler::SampleSurface(context, samplerParams, surface, nullptr);
					PCGTaggedData& output = outputs.emplace_back();
					output.data = pointData;
				}
			}

			return true;
		}

	}
}

