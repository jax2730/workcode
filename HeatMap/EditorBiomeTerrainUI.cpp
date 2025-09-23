#include "EchoEditorStableHeaders.h"
#include "EditorUIManager.h"
#include <filesystem>
#include "EchoEditorRoot.h"
#include "EchoStringConverter.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoPlanetRegionResource.h"
#include "EditorBiomeTerrainUI.h"
#include "FileTools/EchoEditorFileLoadSaveWindow.h"
#include "FileTools/EchoEditorFileToolsManager.h"
#include "EchoDDSCodec.h"
#include "EchoImage.h"
#include <numeric>
#include "EditorBiomeDefineUI.h"
#include "DeformSnow.h"
#include "EchoVideoConfig.h"
#include "EditorBiomeMgr.h"
#include "EchoSphericalOcean.h"
#include "ActorEditorUI/EditorComponentCommonFunctionsUI.h"
#include "Actor\EchoActorSystem.h"
#include "Actor\EchoActor.h"

static int pathLength = 512;

template<typename T>
bool HoverAndWheelControl(T* value, T clampa, T clampb, T offset)
{
	bool changed = false;
	if (ImGui::IsItemHovered())
	{
		ImGui::SetItemUsingMouseWheel();
		T temp = *value;
		temp += (offset * ImGui::GetIO().MouseWheel);
		if (temp >= clampa && temp <= clampb)
		{
			if (std::abs(temp - *value) > 0.001)
				changed = true;
			*value = temp;
		}	
	}
	return changed;
}
template<>
bool HoverAndWheelControl(int* value, int clampa, int clampb, int offset)
{
	bool changed = false;
	if (ImGui::IsItemHovered())
	{
		int temp = *value;
		temp += int(offset * ImGui::GetIO().MouseWheel);
		if (temp >= clampa && temp <= clampb)
		{ 
			if (std::abs(temp - *value) >= 1)
				changed = true;
			*value = temp;
		}
			
	}
	return changed;
}
namespace StampHeightmapUI
{
	using Echo::D3D11TexInfo;
	using Echo::DDSCodec;
	struct Pool
	{
	public:
		// 注意返回的是引用
		static Pool& getInstance()
		{
			static Pool value;  //静态局部变量
			return value;
		}
		const D3D11TexInfo GetTexInfo(const std::string& path)
		{
			if (!map.contains(path))
			{
				DDSCodec ddsc;
				Echo::Image image;
				if(ddsc.setup(path.c_str()))
				{
					ddsc.decode(&image);

					map[path] = {  };
					Echo::EditorRoot::instance()->CreateD3D11Tex(image.getData(), image.getWidth(), image.getHeight(), map[path], DXGI_FORMAT_B8G8R8A8_UNORM);
					image.freeMemory();
					ddsc.cleanup();
				}
				else
				{
					map[path] = D3D11TexInfo{ nullptr, nullptr };
				}
			}
			return map[path];
		}
		void release()
		{
			for (auto&& [str, info] : map)
			{
				if (info.d3D11ResView != nullptr)
				{
					info.d3D11ResView->Release();
					info.d3D11ResView = nullptr;
				}
				if (info.d3D11Texture2D != nullptr)
				{
					info.d3D11Texture2D->Release();
					info.d3D11Texture2D = nullptr;
				}
			}
			std::unordered_map<std::string, D3D11TexInfo>().swap(map);
		}

	private:
		Pool() = default;
		Pool(const Pool& other) = delete; //禁止使用拷贝构造函数
		Pool& operator=(const Pool&) = delete; //禁止使用拷贝赋值运算符
	private:
		std::unordered_map<std::string, D3D11TexInfo> map;
	};

	void GetAllFormatFiles(const std::string & path, std::vector<std::string>&files, std::string&& format)
	{
		if (!std::filesystem::exists(path))
		{
			std::filesystem::create_directories(path);
		}
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory()) {
				GetAllFormatFiles(entry.path().string(), files, std::move(format));
			}
			else if (entry.is_regular_file()) {
				if (entry.path().extension() == format)
				{
					files.push_back(entry.path().string());
				}
			}
		}
	}

	LPITEMIDLIST GetPIDLFromAbsolutePath(LPCSTR pszPath)
	{
		OLECHAR wszPath[MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, pszPath, -1, wszPath, MAX_PATH);
		LPITEMIDLIST pidl = NULL;
		LPSHELLFOLDER pDesktopFolder = NULL;
		ULONG chEaten = 0;
		//DWORD dwAttributes;

		if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
		{
			if (SUCCEEDED(pDesktopFolder->ParseDisplayName(NULL, NULL, wszPath, &chEaten, &pidl, NULL)))
			{
				// Successfully got the PIDL from the absolute path
			}

			pDesktopFolder->Release();
		}

		return pidl;
	}
	bool SelectDirPath(std::string& outPath)
	{
		std::string path = Echo::Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(path.begin(), path.end(), '/', '\\');

		TCHAR szBuffer[MAX_PATH] = { 0 };
		BROWSEINFO bi;
		ZeroMemory(&bi, sizeof(BROWSEINFO));
		bi.hwndOwner = NULL;
		bi.pidlRoot = GetPIDLFromAbsolutePath(_T(path.c_str()));
		bi.pszDisplayName = szBuffer;
		bi.lpszTitle = _T("Select Stamp Template Library Root Directory :");
		bi.ulFlags = BIF_RETURNFSANCESTORS;
		// bi.lpfn = BrowseCallbackProc;
		// bi.lParam = (LPARAM)path.c_str();
		LPITEMIDLIST idl = SHBrowseForFolder(&bi);
		if (idl == NULL)
		{
			return false;
		}
		SHGetPathFromIDList(idl, szBuffer);
		std::string temp = szBuffer;
		if (temp.find("biome_terrain") == std::string::npos)
		{
			return false;
		}
		outPath = szBuffer;
		// memset(outPath, 0, kImagePathLen);
		// memcpy(outPath, szBuffer, strlen(szBuffer));
		return true;
	}
}
namespace
{
	struct [[maybe_unused]] NormalDistribution
	{
		float mean;
		float stdDev;
		// from 0.0 to 3.0 in 0.01 increments
		static constexpr float Z_STEP    = 0.01f;
		static constexpr float Z_TABLE[] =
		{
			0.5000f, 0.5040f, 0.5080f, 0.5120f, 0.5160f, 0.5199f, 0.5239f, 0.5279f, 0.5319f, 0.5359f,
			0.5398f, 0.5438f, 0.5478f, 0.5517f, 0.5557f, 0.5596f, 0.5636f, 0.5675f, 0.5714f, 0.5753f,
			0.5793f, 0.5832f, 0.5871f, 0.5910f, 0.5948f, 0.5987f, 0.6026f, 0.6064f, 0.6103f, 0.6141f,
			0.6179f, 0.6217f, 0.6255f, 0.6293f, 0.6331f, 0.6368f, 0.6406f, 0.6443f, 0.6480f, 0.6517f,
			0.6554f, 0.6591f, 0.6628f, 0.6664f, 0.6700f, 0.6736f, 0.6772f, 0.6808f, 0.6844f, 0.6879f,
			0.6915f, 0.6950f, 0.6985f, 0.7019f, 0.7054f, 0.7088f, 0.7123f, 0.7157f, 0.7190f, 0.7224f,
			0.7257f, 0.7291f, 0.7324f, 0.7357f, 0.7389f, 0.7422f, 0.7454f, 0.7486f, 0.7517f, 0.7549f,
			0.7580f, 0.7611f, 0.7642f, 0.7673f, 0.7704f, 0.7734f, 0.7764f, 0.7794f, 0.7823f, 0.7852f,
			0.7881f, 0.7910f, 0.7939f, 0.7967f, 0.7995f, 0.8023f, 0.8051f, 0.8078f, 0.8106f, 0.8133f,
			0.8159f, 0.8186f, 0.8212f, 0.8238f, 0.8264f, 0.8289f, 0.8315f, 0.8340f, 0.8365f, 0.8389f,
			0.8413f, 0.8438f, 0.8461f, 0.8485f, 0.8508f, 0.8531f, 0.8554f, 0.8577f, 0.8599f, 0.8621f,
			0.8643f, 0.8665f, 0.8686f, 0.8708f, 0.8729f, 0.8749f, 0.8770f, 0.8790f, 0.8810f, 0.8830f,
			0.8849f, 0.8869f, 0.8888f, 0.8907f, 0.8925f, 0.8944f, 0.8962f, 0.8980f, 0.8997f, 0.9015f,
			0.9032f, 0.9049f, 0.9066f, 0.9082f, 0.9099f, 0.9115f, 0.9131f, 0.9147f, 0.9162f, 0.9177f,
			0.9192f, 0.9207f, 0.9222f, 0.9236f, 0.9251f, 0.9265f, 0.9279f, 0.9292f, 0.9306f, 0.9319f,
			0.9332f, 0.9345f, 0.9357f, 0.9370f, 0.9382f, 0.9394f, 0.9406f, 0.9418f, 0.9429f, 0.9441f,
			0.9452f, 0.9463f, 0.9474f, 0.9484f, 0.9495f, 0.9505f, 0.9515f, 0.9525f, 0.9535f, 0.9545f,
			0.9554f, 0.9564f, 0.9573f, 0.9582f, 0.9591f, 0.9599f, 0.9608f, 0.9616f, 0.9625f, 0.9633f,
			0.9641f, 0.9649f, 0.9656f, 0.9664f, 0.9671f, 0.9678f, 0.9686f, 0.9693f, 0.9699f, 0.9706f,
			0.9713f, 0.9719f, 0.9726f, 0.9732f, 0.9738f, 0.9744f, 0.9750f, 0.9756f, 0.9761f, 0.9767f,
			0.9772f, 0.9778f, 0.9783f, 0.9788f, 0.9793f, 0.9798f, 0.9803f, 0.9808f, 0.9812f, 0.9817f,
			0.9821f, 0.9826f, 0.9830f, 0.9834f, 0.9838f, 0.9842f, 0.9846f, 0.9850f, 0.9854f, 0.9857f,
			0.9861f, 0.9864f, 0.9868f, 0.9871f, 0.9875f, 0.9878f, 0.9881f, 0.9884f, 0.9887f, 0.9890f,
			0.9893f, 0.9896f, 0.9898f, 0.9901f, 0.9904f, 0.9906f, 0.9909f, 0.9911f, 0.9913f, 0.9916f,
			0.9918f, 0.9920f, 0.9922f, 0.9925f, 0.9927f, 0.9929f, 0.9931f, 0.9932f, 0.9934f, 0.9936f,
			0.9938f, 0.9940f, 0.9941f, 0.9943f, 0.9945f, 0.9946f, 0.9948f, 0.9949f, 0.9951f, 0.9952f,
			0.9953f, 0.9955f, 0.9956f, 0.9957f, 0.9959f, 0.9960f, 0.9961f, 0.9962f, 0.9963f, 0.9964f,
			0.9965f, 0.9966f, 0.9967f, 0.9968f, 0.9969f, 0.9970f, 0.9971f, 0.9972f, 0.9973f, 0.9974f,
			0.9974f, 0.9975f, 0.9976f, 0.9977f, 0.9977f, 0.9978f, 0.9979f, 0.9979f, 0.9980f, 0.9981f,
			0.9981f, 0.9982f, 0.9982f, 0.9983f, 0.9984f, 0.9984f, 0.9985f, 0.9985f, 0.9986f, 0.9986f,
			0.9987f, 0.9987f, 0.9987f, 0.9988f, 0.9988f, 0.9989f, 0.9989f, 0.9989f, 0.9990f, 0.9990f
		};

		NormalDistribution(const float mean, const float sigma3) : mean(mean), stdDev(sigma3 / 3.0f) {}

		float Area(const float min, const float max) const
		{
			const float zMin    = (min - mean) / stdDev;
			const float zMax    = (max - mean) / stdDev;
			const int zMinIndex = std::isinf(zMin) ? 309 : std::clamp(static_cast<int>(std::round(std::abs(zMin) / Z_STEP)), 0, 309);
			const int zMaxIndex = std::isinf(zMax) ? 309 : std::clamp(static_cast<int>(std::round(std::abs(zMax) / Z_STEP)), 0, 309);
			const float areaMin = zMin >= 0 ? Z_TABLE[zMinIndex] : 1.0f - Z_TABLE[zMinIndex];
			const float areaMax = zMax >= 0 ? Z_TABLE[zMaxIndex] : 1.0f - Z_TABLE[zMaxIndex];
			return areaMax - areaMin;
		}

		float AreaLessThan(const float max) const
		{
			return Area(-std::numeric_limits<float>::infinity(), max);
		}

		float AreaGreaterThan(const float min) const
		{
			return Area(min, std::numeric_limits<float>::infinity());
		}

		std::vector<float> Area(const float begin, const float end, const int steps) const
		{
			std::vector<float> areas;
			float max = begin;
			areas.push_back(AreaLessThan(begin));

			const float step = (end - begin) / (steps - 2);
			for (int i = 0; i < steps - 2; i++)
			{
				const float min = max;
				max += step;
				areas.push_back(Area(min, max));
			}
			areas.push_back(AreaGreaterThan(end));
			return areas;
		}
	};

	struct SineWave
	{
		float freq;
		float amp;
		float base;
		float phase;

		SineWave(const float freq, const float amp, const float base, const float phase) : freq(freq), amp(amp), base(base), phase(phase) {}

		float F(const float x) const
		{
			return base + amp * std::sinf(x * freq + phase);
		}

		float dFdx(const float x) const
		{
			return std::cosf(x * freq + phase) * freq;
		}
	};

	struct TrigonometricUberNoise1D
	{
		SineWave feature;
		SineWave sharpness;
		SineWave slopeErosion;
		SineWave perturb;
		float featureLacunarity;
		float detailAmp;
		static constexpr int octaves = 4;

		explicit TrigonometricUberNoise1D(const Echo::EditorBiomeTerrainUI::GenerateParam& param) :
			feature(param.baseFrequency, param.baseAmplitude, 0.0f, 0.0f),
			sharpness(param.sharpnessFrequency, param.sharpnessRange.y, param.sharpnessRange.x, 1.57f),
			slopeErosion(param.slopeErosionFrequency, param.slopeErosionRange.y, param.slopeErosionRange.x, -1.9f),
			perturb(param.perturbFrequency, param.perturbRange.y, param.perturbRange.x, 0.0f),
			featureLacunarity(param.lacunarity), detailAmp(param.detailAmplitude) {}

		std::vector<float> Elevation(const float begin, const float end, const int step) const
		{
			std::vector<float> elevations;
			const float stepSize = (end - begin) / step;
			for (int i = 0; i < step; i++)
			{
				const float x = begin + i * stepSize;
				float featureFreq = feature.freq;
				float amp = 1.0;
				float noise = feature.base;
				float slopErosionGrad = 0.0f;
				float perturbGrad = 0.0f;
				float sharpnessX = x;
				float erosionX = x;

				float currPerturb = std::clamp(perturb.F(x), -1.0f, 1.0f);

				float sharpnessValue = std::clamp(sharpness.F(sharpnessX), -1.0f, 1.0f);
				float slopeErosionValue = std::clamp(slopeErosion.F(erosionX), 0.0f, 1.0f);

				for (int j = 0; j < octaves; ++j)
				{
					float dampAmp = amp * (j < (octaves >> 1) ? feature.amp : detailAmp);

					if (j == (octaves >> 1))
					{
						sharpnessValue = std::clamp(sharpness.F(sharpnessX), -1.0f, 1.0f);
						slopeErosionValue = std::clamp(slopeErosion.F(erosionX), 0.0f, 1.0f);
					}

					float x1 = x * featureFreq + perturbGrad * 10.0f * Echo::Math::PI;
					float featureNoise  = feature.F(x1);
					float featureGrad   = feature.dFdx(x1);
					const float billowy = std::abs(featureNoise) * 2.0f - 1.0f;
					const float ridged  = (1.0f - std::abs(featureNoise)) * 2.0f - 1.0f;

					featureNoise = std::lerp(featureNoise, ridged, std::max(0.0f, sharpnessValue));
					featureNoise = std::lerp(featureNoise, billowy, std::abs(std::min(0.0f, sharpnessValue)));

					slopErosionGrad += featureGrad * slopeErosionValue;
					perturbGrad += featureGrad * currPerturb * dampAmp;
					dampAmp *= 1.0f / (1.0f + 5.0f * (slopErosionGrad * slopErosionGrad));
					featureNoise *= dampAmp;

					noise += amp * featureNoise;

					featureFreq *= featureLacunarity;
					sharpnessX *= 2.01f;
					erosionX *= 2.01f;
					amp *= 0.5f;
				}

				elevations.push_back(noise);
			}
			return elevations;
		}
	};
}

namespace Echo 
{

    //NOTE(yanghang):Forward declaration.
    void generateCoarseRegionMapAndSDF();
	void generateCoarseRegionMapAndSDFPlane();
        
	static void saveJSONFile(const String& path, cJSON* json)
	{
		if (json)
		{
			char* json_string = cJSON_Print(json);

			DataStream* data_stream = 0;
			data_stream = Root::instance()->CreateDataStream(path.c_str());
			if (data_stream)
			{
				assert(data_stream);
				FileDataStream* file_stream = (FileDataStream*)data_stream;
				file_stream->write(json_string, strlen(json_string));
				SAFE_DELETE(json_string);
				SAFE_DELETE(file_stream);
			}
		}
	}
	EditorBiomeTerrainUI::EditorBiomeTerrainUI(int class_id, ImGuiContext* ctx) :
		BaseUI(class_id, ctx)
	{	
		this->mFlags |= ImGuiWindowFlags_NoScrollWithMouse;
		m_isInit = false;
		m_fileState = FILE_STATE::FIRSTINIT;

		if (nullptr != ctx) {
			ImGui::SetCurrentContext(ctx);
		}
		InitGui(class_id);

		planetGeomtryAllTypeCache.emplace(100, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(300, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(500, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(1000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(2000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(3000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(4000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(6000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(8000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(11000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(18000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(20000, GemotryRadius());
		planetGeomtryAllTypeCache.emplace(22000, GemotryRadius());
	}

	EditorBiomeTerrainUI::~EditorBiomeTerrainUI()
	{
		StampHeightmapUI::Pool::getInstance().release();
		std::ofstream terr;
		terr.open("edit_sph_terr.txt");
		terr << EditorBiomeMgr::getInstance().terrain_ptr->m_terrainFilePath;
		terr.close();
		if (m_RegionGenerator)
		{
			delete m_RegionGenerator;
			m_RegionGenerator = nullptr;
		}
		// EditorBiomeMgr::getInstance().terrain_ptr->destroy();
		ReleaseD3D11TexInfo(m_texTInfo);
		ReleaseD3D11TexInfo(m_texHInfo);
		ReleaseD3D11TexInfo(m_texRHInfo);
	}

	void EditorBiomeTerrainUI::SelectDestributionFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "biome文件\0*.biome\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info,  path))
		{
			if (path.empty() || (path.find(".biome") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为biome的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{				
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->distribution.filePath = path;
				SphericalTerrainResource::loadBiomeDistribution(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->distribution);
				auto&& data = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData;
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->biomeDefineChanged(data->composition, data->distribution,true);
				EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "ReloadBiomeDefine");
				//TODO:更新Destribution
				//EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->biomeDefineChanged(path);
				

					//m_defineUI->ReloadBiomeDefine();
                //IMPORTANT(yanghang):copy geometry data to editor data.
                freshData();

                //IMPORTANT(yanghang):After load file from outside. restore the state dirty flag.
                EditorBiomeMgr::getInstance().terrain_ptr->DistributionChanged = false;
			}
		}
	}

	void EditorBiomeTerrainUI::SaveAsDestributionFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "biome文件\0*.biome\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, ".biome", path))
		{
			if (path.empty() || (path.find(".biome") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为biome的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->distribution.filePath = path;
				SphericalTerrainResource::saveBiomeDistribution(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->distribution);
				EditorBiomeMgr::getInstance().terrain_ptr->DistributionChanged = false;
			}
		}
	}

	void EditorBiomeTerrainUI::SelectCompositionFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "biomecompo文件\0*.biomecompo\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
		{
			if (path.empty() || (path.find(".biomecompo") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为biomecompo的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->composition.filePath = path;
				SphericalTerrainResource::loadBiomeComposition(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->composition);
				auto&& data = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData;
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->biomeDefineChanged(data->composition, data->distribution, true);
				EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "ReloadBiomeDefine");

                EditorBiomeMgr::getInstance().terrain_ptr->ComposeChanged = false;
			}
		}
	}
	void EditorBiomeTerrainUI::SaveAsCompositionFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "biomecompo文件\0*.biomecompo\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, ".biomecompo", path))
		{
			if (path.empty() || (path.find(".biomecompo") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为biomecompo的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->composition.filePath = path;
				SphericalTerrainResource::saveBiomeComposition(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->composition);
				EditorBiomeMgr::getInstance().terrain_ptr->ComposeChanged = false;
			}
		}
	}

	void EditorBiomeTerrainUI::SelectGeomtryFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "terraingeo文件\0*.terraingeo\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
		{
			if (path.empty() || (path.find(".terraingeo") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为terraingeo的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);

                TerrainGeometry& tGeometry= EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry;
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.filePath = path;
				SphericalTerrainResource::loadTerrainGeometry(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry);

				TerrainGeneratorLibrary::instance()->remove(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->m_pTerrainGenerator);

				if (EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.topology == PTS_Sphere) {
					EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->m_pTerrainGenerator = TerrainGeneratorLibrary::instance()->add(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.sphere.gempType);
				}

                auto planet = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain();
				planet->shutDown();
				planet->createImpl(planet->m_terrainData);
				planet->bindTextures();

                freshData();

                EditorBiomeMgr::getInstance().terrain_ptr->GeoChanged = false;
			}
		}
	}

	void EditorBiomeTerrainUI::SaveAsGeomtryFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "terraingeo文件\0*.terraingeo\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, ".terraingeo", path))
		{
			if (path.empty() || (path.find(".terraingeo") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为terraingeo的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);

				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.filePath = path;
				EditorBiomeMgr::getInstance().terrain_ptr->reGenBoundBox();
				SphericalTerrainResource::saveTerrainGeometry(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry);
				EditorBiomeMgr::getInstance().terrain_ptr->GeoChanged = false;
			}
		}
	}

	void EditorBiomeTerrainUI::SelectOceanFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "ocean文件\0*.ocean\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
		{
			if (path.empty() || (path.find(".ocean") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为ocean的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->sphericalOcean.filePath = path;
				SphericalTerrainResource::loadSphericalOcean(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->sphericalOcean);
			}
		}
	}

	void EditorBiomeTerrainUI::SaveAsOceanFile()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "ocean文件\0*.ocean\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, ".ocean", path))
		{
			if (path.empty() || (path.find(".ocean") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为ocean的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->sphericalOcean.filePath = path;
				SphericalTerrainResource::saveSphericalOcean(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->sphericalOcean);
			}
		}
	}

	void EditorBiomeTerrainUI::SaveAs()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "terrain文件\0*.terrain\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, ".terrain", path))
		{
			if (path.empty() || (path.find(".terrain") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为terrain的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{			
				ToRelativePath(path);
				EditorBiomeMgr::getInstance().terrain_ptr->m_terrainFilePath = path;
				std::string fileName = path.substr(0, path.find(".terrain"));

                //IMPORTANT(yanghang):Save as three different files only when
                // their data changed.
                auto& terrainMgr = EditorBiomeMgr::getInstance().terrain_ptr;
                if(terrainMgr->GeoChanged)
                {
                    EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.filePath = "";
                }
                
                if(terrainMgr->DistributionChanged)
                {
                    EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->distribution.filePath = "";
                }

                if(terrainMgr->ComposeChanged)
                {
                    EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->composition.filePath = "";
                }
                
				EditorBiomeMgr::getInstance().terrain_ptr->saveTerrain(fileName);

				EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "ReloadBiomeDefine");
					
				m_fileState = FILE_STATE::OPEN;

                //IMPORTANT(yanghang):Restore the planet data dirty flags
                {
                    auto& terrianMgr = EditorBiomeMgr::getInstance().terrain_ptr;
                    terrianMgr->GeoChanged = false;
                    terrianMgr->ComposeChanged = false;
                    terrianMgr->DistributionChanged = false;
                }
			}
		}
	}

	void EditorBiomeTerrainUI::reLoadTerrain()
	{
		auto& terrainMgr = EditorBiomeMgr::getInstance().terrain_ptr;

		EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "reloadMainTerrain");
		EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "ReloadBiomeDefine");
		//IMPORTANT(yanghang):copy geometry data to editor data.
		freshData();
		m_fileState = FILE_STATE::OPEN;

		terrain = terrainMgr->GetSphericalTerrain();
		terrainData = terrain->m_terrainData.get();

		m_oceanGeoParam = terrainData->sphericalOcean.geomSettings;
		m_oceanShadeParam = terrainData->sphericalOcean.shadeSettings;
		m_oceanFileParam = terrainData->sphericalOcean.fileSetting;

		//IMPORTANT(yanghang):Restore the planet data dirty flags
		{
			auto& terrianMgr = terrainMgr;
			terrianMgr->GeoChanged = false;
			terrianMgr->ComposeChanged = false;
			terrianMgr->DistributionChanged = false;
		}
	}

	void EditorBiomeTerrainUI::ToRelativePath(std::string& path)
	{
		std::replace(path.begin(), path.end(), '\\', '/');
		path = path.substr(path.find("biome_terrain"));
	}

	void EditorBiomeTerrainUI::MarkRGParamDirty()
	{
		if (EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain())
		{
			const SphericalTerrainData& terrainData = *(SphericalTerrainData*)EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData.getPointer();
			m_RGParam.uniform.innerRadius = terrainData.region.uniform.innerRadius;
			m_RGParam.uniform.outerRadius = terrainData.region.uniform.outerRadius;
			m_RGParam.uniform.threshold = terrainData.region.uniform.threshold;
			m_RGParam.uniform.lambda = terrainData.region.uniform.lambda;
		}	
	}

	void EditorBiomeTerrainUI::ReleaseD3D11TexInfo(D3D11TexInfo& info)
	{
		if (info.d3D11ResView)
		{
			info.d3D11ResView->Release();
			info.d3D11ResView = nullptr;
		}

		if (info.d3D11Texture2D)
		{
			info.d3D11Texture2D->Release();
			info.d3D11Texture2D = nullptr;
		}
	}

	unsigned char* EditorBiomeTerrainUI::ToFourChannels(const void* pData, unsigned int sz, bool isHightMap)
	{
		unsigned char* pixels = new unsigned char[sz*4];
		memset(pixels, 0, sz*4);
		for (unsigned int i = 0; i < sz; i++)
		{
			unsigned char r = 0;
			if (isHightMap)
			{
				r = (unsigned char)(((float*)pData)[i] * 255);
			}
			else
			{
				r = ((unsigned char*)pData)[i];
			}
			pixels[i * 4] = r;
			pixels[i * 4 + 1] = r;
			pixels[i * 4 + 2] = r;
			pixels[i * 4 + 3] = 255;
		}
		return pixels;
	}

	void EditorBiomeTerrainUI::CheckParamChange()
	{
		/*if (std::abs(m_heightRange.min - m_heightRangePrev.min) > 0.00001f || std::abs(m_heightRange.max - m_heightRangePrev.max) > 0.00001f)
		{
			pMgr->setHeightRange(m_heightRange);
			m_heightRangePrev = m_heightRange;
		}*/

		if (m_TParam != m_TParamPrev)
		{
			EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->loadTemperatureClimateProcess(m_TParam);
			m_TParamPrev = m_TParam;
		}

		if (m_RHParam != m_RHParamPrev || m_AoParam != m_AoParamPrev)
		{
			EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->loadHumidityClimateProcess(m_RHParam, m_AoParam);
			m_RHParamPrev = m_RHParam;
			m_AoParamPrev = m_AoParam;
		}
	}

	void EditorBiomeTerrainUI::whetherSaveAll()
	{
		if (EditorBiomeMgr::getInstance().terrain_ptr->DistributionChanged)
		{
			SaveAsDestributionFile();
		}
		if (EditorBiomeMgr::getInstance().terrain_ptr->ComposeChanged)
		{
			SaveAsCompositionFile();
		}
		if (EditorBiomeMgr::getInstance().terrain_ptr->GeoChanged)
		{
			SaveAsGeomtryFile();
		}
	}

	void EditorBiomeTerrainUI::ShowGui()
	{
		CheckParamChange();

		static bool developerMode = false;
		

		auto* terrainMgr = EditorBiomeMgr::getInstance().terrain_ptr.get();

		terrain = terrainMgr->GetSphericalTerrain();

		bool terrainAlive = true;
		if (terrain == nullptr)
		{
			terrainAlive = false;
		}
		else
		{
			terrainData = terrain->m_terrainData.get();
		}

		if (m_fileState == FILE_STATE::FIRSTINIT)
		{
			terrainMgr->reload(true);
			reLoadTerrain();
			m_fileState = FILE_STATE::OPEN;
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu(U8("菜单")))
			{
				if (ImGui::MenuItem("新建")) 
				{
					terrainMgr->m_terrainFilePath = terrainMgr->m_default_3dProceduralTerrainFilePath;
					terrainMgr->reload(true);

					terrain = terrainMgr->GetSphericalTerrain();
					terrainData = terrain->m_terrainData.get();

					EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "reloadMainTerrain");
					//EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->biomeDefineChanged(EditorBiomeMgr::getInstance().terrain_ptr->getBiomeDistributionFilePath());
					EditorBiomeMgr::getInstance().callVoidFuc("EditorBiomeDefineUI", "ReloadBiomeDefine");
                    freshData();
                    
					m_fileState = FILE_STATE::OPEN;
				}
				if (ImGui::MenuItem("打开"))
				{
					Echo::FileLoadSaveWinInfo info;
					char utf8_test[] = "terrain文件\0*.terrain\0";
					UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
					memcpy(info.filter, utf8_test, sizeof(utf8_test));

					info.initialDir = Root::instance()->getRootResourcePath() + "biome_terrain";
					std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
					std::string path;
					if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
					{
						if (path.empty() || (path.find(".terrain") == path.npos))
						{
							std::string log = U8("[警告]: 请输入后缀为terrain的资源！");
							EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
						}
						else
						{
							ToRelativePath(path);
							terrainMgr->m_terrainFilePath = path;
							terrainMgr->reload(true);
							reLoadTerrain();
							terrain = terrainMgr->GetSphericalTerrain();
							terrainData = terrain->m_terrainData.get();
						}
					}
				}
				if (terrainAlive)
				{
					if (ImGui::MenuItem("保存"))
					{
						whetherSaveAll();
						std::string fileName = terrainMgr->m_terrainFilePath.substr(0, terrainMgr->m_terrainFilePath.find(".terrain"));
						terrainMgr->saveTerrain(fileName);
					}
					if (ImGui::MenuItem("另存为"))
					{
						SaveAs();
					}
				}

                //TODO(yanghang):Don't commit this.Just for internal test now.
                if(EditorBiomeMgr::getInstance().define_ptr &&
                   EditorBiomeMgr::getInstance().define_ptr->m_biomeData.m_internalMode)
                {
                    if (ImGui::MenuItem("生成离线区域距离场"))
                    {
                        generateCoarseRegionMapAndSDF();
						generateCoarseRegionMapAndSDFPlane();
                    }
                }

#if 0
				if (ImGui::MenuItem("测试"))
				{
					// auto regions = EditorTerrainMgr::ComputePreferredRegionAmount(1000.0f, EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->getMaxRealRadius());
					auto cvec = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->GetCoarseLayerCenters();
					auto vec = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->GetFineLayerCenters();
					std::vector< SphericalVoronoiRegionDefine > temp(50);
					String str;
					for (int i = 0; i < 255; i++)
					{
						str.push_back('r');
					}
					for (int i = 0; i < 255; i++)
					{
						str.push_back('l');
					}
					for(int j = 0;j <10;j++)
					{
						temp[j].StrEx = str;
						temp[j].IntEx = 20;
					}
					for (int j = 11; j < 20; j++)
					{
						temp[j].name = "10+";
						temp[j].TemplateTOD = "0909";
					}
					EditorBiomeMgr::getInstance().terrain_ptr->getherToTerrainFile("biome_terrain/001.terraingeo",
						"biome_terrain/001.biome",
						"biome_terrain/001.biomecompo",
						"biome_terrain/041600", "", true, 3, temp);

					std::vector< SphericalVoronoiRegionDefineImpl > A(3 * 3 * 6);
					for (int i = 0; i < A.size(); ++i)
					{
						A[i].IntEx = i;
						A[i].StrEx = "AAA3*3*6_" + std::to_string(i);
						if(i==0)
						{

							for(int ii=(int)A[i].StrEx.size();ii<256;++ii)
							{
								A[i].StrEx += "r";
							}
							A[i].StrEx += "stop";
						}
					}

					std::vector< SphericalVoronoiRegionDefineImpl > B(2 * 2 * 6);
					for (int i = 0; i < B.size(); ++i)
					{
						B[i].IntEx = i;
						B[i].StrEx = "BBB2*2*6_" + std::to_string(i+i);
					}
					EditorBiomeMgr::getInstance().terrain_ptr->updateRegionData("biome_terrain/terrain_random/2531-a", A, B);
					EditorBiomeMgr::getInstance().terrain_ptr->updateRegionData("biome_terrain/terrain_random/2534-a", A, B);
					/*EditorBiomeMgr::getInstance().terrain_ptr->getherToTerrainFile("biome_terrain/001.terraingeo",
						"biome_terrain/001.biome",
						"biome_terrain/001.biomecompo",
						"biome_terrain/041601.terrain", "", true, 3, temp);*/
					EditorBiomeMgr::getInstance().terrain_ptr->updateRegionData("biome_terrain/terrain_random/2657-a",B, A);
					EditorBiomeMgr::getInstance().terrain_ptr->updateRegionData("biome_terrain/terrain_random/7143-a", B, A);
					EditorBiomeMgr::getInstance().terrain_ptr->updateRegionData("biome_terrain/terrain_random/7075-a", A, B);
				}
#endif
				ImGui::EndMenu();
			}

			ImGui::SameLine();
			if (terrainAlive)
			{
				if (ImGui::BeginMenu(U8("显示模式")))
				{
					if (ImGui::MenuItem("Full"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::Full);
					}
					if (ImGui::MenuItem("Height"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::Height);
					}
					if (ImGui::MenuItem("Temperature"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::Temperature);
					}
					if (ImGui::MenuItem("Humidity"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::Humidity);
					}
					if (ImGui::MenuItem("MaterialIndex"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::MaterialIndex);
					}

					if (ImGui::MenuItem("LOD"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::LOD);
					}
					if (ImGui::MenuItem("GeometryNormal"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::GeometryNormal);
					}
					if (ImGui::MenuItem("RegionIndex"))
					{
						if (!Root::instance()->m_useSphericalVoronoiRegion)
						{
							terrain->setDisplayMode(BiomeDisplayMode::RegionIndex);
						}
						else
						{
							terrain->setDisplayMode(BiomeDisplayMode::PlanetRegionIndex);
						}
					}
					if (!Root::instance()->m_useSphericalVoronoiRegion)
					{
						if (ImGui::MenuItem("RegionIndex Process"))
						{
							terrain->setDisplayMode(BiomeDisplayMode::RegionIndexProcess);
						}
						if (ImGui::MenuItem("RegionIndex Offline"))
						{
							terrain->setDisplayMode(BiomeDisplayMode::RegionIndexOffline);
						}
					}
					if (Root::instance()->m_bManualPlanetRegion)
					{
						if (ImGui::MenuItem("ManualPlanetRegion"))
						{
							terrain->setDisplayMode(BiomeDisplayMode::ManualPlanetRegion);
						}
					}
					if (ImGui::MenuItem("Tile"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::Tile);
					}
					if (ImGui::MenuItem("MaterialLevel"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::MaterialLevel);
					}
					if (ImGui::MenuItem("TriplanarBlendWeight"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::TriplanarBlendWeight);
					}
					if (ImGui::MenuItem("NoisedUV"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::NoisedUV);
					}
					if (ImGui::MenuItem("LerpedUV"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::LerpedUV);
					}
					if (ImGui::MenuItem("TextureTillingNoise"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::TextureTillingNoise);
					}
					if (ImGui::MenuItem("Developer Mode"))
					{
						developerMode = !developerMode;
					}

					if (ImGui::MenuItem("HeatMap"))
					{
						terrain->setDisplayMode(BiomeDisplayMode::GeometryHeatmap);
					}

					ImGui::EndMenu();
				}

				ImGui::SameLine();

				if (ImGui::BeginMenu(U8("海洋")))
				{
					if (ImGui::MenuItem("附加海洋"))
					{
						terrainMgr->sphericalOceanChanged(true);
					}
					if (ImGui::MenuItem("删除海洋"))
					{
						terrainMgr->sphericalOceanChanged(false);
					}
					ImGui::EndMenu();
				}
			}


			ImGui::EndMenuBar();
		}
		ImVec2 uiSize =  ImGui::GetContentRegionAvail();
		if (terrainAlive)
		{
			ImGui::Text(U8("Terrain文件：%s"), terrainMgr->getTerrainFilePath().c_str());

			ImGui::Text(U8("几何文件：%s"), terrainMgr->getGeoFilePath().c_str());
			ImGui::SameLine();
			if (ImGui::Button(U8("导入##几何")))
			{
				SelectGeomtryFile();
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("保存##几何")))
			{
				terrainMgr->reGenBoundBox();
				SphericalTerrainResource::saveTerrainGeometry(terrainData->geometry);
				terrainMgr->GeoChanged = false;
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("另存为##几何")))
			{
				SaveAsGeomtryFile();
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("批量导出##几何")))
			{
				if (!isOpenSaveAsAllTypeGeomtryFilesUI)
					isOpenSaveAsAllTypeGeomtryFilesUI = true;
			}
			ImGui::Text(U8("分布文件：%s"), terrainMgr->getBiomeDistributionFilePath().c_str());
			ImGui::SameLine();
			if (ImGui::Button(U8("导入##分布")))
			{
				SelectDestributionFile();
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("保存##分布")))
			{
				SphericalTerrainResource::saveBiomeDistribution(terrainData->distribution);
				terrainMgr->DistributionChanged = false;
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("另存为##分布")))
			{
				SaveAsDestributionFile();
			}
			ImGui::Text(U8("组合文件：%s"), terrainMgr->getComposeFilePath().c_str());
			ImGui::SameLine();
			if (ImGui::Button(U8("导入")))
			{
				SelectCompositionFile();
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("保存##组合")))
			{
				SphericalTerrainResource::saveBiomeComposition(terrainData->composition);
				terrainMgr->ComposeChanged = false;
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("另存为##组合")))
			{
				SaveAsCompositionFile();
			}

			if (terrainMgr->mainTerrain->m_haveOcean)
			{
				ImGui::Text(U8("海洋文件：%s"), terrainData->sphericalOcean.filePath.c_str());
				ImGui::SameLine();
				if (ImGui::Button(U8("导入##海洋")))
				{
					SelectOceanFile();
					terrain->m_ocean->setupOcean(terrainData->sphericalOcean);
				}
				ImGui::SameLine();
				if (ImGui::Button(U8("保存##海洋")))
				{
					SphericalTerrainResource::saveSphericalOcean(terrainData->sphericalOcean);
					terrain->m_ocean->setupOcean(terrainData->sphericalOcean);
				}
				ImGui::SameLine();
				if (ImGui::Button(U8("另存为##海洋")))
				{
					SaveAsOceanFile();
					terrain->m_ocean->setupOcean(terrainData->sphericalOcean);
				}

			}
			uiSize = ImGui::GetContentRegionAvail();
			if (ImGui::BeginChild("ImagePanel", ImVec2(600, 30.0f), false, ImGuiWindowFlags_NoScrollWithMouse))
			{
				static bool showElevation = false, showTemperature = false, showHumidity = false;
				if (ImGui::Checkbox(U8("显示高度图"), &showElevation))
				{
					if (terrain)
						terrain->showElevation(showElevation);
				}
				ImGui::SameLine();
				if (ImGui::Checkbox(U8("显示温度图"), &showTemperature))
				{
					if (terrain)
						terrain->showTemperature(showTemperature);

				}
				ImGui::SameLine();
				if (ImGui::Checkbox(U8("显示湿度图"), &showHumidity))
				{
					if (terrain)
						terrain->showHumidity(showHumidity);
				}
			}
			ImGui::EndChild();

			static bool transComp = false;
			if (ImGui::Button(U8("组件模式")))
			{
				transComp = true;
			}
			if (transComp)
			{
				ImGui::OpenPopup("zujian");
				if (ImGui::BeginPopup("zujian"))
				{
					ImGui::Text(U8("需要重新保存"));
				}
				if (ImGui::Button(U8("确定##zujian")))
				{
					ImGui::CloseCurrentPopup();
					whetherSaveAll();
					std::string fileName = terrainMgr->m_terrainFilePath.substr(0, terrainMgr->m_terrainFilePath.find(".terrain"));
					terrainMgr->saveTerrain(fileName);
					terrainMgr->destroy();
					terrainMgr->transToComponentFuc(EditorBiomeMgr::getInstance().terrain_ptr->actorPtr, EditorBiomeMgr::getInstance().terrain_ptr->desc, EditorBiomeMgr::getInstance().terrain_ptr->mSceneActorHandle);
					terrainAlive = false;
					transComp = false;
				}
				ImGui::EndPopup();
			}
		}

		if (!terrainAlive)
			return;
		/*if (ImGui::BeginChild("ImagePanel", ImVec2(210, uiSize.y * 1.0f), false))
		{
			ImVec2 uiSize = ImGui::GetContentRegionAvail();

			if (ImGui::BeginChild("高度图图片", ImVec2(uiSize.x, 250), false))
			{
				ImGui::Separator();
				ImGui::Text(U8("高度图"));
							
				if (m_needUpdateImage)
				{
					HeightMap map = pMgr->c_originHeightMap;					
					unsigned char* pixels = ToFourChannels(map.pixels, map.m_x * map.m_y, true);
					ReleaseD3D11TexInfo(m_texHInfo);
					EditorRoot::instance()->CreateD3D11Tex((unsigned char*)pixels, map.m_x, map.m_y, m_texHInfo);	
					delete[] pixels;
				}

				if (m_texHInfo.d3D11ResView)
					ImGui::Image(m_texHInfo.d3D11ResView, ImVec2(200, 200));
			}
			ImGui::EndChild();

			if (ImGui::BeginChild("温度图图片", ImVec2(uiSize.x, 250), false))
			{				
				ImGui::Separator();
				ImGui::Text(U8("温度图"));
				if (m_texTPath.empty())
					m_texTPath = pMgr->m_temperatureTexPath;					

				if (m_needUpdateImage && pMgr->m_proceduralTemperatureMap.pixels != nullptr)
				{
					Bitmap& bitmap = pMgr->m_proceduralTemperatureMap;					
					unsigned char* pixels = ToFourChannels(bitmap.pixels, bitmap.width * bitmap.height);
					ReleaseD3D11TexInfo(m_texTInfo);
					EditorRoot::instance()->CreateD3D11Tex(pixels, bitmap.width, bitmap.height, m_texTInfo);										
					delete[] pixels;
				}

				if (m_texTInfo.d3D11ResView)
					ImGui::Image(m_texTInfo.d3D11ResView, ImVec2(200, 200));
			}
			ImGui::EndChild();

			if (ImGui::BeginChild("湿度图图片", ImVec2(uiSize.x, 250), false))
			{
				ImGui::Separator();
				ImGui::Text(U8("湿度图"));

				if (m_texRHPath.empty())
					m_texRHPath = pMgr->m_humidityTexPath;				

				if (m_needUpdateImage && pMgr->m_proceduralHumidityMap.pixels != nullptr)
				{
					Bitmap& bitmap = pMgr->m_proceduralHumidityMap;
					unsigned char* pixels = ToFourChannels(bitmap.pixels, bitmap.width * bitmap.height);
					ReleaseD3D11TexInfo(m_texRHInfo);
					EditorRoot::instance()->CreateD3D11Tex(pixels, bitmap.width, bitmap.height, m_texRHInfo);
					delete[] pixels;
				}

				if (m_texRHInfo.d3D11ResView)
					ImGui::Image(m_texRHInfo.d3D11ResView, ImVec2(200, 200));
			}
			ImGui::EndChild();

			m_needUpdateImage = false;
		}
		ImGui::EndChild();*/
		//ImGui::SameLine();
		if (ImGui::BeginChild("ParamPanel", ImVec2(500, uiSize.y * 1.0f), false, ImGuiWindowFlags_NoScrollWithMouse| ImGuiWindowFlags_AlwaysAutoResize))
		{		
			ImVec2 uiSize = ImGui::GetContentRegionAvail();

			if (ImGui::BeginChild("几何参数", ImVec2(uiSize.x, 150), false))
			{
				bool changed = false;
				ImGui::Text(U8("高度场参数："));
				if (ImGui::RadioButton(U8("球形拓扑"), m_shapeType == PTS_Sphere && m_sphere.gempType == -1))
				{
					m_shapeType = PTS_Sphere;
					m_sphere.gempType = -1;
					changed = true;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton(U8("环面拓扑"), m_shapeType == PTS_Torus))
				{
					m_shapeType = PTS_Torus;
					m_sphere.gempType = -1;
					changed = true;
				}
				{
					int newLineNumber = 2;
					const auto& shapeTypeOfSphereMap = TerrainGeneratorLibrary::instance()->getTypes();
					for (const auto& shapeTypeOfSphere : shapeTypeOfSphereMap) {
						if (newLineNumber++ % 2 != 0) {
							ImGui::SameLine();
						}
						if (ImGui::RadioButton(shapeTypeOfSphere.second.c_str(), m_shapeType == PTS_Sphere && m_sphere.gempType == shapeTypeOfSphere.first))
						{
							m_shapeType = PTS_Sphere;
							m_sphere.gempType = shapeTypeOfSphere.first;
							changed = true;
						}
					}
				}

				if (changed)
				{
					terrainMgr->setTerrainGeneratorType(m_sphere.gempType);
					terrainMgr->setTopology(m_shapeType);
					if (terrainMgr->mainTerrain->m_haveOcean && terrain->m_ocean != nullptr)
					{
						terrain->m_ocean->MgrRebuild();
					}
						
				}

				switch (m_shapeType)
				{
				case PTS_Sphere:
				{
					auto& sphere = m_sphere;
					changed |= ImGui::InputFloat(U8("半径"), &sphere.radius);
					changed |= HoverAndWheelControl(&sphere.radius, 100.0f, 50000.0f, 10.0f);
					sphere.radius     = std::clamp(sphere.radius, 1.0f, 50000.0f);
					float heightScale = sphere.elevationRatio * sphere.radius;
					changed |= ImGui::InputFloat(U8("海拔"), &heightScale);
					changed |= HoverAndWheelControl(&heightScale, 0.0f, sphere.radius * 0.1f, 1.0f);
					heightScale           = std::clamp(heightScale, 0.0f, sphere.radius * 0.1f);
					sphere.elevationRatio = heightScale / sphere.radius;

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					if (ImGui::Button(U8("重置")))
					{
						sphere.elevationRatio = 0.025f;
						sphere.radius         = 20000.0f;
						changed               = true;
					}
					if (changed)
					{
						terrainMgr->setRadiusElevation(sphere.radius, sphere.elevationRatio);

						m_genParam.baseOctaves  = terrain->get3DNoiseData().baseOctaves;
						m_quadtreeMaxDepth = terrainData->geometry.quadtreeMaxDepth;

						terrainMgr->GeoChanged = true;
					}
				}
				break;
				case PTS_Torus:
				{
					auto& torus = m_torus;
					changed |= ImGui::InputFloat(U8("主半径"), &torus.radius);
					torus.radius      = std::clamp(torus.radius, 1.0f, 50000.0f);

					float minorRadius = torus.relativeMinorRadius * torus.radius;
					changed |= ImGui::InputFloat(U8("次半径"), &minorRadius);
					minorRadius = std::clamp(minorRadius, 1.0f, torus.radius);
					torus.relativeMinorRadius = minorRadius / torus.radius;

					float heightScale = torus.elevationRatio * minorRadius;
					changed |= ImGui::InputFloat(U8("环面海拔"), &heightScale);
					heightScale          = std::clamp(heightScale, 0.0f, minorRadius * 0.5f);
					torus.elevationRatio = heightScale / minorRadius;

					if (changed)
					{
						terrainMgr->setRadiusElevation(torus.radius, torus.relativeMinorRadius, torus.elevationRatio);

						m_quadtreeMaxDepth = terrainData->geometry.quadtreeMaxDepth;
						m_genParam.baseOctaves = terrain->get3DNoiseData().baseOctaves;

						terrainMgr->GeoChanged = true;
					}
				}
				break;
				default: break;
				}
				if (developerMode && ImGui::SliderInt(U8("最大四叉树深度"), &m_quadtreeMaxDepth, 0, 10))
				{
					terrainMgr->setQuadtreeMaxDepth(m_quadtreeMaxDepth);
					terrainMgr->GeoChanged = true;
				}
			}
			ImGui::EndChild();
			uiSize = ImGui::GetContentRegionAvail();
			{
				if (terrainMgr->mainTerrain->m_haveOcean &&
					terrainMgr->mainTerrain->isCreateFinish() &&
					terrain->m_ocean!=nullptr)
				{
					ImGui::Separator();
					ImGui::Text(U8("球面海洋设置："));

					//ImGui::Text(U8("球面海洋文件：%s"), pMgr->m_editSTerrain->m_terrainData->sphericalOcean.name.c_str());

					static bool visible = true;
					if (ImGui::Checkbox(U8("可见##OceanProperty"), &visible))
					{
						terrain->m_ocean->setVisible(visible);
					}

					bool bUseTod = terrain->m_ocean->getIsUseTod();
					if (ImGui::Checkbox(U8("受TOD影响##oceanIsUseTod"), &bUseTod))
					{
						terrain->m_ocean->setIsUSeTod(bUseTod);
					}

					// TODO(Jiamin): Wait to do..
					/*
					std::string meshPath = "testMesh.mesh";
					ImGui::Text(U8("Mesh文件"));
					ImGui::SameLine();
					ImGui::Button((U8("加载##MeshFile")));
					ImGui::SameLine();
					ImGui::Text(meshPath.data());
					*/
					terrain->m_ocean->getFileSetting(m_oceanFileParam);
                    SphereOceanFileSettings& oceanFileSet = terrainData->sphericalOcean.fileSetting;

					std::string normalTexName = m_oceanFileParam.NormalTextureName;
					ImGui::Text(U8("法线贴图"));
					ImGui::SameLine();
					if (ImGui::Button((U8("加载##NormalTexName"))))
					{
						std::string path = SelectOceanTexturePath(".dds");
						if (!path.empty()) {
							normalTexName = path;
							terrain->m_ocean->setNormalTexture(normalTexName);
                            oceanFileSet.NormalTextureName = normalTexName;
						}
					}

					ImGui::SameLine();
					ImGui::Text(normalTexName.data());


					std::string foamTexName = m_oceanFileParam.FoamTextureName;
					ImGui::Text(U8("泡沫贴图"));
					ImGui::SameLine();
					if (ImGui::Button((U8("加载##FoamTexName"))))
					{
						std::string path = SelectOceanTexturePath(".dds");
						if (!path.empty()) {
							foamTexName = path;
							terrain->m_ocean->setFoamTexture(foamTexName);
                            oceanFileSet.FoamTextureName = foamTexName; 
						}
					}
					ImGui::SameLine();
					ImGui::Text(foamTexName.data());

					std::string reflectTextureName = m_oceanFileParam.ReflectTextureName;
					ImGui::Text(U8("反射贴图"));
					ImGui::SameLine();
					if (ImGui::Button((U8("加载##ReflectTextureName"))))
					{
						std::string path = SelectOceanTexturePath(".dds");
						if (!path.empty()) {
							reflectTextureName = path;
							terrain->m_ocean->setReflectTexture(reflectTextureName);
                            oceanFileSet.ReflectTextureName = reflectTextureName;
						}
					}
					ImGui::SameLine();
					ImGui::Text(reflectTextureName.data());

					terrain->m_ocean->getGeomSetting(m_oceanGeoParam);
					bool changed = false;
					static bool followRadius = false;
					static bool followMinorRadius = false;
					if (m_shapeType == PTS_Sphere)
					{
						changed |= ImGui::Checkbox(U8("跟随星球半径##OceanProperty"), &followRadius);
					}

					if (m_shapeType == PTS_Sphere && ImGui::DragFloat(U8("半径##OceanProperty"), &m_oceanGeoParam.radius, 10.f, 1.0f, 200000.f))
					{
						changed = true;
						auto&& ocean = terrain->m_ocean;
						ocean->setGeomSetting(m_oceanGeoParam); 
						ocean->SphericalOceanNodeReset(ocean->computeMaxDepth());
						m_oceanGeoParam.maxDepth = ocean->computeMaxDepth();
						terrain->updateSeaSurfaceTemperature();
					}

					if (m_shapeType == PTS_Torus)
					{
						followRadius = true;
						changed |= ImGui::Checkbox(U8("跟随星球次半径##OceanProperty"), &followMinorRadius);
						float minorRadii = m_oceanGeoParam.relativeMinorRadius * m_oceanGeoParam.radius;

						if (followMinorRadius)
						{
							minorRadii = m_torus.relativeMinorRadius;
							if (!Math::FloatEqual(m_oceanGeoParam.relativeMinorRadius, minorRadii))
							{
								m_oceanGeoParam.relativeMinorRadius = minorRadii;
								changed                             = true;
								terrain->m_ocean->setGeomSetting(m_oceanGeoParam);
								auto&& ocean = terrain->m_ocean;
								ocean->SphericalOceanNodeReset(ocean->computeMaxDepth());
								m_oceanGeoParam.maxDepth = ocean->computeMaxDepth();
								ocean->MgrRebuild();
								terrain->updateSeaSurfaceTemperature();
							}
							minorRadii = minorRadii * m_oceanGeoParam.radius;
							ImGui::Text(std::format("{:.2f} 次半径", minorRadii).c_str());
						}
						else if (ImGui::DragFloat(U8("次半径##OceanProperty"), &minorRadii, 0.001f * m_oceanGeoParam.radius, 0.001f * m_oceanGeoParam.radius, 1.0f * m_oceanGeoParam.radius))
						{
							m_oceanGeoParam.relativeMinorRadius = minorRadii / m_oceanGeoParam.radius;
							changed = true;
							auto&& ocean = terrain->m_ocean;
							ocean->setGeomSetting(m_oceanGeoParam);
							ocean->SphericalOceanNodeReset(ocean->computeMaxDepth());
							m_oceanGeoParam.maxDepth = ocean->computeMaxDepth();
							ocean->MgrRebuild();
							terrain->updateSeaSurfaceTemperature();
						}
					}

					if (followRadius)
					{
						float radius = m_shapeType == PTS_Torus ? m_torus.radius : m_sphere.radius;
						if (m_oceanGeoParam.radius != radius)
						{
							changed = true;
							m_oceanGeoParam.radius = radius;
							terrain->m_ocean->setGeomSetting(m_oceanGeoParam);
							auto&& ocean = terrain->m_ocean;
							ocean->SphericalOceanNodeReset(ocean->computeMaxDepth());
							m_oceanGeoParam.maxDepth = ocean->computeMaxDepth();
							ocean->MgrRebuild();
							terrain->updateSeaSurfaceTemperature();
						}
					}

					// 设置球面海洋的半径，网格密度, 以及球面波信息
					if (ImGui::TreeNode(U8("几何参数")))
					{
						if (ImGui::TreeNode(U8("Wave_1")))
						{
							changed |= ImGui::DragFloat(U8("振幅##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[0].amp, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("锐度##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[0].sharp, 0.01f, 0.0f, 1.0f);

							changed |= ImGui::DragFloat(U8("频率##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[0].freq, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("相位##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[0].phase, 0.01f, 0.0f, 50.0f);

							changed |= ImGui::DragFloat3(U8("X,Y,Z##OceanProperty"), m_oceanGeoParam.sphereWaveInfo[0].dir.ptr(), 0.01f, -1.0f, 1.0f);
							ImGui::TreePop();
						}


						if (ImGui::TreeNode(U8("Wave_2")))
						{
							changed |= ImGui::DragFloat(U8("振幅##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[1].amp, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("锐度##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[1].sharp, 0.01f, 0.0f, 1.0f);

							changed |= ImGui::DragFloat(U8("频率##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[1].freq, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("相位##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[1].phase, 0.01f, 0.0f, 50.0f);

							changed |= ImGui::DragFloat3(U8("X,Y,Z##OceanProperty"), m_oceanGeoParam.sphereWaveInfo[1].dir.ptr(), 0.01f, -1.0f, 1.0f);
							ImGui::TreePop();
						}


						if (ImGui::TreeNode(U8("Wave_3")))
						{
							changed |= ImGui::DragFloat(U8("振幅##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[2].amp, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("锐度##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[2].sharp, 0.01f, 0.0f, 1.0f);

							changed |= ImGui::DragFloat(U8("频率##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[2].freq, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("相位##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[2].phase, 0.01f, 0.0f, 50.0f);

							changed |= ImGui::DragFloat3(U8("X,Y,Z##OceanProperty"), m_oceanGeoParam.sphereWaveInfo[2].dir.ptr(), 0.01f, -1.0f, 1.0f);
							ImGui::TreePop();
						}


						if (ImGui::TreeNode(U8("Wave_4")))
						{
							changed |= ImGui::DragFloat(U8("振幅##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[3].amp, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("锐度##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[3].sharp, 0.01f, 0.0f, 1.0f);

							changed |= ImGui::DragFloat(U8("频率##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[3].freq, 0.01f, 0.0f, 20.0f);

							changed |= ImGui::DragFloat(U8("相位##OceanProperty"), &m_oceanGeoParam.sphereWaveInfo[3].phase, 0.01f, 0.0f, 50.0f);

							changed |= ImGui::DragFloat3(U8("X,Y,Z##OceanProperty"), m_oceanGeoParam.sphereWaveInfo[3].dir.ptr(), 0.01f, -1.0f, 1.0f);
							ImGui::TreePop();
						}
						ImGui::TreePop();
					}
					if (changed) {
						terrain->m_ocean->setGeomSetting(m_oceanGeoParam);
						terrainData->sphericalOcean.geomSettings = m_oceanGeoParam;
					}

					if (ImGui::TreeNode(U8("模板参数")))
					{
						bool changedShading = false;
						terrain->m_ocean->getShadeSetting(m_oceanShadeParam);
						changedShading |= ImGui::ColorEdit4(U8("浅海颜色"), m_oceanShadeParam.shallowColor.ptr(), ImGuiInputTextFlags_EnterReturnsTrue);
						changedShading |= ImGui::ColorEdit4(U8("深海颜色"), m_oceanShadeParam.deepColor.ptr(), ImGuiInputTextFlags_EnterReturnsTrue);
						changedShading |= ImGui::ColorEdit4(U8("泡沫颜色"), m_oceanShadeParam.m_foamColor.ptr(), ImGuiInputTextFlags_EnterReturnsTrue);
						changedShading |= ImGui::ColorEdit4(U8("反射颜色"), m_oceanShadeParam.m_reflectColor.ptr(), ImGuiInputTextFlags_EnterReturnsTrue);

						changedShading |= ImGui::DragFloat(U8("菲涅尔偏移##OceanProperty"), &m_oceanShadeParam.fresnelBias, 0.01f, 0.0f, 1.0f);

						changedShading |= ImGui::DragFloat(U8("菲涅尔强度##OceanProperty"), &m_oceanShadeParam.fresnelIntensity, 0.1f, 0.0f, 15.0f);

						changedShading |= ImGui::DragFloat(U8("菲涅尔系数##OceanProperty"), &m_oceanShadeParam.fresnelRatio, 0.1f, 0.0f, 15.0f);

						changedShading |= ImGui::DragFloat2(U8("纹理比例:X左,Y右##OceanProperty"), m_oceanShadeParam.textureScale.ptr(), 0.001f, 0.0f, 10000.0f);

						changedShading |= ImGui::DragFloat(U8("反射颜色系数##OceanProperty"), &m_oceanShadeParam.reflectionAmount, 0.1f, 0.0f, 100.0f);

						changedShading |= ImGui::DragFloat(U8("水颜色系数##OceanProperty"), &m_oceanShadeParam.waveAmount, 0.01f, 0.0f, 1000.0f);

						// changedShading |= ImGui::DragFloat(U8("微波占比##OceanBumpScale"), &m_oceanShadeParam.bumpScale, 0.001f, 0.0f, 10000.0f);

						changedShading |= ImGui::DragFloat(U8("风速##OceanFlowSpeed"), &m_oceanShadeParam.flowSpeed, 0.001f, 0.0f, 10000.0f);

						changedShading |= ImGui::DragFloat(U8("折射扭曲强度##RefractAmount"), &m_oceanShadeParam.refractAmount, 0.001f, 0.0f, 10000.0f);

						changedShading |= ImGui::DragFloat(U8("海岸柔边范围##BorderSoft"), &m_oceanShadeParam.borderSoft, 0.001f, 0.0f, 10000.0f);


						if (ImGui::TreeNode(U8("高光")))
						{
							float specularPower = 1.f;
							changedShading |= ImGui::DragFloat(U8("高光能量##SpecularPower"), &m_oceanShadeParam.specularPower, 0.001f, 0.0f, 10000.0f);

							float specularStrength = 1.f;
							changedShading |= ImGui::DragFloat(U8("高光强度##SpecularStrength"), &m_oceanShadeParam.specularStrength, 0.001f, 0.0f, 10000.0f);

							float lightStrengthRatio = 1.f;
							changedShading |= ImGui::DragFloat(U8("光照影响强度(已废弃)##LightStrengthRatio"), &m_oceanShadeParam.lightStrengthRatio, 0.01f, 0.0f, 10000.0f);

							ImGui::TreePop();
						}

						if (ImGui::TreeNode(U8("泡沫")))
						{
							float foamBias = 1.0f;
							changedShading |= ImGui::DragFloat(U8("泡沫范围##FoamBias"), &m_oceanShadeParam.foamBias, 0.001f, 0.0f, 10000.0f);


							float foamSoft = 1.0f;
							changedShading |= ImGui::DragFloat(U8("泡沫柔边##FoamSoft"), &m_oceanShadeParam.foamSoft, 0.001f, 0.0f, 10000.0f);

							float foamTilling = 1.0f;
							changedShading |= ImGui::DragFloat(U8("泡沫大小##FoamTilling"), &m_oceanShadeParam.foamTilling, 0.001f, 0.0f, 10000.0f);

							ImGui::TreePop();
						}


						if (changedShading) {
							terrain->m_ocean->setShadeSetting(m_oceanShadeParam);
							terrainData->sphericalOcean.shadeSettings = m_oceanShadeParam;
						}

						/*
						if (ImGui::TreeNode(U8("水下雾效")))
						{
							float underWaterFogDensity = 1.0f;
							ImGui::DragFloat(U8("水下雾效浓度##UnderWaterFogDensity"), &underWaterFogDensity, 0.001f, 0.0f, 10000.0f);

							float underWaterFogStart = 1.0f;
							ImGui::DragFloat(U8("水下雾开始距离##UnderWaterFogStart"), &underWaterFogStart, 0.001f, 0.0f, 10000.0f);

							float underWaterFogEnd = 1.0f;
							ImGui::DragFloat(U8("水下雾结束距离##UnderWaterFogEnd"), &underWaterFogEnd, 0.001f, 0.0f, 10000.0f);
							ImGui::TreePop();
						}
						*/
						ImGui::TreePop();
					}
				}
				
					
			}

            bool& distributionChanged = terrainMgr->DistributionChanged;
            
			if (ImGui::BeginChild("温度图参数", ImVec2(uiSize.x, 250), false))
			{
				ImGui::Separator();
				ImGui::Text(U8("温度图参数："));
				distributionChanged |= ImGui::SliderFloat("温度范围：高##T", &m_TParam.clamp.max, m_TParam.clamp.min, 1.0f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.clamp.max, m_TParam.clamp.min,1.0f, 0.05f);
				m_TParam.clamp.max = std::clamp(m_TParam.clamp.max, m_TParam.clamp.min, 1.0f);

				distributionChanged |= ImGui::SliderFloat("温度范围：低##T", &m_TParam.clamp.min, 0.0f, m_TParam.clamp.max, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.clamp.min, 0.0f, m_TParam.clamp.max, 0.05f);
				m_TParam.clamp.min = std::clamp(m_TParam.clamp.min, 0.0f, m_TParam.clamp.max);

				distributionChanged |= ImGui::SliderInt("亮度##T", &m_TParam.brightness, -150, 150, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.brightness, -150, 150, 1);

				distributionChanged |= ImGui::Checkbox("反相##T", &m_TParam.invert);

				distributionChanged |= ImGui::SliderFloat("曝光度##T", &m_TParam.exposure, -20.0f, 20.0f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.exposure, -20.0f, 20.0f, 0.1f);

				distributionChanged |= ImGui::SliderFloat("位移##T", &m_TParam.exposureOffset, -0.5f, 0.5f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.exposureOffset, -0.5f, 0.5f, 0.01f);

				distributionChanged |= ImGui::SliderFloat("灰度系数校正##T", &m_TParam.exposureGamma, 0.01f, 9.99f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_TParam.exposureGamma, 0.01f, 9.99f,0.1f);
				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				/*if (ImGui::Button(U8("点击保存")))
				{
					memcpy(&pMgr->m_temperatureIP, &m_TParam, sizeof(ImageProcess));					
					pMgr->loadTemperatureClimateProcess();
					m_needUpdateImage = true;
				}
				ImGui::SameLine();*/
				if (ImGui::Button(U8("重置")))
				{
					ImageProcess tmp;
					memcpy(&m_TParam, &tmp, sizeof(ImageProcess));
					terrain->loadTemperatureClimateProcess(m_TParam);

                    distributionChanged |= true;
				}
			}
			ImGui::EndChild();			
			
			if (ImGui::BeginChild("湿度图参数", ImVec2(uiSize.x, 290), false))
			{
				ImGui::Separator();
				ImGui::Text(U8("湿度图参数："));
				distributionChanged |= ImGui::SliderFloat(U8("山体影响半径"), &m_AoParam.radius,0.0015f, 0.2f, "%.5f");
				distributionChanged |= HoverAndWheelControl(&m_AoParam.radius, 0.0001f, 0.2f, 0.00001f);

				distributionChanged |= ImGui::SliderFloat(U8("山体影响强度"), &m_AoParam.amplitude,0, 10000, "%.1f");
				distributionChanged |= HoverAndWheelControl(&m_AoParam.amplitude, 0.0f,100000.0f, 1.0f);

				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				distributionChanged |= ImGui::SliderFloat("湿度范围：高##RH", &m_RHParam.clamp.max, m_TParam.clamp.min, 1.0f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.clamp.max, m_TParam.clamp.min, 1.0f, 0.05f);
				m_TParam.clamp.max = std::clamp(m_TParam.clamp.max, m_TParam.clamp.min, 1.0f);

				distributionChanged |= ImGui::SliderFloat("湿度范围：低##RH", &m_RHParam.clamp.min, 0.0f, m_RHParam.clamp.max, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.clamp.min, 0.0f, m_RHParam.clamp.max, 0.05f);
				m_RHParam.clamp.min = std::clamp(m_RHParam.clamp.min, 0.0f, m_RHParam.clamp.max);

				distributionChanged |= ImGui::SliderInt("亮度##RH", &m_RHParam.brightness, -150, 150, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.brightness, -150, 150, 1);

				distributionChanged |= ImGui::Checkbox("反相##RH", &m_RHParam.invert);

				distributionChanged |= ImGui::SliderFloat("曝光度##RH", &m_RHParam.exposure, -20.0f, 20.0f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.exposure, -20.0f, 20.0f, 0.1f);

				distributionChanged |= ImGui::SliderFloat("位移##RH", &m_RHParam.exposureOffset, -0.5f, 0.5f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.exposureOffset, -0.5f, 0.5f, 0.01f);

				distributionChanged |= ImGui::SliderFloat("灰度系数校正##RH", &m_RHParam.exposureGamma, 0.01f, 9.99f, "%.2f");
				distributionChanged |= HoverAndWheelControl(&m_RHParam.exposureGamma, 0.01f, 9.99f, 0.1f);

				ImGui::Dummy(ImVec2(0.0f, 10.0f));
				/*if (ImGui::Button(U8("点击保存")))
				{					
					memcpy(&pMgr->m_humidityIP, &m_RHParam, sizeof(ImageProcess));
					pMgr->loadHumidityClimateProcess();
					m_needUpdateImage = true;
				}
				ImGui::SameLine();*/
				if (ImGui::Button(U8("重置")))
				{
					ImageProcess tmp;
					memcpy(&m_RHParam, &tmp, sizeof(ImageProcess));
					AoParameter tmpAo;
					memcpy(&m_AoParam, &tmpAo, sizeof(AoParameter));
					terrain->loadHumidityClimateProcess(m_RHParam, m_AoParam);

                    distributionChanged |= true;
				}
			}
			ImGui::EndChild();

			//星球远景云模板id
			ShowPlanetCloudUI(terrain,uiSize);
		}
		ImGui::EndChild();
		ImGui::SameLine();
		if (ImGui::BeginChild("GeneratePanel", ImVec2(600, uiSize.y), false, ImGuiWindowFlags_NoScrollWithMouse))
		{
			if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem(U8("生成")))
				{					
					ImVec2 GeneratePSize = ImGui::GetContentRegionAvail();
					if (ImGui::BeginChild("GenerateParam", GeneratePSize, false))
					{
						// constexpr int steps = 50;
						// static std::vector<float> sharpnessArea = NormalDistribution(m_genParam.sharpnessRange.x, std::abs(m_genParam.sharpnessRange.y)).Area(-1, 1, steps);
						// static float sharpnessAreaMax = *std::ranges::max_element(sharpnessArea);
						// static std::vector<float> slopeErosionArea = NormalDistribution(m_genParam.slopeErosionRange.x, std::abs( m_genParam.slopeErosionRange.y)).Area(0, 1, steps);
						// static float slopeErosionAreaMax = *std::ranges::max_element(slopeErosionArea);
						// static std::vector<float> perturbArea = NormalDistribution(m_genParam.perturbRange.x, std::abs(m_genParam.perturbRange.y)).Area(-0.5, 0.5, steps);
						// static float perturbAreaMax = *std::ranges::max_element(perturbArea);

						static std::vector<float> horizon = TrigonometricUberNoise1D(m_genParam).Elevation(-Math::PI / 2, Math::PI / 2, 200);
						ImGui::PlotLines(U8("\n\n\n\n\n\n\n 0 m"), horizon.data(), static_cast<int>(horizon.size()), 0, U8("海拔分布"),
							-2.0f, 2.0f, ImVec2(500, 200));

						bool generate = false;
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						const bool randAll = ImGui::Button(U8("全部随机"));
						generate |= randAll;
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::TextWrapped(U8("随机生成下列4个种子，改变种子会生产完全不同分布的噪声。"));
						if (ImGui::Button(U8("随机地形构型布局")) || randAll)
						{
							generate = true;
							m_genParam.featureNoiseSeed = Vector3(Math::SymmetricRandom(), Math::SymmetricRandom(), Math::SymmetricRandom());
							m_genParam.featureNoiseSeed = FastFloor(m_genParam.featureNoiseSeed * 289.0f + 0.5f);
						}
						ImGui::SameLine();
						if (ImGui::Button(U8("随机地形锐度布局")) || randAll)
						{
							generate = true;
							m_genParam.sharpnessNoiseSeed = Vector3(Math::SymmetricRandom(), Math::SymmetricRandom(), Math::SymmetricRandom());
							m_genParam.sharpnessNoiseSeed = FastFloor(m_genParam.sharpnessNoiseSeed * 289.0f + 0.5f);
						}
						ImGui::SameLine();
						if (ImGui::Button(U8("随机扁平布局")) || randAll)
						{
							generate = true;
							m_genParam.slopeErosionNoiseSeed = Vector3(Math::SymmetricRandom(), Math::SymmetricRandom(), Math::SymmetricRandom());
							m_genParam.slopeErosionNoiseSeed = FastFloor(m_genParam.slopeErosionNoiseSeed * 289.0f + 0.5f);
						}
						ImGui::SameLine();
						if (ImGui::Button(U8("随机扭曲布局")) || randAll)
						{
							generate = true;
							m_genParam.perturbNoiseSeed = Vector3(Math::SymmetricRandom(), Math::SymmetricRandom(), Math::SymmetricRandom());
							m_genParam.perturbNoiseSeed = FastFloor(m_genParam.perturbNoiseSeed * 289.0f + 0.5f);
						}
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						float freqMax = 4.0f;
						if (m_shapeType == PTS_Torus)
						{
							freqMax = 16.0;
						}
						generate |= ImGui::SliderFloat(U8("基础频率"), &m_genParam.baseFrequency, 0.01f, freqMax, "%.4f");
						generate |= HoverAndWheelControl(&m_genParam.baseFrequency, 0.01f, freqMax, 0.001f);
						ImGui::SameLine(); HelpMarker(U8("增加或降低地形的平铺频率。"));

						generate |= ImGui::SliderFloat(U8("频率级数"), &m_genParam.lacunarity, 1.5f, 2.5f, "%.4f");
						generate |= HoverAndWheelControl(&m_genParam.lacunarity, 1.5f, 2.5f, 0.001f);
						ImGui::SameLine(); HelpMarker(U8("对地形平铺频率进行二次叠加影响。"));

						generate |= ImGui::SliderFloat(U8("基础强度"), &m_genParam.baseAmplitude,0.01f, 4.0f, "%.4f");
						generate |= HoverAndWheelControl(&m_genParam.baseAmplitude, 0.01f, 4.0f, 0.01f);

						generate |= ImGui::SliderFloat(U8("细节强度"), &m_genParam.detailAmplitude, 0.01f, 4.0f, "%.4f");
						generate |= HoverAndWheelControl(&m_genParam.detailAmplitude, 0.01f, 4.0f, 0.01f);

						if (developerMode)
						{
							generate |= ImGui::SliderInt(U8("叠加层数"), &m_genParam.baseOctaves, 0, 16);
						}

						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						if (m_shapeType == PTS_Sphere && m_sphere.gempType != -1)
						{
							generate |= ImGui::DragFloat(U8("边界值"), &m_genParam.terrainMinBorder, 0.001f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
							generate |= ImGui::DragFloat(U8("边界过渡"), &m_genParam.terrainBorderSize, 0.001f, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
							generate |= ImGui::DragFloat(U8("边界海拔"), &m_genParam.terrainBorderHeight, 1e-3f, -1.5f, 1.5f, "%.3f");
						}

						ImGui::Dummy(ImVec2(0.0f, 30.0f));

						ImGui::Text(U8("山丘")); ImGui::SameLine();
						generate |= ImGui::SliderFloat(U8("山峰"), &m_genParam.sharpnessRange.x, -2.0f,2.0f); 
						generate |= HoverAndWheelControl(&m_genParam.sharpnessRange.x, -2.0f, 2.0f, 0.001f);
						ImGui::SameLine(); ImGui::Text(U8("尖锐均值"));

						static bool showSharpnessRange = false;
						if (showSharpnessRange)
						{
							generate |= ImGui::SliderFloat(U8("##随机尖锐区域"), &m_genParam.sharpnessRange.y, 0.0f, 2.0f);
							generate |= HoverAndWheelControl(&m_genParam.sharpnessRange.y, 0.0f, 2.0f, 0.001f);
							ImGui::SameLine(); 
						}
						ImGui::Checkbox("随机尖锐区域", &showSharpnessRange);
						ImGui::SameLine(); HelpMarker(U8("对地形上山峰和山丘保持在一定比例平均分布"));

						// generate |= ImGui::Checkbox(U8("随机尖锐反转"), &m_genParam.revertSharpness);

						static bool showSharpnessFrequency = false;
						if (showSharpnessFrequency)
						{
							generate |= ImGui::SliderFloat(U8("##尖锐频率"), &m_genParam.sharpnessFrequency, 0.01f, freqMax, "%.4f");
							generate |= HoverAndWheelControl(&m_genParam.sharpnessFrequency, 0.01f, freqMax, 0.001f);
							ImGui::SameLine(); 
						}
						ImGui::Checkbox("尖锐频率", &showSharpnessFrequency);
						ImGui::SameLine(); HelpMarker(U8("较大的尖锐度频率值会让山丘-山峰的间隔变化更频繁。"));

						ImGui::Dummy(ImVec2(0.0f, 30.0f));

						generate |= ImGui::SliderFloat(U8("扁平均值"), &m_genParam.slopeErosionRange.x, 0.0f, 1.0f);
						generate |= HoverAndWheelControl(&m_genParam.slopeErosionRange.x, 0.0f, 1.0f, 0.001f);

						static bool showSlopeErosionRange = false;
						if (showSlopeErosionRange)
						{
							generate |= ImGui::SliderFloat(U8("##随机扁平区域"), &m_genParam.slopeErosionRange.y, 0.0f, 5.0f);
							generate |= HoverAndWheelControl(&m_genParam.slopeErosionRange.y, 0.0f, 5.0f, 0.001f);
							ImGui::SameLine();
						}
						ImGui::Checkbox(U8("随机扁平区域"), &showSlopeErosionRange);
						ImGui::SameLine(); HelpMarker(U8("通过对地形上尖锐处的扁平化，可以把山地地形变成高原。或者把地形中的平原更加平坦。"));

						// generate |= ImGui::Checkbox(U8("随机扁平反转"), &m_genParam.revertSlopeErosion);

						static bool showSlopeErosionFrequency = false;
						if (showSlopeErosionFrequency)
						{
							generate |= ImGui::SliderFloat(U8("##扁平频率"), &m_genParam.slopeErosionFrequency, 0.01f, freqMax, "%.4f");
							generate |= HoverAndWheelControl(&m_genParam.slopeErosionFrequency, 0.01f, freqMax, 0.01f);
							ImGui::SameLine();
						}
						ImGui::Checkbox(U8("扁平频率"), &showSlopeErosionFrequency);
						ImGui::SameLine(); HelpMarker(U8("较大的扁平度频率会让平坦-陡峭的间隔变化更频繁。"));
						ImGui::Dummy(ImVec2(0.0f, 30.0f));

						generate |= ImGui::SliderFloat(U8("扭曲均值"), &m_genParam.perturbRange.x, -0.3f, 0.3f);
						generate |= HoverAndWheelControl(&m_genParam.perturbRange.x, -0.3f, 0.3f, 0.001f);

						static bool showPerturbRange = false;
						if (showPerturbRange)
						{
							generate |= ImGui::SliderFloat("##随机扭曲区域", &m_genParam.perturbRange.y, 0.0f, 0.5f);
							generate |= HoverAndWheelControl(&m_genParam.perturbRange.y, 0.0f, 0.5f, 0.001f);
							ImGui::SameLine(); 
						}
						ImGui::Checkbox(U8("随机扭曲区域"), &showPerturbRange);
						ImGui::SameLine(); HelpMarker(U8("扰动-1方向为沿梯度负方向扭曲地形，扰动+1方向为沿梯度正方向扭曲地形，扰动0为不扭曲。"));

						// generate |= ImGui::Checkbox(U8("随机扭曲度反转"), &m_genParam.revertPerturb);

						static bool showPerturbFrequency = false;
						if (showPerturbFrequency)
						{
							generate |= ImGui::SliderFloat("##扭曲频率", &m_genParam.perturbFrequency, 0.01f, freqMax, "%.4f");
							generate |= HoverAndWheelControl(&m_genParam.perturbFrequency, 0.01f, freqMax, 0.01f);
							ImGui::SameLine();
						}
						ImGui::Checkbox(U8("扭曲频率"), &showPerturbFrequency);
						ImGui::SameLine(); HelpMarker(U8("较大的扭曲频率会让扭曲强度的间隔变化更频繁。"));

						ImGui::Dummy(ImVec2(0.0f, 10.0f));

						if (generate)
						{
							// sharpnessArea = NormalDistribution(m_genParam.sharpnessRange.x, std::abs(m_genParam.sharpnessRange.y)).Area(-1, 1, steps);
							// sharpnessAreaMax = *std::ranges::max_element(sharpnessArea);
							// slopeErosionArea = NormalDistribution(m_genParam.slopeErosionRange.x, std::abs(m_genParam.slopeErosionRange.y)).Area(0, 1, steps);
							// slopeErosionAreaMax = *std::ranges::max_element(slopeErosionArea);
							// perturbArea = NormalDistribution(m_genParam.perturbRange.x, std::abs(m_genParam.perturbRange.y)).Area(-0.5, 0.5, steps);
							// perturbAreaMax = *std::ranges::max_element(perturbArea);
							terrainMgr->GeoChanged = true;
							horizon = TrigonometricUberNoise1D(m_genParam).Elevation(-Math::PI / 2, Math::PI / 2, 200);
						
                            Noise3DData noiseData;
                            noiseData.featureNoiseSeed = m_genParam.featureNoiseSeed;
                            noiseData.lacunarity = (m_genParam.lacunarity);
                            noiseData.baseFrequency = (m_genParam.baseFrequency);
                            noiseData.baseAmplitude = (m_genParam.baseAmplitude);
							noiseData.detailAmplitude = m_genParam.detailAmplitude;
                            noiseData.gain = (m_genParam.gain);
                            noiseData.baseOctaves = m_genParam.baseOctaves;
                            noiseData.sharpnessNoiseSeed = (m_genParam.sharpnessNoiseSeed);
                            noiseData.sharpnessRange = Vector2(m_genParam.sharpnessRange.x, (m_genParam.revertSharpness ? -1 : 1) * m_genParam.sharpnessRange.y);
                            noiseData.sharpnessFrequency = (m_genParam.sharpnessFrequency);
                            noiseData.slopeErosionNoiseSeed = (m_genParam.slopeErosionNoiseSeed);
                            noiseData.slopeErosionRange = Vector2(m_genParam.slopeErosionRange.x, (m_genParam.revertSlopeErosion ? -1 : 1) * m_genParam.slopeErosionRange.y);
                            noiseData.slopeErosionFrequency = (m_genParam.slopeErosionFrequency);
                            noiseData.perturbNoiseSeed = (m_genParam.perturbNoiseSeed);
                            noiseData.perturbRange = Vector2(m_genParam.perturbRange.x, (m_genParam.revertPerturb ? -1 : 1) * m_genParam.perturbRange.y);
                            noiseData.perturbFrequency = (m_genParam.perturbFrequency);
							noiseData.terrainMinBorder = m_genParam.terrainMinBorder;
							noiseData.terrainBorderSize = m_genParam.terrainBorderSize;
							noiseData.terrainBorderHeight = m_genParam.terrainBorderHeight;

                            terrain->reload3DNoiseChanged(noiseData);
						}
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("拓印"))
				{
					
					static std::string relativeDirPath = "biome_terrain/StampTerrain";
					std::string dirPath = Root::instance()->getRootResourcePath() + relativeDirPath;
					static std::vector<std::string> paths;
					static std::string template_path;
					static bool isShowAll = false;


					auto freshPaths = [this](const std::string& dirPath, std::vector<std::string>& paths, bool Show)
						{
							std::vector<std::string>().swap(paths);
							StampHeightmapUI::GetAllFormatFiles(dirPath, paths, ".dds");
							if (!isShowAll)
							{
								std::erase_if(paths, [](const std::string& path) {
									auto newpath = path.substr(0, path.find(".dds")) + ".raw";
									return !std::filesystem::exists(newpath);
									});
							}
							for (auto&& s : paths)
							{
								ToRelativePath(s);
							}
						};
					if (paths.empty())
					{
						freshPaths(dirPath, paths, isShowAll);
					}
					ImGui::Text(U8("模板库目录：%s"), relativeDirPath.c_str());
					ImGui::SameLine();
					if (ImGui::Button(U8("打开")))
					{
						if(StampHeightmapUI::SelectDirPath(relativeDirPath))
						{
							dirPath = relativeDirPath;
							std::replace(dirPath.begin(), dirPath.end(), '\\', '/');
							ToRelativePath(relativeDirPath);
							freshPaths(dirPath, paths, isShowAll);
						}
					}
					ImGui::SameLine();
					if (ImGui::Checkbox("##ShowAll", &isShowAll))
					{
						freshPaths(dirPath, paths, isShowAll);
					}
					ImGui::SameLine();
					HelpMarker(U8("选中将显示所有缩略图，否则仅显示具有同名raw文件的缩略图"));
					ImGui::Text(U8("缩略图大小: "));
					ImGui::SameLine();
					static float IBS = 100.0f;
					ImGui::SetNextItemWidth(150.0f);
					ImGui::SliderFloat("##imageSize", &IBS, 60.0f, 150.0f);
					ImVec2 ImageButtonSize(IBS, IBS);

					if (!dirPath.empty())
					{
						if (ImGui::TreeNode("模板缩略图"))
						{
							ImVec2 ImageUiSize = ImGui::GetContentRegionAvail();
							int honriSize = int((ImageUiSize.x + ImGui::GetStyle().ItemSpacing.x) / (IBS + ImGui::GetStyle().ItemSpacing.x)) - 1;
							int verticalSize = honriSize < 0.5f ? 0 : int(paths.size() / honriSize) + 1;

							if (ImGui::BeginChild("Images", ImVec2(ImageUiSize.x, 2 * ImageButtonSize.y), false))
							{
								for (int x = 0; x < verticalSize; x++)
								{
									for (int y = 0; y < honriSize; y++)
									{
										int index = x * honriSize + y;
										if (index >= paths.size())
											break;
										if (ImGui::SelectImageButton(StampHeightmapUI::Pool::getInstance().GetTexInfo(paths[index]).d3D11ResView, ImageButtonSize, template_path== paths[index].substr(0, paths[index].find(".dds")) + ".raw"))
										{
											template_path = paths[index];
											template_path = template_path.substr(0, template_path.find(".dds"));
											template_path += ".raw";
										}
										ImGui::SameLine();
									}
									ImGui::NewLine();
								}
							}
							ImGui::EndChild();
							ImGui::TreePop();
						}

					}


					// ImVec2 WorkingUiSize = ImGui::GetContentRegionAvail();
					//if (ImGui::BeginChild("WorkingSpace", WorkingUiSize, false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
					
						// Stamp Terrain Template
						auto isResolutionValid = [](const String& path)
							{
								auto data = Root::instance()->GetFileDataStream(path.c_str());
								if (data)
								{
									int width = (uint32)sqrt(data->size() / 4);
									delete data;
									if (width == StampTerrain3D::HeightMapDataManager::ForceResolution)
									{
										return true;
									}
								}
								return false;
							};
						ImGui::Text(U8("选中模板: % s"), template_path.c_str());

                        bool& geomChanged = terrainMgr->GeoChanged;

						// Stamp Terrain Instance Create/Delete 
						String str = String(U8("当前实例个数：")/*"Instances Number: "*/) + std::to_string(terrainMgr->StampTerrainInstancesNum());
						ImGui::Text(str.c_str());
						ImGui::SameLine();
						str = String(U8("注意：当前实例个数上限为")) + std::to_string(HeightMapStamp3D::StampTerrainInstancesMax);
						HelpMarker(str.c_str());

						bool created = ImGui::Button(U8("创建")/*"New Instance"*/);
						ImGui::SameLine();
						HelpMarker(U8("基于当前选中的拓印高度图模板创建新实例"));
						if (created)
						{
							bool isValid = isResolutionValid(template_path);
							if (isValid && terrainMgr->StampTerrainInstancesNum() < HeightMapStamp3D::StampTerrainInstancesMax)
							{
								m_StampTerrainData = StampTerrainData();
								m_StampTerrainData.templateID = template_path;
								terrainMgr->createStampTerrainInstanceChanged(m_StampTerrainData);

                                geomChanged |= true;
							}
							else if(!isValid)
							{
								std::string log = U8("当前拓印高度图的分辨率限制为");
								log += std::to_string(StampTerrain3D::HeightMapDataManager::ForceResolution);
								// log += "!";
								MsgBox(U8("警告"), log.c_str(), true);
								// EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
							}
						}
						ImGui::SameLine();
						bool deleted = ImGui::Button(U8("删除")/*"Delete Instance"*/);
						ImGui::SameLine();
						HelpMarker(U8("基于当前选中的实例索引删除实例"));
						if (deleted && terrainMgr->StampTerrainInstancesNum() > 0 && m_StampTerrainData.InstanceID >= 0 && m_StampTerrainData.InstanceID < terrainMgr->StampTerrainInstancesNum())
						{
                            geomChanged |= true;
							terrainMgr->removeStampTerrainInstanceChanged(m_StampTerrainData.InstanceID);
							if (terrainMgr->StampTerrainInstancesNum() == 0)
							{
								m_StampTerrainData = StampTerrainData();
							}
							else
							{
								if (terrainMgr->StampTerrainInstancesNum() <= m_StampTerrainData.InstanceID)
								{
									m_StampTerrainData.InstanceID = terrainMgr->StampTerrainInstancesNum() - 1;
								}
								terrainMgr->reloadStampTerrainInstanceChanged(m_StampTerrainData);
							}
                            
						}
						ImGui::SameLine();
						static bool bDisplay = false;
						ImGui::Checkbox(U8("显示当前选中实例"), &bDisplay);


						// Stamp Terrain Select
						String combo_preview_value;

						int nRegistered = (int)terrainMgr->StampTerrainInstancesNum();
						if (nRegistered == 0)
						{
							m_StampTerrainData = StampTerrainData();
						}
						else
						{
							if (m_StampTerrainData.InstanceID < 0 || m_StampTerrainData.InstanceID >= nRegistered)
							{
								m_StampTerrainData.InstanceID = 0;
							}
							terrainMgr->reloadStampTerrainInstanceChanged(m_StampTerrainData);
							combo_preview_value = std::to_string(m_StampTerrainData.InstanceID);
						}
						//ImGui::SetNextItemWidth(100);
						str = String(U8("当前实例模板：")) + m_StampTerrainData.templateID;
						ImGui::Text(str.c_str());
						if (ImGui::BeginCombo(U8("当前实例索引"), combo_preview_value.c_str(), 0))
						{
							for (int n = 0; n < nRegistered; n++)
							{
								const bool is_selected = (m_StampTerrainData.InstanceID == n);
								if (ImGui::Selectable(std::to_string(n).c_str(), is_selected))
									m_StampTerrainData.InstanceID = n;

								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						//ImGui::PushItemWidth(100);


						// Stamp Terrain Property Change
						bool generate_ = false;
						ImGui::Text(U8("旋转轴：")); ImGui::SameLine();
						HelpMarker(U8("注意：旋转轴的三分量会自动归一化"));
						generate_ |= ImGui::SliderFloat(U8("X轴"), &m_StampTerrainData.axis.x, -1.0f, 1.0f); // ImGui::SameLine();
						generate_ |= ImGui::SliderFloat(U8("Y轴"), &m_StampTerrainData.axis.y, -1.0f, 1.0f); // ImGui::SameLine();
						generate_ |= ImGui::SliderFloat(U8("Z轴"), &m_StampTerrainData.axis.z, -1.0f, 1.0f); // ImGui::SameLine();

						generate_ |= ImGui::SliderFloat(U8("旋转"), &m_StampTerrainData.angle, 0.0f, 360.0f);
						generate_ |= ImGui::SliderFloat(U8("宽度"), &m_StampTerrainData.width, 0.1f, 2.0f);
						generate_ |= ImGui::SliderFloat(U8("高度"), &m_StampTerrainData.height, 0.1f, 2.0f);
						// generate_ |= ImGui::SliderFloat(U8("scale"), &m_StampTerrainData.scale, 0.0f, 1.0f);
						generate_ |= ImGui::SliderFloat(U8("过渡系数"), &m_StampTerrainData.transition, 0.0f, 0.5f); ImGui::SameLine();
						HelpMarker(U8("边缘过渡区域所占比例"));
						// generate_ |= ImGui::SliderFloat(U8("容忍度"), &m_StampTerrainData.tolerance, 0.0f, 2.0f); ImGui::SameLine();
						// HelpMarker(U8("仅在拓印模式为0和1时有效，用来控制原高度图和拓印高度图非边缘过渡区的阈值"));
						generate_ |= ImGui::SliderFloat(U8("海拔高度系数"), &m_StampTerrainData.verticalScale, 0.0f, 10.0f);
						generate_ |= ImGui::SliderFloat(U8("海拔高度位移"), &m_StampTerrainData.verticalTrans, -10.0f, 10.0f);
						// generate_ |= ImGui::SliderInt(U8("拓印模式"), &m_StampTerrainData.behavior, 0, 2); ImGui::SameLine();
						// HelpMarker(U8("0. 取较小值 1. 取较大值 2. 取拓印高度图的值"));
						if (generate_)
						{
							terrainMgr->reloadStampTerrainChanged(m_StampTerrainData);
							terrainMgr->GeoChanged = true;
						}

						Matrix4 mtx;
						if (bDisplay && m_StampTerrainData.InstanceID >= 0 && m_StampTerrainData.InstanceID < terrainMgr->StampTerrainInstancesNum() && terrainMgr->getCurrentInstanceMatrix(m_StampTerrainData.InstanceID, mtx))
						{
							terrain->updateStampTerrainDisplay(true, mtx);
						}
						else
						{
							terrain->updateStampTerrainDisplay(false, Matrix4::IDENTITY);
						}
					

					//ImGui::EndChild();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("区域相关"))
				{
					ImGui::Separator();

					ImGui::Text(U8("随机区域索引查询："));
					static float coord[3] = { 0.0 };
					static int idx = -1;
					if (ImGui::InputFloat3(U8("坐标"), coord))
					{
						idx = terrainMgr->getRegionIndex(coord);
					}
					ImGui::SameLine();
					ImGui::Text("区域：%d", idx);
					if( !Root::instance()->m_useSphericalVoronoiRegion ) {}
					else
					{
						ImGui::Separator();
						// Select ParamsRef
						ImGui::Text(U8("维诺图区域："));
						static PlanetRegionParameter ParamsRef;
						int NumRegions = ParamsRef.x * ParamsRef.y * ParamsRef.z;
						if(m_shapeType == PTS_Sphere)
						{
							ParamsRef.z = 6;
							if (ImGui::BeginCombo(U8("期望区域块数量"), std::to_string(NumRegions).c_str(), 0))
							{
								for (int n = 0; n <= (int)6; n++)
								{
									const bool is_selected = (ParamsRef.x == n);
									if (ImGui::Selectable(std::to_string(ParamsRef.z * n * n).c_str(), is_selected))
										ParamsRef.x = ParamsRef.y = n;

									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
								ImGui::SameLine();
								HelpMarker(U8("0表示任意数量"));
							}
						}
						else
						{
							ParamsRef.z = 1;
							int x = (int)ParamsRef.x, y = (int)ParamsRef.y;
							/*if (ImGui::InputInt("x##voronoi", &x))
							{
								x = std::clamp(x, 0, (y != 0 ? 256 / y : 256));
								ParamsRef.x = x;
							}
							ImGui::SameLine(); HelpMarker(U8("0表示任意数量"));*/

							if (ImGui::InputInt("y##voronoi", &y))
							{
								y = std::clamp(y, 0, SphericalTerrain::BiomeTorusRegionMaxH);
								ParamsRef.x = SphericalTerrain::BiomeTorusRegionRatio * y;
								ParamsRef.y = y;
							}
							ImGui::SameLine();HelpMarker(U8("0表示任意数量"));
						}

						// Generate Region Data according to ParamsRef
						static bool bDisplayCenter = false;
						if (ImGui::Button("随机区域##voronoi"))
						{
							if(terrain)
							{
								auto& Region = terrainData->sphericalVoronoiRegion;

								SphericalVoronoiRegionData region;
								region.prefix = Region.prefix;

								if (region.GenerateFineLevelData(ParamsRef)
									&& region.GenerateCoarseLevelData()
									&& region.SelectCoarseLevelCenter())
								{
									region.Loaded = true;
									Region = region;
									terrain->updateSphericalVoronoiRegionInternal();
									terrain->markSphericalVoronoiRegionCenter(bDisplayCenter);
								}
								
							}

						}

						// Display parameters
						{
							int x = 0, y = 0, z = 0, n = 0;
							const auto& ptr = terrainData->sphericalVoronoiRegion.mData;
							if (!ptr.isNull() && ptr->mData.Params.isParsed)
							{
								const auto& p = ptr->mData.Params;
								x = (int)p.x, y = (int)p.y, z = (int)p.z;
								n = x * y * z;
							}
							ImGui::SameLine();
							ImGui::Text("当前区域个数：%d, 其他参数：x = %d, y = %d, z = %d", n, x, y, z);
						}


						if (ImGui::Checkbox(U8("显示粗略层"), &terrain->bDisplayCoarseLayer))
						{
							const auto& region = terrainData->sphericalVoronoiRegion;
							if (!region.Loaded)terrain->bDisplayCoarseLayer = false;
							terrain->markSphericalVoronoiRegionId(-1);
						}
						ImGui::SameLine();
						if (ImGui::Checkbox(U8("显示区域中心"), &bDisplayCenter))
						{
							terrain->markSphericalVoronoiRegionCenter(bDisplayCenter);
						}
						

					}
					if (Root::instance()->m_bManualPlanetRegion)
					{
						ImGui::Separator();
						ImGui::Text(U8("维诺图生成："));

						static PlanetRegionParameter ParamsRef;
						static bool CustomSeed = false;
						static bool change = false;
						static bool isDirty = false;
						ImGui::Checkbox(U8("自定义种子"), &CustomSeed);

						if (CustomSeed)
						{
							change = true;
							float x = (float)ParamsRef.sx, y = (float)ParamsRef.sy;
							ImGui::InputFloat("x##sphericalvoronoi", &x);
							ImGui::InputFloat("y##sphericalvoronoi", &y);
							ParamsRef.sx = (int)x, ParamsRef.sy = (int)y;
						}
						else
						{
							if (ImGui::Button("随机种子##sphericalvoronoi"))
							{
								change = true;
								Vector2 xy = Vector2(Math::SymmetricRandom(), Math::SymmetricRandom());
								xy = FastFloor(xy * 250.0f + 0.5f);
								ParamsRef.sx = (int)xy.x, ParamsRef.sy = (int)xy.y;
							}
							ImGui::Text("随机种子：%d, %d", ParamsRef.sx, ParamsRef.sy);
						}

						if (m_shapeType == PTS_Sphere)
						{
							int x = (int)ParamsRef.x;
							if (ImGui::InputInt("X##sphericalvoronoi", &x))
							{
								x = Math::Clamp(x, 1, 6);
								change = true;
							}
							ParamsRef.x = ParamsRef.y = (uint32)x;
							ParamsRef.z = 6;
							
						}
						else
						{
							int x = (int)ParamsRef.x, y = (int)ParamsRef.y;
							if (ImGui::InputInt("X##sphericalvoronoi", &x))
							{
								change = true;
								x = Math::Clamp(x, 1, 255);
							}

							if (ImGui::InputInt("Y##sphericalvoronoi", &y))
							{
								change = true;
								y = Math::Clamp(y, 1, 255);
							}
							ParamsRef.x = (uint32)x;
							ParamsRef.y = (uint32)y;
							ParamsRef.z = 1;
						}
						int Total = (int)ParamsRef.x * ParamsRef.y * ParamsRef.z;
						ImGui::SameLine();
						ImGui::Text("区域个数：%d", Total);
						if (Total > 256)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, { 1,0,0,1 });
							ImGui::Text("注意：区域总个数不应该超过256！");
							ImGui::PopStyleColor();
						}
						if (change)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, { 1,0,0,1 });
							ImGui::Text("区域参数有改变，请点击生成查看实际的区域图");
							ImGui::PopStyleColor();
						}
						if (ImGui::Button("刷新所有文件##refreshsphericalvoronoi"))
						{
							RefreshSphericalVoronoiFolder();
						}
						if (Total <= 256 && ImGui::Button("生成##sphericalvoronoi"))
						{
							GeneratePlanetRegion(ParamsRef, true);
							isDirty = true;
							change = false;
						}
						ImGui::SameLine();
						if (ImGui::Button("保存##sphericalvoronoi"))
						{
							if (isDirty)
							{
								SavePlanetRegion();
								isDirty = false;
							}
						}
						
					}

					ImGui::EndTabItem();
					
				}
				if (ImGui::BeginTabItem("Region"))
				{
					auto&& regionDataArray = terrainData->region.biomeArray;
					ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
					if (regionDataArray.size() > 0)
					{
						ImVec2 CanvaseSize = ImGui::GetContentRegionAvail();
						ImGui::BeginChild(U8("CANVASE2"), CanvaseSize, false, WindowFlags | 0);
						{
							for (int i = 0; i < regionDataArray.size(); ++i)
							{
								ImGui::BeginChild(("@defines" + std::to_string(i)).c_str(), ImVec2(CanvaseSize.x, 100));
								std::string nameId = "##Name" + std::to_string(i);
								std::string pathId = "##Path" + std::to_string(i);
								std::string ButtonId = "选择##Button" + std::to_string(i);
								ImGui::Separator(); // Add a separator between items
								// Display name with InputText
								ImGui::Text("Name:");
								ImGui::SameLine();
								ImGui::Text(regionDataArray[i].name.c_str());
								// Display min values with InputInt
								ImGui::SetNextItemWidth(CanvaseSize.x - 100);
								ImGui::InputText(pathId.c_str(), regionDataArray[i].TemplateTOD.data(), pathLength);
								ImGui::SameLine();
								if (ImGui::Button(ButtonId.c_str()))
								{
									regionDataArray[i].TemplateTOD.reserve(pathLength);
									SelectTodPath(regionDataArray[i].TemplateTOD.data());
								}
								ImGui::EndChild();
							}

						}
						ImGui::EndChild();
					}
					ImGui::EndTabItem();
				}

				//if (ImGui::BeginTabItem("大气"))
				//{
				//	static bool atmosphereChanged = false;
				//	atmosphereChanged |= ImGui::DragFloat(U8("大气底部半径"), &m_atmospherePropertyUI.rBottom, 1.f, 0.f, 40000.f);
				//	atmosphereChanged |= ImGui::DragFloat(U8("大气顶部半径"), &m_atmospherePropertyUI.rTop, 1.f, 0.f, 40000.f);

				//	if (atmosphereChanged && terrain)
				//	{
				//		auto& currentTerrainProperty = terrain->m_atmosphereProperty;
				//		currentTerrainProperty.boundary.rTop = m_atmospherePropertyUI.rTop;
				//		currentTerrainProperty.boundary.rBottom = m_atmospherePropertyUI.rBottom;
				//		atmosphereChanged = false;
				//	}
				//	ImGui::EndTabItem();
				//}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();
		MsgListen();
		SaveAsAllTypeGeomtryFilesUI();
	}

	void EditorBiomeTerrainUI::ShowPlanetCloudUI(SphericalTerrain* terrain, const ImVec2& uiSize)
	{
		if (!terrain)
			return;
		if (!EditorBiomeMgr::getInstance().define_ptr)
			return;

		if (ImGui::BeginChild(U8("##云模板库参数"), ImVec2(uiSize.x, 290), false))
		{
			size_t  cloudLibrarySize_ = EditorBiomeMgr::getInstance().define_ptr->m_CloudLibrary.getCloudLibraryData().size();
			ImGui::Text(std::string(U8("远景云当前模板库id范围: -1~") + std::to_string(cloudLibrarySize_ - 1)).c_str());
			ImGui::Text(U8("远景云模板ID:"));
			ImGui::SameLine();
			int32_t cloudId = terrain->getPlanetCloudId();
			if (ImGui::InputInt("##planetCloudId", &cloudId, 0))
				terrain->setPlanetCloudId(cloudId);
			ImGui::SameLine();
			HelpMarker(U8("远景云模板id为-1或者id超出远景云模板库大小时，不显示远景云"));


			if(EditorBiomeMgr::getInstance().define_ptr->m_CloudLibrary.getCloudLibraryData().size())

			if (cloudId != -1 && cloudId < EditorBiomeMgr::getInstance().define_ptr->m_CloudLibrary.getCloudLibraryData().size())
			{
				ImGui::Text("显示远景云");
				ImGui::SameLine();
				bool isShow = terrain->getEnableLowLODCloud();
				if (ImGui::Checkbox("##isShowLowLODCloud", &isShow))
				{
					terrain->setEnableLowLODCloud(isShow);
				}
				ImGui::SameLine();
				HelpMarker(U8("仅为编辑器查看使用，不存入星球文件"));
			}
		}
		ImGui::EndChild();
		
	}

	void EditorBiomeTerrainUI::freshData()
    {
        Noise3DData noiseData = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->get3DNoiseData();
		{
			m_genParam.revertSharpness = noiseData.sharpnessRange.y < 0;
			m_genParam.revertSlopeErosion = noiseData.slopeErosionRange.y < 0;
			m_genParam.revertPerturb = noiseData.perturbRange.y < 0;

			m_genParam.featureNoiseSeed = noiseData.featureNoiseSeed;
			m_genParam.lacunarity = noiseData.lacunarity;
			m_genParam.baseFrequency = noiseData.baseFrequency;
			m_genParam.baseAmplitude = noiseData.baseAmplitude;
			m_genParam.detailAmplitude = noiseData.detailAmplitude;
			m_genParam.gain = noiseData.gain;
			m_genParam.baseOctaves = (int)noiseData.baseOctaves;

			m_genParam.sharpnessNoiseSeed = noiseData.sharpnessNoiseSeed;
			m_genParam.sharpnessRange = Vector2(noiseData.sharpnessRange.x, std::abs(noiseData.sharpnessRange.y));
			m_genParam.sharpnessFrequency = noiseData.sharpnessFrequency;

			m_genParam.slopeErosionNoiseSeed = noiseData.slopeErosionNoiseSeed;
			m_genParam.slopeErosionRange = Vector2(noiseData.slopeErosionRange.x, std::abs(noiseData.slopeErosionRange.y));
			m_genParam.slopeErosionFrequency = noiseData.slopeErosionFrequency;

			m_genParam.perturbNoiseSeed = noiseData.perturbNoiseSeed;
			m_genParam.perturbRange = Vector2(noiseData.perturbRange.x, std::abs(noiseData.perturbRange.y));
			m_genParam.perturbFrequency = noiseData.perturbFrequency;
			m_genParam.terrainMinBorder = noiseData.terrainMinBorder;
			m_genParam.terrainBorderSize = noiseData.terrainBorderSize;
			m_genParam.terrainBorderHeight = noiseData.terrainBorderHeight;
		}

        
        SphericalTerrainData terrainData = {};
        if(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain())
        {
            terrainData = *(SphericalTerrainData*)EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData.getPointer();
        }

        ClimateProcessData& climateProcess = terrainData.distribution.climateProcess;
		memcpy(&m_TParam, &climateProcess.temperatureIP, sizeof(ImageProcess));
		memcpy(&m_RHParam, &climateProcess.humidityIP, sizeof(ImageProcess));
		// TODO : climateProcess.heightRange remain unchanged when mgr->heightRange changed
		// memcpy(&m_heightRange, &climateProcess.heightRange, sizeof(ValueRange));
		memcpy(&m_AoParam, &climateProcess.aoParam, sizeof(AoParameter));
		m_shapeType = terrainData.geometry.topology;
        switch (m_shapeType)
        {
        case PTS_Sphere:
	        m_sphere = terrainData.geometry.sphere;
	        break;
        case PTS_Torus:
	        m_torus = terrainData.geometry.torus;
	        break;
        default: break;
        }
		m_quadtreeMaxDepth = terrainData.geometry.quadtreeMaxDepth;
		// m_heightRangePrev = m_heightRange;

		m_TParam.brightness = std::clamp(m_TParam.brightness, -150, 150);
		m_TParam.exposureGamma = std::clamp(m_TParam.exposureGamma, 0.01f, 9.99f);
		m_TParam.exposure = std::clamp(m_TParam.exposure, -20.0f, 20.0f);
		m_TParam.exposureOffset = std::clamp(m_TParam.exposureOffset, -0.5f, 0.5f);
		m_TParamPrev = m_TParam;


		m_RHParam.brightness = std::clamp(m_RHParam.brightness, -150, 150);
		m_RHParam.exposureGamma = std::clamp(m_RHParam.exposureGamma, 0.01f, 9.99f);
		m_RHParam.exposure = std::clamp(m_RHParam.exposure, -20.0f, 20.0f);
		m_RHParam.exposureOffset = std::clamp(m_RHParam.exposureOffset, -0.5f, 0.5f);
		m_RHParamPrev = m_RHParam;

		m_AoParam.radius = std::clamp(m_AoParam.radius, 0.0f, 0.5f);
		m_AoParam.amplitude = std::clamp(m_AoParam.amplitude, 0.0f, 10000.0f);
		m_AoParamPrev = m_AoParam;

		MarkRGParamDirty();
		
		m_StampTerrainData = StampTerrainData();

        //IMPORTANT(yanghang):Recalculate planet humidity and temperate data.
        auto planet = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain();
        planet->loadTemperatureClimateProcess(m_TParam);
        planet->loadHumidityClimateProcess(m_RHParam, m_AoParam);
    }
    
	void EditorBiomeTerrainUI::InitGui(int class_id)
	{
		if (m_isInit)
			return;

		m_isInit = true;

		mFlags |= ImGuiWindowFlags_MenuBar;
		mUIType = U8("EditorBiomeTerrainUI");
		mUIName = mUIType + "##" + std::to_string(class_id);
		ImGui::SetNextWindowPos(ImVec2(0, 400), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_FirstUseEver);

		if (std::ifstream ifs("edit_sph_terr.txt"); ifs.is_open())
		{
			std::string terr;
			ifs >> terr;
			ifs.close();
			if (exists(std::filesystem::path(Root::instance()->getRootResourcePath() + terr)))
			{
				//EditorBiomeMgr::getInstance().terrain_ptr->m_default_3dProceduralTerrainFilePath = terr;
			}
		}
		EditorBiomeMgr::getInstance().terrain_ptr->reload(true);

        freshData();
        
		// SceneManager* mainSceneManager = Root::instance()->getMainSceneManager();
		// if (mainSceneManager)
		// {
		// 	Camera* pCamera = mainSceneManager->getMainCamera();
		// 	if (pCamera)
		// 		pCamera->setPosition(StringConverter::parseVector3("-4366.48 2742.91 -1309.64", pCamera->getPosition()));
		// }
		std::string TerrainUIreloadFuncName = "EditorBiomeTerrainUI" + std::string("_") + "reLoadTerrain";
		EditorBiomeMgr::getInstance().voidfuncMap[TerrainUIreloadFuncName] = std::bind<void>(&EditorBiomeTerrainUI::reLoadTerrain, this);
	}

	void EditorBiomeTerrainUI::SelectTodPath(char* outPath)
	{
		static std::string dir;
		if (dir.size() < 1)
		{
			dir = Root::instance()->getRootResourcePath() + "tod";
		}

		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "stf文件\0*.stf\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));

		info.initialDir = dir;
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');

		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
		{
			using namespace std::filesystem;
			std::filesystem::path normalized_path = absolute(path);
			std::filesystem::path folder_path = normalized_path.parent_path();
			if (path.empty() || (path.find(".stf") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为stf的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
			}
			else
			{
				dir = folder_path.string();
				std::replace(path.begin(), path.end(), '\\', '/');
				path = path.substr(path.find("tod"));
				memset(outPath, 0, 512);
				memcpy(outPath, path.c_str(), strlen(path.c_str()));
			}
		}
		
	}

	bool EditorBiomeTerrainUI::GeneratePlanetRegion(const PlanetRegionGenerator::Parameter& ParamsRef, bool isR)
	{
		if (!m_RegionGenerator)
		{
			m_RegionGenerator = new PlanetRegionGenerator(PlanetRegionGenerator::Parameter());
		}
		m_RegionGenerator->Reset(ParamsRef);

		if ((ParamsRef.z == 1u && m_RegionGenerator->Torus())
			|| (ParamsRef.z == 6u && m_RegionGenerator->Sphere()))
		{
			if (!isR) return true;
			if (auto terrain = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain())
			{
				if (terrain->m_editingTerrain)
				{
					SphericalTerrain::CreatePlanetRegionTexture(m_RegionGenerator->maps, terrain->m_ManualSphericalVoronoiTex);
					terrain->m_ManualParams = Vector4::ZERO;
					terrain->m_ManualParams.x = static_cast<float>(m_RegionGenerator->params.x);
					terrain->m_ManualParams.y = static_cast<float>(m_RegionGenerator->params.y);
				}
			}
			return true;
		}



		return false;
	}

	bool EditorBiomeTerrainUI::SavePlanetRegionImpl(const String& prefix)
	{
		if (m_RegionGenerator)
		{
			if (!m_RegionGenerator->SaveTo(prefix))
			{
				LogManager::instance()->logMessage("-error- 保存 [" + prefix + "] 失败！", LML_CRITICAL);
				return false;
			}
			return true;
		}
		return false;
	}

	
	void EditorBiomeTerrainUI::RefreshSphericalVoronoiFolder()
	{

		const auto& paths = PlanetRegionRelated::FilePaths;
		for(const auto& path: paths)
		{
			PlanetRegionParameter Params;
			String p = path.substr(0, path.rfind("_["));
			if (PlanetRegionRelated::Parse(path, Params))
			{
				if (GeneratePlanetRegion(Params))
				{
					SavePlanetRegionImpl(p);
				}
			}
		}
	}

	void EditorBiomeTerrainUI::SavePlanetRegion()
	{
		Echo::FileLoadSaveWinInfo info;
		char utf8_test[] = "无后缀文件\0*.\0";
		UTF8ToMultiByte(utf8_test, sizeof(utf8_test));
		memcpy(info.filter, utf8_test, sizeof(utf8_test));
		info.initialDir = Root::instance()->getRootResourcePath() + PlanetRegionRelated::SpherePlanetRegionPath;
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');
		std::string path;
		if (EditorFileToolsManager::instance()->CreateSaveWindow(info, "", path))
		{
			SavePlanetRegionImpl(path);
		}
	}

	std::string EditorBiomeTerrainUI::SelectOceanTexturePath(std::string suffix)
	{
		Echo::FileLoadSaveWinInfo info;

		char ddsPathU8[] = "dds文件\0*.dds";
		UTF8ToMultiByte(ddsPathU8, sizeof(ddsPathU8));

		memcpy(info.filter, ddsPathU8, 15);
		info.initialDir = Root::instance()->getRootResourcePath();
		std::replace(info.initialDir.begin(), info.initialDir.end(), '/', '\\');

		std::string path;
		if (EditorFileToolsManager::instance()->CreateLoadWindow(info, path))
		{
			if (path.empty() || (path.find(".dds") == path.npos))
			{
				std::string log = U8("[警告]: 请输入后缀为 .dds 的资源！");
				EditorUIManager::instance()->InitUIMessageBox(U8("警告！"), log.c_str());
				return "";
			}
			else
			{
				std::replace(path.begin(), path.end(), '\\', '/');
				std::string resRoot = Echo::Root::instance()->getRootResourcePath();
				std::replace(resRoot.begin(), resRoot.end(), '\\', '/');
				size_t pos = path.find(resRoot);
				if (pos != std::string::npos)
					path = path.erase(0, resRoot.size());
				return path;
			}
		}
		return "";
	}

	void EditorBiomeTerrainUI::SaveAsAllTypeGeomtryFilesUI()
	{
		if (!isOpenSaveAsAllTypeGeomtryFilesUI)
			return;
		ImGui::OpenPopup(U8("星球几何文件全形状批量导出"));
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
		if (ImGui::BeginPopupModal(U8("星球几何文件全形状批量导出"), &isOpenSaveAsAllTypeGeomtryFilesUI, flags))
		{
			std::string filePath = EditorBiomeMgr::getInstance().terrain_ptr->getGeoFilePath();
			ImGui::Text("基础文件");
			ImGui::SameLine();
			ImGui::InputText("##基础文件", &filePath, (int)filePath.length(), ImGuiInputTextFlags_ReadOnly);
			static int chicun = 11000;
			if (ImGui::BeginCombo(U8("尺寸"), ("星球尺寸:" + std::to_string(chicun)).c_str()))
			{
				for (auto& [radius, data] : planetGeomtryAllTypeCache)
				{
					if (ImGui::MenuItem(("星球尺寸:" + std::to_string(chicun)).c_str(), "", chicun == radius))
						chicun = radius;
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("计算半径倍率")))
			{
				for (auto& [radius, data] : planetGeomtryAllTypeCache)
					data.multiple = (float)radius / chicun;
			}
			auto& sphere = m_sphere;
			ImGui::Text("半径");
			ImGui::SameLine();
			ImGui::InputFloat("##半径", &sphere.radius, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
			float heightScale = sphere.elevationRatio * sphere.radius;
			ImGui::Text("海拔");
			ImGui::SameLine();
			ImGui::InputFloat("##海拔", &heightScale, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);

			ImGuiTableFlags flag = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Borders
				| ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_ScrollX;
			static std::vector<std::string> tableItem = { U8("尺寸"), U8("非环形地形半径倍率"),U8("环形地形"),U8("多边形边界参数") };
			if (ImGui::BeginTable(UNIQUE_NAME(##星球几何文件全形状批量导出), (int)tableItem.size(), flag/*, ImVec2(0.0f, 6.f * ImGui::GetTextLineHeightWithSpacing())*/))
			{
				for (const std::string& item : tableItem)
					ImGui::TableSetupColumn(item.c_str());
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();

				int index = 0;
				for (auto& [radius, data] : planetGeomtryAllTypeCache)
				{
					for (int column = 0; column < tableItem.size(); column++)
					{
						ImGui::TableSetColumnIndex(column);
						switch (column)
						{
						case 0:
						{
							ImGui::Text(("星球尺寸" + std::to_string(radius)).c_str());
						}
						break;
						case 1:
						{
							ImGui::InputFloat((U8("倍率##半径倍率") + std::to_string(index)).c_str(), &data.multiple);
						}
						break;
						case 2:
						{
							auto& torus = data.torus;
							ImGui::InputFloat((U8("主半径##主半径") + std::to_string(index)).c_str(), &torus.radius);
							torus.radius = std::clamp(torus.radius, 1.0f, 50000.0f);

							float minorRadius = torus.relativeMinorRadius * torus.radius;
							ImGui::InputFloat((U8("次半径##次半径") + std::to_string(index)).c_str(), &minorRadius);
							minorRadius = std::clamp(minorRadius, 1.0f, torus.radius);
							torus.relativeMinorRadius = minorRadius / torus.radius;

							float heightScale = torus.elevationRatio * minorRadius;
							ImGui::InputFloat((U8("环面海拔##环面海拔") + std::to_string(index)).c_str(), &heightScale);
							heightScale = std::clamp(heightScale, 0.0f, minorRadius * 0.5f);
							torus.elevationRatio = heightScale / minorRadius;
						}
						break;
						case 3:
						{
							ImGui::DragFloat((U8("边界值##") + std::to_string(index)).c_str(), &data.terrainMinBorder, 0.001f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
							ImGui::DragFloat((U8("边界过渡##") + std::to_string(index)).c_str(), &data.terrainBorderSize, 0.001f, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
							ImGui::DragFloat((U8("边界海拔##") + std::to_string(index)).c_str(), &data.terrainBorderHeight, 1e-3f, -1.5f, 1.5f, "%.3f");
							ImGui::SameLine();
							if (ImGui::Button((U8("全尺寸应用##") + std::to_string(index)).c_str()))
							{
								for (auto& iter : planetGeomtryAllTypeCache)
								{
									iter.second.terrainMinBorder = data.terrainMinBorder;
									iter.second.terrainBorderSize = data.terrainBorderSize;
									iter.second.terrainBorderHeight = data.terrainBorderHeight;
								}
							}
						}
						break;
						default:
							break;
						}
					}
					ImGui::TableNextRow();
					++index;
				}
				ImGui::EndTable();
			}
			ImGui::Separator();
			if (ImGui::Button(U8("确认")))
			{
				auto start = std::chrono::steady_clock::now();

				isOpenSaveAsAllTypeGeomtryFilesUI = false;
				TerrainGeometry geometryCache = terrainData->geometry;
				std::string filePath = geometryCache.filePath;
				std::replace(filePath.begin(), filePath.end(), '\\', '/');

				for (auto& [radius, data] : planetGeomtryAllTypeCache)
				{
					size_t begin = filePath.npos;
					size_t end = filePath.npos;
					size_t pos = filePath.find_last_of('/');
					if (pos != filePath.npos)
					{
						pos = filePath.find_first_of("_", pos);
						if (pos != filePath.npos)
							begin = filePath.find_first_of("_", pos + 1);
					}
					if (begin != filePath.npos && begin + 1 < filePath.size())
					{
						++begin;
						end = filePath.find_first_of("_", begin);
						if (end != filePath.npos)
						{
							filePath.replace(begin, end - begin, std::to_string(radius));
						}
					}
					if (begin == filePath.npos || end == filePath.npos)
					{
						LogManager::instance()->logMessage("星球几何文件导出源文件路径异常!");
						break;
					}

					auto saveGeometryFile = [&](ProceduralTerrainTopology shapeType, uint32 gempType) {
						std::string geometryFile = filePath;
						std::string shape;
						if (shapeType == PTS_Sphere)
						{
							if (gempType == 1)
								shape = "sj_";
							else if (gempType == 2)
								shape = "zfx_";
							else if (gempType == 3)
								return;// shape = "";
							else if (gempType == 4)
								shape = "8mt_";
							else if (gempType == 5)
								shape = "12mt_";
							else if (gempType == 6)
								shape = "20mt_";
						}
						else if (shapeType == PTS_Torus)
							shape = "hx_";
						geometryFile.insert(begin, shape);

						if (shapeType == PTS_Sphere && gempType != -1)
						{
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainMinBorder = data.terrainMinBorder;
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainBorderSize = data.terrainBorderSize;
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainBorderHeight = data.terrainBorderHeight;
						}
						else
						{
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainMinBorder = geometryCache.noise.terrainMinBorder;
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainBorderSize = geometryCache.noise.terrainBorderSize;
							EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.noise.terrainBorderHeight = geometryCache.noise.terrainBorderHeight;
						}

						EditorBiomeMgr::getInstance().terrain_ptr->setTerrainGeneratorType(gempType);
						EditorBiomeMgr::getInstance().terrain_ptr->setTopology(shapeType);

						if (shapeType == PTS_Sphere)
						{
							EditorBiomeMgr::getInstance().terrain_ptr->setRadiusElevation(geometryCache.sphere.radius * data.multiple, geometryCache.sphere.elevationRatio);
						}
						else if (shapeType == PTS_Torus)
						{
							EditorBiomeMgr::getInstance().terrain_ptr->setRadiusElevation(data.torus.radius, data.torus.relativeMinorRadius, data.torus.elevationRatio);
						}

						EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.filePath = geometryFile;
						EditorBiomeMgr::getInstance().terrain_ptr->reGenBoundBox();
						SphericalTerrainResource::saveTerrainGeometry(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry);
					};

					//环形
					saveGeometryFile(PTS_Torus, -1);
					//球形
					saveGeometryFile(PTS_Sphere, -1);
					//多边形
					const auto& shapeTypeOfSphereMap = TerrainGeneratorLibrary::instance()->getTypes();
					for (const auto& shapeTypeOfSphere : shapeTypeOfSphereMap) {
						saveGeometryFile(PTS_Sphere, shapeTypeOfSphere.first);
					}
				}

				//复原
				{
					EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry = geometryCache;

					TerrainGeneratorLibrary::instance()->remove(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->m_pTerrainGenerator);

					if (EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.topology == PTS_Sphere) {
						EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->m_pTerrainGenerator = TerrainGeneratorLibrary::instance()->add(EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain()->m_terrainData->geometry.sphere.gempType);
					}

					auto planet = EditorBiomeMgr::getInstance().terrain_ptr->GetSphericalTerrain();
					planet->shutDown();
					planet->createImpl(planet->m_terrainData);
					planet->bindTextures();

					freshData();
				}

				auto end = std::chrono::steady_clock::now();
				auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				LogManager::instance()->logMessage("星球几何文件全形状批量导出用时：" + std::to_string(duration_ms) + " ms");

				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			ImGui::SameLine();
			if (ImGui::Button(U8("取消")))
			{
				isOpenSaveAsAllTypeGeomtryFilesUI = false;
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			ImGui::EndPopup();
		}
	}

#define OUTPUT_TEST_IMAGE 0
    
#if OUTPUT_TEST_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif
    
    static void logToConsole(const String& log)
    {
#ifdef _WIN32
        OutputDebugString(log.c_str());
#else
        LogManager::instance()->logMessage(log.c_str());
#endif
    }
    
    void generateCoarseRegionMapAndSDF()
    {
        //NOTE:Voronoi type count and count of each type.
        const int Voronoi_Type_Count = 6;
        const int Voronoi_Count = 4;
        struct VoronoiTypeMap
        {
            String names[Voronoi_Count];
        };

        //IMPORTANT(yanghang):If modify sphericalVoronoiMap on disk,
        // This must be followed.
        VoronoiTypeMap allMaps[Voronoi_Type_Count] =
            {
                {"1_[1x0x0]", "2_[1x141x169]", "3_[1x179x-97]", "4_[1x135x81]"},
                {"5_[2x135x81]", "6_[2x-165x19]", "7_[2x-131x-156]", "8_[2x-240x-208]"},
                {"9_[3x-135x208]", "10_[3x-179x-247]", "11_[3x-165x3]", "12_[3x-136x-149]"},
                {"13_[4x25x92]", "14_[4x22x-113]", "15_[4x-150x85]", "16_[4x111x-73]"},
                {"17_[5x42x245]", "18_[5x-185x-213]", "19_[5x-227x179]", "20_[5x-39x133]"},
                {"21_[6x177x153]", 	"22_[6x125x-163]", "23_[6x-153x-31]", "24_[6x18x-20]"	},
            };

        String filePath = "echo/biome_terrain/spherical_voronoi_map/";
        struct GenerateRegionInfo
        {
            int type = 0;
            String fineRegionName;
            String coarseRegionName;
            std::vector<int> parentLookupTable;
            std::vector<Vector3> fineRegionCenters;
        };

        std::vector<GenerateRegionInfo> infoArray;
        
        for(int typeIndex = 0;
            typeIndex < Voronoi_Type_Count;
            typeIndex++)
        {
            VoronoiTypeMap& currentLevelMap = allMaps[typeIndex];
            //Level 0 doesn't have coarse region map.
            if(typeIndex == 0)
            {
                for(int currentLevelIndex = 0;
                    currentLevelIndex < Voronoi_Count;
                    currentLevelIndex++)
                {
                    GenerateRegionInfo info;
                    info.type = typeIndex + 1;
                    info.fineRegionName = currentLevelMap.names[currentLevelIndex];
                    info.coarseRegionName = "white_coarse_map";
                    infoArray.push_back(info);
                }
                
                continue;
            }
            VoronoiTypeMap& lowLevelMap = allMaps[typeIndex - 1];
            
            for(int currentLevelIndex = 0;
                currentLevelIndex < Voronoi_Count;
                currentLevelIndex++)
            {
                GenerateRegionInfo info;
                info.fineRegionName = currentLevelMap.names[currentLevelIndex];
                
                for(int lowLevelIndex = 0;
                    lowLevelIndex < Voronoi_Count;
                    lowLevelIndex++)
                {
                    info.type = typeIndex + 1;
                    info.coarseRegionName = lowLevelMap.names[lowLevelIndex];
                    infoArray.push_back(info);
                }   
            }
        }

        for(auto& info : infoArray)
        {
            String Path = filePath + info.fineRegionName + ".bin";
            Echo::DataStreamPtr File(Root::instance()->GetFileDataStream(Path.c_str()));
            if (!File.isNull())
            {
                int length = (int)File->size();
                int n = info.type;
                if(length > 0)
                {
                    if (length % sizeof(Vector3) != 0 || length / sizeof(Vector3) != 6 * n * n)
                    {
                        logToConsole("-error-\t " + Path + " file is not successfully parsed!");
                    }
                    else
                    {
                        info.fineRegionCenters.resize(length / sizeof(Vector3));
                        File->read((char*)info.fineRegionCenters.data(), length);

                        if(info.fineRegionCenters.empty())
                        {
                            logToConsole(info.fineRegionName + "'s region centers size is 0!");
                        }
                    }
                }
                else
                {
                    logToConsole(info.fineRegionName + "'s region centers file is empty!");
                }
            }
            else
            {
                logToConsole(info.fineRegionName + "'s region centers file doesn't find!");
            }

            BitmapCube fineCube = {};
            for(int i = 0;
                i < 6;
                i++)
            {
                Bitmap& fineMap = fineCube.maps[i];
                fineMap.width = 512;
                fineMap.height = 512;
                fineMap.bytesPerPixel = 1;
                fineMap.pitch = fineMap.width * fineMap.bytesPerPixel;
                fineMap.pixels = malloc(fineMap.pitch * fineMap.height);
                        
                String fineMapName = filePath + info.fineRegionName +
                    +"_" + std::to_string(i) + ".map";
                File = Root::instance()->GetFileDataStream(fineMapName.c_str());
                if(File->getData())
                {
                    memcpy(fineMap.pixels,
                           File->getData(),
                           fineMap.pitch * fineMap.height);
                }
                else
                {
                    logToConsole("Fine region map:" + fineMapName + String("can't open file!"));
                }
            }
            
            std::vector<int> parentLookupTable;
            std::array<Bitmap, 6> coarseCube;
            
            if(info.type == 1)
            {
                //IMPORTANT:Level 0 doesn't have coarse map. so their parent are the same
                // one.
                //NOTE: ONLY generate fine region sdf.
                //NOTE: Six region is one big coarse region.
                parentLookupTable.push_back(0);
                parentLookupTable.push_back(0);
                parentLookupTable.push_back(0);
                parentLookupTable.push_back(0);
                parentLookupTable.push_back(0);
                parentLookupTable.push_back(0);
            }
            else
            {
                for(int i = 0;
                    i < 6;
                    i++)
                {
                    //IMPORTANT:Level 0 doesn't have coarse map.
                    Bitmap& coarseMap = coarseCube[i];
                    coarseMap.width = 512;
                    coarseMap.height = 512;
                    coarseMap.bytesPerPixel = 1;
                    coarseMap.pitch = coarseMap.width * coarseMap.bytesPerPixel;
                    coarseMap.pixels = malloc(coarseMap.pitch * coarseMap.height);
                        
                    String coarseMapName = filePath + info.coarseRegionName +
                        +"_" + std::to_string(i) + ".map";
                    Echo::DataStreamPtr File(Root::instance()->GetFileDataStream(coarseMapName.c_str()));
                    if(File->getData())
                    {
                        memcpy(coarseMap.pixels,
                               File->getData(),
                               coarseMap.pitch * coarseMap.height);
                    }
                    else
                    {
                        logToConsole("Coarse region map:" + coarseMapName + String("can't open file!"));
                    }
                }

                //IMPORTANT:Level 0 doesn't have coarse map.
                for(auto& center : info.fineRegionCenters)
                {
                    int parent =
                        SphericalTerrain::computeSphericalRegionParentId(center, coarseCube);
                    parentLookupTable.push_back(parent);
                }
            }

            GenerateRegionSDFResult generateResult = 
                GeneratePlanetRegionSDF(fineCube,
                                        parentLookupTable);

            BitmapCube& sdfCube = generateResult.sdfCube;
            BitmapCube& coarseSDFCube = generateResult.coarseSDFCube;
                
            for(int i = 0;
                i < 6;
                i++)
            {
                Bitmap& sdfMap = sdfCube.maps[i];
                Bitmap& coarseSDFMap = coarseSDFCube.maps[i];
                        
                String coarseSDFMapName = filePath + "coarse_" + info.fineRegionName +
                    +"_" + info.coarseRegionName +
                    +"_" + std::to_string(i) + ".sdf";

                if(info.type == 1)
                {
                    coarseSDFMapName = filePath + "coarse_global_white.sdf";
                }
                    
                String fineSDFMapName = filePath + "fine" + info.fineRegionName
                    +"_" + std::to_string(i) + ".sdf";

                DataStream* data_stream = 0;
                data_stream = Root::instance()->CreateDataStream(fineSDFMapName.c_str());
                if(data_stream)
                { 
                    assert(data_stream);
                    FileDataStream* file_stream = (FileDataStream *)data_stream;
                    file_stream->write(sdfMap.pixels,
                                       sdfMap.pitch * sdfMap.height);
                    SAFE_DELETE(file_stream);
                }

                data_stream = Root::instance()->CreateDataStream(coarseSDFMapName.c_str());
                if(data_stream)
                { 
                    assert(data_stream);
                    FileDataStream* file_stream = (FileDataStream *)data_stream;
                    file_stream->write(coarseSDFMap.pixels,
                                       coarseSDFMap.pitch * coarseSDFMap.height);
                    SAFE_DELETE(file_stream);
                }


#if OUTPUT_TEST_IMAGE
                String newFineFileName = fineSDFMapName.substr(0, fineSDFMapName.find(".sdf")) + ".png";
                int error = stbi_write_png(newFineFileName.c_str(),
                                           sdfMap.width,
                                           sdfMap.height,
                                           1,
                                           sdfMap.pixels,
                                           sdfMap.pitch);
                if(error == 0)
                {
                    logToConsole(fineSDFMapName + "output to PNG file fail!");
                }


                String newCoarseFileName = coarseSDFMapName.substr(0, coarseSDFMapName.find(".sdf")) + ".png";
                error = stbi_write_png(newCoarseFileName.c_str(),
                                       coarseSDFMap.width,
                                       coarseSDFMap.height,
                                       1,
                                       coarseSDFMap.pixels,
                                       coarseSDFMap.pitch);
                if(error == 0)
                {
                    logToConsole(coarseSDFMapName + "output to PNG file fail!");
                }
#endif
            }
            
            for(int i = 0;
                i < 6;
                i++)
            {
                Bitmap& coarseMap = coarseCube[i];
                coarseMap.freeMemory();

                Bitmap& fineMap = fineCube.maps[i];
                fineMap.freeMemory();

                Bitmap& sdfMap = sdfCube.maps[i];
                Bitmap& coarseSDFMap = coarseSDFCube.maps[i];
                sdfMap.freeMemory();
                coarseSDFMap.freeMemory();
            }
        }
    }

    namespace
    {
	    template <typename T> constexpr T sqr(T val) { return val * val; }
    }

    void generateCoarseRegionMapAndSDFPlane()
    {
	    using VoronoiTypeMap = std::pair<std::vector<String>, int>;

	    constexpr int regionCountBase = 3 * 1;
	    VoronoiTypeMap allMaps[]      =
	    {
		    { { "1_[3x1x113x-133]", }, regionCountBase * sqr(1) },
		    { { "2_[6x2x-44x-85]", }, regionCountBase * sqr(2) },
		    { { "3_[9x3x112x59]", }, regionCountBase * sqr(3) },
		    { { "4_[12x4x-183x192]", }, regionCountBase * sqr(4) },
		    { { "5_[15x5x168x-17]", }, regionCountBase * sqr(5) },
		    { { "6_[18x6x212x-182]", }, regionCountBase * sqr(6) },
		    { { "7_[21x7x59x-137]", }, regionCountBase * sqr(7) },
		    { { "8_[24x8x237x-165]", }, regionCountBase * sqr(8) },
		    { { "9_[27x9x237x-165]", }, regionCountBase * sqr(9) },
	    };

	    String filePath = "echo/biome_terrain/spherical_voronoi_map/";
	    struct GenerateRegionInfo
	    {
		    int regionCount = 0;
		    String fineRegionName;
		    std::vector<int> parentLookupTable;
	    };

	    std::vector<GenerateRegionInfo> infoArray;

	    for (const auto& [names, count] : allMaps)
	    {
		    for (const auto& name : names)
		    {
			    GenerateRegionInfo info;
			    info.fineRegionName = name;
			    info.regionCount    = count;
			    infoArray.push_back(info);
		    }
	    }

	    for (auto& info : infoArray)
	    {
		    String path = filePath + info.fineRegionName + ".bin";
		    DataStreamPtr file(Root::instance()->GetFileDataStream(path.c_str()));

		    Bitmap fineMap = {};
		    {
			    fineMap.width         = 2048;
			    fineMap.height        = 640;
			    fineMap.bytesPerPixel = 1;
			    fineMap.pitch         = fineMap.width * fineMap.bytesPerPixel;
			    fineMap.pixels        = malloc(fineMap.pitch * fineMap.height);

			    String fineMapName = filePath + info.fineRegionName + "_0.map";
			    file               = Root::instance()->GetFileDataStream(fineMapName.c_str());
			    if (file->getData())
			    {
				    memcpy(fineMap.pixels, file->getData(), fineMap.pitch * fineMap.height);
			    }
			    else
			    {
				    logToConsole("Fine region map:" + fineMapName + String("can't open file!"));
			    }
		    }

		    VoronoiRegionPack pack;
		    pack.loadAdjacents(filePath + info.fineRegionName);
		    std::vector parentLookupTable(pack.Params.x * pack.Params.y, -1);
		    if (!SphericalVoronoiRegionData::computeRegionParentIdPlane("", "", pack.mAdjacents, parentLookupTable))
		    {
			    throw std::runtime_error("computeRegionParentIdPlane failed!");
		    }

		    auto [fineSdf, coarseSdf, mergedId] = GeneratePlanetRegionSDFWarp(fineMap, parentLookupTable);

		    String coarseSdfMapName = filePath + "coarse_" + info.fineRegionName + "_0.sdf";

		    if (info.regionCount == regionCountBase)
		    {
			    coarseSdfMapName = filePath + "coarse_global_white_torus.sdf";
		    }

		    String fineSdfMapName = filePath + "fine" + info.fineRegionName + "_0.sdf";

		    DataStream* dataStream = Root::instance()->CreateDataStream(fineSdfMapName.c_str());
		    if (dataStream)
		    {
			    auto fileStream = dynamic_cast<FileDataStream*>(dataStream);
			    fileStream->write(fineSdf.pixels, fineSdf.pitch * fineSdf.height);
			    SAFE_DELETE(fileStream)
		    }

		    dataStream = Root::instance()->CreateDataStream(coarseSdfMapName.c_str());
		    if (dataStream)
		    {
			    auto fileStream = dynamic_cast<FileDataStream*>(dataStream);
			    fileStream->write(coarseSdf.pixels, coarseSdf.pitch * coarseSdf.height);
			    SAFE_DELETE(fileStream)
		    }

#if OUTPUT_TEST_IMAGE
		    String newFineFileName = fineSdfMapName.substr(0, fineSdfMapName.find(".sdf")) + ".png";

		    if (0 == stbi_write_png(("I:/SUmaster/bin/asset/client/" + newFineFileName).c_str(), fineSdf.width, fineSdf.height, 1, fineSdf.pixels, fineSdf.pitch))
		    {
			    logToConsole(fineSdfMapName + " output to PNG file fail!\n");
		    }

		    String newCoarseFileName = coarseSdfMapName.substr(0, coarseSdfMapName.find(".sdf")) + ".png";

		    if (0 == stbi_write_png(("I:/SUmaster/bin/asset/client/" + newCoarseFileName).c_str(), coarseSdf.width, coarseSdf.height, 1, coarseSdf.pixels, coarseSdf.pitch))
		    {
			    logToConsole(coarseSdfMapName + " output to PNG file fail!\n");
		    }
#endif

		    fineMap.freeMemory();

		    fineSdf.freeMemory();
		    coarseSdf.freeMemory();
		    mergedId.freeMemory();
	    }
    }
} // namespace Echo
