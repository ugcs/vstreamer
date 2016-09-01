// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_save.cpp
*/


#include <ugcs/vstreamer/common.h>
#include "ugcs/vstreamer/ffmpeg_save_mjpeg.h"

namespace ugcs {

    namespace vstreamer {


        ffmpeg_save_mjpeg::ffmpeg_save_mjpeg() {
            frame = new video_frame();
            this->request_ts = -1;
            this->is_initialized = false;
            this->is_running = false;
            this->type = -1;
            this->mjpeg_format_context = NULL;
            this->mjpeg_fmt = NULL;
            this->mjpeg_stream = NULL;
            this->mjpeg_codec = NULL;
            this->mjpeg_codec_context = NULL;
            this->first_frame_ts = 0;

            // on avlibcodec 54 and 53 (linux) we cannot create MJPEG encoder for pix_fmt=AV_PIX_FMT_YUV420P, so
            // we need to use AV_PIX_FMT_YUVJ420P. But in versions 55+ this format is deprecated. So on, in version
            // 56.1.0 (Ubuntu 14.10) we need use AV_PIX_FMT_YUVJ420P again.
#if ((LIBAVCODEC_VERSION_INT >= ((55<<16)+(0<<8)+0)) && (LIBAVCODEC_VERSION_INT < ((56<<16)+(1<<8)+0)))
            pEncodedFormat = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_YUVJ420P;
#else
            pEncodedFormat = AV_PIX_FMT_YUVJ420P; //AV_PIX_FMT_YUVJ420P;
#endif

        }

        ffmpeg_save_mjpeg::~ffmpeg_save_mjpeg() {
            this->close();
        }


        bool ffmpeg_save_mjpeg::set_filename(std::string folder, std::string filename, int type) {
            if (type == VSTR_SAVE_FILE) {
                this->output_filename = utils::createFullFilename(folder, filename, VSTR_RECORDING_VIDEO_EXTENSION);
            } else if (type == VSTR_SAVE_USTREAM) {
                //VSTR_SAVE_USTREAM and others are not implemented
                return false;
            }
            return true;
        }


        bool ffmpeg_save_mjpeg::init(std::string folder, std::string session_name, int width, int height, int type, int64_t request_ts) {

            this->type = type;
            this->first_frame_ts = -1;
            this->request_ts = request_ts;

            // register all ffmpeg modules
            av_register_all();
            avdevice_register_all();
            avcodec_register_all();
            avformat_network_init();

            bool res;
            res = this->set_filename(folder, session_name, type);
            if (!res) {
                return false;
            }

            // init mjpeg
            LOG_DEBUG("MJPEG Initiation for saving process (%s)", this->output_filename.c_str());
            res = init_mjpeg(session_name, width, height);
            if (!res) {
                return false;
            }
            LOG_DEBUG("MJPEG init done! Starting saving process (%s)", this->output_filename.c_str());

            std::thread t(&ffmpeg_save_mjpeg::run, this);
            t.detach();

            LOG_DEBUG("Saving process is started (%s)", this->output_filename.c_str());

            this->is_initialized = true;

            return true;
        }


        void ffmpeg_save_mjpeg::run() {
            std::string metadata_filename = this->output_filename + "." + VSTR_RECORDING_VIDEO_METADATA_EXTENSION;
            if (this->metadata_file.is_open()) {
                this->metadata_file.close();
            }

            this->metadata_file.open(metadata_filename, std::ios::out);

            this->is_running = true;
            while (this->is_running) {
                std::unique_lock<std::mutex> lock(this->frame->video_mutex_);
                // notification will come from video_device when new frame appears.
                this->frame->video_condition_.wait(lock);
                if (this->is_running) {
                    this->save_frame();
                }
            }
            this->metadata_file.close();
            VS_WAIT(500);
            // notify everybody about finishing run-loop
            this->stopping_condition_.notify_all();

        }


        bool ffmpeg_save_mjpeg::init_mjpeg(std::string session_name, int width, int height) {

            // find the mjpeg video encoder (for output or encoding)
#if (LIBAVCODEC_VERSION_MAJOR < 54)
            mjpeg_codec = avcodec_find_encoder(CODEC_ID_MJPEG);
#else
            mjpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
#endif
            if (!mjpeg_codec) {
                LOG_ERR("Save Session (%s): MJPEG Codec not found!\n", session_name.c_str());
                return false;
            }
            // allocate mjpeg codec context (for output or encoding)
            mjpeg_codec_context = avcodec_alloc_context3(mjpeg_codec);
            if (!mjpeg_codec_context) {
                LOG_ERR("Save Session (%s): Could not allocate video MJPEG codec context!\n", session_name.c_str());
                return false;
            }

            mjpeg_format_context = NULL;
            if (this->type == VSTR_SAVE_FILE) {
                //avformat_alloc_output_context2(&mjpeg_format_context, NULL, NULL, output_filename.c_str());
                mjpeg_format_context = avformat_alloc_context();
                //avformat_alloc_output_context2(&flv_format_context, NULL, NULL, output_filename.c_str());
                mjpeg_fmt = av_guess_format("mpg", output_filename.c_str(), NULL);
                mjpeg_format_context->oformat = mjpeg_fmt;
            } else {
                // not implemented
                return false;
            }

            mjpeg_stream = avformat_new_stream(mjpeg_format_context, mjpeg_codec);
            mjpeg_stream->codec = mjpeg_codec_context;
            mjpeg_codec_context->pix_fmt = pEncodedFormat;

            // on versions 55+ (win and mac) we use pEncodedFormat = AV_PIX_FMT_YUV420P and need
            // to set color_range and FF_COMPLIANCE_EXPERIMENTAL
            // TODO!!! CHANGE
#if (LIBAVCODEC_VERSION_INT >= ((55<<16)+(0<<8)+0))
            mjpeg_codec_context->color_range = AVCOL_RANGE_JPEG;
            // ffmpeg way: to define PIX_FMT_YUVJ420P as deprecated, but
            // allow using of PIX_FMT_YUV420P + AVCOL_RANGE_JPEG only with
            // experimental flag. Well, ok.
            mjpeg_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
#endif

            mjpeg_codec_context->width = width;
            mjpeg_codec_context->height = height;
            mjpeg_codec_context->qmin=1;
            mjpeg_codec_context->qmax=1;
            mjpeg_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
// 14.04+ ,win, mac
#if (LIBAVCODEC_VERSION_MAJOR > 54)
            mjpeg_codec_context->time_base = (AVRational){1,1000};
            mjpeg_codec_context->codec_id = AV_CODEC_ID_MJPEG;
            mjpeg_codec_context->codec_tag = av_codec_get_tag(mjpeg_format_context->oformat->codec_tag, AV_CODEC_ID_MJPEG);
#endif

            // if we save to file add headers and flags
            if (this->type == VSTR_SAVE_FILE) {
                av_dump_format(mjpeg_format_context, 0, output_filename.c_str(), 1);
                int ret = avio_open(&mjpeg_format_context->pb, output_filename.c_str(), AVIO_FLAG_WRITE );
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", output_filename.c_str());
                    return false;
                }

                av_dict_set(&mjpeg_format_context->metadata, "test", "123456", 0);

                ret = avformat_write_header(mjpeg_format_context, NULL);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
                    return false;
                }
            }
            else {
                // not implemented
                return false;
            }
            return true;
        }


        bool ffmpeg_save_mjpeg::save_frame() {

#if (LIBAVCODEC_VERSION_MAJOR >= 54)

            AVPacket *t_mjpg_packet = new AVPacket();
            t_mjpg_packet->data=NULL;

            ffmpeg_utils::packet_from_data(t_mjpg_packet, frame->encoded_buffer, frame->encoded_buffer_size);

            if (first_frame_ts < 0) {
                // this is the first frame. Let's decide what to do. If this frame is close enough to
                // real request time, lets save it twice. First time with request ts and second time
                // with current ts. Else if frame is far from request time - let's create dummy first frame
                // with request ts.
                if (frame->ts - request_ts < DUMMY_FRAME_MAXIMUM_LAG_TIME) {
                    t_mjpg_packet->dts = 0;
                    t_mjpg_packet->pts = 0;
                    t_mjpg_packet->flags = 1;
                    av_write_frame (mjpeg_format_context, t_mjpg_packet);
                } else {
                    this->save_dummy_frame(request_ts);
                }
                first_frame_ts = request_ts;
            }

            t_mjpg_packet->dts = frame->ts - first_frame_ts;
            t_mjpg_packet->pts = frame->ts - first_frame_ts;
            t_mjpg_packet->flags = 1;

            av_write_frame (mjpeg_format_context, t_mjpg_packet);
            t_mjpg_packet->size=0;
            t_mjpg_packet->data=NULL;
            delete t_mjpg_packet;

             // save duration
            metadata_file.seekg(0, std::ios::beg);
            metadata_file << this->get_recording_duration();

            return true;
#else
            return false;
#endif
        }


        bool ffmpeg_save_mjpeg::save_dummy_frame(int64_t ts) {

// win & mac only
#if (LIBAVCODEC_VERSION_MAJOR >= 54)
            LOG_DEBUG("Creating black frame (%d x %d)",  mjpeg_codec_context->width, mjpeg_codec_context->height);
            AVFrame *black_frame;
            black_frame = ffmpeg_utils::frame_alloc();

            int height = mjpeg_codec_context->height;
            int width = mjpeg_codec_context->width;

            black_frame->height = height;
            black_frame->width = width;
            black_frame->format = mjpeg_codec_context->pix_fmt;
            black_frame->pts = 0;

            int res = av_image_alloc(black_frame->data, black_frame->linesize, width, height, pEncodedFormat, 16);

            if ( res < 0) {
                LOG_ERROR("Error while creating black frame (allocaton image), %d", res);
                return false;
            }

            this->create_synth_black_frame(black_frame, height, width);

            // open codec for encoding black frame
            mjpeg_codec_context->time_base.num = DEFAULT_FRAMERATE_NUM;
            mjpeg_codec_context->time_base.den = DEFAULT_FRAMERATE_DEN;
            mjpeg_codec_context->bit_rate_tolerance = DEFAULT_FRAMERATE_BIT_TOLERANCE;
            res = avcodec_open2(mjpeg_codec_context, mjpeg_codec, NULL);
            if (res < 0) {
                LOG_ERROR("Error while creating black frame (opening codec ), %d", res);
                return false;
            }

            AVPacket t_mjpg_packet;
            av_init_packet(&t_mjpg_packet);
            t_mjpg_packet.data = NULL;
            t_mjpg_packet.size = 0;
            int got_output;
            res = avcodec_encode_video2(mjpeg_codec_context, &t_mjpg_packet, black_frame, &got_output);
            if (res < 0) {
                LOG_ERROR("Error while creating black frame (encoding video), %d", res);
                return false;
            }

            t_mjpg_packet.dts = 0;
            t_mjpg_packet.pts = 0;
            t_mjpg_packet.flags = 1;
            av_write_frame (mjpeg_format_context, &t_mjpg_packet);
            av_free_packet(&t_mjpg_packet);
            av_free(black_frame);
            LOG_DEBUG("Black frame was successfully created");
            return true;
#else
            return false;
#endif

        }


        void ffmpeg_save_mjpeg::create_synth_black_frame(AVFrame *frame, int height, int width) {
            /* Y */

            int x,y;
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    frame->data[0][y * frame->linesize[0] + x] = 0;

            /* Cb and Cr */
            for (y = 0; y < height / 2; y++) {
                for (x = 0; x < width / 2; x++) {
                    frame->data[1][y * frame->linesize[1] + x] = 0;
                    frame->data[2][y * frame->linesize[2] + x] = 0;
                }
            }
        }


        void ffmpeg_save_mjpeg::close(){
            LOG_INFO("Stopping video recording process (%s)", this->output_filename.c_str());
            if (this->is_running) {
                //stop waiting in run-loop without saving frame

                this->is_running = false;
                this->frame->video_condition_.notify_all();

                LOG_INFO("Stopping video recording process, close files (%s)", this->output_filename.c_str());
                // wait until run-loop stop
                std::unique_lock<std::mutex> lock(this->stopping_mutex_);
                // notification will come from video_device when run-loop stops
                this->stopping_condition_.wait(lock);
            } else {
                LOG_INFO("Stopping video recording process, recording is not active (%s)", this->output_filename.c_str());
            }

            if (this->is_initialized) {
                LOG_INFO("Stopping video recording process, free codec resources (%s)", this->output_filename.c_str());
                // free ffmpeg resources
                av_write_trailer(mjpeg_format_context);

                avcodec_close(mjpeg_codec_context);
                avformat_close_input(&mjpeg_format_context);
                this->is_initialized = false;

            } else {
                LOG_INFO("Video recording process (%s) has not been initialized", this->output_filename.c_str());
            }
            LOG_INFO("Video recording process (%s) is succesfully stopped", this->output_filename.c_str());
        }


        int64_t ffmpeg_save_mjpeg::get_recording_duration() {
            return this->frame->ts - this->first_frame_ts;
        }


        void ffmpeg_save_mjpeg::set_outer_stream_state(outer_stream_state_enum state, std::string msg, outer_stream_error_enum error_code) {
           // not impl
        }



    }
}
