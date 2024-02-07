// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Pin.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Pinning
{
    using namespace std::string_view_literals;

    namespace
    {
        // Source ID to use for the installed source; it does not match any installed source.
        // This does match with the actual ID of the source, but it doesn't really matter
        // as it is handled specially when we see it.
        constexpr std::string_view s_installedSourceId = "*PredefinedInstalledSource"sv;
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

    bool IsStricter(PinType first, PinType second)
    {
        return first > second;
    }

    PinType Stricter(PinType first, PinType second)
    {
        return IsStricter(first, second) ? first : second;
    }

    std::string PinKey::ToString() const
    {
        std::stringstream ss;
        ss << "Package=[" << PackageId << "] Source=[" << SourceId << "]";
        return ss.str();
    }

    PinKey PinKey::GetPinKeyForInstalled(std::string_view systemReferenceString)
    {
        return { systemReferenceString, s_installedSourceId };
    }

    bool PinKey::IsForInstalled() const
    {
        return SourceId == s_installedSourceId;
    }

    std::string Pin::ToString() const
    {
        std::stringstream ss;
        ss << m_key.ToString() << " Type=[" << Pinning::ToString(m_type) << ']';

        if (m_type == PinType::Gating)
        {
            ss << " GatedVersion=[" << m_gatedVersion.ToString() << ']';
        }

        return ss.str();
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