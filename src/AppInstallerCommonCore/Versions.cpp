// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerVersions.h"

namespace AppInstaller::Utility
{
    Version::Version(std::string&& version, std::string_view splitChars) :
        m_version(std::move(version))
    {
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

    Version::Part::Part(const std::string& part)
    {
        size_t end = 0;
        try
        {
            Integer = std::stoull(part, &end);
        }
        CATCH_LOG();
        if (end != part.length())
        {
            Other = part.substr(end);
        }
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
}
