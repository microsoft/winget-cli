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
        return "https://aka.ms/winget-experimentalfeatures";
    }

    void FeaturesCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.Reporter.Info() << Resource::String::FeaturesMessage << std::endl << std::endl;

        auto features = ExperimentalFeature::GetAllFeatures();
        
        if (!features.empty())
        {
            Execution::TableOutput<4> table(context.Reporter, {
                Resource::String::FeaturesFeature,
                Resource::String::FeaturesStatus,
                Resource::String::FeaturesProperty,
                Resource::String::FeaturesLink });
            for (const auto& feature : features)
            {
                table.OutputLine({
                    std::string{ feature.Name() },
                    Resource::Loader::Instance().ResolveString(ExperimentalFeature::IsEnabled(feature.GetFeature()) ? Resource::String::FeaturesEnabled : Resource::String::FeaturesDisabled),
                    std::string { feature.JsonName() },
                    std::string{ feature.Link() } });
            }
            table.Complete();
        }
        else
        {
            // Better work hard to get some out there!
            context.Reporter.Info() << Resource::String::NoExperimentalFeaturesMessage << std::endl;
        }
    }
}
