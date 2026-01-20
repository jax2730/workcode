///////////////////////////////////////////////////////////////////////////////
//
// @file EchoBiomeVegetation.h
// @author CHENSHENG
// @date 2024/06/11 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __EchoBiomeVegetation_H__
#define __EchoBiomeVegetation_H__

#include "EchoPrerequisites.h"
#include "EchoColorValue.h"
#include "EchoVector3.h"
#include "EchoName.h"
#include "EchoBiomeVegBase.h"

namespace Echo
{
	struct VegRoadQueryPackage
	{
		using Element = std::pair<Vector3, bool>;
		using ElementArray = std::vector<Element>;

		class SphericalTerrain* sphPtr = nullptr;
		ElementArray elements;
	};
	struct _RegisterData
	{
		SceneManager* scenePtr = nullptr;
		SphericalTerrain* sphPtr = nullptr;
	};

	namespace SphVeg { struct _Private; }
	class _EchoExport BiomeVegetation : public BiomeVegBase
	{
	public:
		friend class BiomeVegGroup;

		struct CreateInfo
		{
			DVector3 sphPos = DVector3::ZERO;
			Quaternion sphRot = Quaternion::IDENTITY;
			float	sphRadius = 2000.f;
			class SphericalTerrain* sphPtr = nullptr;
			uint16 sphTerMaxOff = 2048u;
			float boxCacheHoldTime = 2.f;
			float infoCacheHoldTime = 4.f;
			float boxVisDistance = 128.f;

			Name			biomeName;
			VegLayerInfoMap vegLayerMap;

			BiomeVegGroup* groupPtr = nullptr;
			class SceneManager* sceneMgrPtr = nullptr;
		};

	public:
		const Name& getName()const;

		void findVisibleObjects(const Camera* pCamera, RenderQueue* pQueue);
		void recheckPosition(const AxisAlignedBox& box);

		void updateSphereDeleteRegion(const Camera* pCamera, const std::vector<Vector4>& sphereDeleteRegion);
		void onChangeTerrainPos(const Vector3& pos);
		void onChangeTerrainRot(const Quaternion& rot);
		void onChangeTerrainRadius(float radius);
		void onChangeTerrainMaxOff(float maxOffset);

		virtual void onReDevide()																override;
		virtual void onDenLimitChange()															override;
		virtual void onDisLevelChange()															override;

		virtual void clearAll()																	override;
		virtual void clearRenderCache()															override;

		virtual void updateLayerParam(int terIndex, const VegLayerInfo& vegLayerInfo)			override;
		virtual void updateAllVegRes(int terIndex, const VegLayerInfoArray& vegLayerInfoArray)	override;
		virtual void removeAllVegRes(int terIndex)												override;
		virtual void collectRuntimeData()												  const override;

		// Only test
		static void ShowBox(uint32 args);
		static void ShowRoad(uint32 args);
		static void ClearRoad(uint32 args);
		static void RecordRoad(uint32 args);
		static std::set<const void*> GetBiomeVegIns(uint32 terrIdx, const std::string& name);
		const void* getBiomeVegIns(uint32 terrIdx, const std::string& name)const;

	private:
		BiomeVegetation(const CreateInfo& createInfo, BiomeVegGroup*);
		virtual ~BiomeVegetation()override;
	private:
		SphVeg::_Private* _dp = nullptr;
		std::vector<Vector4> visibleSphereRegions;
	public:
		inline static const uint32 _MAX_VEG_LAYER_NUM_ = 3u;
	};

#ifdef ECHO_EDITOR
	class _EchoExport BiomeVegetationSparkler {

	public:
		static bool ShouldRender(const void* ptr);
		static void Mark(const void* ptr);
	private:
		inline static std::map<const void*, uint32> s_sparkleMap;
		inline static std::unordered_map<std::string, uint32> s_sparkleIndicator;
		inline static uint64 sMarkerCounter = 0;
	};
#endif
}
#endif // !__EchoBiomeVegetation_H__
