// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file http_generic_server.h
*
* Base class for http-servers realisations
*/


#ifndef VSTREAMER_HTTP_SERVER_H_
#define VSTREAMER_HTTP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ugcs/vsm/vsm.h>

#include <thread>
#include <functional>

#include "ugcs/vstreamer/sockets.h"
#include "ugcs/vstreamer/utils.h"
#include "ugcs/vstreamer/common.h"


/**  Maximum number of server sockets (i.e. protocol families) to listen. */
#define MAX_NUM_SOCKETS    100
#define IO_BUFFER     256
#define BUFFER_SIZE   1024

namespace ugcs{
	namespace vstreamer {

		/** the server request-response types */
		typedef enum {
			A_UNKNOWN, A_STREAM, A_GETINFO, A_COMMAND, A_HELP, A_GETPARAMS, A_SETPARAMS, A_SETSTREAM, A_SETOUTERSTREAM, A_PLAYBACK, A_GETVIDEOINFO, A_DELETEVIDEO, A_DOWNLOADVIDEO
		} answer_t;

		/** request info */
		typedef struct {
			answer_t type;
			char *parameter;
			char *client;
			char *credentials;
		} request;

		/** the iobuffer structure is used to read from the HTTP-client */
		typedef struct {
            /** how full is the buffer */
			int level;
            /** the data */
			char buffer[IO_BUFFER];
		} iobuffer;

		
		/**
		 * @class HttpGenericServer
		 * @brief
		 */
		class HttpGenericServer {
		public:

			/**
            * @brief Constructor
            * @param port - http port of server
            * */
			HttpGenericServer(int port);

            /**
            * @brief Destructor
            */
			~HttpGenericServer();

			/**
			 * @brief  Closes all client threads
			 */
			virtual void cleanUp() = 0;

			/**
			 * @brief  Client thread function
			 *         Serve a connected TCP-client. This thread function is called
			 *         for each connect of a HTTP client. It determines
			 *         if it is a valid HTTP request and dispatches between the different
			 *         response options.
			 */
			virtual void client(sockets::Socket_handle& fd) = 0;

            /**
            * @brief  Starts server
            */
			void run();


		protected:

			/** http port of server */
			int port_;

            /** sockets array */
			sockets::Socket_handle sd[MAX_NUM_SOCKETS];
            /** sockets number */
			int sd_len;
            /** stop requested flag. When true - server begins stopping sequence*/
			bool stop_requested_;

			/**
			 * @brief Send an http response message.
			 * @param fildescriptor fd to send the answer to
			 * @param http response code. Codes 200, 400 and 500 are accepted.
			 * @param error message
             * @param http content type
			 */
			void sendCode(sockets::Socket_handle& fd, int response_code, const char *message, std::string content_type = "text/html");
			
			
			/**
			 * @brief Initializes the iobuffer structure properly
			 * @param Pointer to already allocated iobuffer
			 */
			void initIOBuffer(iobuffer *iobuf);

			/**
			 * @brief Initializes the request structure properly
			 * @param Pointer to already allocated req
			 */
			void initRequest(request *req);

			/**
			 * @brief If strings were assigned to the different members free them.
			 *        This will fail if strings are static, so always use strdup().
			 * @param req: pointer to request structure
			 */
			void freeRequest(request *req);

			/**
			 * @brief read with timeout, implemented without using signals
			 *        tries to read len bytes and returns if enough bytes were read
			 *        or the timeout was triggered. In case of timeout the return
			 *        value may differ from the requested bytes "len".
			 * @param fd.....: fildescriptor to read from
			 * @param iobuf..: iobuffer that allows to use this functions from multiple
			 *                 threads because the complete context is the iobuffer.
			 * @param buffer.: The buffer to store values at, will be set to zero
			 before storing values.
			 * @param len....: the length of buffer
			 * @param timeout: seconds to wait for an answer
			 * @return buffer.: will become filled with bytes read
			 * @return iobuf..: May get altered to save the context for future calls.
			 * @return func().: bytes copied to buffer or -1 in case of error
			 */
			int readWithTimeout(sockets::Socket_handle& fd, iobuffer *iobuf, char *buffer, size_t len, int timeout);

			/**
			 * @brief Read a single line from the provided fildescriptor.
			 *        This funtion will return under two conditions:
			 *        - line end was reached
			 *        - timeout occured
			 * @param fd.....: fildescriptor to read from
			 * @param iobuf..: iobuffer that allows to use this functions from multiple
			 *                 threads because the complete context is the iobuffer.
			 * @param buffer.: The buffer to store values at, will be set to zero
			 before storing values.
			 * @param len....: the length of buffer
			 * @param timeout: seconds to wait for an answer
			 * @return buffer.: will become filled with bytes read
			 * @return iobuf..: May get altered to save the context for future calls.
			 * @return func().: bytes copied to buffer or -1 in case of error
			 */
			int readLineWithTimeout(sockets::Socket_handle& fd, iobuffer *iobuf, char *buffer, size_t len, int timeout);
		};
	}
}
#endif  //VSTREAMER_HTTP_SERVER_H_

