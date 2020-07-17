#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct HomepageCommand final : public Command
    {
        HomepageCommand(std::string_view parent) : Command("homepage", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
