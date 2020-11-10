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

    Version::Version(std::string&& version, std::string_view splitChars)
    {
        Assign(std::move(version), splitChars);
    }

    void Version::Assign(std::string&& version, std::string_view splitChars)
    {
        m_version = std::move(version);
        size_t pos = 0;

        while (pos < m_version.length())
        {
            size_t newPos = m_version.find_first_of(splitChars, pos);

            size_t length = (newPos == std::string::npos ? m_version.length() : newPos) - pos;
            m_parts.emplace_back(m_version.substr(pos, length));

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
        // Sort Latest higher than any other values
        bool thisIsLatest = IsLatest();
        bool otherIsLatest = other.IsLatest();

        if (thisIsLatest || otherIsLatest)
        {
            // If at least one is latest, this can only be less than if the other is and this is not.
            return (otherIsLatest && !thisIsLatest);
        }

        // Sort Unknown lower than any known values
        bool thisIsUnknown = IsUnknown();
        bool otherIsUnknown = other.IsUnknown();

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
        if ((IsLatest() && other.IsLatest()) ||
            (IsUnknown() && other.IsUnknown()))
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
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Latest));
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
        return (m_parts.size() == 1 && m_parts[0].Integer == 0 && Utility::CaseInsensitiveEquals(m_parts[0].Other, s_Version_Part_Unknown));
    }

    Version Version::CreateUnknown()
    {
        Version result;
        result.m_version = s_Version_Part_Unknown;
        result.m_parts.emplace_back(0, std::string{ s_Version_Part_Unknown });
        return result;
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
}
