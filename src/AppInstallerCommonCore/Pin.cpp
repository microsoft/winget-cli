// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Pin.h"

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
        case PinType::Unknown:
        default:
            return "Unknown";
        }
    }

    Pin Pin::CreateBlockingPin(PinKey&& pinKey)
    {
        return { PinType::Blocking, std::move(pinKey) };
    }

    Pin Pin::CreatePinningPin(PinKey&& pinKey)
    {
        return { PinType::Pinning, std::move(pinKey) };
    }

    Pin Pin::CreateGatingPin(PinKey&& pinKey, AppInstaller::Utility::GatedVersion gatedVersion)
    {
        return { PinType::Gating, std::move(pinKey), gatedVersion };
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

    AppInstaller::Utility::GatedVersion Pin::GetGatedVersion() const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, m_type != PinType::Gating);
        return m_gatedVersion.value();
    }

    bool Pin::operator==(const Pin& other) const
    {
        if (m_type != other.m_type || m_key != other.m_key)
        {
            return false;
        }

        if (m_type != PinType::Gating)
        {
            return false;
        }

        return m_gatedVersion == other.m_gatedVersion;
    }
}