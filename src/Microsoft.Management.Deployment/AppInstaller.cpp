#include "pch.h"
#include "AppInstaller.h"
#include "AppInstaller.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        co_await winrt::resume_background();

        auto progress{ co_await winrt::get_progress_token() };
        auto cancellationToken{ co_await winrt::get_cancellation_token() };

        InstallProgress queuedProgress{ InstallProgressState::Queued, 0, 0, 0 };
        progress(queuedProgress);

        std::wstring installCommand{ L"wingetdev install --exact" };
        std::wstring installParameters{ L"" };
        auto uniqueId = options.Id();
        auto packageId = uniqueId.Id;
        if (packageId.size() > 0)
        {
            installParameters += L" --id " + packageId;
        }
        if (uniqueId.Source.size() > 0)
        {
            installParameters += L" --source " + uniqueId.Source;
        }
        else if (options.Manifest().size() > 0)
        {
            installParameters += L" --manifest " + options.Manifest();
        }

        if (installParameters.length() == 0)
        {
            throw hresult_invalid_argument();
        }

        UINT32 totalDownloadSizeSample = 1111110;
        // Sample progress
        InstallProgress beginDownloadProgress{ InstallProgressState::Download, 0, totalDownloadSizeSample, 0 };
        progress(beginDownloadProgress);

        for (int i = 0; i < 10; i++)
        {
            UINT32 downloaded = totalDownloadSizeSample / 10 * i;
            InstallProgress downloadProgress{ InstallProgressState::Download, downloaded, totalDownloadSizeSample, i * 10 };
            progress(downloadProgress);
            Sleep(500);
            if (cancellationToken())
            {
                InstallResult installResult(L"", L"", false);
                co_return installResult;
            }
        }

        InstallProgress endDownloadProgress{ InstallProgressState::Download, totalDownloadSizeSample, totalDownloadSizeSample, 100 };
        progress(endDownloadProgress);

        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&processInfo, sizeof(processInfo));

        std::wstring command = installCommand + installParameters;
        // Start the child process. 
        winrt::check_bool(CreateProcess(NULL,   // use command line
            (LPWSTR)command.c_str(),        // Command line
            NULL,           // Process handle is not inheritable
            NULL,           // Thread handle is not inheritable
            FALSE,          // Set handle inheritance FALSE
            0, // No creation flags
            NULL,           // Use parent environment block
            NULL,           // Use parent starting directory 
            &startupInfo,
            &processInfo)
        );

        InstallProgress installProgress{ InstallProgressState::Install, totalDownloadSizeSample, totalDownloadSizeSample, 100 };
        progress(installProgress);

        // Wait for child process exit.
        if (WaitForSingleObject(processInfo.hProcess, INFINITE) == WAIT_FAILED)
        {
            throw_last_error();
        }

        InstallProgress postInstallProgress{ InstallProgressState::PostInstall, totalDownloadSizeSample, totalDownloadSizeSample, 100 };
        progress(postInstallProgress);

        // Close thread and process handles. 
        winrt::check_bool(CloseHandle(processInfo.hProcess));
        winrt::check_bool(CloseHandle(processInfo.hThread));

        // Sample result
        InstallResult installResult(L"", L"", false);
        co_return installResult;
    }
}
