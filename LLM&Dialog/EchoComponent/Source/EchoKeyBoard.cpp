#include "EchoComponentStableHeaders.h"
#include "EchoKeyBoard.h"

namespace Echo
{
	EchoKeyBoard::EchoKeyBoard()
	{
	}
	EchoKeyBoard::~EchoKeyBoard()
	{
	}
	void EchoKeyBoard::KeyDown(int key)
	{
		bool ischange = false;
		for (auto& iter : KeyboardValueMap)
		{
			if (key == iter.first)
			{
				ischange = true;
				break;
			}
		}
		if (ischange)
			KeyboardValueMap[key] = true;
	}
	void EchoKeyBoard::KeyUp(int key)
	{
		bool ischange = false;
		for (auto& iter : KeyboardValueMap)
		{
			if (key == iter.first)
			{
				ischange = true;
				break;
			}
		}
		if (ischange)
			KeyboardValueMap[key] = false;
	}
	bool EchoKeyBoard::GetKeyBoard(int key)
	{
		if (key == 0)
			return false;
		if (KeyboardValueMap.find(key) != KeyboardValueMap.end())
			return KeyboardValueMap[key];
		else
			return false;
	}
	bool EchoKeyBoard::GetKeyBoardFromName(std::string keyName)
	{
		return GetKeyBoard(GetKeyFromName(keyName));
	}
	int EchoKeyBoard::GetKeyFromName(std::string keyName)
	{
		if (keyName == "")
			return 0;
		if (KeyboardNameMap.find(keyName) != KeyboardNameMap.end())
			return KeyboardNameMap[keyName];
		else
			return 0;
	}
}
