// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file mjpeg_server.cpp
*/

#include "ugcs/vstreamer/ffmpeg_playback.h"

namespace ugcs {

    namespace vstreamer {

        ffmpeg_playback::ffmpeg_playback(video_device* vd) {
            init(vd);
        }

        ffmpeg_playback::~ffmpeg_playback() {
            if (!stop_requested) {
                cleanUp();
            }
        }

        void ffmpeg_playback::init(video_device* vd) {
            video_device_ = vd;
            encoded_buffer = NULL;
            encoded_buffer_size = 0;
            last_frame_time = 0;
        }


        void ffmpeg_playback::sendStream(sockets::Socket_handle& fd) {
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

            while (!stop_requested) {
                /* wait for fresh frames */
                std::unique_lock<std::mutex> lock(video_mutex_);
                video_condition_.wait(lock);
                lock.unlock();
                /* check if frame buffer is large enough, increase it if necessary */
                if (encoded_buffer_size > max_frame_size) {
                    max_frame_size = encoded_buffer_size + tenk;
                    if ((tmp = (unsigned char*)realloc(frame, (size_t)max_frame_size)) == NULL) {
                        free(frame);
                        int ret = sockets::Send_code(fd, 500, "not enough memory");
                        if (ret < 0) {
                            LOG_ERROR("Playback process (%s): write failed, done anyway, error code: %d", video_device_->playback_video_id.c_str(), ret);
                        }
                        else {
                            LOG_DEBUG("Playback process (%s): write ok", video_device_->playback_video_id.c_str());
                        }
                        LOG_ERROR("Playback process (%s): Not enought memory.", video_device_->playback_video_id.c_str());
                        return;
                    }
                    frame = tmp;
                }
                memcpy(frame, encoded_buffer, (size_t)encoded_buffer_size);

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


        void ffmpeg_playback::start(sockets::Socket_handle& fd) {
            stop_requested = false;
            started = true;

            // start file reading
            std::thread t(&ffmpeg_playback::video, this);
            t.detach();

            LOG("Playback process (%s): starting to stream file", video_device_->playback_video_id.c_str());
            // start to send video stream to client
            sendStream(fd);
            // finish sending stream
            ugcs::vstreamer::sockets::Close_socket(fd);
            LOG("Playback process (%s): finishing to stream file", video_device_->playback_video_id.c_str());
            cleanUp();
            return;
        }



        void ffmpeg_playback::cleanUp() {

            LOG("Playback process (%s): Cleaning up ressources allocated by playback thread", video_device_->playback_video_id.c_str());
            if (!stop_requested) {
                stop_requested = true;
                LOG_DEBUG("Playback process (%s): Waiting for video thread to finish", video_device_->playback_video_id.c_str());
                std::unique_lock<std::mutex> lock(video_mutex_);
                video_condition_.wait(lock);
                LOG_DEBUG("Playback process (%s): video thread is finished.", video_device_->playback_video_id.c_str());
            }

            started = false;
        }


        void ffmpeg_playback::video() {

            bool res;

            while (!stop_requested) {
                last_frame_time = utils::getMilliseconds();

                // init capture sequence. Skip if already capturing.
                if (!video_device_->video_cap_opened) {

                    LOG_DEBUG("Playback process (%s): Start opening.", video_device_->playback_video_id.c_str());
                    res = video_device_->open();
                    if (!res) {
                        continue;
                    }
                    // Ok then. Init is done.
                    video_device_->video_cap_opened = true;

                    // Set no_connection_time to zero;
                    LOG_INFO("Playback process (%s): Start reading.", video_device_->playback_video_id.c_str());
                }

                res = video_device_->get_frame(&encoded_buffer, encoded_buffer_size);

                if (res) {
                    // set frame time
                    last_frame_time = utils::getMilliseconds();

                    // done with new frame!
                    std::unique_lock<std::mutex> lock(video_mutex_);
                    video_condition_.notify_all();
                }
                else {
                    // something wrong happend while capturing
                    // free all resources, turn off cap and shut down thread
                    LOG_INFO("Playback process (%s): Something wrong happend while reading file.", video_device_->playback_video_id.c_str());
                    cleanUp();
                    video_device_->video_cap_opened = false;
                }

            }
            if (video_device_->video_cap_opened) {
                video_device_->close();
            }
            LOG_DEBUG("Playback process (%s): Left video stream", video_device_->playback_video_id.c_str());
            video_condition_.notify_all();
            video_device_->video_cap_opened = false;
        }

    }
}

