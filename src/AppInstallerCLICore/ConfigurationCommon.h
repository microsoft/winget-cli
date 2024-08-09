// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include <winrt/Microsoft.Management.Configuration.h>

namespace AppInstaller::CLI
{
    namespace Configuration
    {
        // Validates common arguments between configuration commands.
        void ValidateCommonArguments(Execution::Args& execArgs, bool requireConfigurationSetChoice = false);

        // Sets the module path to install modules in the set processor.
        void SetModulePath(Execution::Context& context, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory const& factory);
    }
}
