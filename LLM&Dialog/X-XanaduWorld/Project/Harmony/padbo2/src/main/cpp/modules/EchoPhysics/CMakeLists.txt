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

searchFiles("../../../../../../../../../../EchoPhysics/Source" allFiles true)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
else ()
	add_definitions(
        -DNDEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-command-line-argument -Wno-unused-const-variable")
endif ()

add_library( # Sets the name of the library.
             EchoPhysics

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${allFiles}
             )
			 
include_directories(
            "../../../../../../../../../../EchoPhysics/Include"
            "../../../../../../../../../../EchoPhysics/PrivateInclude"
			"../../../../../../../../../../EchoCommon/Include"
            "../../../../../../../../../../EchoCommon/PrivateInclude"
			"../../../../../../../../../../ThirdParty/PhysX-4.1/physx/Include"
			"../../../../../../../../../../ThirdParty/PhysX-4.1/pxshared/include"
			"../../../../../../../../../../ThirdParty/PhysX-4.1/physx/source/foundation/include"
			)

add_precompiled_header(EchoPhysics EchoPhysxStableHeaders.h SOURCE_CXX ../../../../../../../../../../EchoPhysics/Source/stdafx.cpp)

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

set_target_properties(EchoPhysics
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${lib_DIR}/Lib/Ohos/${AbiStr}/${CfgName}")

# copy out lib header file... TARGET lib POST_BUILD
add_custom_command(TARGET EchoPhysics POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoPhysics/Include" "${lib_DIR}/Include/EchoPhysics"
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../../../EchoPhysics/PrivateInclude" "${lib_DIR}/PrivateInclude/EchoPhysics"
                   COMMENT "Copying include to sdk directory")
				   