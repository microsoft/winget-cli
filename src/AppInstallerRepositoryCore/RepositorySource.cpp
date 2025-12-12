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
#include "SourceUpdateChecks.h"

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "Microsoft/ConfigurableTestSourceFactory.h"
#endif

#include <winget/GroupPolicy.h>

using namespace AppInstaller::Settings;
using namespace std::chrono_literals;
using namespace AppInstaller::Utility::literals;

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

        std::chrono::milliseconds GetMillisecondsToWait(std::chrono::seconds retryAfter, size_t randomMultiplier = 1)
        {
            if (retryAfter != 0s)
            {
                return std::chrono::duration_cast<std::chrono::milliseconds>(retryAfter);
            }
            else
            {
                // Add a bit of randomness to the retry wait time
                std::default_random_engine randomEngine(std::random_device{}());
                std::uniform_int_distribution<long long> distribution(2000, 10000);

                return std::chrono::milliseconds(distribution(randomEngine) * randomMultiplier);
            }
        }

        struct AddOrUpdateResult
        {
            bool UpdateChecked = false;
            bool MetadataWritten = false;
        };

        template <typename MemberFunc>
        AddOrUpdateResult AddOrUpdateFromDetails(SourceDetails& details, MemberFunc member, IProgressCallback& progress)
        {
            AddOrUpdateResult result;

            auto factory = ISourceFactory::GetForType(details.Type);

            // If we are instructed to wait longer than this, just fail rather than retrying.
            constexpr std::chrono::seconds maximumWaitTimeAllowed = 60s;
            std::chrono::seconds waitSecondsForRetry = 0s;

            // Attempt; if it fails, wait a short time and retry.
            try
            {
                result.UpdateChecked = (factory.get()->*member)(details, progress);
                if (result.UpdateChecked)
                {
                    details.LastUpdateTime = std::chrono::system_clock::now();
                    result.MetadataWritten = true;
                }
                return result;
            }
            catch (const Utility::ServiceUnavailableException& sue)
            {
                waitSecondsForRetry = sue.RetryAfter();

                // Do not retry if the server tell us to wait more than the max time allowed.
                if (waitSecondsForRetry > maximumWaitTimeAllowed)
                {
                    details.DoNotUpdateBefore = std::chrono::system_clock::now() + waitSecondsForRetry;
                    AICLI_LOG(Repo, Info, << "Source `" << details.Name << "` unavailable first try, setting DoNotUpdateBefore to " << details.DoNotUpdateBefore);
                    result.MetadataWritten = true;
                    return result;
                }
            }
            CATCH_LOG();

            std::chrono::milliseconds millisecondsToWait = GetMillisecondsToWait(waitSecondsForRetry);

            AICLI_LOG(Repo, Info, << "Source add/update failed, waiting " << millisecondsToWait.count() << " milliseconds and retrying: " << details.Name);

            if (!ProgressCallback::Wait(progress, millisecondsToWait))
            {
                AICLI_LOG(Repo, Info, << "Source second try cancelled.");
                return {};
            }

            try
            {
                // If this one fails, maybe the problem is persistent.
                result.UpdateChecked = (factory.get()->*member)(details, progress);
                if (result.UpdateChecked)
                {
                    details.LastUpdateTime = std::chrono::system_clock::now();
                    result.MetadataWritten = true;
                }
            }
            catch (const Utility::ServiceUnavailableException& sue)
            {
                details.DoNotUpdateBefore = std::chrono::system_clock::now() + GetMillisecondsToWait(sue.RetryAfter(), 3);
                AICLI_LOG(Repo, Info, << "Source `" << details.Name << "` unavailable second try, setting DoNotUpdateBefore to " << details.DoNotUpdateBefore);
                result.MetadataWritten = true;
            }

            return result;
        }

        AddOrUpdateResult AddSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            return AddOrUpdateFromDetails(details, &ISourceFactory::Add, progress);
        }

        AddOrUpdateResult UpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            return AddOrUpdateFromDetails(details, &ISourceFactory::Update, progress);
        }

        AddOrUpdateResult BackgroundUpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
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
            case PredefinedSource::InstalledForceCacheUpdate:
                details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
                details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::NoneWithForcedCacheUpdate);
                return details;
            case PredefinedSource::InstalledUser:
                details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
                details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::User);
                return details;
            case PredefinedSource::InstalledMachine:
                details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
                details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::Machine);
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
            static constexpr ISourceType SourceType = ISourceType::OpenExceptionProxy;

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

            void* CastTo(ISourceType type) override
            {
                if (type == SourceType)
                {
                    return this;
                }

                return nullptr;
            }

        private:
            SourceDetails m_details;
            std::exception_ptr m_exception;
        };

        // A wrapper that doesn't actually forward the search requests.
        struct TrackingOnlySourceWrapper : public ISource
        {
            TrackingOnlySourceWrapper(std::shared_ptr<ISourceReference> wrapped) : m_wrapped(std::move(wrapped))
            {
                m_identifier = m_wrapped->GetIdentifier();
            }

            const std::string& GetIdentifier() const override { return m_identifier; }

            SourceDetails& GetDetails() const override { return m_wrapped->GetDetails(); }

            SourceInformation GetInformation() const override { return m_wrapped->GetInformation(); }

            SearchResult Search(const SearchRequest&) const override { return {}; }

            void* CastTo(ISourceType) override { return nullptr; }

        private:
            std::shared_ptr<ISourceReference> m_wrapped;
            std::string m_identifier;
        };

        // A wrapper to create another wrapper.
        struct TrackingOnlyReferenceWrapper : public ISourceReference
        {
            TrackingOnlyReferenceWrapper(std::shared_ptr<ISourceReference> wrapped) : m_wrapped(std::move(wrapped)) {}

            std::string GetIdentifier() override { return m_wrapped->GetIdentifier(); }

            SourceDetails& GetDetails() override { return m_wrapped->GetDetails(); }

            SourceInformation GetInformation() override { return m_wrapped->GetInformation(); }

            bool SetCustomHeader(std::optional<std::string>) override { return false; }

            void SetCaller(std::string caller) override { m_wrapped->SetCaller(std::move(caller)); }

            std::shared_ptr<ISource> Open(IProgressCallback&) override
            {
                return std::make_shared<TrackingOnlySourceWrapper>(m_wrapped);
            }

        private:
            std::shared_ptr<ISourceReference> m_wrapped;
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

    SourceTrustLevel ConvertToSourceTrustLevelEnum(std::string_view trustLevel)
    {
        std::string lowerTrustLevel = Utility::ToLower(trustLevel);

        if (lowerTrustLevel == "storeorigin")
        {
            return SourceTrustLevel::StoreOrigin;
        }
        else if (lowerTrustLevel == "trusted")
        {
            return SourceTrustLevel::Trusted;
        }
        else if (lowerTrustLevel == "none")
        {
            return SourceTrustLevel::None;
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    std::string_view SourceTrustLevelEnumToString(SourceTrustLevel trustLevel)
    {
        switch (trustLevel)
        {
        case SourceTrustLevel::StoreOrigin:
            return "StoreOrigin"sv;
        case SourceTrustLevel::Trusted:
            return "Trusted"sv;
        case SourceTrustLevel::None:
            return "None"sv;
        }

        return "Unknown"sv;
    }

    SourceTrustLevel ConvertToSourceTrustLevelFlag(std::vector<std::string> trustLevels)
    {
        Repository::SourceTrustLevel result = Repository::SourceTrustLevel::None;
        for (auto& trustLevel : trustLevels)
        {
            Repository::SourceTrustLevel trustLevelEnum = ConvertToSourceTrustLevelEnum(trustLevel);
            if (trustLevelEnum == Repository::SourceTrustLevel::None)
            {
                return Repository::SourceTrustLevel::None;
            }
            else if (trustLevelEnum == Repository::SourceTrustLevel::Trusted)
            {
                WI_SetFlag(result, Repository::SourceTrustLevel::Trusted);
            }
            else if (trustLevelEnum == Repository::SourceTrustLevel::StoreOrigin)
            {
                WI_SetFlag(result, Repository::SourceTrustLevel::StoreOrigin);
            }
            else
            {
                THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), "Invalid source trust level.");
            }
        }

        return result;
    }

    std::vector<std::string_view> SourceTrustLevelFlagToList(SourceTrustLevel trustLevel)
    {
        std::vector<std::string_view> result;

        if (WI_IsFlagSet(trustLevel, Repository::SourceTrustLevel::Trusted))
        {
            result.emplace_back(Repository::SourceTrustLevelEnumToString(Repository::SourceTrustLevel::Trusted));
        }
        if (WI_IsFlagSet(trustLevel, Repository::SourceTrustLevel::StoreOrigin))
        {
            result.emplace_back(Repository::SourceTrustLevelEnumToString(Repository::SourceTrustLevel::StoreOrigin));
        }

        return result;
    }

    std::string GetSourceTrustLevelForDisplay(SourceTrustLevel trustLevel)
    {
        std::vector<std::string_view> trustLevelList = Repository::SourceTrustLevelFlagToList(trustLevel);
        std::vector<Utility::LocIndString> locIndList(trustLevelList.begin(), trustLevelList.end());
        return Utility::Join("|"_liv, locIndList);
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

    std::optional<WellKnownSource> CheckForWellKnownSource(const SourceDetails& sourceDetails)
    {
        return CheckForWellKnownSourceMatch(sourceDetails.Name, sourceDetails.Arg, sourceDetails.Type);
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
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !IsWellKnownSourceEnabled(source));

        auto details = GetWellKnownSourceDetailsInternal(source);

        // Populate metadata
        SourceList sourceList;
        auto sourceDetailsWithMetadata = sourceList.GetSource(details.Name);
        if (sourceDetailsWithMetadata)
        {
            sourceDetailsWithMetadata->CopyMetadataFieldsTo(details);
        }

        m_sourceReferences.emplace_back(CreateSourceFromDetails(details));
    }

    Source::Source(std::string_view name, std::string_view arg, std::string_view type, SourceTrustLevel trustLevel, bool isExplicit)
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
            details.TrustLevel = trustLevel;
            details.Explicit = isExplicit;
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

        std::shared_ptr<CompositeSource> compositeSource = SourceCast<CompositeSource>(availableSource.m_source);

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
                if (!currentSources[0].get().Explicit)
                {
                    AICLI_LOG(Repo, Info, << "Default source requested, only 1 source available, using the only source: " << currentSources[0].get().Name);
                    InitializeSourceReference(currentSources[0].get().Name);
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Skipping explicit source reference " << currentSources[0].get().Name);
                }
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Default source requested, multiple sources available, adding all to source references.");

                for (auto& source : currentSources)
                {
                    if (!source.get().Explicit)
                    {
                        AICLI_LOG(Repo, Info, << "Adding to source references " << source.get().Name);
                        m_sourceReferences.emplace_back(CreateSourceFromDetails(source));
                    }
                    else
                    {
                        AICLI_LOG(Repo, Info, << "Skipping explicit source reference " << source.get().Name);
                    }
                }

                if (m_sourceReferences.size() > 1)
                {
                    m_isComposite = true;
                }
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

    bool Source::operator==(const Source& other) const
    {
        SourceDetails thisDetails = GetDetails();
        SourceDetails otherDetails = other.GetDetails();

        return (thisDetails.Type == otherDetails.Type && thisDetails.Identifier == otherDetails.Identifier);
    }

    bool Source::operator!=(const Source& other) const
    {
        return !operator==(other);
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

    bool Source::QueryFeatureFlag(SourceFeatureFlag flag) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        return m_source->QueryFeatureFlag(flag);
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

    void Source::SetCaller(std::string caller)
    {
        for (auto& sourceReference : m_sourceReferences)
        {
            sourceReference->SetCaller(caller);
        }
    }

    void Source::SetAuthenticationArguments(Authentication::AuthenticationArguments args)
    {
        for (auto& sourceReference : m_sourceReferences)
        {
            sourceReference->SetAuthenticationArguments(args);
        }
    }

    void Source::SetThreadGlobals(const std::shared_ptr<ThreadLocalStorage::ThreadGlobals>& threadGlobals)
    {
        for (auto& sourceReference : m_sourceReferences)
        {
            sourceReference->SetThreadGlobals(threadGlobals);
        }
    }

    void Source::SetBackgroundUpdateInterval(TimeSpan interval)
    {
        m_backgroundUpdateInterval = interval;
    }

    void Source::InstalledPackageInformationOnly(bool value)
    {
        m_installedPackageInformationOnly = value;
    }

    bool Source::IsWellKnownSource(WellKnownSource wellKnownSource)
    {
        SourceDetails details = GetDetails();
        auto wellKnown = CheckForWellKnownSourceMatch(details.Name, details.Arg, details.Type);
        return wellKnown && wellKnown.value() == wellKnownSource;
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

        auto compositeSource = SourceCast<CompositeSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !compositeSource);

        return compositeSource->GetAvailableSources();
    }

    void Source::AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        auto writableSource = SourceCast<IMutablePackageSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !writableSource);
        writableSource->AddPackageVersion(manifest, relativePath);
    }

    void Source::RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_source);
        auto writableSource = SourceCast<IMutablePackageSource>(m_source);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !writableSource);
        writableSource->RemovePackageVersion(manifest, relativePath);
    }

    std::vector<SourceDetails> Source::Open(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_isSourceToBeAdded || m_sourceReferences.empty());

        std::vector<SourceDetails> result;

        if (!m_source)
        {
            std::vector<std::shared_ptr<ISourceReference>>* sourceReferencesToOpen = nullptr;
            std::vector<std::shared_ptr<ISourceReference>> sourceReferencesForTrackingOnly;
            std::unique_ptr<SourceList> sourceList;

            if (m_installedPackageInformationOnly)
            {
                sourceReferencesToOpen = &sourceReferencesForTrackingOnly;

                // Create a wrapper for each reference
                for (auto& sourceReference : m_sourceReferences)
                {
                    sourceReferencesForTrackingOnly.emplace_back(std::make_shared<TrackingOnlyReferenceWrapper>(sourceReference));
                }
            }
            else
            {
                // Check for updates before opening.
                for (auto& sourceReference : m_sourceReferences)
                {
                    if (ShouldUpdateBeforeOpen(sourceReference.get(), m_backgroundUpdateInterval))
                    {
                        auto& details = sourceReference->GetDetails();

                        try
                        {
                            // TODO: Consider adding a context callback to indicate we are doing the same action
                            // to avoid the progress bar fill up multiple times.
                            AddOrUpdateResult updateResult = BackgroundUpdateSourceFromDetails(details, progress);

                            if (updateResult.MetadataWritten)
                            {
                                if (sourceList == nullptr)
                                {
                                    sourceList = std::make_unique<SourceList>();
                                }

                                auto detailsInternal = sourceList->GetSource(details.Name);
                                detailsInternal->CopyMetadataFieldsFrom(details);
                                sourceList->SaveMetadata(*detailsInternal);
                            }

                            if (!updateResult.UpdateChecked)
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

                sourceReferencesToOpen = &m_sourceReferences;
            }

            if (sourceReferencesToOpen->size() > 1)
            {
                AICLI_LOG(Repo, Info, << "Multiple sources available, creating aggregated source.");
                auto aggregatedSource = std::make_shared<CompositeSource>("*DefaultSource");
                std::vector<std::shared_ptr<OpenExceptionProxy>> openExceptionProxies;

                for (auto& sourceReference : *sourceReferencesToOpen)
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
                m_source = (*sourceReferencesToOpen)[0]->Open(progress);
            }
        }

        return result;
    }

    bool Source::Add(IProgressCallback& progress)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_isSourceToBeAdded || m_sourceReferences.size() != 1);

        auto& sourceDetails = m_sourceReferences[0]->GetDetails();

        // If the source type is empty, use a default.
        // AddSourceForDetails will also check for empty, but we need the actual type before that for validation.
        if (sourceDetails.Type.empty())
        {
            sourceDetails.Type = GetDefaultSourceType();
        }

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

        bool result = AddSourceFromDetails(sourceDetails, progress).UpdateChecked;
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
                AddOrUpdateResult updateResult = UpdateSourceFromDetails(details, progress);

                if (updateResult.MetadataWritten)
                {
                    auto detailsInternal = sourceList.GetSource(details.Name);
                    detailsInternal->CopyMetadataFieldsFrom(details);
                    sourceList.SaveMetadata(*detailsInternal);
                }

                if (!updateResult.UpdateChecked)
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
        // With C++20, consider removing the shared_ptr here and making the one inside PackageTrackingCatalog atomic.
        std::shared_ptr<PackageTrackingCatalog> currentTrackingCatalog = std::atomic_load(&m_trackingCatalog);
        if (!currentTrackingCatalog)
        {
            std::shared_ptr<PackageTrackingCatalog> newTrackingCatalog = std::make_shared<PackageTrackingCatalog>(PackageTrackingCatalog::CreateForSource(*this));

            if (std::atomic_compare_exchange_strong(&m_trackingCatalog, &currentTrackingCatalog, newTrackingCatalog))
            {
                currentTrackingCatalog = newTrackingCatalog;
            }
        }

        return *currentTrackingCatalog;
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

    std::string_view Source::GetDefaultSourceType()
    {
        return ISourceFactory::GetForType("")->TypeName();
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
