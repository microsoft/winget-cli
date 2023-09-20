// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ResumeCommand final : public Command
    {
        ResumeCommand(std::string_view parent) : Command("resume", {}, parent, Visibility::Hidden, Settings::ExperimentalFeature::Feature::Resume) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
