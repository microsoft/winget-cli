// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Locale.h"
#include "AppInstallerStrings.h"
#include "AppInstallerLogging.h"

namespace AppInstaller::Utility
{
    namespace
    {
        // We will just leak this. The module is shared as both functions will always be together.
        HMODULE g_bcp47 = (HMODULE)(-1);
        typedef bool(WINAPI* IsWellFormedTagFunc)(PCWSTR);
        typedef HRESULT(WINAPI* GetDistanceOfClosestLanguageInListFunc)(PCWSTR, PCWSTR, wchar_t, double*);

        HMODULE LoadBcp47ModuleFrom(_In_ PCWSTR moduleName)
        {
            HMODULE module = LoadLibraryExW(moduleName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (module != nullptr)
            {
                // All BCP47 APIs we are interested are always exported from the same dll together. So we just pick anyone for probe.
                IsWellFormedTagFunc func = (IsWellFormedTagFunc)(GetProcAddress(module, "IsWellFormedTag"));
                if (func != nullptr)
                {
                    return module;
                }
                FreeLibrary(module);
            }

            return nullptr;
        }

        HMODULE LoadBcp47Module()
        {
            HMODULE module = LoadBcp47ModuleFrom(L"bcp47mrm.dll");
            if (module == nullptr)
            {
                // In downlevel OS, the API is exposed by bcp47langs.dll.
                module = LoadBcp47ModuleFrom(L"bcp47langs.dll");
            }

            return module;
        }

        void InitializeBcp47Module()
        {
            HMODULE comparand = (HMODULE)(-1);
            if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&g_bcp47), comparand, comparand) == comparand)
            {
                HMODULE module = LoadBcp47Module();
                InterlockedExchangePointer(reinterpret_cast<PVOID*>(&g_bcp47), module);
            }
        }
    }

    bool IsWellFormedBcp47Tag(const std::string& bcp47Tag)
    {
        // Before new SDK is released, we need to use LoadLibrary/GetProcAddress
        InitializeBcp47Module();

        if (g_bcp47 == nullptr)
        {
            // Didn't find an implementation. Just return true.
            AICLI_LOG(Core, Warning, << "bcp47 module not found.");
            return true;
        }

        IsWellFormedTagFunc func = (IsWellFormedTagFunc)(GetProcAddress(g_bcp47, "IsWellFormedTag"));
        if (func != nullptr)
        {
            auto wBcp47Tag = ConvertToUTF16(bcp47Tag);
            return func(wBcp47Tag.c_str());
        }

        // Should not reach here.
        return TRUE;
    }

    double GetDistanceOfLanguage(const std::string& target, const std::string& available)
    {
        // Before new SDK is released, we need to use LoadLibrary/GetProcAddress
        InitializeBcp47Module();

        if (g_bcp47 == nullptr)
        {
            // Didn't find an implementation. Just return 0 as no match.
            AICLI_LOG(Core, Warning, << "bcp47 module not found.");
            return 0;
        }

        GetDistanceOfClosestLanguageInListFunc func =
            (GetDistanceOfClosestLanguageInListFunc)(GetProcAddress(g_bcp47, "GetDistanceOfClosestLanguageInList"));
        if (func != nullptr)
        {
            double distance = 0;
            auto wTarget = ConvertToUTF16(target);
            auto wAvailable = ConvertToUTF16(available);

            // Do not check HRESULT because the method returns ERROR_NO_MATCH on no match, which is a valid case.
            (void)func(wTarget.c_str(), wAvailable.c_str(), L';' /* Not used, we compare one at a time */, &distance);
            return distance;
        }

        // Should not reach here.
        return 0;
    }

    std::vector<std::string> GetUserPreferredLanguages()
    {
        std::vector<std::string> result;

        for (const auto& lang : winrt::Windows::System::UserProfile::GlobalizationPreferences::Languages())
        {
            result.emplace_back(Utility::ConvertToUTF8(lang));
        }

        return result;
    }
}
