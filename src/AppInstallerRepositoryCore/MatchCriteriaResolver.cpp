// Copyright (c) Microsoft Corporation.
#include "pch.h"
#include "MatchCriteriaResolver.h"
#include <AppInstallerStrings.h>

namespace AppInstaller::Repository
{
    namespace
    {
        using ValueMatchFunction = bool (*)(const Utility::NormalizedString&, const Utility::NormalizedString&);

        bool ValueMatchFunction_AlwaysFalse(const Utility::NormalizedString&, const Utility::NormalizedString&)
        {
            return false;
        }

        bool ValueMatchFunction_Exact(const Utility::NormalizedString& a, const Utility::NormalizedString& b)
        {
            return a == b;
        }

        bool ValueMatchFunction_CaseInsensitive(const Utility::NormalizedString& a, const Utility::NormalizedString& b)
        {
            return Utility::ICUCaseInsensitiveEquals(a, b);
        }

        bool ValueMatchFunction_StartsWith(const Utility::NormalizedString& a, const Utility::NormalizedString& b)
        {
            return Utility::ICUCaseInsensitiveStartsWith(a, b);
        }

        bool ValueMatchFunction_Substring(const Utility::NormalizedString& a, const Utility::NormalizedString& b)
        {
            return Utility::ContainsSubstring(FoldCase(a), FoldCase(b));
        }

        ValueMatchFunction GetMatchTypeFunction(MatchType matchType)
        {
            switch (matchType)
            {
            case MatchType::Exact:
                return ValueMatchFunction_Exact;
            case MatchType::CaseInsensitive:
                return ValueMatchFunction_CaseInsensitive;
            case MatchType::StartsWith:
                return ValueMatchFunction_StartsWith;
            case MatchType::Substring:
                return ValueMatchFunction_Substring;
            case MatchType::Fuzzy:
            case MatchType::FuzzySubstring:
            case MatchType::Wildcard:
            default:
                return ValueMatchFunction_AlwaysFalse;
            }
        }

        // Checks that the match query provided is consistent with the values given.
        bool CheckMatchValue(const RequestMatch& query, const Utility::NormalizedString& value, const std::optional<Utility::NormalizedString>& additional)
        {
            auto matchFunction = GetMatchTypeFunction(query.Type);

            if (matchFunction(value, query.Value))
            {
                if (query.Additional)
                {
                    if (additional)
                    {
                        return matchFunction(additional.value(), query.Additional.value());
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return true;
                }
            }
            else
            {
                return false;
            }
        }

        // Ensures that the query matches the criteria.
        bool CheckQuery(const std::optional<RequestMatch>& query, const PackageMatchFilter& criteria)
        {
            if (query && !query->Value.empty())
            {
                return CheckMatchValue(query.value(), criteria.Value, criteria.Additional);
            }

            return true;
        }

        // Ensures that the criteria matches one of the inclusions if provided.
        bool CheckInclusions(const std::vector<PackageMatchFilter>& inclusions, const PackageMatchFilter& criteria)
        {
            if (inclusions.empty())
            {
                return true;
            }

            for (const auto& inclusion : inclusions)
            {
                if (inclusion.Field == criteria.Field &&
                    CheckMatchValue(inclusion, criteria.Value, criteria.Additional))
                {
                    return true;
                }
            }

            return false;
        }

        // Ensures that the criteria doesn't match one of the filters if provided.
        bool CheckFilters(const std::vector<PackageMatchFilter>& filters, const PackageMatchFilter& criteria)
        {
            if (filters.empty())
            {
                return true;
            }

            for (const auto& filter : filters)
            {
                if (filter.Field == criteria.Field &&
                    CheckMatchValue(filter, criteria.Value, criteria.Additional))
                {
                    return false;
                }
            }

            return true;
        }

        // ----------------------- NEW

        PackageVersionProperty GetPackageVersionPropertyFor(PackageMatchField field)
        {
            switch (field)
            {
            case PackageMatchField::Id:
                return PackageVersionProperty::Id;
            case PackageMatchField::Name:
                return PackageVersionProperty::Name;
            case PackageMatchField::Moniker:
                return PackageVersionProperty::Moniker;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        std::optional<MatchType> GetBestMatchType(const SearchRequest& request, PackageMatchField field, const Utility::NormalizedString& value)
        {

        }
    }

    PackageMatchFilter FindBestMatchCriteria(const SearchRequest& request, const IPackageVersion* packageVersion)
    {
        PackageMatchFilter result{ PackageMatchField::Unknown, MatchType::Wildcard };

        // Single value fields
        for (auto field : { PackageMatchField::Id, PackageMatchField::Name, PackageMatchField::Moniker })
        {
            auto propertyValue = packageVersion->GetProperty(GetPackageVersionPropertyFor(field));
            if (propertyValue.empty())
            {
                continue;
            }
        }

        // Multi-value fields
        for (auto field : { PackageMatchField::Command, PackageMatchField::Tag, PackageMatchField::PackageFamilyName,
            PackageMatchField::ProductCode, PackageMatchField::UpgradeCode })
        {

        }
    }
}
