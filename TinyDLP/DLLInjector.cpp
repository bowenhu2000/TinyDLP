#include "DLLInjector.h"
#include "Logger.h"
#include <algorithm>

// Static member definitions
std::wstring DLLInjector::hookDllPath;
std::vector<DWORD> DLLInjector::injectedProcesses;
std::mutex DLLInjector::injectorMutex;
bool DLLInjector::isMonitoring = false;
std::thread g_processMonitorThread;

bool DLLInjector::Initialize() {
    std::lock_guard<std::mutex> lock(injectorMutex);
    
    // Get the path to our hook DLL - look in the correct location
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    std::wstring exePath(currentPath);
    
    // Find the directory containing the executable
    size_t lastSlash = exePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        std::wstring exeDir = exePath.substr(0, lastSlash);
        // Try multiple possible locations for the DLL
        std::vector<std::wstring> possiblePaths = {
            exeDir + L"\\TinyDLP_Hook\\x64\\Debug\\TinyDLP_Hook.dll",
            exeDir + L"\\TinyDLP_Hook\\x64\\Release\\TinyDLP_Hook.dll",
            exeDir + L"\\TinyDLP_Hook\\TinyDLP_Hook.dll",
            exeDir + L"\\TinyDLP_Hook.dll"
        };
        
        for (const auto& path : possiblePaths) {
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
                hookDllPath = path;
                break;
            }
        }
        
        if (hookDllPath.empty()) {
            hookDllPath = exeDir + L"\\TinyDLP_Hook\\x64\\Debug\\TinyDLP_Hook.dll"; // Default path
        }
    } else {
        hookDllPath = L"TinyDLP_Hook.dll";
    }
    
    // Check if the DLL exists
    if (GetFileAttributesW(hookDllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Logger::Log(LOG_ERROR, L"Hook DLL not found: " + hookDllPath);
        Logger::Log(LOG_ERROR, L"Please build the TinyDLP_Hook project first");
        return false;
    }
    
    Logger::Log(LOG_INFO, L"DLL Injector initialized. Hook DLL: " + hookDllPath);
    return true;
}

void DLLInjector::Shutdown() {
    StopProcessMonitoring();
    
    std::lock_guard<std::mutex> lock(injectorMutex);
    
    // Uninject from all processes
    for (DWORD processId : injectedProcesses) {
        UninjectDLL(processId);
    }
    injectedProcesses.clear();
    
    Logger::Log(LOG_INFO, L"DLL Injector shutdown");
}

void DLLInjector::StartProcessMonitoring() {
    std::lock_guard<std::mutex> lock(injectorMutex);
    
    if (isMonitoring) {
        return;
    }
    
    isMonitoring = true;
    
    try {
        g_processMonitorThread = std::thread(ProcessMonitorThread);
        Logger::Log(LOG_INFO, L"Process monitoring started");
        
        // Immediately inject into existing target processes
        InjectIntoExistingProcesses();
        
    } catch (const std::exception&) {
        isMonitoring = false;
        Logger::Log(LOG_ERROR, L"Failed to start process monitoring thread");
    }
}

void DLLInjector::StopProcessMonitoring() {
    {
        std::lock_guard<std::mutex> lock(injectorMutex);
        if (!isMonitoring) {
            return;
        }
        isMonitoring = false;
    }
    
    if (g_processMonitorThread.joinable()) {
        g_processMonitorThread.join();
    }
    
    Logger::Log(LOG_INFO, L"Process monitoring stopped");
}

void DLLInjector::InjectIntoExistingProcesses() {
    Logger::Log(LOG_INFO, L"Injecting into existing target processes...");
    
    std::vector<DWORD> currentProcesses = GetRunningProcesses();
    int injectedCount = 0;
    
    for (DWORD processId : currentProcesses) {
        std::wstring processName = GetProcessName(processId);
        if (ShouldInjectProcess(processName)) {
            Logger::Log(LOG_INFO, L"Injecting to " + processName);
            if (InjectDLL(processId)) {
                injectedCount++;
            }
        }
    }
    
    Logger::Log(LOG_INFO, L"Injected into " + std::to_wstring(injectedCount) + L" existing processes");
}

void DLLInjector::ProcessMonitorThread() {
    Logger::Log(LOG_INFO, L"Process monitor thread started");
    
    // Get initial list of processes
    std::vector<DWORD> currentProcesses = GetRunningProcesses();
    std::vector<DWORD> previousProcesses;
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(injectorMutex);
            if (!isMonitoring) {
                break;
            }
        }
        
        // Get current process list
        currentProcesses = GetRunningProcesses();
        
        // Find new processes - use index-based loop to avoid iterator issues
        for (size_t i = 0; i < currentProcesses.size(); ++i) {
            DWORD processId = currentProcesses[i];
            bool found = false;
            
            // Check if this process was in the previous list
            for (size_t j = 0; j < previousProcesses.size(); ++j) {
                if (previousProcesses[j] == processId) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                OnProcessCreated(processId);
            }
        }
        
        // Find terminated processes - use index-based loop to avoid iterator issues
        for (size_t i = 0; i < previousProcesses.size(); ++i) {
            DWORD processId = previousProcesses[i];
            bool found = false;
            
            // Check if this process is still in the current list
            for (size_t j = 0; j < currentProcesses.size(); ++j) {
                if (currentProcesses[j] == processId) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                OnProcessTerminated(processId);
            }
        }
        
        // Copy current to previous for next iteration
        previousProcesses = currentProcesses;
        
        // Sleep for a bit before checking again
        Sleep(2000); // Check every 2 seconds
    }
    
    Logger::Log(LOG_INFO, L"Process monitor thread ended");
}

bool DLLInjector::InjectDLL(DWORD processId) {
    //std::lock_guard<std::mutex> lock(injectorMutex);
    
    if (IsProcessInjected(processId)) {
        return true; // Already injected
    }
    
    std::wstring processName = GetProcessName(processId);
    
    // Check if we should inject into this process
    if (!ShouldInjectProcess(processName)) {
        return false;
    }
    
    if (InjectDLLIntoProcess(processId, hookDllPath)) {
        injectedProcesses.push_back(processId);
        // Enhanced logging for successful DLL injection
        Logger::Log(LOG_INFO, L"=== DLL Injection Successful ===");
        Logger::Log(LOG_INFO, L"Target Process: " + processName);
        Logger::Log(LOG_INFO, L"Process ID: " + std::to_wstring(processId));
        Logger::Log(LOG_INFO, L"DLL Path: " + hookDllPath);
        Logger::Log(LOG_INFO, L"Process is now protected against PDF saves to USB drives");
        return true;
    } else {
        Logger::Log(LOG_ERROR, L"Failed to inject DLL into process: " + processName + L" (PID: " + std::to_wstring(processId) + L")");
        return false;
    }
}

void DLLInjector::UninjectDLL(DWORD processId) {
    std::lock_guard<std::mutex> lock(injectorMutex);
    
    auto it = std::find(injectedProcesses.begin(), injectedProcesses.end(), processId);
    if (it != injectedProcesses.end()) {
        injectedProcesses.erase(it);
        Logger::Log(LOG_INFO, L"Uninjected DLL from process (PID: " + std::to_wstring(processId) + L")");
    }
}

bool DLLInjector::IsProcessInjected(DWORD processId) {
    //std::lock_guard<std::mutex> lock(injectorMutex);
    return std::find(injectedProcesses.begin(), injectedProcesses.end(), processId) != injectedProcesses.end();
}

std::vector<DWORD> DLLInjector::GetRunningProcesses() {
    std::vector<DWORD> processes;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            processes.push_back(pe32.th32ProcessID);
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return processes;
}

std::wstring DLLInjector::GetProcessName(DWORD processId) {
    std::wstring processName = L"Unknown Process";
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processNameBuffer[MAX_PATH];
        DWORD size = MAX_PATH;
        
        if (QueryFullProcessImageNameW(hProcess, 0, processNameBuffer, &size)) {
            std::wstring fullPath(processNameBuffer);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                processName = fullPath.substr(lastSlash + 1);
            } else {
                processName = fullPath;
            }
        }
        
        CloseHandle(hProcess);
    }
    
    return processName;
}

bool DLLInjector::ShouldInjectProcess(const std::wstring& processName) {
    // List of processes that might save PDF files
    std::vector<std::wstring> targetProcesses = {
        L"cmd.exe"      // Windows Explorer - PRIMARY TARGET
       // L"explorer.exe"      // Windows Explorer - PRIMARY TARGET
       // L"winword.exe",       // Microsoft Word
       // L"excel.exe",         // Microsoft Excel
       // L"powerpnt.exe",      // Microsoft PowerPoint
       // L"acrord32.exe",      // Adobe Acrobat Reader
      //  L"acrobat.exe",       // Adobe Acrobat
       // L"chrome.exe",        // Google Chrome
       // L"firefox.exe",       // Mozilla Firefox
       // L"msedge.exe",        // Microsoft Edge
       // L"notepad.exe",       // Notepad
       // L"notepad++.exe",     // Notepad++
       // L"code.exe",          // Visual Studio Code
       // L"devenv.exe"         // Visual Studio
    };
    
    // Convert to lowercase for comparison
    std::wstring lowerProcessName = processName;
    std::transform(lowerProcessName.begin(), lowerProcessName.end(), lowerProcessName.begin(), ::towlower);
    
    for (const auto& target : targetProcesses) {
        std::wstring lowerTarget = target;
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::towlower);
        
        if (lowerProcessName == lowerTarget) {
            return true;
        }
    }
    
    return false;
}

bool DLLInjector::InjectDLLIntoProcess(DWORD processId, const std::wstring& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        DWORD error = GetLastError();
        Logger::Log(LOG_ERROR, L"Failed to open process " + std::to_wstring(processId) + L". Error: " + std::to_wstring(error));
        return false;
    }
    
    // Allocate memory in the target process
    SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        CloseHandle(hProcess);
        Logger::Log(LOG_ERROR, L"Failed to allocate memory in target process");
        return false;
    }
    
    // Write the DLL path to the allocated memory
    if (!WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), pathSize, NULL)) {
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        Logger::Log(LOG_ERROR, L"Failed to write DLL path to target process");
        return false;
    }
    
    // Get the address of LoadLibraryW
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    
    // Create a remote thread to load the DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, pDllPath, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        Logger::Log(LOG_ERROR, L"Failed to create remote thread in target process");
        return false;
    }
    
    // Wait for the thread to complete
    WaitForSingleObject(hThread, INFINITE);
    
    // Get the exit code (module handle)
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    // Clean up
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    bool success = (exitCode != 0); // Success if LoadLibraryW returned a non-zero handle
    if (!success) {
        Logger::Log(LOG_ERROR, L"LoadLibraryW failed in target process. Exit code: " + std::to_wstring(exitCode));
    }
    
    return success;
}

void DLLInjector::OnProcessCreated(DWORD processId) {
    // Try to inject DLL into the new process
    InjectDLL(processId);
}

void DLLInjector::OnProcessTerminated(DWORD processId) {
    // Remove from our injected processes list
    UninjectDLL(processId);
}
