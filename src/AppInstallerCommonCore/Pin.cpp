// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Pin.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Pinning
{
    using namespace std::string_view_literals;

    std::string_view ToString(PinType type)
    {
        switch (type)
        {
        case PinType::Blocking:
            return "Blocking"sv;
        case PinType::Pinning:
            return "Pinning"sv;
        case PinType::Gating:
            return "Gating"sv;
        case PinType::PinnedByManifest:
            return "PinnedByManifest"sv;
        case PinType::Unknown:
        default:
            return "Unknown";
        }
    }

    PinType ConvertToPinTypeEnum(std::string_view in)
    {
        if (Utility::CaseInsensitiveEquals(in, "Blocking"sv))
        {
            return PinType::Blocking;
        }
        else if (Utility::CaseInsensitiveEquals(in, "Pinning"sv))
        {
            return PinType::Pinning;
        }
        else if (Utility::CaseInsensitiveEquals(in, "Gating"sv))
        {
            return PinType::Gating;
        }
        else if (Utility::CaseInsensitiveEquals(in, "PinnedByManifest"sv))
        {
            return PinType::PinnedByManifest;
        }
        else
        {
            return PinType::Unknown;
        }
    }

    PinType StrictestPinType(PinType first, PinType second)
    {
        return std::max(first, second);
    }

    std::string_view ToString(ExtraIdStringType type)
    {
        switch (type)
        {
        case ExtraIdStringType::ProductCode:
            return "ProductCode"sv;
        case ExtraIdStringType::PackageFamilyName:
            return "PackageFamilyName";
        case ExtraIdStringType::None:
        default:
            return "None";
        }
    }

    std::vector<PinKey> GetPinKeysForAvailablePackage(
        std::string_view packageId,
        std::string sourceId,
        const std::vector<Utility::LocIndString>& installedProductCodes,
        const std::vector<Utility::LocIndString>& installedPackageFamilyNames)
    {
        std::vector<Pinning::PinKey> pinKeys;

        for (const auto& productCode : installedProductCodes)
        {
            pinKeys.emplace_back(packageId, sourceId, Pinning::ExtraIdStringType::ProductCode, productCode);
        }

        for (const auto& packageFamilyName : installedPackageFamilyNames)
        {
            pinKeys.emplace_back(packageId, sourceId, Pinning::ExtraIdStringType::PackageFamilyName, packageFamilyName);
        }

        if (pinKeys.empty())
        {
            pinKeys.emplace_back(packageId, sourceId, Pinning::ExtraIdStringType::None, "");
        }

        return pinKeys;
    }

    Pin Pin::CreateBlockingPin(PinKey&& pinKey)
    {
        return { PinType::Blocking, std::move(pinKey) };
    }

    Pin Pin::CreatePinningPin(PinKey&& pinKey)
    {
        return { PinType::Pinning, std::move(pinKey) };
    }

    Pin Pin::CreateGatingPin(PinKey&& pinKey, GatedVersion&& gatedVersion)
    {
        return { PinType::Gating, std::move(pinKey), std::move(gatedVersion) };
    }

    bool Pin::operator==(const Pin& other) const
    {
        return m_type == other.m_type &&
            m_key == other.m_key &&
            m_gatedVersion == other.m_gatedVersion;
    }
}