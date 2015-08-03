// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_utils.cpp
*/


#include "ugcs/vstreamer/ffmpeg_utils.h"

namespace ugcs {
    namespace vstreamer {
        namespace ffmpeg_utils {


            AVFrame* frame_alloc(){

                AVFrame* frame;

                // Some playing with different versions of ffmpeg for frames allocation
#if (LIBAVCODEC_VERSION_INT >= ((55 << 16) + (0 << 8) + 0))
                frame = av_frame_alloc();

#else
                frame = avcodec_alloc_frame();

#endif
                return frame;

            }



            int packet_from_data(AVPacket *pkt, uint8_t *data, int size) {
// windows & mac & 14.10+
#if (LIBAVCODEC_VERSION_MAJOR >= 55)

                if (size >= INT_MAX - FF_INPUT_BUFFER_PADDING_SIZE)
                    return AVERROR(EINVAL);

                pkt->buf = av_buffer_create(data, size + FF_INPUT_BUFFER_PADDING_SIZE,
                                            av_buffer_default_free, NULL, 0);
                if (!pkt->buf)
                    return AVERROR(ENOMEM);

                pkt->data = data;
                pkt->size = size;

                return 0;
#else
// 14.04
#if (LIBAVCODEC_VERSION_MAJOR >= 54)
                if (size >= INT_MAX - FF_INPUT_BUFFER_PADDING_SIZE)
                    return AVERROR(EINVAL);

                pkt->data = data;
                pkt->size = size;

                return 0;
#endif
#endif
            }

        }
    }
}