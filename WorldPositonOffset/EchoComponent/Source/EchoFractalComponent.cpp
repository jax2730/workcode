#include "EchoComponentStableHeaders.h"
#include "EchoFractalComponent.h"
#include "EchoLSystem.h"
#include "VoxelTerrain/EchoVoxelTerrainFractalDefinition.h"
#include "VoxelTerrain/EchoVoxelTerrainFractal.h"
#include "EchoHelperObj.h"

#include <numeric>
using namespace Echo;
namespace
{
	Echo::ColorValue rootColorValue(0.8f, 0.8f, 0.0f, 0.8f);
	Echo::ColorValue normalColorValue(0.0f, 0.0f, 0.8f, 0.8f);
	Echo::ColorValue highlightColorValue(0.8f, 0.0f, 0.0f, 0.8f);

}
namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(FractalComponent);

	FractalComponent::FractalComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: NodeActorComponent(pActor, type)
#ifdef ECHO_EDITOR
		, mSceneManager(nullptr)
		, mPickHelperPointIndex(-1)
		, mRootPoint(nullptr)
		, mIsEditing(false)
		, mIsDirty(true)
#endif
	{}
	FractalComponent::~FractalComponent() {}

	void FractalComponent::init(ActorComponentInfo* info)
	{
#ifdef ECHO_EDITOR
		mSceneManager = Root::instance()->getMainSceneManager();
		mRootPoint = std::make_shared<SphereHelper>(mSceneManager);
		mRootPoint->setRadius(8.88f);
		mRootPoint->setColor(rootColorValue);
		mRootPoint->setVisible(true);
		mRootPoint->setPosition(Vector3::ZERO);

		mComRootNode->addChild(mRootPoint->GetSceneNode());
		mComRootNode->_update();

		mHelperRoot = std::make_shared<SceneNode>(mSceneManager);
		mComRootNode->addChild(mHelperRoot.get());
#endif
	}

	void FractalComponent::unInit()
	{
	}

	void FractalComponent::createComRootNodeImpl()
	{
		mComRootNode = Root::instance()->getMainSceneManager()->createSceneNode();
	}

#ifdef ECHO_EDITOR
	void FractalComponent::addPoint(const Vector3& position)
	{

		if (auto definition = dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()))
		{
			definition->mProtoType.push_back(position);

			auto point = mProtoTypePoints.emplace_back(std::make_shared<SphereHelper>(mSceneManager));
			point->setRadius(8.88f);
			point->setColor(normalColorValue);
			point->setVisible(true);
			point->setPosition(position);

			mComRootNode->addChild(point->GetSceneNode());
			mComRootNode->_update();
		}
	}

	void FractalComponent::setPointPosition(int index, const Vector3& position)
	{
		if (index < 0 || index >= (int)mProtoTypePoints.size()) return;
		if (auto definition = dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()))
		{
			definition->mProtoType[index] = position;
			mProtoTypePoints[index]->setPosition(position);
		}
	}

	void FractalComponent::removePoint(int index)
	{
		if (index < 0 || index >= (int)mProtoTypePoints.size()) return;
		if (auto definition = dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()))
		{
			definition->mProtoType.erase(definition->mProtoType.begin() + index);

			auto helper = mProtoTypePoints[index];
			mComRootNode->removeChild(helper->GetSceneNode());
			mProtoTypePoints.erase(mProtoTypePoints.begin() + index);
		}
	}

	const std::vector<Vector3>& FractalComponent::getPoints()
	{
		static std::vector<Vector3> points;
		if (auto definition = dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()))
		{
			return definition->mProtoType;
		}
		return points;
	}

	void FractalComponent::generate()
	{
		if(mIsDirty && mDefinition)
		{
			mGenerator.reset();
			mGenerator = std::make_shared<VoxelTerrainFractalImpl>();
			switch (mDefinition->mType)
			{
			case FractalDefinition::EDefault:
			{
				VoxelTerrainFractalDescriptor descriptor;
				descriptor.mDefinition = std::make_unique< DefaultFractalDefinition >(*dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()));
				mGenerator->Load( std::move(descriptor));
				break;
			}
			case FractalDefinition::ELSystem:
			{
				VoxelTerrainFractalDescriptor descriptor;
				descriptor.mDefinition = std::make_unique< LSystemFractalDefinition >(*dynamic_cast<LSystemFractalDefinition*>(mDefinition.get()));
				mGenerator->Load(std::move(descriptor));
				break;
			}
			default:
				mGenerator.reset();
				return;
			}

			std::vector<Vector3> vb;
			std::vector<uint16> ib;
			if(VoxelTerrainFractalImpl::GetVBIB(*mGenerator, vb, ib); !vb.empty() && !ib.empty())
			{
				mHelperObj.clear();
				auto helper = mHelperObj.emplace_back(std::make_shared<HelperObject>(Name{}, HelperObjType_Custom, false, mHelperRoot.get()));
				helper->setRenderType(ECHO_LINE_LIST);
				helper->setHelperColorType(HelperObjColorType_Blue);
				helper->changeCustomPolygon(vb, ib);
				helper->setVisible(true);
				helper->SetRanderingDistance(100000.0f);

				helper = mHelperObj.emplace_back(std::make_shared<HelperObject>(Name{}, HelperObjType_Custom, false, mHelperRoot.get()));
				helper->setRenderType(ECHO_POINT_LIST);
				helper->setHelperColorType(HelperObjColorType_Red);
				ib.clear(); ib.resize(vb.size());
				std::iota(ib.begin(), ib.end(), 0);
				helper->changeCustomPolygon(vb, ib);
				helper->setVisible(true);
				helper->SetRanderingDistance(100000.0f);

				if(mDefinition->mType== FractalDefinition::EDefault)
				{
					for (const auto& v : vb)
					{
						auto& helper = mHelperObj.emplace_back(std::make_shared<HelperObject>(Name{}, HelperObjType_Sphere, false, mHelperRoot.get()));
						helper->setPosition(v);
						helper->setHelperColorType(HelperObjColorType_Green);
						helper->setScale(Vector3(8.88f));
						helper->setVisible(true);
						helper->SetRanderingDistance(100000.0f);
					}
				}
			}

			mIsDirty = false;
		}
	}

	bool FractalComponent::pickHelper(const Ray& ray)
	{
		if (auto helper = getPickHelper())
		{
			helper->setColor(normalColorValue);
		}
		mPickHelperPointIndex = -1;
		if (mProtoTypePoints.empty()) return false;
		float closest_distance = std::numeric_limits<float>::infinity();
		for (int index = 0; index < (int)mProtoTypePoints.size(); ++index)
		{
			auto point = mProtoTypePoints[index];
			if (auto res = point->hitTest(ray); (res.first && res.second < closest_distance))
			{
				closest_distance = res.second;
				mPickHelperPointIndex = index;
			}
		}
		if (auto helper = getPickHelper())
		{
			helper->setColor(highlightColorValue);
		}
		return mPickHelperPointIndex != -1;
	}

	Vector3 FractalComponent::getPickHelperPoint()
	{
		return getPickHelper()->getPosition();
	}

	SphereHelper* FractalComponent::getPickHelper()
	{
		if (mPickHelperPointIndex >= 0 && mPickHelperPointIndex < (int)mProtoTypePoints.size())
		{
			return mProtoTypePoints[mPickHelperPointIndex].get();
		}
		return nullptr;
	}

	void FractalComponent::reset()
	{
		mHelperObj.clear();
		if (mDefinition)
		{
			switch (mDefinition->mType)
			{
			case FractalDefinition::EDefault:
			{
				while (!mProtoTypePoints.empty()) removePoint(0);
				break;
			}
			default:
				break;
			}
			mDefinition.reset();
		}
	}

	void FractalComponent::reload(FractalDefinition&& definition)
	{
		reset();

		switch (definition.mType)
		{
		case FractalDefinition::EDefault:
		{
			mDefinition = std::make_shared<DefaultFractalDefinition>();
			if (auto pointer = dynamic_cast<DefaultFractalDefinition*>(&definition))
			{
				for (const auto& pos : pointer->mProtoType)
				{
					addPoint(pos);
				}
				*dynamic_cast<DefaultFractalDefinition*>(mDefinition.get()) = std::move(*pointer);
			}
			break;
		}
		case FractalDefinition::ELSystem:
		{
			mDefinition = std::make_shared<LSystemFractalDefinition>(std::move(static_cast<LSystemFractalDefinition&>(definition)));
		}
		default:
			break;
		}

		setDirty();
	}
#endif

}
namespace Echo
{ 
	EDITOR_REFLECT_BEGIN(FractalComponentInfo)
	EDITOR_REFLECT_END()

	_DEFINEINFO_VIRFUN_BASE(FractalComponentInfo)

#ifdef ECHO_EDITOR
	bool FractalComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		return true;
	}
#endif
}
