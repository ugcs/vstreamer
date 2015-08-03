// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file control_server.h
*
* Control server declarations
*/


#ifndef VSTREAMER_CONTROL_SERVER_H_
#define VSTREAMER_CONTROL_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ugcs/vsm/vsm.h>

#include <thread>
#include <chrono>

#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/sockets.h"
#include "ugcs/vstreamer/utils.h"
#include "ugcs/vstreamer/http_generic_server.h"
#include "ugcs/vstreamer/mjpeg_server.h"
#include "ugcs/vstreamer/video.h"
#include <ugcs/vstreamer/video_device.h>
#include <ugcs/vstreamer/ffmpeg_playback.h>
#include <json/json.h>


#define CONTROL_HELP_MESSAGE "<html><b>Use the following links to control streaming server</b><br><ul><li><a href=\"/streams\">Get streams info</a></li><li><a href=\"/parameters\">Get or set parameters</a></li></ul></html>"
#define CHUNK_SIZE 64000

namespace ugcs{
namespace vstreamer {

    class video_device;
	  
class ControlServer : HttpGenericServer {
		public:
			/**
			 * @brief  Constructor
			 */
			ControlServer(int port);

			/**
			 * @brief  Destructor - Cleans up
			 */
			 ~ControlServer();

			/**
			 * @brief  Starts the server
			 */
			void start();

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
			
		private:

            /** vstreamer server parametrers from vstreamer.conf*/
            vstreamer_parameters server_parameters;

            /** current maximum port of http-streamer. Next streaming serevers will be opend with next port */
			int max_port_;

            //** overall devices list: autodetected + streams + allowed - excluded */
			std::map<std::string, video_device> device_list;


			std::map<std::string, ugcs::vstreamer::MjpegServer*> http_servers;

			/**
			 * @brief  detect devices and runs mjpeg servers
			 */
			void execute();
		  
			/**
			 * @brief  Get list of video devices (cameras, streams...)
			 */
			void scanForDevices();

			/**
			* @brief check device availability for every registered capturer (ffmpeg, opencv, etc). First working capturer
            * will be set as device.cap_type. If any capturer is found device is added to device_list and http port is set
            * for it.
            *
            * @param dev - video device to check
			*/
			void checkDeviceAvailability(video_device dev);


            /**
            * @brief  send help message to client
            *
            * @param fd - client socket
            */
			void sendHelpMessage(ugcs::vstreamer::sockets::Socket_handle& fd);

            /**
            * @brief  create and send JSON-message with info about current streams
            *
            * @param fd - client socket
            */
            void sendStreamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd);

            /**
            * @brief  create and send JSON-message with info about params value
            *
            * @param fd - client socket
            */
            void sendParamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd);

            /**
            * @brief  accept JSON-message with info about params value and update parameters with new values
            *
            * @param fd - client socket
            */
            void writeParamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body);

			/**
            * @brief
            */
			void startPlayback(ugcs::vstreamer::sockets::Socket_handle& fd, std::string query);

			/**
			* @brief
			*/
			void getVideoMetadata(ugcs::vstreamer::sockets::Socket_handle& fd, std::string video_id);

			void deleteVideo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string video_id);

			void downloadVideo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string video_id);

			void writeStreamInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body);

			void writeOuterStreamInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body);

			int find_next_port();

            std::mutex broadcast_mutex;

	    };
  
}
}



#endif //VSTREAMER_CONTROL_SERVER_H_