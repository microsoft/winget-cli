// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Icon.g.cpp"
#include "Icon.h"
#include "Converters.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void Icon::Initialize(::AppInstaller::Manifest::Icon icon)
    {
        m_icon = std::move(icon);
    }
    hstring Icon::Url()
    {
        return winrt::to_hstring(m_icon.Url);
    }
    IconFileType Icon::FileType()
    {
        return GetDeploymentIconFileType(m_icon.FileType);
    }
    IconResolution Icon::Resolution()
    {
        return GetDeploymentIconResolution(m_icon.Resolution);
    }
    IconTheme Icon::Theme()
    {
        return GetDeploymentIconTheme(m_icon.Theme);
    }
    com_array<uint8_t> Icon::Sha256()
    {
        return com_array<uint8_t>(m_icon.Sha256);
    }
}
