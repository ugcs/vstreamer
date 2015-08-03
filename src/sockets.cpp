// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
 * sockets.cpp
 *
 *  Common part of sockets implementation
 */

#include <ugcs/vstreamer/sockets.h>


int ugcs::vstreamer::sockets::Send_code(sockets::Socket_handle& fd, int response_code, const char *message, std::string content_type) {
    std::string s_tmp(message);
    char buffer[s_tmp.length() + 1024];
    memset(buffer, 0, s_tmp.length() + 1024);

    std::string header_tmp = "Connection: close\r\nServer: vstreamer_server\r\nCache-Control: no-cache\r\nConnection: close\r\nPragma: no-cache\r\n";
    header_tmp = header_tmp + "Content-Length: " + std::to_string(strlen(message)) + "\r\n";

    if (response_code == 200) {
        sprintf(buffer, "HTTP/1.0 200 OK\r\n"
                "Content-type: %s\r\n"
                "%s"
                "\r\n %s",content_type.c_str(),  header_tmp.c_str(), message);
    }
    else if (response_code == 500) {
        sprintf(buffer, "HTTP/1.0 500 Internal Server Error\r\n"
                "Content-type: text/plain\r\n"
                "%s"
                "\r\n"
                "%s", header_tmp.c_str(), message);
    }
    else if (response_code == 400) {
        sprintf(buffer, "HTTP/1.0 400 Bad Request\r\n"
                "Content-type: text/plain\r\n"
                "%s"
                "\r\n"
                "%s", header_tmp.c_str(), message);
    }
    else {
        sprintf(buffer, "HTTP/1.0 501 Not Implemented\r\n"
                "Content-type: text/plain\r\n"
                "%s"
                "\r\n"
                "%s", header_tmp.c_str(), message);
    }

    return send(fd, buffer, strlen(buffer), 0);

}





