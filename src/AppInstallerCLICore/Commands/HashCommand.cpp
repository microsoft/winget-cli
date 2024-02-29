// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "HashCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

#include <AppInstallerMsixInfo.h>

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Utility::literals;

    std::vector<Argument> HashCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::HashFile),
            Argument::ForType(Execution::Args::Type::Msix),
        };
    }

    Resource::LocString HashCommand::ShortDescription() const
    {
        return { Resource::String::HashCommandShortDescription };
    }

    Resource::LocString HashCommand::LongDescription() const
    {
        return { Resource::String::HashCommandLongDescription };
    }

    Utility::LocIndView HashCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-hash"_liv;
    }

    void HashCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyFile(Execution::Args::Type::HashFile) <<
            [](Execution::Context& context)
        {
            auto inputFile = context.Args.GetArg(Execution::Args::Type::HashFile);
            std::ifstream inStream{ Utility::ConvertToUTF16(inputFile), std::ifstream::binary };

            context.Reporter.Info() << "InstallerSha256: "_liv << Utility::LocIndString{ Utility::SHA256::ConvertToString(Utility::SHA256::ComputeHash(inStream)) } << std::endl;

            if (context.Args.Contains(Execution::Args::Type::Msix))
            {
                try
                {
                    Msix::MsixInfo msixInfo{ inputFile };
                    auto signatureHash = msixInfo.GetSignatureHash();

                    context.Reporter.Info() << "SignatureSha256: "_liv << Utility::LocIndString{ Utility::SHA256::ConvertToString(signatureHash) } << std::endl;
                }
                catch (const wil::ResultException& re)
                {
                    context.Reporter.Warn() << 
                        Resource::String::MsixSignatureHashFailed << std::endl <<
                        Resource::String::VerifyFileSignedMsix << std::endl;
                    AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
                }
            }
        };
    }
}
