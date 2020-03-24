// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "HashCommand.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_HashCommand_ArgName_File = "file"sv;
    constexpr std::string_view s_HashCommand_ArgName_Msix = "msix"sv;

    std::vector<Argument> HashCommand::GetArguments() const
    {
        return {
            Argument{ s_HashCommand_ArgName_File, Execution::Args::Type::HashFile, LOCME("The input file to be hashed."), ArgumentType::Positional, true },
            Argument{ s_HashCommand_ArgName_Msix, Execution::Args::Type::Msix, LOCME("If specified, the input file will be treated as msix. Signature hash will be provided if exists."), ArgumentType::Flag },
        };
    }

    std::string HashCommand::ShortDescription() const
    {
        return LOCME("Helper to hash installer files");
    }

    std::vector<std::string> HashCommand::GetLongDescription() const
    {
        return {
            LOCME("Helper to hash installer files"),
        };
    }

    void HashCommand::ExecuteInternal(Execution::Context& context) const
    {
        auto inputFile = context.Args.GetArg(Execution::Args::Type::HashFile);
        std::ifstream inStream{ inputFile, std::ifstream::binary };

        context.Reporter.ShowMsg("File Hash: " + Utility::SHA256::ConvertToString(Utility::SHA256::ComputeHash(inStream)));

        if (context.Args.Contains(Execution::Args::Type::Msix))
        {
            try
            {
                Msix::MsixInfo msixInfo{ inputFile };
                auto signature = msixInfo.GetSignature();
                auto signatureHash = Utility::SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()));

                context.Reporter.ShowMsg("Signature Hash: " + Utility::SHA256::ConvertToString(signatureHash));
            }
            catch (const wil::ResultException&)
            {
                context.Reporter.ShowMsg(
                    "Failed to calculate signature hash. Please verify the input file is a valid signed msix.",
                    Execution::Reporter::Level::Warning);
            }
        }
    }
}
