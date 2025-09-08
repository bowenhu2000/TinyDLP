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
