
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

searchFiles("../../../../../../EchoComponent/Source" allFiles true)
searchFiles("../../../../../../EchoComponent/Script/Source" allFiles true)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function  -Wno-unused-but-set-variable -gline-tables-only -Wno-unused-const-variable ")
else ()
	add_definitions(
        -DNDEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-const-variable ")
endif ()

add_library( # Sets the name of the library.
             EchoComponent

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${allFiles}
             )
			 
include_directories(
            "../../../../../../EchoComponent/Include"
            "../../../../../../EchoComponent/PrivateInclude"
			"../../../../../../EchoComponent/Script/Include"
			"../../../../../../EchoSDK/Include/ThirdParty/tinythread"
			"../../../../../../EchoSDK/Include/ThirdParty/cjson"
			"../../../../../../EchoSDK/Include/ThirdParty/freetype"
			"../../../../../../EchoCommon/Include"
            "../../../../../../EchoCommon/PrivateInclude"
			"../../../../../../EchoEngine/Code/Core/Include"
			"../../../../../../EchoEngine/Code/Core/Include/Role"			
            "../../../../../../EchoEngine/Code/Core/PrivateInclude"
			"../../../../../../EchoEngine/Code/Core/Source/FGUIParser"
			"../../../../../../EchoEngine/Code/Core/Source/FGUIParser/entt/src"
			"../../../../../../GameEngine/Include"
			"../../../../../../GameEngine/PrivateInclude"
			"../../../../../../EchoPhysics/Include"     
            "../../../../../../EchoPhysics/PrivateInclude"              
			)
			
if (ANDROID_ABI STREQUAL armeabi-v7a)
    set(AbiStr "ARM")
elseif (ANDROID_ABI STREQUAL arm64-v8a)
    set(AbiStr "ARM64")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CfgName "Debug")
else ()
	set(CfgName "Release")
endif ()

if(SANITIZE)
	if(${SANITIZE} STREQUAL "hwasan")
		target_compile_options(EchoComponent PUBLIC -fsanitize=hwaddress -fno-omit-frame-pointer)
		target_link_options(EchoComponent PUBLIC -fsanitize=hwaddress)
	endif()
endif()


set(lib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoSDK)

set_target_properties(EchoComponent
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${lib_DIR}/Lib/Android/${AbiStr}/${CfgName}")

# copy out lib header file... TARGET lib POST_BUILD
add_custom_command(TARGET EchoComponent POST_BUILD
					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoComponent/Include" "${lib_DIR}/Include/EchoComponent"	
                    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoComponent/PrivateInclude" "${lib_DIR}/PrivateInclude/EchoComponent"	                     
#					COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoComponent/Script/Include" "${lib_DIR}/Include/EchoComponent"
                   COMMENT "Copying include to sdk directory")
				   