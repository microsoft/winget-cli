// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    struct FeaturesCommand final : public Command
    {
        // This command is used as an example on how experimental features can be used.
        // To enable this command set ExperimentalCmd = true in the settings file.
        FeaturesCommand(std::string_view parent) : Command("features", parent, VisibilityCmd::Hidden) {}

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
#pragma once
