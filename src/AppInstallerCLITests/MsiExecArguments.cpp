// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/MsiExecArguments.h>
#include <AppInstallerErrors.h>

using namespace std::string_view_literals;
using namespace AppInstaller;

TEST_CASE("MsiExecArgs_ParseEmpty", "[msiexec]")
{
    std::vector<std::string_view> emptyArguments = { ""sv, "  ", "\t" };

    for (const auto argString : emptyArguments)
    {
        auto args = Msi::ParseMSIArguments(argString);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }
}

TEST_CASE("MsiExecArgs_ParseUILevel", "[msiexec]")
{
    {
        auto args = Msi::ParseMSIArguments("/qn"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_NONE | INSTALLUILEVEL_UACONLY));
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/qb+"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_BASIC | INSTALLUILEVEL_ENDDIALOG));
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/q"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_NONE | INSTALLUILEVEL_UACONLY));
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/qr"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_REDUCED));
        REQUIRE(args.Properties.empty());
    }

    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("/qr-"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("/q arg"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
}

TEST_CASE("MsiExecArgs_ParseLogMode", "[msiexec]")
{
    {
        auto args = Msi::ParseMSIArguments("/l file.txt"sv);
        REQUIRE(args.LogMode == Msi::DefaultLogMode);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"file.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/le errors.txt"sv);
        REQUIRE(args.LogMode == INSTALLLOGMODE_ERROR);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"errors.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/l! flush.txt"sv);
        REQUIRE(args.LogMode == Msi::DefaultLogMode);
        REQUIRE(args.LogAttributes == INSTALLLOGATTRIBUTES_FLUSHEACHLINE);
        REQUIRE(args.LogFile == L"flush.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/l\"i\" \"quoted path.txt\""sv);
        REQUIRE(args.LogMode == INSTALLLOGMODE_INFO);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"quoted path.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/liwpx+! log.txt"sv);
        REQUIRE(args.LogMode == (INSTALLLOGMODE_INFO | INSTALLLOGMODE_WARNING | INSTALLLOGMODE_PROPERTYDUMP | INSTALLLOGMODE_EXTRADEBUG));
        REQUIRE(args.LogAttributes == (INSTALLLOGATTRIBUTES_FLUSHEACHLINE | INSTALLLOGATTRIBUTES_APPEND));
        REQUIRE(args.LogFile == L"log.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/l* all.txt"sv);
        REQUIRE(args.LogMode == Msi::AllLogMode);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"all.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/l* \"without closing quote.txt"sv);
        REQUIRE(args.LogMode == Msi::AllLogMode);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"without closing quote.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("/l"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("/lz log.txt"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
}

TEST_CASE("MsiExecArgs_ParseProperties", "[msiexec]")
{
    {
        auto args = Msi::ParseMSIArguments("PROPERTY=value"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" PROPERTY=value"sv);
    }

    {
        auto args = Msi::ParseMSIArguments("EMPTY="sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" EMPTY="sv);
    }

    {
        auto args = Msi::ParseMSIArguments("PROPERTY=\"quoted value\""sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" PROPERTY=\"quoted value\""sv);
    }

    {
        auto args = Msi::ParseMSIArguments("PROPERTY=\"escaped \"\" quotes\""sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" PROPERTY=\"escaped \"\" quotes\""sv);
    }

    {
        auto args = Msi::ParseMSIArguments("PROPERTY1=value1       PROPERTY2=value2"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" PROPERTY1=value1 PROPERTY2=value2"sv);
    }

    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("NOSEPARATOR"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("$NOTAPROPERTY=value"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("PROPERTY=not quoted"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("PROPERTY=\"bad \"internal\" quotes\""sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
    REQUIRE_THROWS_HR(Msi::ParseMSIArguments("PROPERTY=\"mismatched quote"sv), APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT);
}

TEST_CASE("MsiExecArgs_ParseMultipleOptions", "[msiexec]")
{
    {
        auto args = Msi::ParseMSIArguments("/li first.txt /le second.txt"sv);
        REQUIRE(args.LogMode == INSTALLLOGMODE_ERROR);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"second.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("PROPERTY1=value1 /qb  PROPERTY2= /lw file.txt"sv);
        REQUIRE(args.LogMode == INSTALLLOGMODE_WARNING);
        REQUIRE(args.LogAttributes == 0);
        REQUIRE(args.LogFile == L"file.txt"sv);
        REQUIRE(args.UILevel == INSTALLUILEVEL_BASIC);
        REQUIRE(args.Properties == L" PROPERTY1=value1 PROPERTY2=");
    }
}

TEST_CASE("MsiExecArgs_ParseLongOptions", "[msiexec]")
{
    {
        auto args = Msi::ParseMSIArguments("/quiet"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_NONE | INSTALLUILEVEL_UACONLY));
        REQUIRE(args.Properties.empty());
    }

    {
        auto args = Msi::ParseMSIArguments("/passive"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == (INSTALLUILEVEL_BASIC | INSTALLUILEVEL_PROGRESSONLY | INSTALLUILEVEL_HIDECANCEL));
        REQUIRE(args.Properties == L" REBOOTPROMPT=S"sv);
    }

    {
        auto args = Msi::ParseMSIArguments("/NoRestart"sv);
        REQUIRE(!args.LogFile.has_value());
        REQUIRE(args.UILevel == INSTALLUILEVEL_DEFAULT);
        REQUIRE(args.Properties == L" REBOOT=ReallySuppress"sv);
    }
}