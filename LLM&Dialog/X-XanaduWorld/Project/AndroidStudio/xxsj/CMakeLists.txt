# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
include(PrecompiledHeader.cmake)
# Declares and names the project.
project("PadBO2")


add_subdirectory(EchoCommon)
add_subdirectory(EchoCore)
add_subdirectory(EchoPhysics)
add_subdirectory(EchoEngine)
add_subdirectory(EchoComponent)
add_subdirectory(EchoGame)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

function(searchFiles path filelist isRecuse)
    if (isRecuse)
        file(GLOB_RECURSE CPPFILES "${path}/*.cpp" "${path}/*.c" "${path}/*.cxx")
        set(${filelist} ${${filelist}} ${CPPFILES} ${CFILES} PARENT_SCOPE)
    else ()
        aux_source_directory(${path} allSrc)
        set(${filelist} ${${filelist}} ${allSrc} PARENT_SCOPE)
    endif ()
endfunction(searchFiles)


searchFiles("../../../Source" allFiles false)
searchFiles("../../../Source/PlatformAndroid" allFiles false)


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
             PadBO2

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
			 ${allFiles}
             )

include_directories(
			"../../../../../EchoSDK/Include/ThirdParty/cjson"
			"../../../../../EchoSDK/Include/ThirdParty/tinythread"
			"../../../../../EchoEngine/Code/Core/Include"
			"../../../../../EchoEngine/Code/Core/Include/Role"			
            "../../../../../EchoEngine/Code/Core/PrivateInclude"				
            "../../../../../GameEngine/Include"
            "../../../../../GameEngine/PrivateInclude"
            "../../../../../EchoCommon/Include"
            "../../../../../EchoCommon/PrivateInclude"
            "../../../../../EchoPhysics/Include"
            "../../../../../EchoPhysics/PrivateInclude"            
            "../../../../../EchoComponent/Include"
            "../../../../../EchoComponent/PrivateInclude"
			"../../../../../EchoGame/Include"
            "../../../../../EchoGame/PrivateInclude"
            "../../../../../EchoEngine/Code/Core/Source/FGUIParser"
	    "../../../../../EchoEngine/Code/Core/Source/FGUIParser/entt/src"
)

target_link_libraries(PadBO2 EchoGame)
target_link_libraries(PadBO2 EchoComponent)
target_link_libraries(PadBO2 EchoEngine)
target_link_libraries(PadBO2 EchoCore)
target_link_libraries(PadBO2 EchoPhysics)
target_link_libraries(PadBO2 EchoCommon)

find_package(game-activity REQUIRED CONFIG)
target_link_libraries(PadBO2 game-activity::game-activity_static)

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
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
)

set(allPhyXLib
    PhysX
)

if (ANDROID_ABI STREQUAL armeabi-v7a)
    set(AbiStr "ARM")
elseif (ANDROID_ABI STREQUAL arm64-v8a)
    set(AbiStr "ARM64")
elseif (ANDROID_ABI STREQUAL x86)
    set(AbiStr "x86")
elseif (ANDROID_ABI STREQUAL x86_64)
    set(AbiStr "x64")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CfgName "DEBUG")
    get_filename_component(libPath "../../../../../EchoSDK/Lib/Android/${AbiStr}/Debug" ABSOLUTE)
else ()
    set(CfgName "")
    get_filename_component(libPath "../../../../../EchoSDK/Lib/Android/${AbiStr}/Release" ABSOLUTE)
endif ()

if(SANITIZE)
    if(${SANITIZE} STREQUAL "hwasan")
        target_compile_options(PadBO2 PUBLIC -fsanitize=hwaddress -fno-omit-frame-pointer)
        target_link_options(PadBO2 PUBLIC -fsanitize=hwaddress)
    endif()
endif()


foreach (lib ${allDepLib})
    set(libName ${lib}-lib)
    add_library(${libName} STATIC IMPORTED)
    set_target_properties(${libName}
      PROPERTIES IMPORTED_LOCATION
      ${libPath}/lib${lib}.a)
    target_link_libraries(PadBO2 ${libName})
endforeach()

foreach (lib ${allPhyXLib})
    set(libName ${lib}-lib)
    add_library(${libName} STATIC IMPORTED)
    set_target_properties(${libName}
      PROPERTIES IMPORTED_LOCATION
      ${libPath}/lib${lib}.a)
    target_link_libraries(PadBO2 ${libName})
endforeach()

add_library(fmod-lib SHARED IMPORTED)
set_target_properties(fmod-lib
    PROPERTIES IMPORTED_LOCATION
    ${libPath}/libfmod.so)
target_link_libraries(PadBO2 fmod-lib)

add_library(fmodstudio-lib SHARED IMPORTED)
set_target_properties(fmodstudio-lib
		PROPERTIES IMPORTED_LOCATION
		${libPath}/libfmodstudio.so)
target_link_libraries(PadBO2 fmodstudio-lib)

#add_library(fmodstudio-lib SHARED IMPORTED)
#set_target_properties(fmodstudio-lib
#		PROPERTIES IMPORTED_LOCATION
#		${libPath}/libfmodstudio.so)
#target_link_libraries(PadBO2 fmodstudio-lib)
# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

set(allNdkLib
    EGL
    GLESv3
    m
    z
    log
    android
    mediandk
)

foreach (lib ${allNdkLib})
    set(libName ${lib}-lib)
    find_library( ${libName} ${lib})
    target_link_libraries(PadBO2 ${${libName}})
endforeach()

