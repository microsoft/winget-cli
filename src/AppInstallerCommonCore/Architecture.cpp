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

        AICLI_LOG(YAML, Error, << "Convert to architecture enum. Unknown architecture: " << archStr);
        return Architecture::Unknown;
    }

    std::vector<Architecture> GetApplicableArchitectures()
    {
        static std::vector<Architecture> applicableArchs;

        if (!applicableArchs.empty())
        {
            return applicableArchs;
        }

        switch (Runtime::GetSystemArchitecture())
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
            return std::distance(it, applicableArchs.end());
        }
        else
        {
            return -1;
        }
    }
}