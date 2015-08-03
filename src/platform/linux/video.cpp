// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* video.cpp
*
*  Windows specific part of video tasks impleme//ntation
*/

// This file should be built only on unix platforms

#include <ugcs/vsm/vsm.h>

#include <ugcs/vstreamer/video.h>

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <algorithm>

int ugcs::vstreamer::video::Get_autodetected_device_list(std::vector<std::string> &found_devices) {
	
	DIR *dir;
	struct dirent *ent;
	int i = 0;
	if ((dir = opendir("/sys/class/video4linux")) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			std::string dir_name(ent->d_name);
			if (dir_name.find("video")!=std::string::npos) {
				i++;
				found_devices.push_back("/dev/" + dir_name);
			}
		}
	}
	else {
		LOG_DEBUG("Get_device_count: Error opening dir video4linux");
	}
	
	return i;
}



