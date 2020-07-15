// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Manifest.h"
#include "ManifestValidation.h"

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

    bool ManifestVer::HasTag() const
    {
        return m_parts.size() == 3 && !m_parts[2].Other.empty();
    }
}
