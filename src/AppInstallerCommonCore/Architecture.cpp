// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerArchitecture.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    Architecture ConvertToArchitectureEnum(const std::string& archStr)
    {
        if (ToLower(archStr) == "x86")
        {
            return Architecture::X86;
        }
        else if (ToLower(archStr) == "x64")
        {
            return Architecture::X64;
        }
        if (ToLower(archStr) == "arm")
        {
            return Architecture::Arm;
        }
        else if (ToLower(archStr) == "arm64")
        {
            return Architecture::Arm64;
        }
        else if (ToLower(archStr) == "neutral")
        {
            return Architecture::Neutral;
        }

        AICLI_LOG(YAML, Info, << "Convert to architecture enum. Unknown architecture: " << archStr);
        return Architecture::Unknown;
    }

    Architecture GetSystemArchitecture()
    {
        Architecture systemArchitecture = Architecture::Unknown;

        SYSTEM_INFO systemInfo;
        ZeroMemory(&systemInfo, sizeof(SYSTEM_INFO));
        GetNativeSystemInfo(&systemInfo);

        switch (systemInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:
        case PROCESSOR_ARCHITECTURE_IA64:
            systemArchitecture = Architecture::X64;
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            systemArchitecture = Architecture::Arm;
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            systemArchitecture = Architecture::Arm64;
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            systemArchitecture = Architecture::X86;
            break;
        }

        return systemArchitecture;
    }

    std::vector<Architecture> GetApplicableArchitectures()
    {
        static std::vector<Architecture> applicableArchs;

        if (!applicableArchs.empty())
        {
            return applicableArchs;
        }

        switch (GetSystemArchitecture())
        {
        case Architecture::Arm64:
            applicableArchs.push_back(Architecture::Arm64);
            applicableArchs.push_back(Architecture::Neutral);
            applicableArchs.push_back(Architecture::Arm);
            applicableArchs.push_back(Architecture::X86);
            break;
        case Architecture::Arm:
            applicableArchs.push_back(Architecture::Arm);
            applicableArchs.push_back(Architecture::Neutral);
            break;
        case Architecture::X86:
            applicableArchs.push_back(Architecture::X86);
            applicableArchs.push_back(Architecture::Neutral);
            break;
        case Architecture::X64:
            applicableArchs.push_back(Architecture::X64);
            applicableArchs.push_back(Architecture::Neutral);
            applicableArchs.push_back(Architecture::X86);
            break;
        default:
            applicableArchs.push_back(Architecture::Neutral);
        }

        return applicableArchs;
    }

    int IsApplicableArchitecture(Architecture arch)
    {
        std::vector<Architecture> applicableArchs = GetApplicableArchitectures();
        auto it = std::find(applicableArchs.begin(), applicableArchs.end(), arch);

        if (it != applicableArchs.end())
        {
            return static_cast<int>(std::distance(it, applicableArchs.end()));
        }
        else
        {
            return -1;
        }
    }
}