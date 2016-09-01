// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_cap.h
*
* FFMPEG capturing class
*/

#ifndef VSTREAMER_FFMPEG_CAP_H_
#define VSTREAMER_FFMPEG_CAP_H_

#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/video.h"
#include "ugcs/vstreamer/base_cap.h"
#include <ugcs/vstreamer/video_device.h>
#include "ugcs/vstreamer/ffmpeg_utils.h"

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


//* default framerate of streamed video (25 fps) /
#define DEFAULT_FRAMERATE_NUM 333  //4
#define DEFAULT_FRAMERATE_DEN 10000  //100
//* default framerate bit tolerance /
#define DEFAULT_FRAMERATE_BIT_TOLERANCE 400000000
// maximum number of errors while trying to get frame
#define MAX_ERROR_NUMBER_GET_FRAME 10



namespace ugcs{
    namespace vstreamer {

        class ffmpeg_save;

        //* @brief FFMPEG capture class */
        class ffmpeg_cap : public base_cap {
        public:

            /**
            * @brief  Constructor
            */
            ffmpeg_cap();

            /**
            * @brief  Destuctor
            */
            ~ffmpeg_cap();

            /** @brief Check device availability for ffmpeg capturing
            *
            * @param vd - video device to check.
            */
            bool check(video_device* video_device_);

            /** @brief Open ffmpeg capture for given device.
            */
            bool open(video_device* video_device_);

            /** @brief Get current frame from given device with ffmpeg.
            *
            * @param video_device_ - video device to grab from.
            * @param encoded_buffer - returned frame data.
            * @param encoded_buffer_size - returned frame size.
            */
            bool get_frame(video_device* video_device_, std::map<int, video_frame*> &frames, int codec_type);

            /** @brief Close capturing process, free device. Free ffmpeg resources
            */
            void close();

            int encode(AVCodecContext *encode_codec_context, AVFrame *encode_frame, AVPacket &encode_packet, std::map<int, video_frame*> &frames, int codec_type);

        private:

            /** input format context (dshow, video4linux or avfoundation) */
            AVFormatContext* format_context;

            //*  input format */
            AVInputFormat *fmt;
            //*  name of input device or stream */
            std::string filenameSrc;
            //* input codec /
            AVCodec *codec;
            //* input codec context /
            AVCodecContext  *codec_context;
            //* output codec (MJPEG) /
            AVCodec *mjpeg_codec;
            AVCodec *flv_codec;
            //* output codec context (MJPEG) */
            AVCodecContext *mjpeg_codec_context;
            AVCodecContext *flv_codec_context;
            //* decoded frame /
            AVFrame *frame;
            //* encoded (mjpeg) frame /
            AVFrame *frame_encoded;
            //* input packet /
            AVPacket packet;
            //* output (mjpeg) packet /
            AVPacket mjpeg_packet;
            AVPacket flv_packet;

            //* buffer for encoded frame /
            uint8_t *buffer;

            //* output pixel format (AV_PIX_FMT_YUV420P or AV_PIX_FMT_YUVJ420P depends on libav versions) /
            AVPixelFormat  pEncodedFormat;

            //* number of video stream inside input data (0 as usual) /
            int videoStream;
            //* result of ffmpeg operations /
            int res;

            void fill_codec_context(AVCodecContext *ctx);

            std::mutex open_cap_mutex;


            int64_t first_frame_dts;
            int64_t prev_dts;
            int64_t start_time;

            bool format_context_initialized;

            bool is_closing;

        };
    }
}

#endif