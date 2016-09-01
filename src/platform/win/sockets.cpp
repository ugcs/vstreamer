// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * sockets.cpp
 *
 *  Windows specific part of sockets implementation
 */

// This file should be built only on windows platforms
#if _WIN32

#include <ugcs/vstreamer/sockets.h>

void
ugcs::vstreamer::sockets::Init_sockets()
{
    static WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {

		}
        
}

void
ugcs::vstreamer::sockets::Done_sockets()
{
	if (WSACleanup() != 0) {
		
	}
}

int
ugcs::vstreamer::sockets::Disable_sigpipe(Socket_handle s)
{
    return 0;
}

int
ugcs::vstreamer::sockets::Close_socket(Socket_handle s)
{
    return closesocket(s);
}

int
ugcs::vstreamer::sockets::get_error() {
	return WSAGetLastError();
}



#endif // _WIN32
