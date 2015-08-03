// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_save.cpp
*/

#include "ugcs/vstreamer/ffmpeg_save_flv.h"

namespace ugcs {

    namespace vstreamer {


        ffmpeg_save_flv::ffmpeg_save_flv() {
            frame = new video_frame();
            flv_packet = new AVPacket();
            save_done = true;
            stop_init = false;
            outer_strem_error_code = VSTR_OST_ERR_NONE;
        }


        ffmpeg_save_flv::~ffmpeg_save_flv() {
            close();
        }


        bool ffmpeg_save_flv::set_filename(std::string folder, std::string filename, int type) {
            if (type == VSTR_SAVE_FILE) {
                if (folder.back() != '\\') {
                    folder += "\\";
                }
                output_filename = folder + filename + ".flv";
            } else if (type == VSTR_SAVE_USTREAM) {
                output_filename = filename;

                // simple check for valid url
                regex::smatch sm;
                regex::regex re ("(http|https|rtmp)\\://){1}\\S+)");
                if (!regex::regex_match(filename, sm, re)) {
                    return false;
                }
            }
            else {
                return false;
            }
            return true;
        }

        bool ffmpeg_save_flv::init(std::string folder, std::string session_name, int width, int height, int type, int64_t request_ts) {


            this->type = type;

            // register all ffmpeg modules
            av_register_all();
            avdevice_register_all();
            avcodec_register_all();
            avformat_network_init();

            // on avlibcodec 54 and 53 (linux) we cannot create MJPEG encoder for pix_fmt=AV_PIX_FMT_YUV420P, so
// we need to use AV_PIX_FMT_YUVJ420P. But in versions 55+ this format is deprecated. So on, in version
// 56.1.0 (Ubuntu 14.10) we need use AV_PIX_FMT_YUVJ420P again.
#if ((LIBAVCODEC_VERSION_INT >= ((55<<16)+(0<<8)+0)) && (LIBAVCODEC_VERSION_INT < ((56<<16)+(1<<8)+0)))
            pEncodedFormat = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_YUVJ420P;
#else
            pEncodedFormat = AV_PIX_FMT_YUVJ420P; //AV_PIX_FMT_YUVJ420P;
#endif
            bool res;
            res = this->set_filename(folder, session_name, type);
            if (!res) {
                set_outer_stream_state(VSTR_OST_STATE_ERROR, "Incorrect URL format", VSTR_OST_ERR_URL);
                LOG_ERR("Save Session (%s): %s \n", session_name.c_str(), state_message.c_str());
                return false;
            }

            // init flv
            LOG_DEBUG("FLV Save Initiation...");
            res = init_flv(session_name, width, height);
            if (!res) {
                return false;
            }
            LOG_DEBUG("FLV Save Init done! Starting saving process.");

            std::thread t(&ffmpeg_save_flv::run, this);
            t.detach();

            LOG_DEBUG("FLV Saving process started");
            return true;
        }

        void ffmpeg_save_flv::run() {
            set_outer_stream_state(VSTR_OST_STATE_PENDING, "");
            stop_init = false;
            while (this->outer_stream_state == VSTR_OST_STATE_RUNNING ||
                    this->outer_stream_state == VSTR_OST_STATE_PENDING) {
                std::unique_lock<std::mutex> lock(this->frame->video_mutex_);
                this->frame->video_condition_.wait(lock);
                if (stop_init) {break;}
                this->save_frame();
            }
            if (stop_init) {
                VS_WAIT(100);
                this->outer_stream_stop_condition.notify_all();
            }
        }


        bool ffmpeg_save_flv::init_flv(std::string session_name, int width, int height) {

            // find the flv video encoder (for output)
            set_outer_stream_state(VSTR_OST_STATE_PENDING, "");

#if (LIBAVCODEC_VERSION_MAJOR < 54)
            flv_codec = avcodec_find_encoder(CODEC_ID_FLV1);
#else
            flv_codec = avcodec_find_encoder(AV_CODEC_ID_FLV1);
#endif
            if (!flv_codec) {

                set_outer_stream_state(VSTR_OST_STATE_ERROR, "FLV1 Codec not found!", VSTR_OST_ERR_CODEC_NOT_FOUND);
                LOG_ERR("Save Session (%s): %s \n", session_name.c_str(), state_message.c_str());
                return false;
            }
            // allocate flv codec context (for output)
            flv_codec_context = avcodec_alloc_context3(flv_codec);
            if (!flv_codec_context) {
                set_outer_stream_state(VSTR_OST_STATE_ERROR, "Could not allocate video FLV1 codec context!", VSTR_OST_ERR_OPEN_CODEC);
                LOG_ERR("Save Session (%s): %s \n", session_name.c_str(), state_message.c_str());
                return false;
            }
            flv_format_context = avformat_alloc_context();
            flv_fmt = av_guess_format("flv", output_filename.c_str(), NULL);
            flv_format_context->oformat = flv_fmt;
            flv_stream = avformat_new_stream(flv_format_context, flv_codec);
            flv_stream->codec = flv_codec_context;
            flv_codec_context->pix_fmt = pEncodedFormat;

            flv_codec_context->width = width;
            flv_codec_context->height = height;

            flv_codec_context->qmin=2;
            flv_codec_context->qmax=32;

            //flv_codec_context->time_base.den = 30;
            //flv_codec_context->time_base.num = 1;

            flv_stream->time_base.den = 1000;
            flv_stream->time_base.num = 1;

            // add headers and flags for saving
            flv_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;

            av_dump_format(flv_format_context, 0, output_filename.c_str(), 1);

            int ret = avio_open(&flv_format_context->pb, output_filename.c_str(), AVIO_FLAG_WRITE);

            if (ret < 0) {
                set_outer_stream_state(VSTR_OST_STATE_ERROR, "Could not open output.", VSTR_OST_ERR_SEND_DATA);
                LOG_ERR("Save Session (%s): %s \n", session_name.c_str(), state_message.c_str());
                return false;
            }

            ret = avformat_write_header(flv_format_context, NULL);
            if (ret < 0) {
                set_outer_stream_state(VSTR_OST_STATE_ERROR, "Could not open output header.", VSTR_OST_ERR_SEND_DATA);
                LOG_ERR("Save Session (%s): %s \n", session_name.c_str(), state_message.c_str());
                return false;
            }

            return true;
        }


        bool ffmpeg_save_flv::save_frame() {

// there is no av_packet_from_data in older versions
#if (LIBAVCODEC_VERSION_MAJOR >= 56)
            if (!save_done) {
                //TODO: timeout
               return false;
            }
            save_done = false;

            AVPacket *t_flv_packet = new AVPacket();

            t_flv_packet->data=NULL;
            t_flv_packet->size=0;

            av_packet_from_data(t_flv_packet, frame->encoded_buffer, frame->encoded_buffer_size);

            t_flv_packet->flags=1;
            // set ts;
            t_flv_packet->dts = frame->ts;
            t_flv_packet->pts = frame->ts;

            av_write_frame (flv_format_context, t_flv_packet);

            set_outer_stream_state(VSTR_OST_STATE_RUNNING, "");

            t_flv_packet->data=NULL;
            t_flv_packet->size=0;
            delete[] t_flv_packet;

            save_done = true;
            return true;
#else
            set_outer_stream_state(VSTR_OST_STATE_ERROR, "Streaming functionality is not available on this platform.");
            return false;
#endif
        }

        bool ffmpeg_save_flv::save_dummy_frame(int64_t ts) {
            return false;
        }

        void ffmpeg_save_flv::close() {
            std::unique_lock<std::mutex> lock(this->outer_stream_stop_mutex_);
            stop_init = true;
            this->outer_stream_stop_condition.wait(lock);
            set_outer_stream_state(VSTR_OST_STATE_DISABLED, "");
        }


        void ffmpeg_save_flv::set_outer_stream_state(outer_stream_state_enum state, std::string msg, outer_stream_error_enum error_code) {
            this->state_message = msg;
            this->outer_stream_state = state;
            this->outer_strem_error_code = error_code;
        }
    }
}