// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <winerror.h>
#include <ntstatus.h>
#include <winternl.h>

namespace AppInstaller::Utility
{
    class NtStatusException final : public std::exception
    {
    public:
        NtStatusException(const char* message, NTSTATUS error) :
            m_code(static_cast<std::uint32_t>(error)),
            m_message(message) {}

        uint32_t Code() const { return m_code; }
        const std::string& Message() const { return m_message; }

    protected:
        std::uint32_t   m_code;
        std::string     m_message;
    };

    class HResultException final : public std::exception
    {
    public:
        HResultException(const char* message, HRESULT error) :
            m_code((error)),
            m_message(message) {}

        HRESULT Code() const { return m_code; }
        const std::string& Message() const { return m_message; }

    protected:
        HRESULT   m_code;
        std::string     m_message;
    };

#define ThrowStatusIfFailed(a, m)             \
    {                                             \
        NTSTATUS _status = a;                     \
        if (!NT_SUCCESS(_status))                 \
        {                                         \
            throw NtStatusException(m, _status);  \
        }                                         \
    }

#define ThrowHRIfFalse(a, m)                                      \
    {                                                                     \
        BOOL _result = a;                                                 \
        if (!_result)                                                     \
        {                                                                 \
            throw HResultException(m, HRESULT_FROM_WIN32(GetLastError()));   \
        }                                                                 \
    }
}
