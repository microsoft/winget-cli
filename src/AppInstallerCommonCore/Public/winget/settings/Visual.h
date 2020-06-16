// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json.h>

namespace AppInstaller::Settings
{
    // The visual style of the progress bar.
    enum class VisualStyle
    {
        NoVT,
        Retro,
        Accent,
        Rainbow,
    };

    struct Visual final
    {
        Visual(const Json::Value& node);
        virtual ~Visual() = default;

        Visual(const Visual&) = default;
        Visual& operator=(const Visual&) = default;

        Visual(Visual&&) = default;
        Visual& operator=(Visual&&) = default;

        std::vector<std::string> Warnings() const { return m_warnings; }

        VisualStyle GetProgressBarVisualStyle() const noexcept { return m_progressBar; }

        static std::string_view GetPropertyName();

    private:
        VisualStyle m_progressBar;

        std::vector<std::string> m_warnings;
    };
}
