// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRuntime.h"

#include <winrt/Windows.Storage.h>

namespace AppInstaller::Runtime
{
    namespace
    {
        bool DoesCurrentProcessHaveIdentity()
        {
            UINT32 length = 0;
            LONG result = GetPackageFamilyName(GetCurrentProcess(), &length, nullptr);
            return (result != APPMODEL_ERROR_NO_PACKAGE);
        }
    }

    bool IsRunningInPackagedContext()
    {
        static bool result = DoesCurrentProcessHaveIdentity();
        return result;
    }

    std::filesystem::path GetPathToTemp()
    {
        if (IsRunningInPackagedContext())
        {
            return { winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path().c_str() };
        }
        else
        {
            wchar_t tempPath[MAX_PATH + 1];
            DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
            return { std::wstring_view{ tempPath, static_cast<size_t>(tempChars) } };
        }
    }
}
