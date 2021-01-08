// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerArchitecture.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    namespace
    {
        void AddArchitectureIfGuestMachineSupported(std::vector<Architecture>& target, Architecture architecture, USHORT guestMachine)
        {
            BOOL supported = FALSE;
            LOG_IF_FAILED(IsWow64GuestMachineSupported(guestMachine, &supported));
            
            if (supported)
            {
                target.push_back(architecture);
            }
        }

        // Gets the applicable architectures for the current machine.
        std::vector<Architecture> CreateApplicableArchitecturesVector()
        {
            std::vector<Architecture> applicableArchs;

            switch (GetSystemArchitecture())
            {
            case Architecture::Arm64:
                applicableArchs.push_back(Architecture::Arm64);
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::Arm, IMAGE_FILE_MACHINE_ARMNT);
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::X86, IMAGE_FILE_MACHINE_I386);
                applicableArchs.push_back(Architecture::Neutral);
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
                AddArchitectureIfGuestMachineSupported(applicableArchs, Architecture::X86, IMAGE_FILE_MACHINE_I386);
                applicableArchs.push_back(Architecture::Neutral);
                break;
            default:
                applicableArchs.push_back(Architecture::Neutral);
            }

            return applicableArchs;
        }
    }

    Architecture ConvertToArchitectureEnum(const std::string& archStr)
    {
        std::string arch = ToLower(archStr);
        if (arch == "x86")
        {
            return Architecture::X86;
        }
        else if (arch == "x64")
        {
            return Architecture::X64;
        }
        else if (arch == "arm")
        {
            return Architecture::Arm;
        }
        else if (arch == "arm64")
        {
            return Architecture::Arm64;
        }
        else if (arch == "neutral")
        {
            return Architecture::Neutral;
        }

        AICLI_LOG(YAML, Info, << "ConvertToArchitectureEnum: Unknown architecture: " << archStr);
        return Architecture::Unknown;
    }

    std::string_view ToString(Architecture architecture)
    {
        switch (architecture)
        {
        case Architecture::Neutral:
            return "Neutral"sv;
        case Architecture::X86:
            return "X86"sv;
        case Architecture::X64:
            return "X64"sv;
        case Architecture::Arm:
            return "Arm"sv;
        case Architecture::Arm64:
            return "Arm64"sv;
        }

        return "Unknown"sv;
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

    const std::vector<Architecture>& GetApplicableArchitectures()
    {
        static std::vector<Architecture> applicableArchs = CreateApplicableArchitecturesVector();
        return applicableArchs;
    }

    int IsApplicableArchitecture(Architecture arch)
    {
        const std::vector<Architecture>& applicableArchs = GetApplicableArchitectures();
        auto it = std::find(applicableArchs.begin(), applicableArchs.end(), arch);

        if (it != applicableArchs.end())
        {
            return static_cast<int>(std::distance(it, applicableArchs.end()));
        }
        else
        {
            return InapplicableArchitecture;
        }
    }
}