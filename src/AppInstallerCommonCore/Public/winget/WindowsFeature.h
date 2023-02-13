// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::WindowsFeature
{
    bool EnableWindowsFeature(const std::string& name);

    bool DisableWindowsFeature(const std::string& name);

    bool IsWindowsFeatureEnabled(const std::string& name);

    bool DoesWindowsFeatureExist(const std::string& name);
}