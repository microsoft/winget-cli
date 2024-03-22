// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/Manifest.h"
#include "AppInstallerVersions.h"

namespace AppInstaller::Pinning
{
    // The pin types are ordered by how "strict" they are.
    // Meaning, the one that is more restrictive goes later.
    // This is used to decide which pin to report if there are multiple pins.
    enum class PinType : int64_t
    {
        // Unknown pin type or not pinned
        Unknown,
        // Pinned by the manifest using the RequiresExplicitUpgrade field.
        // Behaves the same as Pinning pins
        PinnedByManifest,
        // The package is excluded from 'upgrade --all', unless '--include-pinned' is added.
        // 'upgrade <package>' is not blocked.
        Pinning,
        // The package is pinned to a specific version range.
        Gating,
        // The package is blocked from 'upgrade --all' and 'upgrade <package>'.
        // User has to unblock to allow update.
        Blocking,
    };

    std::string_view ToString(PinType type);
    PinType ConvertToPinTypeEnum(std::string_view in);

    // Determines which of two pin types is more strict.
    bool IsStricter(PinType first, PinType second);

    // Returns the stricter of two pin types.
    PinType Stricter(PinType first, PinType second);

    // The set of values needed to uniquely identify a Pin.
    // A Pin can apply to an installed package or to an available package.
    // Pins on available packages can persist when an app is updated outside of winget,
    // but it's hard to have them work when there are multiple installed packages for the same available package.
    // Pins on installed packages work fine when there are multiple installed packages for the same available,
    // but they break when the package is updated outside of winget.
    struct PinKey
    {
        PinKey() = default;
        PinKey(const Manifest::Manifest::string_t& packageId, std::string_view sourceId)
            : PackageId(packageId), SourceId(sourceId) {}

        // Gets a pin key that refers to an installed package by its ProductCode or PackageFamilyName.
        // The sourceId used is a special string to distinguish from available packages.
        static PinKey GetPinKeyForInstalled(std::string_view systemReferenceString);

        bool IsForInstalled() const;

        bool operator==(const PinKey& other) const
        {
            return PackageId == other.PackageId
                && SourceId == other.SourceId;
        }

        bool operator!=(const PinKey& other) const
        {
            return !(*this == other);
        }

        bool operator<(const PinKey& other) const
        {
            // std::tie implements tuple comparison, wherein it checks the first item in the tuple,
            // iff the first elements are equal, then the second element is used for comparison, and so on
            return std::tie(PackageId, SourceId) < std::tie(other.PackageId, other.SourceId);
        }

        // Used for logging
        std::string ToString() const;

        std::string PackageId;
        std::string SourceId;
    };

    struct Pin
    {
        Pin(const Pin&) = default;
        Pin& operator=(const Pin& other) = default;

        Pin(Pin&&) = default;
        Pin& operator=(Pin&&) = default;

        static Pin CreateBlockingPin(PinKey&& pinKey);
        static Pin CreatePinningPin(PinKey&& pinKey);
        static Pin CreateGatingPin(PinKey&& pinKey, Utility::GatedVersion&& gatedVersion);

        static Pin CreateBlockingPin(const PinKey& pinKey) { return CreateBlockingPin(PinKey{ pinKey }); }
        static Pin CreatePinningPin(const PinKey& pinKey) { return CreatePinningPin(PinKey{ pinKey }); }
        static Pin CreateGatingPin(const PinKey& pinKey, const Utility::GatedVersion& gatedVersion) { return CreateGatingPin(PinKey{ pinKey }, Utility::GatedVersion{ gatedVersion }); }

        PinType GetType() const { return m_type; }
        const PinKey& GetKey() const { return m_key; }
        const Utility::GatedVersion& GetGatedVersion() const { return m_gatedVersion; }

        bool operator==(const Pin& other) const;
        bool operator<(const Pin& other) const
        {
            return std::make_pair(m_type, m_key) < std::make_pair(other.m_type, other.m_key);
        }

        // Used for logging
        std::string ToString() const;

    private:
        Pin(PinType type, PinKey&& pinKey, Utility::GatedVersion&& gatedVersion = {})
            : m_type(type), m_key(std::move(pinKey)), m_gatedVersion(std::move(gatedVersion)) {}

        PinType m_type = PinType::Unknown;
        PinKey m_key;
        Utility::GatedVersion m_gatedVersion;
    };
}
