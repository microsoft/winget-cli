// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationWingetDscModuleUnitValidation.h"
#include "ExecutionContext.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::Configuration
{
    bool WingetDscModuleUnitValidator::ValidateConfigurationSetUnit(Execution::Context& context, const ConfigurationUnit& unit)
    {

    }
}