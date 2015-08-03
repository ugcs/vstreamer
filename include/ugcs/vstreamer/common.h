// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file common.h
*
* Common vstreamer declarations
*/

#ifndef VSTREAMER_COMMON_H_
#define VSTREAMER_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <condition_variable>
#include <mutex>

#define VS_WAIT(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms));

#define VSTR_RECORDING_VIDEO_EXTENSION "mkv"
#define VSTR_RECORDING_VIDEO_METADATA_EXTENSION "md"

namespace ugcs{
namespace vstreamer {

    /** Codecs types */
    const int VSTR_CODEC_MJPEG = 1;
    const int VSTR_CODEC_FLV = 2;
    const int VSTR_CODEC_SOMETHING = 4;

    class video_device;

    /** vstreamer configuration parameters class
    */
    typedef struct {
        /** Is autodetection of devices is turned on */
        bool autodetect;
        /** Timeout for video devices while streamer tries to grab a frame */
        int64_t videodevices_timeout;
        /** folder for saved video */
        std::string saved_video_folder;
        /** input http (or any other) streams */
        std::vector<video_device> input_streams;
        /** excluded devices (no stream for them) */
        std::vector<std::string> excluded_devices;
        /** devices that will be checked for stream even thea are not autodetected */
        std::vector<std::string> allowed_devices;
    } vstreamer_parameters;

    typedef struct {
        unsigned char *encoded_buffer;
        int encoded_buffer_size;
        int64_t ts;
        /** condition for video stream */
        std::condition_variable video_condition_;
        /** video stream mutex */
        std::mutex video_mutex_;

    } video_frame;

    /** outer (broadcating) stream type */
    typedef enum {
        VSTR_OST_USTREAM, VSTR_OST_TWITCH, VSTR_OST_YOUTUBE
    } outer_stream_type_enum;

    /** outer (broadcating) stream type */
    typedef enum {
        VSTR_OST_STATE_NOT_AVAILABLE, VSTR_OST_STATE_DISABLED, VSTR_OST_STATE_PENDING, VSTR_OST_STATE_RUNNING, VSTR_OST_STATE_ERROR
    } outer_stream_state_enum;

    typedef enum {
        VSTR_OST_ERR_UNKNOWN, VSTR_OST_ERR_OPEN_VIDEO_DEVICE, VSTR_OST_ERR_SEND_DATA, VSTR_OST_ERR_OPEN_CODEC, VSTR_OST_ERR_CODEC_NOT_FOUND, VSTR_OST_ERR_URL, VSTR_OST_ERR_NONE
    } outer_stream_error_enum;

    typedef enum {
        VSTR_REC_ERR_UNKNOWN, VSTR_REC_ERR_VIDEO_NOT_FOUND, VSTR_REC_ERR_VIDEO_ALREADY_EXISTS, VSTR_REC_ERR_DEVICE_NOT_FOUND, VSTR_REC_ERR_RECORDING_IS_ALREADY_IN_PROCESS, VSTR_REC_ERR_METADATA_NOT_FOUND, VSTR_REC_ERR_RECORD_SESSION_ERROR
    } recording_playback_error_enum;

    const std::string VSTR_OST_USTREAM_NAME = "ustream";
    const std::string VSTR_OST_TWITCH_NAME = "twitch";
    const std::string VSTR_OST_YOUTUBE_NAME = "youtube";
    const std::string VSTR_OST_UNKNOWN_NAME = "unknown";

}
}

#endif //VSTREAMER_COMMON_H_