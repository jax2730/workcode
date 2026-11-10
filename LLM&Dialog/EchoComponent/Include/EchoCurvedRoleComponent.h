///////////////////////////////////////////////////////////////////////////////
//
// @file EchoCurvedRoleComponent.h
// @author leixu	
// @date 2024/2/21
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef  __ECHO_CURVED_ROLE_COMPONENT__
#define	 __ECHO_CURVED_ROLE_COMPONENT__
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoCurvedRole.h"
#include "EchoCurvedRoleUtil.h"
namespace Echo
{

	class _EchoComponentExport CurvedRoleComponentInfo : public ActorComponentInfo
	{
	public:
		CurvedRoleComponentInfo() {}
		CurvedRoleComponentInfo(const Name& type, ActorCOMMoveableType moveable);

		virtual void					ExportData(cJSON* jsonNode) override;
		virtual void					ImportData(cJSON* jsonNode) override;
		virtual void					CopyTo(ActorComponentInfo* info) override;
		virtual void					OverWrite() override;
		void							setFileName(const String fileName) { FileName = fileName; };
		String							getFileName() { return FileName; }
		void							addRoleInfo(String roleName = "", RoleInfo* roleInfo = nullptr);
		void							removeRoleInfo(const String& roleName);
		void							clear();
		void							setAnimation(const String& role, const String& animation, bool isNormal);
		size_t							getRoleIndex(const String& role);
		void							setSkin(const String& roleName, const String& skin);
		void							setAnimationSpeed(const String& roleName, const float& speed);
		void							setAnimationLoop(const String& roleName, const bool& loop);
		void							setStartDistance(const String& roleName, const float& distance);
		void							setRenderDistance(const String& roleName, const float& renderDistance);
		void							setAnimationPlayTimes(const String& roleName, const int& playTimes);
		void							setRoleScale(const String& roleName, const Vector3& scale);
		void							setRoleLod(const String& roleName, const Vector3& lod);
		String							FileName;
		AxisAlignedBox					box;
		float							mNormalSpeed = 0.5f;
		float							mDisturbedSpeed = 1.0f;
		float							mEscapeDistance = 10.0f;
		bool							isStop = false;

		std::vector<String>				roleNames;
		std::vector<String>				normalAnimations;
		std::vector<String>				escapeAnimations;
		std::vector<String>				skins;
		std::vector<float>				animationSpeeds;
		std::vector<String>				animationLoops;
		std::vector<float>				startDistances;
		std::vector<float>				normalAnimationTimes;
		std::vector<float>				escapeAnimationTimes;
		std::vector<float>				renderDistances;
		std::vector<int>				animationPlayTimes;
		std::vector<Vector3>			roleScales;
		std::vector<Vector3>			roleLODDistances;
		Echo::vector<Vector3>::type		mVecPoints;
	public:

		EDITOR_REFLECT
	};

	class _EchoComponentExport CurvedRoleComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(CurvedRoleComponent)

	public:
		virtual void									init(ActorComponentInfo* info)override;
		bool*											getEditing() { return &isEditing; }
		void											setEditing(const bool& isEdit) { isEditing = isEdit; }
		bool											loadCurvedRoleFile(const std::string& filePath);
		bool											saveCurvedRoleFile(const std::string& filePath);
		void											exportJsonScene(cJSON* pParent);
		CurvedRoleComponentInfo*						getInfoData();
		float											getCurveLength();
		void											setReset();
		void											updatePoints();
		void											getAnimationList(const std::string& roleName, std::vector<std::string>& animations);
		void											getSkinList(const std::string& roleName, std::vector<std::string>& skins) ;
		void											getRolesInfo();
		void											setAnimation(const String& role, const String& animation,bool isNormal);
		void											setSkin(const String& role, const String& skin);
		void											setAnimationSpeed(const String& role, const float& animationSpeed);
		void											setPlayTimes(const String& role, const int& playTimes);
		void											setStartPosition(const String& role, const float& startPos);
		void											setAnimationLoop(const String& role, const bool& loop);
		void											setRoleScale(const String& role, const Vector3& scale);
		void											setRoleRenderDistance(const String& role, const float& renderDistance);
		void											setRoleLODDistance(const String& role, float lod[3]);
		void											removeRole(const String& role);
		void											addRole(const RoleInfo& role);
		void											getRoleInfo(RoleInfo& role, size_t index);
		void											insertNullPoint(size_t index);
		void											deletePoint(size_t index);
		void											update();
		void											updateNode();
		bool											pickHelper(const Ray& ray);
		void											setPickHelperIndex(size_t value);
		Vector3											getPickHelperPoint(size_t index = -1);
		void											SetVisible(bool isVisible);
		bool											getVisible();
		void											HighlightPoint(const Ray& ray);
		size_t											getPointCount();
		void											setPickHelperPoint(Vector3 point, size_t index = -1);
		void											updateHelperAndCurveLine();
		Vector3											RestoreToOriginalPoint(const Vector3& pos);
		void											updateCurvedRoleParams(const Vector3& pos,const Vector3& scale, const Quaternion& q);
		CurvedRole*										getCurvedRole() { return mCurvedRole.get(); }
		Vector3											getOffset();
		Quaternion										transformEulerAngles(const Vector3& EulerAngles);
		void											createCurve(const String& name);
		void											KeepScale(const Vector3& cam_pos);
		
	private:

		Curve* mCurve = nullptr;
		void											getDataFromCurvedRole();
		CurvedRoleData*									data = nullptr;
		bool											mAddScene = true;
		Quaternion										mOri = Quaternion::IDENTITY;
		size_t											mHelperPickedIndex = -1;
		CurvedRoleUtil*									mCurvedRoleUtil = nullptr;
		bool											isEditing = false;
		std::shared_ptr<CurvedRole>						mCurvedRole;
		std::map<std::string,std::vector<std::string>>	mAnimationList;
		std::map<std::string,std::vector<std::string>>	mSkinList;
	};

	DEFINECOMFACTORY(CurvedRoleComponent, ALL_MOVEABLE)
	typedef EchoWeakPtr<CurvedRoleComponent> CurvedRoleComponentPtr;
}

#endif // ! __ECHO_CURVED_ROLE_COMPONENT__
