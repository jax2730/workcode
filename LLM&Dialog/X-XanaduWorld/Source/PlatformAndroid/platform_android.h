#ifndef __PLATFORM_ANDROID_H__
#define __PLATFORM_ANDROID_H__

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <string>
#include <unordered_map>

namespace appui
{
	// 点击编辑框的回调
	void on_input_begin(int _x, int _y, const char* _szText);
}



namespace EchoVk
{
	enum ExitCode
	{
		Success = 0, /* App executed as expected */
		Help,        /* App should show help */
		Close,       /* App has been requested to close at initialization */
		FatalError   /* App encountered an unexpected error */
	};

	enum TouchAction
	{
		Down,
		Up,
		Move,
		Cancel,
		PointerDown,
		PointerUp,
		Unknown
	};

	struct TouchInputEvent
	{
		TouchAction action;
		int32_t pointer_id;
		size_t touch_points;
		float pos_x;
		float pos_y;
	};

	enum class KeyCode
	{
		Unknown,
//		Space,
//		Apostrophe, /* ' */
//		Comma,      /* , */
//		Minus,      /* - */
//		Period,     /* . */
//		Slash,      /* / */
//		_0,
//		_1,
//		_2,
//		_3,
//		_4,
//		_5,
//		_6,
//		_7,
//		_8,
//		_9,
//		Semicolon, /* ; */
//		Equal,     /* = */
//		A,
//		B,
//		C,
//		D,
//		E,
//		F,
//		G,
//		H,
//		I,
//		J,
//		K,
//		L,
//		M,
//		N,
//		O,
//		P,
//		Q,
//		R,
//		S,
//		T,
//		U,
//		V,
//		W,
//		X,
//		Y,
//		Z,
//		LeftBracket,  /* [ */
//		Backslash,    /* \ */
//		RightBracket, /* ] */
//		GraveAccent,  /* ` */
		Escape,
//		Enter,
//		Tab,
//		Backspace,
//		Insert,
//		DelKey,
//		Right,
//		Left,
//		Down,
//		Up,
//		PageUp,
//		PageDown,
//		Home,
//		End,
		Back,
//		CapsLock,
//		ScrollLock,
//		NumLock,
//		PrintScreen,
//		Pause,
//		F1,
//		F2,
//		F3,
//		F4,
//		F5,
//		F6,
//		F7,
//		F8,
//		F9,
//		F10,
//		F11,
//		F12,
//		KP_0,
//		KP_1,
//		KP_2,
//		KP_3,
//		KP_4,
//		KP_5,
//		KP_6,
//		KP_7,
//		KP_8,
//		KP_9,
//		KP_Decimal,
//		KP_Divide,
//		KP_Multiply,
//		KP_Subtract,
//		KP_Add,
//		KP_Enter,
//		KP_Equal,
//		LeftShift,
//		LeftControl,
//		LeftAlt,
//		RightShift,
//		RightControl,
//		RightAlt
	};

	enum class KeyAction
	{
		Down,
		Up,
		Repeat,
		Unknown
	};

	struct KeyInputEvent
	{
		KeyCode code;
		KeyAction action;
	};

	inline KeyCode translate_key_code(int key)
	{
		static const std::unordered_map<int, KeyCode> key_lookup =
				{
//						{AKEYCODE_SPACE, KeyCode::Space},
//						{AKEYCODE_APOSTROPHE, KeyCode::Apostrophe},
//						{AKEYCODE_COMMA, KeyCode::Comma},
//						{AKEYCODE_MINUS, KeyCode::Minus},
//						{AKEYCODE_PERIOD, KeyCode::Period},
//						{AKEYCODE_SLASH, KeyCode::Slash},
//						{AKEYCODE_0, KeyCode::_0},
//						{AKEYCODE_1, KeyCode::_1},
//						{AKEYCODE_2, KeyCode::_2},
//						{AKEYCODE_3, KeyCode::_3},
//						{AKEYCODE_4, KeyCode::_4},
//						{AKEYCODE_5, KeyCode::_5},
//						{AKEYCODE_6, KeyCode::_6},
//						{AKEYCODE_7, KeyCode::_7},
//						{AKEYCODE_8, KeyCode::_8},
//						{AKEYCODE_9, KeyCode::_9},
//						{AKEYCODE_SEMICOLON, KeyCode::Semicolon},
//						{AKEYCODE_EQUALS, KeyCode::Equal},
//						{AKEYCODE_A, KeyCode::A},
//						{AKEYCODE_B, KeyCode::B},
//						{AKEYCODE_C, KeyCode::C},
//						{AKEYCODE_D, KeyCode::D},
//						{AKEYCODE_E, KeyCode::E},
//						{AKEYCODE_F, KeyCode::F},
//						{AKEYCODE_G, KeyCode::G},
//						{AKEYCODE_H, KeyCode::H},
//						{AKEYCODE_I, KeyCode::I},
//						{AKEYCODE_J, KeyCode::J},
//						{AKEYCODE_K, KeyCode::K},
//						{AKEYCODE_L, KeyCode::L},
//						{AKEYCODE_M, KeyCode::M},
//						{AKEYCODE_N, KeyCode::N},
//						{AKEYCODE_O, KeyCode::O},
//						{AKEYCODE_P, KeyCode::P},
//						{AKEYCODE_Q, KeyCode::Q},
//						{AKEYCODE_R, KeyCode::R},
//						{AKEYCODE_S, KeyCode::S},
//						{AKEYCODE_T, KeyCode::T},
//						{AKEYCODE_U, KeyCode::U},
//						{AKEYCODE_V, KeyCode::V},
//						{AKEYCODE_W, KeyCode::W},
//						{AKEYCODE_X, KeyCode::X},
//						{AKEYCODE_Y, KeyCode::Y},
//						{AKEYCODE_Z, KeyCode::Z},
//						{AKEYCODE_LEFT_BRACKET, KeyCode::LeftBracket},
//						{AKEYCODE_BACKSLASH, KeyCode::Backslash},
//						{AKEYCODE_RIGHT_BRACKET, KeyCode::RightBracket},
						{AKEYCODE_ESCAPE, KeyCode::Escape},
						{AKEYCODE_BACK, KeyCode::Back},
//						{AKEYCODE_ENTER, KeyCode::Enter},
//						{AKEYCODE_TAB, KeyCode::Tab},
//						{AKEYCODE_DEL, KeyCode::Backspace},
//						{AKEYCODE_INSERT, KeyCode::Insert},
//						{AKEYCODE_DEL, KeyCode::DelKey},
//						{AKEYCODE_SYSTEM_NAVIGATION_RIGHT, KeyCode::Right},
//						{AKEYCODE_SYSTEM_NAVIGATION_LEFT, KeyCode::Left},
//						{AKEYCODE_SYSTEM_NAVIGATION_DOWN, KeyCode::Down},
//						{AKEYCODE_SYSTEM_NAVIGATION_UP, KeyCode::Up},
//						{AKEYCODE_PAGE_UP, KeyCode::PageUp},
//						{AKEYCODE_PAGE_DOWN, KeyCode::PageDown},
//						{AKEYCODE_HOME, KeyCode::Home},
//						{AKEYCODE_CAPS_LOCK, KeyCode::CapsLock},
//						{AKEYCODE_SCROLL_LOCK, KeyCode::ScrollLock},
//						{AKEYCODE_NUM_LOCK, KeyCode::NumLock},
//						{AKEYCODE_BREAK, KeyCode::Pause},
//						{AKEYCODE_F1, KeyCode::F1},
//						{AKEYCODE_F2, KeyCode::F2},
//						{AKEYCODE_F3, KeyCode::F3},
//						{AKEYCODE_F4, KeyCode::F4},
//						{AKEYCODE_F5, KeyCode::F5},
//						{AKEYCODE_F6, KeyCode::F6},
//						{AKEYCODE_F7, KeyCode::F7},
//						{AKEYCODE_F8, KeyCode::F8},
//						{AKEYCODE_F9, KeyCode::F9},
//						{AKEYCODE_F10, KeyCode::F10},
//						{AKEYCODE_F11, KeyCode::F11},
//						{AKEYCODE_F12, KeyCode::F12},
//						{AKEYCODE_NUMPAD_0, KeyCode::KP_0},
//						{AKEYCODE_NUMPAD_1, KeyCode::KP_1},
//						{AKEYCODE_NUMPAD_2, KeyCode::KP_2},
//						{AKEYCODE_NUMPAD_3, KeyCode::KP_3},
//						{AKEYCODE_NUMPAD_4, KeyCode::KP_4},
//						{AKEYCODE_NUMPAD_5, KeyCode::KP_5},
//						{AKEYCODE_NUMPAD_6, KeyCode::KP_6},
//						{AKEYCODE_NUMPAD_7, KeyCode::KP_7},
//						{AKEYCODE_NUMPAD_8, KeyCode::KP_8},
//						{AKEYCODE_NUMPAD_9, KeyCode::KP_9},
//						{AKEYCODE_NUMPAD_DOT, KeyCode::KP_Decimal},
//						{AKEYCODE_NUMPAD_DIVIDE, KeyCode::KP_Divide},
//						{AKEYCODE_NUMPAD_MULTIPLY, KeyCode::KP_Multiply},
//						{AKEYCODE_NUMPAD_SUBTRACT, KeyCode::KP_Subtract},
//						{AKEYCODE_NUMPAD_ADD, KeyCode::KP_Add},
//						{AKEYCODE_NUMPAD_ENTER, KeyCode::KP_Enter},
//						{AKEYCODE_NUMPAD_EQUALS, KeyCode::KP_Equal},
//						{AKEYCODE_SHIFT_LEFT, KeyCode::LeftShift},
//						{AKEYCODE_CTRL_LEFT, KeyCode::LeftControl},
//						{AKEYCODE_ALT_LEFT, KeyCode::LeftAlt},
//						{AKEYCODE_SHIFT_RIGHT, KeyCode::RightShift},
//						{AKEYCODE_CTRL_RIGHT, KeyCode::RightControl},
//						{AKEYCODE_ALT_RIGHT, KeyCode::RightAlt}
				};

		auto key_it = key_lookup.find(key);

		if (key_it == key_lookup.end())
		{
			return KeyCode::Unknown;
		}

		return key_it->second;
	}

	inline KeyAction translate_key_action(int action)
	{
		if (action == AKEY_STATE_DOWN)
		{
			return KeyAction::Down;
		}
		else if (action == AKEY_STATE_UP)
		{
			return KeyAction::Up;
		}

		return KeyAction::Unknown;
	}

	class EchoVkNativePlatform
	{
	public:
		EchoVkNativePlatform(struct android_app* _app);
		~EchoVkNativePlatform();

		ExitCode init();
		ExitCode gameLoop();
		void terminate(ExitCode code);
        void resetWindow(ANativeWindow* window);
		void resize(uint32_t width, uint32_t height);

		void receive_mov_dir_button_data();

		void receive_and_run_cmd();

		void send_render_status_data();

		void set_surface_ready() { surface_ready = true; }
        void set_surface_void() { surface_ready = false; }
		bool get_surface_ready() { return surface_ready; }
		void set_running(bool b) { running = b; }
		void set_close_requested(bool b) { close_requested = b; }

		void process_android_input_events(void);

		void key_input_event(const KeyInputEvent &input_event);
		void touch_input_event(const TouchInputEvent &input_event);
	private:
		android_app* app;

        std::string assetPath;
		std::string log_output;

		bool surface_ready{false};
		bool running{true};                  /* App is currently in focus at an operating system level */
		bool close_requested{false};

	};
}

#endif
