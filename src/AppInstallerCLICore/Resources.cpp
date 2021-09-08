// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"

using namespace AppInstaller::Utility::literals;
using namespace winrt;

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
        catch (hresult_error const& e)
        {
            AICLI_LOG(CLI, Error, << "Failure loading resource file with error: " << e.to_abi());
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
