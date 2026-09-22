// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"
#include "Microsoft/Schema/2_1/DeltaConsistency.h"
#include "Microsoft/Schema/2_1/DeltaGeneration.h"
#include "Microsoft/Schema/2_1/DeltaViews.h"

#include <winget/SQLiteMetadataTable.h>
#include <AppInstallerDateTime.h>

namespace AppInstaller::Repository::Microsoft::Schema::V2_1
{
    namespace
    {
        // Gives the index an identity that a delta can name, so that the two can only ever be
        // paired with each other. This runs during preparation rather than as a separate step,
        // because designating a baseline and generating the empty delta that describes it are one
        // decision: a baseline that nothing was ever written against is of no use to a client.
        void MarkAsBaseline(SQLite::Connection& connection)
        {
            // A delta is a description of change rather than a whole index, so it cannot stand as
            // the baseline for another one. What identifies one is the baseline it names, which it
            // carries whether it is being read on its own or with that baseline attached.
            THROW_HR_IF(E_NOT_VALID_STATE,
                !SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_DeltaBaselineIdentifier).value_or(std::string{}).empty());

            GUID baselineIdentifier;
            THROW_IF_FAILED(CoCreateGuid(&baselineIdentifier));

            std::ostringstream stream;
            stream << baselineIdentifier;
            std::string value = stream.str();

            AICLI_LOG(Repo, Info, << "Marking index as a delta baseline with identifier [" << value << "]");

            SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_BaselineIdentifier, value);
        }
    }

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
            V2_0::PackageUpdateTrackingTable::MigrateToRemovalTracking(connection);
            savepoint.Commit();
            return true;
        }

        return false;
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

    bool Interface::CheckConsistency(const SQLiteIndexConstContext& context, bool log) const
    {
        bool hasBaseline = context.Data.Contains(Property::DeltaBaselineIndexPath);
        bool hasComparison = context.Data.Contains(Property::DeltaComparisonIndexPath);

        // The properties cannot decide what this database is: the working index that *generates* a
        // delta carries the very same baseline path, and it is an ordinary index.
        bool isDelta = IsDeltaIndex(context.Connection);

        // Comparing against a standard index only says something about a merged result.
        THROW_HR_IF(E_INVALIDARG, hasComparison && !isDelta);

        const SQLite::Connection* targetConnection = &context.Connection;
        std::optional<SQLite::Connection> mergedConnection;
        const ISQLiteIndex* targetInterface = this;
        std::unique_ptr<ISQLiteIndex> combinedInterface;

        bool result = true;

        if (isDelta)
        {
            result = Delta::CheckConsistency(context.Connection, log) && result;

            if (!m_isDeltaReadMode)
            {
                if (!hasBaseline)
                {
                    // There is nothing to compare a delta against until it has been merged.
                    THROW_HR_IF(E_INVALIDARG, hasComparison);

                    // We can only check the consistency of the delta itself without a baseline.
                    return result;
                }

                if (result || log)
                {
                    // The combination is opened rather than attached to the caller's connection, which is
                    // const and would be permanently changed by the attach.
                    THROW_HR_IF(E_NOT_VALID_STATE, !context.Data.Contains(Property::DatabaseFilePath));

                    mergedConnection = SQLite::Connection::Create(SQLite::DatabaseSpecifier{
                        context.Data.Get<Property::DatabaseFilePath>().u8string(), SQLite::DatabaseDisposition::Read });

                    combinedInterface = CreateISQLiteIndex(GetVersion());
                    combinedInterface->SetupDeltaReadMode(mergedConnection.value(), SQLite::DatabaseSpecifier{
                        context.Data.Get<Property::DeltaBaselineIndexPath>().u8string(), SQLite::DatabaseDisposition::Read });

                    targetConnection = &mergedConnection.value();
                    targetInterface = combinedInterface.get();
                }
            }
        }

        // Perform the standard consistency check against the merged interface
        if (result || log)
        {
            result = targetInterface->CheckConsistency(*targetConnection, log) && result;
        }

        if (hasComparison && (result || log))
        {
            SQLite::Connection comparison = SQLite::Connection::Create(SQLite::DatabaseSpecifier{
                context.Data.Get<Property::DeltaComparisonIndexPath>().u8string(), SQLite::DatabaseDisposition::Read });

            // The standard index need not be this exact version, so its own interface reads it.
            std::unique_ptr<ISQLiteIndex> comparisonInterface = CreateISQLiteIndex(SQLite::Version::GetSchemaVersion(comparison));

            result = Delta::CheckEquivalence(*targetInterface, *targetConnection, *comparisonInterface, comparison, log) && result;
        }

        return result;
    }

    bool Interface::IsDeltaIndex(const SQLite::Connection& connection) const
    {
        return !SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_DeltaBaselineIdentifier).value_or(std::string{}).empty();
    }

    void Interface::CreateAdditionalPackagingOutput(const SQLiteIndexContext& context)
    {
        SQLite::Connection& connection = context.Connection;

        int64_t currentSequence = V2_0::PackageUpdateTrackingTable::GetCurrentChangeSequence(connection, m_trackingRemovalBehavior);
        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_DeltaBaselineSequence, std::to_string(currentSequence));

        bool hasBaselineIndexPath = context.Data.Contains(Property::DeltaBaselineIndexPath);
        bool markAsBaseline = context.Data.Contains(Property::DeltaMarkAsBaseline);
        bool hasOutputPath = context.Data.Contains(Property::DeltaOutputPath);
        bool hasRelativeSourcePath = context.Data.Contains(Property::DeltaBaselineRelativeSourcePath);
        bool hasPackageVersion = context.Data.Contains(Property::DeltaBaselinePackageVersion);

        if (!hasBaselineIndexPath && !markAsBaseline && !hasOutputPath && !hasRelativeSourcePath && !hasPackageVersion)
        {
            return;
        }

        // Exactly one of the two says what the delta is computed against: an existing baseline, or
        // this index, which is being designated as one. Supplying both is a contradiction and
        // supplying neither leaves the delta with nothing to describe.
        THROW_HR_IF(E_INVALIDARG, hasBaselineIndexPath == markAsBaseline);

        // Beyond that, generation is all or nothing. A partially configured caller has made a
        // mistake, and silently declining would only surface later as a delta that no client can
        // pair with a baseline.
        THROW_HR_IF(E_INVALIDARG, !(hasOutputPath && hasRelativeSourcePath && hasPackageVersion));

        std::filesystem::path deltaOutputPath = context.Data.Get<Property::DeltaOutputPath>();

        Delta::BaselineReference baselineReference
        {
            context.Data.Get<Property::DeltaBaselineRelativeSourcePath>(),
            context.Data.Get<Property::DeltaBaselinePackageVersion>(),
        };

        if (markAsBaseline)
        {
            MarkAsBaseline(connection);

            // This index is its own baseline, so nothing has changed since it and the delta that describes it is empty.
            Delta::Generate(connection, connection, baselineReference, deltaOutputPath, GetVersion(), {}, {});
            return;
        }

        std::filesystem::path baselinePath = context.Data.Get<Property::DeltaBaselineIndexPath>();

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
            baselineReference,
            deltaOutputPath,
            GetVersion(),
            changedPackages,
            removedPackages);
    }
}
