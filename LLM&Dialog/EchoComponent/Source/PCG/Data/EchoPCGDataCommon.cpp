#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGDataCommon.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGUnionData.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		
		PCGTaggedData& PCGDataCollection::AddData(const PCGTaggedData& data)
		{
			taggedDataList.push_back(data);
			return taggedDataList.back();
		}

		void PCGDataCollection::AddData(const std::vector<PCGTaggedData>& inData)
		{
			for (const auto& inTaggedData : inData)
			{
				taggedDataList.push_back(inTaggedData);
			}
		}

		std::vector<PCGTaggedData> PCGDataCollection::GetInputsByPin(const Name& label) const
		{
			std::vector<PCGTaggedData> filteredInputs;

			for (const auto& taggedData : taggedDataList)
			{
				if (taggedData.pinLabel == label)
				{
					filteredInputs.push_back(taggedData);
				}
			}

			return filteredInputs;
		}

		std::vector<PCGTaggedData> PCGDataCollection::GetInputs() const
		{
			std::vector<PCGTaggedData> filteredInputs;

			for (const auto& taggedData : taggedDataList)
			{
				if (taggedData.data != nullptr)
				{
					filteredInputs.push_back(taggedData);
				}
			}

			return filteredInputs;
		}

		std::vector<PCGTaggedData> PCGDataCollection::GetSpatialInputsByPin(const Name& label) const
		{
			std::vector<PCGTaggedData> filteredInputs;

			for (const auto& taggedData : taggedDataList)
			{
				if (taggedData.pinLabel == label && taggedData.data && taggedData.data->IsA<PCGSpatialData>())
				{
					filteredInputs.push_back(taggedData);
				}
			}

			return filteredInputs;
		}

		int32 PCGDataCollection::GetInputCountByPin(const Name& InPinLabel) const {
			int32 Count = 0;
			for (const PCGTaggedData& Data : taggedDataList)
			{
				if (Data.pinLabel == InPinLabel)
				{
					++Count;
				}
			}
			return Count;
		}

		int32 PCGDataCollection::GetSpatialInputCountByPin(const Name& InPinLabel) const
		{
			int32 Count = 0;
			for (const PCGTaggedData& Data : taggedDataList)
			{
				if (Data.pinLabel == InPinLabel && Data.data && Data.data->IsA<PCGSpatialData>())
				{
					++Count;
				}
			}
			return Count;
		}

		const PCGSpatialData* PCGDataCollection::GetSpatialUnionOfInputsByPin(PCGContext* InContext, const Name& InPinLabel, bool& bOutUnionDataCreated) const {

			std::vector<PCGTaggedData> SpatialDataOnPin = GetSpatialInputsByPin(InPinLabel);
			;
			const PCGSpatialData* Result = nullptr;
			PCGUnionData* Union = nullptr;
			for (const PCGTaggedData& Data : SpatialDataOnPin)
			{
				const PCGSpatialData* SpatialInput = Data.data ? Data.data->Cast<PCGSpatialData>() : nullptr;
				if (SpatialInput == nullptr)
				{
					continue;
				}

				if (!Result)
				{
					// First valid data
					Result = SpatialInput;
				}
				else
				{
					if (!Union)
					{
						// Second valid data - set up union
						Union = PCGContext::NewObject<PCGUnionData>(InContext);
						Union->Initialize(Result, SpatialInput);

						// Make result union
						Result = Union;
					}
					else
					{
						// Nth valid data
						Union->AddData(SpatialInput);
					}
				}
			}

			// Indicate whether we created a union data
			bOutUnionDataCreated = !!Union;

			return Result;
		}

		const PCGSpatialData* PCGDataCollection::GetSpatialUnionOfInputsByPin(const Name& InPinLabel, bool& bOutUnionDataCreated) const {
			return GetSpatialUnionOfInputsByPin(nullptr, InPinLabel, bOutUnionDataCreated);
		}

		const PCGTransform PCGTransform::IDENTITY;

		PCGTransform::PCGTransform(const DMatrix4& matrix)
		{
			translation = matrix.getTrans();
			rotation = matrix.extractQuaternion();
			scale = DVector3(matrix.getScale());
		}

		DMatrix4 PCGTransform::ToMatrix() const
		{
			DMatrix4 matrix;
			matrix.makeTransform(translation, scale ,rotation);

			return matrix;
		}

		PCGTransform PCGTransform::Multiply(const DMatrix4& A, const PCGTransform& B)
		{
			DMatrix4 resultMatrix = A * B.ToMatrix();
			return PCGTransform(resultMatrix);
		}

		DVector3 PCGTransform::TransformPosition(const DVector3& localPos) const
		{
			return (rotation * (localPos * scale)) + translation;
		}

		Quaternion PCGTransform::TransformRotation(const Quaternion& localRot) const
		{
			return rotation * localRot;
		}

		Vector3 PCGTransform::TransformScale(const Vector3& localScl) const
		{
			return scale * localScl;
		}

		PCGTransform PCGTransform::Transform(const PCGTransform& localTrans) const
		{
			PCGTransform trans;
			trans.translation = TransformPosition(localTrans.translation);
			trans.rotation = TransformRotation(localTrans.rotation);
			trans.scale = TransformScale(localTrans.scale);
			return trans;
		}

		AxisAlignedBox PCGTransform::Transform(const AxisAlignedBox& localBound) const
		{
			AxisAlignedBox bound;
			Vector3 verts[8];
			localBound.GetVerts(verts);
			for (int i = 0; i != 8; ++i) {
				bound.merge(TransformPosition(verts[i]));
			}
			return bound;
		}



		DVector3 PCGTransform::InverseTransformPosition(const DVector3& worldPos) const
		{
			return rotation.Inverse() * (worldPos - translation) / scale;
		}

		Quaternion PCGTransform::InverseTransformRotation(const Quaternion& worldRot) const
		{
			return rotation.Inverse() * worldRot;
		}

		Vector3 PCGTransform::InverseTransformScale(const Vector3& worldScl) const
		{
			return worldScl / scale;
		}

		PCGTransform PCGTransform::InverseTransform(const PCGTransform& worldTrans) const
		{
			PCGTransform trans;
			trans.translation = InverseTransformPosition(worldTrans.translation);
			trans.rotation = InverseTransformRotation(worldTrans.rotation);
			trans.scale = InverseTransformScale(worldTrans.scale);
			return trans;
		}

		AxisAlignedBox PCGTransform::InverseTransform(const AxisAlignedBox& worldBound) const
		{
			AxisAlignedBox bound;
			Vector3 verts[8];
			worldBound.GetVerts(verts);
			for (int i = 0; i != 8; ++i) {
				bound.merge(InverseTransformPosition(verts[i]));
			}
			return bound;
		}
	}
}
