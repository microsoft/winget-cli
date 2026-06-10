// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <map>
#include <string>

namespace winrt::Microsoft::Management::Configuration
{
    struct ConfigurationUnit;
}

namespace AppInstaller::CLI::Execution
{
    struct Context;
}

namespace AppInstaller::CLI::Configuration
{
    using namespace std::string_view_literals;

    struct WingetDscModuleUnitValidator
    {
        bool ValidateConfigurationSetUnit(AppInstaller::CLI::Execution::Context& context, const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        std::string_view ModuleName() { return "Microsoft.WinGet.DSC"sv; };

    private:
        std::map<std::string, std::string> m_dependenciesSourceAndUnitIdMap;
    };
}
