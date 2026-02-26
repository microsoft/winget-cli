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

    Utility::LocIndView FeaturesCommand::HelpLink() const
    {
        return "https://aka.ms/winget-experimentalfeatures"_liv;
    }

    void FeaturesCommand::ExecuteInternal(Execution::Context& context) const
    {
#ifdef WINGET_DISABLE_EXPERIMENTAL_FEATURES
        context.Reporter.Info() << Resource::String::FeaturesMessageDisabledByBuild << std::endl;
#else
        if (GroupPolicies().IsEnabled(TogglePolicy::Policy::ExperimentalFeatures) &&
            GroupPolicies().IsEnabled(TogglePolicy::Policy::Settings))
        {
            context.Reporter.Info() << Resource::String::FeaturesMessage << std::endl << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::FeaturesMessageDisabledByPolicy << std::endl << std::endl;
        }

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
                    Resource::LocString{ ExperimentalFeature::IsEnabled(feature.GetFeature()) ? Resource::String::FeaturesEnabled : Resource::String::FeaturesDisabled},
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
#endif
    }
}
