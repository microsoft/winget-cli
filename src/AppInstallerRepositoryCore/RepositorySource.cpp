// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"

#include "CompositeSource.h"
#include "SourceFactory.h"
#include "SourceList.h"
#include "SourcePolicy.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/PredefinedWriteableSourceFactory.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Rest/RestSourceFactory.h"

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "Microsoft/ConfigurableTestSourceFactory.h"
#endif

#include <winget/GroupPolicy.h>

using namespace AppInstaller::Settings;
using namespace std::chrono_literals;

namespace AppInstaller::Repository
{
    namespace
    {
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

            if (Utility::CaseInsensitiveEquals(Microsoft::ConfigurableTestSourceFactory::Type(), type))
            {
                return Microsoft::ConfigurableTestSourceFactory::Create();
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
            // Should always come from code, so no need for case insensitivity
            else if (Microsoft::PredefinedWriteableSourceFactory::Type() == type)
            {
                return Microsoft::PredefinedWriteableSourceFactory::Create();
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

        std::string GetStringVectorMessage(const std::vector<std::string>& input)
        {
            std::string result;
            bool first = true;
            for (auto const& field : input)
            {
                if (first)
                {
                    result += field;
                    first = false;
                }
                else
                {
                    result += ", " + field;
                }
            }
            return result;
        }

        // Carries the exception from an OpenSource call and presents it back at search time.
        struct OpenExceptionProxy : public ISource, std::enable_shared_from_this<OpenExceptionProxy>
        {
            OpenExceptionProxy(const SourceDetails& details, std::exception_ptr exception) :
                m_details(details), m_exception(std::move(exception)) {}

            const SourceDetails& GetDetails() const override { return m_details; }

            const std::string& GetIdentifier() const override { return m_details.Identifier; }

            SearchResult Search(const SearchRequest&) const override
            {
                SearchResult result;
                result.Failures.emplace_back(SearchResult::Failure{ shared_from_this(), m_exception });
                return result;
            }

        private:
            SourceDetails m_details;
            std::exception_ptr m_exception;
        };
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
        case SourceOrigin::Metadata:
            return "Metadata"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    ImplicitAgreementFieldEnum GetAgreementFieldsFromSourceInformation(const SourceInformation& info)
    {
        ImplicitAgreementFieldEnum result = ImplicitAgreementFieldEnum::None;

        if (info.RequiredPackageMatchFields.end() != std::find_if(info.RequiredPackageMatchFields.begin(), info.RequiredPackageMatchFields.end(), [&](const auto& field) { return Utility::CaseInsensitiveEquals(field, "market"); }) ||
            info.RequiredQueryParameters.end() != std::find_if(info.RequiredQueryParameters.begin(), info.RequiredQueryParameters.end(), [&](const auto& param) { return Utility::CaseInsensitiveEquals(param, "market"); }))
        {
            WI_SetFlag(result, ImplicitAgreementFieldEnum::Market);
        }

        return result;
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

    bool AddSource(SourceDetails& sourceDetails, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, sourceDetails.Name.empty());

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << sourceDetails.Name << "], Type[" << sourceDetails.Type << "], Arg[" << sourceDetails.Arg << "]");

        // Check all sources for the given name.
        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(sourceDetails.Name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, source != nullptr);

        // Check for a hidden source data that we don't want to collide.
        // TODO: Refactor the source interface so that we don't do this
        auto hiddenSource = GetSource(sourceDetails.Name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS,
            hiddenSource && hiddenSource->Origin != SourceOrigin::User && hiddenSource->Origin != SourceOrigin::Metadata);

        // Check sources allowed by group policy
        auto blockingPolicy = GetPolicyBlockingUserSource(sourceDetails.Name, sourceDetails.Type, sourceDetails.Arg, false);
        if (blockingPolicy != TogglePolicy::Policy::None)
        {
            throw GroupPolicyException(blockingPolicy);
        }

        sourceDetails.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);
        sourceDetails.Origin = SourceOrigin::User;

        bool result = AddSourceFromDetails(sourceDetails, progress);
        if (result)
        {
            AICLI_LOG(Repo, Info, << "Source created with extra data: " << sourceDetails.Data);
            AICLI_LOG(Repo, Info, << "Source created with identifier: " << sourceDetails.Identifier);

            sourceList.AddSource(sourceDetails);
        }

        return result;
    }

    OpenSourceResult OpenSource(std::string_view name, IProgressCallback& progress)
    {
        SourceList sourceList;

        if (name.empty())
        {
            auto currentSources = sourceList.GetCurrentSourceRefs();
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
                std::vector<std::shared_ptr<OpenExceptionProxy>> openExceptionProxies;

                for (auto& source : currentSources)
                {
                    AICLI_LOG(Repo, Info, << "Adding to aggregated source: " << source.get().Name);

                    if (ShouldUpdateBeforeOpen(source))
                    {
                        try
                        {
                            // TODO: Consider adding a context callback to indicate we are doing the same action
                            // to avoid the progress bar fill up multiple times.
                            if (BackgroundUpdateSourceFromDetails(source, progress))
                            {
                                sourceList.SaveMetadata(source);
                            }
                        }
                        catch (...)
                        {
                            LOG_CAUGHT_EXCEPTION();
                            AICLI_LOG(Repo, Warning, << "Failed to update source: " << source.get().Name);
                            result.SourcesWithUpdateFailure.emplace_back(source);
                        }
                    }

                    std::shared_ptr<ISource> openedSource;

                    try
                    {
                        openedSource = CreateSourceFromDetails(source, progress);
                    }
                    catch (...)
                    {
                        LOG_CAUGHT_EXCEPTION();
                        AICLI_LOG(Repo, Warning, << "Failed to open source: " << source.get().Name);
                        openExceptionProxies.emplace_back(std::make_shared<OpenExceptionProxy>(source, std::current_exception()));
                    }

                    if (openedSource)
                    {
                        aggregatedSource->AddAvailableSource(std::move(openedSource));
                    }
                }

                // If all sources failed to open, then throw an exception that is specific to this case.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES, !aggregatedSource->HasAvailableSource());

                // Place all of the proxies into the source to be searched later
                for (auto& proxy : openExceptionProxies)
                {
                    aggregatedSource->AddAvailableSource(std::move(proxy));
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
                            sourceList.SaveMetadata(*source);
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

    OpenSourceResult OpenSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
    {
        OpenSourceResult result;

        // Get the details again by name from the source list because SaveMetadata only updates the LastUpdateTime
        // if the details came from the same instance of the list that's being saved.
        // Some sources that do not need updating like the Installed source, do not have Name values.
        // Restricted sources don't have full functionality
        if (!details.Name.empty())
        {
            SourceList sourceList;
            auto source = sourceList.GetSource(details.Name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source no longer found. Source may have been removed by the user: " << details.Name);
                return {};
            }

            if (ShouldUpdateBeforeOpen(*source))
            {
                try
                {
                    if (BackgroundUpdateSourceFromDetails(*source, progress))
                    {
                        sourceList.SaveMetadata(*source);
                    }
                }
                catch (...)
                {
                    AICLI_LOG(Repo, Warning, << "Failed to update source: " << details.Name);
                    result.SourcesWithUpdateFailure.emplace_back(*source);
                }
            }
        }

        result.Source = CreateSourceFromDetails(details, progress);
        return result;
    }

    std::shared_ptr<ISource> OpenPredefinedSource(PredefinedSource source, IProgressCallback& progress)
    {
        SourceDetails details = GetPredefinedSourceDetails(source);
        return CreateSourceFromDetails(details, progress);
    }

    SourceDetails GetPredefinedSourceDetails(PredefinedSource source)
    {
        SourceDetails details;
        details.Origin = SourceOrigin::Predefined;

        switch (source)
        {
        case PredefinedSource::Installed:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::None);
            return details;
        case PredefinedSource::ARP:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::ARP);
            return details;
        case PredefinedSource::MSIX:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX);
            return details;
        case PredefinedSource::Installing:
            details.Type = Microsoft::PredefinedWriteableSourceFactory::Type();
            // As long as there is only one type this is not particularly needed, but Arg is exposed publicly
            // so this is used here for consistency with other predefined sources.
            details.Arg = Microsoft::PredefinedWriteableSourceFactory::TypeToString(Microsoft::PredefinedWriteableSourceFactory::WriteableType::Installing);
            return details;
        }

        THROW_HR(E_UNEXPECTED);
    }

    SourceDetails GetWellKnownSourceDetails(WellKnownSource source)
    {
        return GetWellKnownSourceDetailsInternal(source);
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

    std::shared_ptr<ISource> CreateCompositeSource(
        const std::shared_ptr<ISource>& installedSource,
        const std::vector<std::shared_ptr<ISource>>& availableSources,
        CompositeSearchBehavior searchBehavior)
    {
        std::shared_ptr<CompositeSource> result = std::make_shared<CompositeSource>("*CompositeSource");

        for (const auto& availableSource : availableSources)
        {
            result->AddAvailableSource(availableSource);
        }

        if (installedSource)
        {
            result->SetInstalledSource(installedSource, searchBehavior);
        }

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
                sourceList.SaveMetadata(*source);
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
            SourceList::RemoveSettingsStreams();
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

    bool SupportsCustomHeader(const SourceDetails& sourceDetails)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (Utility::CaseInsensitiveEquals(Microsoft::ConfigurableTestSourceFactory::Type(), sourceDetails.Type))
        {
            return true;
        }
#endif

        return Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), sourceDetails.Type);
    }

    bool CheckSourceAgreements(const SourceDetails& source)
    {
        SourceList sourceList;
        return sourceList.CheckSourceAgreements(source);
    }

    void SaveAcceptedSourceAgreements(const SourceDetails& source)
    {
        SourceList sourceList;
        sourceList.SaveAcceptedSourceAgreements(source);
    }

    bool ContainsAvailablePackages(SourceOrigin origin)
    {
        return (origin == SourceOrigin::Default || origin == SourceOrigin::GroupPolicy || origin == SourceOrigin::User);
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

    const char* UnsupportedRequestException::what() const noexcept
    {
        if (m_whatMessage.empty())
        {
            m_whatMessage = "The request is not supported.";

            if (!UnsupportedPackageMatchFields.empty())
            {
                m_whatMessage += "Unsupported Package Match Fields: " + GetStringVectorMessage(UnsupportedPackageMatchFields);
            }
            if (!RequiredPackageMatchFields.empty())
            {
                m_whatMessage += "Required Package Match Fields: " + GetStringVectorMessage(RequiredPackageMatchFields);
            }
            if (!UnsupportedQueryParameters.empty())
            {
                m_whatMessage += "Unsupported Query Parameters: " + GetStringVectorMessage(UnsupportedQueryParameters);
            }
            if (!RequiredQueryParameters.empty())
            {
                m_whatMessage += "Required Query Parameters: " + GetStringVectorMessage(RequiredQueryParameters);
            }
        }
        return m_whatMessage.c_str();
    }

    std::string_view MatchTypeToString(MatchType type)
    {
        using namespace std::string_view_literals;

        switch (type)
        {
        case MatchType::Exact:
            return "Exact"sv;
        case MatchType::CaseInsensitive:
            return "CaseInsensitive"sv;
        case MatchType::StartsWith:
            return "StartsWith"sv;
        case MatchType::Substring:
            return "Substring"sv;
        case MatchType::Wildcard:
            return "Wildcard"sv;
        case MatchType::Fuzzy:
            return "Fuzzy"sv;
        case MatchType::FuzzySubstring:
            return "FuzzySubstring"sv;
        }

        return "UnknownMatchType"sv;
    }

    std::string_view PackageMatchFieldToString(PackageMatchField matchField)
    {
        using namespace std::string_view_literals;

        switch (matchField)
        {
        case PackageMatchField::Command:
            return "Command"sv;
        case PackageMatchField::Id:
            return "Id"sv;
        case PackageMatchField::Moniker:
            return "Moniker"sv;
        case PackageMatchField::Name:
            return "Name"sv;
        case PackageMatchField::Tag:
            return "Tag"sv;
        case PackageMatchField::PackageFamilyName:
            return "PackageFamilyName"sv;
        case PackageMatchField::ProductCode:
            return "ProductCode"sv;
        case PackageMatchField::NormalizedNameAndPublisher:
            return "NormalizedNameAndPublisher"sv;
        case PackageMatchField::Market:
            return "Market"sv;
        }

        return "UnknownMatchField"sv;
    }

    PackageMatchField StringToPackageMatchField(std::string_view field)
    {
        std::string toLower = Utility::ToLower(field);

        if (toLower == "command")
        {
            return PackageMatchField::Command;
        }
        else if (toLower == "id")
        {
            return PackageMatchField::Id;
        }
        else if (toLower == "moniker")
        {
            return PackageMatchField::Moniker;
        }
        else if (toLower == "name")
        {
            return PackageMatchField::Name;
        }
        else if (toLower == "tag")
        {
            return PackageMatchField::Tag;
        }
        else if (toLower == "packagefamilyname")
        {
            return PackageMatchField::PackageFamilyName;
        }
        else if (toLower == "productcode")
        {
            return PackageMatchField::ProductCode;
        }
        else if (toLower == "normalizednameandpublisher")
        {
            return PackageMatchField::NormalizedNameAndPublisher;
        }
        else if (toLower == "market")
        {
            return PackageMatchField::Market;
        }

        return PackageMatchField::Unknown;
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
