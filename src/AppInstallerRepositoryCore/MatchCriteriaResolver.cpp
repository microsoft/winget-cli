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

        PackageVersionMultiProperty GetPackageVersionMultiPropertyFor(PackageMatchField field)
        {
            switch (field)
            {
            case PackageMatchField::Command:
                return PackageVersionMultiProperty::Command;
            case PackageMatchField::Tag:
                return PackageVersionMultiProperty::Tag;
            case PackageMatchField::PackageFamilyName:
                return PackageVersionMultiProperty::PackageFamilyName;
            case PackageMatchField::ProductCode:
                return PackageVersionMultiProperty::ProductCode;
            case PackageMatchField::UpgradeCode:
                return PackageVersionMultiProperty::UpgradeCode;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        // Gets the best match type for the given field value and required minimum match type.
        std::optional<MatchType> GetBestMatchType(const RequestMatch& request, MatchType mustBeBetterThanMatchType, const Utility::NormalizedString& value)
        {
            if (request.Value.empty())
            {
                return std::nullopt;
            }

            for (auto matchType : { MatchType::Exact, MatchType::CaseInsensitive, MatchType::StartsWith, MatchType::Substring })
            {
                if (matchType >= mustBeBetterThanMatchType)
                {
                    break;
                }

                auto matchFunction = GetMatchTypeFunction(matchType);

                if (matchFunction(value, request.Value))
                {
                    return matchType;
                }
            }

            return std::nullopt;
        }

        // Gets the best match type for the given field value and required minimum match type.
        std::optional<MatchType> GetBestMatchType(const SearchRequest& request, PackageMatchField field, MatchType mustBeBetterThanMatchType, const Utility::NormalizedString& value)
        {
            std::optional<MatchType> result;

            if (request.Query)
            {
                result = GetBestMatchType(request.Query.value(), mustBeBetterThanMatchType, value);

                if (result)
                {
                    mustBeBetterThanMatchType = result.value();
                }
            }

            for (const auto& filter : request.Filters)
            {
                if (result.value_or(MatchType::Wildcard) == MatchType::Exact)
                {
                    break;
                }

                if (filter.Field == field)
                {
                    std::optional<MatchType> filterResult = GetBestMatchType(filter, mustBeBetterThanMatchType, value);

                    if (filterResult)
                    {
                        result = std::move(filterResult);
                        mustBeBetterThanMatchType = result.value();
                    }
                }
            }

            return result;
        }

        // Gets the best match and updates the result if it should be updated.
        // Returns true to indicate that an exact match has been found.
        bool UpdatePackageMatchFilterCheck(const SearchRequest& request, PackageMatchField field, PackageMatchFilter& result, const Utility::LocIndString& propertyValue)
        {
            Utility::NormalizedString normalizedValue = propertyValue.get();
            auto bestMatch = GetBestMatchType(request, field, result.Type, normalizedValue);

            if (bestMatch && bestMatch.value() < result.Type)
            {
                result.Type = bestMatch.value();
                result.Field = field;
                result.Value = std::move(normalizedValue);
            }

            return MatchType::Exact == result.Type;
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

            if (UpdatePackageMatchFilterCheck(request, field, result, propertyValue))
            {
                break;
            }
        }

        // Multi-value fields
        for (auto field : { PackageMatchField::Command, PackageMatchField::Tag, PackageMatchField::PackageFamilyName,
            PackageMatchField::ProductCode, PackageMatchField::UpgradeCode })
        {
            if (MatchType::Exact == result.Type)
            {
                break;
            }

            auto propertyValues = packageVersion->GetMultiProperty(GetPackageVersionMultiPropertyFor(field));
            if (propertyValues.empty())
            {
                continue;
            }

            for (const auto& propertyValue : propertyValues)
            {
                if (UpdatePackageMatchFilterCheck(request, field, result, propertyValue))
                {
                    break;
                }
            }
        }

        return result;
    }
}
