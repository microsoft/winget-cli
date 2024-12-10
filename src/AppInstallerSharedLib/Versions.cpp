// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerVersions.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    using namespace std::string_view_literals;

    static constexpr std::string_view s_Digit_Characters = "0123456789"sv;
    static constexpr std::string_view s_Version_Part_Latest = "Latest"sv;
    static constexpr std::string_view s_Version_Part_Unknown = "Unknown"sv;

    static constexpr std::string_view s_Approximate_Less_Than = "< "sv;
    static constexpr std::string_view s_Approximate_Greater_Than = "> "sv;

    Version::Version(std::string&& version, std::string_view splitChars)
    {
        Assign(std::move(version), splitChars);
    }

    RawVersion::RawVersion(std::string version, std::string_view splitChars)
    {
        m_trimPrefix = false;
        Assign(std::move(version), splitChars);
    }

    Version::Version(Version baseVersion, ApproximateComparator approximateComparator) : Version(std::move(baseVersion))
    {
        if (approximateComparator == ApproximateComparator::None)
        {
            return;
        }

        THROW_HR_IF(E_INVALIDARG, this->IsApproximate() || this->IsUnknown());

        m_approximateComparator = approximateComparator;
        if (approximateComparator == ApproximateComparator::LessThan)
        {
            m_version = std::string{ s_Approximate_Less_Than } + m_version;
        }
        else if (approximateComparator == ApproximateComparator::GreaterThan)
        {
            m_version = std::string{ s_Approximate_Greater_Than } + m_version;
        }
    }

    void Version::Assign(std::string version, std::string_view splitChars)
    {
        m_version = std::move(Utility::Trim(version));

        // Process approximate comparator if applicable
        std::string baseVersion = m_version;
        if (CaseInsensitiveStartsWith(m_version, s_Approximate_Less_Than))
        {
            m_approximateComparator = ApproximateComparator::LessThan;
            baseVersion = m_version.substr(s_Approximate_Less_Than.length(), m_version.length() - s_Approximate_Less_Than.length());
        }
        else if (CaseInsensitiveStartsWith(m_version, s_Approximate_Greater_Than))
        {
            m_approximateComparator = ApproximateComparator::GreaterThan;
            baseVersion = m_version.substr(s_Approximate_Greater_Than.length(), m_version.length() - s_Approximate_Greater_Than.length());
        }

        // If there is a digit before the split character, or no split characters exist, trim off all leading non-digit characters
        size_t digitPos = baseVersion.find_first_of(s_Digit_Characters);
        size_t splitPos = baseVersion.find_first_of(splitChars);
        if (m_trimPrefix && digitPos != std::string::npos && (splitPos == std::string::npos || digitPos < splitPos))
        {
            baseVersion.erase(0, digitPos);
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

        // Trim version parts
        Trim();

        THROW_HR_IF(E_INVALIDARG, m_approximateComparator != ApproximateComparator::None && IsBaseVersionUnknown());
    }

    void Version::Trim()
    {
        while (!m_parts.empty())
        {
            const Part& part = m_parts.back();
            if (part.Integer == 0 && part.Other.empty())
            {
                m_parts.pop_back();
            }
            else
            {
                return;
            }
        }
    }

    bool Version::operator<(const Version& other) const
    {
        // Sort Latest higher than any other values
        bool thisIsLatest = IsBaseVersionLatest();
        bool otherIsLatest = other.IsBaseVersionLatest();

        if (thisIsLatest && otherIsLatest)
        {
            return ApproximateCompareLessThan(other);
        }
        else if (thisIsLatest || otherIsLatest)
        {
            // If only one is latest, this can only be less than if the other is and this is not.
            return (otherIsLatest && !thisIsLatest);
        }

        // Sort Unknown lower than any known values
        bool thisIsUnknown = IsBaseVersionUnknown();
        bool otherIsUnknown = other.IsBaseVersionUnknown();

        if (thisIsUnknown && otherIsUnknown)
        {
            // This code path should always return false as we disable approximate version for Unknown for now
            return ApproximateCompareLessThan(other);
        }
        else if (thisIsUnknown || otherIsUnknown)
        {
            // If at least one is unknown, this can only be less than if it is and the other is not.
            return (thisIsUnknown && !otherIsUnknown);
        }

        const Part emptyPart{};
        for (size_t i = 0; i < std::max(m_parts.size(), other.m_parts.size()); ++i)
        {
            // Whichever version is shorter, we need to pad it with empty parts
            const Part& partA = (i >= m_parts.size()) ? emptyPart : m_parts[i];
            const Part& partB = (i >= other.m_parts.size()) ? emptyPart : other.m_parts[i];

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

        // All parts were compared and found to be equal
        return ApproximateCompareLessThan(other);
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
        return (m_approximateComparator != ApproximateComparator::LessThan && IsBaseVersionLatest());
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
        return IsBaseVersionUnknown();
    }

    Version Version::CreateUnknown()
    {
        Version result;
        result.m_version = s_Version_Part_Unknown;
        result.m_parts.emplace_back(0, std::string{ s_Version_Part_Unknown });
        return result;
    }

    const Version::Part& Version::PartAt(size_t index) const
    {
        static Part s_zero{};

        if (index < m_parts.size())
        {
            return m_parts[index];
        }
        else
        {
            return s_zero;
        }
    }

    Version Version::GetBaseVersion() const
    {
        Version baseVersion = *this;
        baseVersion.m_approximateComparator = ApproximateComparator::None;
        if (m_approximateComparator == ApproximateComparator::LessThan)
        {
            baseVersion.m_version = m_version.substr(s_Approximate_Less_Than.size());
        }
        else if (m_approximateComparator == ApproximateComparator::GreaterThan)
        {
            baseVersion.m_version = m_version.substr(s_Approximate_Greater_Than.size());
        }
        
        return baseVersion;
    }
    
    bool Version::IsBaseVersionLatest() const
    {
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Latest));
    }

    bool Version::IsBaseVersionUnknown() const
    {
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Unknown));
    }

    bool Version::ApproximateCompareLessThan(const Version& other) const
    {
        // Only true if this is less than, other is not, OR this is none, other is greater than
        return (m_approximateComparator == ApproximateComparator::LessThan && other.m_approximateComparator != ApproximateComparator::LessThan) ||
            (m_approximateComparator == ApproximateComparator::None && other.m_approximateComparator == ApproximateComparator::GreaterThan);
    }

    Version::Part::Part(const std::string& part)
    {
        std::string interimPart = Utility::Trim(part.c_str());
        const char* begin = interimPart.c_str();
        char* end = nullptr;
        errno = 0;
        Integer = strtoull(begin, &end, 10);

        if (errno == ERANGE)
        {
            Integer = 0;
            Other = interimPart;
        }
        else if (static_cast<size_t>(end - begin) != interimPart.length())
        {
            Other = end;
        }

        m_foldedOther = Utility::FoldCase(static_cast<std::string_view>(Other));
    }

    Version::Part::Part(uint64_t integer, std::string other) :
        Integer(integer), Other(std::move(Utility::Trim(other)))
    {
        m_foldedOther = Utility::FoldCase(static_cast<std::string_view>(Other));
    }

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
        else if (m_foldedOther < other.m_foldedOther)
        {
            // Compare the folded versions
            return true;
        }

        // else Other >= other.Other
        return false;
    }

    bool Version::Part::operator==(const Part& other) const
    {
        return Integer == other.Integer && m_foldedOther == other.m_foldedOther;
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

    UInt64Version::UInt64Version(UINT64 version)
    {
        Assign(version);
    }

    UInt64Version::UInt64Version(uint16_t major, uint16_t minor, uint16_t build, uint16_t revision)
    {
        Assign(major, minor, build, revision);
    }

    void UInt64Version::Assign(UINT64 version)
    {
        constexpr UINT64 mask16 = (1 << 16) - 1;
        uint16_t revision = version & mask16;
        uint16_t build = (version >> 0x10) & mask16;
        uint16_t minor = (version >> 0x20) & mask16;
        uint16_t major = (version >> 0x30) & mask16;

        Assign(major, minor, build, revision);
    }

    void UInt64Version::Assign(uint16_t major, uint16_t minor, uint16_t build, uint16_t revision)
    {
        // Construct a string representation of the provided version
        std::stringstream ssVersion;
        ssVersion << major
            << Version::DefaultSplitChars << minor
            << Version::DefaultSplitChars << build
            << Version::DefaultSplitChars << revision;
        m_version = ssVersion.str();

        // Construct the 4 parts
        m_parts = { major, minor, build, revision };

        // Trim version parts
        Trim();
    }

    UInt64Version::UInt64Version(std::string&& version, std::string_view splitChars)
    {
        Assign(std::move(version), splitChars);
    }

    void UInt64Version::Assign(std::string version, std::string_view splitChars)
    {
        Version::Assign(std::move(version), splitChars);

        // After trimming trailing parts (0 or empty),
        // at most 4 parts must be present
        THROW_HR_IF(E_INVALIDARG, m_parts.size() > 4);
        for (const auto& part : m_parts)
        {
            // Check for non-empty Other part
            THROW_HR_IF(E_INVALIDARG, !part.Other.empty());

            // Check for overflow Integer part
            THROW_HR_IF(E_INVALIDARG, part.Integer >> 16 != 0);
        }
    }

    SemanticVersion::SemanticVersion(std::string&& version)
    {
        Assign(std::move(version), DefaultSplitChars);
    }

    void SemanticVersion::Assign(std::string version, std::string_view splitChars)
    {
        // Semantic versions require using the default split character
        THROW_HR_IF(E_INVALIDARG, splitChars != DefaultSplitChars);

        // First split off any trailing build metadata
        std::string interimVersion = Utility::Trim(version);
        size_t buildMetadataPos = interimVersion.find('+', 0);

        if (buildMetadataPos != std::string::npos)
        {
            m_buildMetadata.Assign(interimVersion.substr(buildMetadataPos + 1));
            interimVersion.resize(buildMetadataPos);
        }

        // Now split off the prerelease data
        size_t prereleasePos = interimVersion.find('-', 0);

        if (prereleasePos != std::string::npos)
        {
            m_prerelease.Assign(interimVersion.substr(prereleasePos + 1));
            interimVersion.resize(prereleasePos);
        }

        // Parse main version
        Version::Assign(std::move(interimVersion), splitChars);
        THROW_HR_IF(E_INVALIDARG, IsApproximate());
        THROW_HR_IF(E_INVALIDARG, m_parts.size() > 3);
        for (size_t i = 0; i < 3; ++i)
        {
            THROW_HR_IF(E_INVALIDARG, !PartAt(i).Other.empty());
        }

        // Put rest of version back onto Other of last part
        size_t otherSplit = (prereleasePos != std::string::npos ? prereleasePos : buildMetadataPos);
        if (otherSplit != std::string::npos)
        {
            while (m_parts.size() < 3)
            {
                m_parts.emplace_back();
            }
            m_parts[2].Other = version.substr(otherSplit);
        }

        // Overwrite the whole version string with our whole version string
        m_version = std::move(version);
    }

    bool SemanticVersion::IsPrerelease() const
    {
        return !m_prerelease.IsEmpty();
    }

    const Version& SemanticVersion::PrereleaseVersion() const
    {
        return m_prerelease;
    }

    bool SemanticVersion::HasBuildMetadata() const
    {
        return !m_buildMetadata.IsEmpty();
    }

    const Version& SemanticVersion::BuildMetadata() const
    {
        return m_buildMetadata;
    }
      
    VersionRange::VersionRange(Version minVersion, Version maxVersion)
    {
        THROW_HR_IF(E_INVALIDARG, minVersion > maxVersion);
        m_minVersion = std::move(minVersion);
        m_maxVersion = std::move(maxVersion);
    }

    bool VersionRange::Overlaps(const VersionRange& other) const
    {
        // No overlap if either is an empty range.
        if (IsEmpty() || other.IsEmpty())
        {
            return false;
        }

        return m_minVersion <= other.m_maxVersion && m_maxVersion >= other.m_minVersion;
    }

    bool VersionRange::IsSameAsSingleVersion(const Version& version) const
    {
        if (IsEmpty())
        {
            return false;
        }
    
        return m_minVersion == version && m_maxVersion == version;
    }

    bool VersionRange::ContainsVersion(const Version& version) const
    {
        if (IsEmpty())
        {
            return false;
        }

        return version >= m_minVersion && version <= m_maxVersion;
    }

    bool VersionRange::operator<(const VersionRange& other) const
    {
        THROW_HR_IF(E_INVALIDARG, IsEmpty() || other.IsEmpty() || Overlaps(other));
        
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

    bool GatedVersion::IsValidVersion(Version version) const
    {
        auto gateParts = m_version.GetParts();
        if (gateParts.empty())
        {
            return false;
        }

        if (gateParts.back() != Version::Part("*"))
        {
            // Without wildcards, revert to direct comparison
            return m_version == version;
        }

        auto versionParts = version.GetParts();
        for (size_t i = 0; i < gateParts.size() - 1; ++i)
        {
            if (versionParts.size() > i)
            {
                if (gateParts[i] == versionParts[i])
                {
                    continue;
                }
                else
                {
                    // Mismatch with the gated version
                    return false;
                }
            }
            else
            {
                // Assume trailing 0s on the version
                if (gateParts[i] != Version::Part(0))
                {
                    return false;
                }
            }
        }

        // All version parts matched
        return true;
    }

    bool HasOverlapInVersionRanges(const std::vector<VersionRange>& ranges)
    {
        for (size_t i = 0; i < ranges.size(); i++)
        {
            for (size_t j = i + 1; j < ranges.size(); j++)
            {
                if (ranges[i].Overlaps(ranges[j]))
                {
                    return true;
                }
            }
        }

        return false;
    }

    OpenTypeFontVersion::OpenTypeFontVersion(std::string&& version)
    {
        Assign(std::move(version), DefaultSplitChars);
    }

    void OpenTypeFontVersion::Assign(std::string version, std::string_view splitChars)
    {
        // Open type version requires using the default split character
        THROW_HR_IF(E_INVALIDARG, splitChars != DefaultSplitChars);

        // Split on default split character.
        std::vector<std::string> parts = Split(version, '.', true);

        std::string majorString;
        std::string minorString;

        // Font version must have a "major.minor" part.
        if (parts.size() >= 2)
        {
            // Find first digit and trim all preceding characters. 
            std::string firstPart = parts[0];
            size_t majorStartIndex = firstPart.find_first_of(s_Digit_Characters);

            if (majorStartIndex != std::string::npos)
            {
                firstPart.erase(0, majorStartIndex);
            }

            size_t majorEndIndex = firstPart.find_last_of(s_Digit_Characters);
            majorString = firstPart.substr(0, majorEndIndex + 1);

            // Parse and verify minor part.
            std::string secondPart = parts[1];
            size_t endPos = secondPart.find_first_not_of(s_Digit_Characters);

            // If a non-digit character exists, trim off the remainder.
            if (endPos != std::string::npos)
            {
                secondPart.erase(endPos, secondPart.length());
            }

            minorString = secondPart;
        }

        // Verify results.
        if (!majorString.empty() && !minorString.empty())
        {
            m_parts.emplace_back(majorString);
            m_parts.emplace_back(minorString);
            m_version = Utility::Join(DefaultSplitChars, { majorString, minorString });

            Trim();
        }
        else
        {
            m_version = s_Version_Part_Unknown;
            m_parts.emplace_back(0, std::string{ s_Version_Part_Unknown });
        }
    }
}
