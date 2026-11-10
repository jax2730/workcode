cmake_minimum_required(VERSION 3.4.1)

function(searchFiles path filelist isRecuse)
    if (isRecuse)
        file(GLOB_RECURSE CPPFILES "${path}/*.cpp" "${path}/*.c")
        set(${filelist} ${${filelist}} ${CPPFILES} ${CFILES} PARENT_SCOPE)
    else ()
        aux_source_directory(${path} allSrc)
        set(${filelist} ${${filelist}} ${allSrc} PARENT_SCOPE)
    endif ()
endfunction(searchFiles)

searchFiles("../../../../../../../../../../EchoGame/Source" allFiles true)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
else ()
	add_definitions(
        -DNDEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
endif ()

add_library( # Sets the name of the library.
             EchoGame

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${allFiles}
             )
			 
include_directories(
            "../../../../../../../../../../GameEngine/Include"
            "../../../../../../../../../../GameEngine/PrivateInclude"
			"../../../../../../../../../../EchoEngine/Code/Core/Include"
            "../../../../../../../../../../EchoEngine/Code/Core/PrivateInclude"
			"../../../../../../../../../../EchoCommon/Include"
            "../../../../../../../../../../EchoCommon/PrivateInclude"
			"../../../../../../../../../../EchoPhysics/Include"
            "../../../../../../../../../../EchoPhysics/PrivateInclude"
			"../../../../../../../../../../EchoComponent/Include"
            "../../../../../../../../../../EchoComponent/PrivateInclude"
			"../../../../../../../../../../EchoGame/Include"
            "../../../../../../../../../../EchoGame/PrivateInclude"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/tinythread"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/freetype"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/cjson"
			"../../../../../../../../../../EchoSDK/Include/ThirdParty/FMOD/core"
            "../../../../../../../../../../EchoSDK/Include/ThirdParty/FMOD/studio"
			)

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

set_target_properties(EchoGame
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${lib_DIR}/Lib/Ohos/${AbiStr}/${CfgName}")

# copy out lib header file... TARGET lib POST_BUILD
add_custom_command(TARGET EchoGame POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoGame/Include" "${lib_DIR}/Include/EchoGame"
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoGame/PrivateInclude" "${lib_DIR}/PrivateInclude/EchoGame"
                   COMMENT "Copying include to sdk directory")
				   
