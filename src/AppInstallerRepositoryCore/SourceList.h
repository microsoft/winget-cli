// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ISource.h"
#include <winget/Settings.h>


namespace AppInstaller::Repository
{
    // Built-in values for default sources
    std::string_view GetWellKnownSourceName(WellKnownSource source);
    std::string_view GetWellKnownSourceArg(WellKnownSource source);
    std::string_view GetWellKnownSourceIdentifier(WellKnownSource source);
    std::optional<WellKnownSource> CheckForWellKnownSourceMatch(std::string_view name, std::string_view arg, std::string_view type);

    // SourceDetails with additional data used internally.
    struct SourceDetailsInternal : public SourceDetails
    {
        SourceDetailsInternal() = default;
        SourceDetailsInternal(const SourceDetails& details) : SourceDetails(details) {}

        // Copies the metadata fields to this target.
        void CopyMetadataFieldsTo(SourceDetailsInternal& target);

        // Copies the metadata fields from this source. This only include partial metadata.
        void CopyMetadataFieldsFrom(const SourceDetails& source);

        // If true, this is a tombstone, marking the deletion of a source at a lower priority origin.
        bool IsTombstone = false;

        // If false, this is not visible in GetCurrentSource or GetAllSources, it's only available when explicitly requested.
        bool IsVisible = true;

        // Accepted agreements info.
        std::string AcceptedAgreementsIdentifier;
        int AcceptedAgreementFields = 0;
    };

    // Gets the internal details for a well known source.
    SourceDetailsInternal GetWellKnownSourceDetailsInternal(WellKnownSource source);

    // Struct containing internal implementation of the source list.
    // This contains all source data; including tombstoned sources.
    struct SourceList
    {
        SourceList();

        // Get a list of current sources references which can be used to update the contents in place.
        // e.g. update the LastTimeUpdated value of sources.
        std::vector<std::reference_wrapper<SourceDetailsInternal>> GetCurrentSourceRefs();

        // Current source means source that's not in tombstone
        SourceDetailsInternal* GetCurrentSource(std::string_view name);

        // Source includes ones in tombstone
        SourceDetailsInternal* GetSource(std::string_view name);

        // Add/remove a current source
        void AddSource(const SourceDetailsInternal& details);
        void RemoveSource(const SourceDetailsInternal& details);

        // Save source metadata; the particular source with the metadata update is given.
        // The given source must already be in the internal source list.
        void SaveMetadata(const SourceDetailsInternal& details);

        // Checks the source agreements and returns if agreements are satisfied.
        bool CheckSourceAgreements(std::string_view sourceName, std::string_view agreementsIdentifier, ImplicitAgreementFieldEnum agreementFields);

        // Save agreements information.
        void SaveAcceptedSourceAgreements(std::string_view sourceName, std::string_view agreementsIdentifier, ImplicitAgreementFieldEnum agreementFields);

        // Removes all settings streams associated with the source list.
        // Implements `winget source reset --force`.
        static void RemoveSettingsStreams();

    private:
        // Overwrites the source list with all sources.
        void OverwriteSourceList();

        // Overwrites the source list with the current metadata.
        void OverwriteMetadata();

        // calls std::find_if and return the iterator.
        auto FindSource(std::string_view name, bool includeHidden = false);

        std::vector<SourceDetailsInternal> GetSourcesByOrigin(SourceOrigin origin);
        // Does *NOT* set metadata; call SaveMetadataInternal afterward.
        [[nodiscard]] bool SetSourcesByOrigin(SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources);

        std::vector<SourceDetailsInternal> GetMetadata();
        [[nodiscard]] bool SetMetadata(const std::vector<SourceDetailsInternal>& sources);

        // Save source metadata; the particular source with the metadata update is given.
        // If remove is true, the given source is being removed.
        void SaveMetadataInternal(const SourceDetailsInternal& details, bool remove = false);

        std::vector<SourceDetailsInternal> m_sourceList;
        Settings::Stream m_userSourcesStream;
        Settings::Stream m_metadataStream;
    };
}
