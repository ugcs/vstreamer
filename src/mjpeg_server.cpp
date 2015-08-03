// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file mjpeg_server.cpp
*/

#include "ugcs/vstreamer/mjpeg_server.h"

namespace ugcs {

	namespace vstreamer {

		MjpegServer::MjpegServer(int port, video_device* vd) : HttpGenericServer(port) {
            init(vd);
		}

		MjpegServer::~MjpegServer() {
            if (!stop_requested_) {
                cleanUp();
            }
		}

        void MjpegServer::init(video_device* vd) {
            video_device_ = vd;
            connections_number = 0;
            encoded_buffer = NULL;
            encoded_buffer_size = 0;
			vd->port = this->port_;
        }


		void MjpegServer::sendStream(sockets::Socket_handle& fd) {
			unsigned char *frame = NULL, *tmp = NULL;
			int max_frame_size = 0;
			int tenk = 10 * 1024;
			char buffer[BUFFER_SIZE] = { 0 };
			double timestamp;

			std::string header_tmp = "Connection: close\r\nServer: vstreamer_server\r\n Cache-Control: no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n Pragma: no-cache\r\n";

            sprintf(buffer, "HTTP/1.0 200 OK\r\n"
				"%s"
				"Content-Type: multipart/x-mixed-replace;boundary=boundarydonotcross \r\n"
				"\r\n"
				"--boundarydonotcross \r\n", header_tmp.c_str());



			if (send(fd, buffer, strlen(buffer), 0) < 0) {
				free(frame);
				return;
			}

			while (!stop_requested_) {
				/* wait for fresh frames */
				std::unique_lock<std::mutex> lock(video_mutex_);
				video_condition_.wait(lock);
				lock.unlock();
				/* check if frame buffer is large enough, increase it if necessary */
				if (encoded_buffer_size > max_frame_size) {
					max_frame_size = encoded_buffer_size + tenk;
					if ((tmp = (unsigned char*)realloc(frame, (size_t) max_frame_size)) == NULL) {
						free(frame);
						sendCode(fd, 500, "not enough memory");
						LOG_ERROR("MjpegServer (%d): Not enought memory.", port_);
						return;
					}
					frame = tmp;
				}
				memcpy(frame, encoded_buffer, (size_t) encoded_buffer_size);

				timestamp = (double)utils::getMilliseconds() / 1000;
				// print the individual mimetype and the length
				// sending the content-length fixes random stream disruption observed
				// with firefox
				sprintf(buffer, "Content-Type: image/jpeg\r\n"
					"Content-Length: %d\r\n"
					"X-Timestamp: %.06lf\r\n"
					"\r\n", encoded_buffer_size, timestamp);
				if (send(fd, buffer, strlen(buffer), 0) < 0) { break; }

				if (send(fd, reinterpret_cast <const char*>(frame), encoded_buffer_size, 0) < 0) { break; }

				sprintf(buffer, "\r\n--boundarydonotcross \r\n");

				if (send(fd, buffer, strlen(buffer), 0) < 0) { break; }

			}
			free(frame);
		}


		void MjpegServer::client(sockets::Socket_handle& fd) {
			connections_number++;
			LOG("MjpegServer (%d): HTTP client (%d) connected. Current number of clients: %d", port_, fd, connections_number);
			
			// start to send video stream to client

			sendStream(fd);

			// finish sending stream
			ugcs::vstreamer::sockets::Close_socket(fd);
			connections_number--;
			LOG("MjpegServer (%d): Disconnecting HTTP client. Clients left: %d.", port_, connections_number);
			return;
		}


		void MjpegServer::execute() {

			LOG("MjpegServer (%d): Starting, waiting for clients to start video capture.", port_);

			std::thread tvid(&MjpegServer::video, this);
			tvid.detach();
			run();
		}

		void MjpegServer::cleanUp() {

			LOG("MjpegServer (%d): Cleaning up ressources allocated by server thread", port_);
            if (!stop_requested_) {
                stop_requested_ = true;

                LOG_DEBUG(" MjpegServer (%d): Waiting for video thread to finish.", port_);
                std::unique_lock<std::mutex> lock(video_mutex_);
                video_condition_.wait(lock);
                LOG_DEBUG(" MjpegServer (%d): video thread is finished.", port_);
            }

            started = false;
			for (int i = 0; i < MAX_NUM_SOCKETS; i++) {
				sockets::Close_socket(sd[i]);
			}
			video_device_->server_started = false;
			sockets::Done_sockets();
            started = false;
		}

		void MjpegServer::start() {
            stop_requested_ = false;
            started = true;
			std::thread t(&MjpegServer::execute, this);
			t.detach();
			video_device_->server_started = true;
		}

		void MjpegServer::stop() {
            if (started) {
                started = false;
                cleanUp();
            }
		}


		void MjpegServer::video() {

            bool res;

			while (!stop_requested_) {
                if (connections_number == 0	&& !video_device_->is_recording_active && !video_device_->is_outer_streams_active) {
                    // set first value for frame time even we haven't any frames yet.
                    // (for timeout handling purposes)
                    last_frame_time = utils::getMilliseconds();
                }
				// try to capture only if there are connections
				if (connections_number > 0 || video_device_->is_recording_active || video_device_->is_outer_streams_active) {

					// init capture sequence. Skip if already capturing.
					if (!video_device_->video_cap_opened) {

						//VS_WAIT(1000);
                        LOG_DEBUG("MjpegServer (%d): Start opening", port_);
                        res = video_device_->open();
                        if (!res) {
                            continue;
                        }
						// Ok then. Init is done.
                  		video_device_->video_cap_opened = true;

                        // Set no_connection_time to zero;
						LOG_INFO("MjpegServer (%d): Start capturing, connections number = %d, recording is %s", port_, connections_number, (video_device_->is_recording_active ? "on" : "off"));
					}

			        res = video_device_->get_frame(&encoded_buffer, encoded_buffer_size);

					if (res >= 0) {
                        // set frame time
                        last_frame_time = utils::getMilliseconds();

                        // done with new frame!
						std::unique_lock<std::mutex> lock(video_mutex_);
						video_condition_.notify_all();
					}
					else {
						// something wrong happend while capturing
						// free all resources, turn off cap and try again
						LOG_INFO("MjpegServer (%d): Something wrong happend while capturing. Error Code: %d. Retry!", port_, res);
                        video_device_->close();
                        video_device_->video_cap_opened = false;
					}
				}
				else {
					// if no connections, but cap was inited - free all resources
					// and turn off cap
					if (video_device_->video_cap_opened) {
                        video_device_->close();
					}
					video_device_->video_cap_opened = false;
					// wait a second before check number of connections
					VS_WAIT(1000);
				}
			}
            if (video_device_->video_cap_opened) {
                video_device_->close();
            }
            LOG_DEBUG("MjpegServer (%d): Left video stream", port_);
            video_condition_.notify_all();
            video_device_->video_cap_opened = false;
		}

        bool MjpegServer::isTimeout() {

            // if timeout for device\stream is set - handle this
            if (video_device_->timeout > 0 && connections_number > 0) {
                int64_t current_time = utils::getMilliseconds();
                if ((current_time - last_frame_time)/1000 > video_device_->timeout) {
                    LOG_DEBUG("MjpegServer (%d): Timeout occured.", port_);
                    return true;
                }
            }
            return false;
        }

		int MjpegServer::getPort(){
			return port_;
		}

	}
}

