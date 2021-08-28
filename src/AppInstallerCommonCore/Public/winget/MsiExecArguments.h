// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <list>
#include <string_view>
#include <vector>


// This file defines parsing of the command line arguments passed to msiexec.exe.
//
// Some packages require the UAC prompt for installing even on silent installs. This
// can be done with the MSI API using the INSTALLUILEVEL_UACONLY flag, but msiexec.exe
// does not provide a way to use it. So, we use the MSI API directly instead of
// through msiexec.exe. Since msiexec.exe does some parsing of command line arguments
// before handing off to the API, we replicate that parsing here.
//
// Since we care only about installation, we simplify the parsing by assuming that
// the command line has the form
//   msiexec.exe /i (MSI file) [Other args...]

namespace AppInstaller::Msi
{
    DEFINE_ENUM_FLAG_OPERATORS(INSTALLUILEVEL);
    DEFINE_ENUM_FLAG_OPERATORS(INSTALLLOGMODE);
    DEFINE_ENUM_FLAG_OPERATORS(INSTALLLOGATTRIBUTES);

    constexpr INSTALLLOGMODE DefaultLogMode =
        INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_INFO |
        INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART | INSTALLLOGMODE_ACTIONDATA;

    // All but the four flags that always have to be set explicitly (Verbose, ExtraDebug, LogOnlyOnError, LogPerformance)
    constexpr INSTALLLOGMODE AllLogMode =
        INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_USER | INSTALLLOGMODE_INFO |
        INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART | INSTALLLOGMODE_ACTIONDATA |
        INSTALLLOGMODE_PROPERTYDUMP | INSTALLLOGMODE_COMMONDATA;

    // Arguments parsed from a command line string.
    // Arguments currently supported are:
    //   - Logging options (/l)
    //   - Quiet options (/q)
    //   - Properties (PROPERTY=Value)
    struct MsiParsedArguments
    {
        // Logging options. See: MsiEnableLog()
        INSTALLLOGMODE LogMode = {};
        std::optional<std::wstring> LogFile;
        INSTALLLOGATTRIBUTES LogAttributes = {};

        // UI options. See: MsiSetInternalUI()
        INSTALLUILEVEL UILevel = INSTALLUILEVEL_DEFAULT;

        // Properties string
        std::wstring Properties;
    };

    // Parses a command line string for msiexec.
    // This function assumes that the full command line will have the form
    //   msiexec.exe /i package.msi [arguments]
    // and that it is only parsing the [arguments] part.
    //
    // Note: This does not match msiexec exactly. It does not support options
    // unrelated to install, nor all options for install (e.g. /n).
    MsiParsedArguments ParseMSIArguments(std::string_view arguments);
}
