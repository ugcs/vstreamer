// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* @file video.h
*
* Defines platform specific parts of video tasks implementation.
*/

#ifndef VSTREAMER_VIDEO_H_
#define VSTREAMER_VIDEO_H_

#include <ugcs/vsm/vsm.h>
#include <ugcs/vstreamer/common.h>
#include <ugcs/vstreamer/video_device.h>


#include <ugcs/vstreamer/platform_video.h>
#include <algorithm>


#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

#include <vector>
#include <string>

namespace ugcs {
	namespace vstreamer {

        class video_device;

        namespace video {

            /**
            * @brief performs autodetection of video devices and return its names
            * @param devices found
            * @return number of devices
            */
            int Get_autodetected_device_list(std::vector<std::string> &found_devices);

            /**
            * @brief forms a complete devices list includeing autodetected devices and data from configuration file
            * @param devices list
            * @param configuration data
            */
			void Get_device_list(std::vector<video_device> &found_devices, vstreamer_parameters params);

		}
	}
}

#endif /* VSTREAMER_VIDEO_H_ */
