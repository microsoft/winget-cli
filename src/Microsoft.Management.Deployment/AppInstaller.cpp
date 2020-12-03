#include "pch.h"
#include "AppInstaller.h"
#include "AppInstaller.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void StartAppInstallerCli(std::wstring command)
    {
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&processInfo, sizeof(processInfo));

        // Start the child process. 
        winrt::check_bool(CreateProcess(NULL,   // use command line
            (LPWSTR)command.c_str(),        // Command line
            NULL,           // Process handle is not inheritable
            NULL,           // Thread handle is not inheritable
            FALSE,          // Set handle inheritance FALSE
            0,              // No creation flags
            NULL,           // Use parent environment block
            NULL,           // Use parent starting directory 
            &startupInfo,
            &processInfo)
        );

        // Wait for child process exit.
        if (WaitForSingleObject(processInfo.hProcess, INFINITE) == WAIT_FAILED)
        {
            throw_last_error();
        }

        // Close thread and process handles. 
        winrt::check_bool(CloseHandle(processInfo.hProcess));
        winrt::check_bool(CloseHandle(processInfo.hThread));
    }

    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        co_await winrt::resume_background();

        auto progress{ co_await winrt::get_progress_token() };

        InstallProgress queuedProgress{ InstallProgressState::Queued, 0, 0, 0 };
        progress(queuedProgress);

        std::wstring installCommand{ L"winget install " };
        std::wstring installParameters{ L"" };
        auto uniqueId = options.Id();
        auto packageId = uniqueId.Id;
        if (packageId.size() > 0)
        {
            installParameters = L"--id " + packageId;
        }
        else if (options.Manifest().size() > 0)
        {
            installParameters = L"--manifest " + options.Manifest();
        }

        if (installParameters.length() == 0)
        {
            throw hresult_invalid_argument();
        }

        StartAppInstallerCli(installCommand + installParameters);

        // Sample progress
        InstallProgress beginDownloadProgress{ InstallProgressState::Download, 0, 0, 0 };
        progress(beginDownloadProgress);

        InstallProgress endDownloadProgress{ InstallProgressState::Download, 0, 0, 100 };
        progress(endDownloadProgress);

        // Sample result
        InstallResult installResult(L"", L"", false);
        co_return installResult;
    }
}
