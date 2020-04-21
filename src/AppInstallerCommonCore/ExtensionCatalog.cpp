// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "winget/ExtensionCatalog.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Deployment
{
    ExtensionCatalog::ExtensionCatalog(std::wstring_view extensionName)
    {
        m_catalog = winrt::Windows::ApplicationModel::AppExtensions::AppExtensionCatalog::Open(winrt::hstring(extensionName));
    }
}
