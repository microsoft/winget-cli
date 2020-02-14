// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "pch.h"
using namespace std::string_view_literals;

namespace AppInstaller::CLI
{
    static constexpr std::string_view ARG_APPLICATION = "application"sv;
    static constexpr std::string_view ARG_MANIFEST = "manifest"sv;
    static constexpr std::string_view ARG_INTERACTIVE = "interactive"sv;
    static constexpr std::string_view ARG_SILENT = "silent"sv;
    static constexpr std::string_view ARG_LANGUAGE = "language"sv;
    static constexpr std::string_view ARG_LOG = "log"sv;
    static constexpr std::string_view ARG_OVERRIDE = "override"sv;
    static constexpr std::string_view ARG_INSTALLLOCATION = "installlocation"sv;
}