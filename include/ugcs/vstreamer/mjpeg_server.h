// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file mjpeg_server.h
*
* Mjpeg http streaming servers realisation
*/


#ifndef VSTREAMER_MJPEG_SERVER_H_
#define VSTREAMER_MJPEG_SERVER_H_

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

namespace ugcs{
	namespace vstreamer {
	
		/**
		 * @class MJPEGServer
		 * @brief
		 */
		class MjpegServer:HttpGenericServer
		{
		public:
			/**
			 * @brief  Constructor
			 * @return
			 */
			MjpegServer(int port, video_device* vd);

			/**
			 * @brief  Destructor - Cleans up
			 */
			virtual ~MjpegServer();

			/**
			 * @brief Starts server image processing
			 */
			void execute();

			/**
			 * @brief  Starts the server and spins
			 */
			void start();

			/**
			 * @brief  Stops the server
			 */
			void stop();

            /**
            * @brief  Init server with video_device
            */
            void init(video_device* vd);

			/**
			 * @brief  Closes all client threads
			 */
			void cleanUp();

			/**
			 * @brief  Client thread function
			 *         Serve a connected TCP-client. This thread function is called
			 *         for each connect of a HTTP client. It determines
			 *         if it is a valid HTTP request and dispatches between the different
			 *         response options.
			 */
			void client(sockets::Socket_handle& fd);

            /** @brief Check if timeout ocurred for current device */
            bool isTimeout();

            /** @brief Server state */
            bool started;

			int getPort();

            /** @brief video device server streams */
            video_device* video_device_;

		private:

            /** @brief number of connections to server. When connection number equal to zero
            * image processing stops
            */
			int connections_number;
            /** buffer with image data to stream */
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
#endif // VSTREAMER_MJPEG_SERVER_H

