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
#include "PackageTrackingCatalogSourceFactory.h"

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

        std::shared_ptr<ISourceReference> CreateSourceFromDetails(const SourceDetails& details)
        {
            return ISourceFactory::GetForType(details.Type)->Create(details);
        }

        template <typename MemberFunc>
        bool AddOrUpdateFromDetails(SourceDetails& details, MemberFunc member, IProgressCallback& progress)
        {
            bool result = false;
            auto factory = ISourceFactory::GetForType(details.Type);

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
            auto factory = ISourceFactory::GetForType(details.Type);

            return factory->Remove(details, progress);
        }

        bool ContainsAvailablePackagesInternal(SourceOrigin origin)
        {
            return (origin == SourceOrigin::Default || origin == SourceOrigin::GroupPolicy || origin == SourceOrigin::User);
        }

        // Determines whether (and logs why) a source should be updated before it is opened.
        bool ShouldUpdateBeforeOpen(const SourceDetails& details)
        {
            if (!ContainsAvailablePackagesInternal(details.Origin))
            {
                return false;
            }

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
                result.Failures.emplace_back(SearchResult::Failure{ GetDetails().Name, m_exception });
                return result;
            }

        private:
            SourceDetails m_details;
            std::exception_ptr m_exception;
        };
    }

    std::unique_ptr<ISourceFactory> ISourceFactory::GetForType(std::string_view type)
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
        // Should always come from code, so no need for case insensitivity
        else if (PackageTrackingCatalogSourceFactory::Type() == type)
        {
            return PackageTrackingCatalogSourceFactory::Create();
        }
        else if (Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), type))
        {
            return Rest::RestSourceFactory::Create();
        }

        THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE);
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

    Source::Source() {}

    Source::Source(std::string_view name)
    {
        InitializeSourceReference(name);
    }

    Source::Source(PredefinedSource source)
    {
        SourceDetails details = GetPredefinedSourceDetails(source);
        m_sourceReferences.emplace_back(CreateSourceFromDetails(details));
    }

    Source::Source(WellKnownSource source)
    {
        SourceDetails details = GetWellKnownSourceDetailsInternal(source);
        m_sourceReferences.emplace_back(CreateSourceFromDetails(details));
    }

    Source::Source(std::string_view name, std::string_view arg, std::string_view type)
    {
        m_isSourceToBeAdded = true;
        SourceDetails details;

        std::optional<WellKnownSource> wellKnownSourceCheck = CheckForWellKnownSourceMatch(name, arg, type);

        if (wellKnownSourceCheck)
        {
            details = GetWellKnownSourceDetailsInternal(wellKnownSourceCheck.value());
        }
        else
        {
            details.Name = name;
            details.Arg = arg;
            details.Type = type;
        }

        m_sourceReferences.emplace_back(CreateSourceFromDetails(details));
    }

    Source::Source(const std::vector<Source>& availableSources)
    {
        std::shared_ptr<CompositeSource> compositeSource = std::make_shared<CompositeSource>("*CompositeSource");

        for (const auto& availableSource : availableSources)
        {
            THROW_HR_IF(E_INVALIDARG, !availableSource.m_source || availableSource.IsComposite());
            compositeSource->AddAvailableSource(availableSource.m_source);
        }

        m_source = compositeSource;
        m_isComposite = true;
    }

    Source::Source(const Source& installedSource, const Source& availableSource, CompositeSearchBehavior searchBehavior)
    {
        THROW_HR_IF(E_INVALIDARG, !installedSource.m_source || installedSource.m_isComposite || !availableSource.m_source);

        std::shared_ptr<CompositeSource> compositeSource = std::dynamic_pointer_cast<CompositeSource>(availableSource.m_source);

        if (!compositeSource)
        {
            compositeSource = std::make_shared<CompositeSource>("*CompositeSource");
            compositeSource->AddAvailableSource(availableSource.m_source);
        }

        compositeSource->SetInstalledSource(installedSource, searchBehavior);

        m_source = compositeSource;
        m_isComposite = true;
    }

    Source::Source(std::shared_ptr<ISource> source) : m_source(std::move(source)) {}

    Source::operator bool() const
    {
        return !m_sourceReferences.empty() || m_source != nullptr;
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
            }
            else if (currentSources.size() == 1)
            {
                AICLI_LOG(Repo, Info, << "Default source requested, only 1 source available, using the only source: " << currentSources[0].get().Name);
                InitializeSourceReference(currentSources[0].get().Name);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Default source requested, multiple sources available, adding all to source references.");

                for (auto& source : currentSources)
                {
                    AICLI_LOG(Repo, Info, << "Adding to source references " << source.get().Name);
                    m_sourceReferences.emplace_back(CreateSourceFromDetails(source));
                }

                m_isComposite = true;
            }
        }
        else
        {
            auto source = sourceList.GetCurrentSource(name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source requested, but not found: " << name);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source requested, found: " << source->Name);
                m_sourceReferences.emplace_back(CreateSourceFromDetails(*source));
            }
        }
    }

    std::string Source::GetIdentifier() const
    {
        if (m_source)
        {
            return m_source->GetIdentifier();
        }
        else if (m_sourceReferences.size() == 1)
        {
            return m_sourceReferences[0]->GetIdentifier();
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
    }

    SourceDetails Source::GetDetails() const
    {
        if (m_source)
        {
            return m_source->GetDetails();
        }
        else if (m_sourceReferences.size() == 1)
        {
            return m_sourceReferences[0]->GetDetails();
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
    }

    SourceInformation Source::GetInformation() const
    {
        if (m_source && !m_isComposite)
        {
            return m_source->GetInformation();
        }
        else if (m_sourceReferences.size() == 1)
        {
            return m_sourceReferences[0]->GetInformation();
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
    }

    bool Source::ContainsAvailablePackages() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), IsComposite());
        return ContainsAvailablePackagesInternal(GetDetails().Origin);
    }

    bool Source::SetCustomHeader(std::optional<std::string> header)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_sourceReferences.size() != 1);
        return m_sourceReferences[0]->SetCustomHeader(header);
    }

    SearchResult Source::Search(const SearchRequest& request) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->Search(request);
    }

    ImplicitAgreementFieldEnum Source::GetAgreementFieldsFromSourceInformation() const
    {
        ImplicitAgreementFieldEnum result = ImplicitAgreementFieldEnum::None;

        auto info = GetInformation();
        if (info.RequiredPackageMatchFields.end() != std::find_if(info.RequiredPackageMatchFields.begin(), info.RequiredPackageMatchFields.end(), [&](const auto& field) { return Utility::CaseInsensitiveEquals(field, "market"); }) ||
            info.RequiredQueryParameters.end() != std::find_if(info.RequiredQueryParameters.begin(), info.RequiredQueryParameters.end(), [&](const auto& param) { return Utility::CaseInsensitiveEquals(param, "market"); }))
        {
            WI_SetFlag(result, ImplicitAgreementFieldEnum::Market);
        }

        return result;
    }

    bool Source::CheckSourceAgreements() const
    {
        auto sourceName = GetDetails().Name;
        auto agreementFields = GetAgreementFieldsFromSourceInformation();
        auto agreementsIdentifier = GetInformation().SourceAgreementsIdentifier;

        SourceList sourceList;
        return sourceList.CheckSourceAgreements(sourceName, agreementsIdentifier, agreementFields);
    }

    void Source::SaveAcceptedSourceAgreements() const
    {
        auto sourceName = GetDetails().Name;
        auto agreementFields = GetAgreementFieldsFromSourceInformation();
        auto agreementsIdentifier = GetInformation().SourceAgreementsIdentifier;

        SourceList sourceList;
        return sourceList.SaveAcceptedSourceAgreements(sourceName, agreementsIdentifier, agreementFields);
    }

    bool Source::IsComposite() const
    {
        return m_isComposite;
    }

    std::vector<Source> Source::GetAvailableSources() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source || !m_isComposite);

        auto compositeSource = std::dynamic_pointer_cast<CompositeSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !compositeSource);

        return compositeSource->GetAvailableSources();
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

    std::vector<SourceDetails> Source::Open(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_isSourceToBeAdded || m_sourceReferences.empty());

        std::vector<SourceDetails> result;

        if (!m_source)
        {
            std::unique_ptr<SourceList> sourceList;

            // Check for updates before opening.
            for (auto& sourceReference : m_sourceReferences)
            {
                auto& details = sourceReference->GetDetails();
                if (ShouldUpdateBeforeOpen(details))
                {
                    try
                    {
                        // TODO: Consider adding a context callback to indicate we are doing the same action
                        // to avoid the progress bar fill up multiple times.
                        if (BackgroundUpdateSourceFromDetails(details, progress))
                        {
                            if (sourceList == nullptr)
                            {
                                sourceList = std::make_unique<SourceList>();
                            }

                            auto detailsInternal = sourceList->GetSource(details.Name);
                            detailsInternal->LastUpdateTime = details.LastUpdateTime;
                            sourceList->SaveMetadata(*detailsInternal);
                        }
                        else
                        {
                            AICLI_LOG(Repo, Error, << "Failed to update source: " << details.Name);
                            result.emplace_back(details);
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

            if (m_sourceReferences.size() > 1)
            {
                AICLI_LOG(Repo, Info, << "Multiple sources available, creating aggregated source.");
                auto aggregatedSource = std::make_shared<CompositeSource>("*DefaultSource");
                std::vector<std::shared_ptr<OpenExceptionProxy>> openExceptionProxies;

                for (auto& sourceReference : m_sourceReferences)
                {
                    AICLI_LOG(Repo, Info, << "Adding to aggregated source: " << sourceReference->GetDetails().Name);

                    try

                    {
                        aggregatedSource->AddAvailableSource(sourceReference->Open(progress));
                    }
                    catch (...)
                    {
                        LOG_CAUGHT_EXCEPTION();
                        AICLI_LOG(Repo, Warning, << "Failed to open available source: " << sourceReference->GetDetails().Name);
                        openExceptionProxies.emplace_back(std::make_shared<OpenExceptionProxy>(sourceReference->GetDetails(), std::current_exception()));
                    }
                }

                // If all sources failed to open, then throw an exception that is specific to this case.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES, !aggregatedSource->HasAvailableSource());

                // Place all of the proxies into the source to be searched later
                for (auto& proxy : openExceptionProxies)
                {
                    aggregatedSource->AddAvailableSource(Source{ std::move(proxy) });
                }

                m_source = aggregatedSource;
            }
            else
            {
                m_source = m_sourceReferences[0]->Open(progress);
            }
        }

        return result;
    }

    bool Source::Add(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_isSourceToBeAdded || m_sourceReferences.size() != 1);

        auto& sourceDetails = m_sourceReferences[0]->GetDetails();

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << sourceDetails.Name << "], Type[" << sourceDetails.Type << "], Arg[" << sourceDetails.Arg << "]");

        // Check all sources for the given name.
        SourceList sourceList;

        auto source = sourceList.GetSource(sourceDetails.Name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, source != nullptr && source->Origin != SourceOrigin::Metadata && !source->IsTombstone);

        // Check sources allowed by group policy
        auto blockingPolicy = GetPolicyBlockingUserSource(sourceDetails.Name, sourceDetails.Type, sourceDetails.Arg, false);
        if (blockingPolicy != TogglePolicy::Policy::None)
        {
            throw GroupPolicyException(blockingPolicy);
        }

        sourceDetails.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);

        // Allow the origin to stay as Default if the incoming details match a well known value
        if (!(sourceDetails.Origin == SourceOrigin::Default && CheckForWellKnownSourceMatch(sourceDetails.Name, sourceDetails.Arg, sourceDetails.Type)))
        {
            sourceDetails.Origin = SourceOrigin::User;
        }

        bool result = AddSourceFromDetails(sourceDetails, progress);
        if (result)
        {
            sourceList.AddSource(sourceDetails);
            SaveAcceptedSourceAgreements();
            m_isSourceToBeAdded = false;
            AICLI_LOG(Repo, Info, << "Source created with extra data: " << sourceDetails.Data);
        }

        return result;
    }

    std::vector<SourceDetails> Source::Update(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_isSourceToBeAdded || m_source || m_sourceReferences.empty());

        SourceList sourceList;
        std::vector<SourceDetails> result;

        for (auto& sourceReference : m_sourceReferences)
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !ContainsAvailablePackagesInternal(sourceReference->GetDetails().Origin));

            auto& details = sourceReference->GetDetails();
            AICLI_LOG(Repo, Info, << "Named source to be updated, found: " << details.Name);

            try
            {
                // TODO: Consider adding a context callback to indicate we are doing the same action
                // to avoid the progress bar fill up multiple times.
                if (UpdateSourceFromDetails(details, progress))
                {
                    auto detailsInternal = sourceList.GetSource(details.Name);
                    detailsInternal->LastUpdateTime = details.LastUpdateTime;
                    sourceList.SaveMetadata(*detailsInternal);
                }
                else
                {
                    AICLI_LOG(Repo, Error, << "Failed to update source: " << details.Name);
                    result.emplace_back(details);
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
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_isSourceToBeAdded || m_sourceReferences.size() != 1 || m_source);

        const auto& details = m_sourceReferences[0]->GetDetails();
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

    PackageTrackingCatalog Source::GetTrackingCatalog() const
    {
        if (!m_trackingCatalog)
        {
            m_trackingCatalog = PackageTrackingCatalog::CreateForSource(*this);
        }

        return m_trackingCatalog;
    }

    std::vector<SourceDetails> Source::GetCurrentSources()
    {
        SourceList sourceList;

        std::vector<SourceDetails> result;
        for (auto&& source : sourceList.GetCurrentSourceRefs())
        {
            result.emplace_back(std::move(source));
        }

        return result;
    }

    bool Source::DropSource(std::string_view name)
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
