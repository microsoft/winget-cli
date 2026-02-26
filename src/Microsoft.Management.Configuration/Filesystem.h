// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Filesystem.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Paths used by configuration.
    enum class PathName
    {
        // Local state root for configuration.
        LocalState,
    };

    // Gets the PathDetails used for the given path.
    // This is exposed primarily to allow for testing, GetPathTo should be preferred.
    AppInstaller::Filesystem::PathDetails GetPathDetailsFor(PathName path, bool forDisplay = false);
}
