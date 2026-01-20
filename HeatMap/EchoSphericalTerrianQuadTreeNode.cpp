#include "EchoStableHeaders.h"
#include "EchoSphericalTerrianQuadTreeNode.h"

#include "EchoSphericalTerrainManager.h"
#include "EchoTextureManager.h"

#include "EchoBiomeTerrainBlock.h"
#include "EchoPlanetWarfogRenderable.h"
#include "EchoPolyhedronGravity.h"

using namespace Echo;

namespace
{
	Vector3 NearestPointOnLine(const Vector3& p, const Vector3& a, const Vector3& b)
	{
		const Vector3 ab = b - a;
		const Vector3 ap = p - a;
		float t          = ap.dotProduct(ab) / ab.dotProduct(ab);
		t                = std::clamp(t, 0.0f, 1.0f);
		return a + ab * t;
	}
}

SphericalTerrainQuadTreeNode::SphericalTerrainQuadTreeNode(
	SphericalTerrain* planet, SphericalTerrainQuadTreeNode* parent,
	const Face face, const Quad quad, const int depth, const int indexX, const int indexY) :
	m_quadType(quad),
	m_slice(face),
	m_depth(depth),
	m_levelIndexX(indexX),
	m_levelIndexY(indexY),
	m_gridSize(static_cast<float>(getGridSizeOnCube(depth))),
	m_gridSizeInv(1.0f / m_gridSize),
	m_length(static_cast<float>(getLengthOnCube(depth))),
	m_offset(-1.0f + m_length * Vector2(static_cast<float>(m_levelIndexX), static_cast<float>(m_levelIndexY))),
	m_parent(parent),
	m_root(planet)
{
	enableCustomParameters(true);

	m_root->m_profile.blockMemory += sizeof(SphericalTerrainQuadTreeNode);
}

SphericalTerrainQuadTreeNode::SphericalTerrainQuadTreeNode(SphericalTerrain* planet, SphericalTerrainQuadTreeNode* parent,
                                                           const int slice, const Quad quad, const int depth,
                                                           const int indexX, const int indexY,
                                                           const Vector2& offset, const Vector2& length) :
	m_quadType(quad),
	m_slice(slice),
	m_depth(depth),
	m_levelIndexX(indexX),
	m_levelIndexY(indexY),
	m_gridSize(length * (1.0f / static_cast<float>(GRID_RESOLUTION - 1))),
	m_gridSizeInv(1.0f / m_gridSize),
	m_length(length),
	m_offset(offset),
	m_parent(parent),
	m_root(planet)
{
	enableCustomParameters(true);

	m_root->m_profile.blockMemory += sizeof(SphericalTerrainQuadTreeNode);
}

SphericalTerrainQuadTreeNode::~SphericalTerrainQuadTreeNode()
{
	shutDown();

	m_root->m_profile.blockMemory -= sizeof(SphericalTerrainQuadTreeNode);
}

void SphericalTerrainQuadTreeNode::init()
{
	calcSurfaceArea();
	updateWorldTransform();
	destroyVertexBuffer();
	clearIndexBuffer();

	if (m_depth == m_root->m_maxDepth)
	{
		std::unique_lock lock(m_root->m_leafMutex);
		m_root->m_leaves[m_root->getNodeIndex(*this)] = this;
	}
}

void SphericalTerrainQuadTreeNode::shutDown()
{
	destroyVertexBuffer();
	clearIndexBuffer();

	{
		std::unique_lock lock(m_elevationMutex);
		m_elevations.clear();
	}

	if (m_depth == m_root->m_maxDepth)
	{
		// if (const auto it = m_root->m_leaves.find(m_root->getNodeIndex(*this)); it != m_root->m_leaves.end())
		std::unique_lock lock(m_root->m_leafMutex);
		m_root->m_leaves.erase(m_root->getNodeIndex(*this));
	}

	for (auto&& neighbor : m_neighbors)
	{
		if (neighbor == nullptr) continue;

		// neighbor->m_ibDirty = true;

		neighbor->removeNeighbor(this);
	}

	for (auto && child : m_children)
	{
		child.reset();
	}
}

void SphericalTerrainQuadTreeNode::destroyVertexBuffer()
{
	RenderSystem* renderSystem = Root::instance()->getRenderSystem();
	if (m_vertexBuffer)
	{
		m_root->m_profile.meshMemory -= m_vertexBuffer->dataSize;

		renderSystem->destoryBuffer(m_vertexBuffer);
		m_vertexBuffer = nullptr;
		m_vbDirty = true;
	}
	m_renderOperation.m_stMeshData.m_vertexBuffer.clear();
}

void SphericalTerrainQuadTreeNode::setVertexBuffer(RcBuffer* vb, const int vertexCount)
{
	destroyVertexBuffer();

	m_vertexBuffer = vb;
	m_vbDirty = false;

	m_renderOperation.m_stMeshData.m_vertexBuffer.push_back(m_vertexBuffer);
	m_renderOperation.m_stMeshData.m_vertexCount    = vertexCount;
	m_renderOperation.m_stMeshData.m_vertexBuffSize = vertexCount * sizeof(TerrainVertex);
}
Vector3 SphericalTerrainQuadTreeNode::getSphericalPosition(const Face face, const Vector2& offset, const TerrainGenerator* pGen)
{
	if (pGen) {
		return MapToSphere(face, offset) * pGen->getHeight(face, offset.x, offset.y);
	}
	else {
		return MapToSphere(face, offset);
	}
}

Vector3 SphericalTerrainQuadTreeNode::getSphericalPosition(const Vector2& uvOnQuad) const
{
	return getSphericalPosition(m_slice, m_offset + uvOnQuad * m_length, m_root->m_pTerrainGenerator);
}

std::vector<Vector3> SphericalTerrainQuadTreeNode::getSphericalPositions(
	const Face face, const size_t width, const size_t height, const Vector2& gridSize,
	const Vector2& offset, const TerrainGenerator* pGen)
{
	std::vector<Vector3> pos;
	pos.reserve(width * height);
	bool bGen = pGen != nullptr;
	for (size_t i = 0; i < height; ++i)
	{
		for (size_t j = 0; j < width; ++j)
		{
			const float x = offset.x + j * gridSize.x;
			const float y = offset.y + i * gridSize.y;

			pos.emplace_back(MapToSphere(face, { x, y }));
			if (bGen) {
				pos.back() *= pGen->getHeight(face, x, y);
			}
		}
	}
	return pos;
}

std::vector<Vector3> SphericalTerrainQuadTreeNode::getSphericalPositionsUniform(
	const Face face, const size_t width, const size_t height, const Vector2& gridSize,
	const Vector2& offset, const TerrainGenerator* pGen)
{
	std::vector<Vector3> pos;
	pos.reserve(width * height);
	const bool bGen = pGen != nullptr;
	for (size_t i = 0; i < height; ++i)
	{
		for (size_t j = 0; j < width; ++j)
		{
			const float x = offset.x + j * gridSize.x;
			const float y = offset.y + i * gridSize.y;

			pos.emplace_back(MapToSphereUniform(face, { x, y }));
			if (bGen)
			{
				auto& p = pos.back();
				int f = 0;
				Vector2 xy = Vector2::ZERO;
				MapToCube(p, f, xy);
				p *= pGen->getHeight(f, xy.x, xy.y);
			}
		}
	}
	return pos;
}

std::vector<Vector2> SphericalTerrainQuadTreeNode::getBiomeUv(const uint32_t biomeRes) const
{
	std::vector<Vector2> uv;
	uv.reserve(TILE_VERTICES);

	for (int i = 0; i < GRID_RESOLUTION; ++i)
	{
		for (int j = 0; j < GRID_RESOLUTION; ++j)
		{
			uv.emplace_back(
				static_cast<float>(j) / static_cast<float>(GRID_RESOLUTION - 1.f), // [0, 1]
				static_cast<float>(i) / static_cast<float>(GRID_RESOLUTION - 1.f)  // [0, 1]
				);
		}
	}
	return uv;
}

void SphericalTerrainQuadTreeNode::updateWorldTransform()
{
	DMatrix4 trans = DMatrix4::IDENTITY;
	trans.setTrans(getTranslationLocal());
	//const DMatrix4 trans = getTransformationLocal(m_slice, m_depth, m_levelIndexX, m_levelIndexY).inverseAffine();
	m_world = m_root->m_worldMat * trans;
}

void SphericalTerrainQuadTreeNode::computeApproximateBound(const float elevationRange)
{
	m_bound = AxisAlignedBox::BOX_NULL;

	for (const Vector2 points[] =
	     {
		     Vector2(0, 0),
		     Vector2(1, 0),
		     Vector2(0, 1),
		     Vector2(1, 1),
		     Vector2(0.5, 0.5)
	     }; auto&& p : points)
	{
		auto sph = getSphericalPosition(p);

		m_bound.AddVert(sph * (1.0f + elevationRange));
		m_bound.AddVert(sph);
		m_bound.AddVert(sph * (1.0f - elevationRange));
	}
}

AxisAlignedBox SphericalTerrainQuadTreeNode::getBoundLocal() const
{
	// auto box = m_bound;
	// box.transform(m_root->m_worldMat);
	std::shared_lock lock(m_boundMutex);
	return m_bound;
}

OrientedBox SphericalTerrainQuadTreeNode::getOrientedBoundLocal() const
{
	std::shared_lock lock(m_boundMutex);
	return m_orientedBound;
}

float SphericalTerrainQuadTreeNode::getRenderDistanceLocal() const
{
	// static constexpr float LOD_RATIO = 0.5;
	// constexpr double tileLength = LOD_RATIO * 1.44720250911653528;
		//std::sqrt(4.0 * ECHO_PI / 6.0) * LOD_RATIO;  // 1 / 6 of the sphere
	return m_areaSqrt;
}

void SphericalTerrainQuadTreeNode::clearIndexBuffer()
{
	m_renderOperation.m_stMeshData.m_IndexBufferData.m_indexBuffer = nullptr;
	m_renderOperation.m_stMeshData.m_IndexBufferData.m_indexCount  = 0;
	m_ibDirty = true;
}

bool SphericalTerrainQuadTreeNode::hasSharedEdge(const Edge edge, const SphericalTerrainQuadTreeNode& node) const
{
	auto getDistanceToEdge = [this](const Edge e, const Vector3& p)
	{
		Vector3 a;
		Vector3 b;
		const auto corners = getCornersOnCube();

		switch (e)
		{
		case Top:
			a = corners[TopLft];
			b = corners[TopRht];
			break;
		case Btm:
			a = corners[BtmLft];
			b = corners[BtmRht];
			break;
		case Lft:
			a = corners[BtmLft];
			b = corners[TopLft];
			break;
		case Rht:
			a = corners[BtmRht];
			b = corners[TopRht];
			break;
		case NoneEdge: return std::numeric_limits<float>::max();
		}

		const float dist = p.distance(NearestPointOnLine(p, a, b));
		return dist;
	};

	auto isPointWithinEdge = [&getDistanceToEdge](const Edge e, const Vector3& p, const float tolerance)
	{
		return getDistanceToEdge(e, p) < tolerance;
	};

	auto isLineWithinEdge = [&isPointWithinEdge](const Edge e, const Vector3& a, const Vector3& b, const float tolerance)
	{
		return isPointWithinEdge(e, a, tolerance) && isPointWithinEdge(e, b, tolerance);
	};

	const auto& [btmLft, btmRht, topLft, topRht] = node.getCornersOnCube();
	constexpr float tol = 1e-5f;
	if (isLineWithinEdge(edge, topLft, topRht, tol) ||
		isLineWithinEdge(edge, btmLft, btmRht, tol) ||
		isLineWithinEdge(edge, btmLft, topLft, tol) ||
		isLineWithinEdge(edge, btmRht, topRht, tol))
	{
		return true;
	}

	return false;
}

SphericalTerrainQuadTreeNode::Edge SphericalTerrainQuadTreeNode::hasAnySharedEdge(const SphericalTerrainQuadTreeNode& node) const
{
	for (int i = 0; i < 4; ++i)
	{
		if (hasSharedEdge(static_cast<Edge>(i), node))
		{
			return static_cast<Edge>(i);
		}
	}
	return NoneEdge;
}

Vector3 SphericalTerrainQuadTreeNode::getTranslationLocal() const
{
	return getSphericalPosition({ 0.5f, 0.5f });
}

Matrix3 SphericalTerrainQuadTreeNode::getRotationLocal() const
{
	return getRotationLocal(m_slice, m_depth, m_levelIndexX, m_levelIndexY, m_root->m_pTerrainGenerator);
}

Matrix3 SphericalTerrainQuadTreeNode::getRotationLocal(const Face face, const int depth, const int x, const int y, const TerrainGenerator* pGen)
{
	const float len            = static_cast<float>(getLengthOnCube(depth));
	const Vector2 centreOffset = { (-1.0f + len * (0.5f + x)), (-1.0f + len * (0.5f + y)) };
	const Vector3 centre       = getSphericalPosition(face, centreOffset, pGen);
	const float gridSize       = static_cast<float>(getGridSizeOnCube(depth));

	Vector3 dx;
	if (std::abs(centreOffset.x) <= std::abs(centreOffset.y))
	{
		dx = getSphericalPosition(face, centreOffset + Vector2(gridSize, 0), pGen) - centre;
	}
	else
	{
		dx = getSphericalPosition(face, centreOffset + Vector2(0, gridSize), pGen) - centre;
	}
	Vector3 axisY = centre.normalisedCopy();
	if (pGen)
	{
		axisY = -pGen->getGravity(centre, 1.0f);
	}
	Vector3 axisX = dx - axisY * axisY.dotProduct(dx);
	axisX.normalise();

	Vector3 axisZ = axisX.crossProduct(axisY);
	axisZ.normalise();

	return {
		axisX.x, axisX.y, axisX.z,
		axisY.x, axisY.y, axisY.z,
		axisZ.x, axisZ.y, axisZ.z
	};
}

DMatrix4 SphericalTerrainQuadTreeNode::getTransformationLocal() const
{
	auto local = DMatrix4(getRotationLocal());
	//local.setTrans(Vector3(0, -1, 0));
	return local;
}

int SphericalTerrainQuadTreeNode::getBiomeTextureSlice() const
{
	return m_slice;
}

void SphericalTerrainQuadTreeNode::addNeighbors()
{
	assert(m_parent && "SphericalTerrainQuadTreeNode::addNeighbors can't be called on root face node.");

	auto parentsNeighborsChildSharingEdge = [this](const SphericalTerrainQuadTreeNode* parent) -> SphericalTerrainQuadTreeNode* {
		if (!parent || !parent->hasChildren()) return nullptr;

		auto& children    = parent->m_children;
		Edge e            = NoneEdge;
		const auto result = std::find_if(children.begin(), children.end(),
			[this, &e](const auto& child)
			{
				// Child node doesn't have circumstances where it shares opposite edge with itself.
				if (&*child == this) return false;
				e = hasAnySharedEdge(*child);
				return e != NoneEdge;
			});
		return result == children.end() ? nullptr : result->get();
	};

	// auto hasNeighbor = [](const SphericalTerrainQuadTreeNode* curr, const SphericalTerrainQuadTreeNode* neighbor)
	// {
	// 	return std::find(std::begin(curr->m_neighbors), std::end(curr->m_neighbors), neighbor)
	// 		!= std::end(curr->m_neighbors);
	// };

	switch (m_quadType)
	{
	case BtmLft:
	{
		// add siblings
		addNeighbor(Top, m_parent->m_children.at(TopLft).get());
		addNeighbor(Rht, m_parent->m_children.at(BtmRht).get());

		// add non-siblings
		auto* nbr3 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Btm));
		addNeighbor(Btm, nbr3);
		if (nbr3) nbr3->addNeighbor(nbr3->hasAnySharedEdge(*this), this);
		auto* nbr4 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Lft));
		addNeighbor(Lft, nbr4);
		if (nbr4) nbr4->addNeighbor(nbr4->hasAnySharedEdge(*this), this);
	}
	break;
	case BtmRht:
	{
		// add siblings
		addNeighbor(Top, m_parent->m_children.at(TopRht).get());
		addNeighbor(Lft, m_parent->m_children.at(BtmLft).get());

		// add non-siblings
		auto* nbr3 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Btm));
		addNeighbor(Btm, nbr3);
		if (nbr3) nbr3->addNeighbor(nbr3->hasAnySharedEdge(*this), this);
		auto* nbr4 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Rht));
		addNeighbor(Rht, nbr4);
		if (nbr4) nbr4->addNeighbor(nbr4->hasAnySharedEdge(*this), this);
	}
	break;
	case TopLft:
	{
		// add siblings
		addNeighbor(Btm, m_parent->m_children.at(BtmLft).get());
		addNeighbor(Rht, m_parent->m_children.at(TopRht).get());

		// add non-siblings
		auto* nbr3 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Top));
		addNeighbor(Top, nbr3);
		if (nbr3) nbr3->addNeighbor(nbr3->hasAnySharedEdge(*this), this);
		auto* nbr4 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Lft));
		addNeighbor(Lft, nbr4);
		if (nbr4) nbr4->addNeighbor(nbr4->hasAnySharedEdge(*this), this);
	}
	break;
	case TopRht:
	{
		// add siblings
		addNeighbor(Btm, m_parent->m_children.at(BtmRht).get());
		addNeighbor(Lft, m_parent->m_children.at(TopLft).get());

		// add non-siblings
		auto* nbr3 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Top));
		addNeighbor(Top, nbr3);
		if (nbr3) nbr3->addNeighbor(nbr3->hasAnySharedEdge(*this), this);
		auto* nbr4 = parentsNeighborsChildSharingEdge(getParentsNeighbor(Rht));
		addNeighbor(Rht, nbr4);
		if (nbr4) nbr4->addNeighbor(nbr4->hasAnySharedEdge(*this), this);
	}
	break;
	case RootQuad:
		assert(false && "SphericalTerrainQuadTreeNode::addNeighbors can't be called on root face node.");
		break;
	}
}

void SphericalTerrainQuadTreeNode::addNeighborsRoot()
{
	assert(m_depth == 0);
	const auto& faces = m_root->m_slices;
	switch (m_slice)
	{
	case PositiveX:
		addNeighbor(Top, faces[PositiveY].get());
		addNeighbor(Btm, faces[NegativeY].get());
		addNeighbor(Lft, faces[PositiveZ].get());
		addNeighbor(Rht, faces[NegativeZ].get());
		break;
	case NegativeX:
		addNeighbor(Top, faces[PositiveY].get());
		addNeighbor(Btm, faces[NegativeY].get());
		addNeighbor(Lft, faces[NegativeZ].get());
		addNeighbor(Rht, faces[PositiveZ].get());
		break;
	case PositiveY:
		addNeighbor(Top, faces[NegativeZ].get());
		addNeighbor(Btm, faces[PositiveZ].get());
		addNeighbor(Lft, faces[NegativeX].get());
		addNeighbor(Rht, faces[PositiveX].get());
		break;
	case NegativeY:
		addNeighbor(Top, faces[PositiveZ].get());
		addNeighbor(Btm, faces[NegativeZ].get());
		addNeighbor(Lft, faces[NegativeX].get());
		addNeighbor(Rht, faces[PositiveX].get());
		break;
	case PositiveZ:
		addNeighbor(Top, faces[PositiveY].get());
		addNeighbor(Btm, faces[NegativeY].get());
		addNeighbor(Lft, faces[NegativeX].get());
		addNeighbor(Rht, faces[PositiveX].get());
		break;
	case NegativeZ:
		addNeighbor(Top, faces[PositiveY].get());
		addNeighbor(Btm, faces[NegativeY].get());
		addNeighbor(Lft, faces[PositiveX].get());
		addNeighbor(Rht, faces[NegativeX].get());
		break;
	default: break;
	}
}

SphericalTerrainQuadTreeNode::Edge SphericalTerrainQuadTreeNode::getEdgeSharedWith(const SphericalTerrainQuadTreeNode& neighbor, const Edge neighborsEdge) const
{
	if (this == &neighbor)
	{
		switch (neighborsEdge)
		{
		case Top: return Btm;
		case Btm: return Top;
		case Lft: return Rht;
		case Rht: return Lft;
		case NoneEdge:
		default: return NoneEdge;
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		if (&neighbor == getNeighbor(static_cast<Edge>(i)))
		{
			return static_cast<Edge>(i);
		}
	}
	return NoneEdge;
}

std::pair<SphericalTerrainQuadTreeNode*, SphericalTerrainQuadTreeNode*> SphericalTerrainQuadTreeNode::getChildrenSharing(const Edge edge) const
{
	switch (edge)
	{
	case Top:
		return { getChild(TopLft), getChild(TopRht) };
	case Btm:
		return { getChild(BtmLft), getChild(BtmRht) };
	case Lft:
		return { getChild(BtmLft), getChild(TopLft) };
	case Rht:
		return { getChild(BtmRht), getChild(TopRht) };
	default:
		return { nullptr, nullptr };
	}
}

bool SphericalTerrainQuadTreeNode::isNeighborsChildSubdivided() const
{
	for (int i = 0; i < 4; ++i)
	{
		const auto edge      = static_cast<Edge>(i);
		const auto* neighbor = getNeighbor(edge);
		if (neighbor && !neighbor->m_active && neighbor->hasChildren())
		{
			const Edge neighborEdge = neighbor->getEdgeSharedWith(*this, edge);
			const auto [c0, c1]     = neighbor->getChildrenSharing(neighborEdge);
			if ((c0 && !c0->m_active && c0->hasChildren()) ||
				(c1 && !c1->m_active && c1->hasChildren()))
			{
				return true;
			}
		}
	}
	return false;
}

void SphericalTerrainQuadTreeNode::createChildren(void* memory)
{
	assert(!hasChildren());

	m_children[0] = std::make_unique<SphericalTerrainQuadTreeNode>(m_root, this, m_slice, BtmLft, m_depth + 1, m_levelIndexX << 1, m_levelIndexY << 1);
	m_children[1] = std::make_unique<SphericalTerrainQuadTreeNode>(m_root, this, m_slice, BtmRht, m_depth + 1, m_levelIndexX << 1 | 1, m_levelIndexY << 1);
	m_children[2] = std::make_unique<SphericalTerrainQuadTreeNode>(m_root, this, m_slice, TopLft, m_depth + 1, m_levelIndexX << 1, m_levelIndexY << 1 | 1);
	m_children[3] = std::make_unique<SphericalTerrainQuadTreeNode>(m_root, this, m_slice, TopRht, m_depth + 1, m_levelIndexX << 1 | 1, m_levelIndexY << 1 | 1);
}

void SphericalTerrainQuadTreeNode::destroyChildren()
{
	for (auto&& child : m_children)
	{
		child.reset();
	}
}

SphericalTerrainQuadTreeNode::EdgeMask SphericalTerrainQuadTreeNode::getIndexBufferType() const
{
	EdgeMask mask = 0;
	for (int i = 0; i < 4; ++i)
	{
		mask |= static_cast<EdgeMask>(m_neighbors[i] == nullptr) << i;  // null means no neighbors at same level
	}
	return mask;
}

void SphericalTerrainQuadTreeNode::setIndexBuffer(RcBuffer* ib, const int indexCount)
{
	clearIndexBuffer();

	m_renderOperation.m_stMeshData.m_IndexBufferData.m_indexBuffer = ib;
	m_renderOperation.m_stMeshData.m_IndexBufferData.m_indexCount  = indexCount;
	m_renderOperation.m_stMeshData.m_IndexBufferData.m_indexType   = ECHO_INDEX_TYPE;
	m_renderOperation.m_stMeshData.m_indexBufferSize               = indexCount * sizeof(IndexType);
	m_renderOperation.mPrimitiveType                                  = ECHO_TRIANGLE_LIST;
}

void SphericalTerrainQuadTreeNode::setCustomParameters(const Pass* pPass)
{
	const auto slice = getBiomeTextureSlice();

	ECHO_SAMPLER_STATE linearClamp;
	linearClamp.Filter = ECHO_TEX_FILTER_TRILINEAR;

	ECHO_SAMPLER_STATE pointSampler;

	const RenderSystem* system = Root::instance()->getRenderSystem();
	system->setUniformValue(this, pPass, U_VSCustom0, Vector4(getTranslationLocal()));

	Matrix4 trans = getTransformationLocal();
	system->setUniformValue(this, pPass, U_VSCustom2, &trans, sizeof(Matrix4));

	float uvScale = 1.f / float(1 << m_depth);
	float tileSize = float(getLengthOnCube(m_depth) * m_root->m_radius);
	float baseTile = floor(tileSize / 4.f);

	Vector4 VSCustom1 = {};
	VSCustom1.x = float(m_levelIndexX * uvScale);//x uv offset
	VSCustom1.y = float(m_levelIndexY * uvScale);
	VSCustom1.z = uvScale;	// mat id uv
	VSCustom1.w = uvScale;
	system->setUniformValue(this, pPass, U_VSCustom1, &VSCustom1, sizeof(VSCustom1));
	system->setUniformValue(this, pPass, U_VSCustom3, Vector4(baseTile));	//uv base tilling

	Vector4 VSCustom4 = m_root ? Vector4(float(m_root->m_pos.x), float(m_root->m_pos.y), float(m_root->m_pos.z), 1.0f) : Vector4(0.0f);
	system->setUniformValue(this, pPass, U_VSCustom4, VSCustom4);	//planetCenter

	// baked material
	// if (m_root->m_lowLod)
	// {
	// 	system->setTextureSampleValue(this, pPass, S_CubeMapExt0, m_root->m_terrainData->m_bakedAlbedo, linearClamp);
	// 	return;
	// }

	// real-time material blending
	Vector4 PSCustom1[2] = {};
	PSCustom1[0].x = m_root->m_radius / 4.f;
	PSCustom1[0].y = m_root->m_normalStrength;
	PSCustom1[0].z = m_root->m_weightParamM;
	PSCustom1[0].w = m_root->m_weightParamDelta;

	PSCustom1[1].x = m_root->getDissolveStrength();
	system->setUniformValue(this, pPass, U_PSCustom1, &PSCustom1, sizeof(Vector4) * 2);

	//note by hejiwei:目前最多支持16层材质，每层材质需要一个float类型的tilling和一个float类型的normalstrength
	//16 * 2 个float变量塞到4 * 2个vec4中用uniform传到shader里面
	const auto& biomes = m_root->m_terrainData->composition.usedBiomeTemplateList;
	size_t biomeSize   = biomes.size();

	const int customParamUniformCount                        = SphericalTerrain::MaxMaterialTypes / 4; // = 16 / 4
	const int textureEmissionUniformCount					 = customParamUniformCount * 2;
	Vector4 PSUniformCustomTilling[customParamUniformCount]  = {};
	Vector4 PSUniformNormalStrength[customParamUniformCount] = {};
	Vector4 PSUniformTextureEmission[textureEmissionUniformCount];

	ColorValue PSUniformCustomEmission[SphericalTerrain::MaxMaterialTypes];
	ColorValue blendCol[SphericalTerrain::MaxMaterialTypes];

	for (int i = 0; i < biomeSize; i++)
	{
        if(i < SphericalTerrain::MaxMaterialTypes)
        {
            PSUniformCustomTilling[i / 4][i % 4]  = biomes[i].customTilling;
            PSUniformNormalStrength[i / 4][i % 4] = biomes[i].normalStrength;
			PSUniformCustomEmission[i] = ColorValue::gammaToLinear(biomes[i].emission);
			PSUniformCustomEmission[i].a = biomes[i].emission.a;
            blendCol[i] = ColorValue::gammaToLinear(biomes.at(i).diffuseColor);
			PSUniformTextureEmission[i / 4][i % 4] = biomes[i].enableTextureEmission ? 1.f : 0.f;
			PSUniformTextureEmission[i / 4 + 4][i % 4] = biomes[i].customMetallic;
        }
	}

	system->setUniformValue(this, pPass, U_PSCustom2, &PSUniformCustomTilling[0], sizeof(Vector4) * customParamUniformCount);
	system->setUniformValue(this, pPass, U_PSCustom6, &PSUniformNormalStrength[0], sizeof(Vector4) * customParamUniformCount);
	system->setUniformValue(this, pPass, U_PSCustom3, blendCol, sizeof(blendCol));
	system->setUniformValue(this, pPass, U_PSCustom7, &PSUniformCustomEmission[0], sizeof(ColorValue) * SphericalTerrain::MaxMaterialTypes);
	system->setUniformValue(this, pPass, U_PSCustom8, &PSUniformTextureEmission, sizeof(Vector4) * textureEmissionUniformCount);

	Vector4 PSCustom5 = Vector4(0.8f);//weatherCoverColorThreshold
	system->setUniformValue(this, pPass, U_PSCustom5, &PSCustom5, sizeof(Vector4));

	Vector4 PSCustom10 = m_root->getPlanetRegionParameters(); // xy: Params.xy zw: offset.xy
	system->setUniformValue(this, pPass, U_PSCustom10, &PSCustom10, sizeof(PSCustom10));
#ifdef ECHO_EDITOR
	if (m_root->m_editingTerrain)
	{
		if (m_root->isTorus())
		{
			Vector4 u4(getTranslationLocal());		// For torus height visualization.
			u4.w = 1.0f / m_root->m_relativeMinorRadius;
			system->setUniformValue(this, pPass, U_PSCustom4, u4);
		}

		RcSampler* temperatureTex   = m_root->getTemperatureComputeTargets()[slice];
		RcSampler* humidityTex      = m_root->getHumidityComputeTargets()[slice];
		RcSampler* materialIndexTex = m_root->getMaterialIndexComputeTargets()[slice];
		if (temperatureTex)
		{
			system->setTextureSampleValue(this, pPass, S_Diffuse, temperatureTex, linearClamp);
		}

        if (humidityTex)
        {
            system->setTextureSampleValue(this, pPass, S_Specular, humidityTex, linearClamp);
        }

        if (materialIndexTex)
        {
            system->setTextureSampleValue(this, pPass, S_Custom5, materialIndexTex, pointSampler);
        }

        // 支持编辑器几何热力图显示，仅在该显示模式下绑定：S_Diffuse <- heatmap 面纹理
        if (m_root && m_root->m_displayMode == BiomeDisplayMode::GeometryHeatmap)
        {
            TexturePtr heatmapTex = m_root->SelectGeometryHeatmapRef(slice);
            if (!heatmapTex.isNull())
            {
                system->setTextureSampleValue(this, pPass, S_Diffuse, heatmapTex, linearClamp);
            }
        }

		bool isInit = false;
		{
			auto iSampler = pointSampler;
			if (m_root->isTorus())
			{
				iSampler.AddressU = iSampler.AddressV = ECHO_TEX_ADDRESS_WRAP;
			}
			auto texture = m_root->SelectPlanetRegionRef(slice);

			if (!texture.isNull() && texture->getRcTex())
			{
				system->setTextureSampleValue(this, pPass, S_Custom9, texture->getRcTex(), iSampler);
				isInit = true;
			}

			// silence D3D11 error message about s3 register format mismatch between "R8_UINT" and "UNORM"
			if (!isInit)
			{
				static auto texture = Root::instance()->getRenderSystem()->createTextureRaw(Name("EditorDefaultRegionMap"), ECHO_FMT_R8UI, 1, 1, nullptr, 1);
				system->setTextureSampleValue(this, pPass, S_Custom9, texture, pointSampler);
			}
		}

		Vector4 PSCustom0 = {};
		PSCustom0.x       = (float)m_root->m_displayMode;
		PSCustom0.y       = static_cast<float>(m_depth);
		PSCustom0.z       = m_root->m_displayMode == BiomeDisplayMode::RegionIndexOffline ? (float)std::max(m_root->m_terrainData->region.RegionNum, 1) : (float)(m_root->m_terrainData->region.biomeArray.size() > 0 ? m_root->m_terrainData->region.biomeArray.size() : 1);
		system->setUniformValue(this, pPass, U_PSCustom0, &PSCustom0, sizeof(PSCustom0));

		if (m_root->bDisplayCoarseLayer)
		{
			auto PSCustom12 = m_root->GetHierarchicalSphericalVoronoiParents();
			system->setUniformValue(this, pPass, U_PSCustom12, PSCustom12.data(), (int)PSCustom12.size() * sizeof(int));
			system->setUniformValue(this, pPass, U_PSCustom9, m_root->m_adjacent_coarse_regions.data(), (int)m_root->m_adjacent_coarse_regions.size() * sizeof(int));
		}
		else
		{
			system->setUniformValue(this, pPass, U_PSCustom9, m_root->m_adjacent_fine_regions.data(), (int)m_root->m_adjacent_fine_regions.size() * sizeof(int));
		}

		const Matrix4& PSCustom13 = m_root->m_worldMatInv;
		system->setUniformValue(this, pPass, U_PSCustom13, &PSCustom13, sizeof(PSCustom13));
		const Vector4& PSCustom14 = m_root->m_MLSphericalVoronoiParams;
		system->setUniformValue(this, pPass, U_PSCustom14, &PSCustom14, sizeof(PSCustom14));

		int VSCustom14[4] = {};
		VSCustom14[0]     = (int)m_root->m_bDisplay;

		Matrix4 VSCustom15 = m_root->m_StampTerrainMtx;

		system->setUniformValue(
			this, pPass, U_VSCustom14, &VSCustom14, sizeof(VSCustom14));
		system->setUniformValue(
			this, pPass, U_VSCustom15, &VSCustom15, sizeof(VSCustom15));

		const Vector4 colors[] = {
			{ 0.5, 0, 0, 1 },
			{ 0, 0.5, 0, 1 },
			{ 0, 0, 0.5, 1 },
			{ 0.5, 0.5, 0, 1 },
			{ 0, 0.5, 0.5, 1 }
		};
		system->setUniformValue(this, pPass, U_PSCustom15, colors[m_root->getNodeIndex(*this) % std::size(colors)]);
		return;
	}
#endif

	TexturePtr sphericalVoronoiRegionTex = m_root->SelectPlanetRegionRef(slice);
	if (m_root->m_showSphericalVoronoiRegion && !sphericalVoronoiRegionTex.isNull())
	{
		const Matrix4& PSCustom13 = m_root->m_worldMatInv;
		system->setUniformValue(this, pPass, U_PSCustom13, &PSCustom13, sizeof(PSCustom13));
		system->setTextureSampleValue(this, pPass, S_Custom5, sphericalVoronoiRegionTex, pointSampler);
	}
	else if (!m_root->m_terrainData->m_planetTexRes.isNull())
	{
		const auto& materialIndexTex = m_root->m_terrainData->m_planetTexRes->m_biomeGPUData.matIDTex[slice];
		system->setTextureSampleValue(this, pPass, S_Custom5, materialIndexTex, pointSampler);
	}
	else
	{
		system->setTextureSampleValue(this, pPass, S_Custom5, nullptr, pointSampler);
	}
}

void SphericalTerrainQuadTreeNode::getWorldTransforms(const DBMatrix4** xform) const
{
	*xform = &m_world;
}

void SphericalTerrainQuadTreeNode::getWorldTransforms(DBMatrix4* xform) const
{
	*xform = m_world;
}

void SphericalTerrainQuadTreeNode::getRenderableWorldScale(const Vector3** scale) const
{
	*scale = &m_root->m_realScale;
}

void SphericalTerrainQuadTreeNode::getRenderableWorldOrientation(const Quaternion** ori) const
{
	*ori = &m_root->m_rot;
}

void SphericalTerrainQuadTreeNode::getRenderableWorldPosition(const DBVector3** pos) const
{
	static DBVector3 worldPosition;
	worldPosition = static_cast<DBVector3>(getTranslationLocal() * m_root->m_realScale) + m_root->m_pos;
	*pos = &worldPosition;
}

const Material* SphericalTerrainQuadTreeNode::getMaterial() const
{
	assert(m_root->m_material.isV1());
	return m_root->m_material.getV1();
}

MaterialWrapper SphericalTerrainQuadTreeNode::getMaterialWrapper() const
{
	return m_root->m_material;
}

const LightList& SphericalTerrainQuadTreeNode::getLights(void) const
{
	return SphericalTerrain::s_lightList;
}

void SphericalTerrainQuadTreeNode::calcSurfaceArea()
{
	const Vector3 p0  = MapToSphere(m_slice, m_offset);
	const Vector3 p1  = MapToSphere(m_slice, m_offset + Vector2(m_length.x, 0));
	const Vector3 p2  = MapToSphere(m_slice, m_offset + Vector2(0, m_length.y));
	const Vector3 p3  = MapToSphere(m_slice, m_offset + m_length);
	const Vector3 v01 = p1 - p0;
	const Vector3 v13 = p3 - p1;
	const Vector3 v32 = p2 - p3;
	const Vector3 v20 = p0 - p2;

	auto sphericalAngle = [](const Vector3& n, Vector3 s, Vector3 t)
	{
		s -= s.Dot(n) * n;
		s.normalise();
		t -= t.Dot(n) * n;
		t.normalise();
		return std::acosf(s.Dot(t));
	};

	m_areaSqrt =
		(sphericalAngle(p0, v01, -v20) - ECHO_HALF_PI) +
		(sphericalAngle(p1, v13, -v01) - ECHO_HALF_PI) +
		(sphericalAngle(p2, v20, -v32) - ECHO_HALF_PI) +
		(sphericalAngle(p3, v32, -v13) - ECHO_HALF_PI);
	m_areaSqrt = std::sqrtf(m_areaSqrt);
	if (m_root->m_pTerrainGenerator) {
		m_areaSqrt *= m_root->m_pTerrainGenerator->getSqrtAreaRatio();
	}
}

namespace Echo {
	struct ConvexMeshGravity {

		struct PlaneGravity {
			PlaneGravity() {}
			PlaneGravity(uint32 _index, float _distance) : planeIndex(_index), distance(_distance) {}
			uint32 planeIndex = 0;
			float distance = 0.0f;
		};

		bool initialize(const Name& file) {
			DataStreamPtr stream(Root::instance()->GetFileDataStream(file.c_str()));
			if (stream.isNull()) return false;

			uint32 version;
			uint32 mpSize;
			uint32 planeSize;
			float sqrtAreaRatio;
			stream->readData(&version, 1);

			if (version < 2 || version > 2) return false;

			stream->readData(&mpSize, 1);
			stream->readData(&planeSize, 1);
			stream->readData(&sqrtAreaRatio, 1);

			if (planeSize > 0) {
				std::vector<Vector4> planes(planeSize);
				stream->read(planes.data(), planeSize * sizeof(Vector4));
				mPlanes.resize(planeSize);
				for (int i = 0; i != planeSize; ++i) {
					mPlanes[i].first = Vector3(planes[i].x, planes[i].y, planes[i].z);
					mPlanes[i].second = planes[i].w;
				}
			}

			stream->close();
			return true;
		}


		Vector3 getGravity(const Vector3& _position, float _scale) const {
			if (mPlanes.empty()) {
				return -_position.GetNormalized();
			}
			uint32 planeSize = (uint32)mPlanes.size();

			int zeroGravitySize = 0;
			uint8 curGravitySize = 0;
			PlaneGravity curGravitys[256];
			PlaneGravity minInsidePlane(planeSize, -100.0f);

			for (uint32 polIndex = 0; polIndex != planeSize; ++polIndex) {
				float distance = mPlanes[polIndex].first.dotProduct(_position) + mPlanes[polIndex].second * _scale;
				if (distance < 0.0f) {
					if (distance > minInsidePlane.distance) {
						minInsidePlane.planeIndex = polIndex;
						minInsidePlane.distance = distance;
					}
					continue;
				}
				if (distance < 0.000001f) {
					++zeroGravitySize;
				}
				PlaneGravity& curGravity = curGravitys[curGravitySize++];
				curGravity.planeIndex = polIndex;
				curGravity.distance = distance;
			}
			if (curGravitySize == 0) {
				if (minInsidePlane.planeIndex == planeSize) {
					return -_position.GetNormalized();
				}
				else {
					return -mPlanes[minInsidePlane.planeIndex].first;
				}
			}
			Vector3 gravity = Vector3::ZERO;
			if (curGravitySize == zeroGravitySize) {
				for (int i = 0; i != curGravitySize; ++i) {
					gravity -= mPlanes[curGravitys[i].planeIndex].first;
				}
			}
			else {
				for (int i = 0; i != curGravitySize; ++i) {
					gravity -= mPlanes[curGravitys[i].planeIndex].first * curGravitys[i].distance;
				}
			}
			gravity.Normalize();
			return gravity;
		}

		bool pointIsInside(const Vector3& _position, float _scale) const {
			if (mPlanes.empty()) {
				return _position.LengthSq() <= _scale * _scale;
			}
			uint32 planeSize = (uint32)mPlanes.size();
			for (uint32 polIndex = 0; polIndex != planeSize; ++polIndex) {
				float distance = mPlanes[polIndex].first.dotProduct(_position) + mPlanes[polIndex].second * _scale;
				if (distance > 0.0f) return false;
			}
			return true;
		}
		std::vector<std::pair<Vector3, float>> mPlanes;
	};
	struct ConvexMeshGravityNew {

		ConvexMeshGravityNew() {}
		~ConvexMeshGravityNew() { SAFE_DELETE(mPolyhedronQuery); }

		bool initialize(const String& file) {
			DataStreamPtr stream(Root::instance()->GetFileDataStream(file.c_str()));
			mPolyhedronQuery = new PolyhedronQuery(stream);
			return true;
		}

		Vector3 getGravity(const Vector3& _position, float _scale) const {
			if (mPolyhedronQuery) {
				return mPolyhedronQuery->getGravity(_position);
			}
			else {
				return -_position.GetNormalized();
			}
		}

		bool pointIsInside(const Vector3& _position, float _scale) const {
			return _position.LengthSq() <= _scale * _scale;
		}
		float getBorder(const Vector3& _position) const {
			return mPolyhedronQuery->getBorder(_position);
		}

		PolyhedronQuery* mPolyhedronQuery = nullptr;
	};
	struct GeometryHeightMap {
	public:
		GeometryHeightMap() {}
		~GeometryHeightMap() {
#ifdef ECHO_EDITOR
			destroyTexture();
#endif // ECHO_EDITOR
		}
		bool initialize(const Name& file) {
			DataStreamPtr stream(Root::instance()->GetFileDataStream(file.c_str()));
			if (stream.isNull()) return false;

			uint32 version;
			uint32 mpSize;
			uint32 planeSize;

			stream->readData(&version, 1);

			if (version < 2 || version > 2) return false;

			stream->readData(&mpSize, 1);
			stream->readData(&planeSize, 1);

			stream->readData(&mSqrtAreaRatio, 1);

			if (planeSize > 0) {
				stream->seek(stream->tell() + planeSize * sizeof(Vector4));
			}

			if (mpSize > 0) {
				mMapSize = mpSize;
				for (int face = 0; face < 6; ++face)
				{
					mHeightmaps[face].resize(mMapSize * mMapSize);
					stream->read(mHeightmaps[face].data(), mHeightmaps[face].size() * sizeof(float));
				}
				mMapSizeSub1 = mMapSize - 1;
			}
			
			stream->close();
			return true;
		}

		float _getHeight(SphericalTerrainQuadTreeNode::Face face, int _x, int _y) const {
			int x = Math::Clamp(_x, 0, mMapSizeSub1);
			int y = Math::Clamp(_y, 0, mMapSizeSub1);
			return mHeightmaps[face][x * mMapSize + y];
		}

		float calWeight(float delta) const {
			if (delta < 1.0f) {
				float delta2 = delta * delta;
				return 1.0f - 2.0f * delta2 + delta2 * delta;
			}
			else if (delta < 2.0f) {
				float delta2 = delta * delta;
				return 4.0f - 8.0f * delta + 5.0f * delta2 - delta2 * delta;
			}
			else {
				return 0.0f;
			}
		}
		
		float bicubicInterpolation(SphericalTerrainQuadTreeNode::Face face, int xI, int yI, float deltaX, float deltaY) const {
			Vector4 xW(calWeight(1.0f + deltaX), calWeight(deltaX), calWeight(1.0f - deltaX), calWeight(2.0f - deltaX));
			Vector4 yW(calWeight(1.0f + deltaY), calWeight(deltaY), calWeight(1.0f - deltaY), calWeight(2.0f - deltaY));
			float height = 0.0f;
			for (int i = 0; i != 4; ++i) {
				for (int j = 0; j != 4; ++j) {
					height += _getHeight(face, xI - 1 + i, yI - 1 + j) * xW[i] * yW[j];
				}
			}
			return height;
		}

		float bilinearInterpolation(SphericalTerrainQuadTreeNode::Face face, int xI, int yI, float deltaX, float deltaY) const {
			Vector4 v4;
			v4.w = _getHeight(face, xI, yI);
			v4.z = _getHeight(face, xI + 1, yI);
			v4.y = _getHeight(face, xI + 1, yI + 1);
			v4.x = _getHeight(face, xI, yI + 1);
			float posTerrainHeight = deltaY * ((1.0f - deltaX) * v4.x + deltaX * v4.y) + (1.0f - deltaY) * ((1.0f - deltaX) * v4.w + deltaX * v4.z);
			return posTerrainHeight;
		}

		float getHeight(SphericalTerrainQuadTreeNode::Face face, float _x, float _y) const {
			if (_x < -1.0f || _y < -1.0f || _x > 1.0f || _y > 1.0f) {
				Vector2 xy(_x, _y);
				Vector3 normal = SphericalTerrainQuadTreeNode::MapToSphere(face, xy);
				SphericalTerrainQuadTreeNode::MapToCube(normal, face, xy);
				_x = xy.x;
				_y = xy.y;
			}
			float x = (_x + 1.0f) * 0.5f;
			float y = (_y + 1.0f) * 0.5f;
			float xF = (float)(x * (float)mMapSizeSub1);
			float yF = (float)(y * (float)mMapSizeSub1);
			float xMinF = std::floor(xF);
			float yMinF = std::floor(yF);
			float deltaX = xF - xMinF;
			float deltaY = yF - yMinF;
			int xI = (int)xMinF, yI = (int)yMinF;

			return bilinearInterpolation(face, xI, yI, deltaX, deltaY);
		}

		float getHeight(SphericalTerrainQuadTreeNode::Face face, int xI, int yI, float xF, float yF) const {
			return bilinearInterpolation(face, xI, yI, xF, yF);
		}

		std::vector<float> mHeightmaps[6];
		int mMapSize = 0;
		int mMapSizeSub1 = 0;
		float mSqrtAreaRatio = 1.0f;

#ifdef ECHO_EDITOR
		void createTexture() {
			if (!mTexture.isNull()) return;
			mTexture = TextureManager::instance()->createManual(TEX_TYPE_2D_ARRAY, (uint32)mMapSize, (uint32)mMapSize, 6, 1, ECHO_FMT_R_FP32, 0, 0);
			mTexture->load();
			for (int i = 0; i != 6; ++i) {
				mTexture->setTex2DArrayRawData(mHeightmaps[i].data(), (uint32)mHeightmaps[i].size() * sizeof(float), i, false);
			}
		}
		void destroyTexture() {
			mTexture.setNull();
		}
		TexturePtr mTexture;
#endif // ECHO_EDITOR
	};

	struct GeometryBorderMap {
	public:
		GeometryBorderMap() {}
		~GeometryBorderMap() {
#ifdef ECHO_EDITOR
			destroyTexture();
#endif // ECHO_EDITOR
		}
		bool initialize(const String& file) {
			DataStreamPtr stream(Root::instance()->GetFileDataStream(file.c_str()));
			if (stream.isNull()) return false;

			uint32 mpSize;
			stream->readData(&mpSize, 1);

			if (mpSize > 0) {
				mMapSize = mpSize;
				for (int face = 0; face < 6; ++face)
				{
					mHeightmaps[face].resize(mMapSize * mMapSize);
					stream->read(mHeightmaps[face].data(), mHeightmaps[face].size() * sizeof(float));
				}
				mMapSizeSub1 = mMapSize - 1;
			}

			stream->close();
			return true;
		}

		float _getHeight(SphericalTerrainQuadTreeNode::Face face, int _x, int _y) const {
			int x = Math::Clamp(_x, 0, mMapSizeSub1);
			int y = Math::Clamp(_y, 0, mMapSizeSub1);
			return mHeightmaps[face][x * mMapSize + y];
		}

		float bilinearInterpolation(SphericalTerrainQuadTreeNode::Face face, int xI, int yI, float deltaX, float deltaY) const {
			Vector4 v4;
			v4.w = _getHeight(face, xI, yI);
			v4.z = _getHeight(face, xI + 1, yI);
			v4.y = _getHeight(face, xI + 1, yI + 1);
			v4.x = _getHeight(face, xI, yI + 1);
			float posTerrainHeight = deltaY * ((1.0f - deltaX) * v4.x + deltaX * v4.y) + (1.0f - deltaY) * ((1.0f - deltaX) * v4.w + deltaX * v4.z);
			return posTerrainHeight;
		}

		float getHeight(SphericalTerrainQuadTreeNode::Face face, float _x, float _y) const {
			if (_x < -1.0f || _y < -1.0f || _x > 1.0f || _y > 1.0f) {
				Vector2 xy(_x, _y);
				Vector3 normal = SphericalTerrainQuadTreeNode::MapToSphere(face, xy);
				SphericalTerrainQuadTreeNode::MapToCube(normal, face, xy);
				_x = xy.x;
				_y = xy.y;
			}
			float x = (_x + 1.0f) * 0.5f;
			float y = (_y + 1.0f) * 0.5f;
			float xF = (float)(x * (float)mMapSizeSub1);
			float yF = (float)(y * (float)mMapSizeSub1);
			float xMinF = std::floor(xF);
			float yMinF = std::floor(yF);
			float deltaX = xF - xMinF;
			float deltaY = yF - yMinF;
			int xI = (int)xMinF, yI = (int)yMinF;

			return bilinearInterpolation(face, xI, yI, deltaX, deltaY);
		}
		float getHeight(SphericalTerrainQuadTreeNode::Face face, int xI, int yI, float xF, float yF) const {
			return bilinearInterpolation(face, xI, yI, xF, yF);
		}
		float getHeight(Vector3 _position) const {
			SphericalTerrainQuadTreeNode::Face face;
			float _x = 0.0f;
			float _y = 0.0f;
			Vector2 xy;
			SphericalTerrainQuadTreeNode::MapToCube(_position.GetNormalized(), face, xy);
			_x = xy.x;
			_y = xy.y;
			float x = (_x + 1.0f) * 0.5f;
			float y = (_y + 1.0f) * 0.5f;
			float xF = (float)(x * (float)mMapSizeSub1);
			float yF = (float)(y * (float)mMapSizeSub1);
			float xMinF = std::floor(xF);
			float yMinF = std::floor(yF);
			float deltaX = xF - xMinF;
			float deltaY = yF - yMinF;
			int xI = (int)xMinF, yI = (int)yMinF;

			return bilinearInterpolation(face, xI, yI, deltaX, deltaY);
		}

		std::vector<float> mHeightmaps[6];
		int mMapSize = 0;
		int mMapSizeSub1 = 0;

#ifdef ECHO_EDITOR
		void createTexture() {
			if (!mTexture.isNull()) return;
			mTexture = TextureManager::instance()->createManual(TEX_TYPE_2D_ARRAY, (uint32)mMapSize, (uint32)mMapSize, 6, 1, ECHO_FMT_R_FP32, 0, 0);
			mTexture->load();
			for (int i = 0; i != 6; ++i) {
				mTexture->setTex2DArrayRawData(mHeightmaps[i].data(), (uint32)mHeightmaps[i].size() * sizeof(float), i, false);
			}
		}
		void destroyTexture() {
			mTexture.setNull();
		}
		TexturePtr mTexture;
#endif // ECHO_EDITOR
	};

	struct TerrainGeneratorPrivate {
		TerrainGeneratorPrivate() {
			m_pGravity = new ConvexMeshGravity();
			m_pHeightMap = new GeometryHeightMap();
		}
		~TerrainGeneratorPrivate() {
			SAFE_DELETE(m_pGravity);
			SAFE_DELETE(m_pHeightMap);
			SAFE_DELETE(m_pGravityNew);
			SAFE_DELETE(m_pBorderMap);
		}
		bool reloadGravity(const Name& file) {
			SAFE_DELETE(m_pGravity);
			SAFE_DELETE(m_pGravityNew);
			m_pGravity = new ConvexMeshGravity();
			return m_pGravity->initialize(file);
		}
		bool reloadGravityNew(const String& file) {
			SAFE_DELETE(m_pGravity);
			SAFE_DELETE(m_pGravityNew);
			m_pGravityNew = new ConvexMeshGravityNew();
			return m_pGravityNew->initialize(file);
		}
		bool reloadHeightMap(const Name& file) {
			SAFE_DELETE(m_pHeightMap);
			m_pHeightMap = new GeometryHeightMap();
			return m_pHeightMap->initialize(file);
		}
		bool reloadBorderMap(const String& file) {
			SAFE_DELETE(m_pBorderMap);
			m_pBorderMap = new GeometryBorderMap();
			return m_pBorderMap->initialize(file);
		}

		Vector3 getGravity(const Vector3& _position, float _scale) const {
			if (m_pGravityNew) {
				return m_pGravityNew->getGravity(_position, _scale);
			}
			else if (m_pGravity) {
				return m_pGravity->getGravity(_position, _scale);
			}
			else {
				return -_position.GetNormalized();
			}
		}

		bool pointIsInside(const Vector3& _position, float _scale) const {
			if (m_pGravity) {
				return m_pGravity->pointIsInside(_position, _scale);
			}
			else if (m_pGravityNew) {
				return m_pGravityNew->pointIsInside(_position, _scale);
			}
			else {
				return _position.LengthSq() <= _scale * _scale;
			}
		}

		std::pair<float,float> getHeightAndBorder(SphericalTerrainQuadTreeNode::Face face, float _x, float _y) const {
			if (_x < -1.0f || _y < -1.0f || _x > 1.0f || _y > 1.0f) {
				Vector2 xy(_x, _y);
				Vector3 normal = SphericalTerrainQuadTreeNode::MapToSphere(face, xy);
				SphericalTerrainQuadTreeNode::MapToCube(normal, face, xy);
				_x = xy.x;
				_y = xy.y;
			}
			float x = (_x + 1.0f) * 0.5f;
			float y = (_y + 1.0f) * 0.5f;
			float xF = (float)(x * (float)m_pHeightMap->mMapSizeSub1);
			float yF = (float)(y * (float)m_pHeightMap->mMapSizeSub1);
			float xMinF = std::floor(xF);
			float yMinF = std::floor(yF);
			float deltaX = xF - xMinF;
			float deltaY = yF - yMinF;
			int xI = (int)xMinF, yI = (int)yMinF;

			float height = m_pHeightMap->getHeight(face, xI, yI, deltaX, deltaY);
			float border = m_pBorderMap->getHeight(face, xI, yI, deltaX, deltaY);

			return std::make_pair(height, border);
		}

		ConvexMeshGravity* m_pGravity = nullptr;
		ConvexMeshGravityNew* m_pGravityNew = nullptr;
		GeometryHeightMap* m_pHeightMap = nullptr;
		GeometryBorderMap* m_pBorderMap = nullptr;
	};

	TerrainGenerator::TerrainGenerator(uint32 type, const String& file, const String& mesh, const String& gravity, const String& border) : mType(type), mName(file), mMeshName(mesh), mGravityName(gravity), mBorderName(border) {
		m_pPrivate = new TerrainGeneratorPrivate();
		m_pPrivate->reloadGravity(Name(mName));
		if (!gravity.empty()) {
			m_pPrivate->reloadGravityNew(gravity);
		}
	}
	void TerrainGenerator::reload() {
		bSuccess = false;
		bSuccess = m_pPrivate->reloadHeightMap(Name(mName));
		if (!mBorderName.empty()) {
			m_pPrivate->reloadBorderMap(mBorderName);
		}
	}
	void TerrainGenerator::unload() {
		bSuccess = false;
		SAFE_DELETE(m_pPrivate->m_pHeightMap);
		SAFE_DELETE(m_pPrivate->m_pBorderMap);
	}
	TerrainGenerator::~TerrainGenerator() {
		SAFE_DELETE(m_pPrivate);
	}
	bool TerrainGenerator::isSuccess() const {
		return bSuccess;
	}
	bool TerrainGenerator::isNearby(const Vector3& position, float scale) const {
		return m_pPrivate->pointIsInside(position, scale);
	}
	Vector3 TerrainGenerator::getGravity(const Vector3& position, float scale) const {
		return m_pPrivate->getGravity(position, scale);
	}
	float TerrainGenerator::getHeight(SphericalTerrainQuadTreeNode::Face face, float x, float y) const {
		if (bSuccess) {
			return m_pPrivate->m_pHeightMap->getHeight(face, x, y);
		}
		else {
			return 1.0f;
		}
	}
	float TerrainGenerator::getSqrtAreaRatio() const {
		if (bSuccess) {
			return m_pPrivate->m_pHeightMap->mSqrtAreaRatio;
		}
		else {
			return 1.0f;
		}
	}
	float TerrainGenerator::getBorder(const Vector3& position) const {
		if (m_pPrivate->m_pGravityNew) {
			return m_pPrivate->m_pGravityNew->getBorder(position);
		}
		return 1.0f;
	}
	float TerrainGenerator::getBorderFace(const Vector3& position) const {
		if (m_pPrivate->m_pBorderMap) {
			return m_pPrivate->m_pBorderMap->getHeight(position);
		}
		return 1.0f;
	}
	std::pair<float, float> TerrainGenerator::getHeightAndBorder(SphericalTerrainQuadTreeNode::Face face, float x, float y) const {
		if (bSuccess) {
			return m_pPrivate->getHeightAndBorder(face, x, y);
		}
		else {
			return std::make_pair(1.0f, 1.0f);
		}		
	}

#ifdef ECHO_EDITOR
	TexturePtr TerrainGenerator::getTextureArray() {
		if (bSuccess) {
			m_pPrivate->m_pHeightMap->createTexture();
			return m_pPrivate->m_pHeightMap->mTexture;
		}
		else {
			return TexturePtr();
		}
	}
	TexturePtr TerrainGenerator::getBorderTextureArray() {
		if (m_pPrivate->m_pBorderMap) {
			m_pPrivate->m_pBorderMap->createTexture();
			return m_pPrivate->m_pBorderMap->mTexture;
		}
		else {
			return TexturePtr();
		}
	}
#endif // ECHO_EDITOR
}
