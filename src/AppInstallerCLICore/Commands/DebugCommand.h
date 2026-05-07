// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

#if _DEBUG

namespace AppInstaller::CLI
{
    // Command that is only available with debug builds to aid development.
    // Don't create localized strings for use here.
    struct DebugCommand final : public Command
    {
        DebugCommand(std::string_view parent) : Command("debug", {}, parent, Visibility::Hidden) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Outputs the proxy stub registrations for the manifest.
    struct DumpProxyStubRegistrationsCommand final : public Command
    {
        DumpProxyStubRegistrationsCommand(std::string_view parent) : Command("dump-proxystub-reg", {}, parent) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Outputs some IIDs.
    struct DumpInterestingIIDsCommand final : public Command
    {
        DumpInterestingIIDsCommand(std::string_view parent) : Command("dump-iids", {}, parent) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Outputs the errors as resources.
    struct DumpErrorResourceCommand final : public Command
    {
        DumpErrorResourceCommand(std::string_view parent) : Command("dump-error-resource", {}, parent) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Outputs a sixel image.
    struct ShowSixelCommand final : public Command
    {
        ShowSixelCommand(std::string_view parent) : Command("sixel", {}, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Invokes progress display.
    struct ProgressCommand final : public Command
    {
        ProgressCommand(std::string_view parent) : Command("progress", {}, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Directly invokes a DSCv3 resource function through WinGet's infrastructure.
    struct DebugDscResourceCommand final : public Command
    {
        DebugDscResourceCommand(std::string_view parent) : Command("dsc-resource", {}, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Tests the log viewer extension by emitting logs that exercise all channels, levels, subchannels,
    // continuation lines, long lines, and optionally a streaming follow mode.
    struct LogViewerTestCommand final : public Command
    {
        LogViewerTestCommand(std::string_view parent) : Command("log-viewer", {}, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}

#endif
