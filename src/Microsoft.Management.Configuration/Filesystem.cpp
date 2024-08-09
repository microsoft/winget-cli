// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Filesystem.h"

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace anon
    {
        constexpr std::string_view s_Configuration_LocalState = "Configuration"sv;
    }

    AppInstaller::Filesystem::PathDetails GetPathDetailsFor(PathName path, bool forDisplay)
    {
        AppInstaller::Filesystem::PathDetails result;
        // We should not create directories by default when they are retrieved for display purposes.
        result.Create = !forDisplay;

        switch (path)
        {
        case PathName::LocalState:
            result = GetPathDetailsFor(AppInstaller::Filesystem::PathName::UnpackagedLocalStateRoot, forDisplay);
            result.Path /= anon::s_Configuration_LocalState;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }
}
