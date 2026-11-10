
#ifdef _WIN32

#ifndef _WIN32_REMOTE_CONTOL_
#define  _WIN32_REMOTE_CONTOL_

#include "appview.h"
#include "AppBase.h"
#include "RemoteControl/EchoRemoteControl.h"

using namespace Echo;

inline void remoteKeyCallback(std::vector< KeyCode> risingKey, std::vector< KeyCode> fallingKey) {

	OutputDebugString("process key \r\n");
	for (const auto& key : risingKey) {
		switch (key) {
		case 3:
			get_appview()->cameraMove(3, 0);
		default:
			break;
		}
		char msg[128] = { 0 };
		snprintf(msg, 128, "%d", (int)key);
		OutputDebugString(msg);
		OutputDebugString("\t");
	}
	OutputDebugString("\r\n");
	for (const auto& key : fallingKey) {
		switch (key) {
		case 1:
			get_appview()->attack();
			break;
		case 2:
			get_appview()->sphericalControlerJump();
			break;
		case 3:
			get_appview()->cameraMove(3, 1);
		default:
			break;
		}
		char msg[128] = { 0 };
		snprintf(msg, 128, "%d", (int)key);
		OutputDebugString(msg);
		OutputDebugString("\t");
	}
	OutputDebugString("\r\n");
}

inline void remoteMoveCallback(Vector3 moveDir, uint8 change) {
	if (change != 0) {
		for (int i = 0; i < 3; i++) {
			if (change & (1 << i)) {
				get_appview()->cameraMove(i, (int)moveDir[i]);
			}
		}
	}
}

inline void remoteMouseMoveCallback(Vector2 delta) {
	if (delta != Vector2(0, 0))
	{
		OutputDebugString("process cursor \r\n"); char msg[128] = { 0 };
		snprintf(msg, 128, "%f,%f", delta.x,delta.y);
		OutputDebugString(msg);
		get_appview()->cameraTurn(delta.x, delta.y);
	}
}

inline void initRemoteControl() {
	static RemoteControl remoteControl;
	/* register remote controler */
	{
		OutputDebugString("init zmq listener \r\n");

		RemoteControl::SetMouseMoveCallback(remoteMouseMoveCallback);
		RemoteControl::SetKeyboardCallback(remoteKeyCallback);
		RemoteControl::SetMovementCallback(remoteMoveCallback);
	}
}

inline void onUpdate() {
	RemoteControl::Update();
}

inline void destoryRemoteControl() {
	RemoteControl::StopListener();
}
#endif // !_WIN32_REMOTE_CONTOL_

#endif // _WIN32
