#ifndef __EchoKeyBoard_H__
#define __EchoKeyBoard_H__

#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"
#include "EchoProtocolComponent.h"

namespace Echo
{
	class _EchoComponentExport EchoKeyBoard : public Singleton<EchoKeyBoard>
	{
	public:
		EchoKeyBoard();
		~EchoKeyBoard();
	public:
		void								KeyDown(int key);
		void								KeyUp(int key);

		bool								GetKeyBoard(int key);
		bool								GetKeyBoardFromName(std::string keyName);
		int									GetKeyFromName(std::string keyName);

		const std::map<std::string, int>&	GetKeyboardNameMap() { return KeyboardNameMap; };

		const std::map<int, bool>&			GetKeyboardValueMap() { return KeyboardValueMap; };

	private:
		std::map<std::string, int> KeyboardNameMap =
		{
			{ " ",0 },
			{ "a",0x41 },{ "b",0x42 },{ "c",0x43 },{ "d",0x44 },{ "e",0x45 },{ "f",0x46 },{ "g",0x47 },{ "h",0x48 },{ "i",0x49 },{ "j",0x4a },
			{ "k",0x4b },{ "l",0x4c },{ "m",0x4d },{ "n",0x4e },{ "o",0x4f },{ "p",0x50 },{ "q",0x51 },{ "r",0x52 },{ "s",0x53 },{ "t",0x54 },
			{ "u",0x55 },{ "v",0x56 },{ "w",0x57 },{ "x",0x58 },{ "y",0x59 },{ "z",0x5a },
			{ "shift",16 },{ "ctrl",17 },{ "alt",18 },{ "space",32 },
			{ "0",0x30 },{ "1",0x31 },{ "2",0x32 },{ "3",0x33 },{ "4",0x34 },{ "5",0x35 },{ "6",0x36 },{ "7",0x37 },{ "8",0x38 },{ "9",0x39 }
		};
		std::map<int, bool> KeyboardValueMap =
		{
			{ 0 ,false },
			{ 0x41,false },{ 0x42,false },{ 0x43,false },{ 0x44 ,false },{ 0x45,false },{ 0x46,false },{ 0x47,false },{ 0x48,false },{ 0x49,false },{ 0x4a,false },
			{ 0x4b,false },{ 0x4c,false },{ 0x4d,false },{ 0x4e,false },{ 0x4f,false },{ 0x50,false },{ 0x51,false },{ 0x52 ,false },{ 0x53,false },{ 0x54,false },
			{ 0x55,false },{ 0x56,false },{ 0x57,false },{ 0x58,false },{ 0x59,false },{ 0x5a,false },
			{ 16 ,false },{ 17,false },{ 18 ,false },{ 32,false },
			{ 0x30 ,false },{ 0x31,false },{ 0x32 ,false },{ 0x33,false },{ 0x34 ,false },{ 0x35 ,false },{ 0x36,false },{ 0x37,false },{ 0x38 ,false },{ 0x39,false }
		};
	};
}

#endif // __EchoKeyBoard_H__