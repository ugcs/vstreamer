// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/** @file ffmpeg_utils.h
 * @file ffmpeg)utils.h
 *
 * Various common utilities
 */
#ifndef VSTREAMER_FFMPEG_UTILS_H_
#define VSTREAMER_FFMPEG_UTILS_H_

#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


namespace ugcs {
    namespace vstreamer {
        namespace ffmpeg_utils {

          /**
          * @brief frame allocation for various ffmpeg version
          */
            AVFrame* frame_alloc();

           /**
           * @brief frame allocation for various ffmpeg version
           */
            void frame_free(AVFrame **frame);

            /**
            * @brief createing AVPacket from data азк various ffmpeg versions
            */
            int packet_from_data(AVPacket *pkt, uint8_t *data, int size);
        }
    }
}

#endif /* VSTREAMER_FFMPEG_UTILS_H_ */
