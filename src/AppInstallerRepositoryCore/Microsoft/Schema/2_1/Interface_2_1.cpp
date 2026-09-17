// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"
#include "Microsoft/Schema/2_1/DeltaGeneration.h"
#include "Microsoft/Schema/2_1/DeltaViews.h"

#include <winget/SQLiteMetadataTable.h>
#include <AppInstallerDateTime.h>

namespace AppInstaller::Repository::Microsoft::Schema::V2_1
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V2_0::Interface(normVersion)
    {
        // Removals are recorded rather than deleted, so that delta generation can see which
        // packages have gone away.
        m_trackingRemovalBehavior = V2_0::PackageUpdateTrackingTable::RemovalBehavior::Record;
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 2, 1 };
    }

    bool Interface::MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current)
    {
        THROW_HR_IF_NULL(E_POINTER, current);

        // The 2.0 migration will go to 2.1 due to the removal behavior
        if (V2_0::Interface::MigrateFrom(connection, current))
        {
            return true;
        }

        // Migration from 2.0 → 2.1
        auto currentVersion = current->GetVersion();
        if (currentVersion.MajorVersion == 2 && currentVersion.MinorVersion == 0)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "migrate_from_v2_1");
            V2_0::PackageUpdateTrackingTable::AddRemovalTrackingColumns(connection);
            savepoint.Commit();
            return true;
        }

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

        // The baseline must be a previous version of this database.
        std::string databaseIdentifier = SQLite::MetadataTable::GetNamedValue<std::string>(connection, SQLite::s_MetadataValueName_DatabaseIdentifier);
        std::string baselineDatabaseIdentifier = SQLite::MetadataTable::GetNamedValue<std::string>(baselineConnection, SQLite::s_MetadataValueName_DatabaseIdentifier);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED, databaseIdentifier != baselineDatabaseIdentifier);

        // Ensure that the baseline is earlier in the sequence.
        int64_t baselineSequence = 0;
        std::optional<std::string> baselineSequenceString = SQLite::MetadataTable::TryGetNamedValue<std::string>(baselineConnection, s_MetadataValueName_DeltaBaselineSequence);
        if (baselineSequenceString && !baselineSequenceString->empty())
        {
            baselineSequence = std::stoll(baselineSequenceString.value());
        }

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
