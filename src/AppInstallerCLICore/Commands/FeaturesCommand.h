// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    struct FeaturesCommand final : public Command
    {
        // This command outputs all the experimental features that are available, if they are enabled/disabled
        // and a link to the spec.
        FeaturesCommand(std::string_view parent) : Command("features", parent) {}

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
#pragma once
