#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "platform_android.h"

#include "EchoDataStream.h"
#include "EchoSystemConfigFile.h"
#include "EchoStringConverter.h"

// c++ std:: stl
#include <string>
#include <thread>

#include "EchocJsonHelper.h"
#include "Android/EchoAndroidJavaEnv.h"

#include "EchoProtocolComponent.h"
#include "../appview.h"
#include "../AppBase.h"

using namespace Echo;

static Echo::ECHO_GRAPHIC_PARAM		g_graphicParam;
static AAssetManager* g_assetManager;

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "padbo2", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "padbo2", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "padbo2", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "padbo2", __VA_ARGS__))

//class EnvTool
//{
//private:
//	JNIEnv * env;
//	bool isAttachedThread;
//	static JavaVM *javaVM;
//
//public:
//	static void setVM(JavaVM * vm) { javaVM = vm; }
//	JNIEnv* getEnv() { return env; }
//	EnvTool()
//	{
//		int ret = javaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
//		if (ret < 0)
//		{
//			ret = javaVM->AttachCurrentThread(&env, NULL);
//			if (ret < 0)
//			{
//				env = NULL;
//			}
//			isAttachedThread = true;
//		}
//		else
//			isAttachedThread = false;
//	}
//	~EnvTool()
//	{
//		if (isAttachedThread)
//		{
//			javaVM->DetachCurrentThread();
//			isAttachedThread = false;
//		}
//	}
//};
//JavaVM * EnvTool::javaVM = NULL;

extern "C" {

	class GameApp
	{
	public:
		const char * getPlatformABI();
		GameApp();
		~GameApp();
	};

	/*此简单函数返回平台 ABI，此动态本地库为此平台 ABI 进行编译。*/
	const char * GameApp::getPlatformABI()
	{
#if defined(__arm__)
#if defined(__ARM_ARCH_7A__)
#if defined(__ARM_NEON__)
#define ABI "armeabi-v7a/NEON"
#else
#define ABI "armeabi-v7a"
#endif
#else
#define ABI "armeabi"
#endif
#elif defined(__i386__)
#define ABI "x86"
#else
#define ABI "unknown"
#endif
		LOGI("This dynamic shared library is compiled with ABI: %s", ABI);
		return "This native library is compiled with ABI: %s" ABI ".";
	}

	void GameApp()
	{
	}

	GameApp::GameApp()
	{
	}

	GameApp::~GameApp()
	{
	}

	JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void *reserved) //这是JNI_OnLoad的声明，必须按照这样的方式声明
	{
		//EnvTool::setVM(vm);
		Echo::AndroidJavaEnv::setJavaEnv(vm);
		return JNI_VERSION_1_4; //这里很重要，必须返回版本，否则加载会失败。
	}

#define JNI_FUNC( FUNC ) Java_net_pixelgame_padbo2_##FUNC

	static const int array_max_cnt = 9;
	static std::string g_strInfoArray[9];
	static jobject jvmGameApp = 0;

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_terminate)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
		{
			gpAppView->onRelease(true);
			exit(0);
		}
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_init)(JNIEnv * env, jobject obj, jobjectArray infoArray)
	{
		jsize count = env->GetArrayLength(infoArray);
		for (jsize i=0; i<count && i< array_max_cnt; i++)
		{
			jstring jstrValue = (jstring)env->GetObjectArrayElement(infoArray, i);
			const char * szValue = env->GetStringUTFChars(jstrValue, 0);
			g_strInfoArray[i].assign(szValue);
			env->ReleaseStringUTFChars(jstrValue, szValue);
		}

		Echo::GetArchivePack(g_strInfoArray[0].c_str(), g_strInfoArray[1].c_str());
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_resize)(JNIEnv * env, jobject obj, jint width, jint height)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
		{
			gpAppView->onResize(width, height, 0, 0, width, height);
		}
		else
		{
			g_graphicParam.eClass = Echo::ECHO_DEVICE_GLES3;

			snprintf(g_graphicParam.renderer,	sizeof(g_graphicParam.renderer) - 1,	"%s", g_strInfoArray[3].c_str());
			snprintf(g_graphicParam.vendor,		sizeof(g_graphicParam.vendor) - 1,		"%s", g_strInfoArray[4].c_str());
			snprintf(g_graphicParam.cpuInfo,	sizeof(g_graphicParam.cpuInfo) - 1,		"%s", g_strInfoArray[7].c_str());
			snprintf(g_graphicParam.model,		sizeof(g_graphicParam.model) - 1,		"%s", g_strInfoArray[8].c_str());

			init_appview(true, true, g_strInfoArray[0].c_str(), g_strInfoArray[1].c_str(), &g_graphicParam, width, height);
			gpAppView = get_appview();
			gpAppView->onResize(width, height, 0, 0, width, height);
		}
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_step)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
		{
			gpAppView->onTick();
		}
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_pause)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
			gpAppView->onPause();
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_resume)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
			gpAppView->onResume();
	}

	JNIEXPORT void JNICALL JNI_FUNC(TcgMainActivity_setStateStart)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
			gpAppView->onStart();
	}

	JNIEXPORT void JNICALL JNI_FUNC(TcgMainActivity_setStateStop)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
			gpAppView->onStop();
	}

	bool joystick_mov = false;
	bool joystick_dir = false;

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_onTouch)(
		JNIEnv * env,
		jobject obj,
		jint _action,
		jint _touchId,
		jint _touchCnt,
		jfloat _x,
		jfloat _y)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return;

		if (0 == _action)
		{
			if (100 == _touchId)
			{
				joystick_mov = true;
			}
			else if (200 == _touchId)
			{
				joystick_dir = true;
			}
		}
		else if (1 == _action)
		{
			if (100 == _touchId)
			{
				joystick_mov = false;
				gpAppView->setMoveDir(0.0f, 0.0f);
			}
			else if (200 == _touchId)
			{
				joystick_dir = false;
				gpAppView->setTurnDir(0.0f, 0.0f);
			}
		}
		else if (2 == _action)
		{
			if (100 == _touchId && joystick_mov)
			{
				gpAppView->setMoveDir(_x * 2, _y * 2);
			}

			if (200 == _touchId && joystick_dir)
			{
				gpAppView->setTurnDir(-_x * 2, _y * 2);
			}
		}


        //TODO(yanghang): Temp engine internal input event push to FGUI.
        {
            int touch_down = 0;
            int touch_up = 1;
            int touch_move = 2;

            Echo::Vector2 mouse( _x, _y);
            if(_action == touch_down)
            {
				gpAppView->onMouseLeftDown(mouse);

            }
            else if (_action == touch_up)
            {
				gpAppView->onMouseLeftUp(mouse);
            }
            else if(_action == touch_move)
            {
				gpAppView->onMouseMove(mouse);
            }
        }
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_onTextProc)(JNIEnv * env, jobject obj, jstring _text)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return;

		// jstringTostring在函数内进行堆分配！！!不太合理
		const char * szText = env->GetStringUTFChars(_text, 0);

		std::string test = szText;

		if (test.size() > 0)
		{
			gpAppView->commandParse(test);
		}
		env->ReleaseStringUTFChars(_text, szText);
	}

	JNIEXPORT void JNICALL JNI_FUNC(TcgMainActivity_regEnv)(JNIEnv * env, jobject obj)
	{
// 		EnvTool envTool;
// 		JNIEnv *jni = envTool.getEnv();
		Echo::AndroidJavaEnv envTool;
		Echo::AndroidJavaEnv::SetJavaMainActivity(obj);
		JNIEnv *jni = envTool.GetJavaEnv();
		jvmGameApp = jni->NewGlobalRef((jobject)obj);
	}

	JNIEXPORT jint JNICALL JNI_FUNC(GameLib_getFPS)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return 0;

		return gpAppView->getFps();
	}

	JNIEXPORT jstring JNICALL JNI_FUNC(GameLib_getCameraPosition)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return env->NewStringUTF("");

		std::string ssCamPos = gpAppView->getCameraPosition();
		return env->NewStringUTF(ssCamPos.c_str());
	}

	JNIEXPORT jstring JNICALL JNI_FUNC(GameLib_getResMemUsageStr)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return env->NewStringUTF("");

		std::string ssUsage = gpAppView->getResMemUsageStr();
		return env->NewStringUTF(ssUsage.c_str());
	}

	JNIEXPORT jint JNICALL JNI_FUNC(GameLib_getDrawCall)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return 0;

		return gpAppView->getDrawCall();
	}

	JNIEXPORT jint JNICALL JNI_FUNC(GameLib_getTriangleCnt)(JNIEnv * env, jobject obj)
	{
		auto gpAppView = get_appview();
		if (!gpAppView)
			return 0;

		return gpAppView->getTriangleCnt();
	}

	JNIEXPORT jint JNICALL JNI_FUNC(GameLib_getEchoRenderMode)(JNIEnv *env, jclass clazz)
	{
        ECHO_DEVICE_CLASS deviceType = ECHO_DEVICE_NULL;
		SystemConfigFile cfg((IArchive*)Echo::GetArchivePack(g_strInfoArray[0].c_str(), g_strInfoArray[1].c_str()));
		cfg.load("config/EchoEngine.cfg");
		String sPlatform = "Android";

		const SettingsMultiMap* pSetMap = cfg.getSettingsMap(sPlatform);
		if (pSetMap)
		{
			auto it = pSetMap->find("RenderAPI");
			if (it != pSetMap->end())
			{
				deviceType = (ECHO_DEVICE_CLASS)StringConverter::parseUnsignedInt(it->second);
				g_graphicParam.eClass = deviceType;
			}
		}
		return deviceType;
	}

	JNIEXPORT void JNICALL JNI_FUNC(GameLib_nextScene)(JNIEnv * env, jobject obj)
	{

	}


	JNIEXPORT void JNICALL JNI_FUNC(GameLib_lastScene)(JNIEnv * env, jobject obj)
	{

	}
    JNIEXPORT void JNICALL JNI_FUNC(GameLib_camModeChange)(JNIEnv * env, jobject obj)
    {
        auto gpAppView = get_appview();
        if (!gpAppView)
            return;

        return gpAppView->commandParse("npc");

    }
    JNIEXPORT void JNICALL JNI_FUNC(GameLib_camSpeedUp)(JNIEnv * env, jobject obj)
    {
        auto gpAppView = get_appview();
        if (!gpAppView)
            return;

        return gpAppView->commandParse("csUp");
    }
    JNIEXPORT void JNICALL JNI_FUNC(GameLib_camSpeedReduction)(JNIEnv * env, jobject obj)
    {
        auto gpAppView = get_appview();
        if (!gpAppView)
            return;

        return gpAppView->commandParse("csDown");
    }
}

const char* GetAssetPath()
{
	return g_strInfoArray[0].c_str();
}

const char * GetDataPath()
{
	return g_strInfoArray[1].c_str();
}

const char * GetAppPath()
{
	return g_strInfoArray[2].c_str();
}

AAssetManager * GetAssetManager()
{
	return g_assetManager;
}

void SetOpenGLESShareContext(bool value)
{
	Echo::AndroidJavaEnv envTool;
	JNIEnv *jni = envTool.GetJavaEnv();
	jclass clazz = jni->GetObjectClass(jvmGameApp);
	jmethodID methodID = jni->GetStaticMethodID(clazz, "SetShareContext", "(Z)V");
	if (jni->ExceptionCheck()) {
		assert(false);
	}

	jboolean jValue = value ? 1 : 0;
	jni->CallStaticVoidMethod(clazz, methodID, jValue);
	if (jni->ExceptionCheck()) {
		assert(false);
	}
}

Echo::AndroidJavaEnv * pFMODEnvTool = nullptr;
void AttachNativeThreadFMOD()
{
	if (pFMODEnvTool)
		delete  pFMODEnvTool;

	pFMODEnvTool = new Echo::AndroidJavaEnv();
	pFMODEnvTool->GetJavaEnv();
}

void DetachNativeThreadFMOD()
{
	if (pFMODEnvTool)
	{
		delete  pFMODEnvTool;
		pFMODEnvTool = nullptr;
	}
}

namespace appui
{
	void start_input(JNIEnv * _pEnv, jstring _text)
	{
		//Default class retrieval
		jclass clazz = _pEnv->FindClass("net/pixelgame/xxsj/TcgMainActivity");
		jmethodID methodID = _pEnv->GetStaticMethodID(clazz, "CallInput", "(Ljava/lang/String;)V");
		_pEnv->CallStaticVoidMethod(clazz, methodID, _text);
		_pEnv->DeleteLocalRef(clazz);
	}

	void on_input_begin(int _x, int _y, const char* _szText)
	{
		Echo::AndroidJavaEnv envTool;
		JNIEnv *pEnv = envTool.GetJavaEnv();
		jstring textEdit = pEnv->NewStringUTF( _szText );
		start_input(pEnv, textEdit);
		pEnv->DeleteLocalRef(textEdit);
	}
}

namespace EchoVk
{

/**
 * @brief Process android lifecycle events
 *
 * @param app Android app context
 * @return true Events processed
 * @return false Program should close
 */
    inline bool process_android_events(android_app *app)
    {
        android_poll_source *source;

		int events;

        while (ALooper_pollAll(
				0,
				nullptr,
				&events,
				(void **) &source
				) >= 0)
        {
            if (source)
            {
                source->process(app, source);
            }

            if (app->destroyRequested != 0)
            {
                return false;
            }
        }
        if (app->userData)
        {
            auto platform = reinterpret_cast<EchoVkNativePlatform *>(app->userData);
            platform->process_android_input_events();
        }
        return true;
    }

	inline TouchAction translate_touch_action(int action)
	{
		action &= AMOTION_EVENT_ACTION_MASK;
		if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN)
		{
			return TouchAction::Down;
		}
		else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP)
		{
			return TouchAction::Up;
		}
		else if (action == AMOTION_EVENT_ACTION_CANCEL)
		{
			return TouchAction::Cancel;
		}
		else if (action == AMOTION_EVENT_ACTION_MOVE)
		{
			return TouchAction::Move;
		}

		return TouchAction::Unknown;
	}

    void on_app_cmd(android_app *app, int32_t cmd)
    {
        auto platform = reinterpret_cast<EchoVkNativePlatform *>(app->userData);
        assert(platform && "Platform is not valid");

        switch (cmd)
        {
            case APP_CMD_INIT_WINDOW: {
                platform->resetWindow(app->window);
                platform->resize(ANativeWindow_getWidth(app->window),
                                 ANativeWindow_getHeight(app->window));
                platform->set_surface_ready();
                break;
            }
            case APP_CMD_TERM_WINDOW: {
                platform->set_surface_void();
                break;
            }
            case APP_CMD_CONTENT_RECT_CHANGED: {
                // Get the new size
                auto width  = app->contentRect.right - app->contentRect.left;
                auto height = app->contentRect.bottom - app->contentRect.top;
				if (platform->get_surface_ready())
				{
					platform->resize(width, height);
				}
                break;
            }
            case APP_CMD_PAUSE: {
                if (get_appview()) {
                    get_appview()->onPause();
                }
                platform->set_running(false);
                break;
            }
            case APP_CMD_RESUME: {
                if (get_appview()) {
                    get_appview()->onResume();
                }
                platform->set_running(true);
                break;
            }
            case APP_CMD_DESTROY:{
                auto gpAppView = get_appview();
                if (gpAppView)
                {
                    gpAppView->onRelease(true);
					platform->set_close_requested(true);
                }
                break;
            }
            case APP_CMD_GAINED_FOCUS:
            case APP_CMD_LOST_FOCUS:
            default:{
                break;
            }
        }
    }

    EchoVkNativePlatform::EchoVkNativePlatform(struct android_app* _app)
    : app(_app)
    { }

    EchoVkNativePlatform::~EchoVkNativePlatform()
	{
        app = nullptr;
    }

    ExitCode EchoVkNativePlatform::init()
	{
        g_assetManager = app->activity->assetManager;
        Echo::AndroidJavaEnv::SetJavaMainActivity(app->activity->javaGameActivity);

        //control envTool lifetime
        {
            Echo::AndroidJavaEnv envTool;
            JNIEnv *env = envTool.GetJavaEnv();
            jclass cls = env->GetObjectClass(app->activity->javaGameActivity);
            jmethodID id = env->GetMethodID(cls, "getGpuInfo", "()[Ljava/lang/String;");
            auto vara = reinterpret_cast<jobjectArray>(env->CallObjectMethod(
                    app->activity->javaGameActivity,
                    id));
            jsize len = env->GetArrayLength(vara);
            jstring *jstr = new jstring[len];

            char **resultChar = (char **) malloc(len * sizeof(char *));
            for (int i = 0; i < len; ++i) {
                jstr[i] = reinterpret_cast<jstring>(env->GetObjectArrayElement(vara, i));
                resultChar[i] = (char *) env->GetStringUTFChars(jstr[i], nullptr);
            }

            jfieldID jAssetPathId = env->GetStaticFieldID(cls, "assetPath", "Ljava/lang/String;");
            jfieldID jDataFilePathId = env->GetStaticFieldID(cls, "dataFilePath",
                                                             "Ljava/lang/String;");

            auto jAssetPath = (jstring) (env->GetStaticObjectField(cls, jAssetPathId));
            auto jDataFilePath = (jstring) (env->GetStaticObjectField(cls, jDataFilePathId));

            resultChar[0] = (char *) env->GetStringUTFChars(jAssetPath, nullptr);
            resultChar[1] = (char *) env->GetStringUTFChars(jDataFilePath, nullptr);

            g_strInfoArray[0] = resultChar[0];
            g_strInfoArray[1] = resultChar[1];

            g_strInfoArray[7] = resultChar[7];
            g_strInfoArray[8] = resultChar[8];

            delete[] jstr;
            free(resultChar);
        }
		Echo::GetArchivePack(g_strInfoArray[0].c_str(), g_strInfoArray[1].c_str());

        app->onAppCmd = on_app_cmd;
        app->userData = this;

        // Wait until the android window is loaded before allowing the app to continue
        LOGI("Waiting on window surface to be ready");
        do
        {
            if (!process_android_events(app))
            {
                // Android requested for the app to close
                LOGI("Android app has been destroyed by the OS");
                return ExitCode::Close;
            }
        } while (!surface_ready);

		return ExitCode::Success;
    }

    ExitCode EchoVkNativePlatform::gameLoop()
	{
		auto gpAppView = get_appview();
		while(!close_requested) {
            if (!surface_ready) {
                // Wait until the android window is loaded before allowing the app to continue
                LOGI("Waiting on window surface to be ready");
                do {
                    if (!process_android_events(app)) {
                        // Android requested for the app to close
                        LOGI("Android app has been destroyed by the OS");
                        return ExitCode::Close;
                    }
                } while (!surface_ready);
            }

			while (running) {
				if (gpAppView) {
                    receive_mov_dir_button_data();

					gpAppView->onTick();

                    static auto timeBegin = std::chrono::system_clock::now();
                    auto timeNow = std::chrono::system_clock::now();
                    std::chrono::microseconds DTime = timeNow - timeBegin;
					const std::chrono::microseconds s_oneSec(1000);
                    if (DTime >= s_oneSec)
                    {
                        send_render_status_data();
						receive_and_run_cmd();
                        timeBegin = std::chrono::system_clock::now();
                    }
				}

				if (!process_android_events(app)) {
					// Android requested for the app to close
					LOGI("Android app has been destroyed by the OS");
					return ExitCode::Close;
				}
			}

			if (!process_android_events(app)) {
				// Android requested for the app to close
				LOGI("Android app has been destroyed by the OS");
				return ExitCode::Close;
			}
		}
		return ExitCode::Success;
    }

    void EchoVkNativePlatform::terminate(ExitCode code)
	{
        switch (code)
        {
            case ExitCode::Success:
            case ExitCode::Close:
                log_output.clear();
                break;
            default:
                break;
        }

        AndroidJavaEnv::ReleaseJavaEnv();

        GameActivity_finish(app->activity);

        while (process_android_events(app))
        {
            // Process events until app->destroyRequested is set
        }
    }

    void EchoVkNativePlatform::resetWindow(ANativeWindow* window)
    {
        auto gAppView = get_appview();
        if(gAppView) {
            gAppView->onResetWindow(window);
        }
    }

	void EchoVkNativePlatform::resize(uint32_t width, uint32_t height)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
		{
			gpAppView->onResize(width, height, 0, 0, width, height);
		}
		else
		{
			g_graphicParam.eClass = Echo::ECHO_DEVICE_VULKAN;
            g_graphicParam.pHandle = app->window;

			snprintf(g_graphicParam.cpuInfo, sizeof(g_graphicParam.cpuInfo) - 1, "%s", g_strInfoArray[7].c_str());
			snprintf(g_graphicParam.model, sizeof(g_graphicParam.model) - 1, "%s", g_strInfoArray[8].c_str());
            g_graphicParam.cpuCoreNumber = std::thread::hardware_concurrency();

			init_appview(true, true, g_strInfoArray[0].c_str(), g_strInfoArray[1].c_str(), &g_graphicParam, width, height);
			gpAppView = get_appview();
			gpAppView->onResize(width, height, 0, 0, width, height);
		}
	}

    void EchoVkNativePlatform::process_android_input_events()
    {
        auto input_buf = android_app_swap_input_buffers(app);
        if (!input_buf)
        {
            return;
        }

        if (input_buf->motionEventsCount)
        {
            for (int idx = 0; idx < input_buf->motionEventsCount; idx++)
            {
                auto event = &input_buf->motionEvents[idx];
                assert((event->source == AINPUT_SOURCE_MOUSE ||
                        event->source == AINPUT_SOURCE_TOUCHSCREEN) &&
                       "Invalid motion event source");

                std::int32_t action = event->action;

                float x = GameActivityPointerAxes_getX(&event->pointers[0]);
                float y = GameActivityPointerAxes_getY(&event->pointers[0]);

                if (event->source == AINPUT_SOURCE_TOUCHSCREEN)
                {
                    // Multiple pointers are not supported.
                    size_t       pointer_count = event->pointerCount;
                    std::int32_t pointer_id    = event->pointers[0].id;

					touch_input_event(TouchInputEvent {
                            translate_touch_action(action),
                            pointer_id,
                            pointer_count,
                            x, y
					});
                }
            }
            android_app_clear_motion_events(input_buf);
        }

        if (input_buf->keyEventsCount)
        {
            for (int idx = 0; idx < input_buf->keyEventsCount; idx++)
            {
                auto event = &input_buf->keyEvents[idx];
                assert((event->source == AINPUT_SOURCE_KEYBOARD) &&
                       "Invalid key event source");
				key_input_event(KeyInputEvent{
                        translate_key_code(event->keyCode),
                        translate_key_action(event->action)});
            }
			android_app_clear_key_events(input_buf);
        }
    }

    void EchoVkNativePlatform::key_input_event(const KeyInputEvent &input_event)
    {
		if (input_event.code == KeyCode::Back ||
			input_event.code == KeyCode::Escape)
		{
			auto gpAppView = get_appview();
			if (gpAppView)
            {
                gpAppView->onRelease(true);
                close_requested = true;
                running = false;
            }
		}
    }

	void EchoVkNativePlatform::touch_input_event(const TouchInputEvent &input_event)
	{
		auto gpAppView = get_appview();
		if (gpAppView)
		{
			//TODO(yanghang): Temp engine internal input event push to FGUI.
			Echo::Vector2 mouse(input_event.pos_x, input_event.pos_y);
			if (input_event.action == Down)
			{
				gpAppView->onMouseLeftDown(mouse);

			}
			else if (input_event.action == Up)
			{
				gpAppView->onMouseLeftUp(mouse);
			}
			else if (input_event.action == Move)
			{
				gpAppView->onMouseMove(mouse);
			}
		}
	}

    void EchoVkNativePlatform::receive_and_run_cmd()
    {
		auto gpAppView = get_appview();
		if (!gpAppView)
		{
			return;
		}
        Echo::AndroidJavaEnv envTool;
        JNIEnv *env = envTool.GetJavaEnv();
        jclass    cls         = env->GetObjectClass(app->activity->javaGameActivity);
        jfieldID jcommandId = env->GetStaticFieldID(cls, "command", "Ljava/lang/String;");
		auto jcommand = (jstring)(env->GetStaticObjectField(cls, jcommandId));
		if (jcommand != nullptr)
		{
			const char* command = env->GetStringUTFChars(jcommand, nullptr);

			std::string str = command;

			if (!str.empty())
			{
				gpAppView->commandParse(str);
				env->SetStaticObjectField(cls,jcommandId, nullptr);
			}
		}
    }

    void EchoVkNativePlatform::send_render_status_data() {
        auto gpAppView = get_appview();
        if (!gpAppView)
        {
            return;
        }

		static std::string sLastNonEmptyGameData;

        int nFps = gpAppView->getFps();
        int nDrawCall = gpAppView->getDrawCall();
        int nTriangleCnt = gpAppView->getTriangleCnt();
        std::string camPos = gpAppView->getCameraPosition();
        std::string memUsage = gpAppView->getResMemUsageStr();
        std::string gameData = gpAppView->getGameData();
		if (!gameData.empty())
		{
			sLastNonEmptyGameData = gameData;
		}
		else if (!sLastNonEmptyGameData.empty())
		{
			gameData = sLastNonEmptyGameData;
		}

        Echo::AndroidJavaEnv envTool;
        JNIEnv *env = envTool.GetJavaEnv();
        jclass   cls                = env->GetObjectClass(app->activity->javaGameActivity);

        jfieldID jnFpsId            = env->GetStaticFieldID(cls, "nFps", "I");
        jfieldID jnDrawCallId       = env->GetStaticFieldID(cls, "nDrawCall", "I");
        jfieldID jnTriangleCntId    = env->GetStaticFieldID(cls, "nTriangleCnt", "I");
        jfieldID jcamPosId          = env->GetStaticFieldID(cls, "camPos", "Ljava/lang/String;");
        jfieldID jmemUsageId        = env->GetStaticFieldID(cls, "memUsage", "Ljava/lang/String;");
        jfieldID jgameDataId        = env->GetStaticFieldID(cls, "gameData", "Ljava/lang/String;");

        env->SetStaticIntField(cls,jnFpsId,nFps);
        env->SetStaticIntField(cls,jnDrawCallId,nDrawCall);
        env->SetStaticIntField(cls,jnTriangleCntId,nTriangleCnt);
        env->SetStaticObjectField(cls,jcamPosId,env->NewStringUTF(camPos.c_str()));
        env->SetStaticObjectField(cls,jmemUsageId,env->NewStringUTF(memUsage.c_str()));
        env->SetStaticObjectField(cls,jgameDataId,env->NewStringUTF(gameData.c_str()));
    }

    void EchoVkNativePlatform::receive_mov_dir_button_data() {
        auto gpAppView = get_appview();
        if (!gpAppView)
        {
            return;
        }

        Echo::AndroidJavaEnv envTool;
        JNIEnv *env = envTool.GetJavaEnv();
        jclass   cls            = env->GetObjectClass(app->activity->javaGameActivity);

        jfieldID jnbutton_movID = env->GetStaticFieldID(cls,"button_mov","I");
        jfieldID jnbutton_dirID = env->GetStaticFieldID(cls,"button_dir","I");

        jint jnbutton_mov       = env->GetStaticIntField(cls,jnbutton_movID);
        jint jnbutton_dir       = env->GetStaticIntField(cls,jnbutton_dirID);

        jfieldID jnbutton_jumpID = env->GetStaticFieldID(cls, "jump", "Z");
        jboolean jump = env->GetStaticBooleanField(cls,jnbutton_jumpID);

        if (jnbutton_mov == 1)
        {
            jfieldID jmov_xID   = env->GetStaticFieldID(cls,"mov_x","F");
            jfieldID jmov_yID   = env->GetStaticFieldID(cls,"mov_y","F");

            jfloat jmov_x       = env->GetStaticFloatField(cls,jmov_xID);
            jfloat jmov_y       = env->GetStaticFloatField(cls,jmov_yID);

            //gpAppView->setMoveDir(jmov_x * 2, jmov_y * 2);

            gpAppView->cameraMove(1, jmov_y * 2);
            gpAppView->cameraMove(0, jmov_x * 2);
        }
        else
        {
            //gpAppView->setMoveDir(0.0f, 0.0f);
            gpAppView->cameraMove(1.0f, 0.0f);
            gpAppView->cameraMove(0.0f, 0.0f);
        }

        if (jnbutton_dir == 1)
        {
            jfieldID jdir_xID   = env->GetStaticFieldID(cls,"dir_x","F");
            jfieldID jdir_yID   = env->GetStaticFieldID(cls,"dir_y","F");

            jfloat jdir_x       = env->GetStaticFloatField(cls,jdir_xID);
            jfloat jdir_y       = env->GetStaticFloatField(cls,jdir_yID);

            //gpAppView->setTurnDir(-jdir_x * 2, jdir_y * 2);
            gpAppView->cameraTurn(-jdir_x * 20, jdir_y * 20);
        }
        else
        {
            //gpAppView->setTurnDir(0.0f, 0.0f);
            gpAppView->cameraTurn(0.0f, 0.0f);
        }

        if(jump){
            gpAppView->sphericalControlerJump();
            env->SetStaticBooleanField(cls, jnbutton_jumpID, false);
        }
    }
}
