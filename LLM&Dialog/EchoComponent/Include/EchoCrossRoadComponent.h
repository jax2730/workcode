#ifndef __EchoCrossRoadComponent_H__
#define __EchoCrossRoadComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoMeshColliderComponent.h"

namespace Echo
{
	enum ControlPlace
	{
		NoPlace	= 0b0,
		Left	= 0b1,
		Right	= 0b10,
		Front	= 0b100,
		Back	= 0b1000,
	};

	class _EchoComponentExport CrossRoadComponentInfo : public ActorComponentInfo
	{
	public:
		CrossRoadComponentInfo() {};
		CrossRoadComponentInfo(const Name& type, ActorCOMMoveableType moveable);
		virtual void ExportData(cJSON* jsonNode) override;
		virtual void ImportData(cJSON* jsonNode) override;
		virtual void CopyTo(ActorComponentInfo* info) override;
		virtual void OverWrite() override;
#ifdef ECHO_EDITOR
		virtual bool					getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		void SetPlace(size_t index, ControlPlace place);
		void SetCenter(size_t index, const Vector3& center);
		void SetRay(size_t index, const Vector3& ray);
		void DeleteData(int index = -1);

		Name m_MeshName;
		int PickPlace;
		std::vector<int> m_vPlace;
		std::vector<Vector3> m_vCenter;
		std::vector<Vector3> m_vRay;
		//char ui_MeshDir[256] = { '\0' };	// 记录选中mesh的文件地址

		float	m_RenderDistance = 1000.f;
		float	m_Lod1Distance = 100.f;
		float	m_Lod2Distance = 200.f;
		EDITOR_REFLECT
	};

	class _EchoComponentExport CrossRoadComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(CrossRoadComponent)
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void bindNode() override;

		void update();
		size_t getPointCount();
		void setPickHelperIndex(size_t value);
		bool TryUpdate();

		void frameWork();
		void loadMeshAndData(bool haveInitData = false);
		void LoadMesh(MeshPtr& mesh);
		void LoadData(const MeshPtr& mesh, bool haveInitData = false);

		void ResponseRequest(MeshPtr p_Mesh);
		void CreatePhyMode();
		inline static int GetPlaceNoEnum(ControlPlace place)
		{
			assert(place != ControlPlace::NoPlace && "CrossRoad Error");

			int placei = place;
			int num = 0;
			while (!(placei & 1)) { num++; placei >>= 1; }
			return num;
		}

		void SetPosInPlace(Vector3& pos, ControlPlace place);

		void SetVisitable(bool value);
		void KeepScale(const Vector3& cam_pos);
		bool HitPick(const Ray& ray);
		void HighlightPoint(const Ray& ray);

		size_t GetPickIndex() { return m_iPickControlNum; }
		Vector3 GetPickHelperPoint(int pick_index = -1);
		ControlPlace GetPickIndexPlace(int pick_index = -1);
		void GetPlaceVecData(ControlPlace place, Vector3& pos, Vector3& ray);

		void CreateControlPoint();
		void CreateInfoData();
		void SetControlPoint(int pick_index, ControlPlace place);
		void SetControlPoint(const Vector3& pos, int pick_index = -1);
		void SetControlRay(const Vector3& ray, int pick_index = -1);
		void DeleteControlPoint(int pick_index = -1);
		//std::vector<std::pair<ControlPlace, ControlConnect*>>& GetPrivateDatas();
		size_t GetPrivateDataSize();
		Vector3 GetPrivateDataPoint(size_t pick_index, bool isControl = false);
		ControlPlace GetPrivateControlPlace(size_t pick_index);

		bool GetTrackChange() { return m_isTrackChange; }
		void SetTrackChange(bool value) { m_isTrackChange = value; }
		bool* GetEditingIns() { return &m_isEditing; }
		bool GetEditing() { return m_isEditing; }
		void SetEditing(bool value) { m_isEditing = value; }

		float	GetRenderDistance();
		void	SetRenderDistance(float distance);

		float	GetLodDistance(EM_LOD_LEVEL lodlevel);
		void	SetLodDistance(EM_LOD_LEVEL lodlevel, float distance);

	private:
		class CrossRoadComponentPrivate* m_pPrivate = nullptr;
		MeshColliderComponentPtr m_pPhyMode;
		String m_strComKey;

		size_t m_iPickControlNum = 0;
		bool m_isTrackChange = false;
		bool m_isEditing = false;
	};

	using CrossRoadComponentPtr = EchoWeakPtr<CrossRoadComponent>;

	class _EchoComponentExport CrossRoadComProFunc
	{
	public:
		static void initProtocolFunction() {};
		static void uninitProtocolFunction() {};

		// static ProtocolResult ProCom_ShowTrajectory(void* isShow, ProtocolHandle handle);
	};

	using CrossRoadComponentPtr = EchoWeakPtr<CrossRoadComponent>;

	DEFINECOMFACTORY(CrossRoadComponent, ALL_MOVEABLE);
}

#endif //  __EchoIntersectionComponent_H__
