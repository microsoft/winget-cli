// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRuntime.h"

#include <winrt/Windows.Storage.h>

namespace AppInstaller::Runtime
{
    static const std::set<Architecture> ApplicableArchitectures = GetApplicableArchitectures();

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

    Architecture GetSystemArchitecture()
    {
        Architecture systemArchitecture = Architecture::unknown;

        static SYSTEM_INFO systemInfo;
        ZeroMemory(&systemInfo, sizeof(SYSTEM_INFO));
        GetNativeSystemInfo(&systemInfo);

        switch (systemInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:
        case PROCESSOR_ARCHITECTURE_IA64:
            systemArchitecture = Architecture::x64;
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            systemArchitecture = Architecture::arm;
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            systemArchitecture = Architecture::arm64;
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            systemArchitecture = Architecture::x86;
            break;
        }

        return systemArchitecture;
    }

    std::set<Architecture> GetApplicableArchitectures()
    {
        std::set<Architecture> applicableArchs;
        
        switch (GetSystemArchitecture())
        {
        case Architecture::arm64:
            applicableArchs.insert(Architecture::arm64);
            applicableArchs.insert(Architecture::x86);
            applicableArchs.insert(Architecture::arm);
            applicableArchs.insert(Architecture::neutral);
            break;
        case Architecture::arm:
            applicableArchs.insert(Architecture::arm);
            applicableArchs.insert(Architecture::neutral);
            break;
        case Architecture::x86:
            applicableArchs.insert(Architecture::x86);
            applicableArchs.insert(Architecture::neutral);
            break;
        case Architecture::x64:
            applicableArchs.insert(Architecture::x86);
            applicableArchs.insert(Architecture::x64);
            applicableArchs.insert(Architecture::neutral);
            break;
        }

        return applicableArchs;
    }

    bool IsApplicableArchitecture(Architecture arch)
    {
        return ApplicableArchitectures.find(arch) != ApplicableArchitectures.end();
    }
}
