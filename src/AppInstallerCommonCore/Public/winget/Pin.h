// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/Manifest.h"
#include "AppInstallerVersions.h"

namespace AppInstaller::Pinning
{
    enum class PinType
    {
        Unknown,
        // The package is blocked from 'upgrade --all' and 'upgrade <package>'.
        // User has to unblock to allow update.
        Blocking,
        // The package is excluded from 'upgrade --all', unless '--include-pinned' is added.
        // 'upgrade <package>' is not blocked.
        Pinning,
        // The package is pinned to a specific version.
        Gating,
    };

    std::string_view ToString(PinType type);

    // The set of values needed to uniquely identify a Pin
    struct PinKey
    {
        PinKey() {}
        PinKey(const Manifest::Manifest::string_t& packageId, std::string_view sourceId)
            : PackageId(packageId), SourceId(sourceId) {}

        bool operator==(const PinKey& other) const
        {
            return PackageId == other.PackageId && SourceId == other.SourceId;
        }
        bool operator!=(const PinKey& other) const
        {
            return !(*this == other);
        }

        Manifest::Manifest::string_t PackageId;
        std::string SourceId;
    };

    struct Pin
    {
        static Pin CreateBlockingPin(PinKey&& pinKey);
        static Pin CreatePinningPin(PinKey&& pinKey);
        static Pin CreateGatingPin(PinKey&& pinKey, Utility::GatedVersion&& gatedVersion);

        PinType GetType() const;
        const PinKey& GetKey() const;
        const Manifest::Manifest::string_t& GetPackageId() const;
        std::string_view GetSourceId() const;
        Utility::GatedVersion GetGatedVersion() const;

        bool operator==(const Pin& other) const;

    private:
        Pin(PinType type, PinKey&& pinKey, Utility::GatedVersion&& gatedVersion = {})
            : m_type(type), m_key(std::move(pinKey)), m_gatedVersion(std::move(gatedVersion)) {}

        PinType m_type = PinType::Unknown;
        PinKey m_key;
        Utility::GatedVersion m_gatedVersion;
    };
}
