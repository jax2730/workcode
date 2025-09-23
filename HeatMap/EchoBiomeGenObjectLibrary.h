//-----------------------------------------------------------------------------
// File:   EchoBiomeGenObjectLibrary.h
//
// Author: qiao_yuzhi
//
// Date:   2025-6-3
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//---------

#ifndef EchoBiomeGenObjectLibrary_h__
#define EchoBiomeGenObjectLibrary_h__

#include "EchoPrerequisites.h"
#include "EchoName.h"
#include "EchoVector3.h"
#include <unordered_map>
#include "EchoSingleton.h"
#include "EchocJsonHelper.h"
#include "EchoOrientedBox.h"
#include "EchoFrameListener.h"

namespace Echo
{
	struct GenObjectInfo {
		uint32 custom_type = 0;
		Vector3 scale = Vector3::UNIT_SCALE;
		Quaternion rotation = Quaternion::IDENTITY;
		DVector3 position = DVector3::ZERO;
		Name data_name;
	};

	namespace PCG
	{
		struct StaticMeshInfo {
			bool show = false;
			Name label;
			float scale = 1.0f;
			float up_offset = 0.0f;
			int weight = 1;
		};
		typedef std::vector<StaticMeshInfo> StaticMeshInfoArray;
		typedef std::map<Name, StaticMeshInfoArray> StaticMeshInfoMap;

		struct StaticMeshPCGParam {
			float density = 0.5f;
			bool apply_slope = false;
			bool isSame(const StaticMeshPCGParam& other) { return (std::abs(density - other.density) <= 0.15f) && (apply_slope == other.apply_slope); }
		};
		typedef std::map<Name, StaticMeshPCGParam> StaticMeshPCGParamMap;

		struct StaticMeshLib {
			uint8 data_type = 0;
			bool physics = true;
			bool cast_shadow = true;
			uint32 surface_type = 0;
			uint32 custom_type = 0;
			Name mesh;
		};
		typedef std::map<Name, StaticMeshLib> StaticMeshLibMap;

		struct StaticMesh {
			uint8 data_type = 0;
			bool show = false;
			bool physics = true;
			bool cast_shadow = true;
			float scale = 1.0f;
			uint32 surface_type = 0;
			uint32 custom_type = 0;
			float up_offset = 0.0f;
			Name mesh;
		};
		struct StaticMeshArray {
			std::vector<StaticMesh> meshs;
			std::vector<uint32> percentage;
			uint32 per_count = 0;
			void next(uint32 n = 1) { per_count = (per_count + n) % percentage.size(); }
			StaticMesh& current() { return meshs[percentage[per_count]]; }
		};
		typedef std::map<Name, StaticMeshArray> StaticMeshMap;

		struct BiomeGenObjectLayerInfo {
			Name rule;
			StaticMeshInfoMap meshs;
			StaticMeshPCGParamMap params;
		};

		struct BiomeGenObjectInfo {
			BiomeGenObjectLayerInfo level[3];
		};

		struct BiomeGenObjectLayer {
			Name path;
			StaticMeshMap meshs;
			StaticMeshPCGParamMap params;
		};

		struct BiomeGenObject {
			BiomeGenObjectLayer level[3];
		};
		struct PlanetGenObject {
			std::map<int, Name> path[3];
			std::map<int, StaticMeshMap> meshs[3];
			std::map<int, StaticMeshPCGParamMap> params[3];
		};

		struct GenObjectRule {
			Name pcg_file;
			uint32 level = 0;
			std::vector<Name> labels;
			std::vector<StaticMeshPCGParam> params;
		};
		typedef std::map<Name, GenObjectRule> GenObjectRuleMap;

		class _EchoExport BiomeGenObjectLibraryManager : public Singleton<BiomeGenObjectLibraryManager> {
		public:
			BiomeGenObjectLibraryManager();
			virtual ~BiomeGenObjectLibraryManager();

			void Generate(const StaticMeshInfoMap& meshs, StaticMeshMap& datas) const;
			void ImportLibrary();
			void ExportLibrary();
			void Generate(const Name& ruleName, StaticMeshPCGParamMap& meshs) const;
			void ImportRule();
			void ExportRule();
			void Generate(const Name& label, Name& path) const;
			bool Generate(const BiomeGenObjectInfo& info, BiomeGenObject& data) const;
			bool Generate(const BiomeGenObjectLayerInfo& info, BiomeGenObjectLayer& data) const;
			void CollectResources(const BiomeGenObjectInfo& info, set<String>::type& _resSet) const;
			void CollectResources(const BiomeGenObjectLayerInfo& info, set<String>::type& _resSet) const;
		private:
			void Load();
			void Clear();
		public:
			GenObjectRuleMap mRuleMap;
			StaticMeshLibMap mLibraryMap;
		};

		enum PcgEngineMode : uint32 {
			ePcgEngineMode_Resource_Mesh = (1 << 0),
			ePcgEngineMode_Resource_Actor = (1 << 1),
			ePcgEngineMode_Resource_DistantBuilding = (1 << 2),

			ePcgEngineMode_Terrain_Planet = (1 << 10),
			ePcgEngineMode_Terrain_Karst = (1 << 11),
			ePcgEngineMode_Terrain_Terrain = (1 << 12),

			ePcgEngineMode_Editor_Pcg = (1 << 16),
			ePcgEngineMode_Editor_Oae = (1 << 17),
			ePcgEngineMode_Editor_Scene = (1 << 18),

			ePcgEngineMode_Xxsj = ePcgEngineMode_Resource_Mesh | ePcgEngineMode_Resource_Actor | ePcgEngineMode_Resource_DistantBuilding | ePcgEngineMode_Terrain_Planet | ePcgEngineMode_Terrain_Karst,
			ePcgEngineMode_Su = ePcgEngineMode_Resource_Mesh | ePcgEngineMode_Resource_Actor | ePcgEngineMode_Resource_DistantBuilding | ePcgEngineMode_Terrain_Planet,
			ePcgEngineMode_Sh = ePcgEngineMode_Resource_Mesh | ePcgEngineMode_Resource_Actor | ePcgEngineMode_Resource_DistantBuilding | ePcgEngineMode_Terrain_Terrain,
			ePcgEngineMode_Default = ePcgEngineMode_Resource_Mesh | ePcgEngineMode_Resource_DistantBuilding | ePcgEngineMode_Terrain_Planet,
			ePcgEngineMode_All = 0xFFFFFFFF,
		};

		class _EchoExport GenObjectScheduler {
		public:
			GenObjectScheduler();
			virtual ~GenObjectScheduler();
			virtual const Name& getName() = 0;
			virtual void collectGenObjects(std::map<Name, uint32>& data, uint32 type) = 0;
			virtual void resetGenObjects() = 0;
			virtual void clearGenObjects() = 0;

			void addDeletedObject(uint64 id);
			void removeDeletedObject(uint64 id);
			void clearDeletedObject();
			void getAllDeletedObject(std::set<uint64>& ids);
			virtual void updateDeletedObjects() {}
		protected:
			std::map<uint32, std::map<uint16, std::set<uint16>>> mDeletedObjects;
			std::map<uint32, std::map<uint16, std::set<uint16>>> mStateChangeObjects;
		public:
			static uint32 GetPcgEngineMode() { return gPcgEngineMode; }
			static bool GetPcgEngineMode(PcgEngineMode _mode) { return (gPcgEngineMode & _mode) == _mode; }
			static void SetIsUse(bool value);
			static bool GetIsUse() { return gIsUse; }
			static void SetMinRenderDistance(float v);
			static float GetMinRenderDistance() { return gMinRenderDistance; }
			static void SetLevelStepMul(float v);
			static float GetLevelStepMul() { return gLevelStepMul; }
			static void SetLoadMul(float v);
			static float GetLoadMul() { return gLoadMul; }
			static void SetUnloadMul(float v);
			static float GetUnloadMul() { return gUnloadMul; }
			static void SetLoadAdd(float v);
			static float GetLoadAdd() { return gLoadAdd; }
			static void SetUnloadAdd(float v);
			static float GetUnloadAdd() { return gUnloadAdd; }
			static void SetCullFactorMul(float v);
			static float GetCullFactorMul() { return gCullFactorMul; }
			static void SetMinCullDistance(float v);
			static float GetMinCullDistance() { return gMinCullDistance; }
			static void ImportConfig(const multimap<String, String>::type* pSetMap);
			static void ExecuteCommand(const vector<String>::type& vec);
			static void ShowLog(uint32 type);
			static void ResetAllScheduler();
			static void ClearAllDeletedObject();
		private:
			static inline uint32 gPcgEngineMode = ePcgEngineMode_Default;
			static inline bool gIsUse = true;
			static inline float gMinRenderDistance = 1024.0f;
			static inline float gLevelStepMul = 1.5f;
			static inline float gLoadMul = 1.06f;
			static inline float gUnloadMul = 1.16f;
			static inline float gLoadAdd = 100.0f;
			static inline float gUnloadAdd = 200.0f;
			static inline float gCullFactorMul = 1.0f;
			static inline float gMinCullDistance = 80.0f;
			static std::unordered_map<String, std::function<void(const vector<String>::type& vec)>> gConsoleCommandFunMap;
		};

		class _EchoExport GenObjectSchedulerManager : public FrameListener, public Singleton<GenObjectSchedulerManager> {
		public:
			GenObjectSchedulerManager();
			virtual ~GenObjectSchedulerManager();
			virtual bool frameStarted(const FrameEvent& evt) override;

		private:
			void AddScheduler(GenObjectScheduler* pScheduler);
			void RemoveScheduler(GenObjectScheduler* pScheduler);
			void ShowLog(uint32 type);
			void ResetAllScheduler();
			void ClearAllDeletedObject();

			std::set<GenObjectScheduler*> mClassSet;
			friend class GenObjectScheduler;
		};

	}

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshLib& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshLib& value);

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshInfo& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshInfo& value);

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshPCGParam& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshPCGParam& value);

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::BiomeGenObjectLayerInfo& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::BiomeGenObjectLayerInfo& value);

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::BiomeGenObjectInfo& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::BiomeGenObjectInfo& value);

	_EchoExport void WriteJSNode(JSNODE pRoot, const char* name, const PCG::GenObjectRule& value);
	_EchoExport bool ReadJSNode(JSNODE pRoot, const char* name, PCG::GenObjectRule& value);
}

#endif //EchoBiomeGenObjectLibrary_h__
