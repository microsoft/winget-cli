// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerVersions.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    using namespace std::string_view_literals;

    static constexpr std::string_view s_Version_Part_Latest = "Latest"sv;
    static constexpr std::string_view s_Version_Part_Unknown = "Unknown"sv;

    static constexpr std::string_view s_Approximate_Less_Than = "< "sv;
    static constexpr std::string_view s_Approximate_Greater_Than = "> "sv;

    Version::Version(std::string&& version, std::string_view splitChars)
    {
        Assign(std::move(version), splitChars);
    }

    void Version::Assign(std::string&& version, std::string_view splitChars)
    {
        m_version = std::move(version);

        // Process approximate comparator if applicable
        std::string baseVersion = m_version;
        if (CaseInsensitiveStartsWith(m_version, s_Approximate_Less_Than))
        {
            m_approximateComparator = ApproximateComparator::LessThan;
            baseVersion = m_version.substr(2, m_version.length() - 2);
        }
        else if (CaseInsensitiveStartsWith(m_version, s_Approximate_Greater_Than))
        {
            m_approximateComparator = ApproximateComparator::GreaterThan;
            baseVersion = m_version.substr(2, m_version.length() - 2);
        }

        // Then parse the base version
        size_t pos = 0;

        while (pos < baseVersion.length())
        {
            size_t newPos = baseVersion.find_first_of(splitChars, pos);

            size_t length = (newPos == std::string::npos ? baseVersion.length() : newPos) - pos;
            m_parts.emplace_back(baseVersion.substr(pos, length));

            pos += length + 1;
        }

        // Remove trailing empty versions (0 or empty)
        while (!m_parts.empty())
        {
            const Part& part = m_parts.back();
            if (part.Integer == 0 && part.Other.empty())
            {
                m_parts.pop_back();
            }
            else
            {
                break;
            }
        }
    }

    bool Version::operator<(const Version& other) const
    {
        // If base versions are same, result is baed on approximate comparator
        Version thisBase = *this;
        thisBase.m_approximateComparator = ApproximateComparator::None;
        Version otherBase = other;
        otherBase.m_approximateComparator = ApproximateComparator::None;
        if (thisBase == otherBase)
        {
            // Only true if this is less than, other is not, OR this is none, other is greater than
            return (m_approximateComparator == ApproximateComparator::LessThan && other.m_approximateComparator != ApproximateComparator::LessThan) ||
                (m_approximateComparator == ApproximateComparator::None && other.m_approximateComparator == ApproximateComparator::GreaterThan);
        }

        // The approximate comparator can be ignored now, just compare the base version.

        // Sort Latest higher than any other values
        bool thisIsLatest = IsBaseVersionLatest();
        bool otherIsLatest = other.IsBaseVersionLatest();

        if (thisIsLatest || otherIsLatest)
        {
            // If at least one is latest, this can only be less than if the other is and this is not.
            return (otherIsLatest && !thisIsLatest);
        }

        // Sort Unknown lower than any known values
        bool thisIsUnknown = IsBaseVersionUnknown();
        bool otherIsUnknown = other.IsBaseVersionUnknown();

        if (thisIsUnknown || otherIsUnknown)
        {
            // If at least one is unknown, this can only be less than if it is and the other is not.
            return (thisIsUnknown && !otherIsUnknown);
        }

        for (size_t i = 0; i < m_parts.size(); ++i)
        {
            if (i >= other.m_parts.size())
            {
                // All parts equal to this point
                break;
            }

            const Part& partA = m_parts[i];
            const Part& partB = other.m_parts[i];

            if (partA < partB)
            {
                return true;
            }
            else if (partB < partA)
            {
                return false;
            }
            // else parts are equal, so continue to next part
        }

        // All parts tested were equal, so this is only less if there are more parts in other.
        return m_parts.size() < other.m_parts.size();
    }

    bool Version::operator>(const Version& other) const
    {
        return other < *this;
    }

    bool Version::operator<=(const Version& other) const
    {
        return !(*this > other);
    }

    bool Version::operator>=(const Version& other) const
    {
        return !(*this < other);
    }

    bool Version::operator==(const Version& other) const
    {
        if (m_approximateComparator != other.m_approximateComparator)
        {
            return false;
        }

        if ((IsBaseVersionLatest() && other.IsBaseVersionLatest()) ||
            (IsBaseVersionUnknown() && other.IsBaseVersionUnknown()))
        {
            return true;
        }

        if (m_parts.size() != other.m_parts.size())
        {
            return false;
        }

        for (size_t i = 0; i < m_parts.size(); ++i)
        {
            if (m_parts[i] != other.m_parts[i])
            {
                return false;
            }
        }

        return true;
    }

    bool Version::operator!=(const Version& other) const
    {
        return !(*this == other);
    }

    bool Version::IsLatest() const
    {
        return (m_approximateComparator == ApproximateComparator::None && IsBaseVersionLatest());
    }

    Version Version::CreateLatest()
    {
        Version result;
        result.m_version = s_Version_Part_Latest;
        result.m_parts.emplace_back(0, std::string{ s_Version_Part_Latest });
        return result;
    }

    bool Version::IsUnknown() const
    {
        return (m_approximateComparator == ApproximateComparator::None && IsBaseVersionUnknown());
    }

    Version Version::CreateUnknown()
    {
        Version result;
        result.m_version = s_Version_Part_Unknown;
        result.m_parts.emplace_back(0, std::string{ s_Version_Part_Unknown });
        return result;
    }

    Version Version::CreateLessThanApproximateVersion(const Version& base)
    {
        THROW_HR_IF(E_INVALIDARG, base.IsApproximateVersion());
        return Version{ std::string{ s_Approximate_Less_Than } + base.ToString() };
    }

    Version Version::CreateGreaterThanApproximateVersion(const Version& base)
    {
        THROW_HR_IF(E_INVALIDARG, base.IsApproximateVersion());
        return Version{ std::string{ s_Approximate_Greater_Than } + base.ToString() };
    }

    bool Version::IsBaseVersionLatest() const
    {
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Latest));
    }

    bool Version::IsBaseVersionUnknown() const
    {
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Unknown));
    }

    Version::Part::Part(const std::string& part)
    {
        const char* begin = part.c_str();
        char* end = nullptr;
        errno = 0;
        Integer = strtoull(begin, &end, 10);

        if (errno == ERANGE)
        {
            Integer = 0;
            Other = part;
        }
        else if (static_cast<size_t>(end - begin) != part.length())
        {
            Other = end;
        }
    }

    Version::Part::Part(uint64_t integer, std::string other) :
        Integer(integer), Other(std::move(other)) {}

    bool Version::Part::operator<(const Part& other) const
    {
        if (Integer < other.Integer)
        {
            return true;
        }
        else if (Integer > other.Integer)
        {
            return false;
        }
        else if (Other.empty())
        {
            // If this Other is empty, it is at least >=
            return false;
        }
        else if (!Other.empty() && other.Other.empty())
        {
            // If the other Other is empty and this is not, this is less.
            return true;
        }
        else if (Other < other.Other)
        {
            return true;
        }

        // else Other >= other.Other
        return false;
    }

    bool Version::Part::operator==(const Part& other) const
    {
        return Integer == other.Integer && Other == other.Other;
    }

    bool Version::Part::operator!=(const Part& other) const
    {
        return !(*this == other);
    }

    bool Channel::operator<(const Channel& other) const
    {
        return m_channel < other.m_channel;
    }

    VersionAndChannel::VersionAndChannel(Version&& version, Channel&& channel) : 
        m_version(std::move(version)), m_channel(std::move(channel)) {}

    std::string VersionAndChannel::ToString() const
    {
        std::string result;
        result = m_version.ToString();
        if (!m_channel.ToString().empty())
        {
            result += '[';
            result += m_channel.ToString();
            result += ']';
        }
        return result;
    }

    bool VersionAndChannel::operator<(const VersionAndChannel& other) const
    {
        if (m_channel < other.m_channel)
        {
            return true;
        }
        else if (other.m_channel < m_channel)
        {
            return false;
        }
        // We intentionally invert the order for version here.
        else if (other.m_version < m_version)
        {
            return true;
        }

        // else m_version >= other.m_version
        return false;
    }

    bool VersionAndChannel::IsUpdatedBy(const VersionAndChannel& other) const
    {
        // Channel crossing should not happen here.
        if (!Utility::ICUCaseInsensitiveEquals(m_channel.ToString(), other.m_channel.ToString()))
        {
            return false;
        }

        return m_version < other.m_version;
    }

    VersionRange::VersionRange(Version minVersion, Version maxVersion)
    {
        THROW_HR_IF(E_INVALIDARG, minVersion > maxVersion);
        m_minVersion = std::move(minVersion);
        m_maxVersion = std::move(maxVersion);
    }

    bool VersionRange::HasOverlapWith(const VersionRange& other) const
    {
        // No overlap if either is an empty range.
        if (IsEmpty() || other.IsEmpty())
        {
            return false;
        }

        return (m_minVersion >= other.m_minVersion && m_minVersion <= other.m_maxVersion) ||
            (m_maxVersion >= other.m_minVersion && m_maxVersion <= other.m_maxVersion);
    }

    bool VersionRange::HasOverlapWith(const std::vector<VersionRange>& others) const
    {
        for (auto const& range : others)
        {
            if (HasOverlapWith(range))
            {
                return true;
            }
        }

        return false;
    }

    bool VersionRange::IsSameAsSingleVersion(const Version& version) const
    {
        if (IsEmpty())
        {
            return false;
        }
    
        return m_minVersion == version && m_maxVersion == version;
    }

    bool VersionRange::operator<(const VersionRange& other) const
    {
        THROW_HR_IF(E_INVALIDARG, IsEmpty() || other.IsEmpty() || HasOverlapWith(other));
        
        return m_minVersion < other.m_minVersion;
    }

    const Version& VersionRange::GetMinVersion() const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, IsEmpty());
        return m_minVersion;
    }

    const Version& VersionRange::GetMaxVersion() const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, IsEmpty());
        return m_maxVersion;
    }
}
