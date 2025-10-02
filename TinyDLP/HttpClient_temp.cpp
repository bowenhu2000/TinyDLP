#include "HttpClient.h"
#include "Logger.h"
#include <curl/curl.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace TinyDLP {

// Static initialization
static bool g_curlInitialized = false;

// HttpClient Implementation

HttpClient::HttpClient(const std::string& baseUrl) 
    : curlHandle(nullptr), baseUrl(baseUrl), defaultTimeout(30), verifySSL(true), userAgent("TinyDLP/1.0") {
    if (!g_curlInitialized) {
        InitializeCurl();
    }
    
    curlHandle = curl_easy_init();
    if (!curlHandle) {
        Logger::Log(LOG_ERROR, L"Failed to initialize CURL handle");
    }
}

HttpClient::~HttpClient() {
    CleanupCurlHandle();
}
