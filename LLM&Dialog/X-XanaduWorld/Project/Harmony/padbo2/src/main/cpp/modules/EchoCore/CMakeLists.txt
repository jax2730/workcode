cmake_minimum_required(VERSION 3.5.0)

function(searchFiles path filelist isRecuse)
    if (isRecuse)
        file(GLOB_RECURSE CPPFILES "${path}/*.cpp" "${path}/*.c")
        set(${filelist} ${${filelist}} ${CPPFILES} ${CFILES} PARENT_SCOPE)
    else ()
        aux_source_directory(${path} allSrc)
        set(${filelist} ${${filelist}} ${allSrc} PARENT_SCOPE)
    endif ()
endfunction(searchFiles)

searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source" allFiles false)
#searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/Android" allFiles false)
searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/JobSystem" allFiles false)
searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/Particle" allFiles false)
searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/meshoptimizer" allFiles false)
searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/FGUIParser" allFiles true)
#searchFiles("../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/zstd" allFiles true)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG  -DFREEIMAGE_LIB
			-DTRACY_ENABLE
			-DTRACY_ON_DEMAND
			-DTRACY_DELAYED_INIT
			-DTRACY_MANUAL_LIFETIME
			-DTRACY_NO_CALLSTACK
			-DTRACY_NO_SAMPLING
			-DTRACY_NO_BROADCAST
			-DTRACY_NO_CODE_TRANSFER
			-DTRACY_NO_CRASH_HANDLER
			-DTRACY_NO_SYSTEM_TRACING
			-DTRACY_NO_CONTEXT_SWITCH
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
else ()
	add_definitions(
        -DNDEBUG  -DFREEIMAGE_LIB
			-DTRACY_ENABLE
			-DTRACY_ON_DEMAND
			-DTRACY_DELAYED_INIT
			-DTRACY_MANUAL_LIFETIME
			-DTRACY_NO_CALLSTACK
			-DTRACY_NO_SAMPLING
			-DTRACY_NO_BROADCAST
			-DTRACY_NO_CODE_TRANSFER
			-DTRACY_NO_CRASH_HANDLER
			-DTRACY_NO_SYSTEM_TRACING
			-DTRACY_NO_CONTEXT_SWITCH
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
endif ()

add_library( # Sets the name of the library.
             EchoCore

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${allFiles}
             ../../../../../../../../../../EchoEngine/Code/Core/Source/Ohos/EchoOAssetDataStream.cpp
             ../../../../../../../../../../EchoEngine/Code/Core/Source/Ohos/EchoDefaultOhosArchive.cpp
             ../../../../../../../../../../EchoEngine/Code/Core/Source/Ohos/EchoShaderClientOhos.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/EchoScopedZone.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/EchoTraceManager.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/TracyClient.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/capture/src/captureThread.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/common/tracy_lz4hc.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/common/TracyStackFrames.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyMemory.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyMmap.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyPrint.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyTaskDispatch.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyTextureCompression.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyThreadCompress.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/server/TracyWorker.cpp
			 ../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/StatusGather/EchoCoreStatusData.cpp
             )
			 
include_directories(
            "../../../../../../../../../../EchoEngine/Code/Core/Include"
            "../../../../../../../../../../EchoEngine/Code/Core/PrivateInclude"
			"../../../../../../../../../../EchoEngine/Code/Core/Source/FGUIParser"
			"../../../../../../../../../../EchoEngine/Code/Core/Source/FGUIParser/entt/src"
			"../../../../../../../../../../EchoEngine/Code/Core/Source/simde"
			"../../../../../../../../../../EchoEngine/SDKs/RcSDK/Include"
			"../../../../../../../../../../EchoCommon/Include"
            "../../../../../../../../../../EchoCommon/PrivateInclude"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/tinythread"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/freetype"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/cjson"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/md5"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/FreeImage"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/cpufeatures"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/zzip"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/sqlite"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/stb"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/ffmpeg"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/cpp-httplib"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/echoserver"
			"../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/public/tracy"
			"../../../../../../../../../../EchoEngine/Code/Core/Source/Profiler/vcpkg_installed/x64-windows-static/include/capstone"
			)

add_precompiled_header(EchoCore EchoStableHeaders.h SOURCE_CXX ../../../../../../../../../../EchoEngine/Code/Core/Source/EchoPrecompiledHeaders.cpp)
			
if (OHOS_ARCH STREQUAL armeabi-v7a)
    set(AbiStr "ARM")
elseif (OHOS_ARCH STREQUAL arm64-v8a)
    set(AbiStr "ARM64")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CfgName "Debug")
else ()
	set(CfgName "Release")
endif ()

set(lib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoSDK)

set_target_properties(EchoCore
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${lib_DIR}/Lib/Ohos/${AbiStr}/${CfgName}")

# copy out lib header file... TARGET lib POST_BUILD
add_custom_command(TARGET EchoCore POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoEngine/SDKS/RcSDK/Include" "${lib_DIR}/Include/EchoCore"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoEngine/Code/Core/Include" "${lib_DIR}/Include/EchoCore"
                    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoEngine/Code/Core/PrivateInclude" "${lib_DIR}/PrivateInclude/EchoCore"
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoEngine/SDKS/RcSDK/Lib/Ohos/${AbiStr}/${CfgName}" "${lib_DIR}/Lib/Ohos/${AbiStr}/${CfgName}"
                   COMMENT "Copying include to sdk directory")
				   