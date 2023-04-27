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
    PinType StrictestPinType(PinType first, PinType second);

    // The set of values needed to uniquely identify a Pin.
    struct PinKey
    {
        PinKey() {}
        PinKey(const Manifest::Manifest::string_t& packageId, std::string_view sourceId)
            : PackageId(packageId), SourceId(sourceId) {}

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
            return std::tie(PackageId, SourceId) <
                std::tie(other.PackageId, other.SourceId);
        }

        // Used for logging
        std::string ToString() const;

        const Manifest::Manifest::string_t PackageId;
        const std::string SourceId;
    };

    // Gets a list of the possible pin keys that we may use for an available package
    // given the ProductCodes/PackageFamilyNames we know for the installed version.
    // If there is no ProductCode/PackageFamilyName, returns a single element with no extra id.
    std::vector<PinKey> GetPinKeysForAvailablePackage(
        std::string_view packageId,
        std::string sourceId,
        const std::vector<Utility::LocIndString>& installedProductCodes,
        const std::vector<Utility::LocIndString>& installedPackageFamilyNames);

    struct Pin
    {
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
        bool operator<(const Pin& other) const { return m_key < other.m_key; }

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
