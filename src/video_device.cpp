// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file video_device.cpp
*/


#include <ugcs/vstreamer/common.h>
#include "ugcs/vstreamer/video_device.h"


namespace ugcs {

    namespace vstreamer {

        video_device::video_device() {
            init();
        }

        video_device::video_device(device_type type) {
            init();
            this->type = type;
     }

        video_device::~video_device() {

            if (this->is_cap_defined && this->video_cap_opened) {
                //cap_impl->close();
            }
        }


        void video_device::init() {
            this->height = 0;
            this->width = 0;
            this->server_started = false;
            this->video_cap_opened = false;
            this->cap_type = CAP_NOT_DEFINED;
            this->is_cap_defined = false;
            this->is_recording_active = false;
            this->is_outer_streams_active = false;
            // no choises for now;
            this->file_save_impl = 0;

            this->playback_video_id="";
            this->playback_starting_pos = 0;
            this->playback_speed = 1.0;

            this->index=-1;
            this->port=-1;
            this->timeout=-1;
            this->record_request_ts=-1;
            this->playback_request_ts=-1;
            this->cap_impl = NULL;
            this->type = DEV_CAMERA; // default value

        }

        void video_device::init_outer_streams() {
            // fill outer streams (tree types for now)
            this->add_outer_stream(VSTR_OST_USTREAM, VSTR_OST_STATE_DISABLED);
            this->add_outer_stream(VSTR_OST_TWITCH, VSTR_OST_STATE_NOT_AVAILABLE);
            this->add_outer_stream(VSTR_OST_YOUTUBE, VSTR_OST_STATE_NOT_AVAILABLE);
        }


        void video_device::add_outer_stream(outer_stream_type_enum type, outer_stream_state_enum state) {
            std::shared_ptr<base_save> stream_impl;
            stream_impl = std::make_shared<ffmpeg_save_flv>();
            stream_impl->outer_stream_type = type;
            stream_impl->outer_stream_state = state;
            outer_streams[type] = stream_impl;
        }


        void video_device::init_stream(std::string parameter_string) {

            init();
            this->type = DEV_STREAM;

            std::string val = parameter_string;
            std::stringstream val_stream(val);
            std::string sub_val;
            std::getline(val_stream, sub_val, ';');
            this->name = sub_val;
            std::getline(val_stream, sub_val, ';');
            this->url = sub_val;

            if (std::getline(val_stream, sub_val, ';')) {
                if (utils::isNumeric(sub_val)) {
                    this->timeout = std::stol(sub_val);
                } else {
                    LOG("Timeout for <%s> value is not numeric or is not set. Set timeout to 60 sec.", this->name.c_str());
                    this->timeout = 60;
                }
            }

            if (std::getline(val_stream, sub_val, ';')) {
                if (utils::isNumeric(sub_val)) {
                    this->width = std::stoi(sub_val);
                } else {
                    LOG("Width for <%s> value is not numeric or is not set. Set width to 0.", this->name.c_str());
                    this->width = 0;
                }
            }

            if (std::getline(val_stream, sub_val, ';')) {
                if (utils::isNumeric(sub_val)) {
                    this->height = std::stoi(sub_val);
                } else {
                    LOG("Height for <%s> value is not numeric or is not set. Set height to 0.", this->name.c_str());
                    this->height = 0;
                }
            }

        }


        bool video_device::init_video_cap() {
            bool result;
#ifdef FFMPEG_CAP
            this->cap_impl = new ffmpeg_cap();
            result = cap_impl->check(this);
            if (result) {
                this->cap_type = CAP_FFMPEG;
                this->is_cap_defined = true;
                return true;
            }
#endif

#ifdef OPENCV_CAP
            this->cap_impl = new opencv_cap();
            result = cap_impl->check(this);
            if (result) {
                this->cap_type = CAP_OPENCV;
                this->is_cap_defined = true;
                return true;
            }
#endif
            return false;
        }


        bool video_device::open() {

            if (!this->is_cap_defined) {
                init_video_cap();
            }
            if (this->is_cap_defined) {
                return cap_impl->open(this);
            }
            return false;
        }


        bool video_device::get_frame( unsigned char **encoded_buffer, int& encoded_buffer_size){

            int flags = 0;
            if (this->is_cap_defined) {
                flags += VSTR_CODEC_MJPEG;
                if (this->is_outer_streams_active) {
                    flags += VSTR_CODEC_FLV;
                }
            }
            if (flags>0) {
                bool res = cap_impl->get_frame(this, frames, flags);

                // add frame to every enabled broadcasting
                if (res && this->video_cap_opened) {
                    for (auto iter = outer_streams.begin(); iter != outer_streams.end(); ++iter) {
                        std::shared_ptr<base_save> os = iter->second;
           //             if (os->is_process_running() && frames.count(VSTR_CODEC_FLV) > 0 && this->video_cap_opened) {
                        if (os->is_process_running()) {
                            os->add_frame(&frames, VSTR_CODEC_FLV);
                        }
                    }
                }

                // add frame to file recorder if recording is turning on.
                if (res && this->is_recording_active && this->is_cap_defined) {
                    if (file_save_impl) {
                        file_save_impl->add_frame(&frames, VSTR_CODEC_MJPEG);
                    }
                }

                // copy frame for streaming to clients
                if (res && this->is_cap_defined) {
                    if (frames.count(VSTR_CODEC_MJPEG) > 0 ) {
                        // to buffer
                        video_frame *vf = frames.at(VSTR_CODEC_MJPEG);
                        encoded_buffer_size = vf->encoded_buffer_size;
                        *encoded_buffer = (unsigned char*)realloc(*encoded_buffer, (size_t) encoded_buffer_size);
                        memcpy(*encoded_buffer, vf->encoded_buffer, (size_t) encoded_buffer_size);
                        return true;
                    }
                }
            }
            return false;
        }


        void video_device::close(){
            if (this->is_cap_defined) {
                if (cap_impl) {
                    LOG_DEBUG("Video device %s: capturing implementation is going to be closed", this->name.c_str());
                    this->video_cap_opened = false;
                    cap_impl->close();
                    LOG_DEBUG("Video device %s: capturing implementation was closed successfully", this->name.c_str());
                }
                is_cap_defined = false;
            }
        }


        bool video_device::init_recording(std::string folder, std::string filename, std::string &result_msg, int64_t request_ts) {
            result_msg = "";
            record_request_ts = request_ts;

            if (!this->video_cap_opened) {
                bool res = this->open();
                if (!res) {
                    result_msg=std::to_string(VSTR_REC_ERR_RECORD_SESSION_ERROR);
                    return false;
                }
            }
            file_save_impl = std::make_shared<ffmpeg_save_mjpeg>();
            this->is_recording_active = file_save_impl->init(folder, filename, this->width, this->height, VSTR_SAVE_FILE, record_request_ts);

            if (!this->is_recording_active) {
                result_msg=std::to_string(VSTR_REC_ERR_RECORD_SESSION_ERROR);
            }
            else {
                this->recording_video_id = filename;
            }
            return this->is_recording_active;
    }


        bool video_device::set_outer_stream(outer_stream_type_enum type, std::string url, bool is_active, std::string &result_msg) {

            result_msg = "";
            std::shared_ptr<base_save> stream_impl;

            if (outer_streams.count(type)>0) {
                stream_impl = outer_streams.at(type);
            }
            else {
                // all streams already should be initialized;
                return false;
            }

            if (stream_impl->outer_stream_state == VSTR_OST_STATE_NOT_AVAILABLE) {
                return true;
            }

            // if we try to turn on broadcats and video cap is not opened yet
            // then try to open it.
            if (!this->video_cap_opened && is_active) {
                bool res = this->open();
                if (!res) {
                    stream_impl->set_outer_stream_state(VSTR_OST_STATE_ERROR, "Video capturing open error", VSTR_OST_ERR_OPEN_VIDEO_DEVICE);
                    result_msg = stream_impl->state_message;
                    return false;
                }
            }

            bool res = true;
            if (is_active &&
                    !(stream_impl->outer_stream_state == VSTR_OST_STATE_RUNNING ||
                      stream_impl->outer_stream_state == VSTR_OST_STATE_PENDING)
                ) {
                // init broadcast
                res = stream_impl->init("", url, this->width, this->height, VSTR_SAVE_USTREAM, 0);
            } else if (stream_impl->outer_stream_state == VSTR_OST_STATE_RUNNING ||
                    stream_impl->outer_stream_state == VSTR_OST_STATE_PENDING) {
               if (!is_active) {
                   // stop stream
                   stream_impl->close();
               } else {
                   if (url != stream_impl->output_filename) {
                       // change url and reinit broadcast
                       stream_impl->close();
                       res = stream_impl->init("", url, this->width, this->height, VSTR_SAVE_USTREAM, 0);
                   }
               }
            }

            // additional req: if is_active is false, we need to update url anyway.
            if (!is_active) {
                stream_impl->set_filename("", url, VSTR_SAVE_USTREAM);
            }

            if (!res) {
                result_msg="Init outer streaming session error";
            }

            // set is_outer_streams_active flag
            bool is_anyone_running = false;
            for (auto iter=this->outer_streams.begin(); iter!=this->outer_streams.end(); ++iter) {
                std::shared_ptr<base_save> os = iter->second;
                if (os->outer_stream_state == VSTR_OST_STATE_RUNNING ||
                    os->outer_stream_state == VSTR_OST_STATE_PENDING) {
                        is_anyone_running = true;
                        break;
                }
            }
            this->is_outer_streams_active = is_anyone_running;

            return res;
        }


        void video_device::stop_recording() {
            if (this->is_recording_active) {
                // stop record session
                this->is_recording_active = false;
                // clear file name
                this->recording_video_id = "";
                // clear duration
                //this->recording_first_timestamp = -1;
                //this->recording_current_timestamp = -1;
                if (file_save_impl) {
                    this->file_save_impl->close();
                }
            }
        }


        int64_t video_device::get_recording_duration() {
            if (file_save_impl) {
                return file_save_impl->get_recording_duration();
            }
            return 0;
        }


    }
}