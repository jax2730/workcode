///////////////////////////////////////////////////////////////////////////////
//
// @file BuildingTest.h
// @author luoshuquan
// @date 2020/09/29 Tuesday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "EchoVector3.h"

class BuildingTest
{

private:
	BuildingTest();
	BuildingTest(const BuildingTest &) = delete;
	BuildingTest(BuildingTest &&) = delete;
	BuildingTest & operator = (const BuildingTest &) = delete;

	~BuildingTest();

public:
	static BuildingTest & instance()
	{
		static BuildingTest test;
		return test;
	}

	void init(const Echo::Vector3 & pos = Echo::Vector3::ZERO);
	void init(const Echo::Vector3 & pos, int block_num);
	void clear();

	void setTestFlags(const char * flags);;

private:
	class BuildingTestPrivate * dp = nullptr;
};
