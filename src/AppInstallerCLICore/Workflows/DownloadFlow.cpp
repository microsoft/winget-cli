// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DownloadFlow.h"
#include "MSStoreInstallerHandler.h"
#include <winget/Filesystem.h>
#include <AppInstallerDeployment.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerMsixInfo.h>
#include <winget/AdminSettings.h>
#include <winget/GroupPolicy.h>
#include <winget/ManifestYamlWriter.h>
#include <winget/NetworkSettings.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::Repository;
    using namespace AppInstaller::Utility;
    using namespace AppInstaller::Settings;
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view s_MicrosoftEntraIdAuthorizationHeader = "Authorization"sv;
        // By default Azure blob storage does not accept Microsoft Entra Id authentication.
        // https://learn.microsoft.com/en-us/rest/api/storageservices/versioning-for-the-azure-storage-services#authorize-requests-by-using-microsoft-entra-id-shared-key-or-shared-key-lite
        constexpr std::string_view s_AzureBlobStorageApiVersionHeader = "x-ms-version"sv;
        constexpr std::string_view s_AzureBlobStorageApiVersionValue = "2020-04-08"sv;

        // Get the base download directory path for the installer.
        // Also creates the directory as necessary.
        std::filesystem::path GetInstallerBaseDownloadPath(Execution::Context& context)
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();

            std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
            tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);

            std::filesystem::create_directories(tempInstallerPath);

            return tempInstallerPath;
        }

        // Get the file extension to be used for the installer file.
        std::wstring_view GetInstallerFileExtension(Execution::Context& context)
        {
            const auto& installer = context.Get<Execution::Data::Installer>();
            switch (installer->BaseInstallerType)
            {
            case InstallerTypeEnum::Burn:
            case InstallerTypeEnum::Exe:
            case InstallerTypeEnum::Inno:
            case InstallerTypeEnum::Nullsoft:
            case InstallerTypeEnum::Portable:
                return L".exe"sv;
            case InstallerTypeEnum::Msi:
            case InstallerTypeEnum::Wix:
                return L".msi"sv;
            case InstallerTypeEnum::Msix:
                // Note: We may need to distinguish between .msix and .msixbundle in the future.
                return L".msix"sv;
            case InstallerTypeEnum::Zip:
                return L".zip"sv;
            default:
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }

        // Gets a file name that should not be able to ShellExecute.
        std::filesystem::path GetInstallerPreHashValidationFileName(Execution::Context& context)
        {
            return { SHA256::ConvertToString(context.Get<Execution::Data::Installer>()->Sha256) };
        }

        // Gets the file name that can be used to ShellExecute the file.
        std::filesystem::path GetInstallerPostHashValidationFileName(Execution::Context& context)
        {
            // Get file name from download URI
            std::filesystem::path filename = GetFileNameFromURI(context.Get<Execution::Data::Installer>()->Url);
            std::wstring_view installerExtension = GetInstallerFileExtension(context);

            // Assuming that we find a safe stem value in the URI, use it.
            // This should be extremely common, but just in case fall back to the older name style.
            if (filename.has_stem() && ((filename.wstring().size() + installerExtension.size()) < MAX_PATH))
            {
                filename = filename.stem();
            }
            else
            {
                const auto& manifest = context.Get<Execution::Data::Manifest>();
                filename = Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);
            }

            filename += installerExtension;

            // Make file name suitable for file system path
            filename = Utility::ConvertToUTF16(Utility::MakeSuitablePathPart(filename.u8string()));

            return filename;
        }

        // Gets the file name for the downloaded installer in the format of {id}_{version}_{architecture}_{scope}_{installerType}_{locale}.
        std::filesystem::path GetInstallerDownloadOnlyFileName(Execution::Context& context, const std::wstring_view& extension = {})
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            const auto& installer = context.Get<Execution::Data::Installer>().value();

            std::string packageName = manifest.CurrentLocalization.Get<Localization::PackageName>();
            std::string architecture{ ToString(installer.Arch) };
            std::string installerType{ InstallerTypeToString(installer.EffectiveInstallerType()) };

            std::string fileName = packageName;

            if (!Version(manifest.Version).IsUnknown())
            {
                fileName += '_' + manifest.Version;
            }

            if (installer.Scope != ScopeEnum::Unknown)
            {
                fileName += '_' + std::string{ ScopeToString(installer.Scope) };
            }

            fileName += '_' + architecture + '_' + installerType;

            std::string locale = !installer.Locale.empty() ? installer.Locale : manifest.CurrentLocalization.Locale;
            if (!locale.empty())
            {
                fileName += '_' + locale;
            }

            std::filesystem::path fileNamePath = Utility::ConvertToUTF16(fileName);

            if (!extension.empty())
            {
                fileNamePath += extension;
            }
            else
            {
                fileNamePath += GetInstallerFileExtension(context);
            }

            // Make file name suitable for file system path
            fileNamePath = Utility::ConvertToUTF16(Utility::MakeSuitablePathPart(fileNamePath.u8string()));
            return fileNamePath;
        }

        // Try to remove the installer file, ignoring any errors.
        void RemoveInstallerFile(const std::filesystem::path& path)
        {
            try
            {
                std::filesystem::remove(path);

                // It is assumed that the parent of the installer path will always be a directory
                // If it isn't, then something went severely wrong. However, we will check that
                // it is a directory here just to be safe. If it is an empty directory, remove it.

                if (std::filesystem::is_directory(path.parent_path()) &&
                    std::filesystem::is_empty(path.parent_path()))
                {
                    std::filesystem::remove(path.parent_path());
                }
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file. Reason: " << e.what());
            }
            catch (...)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file. Reason unknown.");
            }

        }

        // Checks the file hash for an existing installer file.
        // Returns true if the file exists and its hash matches, false otherwise.
        // If the hash does not match, deletes the file.
        bool ExistingInstallerFileHasHashMatch(const SHA256::HashBuffer& expectedHash, const std::filesystem::path& filePath, SHA256::HashDetails& fileHashDetails)
        {
            if (std::filesystem::exists(filePath))
            {
                AICLI_LOG(CLI, Info, << "Found existing installer file at '" << filePath << "'. Verifying file hash.");
                std::ifstream inStream{ filePath, std::ifstream::binary };
                fileHashDetails = SHA256::ComputeHashDetails(inStream);

                if (SHA256::AreEqual(expectedHash, fileHashDetails.Hash))
                {
                    return true;
                }

                AICLI_LOG(CLI, Info, << "Hash does not match. Removing existing installer file " << filePath);
                RemoveInstallerFile(filePath);
            }

            return false;
        }

        std::string GetInstallerDownloadAuthenticationToken(const AppInstaller::Authentication::AuthenticationInfo& authInfo, Execution::Context& context)
        {
            // First check if authenticator is already created
            auto& authenticatorsMap = context.Get<AppInstaller::CLI::Execution::Data::InstallerDownloadAuthenticators>();
            auto authenticatorItr = authenticatorsMap->find(authInfo);
            if (authenticatorItr == authenticatorsMap->end())
            {
                AppInstaller::Authentication::Authenticator authenticator{ authInfo, GetAuthenticationArguments(context) };
                authenticatorsMap->emplace(authInfo, std::move(authenticator));
            }

            // Get the authenticator for auth.
            authenticatorItr = authenticatorsMap->find(authInfo);
            THROW_HR_IF(E_UNEXPECTED, authenticatorItr == authenticatorsMap->end());

            auto authResult = authenticatorItr->second.AuthenticateForToken();
            if (FAILED(authResult.Status))
            {
                AICLI_LOG(Repo, Error, << "Authentication failed for installer download. Result: " << authResult.Status);
                THROW_HR_MSG(authResult.Status, "Failed to authenticate for installer download.");
            }

            return authResult.Token;
        }

        // Get additional headers for installer download request. Auth headers are acquired here.
        std::vector<DownloadRequestHeader> GetInstallerDownloadAuthenticationHeaders(const AppInstaller::Manifest::ManifestInstaller& installer, Execution::Context& context)
        {
            std::vector<DownloadRequestHeader> result;

            switch (installer.AuthInfo.Type)
            {
            case AppInstaller::Authentication::AuthenticationType::None:
                // No auth needed
                break;
            case AppInstaller::Authentication::AuthenticationType::MicrosoftEntraId:
            case AppInstaller::Authentication::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage:
                context.Reporter.Info() << Execution::AuthenticationEmphasis << Resource::String::InstallerDownloadRequiresAuthentication << std::endl;
                result.push_back({ std::string{ s_MicrosoftEntraIdAuthorizationHeader }, Authentication::CreateBearerToken(GetInstallerDownloadAuthenticationToken(installer.AuthInfo, context)), true });
                if (installer.AuthInfo.Type == AppInstaller::Authentication::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage)
                {
                    result.push_back({ std::string{ s_AzureBlobStorageApiVersionHeader }, std::string{ s_AzureBlobStorageApiVersionValue }, false });
                }
                break;
            case AppInstaller::Authentication::AuthenticationType::Unknown:
            default:
                THROW_HR_MSG(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED, "The package installer requires authentication that is not supported.");
            }

            // Log result before return
            std::string logMessage = "Installer download headers: ";
            for (const auto& header : result)
            {
                logMessage += header.Name + ": " + (header.IsAuth ? "<Secret>" : header.Value) + "; ";
            }
            AICLI_LOG(CLI, Info, << logMessage);

            return result;
        }
    }

    void DownloadInstaller(Execution::Context& context)
    {
        // Check if file was already downloaded.
        // This may happen after a failed installation or if the download was done
        // separately before, e.g. on COM scenarios.
        context <<
            ReportExecutionStage(ExecutionStage::Download) <<
            CheckForExistingInstaller;

        if (context.IsTerminated())
        {
            return;
        }

        bool installerDownloadOnly = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly);

        // CheckForExistingInstaller will set the InstallerPath if found
        if (!context.Contains(Execution::Data::InstallerPath))
        {
            const auto& installer = context.Get<Execution::Data::Installer>().value();
            switch (installer.BaseInstallerType)
            {
            case InstallerTypeEnum::Exe:
            case InstallerTypeEnum::Burn:
            case InstallerTypeEnum::Inno:
            case InstallerTypeEnum::Msi:
            case InstallerTypeEnum::Nullsoft:
            case InstallerTypeEnum::Portable: 
            case InstallerTypeEnum::Wix:
            case InstallerTypeEnum::Zip:
                context << DownloadInstallerFile;
                break;
            case InstallerTypeEnum::Msix:
                // If the signature hash is provided in the manifest and we are doing an install,
                // we can just verify signature hash without a full download and do a streaming install.
                // Even if we have the signature hash, we still do a full download if InstallerDownloadOnly
                // flag is set, or if we need to use a proxy (as deployment APIs won't use proxy for us).
                // Finally, we require the digest API for streaming install as well.
                if (installer.SignatureSha256.empty()
                    || installerDownloadOnly
                    || Network().GetProxyUri()
                    || !Deployment::IsExpectedDigestsSupported())
                {
                    context << DownloadInstallerFile;
                }
                else
                {
                    context << GetMsixSignatureHash;
                }
                break;
            case InstallerTypeEnum::MSStore:
                if (installerDownloadOnly)
                {
                    context <<
                        MSStoreDownload <<
                        ExportManifest;
                }

                return;
            default:
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }

        context <<
            VerifyInstallerHash <<
            UpdateInstallerFileMotwIfApplicable <<
            RenameDownloadedInstaller;

        if (installerDownloadOnly)
        {
            context << ExportManifest;
        }
    }

    void CheckForExistingInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();
        if (installer.EffectiveInstallerType() == InstallerTypeEnum::MSStore)
        {
            // No installer is downloaded in this case
            return;
        }

        // Try looking for the file with and without extension.
        auto installerPath = GetInstallerBaseDownloadPath(context);
        auto installerFilename = GetInstallerPreHashValidationFileName(context);
        SHA256::HashDetails fileHashDetails;
        if (!ExistingInstallerFileHasHashMatch(installer.Sha256, installerPath / installerFilename, fileHashDetails))
        {
            installerFilename = GetInstallerPostHashValidationFileName(context);
            if (!ExistingInstallerFileHasHashMatch(installer.Sha256, installerPath / installerFilename, fileHashDetails))
            {
                // No match
                return;
            }
        }

        AICLI_LOG(CLI, Info, << "Existing installer file hash matches. Will use existing installer.");
        context.Add<Execution::Data::InstallerPath>(installerPath / installerFilename);
        context.Add<Execution::Data::DownloadHashInfo>(std::make_pair(installer.Sha256,
            DownloadResult{ std::move(fileHashDetails.Hash), fileHashDetails.SizeInBytes }));
    }

    void GetInstallerDownloadPath(Execution::Context& context)
    {
        if (!context.Contains(Execution::Data::InstallerPath))
        {
            auto tempInstallerPath = GetInstallerBaseDownloadPath(context);
            tempInstallerPath /= GetInstallerPreHashValidationFileName(context);
            AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);
            context.Add<Execution::Data::InstallerPath>(std::move(tempInstallerPath));
        }
    }

    void DownloadInstallerFile(Execution::Context& context)
    {
        context << GetInstallerDownloadPath;
        if (context.IsTerminated())
        {
            return;
        }

        const auto& installer = context.Get<Execution::Data::Installer>().value();
        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();

        Utility::DownloadInfo downloadInfo{};
        downloadInfo.DisplayName = Resource::GetFixedString(Resource::FixedString::ProductName);
        // Use the SHA256 hash of the installer as the identifier for the download
        downloadInfo.ContentId = SHA256::ConvertToString(installer.Sha256);

        try
        {
            downloadInfo.RequestHeaders = GetInstallerDownloadAuthenticationHeaders(installer, context);
        }
        catch (const wil::ResultException& re)
        {
            AICLI_LOG(CLI, Error, << "Authentication failed for installer download. Error code: " << re.GetErrorCode());

            if (re.GetErrorCode() == APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED)
            {
                context.Reporter.Error() << Resource::String::InstallerDownloadAuthenticationNotSupported << std::endl;
            }
            else
            {
                context.Reporter.Error() << Resource::String::InstallerDownloadAuthenticationFailed << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        context.Reporter.Info() << Resource::String::Downloading << ' ' << Execution::UrlEmphasis << installer.Url << std::endl;

        DownloadResult downloadResult;

        constexpr int MaxRetryCount = 2;
        constexpr std::chrono::seconds maximumWaitTimeAllowed = 60s;
        for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
        {
            bool success = false;
            try
            {
                downloadResult = context.Reporter.ExecuteWithProgress(std::bind(Utility::Download,
                    installer.Url,
                    installerPath,
                    Utility::DownloadType::Installer,
                    std::placeholders::_1,
                    downloadInfo));

                // User cancelled.
                if (downloadResult.Sha256Hash.empty())
                {
                    context.Reporter.Info() << Resource::String::Cancelled << std::endl;
                    AICLI_TERMINATE_CONTEXT(E_ABORT);
                }

                if (downloadResult.SizeInBytes == 0)
                {
                    AICLI_LOG(CLI, Info, << "Got zero byte file; retrying download after a short wait...");
                    std::this_thread::sleep_for(5s);
                }
                else
                {
                    success = true;
                }
            }
            catch (const ServiceUnavailableException& sue)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    auto waitSecondsForRetry = sue.RetryAfter();
                    if (waitSecondsForRetry > maximumWaitTimeAllowed)
                    {
                        throw;
                    }

                    bool waitCompleted = context.Reporter.ExecuteWithProgress([&waitSecondsForRetry](IProgressCallback& progress)
                        {
                            return ProgressCallback::Wait(progress, waitSecondsForRetry);
                        });

                    if (!waitCompleted)
                    {
                        break;
                    }
                }
                else
                {
                    throw;
                }
            }
            catch (...)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    AICLI_LOG(CLI, Info, << "Failed to download, waiting a bit and retry. Url: " << installer.Url);
                    Sleep(500);
                }
                else
                {
                    throw;
                }
            }

            if (success)
            {
                break;
            }
        }

        context.Add<Execution::Data::DownloadHashInfo>(std::make_pair(installer.Sha256, downloadResult));
    }

    void GetMsixSignatureHash(Execution::Context& context)
    {
        // We use this when the server won't support streaming install to swap to download.
        bool downloadInstead = false;

        try
        {
            const auto& installer = context.Get<Execution::Data::Installer>().value();

            // Signature hash is only used for streaming installs, which don't use proxy
            Msix::MsixInfo msixInfo(installer.Url);

            DownloadResult hashInfo{ msixInfo.GetSignatureHash() };
            // Value is ASCII for MSIXSTRM
            // A sentinel value to indicate that this is a streaming hash rather than a download.
            // The primary purpose is to prevent us from falling into the code path for zero byte files.
            hashInfo.SizeInBytes = 0x4D5349585354524D;

            context.Add<Execution::Data::DownloadHashInfo>(std::make_pair(installer.SignatureSha256, hashInfo));
            context.Add<Execution::Data::MsixDigests>({ std::make_pair(installer.Url, msixInfo.GetDigest()) });
        }
        catch (...)
        {
            AICLI_LOG(CLI, Info, << "Failed to get msix signature hash, fall back to direct download.");
            downloadInstead = true;
        }

        if (downloadInstead)
        {
            context << DownloadInstallerFile;
        }
    }

    void VerifyInstallerHash(Execution::Context& context)
    {
        const auto& [expectedHash, downloadResult] = context.Get<Execution::Data::DownloadHashInfo>();

        if (!std::equal(
            expectedHash.begin(),
            expectedHash.end(),
            downloadResult.Sha256Hash.begin()))
        {
            bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::HashOverride);

            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerHashMismatch(manifest.Id, manifest.Version, manifest.Channel, expectedHash, downloadResult.Sha256Hash, overrideHashMismatch, downloadResult.SizeInBytes, downloadResult.ContentType);

            if (downloadResult.SizeInBytes == 0)
            {
                context.Reporter.Error() << Resource::String::InstallerZeroByteFile << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_ZERO_BYTE_FILE);
            }

            // If running as admin, do not allow the user to override the hash failure.
            if (Runtime::IsRunningAsAdmin())
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchAdminBlock << std::endl;
            }
            else if (!Settings::IsAdminSettingEnabled(Settings::BoolAdminSetting::InstallerHashOverride))
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchError << std::endl;
            }
            else if (overrideHashMismatch)
            {
                context.Reporter.Warn() << Resource::String::InstallerHashMismatchOverridden << std::endl;
                return;
            }
            else
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchOverrideRequired << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Installer hash verified");
            context.Reporter.Info() << Resource::String::InstallerHashVerified << std::endl;

            context.SetFlags(Execution::ContextFlag::InstallerHashMatched);

            if (context.Contains(Execution::Data::PackageVersion) &&
                context.Get<Execution::Data::PackageVersion>()->GetSource() &&
                WI_IsFlagSet(context.Get<Execution::Data::PackageVersion>()->GetSource().GetDetails().TrustLevel, SourceTrustLevel::Trusted))
            {
                context.SetFlags(Execution::ContextFlag::InstallerTrusted);
            }
        }
    }

    void UpdateInstallerFileMotwIfApplicable(Execution::Context& context)
    {
        // An initial MotW is always set to URLZONE_INTERNET at the time the file is downloaded.
        // This function may change that to URLZONE_TRUSTED if appropriate
        if (context.Contains(Execution::Data::InstallerPath))
        {
            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted))
            {
                // We know the installer already went through multiple scans and we can trust it.
                Utility::ApplyMotwIfApplicable(context.Get<Execution::Data::InstallerPath>(), URLZONE_TRUSTED);
            }
            else if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerHashMatched))
            {
                // IAttachmentExecute performs some additional scans before setting MotW, for example invoking anti-virus.
                // A policy can be set to always mark files from a given domain as trusted, so only do this
                // on installers with the right hash to prevent trusting unknown installers.
                const auto& installer = context.Get<Execution::Data::Installer>();
                HRESULT hr = Utility::ApplyMotwUsingIAttachmentExecuteIfApplicable(context.Get<Execution::Data::InstallerPath>(), installer.value().Url, URLZONE_INTERNET);

                // Not using SUCCEEDED(hr) to check since there are cases file is missing after a successful scan
                if (hr != S_OK)
                {
                    switch (hr)
                    {
                    case INET_E_SECURITY_PROBLEM:
                        context.Reporter.Error() << Resource::String::InstallerBlockedByPolicy << std::endl;
                        break;
                    case E_FAIL:
                        context.Reporter.Error() << Resource::String::InstallerFailedVirusScan << std::endl;
                        break;
                    default:
                        context.Reporter.Error() << Resource::String::InstallerFailedSecurityCheck << std::endl;
                    }

                    AICLI_LOG(Fail, Error, << "Installer failed security check. Url: " << installer.value().Url << " Result: " << WINGET_OSTREAM_FORMAT_HRESULT(hr));
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED);
                }
            }
        }
    }

    void ReverifyInstallerHash(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        if (context.Contains(Execution::Data::InstallerPath))
        {
            // Get the hash from the installer file
            const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
            std::ifstream inStream{ installerPath, std::ifstream::binary };
            auto existingFileHashDetails = SHA256::ComputeHashDetails(inStream);
            context.Add<Execution::Data::DownloadHashInfo>(std::make_pair(installer.Sha256,
                DownloadResult{ existingFileHashDetails.Hash, existingFileHashDetails.SizeInBytes }));
        }
        else if (installer.EffectiveInstallerType() == InstallerTypeEnum::MSStore)
        {
            // No installer file in this case
            return;
        }
        else if (installer.EffectiveInstallerType() == InstallerTypeEnum::Msix && !installer.SignatureSha256.empty())
        {
            // We didn't download the installer file before. Just verify the signature hash again.
            context << GetMsixSignatureHash;
        }
        else
        {
            // No installer downloaded
            AICLI_LOG(CLI, Error, << "Installer file not found.");
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        }

        context << VerifyInstallerHash;
    }

    void RenameDownloadedInstaller(Execution::Context& context)
    {
        if (!context.Contains(Execution::Data::InstallerPath))
        {
            // No installer downloaded, no need to rename anything.
            return;
        }

        auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        std::filesystem::path renamedDownloadedInstaller;

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly))
        {
            THROW_HR_IF(E_UNEXPECTED, !context.Contains(Execution::Data::DownloadDirectory));

            std::filesystem::path downloadDirectory = context.Get<Execution::Data::DownloadDirectory>();

            if (!std::filesystem::exists(downloadDirectory))
            {
                std::filesystem::create_directories(downloadDirectory);
            }
            else
            {
                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE), !std::filesystem::is_directory(downloadDirectory));
            }

            renamedDownloadedInstaller = downloadDirectory / GetInstallerDownloadOnlyFileName(context);
            Filesystem::RenameFile(installerPath, renamedDownloadedInstaller);
            context.Reporter.Info() << Resource::String::InstallerDownloaded(Utility::LocIndView{ renamedDownloadedInstaller.u8string() }) << std::endl;
        }
        else
        {
            renamedDownloadedInstaller = installerPath;
            renamedDownloadedInstaller.replace_filename(GetInstallerPostHashValidationFileName(context));

            if (installerPath == renamedDownloadedInstaller)
            {
                // In case we are reusing an existing downloaded file
                return;
            }

            Filesystem::RenameFile(installerPath, renamedDownloadedInstaller);
        }

        installerPath.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << installerPath);
    }

    void RemoveInstaller(Execution::Context& context)
    {
        // Path may not be present if installed from a URL for MSIX
        if (context.Contains(Execution::Data::InstallerPath))
        {
            const auto& path = context.Get<Execution::Data::InstallerPath>();
            AICLI_LOG(CLI, Info, << "Removing installer: " << path);
            RemoveInstallerFile(path);
        }
    }

    void SetDownloadDirectory(Execution::Context& context)
    {
        if (!WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly))
        {
            return;
        }

        if (context.Args.Contains(Execution::Args::Type::DownloadDirectory))
        {
            context.Add<Execution::Data::DownloadDirectory>(std::filesystem::path{ Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::DownloadDirectory)) });
        }
        else
        {
            std::filesystem::path downloadsDirectory = Settings::User().Get<Settings::Setting::DownloadDefaultDirectory>();

            if (downloadsDirectory.empty())
            {
                downloadsDirectory = AppInstaller::Runtime::GetPathTo(AppInstaller::Runtime::PathName::UserProfileDownloads);
            }

            const auto& manifest = context.Get<Execution::Data::Manifest>();
            std::string packageDownloadFolderName = manifest.Id;
            if (!Utility::Version{ manifest.Version }.IsUnknown())
            {
                packageDownloadFolderName += '_' + manifest.Version;
            }
            context.Add<Execution::Data::DownloadDirectory>(downloadsDirectory / Utility::ConvertToUTF16(packageDownloadFolderName));
        }
    }

    void ExportManifest(Execution::Context& context)
    {
        const auto& downloadDirectory = context.Get<Execution::Data::DownloadDirectory>();
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>();

        std::filesystem::path manifestFileName = GetInstallerDownloadOnlyFileName(context, L".yaml");
        auto manifestDownloadPath = downloadDirectory / manifestFileName;
        YamlWriter::OutputYamlFile(manifest, installer.value(), manifestDownloadPath);
        AICLI_LOG(CLI, Info, << "Successfully generated manifest yaml. Path: " << manifestDownloadPath);
    }

    void EnsureSupportForDownload(Execution::Context& context)
    {
        // No checks needed if not download installer only.
        if (WI_IsFlagClear(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly))
        {
            return;
        }

        const auto& installer = context.Get<Execution::Data::Installer>();

        if (installer->DownloadCommandProhibited)
        {
            context.Reporter.Error() << Resource::String::InstallerDownloadCommandProhibited << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_DOWNLOAD_COMMAND_PROHIBITED);
        }
    }

    void InitializeInstallerDownloadAuthenticatorsMap(Execution::Context& context)
    {
        context.Add<Execution::Data::InstallerDownloadAuthenticators>(std::make_shared<std::map<Authentication::AuthenticationInfo, Authentication::Authenticator>>());
    }
}
