//#include "napi/native_api.h"

#include <hilog/log.h>
#include "platform_ohos.h"
#include "file_operator.h"
#include "common.h"

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "Init", "Init begins");
    if ((nullptr == env) || (nullptr == exports)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xFF00, "Init", "env or exports is null");
        return nullptr;
    }
    napi_property_descriptor desc[] = {
        { "getContext", nullptr, PluginManager::GetContext, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "update", nullptr, PluginManager::UpdateMessage, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "command", nullptr, PluginManager::AddCommand, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "touch", nullptr, PluginManager::AddTouchCommand, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    if (napi_ok != napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "Init", "napi_define_properties failed");
        return nullptr;
    }
    
    PluginManager::GetInstance()->Export(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module PadBo2Module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "PadBo2",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&PadBo2Module);
}