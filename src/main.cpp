// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file main.cpp
*/

#include <ugcs/vsm/vsm.h>
#include "ugcs/vstreamer/control_server.h"

using namespace std;

int main( int argc, char** argv )
{
	// vsm init (for config and logging purposes
	ugcs::vsm::Initialize(argc, argv, "vstreamer.conf");

	auto props = ugcs::vsm::Properties::Get_instance();
	int port = props->Get_int("vstreamer.server_port");
	if (port < 0 || port > 65535) {
		LOG_ERR("control server port (%d) out of range, stopping video streamer", port);
		return 1;
	}

	// Start control server. Server will detect devices and start mjpeg streaming servers for each of devices.
	// Also server present some REST services.
	ugcs::vstreamer::ControlServer *server = new ugcs::vstreamer::ControlServer(port);
	server->start();
	
    while(true) {
	    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }

}
