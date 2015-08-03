// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* platform_video.h
*
* Defines platform specific parts of socket implementation.
*/

#ifndef _WIN32
#error "This header should be included only in Windows build."
#endif

#ifndef VSTREAMER_PLATFORM_VIDEO_H_
#define VSTREAMER_PLATFORM_VIDEO_H_

#define DEVICE_FFMPEG_NAME_PREFIX "video="

#include <strmif.h>
#include <locale>

#define DEVICES_INPUT_FORMAT "dshow"

DEFINE_GUID(CLSID_VideoInputDeviceCategory, 0x860bb310, 0x5d01, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);

namespace ugcs {
    namespace vstreamer {
        namespace video {
            std::string convert_ansi_to_utf8(std::string);
        }
    }
}

#endif /* VSTREAMER_PLATFORM_VIDEO_H_ */
