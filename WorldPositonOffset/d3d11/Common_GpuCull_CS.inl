#define MAX_PIPELINE 256u
#define MAX_SHADOW_PIPELINE 4u
//1M space
#define CULLED_INSTANCE_COUNT 1024u //262144u
#define INSTANCE_DATA_SIZE 132u

#define VERTEX_COUNT_PER_CLUSTER 64u
#define TRIANGLE_COUNT_PER_CLUSTER 64u
//128*3
#define INDEX_COUNT_PER_CLUSTER 192u


//store
#define RawBufferStoreUint(buff, offset, value) buff.Store((offset) * 4, value)
#define RawBufferStoreUint2(buff, offset, value1, value2) buff.Store2((offset) * 8, uint2(value1, value2))

//atomic
#define RawInterlockedAdd(buff, offset, value, result) buff.InterlockedAdd((offset * 4), value, result)
#define RawInterlockedAdd_NoRet(buff, offset, value) buff.InterlockedAdd((offset * 4), value)

//load
#define RawBufferLoadUint(buff, offset) buff.Load((offset) * 4)
#define RawBufferLoadUint2(buff, offset) buff.Load2((offset) * 8)
//#define StructuredLoadUint(buff, offset, Dest, prop) Dest.##prop = buff.Load(offset)
//#define StructuredLoadFloat(buff, offset, Dest, prop) Dest.##prop = asfloat(buff.Load(offset))
//#define StructuredLoadInt(buff, offset, Dest, prop) Dest.##prop = asint(buff.Load(offset))


struct InstanceData
{
	float Transform[12];
	float InvTransposeTransform[12];
	uint Flip;
	uint Pipeline;
	uint Begin;
	uint Lens;
	uint BoundBox[3];
	uint MeshID;
	uint RenderGroup;
};

struct InstChunkID
{
	uint InstanceID;
	uint ChunkID;
};

//store InstChunkID
#define InstChunkIDStore RawBufferStoreUint2
//load InstChunkID
#define InstChunkIDLoad (InstChunkID)RawBufferLoadUint2

#define MESH_DATA_SIZE 68u
struct MeshData
{
	uint ChunkID[16];
	uint ChunkCount;
};

#define DISPATCH_OFFSET 8u

#define CHUNK_DATA_SIZE 260u
struct ChunkData
{
	uint ClusterID[64];
	uint ClusterCount;
};

#define CLUSTER_DATA_SIZE 80u
struct ClusterData
{
	//uint SphereData[2];//center + radius
	uint BoundBox[3];
	uint ConeData[4];// last 2 byte is IndexCount
	uint VertexOffset;
	//uint IndexCount;
	uint CubeMap[TRIANGLE_COUNT_PER_CLUSTER*6/32];
};

struct InstClusterID
{
	uint InstanceID;
	uint ClusterID;
};

//store InstClusterID
#define InstClusterIDStore RawBufferStoreUint2
#define InstClusterIDStore_ClusterID(buff, index, value) RawBufferStoreUint(buff, (index) * 2 + 1, value)
//load InstClusterID
#define InstClusterIDLoad (InstClusterID)RawBufferLoadUint2

struct ClusterCulledData
{
	uint ReadFrom;
	uint WriteTo;
	uint IndexCount;
	uint IndexOffset;
	uint InstanceID;
	uint Mask[2];
	uint Pipeline;
	uint IsFlip;
};

//store ClusterCulledData
#define ClusterCulledDataStore_ReadFrom(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 0) * 4, value)
#define ClusterCulledDataStore_WriteTo(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 1) * 4, value)
#define ClusterCulledDataStore_IndexCount(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 2) * 4, value)
#define ClusterCulledDataStore_IndexOffset(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 3) * 4, value)
#define ClusterCulledDataStore_InstanceID(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 4) * 4, value)
#define ClusterCulledDataStore_Mask1(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 5) * 4, value)
#define ClusterCulledDataStore_Mask2(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 6) * 4, value)
#define ClusterCulledDataStore_Pipeline(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 7) * 4, value)
#define ClusterCulledDataStore_IsFlip(buff, index, offset, value) buff.Store(((offset) + (index) * 9 + 8) * 4, value)
//load ClusterCulledData
#define ClusterCulledDataLoad_ReadFrom(buff, index, offset) buff.Load(((offset) + (index) * 9 + 0) * 4)
#define ClusterCulledDataLoad_WriteTo(buff, index, offset) buff.Load(((offset) + (index) * 9 + 1) * 4)
#define ClusterCulledDataLoad_IndexCount(buff, index, offset) buff.Load(((offset) + (index) * 9 + 2) * 4)
#define ClusterCulledDataLoad_IndexOffset(buff, index, offset) buff.Load(((offset) + (index) * 9 + 3) * 4)
#define ClusterCulledDataLoad_InstanceID(buff, index, offset) buff.Load(((offset) + (index) * 9 + 4) * 4)
#define ClusterCulledDataLoad_Mask1(buff, index, offset) buff.Load(((offset) + (index) * 9 + 5) * 4)
#define ClusterCulledDataLoad_Mask2(buff, index, offset) buff.Load(((offset) + (index) * 9 + 6) * 4)
#define ClusterCulledDataLoad_Pipeline(buff, index, offset) buff.Load(((offset) + (index) * 9 + 7) * 4)
#define ClusterCulledDataLoad_IsFlip(buff, index, offset) buff.Load(((offset) + (index) * 9 + 8) * 4)



struct VertexData
{
	float x;
	float y;
	float z;
};
#define VERTEX_COMBINE_DATA_SIZE 24u
struct VertexDataCombine
{
	uint pos0;
	uint pos1;
	uint uv;
	uint nt0;
	uint nt1;
	uint nt2;
};

float2 unpackHalf2x16(uint v)
{
    return f16tof32(uint2(v & 0xFFFFF, v >> 16));
}

//更新Buffer相关
#define VertexBlock VertexDataCombine
struct VertexBlockUpdate
{
	uint memIdx;
	VertexDataCombine data[VERTEX_COUNT_PER_CLUSTER];
};

struct VertexPositionBlock
{
	uint data[2];
};

struct VertexPositionBlockUpdate
{
	uint memIdx;
	VertexPositionBlock data[VERTEX_COUNT_PER_CLUSTER];
};

struct VertexUVBlock
{
	uint data;
};

struct VertexUVBlockUpdate
{
	uint memIdx;	
	VertexUVBlock data[VERTEX_COUNT_PER_CLUSTER];
};

struct VertexNormalTangentBlock
{
	uint data[3];
};

struct VertexNormalTangentBlockUpdate
{
	uint memIdx;
	VertexNormalTangentBlock data[VERTEX_COUNT_PER_CLUSTER];
};

#define IndexBlock uint
//struct IndexBlock
//{
//	uint data[3*TRIANGLE_COUNT_PER_CLUSTER];
//};

struct IndexBlockUpdate
{
	uint memIdx;
	IndexBlock data[3*TRIANGLE_COUNT_PER_CLUSTER];
};

#define ClusterBlock ClusterData
// struct ClusterBlock
// {
 	// uint data[8+TRIANGLE_COUNT_PER_CLUSTER*6/32];
// };

struct ClusterBlockUpdate
{
	uint memIdx;
	ClusterBlock data;
};

#define ChunkBlock ChunkData
//struct ChunkBlock
//{
//	uint data[65];
//};

struct ChunkBlockUpdate
{
	uint memIdx;
	ChunkBlock data;
};

#define MeshBlock MeshData
//struct MeshBlock
//{
//	uint data[17];
//};

struct MeshBlockUpdate
{
	uint memIdx;
	MeshBlock data;
};

#define InstanceBlock InstanceData
// struct InstanceBlock
// {
	// float transform[12];
	// float invTransposeTransform[12];	
	// uint flip;
	// uint pipeline;
	// uint begin;
	// uint lens;
	// uint data[5];
// };

struct InstanceBlockUpdate
{
	uint memIdx;
	InstanceBlock data;
};