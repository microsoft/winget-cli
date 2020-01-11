// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerUtility.h"

namespace AppInstaller::Utility
{
    Runtime::Architecture ConvertToArchitectureEnum(const std::string& archStr)
    {
        if (ToLower(archStr) == "x86")
        {
            return Runtime::Architecture::x86;
        }
        else if (ToLower(archStr) == "x64")
        {
            return Runtime::Architecture::x64;
        }
        if (ToLower(archStr) == "arm")
        {
            return Runtime::Architecture::arm;
        }
        else if (ToLower(archStr) == "arm64")
        {
            return Runtime::Architecture::arm64;
        }
        if (ToLower(archStr) == "neutral")
        {
            return Runtime::Architecture::neutral;
        }
        else
        {
            return Runtime::Architecture::unknown;
        }
    }
}