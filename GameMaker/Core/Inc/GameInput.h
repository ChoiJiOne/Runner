#pragma once

#include <array>
#include <functional>

#include "GameMath.h"


namespace GameMaker
{
/**
 * @brief 입력 상태를 나타냅니다.
 *
 * @note 프레임에 따른 입력 상태입니다.
 * --------------------------------------
 * | 이전 프레임 | 현재 프레임 | 입력 상태 |
 * --------------------------------------
 * |     0      |     0      | NONE     |
 * |     0      |     1      | PRESSED  |
 * |     1      |     0      | RELEASED |
 * |     1      |     1      | HELD     |
 * --------------------------------------
 */
enum class EPressState : int32_t
{
	NONE     = 0x00,
	PRESSED  = 0x01,
	RELEASED = 0x02,
	HELD     = 0x03
};


/**
 * @brief 마우스의 버튼의 종류입니다.
 */
enum class EMouse : int32_t
{
	LEFT   = (1 << ((1) - 1)),
	MIDDLE = (1 << ((2) - 1)),
	RIGHT  = (1 << ((3) - 1)),
};


/**
 * @brief 키보드의 키 값입니다.
 *
 * @see https://wiki.libsdl.org/SDL_Scancode
 */
enum class EKey : int32_t
{
	KEY_UNKNOWN = 0,
	KEY_A = 4,
	KEY_B = 5,
	KEY_C = 6,
	KEY_D = 7,
	KEY_E = 8,
	KEY_F = 9,
	KEY_G = 10,
	KEY_H = 11,
	KEY_I = 12,
	KEY_J = 13,
	KEY_K = 14,
	KEY_L = 15,
	KEY_M = 16,
	KEY_N = 17,
	KEY_O = 18,
	KEY_P = 19,
	KEY_Q = 20,
	KEY_R = 21,
	KEY_S = 22,
	KEY_T = 23,
	KEY_U = 24,
	KEY_V = 25,
	KEY_W = 26,
	KEY_X = 27,
	KEY_Y = 28,
	KEY_Z = 29,
	KEY_1 = 30,
	KEY_2 = 31,
	KEY_3 = 32,
	KEY_4 = 33,
	KEY_5 = 34,
	KEY_6 = 35,
	KEY_7 = 36,
	KEY_8 = 37,
	KEY_9 = 38,
	KEY_0 = 39,
	KEY_RETURN = 40,
	KEY_ESCAPE = 41,
	KEY_BACKSPACE = 42,
	KEY_TAB = 43,
	KEY_SPACE = 44,
	KEY_MINUS = 45,
	KEY_EQUALS = 46,
	KEY_LEFTBRACKET = 47,
	KEY_RIGHTBRACKET = 48,
	KEY_BACKSLASH = 49,
	KEY_NONUSHASH = 50,
	KEY_SEMICOLON = 51,
	KEY_APOSTROPHE = 52,
	KEY_GRAVE = 53,
	KEY_COMMA = 54,
	KEY_PERIOD = 55,
	KEY_SLASH = 56,
	KEY_CAPSLOCK = 57,
	KEY_F1 = 58,
	KEY_F2 = 59,
	KEY_F3 = 60,
	KEY_F4 = 61,
	KEY_F5 = 62,
	KEY_F6 = 63,
	KEY_F7 = 64,
	KEY_F8 = 65,
	KEY_F9 = 66,
	KEY_F10 = 67,
	KEY_F11 = 68,
	KEY_F12 = 69,
	KEY_PRINTSCREEN = 70,
	KEY_SCROLLLOCK = 71,
	KEY_PAUSE = 72,
	KEY_INSERT = 73,
	KEY_HOME = 74,
	KEY_PAGEUP = 75,
	KEY_DELETE = 76,
	KEY_END = 77,
	KEY_PAGEDOWN = 78,
	KEY_RIGHT = 79,
	KEY_LEFT = 80,
	KEY_DOWN = 81,
	KEY_UP = 82,
	KEY_NUMLOCKCLEAR = 83,
	KEY_KP_DIVIDE = 84,
	KEY_KP_MULTIPLY = 85,
	KEY_KP_MINUS = 86,
	KEY_KP_PLUS = 87,
	KEY_KP_ENTER = 88,
	KEY_KP_1 = 89,
	KEY_KP_2 = 90,
	KEY_KP_3 = 91,
	KEY_KP_4 = 92,
	KEY_KP_5 = 93,
	KEY_KP_6 = 94,
	KEY_KP_7 = 95,
	KEY_KP_8 = 96,
	KEY_KP_9 = 97,
	KEY_KP_0 = 98,
	KEY_KP_PERIOD = 99,
	KEY_NONUSBACKSLASH = 100,
	KEY_APPLICATION = 101,
	KEY_POWER = 102,
	KEY_KP_EQUALS = 103,
	KEY_F13 = 104,
	KEY_F14 = 105,
	KEY_F15 = 106,
	KEY_F16 = 107,
	KEY_F17 = 108,
	KEY_F18 = 109,
	KEY_F19 = 110,
	KEY_F20 = 111,
	KEY_F21 = 112,
	KEY_F22 = 113,
	KEY_F23 = 114,
	KEY_F24 = 115,
	KEY_EXECUTION = 116,
	KEY_HELP = 117,
	KEY_MENU = 118,
	KEY_SELECT = 119,
	KEY_STOP = 120,
	KEY_AGAIN = 121,
	KEY_UNDO = 122,
	KEY_CUT = 123,
	KEY_COPY = 124,
	KEY_PASTE = 125,
	KEY_FIND = 126,
	KEY_MUTE = 127,
	KEY_VOLUMEUP = 128,
	KEY_VOLUMEDOWN = 129,
	KEY_KP_COMMA = 133,
	KEY_KP_EQUALSAS400 = 134,
	KEY_INTERNATIONAL1 = 135,
	KEY_INTERNATIONAL2 = 136,
	KEY_INTERNATIONAL3 = 137,
	KEY_INTERNATIONAL4 = 138,
	KEY_INTERNATIONAL5 = 139,
	KEY_INTERNATIONAL6 = 140,
	KEY_INTERNATIONAL7 = 141,
	KEY_INTERNATIONAL8 = 142,
	KEY_INTERNATIONAL9 = 143,
	KEY_LANG1 = 144,
	KEY_LANG2 = 145,
	KEY_LANG3 = 146,
	KEY_LANG4 = 147,
	KEY_LANG5 = 148,
	KEY_LANG6 = 149,
	KEY_LANG7 = 150,
	KEY_LANG8 = 151,
	KEY_LANG9 = 152,
	KEY_ALTERASE = 153,
	KEY_SYSREQ = 154,
	KEY_CANCEL = 155,
	KEY_CLEAR = 156,
	KEY_PRIOR = 157,
	KEY_RETURN2 = 158,
	KEY_SEPARATOR = 159,
	KEY_OUT = 160,
	KEY_OPER = 161,
	KEY_CLEARAGAIN = 162,
	KEY_CRSEL = 163,
	KEY_EXSEL = 164,
	KEY_KP_00 = 176,
	KEY_KP_000 = 177,
	KEY_THOUSANDSSEPARATOR = 178,
	KEY_DECIMALSEPARATOR = 179,
	KEY_CURRENCYUNIT = 180,
	KEY_CURRENCYSUBUNIT = 181,
	KEY_KP_LEFTPAREN = 182,
	KEY_KP_RIGHTPAREN = 183,
	KEY_KP_LEFTBRACE = 184,
	KEY_KP_RIGHTBRACE = 185,
	KEY_KP_TAB = 186,
	KEY_KP_BACKSPACE = 187,
	KEY_KP_A = 188,
	KEY_KP_B = 189,
	KEY_KP_C = 190,
	KEY_KP_D = 191,
	KEY_KP_E = 192,
	KEY_KP_F = 193,
	KEY_KP_XOR = 194,
	KEY_KP_POWER = 195,
	KEY_KP_PERCENT = 196,
	KEY_KP_LESS = 197,
	KEY_KP_GREATER = 198,
	KEY_KP_AMPERSAND = 199,
	KEY_KP_DBLAMPERSAND = 200,
	KEY_KP_VERTICALBAR = 201,
	KEY_KP_DBLVERTICALBAR = 202,
	KEY_KP_COLON = 203,
	KEY_KP_HASH = 204,
	KEY_KP_SPACE = 205,
	KEY_KP_AT = 206,
	KEY_KP_EXCLAM = 207,
	KEY_KP_MEMSTORE = 208,
	KEY_KP_MEMRECALL = 209,
	KEY_KP_MEMCLEAR = 210,
	KEY_KP_MEMADD = 211,
	KEY_KP_MEMSUBTRACT = 212,
	KEY_KP_MEMMULTIPLY = 213,
	KEY_KP_MEMDIVIDE = 214,
	KEY_KP_PLUSMINUS = 215,
	KEY_KP_CLEAR = 216,
	KEY_KP_CLEARENTRY = 217,
	KEY_KP_BINARY = 218,
	KEY_KP_OCTAL = 219,
	KEY_KP_DECIMAL = 220,
	KEY_KP_HEXADECIMAL = 221,
	KEY_LCTRL = 224,
	KEY_LSHIFT = 225,
	KEY_LALT = 226,
	KEY_LGUI = 227,
	KEY_RCTRL = 228,
	KEY_RSHIFT = 229,
	KEY_RALT = 230,
	KEY_RGUI = 231,
	KEY_MODE = 257,
	KEY_AUDIONEXT = 258,
	KEY_AUDIOPREV = 259,
	KEY_AUDIOSTOP = 260,
	KEY_AUDIOPLAY = 261,
	KEY_AUDIOMUTE = 262,
	KEY_MEDIASELECT = 263,
	KEY_WWW = 264,
	KEY_MAIL = 265,
	KEY_CALCULATOR = 266,
	KEY_COMPUTER = 267,
	KEY_AC_SEARCH = 268,
	KEY_AC_HOME = 269,
	KEY_AC_BACK = 270,
	KEY_AC_FORWARD = 271,
	KEY_AC_STOP = 272,
	KEY_AC_REFRESH = 273,
	KEY_AC_BOOKMARKS = 274,
	KEY_BRIGHTNESSDOWN = 275,
	KEY_BRIGHTNESSUP = 276,
	KEY_DISPLAYSWITCH = 277,
	KEY_KBDILLUMTOGGLE = 278,
	KEY_KBDILLUMDOWN = 279,
	KEY_KBDILLUMUP = 280,
	KEY_EJECT = 281,
	KEY_SLEEP = 282,
	KEY_APP1 = 283,
	KEY_APP2 = 284,
	KEY_AUDIOREWIND = 285,
	KEY_AUDIOFASTFORWARD = 286,
	KEY_SOFTLEFT = 287,
	KEY_SOFTRIGHT = 288,
	KEY_CALL = 289,
	KEY_ENDCALL = 290,
	KEY_NUM_SCANCODES = 512
};


/**
 * @brief 윈도우 이벤트의 키 값입니다.
 */
using WindowEventID = int32_t;


/**
 * @brief 윈도우 이벤트 값입니다.
 *
 * @note https://wiki.libsdl.org/SDL_WindowEventID
 */
enum class EWindowEvent : int32_t
{
	NONE            = 0x00,
	SHOWN           = 0x01,
	HIDDEN          = 0x02,
	EXPOSED         = 0x03,
	MOVED           = 0x04,
	RESIZED         = 0x05,
	SIZE_CHANGED    = 0x06,
	MINIMIZED       = 0x07,
	MAXIMIZED       = 0x08,
	RESTORED        = 0x09,
	ENTER           = 0x0A,
	LEAVE           = 0x0B,
	FOCUS_GAINED    = 0x0C,
	FOCUS_LOST      = 0x0D,
	CLOSE           = 0x0E,
	TAKE_FOCUS      = 0x0F,
	HIT_TEST        = 0x10,
	ICCPROF_CHANGED = 0x11,
	DISPLAY_CHANGED = 0x12
};


/**
 * @brief 게임의 입력 상태를 관리합니다.
 * 
 * @note 이 클래스의 모든 멤버 함수 및 변수는 모두 정적(static) 타입입니다.
 */
class GameInput
{
public:
	/**
	 * @brief 가상 키의 입력 상태를 확인합니다.
	 *
	 * @param key 입력 상태를 확인할 키 값입니다.
	 *
	 * @return 키의 입력 상태를 반환합니다.
	 */
	static EPressState GetKeyPressState(const EKey& key);


	/**
	 * @brief 마우스의 입력 상태를 확인합니다.
	 *
	 * @param mouse 입력 상태를 확인할 마우스 버튼입니다.
	 *
	 * @return 마우스의 입력 상태를 반환합니다.
	 */
	static EPressState GetMousePressState(const EMouse& mouse);


	/**
	 * @brief 윈도우 이벤트 액션을 추가합니다.
	 *
	 * @param windowEvent 동작할 액션에 대응하는 윈도우 이벤트입니다.
	 * @param eventAction 이벤트 액션에 대응하는 액션입니다.
	 * @param bIsActive 윈도우 이벤트 액션 활성화 여부입니다. 기본 값은 true입니다.
	 *
	 * @return 윈도우 이벤트의 ID 값을 반환합니다.
	 */
	static WindowEventID AddWindowEventAction(const EWindowEvent& windowEvent, const std::function<void()>& eventAction, bool bIsActive = true);


	/**
	 * @brief 윈도우 이벤트 액션을 삭제합니다.
	 *
	 * @param windowEventID 윈도우 이벤트 액션의 ID 값입니다.
	 *
	 * @note 시그니처에 대응하는 윈도우 이벤트가 존재하지 않으면 아무 동작도 수행하지 않습니다.
	 */
	static void DeleteWindowEventAction(const WindowEventID& windowEventID);


	/**
	 * @brief 윈도우 이벤트 액션의 활성화 여부를 설정합니다.
	 *
	 * @param windowEventID 윈도우 이벤트 액션의 ID 값입니다.
	 * @param bIsActive 윈도우 이벤트의 활성화 여부입니다.
	 *
	 * @note 시그니처에 대응하는 윈도우 이벤트가 존재하지 않으면 아무 동작도 수행하지 않습니다.
	 */
	static void SetActiveWindowEventAction(const WindowEventID& windowEventID, bool bIsActive);
	

private:
	/**
	 * @brief 게임 엔진에서 접근할 수 있도록 friend 선언을 추가합니다.
	 */
	friend class GameEngine;


	/**
	 * @brief 윈도우 이벤트에 대응하는 액션입니다.
	 */
	struct WindowEventAction
	{
		bool				  bIsActive;         // 윈도우 이벤트의 활성화 여부입니다.
		EWindowEvent		  windowEvent;       // 윈도우 이벤트입니다.
		std::function<void()> windowEventAction; // 윈도우 이벤트에 대응하는 액션입니다.
	};


	/**
	 * @brief 키보드의 키 값 상태입니다.
	 */
	struct KeyboardState
	{
		static const int32_t BUFFER_SIZE = 512;        // 키보드의 키 값 상태를 저장하는 버퍼의 크기입니다.
		std::array<uint8_t, BUFFER_SIZE> keybordState; // 키보드의 키 값 상태를 저장하는 버퍼입니다.
	};


	/**
	 * @brief 마우스의 상태입니다.
	 */
	struct MouseState
	{
		uint32_t state; // 마우스 버튼의 상태입니다.
		Vec2i position; // 마우스 버튼의 위치입니다.
	};


	/**
	 * @brief 입력 상태를 업데이트합니다.
	 */
	static void Tick();


	/**
	 * @brief 윈도우 이벤트에 대응하는 액션들을 실행합니다.
	 *
	 * @param windowEvent 실행할 윈도우 이벤트입니다.
	 */
	static void ExecuteWindowEventAction(const EWindowEvent& windowEvent);


	/**
	 * @brief 특정 키가 눌렸는지 확인합니다.
	 *
	 * @param keyboardState 검사를 수행할 키보드 상태입니다.
	 * @param keyCode 눌렸는지 확인할 키의 코드값입니다.
	 *
	 * @return 키가 눌렸다면 true, 그렇지 않으면 false를 반환합니다.
	 */
	static bool IsPressKey(const KeyboardState& keyboardState, const EKey& key);


	/**
	 * @brief 특정 마우스 버튼이 눌렸는지 확인합니다.
	 *
	 * @param mouseState 검사를 수행할 마우스 상태입니다.
	 * @param mouseButton 눌렸는지 확인할 마우스 버튼입니다.
	 *
	 * @return 마우스 버튼이 눌렸다면 true, 그렇지 않으면 false를 반환합니다.
	 */
	static bool IsPressMouse(const MouseState& mouseState, const EMouse& mouse);


private:
	/**
     * @brief 입력 처리 매니저의 업데이트 이전 키보드 입력 상태입니다.
     */
	static KeyboardState prevKeyboardState_;


	/**
     * @brief 입력 처리 매니저의 업데이트 이후 키보드 입력 상태입니다.
     */
	static KeyboardState currKeyboardState_;


	/**
     * @brief 입력 처리 매니저의 업데이트 이전의 마우스 상태입니다.
     */
	static MouseState prevMouseState_;


	/**
     * @brief 입력 처리 매니저의 업데이트 이후의 마우스 상태입니다.
     */
	static MouseState currMouseState_;


	/**
     * @brief 입력 처리 매니저의 이벤트 액션 배열의 크기입니다.
     */
	static uint32_t windowEventActionSize_;


	/**
     * @brief 입력 처리 매니저의 이벤트 액션 배열의 최대 크기입니다.
     */
	static const uint32_t MAX_EVENT_ACTION_SIZE = 100;


	/**
     * @brief 입력 처리 매니저의 이벤트 액션 배열입니다.
     */
	static std::array<WindowEventAction, MAX_EVENT_ACTION_SIZE> windowEventActions_;
};

};