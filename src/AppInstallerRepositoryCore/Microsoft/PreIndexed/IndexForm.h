// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/PreIndexed/PackageStore.h"
#include "Microsoft/SQLiteIndex.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    // What an update did, in the terms that the telemetry event is defined in.
    struct UpdateReport
    {
        std::optional<std::chrono::system_clock::time_point> PreviousIndexPublishedAt;
        std::optional<std::chrono::system_clock::time_point> NewIndexPublishedAt;
        bool UsedDeltaDownload = false;
        std::optional<std::chrono::system_clock::time_point> PreviousBaselinePublishedAt;
        std::optional<std::chrono::system_clock::time_point> NewBaselinePublishedAt;
        bool BaselineUpdated = false;

        // Nothing when no bytes moved, which is how the add path decides whether to report at all.
        std::optional<uint64_t> DownloadedBytes;

        // Whether this update is one that should be reported.
        bool Reportable = false;

        // What the update learned about the source's packages, when that is more than the details
        // it was given already said.
        //
        // A delta names the location of its baseline, so the baseline's identity cannot be known
        // until a delta has been acquired and read. An update is therefore the first operation
        // that can learn it, and the value to store back into SourceDetails::Data is reported
        // here rather than discovered a second time. Only the add path can act on it; every other
        // caller is given a const SourceDetails and has nowhere to put it.
        std::optional<std::string> LearnedIdentities;
    };


    // What an attempt to update a form concluded.
    enum class UpdateResult
    {
        // The store now holds this form, up to date with the source.
        Success,

        // The operation did not complete: it was cancelled, or the lock could not be taken.
        // Nothing can be concluded about whether this form would have worked, so the caller
        // reports failure rather than trying another form.
        Aborted,

        // This form cannot serve the source. The caller should fall back to another.
        Unusable,
    };

    // How a source's index is composed, and therefore which packages it has and how they relate.
    //
    // A form knows nothing about where its packages are kept. It is given a store to act on, and
    // names the packages it wants by identity and slot.
    struct IIndexForm
    {
        virtual ~IIndexForm() = default;

        // Determines the identities of this form's packages, probing the source where the details
        // do not already record them.
        //
        // Probing is confined to the add path: it is the only operation that can persist what it
        // learns. A form whose details are already populated reports what they say, so a source
        // configured before this form existed is not silently migrated onto it.
        //
        // What is discovered here has to be enough to identify the source and to begin acquiring
        // its packages, but need not be all of them: see UpdateReport::LearnedIdentities.
        //
        // Returns the value that should be written to SourceDetails::Data, or nothing when the
        // source does not publish the packages that this form needs.
        virtual std::optional<std::string> DiscoverIdentities(IProgressCallback& progress) = 0;

        // The packages that make up this form, as far as their identities are known.
        // Throws when nothing is known at all; see DiscoverIdentities.
        virtual std::vector<PackageKey> GetPackages() const = 0;


        // Whether the identities of this form's packages are already known, so that GetPackages
        // can be called and no probe is needed.
        virtual bool HasIdentities() const = 0;

        // Whether the store holds every package that this form needs.
        virtual bool IsHeld(const IPackageStore& store) const = 0;

        // Whether the packages that the store holds for this form can be opened together.
        //
        // IsHeld only asks whether each package is present. A form whose packages have to agree
        // with one another can hold a set that does not, and that cannot be answered without
        // reading them, so it is a separate and more expensive question.
        virtual bool IsUsable(IPackageStore& store, IProgressCallback& progress) = 0;

        // The published version of the held index, which is what staleness is judged from.
        virtual std::optional<Msix::PackageVersion> GetHeldVersion(const IPackageStore& store) const = 0;

        // Brings the store up to date with the source.
        //
        // A failure to update is reported by throwing; the result distinguishes the outcomes that
        // are not failures. See UpdateResult.
        virtual UpdateResult Update(IPackageStore& store, bool isBackground, IProgressCallback& progress, UpdateReport& report) = 0;

        // Opens the index that the store holds for this form.
        virtual SQLiteIndex Open(IPackageStore& store, IProgressCallback& progress) = 0;
    };

    // The complete index, as acquired by a client that is not using a delta.
    std::unique_ptr<IIndexForm> CreateFullIndexForm(const SourceDetails& details);

    // A delta paired with the baseline that it names.
    std::unique_ptr<IIndexForm> CreateDeltaIndexForm(const SourceDetails& details);
}
