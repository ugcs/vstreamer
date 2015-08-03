// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * sockets.cpp
 *
 *  Linux specific part of sockets implementation
 */

// This file should be built only on Linux platforms
#include <ugcs/vstreamer/sockets.h>

int
ugcs::vstreamer::sockets::Disable_sigpipe(Socket_handle s)
{
    signal(SIGPIPE, SIG_IGN); 
    return 0;
}
