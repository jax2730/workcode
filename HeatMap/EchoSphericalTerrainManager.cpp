#include "EchoStableHeaders.h"
#include "EchoSphericalTerrainManager.h"

#include <thread>

#include "EchoBiomeVegetation.h"
#include "EchoBiomeVegGroup.h"
#include "EchoDebugDrawManager.h"
#include "EchoMaterialManager.h"
#include "EchoShaderContentManager.h"
#include "EchoShadowmapManager.h"
#include "EchoTextureManager.h"
#include "EchoTimer.h"
#include "EchoTorusTerrainNode.h"
#include "JobSystem/EchoCommonJobManager.h"
#include "EchoPostProcess.h"
#include "EchoVolumetricCloud.h"
#include "EchoSphericalTerrainFog.h"
#include "DeformSnow.h"
#include "EchoDeformSnow.h"
#include "EchoMaterialV2Manager.h"

#include "EchoTerrainModify3D.h"
#include "EchoTerrainStaticModify3D.h"
#include "EchoUberNoiseFBM3D.h"

#include "EchoPlanetWarfogRenderable.h"
#include "EchoSphericalOcean.h"

#include "EchoTorusTerrainNode.h"
#include "EchoTODManager.h"

#define OUTPUT_HEIGHTMAP 0
#if OUTPUT_HEIGHTMAP
#include "DirectXTex.h"
#endif
#include "EchoPlanetRoad.h"
#include "EchoPointLightShadowManager.h"

using namespace Echo;

namespace Echo {

    static void logToConsole(const String& log)
	{
#ifdef _WIN32
		OutputDebugString(log.c_str());
#else
		LogManager::instance()->logMessage(log.c_str());
#endif
	}
    
	class SphericalTerrainExportToolResourceLoader : public ManualResourceLoader {
	public:
		SphericalTerrainExportToolResourceLoader() {}
		virtual void prepareResource(ResourceBase* resource) override {

		}
		virtual void loadResource(ResourceBase* resource) override {
			SphericalTerrainResource* res = (SphericalTerrainResource*)resource;
			res->m_bLoadResult = SphericalTerrainResource::loadSphericalTerrain(res->getName().toString(), *res, false);

			if (!res->bExistResource) return;

			if (!res->geometry.StampTerrainInstances.empty())
			{
				using StampTerrain3D::HeightMapDataManager;
				int size = (int)res->geometry.StampTerrainInstances.size();
				// res->mTextures.resize(size);
				for (int i = 0; i != size; ++i) {
					auto&& path = res->geometry.StampTerrainInstances[i].templateID;

					auto& geometry = res->geometry;
					try {
						geometry.StampTerrainInstances[i].mHeightmap = HeightMapDataPtr(HeightMapDataManager::instance()->prepare(Name(path)));
					}
					catch (...)
					{
						geometry.StampTerrainInstances[i].mHeightmap.setNull();
					}
					if (!geometry.StampTerrainInstances[i].mHeightmap.isNull())
					{
						if (!HeightMapDataManager::isResolutionLegal(geometry.StampTerrainInstances[i].mHeightmap))
						{
							LogManager::instance()->logMessage("-error-\t " + path + " file is not of the resolution allowed!", LML_CRITICAL);
						}
					}
				}
			}
			for (int i = 0; i < res->matIndexMaps.size(); ++i)
			{
				PlanetManager::readBitmap(res->m_matIndexMaps[i], res->matIndexMaps[i].c_str());
			}
			if (res->geometry.topology == PTS_Sphere) {
				res->m_pTerrainGenerator = TerrainGeneratorLibrary::instance()->add(res->geometry.sphere.gempType);
			}
		}
	};
	static SphericalTerrainExportToolResourceLoader gExportToolLoader;

};

namespace
{
	std::string SHAPE_MACRO[2] =
	{
		"PROCEDURAL_SPHERE", "PROCEDURAL_TORUS"
	};

	struct NoiseUniforms
	{
		Vector4 featureNoiseSeed;
		Vector4 slopeErosionNoiseSeed;
		Vector4 sharpnessNoiseSeed;
		Vector4 perturbNoiseSeed;

		float baseFrequency;
		float baseAmplitude;
		float lacunarity;
		float gain;

		Vector2 sharpness;
		float sharpnessBaseFrequency;
		float detailAmplitude;

		Vector2 slopeErosion;
		float slopeErosionBaseFrequency;
		int octaves;

		Vector2 perturb;
		float perturbBaseFrequency;
		float pad;

		NoiseUniforms() = delete;

		explicit NoiseUniforms(const UberNoiseFBM3D* n): pad(0)
		{
			const auto seed0 = n->GetFeatureNoiseSeed();
			const auto seed1 = n->GetSlopeErosionNoiseSeed();
			const auto seed2 = n->GetSharpnessNoiseSeed();
			const auto seed3 = n->GetPerturbNoiseSeed();

			featureNoiseSeed      = Vector4(seed0.x, seed0.y, seed0.z, 0.0f);
			slopeErosionNoiseSeed = Vector4(seed1.x, seed1.y, seed1.z, 0.0f);
			sharpnessNoiseSeed    = Vector4(seed2.x, seed2.y, seed2.z, 0.0f);
			perturbNoiseSeed      = Vector4(seed3.x, seed3.y, seed3.z, 0.0f);

			baseFrequency   = n->GetBaseFrequency();
			baseAmplitude   = n->GetBaseAmplitude();
			detailAmplitude = n->GetDetailAmplitude();
			lacunarity      = n->GetLacunarity();
			gain            = n->GetGain();
			octaves         = n->GetBaseOctaves();

			const auto _sharpness  = n->GetSharpness();
			sharpness              = Vector2(_sharpness.x, _sharpness.y);
			sharpnessBaseFrequency = n->GetSharpnessFrequency();

			const auto _slopeErosion  = n->GetSlopeErosion();
			slopeErosion              = Vector2(_slopeErosion.x, _slopeErosion.y);
			slopeErosionBaseFrequency = n->GetSlopeErosionFrequency();

			const auto _perturb  = n->GetPerturb();
			perturb              = Vector2(_perturb.x, _perturb.y);
			perturbBaseFrequency = n->GetPerturbFrequency();
		}
	};

	struct GridUniforms
	{
		Vector2 offset;
		Vector2 gridSize;
		uint32_t slice;
		float minorRadius;
		Vector2 padding;
	};

	constexpr int GRID_RESOLUTION                           = SphericalTerrainQuadTreeNode::GRID_RESOLUTION;
	constexpr int EXTRA_GRID_RESOLUTION                     = GRID_RESOLUTION + 2;
	constexpr SphericalTerrainQuadTreeNode::IndexType QUADS = GRID_RESOLUTION - 1;
	constexpr int INDEX_COUNT                               = 6 * QUADS * QUADS;
	constexpr int VERTEX_COUNT                              = SphericalTerrainQuadTreeNode::TILE_VERTICES;
	constexpr int EXTRA_VERTEX_COUNT                        = EXTRA_GRID_RESOLUTION * EXTRA_GRID_RESOLUTION;
	constexpr int VB_BYTE_SIZE                              = VERTEX_COUNT * sizeof(TerrainVertex);

	std::vector<SphericalTerrainQuadTreeNode::IndexType> CreateNodeIndicesSimplified(const int simplificationLevel)
	{
		using IndexType = SphericalTerrainQuadTreeNode::IndexType;
		std::vector<IndexType> indices;

		const int quad = QUADS >> simplificationLevel;
		const uint16_t stride = 1 << simplificationLevel;

		indices.reserve(quad * quad * 6);

		for (IndexType j = 0; j < QUADS; j += stride)
		{
			for (IndexType k = 0; k < QUADS; k += stride)
			{
				IndexType i0 = j * GRID_RESOLUTION + k;
				IndexType i1 = j * GRID_RESOLUTION + k + stride;
				IndexType i2 = (j + stride) * GRID_RESOLUTION + k;
				IndexType i3 = (j + stride) * GRID_RESOLUTION + k + stride;

				if ((k / stride % 2 == 0 && j / stride % 2 == 0) || (k / stride % 2 == 1 && j / stride % 2 == 1))
				{
					indices.emplace_back(i0);
					indices.emplace_back(i3);
					indices.emplace_back(i2);

					indices.emplace_back(i0);
					indices.emplace_back(i1);
					indices.emplace_back(i3);
				}
				else //if ((k % 2 == 0 && j % 2 == 1) || (k % 2 == 1 && j % 2 == 0))
				{
					indices.emplace_back(i0);
					indices.emplace_back(i1);
					indices.emplace_back(i2);

					indices.emplace_back(i2);
					indices.emplace_back(i1);
					indices.emplace_back(i3);
				}
			}
		}

		return indices;
	}

	std::vector<SphericalTerrainQuadTreeNode::IndexType> CreateNodeIndices(const SphericalTerrainQuadTreeNode::EdgeMask mask)
	{
		using IndexType = SphericalTerrainQuadTreeNode::IndexType;
		std::vector<IndexType> indices;

		indices.reserve(INDEX_COUNT);
		//
		//	2____________3
		//	|            |
		//	|            |
		//	|            |
		//	|            |
		//	|            |
		//	0____________1
		//

		using Edge = SphericalTerrainQuadTreeNode::Edge;
		auto stitchOnSide = [](IndexType row, IndexType col, const SphericalTerrainQuadTreeNode::EdgeMask pattern) -> IndexType
		{
			if (((pattern & (1 << Edge::Top)) && row == GRID_RESOLUTION - 1 && col % 2) ||
				(pattern & (1 << Edge::Btm) && row == 0 && col % 2))
			{
				++col;
			}
			else if ((pattern & (1 << Edge::Lft) && col == 0 && row % 2) ||
				(pattern & (1 << Edge::Rht) && col == GRID_RESOLUTION - 1 && row % 2))
			{
				++row;
			}

			return row * GRID_RESOLUTION + col;
		};

		auto cleanup = [](const std::vector<IndexType>& in)
		{
			std::vector<IndexType> out;
			out.reserve(in.size());

			for (size_t i = 0; i < in.size(); i += 3)
			{
				const auto i0 = in[i];
				const auto i1 = in[i + 1];
				const auto i2 = in[i + 2];

				if (i0 != i1 && i1 != i2 && i2 != i0)
				{
					out.emplace_back(i0);
					out.emplace_back(i1);
					out.emplace_back(i2);
				}
			}

			return out;
		};

		for (IndexType j = 0; j < QUADS; ++j)
		{
			for (IndexType k = 0; k < QUADS; ++k)
			{
				IndexType i0 = stitchOnSide(j, k, mask);			//j * GRID_RESOLUTION + k;
				IndexType i1 = stitchOnSide(j, k + 1, mask);		//j * GRID_RESOLUTION + k + 1;
				IndexType i2 = stitchOnSide(j + 1, k, mask);		//(j + 1) * GRID_RESOLUTION + k;
				IndexType i3 = stitchOnSide(j + 1, k + 1, mask);	//(j + 1) * GRID_RESOLUTION + k + 1;

				if ((k % 2 == 0 && j % 2 == 0) || (k % 2 == 1 && j % 2 == 1))
				{
					indices.emplace_back(i0);
					indices.emplace_back(i3);
					indices.emplace_back(i2);

					indices.emplace_back(i0);
					indices.emplace_back(i1);
					indices.emplace_back(i3);
				}
				else //if ((k % 2 == 0 && j % 2 == 1) || (k % 2 == 1 && j % 2 == 0))
				{
					indices.emplace_back(i0);
					indices.emplace_back(i1);
					indices.emplace_back(i2);

					indices.emplace_back(i2);
					indices.emplace_back(i1);
					indices.emplace_back(i3);
				}
			}
		}

		return cleanup(indices);
	}

	std::array<int, 3> GetQuadTriangleWindingOrder(const int intX, const int intY, const float fractX, const float fractY)
	{
		std::array triangle { 0, 0, 0 };
		if ((intX % 2 == 0 && intY % 2 == 0) || (intX % 2 == 1 && intY % 2 == 1))
		{
			if (fractY > fractX) // top Left
			{
				triangle[0] = 0;
				triangle[1] = 3;
				triangle[2] = 2;
			}
			else // bottom right
			{
				triangle[0] = 0;
				triangle[1] = 1;
				triangle[2] = 3;
			}
		}
		// bottom left & top right
		else
		{
			if (fractY < 1 - fractX) // bottom left
			{
				triangle[0] = 0;
				triangle[1] = 1;
				triangle[2] = 2;
			}
			else // top right
			{
				triangle[0] = 2;
				triangle[1] = 1;
				triangle[2] = 3;
			}
		}
		return triangle;
	}

	std::vector<SphericalTerrainQuadTreeNode::IndexType> CreateCubeSphereIndices(
		const std::vector<SphericalTerrainQuadTreeNode::IndexType>& faceIb)
	{
		using IndexType = SphericalTerrainQuadTreeNode::IndexType;
		static_assert(VERTEX_COUNT * 6 < std::numeric_limits<IndexType>::max() + 1);
		const size_t faceIdxCnt = faceIb.size();
		std::vector<IndexType> indices(6 * faceIdxCnt);
		for (size_t f = 0; f < 6; ++f)
		{
			std::transform(faceIb.cbegin(), faceIb.cend(), indices.begin() + static_cast<long long>(f * faceIdxCnt),
				[f](const IndexType idx) { return idx + static_cast<IndexType>(f * VERTEX_COUNT); });
		}
		return indices;
	}

	ColorValue LodColor[5] =
	{
		ColorValue { 0.5, 0, 0, 1 },
		ColorValue { 0, 0.5, 0, 1 },
		ColorValue { 0, 0, 0.5, 1 },
		ColorValue { 0.5, 0.5, 0, 1 },
		ColorValue { 0, 0.5, 0.5, 1 }
	};
	int BoxIndices[24] =
	{
		0, 1, 1, 2, 2, 3, 3, 0,
		0, 4, 1, 5, 2, 6, 3, 7,
		4, 5, 5, 6, 6, 7, 7, 4
	};

	void DrawBound(Vector3 vert[8], const ColorValue& col)
	{
		DebugDrawManager::instance()->pushLines(
			vert, 8, BoxIndices, static_cast<int>(std::size(BoxIndices)), col);
	}

#ifdef ECHO_EDITOR
	enum WorldMapType : uint8_t
	{
		ElevationWorldMap = 0,
		TemperatureWorldMap,
		HumidityWorldMap
	};

	class SimpleImage;
	MaterialPtr g_worldMapMat;
	std::unique_ptr<RenderOperation> g_imageRenderOp;
	std::unique_ptr<Pass> g_cylindricalProjection = nullptr;
	std::unique_ptr<Pass> g_cylindricalProjectionFloat = nullptr;
	std::array<std::unique_ptr<SimpleImage>, 3> g_planetViews { nullptr };

	constexpr uint32_t WorldMapWidth  = 1024;
	constexpr uint32_t WorldMapHeight = 512;

	const String IMAGE_VS_HLSL = R"(
		struct VS_OUTPUT
		{
			float4 pos 		: SV_POSITION;
			float2 tc 		: TEXCOORD;
		};

		static const float2 uv[4] = 
		{
			float2(0.0f, 0.0f),
			float2(1.0f, 0.0f),
			float2(0.0f, 1.0f),
			float2(1.0f, 1.0f)
		};	

		uniform float4 U_VSCustom0[4];

		VS_OUTPUT main(uint vtxId : SV_VertexID)
		{
			float4 pos = U_VSCustom0[vtxId];
			VS_OUTPUT output;
			output.pos = pos;
			output.tc = uv[vtxId];
			return output;
		})";

	const String IMAGE_PS_HLSL = R"(
		struct VS_OUTPUT
		{
			float4 pos 		: SV_POSITION;
			float2 tc 		: TEXCOORD;
		};

		Texture2D<float> S_2DExt0;
		SamplerState S_2DExt0SamplerState;

		float4 main(VS_OUTPUT pin) : SV_TARGET
		{
			return float4(S_2DExt0.SampleLevel(S_2DExt0SamplerState, pin.tc, 0).rrr, 1.0f);
		})";

	class SimpleImage final : public Renderable
	{
	public:
		SimpleImage()
		{
			enableCustomParameters(true);
		}

		~SimpleImage() override
		{
			if (m_img)
			{
				Root::instance()->getRenderSystem()->destoryRcComputeTarget(m_img);
			}
		}

		const Material* getMaterial() const override { return g_worldMapMat.get(); }

		const LightList& getLights() const override
		{
			static LightList dummy;
			return dummy;
		}

		RenderOperation* getRenderOperation() override
		{
			return g_imageRenderOp.get();
		}

		void setCustomParameters(const Pass* pPass) override
		{
			const auto* system = Root::instance()->getRenderSystem();

			const Vector2 pos0 = Vector2(m_screenPos.x, 1.0f - m_screenPos.y) * 2.0f - 1.0f;
			const Vector2 pos1 = Vector2(m_screenPos.x + m_screenSize.x, 1.0f - m_screenPos.y) * 2.0f - 1.0f;
			const Vector2 pos2 = Vector2(m_screenPos.x, 1.0f - (m_screenPos.y + m_screenSize.y)) * 2.0f - 1.0f;
			const Vector2 pos3 = Vector2(m_screenPos.x + m_screenSize.x, 1.0f - (m_screenPos.y + m_screenSize.y)) * 2.0f - 1.0f;

			const Vector4 ndcPos[4] =
			{
				Vector4(pos0.x, pos0.y, 0.0f, 1.0f),
				Vector4(pos1.x, pos1.y, 0.0f, 1.0f),
				Vector4(pos2.x, pos2.y, 0.0f, 1.0f),
				Vector4(pos3.x, pos3.y, 0.0f, 1.0f)
			};
			system->setUniformValue(this, pPass, U_VSCustom0, ndcPos, sizeof(ndcPos));
			system->setTextureSampleValue(this, pPass, S_2DExt0, m_outerImg ? m_outerImg : m_img, ECHO_SAMPLER_STATE {});
		}

		void setShow(const bool show)
		{
			m_show       = show;
			auto* system = Root::instance()->getRenderSystem();
			if (show && m_img == nullptr)
			{
				m_img = system->createRcComputeTarget(
					WorldMapWidth,
					WorldMapHeight,
					ECHO_FMT_R8_UNORM);
			}
			else if (!show && m_img != nullptr)
			{
				system->destoryRcComputeTarget(m_img);
				m_img = nullptr;
			}
		}

		RcComputeTarget* m_img = nullptr;
		RcComputeTarget* m_outerImg = nullptr;
		Vector2 m_screenPos {};
		Vector2 m_screenSize {};
		bool m_show = false;
	};
#endif

	class Planet final :
		public SphericalTerrainQuadTreeNode,
		public SphericalTerrain::GenerateVertexListener,
		public SphericalTerrain::GenerateVertexTask
	{
	public:
		friend class Echo::SphericalTerrain;

		explicit Planet(SphericalTerrain* terrain)
			: SphericalTerrainQuadTreeNode(terrain, nullptr, NegativeX, RootQuad, -1, 0, 0) {}

		~Planet() override { cancelTask(); }

		void setCustomParameters(const Pass* pPass) override
		{
            const RenderSystem* system = Root::instance()->getRenderSystem();
			system->setUniformValue(this, pPass, U_VSCustom0, Vector4::ZERO);

			const Matrix4 trans = Matrix4::IDENTITY;
			system->setUniformValue(this, pPass, U_VSCustom2, &trans, sizeof(Matrix4));

			
			ECHO_SAMPLER_STATE linearClamp;
			linearClamp.Filter = ECHO_TEX_FILTER_TRILINEAR;
			if (m_root->isSphere())
			{
				system->setTextureSampleValue(this, pPass, S_CubeMapExt0, m_root->m_terrainData->m_bakedAlbedo, linearClamp);
				system->setTextureSampleValue(this, pPass, S_CubeMapExt1, m_root->m_terrainData->m_bakedEmission, linearClamp);
			}
			else if (m_root->isTorus())
			{
				system->setTextureSampleValue(this, pPass, S_Custom9, m_root->m_terrainData->m_bakedAlbedo, linearClamp);
				system->setTextureSampleValue(this, pPass, S_Custom, m_root->m_terrainData->m_bakedEmission, linearClamp);
			}

			Vector4 PSCustom1[2] = {};
			PSCustom1[0].x = m_root->m_radius / 4.f;
			PSCustom1[0].y = m_root->m_normalStrength;
			PSCustom1[0].z = m_root->m_weightParamM;
			PSCustom1[0].w = m_root->m_weightParamDelta;

			PSCustom1[1].x = m_root->getDissolveStrength();
			system->setUniformValue(this, pPass, U_PSCustom1, &PSCustom1, sizeof(Vector4) * 2);

			const Vector4 PSCustom10 = m_root->getPlanetRegionParameters(); // xy: Params.xy zw: offset.xy
			system->setUniformValue(this, pPass, U_PSCustom10, &PSCustom10, sizeof(PSCustom10));
		}

		const Material* getMaterial() const override
		{
			assert(m_root->m_material_low_LOD.isV1());
			return m_root->m_material_low_LOD.getV1();
		}

		MaterialWrapper getMaterialWrapper() const override
		{
			return m_root->m_material_low_LOD;
		}

		void init() override
		{
			destroyVertexBuffer();
			clearIndexBuffer();
		}

		bool launchTask()
		{
			if (!m_vertexBuffer)
			{
				if (m_generationTicket == 0)
				{
					m_generationTicket = CommonJobManager::instance()->addRequestCommonTask(this, this);
				}
				return true;
			}
			return false;
		}

		void cancelTask()
		{
			if (m_generationTicket)
			{
				CommonJobManager::instance()->removeRequest(m_generationTicket);
				CommonJobManager::instance()->removeListener(this);
			}
		}

		void CommonTaskFinish(uint64 requestId) override
		{
			if (m_vertices.empty()) return;

			auto* vb = Root::instance()->getRenderSystem()->createStaticVB(m_vertices.data(),
				static_cast<uint32>(sizeof(TerrainVertex) * m_vertices.size()));
			setVertexBuffer(vb, static_cast<int>(m_vertices.size()));
			m_vertices.clear();
			m_vertices.shrink_to_fit();
			m_generationTicket = 0;
		}

		void Execute() override
		{
			auto& planetVertices = m_vertices;
			const int sliceX = std::min(6, m_root->m_numSlice[0]);
			planetVertices.resize(static_cast<size_t>(sliceX) * VERTEX_COUNT);
			float localOceanLevel = 0.0f;
			if (m_root->m_haveOcean && m_root->m_ocean)
			{
				if (m_root->isSphere())
				{
					localOceanLevel = m_root->m_ocean->getRadius() / m_root->m_radius;
				}
				else if (m_root->isTorus())
				{
					localOceanLevel = m_root->m_ocean->SOData.m_relativeMinorRadius / m_root->m_relativeMinorRadius;
				}
				
			}
			//float oceanElevation = localOceanLevel - 1.0f;

			for (int slice = 0; slice < sliceX; ++slice)
			{
				std::vector<Vector3> baseShape;
				if (m_root->isSphere())
				{
					constexpr float gridSize = static_cast<float>(getGridSizeOnCube(0));
					baseShape                = getSphericalPositionsUniform(slice, EXTRA_GRID_RESOLUTION, EXTRA_GRID_RESOLUTION,
						Vector2(gridSize), Vector2(-1.0f - gridSize), m_root->m_pTerrainGenerator);
				}
				else if (m_root->isTorus())
				{
					const Vector2 gridSize = 1.0f / Vector2(static_cast<float>(QUADS * sliceX), static_cast<float>(QUADS));
					baseShape              = TorusTerrainNode::getTorusPositions(EXTRA_GRID_RESOLUTION, EXTRA_GRID_RESOLUTION,
						m_root->m_relativeMinorRadius, gridSize, Vector2(static_cast<float>(slice) / sliceX, 0.0f) - gridSize);
				}

				std::vector<float> elePlus(EXTRA_VERTEX_COUNT);
				m_root->generateElevations(baseShape.data(), elePlus.data(), baseShape.size());
				std::transform(elePlus.begin(), elePlus.end(), elePlus.begin(),
					[this /*,oceanElevation*/](float ele)
					{
						ele *= m_root->m_elevationRatio;
						//ele = std::max(ele, oceanElevation);
						return ele;
					});

				std::vector<Vector3> posPlus = std::move(baseShape);
				m_root->applyElevations(posPlus.data(), posPlus.data(), elePlus.data(), EXTRA_VERTEX_COUNT);

				std::vector<Vector3> pos(VERTEX_COUNT);
				for (int i = 1; i < EXTRA_GRID_RESOLUTION - 1; ++i)
				{
					const int srcOffset = i * EXTRA_GRID_RESOLUTION + 1;
					const int dstOffset = (i - 1) * ::GRID_RESOLUTION;
					std::memcpy(pos.data() + dstOffset, posPlus.data() + srcOffset, ::GRID_RESOLUTION * sizeof(Vector3));
				}

				auto nor = SphericalTerrain::computeNormal(::GRID_RESOLUTION, ::GRID_RESOLUTION, posPlus);

				std::vector<Vector2> uv(VERTEX_COUNT);
				if (m_root->isTorus())
				{
					const auto uvOffset  = Vector2(static_cast<float>(slice) / sliceX, 0.0f);
					const Vector2 uvStep = 1.0f / Vector2(static_cast<float>(QUADS * sliceX), static_cast<float>(QUADS));
					for (int i = 0; i < ::GRID_RESOLUTION; ++i)
					{
						for (int j = 0; j < ::GRID_RESOLUTION; ++j)
						{
							const int idx = i * ::GRID_RESOLUTION + j;
							uv[idx]       = uvOffset + uvStep * Vector2(static_cast<float>(j), static_cast<float>(i));
						}
					}
				}

				for (int i = 0; i < VERTEX_COUNT; ++i)
				{
					planetVertices[VERTEX_COUNT * slice + i] = TerrainVertex { .pos = pos[i], .normal = nor[i], .uv = uv[i] };
				}
			}
		}

	private:
		uint64_t m_generationTicket = 0;
	};
}

#ifdef ECHO_EDITOR
struct SphericalTerrain::PcgPipeline
{
	void create(ProceduralTerrainTopology topology, bool terrainStamp, bool editing, TerrainGenerator* pTerrainGenerator);
	void destroy();
	void createResource(ProceduralTerrainTopology topology, int width, int height);
	void destroyResource();

	void generateElevation(const SphericalTerrain& terrain);
	std::array<RcComputeTarget*, 6> generateElevation(const SphericalTerrain& terrain, int width, int height);

	void generateTemperature(const SphericalTerrain& terrain);

	void generateHumidity(const SphericalTerrain& terrain);
	std::array<RcComputeTarget*, 6> generateHumidity(
		const SphericalTerrain& terrain, int width, int height, float amplitude, float range,
		const ImageProcess& imageProcess, bool variance, RcSampler* const* elevationSource);

	void generateMaterialIndex(const SphericalTerrain& terrain);

	void generateVertices(const SphericalTerrain& terrain, const Node& node, RcBuffer* vb);

	void updateWorldMap(SimpleImage& view, RcComputeTarget* const* targets, float valMul = 1.0f, float valAdd = 0.0f);

	template <typename T>
	void readBackMaterialIndex(const SphericalTerrain& terrain, T& bitMaps);

	void generateElevationWorldMap(const SphericalTerrain& terrain, Bitmap& bitMap, RcSampler* const* elevationSource);
	void generateDeviationWorldMap(const SphericalTerrain& terrain, Bitmap& bitMap, RcSampler* const* humidityDest);

	[[nodiscard]] RcComputeTarget** getElevationTargets()
	{
		return std::visit([](auto&& cts) { return cts.m_elevation.data(); }, m_targets);
	}

	[[nodiscard]] RcComputeTarget** getTemperatureTargets()
	{
		return std::visit([](auto&& targets) { return targets.m_temperature.data(); }, m_targets);
	}

	[[nodiscard]] RcComputeTarget** getHumidityTargets()
	{
		return std::visit([](auto&& targets) { return targets.m_humidity.data(); }, m_targets);
	}

	[[nodiscard]] RcComputeTarget** getMaterialIndexTargets()
	{
		return std::visit([](auto&& targets) { return targets.m_matIndex.data(); }, m_targets);
	}

	bool isReady(bool generateBiome)
	{
		const auto* computable = m_computable.get();
		if (!computable) return false;

		if (generateBiome)
		{
			return computable->getComputeData(m_generateElevation.get())->pComputePipeline->pNative &&
				computable->getComputeData(m_generateHumidity.get())->pComputePipeline->pNative &&
				computable->getComputeData(m_generateTemperature.get())->pComputePipeline->pNative &&
				computable->getComputeData(m_generateMaterialIndex.get())->pComputePipeline->pNative &&
				computable->getComputeData(m_generateVertices.get())->pComputePipeline->pNative;
		}

		return computable->getComputeData(m_generateVertices.get())->pComputePipeline->pNative;
	}

private:
	void generateElevationInternal(const SphericalTerrain& terrain, int width, int height, RcComputeTarget* const* maps);
	void generateHumidityInternal(
		const SphericalTerrain& terrain, int width, int height, float amplitude, float range,
		const ImageProcess& imageProcess, bool variance, RcSampler* const* elevationSource,
		RcComputeTarget* const* humidityDest);

	template <bool UNORM = true>
	void cylindricalProjection(RcSampler* const* cubeMap, RcComputeTarget* rectMap, float valMul, float valAdd);

	std::unique_ptr<Computable> m_computable = nullptr;

	std::unique_ptr<Pass> m_generateElevation     = nullptr;
	std::unique_ptr<Pass> m_generateTemperature   = nullptr;
	std::unique_ptr<Pass> m_generateHumidity      = nullptr;
	std::unique_ptr<Pass> m_generateVertices      = nullptr;
	std::unique_ptr<Pass> m_generateMaterialIndex = nullptr;

	template <size_t ArraySize>
	struct TerrainTargets
	{
		static constexpr size_t Slice = ArraySize;

		std::array<RcComputeTarget*, ArraySize> m_elevation   = { nullptr };
		std::array<RcComputeTarget*, ArraySize> m_temperature = { nullptr };
		std::array<RcComputeTarget*, ArraySize> m_humidity    = { nullptr };
		std::array<RcComputeTarget*, ArraySize> m_matIndex    = { nullptr };
	};

	using Cubic       = TerrainTargets<6>;
	using Cylindrical = TerrainTargets<1>;

	std::variant<Cubic, Cylindrical> m_targets;
};

void SphericalTerrain::PcgPipeline::create(const ProceduralTerrainTopology topology, const bool terrainStamp, const bool editing, TerrainGenerator* pTerrainGenerator)
{
	auto& computable = m_computable;
	computable       = std::make_unique<Computable>();

	ECHO_EFFECT_DESC effect;
	auto createPass = [&effect](std::unique_ptr<Pass>& pass, const char* name, const std::string_view macros)
	{
		String shaderTxt;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name(name), shaderTxt);
		if (shaderTxt.empty())
		{
			LogManager::instance()->logMessage(LML_CRITICAL,
				"SphericalTerrain::PcgPipeline::create: Failed to load " + String(name) + " content.");
			return false;
		}
		shaderTxt.insert(0, macros);

		pass                       = std::make_unique<Pass>();
		effect.szComputeShaderText = shaderTxt.c_str();
		effect.strCsName           = name;
		if (const auto res = pass->createComputeEffect(effect); !res)
		{
			LogManager::instance()->logMessage(LML_CRITICAL,
				"SphericalTerrain::PcgPipeline::create: Failed to create " + String(name) + " pass.");
			return false;
		}
		pass->loadEffect();
		return true;
	};

	std::string macros;
	if (terrainStamp)
	{
		macros = "#define ENABLE_STAMP_TERRAIN\n#define STAMP_TERRAIN_INSTANCES_MAX ";
		macros += std::to_string(HeightMapStamp3D::StampTerrainInstancesMax) + "\n";
	}

	macros += "#define GRID_RESOLUTION " + std::to_string(Node::GRID_RESOLUTION) + "\n";
	macros += "#define " + SHAPE_MACRO[topology] + "\n";

	if (pTerrainGenerator && !pTerrainGenerator->getName().empty())
	{
		macros += "#define SAMPLE_HEIGHTMAP_GENERATETERRAIN\n";
	}
	if (pTerrainGenerator && !pTerrainGenerator->getBorderName().empty())
	{
		macros += "#define SAMPLE_BORDERMAP_GENERATETERRAIN\n";
	}

	if (!createPass(m_generateVertices, "PCGGenerateVertices_CS.txt", macros))
	{
		destroy();
		return;
	}
	ComputableUtil::preCompute(computable.get(), m_generateVertices.get());

	if (!editing) return;

	if (!createPass(m_generateElevation, "PCGGenerateElevation_CS.txt", macros))
	{
		destroy();
		return;
	}

	if (!createPass(m_generateTemperature, "PCGGenerateTemperature_CS.txt", macros))
	{
		destroy();
		return;
	}
	if (!createPass(m_generateHumidity, "PCGGenerateHumidity_CS.txt", macros))
	{
		destroy();
		return;
	}
	if (!createPass(m_generateMaterialIndex, "PCGGenerateMaterialIndex_CS.txt", macros))
	{
		destroy();
		return;
	}

	if (g_cylindricalProjection == nullptr)
	{
		auto macrosInternal = macros;
		macrosInternal.append("#define OUTPUT_UNORM\n");
		createPass(g_cylindricalProjection, "PCGCylindricalProjection_CS.txt", macrosInternal);
	}
	if (g_cylindricalProjectionFloat == nullptr)
	{
		auto macrosInternal = macros;
		macrosInternal.append("#define OUTPUT_FLOAT\n");
		createPass(g_cylindricalProjectionFloat, "PCGCylindricalProjection_CS.txt", macrosInternal);
	}

	// trigger computable->getComputeData(pass) creation
	ComputableUtil::preCompute(computable.get(), m_generateElevation.get());
	ComputableUtil::preCompute(computable.get(), m_generateTemperature.get());
	ComputableUtil::preCompute(computable.get(), m_generateHumidity.get());
	ComputableUtil::preCompute(computable.get(), m_generateMaterialIndex.get());
}

void SphericalTerrain::PcgPipeline::destroy()
{
	m_generateElevation.reset();
	m_generateTemperature.reset();
	m_generateHumidity.reset();
	m_generateVertices.reset();
	m_generateMaterialIndex.reset();
	m_computable.reset();
}

void SphericalTerrain::PcgPipeline::createResource(const ProceduralTerrainTopology topology, int width, int height)
{
	switch (topology)
	{
	case PTS_Sphere: m_targets = Cubic();
		break;
	case PTS_Torus: m_targets = Cylindrical();
		break;
	}

	std::visit([width, height](auto&& targets)
	{
		auto* sys = Root::instance()->getRenderSystem();

		for (auto&& target : targets.m_elevation)
			target = sys->createRcComputeTarget(width, height, ECHO_FMT_R_FP32);

		for (auto&& target : targets.m_temperature)
			target = sys->createRcComputeTarget(width, height, ECHO_FMT_R8_UNORM);

		for (auto&& target : targets.m_humidity)
			target = sys->createRcComputeTarget(width, height, ECHO_FMT_R8_UNORM);

		for (auto&& target : targets.m_matIndex)
			target = sys->createRcComputeTarget(width, height, ECHO_FMT_R8UI);
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::destroyResource()
{
	std::visit([](auto&& targets)
	{
		auto* sys = Root::instance()->getRenderSystem();

		for (auto&& target : targets.m_elevation)
		{
			if (target == nullptr) continue;
			sys->destoryRcComputeTarget(target);
			target = nullptr;
		}

		for (auto&& target : targets.m_temperature)
		{
			if (target == nullptr) continue;
			sys->destoryRcComputeTarget(target);
			target = nullptr;
		}

		for (auto&& target : targets.m_humidity)
		{
			if (target == nullptr) continue;
			sys->destoryRcComputeTarget(target);
			target = nullptr;
		}

		for (auto&& target : targets.m_matIndex)
		{
			if (target == nullptr) continue;
			sys->destoryRcComputeTarget(target);
			target = nullptr;
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateElevation(const SphericalTerrain& terrain)
{
	generateElevationInternal(terrain, terrain.m_biomeTextureWidth, terrain.m_biomeTextureWidth, getElevationTargets());
	if (const auto& view = g_planetViews[ElevationWorldMap])
	{
		updateWorldMap(*view, getElevationTargets(), 0.5f / terrain.m_noise->GetSigma3(), 0.5f);
	}
}

std::array<RcComputeTarget*, 6> SphericalTerrain::PcgPipeline::generateElevation(
	const SphericalTerrain& terrain, int width, int height)
{
	const int numSlice = terrain.m_numSlice[0] * terrain.m_numSlice[1];
	std::array<RcComputeTarget*, 6> targets { nullptr };
	for (int i = 0; i < numSlice; ++i)
	{
		targets[i] = Root::instance()->getRenderSystem()->createRcComputeTarget(width, height, ECHO_FMT_R_FP32);
	}
	generateElevationInternal(terrain, width, height, targets.data());
	return targets;
}

void SphericalTerrain::PcgPipeline::generateTemperature(const SphericalTerrain& terrain)
{
	struct PCGGenTempUniforms
	{
		float min;
		float max;
		int processBrightness;
		float brightness;

		int processExposure;
		float exposure;
		float exposureOffset;
		float gammaInv;

		int processInvert;
		int processClamp;
		float clampMin;
		float clampMax;
	};

	auto& [invert, exposure, exposureOffset, exposureGamma, brightness, clamp] =
		terrain.m_terrainData->distribution.climateProcess.temperatureIP;

	PCGGenTempUniforms u0;

	u0.min = -terrain.m_noise->GetSigma3();
	u0.max = terrain.m_noise->GetSigma3();

	u0.processBrightness = brightness != 0;
	u0.brightness        = static_cast<float>(brightness);

	u0.processExposure = exposure != 0.0f || exposureOffset != 0.0f || exposureGamma != 1.0f;
	u0.exposure        = exposure;
	u0.exposureOffset  = exposureOffset;
	u0.gammaInv        = 1.0f / exposureGamma;

	u0.processInvert = invert;

	u0.processClamp = clamp.min != 0.0f || clamp.max != 1.0f;
	u0.clampMin     = clamp.min;
	u0.clampMax     = clamp.max;


	std::visit([&]<typename ResourceType>(ResourceType& resource)
	{
		using Mapping              = std::decay_t<ResourceType>;
		// constexpr bool cubic       = std::is_same_v<Mapping, Cubic>;
		// constexpr bool cylindrical = std::is_same_v<Mapping, Cylindrical>;

		const int width  = terrain.m_biomeTextureWidth;
		const int height = terrain.m_biomeTextureHeight;


		const auto system = Root::instance()->getRenderSystem();
		auto* computable  = m_computable.get();
		const auto* pass  = m_generateTemperature.get();

		for (size_t slice = 0; slice < resource.Slice; ++slice)
		{
			ComputableUtil::preCompute(computable, pass);

			system->setUniformValue(computable, pass, U_CSCustom0, &u0, sizeof(u0));

			constexpr uint32 groupSizeX = 16;
			constexpr uint32 groupSizeY = 16;
			system->setTextureSampleValue(computable, pass, S_2DExt0, resource.m_elevation[slice], ECHO_SAMPLER_STATE {});
			system->setImageValue(computable, pass, S_2DExt1, resource.m_temperature[slice], 0u);

			ComputableUtil::computeSingle(computable, pass, width, height, 1, groupSizeX, groupSizeY, 1);
		}

		if (const auto& view = g_planetViews[TemperatureWorldMap])
		{
			updateWorldMap(*view, getTemperatureTargets());
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateHumidity(const SphericalTerrain& terrain)
{
	const auto& [range, amplitude] = terrain.m_terrainData->distribution.climateProcess.aoParam;
	const auto& imageProcess = terrain.m_terrainData->distribution.climateProcess.humidityIP;
	generateHumidityInternal(terrain, terrain.m_biomeTextureWidth, terrain.m_biomeTextureHeight,
		amplitude, range, imageProcess, false, reinterpret_cast<RcSampler* const*>(getElevationTargets()),
		getHumidityTargets());
	if (const auto& view = g_planetViews[HumidityWorldMap])
	{
		updateWorldMap(*view, getHumidityTargets());
	}
}

std::array<RcComputeTarget*, 6> SphericalTerrain::PcgPipeline::generateHumidity(
	const SphericalTerrain& terrain, int width, int height, const float amplitude, const float range,
	const ImageProcess& imageProcess, bool variance, RcSampler* const* elevationSource)
{
	const int numSlice = terrain.m_numSlice[0] * terrain.m_numSlice[1];
	std::array<RcComputeTarget*, 6> targets { nullptr };
	for (int i = 0; i < numSlice; ++i)
	{
		targets[i] = Root::instance()->getRenderSystem()->createRcComputeTarget(width, height, ECHO_FMT_R8_UNORM);
	}
	generateHumidityInternal(terrain, width, height, amplitude, range, imageProcess, variance, elevationSource, targets.data());
	return targets;
}

void SphericalTerrain::PcgPipeline::generateElevationInternal(const SphericalTerrain& terrain, int width, int height,
                                                              RcComputeTarget* const* maps)
{
	const auto system = Root::instance()->getRenderSystem();
	auto* computable  = m_computable.get();
	const auto* pass  = m_generateElevation.get();

	const auto* noise = terrain.m_noise.get();
	const NoiseUniforms u0(noise);

	std::visit([&]<typename ResourceType>(ResourceType& resource)
	{
		using Mapping              = std::decay_t<ResourceType>;
		constexpr bool cubic       = std::is_same_v<Mapping, Cubic>;
		constexpr bool cylindrical = std::is_same_v<Mapping, Cylindrical>;

		for (size_t slice = 0; slice < resource.Slice; ++slice)
		{
			ComputableUtil::preCompute(computable, pass);

			system->setUniformValue(computable, pass, U_CSCustom0, &u0, sizeof(u0));
			system->setImageValue(computable, pass, S_2DExt0, maps[slice], 0u);

			GridUniforms u1 {};
			if constexpr (cubic)
			{
				if (terrain.m_pTerrainGenerator && terrain.m_pTerrainGenerator->isSuccess())
				{
					ECHO_SAMPLER_STATE sampleState;
					sampleState.Filter = ECHO_TEX_FILTER_BILINEAR;
					auto* tex          = terrain.m_pTerrainGenerator->getTextureArray()->getRcTex();
					system->setTextureSampleValue(computable, pass, S_2DArrayExt1, tex, sampleState);
					TexturePtr borderTexturePtr = terrain.m_pTerrainGenerator->getBorderTextureArray();
					if (!borderTexturePtr.isNull())
					{
						auto* tex2 = borderTexturePtr->getRcTex();
						system->setTextureSampleValue(computable, pass, S_2DArrayExt2, tex2, sampleState);
						const auto u2 = Vector4(terrain.m_terrainMinBorder, terrain.m_terrainBorderSize,
							terrain.m_oneOverTerrainBorderSize, terrain.m_terrainBorderHeight);
						system->setUniformValue(computable, pass, U_CSCustom2, &u2, sizeof(u2));
					}
				}
				u1.offset   = Vector2(-1.0f);
				u1.gridSize = 2.0f / Vector2(static_cast<float>(width - 1), static_cast<float>(height - 1));
				u1.slice    = static_cast<uint32_t>(slice);
			}
			else if constexpr (cylindrical)
			{
				auto pixelSize = 1.0f / Vector2(static_cast<float>(width), static_cast<float>(height));
				u1.offset      = pixelSize * Math::PI;
				u1.gridSize    = pixelSize * Math::TWO_PI;
				u1.minorRadius = terrain.m_relativeMinorRadius;
			}
			system->setUniformValue(computable, pass, U_CSCustom1, &u1, sizeof(u1));

			if (terrain.m_bStampTerrain)
			{
				ECHO_SAMPLER_STATE sampleState;
				sampleState.Filter = ECHO_TEX_FILTER_BILINEAR;
				auto& stamp        = terrain.m_stampterrain3D;
				auto* tex          = stamp->getTextureArray()->getRcTex();
				system->setTextureSampleValue(computable, pass, S_2DArrayExt0, tex, sampleState);
				stamp->FlushUniformBuffer();
				system->setUniformValue(computable, pass, U_CSCustom15, stamp->GetInstances().data(), (int)stamp->GetInstances().size() * sizeof(StampTerrainInstance));
			}

			constexpr uint32 groupSizeX = 16;
			constexpr uint32 groupSizeY = 16;
			ComputableUtil::computeSingle(computable, pass, width, height, 1, groupSizeX, groupSizeY, 1);
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateHumidityInternal(
	const SphericalTerrain& terrain, int width, int height, float amplitude,
	float range, const ImageProcess& imageProcess, bool variance, RcSampler* const* elevationSource,
	RcComputeTarget* const* humidityDest)
{
	const auto system = Root::instance()->getRenderSystem();
	auto* computable  = m_computable.get();
	const auto* pass  = m_generateHumidity.get();

	struct PCGGenHmdUniforms
	{
		float amplitude;
		int outputVariance;
		int processBrightness;
		float brightness;

		int processExposure;
		float exposure;
		float exposureOffset;
		float gammaInv;

		int processInvert;
		int processClamp;
		float clampMin;
		float clampMax;
	};

	const auto& [invert, exposure, exposureOffset, exposureGamma, brightness, clamp] = imageProcess;

	PCGGenHmdUniforms u2 {};
	u2.outputVariance = variance;

	u2.processBrightness = brightness != 0;
	u2.brightness        = static_cast<float>(brightness);

	u2.processExposure = exposure != 0.0f || exposureOffset != 0.0f || exposureGamma != 1.0f;
	u2.exposure        = exposure;
	u2.exposureOffset  = exposureOffset;
	u2.gammaInv        = 1.0f / exposureGamma;

	u2.processInvert = invert;

	u2.processClamp = clamp.min != 0.0f || clamp.max != 1.0f;
	u2.clampMin     = clamp.min;
	u2.clampMax     = clamp.max;

	struct AoSampleUniforms
	{
		Vector4 samples[32];
	};

	AoSampleUniforms u4 {};
	const auto samples  = terrain.m_noise->GetSamplingDisk(range, static_cast<int>(std::size(u4.samples)));
	std::ranges::copy(samples, u4.samples);
	const float elevationRatio = terrain.m_elevationRatio;

	ECHO_SAMPLER_STATE pointWrap;
	pointWrap.AddressU = pointWrap.AddressV = ECHO_TEX_ADDRESS_WRAP;

	std::visit([&]<typename ResourceType>(ResourceType& resource)
	{
		using Mapping              = std::decay_t<ResourceType>;
		constexpr bool cubic       = std::is_same_v<Mapping, Cubic>;
		constexpr bool cylindrical = std::is_same_v<Mapping, Cylindrical>;

		if constexpr (cubic)
		{
			u2.amplitude = elevationRatio * amplitude;
		}
		else if constexpr (cylindrical)
		{
			u2.amplitude = elevationRatio * terrain.m_relativeMinorRadius * amplitude;

			const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
			for (auto&& sample : u4.samples)
			{
				sample.x *= 1.0f / Math::TWO_PI;
				sample.z *= aspectRatio / Math::TWO_PI;
			}
		}

		for (int slice = 0; slice < Mapping::Slice; ++slice)
		{
			ComputableUtil::preCompute(computable, pass);

			if constexpr (cubic)
			{
				GridUniforms u1;
				u1.offset   = Vector2(-1.0f);
				u1.gridSize = 2.0f / Vector2(static_cast<float>(width - 1), static_cast<float>(height - 1));
				u1.slice    = static_cast<uint32_t>(slice);
				system->setUniformValue(computable, pass, U_CSCustom1, &u1, sizeof(u1));
			}
			system->setUniformValue(computable, pass, U_CSCustom2, &u2, sizeof(u2));
			system->setUniformValue(computable, pass, U_CSCustom4, &u4, sizeof(u4));

			constexpr uint32 groupSizeX = 16;
			constexpr uint32 groupSizeY = 16;

			system->setImageValue(computable, pass, S_2DExt0, humidityDest[slice], 0u);

			for (int j = 0; j < Mapping::Slice; ++j)
			{
				system->setTextureSampleValue(computable, pass, S_2DExt1 + j, elevationSource[j], pointWrap);
			}
			ComputableUtil::computeSingle(computable, pass, width, height, 1, groupSizeX, groupSizeY, 1);
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateMaterialIndex(const SphericalTerrain& terrain)
{
	const auto system = Root::instance()->getRenderSystem();
	auto* computable  = m_computable.get();
	const auto* pass  = m_generateMaterialIndex.get();
	const int width   = terrain.m_biomeTextureWidth;
	const int height  = terrain.m_biomeTextureHeight;

	std::visit([&]<typename ResourceType>(ResourceType& resource)
	{
		using Mapping = std::decay_t<ResourceType>;

		for (int slice = 0; slice < Mapping::Slice; ++slice)
		{
			ComputableUtil::preCompute(computable, pass);
			system->setTextureSampleValue(computable, pass, S_2DExt0, resource.m_humidity[slice], ECHO_SAMPLER_STATE {});
			system->setTextureSampleValue(computable, pass, S_2DExt1, resource.m_temperature[slice], ECHO_SAMPLER_STATE {});
			system->setTextureSampleValue(computable, pass, S_2DExt2, terrain.m_terrainData->lookupTex.get()->getRcTex(), ECHO_SAMPLER_STATE {});
			system->setImageValue(computable, pass, S_2DExt3, resource.m_matIndex[slice], 0u);

			constexpr uint32 groupSizeX = 16;
			constexpr uint32 groupSizeY = 16;
			ComputableUtil::computeSingle(computable, pass, width, height, 1, groupSizeX, groupSizeY, 1);
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateVertices(const SphericalTerrain& terrain, const Node& node, RcBuffer* vb)
{
	const auto system = Root::instance()->getRenderSystem();

	auto* computable = m_computable.get();
	const auto* pass = m_generateVertices.get();

	const NoiseUniforms u0(terrain.m_noise.get());
	const int width  = terrain.m_biomeTextureWidth;
	const int height = terrain.m_biomeTextureHeight;

	const auto u2 = Vector4(terrain.m_elevationRatio,
		0.5f / static_cast<float>(width),
		1.0f - 0.5f / static_cast<float>(height), 0);
	
	const Vector3& transInv = -node.getTranslationLocal();
	const auto u3 = Vector4(transInv.x, transInv.y, transInv.z, 0);

	std::visit([&]<typename ResourceType>(ResourceType&)
	{
		using Mapping              = std::decay_t<ResourceType>;
		constexpr bool cubic       = std::is_same_v<Mapping, Cubic>;
		constexpr bool cylindrical = std::is_same_v<Mapping, Cylindrical>;

		ComputableUtil::preCompute(computable, pass);

		system->setUniformValue(computable, pass, U_CSCustom0, &u0, sizeof(u0));
		system->setUniformValue(computable, pass, U_CSCustom2, &u2, sizeof(u2));
		system->setUniformValue(computable, pass, U_CSCustom3, &u3, sizeof(u3));
		
		if (terrain.m_bStampTerrain)
		{
			ECHO_SAMPLER_STATE sampleState;
			sampleState.Filter = ECHO_TEX_FILTER_BILINEAR;

			auto& stamp = terrain.m_stampterrain3D;
			system->setTextureSampleValue(computable, pass, S_2DArrayExt0,
				stamp->getTextureArray()->getRcTex(), sampleState);
			stamp->FlushUniformBuffer();

			const void* stampData = stamp->GetInstances().data();
			uint32 stampDataSize  = (int)stamp->GetInstances().size() * sizeof(StampTerrainInstance);

			system->setUniformValue(computable, pass, U_CSCustom15, stampData, stampDataSize);
		}

		GridUniforms u1;
		if constexpr (cubic)
		{
			if (terrain.m_pTerrainGenerator && terrain.m_pTerrainGenerator->isSuccess()) {
				ECHO_SAMPLER_STATE sampleState;
				sampleState.Filter = ECHO_TEX_FILTER_BILINEAR;
				auto* tex = terrain.m_pTerrainGenerator->getTextureArray()->getRcTex();
				system->setTextureSampleValue(computable, pass, S_2DArrayExt1, tex, sampleState);
				TexturePtr borderTexturePtr = terrain.m_pTerrainGenerator->getBorderTextureArray();
				if (!borderTexturePtr.isNull()) {
					auto* tex2 = borderTexturePtr->getRcTex();
					system->setTextureSampleValue(computable, pass, S_2DArrayExt2, tex2, sampleState);
					const auto u4 = Vector4(terrain.m_terrainMinBorder, terrain.m_terrainBorderSize,
						terrain.m_oneOverTerrainBorderSize,	terrain.m_terrainBorderHeight);
					system->setUniformValue(computable, pass, U_CSCustom4, &u4, sizeof(u4));
				}
			}
			u1.offset   = node.m_offset;
			u1.gridSize = node.m_gridSize;
			u1.slice    = node.m_slice;
		}
		else if constexpr (cylindrical)
		{
			u1.offset      = Math::TWO_PI * Vector2(node.m_offset);
			u1.gridSize    = Math::TWO_PI * Vector2(node.m_gridSize);
			u1.minorRadius = terrain.m_relativeMinorRadius;
		}
		system->setUniformValue(computable, pass, U_CSCustom1, &u1, sizeof(u1));

		constexpr uint32 groupSizeX = 16;
		constexpr uint32 groupSizeY = 16;
		system->setBufferValue(computable, pass, B_CSCustom0, vb);
		ComputableUtil::computeSingle(computable, pass, Node::GRID_RESOLUTION, Node::GRID_RESOLUTION, 1, groupSizeX, groupSizeY, 1);
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::updateWorldMap(SimpleImage& view, RcComputeTarget* const* targets, const float valMul, const float valAdd)
{
	if (!view.m_show) return;

	std::visit([&]<typename ResourceType>(ResourceType&)
	{
		using Mapping              = std::decay_t<ResourceType>;
		constexpr bool cubic       = std::is_same_v<Mapping, Cubic>;
		constexpr bool cylindrical = std::is_same_v<Mapping, Cylindrical>;

		if constexpr (cubic)
		{
			view.m_outerImg = nullptr;
			cylindricalProjection(reinterpret_cast<RcSampler* const*>(targets), view.m_img, valMul, valAdd);
		}
		else if constexpr (cylindrical)
		{
			view.m_outerImg = targets[0];
		}
	}, m_targets);
}

void SphericalTerrain::PcgPipeline::generateElevationWorldMap(
	const SphericalTerrain& terrain, Bitmap& bitMap, RcSampler* const* elevationSource)
{
	if (!Root::instance()->getRenderSystem()->isUseSingleThreadRender())
	{
		ECHO_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "unimplemented", __FUNCTION__);
	}

	const auto elevationMapFloat = Root::instance()->getRenderSystem()->createRcComputeTarget(WorldMapWidth, WorldMapHeight, ECHO_FMT_R_FP32);

	cylindricalProjection<false>(elevationSource, elevationMapFloat, terrain.m_elevationRatio, 0);

	auto* system         = Root::instance()->getRenderSystem();
	constexpr int width  = WorldMapWidth;
	constexpr int height = WorldMapHeight;
	auto& dst            = bitMap;
	assert(!dst.pixels);
	RcReadComputeTarget res;
	system->copyComputeTargetContentsToMemory(elevationMapFloat, res);
	assert(res.dataFormat == ECHO_FMT_R_FP32 && res.realWidth == width && res.realHeight == height && res.dataSize == width * height * sizeof(float) && res.pData);
	dst.width         = res.realWidth;
	dst.height        = res.realHeight;
	dst.bytesPerPixel = sizeof(float);
	dst.pitch         = dst.width * dst.bytesPerPixel;
	dst.byteSize      = res.dataSize;
	dst.pixels        = res.pData;
	res.pData         = nullptr;

	Root::instance()->getRenderSystem()->destoryRcComputeTarget(elevationMapFloat);
}

void SphericalTerrain::PcgPipeline::generateDeviationWorldMap(
	const SphericalTerrain& terrain, Bitmap& bitMap, RcSampler* const* humidityDest)
{
	if (!Root::instance()->getRenderSystem()->isUseSingleThreadRender())
	{
		ECHO_EXCEPT(Exception::ERR_INVALID_STATE, "unimplemented", __FUNCTION__);
	}

	auto stdMap = Root::instance()->getRenderSystem()->createRcComputeTarget(WorldMapWidth, WorldMapHeight, ECHO_FMT_R8_UNORM);

	cylindricalProjection(humidityDest, stdMap, 1, 0);

	auto* system         = Root::instance()->getRenderSystem();
	constexpr int width  = WorldMapWidth;
	constexpr int height = WorldMapHeight;
	auto& dst            = bitMap;
	assert(!dst.pixels);
	RcReadComputeTarget res;
	system->copyComputeTargetContentsToMemory(stdMap, res);
	assert(res.dataFormat == ECHO_FMT_R8_UNORM && res.realWidth == width && res.realHeight == height && res.dataSize == width * height * sizeof(uint8) && res.pData);
	dst.width         = res.realWidth;
	dst.height        = res.realHeight;
	dst.bytesPerPixel = sizeof(uint8);
	dst.pitch         = dst.width * dst.bytesPerPixel;
	dst.byteSize      = res.dataSize;
	dst.pixels        = res.pData;
	res.pData         = nullptr;

	Root::instance()->getRenderSystem()->destoryRcComputeTarget(stdMap);
}

template <typename T>
void SphericalTerrain::PcgPipeline::readBackMaterialIndex(const SphericalTerrain& terrain, T& bitMaps)
{
	auto* system     = Root::instance()->getRenderSystem();
	const int width  = terrain.m_biomeTextureWidth;
	const int height = terrain.m_biomeTextureHeight;

	std::visit([&]<typename ResourceType>(ResourceType& resource)
	{
		using Mapping = std::decay_t<ResourceType>;

		for (int i = 0; i < Mapping::Slice; ++i)
		{
			auto& dst = bitMaps[i];
			assert(!dst.pixels);
			RcReadComputeTarget res;
			system->copyComputeTargetContentsToMemory(resource.m_matIndex[i], res);
			assert(res.dataFormat == ECHO_FMT_R8UI && res.realWidth == width && res.realHeight == height && res.dataSize == width * height && res.pData);
			dst.width         = res.realWidth;
			dst.height        = res.realHeight;
			dst.bytesPerPixel = sizeof(uint8_t);
			dst.pitch         = dst.width * dst.bytesPerPixel;
			dst.byteSize      = res.dataSize;
			dst.pixels        = res.pData;
			res.pData         = nullptr;
		}
	}, m_targets);
}

template <bool UNORM>
void SphericalTerrain::PcgPipeline::cylindricalProjection(RcSampler* const* cubeMap, RcComputeTarget* const rectMap,
														  const float valMul, const float valAdd)
{
	const auto system = Root::instance()->getRenderSystem();
	auto* computable  = m_computable.get();
	const auto* pass  = UNORM ? g_cylindricalProjection.get() : g_cylindricalProjectionFloat.get();

	ComputableUtil::preCompute(computable, pass);
	const auto u0 = Vector4(valMul, valAdd, 0.0f, 0.0f);
	system->setUniformValue(computable, pass, U_CSCustom0, &u0, sizeof(u0));
	system->setImageValue(computable, pass, S_2DExt0, rectMap, 0u);

	ECHO_SAMPLER_STATE linear;
	linear.Filter = ECHO_TEX_FILTER_BILINEAR;
	for (int i = 0; i < Cubic::Slice; ++i)
	{
		system->setTextureSampleValue(computable, pass, S_2DExt1 + i, cubeMap[i], linear);
	}
	constexpr uint32 groupSizeX = 16;
	constexpr uint32 groupSizeY = 16;
	ComputableUtil::computeSingle(computable, pass, WorldMapWidth, WorldMapHeight, 1, groupSizeX, groupSizeY, 1);
}
#endif


void SphericalTerrain::setDetailDistance(const float detailDistance)
{
	s_detailDistance = detailDistance;
}

float SphericalTerrain::getDetailDistance()
{
	return s_detailDistance;
}

void SphericalTerrain::setLodRatio(const float lodRatio)
{
	s_lodRatio = lodRatio;
}

float SphericalTerrain::getLodRatio()
{
	return s_lodRatio;
}

void SphericalTerrain::setForceLowLod(const bool forceLod)
{
	m_forceLowLOD = forceLod;
}

bool Echo::SphericalTerrain::isPlanetTaskFinished()
{
	if (m_planet && m_planet->m_vertexBuffer)
		return true;
	else
		return false;
}

void SphericalTerrain::setVisible(const bool visible)
{
	m_visible = visible;
	if(m_haveOcean && m_ocean){
		m_ocean->setVisible(visible);
	}
	if (m_pPlanetGenObjectScheduler && !visible) m_pPlanetGenObjectScheduler->clearGenObjects();
}

bool SphericalTerrain::getVisible() const
{
	return m_visible;
}

void SphericalTerrain::setHideTiles(bool hide)
{
	m_hideTiles = hide;
}

bool SphericalTerrain::getHideTiles() const
{
	return m_hideTiles;
}

ProceduralSphere::ProceduralSphere() :
	m_noise(std::make_unique<UberNoiseFBM3D>()),
	m_stampterrain3D(std::make_unique<HeightMapStamp3D>(this)) {}

ProceduralSphere::~ProceduralSphere() {
	m_pTerrainGenerator = nullptr;
	m_terrainData.setNull();
}

bool ProceduralSphere::init(const String& filePath)
{
	Name resName = Name(filePath);
	try
	{
		m_terrainData = SphericalTerrainResourcePtr(SphericalTerrainResourceManager::instance()->createResource(resName, true, &gExportToolLoader, nullptr));
		m_terrainData->load();
	}
	catch (...)
	{
		LogManager::instance()->logMessage("SphericalTerrainResource [" + filePath + "] failed.");
		return false;
	}

	if (m_terrainData.isNull() || !m_terrainData->bExistResource) return false;

	if (!m_terrainData->m_bLoadResult)
	{
		LogManager::instance()->logMessage("-error-\t Spherical Terrain files [" + m_terrainData->filePath + "] parse failed!", LML_CRITICAL);
	}

	m_topology = m_terrainData->geometry.topology;
	switch (m_topology)
	{
	case PTS_Sphere:
	{
		auto& sphere     = m_terrainData->geometry.sphere;
		m_radius         = sphere.radius;
		m_elevationRatio = sphere.elevationRatio;
		m_pTerrainGenerator = m_terrainData->m_pTerrainGenerator.get();
		m_numSlice[0] = 6;
		m_numSlice[1] = 1;
	}
	break;
	case PTS_Torus:
	{
		auto& torus           = m_terrainData->geometry.torus;
		m_radius              = torus.radius;
		m_elevationRatio      = torus.elevationRatio;
		m_relativeMinorRadius = torus.relativeMinorRadius;
		m_numSlice[0]         = torus.maxSegments[0];
		m_numSlice[1]         = torus.maxSegments[1];
	}
	break;
	}

	m_maxDepth = m_terrainData->geometry.quadtreeMaxDepth;

	m_stampterrain3D->Import(m_terrainData->geometry.StampTerrainInstances, false);
	applyNoiseState();
	applyStaticModifiers();

	const auto indices = CreateNodeIndices(0);
	s_indexData[0]     = indices;

	m_noiseElevationMin = m_terrainData->geometry.noiseElevationMin;
	m_noiseElevationMax = m_terrainData->geometry.noiseElevationMax;

	// if (!filePath.empty())
	// {
	// 	std::string roadfile = filePath;
	// 	if (!roadfile.empty() &&
	// 		roadfile.find(".terrain") == String::npos)
	// 	{
	// 		roadfile.append(".planetroad");
	// 	}
	// 	else if (roadfile.find_last_of(".") != String::npos)
	// 	{
	// 		roadfile.erase(roadfile.find_last_of("."));
	// 		roadfile.append(".planetroad");
	// 	}
	// 	m_terrainData->mPlanetRoad = PlanetRoadResourceManager::instance()->createOrRetrieve(Name(roadfile)).first;
	// 	try
	// 	{
	// 		m_terrainData->mPlanetRoad->load();
	// 	}
	// 	catch (...)
	// 	{
	// 		LogManager::instance()->logMessage("planetroad load faild!");
	// 	}
	//
	// }

	return true;
}

ProceduralSphere::NodeInfo ProceduralSphere::getNodeInfo(int index, const int maxDepth)
{
	const int sliceTiles = Node::getTileCountOverLevel(maxDepth);
	int slice = 0;
	for (;; ++slice)
	{
		if (index < sliceTiles) break;
		index -= sliceTiles;
	}
	assert(index < sliceTiles);
	int depth = 0;
	for (; depth < maxDepth; ++depth)
		if (index < Node::getTileCountOverLevel(depth))
			break;
	index -= Node::getTileCountOverLevel(depth - 1);
	const int x = index % (1 << depth);
	const int y = index / (1 << depth);

	return { slice, depth, x, y };
}

ProceduralSphere::NodeInfo ProceduralSphere::getNodeInfo(const Node& node)
{
	return { node.m_slice, node.m_depth, node.m_levelIndexX, node.m_levelIndexY };
}

int ProceduralSphere::getNodeIndex(const Node& node) const
{
	return getNodeIndex(getNodeInfo(node));
}
int ProceduralSphere::getNodeIndex(const NodeInfo& info) const
{
	return getNodeIndex(info, m_maxDepth);
}

int ProceduralSphere::getNodeIndex(const NodeInfo& info, const int maxDepth)
{
	return info.slice * Node::getTileCountOverLevel(maxDepth) +
		Node::getTileCountOverLevel(info.depth - 1) +
		info.x + info.y * (1 << info.depth);
}

int ProceduralSphere::getNodesCount() const
{
	return m_numSlice[0] * m_numSlice[1] * Node::getTileCountOverLevel(m_maxDepth);
}

std::vector<int> ProceduralSphere::getLeavesIndex(const int maxDepth, const int maxSlice)
{
	std::vector<int> leaves;
	const int tiles = Node::getLevelTileCount(maxDepth);
	leaves.reserve(static_cast<size_t>(maxSlice) * tiles);
	for (int f = 0; f < maxSlice; ++f)
	{
		const int offset = Node::getTileCountOverLevel(maxDepth) * f + Node::getTileCountOverLevel(maxDepth - 1);
		assert(static_cast<size_t>(offset) + tiles - 1ull < std::numeric_limits<int>::max());
		for (int i = 0; i < tiles; ++i)
		{
			leaves.emplace_back(offset + i);
		}
	}
	return leaves;
}

std::vector<int> ProceduralSphere::getLeavesIndex() const
{
	return getLeavesIndex(m_maxDepth, m_numSlice[0] * m_numSlice[1]);
}

Vector3 ProceduralSphere::getNodeOriginTorus(const NodeInfo& info) const
{
	const int sliceX = info.slice % m_numSlice[0];
	float du = 1.0f / static_cast<float>(m_numSlice[0]);
	float u = du * static_cast<float>(sliceX);
	du = du / static_cast<float>(1 << info.depth);
	u += du * (static_cast<float>(info.x) + 0.5f);

	const float toroidal = Math::TWO_PI * u;
	const float x = std::cos(toroidal);
	constexpr float y = 0.0f;
	const float z = -std::sin(toroidal);
	return { x, y, z };
}

Vector3 ProceduralSphere::getNodeOriginSphere(const NodeInfo& info, const TerrainGenerator* pGen) const
{
	const float len = static_cast<float>(Node::getLengthOnCube(info.depth));
	const Vector2 xy { -1.0f + len * (static_cast<float>(info.x) + 0.5f), -1.0f + len * (static_cast<float>(info.y) + 0.5f) };
	const auto pos = Node::getSphericalPosition(info.slice, xy, pGen);
	return pos;
}

Vector3 ProceduralSphere::getNodeOriginLocal(const int index) const
{
	const auto info = getNodeInfo(index, m_maxDepth);
	if (isSphere()) return getNodeOriginSphere(info, m_pTerrainGenerator);
	if (isTorus()) return getNodeOriginTorus(info);
	return Vector3::ZERO;
}

std::vector<uint16> ProceduralSphere::generateIndexBufferPhysX()
{
	assert(!s_indexData[0].empty());
	return s_indexData[0];
}

std::vector<OrientedBox> ProceduralSphere::getFlatAreas() const
{
	if (!m_staticModifier || m_terrainData.isNull() || !m_terrainData->modifiers.instances) return {};
	return TerrainStaticModify3D::GetFlatAreas(m_radius, *m_terrainData->modifiers.instances);
}

float ProceduralSphere::applyTerrainBorder(const Vector3& vertice, float elevation) const {
	float borderDis = m_pTerrainGenerator->getBorderFace(vertice) - m_terrainMinBorder;
	if (borderDis < 0.0f) {
		return std::max(m_terrainBorderHeight, elevation);
	}
	if (m_terrainBorderSize > 0.0f && borderDis < m_terrainBorderSize) {
		return std::max(std::lerp(m_terrainBorderHeight, elevation, borderDis * m_oneOverTerrainBorderSize), elevation);
	}
	return elevation;
}

void ProceduralSphere::generateElevations(const Vector3* vertices, float* elevations, const size_t size) const
{
	m_noise->EvaluateVal(vertices, elevations, size);
	m_stampterrain3D->EvaluateVal(vertices, elevations, size);
	if (m_pTerrainGenerator) {
		if (m_terrainMinBorder >= 1.0f) {
			for (size_t index = 0; index != size; ++index) {
				elevations[index] = std::max(m_terrainBorderHeight, elevations[index]);
			}
		}
		else if ((m_terrainMinBorder + m_terrainBorderSize) > -1.0f) {
			for (size_t index = 0; index != size; ++index) {
				elevations[index] = applyTerrainBorder(vertices[index], elevations[index]);
			}
		}
	}
}

float ProceduralSphere::generateElevation(const Vector3& vertex) const
{
	float elevation = m_stampterrain3D->EvaluateVal(m_noise->EvaluateVal(vertex), vertex);
	if (m_pTerrainGenerator) {
		if (m_terrainMinBorder >= 1.0f) {
			elevation = std::max(m_terrainBorderHeight, elevation);
		}
		if ((m_terrainMinBorder + m_terrainBorderSize) > -1.0f) {
			elevation = applyTerrainBorder(vertex, elevation);
		}
	}
	return elevation;
}

void ProceduralSphere::computeStaticModify(const int nodeIndex, Vector3* vertices, const Vector3* baseVertices, float* elevations, size_t count) const
{
	if (!m_staticModifier) return;
	const auto& staticModifiers = m_terrainData->modifiers;
	auto inst                   = staticModifiers.nodeToInstances.find(nodeIndex);
	if (inst == staticModifiers.nodeToInstances.end()) return;

	if (isSphere())
	{
		if (hasShapeGenerator())
		{
			m_staticModifier->Evaluate(inst->second, vertices, baseVertices, elevations, count,
				TerrainStaticModify3D::ConvexEvaluator { .GravitySource = m_pTerrainGenerator, .GravityParameter = 1.0f + m_noiseElevationMin });
		}
		else
		{
			m_staticModifier->Evaluate(inst->second, vertices, baseVertices, elevations, count,
				TerrainStaticModify3D::UnitSphereEvaluator {});
		}
	}
	else if (isTorus())
	{
		m_staticModifier->Evaluate(inst->second, vertices, baseVertices, elevations, count,
			TerrainStaticModify3D::UnitTorusEvaluator { .MinorRadius = m_relativeMinorRadius });
	}
}

void ProceduralSphere::applyElevations(Vector3* outVertices, const Vector3* inVertices, const float* elevations, size_t count) const
{
	if (isSphere())
	{
		for (size_t i = 0; i < count; ++i)
		{
			outVertices[i] = Node::applyElevation(inVertices[i], elevations[i]);
		}
	}
	else if (isTorus())
	{
		for (size_t i = 0; i < count; ++i)
		{
			outVertices[i] = TorusTerrainNode::applyElevation(inVertices[i], elevations[i], m_relativeMinorRadius);
		}
	}
}

bool ProceduralSphere::getIndexCubeSurfaceBound(const int index, Vector3& position, Quaternion& rotation, Vector3& halfSize) {
	const auto info = getNodeInfo(index, m_maxDepth);
	
	static std::array<Quaternion, 6> _gRotation = {
		Quaternion(Vector3(0, 0, -1),Vector3(1, 0, 0), Vector3(0, -1, 0)),
		Quaternion(Vector3(0, 0, 1), Vector3(-1, 0, 0),Vector3(0, -1, 0)),
		Quaternion(Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)),
		Quaternion(Vector3(1, 0, 0), Vector3(0, -1, 0),Vector3(0, 0, -1)),
		Quaternion(Vector3(1, 0, 0), Vector3(0, 0, 1), Vector3(0, -1, 0)),
		Quaternion(Vector3(-1, 0, 0),Vector3(0, 0, -1),Vector3(0, -1, 0))
	};
	if (isSphere())
	{
		const Vector2 len(static_cast<float>(Node::getLengthOnCube(info.depth)));
		const Vector2 offset = len * Vector2(static_cast<float>(info.x), static_cast<float>(info.y)) - 1.0f;
		const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(info.depth)));
		const float xl = (GRID_RESOLUTION - 1) * gridSize.x * 0.5f;
		const float yl = (GRID_RESOLUTION - 1) * gridSize.y * 0.5f;
		const float x = offset.x + xl;
		const float y = offset.y + yl;
		position = Node::getCubeVertex(info.slice, { x,y });
		rotation = _gRotation[info.slice];
		halfSize = Vector3(xl, gridSize.x, yl);
		return true;
	}
	return false;
}

std::vector<Vector3> ProceduralSphere::generateVertexBufferPhysX(const int index)
{
	const auto info = getNodeInfo(index, m_maxDepth);
	std::vector<Vector3> baseShape;
	if (isSphere())
	{
		const Vector2 len(static_cast<float>(Node::getLengthOnCube(info.depth)));
		const Vector2 offset = len * Vector2(static_cast<float>(info.x), static_cast<float>(info.y)) - 1.0f;
		const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(info.depth)));
		baseShape = Node::getSphericalPositions(info.slice, GRID_RESOLUTION, GRID_RESOLUTION,
			gridSize, offset, m_pTerrainGenerator);
	}
	else if (isTorus())
	{
		auto [size, offset] = TorusTerrainNode::getGridSizeOffset(info.slice, m_numSlice[0], m_numSlice[1],
			info.depth, info.x, info.y);

		baseShape = TorusTerrainNode::getTorusPositions(
			GRID_RESOLUTION, GRID_RESOLUTION,
			m_relativeMinorRadius,
			size,
			offset);
	}

	std::vector<float> elevations(VERTEX_COUNT);
	generateElevations(baseShape.data(), elevations.data(), VERTEX_COUNT);
	std::transform(elevations.begin(), elevations.end(), elevations.begin(), [this](const float e) { return e * m_elevationRatio; });

	std::vector<Vector3> pos(VERTEX_COUNT);
	applyElevations(pos.data(), baseShape.data(), elevations.data(), VERTEX_COUNT);

	computeStaticModify(index, pos.data(), baseShape.data(), elevations.data(), VERTEX_COUNT);
	
	const auto invTrans = -getNodeOriginLocal(index);
	std::transform(pos.begin(), pos.end(), pos.begin(), [invTrans](const Vector3& v) { return v + invTrans; });

	return pos;
}

std::array<std::vector<ProceduralSphere::Fp16>, 6> ProceduralSphere::generateDetailedHeightmap() const
{
	std::array<std::vector<uint16_t>, 6> faces;

	if (isTorus())
	{
		const int width  = m_numSlice[0] * QUADS * (1 << m_maxDepth) + 1;
		const int height = m_numSlice[1] * QUADS * (1 << m_maxDepth) + 1;

		auto pos = TorusTerrainNode::getTorusPositions(
			width, height,
			m_relativeMinorRadius,
			1.0f / Vector2(static_cast<float>(width - 1), static_cast<float>(height - 1)),
			Vector2(0.0f));

		std::vector<float> fEle(pos.size());
		generateElevations(pos.data(), fEle.data(), pos.size());
		auto& hEle      = faces[0];
		hEle.reserve(fEle.size());
		std::transform(fEle.begin(), fEle.end(), std::back_inserter(hEle),
			[this](const float v) { return convertFloatToHalf(v * m_elevationRatio); });
		return faces;
	}

	auto gen = [&faces, this](const int f)
	{
		const int resolution = (1 << m_maxDepth) * QUADS + 1;
		const Vector2 offset(-1.0f, -1.0f);
		const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(m_maxDepth)));
		const auto pos       = Node::getSphericalPositions(f, resolution, resolution,
			gridSize, offset, m_pTerrainGenerator);
		std::vector<float> fEle(pos.size());
		generateElevations(pos.data(), fEle.data(), pos.size());
		auto& hEle      = faces[f];

		if (m_pTerrainGenerator)
		{
			constexpr size_t stride = sizeof(float) / sizeof(Fp16);
			hEle.resize(fEle.size() * stride);
			for (int i = 0; i < fEle.size(); ++i)
			{
				reinterpret_cast<float*>(hEle.data())[i] = (fEle[i] * m_elevationRatio + 1.0f) * pos[i].length() - 1.0f;
			}
			return;
		}

		hEle.reserve(fEle.size());
		std::transform(fEle.begin(), fEle.end(), std::back_inserter(hEle),
			[this](const float v) { return convertFloatToHalf(v * m_elevationRatio); });
	};
	std::vector<std::thread> threads;
	threads.reserve(6);
	for (int f = 0; f < 6; ++f)
	{
		threads.emplace_back(gen, f);
	}
	for (auto& t : threads)
	{
		t.join();
	}

	return faces;
}

void ProceduralSphere::outputDetailedHeightmap() const
{
#if OUTPUT_HEIGHTMAP
	std::array<std::vector<float>, 6> faces;
	auto gen = [&](const int f)
	{
		const int resolution = (1 << m_maxDepth) * QUADS + 1;
		const Vector2 offset(-1.0f, -1.0f);
		const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(m_maxDepth)));
		const auto pos = Node::getSphericalPositions(f, resolution, resolution,
			gridSize, offset, nullptr);
		std::vector<float> fEle(pos.size());
		generateElevations(pos.data(), fEle.data(), pos.size());
		std::transform(fEle.begin(), fEle.end(), fEle.begin(), [this](const float v)
			{ return v * m_elevationRatio; });
		faces[f] = std::move(fEle);
	};
	std::vector<std::thread> threads;
	threads.reserve(6);
	for (int f = 0; f < 6; ++f)
	{
		threads.emplace_back(gen, f);
	}
	for (auto& t : threads)
	{
		t.join();
	}
	float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::lowest();
	for (const auto & face : faces)
	{
		for (auto&& ele : face)
		{
			min = std::min(min, ele);
			max = std::max(max, ele);
		}
	}
	for (auto & face : faces)
	{
		for (auto&& ele : face)
		{
			ele = (ele - min) / (max - min);
		}
	}

	for (int i = 0; i < 6; ++i)
	{
		const size_t resolution = (1 << m_maxDepth) * QUADS + 1;
		DirectX::Image img
		{
			.width = resolution, .height = resolution, .format = DXGI_FORMAT_R32_FLOAT,
			.rowPitch = resolution * sizeof(float),
			.slicePitch = size_t(resolution) * resolution * sizeof(float),
			.pixels = reinterpret_cast<uint8_t*>(faces[i].data())
		};
		auto name = m_terrainData->getName().toString();
		size_t start = name.find_last_of('/') + 1;
		size_t end = name.find_last_of('.');
		name = name.substr(start, end - start);
		auto lName = std::wstring(name.begin(), name.end());
		DirectX::ScratchImage dstImg;
		DirectX::Convert(img, DXGI_FORMAT_R8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, dstImg);
		SaveToWICFile(dstImg.GetImages()[0], DirectX::WIC_FLAGS_NONE,
			GetWICCodec(DirectX::WIC_CODEC_PNG), std::format(L"{}_{}.png", lName, i).c_str());
		std::ofstream ofs(std::format("{}_{}.bin", name, i), std::ios::binary | std::ios::out);
		ofs.write(reinterpret_cast<const char*>(faces[i].data()), faces[i].size() * sizeof(float));
		ofs.close();
	}
#endif
}

void ProceduralSphere::applyNoiseState()
{
	Noise3DData& data = m_terrainData->geometry.noise;
	m_noise->SetFeatureNoiseSeed(data.featureNoiseSeed);
	m_noise->SetLacunarity(data.lacunarity);
	m_noise->SetBaseFrequency(data.baseFrequency);
	m_noise->SetBaseAmplitude(data.baseAmplitude);
	m_noise->SetDetailAmplitude(data.detailAmplitude);
	m_noise->SetGain(data.gain);
	m_noise->SetBaseOctaves(data.baseOctaves);
	m_noise->SetSharpnessNoiseSeed(data.sharpnessNoiseSeed);
	m_noise->SetSharpness(data.sharpnessRange.x, data.sharpnessRange.y);
	m_noise->SetSharpnessFrequency(data.sharpnessFrequency);
	m_noise->SetSlopeErosionNoiseSeed(data.slopeErosionNoiseSeed);
	m_noise->SetSlopeErosion(data.slopeErosionRange.x, data.slopeErosionRange.y);
	m_noise->SetSlopeErosionFrequency(data.slopeErosionFrequency);
	m_noise->SetPerturbNoiseSeed(data.perturbNoiseSeed);
	m_noise->SetPerturb(data.perturbRange.x, data.perturbRange.y);
	m_noise->SetPerturbFrequency(data.perturbFrequency);
	m_terrainMinBorder = m_terrainData->geometry.noise.terrainMinBorder;
	m_terrainBorderSize = m_terrainData->geometry.noise.terrainBorderSize;
	m_oneOverTerrainBorderSize = 1.0f / std::max(m_terrainBorderSize, 1e-6f);
	m_terrainBorderHeight = m_terrainData->geometry.noise.terrainBorderHeight;
}

void ProceduralSphere::applyStaticModifiers()
{
	if (!m_terrainData.isNull() && m_terrainData->modifiers.instances)
	{
		m_staticModifier = std::make_unique<TerrainStaticModify3D>(
			m_terrainData->modifiers.instances, .05f / m_radius);
	}
	else
	{
		m_staticModifier = nullptr;
	}
}

SphericalTerrain::SphericalTerrain(PlanetManager* mgr) :
#ifdef ECHO_EDITOR
	m_pcg(std::make_unique<PcgPipeline>()),
#endif
	m_modifier(std::make_unique<TerrainModify3D>())
{
	m_mgr = mgr;
	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	m_vbGenAsync = false;
	++s_count;

	m_vegGroupHandle = BiomeVegGroup::GetDefaultGroup();
	m_useSphericalVoronoiRegion  = Root::instance()->m_useSphericalVoronoiRegion;
	m_showSphericalVoronoiRegion = m_useSphericalVoronoiRegion ? mgr->m_showSphericalVoronoiRegion : false;
}

SphericalTerrain::~SphericalTerrain()
{
    SAFE_DELETE(m_warfog);
    SAFE_DELETE(m_cloud);
	SAFE_DELETE(m_PlotDivision);
	--s_count;

	shutDown();
	m_vegGroupHandle = nullptr;

    if(m_isRequestedBiomeTexture)
    {
        m_terrainData->freeBiomeTexture();
    }
}

void SphericalTerrain::GenerateVertexListener::CommonTaskFinish(const uint64 requestId)
{
	if (node == nullptr || node->m_root == nullptr)
	{
		assert(false && "unreachable code");
		LogManager::instance()->logMessage("SphericalTerrain::GenerateVertexListener::CommonTaskFinish: Node or root was empty!!!"
			"Contact LIZIZHEN if you end up here.", LML_CRITICAL);
		return;
	}
	auto& vertices = node->m_vertices;
	if (vertices.empty()) return;
	assert(vertices.size() == VERTEX_COUNT);

	if (node->m_vertexBuffer && node->m_root->m_maxDepth == node->m_depth)
	{
		auto box = node->getBoundLocal();
		box.transformAffine(node->m_root->m_worldMat);
		node->m_root->vegetationRecheck(box);
	}

	if (node->m_vertexBuffer)
	{
		Root::instance()->getRenderSystem()->updateBuffer(node->m_vertexBuffer, vertices.data(),
			static_cast<uint32>(vertices.size() * sizeof(TerrainVertex)), false);
	}
	else
	{
		node->setVertexBuffer(createDynamicVb(vertices));
	}
	vertices.clear();
	vertices.shrink_to_fit();

	node->m_root->m_genVertexRequests.erase(node);
}

void SphericalTerrain::GenerateVertexTask::Execute()
{
	node->m_root->computeVertexCpu(*node);
}

bool SphericalTerrain::createImpl(const SphericalTerrainResourcePtr& resource)
{
	if (resource.isNull() || !resource->bExistResource) return false;

	m_terrainData       = resource;
	m_vbGenAsync        = !m_editingTerrain;
	m_useObb            = !m_editingTerrain;

	m_topology = m_terrainData->geometry.topology;
	m_useLowLODMaterial = !m_editingTerrain && !resource->m_bakedAlbedo.isNull() && !resource->m_bakedEmission.isNull();
	m_haveOcean         = m_terrainData->sphericalOcean.haveOcean;

	bool loadSuccess = m_terrainData->m_bLoadResult;

	if (!loadSuccess)
	{
		LogManager::instance()->logMessage("-error-\t Spherical Terrain files [" + m_terrainData->filePath + "] parse failed!", LML_CRITICAL);
	}
	
	applyGeometry();

	m_stampterrain3D->Import(m_terrainData->geometry.StampTerrainInstances, m_gpuMode && (m_editingTerrain || !m_vbGenAsync));

	if (m_gpuMode && m_editingTerrain)
	{
		updateStampTerrain_impl();
#ifdef ECHO_EDITOR
		createComputePipeline();
		createComputeTarget();
#endif
	}
	
	applyStaticModifiers();

	if (m_useSphericalVoronoiRegion)
	{
		if (m_terrainData->sphericalVoronoiRegion.Loaded)
		{
			m_terrainData->sphericalVoronoiRegion.SelectCoarseLevelCenter();
		}
	}
#ifdef ECHO_EDITOR
	if (m_useSphericalVoronoiRegion && m_editingTerrain)
	{
		const auto& region = m_terrainData->sphericalVoronoiRegion;
		if (region.Loaded)
		{
			updateSphericalVoronoiRegionInternal();
			bool temp = bDisplayCenter;
			markSphericalVoronoiRegionCenter(true);
			bDisplayCenter = temp;
		}
	}
#endif
	m_showSphericalVoronoiRegion = (m_useSphericalVoronoiRegion && m_terrainData->sphericalVoronoiRegion.Loaded) ? m_showSphericalVoronoiRegion : false;

	if (m_material.isNull())
	{
		createMaterial();
	}

	if (s_tileIndexBuffers[0] == nullptr)
	{
		createIndexBuffer();
	}

#ifdef ECHO_EDITOR
	generateBiome();
#endif
	createRoots();

	if (m_haveOcean)
	{
		m_ocean = new SphericalOcean(this);
		//m_ocean->setRadius(m_radius);
		m_ocean->setupOcean(m_terrainData->sphericalOcean);
		m_ocean->init();
		m_ocean->MgrRebuild();
		m_ocean->enableSSR(m_mgr->m_enableOceanSSR);
		m_ocean->setWorldTrans(Transform(m_pos, m_rot));
		m_ocean->setScale(m_scale.x);
	}

	if (m_vegHandle && m_vegGroupHandle)
	{
		m_vegGroupHandle->destoryVeg(m_vegHandle);
		m_vegHandle = nullptr;
	}

	if (m_vegGroupHandle)
	{
		if (m_vegHandle)
		{
			m_vegGroupHandle->destoryVeg(m_vegHandle);
			m_vegHandle = nullptr;
		}

		float maxRadius = std::max(std::max(m_scale.x, m_scale.y), m_scale.z) * m_radius;
		float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
		BiomeVegetation::CreateInfo vegInfo;
		vegInfo.sphTerMaxOff = (uint16)std::ceil(maxElevation * maxRadius);
		vegInfo.sphPos = m_pos;
		vegInfo.sphRot = m_rot;
		vegInfo.sphRadius = maxRadius;
		vegInfo.sphPtr = this;
		vegInfo.vegLayerMap.clear();
		if (m_mgr)
			vegInfo.sceneMgrPtr = m_mgr->m_mgr;

		for (int biomeCompoIndex = 0;
			 biomeCompoIndex < m_terrainData->distribution.distRanges.size();
			 biomeCompoIndex++)
		{
			if (biomeCompoIndex < m_terrainData->composition.biomes.size() &&
				m_terrainData->composition.biomes[biomeCompoIndex].biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
			{
				int biomeIndex = m_terrainData->composition.getBiomeIndex(biomeCompoIndex);
				vegInfo.vegLayerMap[biomeIndex] = m_terrainData->composition.biomes.at(biomeCompoIndex).vegLayers;
			}
		}

		m_vegHandle = m_vegGroupHandle->createVeg(vegInfo);
	}

	if (m_useLowLODMaterial)
	{
		m_planet = std::make_unique<Planet>(this);
		m_planet->init();
	}

	if (isSphere() && !hasShapeGenerator() && SphericalTerrainAtmosData::instance()->mLoaded)
	{
		m_atmos = std::unique_ptr<SphericalTerrainAtmosphereRenderable>(new SphericalTerrainAtmosphereRenderable());
		atmosInitSuccess = m_atmos->initMeshBuffer(SphericalTerrainAtmosData::instance()->mAtmosMesh.get());
		m_atmos->m_planet = this;
	}

	//远景云初始化
	initLowLODCloud();

	//星球路网初始化
	//if (!m_terrainData->mPlanetRoad.isNull() && m_terrainData->mPlanetRoad->isLoaded())
	//	mRoad = new PlanetRoad(this);

#ifdef ECHO_EDITOR
	updateSeaSurfaceTemperature();
#endif

	resetPlanetGenObject();

#ifdef _WIN32

	float maxRadius = std::max(std::max(m_scale.x, m_scale.y), m_scale.z) * m_radius;
	float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
	SphereSnowQuadTreeDada info;
	info.sphTerMaxOff = (uint16)std::ceil(maxElevation * maxRadius);
	info.sphRadius = maxRadius;
	info.sph = this;

	m_deformSnow = new DeformSnow(this);
	/*
	auto&& usedBiomes = this->m_terrainData->composition.usedBiomeTemplateList;

	for (int i = 0; i < usedBiomes.size(); i++)
	{
		if (i < m_terrainData->composition.usedBiomeTemplateList.size())
		{
			SnowQTmgr->addSnowIndices(i);
		}
	}
	*/

#endif // _WIN32

	//Generation succeeded
	onCreateFinish();

	return loadSuccess;
}

void SphericalTerrain::setRegionWarfogAlpha(int id, float in_alpha)
{
    if(!m_warfog)
        return;
    
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif

    Bitmap& maskMap = m_warfog->m_regionWarFogMaskCube;
    if( 0<= id && id < 256 && maskMap.pixels)
    {
        in_alpha = EchoClamp(0.0f, 1.0f, in_alpha);
        uint8 alpha = uint8(in_alpha * 255.0f);
    
        uint32* pixel = (uint32*)maskMap.pixels;
        uint32 newPixel = (pixel[id] & 0xffffff) | alpha << 24;
        pixel[id] = newPixel;
                
        if(!m_warfog->m_regionWarFogMaskCubeDirty)     
            m_warfog->m_regionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("fine region war fog texture update time ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

void SphericalTerrain::setCoarseRegionWarfogAlpha(int id, float in_alpha)
{
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif
    
    if(!m_warfog)
        return;

    Bitmap& maskMap = m_warfog->m_coarseRegionWarFogMaskCube;
    if( 0<= id && id < 256 && maskMap.pixels)
    {
        in_alpha = EchoClamp(0.0f, 1.0f, in_alpha);
        uint8 alpha = uint8(in_alpha * 255.0f);

        uint32* pixel = (uint32*)maskMap.pixels;
        uint32 newPixel = (pixel[id] & 0xffffff) | alpha << 24;
        pixel[id] = newPixel;
        
        if(!m_warfog->m_coarseRegionWarFogMaskCubeDirty)
            m_warfog->m_coarseRegionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("Coarse region war fog texture update time[ ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

void
SphericalTerrain::setSingleRegionFillColor(int id, const ColorValue& in_color)
{
    if(!m_warfog)
        return;
    
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif

    Bitmap& maskMap = m_warfog->m_regionWarFogMaskCube;
    if( 0<= id && id < 256 && maskMap.pixels)
    {
        uint8 r,g,b;
        r = (uint8)(EchoClamp(0.0f, 1.0f, in_color.r) * 255.0f);
        g = (uint8)(EchoClamp(0.0f, 1.0f, in_color.g) * 255.0f);
        b = (uint8)(EchoClamp(0.0f, 1.0f, in_color.b) * 255.0f);
        uint32 color = r << 0
            | g << 8
            | b << 16;
        
        uint32* pixel = (uint32*)maskMap.pixels;
        uint32 newPixel = (pixel[id] & 0xff000000) | color;
        pixel[id] = newPixel;
                
        if(!m_warfog->m_regionWarFogMaskCubeDirty)     
            m_warfog->m_regionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("fine region war fog texture update time ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

void
SphericalTerrain::setSingleCoarseRegionFillColor(int id, const ColorValue& in_color)
{
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif
    
    if(!m_warfog)
        return;

    Bitmap& maskMap = m_warfog->m_coarseRegionWarFogMaskCube;
    if( 0<= id && id < 256 && maskMap.pixels)
    {
        uint8 r,g,b;
        r = (uint8)(EchoClamp(0.0f, 1.0f, in_color.r) * 255.0f);
        g = (uint8)(EchoClamp(0.0f, 1.0f, in_color.g) * 255.0f);
        b = (uint8)(EchoClamp(0.0f, 1.0f, in_color.b) * 255.0f);
        uint32 color = r << 0
            | g << 8
            | b << 16;
        
        uint32* pixel = (uint32*)maskMap.pixels;
        uint32 newPixel = (pixel[id] & 0xff000000) | color;
        pixel[id] = newPixel;
        
        if(!m_warfog->m_coarseRegionWarFogMaskCubeDirty)
            m_warfog->m_coarseRegionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("Coarse region war fog texture update time[ ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

bool SphericalTerrain::init(const String& filePath, const bool bSync, const bool terrainEditor, const bool bAtlas)
{
	m_editingTerrain = terrainEditor;
	m_bAtlas = bAtlas;
	Name resName = Name(filePath);
	SphericalTerrainResourcePtr resPtr = SphericalTerrainResourceManager::instance()->getByName(resName);
	if (bSync) 
	{
		if (resPtr.isNull()) {
			resPtr = SphericalTerrainResourceManager::instance()->createResource(resName);
		}

        try {
            resPtr->load();

            //IMPORTANT:Force to sync load child-texture resource.
            resPtr->requestBiomeTexture(resPtr, true);
            m_isRequestedBiomeTexture = true;
            
            return createImpl(resPtr);
        }
        catch (...)
        {
            LogManager::instance()->logMessage("SphericalTerrainResource [" + filePath + "] failed.");
            return false;
        }
	}
	else
    {
        if (resPtr.isNull() || (!resPtr->isPrepared() && !resPtr->isLoaded()))
        {
            requestResource(Name(filePath));
            return true;
        }
        else
        {
            return createImpl(resPtr);
        }
    }
}

void SphericalTerrain::shutDown()
{
	onDestroy();
	SAFE_DELETE(m_pPlanetGenObjectScheduler);

	SAFE_DELETE(m_deformSnow);
	
	SAFE_DELETE(mRoad);

	m_pTerrainGenerator = nullptr;
	if (m_vegHandle && m_vegGroupHandle)
	{
		m_vegGroupHandle->destoryVeg(m_vegHandle);
		m_vegHandle = nullptr;
	}
	if (m_atmos)
		m_atmos->uninitMeshBuffer();
	disableAtmosphere();
	clearRequest();
	destroyMaterial();
	cancelGenerateVertexTask();
	m_planet.reset();
	{
#if ECHO_EDITOR
		std::unique_lock lock(m_heightQueryMutex);
#endif
		for (auto && slice : m_slices) { slice.reset(); }
		m_slices.clear();
	}
	m_leaves.clear();

	{
#if ECHO_EDITOR
	std::unique_lock lock(m_biomeQueryMutex);
#endif
		m_materialIndexMap.reset();
	}
#ifdef ECHO_EDITOR
	destroyComputeTarget();
	destroyComputePipeline();
	for (auto && map : m_generatedMaterialMap)
	{
		map.freeMemory();
	}
#endif

	if (m_haveOcean)
	{
		SAFE_DELETE(m_ocean);
	}

	freeFbmTexture();
	//freeVoronoiTexture();
	destroyIndexBuffer();

	

}

void SphericalTerrain::createRoots()
{
	cancelGenerateVertexTask();

	applyNoiseState();

	m_modifier->RemoveInstance();

	m_bounds.clear();

	m_elevationMin = m_terrainData->geometry.elevationMin;
	m_elevationMax = m_terrainData->geometry.elevationMax;
	m_noiseElevationMin = m_terrainData->geometry.noiseElevationMin;
	m_noiseElevationMax = m_terrainData->geometry.noiseElevationMax;

#if ECHO_EDITOR
	std::unique_lock lock(m_heightQueryMutex);
#endif
	m_slices.clear();
	m_slices.reserve(static_cast<size_t>(m_numSlice[0]) * m_numSlice[1]);

	if (m_terrainData->geometry.bounds.size() == getNodesCount() + 1)
	{
		m_bounds = m_terrainData->geometry.bounds;
	}

	for (int y = 0; y < m_numSlice[1]; ++y)
	{
		for (int x = 0; x < m_numSlice[0]; ++x)
		{
			if (isSphere())
			{
				m_slices.emplace_back(new Node(this, nullptr, x + y * m_numSlice[0], Node::RootQuad, 0, 0, 0));
			}
			else if (isTorus())
			{
				m_slices.emplace_back(new TorusTerrainNode(this, x, y));
			}
		}
	}

	const float elevationRange = m_noise->GetSigma3() * m_elevationRatio;
	for (auto&& node : m_slices)
	{
		node->init();
		node->addNeighborsRoot();

		if (isBoundAccurate())
		{
			node->setBound(getIndexBound(getNodeIndex(*node)));
		}
		else
		{
			node->computeApproximateBound(elevationRange);
		}

		if (m_editingTerrain) generateVertexBuffer(*node);
		node->m_active = true;
	}
}

void SphericalTerrain::generateMesh()
{
	cancelGenerateVertexTask();

	applyNoiseState();

	m_modifier->RemoveInstance();

	m_bounds.clear();

	m_elevationMin = m_terrainData->geometry.elevationMin;
	m_elevationMax = m_terrainData->geometry.elevationMax;
	m_noiseElevationMin = m_terrainData->geometry.noiseElevationMin;
	m_noiseElevationMax = m_terrainData->geometry.noiseElevationMax;

	for (auto && slice : m_slices)
	{
		Node::traverseTree(slice.get(), [](Node* node)
		{
			node->m_vbDirty = true;
			return true;
		});
	}
}

void SphericalTerrain::generateBiome()
{
	applyNoiseState();

	if (m_gpuMode)
	{
		m_biomeDirty = true;
	}
	else
	{
		assert(false && "Not implemented");
	}
}

void SphericalTerrain::updateGeometry(const Vector3& camL, float minRenderDistSqrL)
{
	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [this, camL, minRenderDistSqrL](Node* node)
		{
			float distSqrL;
			if (m_useObb)
			{
				const auto obb = node->getOrientedBoundLocal();
				distSqrL        = obb.DistanceSqr(camL);
			}
			else
			{
				const auto& aabb = node->getBoundLocal();
				distSqrL         = aabb.squaredDistance(camL);
			}
			float renderDistSqrL = node->getRenderDistanceLocal() * s_lodRatio;
			renderDistSqrL *= renderDistSqrL;
			const bool inDist = distSqrL < minRenderDistSqrL || distSqrL < renderDistSqrL;

			if (node->m_active)
			{
				if ((inDist || node->isNeighborsChildSubdivided()) && canSubdivideNode(*node))
				{
					subdivideNode(*node);
				}
				else if (node->hasChildren())
				{
					for (auto&& c : node->m_children)
					{
						cancelGenerateVertexTask(*c);
					}
					node->destroyChildren();
				}
				else if (node->m_vbDirty)
				{
					generateVertexBuffer(*node);
				}
			}
			else
			{
				if (!inDist && canUnifyNode(*node))
				{
					unifyNode(*node);
				}
				else
				{
					cancelGenerateVertexTask(*node);
				}

				// above call to 'unifyNode' could have activated us, so we need to recheck
				if (!node->m_active && node->hasChildren())
				{
					return true;
				}
			}

			return false;
		});
	}
}

void SphericalTerrain::printLog(const Camera* pCamera)
{
#if defined(_WIN32) && 0
	{
		auto wpos = pCamera->getDerivedPosition();
		auto s_type = getPhySurfaceType(wpos);
		auto type = getSurfaceTypeWs(wpos);
		String log = String("physical surface type: ") + std::to_string(s_type) + String("\n");
		const auto& list = m_terrainData->composition.usedBiomeTemplateList;
		type = type >= 0 && type < list.size() ? list.at(type).biomeID : -1;
		log += String("biome type: ") + std::to_string(type) + String("\n");
		log += String("surface mip: ") + std::to_string(getSurfaceMipWs(wpos)) + "\n";

		Vector3 normal;
		wpos += pCamera->getDerivedDirection() * 10.0f;
		const auto surface = getSurfaceWs(wpos, &normal);

		const float radius = std::abs(static_cast<float>((surface - pCamera->getDerivedPosition()).dotProduct(
			pCamera->getDerivedDirection()))) * 0.02f;
		double depth;
		const bool isUnderWater = isUnderOceanWave(surface, &depth);
		log += String("ocean depth: ") + std::to_string(depth) + "\n";
		DebugDrawManager::instance()->drawSphere(surface, radius, isUnderWater ? ColorValue::White : ColorValue::Red, DebugDrawManager::WORLD_SPACE);

		Vector3 line[] =
		{
			surface,
			surface + normal * 10.0f,
		};
		int lineIdx[] = { 0, 1, };
		DebugDrawManager::instance()->pushLines(line, static_cast<int>(std::size(line)),
			lineIdx, static_cast<int>(std::size(lineIdx)), ColorValue::Green);

		line[1] = surface + getUpVectorWs(surface) * 10.0f;
		log.append(std::format("boarder value : {}\n", getBoarderValue(surface)));
		DebugDrawManager::instance()->pushLines(line, static_cast<int>(std::size(line)),
			lineIdx, static_cast<int>(std::size(lineIdx)), ColorValue::Blue);

		OutputDebugString(log.c_str());
		DVector3 dTri[3];
		getSurfaceTriangleFinestWs(wpos, dTri[0], dTri[1], dTri[2]);
		constexpr int triIdx[] = { 0, 1, 1, 2, 2, 0 };
		Vector3 fTri[3] = { dTri[0], dTri[1], dTri[2] };
		DebugDrawManager::instance()->pushLines(fTri, static_cast<int>(std::size(fTri)),
			triIdx, static_cast<int>(std::size(triIdx)), ColorValue(1, 1, 0, 1));

	/*	int idx = 0 , x = 0, y = 0;
		getPlotDivisionDataByWorldPosition(wpos,idx,x,y);
		setPlotDivisionColor(idx, x, y, ColorValue::Black);*/
	}
#endif

#if 0
	if(m_terrainData->region.Loaded)
	{
		Timer timer;
		// std::vector<Vector3> pos(10000);
		Vector3 pos = pCamera->getDerivedPosition();
		const auto start = timer.getMicroseconds();
		for(int i=0;i<10000;++i)
		{
			computeRegionInformation(pos);
		}
		const auto end = timer.getMicroseconds();

		const float interval = static_cast<float>(end - start);
		std::string message = "RegionQuery (same position for 10000 times) : " + std::to_string(interval) + " us";
		LogManager::instance()->logMessage(message);
	}
#endif

#ifdef _WIN32
	if(0)
	{
		String log = String("--------Profile planet:") + m_terrainData->filePath + String("-------\n");
		OutputDebugString(log.c_str());
	
		log = String("mesh memory")+
			std::to_string(m_profile.meshMemory) +String(" bytes \n ") +
			std::to_string(m_profile.meshMemory / 1024) +String(" KB\n") +
			std::to_string(m_profile.meshMemory / 1024 / 1024) +String(" MB\n");
		OutputDebugString(log.c_str());
	
		log = String("tex memory")+ std::to_string(m_profile.texMemory) + String(" bytes\n") +
			std::to_string(m_profile.texMemory / 1024) +String(" KB\n") +
			std::to_string(m_profile.texMemory / 1024 / 1024) +String(" MB\n");
		OutputDebugString(log.c_str());
	
		log = String("block memory")+ std::to_string(m_profile.blockMemory) +String(" bytes\n") +
			std::to_string(m_profile.blockMemory / 1024) +String(" KB\n") +
			std::to_string(m_profile.blockMemory / 1024 / 1024) +String(" MB\n");
		OutputDebugString(log.c_str());

		log = String("generate mesh time: ")+ 
			std::to_string(m_profile.generateMeshTime/1000) + String("ms\n")+
			std::to_string(m_profile.generateMeshTime/1000/1000) + String("s\n") +
			std::to_string(m_profile.generateMeshTime/1000/1000/60) + String("min\n");
		OutputDebugString(log.c_str());

		log = String("generate physx mesh time: ")+ 
			std::to_string(m_profile.generatePhysxMeshTime/1000) + String("ms\n")+
			std::to_string(m_profile.generatePhysxMeshTime/1000/1000) + String("s\n") +
			std::to_string(m_profile.generatePhysxMeshTime/1000/1000/60) + String("min\n");
		OutputDebugString(log.c_str());

		log = String("generate biome texture time: ")+ 
			std::to_string(m_profile.generateTexTime/1000) + String("ms\n")+
			std::to_string(m_profile.generateTexTime/1000/1000) + String("s\n") +
			std::to_string(m_profile.generateTexTime/1000/1000/60) + String("min\n");
		OutputDebugString(log.c_str());
	}
#endif

#if 0
	auto v3 = GetFineLayerCenters(true);
	for (auto pos : v3)
	{
		DebugDrawManager::instance()->drawSphere(pos,
			10.0f, ColorValue::Green, DebugDrawManager::WORLD_SPACE);
	}
	//DebugDrawManager::instance()->drawSphere()
#endif

	if (m_mgr->m_testGenerateBuffer)
	{
		generateVertexBufferBenchmark();
		m_mgr->m_testGenerateBuffer = false;
	}

	if (m_mgr->m_testQuerySurface)
	{
		surfaceQueryBenchmark();
		m_mgr->m_testQuerySurface = false;
	}

	if (m_mgr->m_testOceanQuerySurface) 
	{
		if (m_ocean)
		{
			//m_ocean->testSurfaceQueryBenchmark();
			//m_mgr->m_testOceanQuerySurface = false;
		}
	}

	if (m_mgr->m_showModifierBoundingBox)
	{
		if (m_staticModifier) m_staticModifier->DrawBounds(m_worldMat, pCamera->getDerivedPosition());
	}

	// OutputDebugStringA(("free4 : " + std::to_string(m_nodePool->m_free4.size()) + "\n").c_str());
}

void SphericalTerrain::findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue)
{
	if (!isCreateFinish()) return;

	const bool isMainCam = pCamera == m_mgr->m_mgr->getMainCamera();
	const auto& snowCamName = DeformSnowMgr::instance()->getSnowCamName();
	const bool isSnowCam = pCamera->getName() == snowCamName;
	if (!(isMainCam || isSnowCam))
		return;
	bool updateGeo = true;
	bool isInvokerCam = true;
	if (isSnowCam)
	{
		isInvokerCam = false;
	}
#ifdef ECHO_EDITOR
	isInvokerCam = isInvokerCam && !pCamera->isSecondCameraRendering();
#endif
	updateGeo = isInvokerCam;

	forceAllComputePipelineExecute();

	printLog(pCamera);

	const auto camW = pCamera->getDerivedPosition();
	const Vector3 camL = m_worldMatInv * camW;

    float hlodRadius = 1.0f;

    if(isSphere())
    {
        hlodRadius = PlanetManager::getSphereHlodRadius(m_terrainData->geometry.sphere.radius);
    }
    else if(isTorus())
    {
        hlodRadius = PlanetManager::getTorusHlodRadius(m_terrainData->geometry.torus.radius);
    }
    else
    {
        assert(false &&"Unkonwn planet topology!");
    }
    
	int hLod = std::clamp(static_cast<int>(std::floorf(std::log2f(
		std::max(camL.length() - hlodRadius, std::numeric_limits<float>::epsilon()) / s_lodRatio))), 0, 4) - 1;
    
	if (m_forceLowLOD) hLod = std::max(0, hLod);
	if (isInvokerCam)
	{
		m_lowLod = m_useLowLODMaterial && hLod >= 0;
	}
	else if (m_lowLod)
	{
		hLod = std::max(0, hLod);
	}

	// atmosphere
	if(m_atmosProperty != nullptr){
		//先获取星球雾后期，如果不存在， Pass
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem != nullptr) {
			RenderStrategy* pCurRenderStrategy = pRenderSystem->getCurRenderStrategy();
			if (pCurRenderStrategy != nullptr) {
				PostProcess* pc = pCurRenderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (pc != nullptr) {
					SphericalTerrainFog* sphFog = dynamic_cast<SphericalTerrainFog*>(pc);
					if (sphFog != nullptr) {

						// 弧光
						if (m_atmosProperty->enableAtmosphere) {

							// 如果当前相机在水下，不画弧光
							if (!pCamera->getCameraUnderWater()) {
								// 弧光开启，投递弧光
								if (m_atmos && m_atmosProperty->enableSecondLayerEffect && atmosInitSuccess) {
									Pass* pass = SphericalTerrainAtmosData::instance()->mAtmosMaterial->getPass(NormalPass);
									RenderQueueGroup* queueGroup = pQueue->getRenderQueueGroup(RenderQueue_Solid_Transparent);
									queueGroup->addRenderable(pass, m_atmos.get(), 0);
								}
							}
						}

						// 星球雾效，只有最近的星球才考虑更新星球雾效
						if(m_mgr->getNearestPlanetToSurface() == this) {
							
							// 当前星球是最近的星球,但是未开启雾效
							if (!m_atmosProperty->enableAtmosphere) {
								sphFog->setTerrain(nullptr);
							}
							// 在切换了新的星球或者当前星球的参数有变化时，更新雾效参数
							else if (m_atmosProerptyDirty == true || sphFog->m_SphericalTerrain != this ) {
								sphFog->setFogParams(
									m_atmosProperty->profile.upperLayerColor,
									m_atmosProperty->profile.lowerLayerColor,
									m_atmosProperty->profile.absorptionProfile[1].expTerm,
									m_atmosProperty->profile.absorptionProfile[1].expScale,
									m_atmosProperty->profile.blendParameter.x,
									m_atmosProperty->profile.blendParameter.y,
									m_atmosProperty->profile.blendParameter.z,
									m_atmosProperty->profile.blendParameter.w,
									m_atmosProperty->profile.skyParam);
								sphFog->setAtmosphereRange(
									m_atmosProperty->boundary.rTop,
									m_atmosProperty->boundary.rBottom,
									m_atmosProperty->boundary.rBlendRange,
									m_atmosProperty->boundary.rBlendDistance
								);
								sphFog->enableAtmosphere(m_atmosProperty->enableSecondLayerEffect);
								sphFog->setTerrain(this);
								m_atmosProerptyDirty = false;
							}
						}
					}
				}
			}
		}
	}

	UpdateVolumetricCloudParam(pCamera);

    //IMPORTANT:According to low-lod state to load or free biome GPU texture.
	if (isInvokerCam)
	{
        if (!m_lowLod)
        {
            if (!m_isRequestedBiomeTexture)
            {
                //NOTE: Ask for load biome detail GPU texture.
                m_terrainData->requestBiomeTexture(m_terrainData);
                m_isRequestedBiomeTexture = true;
            }
        }
        else
        {
            if (m_isRequestedBiomeTexture)
            {
                //NOTE: Free biome detail GPU texture.
                m_terrainData->freeBiomeTexture();
                m_isRequestedBiomeTexture = false;
            }
        }
    }

	//NOTE:Warfog rendering
    // Visibility test.
    Sphere sphere(m_pos, getMaxRealRadius());
    if (m_visible && !isSnowCam)
    {
		if (pCamera->isVisible(sphere))
		{
			if (m_PlotDivision && m_bEnablePlotDivision)
			{
				m_PlotDivision->findVisibleObjects(pCamera, pQueue);
			}

			if (m_warfog && m_enableRegionVisualEffect)
			{
				m_warfog->findVisibleObjects(pCamera, pQueue);
			}
		}

        if(m_enableLowLODCloud && m_cloud)
        {
            //IMPORTANT(yanghang):new hlod for other stuff mesh LOD switch.
            // This hlod is different from planet rendring hlod.
            int newHLOD =
                std::clamp(static_cast<int>(std::floorf(std::log2f(
                                                            std::max(camL.length(), std::numeric_limits<float>::epsilon()) / s_lodRatio))), 0, 3);
            m_cloud->findVisibleObjects(pCamera, pQueue, newHLOD);
        }
	}

	if (m_ocean && m_haveOcean && !isSnowCam)
	{
		m_ocean->findVisibleObjects(pCamera, pQueue,hLod);
	}

	if (m_pPlanetGenObjectScheduler && isInvokerCam && m_visible && !m_bAtlas && (!mRoad || mRoad->isBuild())) {
		m_pPlanetGenObjectScheduler->findVisibleObjects(pCamera);
	}

	// quick exit if camera's far away from planet
	if (m_lowLod &&
        findPlanetVisible(pCamera, pQueue, hLod))
	{
		return;
	}

	if (m_forceLowLOD) return;

	updateGeo = updateGeo && !m_lowLod;
	if (updateGeo)
	{
		m_currFrameRequests = 0;
		float minRenderDistSqrL = s_detailDistance / m_radius; // always subdivide node if closer than detail distance
		minRenderDistSqrL *= minRenderDistSqrL;
		updateGeometry(camL, minRenderDistSqrL);
	}

	//IMPORTANT: Dynamic load biome biome GPU texture for less memory use.
    if(!m_editingTerrain)
	{
        //IMPORTANT(yanghang):Biome GPU Texture isn't ready.
        // force stay in LOW_LOD mode for correct display.
        bool detailGeometryNotReady =
            std::any_of(m_slices.begin(), m_slices.end(), [](const auto& node)
                        {
                            return !(node->m_vertexBuffer || node->hasChildren());
                        });
        if (!m_terrainData->isBiomeTextureReady() ||
			detailGeometryNotReady)
        {
            m_bindGPUBiomeTexture = false;
            //NOTE:Unbind texture. texture maybe freed.
        	if (m_material.isV1())
        	{
        		Material* mat = m_material.getV1();
        		mat->m_textureList[MAT_TEXTURE_CUSTOMER].setNull();
                mat->m_textureList[MAT_TEXTURE_CUSTOMER3].setNull();
        	}
        	else if (m_material.isV2())
        	{
        		MaterialV2* mat = m_material.getV2();
        		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER, TexturePtr{});
        		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, TexturePtr{});
        	}


            //IMPORTANT(yanghang):Force to use LOW_LOD rendering
			if (m_useLowLODMaterial)
			{
				m_lowLod = true;
				hLod = 0;
				findPlanetVisible(pCamera, pQueue, hLod);
			}
            return;
        }
        else
        {
            if (!m_bindGPUBiomeTexture)
            {
                bindTextures();
                m_bindGPUBiomeTexture = true;
            }
        }
    }
    else
    {
        //IMPORTANT:Planet edit mode. Force request TextureRes at initialize.
        // Only try binding texture.
        if (!m_bindGPUBiomeTexture)
        {
            if(m_terrainData->isBiomeTextureReady())
            {
                bindTextures();
                m_bindGPUBiomeTexture = true;
            }
            else
            {
                //NOTE:Unbind texture. texture maybe freed.
            	if (m_material.isV1())
            	{
            		Material* mat = m_material.getV1();
            		mat->m_textureList[MAT_TEXTURE_CUSTOMER].setNull();
            		mat->m_textureList[MAT_TEXTURE_CUSTOMER3].setNull();
            	}
            	else if (m_material.isV2())
            	{
            		MaterialV2* mat = m_material.getV2();
            		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER, TexturePtr{});
            		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, TexturePtr{});
            	}
                return;
            } 
        }
    }

	if (!m_visible) return;

	std::vector<Plane> frustumW;
	for (int i = 0; i < 6; ++i)
	{
		if (i == FRUSTUM_PLANE_FAR && pCamera->getFarClipDistance() == 0) continue;
		frustumW.emplace_back(pCamera->getFrustumPlane(i));
	}

	if (isSphere() && !m_editingTerrain)
	{
		// Vector3 camDir = camPosLocal;
		// https://www.geogebra.org/geometry/adbakjpn
		Vector3 camDirW = camW - m_pos;
		float camDistW  = camDirW.normalise();
		float innerR    = getMaxRealRadius();
		float outerR    = getMinRealRadius();
		float innerTan  = std::max(std::numeric_limits<float>::min(), std::sqrt(std::max(0.0f, camDistW * camDistW - innerR * innerR)));
		// float d0 = innerTan * innerTan / camDist;
		float outerTan     = innerTan + std::sqrt(outerR * outerR - innerR * innerR);
		float horizonDepth = outerTan * innerTan / camDistW;
		const Plane horizonPlane(camDirW, camW - camDirW * horizonDepth);

		frustumW.emplace_back(horizonPlane);
	}

	/// render tiles
	struct NodePass
	{
		Node* node;
		PassType pass;

		NodePass(Node* n, const PassType p) : node(n), pass(p){}
	};

	const auto* sceneMgr  = pCamera->getSceneManager();
	const auto* csmMgr    = sceneMgr->getPsmshadowManager();
	const bool csmEnable  = csmMgr && csmMgr->getShadowEnable();
	const auto* dpsmMgr   = sceneMgr->getPointShadowManager();
	const bool dpsmEnable = dpsmMgr && dpsmMgr->IsEnabled();

	auto calculatePassType = [&](const auto& bounds)
	{
		const bool csm  = csmEnable && csmMgr->testShadowReceive(bounds);
		const bool dpsm = dpsmEnable && dpsmMgr->IsReceivingShadow(bounds);
		auto passType = Material::calPassType(false, m_mgr->m_mgr, true, false, false, csm, false, dpsm);
		return passType;
	};

	std::multimap<float, NodePass> visibleNodes;
	for (auto&& slice : m_slices)
	{
		if (getHideTiles() && !isSnowCam) continue;
		Node::traverseTree(slice.get(), [this, camW, &frustumW, &visibleNodes, &calculatePassType](Node* node)
		{
			if (!node->m_active)
			{
				return true;
			}

			AxisAlignedBox aabb;
			OrientedBox obb;
			float camDistSqr;

			bool disjoint;
			if (m_useObb)
			{
				obb = node->getOrientedBoundLocal();
				obb.Transform(m_realScale, m_rot, m_pos);
				camDistSqr = obb.DistanceSqr(camW);
				disjoint = obb.ContainedBy(frustumW) == OrientedBox::Disjoint;
			}
			else
			{
				aabb = node->getBoundLocal();
				aabb.Transform(m_worldMat);
				camDistSqr = aabb.squaredDistance(camW);

				disjoint = std::any_of(frustumW.begin(), frustumW.end(),
					[&aabb](const Plane& plane) { return plane.getSide(aabb) == Plane::NEGATIVE_SIDE; });
			}

			if (disjoint)
			{
				return false;
			}

			if (node->m_ibDirty)
			{
				const uint8_t mask = node->getIndexBufferType();
				node->setIndexBuffer(s_tileIndexBuffers[mask], s_tileIndexCount[mask]);
				node->m_ibDirty = false;
			}

			visibleNodes.emplace(camDistSqr, NodePass(node, m_useObb ? calculatePassType(obb) : calculatePassType(aabb)));

			//NOTE(yanghang):Debug terrain block bound box.
			if (m_mgr->m_showDebugBoundingbox)
			{
				Vector3 v[8];
				if (m_useObb) obb.GetVertices(v);
				else aabb.GetVerts(v);
				ColorValue lineColor = LodColor[node->m_depth % 5];
				if (m_displayMode == BiomeDisplayMode::Tile) lineColor = LodColor[getNodeIndex(*node) % std::size(LodColor)];
				if (m_mgr->m_showModifierBoundingBox) lineColor = node->m_modifierInstances.empty() ? ColorValue(0.f, 0.6f, 0.f, 1.f) : ColorValue(0.6f, 0.f, 0.f, 1.f);
				DrawBound(v, lineColor);
			}

			return false;
		});
	}

	for (const auto& visibleNode : visibleNodes)
	{
		const auto& nodePass = visibleNode.second;
		RenderQueueGroup* queueGroup = pQueue->getRenderQueueGroup(RenderQueue_Terrain);

		if (isSnowCam && DeformSnowMgr::instance()->m_enable)
		{
#ifdef _WIN32
			Pass* pass = SnowMatMgr::instance()->SnowTerrainMat->getPass(nodePass.pass);
			queueGroup->addRenderable(pass, nodePass.node);
#endif // _WIN32
		}
		else
		{
			if (m_material.isV1())
			{
				Pass* pass = m_material.getV1()->getPass(nodePass.pass);
				queueGroup->addRenderable(pass, nodePass.node);
			}
			else if (m_material.isV2())
			{
				PostRenderableParams params{
					.material = m_material.getV2(),
					.renderable = nodePass.node,
					.renderQueue = pQueue,
					.passMaskFlags = PassMaskFlag::UseLight
				};
				if (nodePass.pass != NormalPass)
				{
					params.passMaskFlags |= PassMaskFlag::ReceiveShadow;
					params.passMaskFlags |= PassMaskFlag::UseDpsm;
				}
				MaterialShaderPassManager::fastPostRenderable(params);
			}
		}
		m_mgr->m_drawTriangleCount += (INDEX_COUNT / 3);
	}

	if (isSnowCam)
		return;

#ifdef ECHO_EDITOR
	showGlobalViews(pQueue);
#endif

	if (m_vegHandle)
	{
		for (auto&& box : m_vegetationRecheckRange)
		{
			m_vegHandle->recheckPosition(box);
		}
	
		if (getSurfaceMipLs(camL) == m_maxDepth)
		{
			m_vegHandle->findVisibleObjects(pCamera, pQueue);
		}
	}
	m_vegetationRecheckRange.clear();

	if (isMainCam && isInvokerCam && mRoad)
		mRoad->findVisibleRoads(pCamera);

	if (m_mgr->m_showModifierBoundingBox)
	{
		m_modifier->DrawBounds(m_worldMat);
	}

}

bool SphericalTerrain::findPlanetVisible(const Camera* pCamera, RenderQueue* pQueue, const int lod)
{
	auto* planet = dynamic_cast<Planet*>(m_planet.get());
	if (!planet) return false;

	if (planet->launchTask())
	{
		m_lowLod = false;
		return false;
	}

	if (!m_visible) return true;

	if (isBoundAccurate())
	{
		for (int i = 0; i < 6; ++i)
		{
			if (i == FRUSTUM_PLANE_FAR) continue;
			if (pCamera->getFrustumPlane(i).getDistance(m_pos) < -getMaxRealRadius()) return true;
		}
	}

	DMatrix4 worldMat;
	auto newPos = m_pos;
	auto newScl = m_scale * m_radius;
	if (pCamera->projectToFarPlane(newPos, newScl, getMaxRealRadius()))
	{
		worldMat.makeTransform(newPos, newScl, m_rot);
	}
	else
	{
		worldMat = m_worldMat;
	}

	m_planet->m_world = worldMat;
	m_planet->setIndexBuffer(s_planetIndexBuffers[lod], s_planetIndexCount[lod] * std::min(6, m_numSlice[0]) / 6);
	RenderQueueGroup* queueGroup = pQueue->getRenderQueueGroup(RenderQueue_Terrain);
	Pass* pass = nullptr;
	if (m_material_low_LOD.isV1())
	{
		Material* mat = m_material_low_LOD.getV1();
		pass = mat->getPass(NormalPass);
		queueGroup->addRenderable(pass, m_planet.get());
	}
	else if (m_material_low_LOD.isV2())
	{
		MaterialV2* mat = m_material_low_LOD.getV2();

		RenderStageType stageType = mat->hasValidRenderStage(RenderStageType::GBufferPass)
			                            ? RenderStageType::GBufferPass
			                            : RenderStageType::SolidPass;

		MaterialShaderPassManager::postRenderableToQueue(mat, m_planet.get(), pQueue, stageType, PassMaskFlags{}, 0);
	}

	return true;
}

void SphericalTerrain::createMaterial()
{
	// TODO: 先用材质兼容模式开关控制
	if (Root::instance()->getEnableMatV2CompatibilityMode() &&
		MaterialShaderPassManager::instance()->existsShaderPass("BiomeTerrain"))
	{
		MaterialV2Ptr material = MaterialV2Manager::instance()->createManual();
		m_material = material;

		Mat::MaterialInfo matInfo = Mat::MaterialInfoBuilder()
		                            .name("BiomeTerrain")
		                            .shaderPassName("BiomeTerrain")
		                            .vertexCompression(false)
		                            .build();

		uint32 n = 0;
		ECHO_IA_DESC iaDesc{};
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, pos);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_POSITION0;
		++n;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, normal);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_NORMAL0;
		++n;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, uv);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT2;
		iaDesc.ElementArray[n].Semantics = ECHO_TEXCOORD0;
		++n;

		iaDesc.nElementCount = n;

		iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		iaDesc.StreamArray[0].nRate = 1;
		iaDesc.StreamArray[0].nStride = sizeof(TerrainVertex);
		iaDesc.nStreamCount = 1;

		std::vector<ShaderMacrosItem> macros;
		macros.emplace_back(SHAPE_MACRO[m_topology]);

#ifdef ECHO_EDITOR
		if (m_editingTerrain)
		{
			macros.emplace_back("_DEBUG_MODE_");
			matInfo.variantMasks.push_back("_EDITOR_MODE_");
		}
#endif

		if (material->createFromMaterialInfo(matInfo, false))
		{
			material->setCullMode(m_editingTerrain ? ECHO_CULL_NONE : ECHO_CULL_BACK);
			material->setCustomVSMacros(macros);
			material->setCustomPSMacros(macros);

			material->setVertexInputAttributeDesc(iaDesc);
			material->load();
		}

		if (m_useLowLODMaterial)
		{
			matInfo = Mat::MaterialInfoBuilder()
			          .name("BiomeTerrain")
			          .shaderPassName("BiomeTerrain")
			          .vertexCompression(false)
			          .addVariantMask("_LOW_LOD_")
			          .build();

			MaterialV2Ptr material_low_LOD = MaterialV2Manager::instance()->createManual();
			m_material_low_LOD = material_low_LOD;

			std::vector<ShaderMacrosItem> macrosLowLOD;
			macrosLowLOD.emplace_back(SHAPE_MACRO[m_topology]);

			if (material_low_LOD->createFromMaterialInfo(matInfo, false))
			{
				material_low_LOD->setCullMode(m_editingTerrain ? ECHO_CULL_NONE : ECHO_CULL_BACK);
				material_low_LOD->setCustomVSMacros(macrosLowLOD);
				material_low_LOD->setCustomPSMacros(macrosLowLOD);

				material_low_LOD->setVertexInputAttributeDesc(iaDesc);
				material_low_LOD->load();
			}
		}
	}
	else
	{
		String vsShader, psShader;
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("BiomeTerrain_VS.txt"), vsShader);
		ShaderContentManager::instance()->GetOrLoadShaderContent(Name("BiomeTerrain_PS.txt"), psShader);

		if (vsShader.empty() || psShader.empty())
		{
			LogManager::instance()->logMessage("-error-\t PlanetTerrain shader files didn't load correctly!", LML_CRITICAL);
			return;
		}

		MaterialPtr material = MaterialManager::instance()->createManual();
		m_material = material;

		//IMPORTANT(yanghang):Materail Object manager need this flag to create shadow pass,
		//when shadow config changed.
		//Recompile shader need this name to reload shader text. 
		material->setShaderName(String("BiomeTerrain_VS.txt"), String("BiomeTerrain_PS.txt"));
		material->setMaterialPassMask(1 << MPO_ShadowResivePass);
		material->addCustomMacros(SHAPE_MACRO[m_topology]);


#ifdef ECHO_EDITOR
		if (m_editingTerrain)
		{
			material->addCustomMacros("_EDITOR_MODE_");
			material->addCustomMacros("_DEBUG_MODE_");
		}
#endif

		uint32 n = 0;
		ECHO_IA_DESC& iaDesc = material->m_desc.IADesc;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, pos);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_POSITION0;
		++n;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, normal);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT3;
		iaDesc.ElementArray[n].Semantics = ECHO_NORMAL0;
		++n;
		iaDesc.ElementArray[n].nStream = 0;
		iaDesc.ElementArray[n].nOffset = offsetof(TerrainVertex, uv);
		iaDesc.ElementArray[n].eType = ECHO_FLOAT2;
		iaDesc.ElementArray[n].Semantics = ECHO_TEXCOORD0;
		++n;

		iaDesc.nElementCount = n;

		iaDesc.StreamArray[0].bPreInstance = ECHO_FALSE;
		iaDesc.StreamArray[0].nRate = 1;
		iaDesc.StreamArray[0].nStride = sizeof(TerrainVertex);
		iaDesc.nStreamCount = 1;

		auto& renderState = material->m_desc.RenderState;
		renderState.DepthTestEnable = ECHO_TRUE;
		renderState.DepthWriteEnable = ECHO_TRUE;
		renderState.CullMode = m_editingTerrain ? ECHO_CULL_NONE : ECHO_CULL_BACK;
		renderState.BlendEnable = ECHO_FALSE;
		renderState.BlendDst = ECHO_BLEND_INVSRCALPHA;
		renderState.BlendSrc = ECHO_BLEND_SRCALPHA;
		renderState.BlendOp = ECHO_BLEND_OP_ADD;
		renderState.PolygonMode = ECHO_PM_SOLID;
		renderState.DrawMode = ECHO_TRIANGLE_LIST;

		renderState.StencilEnable = ECHO_TRUE;
		renderState.StencilRef = TERRAIN_STENCIL_MASK;
		renderState.StencilMask = TERRAIN_STENCIL_MASK;
		renderState.StencilWriteMask = 0xff ^ FIRST_PERSON_STENCIL_MASK;
		renderState.StencilFunc = ECHO_CMP_GREATEREQUAL;

		material->setShaderSrcCode(vsShader, psShader);
		material->createPass(NormalPass);

		const auto* sceneManager = m_mgr->m_mgr;
		const bool shadowEnable  = sceneManager->getPsmshadowManager()->getShadowEnable();
		if (shadowEnable)
		{
			material->createPass(ShadowResivePass);
		}
		material->setMaterialPassMask(1ull << MPO_ShadowResivePass);
		const bool pointShadowEnable = sceneManager->getPointShadowManager()->IsEnabled();
		if (pointShadowEnable)
		{
			material->createPass(PointShadowReceiver);
		}
		if (shadowEnable && pointShadowEnable)
		{
			material->createPass(ShadowReceiverPointShadowReceiver);
		}
		material->setMaterialPointShadowPassMask(MPSPO_PointShadowReceiver, MPSPO_ShadowReceiverPointShadowReceiver);

		material->loadInfo();

		if (m_useLowLODMaterial)
		{
			MaterialPtr material_low_LOD = MaterialManager::instance()->createManual();
			m_material_low_LOD = material_low_LOD;
			material_low_LOD->setShaderName(String("BiomeTerrain_VS.txt"), String("BiomeTerrain_PS.txt"));

			std::memcpy(&material_low_LOD->m_desc.RenderState, &renderState, sizeof(ECHO_RENDER_STATE));
			std::memcpy(&material_low_LOD->m_desc.IADesc, &iaDesc, sizeof(ECHO_IA_DESC));

			material_low_LOD->addCustomMacros("_LOW_LOD_");
			material_low_LOD->addCustomMacros(SHAPE_MACRO[m_topology]);
			material_low_LOD->setShaderSrcCode(vsShader, psShader);
			material_low_LOD->createPass(NormalPass);
			material_low_LOD->loadInfo();
		}
	}

#ifdef ECHO_EDITOR
	if (g_worldMapMat.isNull())
	{
		if (Root::instance()->getDeviceType() != ECHO_DEVICE_DX11)
		{
			LogManager::instance()->logMessage("-error-\t WorldMap material only support DX11!", LML_CRITICAL);
			return;
		}

		g_worldMapMat = MaterialManager::instance()->createManual();

		// even if elementCnt == 0, RC creates InputLayout and throws error
		// TODO: remove if RC able to handle empty IA case
		auto& ia                     = g_worldMapMat->m_desc.IADesc;
		ia.ElementArray[0].nStream   = 0;
		ia.ElementArray[0].nOffset   = 0;
		ia.ElementArray[0].eType     = ECHO_FLOAT1;
		ia.ElementArray[0].Semantics = ECHO_POSITION0;
		ia.nElementCount             = 1;

		auto& rs            = g_worldMapMat->m_desc.RenderState;
		rs.DepthTestEnable  = ECHO_FALSE;
		rs.DepthWriteEnable = ECHO_FALSE;
		rs.CullMode         = ECHO_CULL_NONE;
		rs.BlendEnable      = ECHO_FALSE;
		rs.PolygonMode      = ECHO_PM_SOLID;
		rs.DrawMode         = ECHO_TRIANGLE_STRIP;

		g_worldMapMat->setShaderSrcCode(IMAGE_VS_HLSL, IMAGE_PS_HLSL);
		g_worldMapMat->createPass(NormalPass);
		g_worldMapMat->loadInfo();

		g_imageRenderOp                               = std::make_unique<RenderOperation>();
		g_imageRenderOp->mPrimitiveType               = ECHO_TRIANGLE_STRIP;
		g_imageRenderOp->GetMeshData().m_vertexCount = 4;
		g_imageRenderOp->useIndexes                   = false;
		g_imageRenderOp->isManualVertexFetch          = true;

		for (int i = 0; i < 3; ++i)
		{
			g_planetViews[i] = std::make_unique<SimpleImage>();
		}
	}
#endif
}

void SphericalTerrain::destroyMaterial()
{
	if (!m_material.isNull())
	{
		if (m_material.isV1())
		{
			MaterialManager::instance()->remove(m_material.getV1Ptr());
		}
		else if (m_material.isV2())
		{
			MaterialV2Manager::instance()->remove(m_material.getV2Ptr());
		}
		m_material = nullptr;
	}

	if (!m_material_low_LOD.isNull())
	{
		if (m_material_low_LOD.isV1())
		{
			MaterialManager::instance()->remove(m_material_low_LOD.getV1Ptr());
		}
		else if (m_material_low_LOD.isV2())
		{
			MaterialV2Manager::instance()->remove(m_material_low_LOD.getV2Ptr());
		}
		m_material_low_LOD = nullptr;
	}

#ifdef ECHO_EDITOR
	if (!g_worldMapMat.isNull())
	{
		MaterialManager::instance()->remove(g_worldMapMat);
		g_worldMapMat.setNull();

		for (int i = 0; i < 3; ++i)
		{
			g_planetViews[i].reset();
		}

		g_imageRenderOp.reset();
		g_cylindricalProjection.reset();
		g_cylindricalProjectionFloat.reset();
	}
#endif
}

void SphericalTerrain::applyGeometry()
{
	bool matMapValid = false;
	switch (m_topology)
	{
	case PTS_Sphere:
	{
		auto& sphere = m_terrainData->geometry.sphere;
		setRadius(sphere.radius);
		setElevation(sphere.elevationRatio);
		m_pTerrainGenerator = m_terrainData->m_pTerrainGenerator.get();
		setSliceCount(6, 1);
		m_biomeTextureWidth = m_biomeTextureHeight = BiomeTextureResolution;
		const auto& idMaps = m_terrainData->m_matIndexMaps;
		matMapValid = std::all_of(idMaps.begin(), idMaps.begin() + 6, [this](const auto& map)
		{
			return map.byteSize == static_cast<int>(m_biomeTextureWidth * m_biomeTextureHeight * sizeof(uint8_t));
		});

		if (!matMapValid)
		{
			break;
		}

		for (int i = 0; i < 6; ++i)
		{
			assert(m_terrainData->m_matIndexMaps[i].pixels);
			m_materialIndexMap.faces[i] = &m_terrainData->m_matIndexMaps.at(i);
		}
	}
	break;
	case PTS_Torus:
	{
		auto torus = m_terrainData->geometry.torus;
		setRadius(torus.radius, torus.relativeMinorRadius);
		setElevation(torus.elevationRatio);
		setSliceCount(torus.maxSegments[0], torus.maxSegments[1]);
		m_biomeTextureWidth  = BiomeCylinderTextureWidth;
		m_biomeTextureHeight = std::clamp(static_cast<int>(static_cast<float>(m_biomeTextureWidth) * m_relativeMinorRadius),
			1, m_biomeTextureWidth);

		if (!m_terrainData->m_matIndexMaps[0].pixels) break;

		matMapValid = m_terrainData->m_matIndexMaps[0].byteSize == static_cast<int>(m_biomeTextureWidth * m_biomeTextureHeight * sizeof(uint8_t));

		if (!matMapValid)
		{
			break;
		}

		auto& matIdx = m_materialIndexMap.faces[0];
		assert(m_terrainData->m_matIndexMaps.front().pixels);
		matIdx = &m_terrainData->m_matIndexMaps.front();
	}
	break;
	}
	
	setMaxDepth(m_terrainData->geometry.quadtreeMaxDepth);

	if (!matMapValid)
	{
		LogManager::instance()->logMessage("-ERROR-\t" + m_terrainData->getName().toString() + " Material index map content invalid.", LML_CRITICAL);
	}
}

void SphericalTerrain::bindTextures()
{
	if (m_material.isNull())
	{
		return;
	}
	
	if (m_material.isV1())
	{
		Material* material = m_material.getV1();

		material->m_textureList[MAT_TEXTURE_CUSTOMER6] = loadFbmTexture();
		//material->m_textureList[MAT_TEXTURE_CUSTOMER7] = loadVoronoiTexture();

		if (const auto& data = m_terrainData; !data.isNull())
		{
			if (const auto& textureResource = data->m_planetTexRes; !textureResource.isNull())
			{
				const auto& biomeGpuData = textureResource->m_biomeGPUData;

				material->m_textureList[MAT_TEXTURE_CUSTOMER]  = biomeGpuData.diffuseTexArray;
				material->m_textureList[MAT_TEXTURE_CUSTOMER3] = biomeGpuData.normalTexArray;
			}
			else
			{
				material->m_textureList[MAT_TEXTURE_CUSTOMER]  = nullptr;
				material->m_textureList[MAT_TEXTURE_CUSTOMER3] = nullptr;
			}

			if (!data->lookupTex.isNull())
			{
				material->m_textureList[MAT_TEXTURE_CUSTOMER2] = data->lookupTex;
			}
			else
			{
				material->m_textureList[MAT_TEXTURE_CUSTOMER2] = nullptr;
			}
		}
	}
	else if (m_material.isV2())
	{
		MaterialV2* material = m_material.getV2();

		material->setTexture(Mat::TEXTURE_BIND_CUSTOMER6, loadFbmTexture());
		//material->setTexture(Mat::TEXTURE_BIND_CUSTOMER7, loadVoronoiTexture());

		if (const auto& data = m_terrainData; !data.isNull())
		{
			if (const auto& textureResource = data->m_planetTexRes; !textureResource.isNull())
			{
				const auto& biomeGpuData = textureResource->m_biomeGPUData;

				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER, biomeGpuData.diffuseTexArray);
				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, biomeGpuData.normalTexArray);
			}
			else
			{
				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER, TexturePtr{});
				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, TexturePtr{});
			}

			if (!data->lookupTex.isNull())
			{
				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER2, data->lookupTex);
			}
			else
			{
				material->setTexture(Mat::TEXTURE_BIND_CUSTOMER2, TexturePtr{});
			}
		}
	}
}

void SphericalTerrain::createIndexBuffer()
{
	if (s_tileIndexBuffers[0] != nullptr) return;

	for (int i = 0; i < 16; ++i)
	{
		const auto indices    = CreateNodeIndices(i);
		s_tileIndexCount[i]   = static_cast<int>(indices.size());
		s_indexData[i]		  = indices;
		s_tileIndexBuffers[i] = Root::instance()->getRenderSystem()->createStaticIB(indices.data(),
			static_cast<uint32>(s_tileIndexCount[i] * sizeof(Node::IndexType)));
	}

	for (int i = 0; i < 4; ++i)
	{
		const auto indices = CreateCubeSphereIndices(CreateNodeIndicesSimplified(i));
		s_planetIndexCount[i] = static_cast<int>(indices.size());
		s_planetIndexBuffers[i] = Root::instance()->getRenderSystem()->createStaticIB(indices.data(),
			static_cast<uint32>(s_planetIndexCount[i] * sizeof(Node::IndexType)));
	}
}

void SphericalTerrain::destroyIndexBuffer()
{
	if (s_count > 0) return;

	for (auto&& ib : s_tileIndexBuffers)
	{
		if (!ib) break;
		Root::instance()->getRenderSystem()->destoryBuffer(ib);
		ib = nullptr;
	}
	for (auto && data : s_indexData) data.clear();
	for (int& count : s_tileIndexCount) count = 0;

	for (auto&& ib : s_planetIndexBuffers)
	{
		if (!ib) break;
		Root::instance()->getRenderSystem()->destoryBuffer(ib);
		ib = nullptr;
	}
	for (int& count : s_planetIndexCount) count = 0;

}

#ifdef ECHO_EDITOR
void SphericalTerrain::createComputePipeline()
{
	if (!m_gpuMode) return;

	if (!m_editingTerrain && m_vbGenAsync) return;

	m_pcg->create(m_topology, m_bStampTerrain, m_editingTerrain, m_pTerrainGenerator);
}

void SphericalTerrain::destroyComputePipeline()
{
	m_pcg->destroy();
}

bool SphericalTerrain::isComputePipelineReady() const
{
	return m_pcg->isReady(m_editingTerrain);
}

void SphericalTerrain::createComputeTarget()
{
	if (!m_gpuMode) return;

	if (!m_editingTerrain) return;

	m_pcg->createResource(m_topology, m_biomeTextureWidth, m_biomeTextureHeight);
}

void SphericalTerrain::destroyComputeTarget()
{
	m_pcg->destroyResource();
}

RcComputeTarget** SphericalTerrain::getTemperatureComputeTargets()
{
	return m_pcg->getTemperatureTargets();
}

RcComputeTarget** SphericalTerrain::getHumidityComputeTargets()
{
	return m_pcg->getHumidityTargets();
}

RcComputeTarget** SphericalTerrain::getMaterialIndexComputeTargets()
{
	return m_pcg->getMaterialIndexTargets();
}

void SphericalTerrain::getWorldMaps(Bitmap* elevation, Bitmap* standardDeviation)
{
	if (!m_editingTerrain || m_topology != PTS_Sphere || hasShapeGenerator()) return;

	if (!elevation && !standardDeviation) return;

#if OUTPUT_HEIGHTMAP
	auto outputImage = [this](const Bitmap& map, DXGI_FORMAT fmt, const std::string_view& type)
	{
		const size_t bytePerPixel = DirectX::BitsPerPixel(fmt) / 8;
		const DirectX::Image img
		{
			.width = static_cast<size_t>(map.width),
			.height = static_cast<size_t>(map.height),
			.format = fmt,
			.rowPitch = map.width * bytePerPixel,
			.slicePitch = size_t(map.width) * map.height * bytePerPixel,
			.pixels = static_cast<uint8_t*>(map.pixels)
		};

		auto stem    = m_terrainData->getName().toString();
		size_t start = stem.find_last_of('/') + 1;
		size_t end   = stem.find_last_of('.');
		stem         = stem.substr(start, end - start);
		auto wName   = std::wstring(stem.begin(), stem.end());
		wName += L"_" + std::wstring(type.begin(), type.end()) + L".dds";

		auto hr = DirectX::SaveToDDSFile(img, DirectX::DDS_FLAGS_NONE, wName.c_str());
		if (FAILED(hr))
		{
			__nop(), __debugbreak();
		}
	};
#endif

	int resolution = (Node::GRID_RESOLUTION - 1) << m_maxDepth;

	auto elevations = m_pcg->generateElevation(*this, resolution, resolution);

	if (elevation)
	{
		m_pcg->generateElevationWorldMap(*this, *elevation,
			reinterpret_cast<RcSampler* const*>(elevations.data()));
#if OUTPUT_HEIGHTMAP
		outputImage(*elevation, DXGI_FORMAT_R32_FLOAT, "ele");
#endif
	}

	if (standardDeviation)
	{
		auto humditys = m_pcg->generateHumidity(*this, resolution, resolution, m_radius / 100.0f, 200.f / m_radius,
			ImageProcess {}, true,	reinterpret_cast<RcSampler* const*>(elevations.data()));

		m_pcg->generateDeviationWorldMap(*this, *standardDeviation, reinterpret_cast<RcSampler* const*>(humditys.data()));
#if OUTPUT_HEIGHTMAP
		outputImage(*standardDeviation, DXGI_FORMAT_R8_UNORM, "std");
#endif
		for (const auto target : humditys)
		{
			if (target) Root::instance()->getRenderSystem()->destoryRcComputeTarget(target);
		}
	}

	for (const auto target : elevations)
	{
		if (target)	Root::instance()->getRenderSystem()->destoryRcComputeTarget(target);
	}
}

void SphericalTerrain::computeElevationGPU()
{
	if (!updateStampTerrain()) return;

	m_pcg->generateElevation(*this);
}

void SphericalTerrain::computeTemperatureGPU()
{
	m_pcg->generateTemperature(*this);
#ifdef ECHO_EDITOR
	updateSeaSurfaceTemperature();
#endif
}

void SphericalTerrain::computeHumidityGPU()
{
	m_pcg->generateHumidity(*this);
}

void SphericalTerrain::computeMaterialIndexGPU()
{
	if (m_editingTerrain && m_terrainData->lookupTex.isNull())
	{
		auto [lutTex, lutMap] =
			PlanetManager::generateBiomeLookupMap(m_terrainData->distribution,
				m_terrainData->composition,
				true);
		m_terrainData->lookupTex = lutTex;
		lutMap.freeMemory();
	}

	m_pcg->generateMaterialIndex(*this);
}

void SphericalTerrain::markSphericalVoronoiRegionId(int id)
{
	if (id < 0) id = (int)m_MLSphericalVoronoiParams.x;
	const auto& region = m_terrainData->sphericalVoronoiRegion;
	m_adjacent_coarse_regions.clear();
	m_adjacent_fine_regions.clear();
	m_MLSphericalVoronoiParams = Vector4(-1.0f);
	if (region.Loaded)
	{
		m_MLSphericalVoronoiParams.x = (id < 0 || id >= region.defineArray.size()) ? -1.0f : (float)id;
		if (m_MLSphericalVoronoiParams.x >= 0.0f)
		{
			m_MLSphericalVoronoiParams.y = (float)region.defineArray[id].Parent;
			const auto& adjacents = region.mData->mData.mAdjacents;
			m_adjacent_fine_regions = adjacents[id];
			m_adjacent_coarse_regions = region.coarse_adjacent_regions[region.defineArray[id].Parent];
			m_MLSphericalVoronoiParams.z = bDisplayCoarseLayer ? (float)m_adjacent_coarse_regions.size() : (float)m_adjacent_fine_regions.size();
			m_MLSphericalVoronoiParams.w = bDisplayCoarseLayer ? 1.0f : -1.0f;
		}
	}

}

void SphericalTerrain::updateSphericalVoronoiRegionInternal()
{
	auto& region = m_terrainData->sphericalVoronoiRegion;
	if (!region.Loaded) return;
	const auto& maps = region.mData->mData.mDataMaps;
	if (m_editingTerrain)
	{
		CreatePlanetRegionTexture(maps, m_LoadedSphericalVoronoiTex);

		markSphericalVoronoiRegionId(0);
	}
}

void SphericalTerrain::markSphericalVoronoiRegionCenter(bool IsEnable)
{
	bDisplayCenter = IsEnable;
	if (!bDisplayCenter) return;
	const auto& region = m_terrainData->sphericalVoronoiRegion;
	if (region.Loaded && (currentFineMap != region.prefix || currentCoarseMap != region.coarse_prefix))
	{
		currentFineMap = region.prefix;
		currentCoarseMap = region.coarse_prefix;

		const auto& mData = region.mData->mData;
		const auto& maps = mData.mDataMaps;
		std::array<Bitmap, 6> temp, coarse_temp;
		for (int i = 0; i < (int)mData.Params.z; ++i)
		{
			maps[i].copyTo(temp[i]);
			maps[i].copyTo(coarse_temp[i]);
		}
		// mark the centers
		{
			const auto& centers = mData.mPoints;
			for (int i = 0; i < (int)centers.size(); ++i)
			{
				const Vector3& center = centers[i];
				int face = 0;
				Vector2 uv;
				if (PointToPlane(center, face, uv, false))
				{
					temp[face].getVal<uint8>(uv) = i == 255 ? (uint8)254 : (uint8)255;
				}
			}
		
			if (!region.coarse_centers.empty())
			{
				for (const Vector3& center : region.coarse_centers)
				{
					int face = 0;
					Vector2 uv;
					if (PointToPlane(center, face, uv, false))
					{
						coarse_temp[face].getVal<uint8>(uv) = (uint8)255;
					}
				}
			}
		}
		for (int i = 0; i < 6; ++i)
		{
			if (!temp[i].pixels)
			{
				m_sphericalVoronoiTexturesWC[i].setNull();
				m_sphericalVoronoiCoarseTexturesWC[i].setNull();
			}
			else
			{
				auto& texture = m_sphericalVoronoiTexturesWC[i];
				texture = TextureManager::instance()->createManual(
					TEX_TYPE_2D,
					temp[i].width,
					temp[i].height,
					ECHO_FMT_R8UI,
					(void*)temp[i].pixels,
					(uint32)temp[i].size());
				texture->load();

				auto& coarse_texture = m_sphericalVoronoiCoarseTexturesWC[i];
				coarse_texture = TextureManager::instance()->createManual(
					TEX_TYPE_2D,
					coarse_temp[i].width,
					coarse_temp[i].height,
					ECHO_FMT_R8UI,
					(void*)coarse_temp[i].pixels,
					(uint32)coarse_temp[i].size());
				coarse_texture->load();
			}

			temp[i].freeMemory();
			coarse_temp[i].freeMemory();
		}
		

	}

}
std::vector<int> SphericalTerrain::GetHierarchicalSphericalVoronoiParents() const
{
	const int n = m_terrainData->sphericalVoronoiRegion.mData->mData.Params.Total();
	const auto& defineArray = m_terrainData->sphericalVoronoiRegion.defineArray;
	std::vector<int> temp(n, -1);
	assert(temp.size() == defineArray.size());
	for (int i = 0; i < (int)temp.size(); ++i)
	{
		temp[i] = defineArray[i].Parent;
	}
	return temp;
}

void SphericalTerrain::computeVertexGpu(RcBuffer* vb, const Node& node)
{
	if (!updateStampTerrain()) return;

	m_pcg->generateVertices(*this, node, vb);
}
#endif

void SphericalTerrain::SurfaceAreaInfo::prepare(const float Area, const std::string& prefix)
{
	if (match(Area, prefix)) return;
	mPrepared = false;
	mArea = Area;
	mPrefix = prefix;
	mSurfaceArea.clear();

	PlanetRegionParameter Params;
	if (PlanetRegionRelated::Parse(prefix, Params))
	{
		std::string path = prefix + ".num";
		int num = Params.x * Params.y * Params.z;
		if(auto* data = Root::instance()->GetFileDataStream(path.c_str()))
		{
			if(data->size() == num * sizeof(int))
			{
				mSurfaceArea.resize(num);
				std::vector<int> mPixels(num);
				data->read(mPixels.data(), data->size());
				int sum = 0;
				for (int n : mPixels)
				{
					sum += n;
				}
				if ((Params.z == 1u && sum == TotalTorus)
					||(Params.z == 6u && sum == TotalSphere))
				{
					mPrepared = true;
					for (int id = 0; id < num; ++id)
					{
						mSurfaceArea[id] = static_cast<float>(mPixels[id]) / static_cast<float>(sum) * Area;
					}
				}
			}
			data->close();
		}
	}
}

float SphericalTerrain::getSurfaceArea() const
{
	assert(m_bCreateFinish);
	switch (m_topology)
	{
	case PTS_Sphere:
	{
		float Radius = m_radius;
		if (m_pTerrainGenerator)
		{
			float Ratio = m_pTerrainGenerator->getSqrtAreaRatio();
			Radius *= Ratio;
		}
		return Radius * Radius * unitSphereArea;
	}
		break;
	case PTS_Torus:
	{
		float Radius = Math::PI * m_radius;
		return 4.0f * Radius * Radius * m_relativeMinorRadius;
	}
		break;
	}
	
	return std::numeric_limits<float>::quiet_NaN();
}

float SphericalTerrain::getSurfaceArea(int id)
{
	assert(m_bCreateFinish);
	float Area = getSurfaceArea();
	if (id == -1) return Area;
	if(m_terrainData->sphericalVoronoiRegion.Loaded && !std::isnan(Area))
	{
		int num = (int)m_terrainData->sphericalVoronoiRegion.defineArray.size();
		if (id >= 0 && id < num)
		{
			mSaInfo.prepare(Area, m_terrainData->sphericalVoronoiRegion.prefix);
			if(mSaInfo.mPrepared)
			{
				return mSaInfo.mSurfaceArea[id];
			}
		}
	}
	return std::numeric_limits<float>::quiet_NaN();
}

float SphericalTerrain::getCoarseSurfaceArea(unsigned int id)
{
	if (m_terrainData->sphericalVoronoiRegion.Loaded)
	{
		unsigned int num = (unsigned int)m_terrainData->sphericalVoronoiRegion.defineBaseArray.size();
		if (id < num)
		{
			const auto& defineArray = m_terrainData->sphericalVoronoiRegion.defineArray;
			int n = (int)defineArray.size();
			float Area = 0.0f;
			for (int i = 0; i < n; ++i)
			{
				if (defineArray[i].Parent == id)
				{
					float area = getSurfaceArea(i);
					if (std::isnan(area)) return area;
					Area += area;
				}
			}
			return Area;
		}
	}
	return std::numeric_limits<float>::quiet_NaN();
}

std::vector<int> SphericalTerrain::GetCoarseLayerAdjacentRegions(int id) const
{
	if (!m_terrainData->sphericalVoronoiRegion.Loaded)
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetCoarseLayerAdjacentRegions: The Hierarchical Spherical Voronoi Region is not well loaded!", LML_CRITICAL);
		return std::vector<int>();
	}
	const int n = (int)m_terrainData->sphericalVoronoiRegion.coarse_adjacent_regions.size();
	if (id < 0 || id >= n)
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetCoarseLayerAdjacentRegions: Invalid coarse region index!", LML_CRITICAL);
		return std::vector<int>();
	}
	return m_terrainData->sphericalVoronoiRegion.coarse_adjacent_regions[id];
}

std::vector<int> SphericalTerrain::GetFineLayerAdjacentRegions(int id) const
{
	if (!m_terrainData->sphericalVoronoiRegion.Loaded || m_terrainData->sphericalVoronoiRegion.mData.isNull() || !m_terrainData->sphericalVoronoiRegion.mData->mData.mbAdjacents)
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetFineLayerAdjacentRegions: The Hierarchical Spherical Voronoi Region is not well loaded!", LML_CRITICAL);
		return std::vector<int>();
	}
	
	const auto& adjacents = m_terrainData->sphericalVoronoiRegion.mData->mData.mAdjacents;
	const int n = (int)adjacents.size();
	if (id < 0 || id >= n)
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetFineLayerAdjacentRegions: Invalid fine region index!", LML_CRITICAL);
		return std::vector<int>();
	}
	return adjacents[id];
}

std::vector<Vector3> SphericalTerrain::GetCoarseLayerCenters(bool ToWorldSpace) const
{
	static const std::vector<Vector3> CoarseLayerCenters;
	if (!m_terrainData->sphericalVoronoiRegion.Loaded)
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetCoarseLayerCenters: The Hierarchical Spherical Voronoi Region is not well loaded!", LML_CRITICAL);
		return CoarseLayerCenters;
	}
	const int n = m_terrainData->sphericalVoronoiRegion.mData->mData.Params.x;
	const auto& centers = m_terrainData->sphericalVoronoiRegion.coarse_centers;
	if (n == 1 || centers.empty())
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetCoarseLayerCenters: This may happens when the finer layer has only 6 regions then the whole sphere becomes the coarser layer, or when the finer layer center file ["
			+ Root::instance()->getRootResourcePath() + m_terrainData->sphericalVoronoiRegion.prefix + ".bin] is not loaded successfully!", LML_CRITICAL);
		return CoarseLayerCenters;
	}

	if(ToWorldSpace)
	{
		const auto& InCenters = centers;
		std::vector<Vector3> OutCenters;
		for (const auto& c : InCenters)
		{
#ifdef TOLOCAL
			OutCenters.push_back(PointToLocalSpace(c));
#else
			OutCenters.push_back(PointToWorldSpace(c));
#endif
		}
		return OutCenters;
	}

	return centers;
}

std::vector<Vector3> SphericalTerrain::GetFineLayerCenters(bool ToWorldSpace) const
{
	static const std::vector<Vector3> FineLayerCenters;
	if (!m_terrainData->sphericalVoronoiRegion.Loaded || m_terrainData->sphericalVoronoiRegion.mData.isNull())
	{
		LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetFineLayerCenters: The Hierarchical Spherical Voronoi Region is not well loaded!", LML_CRITICAL);
		return FineLayerCenters;
	}
	const auto& mData = m_terrainData->sphericalVoronoiRegion.mData->mData;
	if (mData.mbPoints)
	{
		if(ToWorldSpace)
		{
			const auto& InCenters = mData.mPoints;
			std::vector<Vector3> OutCenters;
			for (const auto& c : InCenters)
			{
#ifdef TOLOCAL
				OutCenters.push_back(PointToLocalSpace(c));
#else
				OutCenters.push_back(PointToWorldSpace(c));
#endif
			}
			return OutCenters;
		}
		return mData.mPoints;
	}
	LogManager::instance()->logMessage("-warning-\t SphericalTerrain::GetFineLayerCenters: Check if the centers file [" + m_terrainData->sphericalVoronoiRegion.prefix + ".bin] exists in Folder ["
		+ Root::instance()->getRootResourcePath() + PlanetRegionRelated::SpherePlanetRegionPath + "]!", LML_CRITICAL);
	return FineLayerCenters;
}

TexturePtr SphericalTerrain::SelectPlanetRegionRef(const int slice) const
{
	int id = isTorus() ? 0 : slice;
#ifdef ECHO_EDITOR
	if (m_displayMode == BiomeDisplayMode::PlanetRegionIndex)
	{
		return bDisplayCenter ? (bDisplayCoarseLayer ? m_sphericalVoronoiCoarseTexturesWC[id] : m_sphericalVoronoiTexturesWC[id]) : m_LoadedSphericalVoronoiTex[id];
	}
	if (m_displayMode == BiomeDisplayMode::ManualPlanetRegion)
		return m_ManualSphericalVoronoiTex[id];
#endif
	if (Root::instance()->getRuntimeMode() == Root::eRM_Client && m_showSphericalVoronoiRegion)
	{
		return m_LoadedSphericalVoronoiTex[id];
	}
	return TexturePtr();
}

TexturePtr SphericalTerrain::SelectGeometryHeatmapRef(const int slice) const
{
    int id = isTorus() ? 0 : slice;
    if (m_displayMode != BiomeDisplayMode::GeometryHeatmap)
        return TexturePtr();

    // lazy load per face, cache in m_GeometryHeatmapTex
    if (!m_GeometryHeatmapTex[id].isNull())
        return m_GeometryHeatmapTex[id];

    std::string base = m_terrainData->getName().toString();
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    static const char* faceName[6] = { "posx","negx","posy","negy","posz","negz" };
    std::string heatKey = base + ".heatmap_" + faceName[id];
    // Prefer explicit file path with extension to avoid missing resource mapping
    std::string heatPath = heatKey + ".png";

    TexturePtr tex = TextureManager::instance()->getByName(Name(heatPath));
    if (tex.isNull())
    {
        // Try direct load from file path under resource root
        try { tex = TextureManager::instance()->load(Name(heatPath)); }
        catch (...) { tex = TexturePtr(); }
    }
    else if (!tex->isLoaded())
    {
        try { tex->load(); } catch (...) {}
    }
    m_GeometryHeatmapTex[id] = tex;
    return tex;
}

Vector4 SphericalTerrain::getPlanetRegionParameters() const
{
	const auto& offset = m_terrainData->sphericalVoronoiRegion.offset;
#ifdef ECHO_EDITOR
	if (m_displayMode == BiomeDisplayMode::ManualPlanetRegion) return m_ManualParams;
	if (m_displayMode != BiomeDisplayMode::PlanetRegionIndex) return Vector4 { 0, 0, offset.x, offset.y };
#else
	if (Root::instance()->getRuntimeMode() == Root::eRM_Client && !m_showSphericalVoronoiRegion)
	{
		return Vector4 { 0, 0, offset.x, offset.y };
	}
#endif // ECHO_EDITOR
	const auto data = m_terrainData->sphericalVoronoiRegion.mData;
	if (!data.isNull() && data->mData.Params.isParsed)
	{
		const auto& Params = data->mData.Params;
		return Vector4{ static_cast<float>(Params.x), static_cast<float>(Params.y), offset.x, offset.y };
	}
	return Vector4{};
}

bool SphericalTerrain::CreatePlanetRegionTexture(const std::array<Bitmap, 6>& InMaps, std::array<TexturePtr, 6>& OutTextures)
{
	for (int i = 0; i < 6; ++i)
	{
		const auto& map = InMaps[i];
		auto& texture = OutTextures[i];
		texture.setNull();
		if (map.pixels && map.bytesPerPixel == 1)
		{
			texture = TextureManager::instance()->createManual(
				TEX_TYPE_2D,
				map.width,
				map.height,
				ECHO_FMT_R8UI,
				(void*)map.pixels,
				(uint32)map.size());
			texture->load();
		}
	}
	return true;
}

void SphericalTerrain::displaySphericalVoronoiRegion(bool mode)
{
	if (!m_useSphericalVoronoiRegion || !m_terrainData->sphericalVoronoiRegion.Loaded )
		return;
	m_showSphericalVoronoiRegion = mode;
	if ( mode )
	{
		if (m_LoadedSphericalVoronoiTex[0].isNull())
		{
			CreatePlanetRegionTexture(m_terrainData->sphericalVoronoiRegion.mData->mData.mDataMaps, m_LoadedSphericalVoronoiTex);
		}
	}
}


int SphericalTerrain::computeSphericalRegionParentId(const Vector3& localcoord, const std::array<Bitmap, 6>& maps)
{
	Node::Face face;
	Vector2 uv;
	if (PointToCube(localcoord, face, uv))
	{
		return maps[face].getVal<uint8>(uv);
	}
	return -1;
}

bool SphericalTerrain::PointToCube(const Vector3& InPoint, int& face, Vector2& uv)
{
	Vector3 pos = InPoint;
	float sqLength = Math::Sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
	if (sqLength < 1e-6)
	{
		return false;
	}
#if 0
	float invSqLength = 1.0f / sqLength;
	pos *= invSqLength;
#endif
	Node::MapToCube(pos, face, uv);
	uv = uv * 0.5 + 0.5;
	uv.x = Math::lerp(0.5f / BiomeTextureResolution, 1.0f - 0.5f / BiomeTextureResolution, uv.x);
	uv.y = Math::lerp(0.5f / BiomeTextureResolution, 1.0f - 0.5f / BiomeTextureResolution, uv.y);
	return true;
}

bool SphericalTerrain::PointToPlane(const Vector3& InPoint, int& face, Vector2& uv, bool isL) const
{
	Vector3 pos = InPoint;
	if (isSphere())
	{
		return PointToCube(InPoint, face, uv);
	}
	else if (isTorus())
	{
		face = 0;
		uv = isL? (TorusTerrainNode::MapToPlane(pos) + m_terrainData->sphericalVoronoiRegion.offset) : Vector2(pos.x, pos.y);
		uv = Fract(uv);
		return true;
		
	}
	return false;
}

Vector3 SphericalTerrain::PointToLocalSpace(const Vector3& InPoint) const
{
	Vector3 pos = Vector3::ZERO;
	if (isSphere())
	{
		pos = InPoint.normalisedCopy();
	}
	else if (isTorus())
	{
		Vector2 uv = Vector2(InPoint.x, InPoint.y);
		uv -= m_terrainData->sphericalVoronoiRegion.offset;
		uv += 1.0f;
		uv = Fract(uv);
		uv *= Math::TWO_PI;
		pos = TorusTerrainNode::GetTorusPosition(m_relativeMinorRadius, uv.x, uv.y);
	}
	return getSurfaceFinestLs(pos);
}

Vector3 SphericalTerrain::PointToWorldSpace(const Vector3& InPoint) const
{
	Vector3 pos = Vector3::ZERO;
	if (isSphere())
	{
		pos = InPoint.normalisedCopy();
	}
	else if (isTorus())
	{
		Vector2 uv = Vector2(InPoint.x, InPoint.y);
		uv -= m_terrainData->sphericalVoronoiRegion.offset;
		uv += 1.0f;
		uv = Fract(uv);
		uv *= Math::TWO_PI;
		pos = TorusTerrainNode::GetTorusPosition(m_relativeMinorRadius, uv.x, uv.y);
	}
	return ToWorldSpace(getSurfaceFinestLs(pos));
}


int SphericalTerrain::computeSphericalRegionId(const Vector3& worldcoord) const
{
	if (m_terrainData->region.Loaded)
	{
		Vector3 pos = ToLocalSpace(worldcoord);
		// pos.normalize
		float sqLength = Math::Sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
		if (sqLength < 1e-6) return -1;
		float invSqLength = 1.0f / sqLength;
		pos *= invSqLength;
		Node::Face face;
		Vector2 uv;
		Node::MapToCube(pos, face, uv);
		uv = uv * 0.5 + 0.5;
		uv.x = Math::lerp(0.5f / BiomeTextureResolution, 1.0f - 0.5f / BiomeTextureResolution, uv.x);
		uv.y = Math::lerp(0.5f / BiomeTextureResolution, 1.0f - 0.5f / BiomeTextureResolution, uv.y);

		const auto& maps = m_terrainData->region.RegionMaps;
		const auto& map = maps[face];
		assert(map.bytesPerPixel == 1);
		Vector2 coord = uv * static_cast<float>(map.width);
		int x = static_cast<int>(std::floorf(coord.x));
		int y = static_cast<int>(std::floorf(coord.y));
		int id = x + y * map.width;
		if (id < map.size())
		{
			return ((uint8_t*)map.pixels)[id];
		}
	}
	return -1;
}

int SphericalTerrain::computePlanetRegionId(const Vector3& worldpos) const
{
	static String warning = String("-warning- \t") + __FUNCTION__ + String(": ");
	if (m_terrainData->sphericalVoronoiRegion.Loaded)
	{
		Vector3 pos = ToLocalSpace(worldpos);
		Node::Face face;
		Vector2 uv = Vector2::ZERO;
		if (PointToPlane(pos, face, uv, true))
		{
			const auto& maps = m_terrainData->sphericalVoronoiRegion.mData->mData.mDataMaps;
			auto& map = maps[face];
			return map.getVal<uint8>(uv);
		}
		LogManager::instance()->logMessage(warning + "failed. {" + m_terrainData->filePath + "}");
	}
	else
	{
		LogManager::instance()->logMessage(warning + "{" + m_terrainData->filePath + "} isn't loaded.");
	}

	return -1;
}

int SphericalTerrain::computeRegionInformation(const Vector3& worldcoord) const
{
	return m_useSphericalVoronoiRegion ? computePlanetRegionId(worldcoord) : computeSphericalRegionId(worldcoord);
}

RcBuffer* SphericalTerrain::createRawVb()
{
	return Root::instance()->getRenderSystem()->createDefaultRB(nullptr, VB_BYTE_SIZE);
}

RcBuffer* SphericalTerrain::createDynamicVb(const std::vector<TerrainVertex>& vertices)
{
	return Root::instance()->getRenderSystem()->createDynamicVB(vertices.data(), VB_BYTE_SIZE);
}

void SphericalTerrain::computeVertexCpu(Node& node)
{
	std::vector<Vector3> baseShape;
	switch (m_topology)
	{
	case PTS_Sphere:
	{
		baseShape = Node::getSphericalPositions(
			node.m_slice, EXTRA_GRID_RESOLUTION, EXTRA_GRID_RESOLUTION,
			node.m_gridSize, node.m_offset - node.m_gridSize, m_pTerrainGenerator);
	}
	break;
	case PTS_Torus:
	{
		baseShape = TorusTerrainNode::getTorusPositions(
			EXTRA_GRID_RESOLUTION, EXTRA_GRID_RESOLUTION, m_relativeMinorRadius,
			node.m_gridSize, node.m_offset - node.m_gridSize);
	}
	break;
	}

	std::vector<float> elePlus(EXTRA_VERTEX_COUNT);
	generateElevations(baseShape.data(), elePlus.data(), EXTRA_VERTEX_COUNT);
	const auto invTrans = -node.getTranslationLocal();
	std::transform(elePlus.begin(), elePlus.end(), elePlus.begin(),
		[this](const float r) { return r * m_elevationRatio; });

	std::vector<Vector3> posPlus(EXTRA_VERTEX_COUNT);
	applyElevations(posPlus.data(), baseShape.data(), elePlus.data(), EXTRA_VERTEX_COUNT);

	computeStaticModify(getNodeIndex(node), posPlus.data(), baseShape.data(), elePlus.data(), EXTRA_VERTEX_COUNT);

	if (isSphere())
		m_modifier->Evaluate(node.m_modifierInstances, posPlus, baseShape, elePlus);

	auto elevations = std::vector(VERTEX_COUNT, 0.0f);
	for (int i = 1; i < EXTRA_GRID_RESOLUTION - 1; ++i)
	{
		const int srcOffset = i * EXTRA_GRID_RESOLUTION + 1;
		const int dstOffset = (i - 1) * GRID_RESOLUTION;
		std::memcpy(elevations.data() + dstOffset, elePlus.data() + srcOffset, GRID_RESOLUTION * sizeof(float));
	}

	auto rot = node.getRotationLocal();
	AxisAlignedBox obb = AxisAlignedBox::BOX_NULL;
	AxisAlignedBox aabb = AxisAlignedBox::BOX_NULL;

	std::vector<Vector3> pos(VERTEX_COUNT);
	for (int i = 0; i < GRID_RESOLUTION; ++i)
	{
		for (int j = 0; j < GRID_RESOLUTION; ++j)
		{
			const int idx   = i * GRID_RESOLUTION + j;
			const auto curr = posPlus[(i + 1) * EXTRA_GRID_RESOLUTION + j + 1];
			pos[idx]        = curr + invTrans;
			if (m_useObb)
			{
				auto rotatedCurr = curr;
				rotatedCurr.TransformCoord(rot);
				obb.merge(rotatedCurr);
			}
			aabb.merge(curr);
		}
	}

	auto nor = computeNormal(GRID_RESOLUTION, GRID_RESOLUTION, posPlus);

	node.m_vertices = assembleVertices(pos, nor, node.getBiomeUv(BiomeTextureResolution));
	{
		std::unique_lock lock(node.m_elevationMutex);
		node.m_elevations.swap(elevations);
	}

	OrientedBox box;
	if (m_useObb)
	{
		auto center = obb.getCenter();
		rot = rot.Transpose();
		Quaternion orientation;
		orientation.FromRotationMatrix(rot);
		center.TransformCoord(rot);
		box = OrientedBox(center, obb.getHalfSize(), orientation);
	}
	{
		std::unique_lock lock(node.m_boundMutex);
		node.m_bound.merge(aabb);
		if (m_useObb) node.m_orientedBound = box;
	}
	auto parent = node.m_parent;
	while (parent)
	{
		{
			std::unique_lock lock(parent->m_boundMutex);
			parent->m_bound.merge(aabb);
		}
		parent = parent->m_parent;
	}
}

std::vector<Vector3> SphericalTerrain::computeNormal(const size_t width, const size_t height, const std::vector<Vector3>& positionWithRing)
{
	assert(positionWithRing.size() == (width + 2) * (height + 2));
	auto nor = std::vector(width * height, Vector3(0.0f));

	for (size_t i = 0; i < height; ++i)
	{
		for (size_t j = 0; j < width; ++j)
		{
			const size_t idx = i * width + j;

			const auto curr    = positionWithRing[(i + 1) * (width + 2) + j + 1];
			const auto vTop    = positionWithRing[i * (width + 2) + j + 1] - curr;
			const auto vLeft   = positionWithRing[(i + 1) * (width + 2) + j] - curr;
			const auto vRight  = positionWithRing[(i + 1) * (width + 2) + j + 2] - curr;
			const auto vBottom = positionWithRing[(i + 2) * (width + 2) + j + 1] - curr;

			nor[idx] = Vector3::ZERO;
			if (false)
			{
				nor[idx] += vLeft.crossProduct(vTop);
				nor[idx] += vBottom.crossProduct(vLeft);
				nor[idx] += vRight.crossProduct(vBottom);
				nor[idx] += vTop.crossProduct(vRight);
			}
			else
			{
				const auto vTopLft = positionWithRing[i * (width + 2) + j] - curr;
				const auto vTopRgt = positionWithRing[i * (width + 2) + j + 2] - curr;
				const auto vBtmLft = positionWithRing[(i + 2) * (width + 2) + j] - curr;
				const auto vBtmRgt = positionWithRing[(i + 2) * (width + 2) + j + 2] - curr;

				nor[idx] += vTopLft.crossProduct(vTop);
				nor[idx] += vTop.crossProduct(vTopRgt);
				nor[idx] += vTopRgt.crossProduct(vRight);
				nor[idx] += vRight.crossProduct(vBtmRgt);
				nor[idx] += vBtmRgt.crossProduct(vBottom);
				nor[idx] += vBottom.crossProduct(vBtmLft);
				nor[idx] += vBtmLft.crossProduct(vLeft);
				nor[idx] += vLeft.crossProduct(vTopLft);
			}

			nor[idx].normalise();
		}
	}
	return nor;
}

std::vector<TerrainVertex> SphericalTerrain::assembleVertices(const std::vector<Vector3>& pos, const std::vector<Vector3>& nor, const std::vector<Vector2>& uv)
{
	std::vector<TerrainVertex> vertices;
	vertices.reserve(pos.size());
	for (size_t i = 0; i < pos.size(); ++i)
	{
		TerrainVertex v;
		v.pos    = pos[i];
		v.normal = nor[i];
		v.uv     = uv[i];
		vertices.push_back(v);
	}
	return vertices;
}

std::vector<TerrainVertex> SphericalTerrain::copyVertexBufferFromGpu(RcBuffer* vb)
{
	auto* system = Root::instance()->getRenderSystem();
	RcReadBuffer vbReadBuffer;
	std::vector<TerrainVertex> vertices;
	vertices.resize(VERTEX_COUNT);
	system->copyBufferContentsToMemory(vb, vbReadBuffer);
	assert(vbReadBuffer.pData);
	std::memcpy(vertices.data(), vbReadBuffer.pData, VB_BYTE_SIZE);
	std::free(vbReadBuffer.pData);
	return vertices;
}

void SphericalTerrain::addModifier(const uint64_t id, float width, float height,
								   const DVector3& position, const float yaw)
{
	assert(isBoundAccurate());

	const auto axis = getUpVectorWs(position);
	Quaternion rotation;
	rotation.FromAxesToAxes(Vector3::UNIT_Y, axis);
	Quaternion rotYaw;
	rotYaw.RotationY(yaw);
	rotation = rotation * rotYaw;

	DMatrix4 world(rotation);
	world.setTrans(position);
	const DMatrix4 toRoot = m_worldMatInv * world;

	const float elevation = getSurfaceLs(ToLocalSpace(position)).length() - 1.0f;
	width = std::clamp(width, 10.0f, TerrainModify3D::MaxRange);
	height = std::clamp(height, 10.0f, TerrainModify3D::MaxRange);
	m_modifier->AddInstance(id, width, height, elevation, world, toRoot);

	addModifierToNodes(id);
}

void SphericalTerrain::addModifier(const uint64_t id, float radius, const DVector3& position)
{
	assert(isBoundAccurate());

	const auto axis = getUpVectorWs(position);
	Quaternion rotation;
	rotation.FromAxesToAxes(Vector3::UNIT_Y, axis);

	DMatrix4 world(rotation);
	world.setTrans(position);
	const DMatrix4 toRoot = m_worldMatInv * world;

	const float elevation = getSurfaceLs(ToLocalSpace(position)).length() - 1.0f;
	radius = std::clamp(radius, 10.0f, TerrainModify3D::MaxRange);
	m_modifier->AddInstance(id, radius, elevation, world, toRoot);

	addModifierToNodes(id);
}

void SphericalTerrain::removeModifier(uint64_t id)
{
	m_modifier->RemoveInstance(id);

	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [id](Node* node)
		{
			if (node->m_modifierInstances.contains(id))
			{
				node->m_modifierInstances.erase(id);
				node->m_vbDirty = true;
				return true;
			}
			return false;
		});
	}
}

void SphericalTerrain::addModifierToNodes(uint64_t id)
{
	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [id, this](Node* node)
		{
			if (node->m_modifierInstances.contains(id)) return true;

			if (m_modifier->TestIntersection(getIndexBound(getNodeIndex(*node)), id))
			{
				node->m_modifierInstances.emplace(id);
				node->m_vbDirty = true;
				return true;
			}
			return false;
		});
	}
}

bool SphericalTerrain::testModifierIntersect(const uint64_t id, const int node)
{
	return m_modifier->TestIntersection(getIndexBound(node), id);
}

bool SphericalTerrain::containsModifier(const uint64_t id) const
{
	return m_modifier->ContainsInstance(id);
}

void SphericalTerrain::subdivideNode(Node& node)
{
	cancelGenerateVertexTask(node);

	if (!node.hasChildren())
	{
		node.createChildren(nullptr);

		for (auto&& child : node.m_children)
		{
			child->init();

			if (isBoundAccurate())
			{
				child->setBound(getIndexBound(getNodeIndex(*child)));
			}
			else
			{
				child->computeApproximateBound(m_elevationRatio);
			}
			child->m_modifierInstances = m_modifier->TestIntersection(
				child->getBoundLocal(), node.m_modifierInstances);
		}
	}

	for (auto&& child : node.m_children)
	{
		generateVertexBuffer(*child);
	}

	// for (int i = 0; i < 4; ++i)
	// {
	// 	const auto edge = static_cast<Edge>(i);
	// 	if (auto* neighbor = node.getNeighbor(edge); neighbor == nullptr)
	// 	{
	// 		// need to subdivide parent's neighbor
	// 		if (Node* parentsNeighbor = node.getParentsNeighbor(edge))
	// 		{
	// 			subdivideNode(*parentsNeighbor);
	// 		}
	// 	}
	// }

	if (std::all_of(node.m_children.begin(), node.m_children.end(),
		[](const auto& c) { return c->m_vertexBuffer; }))
	{
		for (auto&& child : node.m_children)
		{
			child->addNeighbors();
			child->m_active = true;
		}
		node.m_active  = false;
		{
			std::unique_lock lock(node.m_elevationMutex);
			node.m_elevations.clear();
			node.m_elevations.shrink_to_fit();
		}
		node.destroyVertexBuffer();
	}
}

bool SphericalTerrain::canSubdivideNode(const Node& node)
{
	return node.m_depth < m_maxDepth;
}

void SphericalTerrain::unifyNode(Node& node)
{
	if (m_vbGenAsync)
	{
		for (auto&& c : node.m_children)
		{
			cancelGenerateVertexTask(*c);
		}
	}

	generateVertexBuffer(node);

	if (node.m_vertexBuffer)
	{
		node.destroyChildren();
		node.m_active = true;
	}
}

bool SphericalTerrain::canUnifyNode(const Node& node)
{
	if (!node.hasChildren()) return false;

	if (std::any_of(node.m_children.begin(), node.m_children.end(),
		[](const auto& c) { return c->m_active; }))
	{
		return !node.isNeighborsChildSubdivided();
	}
	return false;
}

bool SphericalTerrain::updateStampTerrain()
{
	if (updateStampTerrain_impl())
	{
#ifdef ECHO_EDITOR
		createComputePipeline();
#endif
	}
	return m_gpuMode && (m_editingTerrain || !m_vbGenAsync);
}

bool SphericalTerrain::updateStampTerrain_impl()
{
	bool Pre = m_bStampTerrain;
	if (m_stampterrain3D->IsEnabled(m_gpuMode && (m_editingTerrain || !m_vbGenAsync)))
	{
		if (!m_bStampTerrain)
		{
			m_bStampTerrain = true;
		}
	}
	else if (m_bStampTerrain)
	{
		m_bStampTerrain = false;
	}
	return m_bStampTerrain != Pre;
}



int SphericalTerrain::getSurfaceTypeWs(const DVector3& worldPos, bool* underwater) const
{
	return getSurfaceTypeLs(ToLocalSpace(worldPos), underwater);
}

int SphericalTerrain::getSurfaceTypeLs(const Vector3& localPos, bool* underwater) const
{
#if ECHO_EDITOR
	std::shared_lock lock(m_biomeQueryMutex);
#endif
	if (m_materialIndexMap.size() == 0) return -1;

	uint8_t val = 0u;
	if (isSphere())
	{
		val = m_materialIndexMap.getVal<uint8_t>(localPos);
	}
	else if (isTorus())
	{
		val = m_materialIndexMap.getVal<uint8_t>(TorusTerrainNode::MapToPlane(localPos), 0);
	}
	if (underwater) *underwater = (val & MaterialOceanMask) != 0u;
	return val & MaterialTypeMask;
}

int Echo::SphericalTerrain::getSurfaceType(const Vector2& localPos, int slice, bool* underwater) const
{
#if ECHO_EDITOR
	std::shared_lock lock(m_biomeQueryMutex);
#endif
	if (m_materialIndexMap.size() == 0) return -1;

	uint8_t val = 0u;
	if (isSphere())
	{
		val = m_materialIndexMap.getVal<uint8_t>(localPos * 0.5f + 0.5f ,slice);
	}
	else if (isTorus())
	{
		val = m_materialIndexMap.getVal<uint8_t>(localPos, 0);
	}
	if (underwater) *underwater = (val & MaterialOceanMask) != 0u;
	return val & MaterialTypeMask;
}

ColorValue SphericalTerrain::getSurfaceColor(const uint8_t surfaceType) const
{
	if (m_terrainData->composition.usedBiomeTemplateList.size() <= surfaceType)
	{
		return ColorValue::Black;
	}

	const auto& biome = m_terrainData->composition.usedBiomeTemplateList[surfaceType];
	return biome.getsRGBFinalColor();
}

bool SphericalTerrain::getIsNearbyLs(const Vector3& localPos) const {
	constexpr float range = 0.2f;
	switch (m_topology)
	{
	case PTS_Sphere:
		return localPos.squaredLength() <= Math::Sqr((1.0f + m_elevationMax) * (1 + range));
	case PTS_Torus:
		return (localPos - Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy()).squaredLength() <=
			Math::Sqr(m_relativeMinorRadius * (1 + m_elevationMax) * (1 + range));
	}
	return false;
}

bool SphericalTerrain::getIsNearbyWs(const Vector3& worldPos) const {
	return getIsNearbyLs(ToLocalSpace(worldPos));
}

Vector3 SphericalTerrain::getGravityLs(const Vector3& localPos) const
{
	switch (m_topology)
	{
	case PTS_Sphere:
		if (m_pTerrainGenerator) {
			return m_pTerrainGenerator->getGravity(localPos, 1.0f + m_noiseElevationMin);
		}
		return -localPos.normalisedCopy();
	case PTS_Torus:
		return -(localPos - Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy()).normalisedCopy();
	}
	return Vector3::ZERO;
}

Vector3 SphericalTerrain::getGravityWs(const Vector3& worldPos) const {
	return m_rot * getGravityLs(ToLocalSpace(worldPos));
}

Quaternion SphericalTerrain::getCardinalOrientation(const Vector3& localPos) const
{
	const auto up = -getGravityLs(localPos);

	if (Math::FloatEqual(Math::Abs(up.dotProduct(Vector3::DOWN)), 1, ECHO_EPSILON))
	{
		if (isSphere())
		{
			const auto east  = Vector3::RIGHT;
			const auto south = east.crossProduct(up).normalisedCopy();
			return { east, up, south };
		}
		if (isTorus())
		{
			const auto circle = Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy();
			const auto east   = Vector3(circle.z, 0, -circle.x);
			const auto south  = east.crossProduct(up).normalisedCopy();
			return { east, up, south };
		}
	}

	const auto east  = up.crossProduct(Vector3::DOWN).normalisedCopy();
	const auto south = east.crossProduct(up).normalisedCopy();
	return { east, up, south };
}

Quaternion SphericalTerrain::getCardinalOrientation(const DVector3& worldPos) const
{
	return m_rot * getCardinalOrientation(ToLocalSpace(worldPos));
}

Vector3 SphericalTerrain::getOriginLs(const Vector3& localPos) const
{
	if (isSphere())
	{
		return Vector3::ZERO;
	}
	if (isTorus())
	{
		return Vector3(localPos.x, 0, localPos.z).normalisedCopy();
	}
	return Vector3::ZERO;
}

double SphericalTerrain::getElevation(const DVector3& worldPos) const
{
	return getElevation(ToLocalSpace(worldPos)) * m_realScale.x;
}

float SphericalTerrain::getElevation(const Vector3& localPos) const
{
	if (isSphere())
	{
		return localPos.length() - 1;
	}
	if (isTorus())
	{
		return localPos.distance(getOriginLs(localPos)) - m_relativeMinorRadius;
	}
	return -1;
}

Vector3 SphericalTerrain::getUpVectorLs(const Vector3& localPos, Vector3* basePos) const
{
	if (isSphere())
	{
		auto up = localPos.normalisedCopy();
		if (basePos)
		{
			auto sphere = up;
			if (m_pTerrainGenerator)
			{
				Face face;
				Vector2 xy;
				Node::MapToCube(sphere, face, xy);
				sphere *= m_pTerrainGenerator->getHeight(face, xy.x, xy.y);
			}
			*basePos = sphere;
		}
		return up;
	}
	if (isTorus())
	{
		const auto axis = Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy();
		const auto up   = (localPos - axis).normalisedCopy();
		if (basePos)
		{
			const auto torus = axis + up * m_relativeMinorRadius;
			*basePos = torus;
		}
		return up;
	}

	return localPos;
}

Vector3 SphericalTerrain::getUpVectorLs(const Vector2& xy, const int slice, Vector3* basePos) const
{
	if (isSphere())
	{
		auto up = Node::MapToSphere(slice, xy);
		if (basePos)
		{
			auto sphere = up;
			if (m_pTerrainGenerator)
			{
				sphere *= m_pTerrainGenerator->getHeight(slice, xy.x, xy.y);
			}
			*basePos = sphere;
		}
		return up;
	}
	if (isTorus())
	{
		const auto localPos = TorusTerrainNode::GetTorusPosition(
			m_relativeMinorRadius, xy.x * Math::TWO_PI, xy.y * Math::TWO_PI);

		const auto axis = Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy();
		const auto up   = (localPos - axis).normalisedCopy();
		if (basePos)
		{
			*basePos = localPos;
		}
		return up;
	}
	return Vector3::ZERO;
}

Vector3 SphericalTerrain::getUpVectorWs(const DVector3& worldPos) const
{
	return m_rot * getUpVectorLs(ToLocalSpace(worldPos));
}

//demo
//Vector3 Echo::SphericalTerrain::getGroundPos(const DVector3& worldPos)
//{
//	Vector3 localPos = ToLocalSpace(worldPos);
//	if (isSphere())
//	{
//		auto up = localPos.normalisedCopy();
//		auto sphere = up;
//		if (m_pTerrainGenerator)
//		{
//			Face face;
//			Vector2 xy;
//			Node::MapToCube(sphere, face, xy);
//			float height = m_pTerrainGenerator->getHeight(face, xy.x, xy.y) * m_radius;
//			return height * up + m_pos;
//		}
//	}
//	if (isTorus())
//	{
//		const auto axis = Vector3(localPos.x, 0.0f, localPos.z).normalisedCopy();
//		const auto up = (localPos - axis).normalisedCopy();
//		
//		const auto torus = (axis + up * m_relativeMinorRadius) * m_radius;
//			
//		return m_pos + torus;
//	}
//	return Vector3::ZERO;
//}

DVector3 SphericalTerrain::getSurfaceWs(const DVector3& worldPos, Vector3* faceNormal) const
{
	auto ret = ToWorldSpace(getSurfaceLs(ToLocalSpace(worldPos), faceNormal));
	if (faceNormal) *faceNormal = m_rot * *faceNormal;
	return ret;
}

Vector3 SphericalTerrain::getSurfaceLs(const Vector3& localPos, Vector3* faceNormal, int* depth) const
{
	auto pos         = localPos;
	const auto lclUp = getUpVectorLs(pos, &pos);
	const Ray ray(pos, -lclUp);

	int slice = 0;
	Vector2 xy;
	if (isSphere())
	{
		Node::MapToCube(pos, slice, xy);
	}
	else if (isTorus())
	{
		xy = TorusTerrainNode::MapToPlane(pos);
	}

	const auto triangle = getSurfaceTriangleLs(xy, slice, depth);
	const auto plane    = Plane(triangle[0], triangle[1], triangle[2]);
	if (faceNormal) *faceNormal = plane.normal;
	auto [intersects, t] = ray.intersects(plane);
	pos                  = ray.getPoint(t);
	return pos;
}

DVector3 SphericalTerrain::getSurfaceWs(const Vector2& xy, const int slice, Vector3* faceNormal) const
{
	Vector3 posLocal;
	const auto upLocal = getUpVectorLs(xy, slice, &posLocal);
	const Ray ray(posLocal, -upLocal);
	const auto triangle = getSurfaceTriangleLs(xy, slice);
	const auto plane = Plane(triangle[0], triangle[1], triangle[2]);
	if (faceNormal) *faceNormal = m_rot * plane.normal;
	auto [intersects, t] = ray.intersects(plane);
	posLocal = ray.getPoint(t);
	return ToWorldSpace(posLocal);
}

std::array<Vector3, 3> SphericalTerrain::getSurfaceTriangleLs(const Vector2& xy, int slice, int* depth) const
{
	std::array triangle { Vector3::ZERO, Vector3::ZERO, Vector3::ZERO };

	if (isTorus())
	{
		const Vector2 sliceXy = xy * Vector2(static_cast<float>(m_numSlice[0]), static_cast<float>(m_numSlice[1]));
		const int sliceX      = std::clamp(static_cast<int>(std::floorf(sliceXy.x)), 0, m_numSlice[0] - 1);
		const int sliceY      = std::clamp(static_cast<int>(std::floorf(sliceXy.y)), 0, m_numSlice[1] - 1);
		slice                 = sliceX + sliceY * m_numSlice[0];
	}

#if ECHO_EDITOR
	std::shared_lock lock(m_heightQueryMutex);
#endif
	const Node* curr = m_slices[slice].get();

	while (curr)
	{
		const auto ofsX    = curr->m_offset.x;
		const auto ofsY    = curr->m_offset.y;
		const auto grid    = curr->m_gridSize;
		const auto gridInv = curr->m_gridSizeInv;
		if (!curr->m_active && curr->hasChildren())
		{
			const auto centerX = ofsX + curr->m_length.x * 0.5f;
			const auto centerY = ofsY + curr->m_length.y * 0.5f;
			const auto quad    = static_cast<Quad>((xy.x < centerX ? 0b00 : 0b01) | (xy.y < centerY ? 0b00 : 0b10));
			curr               = curr->getChild(quad);
		}
		else
		{
			const float x  = (xy.x - ofsX) * gridInv.x;
			const float y  = (xy.y - ofsY) * gridInv.y;
			const int iX   = std::clamp(static_cast<int>(std::floorf(x)), 0, QUADS - 1);
			const int iY   = std::clamp(static_cast<int>(std::floorf(y)), 0, QUADS - 1);
			const float dx = x - iX;
			const float dy = y - iY;

			float elevation[4] { 0.0f };
			if (curr->m_elevationMutex.try_lock_shared())
			{
				if (curr->m_elevations.empty())
				{
					curr->m_elevationMutex.unlock_shared();
					break;
				}

				elevation[0] = curr->m_elevations[iX + iY * GRID_RESOLUTION];
				elevation[1] = curr->m_elevations[iX + 1 + iY * GRID_RESOLUTION];
				elevation[2] = curr->m_elevations[iX + (iY + 1) * GRID_RESOLUTION];
				elevation[3] = curr->m_elevations[iX + 1 + (iY + 1) * GRID_RESOLUTION];
				curr->m_elevationMutex.unlock_shared();
			}
			else
			{
				break;
			}

			if (depth) *depth = curr->m_depth;

			Vector3 position[4] {};
			Vector2 offset[4]
			{
				Vector2(ofsX + iX * grid.x, ofsY + iY * grid.y),
				Vector2(ofsX + (iX + 1) * grid.x, ofsY + iY * grid.y),
				Vector2(ofsX + iX * grid.x, ofsY + (iY + 1) * grid.y),
				Vector2(ofsX + (iX + 1) * grid.x, ofsY + (iY + 1) * grid.y)
			};

			if (isSphere())
			{
				for (int i = 0; i < 4; ++i)
				{
					position[i] = Node::applyElevation(Node::getSphericalPosition(slice, offset[i], m_pTerrainGenerator), elevation[i]);
				}
			}
			else if (isTorus())
			{
				for (int i = 0; i < 4; ++i)
				{
					offset[i] *= Math::TWO_PI;
					position[i] = TorusTerrainNode::GetTorusPosition((elevation[i] + 1.0f) * m_relativeMinorRadius, offset[i].x, offset[i].y);
				}
			}

			auto [uno, dos, tres] = GetQuadTriangleWindingOrder(iX, iY, dx, dy);
			triangle[0]           = position[uno];
			triangle[1]           = position[dos];
			triangle[2]           = position[tres];
			break;
		}
	}
	return triangle;
}

DVector3 SphericalTerrain::getSurfaceFinestWs(const DVector3& worldPos, Vector3* faceNormal) const
{
	auto ret = ToWorldSpace(getSurfaceFinestLs(ToLocalSpace(worldPos), faceNormal));
	if (faceNormal) *faceNormal = m_rot * *faceNormal;
	return ret;
}

Vector3 SphericalTerrain::getSurfaceFinestLs(const Vector3& localPos, Vector3* faceNormal) const
{
	{
		int depth = -1;
		auto pos = getSurfaceLs(localPos, faceNormal, &depth);
		if (depth == m_maxDepth) return pos;
	}

	auto pos      = localPos;
	const auto up = getUpVectorLs(localPos, &pos);
	const Ray ray(pos, -up);
	Vector3 v0, v1, v2;
	getSurfaceTriangleFinestLs(pos, v0, v1, v2);
	const auto plane = Plane(v0, v1, v2);
	if (faceNormal) *faceNormal = plane.normal;
	auto [intersects, t] = ray.intersects(plane);
	pos = ray.getPoint(t);
	return pos;
}

float SphericalTerrain::getDistanceToSurface(const Vector3& planetPos) const
{
	const Vector3& localPos = m_realScaleInv.x * planetPos;
	auto pos         = localPos;
	const auto lclUp = getUpVectorLs(pos);
	const Ray ray(pos, -lclUp);
	int slice = 0;
	Vector2 xy;
	if (isSphere())
	{
		Node::MapToCube(pos, slice, xy);
	}
	else if (isTorus())
	{
		xy = TorusTerrainNode::MapToPlane(pos);
	}
	// int depth = -1;
	auto triangle  = getSurfaceTriangleLs(xy, slice/*, &depth*/);
	// if (depth != m_maxDepth)
	// {
	// 	getSurfaceTriangleFinestLs(localPos, triangle[0], triangle[1], triangle[2]);
	// }
	const auto plane     = Plane(triangle[0], triangle[1], triangle[2]);
	auto [intersects, t] = ray.intersects(plane);
	return m_realScale.x * t;
}

void SphericalTerrain::getSurfaceTriangleFinestLs(const Vector3& localPos, Vector3& p0, Vector3& p1, Vector3& p2) const
{
	float dx = 0.0f, dy = 0.0f;
	int iX   = 0, iY    = 0, slice = 0;
	Vector2 grid;
	int tileX = 0, tileY = 0;

	const int sliceTiles = 1 << m_maxDepth;
	if (isSphere())
	{
		const auto sliceQuads = Vector2(static_cast<float>(QUADS * sliceTiles));
		Vector2 xy;
		Node::MapToCube(localPos, slice, xy);
		xy = xy * 0.5f + 0.5f;

		const auto tileOffset = xy;
		tileX = std::clamp(int(std::truncf(tileOffset.x * float(sliceTiles))), 0, sliceTiles - 1);
		tileY = std::clamp(int(std::truncf(tileOffset.y * float(sliceTiles))), 0, sliceTiles - 1);

		xy *= sliceQuads;
		iX   = static_cast<int>(std::truncf(xy.x));
		iY   = static_cast<int>(std::truncf(xy.y));
		dx   = xy.x - iX;
		dy   = xy.y - iY;
		grid = Vector2(static_cast<float>(Node::getGridSizeOnCube(m_maxDepth)));
	}
	else if (isTorus())
	{
		const auto nSlice = Vector2(float(m_numSlice[0]), float(m_numSlice[1]));
		const auto sliceQuads = float(QUADS * sliceTiles) * nSlice;
		Vector2 xy = TorusTerrainNode::MapToPlane(localPos);

		const int sliceX = std::clamp(int(std::truncf(xy.x * nSlice.x)), 0, m_numSlice[0] - 1);
		const int sliceY = std::clamp(int(std::truncf(xy.y * nSlice.y)), 0, m_numSlice[1] - 1);
		slice  = sliceX + sliceY * m_numSlice[0];
		const auto tileOffset = xy * nSlice - Vector2(float(sliceX), float(sliceY));
		tileX = std::clamp(int(std::truncf(tileOffset.x * float(sliceTiles))), 0, sliceTiles - 1);
		tileY = std::clamp(int(std::truncf(tileOffset.y * float(sliceTiles))), 0, sliceTiles - 1);

		xy *= sliceQuads;
		iX   = static_cast<int>(std::truncf(xy.x));
		iY   = static_cast<int>(std::truncf(xy.y));
		dx   = xy.x - iX;
		dy   = xy.y - iY;
		grid = 1.0f / sliceQuads;
	}

	int nodeIndex = getNodeIndex(NodeInfo { .slice = slice, .depth = m_maxDepth, .x = tileX,.y = tileY });

	Vector3 position[4];
	Vector3 baseShape[4];
	Vector2 offset[4]
	{
		grid * Vector2(static_cast<float>(iX), static_cast<float>(iY)),
		grid * Vector2(static_cast<float>(iX + 1), static_cast<float>(iY)),
		grid * Vector2(static_cast<float>(iX), static_cast<float>(iY + 1)),
		grid * Vector2(static_cast<float>(iX + 1), static_cast<float>(iY + 1)),
	};

	float elevations[4];
	if (isSphere())
	{
		for (int i = 0; i < 4; ++i)
		{
			offset[i] += -1.0f;
			baseShape[i] = Node::getSphericalPosition(slice, offset[i], m_pTerrainGenerator);
		}
		generateElevations(baseShape, elevations, 4);
		for (int i = 0; i < 4; ++i)
		{
			position[i] = Node::applyElevation(baseShape[i], elevations[i] * m_elevationRatio);
		}
	}
	else if (isTorus())
	{
		for (int i = 0; i < 4; ++i)
		{
			offset[i] *= Math::TWO_PI;
			baseShape[i] = TorusTerrainNode::GetTorusPosition(m_relativeMinorRadius, offset[i].x, offset[i].y);
		}
		generateElevations(baseShape, elevations, 4);
		for (int i = 0; i < 4; ++i)
		{
			position[i] = TorusTerrainNode::applyElevation(baseShape[i], elevations[i] * m_elevationRatio, m_relativeMinorRadius);
		}
	}

	if (!m_editingTerrain)
	{
		computeStaticModify(nodeIndex, position, baseShape, elevations, 4);
	}
	
	auto [uno, dos, tres] = GetQuadTriangleWindingOrder(iX, iY, dx, dy);
	p0 = position[uno];
	p1 = position[dos];
	p2 = position[tres];
}

void SphericalTerrain::getSurfaceTriangleFinestWs(const DVector3& worldPos, DVector3& _p0, DVector3& _p1, DVector3& _p2) const {
	{
		const auto pos = ToLocalSpace(worldPos);
		int slice = 0;
		Vector2 xy;
		if (isSphere())
		{
			Node::MapToCube(pos, slice, xy);
		}
		else if (isTorus())
		{
			xy = TorusTerrainNode::MapToPlane(pos);
		}
		int depth = -1;
		const auto triangle = getSurfaceTriangleLs(xy, slice, &depth);
		if (depth == m_maxDepth)
		{
			_p0 = ToWorldSpace(triangle[0]);
			_p1 = ToWorldSpace(triangle[1]);
			_p2 = ToWorldSpace(triangle[2]);
			return;
		}
	}

	Vector3 p0, p1, p2;
	getSurfaceTriangleFinestLs(ToLocalSpace(worldPos), p0, p1, p2);
	_p0 = ToWorldSpace(p0);
	_p1 = ToWorldSpace(p1);
	_p2 = ToWorldSpace(p2);
}

DVector3 SphericalTerrain::getOceanSurface(const DVector3& worldPos) const
{
	if (!m_haveOcean || !m_ocean) return worldPos;
	return m_ocean->getPlainSurface(worldPos);
}

DVector3 SphericalTerrain::getOceanWaveSurface(const DVector3& worldPos) const
{
	if (!m_haveOcean || !m_ocean) return worldPos;
	return m_ocean->calcWaveHeightPosWS(worldPos);
}

bool SphericalTerrain::isUnderOcean(const DVector3& worldPos, double* depth) const
{
	if (!m_haveOcean || !m_ocean) return false;
	return m_ocean->isInside(worldPos, depth);
}

bool SphericalTerrain::isUnderOceanWave(const DVector3& worldPos, double* depth) const
{
	if (!m_haveOcean || !m_ocean) return false;
	return m_ocean->getIsUnderWater(worldPos, nullptr, nullptr, depth);
}

float SphericalTerrain::getBoarderValue(const Vector3& localPos) const {
	if (m_pTerrainGenerator)
	{
		float borderDis = m_pTerrainGenerator->getBorderFace(localPos) - m_terrainMinBorder;
		if (borderDis < 0.0f) {
			return 1.0f;
		}
		if (m_terrainBorderSize > 0 && borderDis < m_terrainBorderSize)
		{
			return std::clamp(1 - borderDis * m_oneOverTerrainBorderSize, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}

float SphericalTerrain::getBoarderValue(const DVector3& worldPos) const
{
	return getBoarderValue(ToLocalSpace(worldPos));
}

void SphericalTerrain::vegetationRecheck(const AxisAlignedBox& box)
{
	m_vegetationRecheckRange.emplace_back(box);
}

int SphericalTerrain::getSurfaceMipWs(const DVector3& worldPos) const
{
	return getSurfaceMipLs(ToLocalSpace(worldPos));
}

int SphericalTerrain::getSurfaceMipLs(const Vector3& localPos) const
{
	int slice = 0;
	Vector2 xy;
	if (isSphere())
	{
		Node::MapToCube(localPos, slice, xy);
	}
	else if (isTorus())
	{
		xy = TorusTerrainNode::MapToPlane(localPos);

		const Vector2 sliceXy = xy * Vector2(static_cast<float>(m_numSlice[0]), static_cast<float>(m_numSlice[1]));
		const int sliceX      = std::clamp(static_cast<int>(std::floorf(sliceXy.x)), 0, m_numSlice[0] - 1);
		const int sliceY      = std::clamp(static_cast<int>(std::floorf(sliceXy.y)), 0, m_numSlice[1] - 1);
		slice                 = sliceX + sliceY * m_numSlice[0];
	}
#if ECHO_EDITOR
	std::shared_lock mainLock(m_heightQueryMutex);
#endif
	const Node* curr = m_slices.at(slice).get();
	while (curr)
	{
		const auto ofsX = curr->m_offset.x;
		const auto ofsY = curr->m_offset.y;
		if (!curr->m_active && curr->hasChildren())
		{
			const auto centerX = ofsX + curr->m_length.x * 0.5f;
			const auto centerY = ofsY + curr->m_length.y * 0.5f;
			const auto quad    = static_cast<Quad>((xy.x < centerX ? 0b00 : 0b01) | (xy.y < centerY ? 0b00 : 0b10));
			curr               = curr->getChild(quad);
		}
		else 
		{
			std::shared_lock lock(curr->m_elevationMutex);
			if (curr->m_elevations.empty()) // generation requested, but not yet generated
			{
				return -1;
			}
			break;
		}
	}
	return curr ? curr->m_depth : -1;
}

#ifdef ECHO_EDITOR

void SphericalTerrain::showElevation(const bool show)
{
	auto& view = g_planetViews[ElevationWorldMap];
	if (!view) return;
	view->setShow(show);
	if (!show) return;
	m_pcg->updateWorldMap(*view, m_pcg->getElevationTargets(), 0.5f / m_noise->GetSigma3(), 0.5f);
}

void SphericalTerrain::showTemperature(const bool show)
{
	auto& view = g_planetViews[TemperatureWorldMap];
	if (!view) return;
	view->setShow(show);
	if (!show) return;
	m_pcg->updateWorldMap(*view, m_pcg->getTemperatureTargets());
}

void SphericalTerrain::showHumidity(const bool show)
{
	auto& view = g_planetViews[HumidityWorldMap];
	if (!view) return;
	view->setShow(show);
	if (!show) return;
	m_pcg->updateWorldMap(*view, m_pcg->getHumidityTargets());
}

void SphericalTerrain::showGlobalViews(RenderQueue* pQueue)
{
	const int numViews = static_cast<int>(std::count_if(g_planetViews.begin(), g_planetViews.end(), [](const auto& view) { return view && view->m_show; }));
	uint32_t vpw, vph;
	Root::instance()->getRenderSystem()->getCurRenderStrategy()->getRenderTargetByType(RT_Main)->getWidthAndHeight(vpw, vph);
	const float space = 10.0f / static_cast<float>(vph);

	const float vpRatio = static_cast<float>(vpw) / static_cast<float>(vph);
	int texW, texH;
	switch (m_topology)
	{
	case PTS_Sphere:
		texW = WorldMapWidth;
		texH = WorldMapHeight;
		break;
	case PTS_Torus:
		texW = m_biomeTextureWidth;
		texH = m_biomeTextureHeight;
		break;
	}
	float screenW       = Math::Min(0.5f, static_cast<float>(texW) / static_cast<float>(vpw));
	float screenH       = Math::Min((1.0f - static_cast<float>(numViews - 1) * space) / static_cast<float>(numViews), static_cast<float>(texH) / static_cast<float>(vph));
	const float tmp     = screenW;
	const float ratio = static_cast<float>(texW) / static_cast<float>(texH);
	screenW = Math::Min(screenW, screenH * ratio / vpRatio);
	screenH = Math::Min(screenH, tmp / ratio * vpRatio);

	float imagePosY = 0.0f;
	for (auto&& view : g_planetViews)
	{
		if (!view || !view->m_show) continue;

		view->m_screenPos            = Vector2(1.0f - screenW, imagePosY);
		view->m_screenSize           = Vector2(screenW, screenH);
		RenderQueueGroup* queueGroup = pQueue->getRenderQueueGroup(RenderQueue_UI_Transparent);
		queueGroup->addRenderable(g_worldMapMat->getPass(NormalPass), view.get(), -100000);

		imagePosY += screenH + space;
	}
}

void SphericalTerrain::updateStampTerrainDisplay(bool bDisplay, const Matrix4& mtx)
{
	m_bDisplay = bDisplay;
	m_StampTerrainMtx = mtx;
}

void SphericalTerrain::computeBoundCpu()
{
#ifdef _WIN32
	OutputDebugStringA("------ Compute bounds started ------\n");
	const auto start = std::chrono::steady_clock::now();
#endif

	const size_t slices = static_cast<size_t>(m_numSlice[0]) * m_numSlice[1];
	std::vector eMinOnSlice(slices, std::numeric_limits<float>::max());
	std::vector eMaxOnSlice(slices, -std::numeric_limits<float>::max());
	std::vector noiseMinOnSlice(slices, std::numeric_limits<float>::max());
	std::vector noiseMaxOnSlice(slices, -std::numeric_limits<float>::max());
	auto computeBounds = [this, &eMinOnSlice, &eMaxOnSlice, &noiseMinOnSlice, &noiseMaxOnSlice](const int slice)
	{
		float& eMin = eMinOnSlice[slice];
		float& eMax = eMaxOnSlice[slice];

		float& nMin = noiseMinOnSlice[slice];
		float& nMax = noiseMaxOnSlice[slice];

		int offset = 1 + slice * Node::getTileCountOverLevel(m_maxDepth);
		for (const int lv = m_maxDepth;;)
		{
			const int currOffset  = offset + Node::getTileCountOverLevel(lv - 1);
			const int tiles       = 1 << lv;
			for (int row = 0; row < tiles; ++row)
			{
				for (int col = 0; col < tiles; ++col)
				{
					AxisAlignedBox bound;
					std::vector<Vector3> pos;
					if (isSphere())
					{
						const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(lv)));
						const Vector2 len(static_cast<float>(Node::getLengthOnCube(lv)));
						const Vector2 gridOffset = Vector2(static_cast<float>(col), static_cast<float>(row)) * len - 1.0f;

						pos = Node::getSphericalPositions(slice, GRID_RESOLUTION, GRID_RESOLUTION,
							gridSize, gridOffset, m_pTerrainGenerator);
					}
					else if (isTorus())
					{
						auto [gridSize, gridOffset] = TorusTerrainNode::getGridSizeOffset(
							slice, m_numSlice[0], m_numSlice[1], lv, col, row);

						pos = TorusTerrainNode::getTorusPositions(
							GRID_RESOLUTION, GRID_RESOLUTION,
							m_relativeMinorRadius,
							gridSize, gridOffset);
					}
					std::vector<float> elevation(pos.size());
					generateElevations(pos.data(), elevation.data(), pos.size());

					for (int i = 0; i < VERTEX_COUNT; ++i)
					{
						const float e = elevation[i] * m_elevationRatio;
						float eReal = e;
						if (isSphere() && m_pTerrainGenerator)
						{
							eReal = (1.0f + e) * pos[i].length() - 1.0f;
						}

						eMin = std::min(eMin, eReal);
						eMax = std::max(eMax, eReal);

						nMin = std::min(nMin, e);
						nMax = std::max(nMax, e);

						auto p = Vector3::ZERO;
						if (isSphere())
						{
							p = Node::applyElevation(pos[i], e);
						}
						else if (isTorus())
						{
							p = TorusTerrainNode::applyElevation(pos[i], e, m_relativeMinorRadius);
						}

						bound.merge(p);
					}

					int currIndex       = currOffset + row * tiles + col;
					m_bounds[currIndex] = bound;
#ifdef _WIN32
					std::string log = "Node " + std::to_string(offset + row * tiles + col) + " complete.\n";
					OutputDebugStringA(log.c_str());
#endif
				}
			}
			break;
		}

		int tiles = 1 << m_maxDepth;
		offset = offset + Node::getTileCountOverLevel(m_maxDepth - 1);
		for (int lv = m_maxDepth - 1; lv >= 0; --lv)
		{
			const int currTiles  = tiles >> 1;
			const int currOffset = offset - Node::getLevelTileCount(lv);
			for (int row = 0; row < currTiles; ++row)
			{
				for (int col = 0; col < currTiles; ++col)
				{
					AxisAlignedBox& bound = m_bounds[currOffset + row * currTiles + col];
					bound.merge(m_bounds[offset + 2 * row * tiles + 2 * col]);
					bound.merge(m_bounds[offset + 2 * row * tiles + 2 * col + 1]);
					bound.merge(m_bounds[offset + (2 * row + 1) * tiles + 2 * col]);
					bound.merge(m_bounds[offset + (2 * row + 1) * tiles + 2 * col + 1]);
				}
			}
			tiles  = currTiles;
			offset = currOffset;
		}
	};

	m_bounds.clear();
	m_bounds.resize(1 + getNodesCount(), AxisAlignedBox::BOX_NULL);
	// m_orientedBounds.clear();
	// m_orientedBounds.resize(getNodesCount());
	std::vector<std::thread> threads;
	threads.reserve(slices);
	for (int i = 0; i < slices; ++i)
	{
		threads.emplace_back(computeBounds, i);
		// computeBounds(static_cast<Face>(i));
	}

	for (auto&& t : threads)
	{
		t.join();
	}

	for (int i = 0; i < slices; ++i)
	{
		m_bounds.front().merge(m_bounds[1 + i * Node::getTileCountOverLevel(m_maxDepth)]);
	}

	m_elevationMin = *std::ranges::min_element(eMinOnSlice);
	m_elevationMax = *std::ranges::max_element(eMaxOnSlice);

	m_noiseElevationMin = *std::ranges::min_element(noiseMinOnSlice);
	m_noiseElevationMax = *std::ranges::max_element(noiseMaxOnSlice);
#ifdef _WIN32
	const auto end = std::chrono::steady_clock::now();
	const std::string log = "========== Compute bounds finished and took " + std::to_string(static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f) + " seconds ==========\n";
	OutputDebugStringA(log.c_str());
#endif
}

void SphericalTerrain::copyMaterialIndexFromGpuMemory()
{
	if (!m_gpuMode) return;
	for (int i = 0; i < 6; ++i)
	{
		m_generatedMaterialMap[i].freeMemory();
		m_generatedMaterialMap[i] = Bitmap {};
	}
	m_pcg->readBackMaterialIndex(*this, m_generatedMaterialMap);
	std::unique_lock lock(m_biomeQueryMutex);
	for (int i = 0; i < 6; ++i)
	{
		m_materialIndexMap.faces[i] = &m_generatedMaterialMap[i];
	}
#ifdef ECHO_EDITOR
	updateBiomePercentage();
#endif
}
#endif

void SphericalTerrain::loadHumidityClimateProcess(const ImageProcess& newIP, const AoParameter& newAP)
{
	m_terrainData->distribution.climateProcess.humidityIP = newIP;
	m_terrainData->distribution.climateProcess.aoParam    = newAP;


	if (m_gpuMode)
	{
#ifdef ECHO_EDITOR
		computeHumidityGPU();
		computeMaterialIndexGPU();
		copyMaterialIndexFromGpuMemory();
#endif
		return;
	}
	else
	{
		assert(false && "Not implemented");
	}
}

void SphericalTerrain::loadTemperatureClimateProcess(const ImageProcess& newIP)
{
	m_terrainData->distribution.climateProcess.temperatureIP = newIP;
	if (m_gpuMode)
	{
#ifdef ECHO_EDITOR
		computeTemperatureGPU();
		computeMaterialIndexGPU();
		copyMaterialIndexFromGpuMemory();
#endif
		return;
	}
	else
	{
		assert(false && "Not implemented");
	}
}

void SphericalTerrain::setDisplayMode(const BiomeDisplayMode mode)
{
	m_displayMode = mode; 
}

void SphericalTerrain::biomeDefineChanged(const String& newPath)
{
	BiomeDistribution newDist = {};
	newDist.filePath      = newPath;

	if (SphericalTerrainResource::loadBiomeDistribution(newDist))
	{
		biomeDefineChanged(m_terrainData->composition,
						   newDist,
						   true);
	}
}

void
SphericalTerrain::biomeDefineChanged(const BiomeComposition& newBiome,
									 const BiomeDistribution& newBiomeDist,
									 const bool reload)
{
	m_terrainData->composition          = newBiome;
	m_terrainData->distribution = newBiomeDist;
	
	//IMPORTANT(yanghang):Scan all biome composition find out textures
	// that aren't duplicated.
	m_terrainData->freshUsedBiomeTextureList();

    //IMPORTANT:Editor mode Sync reload biome texture.
	{
        if(m_terrainData->m_planetTexRes.isNull())
        {
            NameValuePairList params;
            String biomeTexResName =  m_terrainData->getName().toString();
            params["parentResName"] = biomeTexResName;
            auto result  = PlanetTextureResourceManager::instance()->
                createOrRetrieve(m_terrainData->getName(),
                                 false,
                                 0,
                                 &params);
            m_terrainData->m_planetTexRes = result.first;
        }
        else
        {
            if(m_terrainData->m_requestID != 0)
            {
                PlanetTextureResourceManager::instance()->abortRequest(m_terrainData->m_requestID);
            }
            m_terrainData->m_planetTexRes->unload(); 
        }

        if(!(m_terrainData->m_planetTexRes->isPrepared() ||
             m_terrainData->m_planetTexRes->isLoaded()))
        {
            m_terrainData->m_planetTexRes->m_planetRes = m_terrainData;
            
            m_terrainData->m_planetTexRes->prepare();
            m_terrainData->m_planetTexRes->load();                
        }
	}
	
	if (m_editingTerrain)
	{
		auto [lutTex, lutMap] =
			PlanetManager::generateBiomeLookupMap(m_terrainData->distribution,
												  m_terrainData->composition,
												  true);
		
		m_terrainData->lookupTex = lutTex;
		lutMap.freeMemory(); 
	}

	//IMPORTANT(yanghang):Free the useless memory of [biome define].
	//note be hejiwei: 后面cpu计算lut需要用 biome define，暂时先注掉这里
	//m_terrainData->composition.biomeArray.clear();

	if (m_material.isV1())
	{
		Material* mat = m_material.getV1();
		mat->m_textureList[MAT_TEXTURE_CUSTOMER]  = m_terrainData->m_planetTexRes->m_biomeGPUData.diffuseTexArray;
		mat->m_textureList[MAT_TEXTURE_CUSTOMER2] = m_terrainData->lookupTex;
		mat->m_textureList[MAT_TEXTURE_CUSTOMER3] = m_terrainData->m_planetTexRes->m_biomeGPUData.normalTexArray;
		mat->m_textureList[MAT_TEXTURE_CUSTOMER6] = loadFbmTexture();
	}
	else if (m_material.isV2())
	{
		MaterialV2* mat = m_material.getV2();
		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER, m_terrainData->m_planetTexRes->m_biomeGPUData.diffuseTexArray);
		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER2, m_terrainData->lookupTex);
		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER3, m_terrainData->m_planetTexRes->m_biomeGPUData.normalTexArray);
		mat->setTexture(Mat::TEXTURE_BIND_CUSTOMER6, loadFbmTexture());
	}

	if (m_vegHandle)
	{
		for (int biomeCompoIndex = 0;
			 biomeCompoIndex < m_terrainData->distribution.distRanges.size();
			 biomeCompoIndex++)
		{
			if (biomeCompoIndex < newBiome.biomes.size() &&
				newBiome.biomes[biomeCompoIndex].biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
			{
				int biomeIndex = m_terrainData->composition.getBiomeIndex(biomeCompoIndex);
				m_vegHandle->updateAllVegRes(biomeIndex, newBiome.biomes.at(biomeCompoIndex).vegLayers);
			}
		}
	}

	if (m_deformSnow) {
		m_deformSnow->redivide();
	}

	resetPlanetGenObject();

	m_biomeDefineDirty = true;
}

void Echo::SphericalTerrain::sphericalOceanChanged(bool setOcean)
{	
	if (setOcean)
	{
		m_haveOcean = true; // run time
		m_terrainData->sphericalOcean.haveOcean = true; // load and save time

		if (!m_ocean)
		{
			m_ocean = new SphericalOcean(this);
			m_ocean->setWorldTrans(Transform(m_pos, m_rot));
			m_ocean->setScale(m_scale.x);
		}

		m_ocean->getFileSetting(m_terrainData->sphericalOcean.fileSetting);
		m_ocean->getGeomSetting(m_terrainData->sphericalOcean.geomSettings);
		m_ocean->getShadeSetting(m_terrainData->sphericalOcean.shadeSettings);
		m_ocean->init();
		m_ocean->MgrRebuild();
	}
	else
	{
		// Fake unload the Sphercial Ocean, it still in the Sphercial Terrain memory. It will be unvisible.
		m_haveOcean = false;// run time
		m_terrainData->sphericalOcean.haveOcean = false;// load and save time
		SAFE_DELETE(m_ocean);
	}
	

	/*
	
	if (setOcean) {
		m_haveOcean = true; // run time
		m_terrainData->sphericalOcean.haveOcean = true; // load and save time
		// If doesn't have sphrical ocean before
		if (!m_ocean)
		{
			m_ocean = new SphericalOcean();
			m_ocean->setWorldPos(m_pos);
			m_ocean->init();
		}
		m_ocean->setRadius(m_radius);
	}
	else
	{
		// Fake unload the Sphercial Ocean, it still in the Sphercial Terrain memory. It will be unvisible.
		m_haveOcean = false;// run time
		m_terrainData->sphericalOcean.haveOcean = false;// load and save time
	}
	if (m_ocean) {
		m_ocean->getFileSetting(m_terrainData->sphericalOcean.fileSetting);
		m_ocean->getGeomSetting(m_terrainData->sphericalOcean.geomSettings);
		m_ocean->getShadeSetting(m_terrainData->sphericalOcean.shadeSettings);
	}
	*/
}

void SphericalTerrain::reload3DNoiseChanged(Noise3DData& newData)
{
	Noise3DData& oldData          = m_terrainData->geometry.noise;
	oldData.featureNoiseSeed      = newData.featureNoiseSeed;
	oldData.lacunarity            = newData.lacunarity;
	oldData.baseFrequency         = newData.baseFrequency;
	oldData.baseAmplitude         = newData.baseAmplitude;
	oldData.detailAmplitude       = newData.detailAmplitude;
	oldData.gain                  = newData.gain;
	oldData.baseOctaves           = newData.baseOctaves;
	oldData.sharpnessNoiseSeed    = newData.sharpnessNoiseSeed;
	oldData.sharpnessRange        = newData.sharpnessRange;
	oldData.sharpnessFrequency    = newData.sharpnessFrequency;
	oldData.slopeErosionNoiseSeed = newData.slopeErosionNoiseSeed;
	oldData.slopeErosionRange     = newData.slopeErosionRange;
	oldData.slopeErosionFrequency = newData.slopeErosionFrequency;
	oldData.perturbNoiseSeed      = newData.perturbNoiseSeed;
	oldData.perturbRange          = newData.perturbRange;
	oldData.perturbFrequency      = newData.perturbFrequency;
	oldData.terrainMinBorder	  = newData.terrainMinBorder;
	oldData.terrainBorderSize	  = newData.terrainBorderSize;
	oldData.terrainBorderHeight	  = newData.terrainBorderHeight;

	generateBiome();
	m_terrainData->geometry.setBoundDirty();
	generateMesh();
}

void SphericalTerrain::setWorldPos(const DBVector3& pos)
{
	if (pos == m_pos) return;

	m_pos = pos;
	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	m_worldMatInv = m_worldMat.inverseAffine();
	if (m_haveOcean && m_ocean)
	{
		m_ocean->setWorldPos(pos);
	}
	onWorldTransformChanged();

	if (m_vegHandle) 
		m_vegHandle->onChangeTerrainPos(pos);

	if (m_pPlanetGenObjectScheduler) {
		m_pPlanetGenObjectScheduler->setWorldTrans(DTransform(m_pos, m_rot), m_scale);
	}
}

void SphericalTerrain::setWorldRot(const Quaternion& rot)
{
	if (rot == m_rot) return;

	m_rot = rot;
	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	m_worldMatInv = m_worldMat.inverseAffine();
	onWorldTransformChanged();

	if (m_vegHandle) 
		m_vegHandle->onChangeTerrainRot(m_rot);

	if (m_pPlanetGenObjectScheduler) {
		m_pPlanetGenObjectScheduler->setWorldTrans(DTransform(m_pos, m_rot), m_scale);
	}
}

/*
void SphericalTerrain::setWorldScale(const Vector3& scale) 
{
	m_scale = scale;
	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	onWorldTransformChanged();
}
*/
void SphericalTerrain::setWorldTrans(const DTransform& trans, const Vector3& scale)
{
	if (trans.translation == m_pos && trans.rotation == m_rot && scale == m_scale) return;

	m_pos = trans.translation;
	m_rot = trans.rotation;
	m_scale = scale;
	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	m_worldMatInv = m_worldMat.inverseAffine();
	m_realScale = m_scale * m_radius;
	m_realScaleInv = 1 / m_realScale;
	if (m_haveOcean && m_ocean)
	{
		m_ocean->setWorldTrans(Transform(m_pos, m_rot));
		m_ocean->setScale(scale.x);
	}
	onWorldTransformChanged();

	if (m_vegHandle)
	{
		m_vegHandle->onChangeTerrainPos(m_pos);
		m_vegHandle->onChangeTerrainRot(m_rot);

		float maxRadius = std::max(std::max(m_scale.x, m_scale.y), m_scale.z) * m_radius;
		float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
		m_vegHandle->onChangeTerrainRadius(maxRadius);
		m_vegHandle->onChangeTerrainMaxOff(maxRadius * maxElevation);
	}
	if (m_pPlanetGenObjectScheduler) {
		m_pPlanetGenObjectScheduler->setWorldTrans(DTransform(m_pos, m_rot), m_scale);
	}
}

void SphericalTerrain::setRadius(const float radius)
{
	m_radius = radius;
	m_tillingParam = radius * 2 / 10; //每10m贴一张1024的贴图

	m_worldMat.makeTransform(m_pos, m_scale * m_radius, m_rot);
	m_worldMatInv = m_worldMat.inverseAffine();
	m_realScale = m_scale * m_radius;
	m_realScaleInv = 1 / m_realScale;
	onWorldTransformChanged();

	if (m_vegHandle)
	{
		float maxRadius = std::max(std::max(m_scale.x, m_scale.y), m_scale.z) * m_radius;
		m_vegHandle->onChangeTerrainRadius(maxRadius);
		float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
		m_vegHandle->onChangeTerrainMaxOff(maxRadius * maxElevation);
	}
}

void SphericalTerrain::setRadius(const float major, const float minor)
{
	setRadius(major);
	m_relativeMinorRadius = minor;
}

void SphericalTerrain::setTerrainGeneratorType(uint32 type) {
	m_pTerrainGenerator = nullptr;
	m_pTerrainGenerator = TerrainGeneratorLibrary::instance()->get(type);
	if (m_editingTerrain && m_pTerrainGenerator != nullptr) {
		m_pTerrainGenerator->reload();
	}
}

void SphericalTerrain::onWorldTransformChanged()
{
    if(m_warfog)
    {
        m_warfog->updateWorldTransform();
    }

    if(m_cloud)
    {
        m_cloud->updateWorldTransform();
    }

	if (m_PlotDivision)
	{
		m_PlotDivision->updateWorldTransform();
	}
    
	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [&](Node* node)
		{
			node->updateWorldTransform();
			return true;
		});
	}

	if (m_deformSnow) {
		m_deformSnow->redivide();
	}
}

float SphericalTerrain::getMaxRealRadius() const
{
	float radius = 0.0f;
	if (isSphere())
	{
		radius = m_radius * (1.0f + m_elevationMax);
	}
	else if (isTorus())
	{
		radius = m_radius * (1.0f + m_relativeMinorRadius * (1.0f + m_elevationMax));
	}
	radius *= std::max({ std::abs(m_scale.x), std::abs(m_scale.y), std::abs(m_scale.z) });
	return radius;
}

float SphericalTerrain::getMinRealRadius() const
{
	float radius = 0.0f;
	if (isSphere())
	{
		radius = m_radius * (1.0f + m_elevationMin);
	}
	else if (isTorus())
	{
		radius = m_radius * (1.0f - m_relativeMinorRadius * (1.0f + m_elevationMax));
	}
	radius *= std::min({ std::abs(m_scale.x), std::abs(m_scale.y), std::abs(m_scale.z) });
	return radius;
}

void SphericalTerrain::setElevation(const float elevation)
{
	m_elevationRatio = elevation;
	if (m_vegHandle)
	{
		float maxRadius = std::max(std::max(m_scale.x, m_scale.y), m_scale.z) * m_radius;
		float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
		m_vegHandle->onChangeTerrainMaxOff(maxRadius * maxElevation);
	}
}

void SphericalTerrain::setMaxDepth(const int maxDepth)
{
	m_maxDepth = maxDepth;
}

void SphericalTerrain::setSliceCount(int x, int y)
{
	if (isSphere() && (x != 6 || y != 1)) return;
	m_numSlice[0] = x;
	m_numSlice[1] = y;
}

std::vector<int> SphericalTerrain::getLeavesInWs(const AxisAlignedBox& box) const
{
	auto lclBox = box;
	lclBox.transformAffine(m_worldMatInv);
	return getLeavesInLs(lclBox);
}

std::vector<int> SphericalTerrain::getLeavesInLs(const AxisAlignedBox& box) const
{
	if (!isBoundAccurate()) return {};

	if (!m_bounds.front().intersects(box))
	{
		return {};
	}

	std::vector<int> nodes;
	std::function<void(const NodeInfo&)> traverseBoundTree = [&box, &nodes, this, &traverseBoundTree](const NodeInfo& node) -> void
	{
		if (getIndexBound(getNodeIndex(node)).intersects(box))
		{
			if (node.depth == m_maxDepth)
			{
				nodes.emplace_back(getNodeIndex(node));
				return;
			}

			traverseBoundTree({ node.slice, node.depth + 1, node.x << 1, node.y << 1 });
			traverseBoundTree({ node.slice, node.depth + 1, (node.x << 1) | 1, node.y << 1 });
			traverseBoundTree({ node.slice, node.depth + 1, node.x << 1, (node.y << 1) | 1 });
			traverseBoundTree({ node.slice, node.depth + 1, (node.x << 1) | 1, (node.y << 1) | 1 });
		}
	};

	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [this, &box, &nodes, &traverseBoundTree](const SphericalTerrainQuadTreeNode* node)
		{
			if (node->getBoundLocal().intersects(box))
			{
				if (node->m_depth == m_maxDepth)
				{
					nodes.emplace_back(getNodeIndex(*node));
					return false;
				}

				if (!node->hasChildren())
				{
					traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, node->m_levelIndexX << 1, node->m_levelIndexY << 1 });
					traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, (node->m_levelIndexX << 1) | 1, node->m_levelIndexY << 1 });
					traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, node->m_levelIndexX << 1, (node->m_levelIndexY << 1) | 1 });
					traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, (node->m_levelIndexX << 1) | 1, (node->m_levelIndexY << 1) | 1 });
					return false;
				}

				return true;
			}
			return false;
		});
	}
	return nodes;
}

std::vector<int> SphericalTerrain::getTreeNodeInLs(const AxisAlignedBox& box, int _maxDepth) const
{
	if (!isBoundAccurate()) return {};

	if (!m_bounds.front().intersects(box))
	{
		return {};
	}
	
	std::vector<int> nodes;
	std::function<void(const NodeInfo&)> traverseBoundTree = [&box, &nodes, this, &traverseBoundTree, _maxDepth](const NodeInfo& node) -> void
		{
			int curNodeIndex = getNodeIndex(node);
			if (getIndexBound(curNodeIndex).intersects(box))
			{
				if (node.depth == _maxDepth)
				{
					nodes.emplace_back(curNodeIndex);
					return;
				}
				traverseBoundTree({ node.slice, node.depth + 1, node.x << 1, node.y << 1 });
				traverseBoundTree({ node.slice, node.depth + 1, (node.x << 1) | 1, node.y << 1 });
				traverseBoundTree({ node.slice, node.depth + 1, node.x << 1, (node.y << 1) | 1 });
				traverseBoundTree({ node.slice, node.depth + 1, (node.x << 1) | 1, (node.y << 1) | 1 });
			}
		};

	for (auto&& slice : m_slices)
	{
		Node::traverseTree(slice.get(), [this, &box, &nodes, &traverseBoundTree, _maxDepth](const SphericalTerrainQuadTreeNode* node)
			{
				if (node->getBoundLocal().intersects(box))
				{
					if (node->m_depth == _maxDepth)
					{
						nodes.emplace_back(getNodeIndex(*node));
						return false;
					}

					if (!node->hasChildren())
					{
						traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, node->m_levelIndexX << 1, node->m_levelIndexY << 1 });
						traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, (node->m_levelIndexX << 1) | 1, node->m_levelIndexY << 1 });
						traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, node->m_levelIndexX << 1, (node->m_levelIndexY << 1) | 1 });
						traverseBoundTree({ static_cast<Face>(node->m_slice), node->m_depth + 1, (node->m_levelIndexX << 1) | 1, (node->m_levelIndexY << 1) | 1 });
						return false;
					}

					return true;
				}
				return false;
			});
	}
	return nodes;
}

void SphericalTerrain::generateVertexBuffer(Node& node)
{
	if (!node.m_vbDirty) return;

	if (m_vbGenAsync)
	{
		if (m_genVertexRequests.contains(&node)) return;
		if (m_currFrameRequests >= 8/*|| m_genVertexRequests.size() >= 4*/)
			return;

		auto& [listener, task, ticket] = m_genVertexRequests[&node] = {};
		listener.node = task.node = &node;
		ticket = CommonJobManager::instance()->addRequestCommonTask(&task, &listener);
		++m_currFrameRequests;
		return;
	}

	RcBuffer* vb = nullptr;
	std::vector<TerrainVertex> vertices;

#ifdef ECHO_EDITOR
	if (m_gpuMode)
	{
		assert(!m_useObb);
		vb = createRawVb();
		computeVertexGpu(vb, node);
		if (Root::instance()->getRenderSystem()->isUseSingleThreadRender())
		{
			std::vector<float> elevations;
			elevations.reserve(VERTEX_COUNT);
			const auto trans = node.getTranslationLocal();
			const auto vtx   = copyVertexBufferFromGpu(vb);
			AxisAlignedBox aabb = AxisAlignedBox::EXTENT_NULL;
			switch (m_topology)
			{
			case PTS_Sphere:
				std::transform(vtx.begin(), vtx.end(), std::back_inserter(elevations),
					[trans, &aabb, this](const auto& v)
					{
						auto pos = v.pos + trans;
						aabb.merge(pos);
						if (m_pTerrainGenerator)
						{
							Face face;
							Vector2 xy;
							Node::MapToCube(pos, face, xy);
							const auto height = m_pTerrainGenerator->getHeight(face, xy.x, xy.y);
							pos /= height;
						}
						return pos.length() - 1.0f;
					});
				break;
			case PTS_Torus:
				// https://www.geometrictools.com/Documentation/DistanceToCircle3.pdf
				const float invMinorRadius = 1.0f / m_relativeMinorRadius;
				std::transform(vtx.begin(), vtx.end(), std::back_inserter(elevations),
					[trans, invMinorRadius, &aabb](const auto& v)
					{
						constexpr float R     = 1.0f;
						const Vector3 delta   = v.pos + trans; // ∆ = P − C
						aabb.merge(delta);
						const float dotND     = delta.y; // N · ∆
						const Vector3 QmC     = delta - Vector3(0.0f, dotND, 0.0f); // Q − C
						const float lengthQmC = QmC.length(); // |Q − C|
						float sqrDistance;
						if (lengthQmC > 0)
						{
							// The circle point closest to P is unique.
							const auto crossND = // Vector3::UP.crossProduct(delta); // N × ∆
								Vector3(1.0f * delta.z, 0.0f, -1.0f * delta.x);
							const float radial = crossND.length() - R; // |N × ∆| − r
							sqrDistance        = dotND * dotND + radial * radial; // (N · ∆)^2 + (|N × ∆| − r)^2
						}
						else
						{
							// All circle points are equidistant from P .
							sqrDistance = delta.squaredLength() + R * R; // |∆|^2 + r2
						}
						const float distance = sqrt(sqrDistance);
						return distance * invMinorRadius - 1.0f;
					});
				break;
			}
			{
				std::unique_lock lock(node.m_elevationMutex);
				node.m_elevations = elevations;
			}
			{
				std::unique_lock lock(node.m_boundMutex);
				node.m_bound = aabb;
			}
		}
	}
	else
	{
		computeVertexCpu(node);
		vb       = createDynamicVb(node.m_vertices);
	}
#endif

	node.setVertexBuffer(vb);
	m_profile.meshMemory += VB_BYTE_SIZE;
	// if (node.m_vertexBuffer && node.m_root->m_maxDepth == node.m_depth)
		// m_regeneratedLeaves.push_back(&node);
}

void SphericalTerrain::cancelGenerateVertexTask(const Node& node)
{
	if (const auto request = m_genVertexRequests.find(&node); request != m_genVertexRequests.end())
	{
		auto& [listener, task, ticket] = request->second;
		CommonJobManager::instance()->removeRequest(ticket);
		CommonJobManager::instance()->removeListener(&listener);
		m_genVertexRequests.erase(request);
	}
}

void SphericalTerrain::cancelGenerateVertexTask()
{
	for (auto& [node, request] : m_genVertexRequests)
	{
		auto& [listener, task, ticket] = request;
		CommonJobManager::instance()->removeRequest(ticket);
		CommonJobManager::instance()->removeListener(&listener);
	}
	m_genVertexRequests.clear();
}

const AxisAlignedBox& SphericalTerrain::getIndexBound(int index) const {
	EchoAssert(!m_bounds.empty(), "SphericalTerrain::m_bounds is empty");
	return m_bounds[index + 1];
}

std::unordered_map<int, std::vector<uint32>> SphericalTerrain::calculateNodeIntersect(const std::vector<AxisAlignedBox>& boxes) const
{
	if (!isBoundAccurate())
	{
		return {};
	}

	std::unordered_map<int, std::vector<uint32>> result;

	size_t size = boxes.size();
	size = std::min(size, static_cast<size_t>(std::numeric_limits<uint32>::max()));
	for (uint32 iBounds = 0; iBounds < size; ++iBounds)
	{
		const auto& box = boxes[iBounds];

		std::function<void(const NodeInfo&)> traverseBoundTree = [&](const NodeInfo& node) -> void
		{
			const int index = getNodeIndex(node);
			if (!getIndexBound(index).intersects(box)) { return; }

			result[index].emplace_back(iBounds);

			if (node.depth == m_maxDepth) { return; }

			traverseBoundTree({ .slice = node.slice, .depth = node.depth + 1, .x = node.x << 1, .y = node.y << 1 });
			traverseBoundTree({ .slice = node.slice, .depth = node.depth + 1, .x = (node.x << 1) | 1, .y = node.y << 1 });
			traverseBoundTree({ .slice = node.slice, .depth = node.depth + 1, .x = node.x << 1, .y = (node.y << 1) | 1 });
			traverseBoundTree({ .slice = node.slice, .depth = node.depth + 1, .x = (node.x << 1) | 1, .y = (node.y << 1) | 1 });
		};

		for (int iSlice = 0; iSlice < m_numSlice[0] * m_numSlice[1]; ++iSlice)
		{
			traverseBoundTree({ .slice = iSlice, .depth = 0, .x = 0, .y = 0 });
		}
	}

	return result;
}

void SphericalTerrain::setDissolveStrength(float value)
{
	m_fDissolveStrength = value;
	if (m_cloud)
	{
		m_cloud->m_dissolveStrength = value;
	}
}

void SphericalTerrain::addLoadListener(LoadListener* listener) {
	if (listener == nullptr) return;
	auto iter = std::find(m_LoadListeners.begin(), m_LoadListeners.end(), listener);
	if (iter == m_LoadListeners.end()) {
		m_LoadListeners.push_back(listener);
	}
}
void SphericalTerrain::removeLoadListener(LoadListener* listener) {
	if (listener == nullptr) return;
	auto iter = std::find(m_LoadListeners.begin(), m_LoadListeners.end(), listener);
	if (iter != m_LoadListeners.end()) {
		m_LoadListeners.erase(iter);
	}
}
void Echo::SphericalTerrain::setPlanetCloudId(int32_t id)
{
	id = std::max(-1, id);
	id = std::min(63, id);

	if (m_CloudTemplateId != id)
	{
#ifdef ECHO_EDITOR
		if (m_cloud)
			delete m_cloud;
		m_cloud = nullptr;
#endif


		m_CloudTemplateId = id;
		if (m_CloudTemplateId == -1)
		{
			return;
		}

		PlanetCloudTemplateData data;

		if (!SphericalTerrainResourceManager::instance()->getCloudTemplateFromLibrary(m_CloudTemplateId, data))
		{
			StringStream ss;
			ss << "Cloud Template ID Out of CloudLibrary Size : " << m_CloudTemplateId << std::endl;
			LogManager::instance()->logMessage(ss.str());
		}
		else
		{
			if (!m_cloud)
			{
				m_cloud = new PlanetLowLODCloud(this);
				m_cloud->init();
			}
			setCloudUVScale(data.m_UVScale);
			setCloudFlowSpeed(data.m_flowSpeed);
			setCloudHDRFactor(data.m_hdr);
			setCloudMetallic(data.m_metallic);
			setCloudRoughness(data.m_roughness);
			setCloudAmbientFactor(data.m_ambientFactor);
			setCloudNoiseOffsetScale(data.m_noiseOffsetScale);
			setCloudDirByPitchAndYaw(data.m_pitch,data.m_yaw);
			if (!data.m_DiffusePath.empty())
				setCloudTexture(data.m_DiffusePath);
		}

	}

}
void Echo::SphericalTerrain::BuildPlanetRoad(bool sync, bool reBuild)
{
	//if (m_terrainData->mPlanetRoad.isNull() || m_terrainData->mPlanetRoad->getPlanetRoadGroup().roads.empty())
	//{
	//	if (mRoad)
	//		mRoad->clearPlanetRoadCache();
	//	return;
	//}
	if (m_terrainData.isNull())
		return;
	if (!mRoad)
	{
		std::string resName = m_terrainData->getName().toString();
		if (resName.empty())
		{
			LogManager::instance()->logMessage("SphericalTerrain::BuildPlanetRoad() planet res file path is empty.");
			return;
		}
		if (resName.find(".terrain") == String::npos)
		{
			resName.append(".planetroad");
		}
		else if (resName.find_last_of(".") != String::npos)
		{
			resName.erase(resName.find_last_of("."));
			resName.append(".planetroad");
		}
		if (Root::instance()->TestResExist(resName.c_str()))
		{
			mRoad = new PlanetRoad();
			mRoad->init(this, sync);
		}
		else
		{
			LogManager::instance()->logMessage("PlanetRoad::init() can not find file from asset.");
			return;
		}
	}
	else if (reBuild)
		mRoad->rebuildPlanetRoadCache();
}
void SphericalTerrain::onCreateFinish() {
	if (m_bCreateFinish) {
		return;
	}
	m_bCreateFinish = true;

	auto listeners = m_LoadListeners;
	for (LoadListener* listener : listeners)
	{
		listener->OnCreateFinish();
	}
}

void SphericalTerrain::onDestroy()
{
	m_bCreateFinish = false;

	for (LoadListener* listener : m_LoadListeners)
	{
		listener->OnDestroy();
	}
	m_LoadListeners.clear();
}

void SphericalTerrain::operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) {
	if (ticket != mLoadTicket) return;
	m_bWaitCreateFinish = false;
	if (resourcePtr.isNull()) return;
	SphericalTerrainResourcePtr resPtr = SphericalTerrainResourcePtr(resourcePtr);
	try 
	{
		resPtr->load();
		createImpl(resPtr);
	}
	catch (...)
	{
		LogManager::instance()->logMessage("SphericalTerrain::init [" + request.name.toString() + "] failed.");
	}
}

void SphericalTerrain::requestResource(const Name& name)
{
	clearRequest();
	mLoadTicket = ResourceBackgroundQueue::instance()->prepare(SphericalTerrainResourceManager::instance()->getResourceType(), name,
		Echo::WorkQueue::MakePriority(0, Echo::WorkQueue::RESOURCE_TYPE_TERRAIN),
		false, NULL, NULL, this);
	m_bWaitCreateFinish = true;

}
void SphericalTerrain::clearRequest() 
{
	if (!m_bWaitCreateFinish) return;
	ResourceBackgroundQueue::instance()->abortRequest(mLoadTicket);
}

std::vector<Vector3> SphericalTerrain::generateVertexBufferPhysX(const int index)
{
	return generatePositionWithCache(getNodeInfo(index, m_maxDepth), index);
	return ProceduralSphere::generateVertexBufferPhysX(index);
}

std::vector<Vector3> SphericalTerrain::generatePositionWithCache(const NodeInfo& info, const int index)
{
	std::vector<Vector3> baseShape;
	if (isSphere())
	{
		const Vector2 len(static_cast<float>(Node::getLengthOnCube(info.depth)));
		const Vector2 offset = len * Vector2(static_cast<float>(info.x), static_cast<float>(info.y)) - 1.0f;
		const Vector2 gridSize(static_cast<float>(Node::getGridSizeOnCube(info.depth)));
		baseShape = Node::getSphericalPositions(info.slice, GRID_RESOLUTION, GRID_RESOLUTION,
			gridSize, offset, m_pTerrainGenerator);
	}
	else if (isTorus())
	{
		auto [size, offset] = TorusTerrainNode::getGridSizeOffset(info.slice, m_numSlice[0], m_numSlice[1],
			info.depth, info.x, info.y);
		baseShape = TorusTerrainNode::getTorusPositions(
			GRID_RESOLUTION, GRID_RESOLUTION,
			m_relativeMinorRadius,
			size, offset);
	}

	Vector3 invTrans = -getNodeOriginLocal(index);

	std::vector<Vector3> pos(VERTEX_COUNT);
	bool cached = false, cachedModifiers = false;
	std::set<uint64_t> modifiers;
	if (m_leafMutex.try_lock_shared())
	{
		const auto& leaves = m_leaves;
		if (const auto it = leaves.find(index); it != leaves.end())
		{
			if (auto& [id, node] = *it; !node->m_vbDirty && node->m_elevationMutex.try_lock_shared())
			{
				modifiers = node->m_modifierInstances;
				cachedModifiers = true;
				if (!node->m_elevations.empty())
				{
					applyElevations(pos.data(), baseShape.data(), node->m_elevations.data(), VERTEX_COUNT);

					std::transform(pos.begin(), pos.end(), pos.begin(), [invTrans](const auto& p) { return p + invTrans; });
					cached = true;
				}
				node->m_elevationMutex.unlock_shared();
			}
		}
		m_leafMutex.unlock_shared();
	}

	if (!cached)
	{
		std::vector<float> elevations(VERTEX_COUNT);
		generateElevations(baseShape.data(), elevations.data(), baseShape.size());
		std::transform(elevations.begin(), elevations.end(), elevations.begin(),
			[this](const float e) { return e * m_elevationRatio; });

		applyElevations(pos.data(), baseShape.data(), elevations.data(), VERTEX_COUNT);

		computeStaticModify(index, pos.data(), baseShape.data(), elevations.data(), VERTEX_COUNT);

		if (isSphere() && cachedModifiers)
		{
			m_modifier->Evaluate(modifiers, pos, baseShape, elevations);
		}
		else if (isSphere())
		{
			const auto* node           = m_slices.at(info.slice).get();
			const auto* parentInstance = &node->m_modifierInstances;
			while (node->hasChildren())
			{
				const int quadX = (info.x >> std::max(0, m_maxDepth - 1 - node->m_depth)) & 1;
				const int quadY = (info.y >> std::max(0, m_maxDepth - 1 - node->m_depth)) & 1;
				const auto quad = static_cast<Quad>(quadX | quadY << 1);
				node            = node->getChild(quad);
				parentInstance  = &node->m_modifierInstances;
			}

			m_modifier->Evaluate(m_modifier->TestIntersection(getIndexBound(index), *parentInstance),
				pos, baseShape, elevations);
		}

		std::transform(pos.begin(), pos.end(), pos.begin(), [invTrans](const auto& p) { return p + invTrans; });
	}

	// OutputDebugStringA(cached ? "generated form cache\n" : "generated form pcg\n");

	return pos;
}

void SphericalTerrain::generateVertexBufferBenchmark()
{
	Timer timer;
	std::string message;
	const auto start = timer.getMilliseconds();
	constexpr int repeat = 16;
	constexpr int repeat26 = repeat * repeat * 6;
	double dummy = 0;
	for (int f = 0; f < 6; ++f)
	{
		for (int i = 0; i < repeat; ++i)
		{
			for (int j = 0; j < repeat; ++j)
			{
				NodeInfo info { f, 4, i, j };
				auto pos = ProceduralSphere::generateVertexBufferPhysX(getNodeIndex(info));
				dummy += pos[42].LengthSq() - 1.0f;
			}
		}
	}

	const auto end = timer.getMilliseconds();
	const float diff = static_cast<float>(end - start);
	const float tilesPerSecond = repeat26 / (diff / 1000.0f);
	const float secsPerTile = diff / repeat26;
	const float millionSamplesPerSec = tilesPerSecond * VERTEX_COUNT / 1000000.0f;
	 message = message + "SphericalTerrain::generateVertexBufferBenchmark: " + std::to_string(dummy) + " ";
	 message = message + std::to_string(diff) + "ms" +
		" " + std::to_string(tilesPerSecond) + " tiles/s" +
		" " + std::to_string(secsPerTile) + "ms/tile" +
		" " + std::to_string(millionSamplesPerSec) + " million samples/s";
	LogManager::instance()->logMessage(message);
}

void SphericalTerrain::surfaceQueryBenchmark() const
{
	Timer timer;
	const auto start = timer.getMicroseconds();
	Vector3 dummy;
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			for (int k = 0; k < 100; ++k)
			{
				auto pos = Vector3(-1.0f + 2.0f * i / 99.0f, -1.0f + 2.0f * j / 99.0f, -1.0f + 2.0f * k / 99.0f);
				dummy += getSurfaceLs(pos);
			}
		}
	}
	const auto end     = timer.getMicroseconds();
	const auto elapsed = end - start;
	LogManager::instance()->logMessage("SphericalTerrain::surfaceQueryBenchmark: " + std::to_string(elapsed) + " us" +
		" dummy: " + std::to_string(dummy.x) + " " + std::to_string(dummy.y) + " " + std::to_string(dummy.z) +
		" us per query: " + std::to_string(static_cast<float>(elapsed) / 100.f / 100.f / 100.f) + " us");
}

Noise3DData SphericalTerrain::get3DNoiseData()
{
	Noise3DData result = m_terrainData->geometry.noise;
	return result;
}

bool SphericalTerrain::isVegetationCheckValid() const
{
	std::shared_lock lock(m_vegetationCheckMutex);
	return static_cast<bool>(m_vegetationCheckFunction);
}

void SphericalTerrain::setVegetationCheckFunction(VegetationQueryFunc&& func)
{
	std::unique_lock lock(m_vegetationCheckMutex);
	m_vegetationCheckFunction = std::move(func);
}

std::vector<bool> SphericalTerrain::getOnSurface(std::span<Vector3> worldPositions) const
{
	std::shared_lock lock(m_vegetationCheckMutex);
	if (!static_cast<bool>(m_vegetationCheckFunction)) return std::vector(worldPositions.size(), true);
	return m_vegetationCheckFunction(worldPositions);
}

RcSampler* SphericalTerrain::getBiomeIdTexture(const int slice)
{
#ifdef ECHO_EDITOR
	if (m_editingTerrain)
	{
		return getMaterialIndexComputeTargets()[slice];
	}
#endif

	const auto& terrainData = m_terrainData;
	if (terrainData.isNull() || !terrainData->m_loadBiomeTexture) return nullptr;
	const auto& planetTexRes = terrainData->m_planetTexRes;
	if (planetTexRes.isNull()) return nullptr;
	const auto& tex = planetTexRes->m_biomeGPUData.matIDTex[slice];
	if (tex.isNull()) return nullptr;
	return tex->getRcTex();
}

TexturePtr SphericalTerrain::loadFbmTexture()
{
	if (s_fbmNoiseTexture.isNull())
	{
		s_fbmNoiseTexture = TextureManager::instance()->loadTexture(s_fbmPathStr, TEX_TYPE_2D);
	}
	return s_fbmNoiseTexture;
}

void SphericalTerrain::freeFbmTexture()
{
	if (s_count == 0 && !s_fbmNoiseTexture.isNull())
	{
		s_fbmNoiseTexture = nullptr;
		TextureManager::instance()->remove(s_fbmPathStr);
	}
}

//TexturePtr SphericalTerrain::loadVoronoiTexture()
//{
//	if (s_voronoiTexture.isNull())
//	{
//		s_voronoiTexture = TextureManager::instance()->loadTexture(s_voronoiPathStr, TEX_TYPE_2D);
//	}
//	return s_voronoiTexture;
//}
//
//void SphericalTerrain::freeVoronoiTexture()
//{
//	if (s_count == 0 && !s_voronoiTexture.isNull())
//	{
//		s_voronoiTexture = nullptr;
//		TextureManager::instance()->remove(s_voronoiPathStr);
//	}
//}

uint32
SphericalTerrain::getPhySurfaceType(const DVector3& worldPos) const
{
	uint32 result = 0;
	
	int biomeIndex = getSurfaceTypeWs(worldPos);
	if(biomeIndex != -1 && !m_terrainData.isNull())
	{
		if(biomeIndex < m_terrainData->composition.usedBiomeTemplateList.size())
		{
			auto& biomeTemp = m_terrainData->composition.usedBiomeTemplateList[biomeIndex];
			result = biomeTemp.phySurfaceType;
		}
	}
	return (result);
}

void SphericalTerrain::forceAllComputePipelineExecute()
{
#ifdef ECHO_EDITOR
	// pipeline compilation may happen on async thread, need to check if it's ready
	if (!isComputePipelineReady())
		return;

	if (!m_editingTerrain) return;

	if (m_gpuMode && m_biomeDirty)
	{
		computeElevationGPU();
		computeHumidityGPU();
		computeTemperatureGPU();
		computeMaterialIndexGPU();
		copyMaterialIndexFromGpuMemory();
		m_biomeDefineDirty = false;
		m_biomeDirty = false;
		return;
	}

	if (m_gpuMode && m_biomeDefineDirty)
	{
		computeMaterialIndexGPU();
		copyMaterialIndexFromGpuMemory();
		m_biomeDefineDirty = false;
	}
#endif
}

void SphericalTerrain::setEnableRegionVisualEffect(bool enable)
{
	m_enableRegionVisualEffect = enable;

    //IMPORTANT:Async load region ID & sdf texture.
    if(m_warfog)
    {
        if(m_enableRegionVisualEffect)
        {
            m_warfog->asyncLoadTexture();
        }
        else
        {
            m_warfog->freeAsyncLoadTexture();
        }
    }
}

void SphericalTerrain::setDisplayCorseOrFineRegion(bool enableCorse)
{
    if(m_warfog)
    {
        m_warfog->m_displayCorseRegion = enableCorse;
    }
}

bool SphericalTerrain::setHighlightCorseRegionID(int id)
{
	bool result = false;
	if(id < m_terrainData->sphericalVoronoiRegion.defineBaseArray.size())
	{
        if(m_warfog)
        {
            m_warfog->m_highlightCorseRegionID = (float)id;
        }
		result = true;
	}
	else
	{
		LogManager::instance()->
			logMessage("SphericalTerrain::setHighlightCorseRegionID id: ["+ std::to_string(id) + "]is out of corse region range!",
					   LML_NORMAL);
	}
	return (result);
}

bool SphericalTerrain::setHighlightFineRegionID(int id)
{
	bool result = false;
	if(id < m_terrainData->sphericalVoronoiRegion.defineArray.size())
	{
        if(m_warfog)
        {
            m_warfog->m_highlightRegionID = (float)id;
        }
		result = true;
	}
	else
	{
		LogManager::instance()->
			logMessage("SphericalTerrain::setHighlightCorseRegionID id: ["+ std::to_string(id) + "]is out of corse region range!",
					   LML_NORMAL);
	}
	return (result);
}

void SphericalTerrain::setRegionGlowColor(const ColorValue& color)
{
    if(m_warfog)
    {
        m_warfog->m_outlineColor = color;
    }
}

void SphericalTerrain::setRegionHighlightColor(const ColorValue& color)
{
    if(m_warfog)
    {
        m_warfog->m_highlightRegionColor = color;
    }
}
 
void SphericalTerrain::setOutlineWidth(float width)
{
    //const float min = 0.1f;
    //const float max = 0.7f;
    //const float delta = (max -min);
    //width = EchoClamp(0.0f, 1.0f, width);
    if(m_warfog)
    {
        m_warfog->m_outlineWidth = width;// * delta + min;
    }
}

void SphericalTerrain::disableAtmosphere() {
	// 如果当前正在tick的大气雾效来自此星球，置空指针
	RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
	if (pRenderSystem) {
		RenderStrategy* pCurRenderStrategy = pRenderSystem->getCurRenderStrategy();
		if (pCurRenderStrategy) {
			PostProcess* process = pCurRenderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
			if (process) {
				SphericalTerrainFog* sphFog = static_cast<SphericalTerrainFog*>(process);
				if (sphFog->m_SphericalTerrain == this)
					sphFog->setTerrain(nullptr);
			}
		}
	}
}
void SphericalTerrain::setCommonRegionAlpha(float alpha)
{
    alpha = EchoClamp(0.0f, 1.0f, alpha);
    if(m_warfog)
    {
        m_warfog->m_commonColorBlendFactor = alpha;
    }
}

void SphericalTerrain::setHighlightRegionAlpha(float alpha)
{
    alpha = EchoClamp(0.0f, 1.0f, alpha);
    if(m_warfog)
    {
        m_warfog->m_highlightRegionColorBlendFactor = alpha;
    }
}

void SphericalTerrain::setRegionWarfogColor(const ColorValue& color)
{
    if(m_warfog)
    {
        m_warfog->m_warFogColor = color;
    }
}

void SphericalTerrain::setRegionFillColor(const ColorValue& in_color)
{
    if(!m_warfog)
        return;
    
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif
    
    Bitmap& maskMap = m_warfog->m_regionWarFogMaskCube;
    if(maskMap.pixels)
    {
        uint8 r,g,b;
        r = (uint8)(EchoClamp(0.0f, 1.0f, in_color.r) * 255.0f);
        g = (uint8)(EchoClamp(0.0f, 1.0f, in_color.g) * 255.0f);
        b = (uint8)(EchoClamp(0.0f, 1.0f, in_color.b) * 255.0f);
        uint32 color = r << 0 |
            g << 8 |
            b << 16;
        
        int pixelCount = maskMap.width * maskMap.height; 
        for(int pixelIndex = 0;
            pixelIndex < pixelCount;
            pixelIndex++)
        {
            uint32* pixel = (uint32*)maskMap.pixels;
            uint32 newPixel = (pixel[pixelIndex] & 0xff000000) | color;
            pixel[pixelIndex] = newPixel;
        }
                
        if(!m_warfog->m_regionWarFogMaskCubeDirty)     
            m_warfog->m_regionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("fine region war fog texture color full update time ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

void SphericalTerrain::setCoarseRegionFillColor(const ColorValue& in_color)
{
#define Profile_Time 0
#if Profile_Time
    unsigned long updateTime;

    Timer* timer = Root::instance()->getTimer();
    updateTime = timer->getMicroseconds();
#endif
    
    if(!m_warfog)
        return;

    Bitmap& maskMap = m_warfog->m_coarseRegionWarFogMaskCube;
    if(maskMap.pixels)
    {
        uint8 r,g,b;
        r = (uint8)(EchoClamp(0.0f, 1.0f, in_color.r) * 255.0f);
        g = (uint8)(EchoClamp(0.0f, 1.0f, in_color.g) * 255.0f);
        b = (uint8)(EchoClamp(0.0f, 1.0f, in_color.b) * 255.0f);
        uint32 color = r << 0 |
            g << 8 |
            b << 16;

        int pixelCount = maskMap.width * maskMap.height; 
        for(int pixelIndex = 0;
            pixelIndex < pixelCount;
            pixelIndex++)
        {
            uint32* pixel = (uint32*)maskMap.pixels;
            uint32 newPixel = (pixel[pixelIndex] & 0xff000000) | color;
            pixel[pixelIndex] = newPixel;
        }
        
        if(!m_warfog->m_coarseRegionWarFogMaskCubeDirty)
            m_warfog->m_coarseRegionWarFogMaskCubeDirty = true;
    }

#if Profile_Time
    updateTime = timer->getMicroseconds() - updateTime;
    String log = 
        String("Coarse region war fog texture color full update time[ ") +
        std::to_string(updateTime) +String(" micro second ") +
        std::to_string(updateTime / 1000) +String(" ms\n ]");
    logToConsole(log.c_str());
#endif
}

void SphericalTerrain::setRegionHighlightWarfogColor(const ColorValue& color)
{
    if(m_warfog)
    {
        m_warfog->m_highlightWarfogColor = color;
    }
}

void SphericalTerrain::setWarfogMaskUVScale(const float scale)
{
    if(m_warfog)
    {
        m_warfog->m_warFogMaskUVScale = scale;
    }
}

void SphericalTerrain::setWarfogMaskScale(const float scale)
{
    if(m_warfog)
    {
        m_warfog->m_warFogMaskScale = scale;
    }
}

void SphericalTerrain::setOutlineSmoothRange(const float range)
{
    if(m_warfog)
    {
        m_warfog->m_outlineSmoothRange = range;
    }
}

void SphericalTerrain::setAdditionOutLineColor(const ColorValue& color)
{
    if(m_warfog)
    {
        m_warfog->m_additionOutlineColor = color;
    }   
}

void SphericalTerrain::setAdditionOutLineAlpha(const float& v)
{
    if(m_warfog)
    {
        m_warfog->m_additionOutlineAlpha = v;
    }   
}

void SphericalTerrain::setAdditionOutLineWidth(const float& v)
{
    if(m_warfog)
    {
        m_warfog->m_additionOutlineWidth = v;
    }
}

void SphericalTerrain::setAdditionOutLineDistance(const float& v)
{
    if(m_warfog)
    {
        m_warfog->m_additionOutlineDistance = v;
    }
}

void SphericalTerrain::setWarfogMaskTexture(const String& path)
{
    if(m_warfog)
    {
        m_warfog->m_warfogMaskTex = m_mgr->asyncLoadTexture(Name(path));
    }
}

void Echo::SphericalTerrain::setWarFogSelfEmissiveLightColor(const ColorValue& color_)
{
	if (m_warfog)
	{
		m_warfog->setSelfEmissiveLightColor(color_);
	}
}

void Echo::SphericalTerrain::setWarFogSelfEmissiveLightPower(float power)
{
	if (m_warfog)
	{
		m_warfog->setSelfEmissiveLightPower(power);
	}
}

void Echo::SphericalTerrain::setWarFogIsUseFlowingLight(bool bUse)
{
	if (m_warfog)
		m_warfog->setIsUseFlowingLight(bUse);
}

void Echo::SphericalTerrain::setWarFogFlowingDirByYawAndPitch(float yaw, float pitch)
{
	if (m_warfog)
	{
		pitch = std::max(-90.f, std::min(90.f, pitch));
		yaw = std::max(0.f, std::min(360.f, yaw));

	    m_warfog->setFlowingDir(TODManager::convertDirectionFromYawPitchAngle(yaw, pitch));
	}
}

void Echo::SphericalTerrain::setWarFogFlowingDir(const Vector3& dir)
{
	if (m_warfog)
		m_warfog->setFlowingDir(dir);
}

void Echo::SphericalTerrain::setWarFogFlowingWidth(float width)
{
	if (m_warfog)
		m_warfog->setFlowingWidth(width);
}

void Echo::SphericalTerrain::setWarFogFlowingSpeed(float speed)
{
	if (m_warfog)
		m_warfog->setFlowingSpeed(speed);
}

void Echo::SphericalTerrain::setWarFogFlowingColor(const ColorValue& color)
{
	if (m_warfog)
		m_warfog->setFlowingColor(color);
}

void Echo::SphericalTerrain::setWarFogFlowingHDRFactor(float factory_)
{
	if (m_warfog)
		m_warfog->setFlowingHDRFactor(factory_);
}

void Echo::SphericalTerrain::setWarFogDissolveTexture(const std::string& path)
{
	if (m_warfog)
	{
		m_warfog->m_dissolveTex = m_mgr->asyncLoadTexture(Name(path));
	}
}



void Echo::SphericalTerrain::setWarFogDissolveTillingScale(float scale)
{
	if (m_warfog)
		m_warfog->setDissolveTillingScale(scale);
}

void Echo::SphericalTerrain::setWarFogDissolveSoftValue(float value)
{
	if (m_warfog)
		m_warfog->setDissolveSoftValue(value);
}
void Echo::SphericalTerrain::setWarFogDissolveWidth(float width)
{
	if (m_warfog)
		m_warfog->setDissolveWidth(width);
}
void Echo::SphericalTerrain::setWarFogDissolveColor(const Vector3& color)
{
	if (m_warfog)
		m_warfog->setDissolveColor(color);
}
void Echo::SphericalTerrain::setWarFogDissolveColorValue(float value)
{
	if (m_warfog)
		m_warfog->setDissolveColorValue(value);
}

void Echo::SphericalTerrain::setWarFogDissolveValue(int id, float value)
{
	if (m_warfog)
		m_warfog->setDissolveValue(id, value);
}

void SphericalTerrain::setCloudTexture(const String& path)
{
    if(m_cloud)
    {
        m_cloud->m_diffuseTex = m_mgr->asyncLoadTexture(Name(path));
    }
}

void Echo::SphericalTerrain::setCloudDirByPitchAndYaw(float pitch, float yaw)
{
	if (m_cloud)
	{
		pitch = std::max(-90.f, std::min(90.f, pitch));
		yaw = std::max(0.f, std::min(360.f, yaw));

		m_cloud->m_dir = TODManager::convertDirectionFromYawPitchAngle(yaw, pitch);
		m_cloud->m_dir.normalise();
	}
}

void Echo::SphericalTerrain::setEnablePlotDivision(bool bEnable)
{
	m_bEnablePlotDivision = bEnable;
}

void Echo::SphericalTerrain::setPlotDivisionColor(int idx, int x, int y, const ColorValue& color)
{
	if (m_PlotDivision)
		m_PlotDivision->setPlotDivisionColor(idx, x, y, color);
}

void Echo::SphericalTerrain::setPlotDivisionNums(int x, int y)
{
	if (m_PlotDivision)
		m_PlotDivision->setPlotDivisionNums( x, y);
}

bool Echo::SphericalTerrain::getPlotDivisionDataByPosition(const Vector3& pos, int& idx, int& x, int& y)
{
	if (!m_PlotDivision)
	{
		stringstream ss;
		ss << "PlotDivision not init"<<std::endl;
		LogManager::instance()->logMessage(ss.str());
		return false;
	}
	if (m_terrainData.isNull())
	{
		stringstream ss;
		ss << "Spherical init false" << std::endl;
		LogManager::instance()->logMessage(ss.str());
		return false;
	}
    
	if (m_terrainData->geometry.topology == ProceduralTerrainTopology::PTS_Sphere)
	{
		Vector2 resXY;
		Node::MapToCube(pos, idx, resXY);
		resXY.x = (resXY.x + 1.f) * 0.5f;
		resXY.y = (resXY.y + 1.f) * 0.5f;
		x = static_cast<int>(m_PlotDivision->m_texWidth * resXY.x * m_PlotDivision->m_XFactory);
		y = static_cast<int>(m_PlotDivision->m_texHeight * resXY.y * m_PlotDivision->m_YFactory);
		return true;
	}
	else if (m_terrainData->geometry.topology == ProceduralTerrainTopology::PTS_Torus)
	{
		idx = 0;
		Vector2 resXY = TorusTerrainNode::MapToPlane(pos);
		x = static_cast<int>(m_PlotDivision->m_texWidth * resXY.x * m_PlotDivision->m_XFactory);
		y = static_cast<int>(m_PlotDivision->m_texHeight * resXY.y * m_PlotDivision->m_YFactory);
		return true;
	}
	return false;
}

bool Echo::SphericalTerrain::getPlotDivisionDataByWorldPosition(const DVector3& pos, int& idx, int& x, int& y)
{
	Vector3 localPos = ToLocalSpace(pos);
	return getPlotDivisionDataByPosition(localPos,idx,x,y);
}

void Echo::SphericalTerrain::setEnableVolumetricCloud(const bool& val)
{
	m_bEnableVolumetricCloud = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->IsVisible = val;
		});
}

void Echo::SphericalTerrain::setLayerBottomAltitudeKm(const float& val)
{
	m_fLayerBottomAltitudeKm = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->LayerBottomAltitudeKm = val;
		});
}

void Echo::SphericalTerrain::setLayerHeightKm(const float& val)
{
	m_fLayerHeightKm = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->LayerHeightKm = val;
		});
}

void Echo::SphericalTerrain::setTracingStartMaxDistance(const float& val)
{
	m_fTracingStartMaxDistance = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->TracingStartMaxDistance = val;
		});
}

void Echo::SphericalTerrain::setTracingMaxDistance(const float& val)
{
	m_fTracingMaxDistance = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->TracingMaxDistance = val;
		});
}

void Echo::SphericalTerrain::setCloudBasicNoiseScale(const float& val)
{
	m_fCloudBasicNoiseScale = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->CloudBasicNoiseScale = val;
		});
}

void Echo::SphericalTerrain::setStratusCoverage(const float& val)
{
	m_fStratusCoverage = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->StratusCoverage = val;
		});
}

void Echo::SphericalTerrain::setStratusContrast(const float& val)
{
	m_fStratusContrast = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->StratusContrast = val;
		});
}

void Echo::SphericalTerrain::setMaxDensity(const float& val)
{
	m_fMaxDensity = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->MaxDensity = val;
		});
}

void Echo::SphericalTerrain::setCloudPhaseG(const float& val)
{
	m_fCloudPhaseG = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->CloudPhaseG = val;
		});
}

void Echo::SphericalTerrain::setCloudPhaseG2(const float& val)
{
	m_fCloudPhaseG2 = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->CloudPhaseG2 = val;
		});
}

void Echo::SphericalTerrain::setCloudPhaseMixFactor(const float& val)
{
	m_fCloudPhaseMixFactor = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->CloudPhaseMixFactor = val;
		});
}

void Echo::SphericalTerrain::setMsScattFactor(const float& val)
{
	m_fMsScattFactor = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->MsScattFactor = val;
		});
}

void Echo::SphericalTerrain::setMsExtinFactor(const float& val)
{
	m_fMsExtinFactor = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->MsExtinFactor = val;
		});
}

void Echo::SphericalTerrain::setMsPhaseFactor(const float& val)
{
	m_fMsPhaseFactor = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->MsPhaseFactor = val;
		});
}

void Echo::SphericalTerrain::setWindOffset(const Vector3& val)
{
	m_vWindOffset = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		cloud->WindOffset = Vector4(val.x, val.y, val.z, 1.0f);
		});
}

void Echo::SphericalTerrain::setCloudColor(const ColorValue& val)
{
	m_cCloudColor = val;
	applyToVolumetricCloud([val](EchoVolumetricCloud* cloud) {
		ColorValue cloudcolor = ColorValue::gammaToLinear(val);
		cloud->CloudColor = Vector4(cloudcolor.r, cloudcolor.g, cloudcolor.b, 0.0f);
		});
}

void Echo::SphericalTerrain::applyToVolumetricCloud(const std::function<void(EchoVolumetricCloud*)>& func)
{
#ifdef ECHO_EDITOR
	if (isSphere())
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem != nullptr)
		{
			RenderStrategy* pCurRenderStrategy = pRenderSystem->getCurRenderStrategy();
			if (pCurRenderStrategy != nullptr)
			{
				PostProcess* pc = pCurRenderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::VolumetricCloud);
				if (pc != nullptr)
				{
					EchoVolumetricCloud* pCloud = dynamic_cast<EchoVolumetricCloud*>(pc);
					if (pCloud != nullptr && m_mgr->getNearestPlanet() == this)
					{
						func(pCloud);
					}
				}
			}
		}
	}
#endif
}

void Echo::SphericalTerrain::UpdateVolumetricCloudParam(const Camera* pCamera)
{
	Vector3 cameraPos = pCamera->getPosition();

	float cameraDistance = cameraPos.distance(Vector3(static_cast<float>(m_pos.x), static_cast<float>(m_pos.y), static_cast<float>(m_pos.z)));

	bool IsRender = (cameraDistance < 1500.0f * m_fTracingStartMaxDistance) ? true : false;

	if (m_bEnableVolumetricCloud && m_bVolumetricCloudProerptyDirty && isSphere() && IsRender)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem != nullptr)
		{
			RenderStrategy* pCurRenderStrategy = pRenderSystem->getCurRenderStrategy();
			if (pCurRenderStrategy != nullptr)
			{
				PostProcess* pc = pCurRenderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::VolumetricCloud);
				if (pc != nullptr)
				{
					EchoVolumetricCloud* pCloud = dynamic_cast<EchoVolumetricCloud*>(pc);
					if (pCloud != nullptr)
					{
						pCloud->IsVisible = true;
						pCloud->PlanetPosition = Vector4(static_cast<float>(m_pos.x), static_cast<float>(m_pos.y), static_cast<float>(m_pos.z), 1.f);
						pCloud->PlanetRadiusKm = getMaxRealRadius() / 1000.0f;
						pCloud->LayerBottomAltitudeKm = m_fLayerBottomAltitudeKm;
						pCloud->LayerHeightKm = m_fLayerHeightKm;
						pCloud->TracingStartMaxDistance = m_fTracingStartMaxDistance;
						pCloud->TracingMaxDistance = m_fTracingMaxDistance;

						pCloud->CloudBasicNoiseScale = m_fCloudBasicNoiseScale;
						pCloud->StratusCoverage = m_fStratusCoverage;
						pCloud->StratusContrast = m_fStratusContrast;
						pCloud->MaxDensity = m_fMaxDensity;

						pCloud->CloudPhaseG = m_fCloudPhaseG;
						pCloud->CloudPhaseG2 = m_fCloudPhaseG2;
						pCloud->CloudPhaseMixFactor = m_fCloudPhaseMixFactor;
						pCloud->MsScattFactor = m_fMsScattFactor;
						pCloud->MsExtinFactor = m_fMsExtinFactor;
						pCloud->MsPhaseFactor = m_fMsPhaseFactor;

						ColorValue cloudcolor = ColorValue::gammaToLinear(m_cCloudColor);
						pCloud->CloudColor = Vector4(cloudcolor.r, cloudcolor.g, cloudcolor.b, 0.0f);

						m_bVolumetricCloudProerptyDirty = false;
					}
				}
			}
		}
	}

	if (!m_bVolumetricCloudProerptyDirty && !IsRender)
	{
		m_bVolumetricCloudProerptyDirty = true;
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem != nullptr)
		{
			RenderStrategy* pCurRenderStrategy = pRenderSystem->getCurRenderStrategy();
			if (pCurRenderStrategy != nullptr)
			{
				PostProcess* pc = pCurRenderStrategy->getPostProcessChain()->getPostProcess(PostProcess::Type::VolumetricCloud);
				if (pc != nullptr)
				{
					EchoVolumetricCloud* pCloud = dynamic_cast<EchoVolumetricCloud*>(pc);
					if (pCloud != nullptr)
					{
						pCloud->IsVisible = false;
					}
				}
			}
		}
	}
}

void SphericalTerrain::setEnableLowLODCloud(bool enalbe)
{
    m_enableLowLODCloud = enalbe;
}

void SphericalTerrain::setCloudUVScale(float v)
{
    if(m_cloud)
    {
        m_cloud->m_UVScale = v;
    }
}
void SphericalTerrain::setCloudFlowSpeed(float v)
{
    if(m_cloud)
    {
        m_cloud->m_flowSpeed = v;
    }
}
void SphericalTerrain::setCloudHDRFactor(float v)
{
    if(m_cloud)
    {
        m_cloud->m_hdr = v;
    }
}
void SphericalTerrain::setCloudMetallic(float v)
{
    if(m_cloud)
    {
        m_cloud->m_metallic = v;
    }
}
void SphericalTerrain::setCloudRoughness(float v)
{
    if(m_cloud)
    {
        m_cloud->m_roughness = v;
    }
}
void SphericalTerrain::setCloudAmbientFactor(float v)
{
    if(m_cloud)
    {
        m_cloud->m_ambientFactor = v;
    }
}
void SphericalTerrain::setCloudNoiseOffsetScale(float v)
{
    if(m_cloud)
    {
        m_cloud->m_noiseOffsetScale = v;
    }
}

void SphericalTerrain::initWarfog()
{
    if(!m_warfog)
    {
        m_warfog = new WarfogRenderable(this);
        m_warfog->init();
    }
}

void Echo::SphericalTerrain::initPlotDivision()
{
	if (!m_PlotDivision)
	{
		m_PlotDivision = new PlanetPlotDivision(this);
		m_PlotDivision->init();
	}
}

float Echo::SphericalTerrain::getSnowHeight(uint32_t matid)
{
#ifdef _WIN32
	if (m_terrainData.isNull())return 0.f;
	if (matid >= m_terrainData->composition.usedBiomeTemplateList.size()) {
		return 0.f;
	}
	return m_terrainData->composition.usedBiomeTemplateList[matid].snow.snowHeight;
#else
	return 0.f;
#endif
}

void SphericalTerrain::initLowLODCloud()
{
  
	if (!m_terrainData.isNull())
	{
		setPlanetCloudId(m_terrainData->m_CloudTemplateId);
	}
       
}
#ifdef ECHO_EDITOR
void Echo::SphericalTerrain::updateBiomePercentage()
{
	if (m_terrainData->composition.usedBiomeTemplateList.empty())
		return;
	biomePercentage.assign(m_terrainData->composition.usedBiomeTemplateList.size(), 0.f);
	std::vector<int> biomeCount(m_terrainData->composition.usedBiomeTemplateList.size(), 0);

	int size = 0;
	for (const Bitmap* data : m_materialIndexMap.faces)
	{
		uint8_t* pixel = (uint8_t*)data->pixels;
		int mapSize = data->pitch * data->height;
		size += mapSize;
		for (int i = 0; i < mapSize; ++i)
		{
			uint8 id = *pixel;
			biomeCount[id] += 1;
			pixel++;
		}
	}
	int bimoeSize = (int)m_terrainData->composition.usedBiomeTemplateList.size();
	for (int i = 0; i < bimoeSize; ++i)
	{
		biomePercentage[i] = (float)biomeCount[i] / size;
	}
}

void Echo::SphericalTerrain::updateBiomePercentageCache(int index)
{
	const std::vector<BiomeDistributeRange>& distRanges = m_terrainData->distribution.distRanges;
	if (index >= biomePercentageCache.size() || index >= distRanges.size())
	{
		//LogManager::instance()->logMessage("SphericalTerrain::updateBiomePercentageCache() index error");
		return;
	}
	const ValueRange& temperatureRange = m_terrainData->distribution.temperatureRange;
	const ValueRange& temperature = distRanges[index].temperature;

	auto getVal = [](float val, float SeaSurfaceTemperature, const ValueRange& temperatureRange) {
		if (Math::FloatEqual(val, SeaSurfaceTemperature, 1e-3f))
			return SeaSurfaceTemperature;
		else if (val > SeaSurfaceTemperature)
			return std::clamp((val - SeaSurfaceTemperature) / (temperatureRange.max - SeaSurfaceTemperature), 0.f, 1.f);
		else
			return std::clamp(-(SeaSurfaceTemperature - val) / (SeaSurfaceTemperature - temperatureRange.min), -1.f, 0.f);
	};

	biomePercentageCache[index][0] = getVal(temperature.min, m_SeaSurfaceTemperature, temperatureRange);
	biomePercentageCache[index][1] = getVal(temperature.max, m_SeaSurfaceTemperature, temperatureRange);
	
}

void Echo::SphericalTerrain::initBiomePercentageCache()
{
	if (!m_editingTerrain || !m_ocean)
		return;
	const std::vector<BiomeDistributeRange>& distRanges = m_terrainData->distribution.distRanges;
	biomePercentageCache.assign(distRanges.size(), { 0.f,0.f });
	for (int i = 0; i < distRanges.size(); ++i)
	{
		updateBiomePercentageCache(i);
	}
}

void Echo::SphericalTerrain::updateSeaSurfaceTemperature()
{
	if (m_editingTerrain && m_ocean)
	{
		auto& [invert, exposure, exposureOffset, exposureGamma, _brightness, clamp] =
			m_terrainData->distribution.climateProcess.temperatureIP;
		float min = -m_noise->GetSigma3();
		float max = m_noise->GetSigma3();

		int processBrightness = _brightness != 0;
		float brightness = static_cast<float>(_brightness);

		int processExposure = exposure != 0.0f || exposureOffset != 0.0f || exposureGamma != 1.0f;
		//float exposure = exposure;
		//float exposureOffset = exposureOffset;
		float gammaInv = 1.0f / exposureGamma;

		int processInvert = invert;

		//int processClamp = clamp.min != 0.0f || clamp.max != 1.0f;
		//float clampMin = clamp.min;
		//float clampMax = clamp.max;

		float temp = m_SeaSurfaceTemperature;

		SphereOceanGeomSettings oceanParam;
		m_ocean->getGeomSetting(oceanParam);
		float elevation = 0.f;
		if (isSphere())
			elevation = (oceanParam.radius / getRadius() - 1) / m_elevationRatio;
		else if (isTorus())
			elevation = (oceanParam.relativeMinorRadius / m_relativeMinorRadius - 1) / m_elevationRatio;

		auto saturate = [](float value)
		{
			return std::max(std::min(1.0f, value), 0.0f);
		};
		auto _clamp = [](float maxf, float minf, float f)
		{
			if (maxf < minf)
			{
				float c = minf;
				minf = maxf;
				maxf = c;
			}
			return std::max(std::min(maxf, f), minf);
		};
		float SeaSurfaceTemperatureRange = saturate(1.f - (elevation - min) / (max - min));
		if (processBrightness)
		{
			SeaSurfaceTemperatureRange = 1.f -  (float)pow((1.f - SeaSurfaceTemperatureRange), pow(1.015f, brightness));
			SeaSurfaceTemperatureRange = /*saturate*/(SeaSurfaceTemperatureRange);
		}

		if (processExposure)
		{
			SeaSurfaceTemperatureRange *=  pow(2.f, exposure);
			SeaSurfaceTemperatureRange = /*saturate*/(SeaSurfaceTemperatureRange + exposureOffset);
			SeaSurfaceTemperatureRange = pow(SeaSurfaceTemperatureRange, gammaInv);
		}

		if (processInvert)
		{
			SeaSurfaceTemperatureRange = 1.f - SeaSurfaceTemperatureRange;
		}

		if (Math::FloatEqual(SeaSurfaceTemperatureRange, 0.f))
			SeaSurfaceTemperatureRange = 0.f;
		
		m_SeaSurfaceTemperature = EchoLerp(
			m_terrainData->distribution.temperatureRange.min,
			m_terrainData->distribution.temperatureRange.max,
			SeaSurfaceTemperatureRange);
		if (Math::FloatEqual(m_SeaSurfaceTemperature, temp, 1e-3f))
			m_SeaSurfaceTemperature = temp;
		

		//创建时初始化海平面温度，不更新地貌温度范围
		if (!m_bCreateFinish || biomePercentageCache.size() != m_terrainData->distribution.distRanges.size())
		{
			initBiomePercentageCache();
		}
		else if (m_SeaSurfaceTemperature != temp)
		{
			temp = m_SeaSurfaceTemperature;
			const ValueRange& temperatureRange = m_terrainData->distribution.temperatureRange;
			if (m_SeaSurfaceTemperature > temperatureRange.max || Math::FloatEqual(SeaSurfaceTemperatureRange, temperatureRange.max, 1e-3f))
				m_SeaSurfaceTemperature = temperatureRange.max;
			if (m_SeaSurfaceTemperature < temperatureRange.min || Math::FloatEqual(SeaSurfaceTemperatureRange, temperatureRange.min, 1e-3f))
				m_SeaSurfaceTemperature = temperatureRange.min;

			std::vector<BiomeDistributeRange>& distRanges = m_terrainData->distribution.distRanges;
			auto fixTemperatureRange = [&](float& val, int index, int type) {
				if (Math::FloatEqual(m_SeaSurfaceTemperature, temperatureRange.max, 1e-3f) && biomePercentageCache[index][type] >= 0)
				{
					val = temperatureRange.max;
				}
				else if (Math::FloatEqual(m_SeaSurfaceTemperature, temperatureRange.min, 1e-3f) && biomePercentageCache[index][type] < 0)
				{
					val = temperatureRange.min;
				}
				else
				{
					if (biomePercentageCache[index][type] < 0)
						val = Math::lerp(temperatureRange.min, m_SeaSurfaceTemperature, 1.f - abs(biomePercentageCache[index][type]));
					else
						val = Math::lerp(m_SeaSurfaceTemperature, temperatureRange.max, abs(biomePercentageCache[index][type]));
				}
			};
			for (int i = 0; i < distRanges.size(); ++i)
			{
				ValueRange& temperature = distRanges[i].temperature;

				fixTemperatureRange(temperature.min, i, 0);
				fixTemperatureRange(temperature.max, i, 1);
			}
			m_SeaSurfaceTemperature = temp;
			biomeDefineChanged(m_terrainData->composition, m_terrainData->distribution);
		}
	}
}
#endif

bool Echo::SphericalTerrain::exportPlanetGenObjects(const std::string& folderPath) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	std::string filePath = m_terrainData->getName().toString();
	{
		auto sufPox = filePath.find_last_of(".");
		if (sufPox != filePath.npos) {
			filePath = filePath.substr(0, sufPox);
		}
		std::replace(filePath.begin(), filePath.end(), '\\', '/');
		std::replace(filePath.begin(), filePath.end(), '/', '_');
		filePath.append(".golist");
	}
	if (!folderPath.empty()) {
		if (folderPath.back() == '/' || folderPath.back() == '\\') {
			filePath = folderPath + filePath;
		}
		else {
			filePath = folderPath + "\\" + filePath;
		}
	}
	return m_pPlanetGenObjectScheduler->ExportGenObjects(filePath, "");
}

bool Echo::SphericalTerrain::exportPlanetGenObjects() {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	std::string filePath = m_terrainData->getName().toString();
	{
		auto sufPox = filePath.find_last_of(".");
		if (sufPox != filePath.npos) {
			filePath = filePath.substr(0, sufPox);
		}
		std::replace(filePath.begin(), filePath.end(), '\\', '/');
		std::replace(filePath.begin(), filePath.end(), '/', '_');
		filePath.append(".golist");
		filePath = "../server/biome_terrain/" + filePath;
	}
	return m_pPlanetGenObjectScheduler->ExportGenObjects(filePath, "");
}

void Echo::SphericalTerrain::resetPlanetGenObject() {
	SAFE_DELETE(m_pPlanetGenObjectScheduler);
	if (m_bAtlas) return;
	PCG::PlanetGenObject _PlanetGenObject;
	bool bExistStaticMesh = false;
	for (int biomeCompoIndex = 0;
		biomeCompoIndex < m_terrainData->distribution.distRanges.size();
		biomeCompoIndex++)
	{
		if (biomeCompoIndex < m_terrainData->composition.biomes.size() &&
			m_terrainData->composition.biomes[biomeCompoIndex].biomeTemp.biomeID != BiomeTemplate::s_invalidBiomeID)
		{
			int biomeIndex = m_terrainData->composition.getBiomeIndex(biomeCompoIndex);
			auto& staticMeshsRef = m_terrainData->composition.biomes.at(biomeCompoIndex).genObjects;
			PCG::BiomeGenObject tempStaticMesh;
			if (PCG::BiomeGenObjectLibraryManager::instance()->Generate(staticMeshsRef, tempStaticMesh)) {
				for (int levelIndex = 0; levelIndex != 3; ++levelIndex) {
					if (!tempStaticMesh.level[levelIndex].path.empty()) {
						_PlanetGenObject.path[levelIndex].insert(std::make_pair(biomeIndex, tempStaticMesh.level[levelIndex].path));
						_PlanetGenObject.meshs[levelIndex].insert(std::make_pair(biomeIndex, tempStaticMesh.level[levelIndex].meshs));
						_PlanetGenObject.params[levelIndex].insert(std::make_pair(biomeIndex, tempStaticMesh.level[levelIndex].params));
						bExistStaticMesh = true;
					}
				}
			}
		}
	}
	if (bExistStaticMesh) {
		if (gNewPlanetGenObjectScheduler) {
			m_pPlanetGenObjectScheduler = gNewPlanetGenObjectScheduler(this);
			float maxRadius = m_radius;
			float maxElevation = std::max(std::abs(m_elevationMin), std::abs(m_elevationMax));
			float maxRealRadius = 0.0f;
			if (isSphere())
			{
				maxRealRadius = m_radius * (1.0f + m_elevationMax);
			}
			else if (isTorus())
			{
				maxRealRadius = m_radius * (1.0f + m_relativeMinorRadius * (1.0f + m_elevationMax));
			}
			PCG::PlanetGenObjectScheduler::CreateInfo genInfo;
			genInfo.sphTerMaxOff = (uint16)std::ceil(maxElevation * maxRadius);
			genInfo.sphPos = m_pos;
			genInfo.sphRot = m_rot;
			genInfo.sphRotInv = m_rot.Inverse();
			genInfo.sphScl = m_scale;
			genInfo.sphRadius = maxRadius;
			genInfo.maxRealRadius = maxRealRadius;
			genInfo.sphPtr = this;
			genInfo.randomSeed = m_terrainData->composition.randomSeed;
			genInfo.sceneName = m_mgr->m_mgr->getName();
			m_pPlanetGenObjectScheduler->Init(genInfo, _PlanetGenObject);
		}
	}
}

bool Echo::SphericalTerrain::addDeletedGenObject(uint64 id) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	m_pPlanetGenObjectScheduler->addDeletedObject(id);
	return true;
}
bool Echo::SphericalTerrain::removeDeletedGenObject(uint64 id) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	m_pPlanetGenObjectScheduler->removeDeletedObject(id);
	return true;
}
bool Echo::SphericalTerrain::getAllDeletedGenObject(std::set<uint64>& ids) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	m_pPlanetGenObjectScheduler->getAllDeletedObject(ids);
	return true;
}
bool Echo::SphericalTerrain::hideGenObjects(const std::set<uint64>& ids) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	m_pPlanetGenObjectScheduler->setGenObjectsVisible(ids, false);
	return true;
}
bool Echo::SphericalTerrain::showGenObjects(const std::set<uint64>& ids) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	m_pPlanetGenObjectScheduler->setGenObjectsVisible(ids, true);
	return true;
}
bool Echo::SphericalTerrain::queryGenObjects(const DVector3& worldPos, const Quaternion& worldRot, const Vector3& halfSize, std::set<uint64>& objIds, bool checkVisible) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	return m_pPlanetGenObjectScheduler->queryGenObjects(worldPos, worldRot, halfSize, objIds, checkVisible);
}
bool Echo::SphericalTerrain::queryGenObjects(const DVector3& worldPos, float radius, std::set<uint64>& objIds, bool checkVisible) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	return m_pPlanetGenObjectScheduler->queryGenObjects(worldPos, radius, objIds, checkVisible);
}

bool Echo::SphericalTerrain::getGenObjectInfo(uint64 id, GenObjectInfo& info) {
	if (m_pPlanetGenObjectScheduler == nullptr) return false;
	return m_pPlanetGenObjectScheduler->getGenObject(id, info);
}

std::function<PCG::PlanetGenObjectScheduler* (SphericalTerrain*)> Echo::SphericalTerrain::gNewPlanetGenObjectScheduler = nullptr;
