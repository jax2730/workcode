//-----------------------------------------------------------------------------
// File:   AppBase.h
//
// Author: chenanzhi
// Desc:
// Date:  2023-2-23
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __AppBase_H__
#define __AppBase_H__

#include "EchoPrerequisites.h"
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

class AppBase
{
public:
	AppBase() {}
	virtual ~AppBase() {}

	virtual void						onTick() = 0;
	virtual void						onRelease(bool releaseEngine = false) = 0;
	virtual void						onResize(Echo::uint32, Echo::uint32, Echo::uint32 safeAreaLeft, Echo::uint32 safeAreaTop, Echo::uint32 safeAreaRight, Echo::uint32 safeAreaBottom) = 0;

	virtual void						onStart() = 0;
	virtual void						onPause() = 0;
	virtual void						onResume() = 0;
	virtual void						onStop() = 0;
	virtual void						onResetWindow(void* handle) = 0;

	virtual void						setFPS(float _fps) = 0;
	virtual void						getWindowSize(Echo::uint32& width, Echo::uint32& height) = 0;
	virtual unsigned int				getTick() = 0;

	//////////////////////////////////////////////////////////////////////////
	virtual void						setNextLogicCmdInfo(const char* file, int line) = 0;
	virtual void						postToLogic_(const std::function<void()>& cmd) = 0;
	virtual void						postToLogic_(std::function<void()>&& cmd) = 0;

	virtual void						setNextEngineCmdInfo(const char* file, int line) = 0;
	virtual void						postToEngine_(const std::function<void()>& cmd) = 0;
	virtual void						postToEngine_(std::function<void()>&& cmd) = 0;

	virtual	void						postToCache_(const std::function<void()>& cmd) = 0;
	virtual void						postToCache_(std::function<void()>&& cmd) = 0;
	virtual	void						postToMain_(const std::function<void()>& cmd) = 0;
	virtual void						postToMain_(std::function<void()>&& cmd) = 0;
	virtual void						commandParse(const std::string& command) = 0;

	virtual void						onMouseMove(const Echo::Vector2& point) = 0;
	virtual void						onMouseLeftDown(const Echo::Vector2& point) = 0;
	virtual void						onMouseLeftUp(const Echo::Vector2& point) = 0;

	virtual void						cameraMove(int type, int val) = 0;
	virtual void						cameraTurn(float x, float y) = 0;
	virtual void						cameraDisChg(int val) = 0;
	virtual void						setMoveDir(float x, float y) = 0;
	virtual void						setTurnDir(float x, float y) = 0;
	virtual void						incCameraSpeed() = 0;
	virtual void						decCameraSpeed() = 0;
	virtual void						attack() = 0;
	virtual void						switchPlayMode() = 0;
	virtual void						createRobotObject() = 0;
	virtual void						sphericalControlerJump() {};

	virtual int							getFps() = 0;
	virtual int							getDrawCall() = 0;
	virtual int							getTriangleCnt() = 0;
	virtual std::string					getCameraPosition() = 0;
	virtual std::string					getCameraOrientation() = 0;
	virtual std::string					getResMemUsageStr() = 0;
	virtual void						create3DUIGunTest() = 0;
	virtual std::string					getGameData() = 0;

	virtual void						loadSenceByName(const std::string& name) = 0;

public:
	virtual void						OnKeyDown(unsigned int key) = 0;
	virtual void						OnKeyUp(unsigned int key) = 0;
	virtual void						OnMouseMove(Echo::Vector2 point, unsigned int buttonsFlag) = 0;
	virtual void						OnMouseLeftDown(Echo::Vector2 point) = 0;
	virtual void						OnMouseLeftUp(Echo::Vector2 point) = 0;
	virtual void						OnMouseRightDown(Echo::Vector2 point) = 0;
	virtual void						OnMouseRightUp(Echo::Vector2 point) = 0;
	virtual void						OnKillFocus() = 0;
	virtual void						OnInputText(char16_t c) = 0;

	virtual bool						isHasImGuiWindowHovered() const = 0;
};


//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
