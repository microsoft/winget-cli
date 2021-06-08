// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"
#include "SourceList.h"
#include "SourcePolicy.h"
#include "CompositeSource.h"
#include "SourceFactory.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Rest/RestSourceFactory.h"
#include <winget/GroupPolicy.h>

namespace AppInstaller::Repository
{
    using namespace Settings;

    using namespace std::chrono_literals;

    namespace
    {
        void EnsureSourceIsRemovable(const SourceDetailsInternal& source)
        {
            // Block removing sources added by Group Policy
            if (source.Origin == SourceOrigin::GroupPolicy)
            {
                AICLI_LOG(Repo, Error, << "Cannot remove source added by Group Policy");
                throw GroupPolicyException(TogglePolicy::Policy::AdditionalSources);
            }

            // Block removing default sources required by Group Policy.
            if (source.Origin == SourceOrigin::Default)
            {
                if (GroupPolicies().GetState(TogglePolicy::Policy::DefaultSource) == PolicyState::Enabled &&
                    source.Identifier == WingetCommunityDefaultSourceId())
                {
                    throw GroupPolicyException(TogglePolicy::Policy::DefaultSource);
                }

                if (GroupPolicies().GetState(TogglePolicy::Policy::MSStoreSource) == PolicyState::Enabled &&
                    source.Identifier == WingetMSStoreDefaultSourceId())
                {
                    throw GroupPolicyException(TogglePolicy::Policy::MSStoreSource);
                }
            }
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        static std::map<std::string, std::function<std::unique_ptr<ISourceFactory>()>> s_Sources_TestHook_SourceFactories;
#endif

        std::unique_ptr<ISourceFactory> GetFactoryForType(std::string_view type)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            // Tests can ensure case matching
            auto itr = s_Sources_TestHook_SourceFactories.find(std::string(type));
            if (itr != s_Sources_TestHook_SourceFactories.end())
            {
                return itr->second();
            }
#endif

            // For now, enable an empty type to represent the only one we have.
            if (type.empty() ||
                Utility::CaseInsensitiveEquals(Microsoft::PreIndexedPackageSourceFactory::Type(), type))
            {
                return Microsoft::PreIndexedPackageSourceFactory::Create();
            }
            // Should always come from code, so no need for case insensitivity
            else if (Microsoft::PredefinedInstalledSourceFactory::Type() == type)
            {
                return Microsoft::PredefinedInstalledSourceFactory::Create();
            }
            else if (Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), type))
            {
                return Rest::RestSourceFactory::Create();
            }

            THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE);
        }

        std::shared_ptr<ISource> CreateSourceFromDetails(const SourceDetails& details, IProgressCallback& progress)
        {
            return GetFactoryForType(details.Type)->Create(details, progress);
        }

        template <typename MemberFunc>
        bool AddOrUpdateFromDetails(SourceDetails& details, MemberFunc member, IProgressCallback& progress)
        {
            bool result = false;
            auto factory = GetFactoryForType(details.Type);

            // Attempt; if it fails, wait a short time and retry.
            try
            {
                result = (factory.get()->*member)(details, progress);
                if (result)
                {
                    details.LastUpdateTime = std::chrono::system_clock::now();
                }
                return result;
            }
            CATCH_LOG();

            AICLI_LOG(Repo, Info, << "Source add/update failed, waiting a bit and retrying: " << details.Name);
            std::this_thread::sleep_for(2s);

            // If this one fails, maybe the problem is persistent.
            result = (factory.get()->*member)(details, progress);
            if (result)
            {
                details.LastUpdateTime = std::chrono::system_clock::now();
            }
            return result;
        }

        bool AddSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            return AddOrUpdateFromDetails(details, &ISourceFactory::Add, progress);
        }

        bool UpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            return AddOrUpdateFromDetails(details, &ISourceFactory::Update, progress);
        }

        bool BackgroundUpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            return AddOrUpdateFromDetails(details, &ISourceFactory::BackgroundUpdate, progress);
        }

        bool RemoveSourceFromDetails(const SourceDetails& details, IProgressCallback& progress)
        {
            auto factory = GetFactoryForType(details.Type);

            return factory->Remove(details, progress);
        }

        // Determines whether (and logs why) a source should be updated before it is opened.
        bool ShouldUpdateBeforeOpen(const SourceDetails& details)
        {
            constexpr static auto s_ZeroMins = 0min;
            auto autoUpdateTime = User().Get<Setting::AutoUpdateTimeInMinutes>();

            // A value of zero means no auto update, to get update the source run `winget update`
            if (autoUpdateTime != s_ZeroMins)
            {
                auto autoUpdateTimeMins = std::chrono::minutes(autoUpdateTime);
                auto timeSinceLastUpdate = std::chrono::system_clock::now() - details.LastUpdateTime;
                if (timeSinceLastUpdate > autoUpdateTimeMins)
                {
                    AICLI_LOG(Repo, Info, << "Source past auto update time [" <<
                        std::chrono::duration_cast<std::chrono::minutes>(autoUpdateTimeMins).count() << " mins]; it has been at least " <<
                        std::chrono::duration_cast<std::chrono::minutes>(timeSinceLastUpdate).count() << " mins");
                    return true;
                }
            }

            return false;
        }
    }

    std::string_view ToString(SourceOrigin origin)
    {
        switch (origin)
        {
        case SourceOrigin::Default:
            return "Default"sv;
        case SourceOrigin::User:
            return "User"sv;
        case SourceOrigin::GroupPolicy:
            return "GroupPolicy"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<SourceDetails> GetSources()
    {
        SourceList sourceList;

        std::vector<SourceDetails> result;
        for (auto&& source : sourceList.GetCurrentSourceRefs())
        {
            result.emplace_back(std::move(source));
        }

        return result;
    }

    std::optional<SourceDetails> GetSource(std::string_view name)
    {
        // Check all sources for the given name.
        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            return {};
        }
        else
        {
            return *source;
        }
    }

    bool AddSource(std::string_view name, std::string_view type, std::string_view arg, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << name << "], Type[" << type << "], Arg[" << arg << "]");

        // Check all sources for the given name.
        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, source != nullptr);

        // Check sources allowed by group policy
        auto blockingPolicy = GetPolicyBlockingUserSource(name, type, arg, false);
        if (blockingPolicy != TogglePolicy::Policy::None)
        {
            throw GroupPolicyException(blockingPolicy);
        }

        SourceDetailsInternal details;
        details.Name = name;
        details.Type = type;
        details.Arg = arg;
        details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);
        details.Origin = SourceOrigin::User;

        bool result = AddSourceFromDetails(details, progress);
        if (result)
        {
            AICLI_LOG(Repo, Info, << "Source created with extra data: " << details.Data);
            AICLI_LOG(Repo, Info, << "Source created with identifier: " << details.Identifier);

            sourceList.AddSource(details);
        }

        return result;
    }

    OpenSourceResult OpenSource(std::string_view name, IProgressCallback& progress)
    {
        SourceList sourceList;
        auto currentSources = sourceList.GetCurrentSourceRefs();

        if (name.empty())
        {
            if (currentSources.empty())
            {
                AICLI_LOG(Repo, Info, << "Default source requested, but no sources configured");
                return {};
            }
            else if (currentSources.size() == 1)
            {
                AICLI_LOG(Repo, Info, << "Default source requested, only 1 source available, using the only source: " << currentSources[0].get().Name);
                return OpenSource(currentSources[0].get().Name, progress);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Default source requested, multiple sources available, creating aggregated source.");
                auto aggregatedSource = std::make_shared<CompositeSource>("*DefaultSource");
                OpenSourceResult result;

                bool sourceUpdated = false;
                for (auto& source : currentSources)
                {
                    AICLI_LOG(Repo, Info, << "Adding to aggregated source: " << source.get().Name);

                    if (ShouldUpdateBeforeOpen(source))
                    {
                        try
                        {
                            // TODO: Consider adding a context callback to indicate we are doing the same action
                            // to avoid the progress bar fill up multiple times.
                            sourceUpdated = BackgroundUpdateSourceFromDetails(source, progress) || sourceUpdated;
                        }
                        catch (...)
                        {
                            AICLI_LOG(Repo, Warning, << "Failed to update source: " << source.get().Name);
                            result.SourcesWithUpdateFailure.emplace_back(source);
                        }
                    }
                    aggregatedSource->AddAvailableSource(CreateSourceFromDetails(source, progress));
                }

                if (sourceUpdated)
                {
                    sourceList.SaveMetadata();
                }

                result.Source = aggregatedSource;
                return result;
            }
        }
        else
        {
            auto source = sourceList.GetCurrentSource(name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source requested, but not found: " << name);
                return {};
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source requested, found: " << source->Name);

                OpenSourceResult result;

                if (ShouldUpdateBeforeOpen(*source))
                {
                    try
                    {
                        if (BackgroundUpdateSourceFromDetails(*source, progress))
                        {
                            sourceList.SaveMetadata();
                        }
                    }
                    catch (...)
                    {
                        AICLI_LOG(Repo, Warning, << "Failed to update source: " << (*source).Name);
                        result.SourcesWithUpdateFailure.emplace_back(*source);
                    }
                }

                result.Source = CreateSourceFromDetails(*source, progress);
                return result;
            }
        }
    }

    std::shared_ptr<ISource> OpenPredefinedSource(PredefinedSource source, IProgressCallback& progress)
    {
        SourceDetails details;
        details.Origin = SourceOrigin::Predefined;

        switch (source)
        {
        case PredefinedSource::Installed:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::None);
            return CreateSourceFromDetails(details, progress);
        case PredefinedSource::ARP:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::ARP);
            return CreateSourceFromDetails(details, progress);
        case PredefinedSource::MSIX:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX);
            return CreateSourceFromDetails(details, progress);
        }

        THROW_HR(E_UNEXPECTED);
    }

    std::shared_ptr<ISource> CreateCompositeSource(const std::shared_ptr<ISource>& installedSource, const std::shared_ptr<ISource>& availableSource, CompositeSearchBehavior searchBehavior)
    {
        std::shared_ptr<CompositeSource> result = std::dynamic_pointer_cast<CompositeSource>(availableSource);

        if (!result)
        {
            result = std::make_shared<CompositeSource>("*CompositeSource");
            result->AddAvailableSource(availableSource);
        }

        result->SetInstalledSource(installedSource, searchBehavior);

        return result;
    }

    bool UpdateSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, found: " << source->Name);

            bool result = UpdateSourceFromDetails(*source, progress);
            if (result)
            {
                sourceList.SaveMetadata();
            }

            return result;
        }
    }

    bool RemoveSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, found: " << source->Name << " [" << ToString(source->Origin) << ']');

            EnsureSourceIsRemovable(*source);

            bool result = RemoveSourceFromDetails(*source, progress);
            if (result)
            {
                sourceList.RemoveSource(*source);
            }

            return result;
        }
    }

    bool DropSource(std::string_view name)
    {
        if (name.empty())
        {
            Settings::Stream{ Settings::Stream::UserSources }.Remove();
            Settings::Stream{ Settings::Stream::SourcesMetadata }.Remove();
            return true;
        }
        else
        {
            SourceList sourceList;

            auto source = sourceList.GetCurrentSource(name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, but not found: " << name);
                return false;
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, found: " << source->Name);

                EnsureSourceIsRemovable(*source);
                sourceList.RemoveSource(*source);

                return true;
            }
        }
    }

    bool SearchRequest::IsForEverything() const
    {
        return (!Query.has_value() && Inclusions.empty() && Filters.empty());
    }

    std::string SearchRequest::ToString() const
    {
        std::ostringstream result;

        result << "Query:";
        if (Query)
        {
            result << '\'' << Query.value().Value << "'[" << MatchTypeToString(Query.value().Type) << ']';
        }
        else
        {
            result << "[none]";
        }

        for (const auto& include : Inclusions)
        {
            result << " Include:" << PackageMatchFieldToString(include.Field) << "='" << include.Value << "'";
            if (include.Additional)
            {
                result << "+'" << include.Additional.value() << "'";
            }
            result << "[" << MatchTypeToString(include.Type) << "]";
        }

        for (const auto& filter : Filters)
        {
            result << " Filter:" << PackageMatchFieldToString(filter.Field) << "='" << filter.Value << "'[" << MatchTypeToString(filter.Type) << "]";
        }

        if (MaximumResults)
        {
            result << " Limit:" << MaximumResults;
        }

        return result.str();
    }

    std::string_view ToString(PackageVersionMetadata pvm)
    {
        switch (pvm)
        {
        case PackageVersionMetadata::InstalledType: return "InstalledType"sv;
        case PackageVersionMetadata::InstalledScope: return "InstalledScope"sv;
        case PackageVersionMetadata::InstalledLocation: return "InstalledLocation"sv;
        case PackageVersionMetadata::StandardUninstallCommand: return "StandardUninstallCommand"sv;
        case PackageVersionMetadata::SilentUninstallCommand: return "SilentUninstallCommand"sv;
        case PackageVersionMetadata::Publisher: return "Publisher"sv;
        case PackageVersionMetadata::InstalledLocale: return "InstalledLocale"sv;
        default: return "Unknown"sv;
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_SetSourceFactoryOverride(const std::string& type, std::function<std::unique_ptr<ISourceFactory>()>&& factory)
    {
        s_Sources_TestHook_SourceFactories[type] = std::move(factory);
    }

    void TestHook_ClearSourceFactoryOverrides()
    {
        s_Sources_TestHook_SourceFactories.clear();
    }
#endif
}
