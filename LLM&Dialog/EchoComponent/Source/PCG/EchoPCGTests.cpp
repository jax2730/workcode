#include "EchoComponentStableHeaders.h"

#ifdef ECHO_EDITOR

#include "PCG/EchoPCGTests.h"

#include "PCG/EchoPCGGraph.h"
#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/Graph/EchoPCGGraphCompiler.h"
#include "PCG/Graph/EchoPCGGraphExecutor.h"

#include "PCG/Elements/EchoPCGLandscapeGetter.h"
#include "PCG/Elements/EchoPCGSurfaceSampler.h"
#include "PCG/Elements/EchoPCGStaticMeshSpawner.h"

#include "EchoPCGBaseElement.h"
#include "EchoPCGOctree.h"
#include "PCG/EchoInstancedStaticMeshComponent.h"
#include "EchoHelperPreloading.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace Test
		{
			using PCG::Octree;
			using PCG::OctreeElementId;

			struct AABBOctreeSemantics
			{
				enum { MaxElementsPerLeaf = 4 };
				enum { MinInclusiveElementsPerNode = 7 };
				enum { MaxNodeDepth = 12 };

				FORCEINLINE static const AxisAlignedBox& GetBoundingBox(const AxisAlignedBox& inAABB)
				{
					return inAABB;
				}

				FORCEINLINE static void SetElementId(const AxisAlignedBox& Element, OctreeElementId OctreeElementID)
				{
				}
			};

			using AABBOctree = Octree<AxisAlignedBox, AABBOctreeSemantics>;
			void TestOctree()
			{
				// Initialize octree: root node is a 2048x2048x2048 cube (centered at origin)
				Vector3 origin = { 0.0f, 0.0f, 0.0f };
				float extent = 1024.0f;
				AABBOctree octree{ origin, extent };

				// ================= Test Data Setup =================
				// Create test elements with various spatial relationships
				const std::vector<AxisAlignedBox> testElements = {
					// Base test element (full overlap with first query)
					{ { -128.0f, -128.0f, -128.0f }, { 128.0f, 128.0f, 128.0f } },

					// Fully inside query box
					{ { -32.0f, -32.0f, -32.0f }, { 16.0f, 16.0f, 16.0f } },

					// Partial overlap cases
					{ { -64.0f, -64.0f, -64.0f }, { 0.0f, 0.0f, 0.0f } },  // Touches query box
					{ { -128.0f, -128.0f, 0.0f },  { 0.0f, 0.0f, 64.0f } }, // Z-axis extension

					// Non-overlapping elements
					{ { 256.0f, 256.0f, 256.0f },  { 384.0f, 384.0f, 384.0f } },   // Positive quadrant
					{ { -512.0f, -512.0f, 256.0f }, { -384.0f, -384.0f, 384.0f } },// Negative XY

					// Giant element (covers entire octree)
					{ { -1024.0f, -1024.0f, -1024.0f }, { 1024.0f, 1024.0f, 1024.0f } },

					// Edge cases
					{ { -64.0f, -64.0f, -64.0f }, { 64.0f, 64.0f, 64.0f } }, // Exact query match
					{ { -65.0f, -64.0f, -64.0f }, { -64.0f, 64.0f, 64.0f } } // 1-unit offset
				};

				// Insert all test elements
				for (const auto& elem : testElements)
				{
					octree.AddElement(elem);
				}

				// ================= Test Cases =================
				// Test 1: Basic intersection (should hit)
				{
					AxisAlignedBox boxQ = { { -64.0f, -64.0f, -64.0f }, { 0.0f, 0.0f, 0.0f } };
					bool intersect = false;
					octree.FindFirstElementWithBoundsTest(boxQ, [&intersect](const AxisAlignedBox&)
						{
							intersect = true;
							return false;
						});
					assert(intersect && "Test1: Should detect base intersection");
				}

				// Test 2: Full containment (should hit)
				{
					AxisAlignedBox boxQ = { { -16.0f, -16.0f, -16.0f }, { 16.0f, 16.0f, 16.0f } };
					bool intersect = false;
					octree.FindFirstElementWithBoundsTest(boxQ, [&intersect](const AxisAlignedBox&)
						{
							intersect = true;
							return false;
						});
					assert(intersect && "Test2: Failed contained query");
				}

				// Test 3: Non-overlapping region (FIXED: should NOT hit)
				{
					// Query outside octree bounds (beyond root node)
					AxisAlignedBox boxQ = { { 1500.0f, 1500.0f, 1500.0f }, { 1600.0f, 1600.0f, 1600.0f } };
					bool intersect = false;
					octree.FindFirstElementWithBoundsTest(boxQ, [&intersect](const AxisAlignedBox&)
						{
							intersect = true;
							return false;
						});
					assert(!intersect && "Test3: False positive detected");
				}

				// Test 4: Giant element coverage (should hit multiple)
				{
					AxisAlignedBox boxQ = { { -1024.0f, -1024.0f, -1024.0f }, { 1024.0f, 1024.0f, 1024.0f } };
					int hitCount = 0;
					octree.FindFirstElementWithBoundsTest(boxQ, [&hitCount](const AxisAlignedBox&)
						{
							hitCount++;
							return true; // Continue searching
						});
					assert(hitCount >= 3 && "Test4: Under-counted overlaps");
				}

				// Test 5: Boundary precision (should hit)
				{
					AxisAlignedBox boxQ = { { -64.0f, -64.0f, -64.0f }, { 64.0f, 64.0f, 64.0f } };
					bool intersect = false;
					octree.FindFirstElementWithBoundsTest(boxQ, [&intersect](const AxisAlignedBox&)
						{
							intersect = true;
							return false;
						});
					assert(intersect && "Test5: Boundary precision failure");
				}
			}

		}
	}

}

#endif // ECHO_EDITOR

