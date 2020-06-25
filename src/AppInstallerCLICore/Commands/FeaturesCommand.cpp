// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FeaturesCommand.h"
#include "TableOutput.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    using namespace Utility::literals;
    using namespace AppInstaller::Settings;

    Resource::LocString FeaturesCommand::ShortDescription() const
    {
        return { Resource::String::FeaturesCommandShortDescription };
    }

    Resource::LocString FeaturesCommand::LongDescription() const
    {
        return { Resource::String::FeaturesCommandLongDescription };
    }

    std::string FeaturesCommand::HelpLink() const
    {
        return "https://aka.ms/winget-seattings";
    }

    void FeaturesCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.Reporter.Info() << Resource::String::FeaturesMessage << std::endl << std::endl;

        Execution::TableOutput<3> table(context.Reporter, { "Feature", "Status", "Link"});
        auto featuresInfo = UserSettings::GetFeaturesInfo();
        for (const auto& featureInfo : featuresInfo)
        {
            table.OutputLine({ std::get<0>(featureInfo), std::get<1>(featureInfo), std::get<2>(featureInfo) });
        }

        if (!featuresInfo.empty())
        {
            table.Complete();
        }
        else
        {
            // Better work hard to get some out there!
            context.Reporter.Info() << Resource::String::NoExperimentalFeaturesMessage << std::endl;
        }
    }
}
