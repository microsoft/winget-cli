// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Filesystem
{
    void CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link);

    DWORD CopyFileWithProgressCallback(const std::filesystem::path& from, const std::filesystem::path& to, IProgressCallback& progress);
}