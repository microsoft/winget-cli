// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/FileCache.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

namespace AppInstaller::Caching
{
    namespace anon
    {
        std::string_view GetNameForType(FileCache::Type type)
        {
            switch (type)
            {
            case FileCache::Type::IndexV1_Manifest: return "V1_M";
            case FileCache::Type::IndexV2_PackageVersionData: return "V2_PVD";
            case FileCache::Type::IndexV2_Manifest: return "V2_M";
            case FileCache::Type::Icon: return "Icon";
#ifndef AICLI_DISABLE_TEST_HOOKS
            case FileCache::Type::Tests: return "Tests";
#endif
            }

            THROW_HR(E_UNEXPECTED);
        }

        std::unique_ptr<std::stringstream> GetUpstreamFile(const std::string& basePath, const std::string& relativePath, const Utility::SHA256::HashBuffer& expectedHash)
        {
            // Until signed files are implemented, fail on an empty hash
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, expectedHash.empty());

            std::string fullPath = basePath;
            if (fullPath.back() != '/')
            {
                fullPath += '/';
            }
            fullPath += relativePath;

            if (Utility::IsUrlRemote(fullPath))
            {
                auto result = std::make_unique<std::stringstream>();

                AICLI_LOG(Core, Verbose, << "Getting upstream file from remote: " << fullPath);
                ProgressCallback emptyCallback;

                constexpr int MaxRetryCount = 2;
                constexpr std::chrono::seconds maximumWaitTimeAllowed = 10s;
                for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
                {
                    try
                    {
                        auto downloadResult = Utility::DownloadToStream(fullPath, *result, Utility::DownloadType::Manifest, emptyCallback);

                        if (!expectedHash.empty() &&
                            !Utility::SHA256::AreEqual(expectedHash, downloadResult.Sha256Hash))
                        {
                            AICLI_LOG(Core, Verbose, << "Invalid hash from [" << fullPath << "]: expected [" << Utility::SHA256::ConvertToString(expectedHash) << "], got [" << Utility::SHA256::ConvertToString(downloadResult.Sha256Hash) << "]");
                            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
                        }

                        break;
                    }
                    catch (const Utility::ServiceUnavailableException& sue)
                    {
                        if (retryCount < MaxRetryCount - 1)
                        {
                            auto waitSecondsForRetry = sue.RetryAfter();
                            if (waitSecondsForRetry > maximumWaitTimeAllowed)
                            {
                                throw;
                            }

                            // TODO: Get real progress callback to allow cancelation.
                            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(waitSecondsForRetry);
                            Sleep(static_cast<DWORD>(ms.count()));
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
                            AICLI_LOG(Core, Verbose, << "Getting upstream file failed, waiting a bit and retrying: " << fullPath);
                            Sleep(500);
                        }
                        else
                        {
                            throw;
                        }
                    }
                }

                return result;
            }
            else
            {
                AICLI_LOG(Core, Verbose, << "Getting upstream file from local: " << fullPath);
                std::ifstream fileStream{ fullPath, std::ios_base::in | std::ios_base::binary };
                std::string fileContents = Utility::ReadEntireStream(fileStream);

                auto fileContentsHash = Utility::SHA256::ComputeHash(fileContents);

                if (expectedHash.empty() || Utility::SHA256::AreEqual(expectedHash, fileContentsHash))
                {
                    return std::make_unique<std::stringstream>(std::move(fileContents));
                }
                else
                {
                    THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
                }
            }
        }
    }

    FileCache::Details::Details(FileCache::Type type, std::string identifier) :
        Type(type), Identifier(std::move(identifier))
    {
        switch (type)
        {
        case Type::IndexV1_Manifest:
        case Type::IndexV2_PackageVersionData:
        case Type::IndexV2_Manifest:
        case Type::Icon:
#ifndef AICLI_DISABLE_TEST_HOOKS
        case Type::Tests:
#endif
            BasePath = Runtime::PathName::Temp;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::filesystem::path FileCache::Details::GetCachePath() const
    {
        std::filesystem::path result = Runtime::GetPathTo(BasePath);
        result /= "cache";
        result /= anon::GetNameForType(Type);
        result /= Utility::ConvertToUTF16(Identifier);
        return result;
    }

    FileCache::FileCache(Type type, std::string identifier, std::vector<std::string> sources) :
        m_details(type, std::move(identifier)), m_sources(std::move(sources))
    {
        m_cacheBase = m_details.GetCachePath();
    }

    const FileCache::Details& FileCache::GetDetails() const
    {
        return m_details;
    }

    std::unique_ptr<std::istream> FileCache::GetFile(const std::filesystem::path& relativePath, const Utility::SHA256::HashBuffer& expectedHash) const
    {
        std::filesystem::path cachedFilePath = m_cacheBase / relativePath;

        // Check cache for matching file
        try
        {
            if (std::filesystem::is_regular_file(cachedFilePath))
            {
                AICLI_LOG(Core, Verbose, << "Reading cached file [" << cachedFilePath << "]");

                std::ifstream fileStream{ cachedFilePath, std::ios_base::in | std::ios_base::binary };
                std::string fileContents = Utility::ReadEntireStream(fileStream);

                auto fileContentsHash = Utility::SHA256::ComputeHash(fileContents);

                if (Utility::SHA256::AreEqual(expectedHash, fileContentsHash))
                {
                    return std::make_unique<std::istringstream>(std::move(fileContents));
                }
                else
                {
                    AICLI_LOG(Core, Verbose, << "Removing cached file [" << cachedFilePath << "] due to hash mismatch; expected [" <<
                        Utility::SHA256::ConvertToString(expectedHash) << "] but was [" << Utility::SHA256::ConvertToString(fileContentsHash) << "]");
                }
            }

            std::filesystem::remove_all(cachedFilePath);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION_MSG("Error while attempting to read cached file");
        }

        // Making it here means that we do not have a cached file or it needed to be updated and was removed.
        auto result = GetUpstreamFile(relativePath.u8string(), expectedHash);

        // GetUpstreamFile only returns with a successfully verified hash, we just need to write the file out.
        // Only log failures as caching is an optimization.
        try
        {
            std::filesystem::create_directories(cachedFilePath.parent_path());

            AICLI_LOG(Core, Verbose, << "Writing cached file [" << cachedFilePath << "]");
            std::ofstream fileStream{ cachedFilePath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc };
            LOG_LAST_ERROR_IF(fileStream.fail());
            fileStream << result->str() << std::flush;
            LOG_LAST_ERROR_IF(fileStream.fail());
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION_MSG("Error while attempting to write cached file");
        }

        return result;
    }

    std::unique_ptr<std::stringstream> FileCache::GetUpstreamFile(std::string relativePath, const Utility::SHA256::HashBuffer& expectedHash) const
    {
        // Replace backslashes with forward slashes for HTTP requests (since local can handle them).
        Utility::FindAndReplace(relativePath, "\\", "/");

        std::exception_ptr firstException;

        for (const auto& upstream : m_sources)
        {
            try
            {
                return anon::GetUpstreamFile(upstream, relativePath, expectedHash);
            }
            catch(...)
            {
                LOG_CAUGHT_EXCEPTION_MSG("GetUpstreamFile failed on source: %hs", upstream.c_str());
                if (!firstException)
                {
                    firstException = std::current_exception();
                }
            }
        }

        if (firstException)
        {
            std::rethrow_exception(firstException);
        }

        // Somewhat arbitrary error that should only happen if no upstream sources provided.
        THROW_HR(E_NOT_SET);
    }
}
