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
#include <mutex>
#include <map>

namespace ugcs{
    namespace vstreamer {

        typedef enum {
            VSTR_SAVE_FILE, VSTR_SAVE_USTREAM
        } save_t;

        //class video_device;

        class base_save {
        public:
            void add_frame(std::map<int, video_frame*> *frames, int frame_type);

            bool is_process_running();

            /** @brief Add frame for saving.
            */
            //void add_frame(std::map<int, video_frame*> *frames, int frame_type);

            /** @brief Open capture for given device.
            */
            virtual bool init(std::string folder, std::string session_name, int width, int height, int type, int64_t request_ts) = 0;

            /** @brief Main saving loop. Waiting for new frame and initiated saving (or broadcasting) function
            */
            virtual void run() = 0;

            /** @brief Save current frame.
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

            /** @brief get recording duration */
            virtual int64_t get_recording_duration() = 0;

            video_frame *frame;

            std::string output_filename;

            int type;

            outer_stream_type_enum outer_stream_type;

            std::string state_message;

            /** @broadcasting state.*/
            outer_stream_state_enum outer_stream_state;
            /** @broadcasting error code.*/
            outer_stream_error_enum outer_strem_error_code;

        protected:

            /** condition for stopping run-loop */
            std::condition_variable stopping_condition_;
            /** stopping run-loop mutex */
            std::mutex stopping_mutex_;
            /** saving process status */
            bool is_running;
            /** init status */
            bool is_initialized;

        };
    }
}

#endif