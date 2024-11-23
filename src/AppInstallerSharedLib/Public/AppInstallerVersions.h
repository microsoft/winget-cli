// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Utility
{
    using namespace std::string_view_literals;

    // Creates a comparable version object from a string.
    // Versions are parsed by:
    //  1. Parse approximate comparator sign if applicable
    //  2. Splitting the string based on the given splitChars (or DefaultSplitChars)
    //  3. Parsing a leading, positive integer from each split part
    //  4. Saving any remaining, non-digits as a supplemental value
    //
    // Versions are compared by:
    //  for each part in each version
    //      if one side has no more parts, perform the remaining comparisons against an empty part
    //      if integers not equal, return comparison of integers
    //      else if only one side has a non-empty string part, it is less
    //      else if string parts not equal, return comparison of strings
    //  if each part has been compared, use approximate comparator if applicable
    //
    //  Note: approximate to another approximate version is invalid.
    //        approximate to Unknown is invalid.
    struct Version
    {
        // Used in approximate version to indicate the relation to the base version.
        enum class ApproximateComparator
        {
            None,
            LessThan,
            GreaterThan,
        };

        // The default characters to split a version string on.
        constexpr static std::string_view DefaultSplitChars = "."sv;

        Version() = default;

        Version(const std::string& version, std::string_view splitChars = DefaultSplitChars) :
            Version(std::string(version), splitChars) {}
        Version(std::string&& version, std::string_view splitChars = DefaultSplitChars);

        // Constructing an approximate version from a base version.
        Version(Version baseVersion, ApproximateComparator approximateComparator);

        // Resets the version's value to the input.
        virtual void Assign(std::string version, std::string_view splitChars = DefaultSplitChars);

        // Gets the full version string used to construct the Version.
        const std::string& ToString() const { return m_version; }

        bool operator<(const Version& other) const;
        bool operator>(const Version& other) const;
        bool operator<=(const Version& other) const;
        bool operator>=(const Version& other) const;
        bool operator==(const Version& other) const;
        bool operator!=(const Version& other) const;

        // Determines if this version is the sentinel value defining the 'Latest' version
        bool IsLatest() const;

        // Returns a Version that will return true for IsLatest
        static Version CreateLatest();

        // Determines if this version is the sentinel value defining an 'Unknown' version
        bool IsUnknown() const;

        // Returns a Version that will return true for IsUnknown
        static Version CreateUnknown();

        // Gets a bool indicating whether the full version string is empty.
        // Does not indicate that Parts is empty; for instance when "0.0" is given,
        // this will be false while GetParts().empty() would be true.
        bool IsEmpty() const { return m_version.empty(); }

        // An individual version part in between split characters.
        struct Part
        {
            Part() = default;
            Part(uint64_t integer) : Integer(integer) {}
            Part(const std::string& part);
            Part(uint64_t integer, std::string other);

            bool operator<(const Part& other) const;
            bool operator==(const Part& other) const;
            bool operator!=(const Part& other) const;

            uint64_t Integer = 0;
            std::string Other;

        private:
            std::string m_foldedOther;
        };

        // Gets the part breakdown for a given version.
        const std::vector<Part>& GetParts() const { return m_parts; }

        // Gets the part at the given index; or the implied zero part if past the end.
        const Part& PartAt(size_t index) const;
        
        // Returns if the version is an approximate version.
        bool IsApproximate() const { return m_approximateComparator != ApproximateComparator::None; }

        // Get the base version from approximate version, or return a copy if the version is not approximate.
        Version GetBaseVersion() const;

    protected:

        bool IsBaseVersionLatest() const;
        bool IsBaseVersionUnknown() const;
        // Called by overloaded less than operator implementation when base version already compared and equal, less than determined by approximate comparator.
        bool ApproximateCompareLessThan(const Version& other) const;

        std::string m_version;
        std::vector<Part> m_parts;
        bool m_trimPrefix = true;
        ApproximateComparator m_approximateComparator = ApproximateComparator::None;

      // Remove trailing empty parts (0 or empty)
        void Trim();
    };

    // Version that does not have leading non-digit characters trimmed
    struct RawVersion : protected Version
    {
        RawVersion() { m_trimPrefix = false; }
        RawVersion(std::string version, std::string_view splitChars = DefaultSplitChars);

        using Version::GetParts;
    };

    // Four parts version number: 16-bits.16-bits.16-bits.16-bits
    struct UInt64Version : public Version
    {
        UInt64Version() = default;
        UInt64Version(UINT64 version);
        UInt64Version(uint16_t major, uint16_t minor, uint16_t build, uint16_t revision);
        UInt64Version(std::string&& version, std::string_view splitChars = DefaultSplitChars);
        UInt64Version(const std::string& version, std::string_view splitChars = DefaultSplitChars) :
            UInt64Version(std::string(version), splitChars) {}

        void Assign(std::string version, std::string_view splitChars = DefaultSplitChars) override;
        void Assign(UINT64 version);
        void Assign(uint16_t major, uint16_t minor, uint16_t build, uint16_t revision);

        UINT64 Major() const { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        UINT64 Minor() const { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        UINT64 Build() const { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }
        UINT64 Revision() const { return m_parts.size() > 3 ? m_parts[3].Integer : 0; }
    };

    // A semantic version as defined by semver.org
    // This is currently fairly loose in its restrictions on a version, and is largely to separate out the prelease and build metadata portions.
    struct SemanticVersion : public Version
    {
        SemanticVersion() = default;
        SemanticVersion(std::string&& version);
        SemanticVersion(const std::string& version) :
            SemanticVersion(std::string(version)) {}

        void Assign(std::string version, std::string_view splitChars = DefaultSplitChars) override;

        // Indicates that the version is pre-release
        bool IsPrerelease() const;
        // The pre-release version
        const Version& PrereleaseVersion() const;

        // Indicates that the version has build metadata
        bool HasBuildMetadata() const;
        // The build metadata version
        const Version& BuildMetadata() const;

    private:
        Version m_prerelease;
        Version m_buildMetadata;
    };

    // Version range represented by a min version and max version, both inclusive.
    struct VersionRange
    {
        VersionRange() { m_isEmpty = true; };
        VersionRange(Version minVersion, Version maxVersion);

        bool IsEmpty() const { return m_isEmpty; }

        // Checks if version ranges overlap. Empty version range does not overlap with any version range.
        bool Overlaps(const VersionRange& other) const;

        // Checks if the version range is effectively the same as a single version.
        bool IsSameAsSingleVersion(const Version& version) const;

        // Checks if a version is within the version range
        bool ContainsVersion(const Version& version) const;

        // < operator will thow if compared with an empty range or an overlapped range
        bool operator<(const VersionRange& other) const;

        const Version& GetMinVersion() const;
        const Version& GetMaxVersion() const;

    private:
        Version m_minVersion;
        Version m_maxVersion;
        bool m_isEmpty = false;
    };

    // A range of versions indicated by a version and optionally a wildcard at the end.
    struct GatedVersion
    {
        GatedVersion() {}
        GatedVersion(Version&& version) : m_version(std::move(version)) {}
        GatedVersion(const Version& version) : m_version(version) {}
        GatedVersion(std::string_view versionString) : m_version(std::string{ versionString }) {}
        GatedVersion(const std::string& versionString) : m_version(versionString) {}

        // Determines whether a given version falls within this Gated version.
        // I.e., whether it matches up to the wildcard
        bool IsValidVersion(Version version) const;

        bool operator==(const GatedVersion& other) const { return m_version == other.m_version; }
        const std::string& ToString() const { return m_version.ToString(); }

    private:
        // Hold the version string as a Version object that makes it easy to access each of
        // the version's parts. The real magic is in IsValidVersion()
        Version m_version;
    };

    // A channel string; existing solely to give a type.
    //
    // Compared lexicographically.
    struct Channel
    {
        Channel() = default;
        Channel(const std::string& channel) : m_channel(channel) {}
        Channel(std::string&& channel) : m_channel(std::move(channel)) {}

        const std::string& ToString() const { return m_channel; }

        bool operator<(const Channel& other) const;

    private:
        std::string m_channel;
    };

    // Contains a version and channel.
    // These are compared by:
    //  if channel not equal, return compare channel
    //  else return !compare version
    //
    // The implication of this is that the default less sort will be:
    //  2.0, ""
    //  1.0, ""
    //  3.0, "alpha"
    //  2.0, "alpha"
    struct VersionAndChannel
    {
        VersionAndChannel() = default;
        VersionAndChannel(Version&& version, Channel&& channel);

        const Version& GetVersion() const { return m_version; }
        const Channel& GetChannel() const { return m_channel; }

        std::string ToString() const;

        bool operator<(const VersionAndChannel& other) const;

        // A convenience function to make more semantic sense at call sites over the somewhat awkward less than ordering.
        bool IsUpdatedBy(const VersionAndChannel& other) const;

    private:
        Version m_version;
        Channel m_channel;
    };

    // Checks if there are overlaps within the list of version ranges
    bool HasOverlapInVersionRanges(const std::vector<VersionRange>& ranges);

    // The OpenType font version.
    // The format of this version type is 'Version 1.234 ;567'
    // The only part that is of importance is the 'Major.Minor' parts.
    // The 'Version' string is typically found at the beginning of the version string.
    // Any value after a digit that is not a '.' represents some other meaning.
    struct OpenTypeFontVersion : Version
    {
        OpenTypeFontVersion() = default;

        OpenTypeFontVersion(std::string&& version);
        OpenTypeFontVersion(const std::string& version) :
            OpenTypeFontVersion(std::string(version)) {}

        void Assign(std::string version, std::string_view splitChars = DefaultSplitChars) override;
    };
}
