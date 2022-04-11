// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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
            case APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED:
                return "Installer failed security check";
            case APPINSTALLER_CLI_ERROR_DOWNLOAD_SIZE_MISMATCH:
                return "Download size does not match expected content length";
            case APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND:
                return "Uninstall command not found";
            case APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED:
                return "Running uninstall command failed";
            case APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR:
                return "ICU break iterator error";
            case APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR:
                return "ICU casemap error";
            case APPINSTALLER_CLI_ERROR_ICU_REGEX_ERROR:
                return "ICU regex error";
            case APPINSTALLER_CLI_ERROR_IMPORT_INSTALL_FAILED:
                return "Failed to install one or more imported packages";
            case APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND:
                return "Could not find one or more requested packages";
            case APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE:
                return "Json file is invalid";
            case APPINSTALLER_CLI_ERROR_SOURCE_NOT_REMOTE:
                return "The source location is not remote";
            case APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE:
                return "The configured rest source is not supported";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA:
                return "Invalid data returned by rest source";
            case APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY:
                return "Operation is blocked by Group Policy";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_INTERNAL_ERROR:
                return "Rest source internal error";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL:
                return "Invalid rest source url";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE:
                return "Unsupported MIME type returned by rest source";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION:
                return "Invalid rest source contract version";
            case APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE:
                return "The source data is corrupted or tampered";
            case APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE:
                return "Error reading from the stream";
            case APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED:
                return "Package agreements were not agreed to";
            case APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR:
                return "Error reading input in prompt";
            case APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST:
                return "The search request is not supported by one or more sources";
            case APPINSTALLER_CLI_ERROR_RESTSOURCE_ENDPOINT_NOT_FOUND:
                return "The rest source endpoint is not found.";
            case APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED:
                return "Failed to open the source.";
            case APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED:
                return "Source agreements were not agreed to";
            case APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH:
                return "Header size exceeds the allowable limit of 1024 characters. Please reduce the size and try again.";
            case APPINSTALLER_CLI_ERROR_MSI_INSTALL_FAILED:
                return "Running MSI install failed";
            case APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT:
                return "Arguments for msiexec are invalid";
            case APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES:
                return "Failed to open one or more sources";
            case APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED:
                return "Failed to validate dependencies";
            case APPINSTALLER_CLI_ERROR_MISSING_PACKAGE:
                return "One or more package is missing";
            case APPINSTALLER_CLI_ERROR_INVALID_TABLE_COLUMN:
                return "Invalid table column";
            case APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_NOT_NEWER:
                return "The upgrade version is not newer than the installed version";
            case APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_UNKNOWN:
                return "Upgrade version is unknown and override is not specified";
            case APPINSTALLER_CLI_ERROR_ICU_CONVERSION_ERROR:
                return "ICU conversion error";
            case APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE:
                return "Application is currently running.Exit the application then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_INSTALL_IN_PROGRESS:
                return "Another installation is already in progress.Try again later.";
            case APPINSTALLER_CLI_ERROR_INSTALL_FILE_IN_USE:
                return "One or more file is being used.Exit the application then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY:
                return "This package has a dependency missing from your system.";
            case APPINSTALLER_CLI_ERROR_INSTALL_DISK_FULL:
                return "There's no more space on your PC. Make space, then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_INSUFFICIENT_MEMORY:
                return "There's not enough memory available to install. Close other applications then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_NO_NETWORK:
                return "This application requires internet connectivity.Connect to a network then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT:
                return "This application encountered an error during installation.Contact support.";
            case APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH:
                return "Restart your PC to finish installation.";
            case APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL:
                return "Your PC will restart to finish installation.";
            case APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_INITIATED:
                return "Installation failed. Restart your PC then try again.";
            case APPINSTALLER_CLI_ERROR_INSTALL_CANCELLED_BY_USER:
                return "You cancelled the installation.";
            case APPINSTALLER_CLI_ERROR_INSTALL_ALREADY_INSTALLED:
                return "Another version of this application is already installed.";
            case APPINSTALLER_CLI_ERROR_INSTALL_DOWNGRADE:
                return "A higher version of this application is already installed.";
            case APPINSTALLER_CLI_ERROR_INSTALL_BLOCKED_BY_POLICY:
                return "Organization policies are preventing installation. Contact your admin.";
            default:
                return "Unknown Error Code";
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

    std::string GetUserPresentableMessage(HRESULT hr)
    {
        std::ostringstream strstr;
        GetUserPresentableMessageForHR(strstr, hr);
        return strstr.str();
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    std::string GetUserPresentableMessage(const winrt::hresult_error& hre)
    {
        std::ostringstream strstr;
        GetUserPresentableMessageForHR(strstr, hre.code());
        return strstr.str();
    }
#endif
}
