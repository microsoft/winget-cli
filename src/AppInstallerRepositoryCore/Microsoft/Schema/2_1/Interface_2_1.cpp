// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"
#include "Microsoft/Schema/2_1/DeltaGeneration.h"
#include "Microsoft/Schema/2_1/DeltaViews.h"

#include <winget/SQLiteMetadataTable.h>
#include <AppInstallerDateTime.h>

#include <sstream>

namespace AppInstaller::Repository::Microsoft::Schema::V2_1
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V2_0::Interface(normVersion)
    {
        // Removals are recorded rather than deleted, so that delta generation can see which
        // packages have gone away. This is the difference that 2.1 exists for.
        m_trackingRemovalBehavior = V2_0::PackageUpdateTrackingTable::RemovalBehavior::Record;
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 2, 1 };
    }

    bool Interface::MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current)
    {
        THROW_HR_IF_NULL(E_POINTER, current);

        auto currentVersion = current->GetVersion();

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "migrate_from_v2_1");

        // Attempt a migration to 2.0 first, which will only return true if it actually performed a migration
        bool v2result = V2_0::Interface::MigrateFrom(connection, current);

        // Migration from 2.0 → 2.1: add the is_removed column to update_tracking.
        if (v2result || (currentVersion.MajorVersion == 2 && currentVersion.MinorVersion == 0))
        {
            V2_0::PackageUpdateTrackingTable::AddRemovalTrackingColumns(connection);
            savepoint.Commit();
            return true;
        }

        savepoint.Rollback(true);
        return false;
    }

    void Interface::MarkAsBaseline(SQLite::Connection& connection)
    {
        GUID baselineIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&baselineIdentifier));

        std::ostringstream stream;
        stream << baselineIdentifier;
        std::string value = stream.str();

        AICLI_LOG(Repo, Info, << "Marking index as a delta baseline with identifier [" << value << "]");

        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_BaselineIdentifier, value);
    }

    void Interface::SetupDeltaReadMode(SQLite::Connection& connection, const SQLite::DatabaseSpecifier& baseline)
    {
        Delta::SetupReadMode(connection, baseline);

        // The merged data is presented through views rather than tables, so the checks that the
        // base makes to decide whether this index has been packaged cannot see it. Record that the
        // question is already settled: a delta is only ever read, and only in its packaged form.
        m_isDeltaReadMode = true;
        m_internalInterfaceChecked = true;
    }

    void Interface::CreateAdditionalPackagingOutput(const SQLiteIndexContext& context)
    {
        SQLite::Connection& connection = context.Connection;

        // Record the point from which a delta against this index should be computed. Every 2.1 index
        // does this, because any of them may later be designated as a baseline.
        // TODO: We also need to ensure that our times are UTC / not impacted by timezone shifts, etc.
        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_DeltaBaselineTime, std::to_string(Utility::GetCurrentUnixEpoch()));

        // The sequence is what a delta actually uses; the time is retained because it is what the
        // 2.0 version data manifest export reads, and because it remains useful diagnostically.
        // A sequence is preferred here because the boundary it defines is exact. Whole second times
        // cannot separate a change written during the baseline's own second from one written before
        // it, so the time based window has to be inclusive and re-carries everything written in that
        // second. A sequence is also immune to the clock stepping backwards, which under the time
        // scheme silently drops a change and leaves a stale baseline row visible forever.
        //
        // It has to be recorded rather than recomputed from the baseline later: preparing an index
        // drops the tracking table, so a baseline has none to read. That is also why this runs where
        // it does, before the drop. Even had the table survived, its maximum is taken over whatever
        // rows remain and would fall below the true high water mark once any were removed, so a
        // later delta would re-carry changes the baseline already contains.
        int64_t currentSequence = V2_0::PackageUpdateTrackingTable::GetCurrentChangeSequence(connection, m_trackingRemovalBehavior);
        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_DeltaBaselineSequence, std::to_string(currentSequence));

        if (!context.Data.Contains(Property::DeltaBaselineIndexPath) ||
            !context.Data.Contains(Property::DeltaOutputPath))
        {
            return;
        }

        std::filesystem::path baselinePath = context.Data.Get<Property::DeltaBaselineIndexPath>();
        std::filesystem::path deltaOutputPath = context.Data.Get<Property::DeltaOutputPath>();

        AICLI_LOG(Repo, Info, << "Generating a delta index against baseline [" << baselinePath << "]");

        SQLite::Connection baselineConnection = SQLite::Connection::Create(baselinePath.u8string(), SQLite::Connection::OpenDisposition::ReadOnly);

        // The changes to capture are those written after the baseline recorded its own sequence.
        int64_t baselineSequence = 0;
        std::optional<std::string> baselineSequenceString = SQLite::MetadataTable::TryGetNamedValue<std::string>(baselineConnection, s_MetadataValueName_DeltaBaselineSequence);
        if (baselineSequenceString && !baselineSequenceString->empty())
        {
            baselineSequence = std::stoll(baselineSequenceString.value());
        }

        // A sequence fails in the one direction a time does not: if this index was rebuilt since the
        // baseline was taken, its counter restarted below the baseline's value and the window is
        // empty. That would produce a silently empty delta, so refuse instead. The equal case is
        // legitimate and simply means nothing has changed.
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED, currentSequence < baselineSequence);

        auto changedPackages = V2_0::PackageUpdateTrackingTable::GetUpdatesSinceSequence(connection, baselineSequence, m_trackingRemovalBehavior);
        auto removedPackages = V2_0::PackageUpdateTrackingTable::GetRemovalsSinceSequence(connection, baselineSequence, m_trackingRemovalBehavior);

        Delta::Generate(
            connection,
            baselineConnection,
            deltaOutputPath,
            GetVersion(),
            changedPackages,
            removedPackages);
    }
}
