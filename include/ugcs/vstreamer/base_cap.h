// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file base_cap.h
*
* Base abstract class for video-capturing classes
*/

#ifndef VSTREAMER_BASE_CAP_H_
#define VSTREAMER_BASE_CAP_H_

#include "ugcs/vstreamer/common.h"
#include "video_device.h"

namespace ugcs{
    namespace vstreamer {

        class video_device;

        class base_cap {
        public:

            /** @brief Open capture for given device.
            */
            virtual bool open(video_device* video_device_) = 0;

            /** @brief Get current frame from given device.
            *
            * @param video_device_ - video device to grab from.
            * @param encoded_buffer - returned frame data.
            * @param encoded_buffer_size - returned frame size.
            */
            virtual bool get_frame(video_device* video_device_, std::map<int, video_frame*> &frames, int codec_type) = 0;

            /** @brief Close capturing process, free device.
            */
            virtual void close() = 0;

            /** @brief Check device availability for capturing
            *
            * @param video_device_ - video device to check.
            */
            virtual bool check(video_device* video_device_) = 0;

        };
    }
}

#endif