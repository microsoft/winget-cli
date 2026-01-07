// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::Debugging
{
    // Enables a self initiated minidump on certain process level failures.
    // Only the first call to EnableSelfInitiatedMinidump has any effect.
    void EnableSelfInitiatedMinidump();

    // Enables a self initiated minidump on certain process level failures.
    // Creates the minidump in the given location.
    // Only the first call to EnableSelfInitiatedMinidump has any effect.
    void EnableSelfInitiatedMinidump(const std::filesystem::path& filePath);

    // Forces the minidump to be written.
    void WriteMinidump();
}
