// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Microsoft/PreIndexed/IndexForm.h"
#include "Microsoft/PreIndexed/PackageStore.h"
#include "Microsoft/PreIndexed/SourceData.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "SourceUpdateChecks.h"

#include <AppInstallerDateTime.h>
#include <winget/ExperimentalFeature.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace AppInstaller::Repository::Microsoft::PreIndexed;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // Whether a source may be composed of a delta and a baseline rather than a full index.
        bool IsDeltaIndexEnabled()
        {
            return Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::DeltaIndex);
        }

        // Selects the form to use when opening a source.
        std::unique_ptr<IIndexForm> SelectFormForOpen(const SourceDetails& details)
        {
            if (IsDeltaIndexEnabled())
            {
                auto deltaForm = CreateDeltaIndexForm(details);

                // While experimental, lock on to delta if we have the bare minimum.
                // Falling back to the full form is quite complex and may result in hiding issues.
                if (deltaForm->HasIdentities())
                {
                    return deltaForm;
                }
            }

            return CreateFullIndexForm(details);
        }

        // Measures how long opening a source took, and which path got there.
        struct SourceOpenTimer
        {
            using clock = std::chrono::steady_clock;

            SourceOpenTimer(const std::string& sourceName) : m_sourceName(sourceName), m_start(clock::now()) {}

            ~SourceOpenTimer()
            {
                const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_start).count();
                AICLI_LOG(Repo, Info, << "Packaged source open for '" << m_sourceName << "' " << (m_succeeded ? "succeeded" : "failed") <<
                    " in " << totalMs << " ms [mode=" << (m_usedLockFallback ? "fallbackLocked" : "optimistic") << "]");
            }

            void MarkFallbackLocked() { m_usedLockFallback = true; }
            void MarkSucceeded() { m_succeeded = true; }

        private:
            const std::string& m_sourceName;
            clock::time_point m_start;
            bool m_succeeded = false;
            bool m_usedLockFallback = false;
        };

        // A reference to a preindexed package source.
        struct PreIndexedSourceReference : public ISourceReference
        {
            PreIndexedSourceReference(const SourceDetails& details) : m_details(details)
            {
                if (m_details.Identifier.empty() && !m_details.Data.empty())
                {
                    // We didn't use to store the source identifier, so we compute it here in case
                    // it's missing from the details.
                    m_details.Identifier = SourceData{ m_details.Data }.SourceIdentity();
                }

                m_form = SelectFormForOpen(m_details);
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            bool ShouldUpdateBeforeOpen(const std::optional<TimeSpan>& requestedUpdateInterval) override
            {
                // Nothing here reports progress, but deciding whether the source holds usable data
                // can require reading what it holds.
                ProgressCallback progress;

                auto store = CreateCompositeStore(m_details);
                auto currentVersion = m_form->GetHeldVersion(*store);

                // If we can't find a good package, then we have to update to operate
                if (!currentVersion)
                {
                    AICLI_LOG(Repo, Verbose, << "Source `" << m_details.Name << "` has no data");
                    return true;
                }

                // Check that the form is consistent with itself
                if (!m_form->IsUsable(*store, progress))
                {
                    AICLI_LOG(Repo, Verbose, << "Source `" << m_details.Name << "` is not currently usable");
                    return true;
                }

                using namespace std::chrono_literals;
                using clock = std::chrono::system_clock;

                // Attempt to convert the package version to a time_point
                clock::time_point versionTime = Utility::GetTimePointFromVersion(currentVersion.value());

                // Since we expect that the version time indicates creation time, don't let it be far in the future.
                auto now = clock::now();
                if (versionTime > now && versionTime - now > 24h)
                {
                    versionTime = clock::time_point::min();
                }

                // Use the later of the version and last update times
                clock::time_point timeToCheck = (versionTime > m_details.LastUpdateTime ? versionTime : m_details.LastUpdateTime);

                return IsAfterUpdateCheckTime(m_details.Name, timeToCheck, requestedUpdateInterval);
            }

            std::shared_ptr<ISource> Open(IProgressCallback& progress) override
            {
                SourceOpenTimer openTimer{ m_details.Name };

                auto store = CreateCompositeStore(m_details);

                std::optional<SQLiteIndex> index;

                if (store->AllowsUnlockedRead())
                {
                    // The optimistic open reads in place, so it is attempted without the lock and
                    // retried under it only when that fails.
                    try
                    {
                        index.emplace(m_form->Open(*store, progress));
                    }
                    catch (...)
                    {
                        if (progress.IsCancelledBy(CancelReason::Any))
                        {
                            throw;
                        }

                        LOG_CAUGHT_EXCEPTION_MSG("Optimistic packaged source open failed, retrying under lock for source: %hs", m_details.Name.c_str());
                    }
                }

                if (!index)
                {
                    openTimer.MarkFallbackLocked();

                    auto lock = store->Lock(progress);
                    if (!lock)
                    {
                        return {};
                    }

                    index.emplace(m_form->Open(*store, progress));
                }

                if (progress.IsCancelledBy(CancelReason::Any))
                {
                    AICLI_LOG(Repo, Info, << "Cancelling open upon request");
                    return {};
                }

                openTimer.MarkSucceeded();
                return std::make_shared<SQLiteIndexSource>(m_details, std::move(index.value()), false, true);
            }

        private:
            SourceDetails m_details;
            std::unique_ptr<IIndexForm> m_form;
        };

        // The factory for a preindexed package source.
        struct PreIndexedFactory : public ISourceFactory
        {
            std::string_view TypeName() const override
            {
                return PreIndexedPackageSourceFactory::Type();
            }

            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override
            {
                // With more than one source implementation, we will probably need to probe first
                THROW_HR_IF(E_INVALIDARG, !details.Type.empty() && details.Type != PreIndexedPackageSourceFactory::Type());

                return std::make_shared<PreIndexedSourceReference>(details);
            }

            bool Add(SourceDetails& details, IProgressCallback& progress) override
            {
                if (details.Type.empty())
                {
                    // With more than one source implementation, we will probably need to probe first
                    details.Type = PreIndexedPackageSourceFactory::Type();
                    AICLI_LOG(Repo, Info, << "Initializing source type: " << details.Name << " => " << details.Type);
                }
                else
                {
                    THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());
                }

                // Adding is the only operation that can establish what a source's packages are, so
                // it is the only one that discovers them and writes the result back.
                UpdateReport report;
                UpdateResult result = UpdateResult::Unusable;

                if (IsDeltaIndexEnabled())
                {
                    try
                    {
                        auto form = CreateDeltaIndexForm(details);
                        auto data = form->DiscoverIdentities(progress);

                        if (data)
                        {
                            WriteDiscoveredIdentities(details, data.value());

                            auto store = CreateStore(details);
                            result = form->Update(*store, false, progress, report);

                            if (report.LearnedIdentities)
                            {
                                // The update found the rest of what the source is made of. The
                                // identity the source is keyed on does not move, since it is
                                // derived from the delta and that was known from the first call.
                                WriteDiscoveredIdentities(details, report.LearnedIdentities.value());
                            }

                            if (result != UpdateResult::Success)
                            {
                                // The source is about to settle on the full index instead, and a
                                // form that was given up on is not something the source will ever
                                // come back to. Whatever the attempt left behind is keyed on the
                                // delta rather than on the full index, so nothing the full index
                                // does afterwards would displace it.
                                DiscardPackagesOf(details, *store, *form, progress);
                            }
                        }
                    }
                    catch (...)
                    {
                        if (progress.IsCancelledBy(CancelReason::Any))
                        {
                            throw;
                        }

                        LOG_CAUGHT_EXCEPTION_MSG("Delta discovery failed while adding source: %hs", details.Name.c_str());
                        result = UpdateResult::Unusable;
                    }
                }

                if (result == UpdateResult::Unusable)
                {
                    // Either the source publishes no delta, or the one it publishes cannot serve
                    // it. Adding must still succeed, so the full index is acquired instead.
                    report = {};

                    // Nothing of the delta attempt is kept: the source is settling on the full
                    // index, and Data naming a delta is what would send it back to the other form.
                    details.Data.clear();
                    details.Identifier.clear();

                    auto form = CreateFullIndexForm(details);
                    auto data = form->DiscoverIdentities(progress);
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING, !data);

                    WriteDiscoveredIdentities(details, data.value());

                    auto store = CreateStore(details);
                    result = form->Update(*store, false, progress, report);
                }

                // Unlike an update, an add reports only when something was transferred.
                if (report.DownloadedBytes && report.NewIndexPublishedAt)
                {
                    try
                    {
                        Logging::Telemetry().LogPreindexedPackageUpdate(
                            details.Identifier,
                            report.PreviousIndexPublishedAt,
                            report.NewIndexPublishedAt.value(),
                            report.UsedDeltaDownload,
                            report.PreviousBaselinePublishedAt,
                            report.NewBaselinePublishedAt,
                            report.BaselineUpdated,
                            report.DownloadedBytes.value(),
                            true);
                    }
                    CATCH_LOG();
                }

                return result == UpdateResult::Success;
            }

            bool Update(const SourceDetails& details, IProgressCallback& progress) override
            {
                return UpdateBase(details, false, progress);
            }

            bool BackgroundUpdate(const SourceDetails& details, IProgressCallback& progress) override
            {
                return UpdateBase(details, true, progress);
            }

            bool Remove(const SourceDetails& details, IProgressCallback& progress) override
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                auto store = CreateCompositeStore(details);

                auto lock = store->Lock(progress);
                if (!lock)
                {
                    return false;
                }

                // Every form that the details name packages for contributes, so that a delta
                // capable source does not leave its delta behind.
                std::vector<PackageKey> packages;
                std::set<std::string> seenIdentities;

                auto addPackagesFrom = [&](const std::unique_ptr<IIndexForm>& form)
                    {
                        if (!form->HasIdentities())
                        {
                            return;
                        }

                        for (auto& package : form->GetPackages())
                        {
                            if (seenIdentities.insert(package.Identity).second)
                            {
                                packages.emplace_back(std::move(package));
                            }
                        }
                    };

                addPackagesFrom(CreateFullIndexForm(details));
                addPackagesFrom(CreateDeltaIndexForm(details));

                store->Remove(packages, progress);

                return true;
            }

        private:
            // Records what a form discovered about the source's packages.
            static void WriteDiscoveredIdentities(SourceDetails& details, const std::string& data)
            {
                details.Data = data;

                // The identity of the source itself is derived from what it is made of rather
                // than chosen, so that it is the same every time it is computed. The local state
                // directory and the cross process lock are both derived from it, which is why it
                // must not depend on how much has been learned so far; see
                // SourceData::SourceIdentity.
                details.Identifier = SourceData{ data }.SourceIdentity();
            }

            // Drops everything a form left behind, for a form that the source is not going to use.
            //
            // Best effort: the source is still going to be added, and leaving a package behind is
            // not a reason to refuse it.
            static void DiscardPackagesOf(const SourceDetails& details, IPackageStore& store, IIndexForm& form, IProgressCallback& progress)
            {
                try
                {
                    auto lock = store.Lock(progress);
                    if (!lock)
                    {
                        return;
                    }

                    // The store is keyed on this form's own identity, so clearing it clears exactly
                    // what the attempt created and nothing the source goes on to use.
                    store.Remove(form.GetPackages(), progress);
                }
                catch (...)
                {
                    LOG_CAUGHT_EXCEPTION_MSG("Could not discard the packages of an unused form for source: %hs", details.Name.c_str());
                }
            }

            bool UpdateBase(const SourceDetails& details, bool isBackground, IProgressCallback& progress)

            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                auto store = CreateStore(details);

                if (IsDeltaIndexEnabled())
                {
                    UpdateReport report;
                    UpdateResult result = UpdateResult::Unusable;

                    auto form = CreateDeltaIndexForm(details);

                    // An update never probes for a delta: the details say whether the source
                    // has one. A source configured with only a full index identity names no
                    // delta and uses the full index, which is what it did before deltas existed.
                    if (form->HasIdentities())
                    {
                        result = form->Update(*store, isBackground, progress, report);

                        // While the feature is experimental, fail if things are not as they are expected
                        // When moving off of experimental, consider if we should support fallback.
                        // Fallback complicates the open side considerably.
                        if (IsDeltaIndexEnabled())
                        {
                            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, result == UpdateResult::Unusable);
                        }
                    }

                    if (result == UpdateResult::Success)
                    {
                        LogUpdate(details, report, isBackground);
                        return true;
                    }

                    if (result == UpdateResult::Aborted)
                    {
                        // The operation did not complete, so nothing was learned about whether the
                        // delta would have served. Falling back here would acquire a full index
                        // that the source may not have needed.
                        return false;
                    }

                    // Only a delta that cannot serve the source falls back; the delta is an
                    // optimization, never a correctness dependency.
                    AICLI_LOG(Repo, Info, << "Falling back to the full index for source: " << details.Name);
                }

                auto form = CreateFullIndexForm(details);

                UpdateReport report;
                UpdateResult result = form->Update(*store, isBackground, progress, report);

                LogUpdate(details, report, isBackground);

                return result == UpdateResult::Success;
            }

            // Whether an update is reported is the form's decision; see UpdateReport::Reportable.
            void LogUpdate(const SourceDetails& details, const UpdateReport& report, bool isBackground)
            {
                if (!report.Reportable || !report.NewIndexPublishedAt)
                {
                    return;
                }

                Logging::Telemetry().LogPreindexedPackageUpdate(
                    details.Identifier,
                    report.PreviousIndexPublishedAt,
                    report.NewIndexPublishedAt.value(),
                    report.UsedDeltaDownload,
                    report.PreviousBaselinePublishedAt,
                    report.NewBaselinePublishedAt,
                    report.BaselineUpdated,
                    report.DownloadedBytes.value_or(0),
                    !isBackground);
            }
        };
    }

    std::unique_ptr<ISourceFactory> PreIndexedPackageSourceFactory::Create()
    {
        return std::make_unique<PreIndexedFactory>();
    }
}
