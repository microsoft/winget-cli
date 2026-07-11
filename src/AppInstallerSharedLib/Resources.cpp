// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Resources.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerErrors.h"

namespace AppInstaller
{
    namespace Resource
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

        // Utility class to load resources
        struct Loader
        {
            // Gets the singleton instance of the resource loader.
            static const Loader& Instance()
            {
                static Loader instance;
                return instance;
            }

            // Gets the string resource value.
            std::string ResolveString(std::wstring_view resKey) const
            {
                if (resKey.empty())
                {
                    return {};
                }

                if (m_wingetLoader)
                {
                    return Utility::ConvertToUTF8(m_wingetLoader.GetString(resKey));
                }

                // Loader failed to load resource file, print the resource key instead.
                return Utility::ConvertToUTF8(resKey);
            }

            // Gets the string resource value or nothing if not present.
            std::optional<Resource::LocString> TryResolveString(std::wstring_view resKey) const
            {
                if (!resKey.empty() && m_wingetLoader)
                {
                    try
                    {
                        winrt::hstring result = m_wingetLoader.GetString(resKey);

                        if (!result.empty())
                        {
                            return Resource::LocString{ Utility::LocIndString{ Utility::ConvertToUTF8(result) } };
                        }
                    }
                    CATCH_LOG()
                }

                return {};
            }

        private:
            winrt::Windows::ApplicationModel::Resources::ResourceLoader m_wingetLoader;

            Loader() : m_wingetLoader(nullptr)
            {
                try
                {
                    // The default constructor of ResourceLoader throws a winrt::hresult_error exception
                    // when resource.pri is not found. ResourceLoader::GetForViewIndependentUse also throws
                    // a winrt::hresult_error but for reasons unknown it only gets caught when running on the
                    // debugger. Running without a debugger will result in a crash that not even adding a
                    // catch all will fix. To provide a good error message we call the default constructor
                    // before calling GetForViewIndependentUse.
                    m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader();
                    m_wingetLoader = winrt::Windows::ApplicationModel::Resources::ResourceLoader::GetForViewIndependentUse(L"winget");
                }
                catch (const winrt::hresult_error& hre)
                {
                    // This message cannot be localized.
                    AICLI_LOG(CLI, Error, << "Failure loading resource file with error: " << hre.code());
                    m_wingetLoader = nullptr;
                }
            }
        };
    }

    namespace StringResource
    {
        std::string StringId::Resolve() const
        {
            return Resource::Loader::Instance().ResolveString(*this);
        }

        std::ostream& operator<<(std::ostream& out, StringId si)
        {
            return (out << Resource::LocString{ si });
        }

        std::optional<Resource::LocString> TryResolveString(std::wstring_view resKey)
        {
            return Resource::Loader::Instance().TryResolveString(resKey);
        }
    }
}
