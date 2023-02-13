// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/WindowsFeature.h"
#include "Public/AppInstallerLogging.h"

namespace AppInstaller::WindowsFeatures
{
    namespace
    {
        struct WindowsFeatureHelper
        {
            WindowsFeatureHelper()
            {
                m_module.reset(LoadLibraryEx(L"dismapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
                if (!m_module)
                {
                    AICLI_LOG(Core, Verbose, << "Could not load dismapi.dll");
                    return;
                }



            }
        private:
            wil::unique_hmodule m_module;
        };
    }
}