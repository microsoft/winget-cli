// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ConfigureExportCommand final : public Command
    {
        ConfigureExportCommand(std::string_view parent) : Command("export", parent, Settings::ExperimentalFeature::Feature::ConfigureExport) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
    };
}
