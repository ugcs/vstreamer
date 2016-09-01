// Copyright (c) 2015, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file ffmpeg_utils.cpp
*/


#include "ugcs/vstreamer/ffmpeg_utils.h"

namespace ugcs {
    namespace vstreamer {
        namespace ffmpeg_utils {

            void frame_free(AVFrame **frame) {
#if (LIBAVCODEC_VERSION_INT > ((53<<16)+(99<<8)+0))
                avcodec_free_frame(frame);
#else
                av_free(frame);
#endif
            }
        }
    }
}

