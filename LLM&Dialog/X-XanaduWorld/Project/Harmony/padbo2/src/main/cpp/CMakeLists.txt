# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
include(PrecompiledHeader.cmake)
project(PadBO2)

add_subdirectory(modules/EchoCommon)
add_subdirectory(modules/EchoCore)
add_subdirectory(modules/EchoPhysics)
add_subdirectory(modules/EchoEngine)
add_subdirectory(modules/EchoComponent)
add_subdirectory(modules/EchoGame)

function(searchFiles path filelist isRecuse)
    if (isRecuse)
        file(GLOB_RECURSE CPPFILES "${path}/*.cpp" "${path}/*.c" "${path}/*.cxx")
        set(${filelist} ${${filelist}} ${CPPFILES} ${CFILES} PARENT_SCOPE)
    else ()
        aux_source_directory(${path} allSrc)
        set(${filelist} ${${filelist}} ${allSrc} PARENT_SCOPE)
    endif ()
endfunction(searchFiles)

searchFiles("../../../../../../Source" allFiles false)
searchFiles("../../../../../../Source/PlatformOhos" allFiles false)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG
    )
else ()
	add_definitions(
        -DNDEBUG
    )
endif ()

add_library( # Sets the name of the library.
             padbo2

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
			 ${allFiles}
			 napi_init.cpp
			 )

include_directories(
			"../../../../../../../../EchoSDK/Include/ThirdParty/cjson"
			"../../../../../../../../EchoSDK/Include/ThirdParty/tinythread"
			"../../../../../../../../EchoEngine/Code/Core/Include"
            "../../../../../../../../EchoEngine/Code/Core/PrivateInclude"
            "../../../../../../../../GameEngine/Include"
            "../../../../../../../../GameEngine/PrivateInclude"
            "../../../../../../../../EchoCommon/Include"
            "../../../../../../../../EchoCommon/PrivateInclude"
            "../../../../../../../../EchoPhysics/Include"
            "../../../../../../../../EchoPhysics/PrivateInclude"
            "../../../../../../../../EchoComponent/Include"
            "../../../../../../../../EchoComponent/PrivateInclude"
			"../../../../../../../../EchoGame/Include"
            "../../../../../../../../EchoGame/PrivateInclude"

            "../../../../../../Source/PlatformOhos"
)

target_link_libraries(padbo2 EchoGame)
target_link_libraries(padbo2 EchoComponent)
target_link_libraries(padbo2 EchoEngine)
target_link_libraries(padbo2 EchoCore)
target_link_libraries(padbo2 EchoPhysics)
target_link_libraries(padbo2 EchoCommon)

if (OHOS_ARCH STREQUAL armeabi-v7a)
    set(AbiStr "ARM")
elseif (OHOS_ARCH STREQUAL arm64-v8a)
    set(AbiStr "ARM64")
elseif (OHOS_ARCH STREQUAL x86)
    set(AbiStr "x86")
elseif (OHOS_ARCH STREQUAL x86_64)
    set(AbiStr "x64")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CfgName "DEBUG")
    get_filename_component(libPath "../../../../../../../../EchoSDK/Lib/Ohos/${AbiStr}/Debug" ABSOLUTE)
else ()
    set(CfgName "")
    get_filename_component(libPath "../../../../../../../../EchoSDK/Lib/Ohos/${AbiStr}/Release" ABSOLUTE)
endif ()

set(allDepLib
    RenderCommand
    RenderCommandGLES3
    RenderCommandVulkan
    tinythread
    EchoJson
    freetype
    FreeImage
    libpng
    cpufeatures
    md5
    ktx
	zzip
	sqlite
    #TODO（jianglong）：以下为视频解码相关的库，还未进行鸿蒙适配，暂且注掉
    #avcodec
    #avdevice
    #avfilter
    #avformat
    #avutil
    #postproc
    #swresample
    #swscale
)
foreach (lib ${allDepLib})
    set(libName ${lib}-lib)
    add_library(${libName} STATIC IMPORTED)
    set_target_properties(${libName}
      PROPERTIES IMPORTED_LOCATION
      ${libPath}/lib${lib}.a)
    target_link_libraries(padbo2 ${libName})
endforeach()

set(allPhyXLib
    PhysX
)
foreach (lib ${allPhyXLib})
    set(libName ${lib}-lib)
    add_library(${libName} STATIC IMPORTED)
    set_target_properties(${libName}
      PROPERTIES IMPORTED_LOCATION
      ${libPath}/lib${lib}.a)
    target_link_libraries(padbo2 ${libName})
endforeach()

set(allNdkLib
    EGL
    GLESv3
    m
    z
    ace_ndk.z
    rawfile.z
)
foreach (lib ${allNdkLib})
    set(libName ${lib}-lib)
    find_library( ${libName} ${lib})
    target_link_libraries(padbo2 ${${libName}})
endforeach()

find_library(
    # Sets the name of the path variable.
    hilog-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    hilog_ndk.z
)

add_library(fmod-lib SHARED IMPORTED)
add_library(fmodstudio-lib SHARED IMPORTED)
#set_target_properties(fmodstudio-lib
#    PROPERTIES IMPORTED_LOCATION
#    ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/libfmodstudio.so)
#target_link_libraries(padbo2 fmodstudio-lib)

target_link_libraries(padbo2 libace_napi.z.so ${hilog-lib} libuv.so)
target_link_libraries(padbo2 ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/libfmodstudio.so)
target_link_libraries(padbo2 ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/libfmod.so)