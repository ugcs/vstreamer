// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* video.cpp
*
*
*  Windows specific part of video tasks impleme//ntation
*/

// This file should be built only on mac


#include <ugcs/vstreamer/video.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>

#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/IOCFPlugIn.h>

#include <CoreFoundation/CoreFoundation.h>


int ugcs::vstreamer::video::Get_autodetected_device_list(std::vector<std::string> &found_devices) {

	int i = 0;

	kern_return_t	kernResult;
	CFMutableDictionaryRef	classesToMatch;
	io_iterator_t mediaIterator;
	io_object_t		nextMedia;

	classesToMatch = IOServiceMatching("IOUSBDevice");

	if (classesToMatch == NULL) {
		return 0;
	}

	kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &mediaIterator);

	while (nextMedia = IOIteratorNext(mediaIterator)) {
		CFTypeRef name;
		IOUSBDeviceInterface **deviceInterface = NULL;
		IOCFPlugInInterface	**plugInInterface = NULL;
		SInt32 score;

		kern_return_t kr = IOCreatePlugInInterfaceForService(nextMedia, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		if ((kIOReturnSuccess != kr) || !plugInInterface) {
			continue;
		}

		HRESULT res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*)&deviceInterface);
		(*plugInInterface)->Release(plugInInterface);
		if (res || deviceInterface == NULL) {
			continue;
		}

		UInt8 currentDeviceClass;
		(*deviceInterface)->GetDeviceClass(deviceInterface, &currentDeviceClass);
		if (currentDeviceClass != kUSBMiscellaneousClass) {
			continue;
		}

		name = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kUSBProductString), kCFAllocatorDefault, 0);

		if (name == NULL) {
			found_devices.push_back("USB Camera");
			i++;
		}
		else {
			const char *buf = CFStringGetCStringPtr((CFStringRef)name, kCFStringEncodingMacRoman);
			if (buf == NULL) {
				CFIndex length = CFStringGetLength((CFStringRef)name);
		                CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
                		char *buffer = (char *)malloc(maxSize);
		                if (CFStringGetCString((CFStringRef)name, buffer, maxSize, kCFStringEncodingUTF8) ) {
		                    std::string std_name(buffer);
                		    found_devices.push_back(std_name);
		                }
		                else {
                		    found_devices.push_back("Undefined camera");
		                }
			}
			else {
				std::string std_name(buf);
				found_devices.push_back(std_name);
			}
			i++;
		}
	}
	return i;
}




