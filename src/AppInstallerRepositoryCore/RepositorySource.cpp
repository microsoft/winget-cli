// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ISource.h"
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

        std::shared_ptr<ISource> CreateSourceFromDetails(const SourceDetails& details)
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

        // Carries the exception from an OpenSource call and presents it back at search time.
        struct OpenExceptionProxy : public ISource, std::enable_shared_from_this<OpenExceptionProxy>
        {
            OpenExceptionProxy(const std::string& identifier, const SourceDetails& details, std::exception_ptr exception) :
                m_identifier(identifier), m_details(details), m_exception(std::move(exception)) {}

            SourceDetails& GetDetails() override { return m_details; }

            const std::string& GetIdentifier() const override { return m_identifier; }

            SearchResult Search(const SearchRequest&) const override
            {
                SearchResult result;
                result.Failures.emplace_back(SearchResult::Failure{ shared_from_this(), m_exception });
                return result;
            }

        private:
            std::string m_identifier;
            SourceDetails m_details;
            std::exception_ptr m_exception;
        };

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
    }

    std::string_view ToString(SourceOrigin origin)
    {
        switch (origin)
        {
        case SourceOrigin::Default:
            return "Default"sv;
        case SourceOrigin::User:
            return "User"sv;
        case SourceOrigin::Predefined:
            return "Predefined"sv;
        case SourceOrigin::GroupPolicy:
            return "GroupPolicy"sv;
        case SourceOrigin::Metadata:
            return "Metadata"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    Source::Source()
    {
        InitializeSourceReference(""sv);
    }

    Source::Source(std::string_view name)
    {
        m_isNamedSource = !name.empty();
        InitializeSourceReference(name);
    }

    Source::Source(PredefinedSource source)
    {
        SourceDetails details = GetPredefinedSourceDetails(source);
        m_source = CreateSourceFromDetails(details);
    }

    Source::Source(WellKnownSource source)
    {
        SourceDetails details = GetWellKnownSourceDetailsInternal(source);
        m_source = CreateSourceFromDetails(details);
    }

    Source::Source(std::string_view name, std::string_view arg, std::string_view type)
    {
        SourceDetails details;
        details.Name = name;
        details.Arg = arg;
        details.Type = type;
        m_source = CreateSourceFromDetails(details);
    }

    Source::Source(const std::vector<Source>& availableSources)
    {
        std::shared_ptr<CompositeSource> compositeSource = std::make_shared<CompositeSource>("*CompositeSource");

        for (const auto& availableSource : availableSources)
        {
            THROW_HR_IF(E_INVALIDARG, !availableSource);
            compositeSource->AddAvailableSource(availableSource.m_source);
        }

        m_source = compositeSource;
    }

    Source::Source(const Source& installedSource, const Source& availableSource, CompositeSearchBehavior searchBehavior)
    {
        THROW_HR_IF(E_INVALIDARG, !installedSource || !availableSource);

        std::shared_ptr<CompositeSource> compositeSource = std::dynamic_pointer_cast<CompositeSource>(availableSource.m_source);

        if (!compositeSource)
        {
            compositeSource = std::make_shared<CompositeSource>("*CompositeSource");
            compositeSource->AddAvailableSource(availableSource.m_source);
        }

        compositeSource->SetInstalledSource(installedSource.m_source, searchBehavior);

        m_source = compositeSource;
    }

    Source::Source(std::shared_ptr<ISource> source, bool isNamedSource) : m_source(std::move(source)), m_isNamedSource(isNamedSource) {}

    Source::operator bool() const
    {
        return m_source != nullptr;
    }

    void Source::InitializeSourceReference(std::string_view name)
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

    const std::string Source::GetIdentifier() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->GetIdentifier();
    }

    const SourceDetails Source::GetDetails() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source || m_source->IsComposite());
        return m_source->GetDetails();
    }

    const SourceInformation Source::GetInformation() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source || m_source->IsComposite());
        return m_source->GetInformation();
    }

    bool Source::SetCustomHeader(std::string_view header)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->SetCustomHeader(header);
    }

    SearchResult Source::Search(const SearchRequest& request) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->Search(request);
    }

    ImplicitAgreementFieldEnum Source::GetAgreementFieldsFromSourceInformation()
    {
        auto info = GetInformation();

        ImplicitAgreementFieldEnum result = ImplicitAgreementFieldEnum::None;

        if (info.RequiredPackageMatchFields.end() != std::find_if(info.RequiredPackageMatchFields.begin(), info.RequiredPackageMatchFields.end(), [&](const auto& field) { return Utility::CaseInsensitiveEquals(field, "market"); }) ||
            info.RequiredQueryParameters.end() != std::find_if(info.RequiredQueryParameters.begin(), info.RequiredQueryParameters.end(), [&](const auto& param) { return Utility::CaseInsensitiveEquals(param, "market"); }))
        {
            WI_SetFlag(result, ImplicitAgreementFieldEnum::Market);
        }

        return result;
    }

    bool Source::CheckSourceAgreements()
    {
        auto configuration = GetDetails();
        auto agreementFields = GetAgreementFieldsFromSourceInformation();
        auto info = GetInformation();

        SourceList sourceList;
        return sourceList.CheckSourceAgreements(configuration.Name, info.SourceAgreementsIdentifier, agreementFields);
    }

    void Source::SaveAcceptedSourceAgreements()
    {
        auto configuration = GetDetails();
        auto agreementFields = GetAgreementFieldsFromSourceInformation();
        auto info = GetInformation();

        SourceList sourceList;
        return sourceList.SaveAcceptedSourceAgreements(configuration.Name, info.SourceAgreementsIdentifier, agreementFields);
    }

    bool Source::IsComposite() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->IsComposite();
    }

    std::vector<Source> Source::GetAvailableSources()
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source || !m_source->IsComposite());

        std::vector<Source> result;
        for (auto const& source : m_source->GetAvailableSources())
        {
            result.emplace_back(source, true);
        }

        return result;
    }

    void Source::AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        auto writableSource = std::dynamic_pointer_cast<IMutablePackageSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !writableSource);
        writableSource->AddPackageVersion(manifest, relativePath);
    }

    void Source::RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        auto writableSource = std::dynamic_pointer_cast<IMutablePackageSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !writableSource);
        writableSource->RemovePackageVersion(manifest, relativePath);
    }

    std::vector<SourceDetails> Source::Open(IProgressCallback& progress, bool skipUpdateBeforeOpen)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);

        SourceList sourceList;
        std::vector<SourceDetails> result;

        if (!skipUpdateBeforeOpen)
        {
            std::vector<std::reference_wrapper<SourceDetails>> sources;
            if (m_source->IsComposite())
            {
                for (auto const& source : m_source->GetAvailableSources())
                {
                    sources.emplace_back(source->GetDetails());
                }
            }
            else
            {
                sources.emplace_back(m_source->GetDetails());
            }

            for (SourceDetails& details : sources)
            {
                if (ShouldUpdateBeforeOpen(details))
                {
                    try
                    {
                        // TODO: Consider adding a context callback to indicate we are doing the same action
                        // to avoid the progress bar fill up multiple times.
                        if (BackgroundUpdateSourceFromDetails(details, progress))
                        {
                            sourceList.SaveMetadata(details);
                        }
                    }
                    catch (...)
                    {
                        LOG_CAUGHT_EXCEPTION();
                        AICLI_LOG(Repo, Warning, << "Failed to update source: " << details.Name);
                        result.emplace_back(details);
                    }
                }
            }
        }

        m_source->Open(progress);

        return result;
    }

    bool Source::Add(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);

        SourceDetails sourceDetails = m_source->GetDetails();

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << sourceDetails.Name << "], Type[" << sourceDetails.Type << "], Arg[" << sourceDetails.Arg << "]");

        // Check all sources for the given name.
        SourceList sourceList;

        auto source = sourceList.GetCurrentSource(sourceDetails.Name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, source != nullptr);

        // Check for a hidden source data that we don't want to collide.
        // TODO: Refactor the source interface so that we don't do this
        auto hiddenSource = sourceList.GetSource(sourceDetails.Name);
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

            sourceList.AddSource(sourceDetails);
        }

        return result;
    }

    std::vector<SourceDetails> Source::Update(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);

        SourceList sourceList;
        std::vector<SourceDetails> result;

        std::vector<std::reference_wrapper<SourceDetails>> sources;
        if (m_source->IsComposite())
        {
            for (auto const& source : m_source->GetAvailableSources())
            {
                sources.emplace_back(source->GetDetails());
            }
        }
        else
        {
            sources.emplace_back(m_source->GetDetails());
        }

        for (SourceDetails& details : sources)
        {
            try
            {
                AICLI_LOG(Repo, Info, << "Named source to be updated, found: " << details.Name);

                // TODO: Consider adding a context callback to indicate we are doing the same action
                // to avoid the progress bar fill up multiple times.
                if (UpdateSourceFromDetails(details, progress))
                {
                    sourceList.SaveMetadata(details);
                }
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION();
                AICLI_LOG(Repo, Error, << "Failed to update source: " << details.Name);
                result.emplace_back(details);
            }
        }

        return result;
    }

    bool Source::Remove(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source || !m_isNamedSource);

        const auto& details = m_source->GetDetails();
        AICLI_LOG(Repo, Info, << "Named source to be removed, found: " << details.Name << " [" << ToString(details.Origin) << ']');

        EnsureSourceIsRemovable(details);

        bool result = RemoveSourceFromDetails(details, progress);
        if (result)
        {
            SourceList sourceList;
            sourceList.RemoveSource(details);
        }

        return result;
    }

    void Source::Drop()
    {
        if (m_isNamedSource)
        {
            SourceList sourceList;
            const auto& details = m_source->GetDetails();

            AICLI_LOG(Repo, Info, << "Named source to be dropped, found: " << details.Name);

            EnsureSourceIsRemovable(details);
            sourceList.RemoveSource(details);
        }
        else
        {
            SourceList::RemoveSettingsStreams();
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
