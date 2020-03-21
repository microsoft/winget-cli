// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestValidation.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> ValidateAndProcessFields(const YAML::Node& rootNode, const std::vector<ManifestFieldInfo> fieldInfos)
    {
        std::vector<ValidationError> errors;

        if (rootNode.size() == 0)
        {
            errors.emplace_back(ManifestError::InvalidRootNode, "", "", rootNode.Mark().line, rootNode.Mark().column);
            return errors;
        }

        std::set<std::string> processedFields;

        for (auto const& keyValuePair : rootNode)
        {
            std::string key = keyValuePair.first.as<std::string>();
            YAML::Node valueNode = keyValuePair.second;

            // We'll do case insensitive search first and validate correct case later.
            auto fieldIter = std::find_if(fieldInfos.begin(), fieldInfos.end(),
                [&](auto const& s)
                {
                    return Utility::CaseInsensitiveEquals(s.Name, key);
                });

            if (fieldIter != fieldInfos.end())
            {
                ManifestFieldInfo fieldInfo = *fieldIter;

                // Make sure the found key is in Pascal Case
                if (key != fieldInfo.Name)
                {
                    errors.emplace_back(ManifestError::FieldIsNotPascalCase, key, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column);
                }

                // Make sure it's not a duplicate key
                if (!processedFields.insert(fieldInfo.Name).second)
                {
                    errors.emplace_back(ManifestError::FieldDuplicate, fieldInfo.Name, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column);
                }

                // Validate non empty value is provided for required fields
                if (fieldInfo.Required)
                {
                    if (!valueNode.IsDefined() || valueNode.IsNull() ||  // Should be defined and not null
                        (valueNode.IsScalar() && valueNode.as<std::string>().empty()) ||  // Scalar type should have content
                        ((valueNode.IsMap() || valueNode.IsSequence()) && valueNode.size() == 0))  // Map or sequence type should have size greater than 0
                    {
                        errors.emplace_back(ManifestError::RequiredFieldEmpty, fieldInfo.Name, "", valueNode.Mark().line, valueNode.Mark().column);
                    }
                }

                // Validate value against regex
                if (!fieldInfo.RegEx.empty())
                {
                    std::string value = valueNode.as<std::string>();
                    std::regex pattern{ fieldInfo.RegEx };
                    if (!std::regex_match(value, pattern))
                    {
                        errors.emplace_back(ManifestError::InvalidFieldValue, fieldInfo.Name, value, valueNode.Mark().line, valueNode.Mark().column);
                        continue;
                    }
                }

                if (!valueNode.IsNull())
                {
                    fieldInfo.ProcessFunc(valueNode);
                }
            }
            else
            {
                errors.emplace_back(ManifestError::FieldUnknown, key, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column);
            }
        }

        // Make sure required fields are provided
        for (auto const& fieldInfo : fieldInfos)
        {
            if (fieldInfo.Required && processedFields.find(fieldInfo.Name) == processedFields.end())
            {
                errors.emplace_back(ManifestError::RequiredFieldMissing, fieldInfo.Name);
            }
        }

        return errors;
    }
}