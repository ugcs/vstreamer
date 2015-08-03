// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_cap.cpp
*/

#include "ugcs/vstreamer/ffmpeg_cap.h"


namespace ugcs {

    namespace vstreamer {


        ffmpeg_cap::ffmpeg_cap() {
            prev_dts = -1;
            first_frame_dts = -1;
            start_time = 0;

        }


        ffmpeg_cap::~ffmpeg_cap() {

        }


        bool ffmpeg_cap::check(video_device* video_device_) {
            LOG_DEBUG("FFMPEG.check: Checking device %s", video_device_->name.c_str());
            av_register_all();
            avdevice_register_all();
            avcodec_register_all();
            avformat_network_init();

            AVFormatContext* format_context = avformat_alloc_context();
            AVInputFormat *fmt = NULL;

            std::string filenameSrc;
            if (video_device_->type == DEV_CAMERA) {
                filenameSrc = DEVICE_FFMPEG_NAME_PREFIX + video_device_->name;
                fmt = av_find_input_format(DEVICES_INPUT_FORMAT);
            }
            else if (video_device_->type == DEV_STREAM || video_device_->type == DEV_FILE) {
                filenameSrc = video_device_->url;
            }
            else {
                LOG_ERROR("FFMPEG.check: Unknown device type");
                return false;
            }

            int err = avformat_open_input(&format_context, filenameSrc.c_str(), fmt, NULL);
            if (err != 0) {
                // cannot open device
                LOG_DEBUG("FFMPEG.check: Cannot open device %s. Error code: %d", filenameSrc.c_str(), err);
                return false;
            }
            avformat_close_input(&format_context);

            LOG("FFMPEG.check: Device %s is found", video_device_->name.c_str());

            return true;
        }


        bool ffmpeg_cap::open(video_device* video_device_) {

            if (open_cap_mutex.try_lock()) {
                open_cap_mutex.unlock();
            } else {
                VS_WAIT(100);
                return false;
            }
            std::lock_guard<std::mutex> lock(open_cap_mutex);

            // register all ffmpeg modules
            av_register_all();
            avdevice_register_all();
            avcodec_register_all();
            avformat_network_init();

            format_context = avformat_alloc_context();
            fmt = NULL;

// on avlibcodec 54 and 53 (linux) we cannot create MJPEG encoder for pix_fmt=AV_PIX_FMT_YUV420P, so
// we need to use AV_PIX_FMT_YUVJ420P. But in versions 55+ this format is deprecated. So on, in version
// 56.1.0 (Ubuntu 14.10) we need use AV_PIX_FMT_YUVJ420P again.
#if ((LIBAVCODEC_VERSION_INT >= ((55<<16)+(0<<8)+0)) && (LIBAVCODEC_VERSION_INT < ((56<<16)+(1<<8)+0)))
            pEncodedFormat = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_YUVJ420P;
#else
            pEncodedFormat = AV_PIX_FMT_YUVJ420P; //AV_PIX_FMT_YUVJ420P;
#endif

            //video_device_->video_cap_opened = false;

            // init capture sequence.
            {
                VS_WAIT(1000);

                // different "input names" for devices and streams
                // for example for dshow name of camera is "video=<camera name>"
                if (video_device_->type == DEV_CAMERA) {
                    filenameSrc = DEVICE_FFMPEG_NAME_PREFIX + video_device_->name;
                    fmt = av_find_input_format(DEVICES_INPUT_FORMAT);
                }
                else if (video_device_->type == DEV_STREAM || video_device_->type == DEV_FILE) {
                    filenameSrc = video_device_->url;
                }
                        else {
                    LOG_ERROR("FFMPEG.check: Unknown device type");
                    return false;
                }

                // find the mjpeg video encoder (for output)
#if (LIBAVCODEC_VERSION_MAJOR < 54)
                mjpeg_codec = avcodec_find_encoder(CODEC_ID_MJPEG);
#else
                mjpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
#endif
                if (!mjpeg_codec) {
                    LOG_ERR("Video Device (%s): MJPEG Codec not found!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                // allocate mjpeg codec context (for output)
                mjpeg_codec_context = avcodec_alloc_context3(mjpeg_codec);
                if (!mjpeg_codec_context) {
                    LOG_ERR("Video Device (%s): Could not allocate video MJPEG codec context!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                // find the flv video encoder (for output)
#if (LIBAVCODEC_VERSION_MAJOR < 54)
                flv_codec = avcodec_find_encoder(CODEC_ID_FLV1);
#else
                flv_codec = avcodec_find_encoder(AV_CODEC_ID_FLV1);
#endif
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

                // try to open input
                int err = avformat_open_input(&format_context, filenameSrc.c_str(), fmt, NULL);
                if (err != 0) {
                    LOG_ERR("Video Device (%s): Unable to open input device or stream!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                // get info about input
                avformat_find_stream_info(format_context, 0);


                // search for video stream
                videoStream = -1;
                for (int i = 0; i < format_context->nb_streams; i++) {

                    av_dump_format(format_context, i, filenameSrc.c_str(), 0);
                    if (format_context->streams[i]->codec->coder_type == AVMEDIA_TYPE_VIDEO) {
                        videoStream = i;
                        break;
                    }
                }

                // no video stream - keep trying
                if (videoStream == -1) {
                    LOG_ERR("Video Device (%s): No videostream was found in input device or stream!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }


                codec_context = format_context->streams[videoStream]->codec;

                // set codec context width and height from video device data
                if (video_device_->width > 0 && video_device_->height > 0) {
                    codec_context->width = video_device_->width;
                    codec_context->height = video_device_->height;
                }
                // or else set device picture size from codec context
                else if (codec_context->width > 0 && codec_context->height > 0) {
                    video_device_->width = codec_context->width;
                    video_device_->height = codec_context->height;

                }

                if (codec_context->width == 0 || codec_context->height == 0 ) {
                    LOG_ERR("Video Device (%s): Cannot obtain frame size. \n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                codec = avcodec_find_decoder(codec_context->codec_id);
                // codec not found
                if (codec == NULL) {
                    LOG_ERR("Video Device (%s): No codec was found for input device or stream!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                if (avcodec_open2(codec_context, codec, NULL) < 0) {
                    LOG_ERR("Video Device (%s):  Cannot open codec for input device or stream!\n", video_device_->name.c_str());
                    video_device_->video_cap_opened = false;
                    return false;
                }

                frame = ffmpeg_utils::frame_alloc();
                frame_encoded = ffmpeg_utils::frame_alloc();

                // init encoded frame
                int numBytes;
                numBytes = avpicture_get_size(pEncodedFormat, codec_context->width, codec_context->height);
                buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
                avpicture_fill((AVPicture *) frame_encoded, buffer, pEncodedFormat, codec_context->width, codec_context->height);

                av_init_packet(&packet);

                // Ok then. Init is done.
                video_device_->video_cap_opened = true;
            }
            return true;
        }


        void ffmpeg_cap::fill_codec_context(AVCodecContext *ctx) {
            ctx->pix_fmt = pEncodedFormat;

            // on versions 55+ (win and mac) we use pEncodedFormat = AV_PIX_FMT_YUV420P and need
            // to set color_range and FF_COMPLIANCE_EXPERIMENTAL
#if (LIBAVCODEC_VERSION_INT >= ((55<<16)+(0<<8)+0))
            ctx->color_range = AVCOL_RANGE_JPEG;
            // ffmpeg way: to define PIX_FMT_YUVJ420P as deprecated, but
            // allow using of PIX_FMT_YUV420P + AVCOL_RANGE_JPEG only with
            // experimental flag. Well, ok.
            ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
#endif

            ctx->width = codec_context->width;
            ctx->height = codec_context->height;

            // set top quality
            ctx->qmin=1;
            ctx->qmax=1;

            // set fps and bitrate tolerance
            if (ctx->time_base.num == 0) {

                ctx->time_base.num = format_context->streams[videoStream]->avg_frame_rate.num;
                ctx->time_base.den = format_context->streams[videoStream]->avg_frame_rate.den;
                ctx->bit_rate_tolerance = codec_context->bit_rate_tolerance * ctx->time_base.num;
            }
            else {
                ctx->time_base.num = codec_context->time_base.num;
                ctx->time_base.den = codec_context->time_base.den;
            }

            // if no framerate is finally set, set 25 fps
            // need for avfoundation (Mac OS) mostly
            if (ctx->time_base.num == 0 || ctx->time_base.den==0) {
                ctx->time_base.num = DEFAULT_FRAMERATE_NUM;
                ctx->time_base.den = DEFAULT_FRAMERATE_DEN;
            }

            // if no bit_rate_tolerance is finally set, set to default
            // need for avfoundation (Mac OS) mostly
            if (ctx->bit_rate_tolerance == 0) {
                ctx->bit_rate_tolerance = DEFAULT_FRAMERATE_BIT_TOLERANCE;
            }
        }


        int ffmpeg_cap::encode(AVCodecContext *encode_codec_context, AVFrame *encode_frame, AVPacket &encode_packet, std::map<int, video_frame*> &frames, int codec_type) {

            video_frame* vf;

            if (frames.count(codec_type) > 0) {
                vf = frames.at(codec_type);
            } else
            {
                vf = new video_frame();
                frames[codec_type] = vf;
            }
            vf->ts = utils::getMilliseconds();

            // do the encoding
            // (slightly different for different ffmpeg versions)

#if (LIBAVCODEC_VERSION_INT > ((53<<16)+(99<<8)+0))
            int got_output;
            int ret = avcodec_encode_video2(encode_codec_context, &encode_packet, encode_frame, &got_output);
            if (ret < 0) {
                return ret;
            }

            vf->encoded_buffer_size = encode_packet.size;
            vf->encoded_buffer = (unsigned char*)realloc(vf->encoded_buffer, (size_t)vf->encoded_buffer_size);
            memcpy(vf->encoded_buffer, encode_packet.data, (size_t)vf->encoded_buffer_size);

#else
            uint8_t *outbuf;
            uint32_t outbuf_size;
            outbuf_size = codec_context->width * codec_context->height * 4;
            outbuf = (uint8_t *)av_malloc(outbuf_size);
            vf->encoded_buffer_size = avcodec_encode_video(encode_codec_context, outbuf, outbuf_size, encode_frame);
            vf->encoded_buffer = (unsigned char*)realloc(vf->encoded_buffer, vf->encoded_buffer_size);
            memcpy(vf->encoded_buffer, outbuf, vf->encoded_buffer_size);
            av_free(outbuf);
#endif

            return 0;
        }


        bool ffmpeg_cap::get_frame(video_device* video_device_, std::map<int, video_frame*> &frames, int codec_type) {

                bool is_ok = true;
                int err_count = 0;

                // capturing loop
                while (is_ok) {

                    if (err_count > MAX_ERROR_NUMBER_GET_FRAME) {
                        LOG_ERR("Video Device (%s): Errors count exceed maximum. Closing cap. \n", video_device_->name.c_str());
                        is_ok = false;
                        continue;
                    }
                    if (!video_device_->video_cap_opened) {
                        if (!open(video_device_))
                        {
                            err_count ++;
                            continue;
                        }
                    }
                    if (video_device_->type == DEV_FILE) {
                        if (first_frame_dts < 0) {
                            res = av_read_frame(format_context, &packet);
                            if (res < 0) { return false; }
                            first_frame_dts = packet.dts;
                            start_time = utils::getMicroseconds();
                            av_free_packet(&packet);
                        }


                        int64_t ts_need = static_cast<int64_t>(((utils::getMicroseconds() - start_time) / 1000) * video_device_->playback_speed + first_frame_dts + video_device_->playback_starting_pos);
                        res = av_seek_frame(format_context, videoStream, ts_need, AVSEEK_FLAG_ANY);
                        res = av_read_frame(format_context, &packet);

                        if (res>0 && packet.convergence_duration == prev_dts) {
                            // it's same frame as on previous step, wait for new
                            av_free_packet(&packet);
                            VS_WAIT(10);
                            continue;
                        }
                        VS_WAIT(10);
                    } else if (video_device_->type == DEV_STREAM || video_device_->type == DEV_CAMERA) {
                        res = av_read_frame(format_context, &packet);
                    }

                    if (res >= 0) {
                        if (packet.stream_index == videoStream) {

                            if (video_device_->type == DEV_FILE) {
                                // for file playback simply copy packet to buffer without encoding
                                video_frame *vf;
                                prev_dts = packet.convergence_duration;
                                if (frames.count(VSTR_CODEC_MJPEG) > 0) {
                                    vf = frames.at(VSTR_CODEC_MJPEG);
                                } else {
                                    vf = new video_frame();
                                    frames[VSTR_CODEC_MJPEG] = vf;
                                }
                                vf->ts = utils::getMilliseconds();


                                vf->encoded_buffer_size = packet.size;
                                vf->encoded_buffer = (unsigned char *) realloc(vf->encoded_buffer, (size_t)vf->encoded_buffer_size);
                                memcpy(vf->encoded_buffer, packet.data, (size_t)vf->encoded_buffer_size);
                                av_free_packet(&packet);
                                return true;
                            }

                            // for stream or camera do encoding
                            if (video_device_->type == DEV_STREAM || video_device_->type == DEV_CAMERA) {
                                // decode packet into frame
                                avcodec_decode_video2(codec_context, frame, &frameFinished, &packet);

                                if (frameFinished) {

                                    // convert input frame into output frame
                                    struct SwsContext *img_convert_ctx;
                                    img_convert_ctx = sws_getCachedContext(NULL, codec_context->width,
                                                                           codec_context->height,
                                                                           codec_context->pix_fmt, codec_context->width,
                                                                           codec_context->height, pEncodedFormat, 1,
                                                                           NULL, NULL, NULL);
                                    sws_scale(img_convert_ctx, ((AVPicture *) frame)->data,
                                              ((AVPicture *) frame)->linesize, 0, codec_context->height,
                                              ((AVPicture *) frame_encoded)->data,
                                              ((AVPicture *) frame_encoded)->linesize);


                                    //* MJPEG Stuff */
                                    if (codec_type & VSTR_CODEC_MJPEG) {
                                        fill_codec_context(mjpeg_codec_context);
                                        res = avcodec_open2(mjpeg_codec_context, mjpeg_codec, NULL);
                                        // open codec for encoding
                                        if (res < 0) {
                                            LOG_ERR("Video Device (%s): Could not open MJPEG codec. Error code (%d)\n",
                                                    video_device_->name.c_str(), res);
                                            sws_freeContext(img_convert_ctx);
                                            VS_WAIT(100);
                                            err_count++;
                                            continue;
                                        }
                                        // convert to mjpeg
                                        av_init_packet(&mjpeg_packet);
                                        mjpeg_packet.data = NULL;
                                        mjpeg_packet.size = 0;
                                        // do the encoding
                                        int ret = encode(mjpeg_codec_context, frame_encoded, mjpeg_packet,
                                                         frames, VSTR_CODEC_MJPEG);


                                        av_free_packet(&mjpeg_packet);

                                        if (ret < 0) {
                                            LOG_ERR("Video Device (%s): Error MJPEG-encoding frame. Error code (%d)\n",
                                                    video_device_->name.c_str(), res);
                                            sws_freeContext(img_convert_ctx);
                                            err_count++;
                                            continue;
                                        }
                                    }
                                    // FLV STUFF
                                    if (codec_type & VSTR_CODEC_FLV) {
                                        fill_codec_context(flv_codec_context);
                                        flv_codec_context->qmin = 2;
                                        flv_codec_context->qmax = 31;
                                        res = avcodec_open2(flv_codec_context, flv_codec, NULL);
                                        // open codec for encoding
                                        if (res < 0) {
                                            LOG_ERR("Video Device (%s): Could not open FLV codec. Error code (%d)\n",
                                                    video_device_->name.c_str(), res);
                                            sws_freeContext(img_convert_ctx);
                                            VS_WAIT(100);
                                            err_count++;
                                            continue;
                                        }
                                        // convert to flv
                                        av_init_packet(&flv_packet);
                                        flv_packet.data = NULL;
                                        flv_packet.size = 0;

                                        // do the encoding
                                        int ret = encode(flv_codec_context, frame_encoded, flv_packet,
                                                         frames, VSTR_CODEC_FLV);
                                        av_free_packet(&flv_packet);
                                        if (ret < 0) {
                                            LOG_ERR("Video Device (%s): Error FLV-encoding frame. Error code (%d)\n",
                                                    video_device_->name.c_str(), res);
                                            sws_freeContext(img_convert_ctx);
                                            err_count++;
                                            continue;
                                        }
                                    }
                                    // clear up
                                    sws_freeContext(img_convert_ctx);
                                }
                                av_free_packet(&packet);
                                return true;
                            }
                        }
                    }
                    else {
                        // something wrong happend while capturing
                        // free all resources, turn off cap and try again
                        LOG_ERR("Video Device (%s): Something wrong happened while capturing. Error Code: %d. Retry!\n", video_device_->name.c_str(), res);
                        video_device_->video_cap_opened = false;
                        close();
                        err_count++;
                        // for file playback return false()
                        if (video_device_->type == DEV_FILE) {
                            return false;
                        }
                    }
            }

            if (video_device_->video_cap_opened) {
                video_device_->video_cap_opened = false;
                close();
            }

            return false;
        }


        void ffmpeg_cap::close() {
            if (packet.pts != 0) {
                av_free_packet(&packet);
            }
            avcodec_close(codec_context);
            avcodec_close(mjpeg_codec_context);
            avcodec_close(flv_codec_context);
            av_free(frame);
            av_free(frame_encoded);
            av_free(buffer);
            avformat_close_input(&format_context);
        }


    }
}