// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>
#include <functional>
#include <wil/result.h>
#include <AppInstallerErrors.h>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
    namespace ManifestError
    {
        const char* const InvalidRootNode = "Manifest: Encountered unexpected root node.";
        const char* const KeyUnknown = "Manifest: Unknown key";
        const char* const KeyIsNotPascalCase = "Manifest: All keys should be PascalCased";
        const char* const KeyDuplicate = "Manifest: Duplicate key found in the manifest";
        const char* const RequiredFieldEmpty = "Manifest: Required field with empty value";
        const char* const RequiredFieldMissing = "Manifest: Required field missing";
        const char* const InvalidFieldValue = "Manifest: Invalid field";
    }

    struct ValidationError
    {
        std::string Message;
        std::string Field;
        std::string Value;
        int Line;
        int Column;

        ValidationError(std::string message, std::string field = "", std::string value = "", int line = -1, int column = -1) :
            Message(message), Field(field), Value(value), Line(line), Column(column) {}
    };

    struct ManifestFieldInfo
    {
        std::string Name;
        std::function<void(const YAML::Node&)> ProcessFunc;
        bool Required = false;
        std::string RegEx = "";
    };

    std::vector<ValidationError> ValidateAndProcessFields(const YAML::Node& rootNode, const std::vector<ManifestFieldInfo> fieldInfos);

    struct ManifestException : public wil::ResultException
    {
        ManifestException() : wil::ResultException(APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) {}
    };
}