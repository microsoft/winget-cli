// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>

using namespace winrt::Windows::ApplicationModel::Resources;
using namespace winrt::Windows::ApplicationModel::Resources::Core;

namespace AppInstaller::CLI
{
    class Resources
    {
    public:
        static const Resources& GetInstance();

        std::string ResolveWingetString(
            std::wstring_view resKey) const;

        std::wstring ResolveWingetWString(
            std::wstring_view resKey) const;

    private:
        winrt::Windows::ApplicationModel::Resources::ResourceLoader m_wingetLoader;

        Resources();
    };
}