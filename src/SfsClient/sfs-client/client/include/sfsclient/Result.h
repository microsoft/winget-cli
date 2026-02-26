// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace SFS
{
class Result
{
  public:
    enum Code : uint32_t
    {
        Success = 0x00000000,

        //
        // Failure codes
        //

        // Generic errors start at 0x8000'0000
        InvalidArg = 0x8000'0001,
        NotImpl = 0x8000'0002,
        NotSet = 0x8000'0003,
        OutOfMemory = 0x8000'0004,
        Unexpected = 0x8000'0005,

        // Connection errors start at 0x8000'1000
        ConnectionSetupFailed = 0x8000'1000,
        ConnectionUnexpectedError = 0x8000'1001,
        ConnectionUrlSetupFailed = 0x8000'1002,

        // Http Errors start at 0x8000'2000
        // Generic Http errors
        HttpTimeout = 0x8000'2000,
        HttpUnexpected = 0x8000'2001,

        // Last 3 digits mapped to Http status codes
        HttpBadRequest = 0x8000'2400,
        HttpNotFound = 0x8000'2404,
        HttpMethodNotAllowed = 0x8000'2405,
        HttpTooManyRequests = 0x8000'2429,
        HttpServiceNotAvailable = 0x8000'2503,

        // Service errors start at 0x8000'3000
        ServiceInvalidResponse = 0x8000'3000,
        ServiceUnexpectedContentType = 0x8000'3001,
    };

    Result(Code code) noexcept;
    Result(Code code, std::string message) noexcept;

    Code GetCode() const noexcept;

    /// @brief Returns the message associated with the result code.
    /// @note "GetMsg" is used instead of "GetMessage" to avoid conflicts with Windows API.
    const std::string& GetMsg() const noexcept;

    bool IsSuccess() const noexcept;
    bool IsFailure() const noexcept;

    /**
     * @brief Returns true if the result code represents a successful operation.
     */
    operator bool() const noexcept;

    bool operator==(Code resultCode) const noexcept;
    bool operator!=(Code resultCode) const noexcept;

    // Results should not be compared directly
    bool operator==(Result) const = delete;
    bool operator!=(Result) const = delete;
    bool operator>(Result) const = delete;
    bool operator>=(Result) const = delete;
    bool operator<(Result) const = delete;
    bool operator<=(Result) const = delete;

  private:
    Code m_code;
    std::string m_message;
};

std::string_view ToString(Result::Code code) noexcept;
} // namespace SFS

std::ostream& operator<<(std::ostream& os, const SFS::Result& value);
std::ostream& operator<<(std::ostream& os, const SFS::Result::Code& value);
