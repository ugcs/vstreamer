// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * platform_sockets.h
 *
 * Defines platform specific parts of socket implementation.
 */

#ifndef VSTREAMER_SOCKETS_H_
#define VSTREAMER_SOCKETS_H_

#include <ugcs/vstreamer/platform_sockets.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <cstring>

namespace ugcs
{
namespace vstreamer
{
namespace sockets
{

// These two calls are for WSAStartup and friends. Nop in unix world.
void
Init_sockets();
void
Done_sockets();


int 
Disable_sigpipe(Socket_handle);

int
Close_socket(Socket_handle);


/**
         * @brief Send an http response message.
         * @param fildescriptor fd to send the answer to
         * @param http response code. Codes 200, 400 and 500 are accepted.
         * @param error message
         * @param http content type
         */
int Send_code(sockets::Socket_handle& fd, int response_code, const char *message, std::string content_type = "text/html");


int get_error();

}
}
}

#endif /* VSTREAMER_SOCKETS_H_ */
