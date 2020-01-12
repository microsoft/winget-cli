// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerArchitecture.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    static const std::set<Architecture> ApplicableArchitectures = GetApplicableArchitectures();

    Architecture ConvertToArchitectureEnum(const std::string& archStr)
    {
        if (ToLower(archStr) == "x86")
        {
            return Architecture::x86;
        }
        else if (ToLower(archStr) == "x64")
        {
            return Architecture::x64;
        }
        if (ToLower(archStr) == "arm")
        {
            return Architecture::arm;
        }
        else if (ToLower(archStr) == "arm64")
        {
            return Architecture::arm64;
        }
        if (ToLower(archStr) == "neutral")
        {
            return Architecture::neutral;
        }
        else
        {
            return Architecture::unknown;
        }
    }

    std::set<Architecture> GetApplicableArchitectures()
    {
        std::set<Architecture> applicableArchs;

        switch (Runtime::GetSystemArchitecture())
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