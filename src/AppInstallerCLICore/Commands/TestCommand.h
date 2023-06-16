// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "Command.h"

namespace AppInstaller::CLI
{
    // Command: winget test
    // Convenient command for debugging. Waits infinitely.
    // Use this if you want to debug things that happen out side of workflows or modify locally to do whatever you need.
    struct TestCommand final : public Command
    {
        TestCommand(std::string_view parent) : Command("test", {}, parent, Visibility::Hidden) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Command: winget test appshutdown
    // Verifies the window was created and waits for the app shutdown event.
    // Used in E2E.
    struct TestAppShutdownCommand final : public Command
    {
        TestAppShutdownCommand(std::string_view parent) : Command("appshutdown", {}, parent, Visibility::Hidden) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}

#endif
