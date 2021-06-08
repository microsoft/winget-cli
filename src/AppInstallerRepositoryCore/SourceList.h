// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerRepositorySource.h"
#include <winget/Settings.h>


namespace AppInstaller::Repository
{
    // Built-in values for default sources
    std::string_view WingetCommunityDefaultSourceName();
    std::string_view WingetCommunityDefaultSourceArg();
    std::string_view WingetCommunityDefaultSourceId();

    std::string_view WingetMSStoreDefaultSourceName();
    std::string_view WingetMSStoreDefaultSourceArg();
    std::string_view WingetMSStoreDefaultSourceId();

    // SourceDetails with additional data used internally.
    struct SourceDetailsInternal : public SourceDetails
    {
        // If true, this is a tombstone, marking the deletion of a source at a lower priority origin.
        bool IsTombstone = false;
    };

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
        void AddSource(const SourceDetailsInternal& source);
        void RemoveSource(const SourceDetailsInternal& source);

        // Save source metadata. Currently only LastTimeUpdated is used.
        void SaveMetadata() const;

    private:
        // calls std::find_if and return the iterator.
        auto FindSource(std::string_view name, bool includeTombstone = false);

        std::vector<SourceDetailsInternal> m_sourceList;
        Settings::Stream m_userSourcesStream;
        Settings::Stream m_metadataStream;
    };
}
