// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/winget/UserSettings.h"
#include "DODownloader.h"

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Settings;

namespace AppInstaller::Utility
{
    std::optional<std::vector<BYTE>> WinINetDownloadToStream(
        const std::string& url,
        std::ostream& dest,
        IProgressCallback& progress,
        bool computeHash)
    {
        AICLI_LOG(Core, Info, << "WinINet downloading from url: " << url);

        wil::unique_hinternet session(InternetOpenA(
            "winget-cli",
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0));
        THROW_LAST_ERROR_IF_NULL_MSG(session, "InternetOpen() failed.");

        wil::unique_hinternet urlFile(InternetOpenUrlA(
            session.get(),
            url.c_str(),
            NULL,
            0,
            INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, // This allows http->https redirection
            0));
        THROW_LAST_ERROR_IF_NULL_MSG(urlFile, "InternetOpenUrl() failed.");

        // Check http return status
        DWORD requestStatus = 0;
        DWORD cbRequestStatus = sizeof(requestStatus);

        THROW_LAST_ERROR_IF_MSG(!HttpQueryInfoA(urlFile.get(),
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &requestStatus,
            &cbRequestStatus,
            nullptr), "Query download request status failed.");

        if (requestStatus != HTTP_STATUS_OK)
        {
            AICLI_LOG(Core, Error, << "Download request failed. Returned status: " << requestStatus);
            THROW_HR_MSG(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, requestStatus), "Download request status is not success.");
        }

        AICLI_LOG(Core, Verbose, << "Download request status success.");

        // Get content length. Don't fail the download if failed.
        LONGLONG contentLength = 0;
        DWORD cbContentLength = sizeof(contentLength);

        HttpQueryInfoA(
            urlFile.get(),
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
            &contentLength,
            &cbContentLength,
            nullptr);
        AICLI_LOG(Core, Verbose, << "Download size: " << contentLength);

        // Setup hash engine
        SHA256 hashEngine;

        const int bufferSize = 1024 * 1024; // 1MB
        auto buffer = std::make_unique<BYTE[]>(bufferSize);

        BOOL readSuccess = true;
        DWORD bytesRead = 0;
        LONGLONG bytesDownloaded = 0;

        do
        {
            if (progress.IsCancelled())
            {
                AICLI_LOG(Core, Info, << "Download cancelled.");
                return {};
            }

            readSuccess = InternetReadFile(urlFile.get(), buffer.get(), bufferSize, &bytesRead);

            THROW_LAST_ERROR_IF_MSG(!readSuccess, "InternetReadFile() failed.");

            if (computeHash)
            {
                hashEngine.Add(buffer.get(), bytesRead);
            }

            dest.write((char*)buffer.get(), bytesRead);

            bytesDownloaded += bytesRead;

            if (bytesRead != 0)
            {
                progress.OnProgress(bytesDownloaded, contentLength, ProgressType::Bytes);
            }

        } while (bytesRead != 0);

        dest.flush();

        // Check download size matches if content length is provided in response header
        if (contentLength > 0)
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_DOWNLOAD_SIZE_MISMATCH, bytesDownloaded != contentLength);
        }

        std::vector<BYTE> result;
        if (computeHash)
        {
            result = hashEngine.Get();
            AICLI_LOG(Core, Info, << "Download hash: " << SHA256::ConvertToString(result));
        }

        AICLI_LOG(Core, Info, << "Download completed.");

        return result;
    }

    std::optional<std::vector<BYTE>> DownloadToStream(
        const std::string& url,
        std::ostream& dest,
        DownloadType,
        IProgressCallback& progress,
        bool computeHash,
        std::optional<DownloadInfo>)
    {
        THROW_HR_IF(E_INVALIDARG, url.empty());
        return WinINetDownloadToStream(url, dest, progress, computeHash);
    }

    std::optional<std::vector<BYTE>> Download(
        const std::string& url,
        const std::filesystem::path& dest,
        DownloadType type,
        IProgressCallback& progress,
        bool computeHash,
        std::optional<DownloadInfo> info)
    {
        THROW_HR_IF(E_INVALIDARG, url.empty());
        THROW_HR_IF(E_INVALIDARG, dest.empty());

        AICLI_LOG(Core, Info, << "Downloading to path: " << dest);

        std::filesystem::create_directories(dest.parent_path());

        // Only Installers should be downloaded with DO currently, as:
        //  - Index :: Constantly changing blob at same location is not what DO is for
        //  - Manifest :: DO overhead is not needed for small files
        //  - WinGetUtil :: Intentionally not using DO at this time
        if (type == DownloadType::Installer)
        {
            // Determine whether to try DO first or not, as this is the only choice currently supported.
            InstallerDownloader setting = User().Get<Setting::NetworkDownloader>();

            if (setting == InstallerDownloader::Default ||
                setting == InstallerDownloader::DeliveryOptimization)
            {
                try
                {
                    auto result = DODownload(url, dest, progress, computeHash, info);
                    // Since we cannot pre-apply to the file with DO, post-apply the MotW to the file.
                    // Only do so if the file exists, because cancellation will not throw here.
                    if (std::filesystem::exists(dest))
                    {
                        ApplyMotwIfApplicable(dest, URLZONE_INTERNET);
                    }
                    return result;
                }
                catch (const wil::ResultException& re)
                {
                    // Fall back to WinINet below unless the specific error is not one that should be ignored.
                    // We need to be careful not to bypass metered networks or other reasons that might
                    // intentionally cause the download to be blocked.
                    HRESULT hr = re.GetErrorCode();
                    if (IsDOErrorFatal(hr))
                    {
                        throw;
                    }
                    else
                    {
                        // Send telemetry so that we can understand the reasons for DO failing
                        Logging::Telemetry().LogNonFatalDOError(url, hr);
                    }
                }

                // If we reach this point, we are intending to fall through to WinINet.
                // Remove any file that may have been placed in the target location.
                if (std::filesystem::exists(dest))
                {
                    std::filesystem::remove(dest);
                }
            }
        }

        std::ofstream emptyDestFile(dest);
        emptyDestFile.close();
        ApplyMotwIfApplicable(dest, URLZONE_INTERNET);

        // Use std::ofstream::app to append to previous empty file so that it will not
        // create a new file and clear motw.
        std::ofstream outfile(dest, std::ofstream::binary | std::ofstream::app);
        return WinINetDownloadToStream(url, outfile, progress, computeHash);
    }

    using namespace std::string_view_literals;
    constexpr std::string_view s_http_start = "http://"sv;
    constexpr std::string_view s_https_start = "https://"sv;

    bool IsUrlRemote(std::string_view url)
    {
        // Very simple choice right now: "does it start with http:// or https://"?
        if (CaseInsensitiveStartsWith(url, s_http_start) ||
            CaseInsensitiveStartsWith(url, s_https_start))
        {
            return true;
        }

        return false;
    }

    bool IsUrlSecure(std::string_view url)
    {
        // Very simple choice right now: "does it start with https://"?
        if (CaseInsensitiveStartsWith(url, s_https_start))
        {
            return true;
        }

        return false;
    }

    void ApplyMotwIfApplicable(const std::filesystem::path& filePath, URLZONE zone)
    {
        AICLI_LOG(Core, Info, << "Started applying motw to " << filePath << " with zone: " << zone);

        if (!IsNTFS(filePath))
        {
            AICLI_LOG(Core, Info, << "File system is not NTFS. Skipped applying motw");
            return;
        }

        Microsoft::WRL::ComPtr<IZoneIdentifier> zoneIdentifier;
        THROW_IF_FAILED(CoCreateInstance(CLSID_PersistentZoneIdentifier, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&zoneIdentifier)));
        THROW_IF_FAILED(zoneIdentifier->SetId(zone));

        Microsoft::WRL::ComPtr<IPersistFile> persistFile;
        THROW_IF_FAILED(zoneIdentifier.As(&persistFile));
        THROW_IF_FAILED(persistFile->Save(filePath.c_str(), TRUE));

        AICLI_LOG(Core, Info, << "Finished applying motw");
    }

    HRESULT ApplyMotwUsingIAttachmentExecuteIfApplicable(const std::filesystem::path& filePath, const std::string& source)
    {
        AICLI_LOG(Core, Info, << "Started applying motw using IAttachmentExecute to " << filePath);

        if (!IsNTFS(filePath))
        {
            AICLI_LOG(Core, Info, << "File system is not NTFS. Skipped applying motw");
            return S_OK;
        }

        // Attachment execution service needs STA to succeed, so we'll create a new thread and CoInitialize with STA.
        HRESULT aesSaveResult = S_OK;
        auto updateMotw = [&]() -> HRESULT
        {
            Microsoft::WRL::ComPtr<IAttachmentExecute> attachmentExecute;
            RETURN_IF_FAILED(CoCreateInstance(CLSID_AttachmentServices, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&attachmentExecute)));
            RETURN_IF_FAILED(attachmentExecute->SetLocalPath(filePath.c_str()));
            RETURN_IF_FAILED(attachmentExecute->SetSource(Utility::ConvertToUTF16(source).c_str()));
            aesSaveResult = attachmentExecute->Save();
            RETURN_IF_FAILED(aesSaveResult);
            return S_OK;
        };

        HRESULT hr = S_OK;

        std::thread aesThread([&]()
            {
                hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                if (FAILED(hr))
                {
                    return;
                }

                hr = updateMotw();
                CoUninitialize();
            });

        aesThread.join();

        AICLI_LOG(Core, Info, << "Finished applying motw using IAttachmentExecute. Result: " << hr << " IAttachmentExecute::Save() result: " << aesSaveResult);

        return aesSaveResult;
    }
}
