// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    Resources& Resources::GetInstance()
    {
        static Resources instance;
        return instance;
    }

    Resources::Resources()
    {
        m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse(L"winget");
    }

    std::string Resources::ResolveWingetString(
        std::wstring_view resKey)
    {
        return Utility::ConvertToUTF8(ResolveWingetWString(resKey));
    }

    std::wstring Resources::ResolveWingetWString(
        std::wstring_view resKey)
    {
        std::wstring localized(m_wingetLoader.GetString(resKey));
        return localized;
    }
}