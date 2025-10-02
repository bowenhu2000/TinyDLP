# TinyDLP HTTP Client Implementation

This directory contains a comprehensive C++ HTTP client implementation based on the cURL library for TinyDLP.

## Overview

The HTTP client provides:
- **HttpClient**: Low-level HTTP client for REST API calls
- **ConfigDownloader**: High-level class for downloading configuration files from TinyDLP server
- **SSL/TLS Support**: Secure communication using OpenSSL
- **Progress Callbacks**: Real-time download/upload progress monitoring
- **Error Handling**: Comprehensive error reporting and logging

## Files

- `HttpClient.h` - Header file with class definitions
- `HttpClient.cpp` - Implementation of HTTP client functionality
- `HttpClientTest.cpp` - Test and example usage code

## Dependencies

### Required Libraries
1. **cURL Library** - HTTP client library
2. **OpenSSL** - SSL/TLS support for secure connections

### Build Prerequisites
- Visual Studio 2019 or later
- Perl (Strawberry Perl recommended)
- NASM (Netwide Assembler)
- CMake (for cURL build)

## Building Dependencies

### 1. Build OpenSSL
```powershell
# First build OpenSSL 3.5.0
.\build_openssl.ps1
```

### 2. Build cURL
```powershell
# Then build cURL with OpenSSL support
.\build_curl.ps1
```

## Integration with TinyDLP

### Project Configuration

Add to your Visual Studio project:

1. **Include Directories**:
   - `C:\Users\BowenGit\Documents\GitHub\TinyDLP\curl\x64\include`
   - `C:\Users\BowenGit\Documents\GitHub\TinyDLP\openssl350\x64\include`

2. **Library Directories**:
   - `C:\Users\BowenGit\Documents\GitHub\TinyDLP\curl\x64\lib`
   - `C:\Users\BowenGit\Documents\GitHub\TinyDLP\openssl350\x64\lib`

3. **Additional Dependencies**:
   - `libcurl.lib`
   - `libssl.lib`
   - `libcrypto.lib`
   - `ws2_32.lib`
   - `wldap32.lib`
   - `crypt32.lib`
   - `normaliz.lib`

### Usage Example

```cpp
#include "HttpClient.h"
using namespace TinyDLP;

// Initialize CURL
HttpClient::InitializeCurl();

// Create HTTP client
HttpClient client("https://api.tinydlp.com");

// Make GET request
auto response = client.Get("/api/status");
if (response.success) {
    std::cout << "Response: " << response.body << std::endl;
}

// Create config downloader
ConfigDownloader downloader("https://api.tinydlp.com", "your-api-key");

// Download configuration
if (downloader.DownloadConfig("settings.json", "C:\\TinyDLP\\configs\\settings.json")) {
    std::cout << "Config downloaded successfully" << std::endl;
}

// Cleanup
HttpClient::CleanupCurl();
```

## API Reference

### HttpClient Class

#### Constructor
```cpp
HttpClient(const std::string& baseUrl = "");
```

#### Configuration Methods
```cpp
void SetBaseUrl(const std::string& url);
void SetApiKey(const std::string& key);
void SetUserAgent(const std::string& agent);
void SetDefaultTimeout(int seconds);
void SetSSLVerification(bool verify);
```

#### HTTP Methods
```cpp
HttpResponse Get(const std::string& endpoint, const std::map<std::string, std::string>& headers = {});
HttpResponse Post(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers = {});
HttpResponse Put(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& headers = {});
HttpResponse Delete(const std::string& endpoint, const std::map<std::string, std::string>& headers = {});
HttpResponse Request(const HttpRequest& request, ProgressCallback progressCallback = nullptr);
```

### ConfigDownloader Class

#### Constructor
```cpp
ConfigDownloader(const std::string& serverUrl, const std::string& apiKey = "");
```

#### Configuration Management
```cpp
bool DownloadConfig(const std::string& configName, const std::string& localPath);
std::string GetConfigContent(const std::string& configName);
std::vector<std::string> ListAvailableConfigs();
bool SaveConfig(const std::string& configName, const std::string& content);
bool DeleteConfig(const std::string& configName);
```

#### Server Communication
```cpp
bool TestConnection();
std::string GetServerVersion();
bool UploadLogs(const std::string& logContent);
```

## Features

### HTTP Client Features
- **Multiple HTTP Methods**: GET, POST, PUT, DELETE
- **Custom Headers**: Add custom HTTP headers
- **Progress Monitoring**: Real-time download/upload progress
- **SSL/TLS Support**: Secure HTTPS connections
- **Timeout Control**: Configurable request timeouts
- **Error Handling**: Comprehensive error reporting
- **URL Encoding**: Automatic URL parameter encoding
- **Redirect Following**: Automatic redirect handling

### ConfigDownloader Features
- **Configuration Management**: Download, upload, delete configs
- **Server Communication**: Health checks, version info
- **Log Upload**: Send logs to server
- **API Key Authentication**: Bearer token authentication
- **Error Logging**: Integration with TinyDLP logging system

## Error Handling

The HTTP client provides comprehensive error handling:

```cpp
auto response = client.Get("/api/endpoint");
if (!response.success) {
    std::cout << "Error: " << response.errorMessage << std::endl;
    std::cout << "Status Code: " << response.statusCode << std::endl;
}
```

## Progress Monitoring

Monitor download/upload progress with callbacks:

```cpp
auto progressCallback = [](double downloadTotal, double downloaded, double uploadTotal, double uploaded) -> bool {
    if (downloadTotal > 0) {
        double progress = (downloaded / downloadTotal) * 100.0;
        std::cout << "Progress: " << progress << "%" << std::endl;
    }
    return true; // Continue operation
};

auto response = client.Request(request, progressCallback);
```

## Security

- **SSL/TLS Verification**: Enabled by default
- **Certificate Validation**: Validates server certificates
- **API Key Authentication**: Secure API access
- **HTTPS Support**: Encrypted communication

## Performance

- **Connection Reuse**: Efficient connection management
- **Static Linking**: No runtime dependencies
- **Memory Management**: RAII-based resource management
- **Thread Safety**: Safe for multi-threaded use

## Testing

Run the test program to verify functionality:

```cpp
// Compile and run HttpClientTest.cpp
// This will test all HTTP client functionality
```

## Troubleshooting

### Common Issues

1. **cURL not found**
   - Ensure cURL library is built and linked
   - Check include and library directories

2. **OpenSSL errors**
   - Verify OpenSSL is built correctly
   - Check OpenSSL library paths

3. **SSL certificate errors**
   - Check server certificate validity
   - Consider disabling SSL verification for testing

4. **Connection timeouts**
   - Increase timeout values
   - Check network connectivity

### Debug Information

Enable verbose logging in cURL:
```cpp
// Add to HttpRequest
request.headers["CURLOPT_VERBOSE"] = "1";
```

## Future Enhancements

- [ ] JSON parsing support
- [ ] OAuth 2.0 authentication
- [ ] Connection pooling
- [ ] Retry mechanisms
- [ ] Compression support
- [ ] WebSocket support

## License

This HTTP client implementation is part of the TinyDLP project and follows the same license terms.
