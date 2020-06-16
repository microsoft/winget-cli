// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json.h>

#include <chrono>

namespace AppInstaller::Settings
{
    using namespace std::chrono_literals;

    struct Source final
    {
        Source(const Json::Value& node);
        virtual ~Source() = default;

        Source(const Source&) = default;
        Source& operator=(const Source&) = default;

        Source(Source&&) = default;
        Source& operator=(Source&&) = default;

        std::vector<std::string> Warnings() const { return m_warnings; }

        uint32_t GetAutoUpdateTimeInMinutes() const noexcept { return m_autoUpdateIntervalInMinutes; }

        static std::string_view GetPropertyName();

    private:
        uint32_t m_autoUpdateIntervalInMinutes;

        std::vector<std::string> m_warnings;

    };
}
