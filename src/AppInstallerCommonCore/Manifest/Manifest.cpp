// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"
#include "winget/ManifestValidation.h"

namespace AppInstaller::Manifest
{
    ManifestVer::ManifestVer(std::string_view version)
    {
        bool validationSuccess = true;

        // Separate the extensions out
        size_t hyphenPos = version.find_first_of('-');
        if (hyphenPos != std::string_view::npos)
        {
            // The first part is the main version
            Assign(std::string{ version.substr(0, hyphenPos) }, ".");

            // The second part is the extensions
            hyphenPos += 1;
            while (hyphenPos < version.length())
            {
                size_t newPos = version.find_first_of('-', hyphenPos);

                size_t length = (newPos == std::string::npos ? version.length() : newPos) - hyphenPos;
                m_extensions.emplace_back(std::string{ version.substr(hyphenPos, length) }, ".");

                hyphenPos += length + 1;
            }
        }
        else
        {
            Assign(std::string{ version }, ".");
        }

        if (m_parts.size() > 3)
        {
            validationSuccess = false;
        }
        else
        {
            for (size_t i = 0; i < m_parts.size(); i++)
            {
                if (!m_parts[i].Other.empty())
                {
                    validationSuccess = false;
                    break;
                }
            }

            for (const Version& ext : m_extensions)
            {
                if (ext.GetParts().empty() || ext.GetParts()[0].Integer != 0)
                {
                    validationSuccess = false;
                    break;
                }
            }
        }

        if (!validationSuccess)
        {
            std::vector<ValidationError> errors;
            errors.emplace_back(ManifestError::InvalidFieldValue, "ManifestVersion", std::string{ version });
            THROW_EXCEPTION(ManifestException(std::move(errors)));
        }
    }

    bool ManifestVer::HasExtension() const
    {
        return !m_extensions.empty();
    }

    bool ManifestVer::HasExtension(std::string_view extension) const
    {
        for (const Version& ext : m_extensions)
        {
            const auto& parts = ext.GetParts();
            if (!parts.empty() && parts[0].Integer == 0 && parts[0].Other == extension)
            {
                return true;
            }
        }

        return false;
    }
}
