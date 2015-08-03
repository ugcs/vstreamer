// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * platform_sockets.h
 *
 * Defines platform specific parts of socket implementation.
 */

#ifndef _WIN32
#error "This header should be included only in Windows build."
#endif

#ifndef VSTREAMER_PLATFORM_SOCKETS_H_
#define VSTREAMER_PLATFORM_SOCKETS_H_

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0501 //http://stackoverflow.com/questions/11786479/why-cant-g-find-getnameinfo-in-ws2-32
//#endif



// Windows specific headers for socket stuff

#include <winsock2.h>
#include <ws2tcpip.h> // for socklen_t
#include <io.h>


/* Some weird code can define these macros in Windows headers. */
// ???????????
#ifdef ERROR
#undef ERROR
#endif
#ifdef interface
#undef interface
#endif
#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif


namespace ugcs
{
//namespace vsm
namespace vstreamer
{
namespace sockets
{

// Windows specific socket handle
typedef SOCKET Socket_handle; /* Win */

// Only linux build sets SEND_FLAGS to nonzero.
const int SEND_FLAGS = 0;

}// namespace platform
}
}

#endif /* VSTREAMER_PLATFORM_SOCKETS_H_ */
