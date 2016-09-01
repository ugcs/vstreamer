// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_save.h
*
* FFMPEG saving class
*/

#ifndef VSTREAMER_FFMPEG_SAVE_MJPEG_H_
#define VSTREAMER_FFMPEG_SAVE_MJPEG_H_

#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/video.h"
#include "ugcs/vstreamer/base_save.h"
#include <iostream>
#include <fstream>
#include "ugcs/vstreamer/ffmpeg_utils.h"
//#define __STDC_CONSTANT_MACROS
//
//extern "C" {
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libavdevice/avdevice.h>
//#include <libswscale/swscale.h>
//#include <libavutil/imgutils.h>
//}

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

#define DUMMY_FRAME_MAXIMUM_LAG_TIME 100

namespace ugcs{
    namespace vstreamer {

        //* @brief FFMPEG save class */
        class ffmpeg_save_mjpeg : public base_save {
        public:

            /**
            * @brief  Constructor
            */
            ffmpeg_save_mjpeg();

            /**
            * @brief  Destuctor
            */
            ~ffmpeg_save_mjpeg();


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

            void set_outer_stream_state(outer_stream_state_enum state, std::string msg, outer_stream_error_enum error_code = VSTR_OST_ERR_NONE);

            /** @brief Save dummy frame (black screen or something).
            *
            * @param ts - timestamp in milliseconds.
            */
            bool save_dummy_frame(int64_t ts);

            /** @brief Close saving process, free device. Free ffmpeg resources
            */
            void close();

            int64_t get_recording_duration();

        private:

            /** @brief Create synthetic black frame of given height and width. */
            void create_synth_black_frame(AVFrame *frame, int height, int width);

            //* output mjpeg format context */
            AVFormatContext* mjpeg_format_context;

            AVOutputFormat *mjpeg_fmt;

            //* output stream */
            AVStream *mjpeg_stream;

            //* output codec (MJPEG) */
            AVCodec *mjpeg_codec;

            //* output codec context (MJPEG) */
            AVCodecContext *mjpeg_codec_context;

            //* output pixel format (AV_PIX_FMT_YUV420P or AV_PIX_FMT_YUVJ420P depends on libav versions) /
            AVPixelFormat  pEncodedFormat;

            //* metadata file pointer /
            std::fstream metadata_file;

            int64_t first_frame_ts;

            bool init_mjpeg(std::string session_name, int width, int height);

            //* timestamp of real request to start recording /
            int64_t request_ts;

        };
    }
}

#endif