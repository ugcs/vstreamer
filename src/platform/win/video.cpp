// Copyright (c) 2014, Smart Projects Holdings Ltd
// All rights reserved.
// See LICENSE file for license details.

/*
* video.cpp
*
*
*  Windows specific part of video tasks impleme//ntation
*/

// This file should be built only on windows platforms
#if _WIN32

#include <ugcs/vstreamer/video.h>

int ugcs::vstreamer::video::Get_autodetected_device_list(std::vector<std::string> &found_devices) {



    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    int device_counter = 0;

    HRESULT hr = CoInitialize(NULL);

    hr = CoCreateInstance(CLSID_VideoInputDeviceCategory, 0,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
        reinterpret_cast<void**>(&pDevEnum));

    if (SUCCEEDED(hr)) {
        // Create an enumerator for the video capture category.
        hr = pDevEnum->CreateClassEnumerator(
            CLSID_VideoInputDeviceCategory,
            &pEnum, 0);

        if (hr == S_OK) {

            IMoniker *pMoniker = NULL;

            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {

                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
                    (void**)(&pPropBag));

                if (FAILED(hr)) {
                    pMoniker->Release();
                    continue;  // Skip this one, maybe the next one will work.
                }

                // Find the description or friendly name.
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"Description", &varName, 0);

                if (FAILED(hr)) hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                if (SUCCEEDED(hr)) {

                    hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                    int count = 0;
                    static char device_names[255];
                    int maxLen = sizeof(device_names) / sizeof(device_names[0]) - 2;
                    while (varName.bstrVal[count] != 0x00 && count < maxLen) {
                        device_names[count] = (char)varName.bstrVal[count];
                        count++;
                    }
                    device_names[count] = 0;
                    std::string utf8_device_name = convert_ansi_to_utf8(device_names);

                    found_devices.push_back(utf8_device_name);
                    device_counter++;
                }

                pPropBag->Release();
                pPropBag = NULL;

                pMoniker->Release();
                pMoniker = NULL;

            }

            pDevEnum->Release();
            pDevEnum = NULL;

            pEnum->Release();
            pEnum = NULL;
        }
    }
    CoUninitialize();


	return device_counter;
}


std::string ugcs::vstreamer::video::convert_ansi_to_utf8(std::string ansi_str) {

    int wchars_size =  MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str() , -1, NULL , 0 );
    wchar_t* wstr = new wchar_t[wchars_size];
    MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str() , -1, wstr , wchars_size );

    int unt8_size  = WideCharToMultiByte(CP_UTF8, 0, wstr, wchars_size, NULL,  0, NULL, NULL);
    char* utf8_str = new char[unt8_size];
    WideCharToMultiByte(CP_UTF8, 0, wstr, wchars_size, utf8_str, unt8_size, NULL, NULL);

    std::string ret_str = utf8_str;

    return ret_str;

}


#endif // _WIN32
