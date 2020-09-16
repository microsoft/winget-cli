// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"
#include "winget/ManifestValidation.h"

namespace AppInstaller::Manifest
{
    ManifestVer::ManifestVer(std::string version, bool fullValidation) : Version(std::move(version), ".")
    {
        bool validationSuccess = true;

        if (m_parts.size() > 3)
        {
            validationSuccess = false;
        }
        else
        {
            for (size_t i = 0; i < m_parts.size(); i++)
            {
                if (!m_parts[i].Other.empty() &&
                    (i < 2 || fullValidation))
                {
                    validationSuccess = false;
                    break;
                }
            }
        }

        if (!validationSuccess)
        {
            std::vector<ValidationError> errors;
            errors.emplace_back(ManifestError::InvalidFieldValue, "ManifestVersion", m_version);
            THROW_EXCEPTION(ManifestException(std::move(errors)));
        }
    }

    bool ManifestVer::HasExtension() const
    {
        return m_parts.size() == 3 && !m_parts[2].Other.empty();
    }

    bool ManifestVer::HasExtension(std::string_view extension) const
    {
        // Could parse in the constructor, but we don't need the info that often, so just check here.

        if (!HasExtension())
        {
            return false;
        }

        // First, split every extension by hyphen.
        const std::string& other = m_parts[2].Other;
        size_t pos = 0;

        while (pos < other.length())
        {
            size_t newPos = other.find_first_of('-', pos);

            size_t length = (newPos == std::string::npos ? other.length() : newPos) - pos;
            Version extVer = other.substr(pos, length);

            // In the future we might want a more robust versioning scheme for extensions, so here
            // we use Version to parse the extension. The first part should be the extension name.
            const auto& parts = extVer.GetParts();
            if (!parts.empty() && parts[0].Integer == 0 && parts[0].Other == extension)
            {
                return true;
            }

            pos += length + 1;
        }

        return false;
    }
}
