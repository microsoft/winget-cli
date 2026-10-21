// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SourceFactory.h"

#include <AppInstallerMsixInfo.h>
#include <AppInstallerVersions.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    using namespace std::string_view_literals;

    // The full index, published under two names so that a client too old to read the current
    // format can still find one that it understands.
    static constexpr std::string_view s_PackageFileName = "source.msix"sv;
    static constexpr std::string_view s_V2_PackageFileName = "source2.msix"sv;

    // The delta, published at a fixed name beside the full index.
    static constexpr std::string_view s_DeltaPackageFileName = "delta.msix"sv;

    // Gets the set of package locations that should be tried, in order.
    // Every relative location is tried against the primary arg before any is tried against the
    // alternate, so that a source stays on its primary location whenever that location can
    // serve the request at all.
    std::vector<std::string> GetPackageLocations(const SourceDetails& details, const std::vector<std::string_view>& relativeLocations);

    // The locations of the full index, which is what a client that is not using a delta acquires.
    std::vector<std::string> GetFullIndexPackageLocations(const SourceDetails& details);

    // The locations of the delta.
    std::vector<std::string> GetDeltaPackageLocations(const SourceDetails& details);

    // The locations of the baseline that a delta names.
    std::vector<std::string> GetBaselinePackageLocations(const SourceDetails& details, const std::string& relativeSourcePath);

    // Abstracts the fallback for package location when the MsixInfo is needed.
    struct PreIndexedPackageInfo
    {
        template <typename LocationCheck>
        PreIndexedPackageInfo(std::vector<std::string> potentialLocations, LocationCheck&& locationCheck)
        {
            for (const auto& location : potentialLocations)
            {
                locationCheck(location);
            }

            std::exception_ptr primaryException;

            for (const auto& location : potentialLocations)
            {
                try
                {
                    m_msixInfo = std::make_unique<Msix::MsixInfo>(location);
                    m_packageLocation = location;
                    return;
                }
                catch (...)
                {
                    LOG_CAUGHT_EXCEPTION_MSG("PreIndexedPackageInfo failed on location: %hs", location.c_str());
                    if (!primaryException)
                    {
                        primaryException = std::current_exception();
                    }
                }
            }

            std::rethrow_exception(primaryException);
        }

        const std::string& PackageLocation() const { return m_packageLocation; }
        Msix::MsixInfo& MsixInfo() { return *m_msixInfo; }

    private:
        std::string m_packageLocation;
        std::unique_ptr<Msix::MsixInfo> m_msixInfo;
    };

    // Abstracts the fallback for package location when an update is being done.
    struct PreIndexedPackageUpdateCheck
    {
        PreIndexedPackageUpdateCheck(std::vector<std::string> potentialLocations);

        const std::string& PackageLocation() const { return m_packageLocation; }
        const Msix::PackageVersion& AvailableVersion() const { return m_availableVersion; }

    private:
        std::string m_packageLocation;
        Msix::PackageVersion m_availableVersion;
    };
}
