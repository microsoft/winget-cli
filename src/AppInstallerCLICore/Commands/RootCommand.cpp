// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"
#include "Localization.h"

#include "InstallCommand.h"
#include "ShowCommand.h"
#include "SourceCommand.h"
#include "SearchCommand.h"
#include "HashCommand.h"
#include "ValidateCommand.h"

namespace AppInstaller::CLI
{
    // TODO: REMOVE ME
    using namespace std::chrono_literals;
    struct ProgressTest final : public Command
    {
        ProgressTest(std::string_view parent) : Command("pt", parent) {}

        void ExecuteInternal(Execution::Context& context) const override
        {
            Execution::Reporter& r = context.Reporter;

            r.Info() << "No progress occurs" << std::endl;
            context.Reporter.ExecuteWithProgress([](ProgressCallback&) { std::this_thread::sleep_for(10s); });

            if (context.IsTerminated()) return;

            r.Info() << "Percent progress" << std::endl;
            context.Reporter.ExecuteWithProgress(PercentProgress);

            if (context.IsTerminated()) return;

            r.Info() << "Bytes progress, max unknown" << std::endl;
            context.Reporter.ExecuteWithProgress(BytesProgressMaxUnknown);

            if (context.IsTerminated()) return;

            r.Info() << "Bytes progress, max known" << std::endl;
            context.Reporter.ExecuteWithProgress(BytesProgressMaxKnown);

            if (context.IsTerminated()) return;

            r.Info() << "Multi-use" << std::endl;
            context.Reporter.ExecuteWithProgress([](ProgressCallback& cb) {
                BytesProgressMaxKnown(cb);
                BytesProgressMaxUnknown(cb);
                BytesProgressMaxKnown(cb);
                PercentProgress(cb);
                });
        }

        static void PercentProgress(ProgressCallback& cb) {
            auto waitTime = 30ms;
            for (uint64_t i = 1; i <= 100; ++i)
            {
                if (cb.IsCancelled()) return;
                std::this_thread::sleep_for(waitTime);
                cb.OnProgress(i, 100, ProgressType::Percent);
            }
        }

        static void BytesProgressMaxUnknown(ProgressCallback& cb) {
            auto waitTime = 30ms;
            uint64_t multi = 87654;
            for (uint64_t i = 1; i <= 100; ++i)
            {
                if (cb.IsCancelled()) return;
                std::this_thread::sleep_for(waitTime);
                cb.OnProgress(i * multi, 0, ProgressType::Bytes);
            }
        }

        static void BytesProgressMaxKnown(ProgressCallback& cb) {
            auto waitTime = 30ms;
            uint64_t multi = 87654;
            for (uint64_t i = 1; i <= 100; ++i)
            {
                if (cb.IsCancelled()) return;
                std::this_thread::sleep_for(waitTime);
                cb.OnProgress(i * multi, 100 * multi, ProgressType::Bytes);
            }
        }
    };

    std::vector<std::unique_ptr<Command>> RootCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<InstallCommand>(FullName()),
            std::make_unique<ShowCommand>(FullName()),
            std::make_unique<SourceCommand>(FullName()),
            std::make_unique<SearchCommand>(FullName()),
            std::make_unique<HashCommand>(FullName()),
            std::make_unique<ValidateCommand>(FullName()),
            std::make_unique<ProgressTest>(FullName()),
        });
    }

    std::vector<Argument> RootCommand::GetArguments() const
    {
        return
        {
            Argument{ "version", 'v', Execution::Args::Type::ListVersions, LOCME("Display the version of the tool"), ArgumentType::Flag, Visibility::Help },
        };
    }

    std::string RootCommand::GetLongDescription() const
    {
        return LOCME("AppInstaller command line utility enables installing applications from the command line.");
    }

    void RootCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            context.Reporter.Info() << 'v' << Runtime::GetClientVersion() << std::endl;
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
