// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file base_save.h
*
* Base abstract class for video-saving classes
*/

#ifndef VSTREAMER_BASE_SAVE_H_
#define VSTREAMER_BASE_SAVE_H_

#include "ugcs/vstreamer/common.h"

namespace ugcs{
    namespace vstreamer {

        typedef enum {
            VSTR_SAVE_FILE, VSTR_SAVE_USTREAM
        } save_t;

        //class video_device;

        class base_save {
        public:

            /** @brief Open capture for given device.
            */
            virtual bool init(std::string folder, std::string session_name, int width, int height, int type, int64_t request_ts) = 0;

            virtual void run() = 0;

            bool is_running;
            /** @broadcasting state.*/
            outer_stream_state_enum outer_stream_state;
            /** @broadcasting error code.*/
            outer_stream_error_enum outer_strem_error_code;

            /** @brief Save given frame.
            *
            * @param encoded_buffer - frame data to save.
            * @param encoded_buffer_size - frame size.
            * @param ts - timestamp in milliseconds.
            */
            virtual bool save_frame() = 0;

            virtual bool set_filename(std::string folder, std::string filename, int type) = 0;

            virtual void set_outer_stream_state(outer_stream_state_enum state, std::string msg, outer_stream_error_enum error_code = VSTR_OST_ERR_NONE) = 0;

            /** @brief Save dummy frame (black screen or something).
            *
            * @param ts - timestamp in milliseconds.
            */
            virtual bool save_dummy_frame(int64_t ts) = 0;

            /** @brief Close saving process, free device.*/
            virtual void close() = 0;

            video_frame *frame;

            std::string output_filename;

            int type;

            outer_stream_type_enum outer_stream_type;

            std::string state_message;

        };
    }
}

#endif