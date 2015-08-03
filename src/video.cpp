// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file video.cpp
*/


#include <ugcs/vstreamer/video.h>

void ugcs::vstreamer::video::Get_device_list(std::vector<video_device> &found_devices, vstreamer_parameters params) {


    std::vector<std::string> camera_names;
    // get camera names
    if (params.autodetect) {
        ugcs::vstreamer::video::Get_autodetected_device_list(camera_names);
    }

    // add allowed devices which are not in list
    for (int i=0; i<params.allowed_devices.size(); i++) {
        if (std::find(camera_names.begin(), camera_names.end(), params.allowed_devices[i]) == camera_names.end()) {
            camera_names.push_back(params.allowed_devices[i]);
        }
    }

    // remove excluded devices
    std::vector<std::string> excluded_devices = params.excluded_devices;
    camera_names.erase(std::remove_if(camera_names.begin(), camera_names.end(),
                    [excluded_devices](std::string val) { return (std::find(excluded_devices.begin(), excluded_devices.end(), val) != excluded_devices.end()); }),
            camera_names.end());


    // fill found_devices with cameras
    for (int i=0; i<camera_names.size(); i++) {
        video_device dev(DEV_CAMERA);
        dev.name = camera_names[i];
        dev.timeout = params.videodevices_timeout;
        dev.index = i;
        found_devices.push_back(dev);
    }
}


