// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <wininet.h>
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/winget/UserSettings.h"
#include "Public/winget/NetworkSettings.h"
#include "Public/winget/Filesystem.h"
#include "DODownloader.h"
#include "HttpStream/HttpRandomAccessStream.h"

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Filesystem;
using namespace AppInstaller::Utility::HttpStream;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Web::Http::Headers;
using namespace winrt::Windows::Web::Http::Filters;

namespace AppInstaller::Utility
{
    namespace
    {
        std::wstring GetHttpQueryString(const wil::unique_hinternet& urlFile, DWORD queryProperty)
        {
            std::wstring result = {};
            DWORD length = 0;
            if (!HttpQueryInfoW(urlFile.get(),
                queryProperty,
                &result[0],
                &length,
                nullptr))
            {
                auto lastError = GetLastError();
                if (lastError == ERROR_INSUFFICIENT_BUFFER)
                {
                    // lpdwBufferLength contains the size, in bytes, of a buffer large enough to receive the requested information
                    // without the nul char. not the exact buffer size.
                    auto size = static_cast<size_t>(length) / sizeof(wchar_t);
                    result.resize(size + 1);
                    if (HttpQueryInfoW(urlFile.get(),
                        queryProperty,
                        &result[0],
                        &length,
                        nullptr))
                    {
                        // because the buffer can be bigger remove possible null chars
                        result.erase(result.find(L'\0'));
                    }
                    else
                    {
                        AICLI_LOG(Core, Error, << "Error retrieving header value [" << queryProperty << "]: " << GetLastError());
                        result.clear();
                    }
                }
                else
                {
                    AICLI_LOG(Core, Error, << "Error retrieving header [" << queryProperty << "]: " << GetLastError());
                }
            }

            return result;
        }

        // Gets the retry after value in terms of a delay in seconds
        std::chrono::seconds GetRetryAfter(const HttpDateOrDeltaHeaderValue& retryAfter)
        {
            if (retryAfter)
            {
                auto delta = retryAfter.Delta();
                if (delta)
                {
                    return  std::chrono::duration_cast<std::chrono::seconds>(delta.GetTimeSpan());
                }

                auto dateTimeRef = retryAfter.Date();
                if (dateTimeRef)
                {
                    auto dateTime = dateTimeRef.GetDateTime();
                    auto now = winrt::clock::now();

                    if (dateTime > now)
                    {
                        return std::chrono::duration_cast<std::chrono::seconds>(dateTime - now);
                    }
                }
            }

            return 0s;
        }

        std::chrono::seconds GetRetryAfter(const wil::unique_hinternet& urlFile)
        {
            std::wstring retryAfter = GetHttpQueryString(urlFile, HTTP_QUERY_RETRY_AFTER);
            return retryAfter.empty() ? 0s : AppInstaller::Utility::GetRetryAfter(retryAfter);
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    namespace TestHooks
    {
        static std::function<DownloadResult(
            const std::string& url,
            const std::filesystem::path& dest,
            DownloadType type,
            IProgressCallback& progress,
            std::optional<DownloadInfo> info)>* s_Download_Function_Override = nullptr;

        void SetDownloadResult_Function_Override(std::function<DownloadResult(
            const std::string& url,
            const std::filesystem::path& dest,
            DownloadType type,
            IProgressCallback& progress,
            std::optional<DownloadInfo> info)>* value)
        {
            s_Download_Function_Override = value;
        }
    }
#endif

    DownloadResult WinINetDownloadToStream(
        const std::string& url,
        std::ostream& dest,
        IProgressCallback& progress,
        std::optional<DownloadInfo> info)
    {
        // For AICLI_LOG usages with string literals.
        #pragma warning(push)
        #pragma warning(disable:26449)

        AICLI_LOG(Core, Info, << "WinINet downloading from url: " << url);

        auto agentWide = Utility::ConvertToUTF16(Runtime::GetDefaultUserAgent().get());
        wil::unique_hinternet session;

        const auto& proxyUri = Network().GetProxyUri();
        if (proxyUri)
        {
            AICLI_LOG(Core, Info, << "Using proxy " << proxyUri.value());
            session.reset(InternetOpen(
                agentWide.c_str(),
                INTERNET_OPEN_TYPE_PROXY,
                Utility::ConvertToUTF16(proxyUri.value()).c_str(),
                NULL,
                0));
        }
        else
        {
            session.reset(InternetOpen(
                agentWide.c_str(),
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0));
        }

        THROW_LAST_ERROR_IF_NULL_MSG(session, "InternetOpen() failed.");

        std::string customHeaders;
        if (info && info->RequestHeaders.size() > 0)
        {
            for (const auto& header : info->RequestHeaders)
            {
                customHeaders += header.Name + ": " + header.Value + "\r\n";
            }
        }
        std::wstring customHeadersWide = Utility::ConvertToUTF16(customHeaders);

        auto urlWide = Utility::ConvertToUTF16(url);
        wil::unique_hinternet urlFile(InternetOpenUrl(
            session.get(),
            urlWide.c_str(),
            customHeadersWide.empty() ? NULL : customHeadersWide.c_str(),
            customHeadersWide.empty() ? 0 : (DWORD)(customHeadersWide.size()),
            INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, // This allows http->https redirection
            0));
        THROW_LAST_ERROR_IF_NULL_MSG(urlFile, "InternetOpenUrl() failed.");

        // Check http return status
        DWORD requestStatus = 0;
        DWORD cbRequestStatus = sizeof(requestStatus);

        THROW_LAST_ERROR_IF_MSG(!HttpQueryInfoW(urlFile.get(),
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &requestStatus,
            &cbRequestStatus,
            nullptr), "Query download request status failed.");

        constexpr DWORD TooManyRequest = 429;

        switch (requestStatus)
        {
        case HTTP_STATUS_OK:
            // All good
            break;
        case TooManyRequest:
        case HTTP_STATUS_SERVICE_UNAVAIL:
        {
            THROW_EXCEPTION(ServiceUnavailableException(GetRetryAfter(urlFile)));
        }
        default:
            AICLI_LOG(Core, Error, << "Download request failed. Returned status: " << requestStatus);
            THROW_HR_MSG(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, requestStatus), "Download request status is not success.");
        }

        AICLI_LOG(Core, Verbose, << "Download request status success.");

        // Get content length. Don't fail the download if failed.
        LONGLONG contentLength = 0;
        DWORD cbContentLength = sizeof(contentLength);

        HttpQueryInfoW(
            urlFile.get(),
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
            &contentLength,
            &cbContentLength,
            nullptr);
        AICLI_LOG(Core, Verbose, << "Download size: " << contentLength);

        std::string contentType = Utility::ConvertToUTF8(GetHttpQueryString(urlFile, HTTP_QUERY_CONTENT_TYPE));
        AICLI_LOG(Core, Verbose, << "Content Type: " << contentType);

        // Setup hash engine
        SHA256 hashEngine;

        const int bufferSize = 1024 * 1024; // 1MB
        auto buffer = std::make_unique<BYTE[]>(bufferSize);

        BOOL readSuccess = true;
        DWORD bytesRead = 0;
        LONGLONG bytesDownloaded = 0;

        do
        {
            if (progress.IsCancelledBy(CancelReason::Any))
            {
                AICLI_LOG(Core, Info, << "Download cancelled.");
                return {};
            }

            readSuccess = InternetReadFile(urlFile.get(), buffer.get(), bufferSize, &bytesRead);

            THROW_LAST_ERROR_IF_MSG(!readSuccess, "InternetReadFile() failed.");

            hashEngine.Add(buffer.get(), bytesRead);

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

        DownloadResult result;
        result.SizeInBytes = static_cast<uint64_t>(bytesDownloaded);
        result.ContentType = std::move(contentType);
        result.Sha256Hash = hashEngine.Get();
        AICLI_LOG(Core, Info, << "Download hash: " << SHA256::ConvertToString(result.Sha256Hash));

        AICLI_LOG(Core, Info, << "Download completed.");

        #pragma warning(pop)

        return result;
    }

    std::map<std::string, std::string> GetHeaders(std::string_view url)
    {
        // TODO: Use proxy info. HttpClient does not support using a custom proxy, only using the system-wide one.
        AICLI_LOG(Core, Verbose, << "Retrieving headers from url: " << url);

        HttpBaseProtocolFilter filter;
        filter.CacheControl().ReadBehavior(HttpCacheReadBehavior::MostRecent);

        HttpClient client(filter);
        client.DefaultRequestHeaders().Connection().Clear();
        client.DefaultRequestHeaders().Append(L"Connection", L"close");
        client.DefaultRequestHeaders().UserAgent().ParseAdd(Utility::ConvertToUTF16(Runtime::GetDefaultUserAgent().get()));

        winrt::Windows::Foundation::Uri uri{ Utility::ConvertToUTF16(url) };
        HttpRequestMessage request(HttpMethod::Head(), uri);

        HttpResponseMessage response = client.SendRequestAsync(request, HttpCompletionOption::ResponseHeadersRead).get();

        switch (response.StatusCode())
        {
        case HttpStatusCode::Ok:
            // All good
            break;
        case HttpStatusCode::TooManyRequests:
        case HttpStatusCode::ServiceUnavailable:
        {
            THROW_EXCEPTION(ServiceUnavailableException(GetRetryAfter(response.Headers().RetryAfter())));
        }
        default:
            THROW_HR(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, response.StatusCode()));
        }

        std::map<std::string, std::string> result;

        for (const auto& header : response.Headers())
        {
            result.emplace(Utility::FoldCase(static_cast<std::string_view>(Utility::ConvertToUTF8(header.Key()))), Utility::ConvertToUTF8(header.Value()));
        }

        return result;
    }

    DownloadResult DownloadToStream(
        const std::string& url,
        std::ostream& dest,
        DownloadType,
        IProgressCallback& progress,
        std::optional<DownloadInfo> info)
    {
        THROW_HR_IF(E_INVALIDARG, url.empty());
        return WinINetDownloadToStream(url, dest, progress, info);
    }

    DownloadResult Download(
        const std::string& url,
        const std::filesystem::path& dest,
        DownloadType type,
        IProgressCallback& progress,
        std::optional<DownloadInfo> info)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (TestHooks::s_Download_Function_Override)
        {
            return (*TestHooks::s_Download_Function_Override)(url, dest, type, progress, info);
        }
#endif

        THROW_HR_IF(E_INVALIDARG, url.empty());
        THROW_HR_IF(E_INVALIDARG, dest.empty());

        AICLI_LOG(Core, Info, << "Downloading to path: " << dest);

        std::filesystem::create_directories(dest.parent_path());

        // Only Installers should be downloaded with DO currently, as:
        //  - Index :: Constantly changing blob at same location is not what DO is for
        //  - Manifest / InstallerMetadataCollectionInput :: DO overhead is not needed for small files
        //  - WinGetUtil :: Intentionally not using DO at this time
        if (type == DownloadType::Installer)
        {
            if (Network().GetInstallerDownloader() == InstallerDownloader::DeliveryOptimization)
            {
                try
                {
                    auto result = DODownload(url, dest, progress, info);
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
        return WinINetDownloadToStream(url, outfile, progress, info);
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
    
    static inline bool FileSupportsMotw(const std::filesystem::path& path)
    {
        return SupportsNamedStreams(path);
    }

    void ApplyMotwIfApplicable(const std::filesystem::path& filePath, URLZONE zone)
    {
        AICLI_LOG(Core, Info, << "Started applying motw to " << filePath << " with zone: " << zone);

        if (!FileSupportsMotw(filePath))
        {
            AICLI_LOG(Core, Info, << "File system does not support ADS. Skipped applying motw");
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

    void RemoveMotwIfApplicable(const std::filesystem::path& filePath)
    {
        AICLI_LOG(Core, Info, << "Started removing motw to " << filePath);

        if (!FileSupportsMotw(filePath))
        {
            AICLI_LOG(Core, Info, << "File system does not support ADS. Skipped removing motw");
            return;
        }

        Microsoft::WRL::ComPtr<IZoneIdentifier> zoneIdentifier;
        THROW_IF_FAILED(CoCreateInstance(CLSID_PersistentZoneIdentifier, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&zoneIdentifier)));

        Microsoft::WRL::ComPtr<IPersistFile> persistFile;
        THROW_IF_FAILED(zoneIdentifier.As(&persistFile));

        auto hr = persistFile->Load(filePath.c_str(), STGM_READ);
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            // IPersistFile::Load returns same error for "file not found" and "motw not found".
            // Check if the file exists to be sure we are on the "motw not found" case.
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !std::filesystem::exists(filePath));

            AICLI_LOG(Core, Info, << "File does not contain motw. Skipped removing motw");
            return;
        }

        THROW_IF_FAILED(zoneIdentifier->Remove());
        THROW_IF_FAILED(persistFile->Save(NULL, TRUE));

        AICLI_LOG(Core, Info, << "Finished removing motw");
    }

    HRESULT ApplyMotwUsingIAttachmentExecuteIfApplicable(const std::filesystem::path& filePath, const std::string& source, URLZONE zoneIfScanFailure)
    {
        AICLI_LOG(Core, Info, << "Started applying motw using IAttachmentExecute to " << filePath);

        if (!FileSupportsMotw(filePath))
        {
            AICLI_LOG(Core, Info, << "File system does not support ADS. Skipped applying motw");
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

            // IAttachmentExecute::Save() expects the local file to be clean(i.e. it won't clear existing motw if it thinks the source url is trusted)
            RemoveMotwIfApplicable(filePath);

            aesSaveResult = attachmentExecute->Save();

            // Reapply desired zone upon scan failure.
            // Not using SUCCEEDED(hr) to check since there are cases file is missing after a successful scan
            if (aesSaveResult != S_OK && std::filesystem::exists(filePath))
            {
                ApplyMotwIfApplicable(filePath, zoneIfScanFailure);
            }

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

    Microsoft::WRL::ComPtr<IStream> GetReadOnlyStreamFromURI(std::string_view uriStr)
    {
        Microsoft::WRL::ComPtr<IStream> inputStream;
        if (Utility::IsUrlRemote(uriStr))
        {
            // Get an IStream from the input uri and try to create package or bundler reader.
            winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(uriStr));

            winrt::com_ptr<HttpRandomAccessStream> httpRandomAccessStream = winrt::make_self<HttpRandomAccessStream>();

            try
            {
                auto randomAccessStream = httpRandomAccessStream->InitializeAsync(uri).get();

                ::IUnknown* rasAsIUnknown = (::IUnknown*)winrt::get_abi(randomAccessStream);
                THROW_IF_FAILED(CreateStreamOverRandomAccessStream(
                    rasAsIUnknown,
                    IID_PPV_ARGS(inputStream.ReleaseAndGetAddressOf())));
            }
            catch (const winrt::hresult_error& hre)
            {
                if (hre.code() == APPINSTALLER_CLI_ERROR_SERVICE_UNAVAILABLE)
                {
                    THROW_EXCEPTION(AppInstaller::Utility::ServiceUnavailableException(httpRandomAccessStream->RetryAfter()));
                }

                throw;
            }
        }
        else
        {
            std::filesystem::path path(Utility::ConvertToUTF16(uriStr));
            THROW_IF_FAILED(SHCreateStreamOnFileEx(path.c_str(),
                STGM_READ | STGM_SHARE_DENY_WRITE | STGM_FAILIFTHERE, 0, FALSE, nullptr, &inputStream));
        }

        return inputStream;
    }

    std::chrono::seconds GetRetryAfter(const std::wstring& retryAfter)
    {
        try
        {
            winrt::hstring hstringValue{ retryAfter };
            HttpDateOrDeltaHeaderValue headerValue = nullptr;
            HttpDateOrDeltaHeaderValue::TryParse(hstringValue, headerValue);
            return GetRetryAfter(headerValue);
        }
        catch (...)
        {
            AICLI_LOG(Core, Error, << "Retry-After value not supported: " << Utility::ConvertToUTF8(retryAfter));
        }

        return 0s;
    }

    std::chrono::seconds GetRetryAfter(const HttpResponseMessage& response)
    {
        return GetRetryAfter(response.Headers().RetryAfter());
    }
}
