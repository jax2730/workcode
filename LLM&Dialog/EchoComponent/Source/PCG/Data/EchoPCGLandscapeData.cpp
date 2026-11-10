#include "EchoComponentStableHeaders.h"

#include "EchoOAESystem.h"
#include "PCG/Data/EchoPCGLandscapeData.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGLandscape
		{
			std::shared_ptr<PCGLandscapeInterface> GetPCGLandscape_OAE()
			{
				return std::make_shared<PCGLandscape_OAE>();
			}
		}

		PCGLandscape_OAE::PCGLandscape_OAE()
		{
			m_OAEManager = OAE::OAESystem::instance()->getMainManager();
		}

		PCGLandscape_OAE::~PCGLandscape_OAE()
		{

		}

		double PCGLandscape_OAE::GetInterpolatedHeight(double x, double z)
		{
			return m_OAEManager->getTerrainHeight(x, z);
		}

		void PCGLandscapeData::Initialize()
		{
			m_Landscape = PCGLandscape::GetPCGLandscape_OAE();
		}

		AxisAlignedBox PCGLandscapeData::GetBounds() const
		{
			return m_Bounds;
		}

		bool PCGLandscapeData::ProjectPoint(const PCGTransform& inTransform, const AxisAlignedBox& inBounds, const PCGProjectionParams& inParams, PCGPoint& outPoint, PCGMetadata* OutMetadata) const
		{
			// TODO: Get tangents and normal, now only get height
			DVector3 oldLocation = inTransform.GetLocation();
			if (!m_Landscape)
			{
				outPoint.transform.SetLocation(DVector3{ oldLocation.x, 0.0, oldLocation.z });
				return true;
			}

			double height = m_Landscape->GetInterpolatedHeight(oldLocation.x, oldLocation.z);
			outPoint.transform.SetLocation(DVector3{ oldLocation.x, height, oldLocation.z });
			return true;
		}
	}
}
