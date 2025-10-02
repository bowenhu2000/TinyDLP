#include "HttpClient.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace TinyDLP;

// Example usage of HttpClient
void TestHttpClient() {
    std::cout << "=== Testing HttpClient ===" << std::endl;
    
    // Initialize CURL
    if (!HttpClient::InitializeCurl()) {
        std::cout << "Failed to initialize CURL" << std::endl;
        return;
    }
    
    // Create HTTP client
    HttpClient client("https://httpbin.org");
    
    // Test GET request
    std::cout << "\n1. Testing GET request..." << std::endl;
    auto response = client.Get("/get");
    std::cout << "Status: " << response.statusCode << std::endl;
    std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
    if (!response.success) {
        std::cout << "Error: " << response.errorMessage << std::endl;
    }
    
    // Test POST request
    std::cout << "\n2. Testing POST request..." << std::endl;
    std::string jsonData = "{\"name\": \"TinyDLP\", \"version\": \"1.0\"}";
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    
    response = client.Post("/post", jsonData, headers);
    std::cout << "Status: " << response.statusCode << std::endl;
    std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
    
    // Test with progress callback
    std::cout << "\n3. Testing with progress callback..." << std::endl;
    auto progressCallback = [](double downloadTotal, double downloaded, double uploadTotal, double uploaded) -> bool {
        if (downloadTotal > 0) {
            double progress = (downloaded / downloadTotal) * 100.0;
            std::cout << "Download progress: " << progress << "%" << std::endl;
        }
        return true; // Continue download
    };
    
    HttpRequest request;
    request.url = "https://httpbin.org/delay/2";
    request.method = "GET";
    request.timeoutSeconds = 10;
    
    response = client.Request(request, progressCallback);
    std::cout << "Status: " << response.statusCode << std::endl;
    std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
    
    // Cleanup
    HttpClient::CleanupCurl();
}

// Example usage of ConfigDownloader
void TestConfigDownloader() {
    std::cout << "\n=== Testing ConfigDownloader ===" << std::endl;
    
    // Initialize CURL
    if (!HttpClient::InitializeCurl()) {
        std::cout << "Failed to initialize CURL" << std::endl;
        return;
    }
    
    // Create config downloader (using a test server)
    ConfigDownloader downloader("https://httpbin.org", "test-api-key");
    
    // Test connection
    std::cout << "\n1. Testing connection..." << std::endl;
    bool connected = downloader.TestConnection();
    std::cout << "Connection: " << (connected ? "Success" : "Failed") << std::endl;
    
    // Test getting server version
    std::cout << "\n2. Getting server version..." << std::endl;
    std::string version = downloader.GetServerVersion();
    std::cout << "Server version: " << version << std::endl;
    
    // Test downloading config
    std::cout << "\n3. Downloading config..." << std::endl;
    bool downloaded = downloader.DownloadConfig("test-config", "test-config.json");
    std::cout << "Config download: " << (downloaded ? "Success" : "Failed") << std::endl;
    
    // Test getting config content
    std::cout << "\n4. Getting config content..." << std::endl;
    std::string content = downloader.GetConfigContent("test-config");
    std::cout << "Config content length: " << content.length() << " bytes" << std::endl;
    
    // Test uploading logs
    std::cout << "\n5. Uploading logs..." << std::endl;
    std::string logData = "{\"timestamp\": \"2025-01-01T00:00:00Z\", \"level\": \"INFO\", \"message\": \"Test log entry\"}";
    bool uploaded = downloader.UploadLogs(logData);
    std::cout << "Log upload: " << (uploaded ? "Success" : "Failed") << std::endl;
    
    // Cleanup
    HttpClient::CleanupCurl();
}

// Example of integrating with TinyDLP
void TinyDLPIntegrationExample() {
    std::cout << "\n=== TinyDLP Integration Example ===" << std::endl;
    
    // Initialize CURL
    if (!HttpClient::InitializeCurl()) {
        Logger::Log(LOG_ERROR, L"Failed to initialize CURL for HTTP client");
        return;
    }
    
    // Create config downloader for TinyDLP server
    ConfigDownloader downloader("https://tinydlp-server.example.com", "your-api-key-here");
    
    // Set config path
    downloader.SetConfigPath("C:\\TinyDLP\\configs");
    
    // Test connection to TinyDLP server
    if (downloader.TestConnection()) {
        Logger::Log(LOG_INFO, L"Connected to TinyDLP server successfully");
        
        // Download configuration files
        std::vector<std::string> configs = downloader.ListAvailableConfigs();
        Logger::Log(LOG_INFO, L"Found " + std::to_wstring(configs.size()) + L" configuration files");
        
        for (const auto& configName : configs) {
            std::string localPath = "C:\\TinyDLP\\configs\\" + configName;
            if (downloader.DownloadConfig(configName, localPath)) {
                Logger::Log(LOG_INFO, L"Downloaded config: " + std::wstring(configName.begin(), configName.end()));
            } else {
                Logger::Log(LOG_ERROR, L"Failed to download config: " + std::wstring(configName.begin(), configName.end()));
            }
        }
        
        // Upload current logs to server
        std::string logContent = "{\"logs\": [\"Log entry 1\", \"Log entry 2\"]}";
        if (downloader.UploadLogs(logContent)) {
            Logger::Log(LOG_INFO, L"Logs uploaded to server successfully");
        } else {
            Logger::Log(LOG_WARNING, L"Failed to upload logs to server");
        }
        
    } else {
        Logger::Log(LOG_ERROR, L"Failed to connect to TinyDLP server");
    }
    
    // Cleanup
    HttpClient::CleanupCurl();
}

// Main function for testing
int main() {
    std::cout << "TinyDLP HTTP Client Test" << std::endl;
    std::cout << "========================" << std::endl;
    
    try {
        TestHttpClient();
        TestConfigDownloader();
        TinyDLPIntegrationExample();
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    std::cout << "\nTest completed!" << std::endl;
    return 0;
}
