// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_5/Interface.h"
#include "Microsoft/Schema/1_5/ArpVersionVirtualTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_5
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_4::Interface(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 1, 5 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_5");

        V1_4::Interface::CreateTables(connection, options);

        V1_0::ManifestTable::AddColumn(connection, { ArpMinVersionVirtualTable::ManifestColumnName(), SQLite::Builder::Type::RowId });
        V1_0::ManifestTable::AddColumn(connection, { ArpMaxVersionVirtualTable::ManifestColumnName(), SQLite::Builder::Type::RowId });

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_5");

        SQLite::rowid_t manifestId = V1_4::Interface::AddManifest(connection, manifest, relativePath);

        auto arpVersionRange = manifest.GetArpVersionRange();
        Manifest::string_t arpMinVersion, arpMaxVersion;

        if (!arpVersionRange.IsEmpty())
        {
            // Check to see if adding this version range will create a conflict
            SQLite::rowid_t packageIdentifier = V1_0::ManifestTable::GetIdById<V1_0::IdTable>(connection, manifestId).value();
            std::vector<Utility::VersionRange> ranges = GetArpVersionRanges(connection, packageIdentifier);
            ranges.push_back(arpVersionRange);

            if (Utility::HasOverlapInVersionRanges(ranges))
            {
                AICLI_LOG(Repo, Error, << "Overlapped Arp version ranges found for package. All ranges currently in index followed by new range:\n" << [&]() {
                        std::stringstream stream;
                        for (const auto& range : ranges)
                        {
                            stream << '[' << range.GetMinVersion().ToString() << "] - [" << range.GetMaxVersion().ToString() << "]\n";
                        }
                        return std::move(stream).str();
                    }());
                THROW_HR(APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED);
            }

            arpMinVersion = arpVersionRange.GetMinVersion().ToString();
            arpMaxVersion = arpVersionRange.GetMaxVersion().ToString();
        }

        SQLite::rowid_t arpMinVersionId = V1_0::VersionTable::EnsureExists(connection, arpMinVersion);
        SQLite::rowid_t arpMaxVersionId = V1_0::VersionTable::EnsureExists(connection, arpMaxVersion);
        V1_0::ManifestTable::UpdateValueIdById<ArpMinVersionVirtualTable>(connection, manifestId, arpMinVersionId);
        V1_0::ManifestTable::UpdateValueIdById<ArpMaxVersionVirtualTable>(connection, manifestId, arpMaxVersionId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_5");

        auto [indexModified, manifestId] = V1_4::Interface::UpdateManifest(connection, manifest, relativePath);

        auto [oldMinVersionId, oldMaxVersionId] =
            V1_0::ManifestTable::GetIdsById<ArpMinVersionVirtualTable, ArpMaxVersionVirtualTable>(connection, manifestId);

        auto arpVersionRange = manifest.GetArpVersionRange();
        Manifest::string_t arpMinVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMinVersion().ToString();
        Manifest::string_t arpMaxVersion = arpVersionRange.IsEmpty() ? "" : arpVersionRange.GetMaxVersion().ToString();

        SQLite::rowid_t arpMinVersionId = V1_0::VersionTable::EnsureExists(connection, arpMinVersion);
        SQLite::rowid_t arpMaxVersionId = V1_0::VersionTable::EnsureExists(connection, arpMaxVersion);

        // For cleaning up the old entries after update if applicable
        bool cleanOldMinVersionId = false;
        bool cleanOldMaxVersionId = false;

        if (arpMinVersionId != oldMinVersionId)
        {
            V1_0::ManifestTable::UpdateValueIdById<ArpMinVersionVirtualTable>(connection, manifestId, arpMinVersionId);
            cleanOldMinVersionId = true;
            indexModified = true;
        }

        if (arpMaxVersionId != oldMaxVersionId)
        {
            V1_0::ManifestTable::UpdateValueIdById<ArpMaxVersionVirtualTable>(connection, manifestId, arpMaxVersionId);
            cleanOldMaxVersionId = true;
            indexModified = true;
        }

        if (!arpVersionRange.IsEmpty())
        {
            // Check to see if the new set of version ranges created a conflict.
            // We could have done this before attempting the update but it would be more complex and SQLite gives us easy rollback.
            SQLite::rowid_t packageIdentifier = V1_0::ManifestTable::GetIdById<V1_0::IdTable>(connection, manifestId).value();
            std::vector<Utility::VersionRange> ranges = GetArpVersionRanges(connection, packageIdentifier);

            if (Utility::HasOverlapInVersionRanges(ranges))
            {
                AICLI_LOG(Repo, Error, << "Overlapped Arp version ranges found for package. Ranges that would be present with attempted upgrade:\n" << [&]() {
                        std::stringstream stream;
                        for (const auto& range : ranges)
                        {
                            stream << '[' << range.GetMinVersion().ToString() << "] - [" << range.GetMaxVersion().ToString() << "]\n";
                        }
                        return std::move(stream).str();
                    }());
                THROW_HR(APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED);
            }
        }

        if (cleanOldMinVersionId && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), oldMinVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, oldMinVersionId);
        }

        if (cleanOldMaxVersionId && oldMaxVersionId != oldMinVersionId && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), oldMaxVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, oldMaxVersionId);
        }

        savepoint.Commit();

        return { indexModified, manifestId };
     }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        // Get the old arp version ids of the values from the manifest table
        auto [arpMinVersionId, arpMaxVersionId] =
            V1_0::ManifestTable::GetIdsById<ArpMinVersionVirtualTable, ArpMaxVersionVirtualTable>(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "RemoveManifestById_v1_5");

        // Removes the manifest.
        V1_4::Interface::RemoveManifestById(connection, manifestId);

        // Remove the versions that are not needed.
        if (NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), arpMinVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, arpMinVersionId);
        }

        if (arpMaxVersionId != arpMinVersionId && NotNeeded(connection, V1_0::VersionTable::TableName(), V1_0::VersionTable::ValueName(), arpMaxVersionId))
        {
            V1_0::VersionTable::DeleteById(connection, arpMaxVersionId);
        }

        savepoint.Commit();
    }

    bool Interface::NotNeeded(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id) const
    {
        bool result = V1_4::Interface::NotNeeded(connection, tableName, valueName, id);

        if (result && tableName == V1_0::VersionTable::TableName())
        {
            if (valueName != V1_0::VersionTable::ValueName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, V1_0::VersionTable::ValueName(), id) && result;
            }
            if (valueName != ArpMinVersionVirtualTable::ManifestColumnName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, ArpMinVersionVirtualTable::ManifestColumnName(), id) && result;
            }
            if (valueName != ArpMaxVersionVirtualTable::ManifestColumnName())
            {
                result = !V1_0::ManifestTable::IsValueReferenced(connection, ArpMaxVersionVirtualTable::ManifestColumnName(), id) && result;
            }
        }

        return result;
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_4::Interface::CheckConsistency(connection, log);

        // If the v1.4 index was consistent, or if full logging of inconsistency was requested, check the v1.5 data.
        if (result || log)
        {
            result = V1_0::ManifestTable::CheckConsistency<ArpMinVersionVirtualTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = V1_0::ManifestTable::CheckConsistency<ArpMaxVersionVirtualTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ValidateArpVersionConsistency(connection, log) && result;
        }

        return result;
    }

    std::optional<std::string> Interface::GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const
    {
        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::ArpMinVersion:
            return V1_0::ManifestTable::GetValueById<ArpMinVersionVirtualTable>(connection, manifestId);
        case AppInstaller::Repository::PackageVersionProperty::ArpMaxVersion:
            return V1_0::ManifestTable::GetValueById<ArpMaxVersionVirtualTable>(connection, manifestId);
        default:
            return V1_4::Interface::GetPropertyByManifestIdInternal(connection, manifestId, property);
        }
    }

    std::vector<Utility::VersionRange> Interface::GetArpVersionRanges(const SQLite::Connection& connection, SQLite::rowid_t packageIdentifier) const
    {
        std::vector<Utility::VersionRange> ranges;
        auto versionKeys = GetVersionKeysById(connection, packageIdentifier);
        for (auto const& versionKey : versionKeys)
        {
            auto arpMinVersion = GetPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionProperty::ArpMinVersion).value_or("");
            auto arpMaxVersion = GetPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionProperty::ArpMaxVersion).value_or("");

            // Either both empty or both not empty
            THROW_HR_IF(E_UNEXPECTED, arpMinVersion.empty() != arpMaxVersion.empty());

            if (!arpMinVersion.empty() && !arpMaxVersion.empty())
            {
                ranges.emplace_back(Utility::VersionRange{ Utility::Version{ std::move(arpMinVersion) }, Utility::Version{ std::move(arpMaxVersion) } });
            }
        }
        return ranges;
    }

    bool Interface::ValidateArpVersionConsistency(const SQLite::Connection& connection, bool log) const
    {
        try
        {
            bool result = true;

            // Search everything
            SearchRequest request;
            auto searchResult = Search(connection, request);
            for (auto const& match : searchResult.Matches)
            {
                // Get arp version ranges for each package to check
                std::vector<Utility::VersionRange> ranges = GetArpVersionRanges(connection, match.first);

                // Check overlap
                if (Utility::HasOverlapInVersionRanges(ranges))
                {
                    AICLI_LOG(Repo, Error, << "Overlapped Arp version ranges found for package. PackageRowId: " << match.first);
                    result = false;

                    if (!log)
                    {
                        break;
                    }
                }
            }

            return result;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "ValidateArpVersionConsistency() encountered internal error. Returning false.");
            return false;
        }
    }
}
