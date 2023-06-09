// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct DownloadCommand final : public Command
    {
        DownloadCommand(std::string_view parent) : Command("download", {} /* aliases */, parent, Settings::ExperimentalFeature::Feature::Download) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
