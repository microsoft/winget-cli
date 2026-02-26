// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/RepositorySearch.h"

using namespace AppInstaller::Settings;
using namespace std::chrono_literals;

namespace AppInstaller::Repository
{
    namespace
    {
        std::string GetStringVectorMessage(const std::vector<std::string>& input)
        {
            std::string result;
            bool first = true;
            for (auto const& field : input)
            {
                if (first)
                {
                    result += field;
                    first = false;
                }
                else
                {
                    result += ", " + field;
                }
            }
            return result;
        }
    }

    bool SearchRequest::IsForEverything() const
    {
        return (!Query.has_value() && Inclusions.empty() && Filters.empty());
    }

    std::string SearchRequest::ToString() const
    {
        std::ostringstream result;

        result << "Query:";
        if (Query)
        {
            result << '\'' << Query.value().Value << "'[" << Repository::ToString(Query.value().Type) << ']';
        }
        else
        {
            result << "[none]";
        }

        for (const auto& include : Inclusions)
        {
            result << " Include:" << Repository::ToString(include.Field) << "='" << include.Value << "'";
            if (include.Additional)
            {
                result << "+'" << include.Additional.value() << "'";
            }
            result << "[" << Repository::ToString(include.Type) << "]";
        }

        for (const auto& filter : Filters)
        {
            result << " Filter:" << Repository::ToString(filter.Field) << "='" << filter.Value << "'[" << Repository::ToString(filter.Type) << "]";
        }

        if (MaximumResults)
        {
            result << " Limit:" << MaximumResults;
        }

        return result.str();
    }

    std::string_view ToString(PackageVersionMetadata pvm)
    {
        switch (pvm)
        {
        case PackageVersionMetadata::InstalledType: return "InstalledType"sv;
        case PackageVersionMetadata::InstalledScope: return "InstalledScope"sv;
        case PackageVersionMetadata::InstalledLocation: return "InstalledLocation"sv;
        case PackageVersionMetadata::StandardUninstallCommand: return "StandardUninstallCommand"sv;
        case PackageVersionMetadata::SilentUninstallCommand: return "SilentUninstallCommand"sv;
        case PackageVersionMetadata::Publisher: return "Publisher"sv;
        case PackageVersionMetadata::InstalledLocale: return "InstalledLocale"sv;
        case PackageVersionMetadata::TrackingWriteTime: return "TrackingWriteTime"sv;
        case PackageVersionMetadata::InstalledArchitecture: return "InstalledArchitecture"sv;
        case PackageVersionMetadata::PinnedState: return "PinnedState"sv;
        case PackageVersionMetadata::UserIntentArchitecture: return "UserIntentArchitecture"sv;
        case PackageVersionMetadata::UserIntentLocale: return "UserIntentLocale"sv;
        default: return "Unknown"sv;
        }
    }

    bool PackageVersionKey::IsMatch(const PackageVersionKey& other) const
    {
        return
            ((other.SourceId.empty() || other.SourceId == SourceId) &&
             (other.Version.empty() || Utility::Version{ other.Version } == Utility::Version{ Version }) &&
             (other.Channel.empty() || Utility::ICUCaseInsensitiveEquals(other.Channel, Channel)));
    }

    bool PackageVersionKey::IsDefaultLatest() const
    {
        return Version.empty() && Channel.empty();
    }

    PackageVersionMultiProperty PackageMultiPropertyToPackageVersionMultiProperty(PackageMultiProperty property)
    {
        switch (property)
        {
        case PackageMultiProperty::PackageFamilyName: return PackageVersionMultiProperty::PackageFamilyName;
        case PackageMultiProperty::ProductCode: return PackageVersionMultiProperty::ProductCode;
        case PackageMultiProperty::UpgradeCode: return PackageVersionMultiProperty::UpgradeCode;
        case PackageMultiProperty::NormalizedName: return PackageVersionMultiProperty::Name;
        case PackageMultiProperty::NormalizedPublisher: return PackageVersionMultiProperty::Publisher;
        case PackageMultiProperty::Tag: return PackageVersionMultiProperty::Tag;
        case PackageMultiProperty::Command: return PackageVersionMultiProperty::Command;
        default:
            THROW_HR_MSG(E_UNEXPECTED, "PackageMultiProperty must map to a PackageVersionMultiProperty");
        }
    }

    const char* UnsupportedRequestException::what() const noexcept
    {
        if (m_whatMessage.empty())
        {
            m_whatMessage = "The request is not supported.";

            if (!UnsupportedPackageMatchFields.empty())
            {
                m_whatMessage += "Unsupported Package Match Fields: " + GetStringVectorMessage(UnsupportedPackageMatchFields);
            }
            if (!RequiredPackageMatchFields.empty())
            {
                m_whatMessage += "Required Package Match Fields: " + GetStringVectorMessage(RequiredPackageMatchFields);
            }
            if (!UnsupportedQueryParameters.empty())
            {
                m_whatMessage += "Unsupported Query Parameters: " + GetStringVectorMessage(UnsupportedQueryParameters);
            }
            if (!RequiredQueryParameters.empty())
            {
                m_whatMessage += "Required Query Parameters: " + GetStringVectorMessage(RequiredQueryParameters);
            }
        }
        return m_whatMessage.c_str();
    }

    std::string_view ToString(MatchType type)
    {
        using namespace std::string_view_literals;

        switch (type)
        {
        case MatchType::Exact:
            return "Exact"sv;
        case MatchType::CaseInsensitive:
            return "CaseInsensitive"sv;
        case MatchType::StartsWith:
            return "StartsWith"sv;
        case MatchType::Substring:
            return "Substring"sv;
        case MatchType::Wildcard:
            return "Wildcard"sv;
        case MatchType::Fuzzy:
            return "Fuzzy"sv;
        case MatchType::FuzzySubstring:
            return "FuzzySubstring"sv;
        }

        return "UnknownMatchType"sv;
    }

    std::string_view ToString(PackageMatchField matchField)
    {
        using namespace std::string_view_literals;

        switch (matchField)
        {
        case PackageMatchField::Command:
            return "Command"sv;
        case PackageMatchField::Id:
            return "Id"sv;
        case PackageMatchField::Moniker:
            return "Moniker"sv;
        case PackageMatchField::Name:
            return "Name"sv;
        case PackageMatchField::Tag:
            return "Tag"sv;
        case PackageMatchField::PackageFamilyName:
            return "PackageFamilyName"sv;
        case PackageMatchField::ProductCode:
            return "ProductCode"sv;
        case PackageMatchField::UpgradeCode:
            return "UpgradeCode"sv;
        case PackageMatchField::NormalizedNameAndPublisher:
            return "NormalizedNameAndPublisher"sv;
        case PackageMatchField::Market:
            return "Market"sv;
        }

        return "UnknownMatchField"sv;
    }

    PackageMatchField StringToPackageMatchField(std::string_view field)
    {
        std::string toLower = Utility::ToLower(field);

        if (toLower == "command")
        {
            return PackageMatchField::Command;
        }
        else if (toLower == "id")
        {
            return PackageMatchField::Id;
        }
        else if (toLower == "moniker")
        {
            return PackageMatchField::Moniker;
        }
        else if (toLower == "name")
        {
            return PackageMatchField::Name;
        }
        else if (toLower == "tag")
        {
            return PackageMatchField::Tag;
        }
        else if (toLower == "packagefamilyname")
        {
            return PackageMatchField::PackageFamilyName;
        }
        else if (toLower == "productcode")
        {
            return PackageMatchField::ProductCode;
        }
        else if (toLower == "upgradecode")
        {
            return PackageMatchField::UpgradeCode;
        }
        else if (toLower == "normalizednameandpublisher")
        {
            return PackageMatchField::NormalizedNameAndPublisher;
        }
        else if (toLower == "market")
        {
            return PackageMatchField::Market;
        }

        return PackageMatchField::Unknown;
    }
}
