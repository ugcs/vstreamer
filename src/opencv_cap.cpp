// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file opencv_cap.cpp
*/


#ifdef OPENCV_CAP

#include "ugcs/vstreamer/opencv_cap.h"

namespace ugcs {

    namespace vstreamer {

        opencv_cap::opencv_cap() {
            fcap = new ffmpeg_cap();
        }

        opencv_cap::~opencv_cap() {

        }

        bool opencv_cap::check(video_device* video_device_) {
            LOG_DEBUG("OPENCV.check: Checking device %s", video_device_->name.c_str());
            bool res = open(video_device_);
            close();
            return res;
        }


        bool opencv_cap::open(video_device* video_device_) {

            if (video_device_->type==DEV_STREAM) {
                cap.open(video_device_->name);
            }
            else {
            LOG_DEBUG("Device with index %d tryed to open",video_device_->index );
                cap.open(video_device_->index);
            }

            LOG_DEBUG("Result: %d", cap.isOpened() );

            // grab one frame to get frame size;
            if (cap.isOpened()) {

                if (cap.grab()) {
                    cv::Mat frame;
                    cap >> frame;
                    if (frame.data) {
                        video_device_->height = frame.size().height;
                        video_device_->width = frame.size().width;
                    }
                }

                // FFMPEG STUFF: register modules and init flv context for broadcast encoding
                // register all ffmpeg modules
                av_register_all();
                avdevice_register_all();
                avcodec_register_all();
                avformat_network_init();

                pEncodedFormat = AV_PIX_FMT_YUV420P;
                flv_codec = avcodec_find_encoder(AV_CODEC_ID_FLV1);
                if (!flv_codec) {
                    LOG_ERR("Video Device (%s): FLV Codec not found!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                // allocate flv codec context (for output)
                flv_codec_context = avcodec_alloc_context3(flv_codec);
                if (!flv_codec_context) {
                    LOG_ERR("Video Device (%s): Could not allocate video FLV codec context!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                frame_encoded = av_frame_alloc();
                int numBytes;
                numBytes = avpicture_get_size(pEncodedFormat, video_device_->width, video_device_->height);
                buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
                avpicture_fill((AVPicture *) frame_encoded, buffer, pEncodedFormat, video_device_->width, video_device_->height);
            }

            return cap.isOpened();
        }


        bool opencv_cap::get_frame(video_device* video_device_, std::map<int, video_frame*> &frames, int codec_type) {

            int err_count = 0;
            cv::Mat frame;

            while (err_count < OPENCV_CAP_MAX_ERROR_COUNT) {
                if (cap.isOpened()) {
                    if (cap.grab()) {
                        cap >> frame;

                        if (frame.data) {

                            //* MJPEG Stuff */
                            if (codec_type & VSTR_CODEC_MJPEG) {

                                std::vector<int> encode_params;
                                std::vector<uchar> encoded_vector;

                                encode_params.push_back(CV_IMWRITE_JPEG_QUALITY);
                                encode_params.push_back(95);

                                video_frame* vf;
                                if (frames.count(VSTR_CODEC_MJPEG) > 0) {
                                    vf = frames.at(VSTR_CODEC_MJPEG);
                                } else
                                {
                                    vf = new video_frame();
                                    frames[VSTR_CODEC_MJPEG] = vf;
                                }
                                vf->ts = utils::getMilliseconds();

                                cv::imencode(".jpeg", frame, encoded_vector, encode_params);

                                vf->encoded_buffer_size = encoded_vector.size();
                                vf->encoded_buffer = (unsigned char*)realloc(vf->encoded_buffer, (size_t)vf->encoded_buffer_size);
                                memcpy(vf->encoded_buffer, &encoded_vector[0], (size_t)vf->encoded_buffer_size);

                            }

                            //* FLV Stuff */
                            if (codec_type & VSTR_CODEC_FLV) {
                                // convert opencv frame to YUV420
                                cv::Mat yuv_frame;
                                cv::cvtColor(frame , yuv_frame, CV_BGR2YUV_I420 );
                                // fill ffmpeg frame
                                avpicture_fill((AVPicture*)frame_encoded, yuv_frame.data, pEncodedFormat, video_device_->width, video_device_->height);
                                // init context
                                flv_codec_context->pix_fmt = pEncodedFormat;
                                flv_codec_context->color_range = AVCOL_RANGE_JPEG;
                                flv_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
                                flv_codec_context->width = video_device_->width;
                                flv_codec_context->height = video_device_->height;
                                // set top quality
                                flv_codec_context->qmin=2;
                                flv_codec_context->qmax=31;
                                // set fps and bitrate tolerance
                                flv_codec_context->time_base.num = DEFAULT_FRAMERATE_NUM;
                                flv_codec_context->time_base.den = DEFAULT_FRAMERATE_DEN;
                                flv_codec_context->bit_rate_tolerance = DEFAULT_FRAMERATE_BIT_TOLERANCE;

                                int res = avcodec_open2(flv_codec_context, flv_codec, NULL);
                                // open codec for encoding
                                if (res < 0) {
                                    LOG_ERR("Video Device (%s): Could not open FLV codec. Error code (%d)\n", video_device_->name.c_str(), res);
                                    VS_WAIT(100);
                                    err_count++;
                                    continue;
                                }
                                // convert to flv
                                av_init_packet(&flv_packet);
                                flv_packet.data = NULL;
                                flv_packet.size = 0;

                                // do the encoding
                                int ret = fcap->encode(flv_codec_context, frame_encoded, flv_packet, frames, VSTR_CODEC_FLV);
                                av_free_packet(&flv_packet);
                                if (ret < 0) {
                                    LOG_ERR("Video Device (%s): Error FLV-encoding frame. Error code (%d)\n", video_device_->name.c_str(), res);
                                    err_count++;
                                    continue;
                                }
                            }

                            return true;

                        } else {
                            err_count++;
                            continue;
                        }
                    }
                }
                else {
                    open(video_device_);
                    err_count++;
                }
            }
            return false;
        }




        void opencv_cap::close() {
            cap.release();
        }

    }
}

#endif
