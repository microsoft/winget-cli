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
        // A simple struct to hold the data
        struct HResultData
        {
            HRESULT Value;
            std::string_view Symbol;
            std::string_view Description;

            bool operator<(const HResultData& other) const
            {
                return Value < other.Value;
            }
        };

        // HRESULT information for our errors
        struct WinGetHResultInformation : public Errors::HResultInformation
        {
            constexpr WinGetHResultInformation(HRESULT value, std::string_view symbol, std::string_view unlocalizedDescription) :
                Errors::HResultInformation(value, symbol), m_unlocalizedDescription(unlocalizedDescription)
            {}

            constexpr WinGetHResultInformation(const HResultData& data) :
                Errors::HResultInformation(data.Value, data.Symbol), m_unlocalizedDescription(data.Description)
            {}

            Utility::LocIndString GetDescription() const override
            {
                auto localizedDescription = StringResource::TryResolveString(Utility::ConvertToUTF16(Symbol()));
                return localizedDescription ?
                    std::move(localizedDescription).value() :
                    Utility::LocIndString{ m_unlocalizedDescription };
            }

        private:
            std::string_view m_unlocalizedDescription;
        };

        // HRESULT information for our errors, with the unlocalized description only.
        struct WinGetHResultInformationUnlocalized : public Errors::HResultInformation
        {
            constexpr WinGetHResultInformationUnlocalized(HRESULT value, std::string_view symbol, std::string_view unlocalizedDescription) :
                Errors::HResultInformation(value, symbol), m_unlocalizedDescription(unlocalizedDescription)
            {}

            constexpr WinGetHResultInformationUnlocalized(const HResultData& data) :
                Errors::HResultInformation(data.Value, data.Symbol), m_unlocalizedDescription(data.Description)
            {}

            Utility::LocIndString GetDescription() const override
            {
                return Utility::LocIndString{ m_unlocalizedDescription };
            }

        private:
            std::string_view m_unlocalizedDescription;
        };

        // The information entry for an HRESULT not in the list (someone probably forgot to add an entry)
        struct UnknownHResultInformation : public Errors::HResultInformation
        {
            constexpr UnknownHResultInformation(HRESULT value) :
                Errors::HResultInformation(value)
            {}

            Utility::LocIndString GetDescription() const override
            {
                auto localizedDescription = StringResource::TryResolveString(StringResource::String::UnknownErrorCode);
                return localizedDescription ?
                    std::move(localizedDescription).value() :
                    Utility::LocIndString{ "Unknown error code"sv };
            }
        };

#define WINGET_HRESULT_INFO(_name_,_description_) HResultData{ _name_, #_name_, _description_ }

        constexpr const HResultData s_wingetHResultData[] =
        {
            // Changes to any of these errors require the corresponding resource string in winget.resw to be updated.
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
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR, "Rest API internal error"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, "Invalid rest source url"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTAPI_UNSUPPORTED_MIME_TYPE, "Unsupported MIME type returned by rest API"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION, "Invalid rest source contract version"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, "The source data is corrupted or tampered"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE, "Error reading from the stream"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED, "Package agreements were not agreed to"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR, "Error reading input in prompt"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST, "The search request is not supported by one or more sources"),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND, "The rest API endpoint is not found."),
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
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SERVICE_UNAVAILABLE, "A required service is busy or unavailable. Try again later."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESUME_ID_NOT_FOUND, "The guid provided does not correspond to a valid resume state."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH, "The current client version did not match the client version of the saved state."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE, "The resume state data is invalid."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_CANNOT_OPEN_CHECKPOINT_INDEX, "Unable to open the checkpoint database."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_RESUME_LIMIT_EXCEEDED, "Exceeded max resume limit."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, "Invalid authentication info."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED, "Authentication method not supported."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED, "Authentication failed."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_AUTHENTICATION_INTERACTIVE_REQUIRED, "Authentication failed. Interactive authentication required."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_AUTHENTICATION_CANCELLED_BY_USER, "Authentication failed. User cancelled."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT, "Authentication failed. Authenticated account is not the desired account."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND, "Repair command not found."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_REPAIR_NOT_APPLICABLE, "Repair operation is not applicable."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED, "Repair operation failed."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED, "The installer technology in use doesn't support repair."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_REPAIR_PROHIBITED, "Repair operations involving administrator privileges are not permitted on packages installed within the user scope."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SQLITE_CONNECTION_TERMINATED, "The SQLite connection was terminated to prevent corruption."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED, "Failed to get Microsoft Store package catalog."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE, "No applicable Microsoft Store package found from Microsoft Store package catalog."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SFSCLIENT_API_FAILED, "Failed to get Microsoft Store package download information."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_SFSCLIENT_PACKAGE, "No applicable Microsoft Store package download information found."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED, "Failed to retrieve Microsoft Store package license."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_SFSCLIENT_PACKAGE_NOT_SUPPORTED, "The Microsoft Store package does not support download."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED_FORBIDDEN, "Failed to retrieve Microsoft Store package license. The Microsoft Entra Id account does not have the required privilege."),
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALLER_ZERO_BYTE_FILE, "Downloaded zero byte installer; ensure that your network connection is working properly."),

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
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL, "Installation failed. Restart your PC then try again."),
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
            WINGET_HRESULT_INFO(APPINSTALLER_CLI_ERROR_INSTALL_CUSTOM_ERROR, "Installation failed with a custom installer error."),

            // Status values for check package installed status results.
            // Partial success has the success bit(first bit) set to 0.
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND, "The Apps and Features Entry for the package could not be found."),
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND, "The install location could not be found."),
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_HASH_MISMATCH, "The hash of the existing file did not match."),
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_NOT_FOUND, "File not found."),
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_ACCESS_ERROR, "The file could not be accessed."),

            // Configuration Errors
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, "The configuration file is invalid."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_YAML, "The YAML syntax is invalid."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, "A configuration field has an invalid type."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, "The configuration has an unknown version."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_SET_APPLY_FAILED, "An error occurred while applying the configuration."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, "The configuration contains a duplicate identifier."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, "The configuration is missing a dependency."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, "The configuration has an unsatisfied dependency."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_ASSERTION_FAILED, "An assertion for the configuration unit failed."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MANUALLY_SKIPPED, "The configuration was manually skipped."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED, "The user declined to continue execution."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE, "The dependency graph contains a cycle which cannot be resolved."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, "The configuration has an invalid field value."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_MISSING_FIELD, "The configuration is missing a field."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_TEST_FAILED, "Some of the configuration units failed while testing their state."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_TEST_NOT_RUN, "Configuration state was not tested."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_GET_FAILED, "The configuration unit failed getting its properties."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_HISTORY_ITEM_NOT_FOUND, "The specified configuration could not be found."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_PARAMETER_INTEGRITY_BOUNDARY, "Parameter cannot be passed across integrity boundary."),

            // Configuration Processor Errors
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED, "The configuration unit was not installed."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY, "The configuration unit could not be found."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES, "Multiple matches were found for the configuration unit; specify the module to select the correct one."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_GET, "The configuration unit failed while attempting to get the current system state."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST, "The configuration unit failed while attempting to test the current system state."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_SET, "The configuration unit failed while attempting to apply the desired state."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT, "The module for the configuration unit is available in multiple locations with the same version."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE, "Loading the module for the configuration unit failed."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT, "The configuration unit returned an unexpected result during execution."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT, "A unit contains a setting that requires the config root."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE_ADMIN, "Loading the module for the configuration unit failed because it requires administrator privileges to run."),
            WINGET_HRESULT_INFO(WINGET_CONFIG_ERROR_NOT_SUPPORTED_BY_PROCESSOR, "Operation is not supported by the configuration processor."),

            // Errors without the error bit set
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE, "The install location is not applicable."),
            WINGET_HRESULT_INFO(WINGET_INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, "The file was found but the hash was not checked."),
        };

        // Map externally defined HRESULTs to error messages we want to use here
        constexpr const HResultData s_externalHResultData[] =
        {
            // Changes to any of these errors require the corresponding resource string in winget.resw to be updated.
            HResultData{ static_cast<HRESULT>(0x803FB103), "StoreInstall_PackageNotAvailableForCurrentSystem", "The package is not compatible with the current Windows version or platform." },
            HResultData{ static_cast<HRESULT>(0x803FB104), "StoreInstall_PackageNotAvailableForCurrentSystem", "The package is not compatible with the current Windows version or platform." },
            HResultData{ static_cast<HRESULT>(0x803FB106), "StoreInstall_PackageNotAvailableForCurrentSystem", "The package is not compatible with the current Windows version or platform." },
        };

        template <size_t ArraySize>
        const HResultData* FindHResultData(HRESULT value, const HResultData (&dataArray)[ArraySize])
        {
            auto itr = std::lower_bound(std::cbegin(dataArray), std::cend(dataArray), HResultData{ value });

            if (itr != std::cend(dataArray) && itr->Value == value)
            {
                return itr;
            }

            return nullptr;
        }

        const HResultData* FindWinGetHResultData(HRESULT value)
        {
            return FindHResultData(value, s_wingetHResultData);
        }

        const HResultData* FindExternalHResultData(HRESULT value)
        {
            return FindHResultData(value, s_externalHResultData);
        }

        Utility::LocIndString GetMessageForAppInstallerHR(HRESULT hr)
        {
            const HResultData* data = FindWinGetHResultData(hr);
            return data ?
                WinGetHResultInformation(*data).GetDescription() :
                UnknownHResultInformation(hr).GetDescription();
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
                const HResultData* data = FindExternalHResultData(hr);

                if (data)
                {
                    strstr << WinGetHResultInformation(*data).GetDescription();
                }
                else
                {
                    strstr << std::system_category().message(hr);
                }
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

        HRESULT HResultInformation::Value() const
        {
            return m_value;
        }

        bool HResultInformation::operator<(const HResultInformation& other) const
        {
            return m_value < other.m_value;
        }

        Utility::LocIndView HResultInformation::Symbol() const
        {
            return Utility::LocIndView{ m_symbol };
        }

        Utility::LocIndString HResultInformation::GetDescription() const
        {
            if (HRESULT_FACILITY(m_value) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                // In case external code attempts to construct HResultInformation directly
                return GetMessageForAppInstallerHR(m_value);
            }
            else
            {
                return Utility::LocIndString{ std::system_category().message(m_value) };
            }
        }

        std::unique_ptr<HResultInformation> HResultInformation::Find(HRESULT value)
        {
            if (HRESULT_FACILITY(value) == APPINSTALLER_CLI_ERROR_FACILITY)
            {
                const HResultData* data = FindWinGetHResultData(value);

                if (data)
                {
                    return std::make_unique<WinGetHResultInformation>(*data);
                }
                else
                {
                    return std::make_unique<UnknownHResultInformation>(value);
                }
            }
            else
            {
                return std::make_unique<HResultInformation>(value);
            }
        }

        std::vector<std::unique_ptr<HResultInformation>> HResultInformation::Find(std::string_view value)
        {
            std::vector<std::unique_ptr<HResultInformation>> result;

            auto addToResultIf = [&](auto predicate)
            {
                for (const HResultData& data : s_wingetHResultData)
                {
                    if (predicate(data) &&
                        std::none_of(result.begin(), result.end(), [&](const std::unique_ptr<HResultInformation>& info) { return info->Value() == data.Value; }))
                    {
                        result.emplace_back(std::make_unique<WinGetHResultInformation>(data));
                    }
                }
            };

            addToResultIf([&](const HResultData& data) { return Utility::CaseInsensitiveEquals(data.Symbol, value); });
            addToResultIf([&](const HResultData& data) { return Utility::CaseInsensitiveContainsSubstring(data.Symbol, value); });
            addToResultIf([&](const HResultData& data) { return Utility::CaseInsensitiveContainsSubstring(data.Description, value); });

            return result;
        }

        std::vector<std::unique_ptr<HResultInformation>> GetWinGetErrors()
        {
            std::vector<std::unique_ptr<HResultInformation>> result;
            result.reserve(ARRAYSIZE(s_wingetHResultData));

            for (const HResultData& data : s_wingetHResultData)
            {
                result.emplace_back(std::make_unique<WinGetHResultInformationUnlocalized>(data));
            }

            return result;
        }
    }
}
