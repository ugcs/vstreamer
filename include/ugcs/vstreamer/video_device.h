// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* @file video_device.h
*
* Defines Video Device class.
*/

#ifndef VSTREAMER_VIDEO_DEVICE_H_
#define VSTREAMER_VIDEO_DEVICE_H_

#include <ugcs/vsm/vsm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>



#include "ugcs/vstreamer/utils.h"

#include "ugcs/vstreamer/base_cap.h"

#ifdef FFMPEG_CAP
#include "ugcs/vstreamer/ffmpeg_cap.h"
#endif

#ifdef OPENCV_CAP
#include "ugcs/vstreamer/opencv_cap.h"
#endif

#include "ugcs/vstreamer/ffmpeg_save_mjpeg.h"
#include "ugcs/vstreamer/ffmpeg_save_flv.h"

#define VS_WAIT(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms));

namespace ugcs{
    namespace vstreamer {

        /** Device type: camera or stream */
        typedef enum {
            DEV_CAMERA, DEV_STREAM, DEV_FILE
        } device_type;

        /** Device capturing type */
        typedef enum {
            CAP_NOT_DEFINED, CAP_FFMPEG, CAP_OPENCV
        } cap_type;



        class base_save;

        /** @class video_device */
        class video_device {
        public:

            /** @brief Constructor */
            video_device();

            /** @brief Constructor
            * @param device type (DEV_CAMERA or DEV_STREAM)
            */
            video_device(device_type type);

            /** @brief Destructor */
            ~video_device();

            /** @brief Init device of stream type with data from config
            * @param string from config like Ardrone;tcp://192.168.1.1:5555;60;640;360. (Name;URL;Timeout;Width;Height)
            */
            void init_stream(std::string parameter_string);

            /** @brief Determine and init video cap realisation for device.
            * @return true if initialisation is successful
            */
            bool init_video_cap();

            /** @brief Inint outer streams (ustream\twitch\youtube)
            */
            void init_outer_streams();

            /** @brief Open video cap for device. If video cap is not initialised yet - it will be
            * @return true if video cap opens successfully
            */
            bool open();

            /** @brief Get current frame data from device
            * @param buffer with data (out)
            * @param size of buffer with data (out)
            * @return true if success
            */
            bool get_frame(unsigned char **encoded_buffer, int& encoded_buffer_size);

            /** @brief Init recording session.
            *
            * @param filename - filename without extension.
            * @param result_msg - message with error or noerror info.
            */
            bool init_recording(std::string folder, std::string filename, std::string &result_msg, int64_t request_ts);

            /** @brief get duration for currently recording file.
           */
            int64_t get_recording_duration();


            void stop_recording();


            bool set_outer_stream(outer_stream_type_enum type, std::string url, bool is_active, std::string &result_msg);


            /** @brief Close video cap and free device */
            void close();

            /** device name */
            std::string name;
            /** device type (camera or stream) */
            device_type type;
            /** device index (has a meaning for cameras for certain capturer realisations) */
            int index;
            /** url of a stream */
            std::string url;
            /** port of streaming server assotiated with this device */
            int port;
            /** http srever state */
            bool server_started;
            /** video cap state */
            bool video_cap_opened;
            /** device timeout setting (from config) */
            int64_t timeout;
            /** device image width */
            int width;
            /** device image height */
            int height;
            /** device capturing type (CAP_FFMPEG, CAP_OPENCV, ...) */
            int cap_type;
            /** name of recording file */
            std::string recording_video_id;
            /** last recording error code */
            std::string last_recording_error_code;
            /** is recording session active now */
            bool is_recording_active;

            bool is_outer_streams_active;
            /** broadcasting streams */
            std::map<int, std::shared_ptr<base_save>> outer_streams;
            std::shared_ptr<base_save> o_stream_tmp;
            // video id for playback given from client.
            // equial to filename without path and extension.
            std::string playback_video_id;
            // playback starting position
            int64_t playback_starting_pos;
            // playback speed
            double playback_speed;
            // record request timestamp for futher video sync
            int64_t record_request_ts;
            // playback request timestamp for correct video timing
            int64_t playback_request_ts;

        private:
            /** Initialisation of device */
            void init();
            /** true if cap is already initialised */
            bool is_cap_defined;
            /** video cap implementation */
            base_cap* cap_impl;
            /** video rec implementation */
            std::shared_ptr<base_save> file_save_impl;

            std::map<int, video_frame*> frames;

            void add_outer_stream(outer_stream_type_enum type, outer_stream_state_enum state);


        };
    }
}

#endif //VSTREAMER_VIDEO_DEVICE_H_