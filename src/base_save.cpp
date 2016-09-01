// Copyright (c) 2015, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file base_save.cpp
*/

#include "ugcs/vstreamer/base_save.h"

namespace ugcs {
namespace vstreamer {


    bool base_save::is_process_running() {
        return this->is_running;

    }


    void base_save::add_frame(std::map<int, video_frame*> *frames, int frame_type) {
        if (frames->count(frame_type) > 0 ) {

                video_frame *vf = frames->at(frame_type);

                this->frame->encoded_buffer_size = vf->encoded_buffer_size;
                this->frame->encoded_buffer = (unsigned char *) realloc(this->frame->encoded_buffer,
                                                                      (size_t) this->frame->encoded_buffer_size);
                memcpy(this->frame->encoded_buffer, vf->encoded_buffer,
                       (size_t) this->frame->encoded_buffer_size);
                this->frame->ts = vf->ts;
                this->frame->video_condition_.notify_all();
            }
    }

}
}