# Copyright (c) 2014, Smart Projects Holdings Ltd

# All rights reserved.

# See LICENSE file for license details.

cmake_minimum_required(VERSION 2.8.3)

project(vstreamer)

#SET(CMAKE_C_COMPILER "/opt/local/bin/gcc-mp-4.8")
#SET(CMAKE_CXX_COMPILER "/opt/local/bin/g++-mp-4.8")

add_definitions( -DFFMPEG_CAP=1 )
add_definitions( -DVIDEO_SAVE_ENABLED=1 )

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	SET(CMAKE_BUILD_TYPE RELEASE)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

SET(UGCS_PACKAGE_SUMMARY "UgCS Mjpeg Video Streamer used to stream video from different devices along with postprocessing")
set(UGCS_VSM_USER_GUIDE_LATEX_TITLE "Video Streamer User Guide")


SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"
					  "${CMAKE_SOURCE_DIR}/../build-scripts/cmake"
					  "${VSM_SDK_DIR}/share/cmake")

include("configure_packaging")

include("FindLibav")

message("AVFormat lib: ${LIBAV_AVFORMAT_LIBRARIES}")

include("vstreamer_common")


if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    FIND_PACKAGE(OpenCV REQUIRED)
    add_definitions( -DOPENCV_CAP=1 )
endif()


if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	exec_program(uname ARGS -r OUTPUT_VARIABLE DARWIN_VERSION)
    MESSAGE(STATUS "DARWIN_VERSION=${DARWIN_VERSION}")
endif()


Find_platform_sources("." PLATFORM_INCLUDES PLATFORM_SOURCES)

# Deal with path to SDK
if (NOT DEFINED VSM_SDK_DIR)
    if (DEFINED ENV{VSM_SDK_DIR})
        set(VSM_SDK_DIR $ENV{VSM_SDK_DIR})
    else()
        if(CMAKE_SYSTEM_NAME MATCHES "Windows")
            set(VSM_SDK_DIR "C:/UGCS/vsm-sdk")
        elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(VSM_SDK_DIR "/opt/ugcs/vsm-sdk")
	elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	    set(VSM_SDK_DIR "/Applications/UgCS/vsm-sdk")
        endif()
    endif()
endif()

# Common for all VSMs
LIST(APPEND CMAKE_MODULE_PATH "${VSM_SDK_DIR}/share/cmake")
include("ugcs/vsm")

# remove flags which prevent building with opencv headers
string(REPLACE "-Wextra" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wold-style-cast" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Default VSM config file
# Optional arguments are source and destination file locations.
# For example Build_vsm_config(/path/to/your/vsm.conf) to copy vsm.conf from
# your location to default binary folder location or
# Build_vsm_config(/path/to/your/vsm.conf ${CMAKE_BINARY_DIR}/your.name) to
# copy and rename the destination file also.
Build_vsm_config(${CMAKE_SOURCE_DIR}/vsm.conf)
# VSM source and header files
#file(GLOB SOURCES "*.cpp" "*.h")

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")

    #10.9.2 - 10.9.5
	if (DARWIN_VERSION MATCHES "13.1.0" OR DARWIN_VERSION MATCHES "13.2.0" OR DARWIN_VERSION MATCHES "13.3.0" OR DARWIN_VERSION MATCHES "13.4.0")
	
		FILE(COPY	"/System/Library/Frameworks/IOKit.framework/Headers/usb/USB.h"
					DESTINATION "${CMAKE_BINARY_DIR}/include/IOKit/usb"
					NO_SOURCE_PERMISSIONS)
		SET(ENV{LC_TYPE} "C")
		SET(ENV{LANG} "C")
		execute_process(COMMAND sed -i "" "s/#pragma options align=reset/#pragma pack()/g" ${CMAKE_BINARY_DIR}/include/IOKit/usb/USB.h)
	endif()
	
	
	#10.10.4
	if (DARWIN_VERSION MATCHES "14.4.0")
	
	
	   FILE(COPY "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/System/Library/Frameworks/IOKit.framework/Headers/usb/USB.h"
                                DESTINATION "${CMAKE_BINARY_DIR}/include/IOKit/usb"
                                NO_SOURCE_PERMISSIONS)


        FILE(COPY "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include/dispatch/object.h"
                                DESTINATION  "${CMAKE_BINARY_DIR}/include/dispatch"
                                NO_SOURCE_PERMISSIONS)


        SET(ENV{LC_TYPE} "C")
        SET(ENV{LANG} "C")
        execute_process(COMMAND sed -i "" "s/#pragma options align=reset/#pragma pack()/g" ${CMAKE_BINARY_DIR}/include/IOKit/usb/USB.h)
        execute_process(COMMAND sed -i "" "s/typedef void (^dispatch_block_t)(void)/typedef void* dispatch_block_t/g" ${CMAKE_BINARY_DIR}/include/dispatch/object.h)
	endif()

	
endif()

include_directories(
    include
	"${VSM_SDK_DIR}/include"
    ${PLATFORM_INCLUDES}
	${LIBAV_INCLUDE_DIRS}
	${CMAKE_BINARY_DIR}/include
  )

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    include_directories(
	    ${OpenCV_INCLUDE_DIRS}
    )
endif()


file(GLOB SOURCES "src/*.cpp" "src/json/*.cpp")
set(SOURCES ${SOURCES}  ${PLATFORM_SOURCES} )



add_executable(${EXECUTABLE} ${SOURCES})


message(">>>>>> ${LIBAV_AVFORMAT_LIBRARIES}")
## Specify libraries to link a library or executable target against
#TARGET_LINK_LIBRARIES(vstreamer ${OpenCV_LIBS}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${VSM_LIBS}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVCODEC_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVFORMAT_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVDEVICE_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVUTIL_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_SWSCALE_LIBRARIES})



# !!! Attention !!!
# should also patch /System/Library/Frameworks/IOKit.framework/Headers/USB.h
# change #pragma options align=reset to #pragma pack()
#
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  find_library(IOKIT NAMES IOKit)
  find_library(COREF NAMES CoreFoundation)
  TARGET_LINK_LIBRARIES(${EXECUTABLE} ${IOKIT} ${COREF})

endif()



if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  TARGET_LINK_LIBRARIES(${EXECUTABLE} ${OpenCV_LIBS})
  TARGET_LINK_LIBRARIES(${EXECUTABLE} Ws2_32 Userenv bfd iberty dbghelp z strmiids)
endif()

if (LINUX_DISTRO_VERSION VERSION_EQUAL "14.10" OR LINUX_DISTRO_VERSION VERSION_EQUAL "15.04")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libavdevice55, libswscale3")
elseif(LINUX_DISTRO_VERSION VERSION_EQUAL "16.04")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libavdevice-ffmpeg56")
else ()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libavdevice53, libswscale2")
endif ()

configure_file("${CMAKE_SOURCE_DIR}/license.txt" "${CMAKE_BINARY_DIR}/license.txt")

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")

    # Gather all target dependant libraries and patch RPATHs for libraries and executable
    # Exclude c++ runtime libraries as those are added by stdcppruntime component
	add_custom_command(
	    TARGET ${EXECUTABLE}
	    POST_BUILD
        COMMAND /bin/bash "${UGCS_BUILD_SCRIPTS_DIR}/fixup_mac_binary_rpaths.bash" ${EXECUTABLE} "/opt/local/lib" "dylibs" "libstdc++.6.dylib" "libgcc_s.1.dylib"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # Install patched libraries into common lib dir.
    install (DIRECTORY "${CMAKE_BINARY_DIR}/dylibs/"
            DESTINATION "${UGCS_INSTALL_LIB_DIR}"
            FILES_MATCHING PATTERN "*.dylib")
endif()

Add_install_target()

#Add package target.
include(CPack)

