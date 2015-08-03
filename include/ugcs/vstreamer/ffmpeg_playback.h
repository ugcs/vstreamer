// Copyright (c) 2015, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_playback.h
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include <ugcs/vsm/vsm.h>

#include "ugcs/vstreamer/sockets.h"
#include "ugcs/vstreamer/utils.h"
#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/http_generic_server.h"
#include "ugcs/vstreamer/video.h"


#ifndef VSTREAMER_FFMPEG_PLAYBACK_H_
#define VSTREAMER_FFMPEG_PLAYBACK_H_
namespace ugcs{
    namespace vstreamer {

        /**
         * @class ffmpeg_playback
         * @brief
         */
        class ffmpeg_playback
        {
        public:
            /**
             * @brief  Constructor
             * @return
             */
            ffmpeg_playback(video_device* vd);

            /**
             * @brief  Destructor - Cleans up
             */
            ~ffmpeg_playback();

            /**
            * @brief  init class
            */
            void init(video_device* vd);

             /**
             * @brief  Closes all client threads
             */
            void cleanUp();

             /**
             * @brief starts video reading and streaming
             * for one client only
             */
            void start(sockets::Socket_handle& fd);

            /** @brief video device server streams */
            video_device* video_device_;

            // is playback started
            bool started;

        private:
            /** stop requested flag. When true - server begins stopping sequence*/
            bool stop_requested;
            /** with image data to stream */
            unsigned char *encoded_buffer;
            /** size of buffer with image data to stream */
            int encoded_buffer_size;
            /** condition for video stream */
            std::condition_variable video_condition_;
            /** video stream mutex */
            std::mutex video_mutex_;
            /** timestamp when last frame was recieved (for timeout detection) */
            int64_t last_frame_time;

            /** @brief loop for video capturing */
            void video();
            /**
             * @brief Send a complete HTTP response and a stream of JPG-frames.
             * @param fildescriptor fd to send the answer to
             */
            void sendStream(sockets::Socket_handle& fd);

        };

    }
}
#endif // VSTREAMER_FFMPEG_PLAYBACK_H_