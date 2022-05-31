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
    //  1. Splitting the string based on the given splitChars (or DefaultSplitChars)
    //  2. Parsing a leading, positive integer from each split part
    //  3. Saving any remaining, non-digits as a supplemental value
    //
    // Versions are compared by:
    //  for each part in each version
    //      if both sides have no more parts, return equal
    //      else if one side has no more parts, it is less
    //      else if integers not equal, return comparison of integers
    //      else if only one side has a non-empty string part, it is less
    //      else if string parts not equal, return comparison of strings
    struct Version
    {
        // The default characters to split a version string on.
        constexpr static std::string_view DefaultSplitChars = "."sv;

        Version() = default;

        Version(const std::string& version, std::string_view splitChars = DefaultSplitChars) :
            Version(std::string(version), splitChars) {}
        Version(std::string&& version, std::string_view splitChars = DefaultSplitChars);

        // Resets the version's value to the input.
        void Assign(std::string&& version, std::string_view splitChars = DefaultSplitChars);

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

        // An individual version part in between split characters.
        struct Part
        {
            Part(const std::string& part);
            Part(uint64_t integer, std::string other);

            bool operator<(const Part& other) const;
            bool operator==(const Part& other) const;
            bool operator!=(const Part& other) const;

            uint64_t Integer = 0;
            std::string Other;
        };

        // Used in approximate version to indicate the relation to the base version.
        enum class ApproximateComparator
        {
            None,
            LessThan,
            GreaterThan,
        };

        // Gets the part breakdown for a given version; used for tests.
        const std::vector<Part>& GetParts() const { return m_parts; }

        // Returns if the version is an approximate version.
        bool IsApproximateVersion() const { return m_approximateComparator != ApproximateComparator::None; }

        // Static methods to create an approximate version from a base version.
        static Version CreateLessThanApproximateVersion(const Version& base);
        static Version CreateGreaterThanApproximateVersion(const Version& base);

    protected:

        bool IsBaseVersionLatest() const;
        bool IsBaseVersionUnknown() const;

        std::string m_version;
        std::vector<Part> m_parts;
        ApproximateComparator m_approximateComparator = ApproximateComparator::None;
    };

    // Version range represented by a min version and max version, both inclusive.
    struct VersionRange
    {
        VersionRange() { m_isEmpty = true; };
        VersionRange(Version minVersion, Version maxVersion);

        bool IsEmpty() const { return m_isEmpty; }

        // Checks if version ranges overlap. Empty version range does not overlap with any version range.
        bool HasOverlapWith(const VersionRange& other) const;
        bool HasOverlapWith(const std::vector<VersionRange>& others) const;

        // Checks if the version range is effectively the same as a single version.
        bool IsSameAsSingleVersion(const Version& version) const;

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

    bool HasOverlapInVersionRanges(const std::vector<VersionRange>& rabges);
}
