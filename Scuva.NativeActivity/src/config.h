// Scuva: config.h
#pragma once

// Include STL headers
#include <string>

// Declare macros
#define LOG_TAG "Scuva"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

// Declare Constants
// -----------------
const std::string BASE_PATH = "/mnt/sdcard/Scuva/files";
#define LOG_PATH "/_scuva_log.txt"

// Define Server/Client Addresses
#define SERVER_ADDRESS "192.168.0.3"			// Home WLAN
#define CLIENT_ADDRESS "192.168.0.5"
//#define SERVER_ADDRESS "192.168.55.1"
//#define CLIENT_ADDRESS "192.168.11.1"
//#define SERVER_ADDRESS "127.0.0.1"
//#define CLIENT_ADDRESS "127.0.0.1"

// FIN