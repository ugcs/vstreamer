// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * platform_sockets.h
 *
 * Defines platform specific parts of socket implementation.
 */

#ifndef __APPLE__
#error "This header should be included only in macos build."
#endif

#ifndef VSTREAMER_PLATFORM_SOCKETS_H_
#define VSTREAMER_PLATFORM_SOCKETS_H_


#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>  // inet_ntoa
#include <netdb.h>      // addrinfo
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

namespace ugcs
{
namespace vstreamer
{
namespace sockets
{

/** Unix specific socket handle */
typedef int Socket_handle;

/** Only linux build sets SEND_FLAGS to nonzero. */
const int SEND_FLAGS = 0;

}// namespace platform
}
}

#endif /* VSTREAMER_PLATFORM_SOCKETS_H_ */
