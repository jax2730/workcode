///////////////////////////////////////////////////////////////////////////////
//
// @file SceneHeatMap.h
// @author luoshuquan
// @date 2020/04/03 Friday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __SceneHeatMap_H__
#define __SceneHeatMap_H__

class SceneHeatMapPrivate;
namespace Echo
{
	class WorldManager;
}
class SceneHeatMap
{
public:
	SceneHeatMap();
	~SceneHeatMap();

	/**
	@brief 根据场景的大小配置热力图
	@param
	@return
	@remark
	*/
	void setup();

	void setPath(const char* _datapath);
	/**
	@brief 分析场景并生成热力图
	@param
	@return
	@remark
	*/
	void generate(Echo::WorldManager * worldmgr);

	/**
	@brief 解析输入的指令
	*/
	void parsecommand(const char* command);
private:
	SceneHeatMap(const SceneHeatMap &) {}
	SceneHeatMap(SceneHeatMap &&) {}
	SceneHeatMap & operator = (const SceneHeatMap &) { return *this; }
	SceneHeatMap & operator = (const SceneHeatMap &&) { return *this; }

private:
	SceneHeatMapPrivate *dp = nullptr;
}; // SceneHeatMap

#endif // __SceneHeatMap_H__
