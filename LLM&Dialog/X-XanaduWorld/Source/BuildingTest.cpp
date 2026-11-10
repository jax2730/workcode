///////////////////////////////////////////////////////////////////////////////
//
// @file BuildingTest.cpp
// @author luoshuquan
// @date 2020/09/29 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "BuildingTest.h"
#include "EchoRoot.h"
#include "EchoLogManager.h"
#include "EchoFrameListener.h"
#include "EchoMovableObject.h"
#include "EchoBuildingObject.h"
#include "EchoStringVector.h"
#include "EchoStringConverter.h"
#include "EchoName.h"

using namespace Echo;

namespace
{
	static int building_num = 4;
	//static int storey = 10;
	//static int row_block = 10, column_block = 10;

	vector<String>::type BlockVec = {
		"model/block/xzj/pj_qb_001_h.mesh",
		"model/block/xzj/pj_wy_001_h.mesh",
		"model/block/xzj/pj_thb_001_h.mesh",
		"model/block/xzj/pj_wy_001_h.mesh",
		"model/block/xzj/pj_wd30_001_h.mesh",
		"model/block/xzj/pj_thb_001_h.mesh",
		"model/block/xzj/pj_dj_001_h.mesh",
		"model/block/xzj/pj_dj_001_h.mesh",
		"model/block/xzj/pj_qb_001_h.mesh",
		"model/block/xzj/pj_wy_001_h.mesh",
		"model/block/xzj/pj_dj_003_h.mesh",
		"model/block/xzj/pj_thb_001_h.mesh",
		"model/block/xzj/pj_zz_001_h.mesh",
		"model/block/xzj/pj_qb_001_h.mesh",
		"model/block/xzj/pj_thb_001_h.mesh",
		"model/furniture/jj_lideng_001.mesh",
		"model/block/xzj/pj_wdgj_001.mesh",
		"model/furniture/jj_chucungui_001.mesh",
		"model/block/xzj/pj_lt_002_h.mesh",
		"model/furniture/jj_futai_001.mesh",
		"model/block/xzj/pj_wy_001_h.mesh",
		"model/block/xzj/pj_wdgj_001.mesh",
		"model/block/tg/tg_men_001.mesh",
	};
}

class BuildingTestPrivate : public FrameListener
{
public:
	std::vector<std::unique_ptr<BuildingObject>> buildings;

	enum TestFlag
	{
		eTF_Aging,
		eTF_CoverWeather,
		eTF_Mask,
		eTF_Transform,
	};

	std::bitset<std::numeric_limits<unsigned>::digits> flags;

	virtual bool frameStarted(const FrameEvent& evt) override;
	virtual bool frameEnded(const FrameEvent& evt) override;
}; // BuildingTestPrivate

bool BuildingTestPrivate::frameStarted(const FrameEvent & evt)
{
	if (flags.test(eTF_Aging))
	{
		for (auto & i : buildings) {
			i->setAgeingStrength(evt.timeElapsed - Math::Floor(evt.timeElapsed));
		}
	}

	if (flags.test(eTF_CoverWeather))
	{
		bool bSet = true;
		for (auto & i : buildings)
		{
			const auto & blocks = i->getBuildMap();
			for (auto & block : blocks) {
				block.second->setCoverWeatherScale(bSet ? 0.f : 1.f);
				bSet = !bSet;
			}
		}
		flags.reset(eTF_CoverWeather);
	}

	if (flags.test(eTF_Mask))
	{
		static bool MASK_COLOR_SET = true;
		if (MASK_COLOR_SET)
		{
			for (auto & i : buildings)
			{
				const auto & blocks = i->getBuildMap();
				for (auto & block : blocks) {
					block.second->setMaskColor(1, Echo::ColorValue::Red);
				}
			}
		}
		else
		{
			for (auto & i : buildings)
			{
				const auto & blocks = i->getBuildMap();
				for (auto & block : blocks) {
					block.second->resetMaskColor(1);
				}
			}
		}
		MASK_COLOR_SET = !MASK_COLOR_SET;
		flags.reset(eTF_Mask);
	}
	
	if (flags.test(eTF_Transform))
	{
		static int Step = 0;
		float fDir = (Step++ / 200) % 2 == 0 ? 1.f : -1.f;
		//float fDir = 1.f;
		for (auto & i : buildings)
		{
			Echo::Vector3 pos = i->getPosition();
			pos += fDir * evt.timeSinceLastFrame * 10.f;
			i->setPosition(pos);
		}
	}

	return true;
}

bool BuildingTestPrivate::frameEnded(const FrameEvent & evt)
{
	return true;
}

BuildingTest::BuildingTest()
	: dp(new BuildingTestPrivate)
{
}

BuildingTest::~BuildingTest()
{
	delete dp; dp = nullptr;
}

void BuildingTest::init(const Vector3 & pos)
{
	clear();

	if (building_num <= 0)
		return;

	using namespace Echo;
	Root::instance()->addFrameListener(dp);

	int num_per_row = int(Math::Sqrt(building_num * 1.f) + 0.5f);
	for (int num=0; num<building_num; ++num)
	{
		int cb = num / num_per_row;
		int rb = num % num_per_row;
		BuildingObject * pBuildingObj = new BuildingObject();
		pBuildingObj->loadFromJson("construction/building/cy_jiulicheng_001.building", true);
		pBuildingObj->setPosition(pos + DBVector3(rb * 33.f, 0.f, cb * 33.f));
		LogManager::instance()->logMessage("Building block num: " +
				StringConverter::toString(pBuildingObj->getBuildMap().size()));
		dp->buildings.emplace_back(pBuildingObj);
	}
}

/*
void BuildingTest::init(const Vector3 & pos)
{
	clear();

	using namespace Echo;
	Root::instance()->addFrameListener(dp);

	// read block list
	DataStreamPtr blockStream(Root::instance()->GetFileDataStream("building_test.txt"));
	if (!blockStream.isNull())
	{
		size_t sz = blockStream->size();
		String sData;
		sData.resize(sz, 0);
		blockStream->read(&sData[0], sz);

		BlockVec = StringUtil::split(sData, "\r\n");

		LogManager::instance()->logMessage(LML_CRITICAL, "open building_test.txt successed!");
	}
	else
	{
		LogManager::instance()->logMessage(LML_CRITICAL, "open building_test.txt failed!");
	}

	if (BlockVec.empty())
	{
		LogManager::instance()->logMessage(LML_CRITICAL, "block list is empty!");
		return;
	}

	int num_per_row = int(Math::Sqrt(building_num * 1.f) + 0.5f);
	for (int num=0; num<building_num; ++num)
	{
		int cb = num / num_per_row;
		int rb = num % num_per_row;

		BuildingObject *pBuildingObj = new BuildingObject();
		pBuildingObj->setInitFinish();
		for (int s=0; s<storey; ++s)
		{
			for (int cl=0; cl<column_block; ++cl)
			{
				for (int rl=0; rl<row_block; ++rl)
				{
					uint64 id = s * row_block * column_block + rl * row_block + cl;
					pBuildingObj->addBuildBlock(id, Name(BlockVec[id % BlockVec.size()].c_str()),
							Vector3(rl * 3.f, s * 3.f, cl * 3.f));
				}
			}
		}
		pBuildingObj->setPosition(pos + Vector3(rb * 33.f, 0.f, cb * 33.f));
		//pBuildingObj->setAgeingStrength(1.f);
		dp->buildings.emplace_back(pBuildingObj);
	}
}
*/

void BuildingTest::init(const Echo::Vector3 & pos, int block_num)
{
	if (block_num < 0)
		return;

	building_num = block_num;
	init(pos);
}

void BuildingTest::clear()
{
	dp->buildings.clear();
	Root::instance()->removeFrameListener(dp);
}

void BuildingTest::setTestFlags(const char * flags)
{
	dp->flags = decltype(dp->flags)(flags);
}
