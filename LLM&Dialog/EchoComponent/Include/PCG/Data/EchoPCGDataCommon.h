#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"

namespace Echo
{
	namespace PCGV2
	{
		class _EchoComponentExport PCGData : public PCGBase
		{
		public:
			PCGData() {}
			virtual ~PCGData() {}

			virtual PCGDataType GetDataType() const { return PCGDataType::None; };

			static const Name& StaticClass() { static const Name gClass = Name("PCGData"); return gClass; }
			virtual bool IsBaseOf(const Name& _class) const { return _class == PCGData::StaticClass(); }

			template<typename PCGBASE>
			bool IsA() const {
				return IsBaseOf(PCGBASE::StaticClass());
			}

			template<typename PCGBASE>
			PCGBASE* Cast() {
				if (IsA<PCGBASE>()) {
					return (PCGBASE*)this;
				}
				return nullptr;
			}

			template<typename PCGBASE>
			const PCGBASE* Cast() const {
				if (IsA<PCGBASE>()) {
					return (const PCGBASE*)this;
				}
				return nullptr;
			}
		};

#define PCGDATA_BODY(CLASSNAME, PARENTCLASSNAME)\
public:\
static const Name& StaticClass() { static const Name gClass = Name(#CLASSNAME); return gClass; };\
virtual bool IsBaseOf(const Name& _class) const override { return _class == CLASSNAME::StaticClass() || PARENTCLASSNAME::IsBaseOf(_class); }\

		struct _EchoComponentExport PCGTaggedData
		{
			TObjectPtr<const PCGData> data = nullptr;

			std::set<String> tags;

			Name pinLabel = Name::g_cEmptyName;
		};

		class PCGSpatialData;

		struct _EchoComponentExport PCGDataCollection
		{
			PCGTaggedData& AddData(const PCGTaggedData& data);

			void AddData(const std::vector<PCGTaggedData>& inData);

			std::vector<PCGTaggedData> GetInputsByPin(const Name& label) const;

			std::vector<PCGTaggedData> GetInputs() const;

			std::vector<PCGTaggedData> GetSpatialInputsByPin(const Name& label) const;

			int32 GetInputCountByPin(const Name& InPinLabel) const;

			int32 GetSpatialInputCountByPin(const Name& InPinLabel) const;

			const PCGSpatialData* GetSpatialUnionOfInputsByPin(PCGContext* InContext, const Name& InPinLabel, bool& bOutUnionDataCreated) const;

			const PCGSpatialData* GetSpatialUnionOfInputsByPin(const Name& InPinLabel, bool& bOutUnionDataCreated) const;

			std::vector<PCGTaggedData> taggedDataList;
		};

		struct _EchoComponentExport PCGTransform
		{
			DVector3 translation = Vector3::ZERO;

			Quaternion rotation = Quaternion::IDENTITY;

			Vector3 scale = Vector3::UNIT_SCALE;

			static const PCGTransform IDENTITY;

			PCGTransform() {}

			PCGTransform(const DVector3& _translation, const Quaternion& _rotation = Quaternion::IDENTITY, const Vector3& _scale = Vector3::UNIT_SCALE) : translation(_translation), rotation(_rotation), scale(_scale) {}

			PCGTransform(const DMatrix4& matrix);

			ECHO_FORCE_INLINE DVector3 GetLocation() const { return translation; }

			ECHO_FORCE_INLINE void SetLocation(const DVector3& newLocation) { SetTranslation(newLocation); }

			ECHO_FORCE_INLINE DVector3 GetTranslation() const { return translation; }

			ECHO_FORCE_INLINE void SetTranslation(const DVector3& newTranslation) { translation = newTranslation; }

			ECHO_FORCE_INLINE Quaternion GetRotation() const { return rotation; }

			ECHO_FORCE_INLINE void SetRotation(const Quaternion& newRotation) { rotation = newRotation; }

			ECHO_FORCE_INLINE Vector3 GetScale() const { return scale; }

			ECHO_FORCE_INLINE void SetScale(const Vector3& newScale) { scale = newScale; }

			DVector3 TransformPosition(const DVector3&) const;

			Quaternion TransformRotation(const Quaternion&) const;

			Vector3 TransformScale(const Vector3&) const;

			PCGTransform Transform(const PCGTransform&) const;

			AxisAlignedBox Transform(const AxisAlignedBox&) const;

			DVector3 InverseTransformPosition(const DVector3&) const;

			Quaternion InverseTransformRotation(const Quaternion&) const;

			Vector3 InverseTransformScale(const Vector3&) const;

			PCGTransform InverseTransform(const PCGTransform&) const;

			AxisAlignedBox InverseTransform(const AxisAlignedBox&) const;

			DMatrix4 ToMatrix() const;

			static PCGTransform Multiply(const DMatrix4& A, const PCGTransform& B);
		};


		class PCGMetadata : public PCGBase
		{
		public:
			PCGMetadata(const PCGData*) {}
			~PCGMetadata() {}

			void Initialize(const PCGMetadata*) {}
			void AddAttributes(const PCGMetadata*) {}
		};
	}
}
