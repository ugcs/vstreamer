// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file opencv_cap.h
*
* OpenCV capturing realisation
*/


#ifndef VSTREAMER_OPENCV_CAP_H_
#define VSTREAMER_OPENCV_CAP_H_

#ifdef OPENCV_CAP

#include "ugcs/vstreamer/common.h"
#include "ugcs/vstreamer/video.h"
#include "ugcs/vstreamer/base_cap.h"
#include <ugcs/vstreamer/video_device.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "ugcs/vstreamer/ffmpeg_cap.h"


#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

#define OPENCV_CAP_MAX_ERROR_COUNT 5

namespace ugcs{
    namespace vstreamer {

        class ffmpeg_cap;

class video_device;




	    /**
		 * @class opencv_cap
		 * @brief OpenCV capturing realisation
		 */
        class opencv_cap : public base_cap {
        public:

            /**
            * @brief Constructor
            */
            opencv_cap();

            /**
            * @brief Destructor
            */
            ~opencv_cap();

            /** @brief Check device availability for capturing with OpenCV
            *
            * @param video_device_ - video device to check.
            */
            bool check(video_device* video_device_); // тут нужна ссылка на конкретную реализацию капы

            /** @brief Open OpenCV capture for given device.
            */
            bool open(video_device* video_device_);

            /** @brief Get current frame from given device.
            *
            * @param video_device_ - video device to grab from.
            * @param encoded_buffer - returned frame data.
            * @param encoded_buffer_size - returned frame size.
            */
           // bool get_frame(video_device* video_device_,  unsigned char **encoded_buffer, int& encoded_buffer_size);

            bool get_frame(video_device* video_device_, std::map<int, video_frame*> &frames, int codec_type);

            /** @brief Close OpenCV capturing process, free device.
            */
            void close();

        private:
            /** OpenCV capturer */
            cv::VideoCapture cap;



            ffmpeg_cap* fcap;
            /** input format context (dshow, video4linux or avfoundation) */
            AVCodec *flv_codec;
            //* output codec context (MJPEG) */
            AVCodecContext *flv_codec_context;
            //* decoded frame /
            //AVFrame *frame;
            //* encoded (mjpeg) frame /
            AVFrame *frame_encoded;
            //* input packet /
            //AVPacket packet;
            //* output (mjpeg) packet /
            AVPacket flv_packet;

            //* buffer for encoded frame /
            uint8_t *buffer;

            //* output pixel format (AV_PIX_FMT_YUV420P or AV_PIX_FMT_YUVJ420P depends on libav versions) /
            AVPixelFormat  pEncodedFormat;


        };
    }
}


#endif

#endif