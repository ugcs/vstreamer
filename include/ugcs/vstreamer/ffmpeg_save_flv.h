// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_save.h
*
* FFMPEG saving class
*/

#ifndef VSTREAMER_FFMPEG_SAVE_FLV_H_
#define VSTREAMER_FFMPEG_SAVE_FLV_H_

#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/video.h"
#include "ugcs/vstreamer/base_save.h"


#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

#include <vector>
#include <string>


#ifndef AVPixelFormat
#define AVPixelFormat PixelFormat
#endif

#ifndef AV_PIX_FMT_YUV420P
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#endif

#ifndef AV_PIX_FMT_YUVJ420P
#define AV_PIX_FMT_YUVJ420P PIX_FMT_YUVJ420P
#endif

namespace ugcs{
    namespace vstreamer {

        //* @brief FFMPEG save class */
        class ffmpeg_save_flv : public base_save {
        public:

            /**
            * @brief  Constructor
            */
            ffmpeg_save_flv();

            /**
            * @brief  Destuctor
            */
            ~ffmpeg_save_flv();


            /** @brief Open ffmpeg capture for given device.
            */
            bool init(std::string folder, std::string session_name, int width, int height, int type, int64_t request_ts);


            void run();


            /** @brief Get current frame from given device with ffmpeg.
            *
            * @param encoded_buffer - saved frame data.
            * @param encoded_buffer_size - frame size.
            * @param ts - timestamp in milliseconds.
            */
            bool save_frame();


            bool set_filename(std::string folder, std::string filename, int type);


            /** @brief Save dummy frame (black screen or something).
            *
            * @param ts - timestamp in milliseconds.
            */
            bool save_dummy_frame(int64_t ts);

            /** @brief Close saving process, free device. Free ffmpeg resources
            */
            void close();

            void set_outer_stream_state(outer_stream_state_enum state, std::string msg, outer_stream_error_enum error_code = VSTR_OST_ERR_NONE);

        private:

            //std::string output_filename;
            int type;

            //* output flv format context */
            AVFormatContext* flv_format_context;

            AVOutputFormat *flv_fmt;

            //* output stream */
            AVStream *flv_stream;

            //* output codec (FLV) */
            AVCodec *flv_codec;

            //* output codec context (FLV) */
            AVCodecContext *flv_codec_context;

            //* output (flv) packet */
            AVPacket *flv_packet;

            //* output pixel format (AV_PIX_FMT_YUV420P or AV_PIX_FMT_YUVJ420P depends on libav versions) */
            AVPixelFormat  pEncodedFormat;

            bool init_flv(std::string session_name, int width, int height);

            bool save_done;

            bool stop_init;

            /** condition for video stream */
            std::condition_variable outer_stream_stop_condition;
            /** video stream mutex */
            std::mutex outer_stream_stop_mutex_;

        };
    }
}

#endif