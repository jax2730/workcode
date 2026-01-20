#ifndef ECHO_SPHERICAL_TERRAIN_QUAD_TREE_NODE_H
#define ECHO_SPHERICAL_TERRAIN_QUAD_TREE_NODE_H

#include <shared_mutex>

namespace Echo
{
	class SphericalTerrain;
	class TerrainGenerator;

	class _EchoExport SphericalTerrainQuadTreeNode : public Renderable
	{
	public:
		using Face = int;
		static constexpr Face PositiveX = 0;
		static constexpr Face NegativeX = 1;
		static constexpr Face PositiveY = 2;
		static constexpr Face NegativeY = 3;
		static constexpr Face PositiveZ = 4;
		static constexpr Face NegativeZ = 5;

		enum Quad : int8_t
		{
			RootQuad = -1,
			BtmLft   = 0,
			BtmRht   = 1,
			TopLft   = 2,
			TopRht   = 3
		};

		enum Edge : int8_t
		{
			NoneEdge = -1,
			Top      = 0,
			Btm      = 1,
			Lft      = 2,
			Rht      = 3
		};

		using EdgeMask = uint8_t;

		// Orthogonal basis for each face of a cube.
		// Forward vector is the normal of the face, as well as the center of the cube face.
		// Point on face can be parameterized by two coordinates (x, y).
		// P = FaceForward + FaceRight * x + FaceUp * y; where x, y in [-1, 1]
		inline static const Vector3 FaceForward[6] =
		{
			Vector3(1, 0, 0),
			Vector3(-1, 0, 0),
			Vector3(0, 1, 0),
			Vector3(0, -1, 0),
			Vector3(0, 0, 1),
			Vector3(0, 0, -1)
		};

		inline static const Vector3 FaceRight[6] =
		{
			Vector3(0, 0, -1),
			Vector3(0, 0, 1),
			Vector3(1, 0, 0),
			Vector3(1, 0, 0),
			Vector3(1, 0, 0),
			Vector3(-1, 0, 0)
		};

		inline static const Vector3 FaceUp[6] =
		{
			Vector3(0, 1, 0),
			Vector3(0, 1, 0),
			Vector3(0, 0, -1),
			Vector3(0, 0, 1),
			Vector3(0, 1, 0),
			Vector3(0, 1, 0)
		};

        static void MapToCube_withFace(const Vector3& sph,
                                       const Face& face,
                                       Vector2& xy)
		{
            const auto aSph = Vector3(std::abs(sph.x), std::abs(sph.y), std::abs(sph.z));

			float t;
			if (face == PositiveX || face == NegativeX)
			{
				t    = 1.0f / aSph.x;
			}
			else if (face == PositiveY || face == NegativeY)
			{
				t    = 1.0f / aSph.y;
			}
			else
			{
				t    = 1.0f / aSph.z;
			}

            const Vector3 cube = sph * t;
            
			xy.x = std::clamp(cube.Dot(FaceRight[face]), -1.0f, 1.0f);
			xy.y = std::clamp(cube.Dot(FaceUp[face]), -1.0f, 1.0f);
		}
        
		static void MapToCube(const Vector3& sph, Face& face, Vector2& xy)
		{
			const auto aSph = Vector3(std::abs(sph.x), std::abs(sph.y), std::abs(sph.z));

			float t;
			if (aSph.x >= aSph.y && aSph.x >= aSph.z)
			{
				face = sph.x >= 0.0 ? PositiveX : NegativeX;
				t    = 1.0f / aSph.x;
			}
			else if (aSph.y >= aSph.z && aSph.y >= aSph.x)
			{
				face = sph.y >= 0.0 ? PositiveY : NegativeY;
				t    = 1.0f / aSph.y;
			}
			else
			{
				face = sph.z >= 0.0 ? PositiveZ : NegativeZ;
				t    = 1.0f / aSph.z;
			}

			const Vector3 cube = sph * t;

			xy.x = std::clamp(cube.Dot(FaceRight[face]), -1.0f, 1.0f);
			xy.y = std::clamp(cube.Dot(FaceUp[face]), -1.0f, 1.0f);
		}

		static Vector3 getCubeVertex(const Face face, const Vector2& xy)
		{
			return FaceForward[face] + FaceRight[face] * xy.x + FaceUp[face] * xy.y;
		}

		static Vector3 MapToSphere(const Face face, const Vector2& xy)
		{
			auto sp = getCubeVertex(face, xy);
			sp.normalise();
			return sp;
		}

		static Vector3 MapToSphereUniform(const Face face, const Vector2& xy)
		{
			// http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
			const auto sp    = getCubeVertex(face, xy);
			const auto p2    = sp * sp;
			const Vector3 dp =
			{
				std::sqrtf(1.0f - 0.5f * (p2.y + p2.z) + p2.y * p2.z / 3.0f),
				std::sqrtf(1.0f - 0.5f * (p2.z + p2.x) + p2.z * p2.x / 3.0f),
				std::sqrtf(1.0f - 0.5f * (p2.x + p2.y) + p2.x * p2.y / 3.0f)
			};
			return sp * dp;
		}

		using IndexType                       = uint16;
		static constexpr auto ECHO_INDEX_TYPE = ECHO_INDEX_TYPE_UINT16;

		// static constexpr int GRID_RESOLUTION       = 255; // 2^8 - 1
		static constexpr int GRID_RESOLUTION = 65; // 2^7 + 1
		static constexpr int TILE_VERTICES   = GRID_RESOLUTION * GRID_RESOLUTION;

		// static constexpr int TEXTURE_RESOLUTION    = GRID_RESOLUTION + 1; // 2^8 - 1
		static constexpr double MAX_CUBE_LENGTH = 2.0;

		static constexpr double getLengthOnCube(const int depth)
		{
			if (depth < 0) return MAX_CUBE_LENGTH;
			return MAX_CUBE_LENGTH / (1ull << depth);
		}

		static constexpr double getLengthOnSphere(const int depth)
		{
			constexpr double len0 = 1.4472025091165353; /*std::sqrt(4 * 3.14159265358979323846 / 6)*/
			if (depth < 0) return len0;
			return len0 / (1ull << depth);
		}

		static constexpr double MAX_CUBE_GRID_SIZE = MAX_CUBE_LENGTH / (GRID_RESOLUTION - 1);

		static constexpr double getGridSizeOnCube(const int depth)
		{
			if (depth < 0) return MAX_CUBE_GRID_SIZE;
			return MAX_CUBE_GRID_SIZE / (1ull << depth);
		}

		static constexpr int getLevelTileCount(const int depth)
		{
			if (depth < 0) return 0;
			constexpr int count[] = {
				1ull << (2 * 0), 1ull << (2 * 1), 1ull << (2 * 2), 1ull << (2 * 3),
				1ull << (2 * 4), 1ull << (2 * 5), 1ull << (2 * 6), 1ull << (2 * 7),
				1ull << (2 * 8), 1ull << (2 * 9), 1ull << (2 * 10), 1ull << (2 * 11),
				1ull << (2 * 12), 1ull << (2 * 13), 1ull << (2 * 14),
			};
			return depth < std::size(count)
				       ? count[depth]
				       : static_cast<int>(1ull << (2 * depth));
		}

		static constexpr int getTileCountOverLevel(const int maxDepth)
		{
			if (maxDepth < 0) return 0;
			constexpr int count[] =
			{
				((1ull << (2 * (0 + 1))) - 1ull) / 3ull, ((1ull << (2 * (1 + 1))) - 1ull) / 3ull, ((1ull << (2 * (2 + 1))) - 1ull) / 3ull,
				((1ull << (2 * (3 + 1))) - 1ull) / 3ull, ((1ull << (2 * (4 + 1))) - 1ull) / 3ull, ((1ull << (2 * (5 + 1))) - 1ull) / 3ull,
				((1ull << (2 * (6 + 1))) - 1ull) / 3ull, ((1ull << (2 * (7 + 1))) - 1ull) / 3ull, ((1ull << (2 * (8 + 1))) - 1ull) / 3ull,
				((1ull << (2 * (9 + 1))) - 1ull) / 3ull, ((1ull << (2 * (10 + 1))) - 1ull) / 3ull, ((1ull << (2 * (11 + 1))) - 1ull) / 3ull,
				((1ull << (2 * (12 + 1))) - 1ull) / 3ull, ((1ull << (2 * (13 + 1))) - 1ull) / 3ull, ((1ull << (2 * (14 + 1))) - 1ull) / 3ull,
			};
			return maxDepth < std::size(count)
				       ? count[maxDepth]
				       : static_cast<int>(((1ull << (2 * (maxDepth + 1))) - 1ull) / 3ull);
		}

		static constexpr double MIN_GRID_SIZE    = 4.0;
		static constexpr double ELEVATION_DETAIL = 2.0;

		SphericalTerrainQuadTreeNode() = delete;
		SphericalTerrainQuadTreeNode(SphericalTerrain* planet, SphericalTerrainQuadTreeNode* parent,
		                             Face face, Quad quad, int depth, int indexX, int indexY);
		SphericalTerrainQuadTreeNode(SphericalTerrain* planet, SphericalTerrainQuadTreeNode* parent,
		                             int slice, Quad quad, int depth, int indexX, int indexY,
		                             const Vector2& offset, const Vector2& length);

		~SphericalTerrainQuadTreeNode() override;

		SphericalTerrainQuadTreeNode(const SphericalTerrainQuadTreeNode&)            = delete;
		SphericalTerrainQuadTreeNode& operator=(const SphericalTerrainQuadTreeNode&) = delete;

		SphericalTerrainQuadTreeNode(SphericalTerrainQuadTreeNode&&) noexcept            = delete;
		SphericalTerrainQuadTreeNode& operator=(SphericalTerrainQuadTreeNode&&) noexcept = delete;

		// NOTE(yanghang): Func from Renderable
		void getWorldTransforms(const DBMatrix4** xform) const override;
		void getWorldTransforms(DBMatrix4* xform) const override;
		void getRenderableWorldScale(const Vector3** scale) const override;
		void getRenderableWorldOrientation(const Quaternion** ori) const override;
		void getRenderableWorldPosition(const DBVector3** pos) const override;
		RenderOperation* getRenderOperation() override { return &m_renderOperation; }
		const LightList& getLights() const override;
		uint32 getInstanceNum() override { return 1; }

		void setCustomParameters(const Pass* pPass) override;
		const Material* getMaterial() const override;
		MaterialWrapper getMaterialWrapper() const override;

		template <typename Cond>
		static void traverseTree(
			SphericalTerrainQuadTreeNode* node,
			const Cond& stoppingCond)
		{
			if (!node) return;

			if (!stoppingCond(node)) return;

			if (!node->hasChildren()) return;

			for (auto&& child : node->m_children)
			{
				traverseTree(child.get(), stoppingCond);
			}
		}

		virtual void init();
		void shutDown();

		int getLodLevel() const { return m_depth; }

		[[nodiscard]] static Vector3 getSphericalPosition(const Face face, const Vector2& offset, const TerrainGenerator* pGen = nullptr);

		[[nodiscard]] Vector3 getSphericalPosition(const Vector2& uvOnQuad) const;

		[[nodiscard]] static std::vector<Vector3> getSphericalPositions(
			Face face, size_t width, size_t height, const Vector2& gridSize,
			const Vector2& offset, const TerrainGenerator* pGen = nullptr);

		[[nodiscard]] static std::vector<Vector3> getSphericalPositionsUniform(
			Face face, size_t width, size_t height, const Vector2& gridSize,
			const Vector2& offset, const TerrainGenerator* pGen = nullptr);

		[[nodiscard]] static Vector3 applyElevation(const Vector3& vertex, const float elevation)
		{
			return vertex * (1.0f + elevation);
		}

		[[nodiscard]] std::vector<Vector2> getBiomeUv(uint32_t biomeRes) const;

		void updateWorldTransform();
		virtual void computeApproximateBound(float elevationRange);
		void setBound(const AxisAlignedBox& bound) { m_bound = bound; }
		AxisAlignedBox getBoundLocal() const;
		OrientedBox getOrientedBoundLocal() const;
		float getRenderDistanceLocal() const;

		friend class SphericalTerrain;
		friend class ProceduralSphere;

	protected:
		virtual void calcSurfaceArea();
		void setVertexBuffer(RcBuffer* vb, int vertexCount = TILE_VERTICES);
		void destroyVertexBuffer();

		void setIndexBuffer(RcBuffer* ib, int indexCount);
		void clearIndexBuffer();

		virtual bool hasSharedEdge(Edge edge, const SphericalTerrainQuadTreeNode& node) const;
		Edge hasAnySharedEdge(const SphericalTerrainQuadTreeNode& node) const;

		virtual Vector3 getTranslationLocal() const;

		virtual Matrix3 getRotationLocal() const;

		static Matrix3 getRotationLocal(Face face, int depth, int x, int y, const TerrainGenerator* pGen);

		DMatrix4 getTransformationLocal() const;

		virtual int getBiomeTextureSlice() const;

		void removeNeighbor(const SphericalTerrainQuadTreeNode* node)
		{
			for (auto&& neighbor : m_neighbors)
			{
				if (neighbor != node) continue;
				neighbor = nullptr;
			}
			m_ibDirty = true;
		}

		void addNeighbor(const Edge edge, SphericalTerrainQuadTreeNode* neighbor)
		{
			m_neighbors[edge] = neighbor;
			m_ibDirty         = true;
		}

		SphericalTerrainQuadTreeNode* getNeighbor(const Edge edge) const
		{
			return m_neighbors[edge];
		}

		void addNeighbors();
		virtual void addNeighborsRoot();

		SphericalTerrainQuadTreeNode* getParentsNeighbor(const Edge edge) const
		{
			if (!m_parent) return nullptr;
			return m_parent->getNeighbor(edge);
		}

		Edge getEdgeSharedWith(const SphericalTerrainQuadTreeNode& neighbor, Edge neighborsEdge) const;
		std::pair<SphericalTerrainQuadTreeNode*, SphericalTerrainQuadTreeNode*> getChildrenSharing(Edge edge) const;
		bool isNeighborsChildSubdivided() const;

		virtual void createChildren(void* memory);
		void destroyChildren();

		bool hasChildren() const { return m_children.front() != nullptr; }

		SphericalTerrainQuadTreeNode* getChild(const Quad q) const
		{
			return m_children.at(q).get();
		}

		EdgeMask getIndexBufferType() const;

		bool m_active  = false;
		bool m_ibDirty = true; // set to true when neighboring node has changed
		bool m_vbDirty = true; // set to true when modifiers has changed
		Quad m_quadType;

		uint16_t m_slice;
		uint16_t m_depth;
		uint16_t m_levelIndexX;
		uint16_t m_levelIndexY;

		Vector2 m_gridSize;
		Vector2 m_gridSizeInv;
		Vector2 m_length;
		Vector2 m_offset;

		float m_areaSqrt = 0.0f;

		DBMatrix4 m_world = DBMatrix4::IDENTITY;

		SphericalTerrainQuadTreeNode* m_parent = nullptr;
		SphericalTerrainQuadTreeNode* m_neighbors[4] { nullptr };

		std::array<std::unique_ptr<SphericalTerrainQuadTreeNode>, 4> m_children { nullptr };

		AxisAlignedBox m_bound;
		mutable std::shared_mutex m_boundMutex;
		OrientedBox m_orientedBound {};

		RenderOperation m_renderOperation;

		//IMPORTANT: Planet which owns the spherical terrain node.
		SphericalTerrain* const m_root = nullptr;

		RcBuffer* m_vertexBuffer = nullptr;

		std::vector<TerrainVertex> m_vertices;
		std::vector<float> m_elevations;
		mutable std::shared_mutex m_elevationMutex;

		std::set<uint64_t> m_modifierInstances;

	private:
		std::array<Vector3, 4> getCornersOnCube() const
		{
			std::array<Vector3, 4> corners;
			corners[BtmLft] = getCubeVertex(m_slice, m_offset);
			corners[BtmRht] = getCubeVertex(m_slice, m_offset + Vector2(m_length.x, 0));
			corners[TopLft] = getCubeVertex(m_slice, m_offset + Vector2(0, m_length.y));
			corners[TopRht] = getCubeVertex(m_slice, m_offset + Vector2(m_length.x, m_length.y));
			return corners;
		}
	};


	struct TerrainGeneratorPrivate;
	class _EchoExport TerrainGenerator {
		friend class TerrainGeneratorLibrary;
	public:
		TerrainGenerator(uint32 type, const String& file, const String& mesh, const String& gravity, const String& border);
		~TerrainGenerator();
		bool isSuccess() const;
		bool isNearby(const Vector3& position, float scale) const;
		Vector3 getGravity(const Vector3& position, float scale) const;
		float getHeight(SphericalTerrainQuadTreeNode::Face face, float x, float y) const;
		float getSqrtAreaRatio() const;
		float getBorder(const Vector3& position) const;
		float getBorderFace(const Vector3& position) const;
		std::pair<float,float> getHeightAndBorder(SphericalTerrainQuadTreeNode::Face face, float x, float y) const;
#ifdef ECHO_EDITOR
		TexturePtr getTextureArray();
		TexturePtr getBorderTextureArray();
#endif // ECHO_EDITOR

		uint32 getType() const { return mType; }
		const String& getName() const { return mName; }
		const String& getMeshName() const { return mMeshName; }
		const String& getGravityName() const { return mGravityName; }
		const String& getBorderName() const { return mBorderName; }

		void reload();
		void unload();
	private:
		TerrainGeneratorPrivate* m_pPrivate = nullptr;
		uint32 mType = 0;
		String mName;
		String mMeshName;
		String mGravityName;
		String mBorderName;
		bool bSuccess = false;
	};
}
#endif
