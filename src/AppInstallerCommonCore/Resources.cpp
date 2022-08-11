// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Resources.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"
#include "Public//AppInstallerErrors.h"

namespace AppInstaller::Resource
{
    namespace
    {
        std::pair<void*, size_t> GetResourceData(PCWSTR resourceName, PCWSTR resourceType)
        {
            HMODULE resourceModule = nullptr;
            GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<PCWSTR>(GetResourceData),
                &resourceModule);
            THROW_LAST_ERROR_IF_NULL(resourceModule);

            HRSRC resourceInfoHandle = FindResourceW(resourceModule, resourceName, resourceType);
            THROW_LAST_ERROR_IF_NULL(resourceInfoHandle);

            HGLOBAL resourceMemoryHandle = LoadResource(resourceModule, resourceInfoHandle);
            THROW_LAST_ERROR_IF_NULL(resourceMemoryHandle);

            DWORD resourceSize = SizeofResource(resourceModule, resourceInfoHandle);
            THROW_LAST_ERROR_IF(resourceSize == 0);

            void* resourceContent = LockResource(resourceMemoryHandle);
            THROW_HR_IF_NULL(E_UNEXPECTED, resourceContent);

            return std::make_pair(resourceContent, static_cast<size_t>(resourceSize));
        }
    }

    std::string_view GetResourceAsString(int resourceName, int resourceType)
    {
        return GetResourceAsString(MAKEINTRESOURCE(resourceName), MAKEINTRESOURCE(resourceType));
    }

    std::string_view GetResourceAsString(PCWSTR resourceName, PCWSTR resourceType)
    {
        auto resourceData = GetResourceData(resourceName, resourceType);
        return { reinterpret_cast<char*>(resourceData.first), resourceData.second };
    }

    std::pair<const BYTE*, size_t> GetResourceAsBytes(int resourceName, int resourceType)
    {
        return GetResourceAsBytes(MAKEINTRESOURCE(resourceName), MAKEINTRESOURCE(resourceType));
    }

    std::pair<const BYTE*, size_t> GetResourceAsBytes(PCWSTR resourceName, PCWSTR resourceType)
    {
        auto resourceData = GetResourceData(resourceName, resourceType);
        return std::make_pair(reinterpret_cast<BYTE*>(resourceData.first), resourceData.second);
    }

    LocString::LocString(StringResource::StringId id) : Utility::LocIndString(id()) {}

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
            throw ResourceOpenException(hre);
        }
    }

    std::string Loader::ResolveString(
        std::wstring_view resKey) const
    {
        return Utility::ConvertToUTF8(m_wingetLoader.GetString(resKey));
    }

    ResourceOpenException::ResourceOpenException(const winrt::hresult_error& hre)
    {
        m_message = "Could not open the resource file: " + GetUserPresentableMessage(hre);
    }
}
