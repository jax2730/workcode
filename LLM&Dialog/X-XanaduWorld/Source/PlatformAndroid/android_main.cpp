//
// Created by changziheng on 2023/6/1.
//

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "platform_android.h"


extern "C" {
void android_main(struct android_app* app);
};


using namespace EchoVk;

void android_main(struct android_app* app)
{
    auto* platform = new EchoVkNativePlatform(app);
    ExitCode code = platform->init();

    if (code == ExitCode::Success)
    {
        code = platform->gameLoop();
    }

    platform->terminate(code);

    delete platform;
    exit(0);
}
