// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"


namespace AppInstaller
{
    namespace
    {
        const char* GetMessageForAppInstallerHR(HRESULT hr)
        {
            switch (hr)
            {
            case APPINSTALLER_CLI_ERROR_INTERNAL_ERROR:
                return "Internal Error";
            case APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS:
                return "Invalid command line arguments";
            case APPINSTALLER_CLI_ERROR_COMMAND_FAILED:
                return "Executing command failed";
            case APPINSTALLER_CLI_ERROR_MANIFEST_FAILED:
                return "Opening manifest failed";
            case APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED:
                return "Cancellation signal received";
            case APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED:
                return "Running ShellExecute failed";
            case APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION:
                return "Cannot process manifest. The manifest version is higher than supported. Please update the client.";
            case APPINSTALLER_CLI_ERROR_DOWNLOAD_FAILED:
                return "Downloading installer failed";
            case APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX:
                return "Cannot write to index; it is a higher schema version";
            case APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED:
                return "The index is corrupt";
            case APPINSTALLER_CLI_ERROR_SOURCES_INVALID:
                return "The configured source information is corrupt";
            case APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS:
                return "The source name is already configured";
            case APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE:
                return "The source type is invalid";
            case APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE:
                return "The MSIX file is a bundle, not a package";
            case APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING:
                return "Data required by the source is missing";
            case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER:
                return "None of the installers are applicable for the current system";
            case APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH:
                return "The installer file's hash does not match the manifest";
            case APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST:
                return "The source name does not exist";
            case APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS:
                return "The source location is already configured under another name";
            case APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND:
                return "No packages found";
            case APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED:
                return "No sources are configured";
            case APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND:
                return "Multiple packages found matching the criteria";
            case APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND:
                return "No manifest found matching the criteria";
            case APPINSTALLER_CLI_ERROR_EXTENSION_PUBLIC_FAILED:
                return "Failed to get Public folder from source package";
            case APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN:
                return "Command requires administrator privileges to run";
            case APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE:
                return "The source location is not secure";
            case APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY:
                return "The Microsoft Store client is blocked by policy";
            case APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY:
                return "The Microsoft Store app is blocked by policy";
            case APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED:
                return "The feature is currently under development. It can be enabled using winget settings.";
            case APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED:
                return "Failed to install the Microsoft Store app";
            case APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD:
                return "Failed to perform auto complete";
            case APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED:
                return "Failed to initialize YAML parser";
            case APPINSTALLER_CLI_ERROR_YAML_INVALID_MAPPING_KEY:
                return "Encountered an invalid YAML key";
            case APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY:
                return "Encountered a duplicate YAML key";
            case APPINSTALLER_CLI_ERROR_YAML_INVALID_OPERATION:
                return "Invalid YAML operation";
            case APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED:
                return "Failed to build YAML doc";
            case APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE:
                return "Invalid YAML emitter state";
            case APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA:
                return "Invalid YAML data";
            case APPINSTALLER_CLI_ERROR_LIBYAML_ERROR:
                return "LibYAML error";
            case APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_WARNING:
                return "Manifest validation succeeded with warning";
            case APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE:
                return "Manifest validation failed";
            case APPINSTALLER_CLI_ERROR_INVALID_MANIFEST:
                return "Manifest is invalid";
            case APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE:
                return "No applicable update found";
            case APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE:
                return "winget upgrade --all completed with failures";
            default:
                return "Uknown Error Code";
            }
        }

        void GetUserPresentableMessageForHR(std::ostringstream& strstr, HRESULT hr)
        {
            strstr << "0x" << Logging::SetHRFormat << hr << " : ";

            if (HRESULT_FACILITY(hr) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                strstr << GetMessageForAppInstallerHR(hr);
            }
            else
            {
                strstr << std::system_category().message(hr);
            }
        }
    }

    std::string GetUserPresentableMessage(const wil::ResultException& re)
    {
        const auto& info = re.GetFailureInfo();

        std::ostringstream strstr;

        // We assume that if the exception has a message, that message is relevant to show to the user.
        if (info.pszMessage)
        {
            strstr << Utility::ConvertToUTF8(info.pszMessage) << std::endl;
        }

        GetUserPresentableMessageForHR(strstr, re.GetErrorCode());

        return strstr.str();
    }

    std::string GetUserPresentableMessage(const std::exception& e)
    {
        return e.what();
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    std::string GetUserPresentableMessage(const winrt::hresult_error& hre)
    {
        return Utility::ConvertToUTF8(hre.message());
    }
#endif
}
