// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/** @file utils.h
 * @file utils.h
 *
 * Various common utilities
 */
#ifndef VSTREAMER_UTILS_H_
#define VSTREAMER_UTILS_H_

#include <chrono>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>


namespace ugcs {
namespace vstreamer {
namespace utils {


    /**
    * @brief Return milliseconds for mjpeg timestamps
    * @return milliseconds for mjpeg timestamps
    */
    int64_t getMilliseconds();

    /**
    * @brief Return microseconds for mjpeg timestamps
    * @return microseconds for mjpeg timestamps
    */
    int64_t getMicroseconds();
    /**
    * @brief Check if string is numeric (decimal)
    * @param str - input string
    * @return true, if input string is numeric (decimal)
    */
    bool isNumeric(std::string str);
    /**
    * @brief Create full filename with path
    * @param folder - folder path
    * @param name -  filename
    * @param ext - file extension
    * @return full filename
    */
    std::string createFullFilename(std::string folder, std::string name, std::string ext);

    /**
       * @brief Check if file exists
       * @param filename -  filename
       * @return true if file exists
       */
    bool checkFileExists(std::string filename);

    /**
   * @brief Get URI Query string
   * @param header - HTTP header of request
   * @param operation_name -  operation name, for example "playback?" or video/
   * @return querystring
   */
    std::string getURIQueryString(std::string header, std::string operation_name);

    /**
  * @brief Transform int value to hex and return it as string
  * @param value - value
  * @return int as hex as srting
  */
    std::string long_to_hex_string(long value);


}
} 
} 

#endif /* VSTREAMER_UTILS_H_ */
