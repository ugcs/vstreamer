// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * sockets.cpp
 *
 *  Linux\Mac specific part of sockets implementation
 */

// This file should be built only on Linux platforms
#include <ugcs/vstreamer/sockets.h>
#include <unistd.h>

void
ugcs::vstreamer::sockets::Init_sockets() {
};

void
ugcs::vstreamer::sockets::Done_sockets() {
};


int
ugcs::vstreamer::sockets::Close_socket(sockets::Socket_handle s)
{
    return close(s);
    
}

