#pragma once

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "ControlHelperRenderable.h"
namespace Echo
{
#ifdef ECHO_EDITOR
	namespace Fractal
	{
		struct FractalDefinition;
		struct DefaultFractalDefinition;
		struct LSystemFractalDefinition;
	}
	using EchoBox::SphereHelper;
#endif
	class _EchoComponentExport FractalComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(FractalComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		virtual void					init(ActorComponentInfo* info) override;
		virtual void					unInit() override;
	private:
		virtual void					createComRootNodeImpl() override;
#ifdef ECHO_EDITOR
	public:
		typedef Fractal::FractalDefinition FractalDefinition;
		typedef Fractal::DefaultFractalDefinition DefaultFractalDefinition;
		typedef Fractal::LSystemFractalDefinition LSystemFractalDefinition;

		void							addPoint(const Vector3& position);
		void							setPointPosition(int index, const Vector3& position);
		void							removePoint(int index);
		const std::vector<Vector3>&		getPoints();
		void							generate();
		bool							pickHelper(const Ray& ray);
		int								getPickHelperIndex() const { return mPickHelperPointIndex; }
		Vector3							getPickHelperPoint();
		SphereHelper*					getPickHelper();

		void							setEditing(bool isEditing) { mIsEditing = isEditing; }
		bool							isEditing() const { return mIsEditing; }
		void							setDirty() { mIsDirty = true; }
		void							reset();
		void							reload(FractalDefinition&& definition);

		String												mPath;
		std::shared_ptr<FractalDefinition>					mDefinition;
	private:
		SceneManager*										mSceneManager;
		int													mPickHelperPointIndex;
		std::shared_ptr<SphereHelper>						mRootPoint;
		std::vector<std::shared_ptr<SphereHelper>>			mProtoTypePoints;
		std::shared_ptr<SceneNode>							mHelperRoot;
		std::vector<std::shared_ptr<HelperObject>>			mHelperObj;

		bool												mIsEditing;
		bool												mIsDirty;
		std::shared_ptr<VoxelTerrainFractalImpl>			mGenerator;
#endif
	};

	class _EchoComponentExport FractalComponentInfo : public ActorComponentInfo {
	public:
										FractalComponentInfo() {}
										FractalComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {};
		virtual							~FractalComponentInfo() {};
#ifdef ECHO_EDITOR
		virtual bool getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		_DECLCOMINFO_VIRFUN
		EDITOR_REFLECT
	};

	DEFINECOMFACTORY(FractalComponent, ALL_MOVEABLE);
}
