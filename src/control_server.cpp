// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/**
* @file control_server.cpp
*/

#include "ugcs/vstreamer/control_server.h"

namespace ugcs {
namespace vstreamer {
 
  
  
	ControlServer::ControlServer(int port) : HttpGenericServer(port), max_port_(port) {
	}

	ControlServer::~ControlServer() {
	}


	void ControlServer::start() {

        startSSDPListener();

		std::thread t(&ControlServer::execute, this);
		t.detach();
	}
  

	void ControlServer::execute() {
		// search for devices
		std::thread t(&ControlServer::scanForDevices, this);
		t.detach();
		run();
		
	}
  
    int ControlServer::find_next_port(){

        bool found_next_port = false;
        int next_port = max_port_;

        while (!found_next_port) {
            next_port++;
            bool server_with_port_exists = false;
            for (auto iter = http_servers.begin(); iter != http_servers.end(); iter++) {
                MjpegServer *ms = iter->second;
                if (ms->getPort() == next_port) {server_with_port_exists = true; break;}
            }
            if (!server_with_port_exists) {found_next_port = true;}
        }
        return next_port;
    }


	void ControlServer::scanForDevices() {

		auto props = ugcs::vsm::Properties::Get_instance();


        // get streams (ardrone and gopro) parameters from config
		for (auto iter = props->begin("vstreamer.inputstream"); iter != props->end(); iter++) {
			video_device is(DEV_STREAM);
            std::string val = props->Get((*iter));
            is.init_stream(val);
			server_parameters.input_streams.push_back(is);
			LOG("Input streams: %s  %s   %d  %d  %i", is.name.c_str(), is.url.c_str(), is.width, is.height, (int)is.timeout);
		}

		// get excluded devices
		for (auto iter = props->begin("vstreamer.videodevices.exclude"); iter != props->end(); iter++) {
			std::string val = props->Get((*iter));
            server_parameters.excluded_devices.push_back(val);
			LOG("Device excluded: %s", val.c_str());
		}

        // get allowed devices
        for (auto iter = props->begin("vstreamer.videodevices.allow"); iter != props->end(); iter++) {
            std::string val = props->Get((*iter));
            server_parameters.allowed_devices.push_back(val);
            LOG("Device allowed: %s", val.c_str());
        }

        // get autodetecting flag
        server_parameters.autodetect = (props->Get_int("vstreamer.videodevices.autodetect") != 0);

        // get videodevices timeout
        server_parameters.videodevices_timeout = props->Get_int("vstreamer.videodevices.timeout");

        // get folder for saved video
        server_parameters.saved_video_folder = props->Get("vstreamer.saved_video.folder");


		while (!stop_requested_) {


            VS_WAIT(1000);
			
			// find cameras
			std::vector<video_device> found_devices;
            video::Get_device_list(found_devices, server_parameters);
            // add streams
            found_devices.insert(found_devices.end(), server_parameters.input_streams.begin(), server_parameters.input_streams.end());

            int device_count = found_devices.size();

            // start http server for every new found device
            for (int i = 0; i < device_count; i++) {
                // if device already in list- do not cap
                std::string device_name = found_devices[i].name;
                if (device_list.count(device_name) == 0) {

                    // ok, new device.
                    // check if this device is capable to capture video
                    bool res = found_devices[i].init_video_cap();
                    if (res) {

                        found_devices[i].port = this->find_next_port();
                        device_list[device_name] = found_devices[i];
                        device_list[device_name].init_outer_streams();

                        VS_WAIT(500);

                        // check if device server is not running already
                        if (!device_list[device_name].server_started) {

                            ugcs::vstreamer::MjpegServer *mjpeg_server;
                            if (http_servers.count(device_name) > 0) {
                                mjpeg_server = http_servers[device_name];
                                mjpeg_server->init(&device_list[device_name]);
                            } else {
                                mjpeg_server = new ugcs::vstreamer::MjpegServer(device_list[device_name].port,
                                                                                &device_list[device_name]);
                                http_servers[device_name] = mjpeg_server;
                            }
                            mjpeg_server->start();
                        }
                    }
                }
            }

            // check states of http servers
            for (auto iter = http_servers.begin(); iter != http_servers.end(); iter++) {
                MjpegServer *ms = iter->second;
                std::string server_device_name = iter->first;  // device name
                 // check timeout
                if (ms->started && ms->isTimeout()) {
                    ms->stop();
                    //http_servers.erase(is.name);
                    device_list.erase(server_device_name);
                    continue;
                }
                // check device availability
                bool device_exists = false;
                for (int i=0; i<found_devices.size(); i++) {
                    if (found_devices[i].name == server_device_name) {device_exists = true;}
                }
                if (!device_exists) {
                    ms->stop();
                    device_list.erase(server_device_name);
                }
            }
		}
	}
		

	void ControlServer::client(sockets::Socket_handle& fd) {

		int64_t request_ts_milli = utils::getMilliseconds();
        int64_t request_ts_micro = utils::getMicroseconds();

        int cnt;
		char buffer[BUFFER_SIZE] = { 0 };
        //, *pb = buffer;
		iobuffer iobuf;
		request req;

        LOG_DEBUG("Command Server: client is started, fd=%d", fd);
		
		/* initializes the structures */
		initIOBuffer(&iobuf);
		initRequest(&req);

		/* What does the client want to receive? Read the request. */
		memset(buffer, 0, sizeof(buffer));
		if ((cnt = readLineWithTimeout(fd, &iobuf, buffer, sizeof(buffer) - 1, 500))
				== -1) {
			LOG_ERROR("Command Server: Error while reading request, cnt=%d, sizeof(buffer) =%zu ", cnt, sizeof(buffer));
			ugcs::vstreamer::sockets::Close_socket(fd);
			return;
		}
    LOG_DEBUG("%s", buffer);
		/* determine what to deliver */
		if (strstr(buffer, "GET /streams") != NULL) {
			req.type = A_GETINFO;
			LOG_DEBUG("Command Server: Requested streams info");
		}
        else if(strstr(buffer, "PUT /stream") != NULL) {
            req.type = A_SETSTREAM;
            LOG_DEBUG("Command Server: Requested stream update");
        }
        else if(strstr(buffer, "GET /parameters") != NULL) {
            req.type = A_GETPARAMS;
            LOG_DEBUG("Command Server: Requested parameters info");
        }
        else if(strstr(buffer, "PUT /parameters") != NULL) {
            req.type = A_SETPARAMS;
            LOG_DEBUG("Command Server: Requested parameters update");

        }
        else if(strstr(buffer, "POST /outerstream") != NULL) {
            req.type = A_SETOUTERSTREAM;
            LOG_DEBUG("Command Server: Requested outer stream update");

        }
        else if(strstr(buffer, "GET /playback") != NULL) {
            req.type = A_PLAYBACK;
            LOG_DEBUG("Command Server: Requested playback");

        }
        else if(strstr(buffer, "GET /video/") != NULL) {
            req.type = A_GETVIDEOINFO;
            LOG_DEBUG("Command Server: Requested video info");

        }
        else if(strstr(buffer, "DELETE /video/") != NULL) {
            req.type = A_DELETEVIDEO;
            LOG_DEBUG("Command Server: Requested video delete");

        }
        else if(strstr(buffer, "GET /download/") != NULL) {
            req.type = A_DOWNLOADVIDEO;
            LOG_DEBUG("Command Server: Requested video download");

        }
        else { // Display help
			req.type = A_HELP;
			LOG_DEBUG("Command Server: Requested help screen");
		}


		/* now it's time to answer */
		switch (req.type) {
		case A_GETINFO: {
			LOG_DEBUG("Command Server: Request for streams info");
			sendStreamsInfo(fd);
			break;
		}
        case A_GETPARAMS: {
            LOG_DEBUG("Command Server: Request for params info");
            sendParamsInfo(fd);
            break;
        }
        case A_PLAYBACK: {
            std::string header(buffer);
            std::string query = utils::getURIQueryString(header, "playback?");
            LOG_DEBUG("Command Server: Request for playback with query %s.", query.c_str());
            startPlayback(fd, query, request_ts_micro);
            break;
        }
        case A_GETVIDEOINFO:
        case A_DELETEVIDEO:
        {
            std::string header(buffer);
            std::string video_id_param = utils::getURIQueryString(header, "video/");
            LOG_DEBUG("Command Server: Request for video %s.", video_id_param.c_str());
            if (req.type == A_GETVIDEOINFO) {
                getVideoMetadata(fd, video_id_param);
            } else if (req.type == A_DELETEVIDEO) {
                deleteVideo(fd, video_id_param);
            }
            break;
        }
            case A_DOWNLOADVIDEO:
            {
                std::string header(buffer);
                std::string video_id_param = utils::getURIQueryString(header, "download/");
                LOG_DEBUG("Command Server: Request for video download %s.", video_id_param.c_str());
                downloadVideo(fd, video_id_param);
                break;
            }
        case A_SETSTREAM:
        case A_SETPARAMS:
        case A_SETOUTERSTREAM:
        {
            LOG_DEBUG("Command Server: Request for params set");
            // need to parse body
            // skip headers except Content Length
            int content_length = 0;
            do {
                memset(buffer, 0, sizeof(buffer));

                if ((cnt = readLineWithTimeout(fd, &iobuf, buffer, sizeof(buffer) - 1, 5)) == -1) {
                    freeRequest(&req);
                    ugcs::vstreamer::sockets::Close_socket(fd);
                    return;
                }
                if (strstr(buffer, "Content-Length: ") != NULL) {
                    char *val_str = strdup(buffer + strlen("Content-Length: "));
                    std::string val(val_str);
                    free(val_str);
                    content_length = std::stoi(val);
                }
            } while (cnt > 2 && !(buffer[0] == '\r' && buffer[1] == '\n'));
            // read body
            LOG_DEBUG("Content length=%d", content_length);

            if (content_length<1) {
                LOG_ERROR("Command Server: Incorrect content Length^ %d  ", content_length);
                ugcs::vstreamer::sockets::Close_socket(fd);
                return;
            }
            char *body_buffer = new char[content_length+1];

            if (readWithTimeout(fd, &iobuf, body_buffer, (size_t)content_length, 5) == -1) {
                freeRequest(&req);
                ugcs::vstreamer::sockets::Close_socket(fd);
                return;
            }

            // end of string
            body_buffer[content_length] = '\0';

            std::string body(body_buffer);
            LOG_DEBUG("Body=%s", body.c_str());

            delete [] body_buffer;

            if (req.type == A_SETPARAMS) {
                writeParamsInfo(fd, body);
            }
            else if (req.type == A_SETSTREAM) {
                writeStreamInfo(fd, body, request_ts_milli);
            }
            else if (req.type == A_SETOUTERSTREAM) {
                writeOuterStreamInfo(fd, body);
            }

            break;
        }
		default:
			LOG_DEBUG("Command Server: Unknown or help request, Sending help message");
			sendHelpMessage(fd);
		}
		
		freeRequest(&req);

		LOG("Command Server: Disconnecting HTTP client");
		return;
	}

	void ControlServer::cleanUp() {
		LOG("Command Server: Cleaning up ressources allocated by server thread");
		stop_requested_ = true;

		for (int i = 0; i < MAX_NUM_SOCKETS; i++) {
			//sockets::Close_socket(sd[i]);
		}

        stopSSDPListener();

        sockets::Done_sockets();

    }
	
	void ControlServer::sendStreamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd) {
		/* message looks like
		[
			{"port": 8082, "name": "camera 0", "type": 0},
			{"port": 8083, "name": "camera 1", "type": 0},
			{"port": 8084, "name": "ArDrone", "type": 1}
		]
		*/
		
		std::string msg = "[\r\n";
		std::map<std::string, video_device>::iterator iter;
		bool isFirstStream = true;
		for (iter = device_list.begin(); iter != device_list.end(); ++iter) {
			video_device *dv = &(iter->second);
			if (dv->server_started) {
				if (isFirstStream) {
					isFirstStream = false; 
				} else { 
					msg += ", \r\n";
				}
				msg += "{\"port\":" + std::to_string(dv->port) + ", ";
				msg += "\"name\":\"" + dv->name + "\", ";
				msg += "\"index\":\"" + std::to_string(dv->index) + "\", ";
                msg += "\"is_recording_active\":" + (std::string)(dv->is_recording_active ? "true" : "false") + ", ";
                msg += "\"video_id\":\"" + dv->recording_video_id + "\", ";
                msg += "\"recording_duration_sec\":" + std::to_string((int)(dv->get_recording_duration()/1000)) + ", ";
                msg += "\"last_recording_error_code\":\"" + dv->last_recording_error_code + "\", ";
				msg += "\"type\":" + std::to_string(dv->type) + ", ";

                msg += "\"outer_streams\":[";
                bool is_first_outer = true;
                for (auto os_iter = dv->outer_streams.begin(); os_iter != dv->outer_streams.end(); ++os_iter ) {
                    if (is_first_outer) {
                        is_first_outer = false;
                    } else {
                        msg += ", \r\n";
                    }
                    std::shared_ptr<base_save> os = os_iter->second;
                    msg += "{\"url\":\"" + os->output_filename + "\", ";
                    if (os->outer_stream_type == VSTR_OST_USTREAM) {
                        msg += "\"type\":\"" + VSTR_OST_USTREAM_NAME + "\", ";
                    } else if (os->outer_stream_type == VSTR_OST_TWITCH) {
                        msg += "\"type\":\"" + VSTR_OST_TWITCH_NAME + "\", ";
                    } else if (os->outer_stream_type == VSTR_OST_YOUTUBE) {
                        msg += "\"type\":\"" + VSTR_OST_YOUTUBE_NAME + "\", ";
                    } else {
                        msg += "\"type\":\"" + VSTR_OST_UNKNOWN_NAME + "\", ";
                    }
                    //msg += "\"state_msg\":\"" + os->state_message + "\", ";
                    if (os->outer_strem_error_code != VSTR_OST_ERR_NONE) {
                        msg += "\"error_code\":" + std::to_string(os->outer_strem_error_code) + ", ";
                    }
                    msg += "\"state\":" + std::to_string(os->outer_stream_state) + " }";
                }
                msg += "]}\r\n";
			}
		}
		msg += "]\r\n";
		
		if (msg.length()>0) {
			sendCode(fd, 200, msg.c_str(), "application/json");
			LOG_DEBUG("Command Server: REST GET Streaminfo response");
		}
	}

    void ControlServer::writeOuterStreamInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body) {
        // parse body
        // {"port": 8082, "streams":[{"type"="ustream", "url":"url", "is_active": true},
        //                           {"type"="twitch", "url":"url", "is_active": false}]
        // }
        std::string response = "";
        Json::Reader jreader;
        Json::Value root;

        jreader.parse(body.c_str(), root, false);
        int req_port = root.get("port", 0).asInt();
        Json::Value streams = root.get("streams", "");

        outer_stream_type_enum stream_type;

        if (!streams.isArray()) {
            response = "No outer streams send for " + std::to_string(req_port);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Error while opening broadcastfor port %d. No outer streams in request.", req_port);
            return;
        }

        std::lock_guard<std::mutex> lock(broadcast_mutex);

        // find device with port
        video_device *dv = nullptr;
        bool device_is_found = false;
        for (auto iter = device_list.begin(); iter != device_list.end(); ++iter) {
            dv = &(iter->second);
            if (dv->port == req_port) {
                device_is_found = true;
                break;
            }
        }
        if (device_is_found) {
            // send http response
            sendCode(fd, 200, "", "application/json");
            // iterate through streams
            for(Json::Value::iterator iter=streams.begin(); iter!=streams.end(); iter++) {
                Json::UInt ui  = iter.index();
                Json::Value stream = streams.get(ui, "");

                bool req_is_active = stream.get("is_active", false).asBool();
                std::string req_type = stream.get("type", "").asString();
                std::string req_url = stream.get("url", "").asString();



                // check type
                if (req_type == VSTR_OST_USTREAM_NAME) {
                    stream_type = VSTR_OST_USTREAM;
                } else if (req_type == VSTR_OST_TWITCH_NAME) {
                    stream_type = VSTR_OST_TWITCH;
                } else if (req_type == VSTR_OST_YOUTUBE_NAME) {
                    stream_type = VSTR_OST_YOUTUBE;
                } else {
                    LOG_ERROR("Error while opening broadcastfor port %d. Unsupported stream type <%s>.", req_port, req_type.c_str());
                    return;
                }

                bool res = dv->set_outer_stream(stream_type, req_url, req_is_active, response);
                if (!res) {
                    LOG_ERROR("Error while opening broadcast of type <%s> for port %d. %s", req_type.c_str(), req_port, response.c_str());
                    return;
                }
            }
        }
        else {
            response = "Error while opening broadcast. No devices with port " + std::to_string(req_port) + "were found.";
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("%s", response.c_str());
            return;
        }
        return;
    }

    void ControlServer::writeStreamInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body, int64_t ts_milli) {
        // parse body
        // {"port": 8082, "is_recording_active": true, "video_id": "test"}
        std::string response = "";
        Json::Reader jreader;
        Json::Value root;
        jreader.parse(body.c_str(), root, false);
        int req_port = root.get("port", 0).asInt();
        bool req_is_recording_active = root.get("is_recording_active", false).asBool();
        std::string req_recording_filename = root.get("video_id", "").asString();

        // find device with port
        video_device *dv = nullptr;
        bool device_is_found = false;
        for (auto iter = device_list.begin(); iter != device_list.end(); ++iter) {
            dv = &(iter->second);
            if (dv->port == req_port) {
                device_is_found = true;
                break;
            }
        }
        if (device_is_found) {
            // if device to be started:
            if (req_is_recording_active) {
                // if device is already started
                if (dv->is_recording_active) {
                    response = std::to_string(VSTR_REC_ERR_RECORDING_IS_ALREADY_IN_PROCESS);
                    sendCode(fd, 400, response.c_str(), "application/json");
                    return;
                }
                else {
                    // check if filename present
                    if (req_recording_filename.length()<1) {
                        //No filename provided for records session
                        response = std::to_string(VSTR_REC_ERR_VIDEO_NOT_FOUND);
                        sendCode(fd, 400, response.c_str(), "application/json");
                        return;
                    }
                    // check if file already exists
                    std::string filename = utils::createFullFilename(server_parameters.saved_video_folder, req_recording_filename, VSTR_RECORDING_VIDEO_EXTENSION);
                    if (utils::checkFileExists(filename)) {
                        response = std::to_string(VSTR_REC_ERR_VIDEO_ALREADY_EXISTS);
                        sendCode(fd, 400, response.c_str(), "application/json");
                        return;
                    }
                    else {
                        // try to init recording session
                        bool res = dv->init_recording(server_parameters.saved_video_folder,
                                                      req_recording_filename, response, ts_milli);
                        if (!res) {
                            sendCode(fd, 400, response.c_str(), "application/json");
                            return;
                        }
                        dv->is_recording_active = true;
                    }
                }
            }
            // if device to be stopped:
            else {
                // if device is already stopped
                dv->stop_recording();
            }
        }
        else {
            // http response with error
            response = std::to_string(VSTR_REC_ERR_DEVICE_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            return;
        }

        // http response

        sendCode(fd, 200, "", "application/json");
        return;

    }

	void ControlServer::sendHelpMessage(ugcs::vstreamer::sockets::Socket_handle& fd) {
		char buffer[BUFFER_SIZE] = { 0 };

		sprintf(buffer, CONTROL_HELP_MESSAGE);
		
		sendCode(fd, 200, buffer);
	}

    void ControlServer::sendParamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd) {
        /* message looks like
            {"autodetect": true}
        */
        std::string msg = "{\"autodetect\": ";
        msg += (server_parameters.autodetect ? "true" : "false");
        msg += "}";

        sendCode(fd, 200, msg.c_str(), "application/json");
        LOG_DEBUG("Command Server: REST GET ParametersInfo response %s", msg.c_str());
    }

    void ControlServer::writeParamsInfo(ugcs::vstreamer::sockets::Socket_handle& fd, std::string body) {

        // parse body
        // {"autodetect": true}
        Json::Reader jreader;
        Json::Value root;
        jreader.parse(body.c_str(), root, false);
        int autodetect = (int)(root.get("autodetect", 0).asBool());

        // write props to config
        auto props = ugcs::vsm::Properties::Get_instance();
        props->Set("vstreamer.videodevices.autodetect", autodetect);
        props->Store(std::cout);

        // http response
        std::string msg = "";
        sendCode(fd, 200, msg.c_str(), "application/json");
        return;

    }

    void ControlServer::startPlayback(ugcs::vstreamer::sockets::Socket_handle& fd, std::string query, int64_t ts_micro) {

        //parse query string
        //video_id=XXXX&speed=XX&pos=XXXXX
        std::stringstream stream_query(query);
        std::string item;
        std::string video_id="";
        int64_t pos=0;
        double speed=1;
        while (std::getline(stream_query, item, '&')) {
            std::size_t found = item.find("=");
            std::string key = item.substr(0,found);
            if (key == "video_id") {
                video_id = item.substr(found+1, item.length()-1);
            } else if (key == "pos") {
                pos = std::atol(item.substr(found+1, item.length()-1).c_str());
            } else if (key == "speed") {
                speed = std::atof(item.substr(found+1, item.length()-1).c_str());
            }
        }

        if (video_id.length() == 0) {
            std::string response = "Bad query parameters";
            sendCode(fd, 400, response.c_str(), "application/json");
            return;
        }

        // create fiilename
        std::string filename = utils::createFullFilename(server_parameters.saved_video_folder, video_id, VSTR_RECORDING_VIDEO_EXTENSION);



        // check if file exists
        if (!utils::checkFileExists(filename)) {
            std::string response = "File " + filename + " not found.";
            sendCode(fd, 400, response.c_str(), "application/json");
            return;
        }

        // video device for plaback (file)
        video_device *vd = new video_device(DEV_FILE);
        vd->url = filename;
        vd->name = filename;
        vd->timeout = 60;
        vd->playback_video_id = video_id;
        vd->playback_speed = speed;
        vd->playback_starting_pos = pos;
        vd->playback_request_ts = ts_micro;

        ffmpeg_playback *fp;
        fp = new ffmpeg_playback(vd);
        // start playback
        fp->start(fd);
    }

    void ControlServer::getVideoMetadata(ugcs::vstreamer::sockets::Socket_handle &fd, std::string video_id) {
        // first, search for devices with video_id recording active.
        // if found - return null for duration
        video_device *dv;
        for (auto iter = device_list.begin(); iter != device_list.end(); ++iter) {
            dv = &(iter->second);
            if (dv->recording_video_id == video_id && dv->is_recording_active) {
                std::string msg = "{ \"duration\": null } \r\n";
                sendCode(fd, 200, msg.c_str(), "application/json");
                LOG_DEBUG("Command Server: REST GET VideoInfo response %s", msg.c_str());
                return;
            }
        }

        // else search for video and metadata files.
        std::string filename = utils::createFullFilename(server_parameters.saved_video_folder, video_id, VSTR_RECORDING_VIDEO_EXTENSION);
        // check if file exists
        if (!utils::checkFileExists(filename)) {
            std::string response = std::to_string(VSTR_REC_ERR_VIDEO_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot find video file %s", filename.c_str());
            return;
        }
        // check metadata
        filename = filename + "." + VSTR_RECORDING_VIDEO_METADATA_EXTENSION;
        if (!utils::checkFileExists(filename)) {
            std::string response = std::to_string(VSTR_REC_ERR_METADATA_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot find metadata file %s", filename.c_str());
            return;
        }
        // open metadata
        std::fstream md_file;
        md_file.open(filename, std::ios::in);
        std::string duration;
        std::getline(md_file, duration);
        md_file.close();

        std::string msg = "{ \"duration\":" + duration + " } \r\n";

        sendCode(fd, 200, msg.c_str(), "application/json");
        LOG_DEBUG("Command Server: REST GET VideoInfo response %s", msg.c_str());

    }

    void ControlServer::deleteVideo(ugcs::vstreamer::sockets::Socket_handle &fd, std::string video_id) {
        // search for video and metadata files.
        std::string filename = utils::createFullFilename(server_parameters.saved_video_folder, video_id, VSTR_RECORDING_VIDEO_EXTENSION);
        // check if file exists
        if (!utils::checkFileExists(filename)) {
            std::string response = std::to_string(VSTR_REC_ERR_VIDEO_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot find video file %s", filename.c_str());
            return;
        }
        int res = std::remove(filename.c_str());

        if (res!=0) {
            std::string response = std::to_string(VSTR_REC_ERR_UNKNOWN);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot delete video file %s, error code: %d", filename.c_str(), errno);
            return;
        }

        // check metadata
        filename = filename + "." + VSTR_RECORDING_VIDEO_METADATA_EXTENSION;
        if (!utils::checkFileExists(filename)) {
            std::string response = std::to_string(VSTR_REC_ERR_METADATA_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot find metadata file %s", filename.c_str());
            return;
        }
        res = std::remove(filename.c_str());
        if (res!=0) {
            std::string response = std::to_string(VSTR_REC_ERR_UNKNOWN);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot delete metadata file %s, error code: %d", filename.c_str(), errno);
            return;
        }

        sendCode(fd, 200, "", "application/json");
        LOG_DEBUG("files %s are deleted.", filename.c_str());

    }

    void ControlServer::downloadVideo(ugcs::vstreamer::sockets::Socket_handle &fd, std::string video_id) {
        // search for video
        std::string filename = utils::createFullFilename(server_parameters.saved_video_folder, video_id, VSTR_RECORDING_VIDEO_EXTENSION);
        // check if file exists
        if (!utils::checkFileExists(filename)) {
            std::string response = std::to_string(VSTR_REC_ERR_VIDEO_NOT_FOUND);
            sendCode(fd, 400, response.c_str(), "application/json");
            LOG_ERROR("Command Server: Cannot find video file %s", filename.c_str());
            return;
        }
        LOG_DEBUG("Start to send %s.", filename.c_str());
        //get file size
        std::ifstream video_file;
        video_file.open(filename, std::ios::ate | std::ios::binary | std::ios::in);
        int64_t filesize = video_file.tellg();
        video_file.seekg(0, std::ios::beg);
        //send header
        char buffer_to_send_header[1000] = { 0 };
        std::string short_name = video_id + "." + VSTR_RECORDING_VIDEO_EXTENSION;
        sprintf(buffer_to_send_header, "HTTP/1.1 200 OK\r\n"
                "Server: vstreamer_server\r\n"
                "Transfer-Encoding: chunked\r\n"
                "Content-Type: binary/octet-stream \r\n"
                "Content-Disposition: attachment; filename=\"%s\"\r\n"
                "Content-Length: %" PRId64 "\r\n"
                "\r\n", short_name.c_str(), filesize);
        if (send(fd, buffer_to_send_header, strlen(buffer_to_send_header), 0) < 0) {
            LOG_ERROR("Error while downloading video (message header) %s.", filename.c_str());
            return;
        }
        // send chunks
        const char crlf[]       = { '\r', '\n' };
        const char last_chunk[] = { '0', '\r', '\n' };

        while (filesize>0) {
            long bytes_to_read = CHUNK_SIZE;
            if (bytes_to_read>filesize) {bytes_to_read = (int)filesize;}
            char buffer[CHUNK_SIZE+1000] = { 0 };
            char chunk_header_buffer[1000] = { 0 };
            video_file.read(buffer, bytes_to_read);
            // send chunk header
            sprintf(chunk_header_buffer, "%s\r\n", utils::long_to_hex_string(bytes_to_read).c_str());
            if (send(fd, chunk_header_buffer, strlen(chunk_header_buffer), 0) < 0) {
                LOG_ERROR("Error while downloading video (chunk header) %s.", filename.c_str());
                sockets::Close_socket(fd);
                return;
            }
            // send chunk with crlf
            if (send(fd, buffer, bytes_to_read, 0) < 0) {
                LOG_ERROR("Error while downloading video (chunk) %s.", filename.c_str());
                sockets::Close_socket(fd);
                return;
            }
            if (send(fd, crlf, 2, 0) < 0) {
                LOG_ERROR("Error while downloading video (chunk tail) %s.", filename.c_str());
                sockets::Close_socket(fd);
                return;
            }
            // remaining filesize to send
            filesize -= bytes_to_read;
        }
        // send final chunk
        if (send(fd, last_chunk, 3, 0) < 0) {
            LOG_ERROR("Error while downloading video (last chunk) %s.", filename.c_str());
            sockets::Close_socket(fd);
            return;
        }
        if (send(fd, crlf, 2, 0) < 0) {
            LOG_ERROR("Error while downloading video (tail) %s.", filename.c_str());
            sockets::Close_socket(fd);
            return;
        }
        sockets::Close_socket(fd);
        LOG_DEBUG("Finish to send %s.", filename.c_str());
    }


    void ControlServer::startSSDPListener() {

        discoverer = ugcs::vsm::Service_discovery_processor::Create();;

        proc_context = ugcs::vsm::Request_processor::Create("callback processor");
        proc_context->Enable();

        worker = ugcs::vsm::Request_worker::Create(
                "VSTREAMER processor worker",
                std::initializer_list<ugcs::vsm::Request_container::Ptr>({proc_context}));

        worker->Enable();
        discoverer->Enable();

        // detector
        // TODO: move it to method
        auto detector = [&](std::string type, std::string name, std::string loc, std::string id, bool)
        {
            LOG_DEBUG("SSDP Video stream found NT:%s, USN:%s, ID:%s, LOC:%s ", type.c_str(), name.c_str(), id.c_str(), loc.c_str());
            video_device ssdp_stream_video_device(DEV_STREAM);
            // create parameters string to initialize STREAM Device
            // as name + (id) + location + timeout:
            // DJI Android Video (79cb1eb7);rtsp://192.168.8.103:8083;60
            std::string stream_name = name + " (" + id + ")";
            std::string val = stream_name + ";" + loc + ";60";
            ssdp_stream_video_device.init_stream(val);

            int found_device_index = -1;
            //find video device with same name
            for(int i=0; i<server_parameters.input_streams.size(); i++){
                if (server_parameters.input_streams[i].name == stream_name) {
                    found_device_index = i;
                }
            }
            if (found_device_index >= 0) {
                // replace stream info if found
                server_parameters.input_streams[found_device_index] = ssdp_stream_video_device;
            } else {
                // add new stream info else
                server_parameters.input_streams.push_back(ssdp_stream_video_device);
            }

        };

        discoverer->Subscribe_for_service(SSDP_VIDEO_SERVICE_NT, ugcs::vsm::Service_discovery_processor::Make_detection_handler(detector), proc_context);
    }


    void ControlServer::stopSSDPListener() {
        if (discoverer != NULL) {
            discoverer->Disable();
        }
        if (proc_context != NULL) {
            proc_context->Disable();
        }
        if (worker != NULL) {
            worker->Disable();
        }
    }

}
}
