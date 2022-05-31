// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "ArpVersionValidation.h"
#include <winget/ManifestValidation.h>

namespace AppInstaller::Repository
{
    namespace
    {
        std::vector<Utility::VersionRange> GetArpVersionRangesByPackageRowId(Microsoft::SQLiteIndex* index, Microsoft::SQLiteIndex::IdType packageRowId)
        {
            std::vector<Utility::VersionRange> result;

            auto versionKeys = index->GetVersionKeysById(packageRowId);
            for (auto const& versionKey : versionKeys)
            {
                std::optional<Microsoft::SQLiteIndex::IdType> manifestRowId = index->GetManifestIdByKey(packageRowId, versionKey.GetVersion().ToString(), versionKey.GetChannel().ToString());
                if (manifestRowId)
                {
                    auto arpMinVersion = index->GetPropertyByManifestId(manifestRowId.value(), PackageVersionProperty::ArpMinVersion).value_or("");
                    auto arpMaxVersion = index->GetPropertyByManifestId(manifestRowId.value(), PackageVersionProperty::ArpMaxVersion).value_or("");

                    // Either both empty or both not empty
                    THROW_HR_IF(E_UNEXPECTED, arpMinVersion.empty() != arpMaxVersion.empty());

                    if (!arpMinVersion.empty() && !arpMaxVersion.empty())
                    {
                        result.emplace_back(Utility::VersionRange{ Utility::Version{ arpMinVersion }, Utility::Version{ arpMaxVersion } });
                    }
                }
            }

            return result;
        }
    }

    void ValidateManifestArpVersion(Microsoft::SQLiteIndex* index, const Manifest::Manifest& manifest)
    {
        try
        {
            auto manifestArpVersionRange = manifest.GetArpVersionRange();
            if (manifestArpVersionRange.IsEmpty())
            {
                return;
            }

            SearchRequest request;
            request.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, manifest.Id);
            auto searchResult = index->Search(request);
            if (searchResult.Matches.empty())
            {
                return;
            }

            auto arpVersionRangesInIndex = GetArpVersionRangesByPackageRowId(index, searchResult.Matches[0].first);
            for (auto const& arpInIndex : arpVersionRangesInIndex)
            {
                if (manifestArpVersionRange.HasOverlapWith(arpInIndex))
                {
                    std::string errorMsg = Manifest::ManifestError::ArpVersionOverlapWithIndex;
                    errorMsg.append("[" + arpInIndex.GetMinVersion().ToString() + ", " + arpInIndex.GetMaxVersion().ToString() + "]");
                    AICLI_LOG(Repo, Error, << errorMsg);
                    THROW_EXCEPTION(Manifest::ManifestException(
                        { Manifest::ValidationError(errorMsg) },
                        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
                }
            }
        }
        catch (const Manifest::ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "ValidateManifestArpVersion() encountered internal error.");
            THROW_EXCEPTION(Manifest::ManifestException(
                { Manifest::ValidationError(Manifest::ManifestError::ArpVersionValidationInternalError) },
                APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
        }
    }

    bool ValidateArpVersionConsistency(Microsoft::SQLiteIndex* index)
    {
        try
        {
            // Search everything
            SearchRequest request;
            auto searchResult = index->Search(request);
            for (auto const& match : searchResult.Matches)
            {
                auto arpVersionRangesInIndex = GetArpVersionRangesByPackageRowId(index, match.first);
                if (Utility::HasOverlapInVersionRanges(arpVersionRangesInIndex))
                {
                    AICLI_LOG(Repo, Error, << "Overlapped Arp version ranges found for package. PackageRowId: " << match.first);
                    return false;
                }
            }

            return true;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "ValidateArpVersionConsistency() encountered internal error. Returning false.");
            return false;
        }
    }
}