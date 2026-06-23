// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <system_error>

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace std::string_literals;

TEST_CASE("EnsureSortedErrorList", "[errors]")
{
    auto errors = Errors::GetWinGetErrors();
    for (size_t i = 1; i < errors.size(); ++i)
    {
        INFO(errors[i - 1]->Symbol() << " then " << errors[i]->Symbol());
        REQUIRE(errors[i]->Value() > errors[i - 1]->Value());
    }
}

TEST_CASE("Win32HResultMessageUsesWin32Code", "[errors]")
{
    constexpr HRESULT internetCannotConnect = HRESULT_FROM_WIN32(ERROR_INTERNET_CANNOT_CONNECT);
    const std::string message = GetUserPresentableMessage(internetCannotConnect);
    const std::string expectedSystemMessage = std::system_category().message(ERROR_INTERNET_CANNOT_CONNECT);
    const std::string hresultSystemMessage = std::system_category().message(internetCannotConnect);

    INFO(message);
    INFO(expectedSystemMessage);
    INFO(hresultSystemMessage);
    REQUIRE(message.find("0x80072efd") != std::string::npos);
    REQUIRE(message.find(expectedSystemMessage) != std::string::npos);
    REQUIRE(message.find(hresultSystemMessage) == std::string::npos);
}