---
title: Exit Codes
description: WinGet return codes and their meanings
ms.date: 05/02/2023
ms.topic: article
ms.localizationpriority: medium
---

# Return Codes

## General Errors

| Hex | Decimal | Symbol | Description |
|-------------|-------------|-------------|-------------|
| 0x8A150001 | -1978335231 | APPINSTALLER_CLI_ERROR_INTERNAL_ERROR | Internal Error |
| 0x8A150002 | -1978335230 | APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS | Invalid command line arguments |
| 0x8A150003 | -1978335229 | APPINSTALLER_CLI_ERROR_COMMAND_FAILED | Executing command failed |
| 0x8A150004 | -1978335228 | APPINSTALLER_CLI_ERROR_MANIFEST_FAILED | Opening manifest failed |
| 0x8A150005 | -1978335227 | APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED | Cancellation signal received |
| 0x8A150006 | -1978335226 | APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED | Running ShellExecute failed |
| 0x8A150007 | -1978335225 | APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION | Cannot process manifest. The manifest version is higher than supported. Please update the client. |
| 0x8A150008 | -1978335224 | APPINSTALLER_CLI_ERROR_DOWNLOAD_FAILED | Downloading installer failed |
| 0x8A150009 | -1978335223 | APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX | Cannot write to index; it is a higher schema version |
| 0x8A15000A | -1978335222 | APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED | The index is corrupt |
| 0x8A15000B | -1978335221 | APPINSTALLER_CLI_ERROR_SOURCES_INVALID | The configured source information is corrupt |
| 0x8A15000C | -1978335220 | APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS | The source name is already configured |
| 0x8A15000D | -1978335219 | APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE | The source type is invalid |
| 0x8A15000E | -1978335218 | APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE | The MSIX file is a bundle, not a package |
| 0x8A15000F | -1978335217 | APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING | Data required by the source is missing |
| 0x8A150010 | -1978335216 | APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER | None of the installers are applicable for the current system |
| 0x8A150011 | -1978335215 | APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH | The installer file's hash does not match the manifest |
| 0x8A150012 | -1978335214 | APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST | The source name does not exist |
| 0x8A150013 | -1978335213 | APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS | The source location is already configured under another name |
| 0x8A150014 | -1978335212 | APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND | No packages found |
| 0x8A150015 | -1978335211 | APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED | No sources are configured |
| 0x8A150016 | -1978335210 | APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND | Multiple packages found matching the criteria |
| 0x8A150017 | -1978335209 | APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND | No manifest found matching the criteria |
| 0x8A150018 | -1978335208 | APPINSTALLER_CLI_ERROR_EXTENSION_PUBLIC_FAILED | Failed to get Public folder from source package |
| 0x8A150019 | -1978335207 | APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN | Command requires administrator privileges to run |
| 0x8A15001A | -1978335206 | APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE | The source location is not secure |
| 0x8A15001B | -1978335205 | APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY | The Microsoft Store client is blocked by policy |
| 0x8A15001C | -1978335204 | APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY | The Microsoft Store app is blocked by policy |
| 0x8A15001D | -1978335203 | APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED | The feature is currently under development. It can be enabled using winget settings. |
| 0x8A15001E | -1978335202 | APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED | Failed to install the Microsoft Store app |
| 0x8A15001F | -1978335201 | APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD | Failed to perform auto complete |
| 0x8A150020 | -1978335200 | APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED | Failed to initialize YAML parser |
| 0x8A150021 | -1978335199 | APPINSTALLER_CLI_ERROR_YAML_INVALID_MAPPING_KEY | Encountered an invalid YAML key |
| 0x8A150022 | -1978335198 | APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY | Encountered a duplicate YAML key |
| 0x8A150023 | -1978335197 | APPINSTALLER_CLI_ERROR_YAML_INVALID_OPERATION | Invalid YAML operation |
| 0x8A150024 | -1978335196 | APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED | Failed to build YAML doc |
| 0x8A150025 | -1978335195 | APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE | Invalid YAML emitter state |
| 0x8A150026 | -1978335194 | APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA | Invalid YAML data |
| 0x8A150027 | -1978335193 | APPINSTALLER_CLI_ERROR_LIBYAML_ERROR | LibYAML error |
| 0x8A150028 | -1978335192 | APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_WARNING | Manifest validation succeeded with warning |
| 0x8A150029 | -1978335191 | APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE | Manifest validation failed |
| 0x8A15002A | -1978335190 | APPINSTALLER_CLI_ERROR_INVALID_MANIFEST | Manifest is invalid |
| 0x8A15002B | -1978335189 | APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE | No applicable update found |
| 0x8A15002C | -1978335188 | APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE | winget upgrade --all completed with failures |
| 0x8A15002D | -1978335187 | APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED | Installer failed security check |
| 0x8A15002E | -1978335186 | APPINSTALLER_CLI_ERROR_DOWNLOAD_SIZE_MISMATCH | Download size does not match expected content length |
| 0x8A15002F | -1978335185 | APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND | Uninstall command not found |
| 0x8A150030 | -1978335184 | APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED | Running uninstall command failed |
| 0x8A150031 | -1978335183 | APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR | ICU break iterator error |
| 0x8A150032 | -1978335182 | APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR | ICU casemap error |
| 0x8A150033 | -1978335181 | APPINSTALLER_CLI_ERROR_ICU_REGEX_ERROR | ICU regex error |
| 0x8A150034 | -1978335180 | APPINSTALLER_CLI_ERROR_IMPORT_INSTALL_FAILED | Failed to install one or more imported packages |
| 0x8A150035 | -1978335179 | APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND | Could not find one or more requested packages |
| 0x8A150036 | -1978335178 | APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE | Json file is invalid |
| 0x8A150037 | -1978335177 | APPINSTALLER_CLI_ERROR_SOURCE_NOT_REMOTE | The source location is not remote |
| 0x8A150038 | -1978335176 | APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE | The configured rest source is not supported |
| 0x8A150039 | -1978335175 | APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA | Invalid data returned by rest source |
| 0x8A15003A | -1978335174 | APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY | Operation is blocked by Group Policy |
| 0x8A15003B | -1978335173 | APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR | Rest API internal error |
| 0x8A15003C | -1978335172 | APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL | Invalid rest source url |
| 0x8A15003D | -1978335171 | APPINSTALLER_CLI_ERROR_RESTAPI_UNSUPPORTED_MIME_TYPE | Unsupported MIME type returned by rest API |
| 0x8A15003E | -1978335170 | APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION | Invalid rest source contract version |
| 0x8A15003F | -1978335169 | APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE | The source data is corrupted or tampered |
| 0x8A150040 | -1978335168 | APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE | Error reading from the stream |
| 0x8A150041 | -1978335167 | APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED | Package agreements were not agreed to |
| 0x8A150042 | -1978335166 | APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR | Error reading input in prompt |
| 0x8A150043 | -1978335165 | APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST | The search request is not supported by one or more sources |
| 0x8A150044 | -1978335164 | APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND | The rest API endpoint is not found. |
| 0x8A150045 | -1978335163 | APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED | Failed to open the source. |
| 0x8A150046 | -1978335162 | APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED | Source agreements were not agreed to |
| 0x8A150047 | -1978335161 | APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH | Header size exceeds the allowable limit of 1024 characters. Please reduce the size and try again. |
| 0x8A150048 | -1978335160 | APPINSTALLER_CLI_ERROR_MISSING_RESOURCE_FILE | Missing resource file |
| 0x8A150049 | -1978335159 | APPINSTALLER_CLI_ERROR_MSI_INSTALL_FAILED | Running MSI install failed |
| 0x8A15004A | -1978335158 | APPINSTALLER_CLI_ERROR_INVALID_MSIEXEC_ARGUMENT | Arguments for msiexec are invalid |
| 0x8A15004B | -1978335157 | APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES | Failed to open one or more sources |
| 0x8A15004C | -1978335156 | APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED | Failed to validate dependencies |
| 0x8A15004D | -1978335155 | APPINSTALLER_CLI_ERROR_MISSING_PACKAGE | One or more package is missing |
| 0x8A15004E | -1978335154 | APPINSTALLER_CLI_ERROR_INVALID_TABLE_COLUMN | Invalid table column |
| 0x8A15004F | -1978335153 | APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_NOT_NEWER | The upgrade version is not newer than the installed version |
| 0x8A150050 | -1978335152 | APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_UNKNOWN | Upgrade version is unknown and override is not specified |
| 0x8A150051 | -1978335151 | APPINSTALLER_CLI_ERROR_ICU_CONVERSION_ERROR | ICU conversion error |
| 0x8A150052 | -1978335150 | APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED | Failed to install portable package |
| 0x8A150053 | -1978335149 | APPINSTALLER_CLI_ERROR_PORTABLE_REPARSE_POINT_NOT_SUPPORTED | Volume does not support reparse points. |
| 0x8A150054 | -1978335148 | APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS | Portable package from a different source already exists. |
| 0x8A150055 | -1978335147 | APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY | Unable to create symlink, path points to a directory. |
| 0x8A150056 | -1978335146 | APPINSTALLER_CLI_ERROR_INSTALLER_PROHIBITS_ELEVATION | The installer cannot be run from an administrator context. |
| 0x8A150057 | -1978335145 | APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED | Failed to uninstall portable package |
| 0x8A150058 | -1978335144 | APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED | Failed to validate DisplayVersion values against index. |
| 0x8A150059 | -1978335143 | APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT | One or more arguments are not supported. |
| 0x8A15005A | -1978335142 | APPINSTALLER_CLI_ERROR_BIND_WITH_EMBEDDED_NULL | Embedded null characters are disallowed for SQLite |
| 0x8A15005B | -1978335141 | APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND | Failed to find the nested installer in the archive. |
| 0x8A15005C | -1978335140 | APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED | Failed to extract archive. |
| 0x8A15005D | -1978335139 | APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_INVALID_PATH | Invalid relative file path to nested installer provided. |
| 0x8A15005E | -1978335138 | APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH | The server certificate did not match any of the expected values. |
| 0x8A15005F | -1978335137 | APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED | Install location must be provided. |
| 0x8A150060 | -1978335136 | APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED | Archive malware scan failed. |
| 0x8A150061 | -1978335135 | APPINSTALLER_CLI_ERROR_PACKAGE_ALREADY_INSTALLED | Found at least one version of the package installed. |
| 0x8A150062 | -1978335134 | APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS | A pin already exists for the package. |
| 0x8A150063 | -1978335133 | APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST | There is no pin for the package. |
| 0x8A150064 | -1978335132 | APPINSTALLER_CLI_ERROR_CANNOT_OPEN_PINNING_INDEX | Unable to open the pin database. |
| 0x8A150065 | -1978335131 | APPINSTALLER_CLI_ERROR_MULTIPLE_INSTALL_FAILED | One or more applications failed to install |
| 0x8A150066 | -1978335130 | APPINSTALLER_CLI_ERROR_MULTIPLE_UNINSTALL_FAILED | One or more applications failed to uninstall |
| 0x8A150067 | -1978335129 | APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE | One or more queries did not return exactly one match |
| 0x8A150068 | -1978335128 | APPINSTALLER_CLI_ERROR_PACKAGE_IS_PINNED | The package has a pin that prevents upgrade. |
| 0x8A150069 | -1978335127 | APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB | The package currently installed is the stub package |
| 0x8A15006A | -1978335126 | APPINSTALLER_CLI_ERROR_APPTERMINATION_RECEIVED | Application shutdown signal received |
| 0x8A15006B | -1978335125 | APPINSTALLER_CLI_ERROR_DOWNLOAD_DEPENDENCIES | Failed to download package dependencies. |
| 0x8A15006C | -1978335124 | APPINSTALLER_CLI_ERROR_DOWNLOAD_COMMAND_PROHIBITED | Failed to download package. Download for offline installation is prohibited. |
| 0x8A15006D | -1978335123 | APPINSTALLER_CLI_ERROR_SERVICE_UNAVAILABLE | A required service is busy or unavailable. Try again later. |
| 0x8A15006E | -1978335122 | APPINSTALLER_CLI_ERROR_RESUME_ID_NOT_FOUND | The guid provided does not correspond to a valid resume state. |
| 0x8A15006F | -1978335121 | APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH | The current client version did not match the client version of the saved state. |
| 0x8A150070 | -1978335120 | APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE | The resume state data is invalid. |
| 0x8A150071 | -1978335119 | APPINSTALLER_CLI_ERROR_CANNOT_OPEN_CHECKPOINT_INDEX | Unable to open the checkpoint database. |
| 0x8A150072 | -1978335118 | APPINSTALLER_CLI_ERROR_RESUME_LIMIT_EXCEEDED | Exceeded max resume limit. |
| 0x8A150073 | -1978335117 | APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO | Invalid authentication info. |
| 0x8A150074 | -1978335116 | APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED | Authentication method not supported. |
| 0x8A150075 | -1978335115 | APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED | Authentication failed. |
| 0x8A150076 | -1978335114 | APPINSTALLER_CLI_ERROR_AUTHENTICATION_INTERACTIVE_REQUIRED | Authentication failed. Interactive authentication required. |
| 0x8A150077 | -1978335113 | APPINSTALLER_CLI_ERROR_AUTHENTICATION_CANCELLED_BY_USER | Authentication failed. User cancelled. |
| 0x8A150078 | -1978335112 | APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT | Authentication failed. Authenticated account is not the desired account. |
| 0x8A150079 | -1978335111 | APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND | Repair command not found. |
| 0x8A15007A | -1978335110 | APPINSTALLER_CLI_ERROR_REPAIR_NOT_APPLICABLE | Repair operation is not applicable. |
| 0x8A15007B | -1978335109 | APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED | Repair operation failed. |
| 0x8A15007C | -1978335108 | APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED | The installer technology in use doesn't support repair. |
| 0x8A15007D | -1978335107 | APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_REPAIR_PROHIBITED | Repair operations involving administrator privileges are not permitted on packages installed within the user scope. |
| 0x8A15007E | -1978335106 | APPINSTALLER_CLI_ERROR_SQLITE_CONNECTION_TERMINATED | The SQLite connection was terminated to prevent corruption. |
| 0x8A15007F | -1978335105 | APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED | Failed to get Microsoft Store package catalog. |
| 0x8A150080 | -1978335104 | APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE | No applicable Microsoft Store package found from Microsoft Store package catalog. |
| 0x8A150081 | -1978335103 | APPINSTALLER_CLI_ERROR_SFSCLIENT_API_FAILED | Failed to get Microsoft Store package download information. |
| 0x8A150082 | -1978335102 | APPINSTALLER_CLI_ERROR_NO_APPLICABLE_SFSCLIENT_PACKAGE | No applicable Microsoft Store package download information found. |
| 0x8A150083 | -1978335101 | APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED | Failed to retrieve Microsoft Store package license. |
| 0x8A150084 | -1978335100 | APPINSTALLER_CLI_ERROR_SFSCLIENT_PACKAGE_NOT_SUPPORTED | The Microsoft Store package does not support download command. |
| 0x8A150085 | -1978335099 | APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED_FORBIDDEN | Failed to retrieve Microsoft Store package license. The Microsoft Entra Id account does not have required privilege. |
| 0x8A150086 | -1978335098 | APPINSTALLER_CLI_ERROR_INSTALLER_ZERO_BYTE_FILE | Downloaded zero byte installer; ensure that your network connection is working properly. |

## Install errors.

| Hex | Decimal | Symbol | Description |
|-------------|-------------|-------------|-------------|
| 0x8A150101 | -1978334975 | APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE | Application is currently running. Exit the application then try again. |
| 0x8A150102 | -1978334974 | APPINSTALLER_CLI_ERROR_INSTALL_INSTALL_IN_PROGRESS | Another installation is already in progress. Try again later. |
| 0x8A150103 | -1978334973 | APPINSTALLER_CLI_ERROR_INSTALL_FILE_IN_USE | One or more file is being used. Exit the application then try again. |
| 0x8A150104 | -1978334972 | APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY | This package has a dependency missing from your system. |
| 0x8A150105 | -1978334971 | APPINSTALLER_CLI_ERROR_INSTALL_DISK_FULL | There's no more space on your PC. Make space, then try again. |
| 0x8A150106 | -1978334970 | APPINSTALLER_CLI_ERROR_INSTALL_INSUFFICIENT_MEMORY | There's not enough memory available to install. Close other applications then try again. |
| 0x8A150107 | -1978334969 | APPINSTALLER_CLI_ERROR_INSTALL_NO_NETWORK | This application requires internet connectivity. Connect to a network then try again. |
| 0x8A150108 | -1978334968 | APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT | This application encountered an error during installation. Contact support. |
| 0x8A150109 | -1978334967 | APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH | Restart your PC to finish installation. |
| 0x8A15010A | -1978334966 | APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL | Installation failed. Restart your PC then try again. |
| 0x8A15010B | -1978334965 | APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_INITIATED | Your PC will restart to finish installation. |
| 0x8A15010C | -1978334964 | APPINSTALLER_CLI_ERROR_INSTALL_CANCELLED_BY_USER | You cancelled the installation. |
| 0x8A15010D | -1978334963 | APPINSTALLER_CLI_ERROR_INSTALL_ALREADY_INSTALLED | Another version of this application is already installed. |
| 0x8A15010E | -1978334962 | APPINSTALLER_CLI_ERROR_INSTALL_DOWNGRADE | A higher version of this application is already installed. |
| 0x8A15010F | -1978334961 | APPINSTALLER_CLI_ERROR_INSTALL_BLOCKED_BY_POLICY | Organization policies are preventing installation. Contact your admin. |
| 0x8A150110 | -1978334960 | APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES | Failed to install package dependencies. |
| 0x8A150111 | -1978334959 | APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE_BY_APPLICATION | Application is currently in use by another application. |
| 0x8A150112 | -1978334958 | APPINSTALLER_CLI_ERROR_INSTALL_INVALID_PARAMETER | Invalid parameter. |
| 0x8A150113 | -1978334957 | APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED | Package not supported by the system. |
| 0x8A150114 | -1978334956 | APPINSTALLER_CLI_ERROR_INSTALL_UPGRADE_NOT_SUPPORTED | The installer does not support upgrading an existing package. |
| 0x8A150115 | -1978334955 | APPINSTALLER_CLI_ERROR_INSTALL_CUSTOM_ERROR | Installation failed with installer custom error. |

## Check for package installed status

| Hex | Decimal | Symbol | Description |
|-------------|-------------|-------------|-------------|
| 0x8A150201 | -1978334719 | WINGET_INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND | The Apps and Features Entry for the package could not be found. |
| 0x8A150202 | -1978334718 | WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE | The install location is not applicable. |
| 0x8A150203 | -1978334717 | WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND | The install location could not be found. |
| 0x8A150204 | -1978334716 | WINGET_INSTALLED_STATUS_FILE_HASH_MISMATCH | The hash of the existing file did not match. |
| 0x8A150205 | -1978334715 | WINGET_INSTALLED_STATUS_FILE_NOT_FOUND | File not found. |
| 0x8A150206 | -1978334714 | WINGET_INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK | The file was found but the hash was not checked. |
| 0x8A150207 | -1978334713 | WINGET_INSTALLED_STATUS_FILE_ACCESS_ERROR | The file could not be accessed. |

## Configuration Errors

| Hex | Decimal | Symbol | Description |
|-------------|-------------|-------------|-------------|
| 0x8A15C001 | -1978286079 | WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE | The configuration file is invalid. |
| 0x8A15C002 | -1978286078 | WINGET_CONFIG_ERROR_INVALID_YAML | The YAML syntax is invalid. |
| 0x8A15C003 | -1978286077 | WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE | A configuration field has an invalid type. |
| 0x8A15C004 | -1978286076 | WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION | The configuration has an unknown version. |
| 0x8A15C005 | -1978286075 | WINGET_CONFIG_ERROR_SET_APPLY_FAILED | An error occurred while applying the configuration. |
| 0x8A15C006 | -1978286074 | WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER | The configuration contains a duplicate identifier. |
| 0x8A15C007 | -1978286073 | WINGET_CONFIG_ERROR_MISSING_DEPENDENCY | The configuration is missing a dependency. |
| 0x8A15C008 | -1978286072 | WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED | The configuration has an unsatisfied dependency. |
| 0x8A15C009 | -1978286071 | WINGET_CONFIG_ERROR_ASSERTION_FAILED | An assertion for the configuration unit failed. |
| 0x8A15C00A | -1978286070 | WINGET_CONFIG_ERROR_MANUALLY_SKIPPED | The configuration was manually skipped. |
| 0x8A15C00B | -1978286069 | WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED | A warning was thrown and the user declined to continue execution. |
| 0x8A15C00C | -1978286068 | WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE | The dependency graph contains a cycle which cannot be resolved. |
| 0x8A15C00D | -1978286067 | WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE | The configuration has an invalid field value. |
| 0x8A15C00E | -1978286066 | WINGET_CONFIG_ERROR_MISSING_FIELD | The configuration is missing a field. |
| 0x8A15C00F | -1978286065 | WINGET_CONFIG_ERROR_TEST_FAILED | Some of the configuration units failed while testing their state. |
| 0x8A15C010 | -1978286064 | WINGET_CONFIG_ERROR_TEST_NOT_RUN | Configuration state was not tested. |
| 0x8A15C011 | -1978286063 | WINGET_CONFIG_ERROR_GET_FAILED | The configuration unit failed getting its properties. |
| 0x8A15C012 | -1978286062 | WINGET_CONFIG_ERROR_HISTORY_ITEM_NOT_FOUND | The specified configuration could not be found. |
| 0x8A15C013 | -1978286061 | WINGET_CONFIG_ERROR_PARAMETER_INTEGRITY_BOUNDARY | Parameter cannot be passed across integrity boundary. |

## Configuration Processor Errors

| Hex | Decimal | Symbol | Description |
|-------------|-------------|-------------|-------------|
| 0x8A15C101 | -1978285823 | WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED | The configuration unit was not installed. |
| 0x8A15C102 | -1978285822 | WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY | The configuration unit could not be found. |
| 0x8A15C103 | -1978285821 | WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES | Multiple matches were found for the configuration unit specify the module to select the correct one. |
| 0x8A15C104 | -1978285820 | WINGET_CONFIG_ERROR_UNIT_INVOKE_GET | The configuration unit failed while attempting to get the current system state. |
| 0x8A15C105 | -1978285819 | WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST | The configuration unit failed while attempting to test the current system state. |
| 0x8A15C106 | -1978285818 | WINGET_CONFIG_ERROR_UNIT_INVOKE_SET | The configuration unit failed while attempting to apply the desired state. |
| 0x8A15C107 | -1978285817 | WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT | The module for the configuration unit is available in multiple locations with the same version. |
| 0x8A15C108 | -1978285816 | WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE | Loading the module for the configuration unit failed. |
| 0x8A15C109 | -1978285815 | WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT | The configuration unit returned an unexpected result during execution. |
| 0x8A15C110 | -1978285814 | WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT | A unit contains a setting that requires the config root. |
| 0x8A15C111 | -1978285813 | WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE_ADMIN | Loading the module for the configuration unit failed because it requires administrator privileges to run. |
| 0x8A15C112 | -1978285812 | WINGET_CONFIG_ERROR_NOT_SUPPORTED_BY_PROCESSOR | Operation is not supported by the configuration processor. |
