#pragma once

#include "Common.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
struct curl_slist;

namespace TinyDLP {

// HTTP response structure
struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string errorMessage;
    bool success;
    
    HttpResponse() : statusCode(0), success(false) {}
};

// HTTP request configuration
struct HttpRequest {
    std::string url;
    std::string method;  // GET, POST, PUT, DELETE
    std::map<std::string, std::string> headers;
    std::string body;
    int timeoutSeconds;
    bool verifySSL;
    std::string userAgent;
    
    HttpRequest() : method("GET"), timeoutSeconds(30), verifySSL(true), userAgent("TinyDLP/1.0") {}
};

// Progress callback function type
typedef std::function<bool(double downloadTotal, double downloaded, double uploadTotal, double uploaded)> HttpProgressCallback;

// HTTP Client class
class HttpClient {
private:
    void* curlHandle;  // CURL* handle
    std::string baseUrl;
    std::string apiKey;
    std::string userAgent;
    int defaultTimeout;
    bool verifySSL;
    
    // Internal helper methods
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static int ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    
    void SetupCurlOptions(const HttpRequest& request, HttpResponse& response, HttpProgressCallback progressCallback = nullptr);
    void CleanupCurlHandle();
    
public:
    // Constructor and destructor
    HttpClient(const std::string& baseUrl = "");
    ~HttpClient();
    
    // Copy constructor and assignment operator (disabled)
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    // Move constructor and assignment operator
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(HttpClient&& other) noexcept;
    
    // Configuration methods
    void SetBaseUrl(const std::string& url);
    void SetApiKey(const std::string& key);
    void SetUserAgent(const std::string& agent);
    void SetDefaultTimeout(int seconds);
    void SetSSLVerification(bool verify);
    
    // HTTP methods
    HttpResponse Get(const std::string& endpoint, const std::map<std::string, std::string>& headers = {});
    HttpResponse Post(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers = {});
    HttpResponse Put(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers = {});
    HttpResponse Delete(const std::string& endpoint, const std::map<std::string, std::string>& headers = {});
    
    // Generic request method
    HttpResponse Request(const HttpRequest& request, HttpProgressCallback progressCallback = nullptr);
    
    // Utility methods
    std::string UrlEncode(const std::string& str);
    std::string BuildQueryString(const std::map<std::string, std::string>& params);
    bool IsInitialized() const;
    
    // Static utility methods
    static std::string GetErrorMessage(int curlCode);
    static bool InitializeCurl();
    static void CleanupCurl();
};

// Configuration downloader class
class ConfigDownloader {
private:
    std::unique_ptr<HttpClient> httpClient;
    std::string serverUrl;
    std::string apiKey;
    std::string configPath;
    
public:
    ConfigDownloader(const std::string& serverUrl, const std::string& apiKey = "");
    
    // Configuration download methods
    bool DownloadConfig(const std::string& configName, const std::string& localPath);
    std::string GetConfigContent(const std::string& configName);
    std::vector<std::string> ListAvailableConfigs();
    
    // Server communication methods
    bool TestConnection();
    std::string GetServerVersion();
    bool UploadLogs(const std::string& logContent);
    
    // Configuration management
    bool SaveConfig(const std::string& configName, const std::string& content);
    bool DeleteConfig(const std::string& configName);
    
    // Utility methods
    void SetServerUrl(const std::string& url);
    void SetApiKey(const std::string& key);
    void SetConfigPath(const std::string& path);
};

} // namespace TinyDLP
