#pragma once

#include "Common.h"
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

class DLLInjector {
private:
    static std::wstring hookDllPath;
    static std::vector<DWORD> injectedProcesses;
    static std::mutex injectorMutex;
    static bool isMonitoring;

public:
    static bool Initialize();
    static void Shutdown();
    static void StartProcessMonitoring();
    static void StopProcessMonitoring();
    static bool InjectDLL(DWORD processId);
    static void UninjectDLL(DWORD processId);
    static bool IsProcessInjected(DWORD processId);
    static bool IsProcessInjectedUnsafe(DWORD processId);
    
private:
    static void ProcessMonitorThread();
    static void InjectIntoExistingProcesses();
    static bool ShouldInjectProcess(const std::wstring& processName);
    static std::vector<DWORD> GetRunningProcesses();
    static std::wstring GetProcessName(DWORD processId);
    static bool InjectDLLIntoProcess(DWORD processId, const std::wstring& dllPath);
    static void OnProcessCreated(DWORD processId);
    static void OnProcessTerminated(DWORD processId);
};
