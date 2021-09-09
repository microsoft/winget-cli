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

    Loader::Loader() : m_wingetLoader(nullptr)
    {
        try
        {
            // The default constructor of ResourceLoader throws a winrt::hresult_error exception
            // when resource.pri is not found. ResourceLoader::GetForViewIndependentUse also throws
            // a winrt::hresult_error but for reasons unknown it only gets catch when running on the
            // debugger. Running without a debugger will result in a crash that not even adding a
            // catch all fix. To provide a good error message we call the default constructor
            // before calling GetForViewIndependentUse.
            m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader();
            m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse(L"winget");
        }
        catch (const winrt::hresult_error& hre)
        {
            // This message cannot be localized.
            AICLI_LOG(CLI, Error, << "Failure loading resource file with error: " << hre.code());
            throw MissingResourceFileException();
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
