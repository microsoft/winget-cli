// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "OSInfo.h"

#ifdef _WIN32

#include <windows.h>

#include <sysinfoapi.h>

#elif __linux__

#include <sys/utsname.h>

#endif

using namespace SFS::details;

namespace
{
#ifdef _WIN32
std::string GetPlatform()
{
    return "Windows";
}

std::string GetOSMachineInfo()
{
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);

    switch (systemInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    default:
        return "Unknown";
    }
}

#elif __linux__

std::string GetPlatform()
{
    // Return kernel name, usually "Linux"
    utsname buf;
    if (uname(&buf) >= 0)
    {
        return buf.sysname;
    }
    return "Unknown";
}

std::string GetOSMachineInfo()
{
    utsname buf;
    if (uname(&buf) >= 0)
    {
        return buf.machine;
    }
    return "Unknown";
}

#else
#error "Unsupported platform"
#endif
} // namespace

std::string osinfo::GetPlatform()
{
    return ::GetPlatform();
}

std::string osinfo::GetOSMachineInfo()
{
    return ::GetOSMachineInfo();
}
