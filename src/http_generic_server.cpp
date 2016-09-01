// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file http_generic_server.h
*/
#include "ugcs/vstreamer/http_generic_server.h"


template<typename T>
inline T min(T a, T b) {
	return (a < b ? a : b);
}

namespace ugcs {

namespace vstreamer {
	
	HttpGenericServer::HttpGenericServer(int port) :
		stop_requested_(false), port_(port) {
		sd_len = 0;
	}

	HttpGenericServer::~HttpGenericServer() {
	}

	void HttpGenericServer::run() {

		LOG("HttpServer (%d): Starting http server", port_);
		ugcs::vstreamer::sockets::Init_sockets();

		struct addrinfo *aip, *aip2;
		struct addrinfo hints;
		struct sockaddr_storage client_addr;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		fd_set selectfds;
		int max_fds = 0;

		char name[NI_MAXHOST];

		memset(&hints, '\0', sizeof(hints));
		//todo: change to INET
		hints.ai_family = PF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;


		int err;
		snprintf(name, sizeof(name), "%d", port_);

		if ((err = getaddrinfo(NULL, name, &hints, &aip)) != 0) {
			LOG_ERROR("HttpServer (%d): %s", port_, gai_strerror(err));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < MAX_NUM_SOCKETS; i++)
			sd[i] = (sockets::Socket_handle) -1;

		/* open sockets for server (1 socket / address family) */
		int i = 0;
		int on;

		for (aip2 = aip; aip2 != NULL; aip2 = aip2->ai_next) {
			if ((sd[i] = socket(aip2->ai_family, aip2->ai_socktype, 0)) < 0) {
				continue;
			}
		
			if (sockets::Disable_sigpipe(sd[i]) < 0) {
				LOG_ERROR("HttpServer (%d): set nonblocking failed ", port_);
			}


			/** ignore "socket already in use" errors */
			on = 1;
			if (setsockopt(sd[i], SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
				LOG_ERROR("HttpServer (%d): setsockopt(SO_REUSEADDR) failed ", port_);
			}

			/** IPv6 socket should listen to IPv6 only, otherwise we will get "socket already in use" */
			on = 1;
			if (aip2->ai_family == AF_INET6 && setsockopt(sd[i], IPPROTO_IPV6,
				IPV6_V6ONLY, (char *)&on, sizeof(on)) < 0) {
				LOG_ERROR("HttpServer (%d): setsockopt(IPV6_V6ONLY) failed ", port_);
			}

			if (bind(sd[i], aip2->ai_addr, aip2->ai_addrlen) < 0) {
				LOG_ERROR("HttpServer (%d): bind error, %d, error: %d ", port_, sd[i], sockets::get_error());
				sd[i] = (sockets::Socket_handle) -1;
				continue;
			}

			if (listen(sd[i], 10) < 0) {
				LOG_ERROR("HttpServer (%d): listen error ", port_);
				sd[i] = (sockets::Socket_handle) -1;
			}
			else {
				i++;
				if (i >= MAX_NUM_SOCKETS) {
					LOG_ERROR("HttpServer (%d): Maximum number of server sockets exceeded ", port_);
					i--;
					break;
				}
			}
		}

		sd_len = i;


		if (sd_len < 1) {
			LOG_ERROR("HttpServer (%d): Bind failed ", port_);
			exit(EXIT_FAILURE);
		}
		else {
			LOG("HttpServer (%d): Bind succeded ", port_);
		}

		LOG("HttpServer (%d): Waiting for clients to connect ", port_);
	
		/* create a child for every client that connects */
		while (!stop_requested_) {
			
			do {
				FD_ZERO(&selectfds);
				for (i = 0; i < MAX_NUM_SOCKETS; i++) {
					if (sd[i] != -1) {
						FD_SET(sd[i], &selectfds);
						if (sd[i] > max_fds)
							max_fds = sd[i];
					}
				}

				err = select(max_fds + 1, &selectfds, NULL, NULL, NULL);

				if (err < 0 && errno != EINTR) {
					LOG_ERROR("HttpServer (%d): Select failed ", port_);
					exit(EXIT_FAILURE);
				}

			} while (err <= 0 && !stop_requested_);
			LOG("HttpServer (%d): Client connected ", port_);

			for (i = 0; i < max_fds + 1; i++) {
				if (i<MAX_NUM_SOCKETS) {
					if (sd[i] != -1 && FD_ISSET(sd[i], &selectfds)) {
						sockets::Socket_handle fd = accept(sd[i], (struct sockaddr *) &client_addr,
														   &addr_len);
						if (sockets::Disable_sigpipe(fd) < 0) {
							LOG_ERROR("HttpServer (%d): set nonblocking fd failed ", port_);
						}
						/* start new thread that will handle this TCP connected client */

						if (getnameinfo((struct sockaddr *) &client_addr, addr_len,
										name, sizeof(name), NULL, 0, NI_NUMERICHOST) == 0) {
							LOG("HttpServer (%d): Serving client %s", port_, name);
						}
						if (fd == -1) {
							sd[i] = (sockets::Socket_handle) -1;
							break;
						}
						std::thread trr(std::bind(&HttpGenericServer::client, this, fd));
						trr.detach();
					}
				}
			}
		}

		LOG("HttpServer (%d): Leaving server thread, calling cleanup function now", port_);
		freeaddrinfo(aip);
		cleanUp();
	}


		void HttpGenericServer::initIOBuffer(iobuffer *iobuf) {
			memset(iobuf->buffer, 0, sizeof(iobuf->buffer));
			iobuf->level = 0;
		}

		void HttpGenericServer::initRequest(request *req) {
			req->type = A_UNKNOWN;
			req->type = A_UNKNOWN;
			req->parameter = NULL;
			req->client = NULL;
			req->credentials = NULL;
		}

		void HttpGenericServer::freeRequest(request *req) {
			if (req->parameter != NULL)
				free(req->parameter);
			if (req->client != NULL)
				free(req->client);
			if (req->credentials != NULL)
				free(req->credentials);
		}

		int HttpGenericServer::readWithTimeout(sockets::Socket_handle& fd, iobuffer *iobuf, char *buffer,
			size_t len, int timeout) {
			size_t copied = 0;
			int rc, i;
			fd_set fds;
			struct timeval tv;

			memset(buffer, 0, len);

			while ((copied < len)) {
				i = min((size_t)iobuf->level, len - copied);
				memcpy(buffer + copied, iobuf->buffer + IO_BUFFER - iobuf->level, (size_t) i);

				iobuf->level -= i;
				copied += i;
				if (copied >= len)
					return copied;

				/* select will return in case of timeout or new data arrived */
				tv.tv_sec = timeout;
				tv.tv_usec = 0;
				FD_ZERO(&fds);
				FD_SET(fd, &fds);
				if ((rc = select(fd + 1, &fds, NULL, NULL, &tv)) <= 0) {
					if (rc < 0)
						exit(EXIT_FAILURE);

					/* this must be a timeout */
					return copied;
				}

				initIOBuffer(iobuf);

				/*
				 * there should be at least one byte, because select signalled it.
				 * But: It may happen (very seldomly), that the socket gets closed remotly between
				 * the select() and the following read. That is the reason for not relying
				 * on reading at least one byte.
				 */
				//if ((iobuf->level = read(fd, &iobuf->buffer, IO_BUFFER)) <= 0) {
				if ((iobuf->level = recv(fd, iobuf->buffer, IO_BUFFER, 0)) <= 0) {
					/* an error occured */
					LOG_ERROR("HttpServer (%d): Error while reading request", port_);
					return -1;
				}

				/* align data to the end of the buffer if less than IO_BUFFER bytes were read */
				memmove(iobuf->buffer + (IO_BUFFER - iobuf->level), iobuf->buffer,
						(size_t) iobuf->level);
			}

			return 0;
		}

		int HttpGenericServer::readLineWithTimeout(sockets::Socket_handle& fd, iobuffer *iobuf, char *buffer,
			size_t len, int timeout) {
			char c = '\0', *out = buffer;
			unsigned int i;

			memset(buffer, 0, len);

			for (i = 0; i < len && c != '\n'; i++) {
				if (readWithTimeout(fd, iobuf, &c, 1, timeout) <= 0) {
					/* timeout or error occured */
					return -1;
				}
				*out++ = c;
			}

			return i;
		}


        void HttpGenericServer::sendCode(sockets::Socket_handle& fd, int response_code, const char *message, std::string content_type) {
			int ret = sockets::Send_code(fd, response_code, message, content_type);
			if (ret < 0) {
				LOG_ERROR("HttpServer (%d): write failed, done anyway, error code: %d", port_, ret);
			}
			else {
				LOG_DEBUG("HttpServer (%d): write ok", port_);
			}

		}
}
}

