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

SET(CMAKE_BUILD_TYPE Debug)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

SET(UGCS_PACKAGE_SUMMARY "UgCS Mjpeg Video Streamer used to stream video from different devices along with postprocessing")
set(UGCS_VSM_USER_GUIDE_LATEX_TITLE "Video Streamer User Guide")


SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"
					  "${CMAKE_SOURCE_DIR}/../build-scripts/cmake"
					  "${VSM_SDK_DIR}/share/cmake")

include("configure_packaging")

include("FindLibav")

include("vstreamer_common")


if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    FIND_PACKAGE(OpenCV REQUIRED)
    add_definitions( -DOPENCV_CAP=1 )
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
	
	FILE(COPY	"/System/Library/Frameworks/IOKit.framework/Headers/usb/USB.h"
				DESTINATION "${CMAKE_BINARY_DIR}/include/IOKit/usb"
				NO_SOURCE_PERMISSIONS)
	SET(ENV{LC_TYPE} "C")
	SET(ENV{LANG} "C")
	execute_process(COMMAND sed -i "" "s/#pragma options align=reset/#pragma pack()/g" ${CMAKE_BINARY_DIR}/include/IOKit/usb/USB.h)
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


## Specify libraries to link a library or executable target against
#TARGET_LINK_LIBRARIES(vstreamer ${OpenCV_LIBS}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${VSM_LIBS}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVCODEC_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVFORMAT_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVDEVICE_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_AVUTIL_LIBRARIES}) 
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${LIBAV_SWSCALE_LIBRARIES})

message(">>>>>>>>> ${LIBAV_AVFORMAT_LIBRARIES}")

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
else ()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libavdevice53, libswscale2")
endif ()

function(Install_system_library TARGET_NAME LIB_NAME)
    #get_target_property(MY_LIBFILE ${TARGET_NAME} LOCATION)
	set(MY_LIBFILE "${CMAKE_BINARY_DIR}/${TARGET_NAME}")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		add_custom_command(
				TARGET ${EXECUTABLE}
				POST_BUILD
				COMMAND /bin/bash "${UGCS_BUILD_SCRIPTS_DIR}/get_mac_lib_from_binary.bash" ${MY_LIBFILE} ${LIB_NAME} ${CMAKE_BINARY_DIR})
	endif()


	# Add library to installation
    install(FILES "${CMAKE_BINARY_DIR}/${LIB_NAME}"
            DESTINATION "${UGCS_INSTALL_LIB_DIR}")
endfunction()


configure_file("${CMAKE_SOURCE_DIR}/license.txt" "${CMAKE_BINARY_DIR}/license.txt")


if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	#fmpeg
    #Install_system_library(${EXECUTABLE} "libgcc_s.1.dylib")
    #Install_system_library(${EXECUTABLE} "libstdc++.6.dylib")

	Install_system_library(${EXECUTABLE} "libavcodec.55.dylib")
	Install_system_library(${EXECUTABLE} "libavformat.55.dylib")
	Install_system_library(${EXECUTABLE} "libavutil.52.dylib")
	Install_system_library(${EXECUTABLE} "libswscale.2.dylib")
	Install_system_library(${EXECUTABLE} "libavdevice.55.dylib")

	Install_system_library("libavdevice.55.dylib" "libavfilter.4.dylib")
	Install_system_library("libavdevice.55.dylib" "libpostproc.52.dylib")
	Install_system_library("libavdevice.55.dylib" "libavresample.1.dylib")

	Install_system_library("libavcodec.55.dylib" "libswresample.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libiconv.2.dylib")
	Install_system_library("libavcodec.55.dylib" "libSDL-1.2.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libnettle.4.dylib")
	Install_system_library("libavcodec.55.dylib" "libhogweed.2.dylib")
	Install_system_library("libavcodec.55.dylib" "libgmp.10.dylib")
	Install_system_library("libavcodec.55.dylib" "libx264.142.dylib")
	Install_system_library("libavcodec.55.dylib" "libvorbisenc.2.dylib")
	Install_system_library("libavcodec.55.dylib" "libvorbis.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libogg.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libtheoraenc.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libtheoradec.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libspeex.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libschroedinger-1.0.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libopus.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libopenjpeg.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libmp3lame.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libmodplug.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libfribidi.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libglib-2.0.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libintl.8.dylib")
	Install_system_library("libavcodec.55.dylib" "libfreetype.6.dylib")
	Install_system_library("libavcodec.55.dylib" "libfontconfig.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libbluray.1.dylib")
	Install_system_library("libavcodec.55.dylib" "libass.5.dylib")
	Install_system_library("libavcodec.55.dylib" "libgnutls.28.dylib")
	Install_system_library("libavcodec.55.dylib" "libbz2.1.0.dylib")
	Install_system_library("libavcodec.55.dylib" "libz.1.dylib")
	Install_system_library("libavformat.55.dylib" "libX11.6.dylib")
	Install_system_library("libSDL-1.2.0.dylib" "libXext.6.dylib")
	Install_system_library("libSDL-1.2.0.dylib" "libXrandr.2.dylib")
	Install_system_library("libSDL-1.2.0.dylib" "libXrender.1.dylib")
	Install_system_library("libschroedinger-1.0.0.dylib" "liborc-0.4.0.dylib")
	Install_system_library("libfreetype.6.dylib" "libpng16.16.dylib")
	Install_system_library("libfontconfig.1.dylib" "libexpat.1.dylib")
	Install_system_library("libbluray.1.dylib" "libxml2.2.dylib")
	Install_system_library("libass.5.dylib" "libenca.0.dylib")
	Install_system_library("libgnutls.28.dylib" "libp11-kit.0.dylib")
	Install_system_library("libgnutls.28.dylib" "libtasn1.6.dylib")
	Install_system_library("libX11.6.dylib" "libxcb.1.dylib")
	Install_system_library("libxml2.2.dylib" "liblzma.5.dylib")
	Install_system_library("libp11-kit.0.dylib" "libffi.6.dylib")
	Install_system_library("libxcb.1.dylib" "libXau.6.dylib")
	Install_system_library("libxcb.1.dylib" "libXdmcp.6.dylib")

	LIST(APPEND LIB_NAME_LIST "libswscale.2.dylib" "libavcodec.55.dylib" "libavformat.55.dylib" "libavdevice.55.dylib" "libavutil.52.dylib" "libswresample.0.dylib" "libiconv.2.dylib" "libSDL-1.2.0.dylib" "libnettle.4.dylib" "libhogweed.2.dylib" "libgmp.10.dylib" "libx264.142.dylib" "libvorbisenc.2.dylib" "libvorbis.0.dylib" "libogg.0.dylib" "libtheoraenc.1.dylib" "libtheoradec.1.dylib" "libspeex.1.dylib" "libschroedinger-1.0.0.dylib" "libopus.0.dylib" "libopenjpeg.1.dylib" "libmp3lame.0.dylib" "libmodplug.1.dylib" "libfribidi.0.dylib" "libglib-2.0.0.dylib" "libintl.8.dylib" "libfreetype.6.dylib" "libfontconfig.1.dylib" "libbluray.1.dylib" "libass.5.dylib" "libgnutls.28.dylib" "libbz2.1.0.dylib" "libz.1.dylib" "libX11.6.dylib" "libavfilter.4.dylib" "libpostproc.52.dylib" "libavresample.1.dylib" "libXext.6.dylib" "libXrandr.2.dylib" "libXrender.1.dylib" "liborc-0.4.0.dylib" "libpng16.16.dylib" "libexpat.1.dylib" "libxml2.2.dylib" "libenca.0.dylib" "libp11-kit.0.dylib" "libtasn1.6.dylib" "libxcb.1.dylib" "liblzma.5.dylib" "libffi.6.dylib" "libXau.6.dylib" "libXdmcp.6.dylib")	


    #Patch_mac_binary("${EXECUTABLE}" "libgcc_s.1.dylib")
    #Patch_mac_binary("${EXECUTABLE}" "libstdc++.6.dylib" "libgcc_s.1.dylib")


	FOREACH(ITEM ${LIB_NAME_LIST})
		Patch_mac_binary("${EXECUTABLE}" ${ITEM} ${LIB_NAME_LIST})
	ENDFOREACH()

	Patch_mac_binary("${EXECUTABLE}" "${EXECUTABLE}" ${LIB_NAME_LIST})
	
	
endif()

Add_install_target()

#Add package target.
include(CPack)
