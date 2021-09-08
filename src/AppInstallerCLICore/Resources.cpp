// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"

using namespace AppInstaller::Utility::literals;

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
        try
        {
            m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse(L"winget");
        }
        catch (const winrt::hresult_error& hre)
        {
            // This message cannot be localized.
            AICLI_LOG(CLI, Error, << "Failure loading resource file with error: " << hre.code());
            throw;
        }
    }

    std::string Loader::ResolveString(
        std::wstring_view resKey) const
    {
        return Utility::ConvertToUTF8(m_wingetLoader.GetString(resKey));
    }

    Utility::LocIndView GetFixedString(FixedString fs)
    {
        switch (fs)
        {
        case FixedString::ProductName: return "Windows Package Manager"_liv;
        }

        THROW_HR(E_UNEXPECTED);
    }
}
