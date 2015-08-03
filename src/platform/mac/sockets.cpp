// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * sockets.cpp
 *
 *  Mac specific part of sockets implementation
 */

// This file should be built only on Mac platforms
#include <ugcs/vstreamer/sockets.h>

int
ugcs::vstreamer::sockets::Disable_sigpipe(sockets::Socket_handle handle)
{
    int val = 1;
    if (setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val))) {
        return -1;
    }
    return 0;
}


