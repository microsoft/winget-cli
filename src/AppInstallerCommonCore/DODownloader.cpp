// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DODownloader.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"
#include "winget/UserSettings.h"

// TODO: Get this from the Windows SDK when available
#define DODownloadProperty_HttpRedirectionTarget static_cast<DODownloadProperty>(DODownloadProperty_NonVolatile + 1)
#define DODownloadProperty_HttpResponseHeaders static_cast<DODownloadProperty>(DODownloadProperty_HttpRedirectionTarget + 1)
#define DODownloadProperty_HttpServerIPAddress static_cast<DODownloadProperty>(DODownloadProperty_HttpResponseHeaders + 1)
#define DODownloadProperty_HttpStatusCode static_cast<DODownloadProperty>(DODownloadProperty_HttpServerIPAddress + 1)

namespace AppInstaller::Utility
{
    namespace
    {
        std::optional<std::string> ExtractContentType(const std::optional<std::string>& headers)
        {
            if (!headers)
            {
                return std::nullopt;
            }

            static constexpr std::string_view s_ContentType = "content-type:"sv;
            auto headerLines = Utility::SplitIntoLines(headers.value());

            for (const auto& header : headerLines)
            {
                std::string_view headerView = header;
                if (header.length() >= s_ContentType.length())
                {
                    std::string lowerFragment = ToLower(headerView.substr(0, s_ContentType.length()));
                    if (s_ContentType == lowerFragment)
                    {
                        return Trim(header.substr(s_ContentType.length()));
                    }
                }
            }

            return std::nullopt;
        }
    }

    namespace DeliveryOptimization
    {
        // Represents a download work item for Delivery Optimization.
        struct Download
        {
            Download(IDOManager* manager)
            {
                THROW_IF_FAILED(manager->CreateDownload(&m_download));

                // Cloaking - sets the authentication information that will be used to make calls on the DO interface proxy.
                // This will make sure DO server impersonates the correct client identity.
                THROW_IF_FAILED(CoSetProxyBlanket(
                    m_download.get(),
                    RPC_C_AUTHN_DEFAULT,
                    RPC_C_AUTHZ_DEFAULT,
                    COLE_DEFAULT_PRINCIPAL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_DEFAULT));
            }

            ~Download()
            {
                DO_DOWNLOAD_STATUS downloadStatus;
                if (SUCCEEDED_LOG(m_download->GetStatus(&downloadStatus)))
                {
                    if (downloadStatus.State == DODownloadState_Transferred)
                    {
                        // Calling IDODownload::Finalize() to inform DO that the DO job can be cleaned up.
                        // Otherwise, the resources associated with the job can be kept for a number of days
                        // until expiration set by DO.
                        (void)LOG_IF_FAILED(m_download->Finalize());
                    }
                    else if (downloadStatus.State != DODownloadState_Finalized)
                    {
                        // For any other state, abort the download since it's no longer in use.
                        // This will allow DO to clean up the cache for the associated content ID.
                        (void)LOG_IF_FAILED(m_download->Abort());
                    }
                }
            }

            void SetProperty(DODownloadProperty prop, const std::wstring& value)
            {
                wil::unique_variant var;
                var.bstrVal = ::SysAllocString(value.c_str());
                THROW_IF_NULL_ALLOC(var.bstrVal);
                var.vt = VT_BSTR;
                THROW_IF_FAILED(m_download->SetProperty(prop, &var));
            }

            void SetProperty(DODownloadProperty prop, std::string_view value)
            {
                SetProperty(prop, Utility::ConvertToUTF16(value));
            }

            void SetProperty(DODownloadProperty prop, uint32_t value)
            {
                wil::unique_variant var;
                var.ulVal = value;
                var.vt = VT_UI4;
                THROW_IF_FAILED(m_download->SetProperty(prop, &var));
            }

            void SetProperty(DODownloadProperty prop, bool value)
            {
                wil::unique_variant var;
                var.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
                var.vt = VT_BOOL;
                THROW_IF_FAILED(m_download->SetProperty(prop, &var));
            }

            template<typename T>
            void SetUnknownProperty(DODownloadProperty prop, T&& value)
            {
                wil::unique_variant var;
                var.punkVal = nullptr;
                var.vt = VT_UNKNOWN;
                if (value)
                {
                    THROW_IF_FAILED(value->QueryInterface(IID_PPV_ARGS(&var.punkVal)));
                }
                THROW_IF_FAILED(m_download->SetProperty(prop, &var));
            }

            template<typename T>
            std::optional<T> TryGetProperty(DODownloadProperty prop)
            {
                std::optional<T> result;
                wil::unique_variant var;
                HRESULT hr = m_download->GetProperty(prop, &var);
                if (SUCCEEDED(hr))
                {
                    T value;
                    if (ExtractFromVariant(var, value))
                    {
                        result = std::move(value);
                    }
                }
                return result;
            }

            void Uri(std::string_view uri)
            {
                SetProperty(DODownloadProperty_Uri, uri);
            }

            void ContentId(std::string_view contentId)
            {
                SetProperty(DODownloadProperty_ContentId, contentId);
            }

            void DisplayName(std::string_view displayName)
            {
                SetProperty(DODownloadProperty_DisplayName, displayName);
            }

            void LocalPath(const std::filesystem::path& localPath)
            {
                SetProperty(DODownloadProperty_LocalPath, localPath.wstring());
            }

            void CorrelationVector(std::string_view correlationVector)
            {
                SetProperty(DODownloadProperty_CorrelationVector, correlationVector);
            }

            void NoProgressTimeoutSeconds(uint32_t noProgressTimeoutSeconds)
            {
                SetProperty(DODownloadProperty_NoProgressTimeoutSeconds, noProgressTimeoutSeconds);
            }

            void ForegroundPriority(bool foregroundPriority)
            {
                SetProperty(DODownloadProperty_ForegroundPriority, foregroundPriority);
            }

            void BlockingMode(bool blockingMode)
            {
                SetProperty(DODownloadProperty_BlockingMode, blockingMode);
            }

            void CallbackInterface(IDODownloadStatusCallback* callbackInterface)
            {
                SetUnknownProperty(DODownloadProperty_CallbackInterface, callbackInterface);
            }

            void StreamInterface(IStream* streamInterface)
            {
                SetUnknownProperty(DODownloadProperty_StreamInterface, streamInterface);
            }

            void CustomHeaders(const std::vector<DownloadRequestHeader>& headers)
            {
                // DODownloadProperty_HttpCustomAuthHeaders is not used (does not work in our auth scenario). It is only used when challenged.
                std::string customHeaders;
                for (const auto& header : headers)
                {
                    customHeaders += header.Name + ": " + header.Value + "\r\n";
                }

                if (!customHeaders.empty())
                {
                    SetProperty(DODownloadProperty_HttpCustomHeaders, customHeaders);
                }
            }

            // Properties that may be interesting for future use:
            // https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/deliveryoptimizationdownloadtypes/ne-deliveryoptimizationdownloadtypes-dodownloadproperty
            //  - DODownloadProperty_CostPolicy :: Allow user to specify how to behave on metered networks

            void Start()
            {
                DO_DOWNLOAD_RANGES_INFO emptyRanges{};
                emptyRanges.RangeCount = 0;
                THROW_IF_FAILED(m_download->Start(&emptyRanges));
            }

            // Returns true if Abort was successful; false if not.
            bool Cancel()
            {
                return SUCCEEDED_LOG(m_download->Abort());
            }

            void Finalize()
            {
                THROW_IF_FAILED(m_download->Finalize());
            }

            DO_DOWNLOAD_STATUS Status()
            {
                DO_DOWNLOAD_STATUS result{};
                THROW_IF_FAILED(m_download->GetStatus(&result));
                return result;
            }

        private:
            bool ExtractFromVariant(const VARIANT& var, std::string& value)
            {
                if (var.vt == VT_BSTR && var.bstrVal != nullptr)
                {
                    value = Utility::ConvertToUTF8(var.bstrVal);
                    return true;
                }
                else if (var.vt == (VT_BSTR | VT_BYREF) && var.pbstrVal != nullptr && *var.pbstrVal != nullptr)
                {
                    value = Utility::ConvertToUTF8(*var.pbstrVal);
                    return true;
                }

                return false;
            }

            wil::com_ptr<IDODownload> m_download;
        };

        // The top level Delivery Optimization manager object.
        struct Manager
        {
            Manager()
            {
                THROW_IF_FAILED(CoCreateInstance(
                    __uuidof(::DeliveryOptimization),
                    nullptr,
                    CLSCTX_LOCAL_SERVER,
                    IID_PPV_ARGS(&m_manager)));
            }

            Download CreateDownload()
            {
                return { m_manager.get() };
            }

        private:
            wil::com_ptr<IDOManager> m_manager;
        };

        // Status callback handler
        class DODownloadStatusCallback : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            IDODownloadStatusCallback>
        {
        public:
            DODownloadStatusCallback(IProgressCallback& progress) :
                m_progress(progress)
            {
            }

            IFACEMETHOD(OnStatusChange)(IDODownload*, const DO_DOWNLOAD_STATUS* status)
            {
                {
                    std::lock_guard<std::mutex> guard(m_statusMutex);
                    m_currentStatus = *status;
                }
                m_statusCV.notify_all();
                return S_OK;
            }

            static HRESULT Create(
                IProgressCallback& progress,
                DODownloadStatusCallback** result)
            {
                Microsoft::WRL::ComPtr<DODownloadStatusCallback> localResult = Microsoft::WRL::Make<DODownloadStatusCallback>(progress);
                RETURN_IF_NULL_ALLOC(localResult);

                *result = localResult.Detach();
                return S_OK;
            }

            // Simply breaks the wait in Wait; the progress object must already be cancelled to force it out.
            void Cancel()
            {
                m_statusCV.notify_all();
            }

            // Returns true on successful completion, false on cancellation, and throws on an error.
            bool Wait()
            {
                std::unique_lock<std::mutex> lock(m_statusMutex);

                // If there is no transfer status update for m_doNoProgressTimeout, we will fail.
                auto timeoutTime = std::chrono::steady_clock::now() + Settings::User().Get<Settings::Setting::NetworkDOProgressTimeoutInSeconds>();
                std::optional<UINT64> initialTransferAmount;
                bool transferChange = false;

                while (!m_progress.IsCancelledBy(CancelReason::Any))
                {
                    if (!transferChange)
                    {
                        if (m_statusCV.wait_until(lock, timeoutTime) == std::cv_status::timeout)
                        {
                            THROW_HR(DO_E_DOWNLOAD_NO_PROGRESS);
                        }
                    }
                    else
                    {
                        m_statusCV.wait(lock);
                    }

                    // Since we just finished a wait, check for cancellation before handling anything else
                    if (m_progress.IsCancelledBy(CancelReason::Any))
                    {
                        return false;
                    }

                    AICLI_LOG(Core, Verbose, << "DO State " << m_currentStatus.State << ", " << m_currentStatus.BytesTransferred << " / " << m_currentStatus.BytesTotal <<
                        ", Error 0x" << Logging::SetHRFormat << m_currentStatus.Error << ", extended error 0x" << Logging::SetHRFormat << m_currentStatus.ExtendedError);

                    // No matter the state, we are considering any error set to be a failure
                    if (FAILED(m_currentStatus.Error))
                    {
                        AICLI_LOG(Core, Error, << "DeliveryOptimization error: 0x" << Logging::SetHRFormat << m_currentStatus.Error <<
                            ", extended error: 0x" << Logging::SetHRFormat << m_currentStatus.ExtendedError);
                        THROW_HR(m_currentStatus.Error);
                    }

                    switch (m_currentStatus.State)
                    {
                        // These states are ignored.
                    case DODownloadState_Created:
                    case DODownloadState_Paused:
                        break;

                    case DODownloadState_Transferring:
                        if (m_currentStatus.BytesTransferred || m_currentStatus.BytesTotal)
                        {
                            m_progress.OnProgress(m_currentStatus.BytesTransferred, m_currentStatus.BytesTotal, ProgressType::Bytes);
                        }

                        if (!initialTransferAmount)
                        {
                            initialTransferAmount = m_currentStatus.BytesTransferred;
                        }
                        else if (m_currentStatus.BytesTransferred != initialTransferAmount.value())
                        {
                            transferChange = true;
                        }
                        break;

                        // These are considered to be 'done'
                    case DODownloadState_Transferred:
                    case DODownloadState_Finalized:
                        if (m_currentStatus.BytesTransferred || m_currentStatus.BytesTotal)
                        {
                            m_progress.OnProgress(m_currentStatus.BytesTransferred, m_currentStatus.BytesTotal, ProgressType::Bytes);
                        }
                        return true;

                        // This is the cancelled state
                    case DODownloadState_Aborted:
                        return false;
                    }
                }

                return false;
            }

        private:
            IProgressCallback& m_progress;
            std::mutex m_statusMutex;
            std::condition_variable m_statusCV;
            DO_DOWNLOAD_STATUS m_currentStatus = {};
        };
    }

    // Debugging tip:
    // From an elevated PowerShell, run:
    // > Get-DeliveryOptimizationLog | Set-Content doLogs.txt
    DownloadResult DODownload(
        const std::string& url,
        const std::filesystem::path& dest,
        IProgressCallback& progress,
        std::optional<DownloadInfo> info)
    {
        AICLI_LOG(Core, Info, << "DeliveryOptimization downloading from url: " << url);

        // Remove the target file since DO will not overwrite
        std::filesystem::remove(dest);

        DeliveryOptimization::Manager manager;
        DeliveryOptimization::Download download = manager.CreateDownload();

        wil::com_ptr<DeliveryOptimization::DODownloadStatusCallback> callback;
        THROW_IF_FAILED(DeliveryOptimization::DODownloadStatusCallback::Create(progress, &callback));

        download.Uri(url);
        download.ForegroundPriority(true);
        download.LocalPath(dest);
        download.CallbackInterface(callback.get());

        if (info)
        {
            if (!info->DisplayName.empty())
            {
                download.DisplayName(info->DisplayName);
            }

            if (!info->ContentId.empty())
            {
                download.ContentId(info->ContentId);
            }

            if (!info->RequestHeaders.empty())
            {
                download.CustomHeaders(info->RequestHeaders);
            }
        }

        download.Start();

        auto cancelLifetime = progress.SetCancellationFunction([&download, &callback]()
            {
                AICLI_LOG(Core, Info, << "Download cancelled.");
                download.Cancel();
                callback->Cancel();
            });

        // Check to handle cancellation between Start and SetCancellationFunction
        if (progress.IsCancelledBy(CancelReason::Any))
        {
            AICLI_LOG(Core, Info, << "Download cancelled.");
            download.Cancel();
            return {};
        }

        // Wait returns true for success, false for cancellation, and throws on error.
        if (callback->Wait())
        {
            // Grab the headers so that we can use them later
            std::optional<std::string> responseHeaders = download.TryGetProperty<std::string>(DODownloadProperty_HttpResponseHeaders);

            // Finalize is required to flush the data and change the file name.
            download.Finalize();
            AICLI_LOG(Core, Info, << "Download completed.");

            std::ifstream inStream{ dest, std::ifstream::binary };
            auto hashDetails = SHA256::ComputeHashDetails(inStream);

            DownloadResult result;
            result.Sha256Hash = std::move(hashDetails.Hash);
            result.SizeInBytes = hashDetails.SizeInBytes;
            result.ContentType = ExtractContentType(responseHeaders);

            return result;
        }

        return {};
    }

    bool IsDOErrorFatal(HRESULT error)
    {
        // If this gets to be large, store in a sorted array and binary search on it.
        // There will be more to update here, which we should be able to discover through telemetry.
        return
            error == DO_E_BLOCKED_BY_COST_TRANSFER_POLICY ||
            error == DO_E_BLOCKED_BY_CELLULAR_POLICY ||
            error == DO_E_BLOCKED_BY_POWER_STATE ||
            error == DO_E_BLOCKED_BY_NO_NETWORK;
    }
}
