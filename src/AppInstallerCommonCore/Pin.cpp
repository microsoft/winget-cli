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

    Pin Pin::CreateBlockingPin(PinKey&& pinKey, Version version)
    {
        return { PinType::Blocking, std::move(pinKey), version.ToString() };
    }

    Pin Pin::CreatePinningPin(PinKey&& pinKey, Version version)
    {
        return { PinType::Pinning, std::move(pinKey), version.ToString() };
    }

    Pin Pin::CreateGatingPin(PinKey&& pinKey, GatedVersion gatedVersion)
    {
        return { PinType::Gating, std::move(pinKey), gatedVersion.ToString() };
    }

    PinType Pin::GetType() const
    {
        return m_type;
    }

    const PinKey& Pin::GetKey() const
    {
        return m_key;
    }

    const AppInstaller::Manifest::Manifest::string_t& Pin::GetPackageId() const 
    {
        return m_key.PackageId;
    }

    std::string_view Pin::GetSourceId() const
    {
        return m_key.SourceId;
    }

    std::string_view Pin::GetVersionString() const
    {
        return m_versionString;
    }

    bool Pin::operator==(const Pin& other) const
    {
        return m_type == other.m_type &&
            m_key == other.m_key &&
            m_versionString == other.m_versionString;
    }
}