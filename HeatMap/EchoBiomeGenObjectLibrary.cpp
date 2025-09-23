#include "EchoStableHeaders.h"
#include "EchoBiomeGenObjectLibrary.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <functional>
#include <tuple>
#include <random>
#include "EchoSingleton.h"
#include "EchoLogManager.h"
#include "EchoName.h"
#include "EchoVector3.h"

namespace Echo
{

	namespace AutoConsoleCommand {

		template <typename T>
		T fromString(const std::string& str) {
			std::istringstream iss(str);
			T value;
			if (!(iss >> value)) {
				LogManager::instance()->logMessage(std::string("--error-- Template Class [") + typeid(T).name() + "]::fromString.");
				value = {};
			}
			return value;
		}

		template <>
		std::string fromString<std::string>(const std::string& str) { return str; }

		template <>
		Echo::Name fromString<Echo::Name>(const std::string& str) { return Echo::Name(str); }

		template <>
		Echo::Vector3 fromString<Echo::Vector3>(const std::string& str) {
			std::istringstream iss(str);
			Vector3 value;
			char sep0, sep1, sep2, sep3;
			if (!(iss >> sep0 >> value.x >> sep1 >> value.y >> sep2 >> value.z >> sep3) || sep0 != '(' || sep1 != ',' || sep2 != ',' || sep3 != ')') {
				LogManager::instance()->logMessage("--error-- Template Class [Vector3]::fromString.");
				value = {};
			}
			return value;
		}

		template <typename T>
		struct function_traits;

		template <typename Func>
		auto GenerateConsoleCommand(Func&& func) {
			using Traits = function_traits<std::decay_t<Func>>;

			return [func = std::forward<Func>(func)](const std::vector<std::string>& strArgs) {
				if (strArgs.size() != Traits::arity) {
					LogManager::instance()->logMessage("--error-- Console Command Argument count mismatch");
					return;
				}

				return[&]<std::size_t... I>(std::index_sequence<I...>) {
					return func(fromString<typename Traits::template arg<I>::type>(strArgs[I])...);
				}(std::make_index_sequence<Traits::arity>{});
				};
		}

		// 普通函数
		template <typename R, typename... Args>
		struct function_traits<R(Args...)> {
			static constexpr std::size_t arity = sizeof...(Args);
			using result_type = R;

			template <std::size_t N>
			struct arg {
				using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
			};
		};

		// 函数指针
		template <typename R, typename... Args>
		struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

		// 成员函数指针
		template <typename C, typename R, typename... Args>
		struct function_traits<R(C::*)(Args...)> : public function_traits<R(Args...)> {};

		// const成员函数指针
		template <typename C, typename R, typename... Args>
		struct function_traits<R(C::*)(Args...) const> : public function_traits<R(Args...)> {};

		// 函数对象/lambda
		template <typename F>
		struct function_traits : public function_traits<decltype(&F::operator())> {};

	};

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshLib& value) {

		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "data_type", value.data_type);
		WriteJSNode(data_node, "physics", value.physics);
		WriteJSNode(data_node, "cast_shadow", value.cast_shadow);
		WriteJSNode(data_node, "mesh", value.mesh);
		WriteJSNode(data_node, "surface_type", value.surface_type);
		WriteJSNode(data_node, "custom_type", value.custom_type);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshLib& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "data_type", value.data_type);
		ReadJSNode(data_node, "physics", value.physics);
		ReadJSNode(data_node, "cast_shadow", value.cast_shadow);
		ReadJSNode(data_node, "mesh", value.mesh);
		ReadJSNode(data_node, "surface_type", value.surface_type);
		ReadJSNode(data_node, "custom_type", value.custom_type);
		return true;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshInfo& value) {

		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "label", value.label);
		WriteJSNode(data_node, "scale", value.scale);
		WriteJSNode(data_node, "up_offset", value.up_offset);
		WriteJSNode(data_node, "weight", value.weight);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshInfo& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "label", value.label);
		ReadJSNode(data_node, "scale", value.scale);
		ReadJSNode(data_node, "up_offset", value.up_offset);
		ReadJSNode(data_node, "weight", value.weight);
		return true;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::StaticMeshPCGParam& value) {
		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "density", value.density);
		WriteJSNode(data_node, "apply_slope", value.apply_slope);
	}
	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::StaticMeshPCGParam& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "density", value.density);
		ReadJSNode(data_node, "apply_slope", value.apply_slope);
		return true;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::BiomeGenObjectInfo& value) {

		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "high", value.level[0]);
		WriteJSNode(data_node, "middle", value.level[1]);
		WriteJSNode(data_node, "low", value.level[2]);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::BiomeGenObjectInfo& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "high", value.level[0]);
		ReadJSNode(data_node, "middle", value.level[1]);
		ReadJSNode(data_node, "low", value.level[2]);
		return true;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::BiomeGenObjectLayerInfo& value) {

		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "rule", value.rule);
		WriteJSNode(data_node, "meshs", value.meshs);
		WriteJSNode(data_node, "params", value.params);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::BiomeGenObjectLayerInfo& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "rule", value.rule);
		ReadJSNode(data_node, "meshs", value.meshs);
		ReadJSNode(data_node, "params", value.params);
		return true;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const PCG::GenObjectRule& value) {

		cJSON* data_node = cJSON_CreateObject();
		if (data_node == nullptr) return;
		cJSON_AddItemToObject(pRoot, name, data_node);
		WriteJSNode(data_node, "pcg_file", value.pcg_file);
		WriteJSNode(data_node, "labels", value.labels);
		WriteJSNode(data_node, "level", value.level);
		WriteJSNode(data_node, "params", value.params);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, PCG::GenObjectRule& value) {
		JSNODE data_node = cJSON_GetObjectItem(pRoot, name);
		if (data_node == nullptr) return false;
		ReadJSNode(data_node, "pcg_file", value.pcg_file);
		ReadJSNode(data_node, "labels", value.labels);
		ReadJSNode(data_node, "level", value.level);
		ReadJSNode(data_node, "params", value.params);
		if (value.labels.size() != value.params.size()) {
			value.params.resize(value.labels.size());
		}
		return true;
	}

	namespace PCG
	{
		static Name gDefaultLibrary = Name("echo/biome_terrain/static_object/default.library");
		static Name gDefaultRule = Name("echo/biome_terrain/static_object/default.rule");

		BiomeGenObjectLibraryManager::BiomeGenObjectLibraryManager() { Load(); }
		BiomeGenObjectLibraryManager::~BiomeGenObjectLibraryManager() { Clear(); }

		void BiomeGenObjectLibraryManager::Load() {
			ImportRule();
			ImportLibrary();
		}

		void BiomeGenObjectLibraryManager::Clear() {
			mLibraryMap.clear();
		}
		bool BiomeGenObjectLibraryManager::Generate(const BiomeGenObjectLayerInfo& info, BiomeGenObjectLayer& data) const {
			Generate(info.rule, data.path);
			Generate(info.meshs, data.meshs);
			data.params = info.params;
			return !data.path.empty();
		}
		bool BiomeGenObjectLibraryManager::Generate(const BiomeGenObjectInfo& info, BiomeGenObject& data) const {
			bool bSuccess = false;
			for (int i = 0; i != 3; ++i) {
				bSuccess = Generate(info.level[i], data.level[i]) || bSuccess;
			}
			return bSuccess;
		}

		void BiomeGenObjectLibraryManager::Generate(const Name& label, Name& path) const {
			auto iter = mRuleMap.find(label);
			if (iter == mRuleMap.end()) {
				path = Name::g_cEmptyName;
			}
			else {
				path = iter->second.pcg_file;
			}
		}

		void BiomeGenObjectLibraryManager::ImportRule() {
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(gDefaultRule.c_str(), true));
			if (pDataStream.isNull())
			{
				return;
			}

			size_t nSize = pDataStream->size();
			char* pData = new char[nSize + 1];
			memset(pData, 0, nSize + 1);
			pDataStream->read(pData, nSize);
			pDataStream.setNull();
			JSNODE hRoot = cJSON_Parse(pData);
			if (0 != hRoot)
			{
				ReadJSNode(hRoot, "data", mRuleMap);
			}
			cJSON_Delete(hRoot);
			delete[] pData;
		}

		void BiomeGenObjectLibraryManager::ExportRule() {

			DataStreamPtr pDataStream(Root::instance()->CreateDataStream(gDefaultRule.c_str()));
			if (pDataStream.isNull())
			{
				return;
			}
			JSNODE hRoot = cJSON_CreateObject();
			if (0 != hRoot)
			{
				WriteJSNode(hRoot, "data", mRuleMap);

				char* out = cJSON_Print(hRoot);
				cJSON_Delete(hRoot);

				if (0 == out)
					return;

				pDataStream->writeData(out, strlen(out));
				pDataStream->close();
				free(out);
			}
		}

		void BiomeGenObjectLibraryManager::ImportLibrary() {
			DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(gDefaultLibrary.c_str(), true));
			if (pDataStream.isNull())
			{
				return;
			}

			size_t nSize = pDataStream->size();
			char* pData = new char[nSize + 1];
			memset(pData, 0, nSize + 1);
			pDataStream->read(pData, nSize);
			pDataStream.setNull();
			JSNODE hRoot = cJSON_Parse(pData);
			if (0 != hRoot)
			{
				StaticMeshLibMap& mMap = mLibraryMap;
				mMap.clear();
				ReadJSNode(hRoot, "data", mMap);
			}
			cJSON_Delete(hRoot);
			delete[] pData;
		}
		void BiomeGenObjectLibraryManager::ExportLibrary() {

			const StaticMeshLibMap& mMap = mLibraryMap;

			DataStreamPtr pDataStream(Root::instance()->CreateDataStream(gDefaultLibrary.c_str()));
			if (pDataStream.isNull())
			{
				return;
			}
			JSNODE hRoot = cJSON_CreateObject();
			if (0 != hRoot)
			{
				WriteJSNode(hRoot, "data", mMap);

				char* out = cJSON_Print(hRoot);
				cJSON_Delete(hRoot);

				if (0 == out)
					return;

				pDataStream->writeData(out, strlen(out));
				pDataStream->close();
				free(out);
			}
		}

		void BiomeGenObjectLibraryManager::Generate(const Name& ruleName, StaticMeshPCGParamMap& meshMap) const {
			auto iter = mRuleMap.find(ruleName);
			if (iter == mRuleMap.end()) return;
			int size = (int)iter->second.labels.size();
			for (int i = 0; i < size; ++i) {
				meshMap[iter->second.labels[i]] = iter->second.params[i];
			}
		}
		void BiomeGenObjectLibraryManager::Generate(const StaticMeshInfoMap& _meshs, StaticMeshMap& _datas) const {
			_datas.clear();
			for (const auto& iter : _meshs) {
				uint32 arraySize = (uint32)iter.second.size();
				auto& datas = _datas[iter.first];
				datas.meshs.resize(arraySize);
				for (uint32 i = 0; i < arraySize; ++i) {
					const auto& mesh = iter.second[i];
					auto iter2 = mLibraryMap.find(mesh.label);
					if (iter2 != mLibraryMap.end()) {
						auto& data = datas.meshs[i];
						data.data_type = iter2->second.data_type;
						data.physics = iter2->second.physics;
						data.cast_shadow = iter2->second.cast_shadow;
						data.surface_type = iter2->second.surface_type;
						data.custom_type = iter2->second.custom_type;
						data.mesh = iter2->second.mesh;
						data.scale = mesh.scale;
						data.up_offset = mesh.up_offset;
						data.show = mesh.show;
						for (int j = 0; j < mesh.weight; ++j) {
							datas.percentage.push_back(i);
						}
					}
#ifdef ECHO_EDITOR
					else {
						auto& data = datas.meshs[i];
						data.data_type = 0;
						data.mesh = Name::g_cEmptyName;
						data.scale = mesh.scale;
						data.up_offset = mesh.up_offset;
						data.show = mesh.show;
						for (int j = 0; j < mesh.weight; ++j) {
							datas.percentage.push_back(i);
						}
					}
#endif // ECHO_EDITOR
				}
				std::shuffle(datas.percentage.begin(), datas.percentage.end(), std::default_random_engine());
			}
		}


		void BiomeGenObjectLibraryManager::CollectResources(const BiomeGenObjectInfo& info, set<String>::type& _resSet) const {
			for (int i = 0; i != 3; ++i) {
				CollectResources(info.level[i], _resSet);
			}
		}
		void BiomeGenObjectLibraryManager::CollectResources(const BiomeGenObjectLayerInfo& info, set<String>::type& _resSet) const {
			BiomeGenObjectLayer data;
			Generate(info, data);
			if (!data.path.empty()) {
				_resSet.insert(data.path.toString());
			}
			for (auto& mesh : data.meshs) {
				for (auto& mm : mesh.second.meshs) {
					if (!mm.mesh.empty()) {
						_resSet.insert(mm.mesh.toString());
					}
				}
			}
		}
	}
}
namespace Echo {

	namespace PCG {

		union PID {
			PID(uint64 _id) : id(_id) {}
			struct {
				union {
					struct {
						uint16 cid_0;
						uint16 cid_1;
					};
					uint32 cid;
				};
				uint32 pid;
			};
			uint64 id = 0;
		};
		void GenObjectScheduler::addDeletedObject(uint64 id) {
			PID pcgid(id);
			auto& idSet = mDeletedObjects[pcgid.pid];
			idSet[pcgid.cid_0].insert(pcgid.cid_1);
			mStateChangeObjects[pcgid.pid][pcgid.cid_0].insert(pcgid.cid_1);
		}
		void GenObjectScheduler::removeDeletedObject(uint64 id) {
			PID pcgid(id);
			auto& idSet = mDeletedObjects[pcgid.pid];
			idSet[pcgid.cid_0].erase(pcgid.cid_1);
			mStateChangeObjects[pcgid.pid][pcgid.cid_0].insert(pcgid.cid_1);
		}
		void GenObjectScheduler::clearDeletedObject() {
			if (mStateChangeObjects.empty()) {
				mStateChangeObjects.swap(mDeletedObjects);
			}
			else {
				mStateChangeObjects.insert(mDeletedObjects.begin(), mDeletedObjects.end());
				mDeletedObjects.clear();
			}
		}
		void GenObjectScheduler::getAllDeletedObject(std::set<uint64>& ids) {
			PID pcgid(0);
			for (auto& iter1 : mDeletedObjects) {
				pcgid.pid = iter1.first;
				for (auto& iter2 : iter1.second) {
					pcgid.cid_0 = iter2.first;
					for (auto& iter3 : iter2.second) {
						pcgid.cid_1 = iter3;
						ids.insert(pcgid.id);
					}
				}
			}
		}

		void GenObjectScheduler::SetMinRenderDistance(float value) {
			value = std::max(value, 256.0f);
			bool bDirty = (gMinRenderDistance != value);
			gMinRenderDistance = value;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetLevelStepMul(float value) {
			value = Math::Clamp(value, 1.0f, 10.0f);
			bool bDirty = (gLevelStepMul != value);
			gLevelStepMul = value;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::ResetAllScheduler() {
			GenObjectSchedulerManager::instance()->ResetAllScheduler();
		}
		void GenObjectScheduler::ClearAllDeletedObject() {
			GenObjectSchedulerManager::instance()->ClearAllDeletedObject();
		}
		void GenObjectScheduler::SetLoadMul(float v) {
			v = Math::Clamp(v, 1.0f, 2.0f);
			bool bDirty = (gLoadMul != v);
			gLoadMul = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetUnloadMul(float v) {
			v = Math::Clamp(v, 1.0f, 2.0f);
			bool bDirty = (gUnloadMul != v);
			gUnloadMul = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetLoadAdd(float v) {
			v = Math::Clamp(v, 0.0f, 1000.0f);
			bool bDirty = (gLoadAdd != v);
			gLoadAdd = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetUnloadAdd(float v) {
			v = Math::Clamp(v, 0.0f, 1000.0f);
			bool bDirty = (gUnloadAdd != v);
			gUnloadAdd = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetCullFactorMul(float v) {
			v = Math::Clamp(v, 0.001f, 10.0f);
			bool bDirty = (gCullFactorMul != v);
			gCullFactorMul = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetMinCullDistance(float v) {
			v = Math::Clamp(v, 10.0f, 1000.0f);
			bool bDirty = (gMinCullDistance != v);
			gMinCullDistance = v;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::SetIsUse(bool value) {
			bool bDirty = (gIsUse != value);
			gIsUse = value;
			if (bDirty) ResetAllScheduler();
		}
		void GenObjectScheduler::ShowLog(uint32 type) {
			GenObjectSchedulerManager::instance()->ShowLog(type);
		}
		void GenObjectScheduler::ImportConfig(const multimap<String, String>::type* pSetMap) {
			multimap<String, String>::type::const_iterator it = pSetMap->find("UsePCG");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetIsUse((StringConverter::parseUnsignedInt(it->second) != 0));
			}
			else
			{
				PCG::GenObjectScheduler::SetIsUse(true);
			}
			it = pSetMap->find("PcgMinRenderDistance");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetMinRenderDistance(StringConverter::parseFloat(it->second, 600.0f));
			}
			else
			{
				PCG::GenObjectScheduler::SetMinRenderDistance(600.0f);
			}
			it = pSetMap->find("PcgLevelStepMul");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetLevelStepMul(StringConverter::parseFloat(it->second, 1.5f));
			}
			else
			{
				PCG::GenObjectScheduler::SetLevelStepMul(1.5f);
			}
			it = pSetMap->find("PcgLoadMul");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetLoadMul(StringConverter::parseFloat(it->second, 1.06f));
			}
			else
			{
				PCG::GenObjectScheduler::SetLoadMul(1.06f);
			}
			it = pSetMap->find("PcgUnloadMul");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetUnloadMul(StringConverter::parseFloat(it->second, 1.16f));
			}
			else
			{
				PCG::GenObjectScheduler::SetUnloadMul(1.16f);
			}
			it = pSetMap->find("PcgLoadAdd");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetLoadAdd(StringConverter::parseFloat(it->second, 100.0f));
			}
			else
			{
				PCG::GenObjectScheduler::SetLoadAdd(100.0f);
			}
			it = pSetMap->find("PcgUnloadAdd");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetUnloadAdd(StringConverter::parseFloat(it->second, 200.0f));
			}
			else
			{
				PCG::GenObjectScheduler::SetUnloadAdd(200.0f);
			}
			it = pSetMap->find("PcgCullFactorMul");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetCullFactorMul(StringConverter::parseFloat(it->second, 3.0f));
			}
			else
			{
				PCG::GenObjectScheduler::SetCullFactorMul(3.0f);
			}
			it = pSetMap->find("PcgMinCullDistance");
			if (it != pSetMap->end())
			{
				PCG::GenObjectScheduler::SetMinCullDistance(StringConverter::parseFloat(it->second, 120.0f));
			}
			else
			{
				PCG::GenObjectScheduler::SetMinCullDistance(120.0f);
			}
			it = pSetMap->find("PcgEngineMode");
			if (it != pSetMap->end())
			{
				gPcgEngineMode = StringConverter::parseUnsignedInt(it->second, ePcgEngineMode_Default);
			}
			else
			{
				gPcgEngineMode = ePcgEngineMode_Default;
			}
		}
		void GenObjectScheduler::ExecuteCommand(const vector<String>::type& vec) {
			if (vec.size() < 1) return;
			auto commandIter = gConsoleCommandFunMap.find(vec[0]);
			if (commandIter == gConsoleCommandFunMap.end()) return;
			std::vector<std::string> args(vec.begin() + 1, vec.end());
			commandIter->second(args);
		}

		std::unordered_map<String, std::function<void(const vector<String>::type&)>> GenObjectScheduler::gConsoleCommandFunMap = {
			{"rd",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetMinRenderDistance)},
			{"lsm",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetLevelStepMul)},
			{"reset",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::ResetAllScheduler)},
			{"use",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetIsUse)},
			{"lm",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetLoadMul)},
			{"ulm",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetUnloadMul)},
			{"la",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetLoadAdd)},
			{"ula",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetUnloadAdd)},
			{"cfm",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetCullFactorMul)},
			{"mcd",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::SetMinCullDistance)},
			{"log",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::ShowLog)},
			{"cldo",AutoConsoleCommand::GenerateConsoleCommand(GenObjectScheduler::ClearAllDeletedObject)}
		};

		GenObjectScheduler::GenObjectScheduler() {
			GenObjectSchedulerManager::instance()->AddScheduler(this);
		}
		GenObjectScheduler::~GenObjectScheduler() {
			GenObjectSchedulerManager::instance()->RemoveScheduler(this);
		}
		GenObjectSchedulerManager::GenObjectSchedulerManager() {
			Root::instance()->addFrameListener(this);
		}
		GenObjectSchedulerManager::~GenObjectSchedulerManager() {
			Root::instance()->removeFrameListener(this);
		}
		bool GenObjectSchedulerManager::frameStarted(const FrameEvent& evt) {
			for (GenObjectScheduler* pScheduler : mClassSet) {
				pScheduler->updateDeletedObjects();
			}
			return true;
		}
		void GenObjectSchedulerManager::AddScheduler(GenObjectScheduler* pScheduler) {
			mClassSet.insert(pScheduler);
		}
		void GenObjectSchedulerManager::RemoveScheduler(GenObjectScheduler* pScheduler) {
			mClassSet.erase(pScheduler);
		}
		void GenObjectSchedulerManager::ResetAllScheduler() {
			for (GenObjectScheduler* pScheduler : mClassSet) {
				pScheduler->resetGenObjects();
			}
		}
		void GenObjectSchedulerManager::ClearAllDeletedObject() {
			for (GenObjectScheduler* pScheduler : mClassSet) {
				pScheduler->clearDeletedObject();
			}
		}
		void GenObjectSchedulerManager::ShowLog(uint32 type) {
			for (GenObjectScheduler* pScheduler : mClassSet) {
				std::map<Name, uint32> data;
				pScheduler->collectGenObjects(data, type);
				LogManager::instance()->logMessage("-------------------------------------------");
				LogManager::instance()->logMessage(pScheduler->getName().c_str());
				uint32 sum = 0;
				for (auto& iter : data) {
					LogManager::instance()->logMessage("[" + iter.first.toString() + "]" + std::to_string(iter.second));
					sum += iter.second;
				}
				LogManager::instance()->logMessage("Sum:" + std::to_string(sum));
				LogManager::instance()->logMessage("-------------------------------------------");
			}
		}
	}

}
