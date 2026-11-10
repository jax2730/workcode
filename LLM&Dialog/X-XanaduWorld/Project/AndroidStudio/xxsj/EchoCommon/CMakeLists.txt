
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

searchFiles("../../../../../../EchoCommon/Source" allFiles false)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -D_DEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-const-variable ")
else ()
	add_definitions(
        -DNDEBUG
    )
	set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-const-variable ")
endif ()

add_library( # Sets the name of the library.
             EchoCommon

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ${allFiles}
             )
			 
include_directories(
            "../../../../../../EchoCommon/Include"
            "../../../../../../EchoCommon/PrivateInclude"
			"../../../../../../EchoSDK/Include/ThirdParty/tinythread"
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
        target_compile_options(EchoCommon PUBLIC -fsanitize=hwaddress -fno-omit-frame-pointer)
        target_link_options(EchoCommon PUBLIC -fsanitize=hwaddress)
    endif()
endif()

set(lib_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoSDK)

set_target_properties(EchoCommon
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${lib_DIR}/Lib/Android/${AbiStr}/${CfgName}")

# copy out lib header file... TARGET lib POST_BUILD
add_custom_command(TARGET EchoCommon POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoCommon/Include" "${lib_DIR}/Include/EchoCommon" 
                   COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../EchoCommon/PrivateInclude" "${lib_DIR}/PrivateInclude/EchoCommon" 
                   COMMENT "Copying include to sdk directory")
				   