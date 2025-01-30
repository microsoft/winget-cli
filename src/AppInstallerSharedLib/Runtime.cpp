// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/winget/Runtime.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"


namespace AppInstaller::Runtime
{
    using namespace Utility;

    namespace
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_PreviewBuildSuffix = "-preview"sv;

        // Gets a boolean indicating whether the current process has identity.
        bool DoesCurrentProcessHaveIdentity()
        {
            UINT32 length = 0;
            LONG result = ::GetPackageFamilyName(GetCurrentProcess(), &length, nullptr);
            return (result != APPMODEL_ERROR_NO_PACKAGE);
        }

        std::unique_ptr<byte[]> GetPACKAGE_ID()
        {
            UINT32 bufferLength = 0;
            LONG gcpiResult = GetCurrentPackageId(&bufferLength, nullptr);
            THROW_HR_IF(E_UNEXPECTED, gcpiResult != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(bufferLength);

            gcpiResult = GetCurrentPackageId(&bufferLength, buffer.get());
            if (FAILED_WIN32_LOG(gcpiResult))
            {
                return {};
            }

            return buffer;
        }

        // Gets the package name; only succeeds if running in a packaged context.
        std::string GetPackageName()
        {
            std::unique_ptr<byte[]> buffer = GetPACKAGE_ID();
            if (!buffer)
            {
                return {};
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            return Utility::ConvertToUTF8(packageId->name);
        }

        // Gets the package version; only succeeds if running in a packaged context.
        std::optional<PACKAGE_VERSION> GetPACKAGE_VERSION()
        {
            std::unique_ptr<byte[]> buffer = GetPACKAGE_ID();
            if (!buffer)
            {
                return {};
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            return packageId->version;
        }
    }

    bool IsRunningInPackagedContext()
    {
        static bool result = DoesCurrentProcessHaveIdentity();
        return result;
    }

    LocIndString GetClientVersion()
    {
        std::ostringstream strstr;
        strstr << VERSION_MAJOR << '.' << VERSION_MINOR << '.' << VERSION_BUILD;

        if (!IsReleaseBuild())
        {
            strstr << s_PreviewBuildSuffix;
        }

        return LocIndString{ strstr.str() };
    }

    std::wstring GetPackageFamilyName()
    {
        UINT32 length = 0;
        LONG returnValue = ::GetPackageFamilyName(GetCurrentProcess(), &length, nullptr);

        if (returnValue == APPMODEL_ERROR_NO_PACKAGE)
        {
            return {};
        }

        if (returnValue != ERROR_INSUFFICIENT_BUFFER)
        {
            THROW_IF_WIN32_ERROR(returnValue);
        }

        std::wstring result(length, '\0');
        returnValue = ::GetPackageFamilyName(GetCurrentProcess(), &length, &result[0]);
        THROW_IF_WIN32_ERROR(returnValue);
        THROW_HR_IF(E_UNEXPECTED, length == 0);

        result.resize(length - 1);
        return result;
    }

    LocIndString GetPackageVersion()
    {
        using namespace std::string_literals;

        if (IsRunningInPackagedContext())
        {
            auto version = GetPACKAGE_VERSION();

            if (!version)
            {
                // In the extremely unlikely event of a failure, this is merely a sentinel value
                // to indicated such.  The only other option is to completely prevent execution,
                // which seems unnecessary.
                return LocIndString{ "error"sv };
            }

            std::ostringstream strstr;
            strstr << GetPackageName() << " v" << version->Major << '.' << version->Minor << '.' << version->Build << '.' << version->Revision;

            return LocIndString{ strstr.str() };
        }
        else
        {
            // Calling code should avoid calling in when this is the case.
            return LocIndString{ "none"sv };
        }
    }

    LocIndString GetOSVersion()
    {
        winrt::Windows::System::Profile::AnalyticsInfo analyticsInfo{};
        auto versionInfo = analyticsInfo.VersionInfo();

        uint64_t version = std::stoull(Utility::ConvertToUTF8(versionInfo.DeviceFamilyVersion()));
        uint16_t parts[4];

        for (size_t i = 0; i < ARRAYSIZE(parts); ++i)
        {
            parts[i] = version & 0xFFFF;
            version = version >> 16;
        }

        std::ostringstream strstr;
        strstr << Utility::ConvertToUTF8(versionInfo.DeviceFamily()) << " v" << parts[3] << '.' << parts[2] << '.' << parts[1] << '.' << parts[0];

        return LocIndString{ strstr.str() };
    }

    std::string GetOSRegion()
    {
        winrt::Windows::Globalization::GeographicRegion region;
        return Utility::ConvertToUTF8(region.CodeTwoLetter());
    }

    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version)
    {
        DWORD versionParts[3] = {};

        for (size_t i = 0; i < ARRAYSIZE(versionParts) && i < version.GetParts().size(); ++i)
        {
            versionParts[i] = static_cast<DWORD>(std::min(static_cast<decltype(version.GetParts()[i].Integer)>(std::numeric_limits<DWORD>::max()), version.GetParts()[i].Integer));
        }

        OSVERSIONINFOEXW osVersionInfo{};
        osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
        osVersionInfo.dwMajorVersion = versionParts[0];
        osVersionInfo.dwMinorVersion = versionParts[1];
        osVersionInfo.dwBuildNumber = versionParts[2];
        osVersionInfo.wServicePackMajor = 0;
        osVersionInfo.wServicePackMinor = 0;

        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR;

        DWORDLONG conditions = 0;
        VER_SET_CONDITION(conditions, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_MINORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

        BOOL result = VerifyVersionInfoW(&osVersionInfo, mask, conditions);
        if (!result)
        {
            THROW_LAST_ERROR_IF(GetLastError() != ERROR_OLD_WIN_VERSION);
        }
        return !!result;
    }

    bool IsRunningAsAdmin()
    {
        return wil::test_token_membership(nullptr, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
    }

    bool IsRunningAsSystem()
    {
        return wil::test_token_membership(nullptr, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
    }

    bool IsRunningAsAdminOrSystem()
    {
        return IsRunningAsAdmin() || IsRunningAsSystem();
    }

    bool IsRunningWithLimitedToken()
    {
        return wil::get_token_information<TOKEN_ELEVATION_TYPE>() == TokenElevationTypeLimited;
    }
}
