#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/Data/EchoPCGSpatialData.h"

namespace Echo
{
	namespace PCGV2
	{
		struct PCGPoint;
		class PCGLandscapeInterface;

		namespace PCGLandscape
		{
			std::shared_ptr<PCGLandscapeInterface> GetPCGLandscape_OAE();
		}


		class _EchoComponentExport PCGLandscapeInterface
		{
		public:
			virtual ~PCGLandscapeInterface() = default;

			virtual double GetInterpolatedHeight(double x, double z) = 0;
		};

		class _EchoComponentExport PCGLandscape_OAE : public PCGLandscapeInterface
		{
		public:
			PCGLandscape_OAE();
			virtual ~PCGLandscape_OAE();

			virtual double GetInterpolatedHeight(double x, double z) override;

		private:
			OAE::OAEManager* m_OAEManager;
		};

		struct PCGLandscapeDataProps
		{
			bool bGetHeightOnly = false;
		};

		class _EchoComponentExport PCGLandscapeData : public PCGSpatialData
		{
			PCGDATA_BODY(PCGLandscapeData, PCGSpatialData)
		public:
			void Initialize();

			virtual PCGDataType GetDataType() const override { return PCGDataType::Landscape; };

			virtual AxisAlignedBox GetBounds() const override;

			virtual bool ProjectPoint(const PCGTransform& inTransform, const AxisAlignedBox& inBounds, const PCGProjectionParams& inParams, PCGPoint& outPoint, PCGMetadata* OutMetadata) const override;

		private:
			// Landscape data
			std::shared_ptr<PCGLandscapeInterface> m_Landscape;

			AxisAlignedBox m_Bounds;

			PCGLandscapeDataProps m_DataProps;
		};
	}
}
