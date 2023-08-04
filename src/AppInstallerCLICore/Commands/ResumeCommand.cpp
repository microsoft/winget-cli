// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "ResumeCommand.h"
#include "Workflows/ResumeFlow.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;
    using namespace Checkpoint;

    std::vector<Argument> ResumeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ResumeGuid),
        };
    }

    Resource::LocString ResumeCommand::ShortDescription() const
    {
        return { Resource::String::ResumeCommandShortDescription };
    }

    Resource::LocString ResumeCommand::LongDescription() const
    {
        return { Resource::String::ResumeCommandLongDescription };
    }

    Utility::LocIndView ResumeCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-resume"_liv;
    }

    void ResumeCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::EnsureSupportForResume;

        if (context.IsTerminated())
        {
            return;
        }

        int rootContextId = CheckpointManager::Instance().GetFirstContextId();
        auto resumeContextPtr = context.CreateEmptyContext(rootContextId);

        Context& resumeContext = *resumeContextPtr;
        auto previousThreadGlobals = resumeContext.SetForCurrentThread();

        resumeContext <<
            Workflow::LoadInitialResumeState;

        auto executingCommandPtr = resumeContext.GetExecutingCommand();
        Command& executingCommand = *executingCommandPtr;
        executingCommand.Execute(resumeContext);
    }
}
