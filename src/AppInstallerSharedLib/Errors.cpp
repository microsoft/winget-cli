// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"
#include "Public/winget/Resources.h"


namespace AppInstaller
{
    namespace
    {
        // HRESULT information for our errors
        struct WinGetHResultInformation : public Errors::HResultInformation
        {
            constexpr WinGetHResultInformation(HRESULT value, std::string_view symbol, std::string_view unlocalizedDescription) :
                Errors::HResultInformation(value, symbol), m_unlocalizedDescription(unlocalizedDescription)
            {}

            std::string GetDescription() override
            {
                // TODO
            }

        private:
            std::string_view m_unlocalizedDescription;
        };

#define WINGET_HRESULT_INFO(_name_,_description_) WinGetHResultInformation{ _name_, #_name_, _description_ }

        constexpr WinGetHResultInformation s_wingetHResultInformations[] =
        {
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR, "Internal Error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, "Invalid command line arguments"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_COMMAND_FAILED, "Executing command failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MANIFEST_FAILED, "Opening manifest failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED, "Cancellation signal received"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED, "Running ShellExecute failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION, "Cannot process manifest. The manifest version is higher than supported. Please update the client."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DOWNLOAD_FAILED, "Downloading installer failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, "Cannot write to index; it is a higher schema version"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED, "The index is corrupt"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCES_INVALID, "The configured source information is corrupt"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, "The source name is already configured"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE, "The source type is invalid"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, "The MSIX file is a bundle, not a package"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING, "Data required by the source is missing"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER, "None of the installers are applicable for the current system"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH, "The installer file's hash does not match the manifest"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST, "The source name does not exist"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS, "The source location is already configured under another name"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND, "No packages found"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED, "No sources are configured"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND, "Multiple packages found matching the criteria"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND, "No manifest found matching the criteria"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_EXTENSION_PUBLIC_FAILED, "Failed to get Public folder from source package"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN, "Command requires administrator privileges to run"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, "The source location is not secure"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY, "The Microsoft Store client is blocked by policy"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY, "The Microsoft Store app is blocked by policy"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED, "The feature is currently under development. It can be enabled using winget settings."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED, "Failed to install the Microsoft Store app"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD, "Failed to perform auto complete"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, "Failed to initialize YAML parser"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_INVALID_MAPPING_KEY, "Encountered an invalid YAML key"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY, "Encountered a duplicate YAML key"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_INVALID_OPERATION, "Invalid YAML operation"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED, "Failed to build YAML doc"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE, "Invalid YAML emitter state"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA, "Invalid YAML data"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_LIBYAML_ERROR, "LibYAML error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_WARNING, "Manifest validation succeeded with warning"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE, "Manifest validation failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, "Manifest is invalid"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE, "No applicable update found"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE, "winget upgrade --all completed with failures"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED, "Installer failed security check"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DOWNLOAD_SIZE_MISMATCH, "Download size does not match expected content length"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND, "Uninstall command not found"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED, "Running uninstall command failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR, "ICU break iterator error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR, "ICU casemap error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ICU_REGEX_ERROR, "ICU regex error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_IMPORT_INSTALL_FAILED, "Failed to install one or more imported packages"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND, "Could not find one or more requested packages"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, "Json file is invalid"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_NOT_REMOTE, "The source location is not remote"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, "The configured rest source is not supported"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA, "Invalid data returned by rest source"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, "Operation is blocked by Group Policy"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INTERNAL_ERROR, "Rest source internal error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, "Invalid rest source url"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE, "Unsupported MIME type returned by rest source"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION, "Invalid rest source contract version"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, "The source data is corrupted or tampered"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE, "Error reading from the stream"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED, "Package agreements were not agreed to"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR, "Error reading input in prompt"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST, "The search request is not supported by one or more sources"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_ENDPOINT_NOT_FOUND, "The rest source endpoint is not found."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED, "Failed to open the source."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED, "Source agreements were not agreed to"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH, "Header size exceeds the allowable limit of 1024 characters. Please reduce the size and try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MISSING_RESOURCE_FILE, "Missing resource file"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MSI_INSTALL_FAILED, "Running MSI install failed"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT, "Arguments for msiexec are invalid"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES, "Failed to open one or more sources"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED, "Failed to validate dependencies"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE, "One or more package is missing"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_TABLE_COLUMN, "Invalid table column"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_NOT_NEWER, "The upgrade version is not newer than the installed version"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_UNKNOWN, "Upgrade version is unknown and override is not specified"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ICU_CONVERSION_ERROR, "ICU conversion error"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED, "Failed to install portable package"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PORTABLE_REPARSE_POINT_NOT_SUPPORTED, "Volume does not support reparse points"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, "Portable package from a different source already exists."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY, "Unable to create symlink, path points to a directory."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALLER_PROHIBITS_ELEVATION, "The installer cannot be run from an administrator context."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED, "Failed to uninstall portable package"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED, "Failed to validate DisplayVersion values against index."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT, "One or more arguments are not supported."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_BIND_WITH_EMBEDDED_NULL, "Embedded null characters are disallowed for SQLite"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND, "Failed to find the nested installer in the archive."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED, "Failed to extract archive."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_INVALID_PATH, "Invalid relative file path to nested installer provided."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH, "The server certificate did not match any of the expected values."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED, "Install location must be provided."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED, "Archive malware scan failed."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_ALREADY_INSTALLED, "Found at least one version of the package installed."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS, "A pin already exists for the package."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST, "There is no pin for the package."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CANNOT_OPEN_PINNING_INDEX, "Unable to open the pin database."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MULTIPLE_INSTALL_FAILED, "One or more applications failed to install"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_MULTIPLE_UNINSTALL_FAILED, "One or more applications failed to uninstall"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE, "One or more queries did not return exactly one match"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_IS_PINNED, "The package has a pin that prevents upgrade."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB, "The package currently installed is the stub package"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_APPTERMINATION_RECEIVED, "Application shutdown signal received"),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DOWNLOAD_DEPENDENCIES, "Failed to download package dependencies."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DOWNLOAD_COMMAND_PROHIBITED, "Failed to download package. Download for offline installation is prohibited."),

// Install errors.
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE, "Application is currently running. Exit the application then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_INSTALL_IN_PROGRESS, "Another installation is already in progress. Try again later."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_FILE_IN_USE, "One or more file is being used. Exit the application then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY, "This package has a dependency missing from your system."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_DISK_FULL, "There's no more space on your PC. Make space, then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_INSUFFICIENT_MEMORY, "There's not enough memory available to install. Close other applications then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_NO_NETWORK, "This application requires internet connectivity. Connect to a network then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT, "This application encountered an error during installation. Contact support."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH, "Restart your PC to finish installation."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL, "Installation failed. Restart your PC then try again."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_INITIATED, "Your PC will restart to finish installation."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_CANCELLED_BY_USER, "You cancelled the installation."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_ALREADY_INSTALLED, "Another version of this application is already installed."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_DOWNGRADE, "A higher version of this application is already installed."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_BLOCKED_BY_POLICY, "Organization policies are preventing installation. Contact your admin."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES, "Failed to install package dependencies."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE_BY_APPLICATION, "Application is currently in use by another application."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_INVALID_PARAMETER, "Invalid parameter."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED, "Package not supported by the system."),
WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_UPGRADE_NOT_SUPPORTED, "The installer does not support upgrading an existing package."),

// Status values for check package installed status results.
// Partial success has the success bit(first bit) set to 0.
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND, "The Apps and Features Entry for the package could not be found."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE, "The install location is not applicable."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND, "The install location could not be found."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_HASH_MISMATCH, "The hash of the existing file did not match."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_NOT_FOUND, "File not found."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, "The file was found but the hash was not checked."),
WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_ACCESS_ERROR, "The file could not be accessed."),

// Configuration Errors
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_YAML, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_SET_APPLY_FAILED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_ASSERTION_FAILED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MANUALLY_SKIPPED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MISSING_FIELD, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_TEST_FAILED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_TEST_NOT_RUN, ),

// Configuration Processor Errors
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_GET, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_SET, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT, ),
WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT, ),
        };

        const char* GetMessageForAppInstallerHR(HRESULT hr)
        {
            switch (hr)
            {
            // Configuration Errors
            case WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE:
                return "The configuration file is invalid.";
            case WINGET_CONFIG_ERROR_INVALID_YAML:
                return "The YAML syntax is invalid.";
            case WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE:
                return "A configuration field has an invalid type.";
            case WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION:
                return "The configuration has an unknown version.";
            case WINGET_CONFIG_ERROR_SET_APPLY_FAILED:
                return "An error occurred while applying the configuration.";
            case WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER:
                return "The configuration contains a duplicate identifier.";
            case WINGET_CONFIG_ERROR_MISSING_DEPENDENCY:
                return "The configuration is missing a dependency.";
            case WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED:
                return "The configuration has an unsatisfied dependency.";
            case WINGET_CONFIG_ERROR_ASSERTION_FAILED:
                return "An assertion for the configuration unit failed.";
            case WINGET_CONFIG_ERROR_MANUALLY_SKIPPED:
                return "The configuration was manually skipped.";
            case WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED:
                return "A warning was thrown and the user declined to continue execution.";
            case WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE:
                return "The dependency graph contains a cycle which cannot be resolved.";
            case WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE:
                return "The configuration has an invalid field value.";
            case WINGET_CONFIG_ERROR_MISSING_FIELD:
                return "The configuration is missing a field.";
            case WINGET_CONFIG_ERROR_TEST_FAILED:
                return "Some of the configuration units failed while testing their state.";
            case WINGET_CONFIG_ERROR_TEST_NOT_RUN:
                return "Configuration state was not tested.";

            // Configuration Processor Errors
            case WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED:
                return "The configuration unit was not installed.";
            case WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY:
                return "The configuration unit could not be found.";
            case WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES:
                return "Multiple matches were found for the configuration unit; specify the module to select the correct one.";
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_GET:
                return "The configuration unit failed while attempting to get the current system state.";
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST:
                return "The configuration unit failed while attempting to test the current system state.";
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_SET:
                return "The configuration unit failed while attempting to apply the desired state.";
            case WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT:
                return "The module for the configuration unit is available in multiple locations with the same version.";
            case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE:
                return "Loading the module for the configuration unit failed.";
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT:
                return "The configuration unit returned an unexpected result during execution.";

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

    namespace Errors
    {
        constexpr HResultInformation::HResultInformation(HRESULT value) :
            m_value(value) {}

        constexpr HResultInformation::HResultInformation(HRESULT value, std::string_view symbol) :
            m_value(value), m_symbol(symbol) {}

        HRESULT HResultInformation::Value()
        {
            return m_value;
        }

        std::string_view HResultInformation::Symbol()
        {
            return m_symbol;
        }

        std::string HResultInformation::GetDescription()
        {
            if (HRESULT_FACILITY(m_value) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                // TODO: Replace this branch with implementation specific to our facility
                return GetMessageForAppInstallerHR(m_value);
            }
            else
            {
                return std::system_category().message(m_value);
            }
        }

        HResultInformation HResultInformation::Find(HRESULT value)
        {
            if (HRESULT_FACILITY(value) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                return FindAICLIHResultInformation(value);
            }
            else
            {
                return { value };
            }
        }

        std::vector<const HResultInformation*> HResultInformation::Find(std::string_view value)
        {
            // This search 
        }
    }
}
