#pragma once

namespace AppInstaller::Utility
{
    enum class Architecture
    {
        unknown = -1,
        neutral,
        x86,
        x64,
        arm,
        arm64
    };

    Architecture ConvertToArchitectureEnum(const std::string& archStr);

    std::set<Architecture> GetApplicableArchitectures();

    bool IsApplicableArchitecture(Architecture arch);
}