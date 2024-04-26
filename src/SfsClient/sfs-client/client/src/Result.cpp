// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Result.h"

#include <ostream>

using namespace SFS;

Result::Result(Code code) noexcept : m_code(code)
{
}

Result::Result(Code code, std::string message) noexcept : Result(code)
{
    try
    {
        m_message = std::move(message);
    }
    catch (...)
    {
        // Ignore exception, leave message empty
    }
}

Result::Code Result::GetCode() const noexcept
{
    return m_code;
}

const std::string& Result::GetMsg() const noexcept
{
    return m_message;
}

bool Result::IsSuccess() const noexcept
{
    return GetCode() == Success;
}

bool Result::IsFailure() const noexcept
{
    return !IsSuccess();
}

Result::operator bool() const noexcept
{
    return IsSuccess();
}

bool Result::operator==(Code resultCode) const noexcept
{
    return GetCode() == resultCode;
}

bool Result::operator!=(Code resultCode) const noexcept
{
    return GetCode() != resultCode;
}

std::string_view SFS::ToString(Result::Code code) noexcept
{
    switch (code)
    {
    case Result::Success:
        return "Success";

    //
    // Failure codes
    //

    // Generic errors
    case Result::InvalidArg:
        return "InvalidArg";
    case Result::NotImpl:
        return "NotImpl";
    case Result::NotSet:
        return "NotSet";
    case Result::OutOfMemory:
        return "OutOfMemory";
    case Result::Unexpected:
        return "Unexpected";

    // Connection errors
    case Result::ConnectionSetupFailed:
        return "ConnectionSetupFailed";
    case Result::ConnectionUnexpectedError:
        return "ConnectionUnexpectedError";
    case Result::ConnectionUrlSetupFailed:
        return "ConnectionUrlSetupFailed";

    // Http Errors
    case Result::HttpTimeout:
        return "HttpTimeout";
    case Result::HttpUnexpected:
        return "HttpUnexpected";
    case Result::HttpBadRequest:
        return "HttpBadRequest";
    case Result::HttpNotFound:
        return "HttpNotFound";
    case Result::HttpMethodNotAllowed:
        return "HttpMethodNotAllowed";
    case Result::HttpTooManyRequests:
        return "HttpTooManyRequests";
    case Result::HttpServiceNotAvailable:
        return "HttpServiceNotAvailable";

    // Service errors
    case Result::ServiceInvalidResponse:
        return "ServiceInvalidResponse";
    case Result::ServiceUnexpectedContentType:
        return "ServiceUnexpectedContentType";
    }
    return "";
}

std::ostream& operator<<(std::ostream& os, const Result& result)
{
    os << ToString(result.GetCode());
    return os;
}

std::ostream& operator<<(std::ostream& os, const Result::Code& code)
{
    os << ToString(code);
    return os;
}
