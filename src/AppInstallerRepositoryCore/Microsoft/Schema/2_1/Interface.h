// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V2_1
{
    // The point in time from which the next delta generated against this index should be computed.
    static constexpr std::string_view s_MetadataValueName_DeltaBaselineTime = "deltaBaselineTime"sv;

    // The change sequence from which the next delta generated against this index should be computed.
    // This is the value a delta actually uses; the time above is retained for the 2.0 export and for
    // diagnostics. A sequence gives an exact, exclusive boundary that whole second times cannot.
    static constexpr std::string_view s_MetadataValueName_DeltaBaselineSequence = "deltaBaselineSequence"sv;

    // Identifies this index as a baseline that deltas may be generated against.
    //
    // The database identifier cannot serve this purpose. An index is prepared from a copy of a
    // long lived working index, and copying carries the identifier along, so every index produced
    // in a baseline period shares one. Designation therefore stamps its own fresh identity.
    static constexpr std::string_view s_MetadataValueName_BaselineIdentifier = "baselineIdentifier"sv;

    // Written into a delta, naming the baseline that it was generated against. A delta is only
    // meaningful when paired with that exact baseline, so this is checked when the two are opened.
    static constexpr std::string_view s_MetadataValueName_DeltaBaselineIdentifier = "deltaBaselineIdentifier"sv;

    // Interface to schema version 2.1 exposed through ISQLiteIndex.
    // Version 2.1 adds the is_removed column to the update_tracking table,
    // enabling delta index generation that can represent package removals.
    struct Interface : public V2_0::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;

        // Version 2.0
        bool MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current) override;

        // Version 2.1

        // Designates this index as a baseline that deltas may be generated against.
        void MarkAsBaseline(SQLite::Connection& connection) override;

        // Sets this index up to read the combination of a delta and the baseline it was generated
        // against. Attaches the baseline and defines the merged views, after which every inherited
        // read path operates on the combination. Must be called before any read.
        void SetupDeltaReadMode(SQLite::Connection& connection, const SQLite::DatabaseSpecifier& baseline) override;

    protected:
        // Records the baseline time for this index, and generates a delta index against a previous
        // baseline when the caller has supplied the paths to do so.
        void CreateAdditionalPackagingOutput(const SQLiteIndexContext& context) override;
    };
}
