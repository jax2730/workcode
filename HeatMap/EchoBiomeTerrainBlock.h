//-----------------------------------------------------------------------------
// File:   EchoBiomeTerrainBlock.h
//
// Author: yanghang
//
// Date:  2024-03-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#if !defined(ECHO_BIOME_TERRAIN_BLOCK_H)
#include "EchoPrerequisites.h"

namespace Echo
{
	class UberNoiseFBM2D;

    struct TerrainVertex
    {
        Vector3 pos;
        Vector3 normal;
        Vector2 uv;
    };

    struct HeightMap
    {
        float* pixels = 0;
        int m_x = 0,m_y = 0;
        float min = 0.f,max = 0.f;
        
        float getHeightmapValue(Vector2 uv);
		float getHeightmapValue(int x, int y)
		{
			x = Clamp(0, m_x - 1, x);
			y = Clamp(0, m_y - 1, y);
			return pixels[x + y * m_x] / max;
		}
        void freeMemory();
        int size()const;
        
		void copyTo(HeightMap& other) const;
        static HeightMap createHeightMap(const int x, const int y);
    };

    //IMPORTANT(yanghang):If changed, YOU must change code in the PlanetTerrain_PS.txt
    enum class BiomeDisplayMode
    {
        Full = 0,//show all height distail.
        Height,//Show height mesh without nothing.
        Temperature,//Show temeprature as color in real height map.
        Humidity,//Show humidity as color in real height map.
        MaterialIndex,//Show Material Index color
        Normal,
        PBRParam,
        LOD,
		GeometryNormal,
		RegionIndex,
		Tile,
		MaterialLevel,			//Show current shading strategy
		TriplanarBlendWeight,
		NoisedUV,
		RegionIndexOffline,
		RegionIndexProcess,
		PlanetRegionIndex,
		ManualPlanetRegion,
		LerpedUV,
		TextureTillingNoise,
		RegionSDF,
		GeometryHeatmap
    };

    
    class _EchoExport BiomeTerrainBlock :public Renderable
    {
    public:
        
        BiomeTerrainBlock();
        ~BiomeTerrainBlock();
        
        // NOTE(yanghang): Func from Renderable
        virtual void getWorldTransforms(const DBMatrix4** xform) const override {*xform = &m_worldMat;}
        virtual void getWorldTransforms(DBMatrix4* xform) const override {*xform = m_worldMat;}
        virtual RenderOperation* getRenderOperation() override {return &m_renderOperation;}
        virtual const Material* getMaterial(void) const override {return m_material.get();}
        virtual const LightList& getLights(void) const override {return m_lightList;}
        virtual uint32 getInstanceNum() override {return 1;}
        virtual void setCustomParameters(const Pass* pPass)override;
    
        void init();

        void generateMesh(HeightMap& heightmap);

        void setWorldPos(const Vector3& pos);
		void setWorldRot(const Quaternion& rot);
		void setWorldTrans(const Transform& trans);
        void setScale(const float& scale);
        
        // NOTE: GPU buffer.
        RcBuffer* m_vertexBuffer;
        RcBuffer* m_indexBuffer;
        
        MaterialPtr m_material;
        RenderOperation m_renderOperation;
                
        // TODO(yanghang): Seemed that no need light source.
        LightList	m_lightList;
    
        std::vector<TerrainVertex> m_vertices;
        std::vector<uint32> m_indices;

        DBMatrix4 m_worldMat;

        BiomeDisplayMode m_displayMode;

		//单套材质相关系数
		float m_tillingParam = 20.f;
		float m_normalStrenth = 2.f;

		//三平面投影计算权重需要的系数 每个方向的权重为 pow(abs(Normal) - delta, m)
		float m_weightParamM = 3.f;
		float m_weightParamDelta = 0.5f;

		float isReady = false;

        float m_scale;

        float m_LODLevel = 0;
#ifdef ECHO_EDITOR
		bool m_bDisplay{ false };
		Matrix4 m_StampTerrainMtx{ Matrix4::IDENTITY };
#endif
    };

}
#define ECHO_BIOME_TERRAIN_BLOCK_H
#endif
