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

    std::string GetClientVersion()
    {
        using namespace std::string_literals;

        if (IsRunningInPackagedContext())
        {
            UINT32 bufferLength = 0;
            LONG gcpiResult = GetCurrentPackageId(&bufferLength, nullptr);
            THROW_HR_IF(E_UNEXPECTED, gcpiResult != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(bufferLength);

            gcpiResult = GetCurrentPackageId(&bufferLength, buffer.get());
            if (FAILED_WIN32_LOG(gcpiResult))
            {
                return "error"s;
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            PACKAGE_VERSION& version = packageId->version;

            std::ostringstream strstr;
            strstr << version.Major << '.' << version.Minor << '.' << version.Build << '.' << version.Revision;

            return strstr.str();
        }
        else
        {
            return "unknown"s;
        }
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
