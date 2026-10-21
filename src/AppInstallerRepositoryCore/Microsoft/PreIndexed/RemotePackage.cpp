// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/RemotePackage.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerStrings.h>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    namespace
    {
        // The header that a well behaved server sets so that a client can learn the published
        // version without downloading the package.
        static constexpr std::string_view s_PackageVersionHeader = "x-ms-meta-sourceversion"sv;

        // Construct the package location from the given details.
        // Currently expects that the arg is an https uri pointing to the root of the data.
        std::string GetPackageLocation(const std::string& basePath, std::string_view fileName)
        {
            std::string result = basePath;
            if (result.back() != '/')
            {
                result += '/';
            }
            result += fileName;
            return result;
        }

        Msix::PackageVersion GetAvailableVersionFrom(const std::string& packageLocation)
        {
            if (Utility::IsUrlRemote(packageLocation))
            {
                std::map<std::string, std::string> headers = Utility::GetHeaders(packageLocation);
                auto itr = headers.find(std::string{ s_PackageVersionHeader });
                if (itr != headers.end())
                {
                    AICLI_LOG(Repo, Verbose, << "Header indicates version is: " << itr->second);
                    return { itr->second };
                }

                // We did not find the header we were looking for, log the ones we did find
                AICLI_LOG(Repo, Verbose, << "Did not find " << s_PackageVersionHeader << " in:\n" << [&]()
                    {
                        std::ostringstream headerLog;
                        for (const auto& header : headers)
                        {
                            headerLog << "  " << header.first << " : " << header.second << '\n';
                        }
                        return std::move(headerLog).str();
                    }());
            }

            AICLI_LOG(Repo, Verbose, << "Reading package data to determine version");
            Msix::MsixInfo info{ packageLocation };
            auto manifest = info.GetAppPackageManifests();

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, manifest.size() > 1);
            THROW_HR_IF(E_UNEXPECTED, manifest.size() == 0);

            return manifest[0].GetIdentity().GetVersion();
        }
    }

    std::vector<std::string> GetPackageLocations(const SourceDetails& details, const std::vector<std::string_view>& relativeLocations)
    {
        THROW_HR_IF(E_INVALIDARG, details.Arg.empty());

        std::vector<std::string> result;

        for (std::string_view relativeLocation : relativeLocations)
        {
            result.emplace_back(GetPackageLocation(details.Arg, relativeLocation));
        }

        if (!details.AlternateArg.empty())
        {
            for (std::string_view relativeLocation : relativeLocations)
            {
                result.emplace_back(GetPackageLocation(details.AlternateArg, relativeLocation));
            }
        }

        return result;
    }

    std::vector<std::string> GetFullIndexPackageLocations(const SourceDetails& details)
    {
        return GetPackageLocations(details, { s_V2_PackageFileName, s_PackageFileName });
    }

    std::vector<std::string> GetDeltaPackageLocations(const SourceDetails& details)
    {
        return GetPackageLocations(details, { s_DeltaPackageFileName });
    }

    std::vector<std::string> GetBaselinePackageLocations(const SourceDetails& details, const std::string& relativeSourcePath)
    {
        THROW_HR_IF(E_INVALIDARG, relativeSourcePath.empty());

        return GetPackageLocations(details, { relativeSourcePath });
    }

    PreIndexedPackageUpdateCheck::PreIndexedPackageUpdateCheck(std::vector<std::string> potentialLocations)
    {
        std::exception_ptr primaryException;

        for (const auto& location : potentialLocations)
        {
            try
            {
                m_availableVersion = GetAvailableVersionFrom(location);
                m_packageLocation = location;
                return;
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION_MSG("PreIndexedPackageUpdateCheck failed on location: %hs", location.c_str());
                if (!primaryException)
                {
                    primaryException = std::current_exception();
                }
            }
        }

        std::rethrow_exception(primaryException);
    }
}
