// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Resources.h"


namespace AppInstaller::CLI::Resource
{
    LocString::LocString(StringId id) : Utility::LocIndString(Loader::Instance().ResolveString(id)) {}

    const Loader& Loader::Instance()
    {
        static Loader instance;
        return instance;
    }

    Loader::Loader()
    {
        m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse(L"winget");
    }

    std::string Loader::ResolveString(
        std::wstring_view resKey) const
    {
        return Utility::ConvertToUTF8(m_wingetLoader.GetString(resKey));
    }
}