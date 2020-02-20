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
    static constexpr std::string_view ARG_QUERY = "query"sv;
    static constexpr std::string_view ARG_ID = "id"sv;
    static constexpr std::string_view ARG_NAME = "name"sv;
    static constexpr std::string_view ARG_MONIKER = "moniker"sv;
    static constexpr std::string_view ARG_TAG = "tag"sv;
    static constexpr std::string_view ARG_COMMAND = "command"sv;
    static constexpr std::string_view ARG_SOURCE = "source"sv;
    static constexpr std::string_view ARG_COUNT = "count"sv;
    static constexpr std::string_view ARG_EXACT = "exact"sv;
    static constexpr std::string_view ARG_VERSION = "version"sv;
    static constexpr std::string_view ARG_CHANNEL = "channel"sv;
}