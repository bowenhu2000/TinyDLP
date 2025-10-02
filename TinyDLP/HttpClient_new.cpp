#include "HttpClient.h"
#include "Logger.h"
#include <curl/curl.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
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

HttpClient::HttpClient(HttpClient&& other) noexcept
    : curlHandle(other.curlHandle), baseUrl(std::move(other.baseUrl)), 
      apiKey(std::move(other.apiKey)), userAgent(std::move(other.userAgent)),
      defaultTimeout(other.defaultTimeout), verifySSL(other.verifySSL) {
    other.curlHandle = nullptr;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        CleanupCurlHandle();
        curlHandle = other.curlHandle;
        baseUrl = std::move(other.baseUrl);
        apiKey = std::move(other.apiKey);
        userAgent = std::move(other.userAgent);
        defaultTimeout = other.defaultTimeout;
        verifySSL = other.verifySSL;
        other.curlHandle = nullptr;
    }
    return *this;
}

void HttpClient::CleanupCurlHandle() {
    if (curlHandle) {
        curl_easy_cleanup(static_cast<CURL*>(curlHandle));
        curlHandle = nullptr;
    }
}

void HttpClient::SetBaseUrl(const std::string& url) {
    baseUrl = url;
}

void HttpClient::SetApiKey(const std::string& key) {
    apiKey = key;
}

void HttpClient::SetUserAgent(const std::string& agent) {
    userAgent = agent;
}

void HttpClient::SetDefaultTimeout(int seconds) {
    defaultTimeout = seconds;
}

void HttpClient::SetSSLVerification(bool verify) {
    verifySSL = verify;
}

bool HttpClient::IsInitialized() const {
    return curlHandle != nullptr;
}

// Static callback functions
size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t HttpClient::HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string header(static_cast<char*>(contents), totalSize);
    
    // Remove trailing newline characters
    while (!header.empty() && (header.back() == '\r' || header.back() == '\n')) {
        header.pop_back();
    }
    
    if (!header.empty()) {
        auto colonPos = header.find(':');
        if (colonPos != std::string::npos) {
            std::string key = header.substr(0, colonPos);
            std::string value = header.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Convert to lowercase for consistency
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            std::map<std::string, std::string>* headers = static_cast<std::map<std::string, std::string>*>(userp);
            (*headers)[key] = value;
        }
    }
    
    return totalSize;
}

int HttpClient::ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    HttpProgressCallback* callback = static_cast<HttpProgressCallback*>(clientp);
    if (callback && *callback) {
        return (*callback)(dltotal, dlnow, ultotal, ulnow) ? 0 : 1;
    }
    return 0;
}

void HttpClient::SetupCurlOptions(const HttpRequest& request, HttpResponse& response, HttpProgressCallback progressCallback) {
    CURL* curl = static_cast<CURL*>(curlHandle);
    if (!curl) return;
    
    // Reset curl handle
    curl_easy_reset(curl);
    
    // Set URL
    std::string fullUrl = baseUrl.empty() ? request.url : baseUrl + request.url;
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    
    // Set HTTP method
    if (request.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!request.body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.body.length());
        }
    } else if (request.method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (!request.body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.body.length());
        }
    } else if (request.method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    // Set headers
    struct curl_slist* headerList = nullptr;
    for (const auto& header : request.headers) {
        std::string headerStr = header.first + ": " + header.second;
        headerList = curl_slist_append(headerList, headerStr.c_str());
    }
    
    // Add API key if available
    if (!apiKey.empty()) {
        std::string authHeader = "Authorization: Bearer " + apiKey;
        headerList = curl_slist_append(headerList, authHeader.c_str());
    }
    
    // Add user agent
    std::string userAgentStr = !request.userAgent.empty() ? request.userAgent : userAgent;
    if (!userAgentStr.empty()) {
        std::string uaHeader = "User-Agent: " + userAgentStr;
        headerList = curl_slist_append(headerList, uaHeader.c_str());
    }
    
    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    
    // Set callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
    
    // Set progress callback if provided
    if (progressCallback) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progressCallback);
    } else {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    }
    
    // Set timeout
    int timeout = request.timeoutSeconds > 0 ? request.timeoutSeconds : defaultTimeout;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    
    // Set SSL options
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, request.verifySSL ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request.verifySSL ? 2L : 0L);
    
    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    
    // Clean up headers after request
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
}

HttpResponse HttpClient::Request(const HttpRequest& request, HttpProgressCallback progressCallback) {
    HttpResponse response;
    
    if (!curlHandle) {
        response.errorMessage = "CURL handle not initialized";
        return response;
    }
    
    SetupCurlOptions(request, response, progressCallback);
    
    CURL* curl = static_cast<CURL*>(curlHandle);
    CURLcode res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
        long statusCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        response.statusCode = static_cast<int>(statusCode);
        response.success = (statusCode >= 200 && statusCode < 300);
    } else {
        response.errorMessage = GetErrorMessage(res);
        response.success = false;
    }
    
    return response;
}

HttpResponse HttpClient::Get(const std::string& endpoint, const std::map<std::string, std::string>& headers) {
    HttpRequest request;
    request.url = endpoint;
    request.method = "GET";
    request.headers = headers;
    return Request(request);
}

HttpResponse HttpClient::Post(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers) {
    HttpRequest request;
    request.url = endpoint;
    request.method = "POST";
    request.body = body;
    request.headers = headers;
    if (!body.empty() && headers.find("Content-Type") == headers.end()) {
        request.headers["Content-Type"] = "application/json";
    }
    return Request(request);
}

HttpResponse HttpClient::Put(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers) {
    HttpRequest request;
    request.url = endpoint;
    request.method = "PUT";
    request.body = body;
    request.headers = headers;
    if (!body.empty() && headers.find("Content-Type") == headers.end()) {
        request.headers["Content-Type"] = "application/json";
    }
    return Request(request);
}

HttpResponse HttpClient::Delete(const std::string& endpoint, const std::map<std::string, std::string>& headers) {
    HttpRequest request;
    request.url = endpoint;
    request.method = "DELETE";
    request.headers = headers;
    return Request(request);
}

std::string HttpClient::UrlEncode(const std::string& str) {
    CURL* curl = static_cast<CURL*>(curlHandle);
    if (!curl) return str;
    
    char* encoded = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
    if (encoded) {
        std::string result(encoded);
        curl_free(encoded);
        return result;
    }
    return str;
}

std::string HttpClient::BuildQueryString(const std::map<std::string, std::string>& params) {
    std::stringstream query;
    bool first = true;
    
    for (const auto& param : params) {
        if (!first) query << "&";
        query << UrlEncode(param.first) << "=" << UrlEncode(param.second);
        first = false;
    }
    
    return query.str();
}

std::string HttpClient::GetErrorMessage(int curlCode) {
    return curl_easy_strerror(static_cast<CURLcode>(curlCode));
}

bool HttpClient::InitializeCurl() {
    if (!g_curlInitialized) {
        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        g_curlInitialized = (res == CURLE_OK);
        if (!g_curlInitialized) {
            Logger::Log(LOG_ERROR, L"Failed to initialize CURL global state");
        }
    }
    return g_curlInitialized;
}

void HttpClient::CleanupCurl() {
    if (g_curlInitialized) {
        curl_global_cleanup();
        g_curlInitialized = false;
    }
}

// ConfigDownloader Implementation

ConfigDownloader::ConfigDownloader(const std::string& serverUrl, const std::string& apiKey)
    : serverUrl(serverUrl), apiKey(apiKey), configPath("configs") {
    httpClient = std::make_unique<HttpClient>(serverUrl);
    httpClient->SetApiKey(apiKey);
    httpClient->SetUserAgent("TinyDLP-Client/1.0");
}

bool ConfigDownloader::DownloadConfig(const std::string& configName, const std::string& localPath) {
    std::string endpoint = "/api/configs/" + configName;
    auto response = httpClient->Get(endpoint);
    
    if (response.success) {
        std::ofstream file(localPath, std::ios::binary);
        if (file.is_open()) {
            file.write(response.body.c_str(), response.body.length());
            file.close();
            Logger::Log(LOG_INFO, L"Config downloaded successfully: " + std::wstring(configName.begin(), configName.end()));
            return true;
        } else {
            Logger::Log(LOG_ERROR, L"Failed to write config file: " + std::wstring(localPath.begin(), localPath.end()));
        }
    } else {
        Logger::Log(LOG_ERROR, L"Failed to download config: " + std::wstring(configName.begin(), configName.end()) + L" - " + std::wstring(response.errorMessage.begin(), response.errorMessage.end()));
    }
    
    return false;
}

std::string ConfigDownloader::GetConfigContent(const std::string& configName) {
    std::string endpoint = "/api/configs/" + configName;
    auto response = httpClient->Get(endpoint);
    
    if (response.success) {
        return response.body;
    } else {
        Logger::Log(LOG_ERROR, L"Failed to get config content: " + std::wstring(configName.begin(), configName.end()));
        return "";
    }
}

std::vector<std::string> ConfigDownloader::ListAvailableConfigs() {
    std::vector<std::string> configs;
    std::string endpoint = "/api/configs";
    auto response = httpClient->Get(endpoint);
    
    if (response.success) {
        // Parse JSON response (simplified - in real implementation, use a JSON library)
        // For now, assume response is a simple list of config names separated by newlines
        std::istringstream stream(response.body);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                configs.push_back(line);
            }
        }
    } else {
        Logger::Log(LOG_ERROR, L"Failed to list available configs");
    }
    
    return configs;
}

bool ConfigDownloader::TestConnection() {
    std::string endpoint = "/api/health";
    auto response = httpClient->Get(endpoint);
    return response.success;
}

std::string ConfigDownloader::GetServerVersion() {
    std::string endpoint = "/api/version";
    auto response = httpClient->Get(endpoint);
    
    if (response.success) {
        return response.body;
    }
    return "";
}

bool ConfigDownloader::UploadLogs(const std::string& logContent) {
    std::string endpoint = "/api/logs";
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    
    auto response = httpClient->Post(endpoint, logContent, headers);
    return response.success;
}

bool ConfigDownloader::SaveConfig(const std::string& configName, const std::string& content) {
    std::string endpoint = "/api/configs/" + configName;
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    
    auto response = httpClient->Put(endpoint, content, headers);
    return response.success;
}

bool ConfigDownloader::DeleteConfig(const std::string& configName) {
    std::string endpoint = "/api/configs/" + configName;
    auto response = httpClient->Delete(endpoint);
    return response.success;
}

void ConfigDownloader::SetServerUrl(const std::string& url) {
    serverUrl = url;
    httpClient->SetBaseUrl(url);
}

void ConfigDownloader::SetApiKey(const std::string& key) {
    apiKey = key;
    httpClient->SetApiKey(key);
}

void ConfigDownloader::SetConfigPath(const std::string& path) {
    configPath = path;
}

} // namespace TinyDLP
