// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/Manifest.h"
#include "AppInstallerVersions.h"

namespace AppInstaller::Pinning
{
    enum class PinType
    {
        // Unknown pin type or not pinned
        Unknown,
        // Pinned by the manifest using the RequiresExplicitUpgrade field.
        // Behaves the same as Pinning pins
        PinnedByManifest,
        // The package is blocked from 'upgrade --all' and 'upgrade <package>'.
        // User has to unblock to allow update.
        Blocking,
        // The package is excluded from 'upgrade --all', unless '--include-pinned' is added.
        // 'upgrade <package>' is not blocked.
        Pinning,
        // The package is pinned to a specific version range.
        Gating,
    };

    std::string_view ToString(PinType type);
    PinType ConvertToPinTypeEnum(std::string_view in);

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
        bool operator<(const PinKey& other) const
        {
            return PackageId < other.PackageId || (PackageId == other.PackageId && SourceId < other.SourceId);
        }

        Manifest::Manifest::string_t PackageId;
        std::string SourceId;
    };

    struct Pin
    {
        static Pin CreateBlockingPin(PinKey&& pinKey);
        static Pin CreatePinningPin(PinKey&& pinKey);
        static Pin CreateGatingPin(PinKey&& pinKey, Utility::GatedVersion&& gatedVersion);

        PinType GetType() const { return m_type; }
        const PinKey& GetKey() const { return m_key; }
        const Manifest::Manifest::string_t& GetPackageId() const { return m_key.PackageId; }
        const std::string& GetSourceId() const { return m_key.SourceId;  }
        const Utility::GatedVersion& GetGatedVersion() const { return m_gatedVersion; }

        bool operator==(const Pin& other) const;

    private:
        Pin(PinType type, PinKey&& pinKey, Utility::GatedVersion&& gatedVersion = {})
            : m_type(type), m_key(std::move(pinKey)), m_gatedVersion(std::move(gatedVersion)) {}

        PinType m_type = PinType::Unknown;
        PinKey m_key;
        Utility::GatedVersion m_gatedVersion;
    };
}
