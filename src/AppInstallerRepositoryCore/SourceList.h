// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySource.h"
#include <winget/Settings.h>


namespace AppInstaller::Repository
{
    // Built-in values for default sources
    std::string_view GetWellKnownSourceName(WellKnownSource source);
    std::string_view GetWellKnownSourceArg(WellKnownSource source);
    std::string_view GetWellKnownSourceIdentifier(WellKnownSource source);

    // SourceDetails with additional data used internally.
    struct SourceConfigurationInternal : public SourceConfiguration
    {
        SourceConfigurationInternal() = default;
        SourceConfigurationInternal(const SourceDetails& details) : SourceConfiguration(details) {}

        // Copies the metadata fields from this to target.
        void CopyMetadataFieldsTo(SourceConfigurationInternal& target);

        // If true, this is a tombstone, marking the deletion of a source at a lower priority origin.
        bool IsTombstone = false;
        std::string AcceptedAgreementsIdentifier;
        int AcceptedAgreementFields = 0;
    };

    // Gets the internal details for a well known source.
    SourceConfigurationInternal GetWellKnownSourceDetailsInternal(WellKnownSource source);

    // Struct containing internal implementation of the source list.
    // This contains all source data; including tombstoned sources.
    struct SourceList
    {
        SourceList();

        // Get a list of current sources references which can be used to update the contents in place.
        // e.g. update the LastTimeUpdated value of sources.
        std::vector<std::reference_wrapper<SourceConfigurationInternal>> GetCurrentSourceRefs();

        // Current source means source that's not in tombstone
        SourceConfigurationInternal* GetCurrentSource(std::string_view name);

        // Source includes ones in tombstone
        SourceConfigurationInternal* GetSource(std::string_view name);

        // Add/remove a current source
        void AddSource(const SourceConfigurationInternal& details);
        void RemoveSource(const SourceConfigurationInternal& details);

        // Save source metadata; the particular source with the metadata update is given.
        // The given source must already be in the internal source list.
        void SaveMetadata(const SourceConfigurationInternal& details);

        // Checks the source agreements and returns if agreements are satisfied.
        bool CheckSourceAgreements(const SourceDetails& details);

        // Save agreements information.
        void SaveAcceptedSourceAgreements(const SourceDetails& details);

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

        std::vector<SourceConfigurationInternal> GetSourcesByOrigin(SourceOrigin origin);
        // Does *NOT* set metadata; call SaveMetadataInternal afterward.
        [[nodiscard]] bool SetSourcesByOrigin(SourceOrigin origin, const std::vector<SourceConfigurationInternal>& sources);

        std::vector<SourceConfigurationInternal> GetMetadata();
        [[nodiscard]] bool SetMetadata(const std::vector<SourceConfigurationInternal>& sources);

        // Save source metadata; the particular source with the metadata update is given.
        // If remove is true, the given source is being removed.
        void SaveMetadataInternal(const SourceConfigurationInternal& details, bool remove = false);

        std::vector<SourceConfigurationInternal> m_sourceList;
        Settings::Stream m_userSourcesStream;
        Settings::Stream m_metadataStream;
    };
}
