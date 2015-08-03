// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file utils.cpp
*/


#include "ugcs/vstreamer/utils.h"

namespace ugcs {
namespace vstreamer {
namespace utils {

	int64_t getMilliseconds() {
        int64_t milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
		return milliseconds_since_epoch;

	}

    int64_t getMicroseconds() {
        int64_t microseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::microseconds(1);
        return microseconds_since_epoch;
    }


    bool isNumeric(std::string str) {
        if (str.length()==0) {
            return false;
        }
        return std::all_of(str.begin(), str.end(), ::isdigit);
    }

    std::string createFullFilename(std::string folder, std::string name, std::string ext) {
        if (folder.back()!= '/' && folder.back() != '\\') {
            folder = folder + "/";
        }
        return folder + name + "." + ext;
    }

    bool checkFileExists(std::string filename) {
        // check if file exists
        struct stat   buffer;
        return (stat (filename.c_str(), &buffer) == 0);
    }

    std::string getURIQueryString(std::string header, std::string operation_name) {
        std::size_t found = header.find(operation_name);
        std::size_t found2 = header.find("HTTP");
        std::string query = header.substr(found+operation_name.length(), found2-found-operation_name.length()-1);
        return query;
    }

    std::string long_to_hex_string(long value)
    {
        std::ostringstream stream;
        stream << std::hex << value;
        return stream.str();
    }


}
}
}