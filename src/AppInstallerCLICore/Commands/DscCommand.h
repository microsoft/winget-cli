// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/ExperimentalFeature.h>

namespace AppInstaller::CLI
{
    struct DscCommand final : public Command
    {
        DscCommand(std::string_view parent) : Command(StaticName(), parent, Settings::ExperimentalFeature::Feature::ConfigurationDSCv3) {}

        static constexpr std::string_view StaticName() { return "dscv3"sv; };

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
