---
title: Exit Codes
description: WinGet return codes and their meanings
ms.date: 05/02/2023
ms.topic: article
ms.localizationpriority: medium
---

# Return Codes

## General Errors

| Hex | Decimal | Description |
|-------------|-------------|-------------| 
| 0x8A150001 | -1978335231 | Internal Error
| 0x8A150002 | -1978335230 | Invalid command line arguments
| 0x8A150003 | -1978335229 | Executing command failed
| 0x8A150004 | -1978335228 | Opening manifest failed
| 0x8A150005 | -1978335227 | Cancellation signal received
| 0x8A150006 | -1978335226 | Running ShellExecute failed
| 0x8A150007 | -1978335225 | Cannot process manifest. The manifest version is higher than supported. Please update the client.
| 0x8A150008 | -1978335224 | Downloading installer failed
| 0x8A150009 | -1978335223 | Cannot write to index | it is a higher schema version
| 0x8A15000A | -1978335222 | The index is corrupt
| 0x8A15000B | -1978335221 | The configured source information is corrupt
| 0x8A15000C | -1978335220 | The source name is already configured
| 0x8A15000D | -1978335219 | The source type is invalid
| 0x8A15000E | -1978335218 | The MSIX file is a bundle, not a package
| 0x8A15000F | -1978335217 | Data required by the source is missing
| 0x8A150010 | -1978335216 | None of the installers are applicable for the current system
| 0x8A150011 | -1978335215 | The installer file's hash does not match the manifest
| 0x8A150012 | -1978335214 | The source name does not exist
| 0x8A150013 | -1978335213 | The source location is already configured under another name
| 0x8A150014 | -1978335212 | No packages found
| 0x8A150015 | -1978335211 | No sources are configured
| 0x8A150016 | -1978335210 | Multiple packages found matching the criteria
| 0x8A150017 | -1978335209 | No manifest found matching the criteria
| 0x8A150018 | -1978335208 | Failed to get Public folder from source package
| 0x8A150019 | -1978335207 | Command requires administrator privileges to run
| 0x8A15001A | -1978335206 | The source location is not secure
| 0x8A15001B | -1978335205 | The Microsoft Store client is blocked by policy
| 0x8A15001C | -1978335204 | The Microsoft Store app is blocked by policy
| 0x8A15001D | -1978335203 | The feature is currently under development. It can be enabled using winget settings.
| 0x8A15001E | -1978335202 | Failed to install the Microsoft Store app
| 0x8A15001F | -1978335201 | Failed to perform auto complete
| 0x8A150020 | -1978335200 | Failed to initialize YAML parser
| 0x8A150021 | -1978335199 | Encountered an invalid YAML key
| 0x8A150022 | -1978335198 | Encountered a duplicate YAML key
| 0x8A150023 | -1978335197 | Invalid YAML operation
| 0x8A150024 | -1978335196 | Failed to build YAML doc
| 0x8A150025 | -1978335195 | Invalid YAML emitter state
| 0x8A150026 | -1978335194 | Invalid YAML data
| 0x8A150027 | -1978335193 | LibYAML error
| 0x8A150028 | -1978335192 | Manifest validation succeeded with warning
| 0x8A150029 | -1978335191 | Manifest validation failed
| 0x8A15002A | -1978335190 | Manifest is invalid
| 0x8A15002B | -1978335189 | No applicable update found
| 0x8A15002C | -1978335188 | winget upgrade --all completed with failures
| 0x8A15002D | -1978335187 | Installer failed security check
| 0x8A15002E | -1978335186 | Download size does not match expected content length
| 0x8A15002F | -1978335185 | Uninstall command not found
| 0x8A150030 | -1978335184 | Running uninstall command failed
| 0x8A150031 | -1978335183 | ICU break iterator error
| 0x8A150032 | -1978335182 | ICU casemap error
| 0x8A150033 | -1978335181 | ICU regex error
| 0x8A150034 | -1978335180 | Failed to install one or more imported packages
| 0x8A150035 | -1978335179 | Could not find one or more requested packages
| 0x8A150036 | -1978335178 | Json file is invalid
| 0x8A150037 | -1978335177 | The source location is not remote
| 0x8A150038 | -1978335176 | The configured rest source is not supported
| 0x8A150039 | -1978335175 | Invalid data returned by rest source
| 0x8A15003A | -1978335174 | Operation is blocked by Group Policy
| 0x8A15003B | -1978335173 | Rest source internal error
| 0x8A15003C | -1978335172 | Invalid rest source url
| 0x8A15003D | -1978335171 | Unsupported MIME type returned by rest source
| 0x8A15003E | -1978335170 | Invalid rest source contract version
| 0x8A15003F | -1978335169 | The source data is corrupted or tampered
| 0x8A150040 | -1978335168 | Error reading from the stream
| 0x8A150041 | -1978335167 | Package agreements were not agreed to
| 0x8A150042 | -1978335166 | Error reading input in prompt
| 0x8A150043 | -1978335165 | The search request is not supported by one or more sources
| 0x8A150044 | -1978335164 | The rest source endpoint is not found.
| 0x8A150045 | -1978335163 | Failed to open the source.
| 0x8A150046 | -1978335162 | Source agreements were not agreed to
| 0x8A150047 | -1978335161 | Header size exceeds the allowable limit of 1024 characters. Please reduce the size and try again.
| 0x8A150048 | -1978335160 | Missing resource file
| 0x8A150049 | -1978335159 | Running MSI install failed
| 0x8A15004A | -1978335158 | Arguments for msiexec are invalid
| 0x8A15004B | -1978335157 | Failed to open one or more sources
| 0x8A15004C | -1978335156 | Failed to validate dependencies
| 0x8A15004D | -1978335155 | One or more package is missing
| 0x8A15004E | -1978335154 | Invalid table column
| 0x8A15004F | -1978335153 | The upgrade version is not newer than the installed version
| 0x8A150050 | -1978335152 | Upgrade version is unknown and override is not specified
| 0x8A150051 | -1978335151 | ICU conversion error
| 0x8A150052 | -1978335150 | Failed to install portable package
| 0x8A150053 | -1978335149 | Volume does not support reparse points.
| 0x8A150054 | -1978335148 | Portable package from a different source already exists.
| 0x8A150055 | -1978335147 | Unable to create symlink, path points to a directory.
| 0x8A150056 | -1978335146 | The installer cannot be run from an administrator context.
| 0x8A150057 | -1978335145 | Failed to uninstall portable package
| 0x8A150058 | -1978335144 | Failed to validate DisplayVersion values against index.
| 0x8A150059 | -1978335143 | One or more arguments are not supported.
| 0x8A15005A | -1978335142 | Embedded null characters are disallowed for SQLite
| 0x8A15005B | -1978335141 | Failed to find the nested installer in the archive.
| 0x8A15005C | -1978335140 | Failed to extract archive.
| 0x8A15005D | -1978335139 | Invalid relative file path to nested installer provided.
| 0x8A15005E | -1978335138 | The server certificate did not match any of the expected values.
| 0x8A15005F | -1978335137 | Install location must be provided.
| 0x8A150060 | -1978335136 | Archive malware scan failed.
| 0x8A150061 | -1978335135 | Found at least one version of the package installed.
| 0x8A150062 | -1978335134 | A pin already exists for the package.
| 0x8A150063 | -1978335133 | There is no pin for the package.
| 0x8A150064 | -1978335132 | Unable to open the pin database.
| 0x8A150065 | -1978335131 | One or more applications failed to install
| 0x8A150066 | -1978335130 | One or more applications failed to uninstall
| 0x8A150067 | -1978335129 | One or more queries did not return exactly one match
| 0x8A150068 | -1978335128 | The package has a pin that prevents upgrade.

## Install errors.

| Hex | Decimal | Description |
|-------------|-------------|-------------| 
| 0x8A150068 | -1978335128 | Application is currently running. Exit the application then try again.
| 0x8A150069 | -1978335127 | Another installation is already in progress. Try again later.
| 0x8A150070 | -1978335120 | One or more file is being used. Exit the application then try again.
| 0x8A150071 | -1978335119 | This package has a dependency missing from your system.
| 0x8A150072 | -1978335118 | There's no more space on your PC. Make space, then try again.
| 0x8A150073 | -1978335117 | There's not enough memory available to install. Close other applications then try again.
| 0x8A150074 | -1978335116 | This application requires internet connectivity.Connect to a network then try again.
| 0x8A150075 | -1978335115 | This application encountered an error during installation.Contact support.
| 0x8A150076 | -1978335114 | Restart your PC to finish installation.
| 0x8A150077 | -1978335113 | Your PC will restart to finish installation.
| 0x8A150078 | -1978335112 | Installation failed. Restart your PC then try again.
| 0x8A150079 | -1978335111 | You cancelled the installation.
| 0x8A150080 | -1978335104 | Another version of this application is already installed.
| 0x8A150081 | -1978335103 | A higher version of this application is already installed.
| 0x8A150082 | -1978335102 | Organization policies are preventing installation. Contact your admin.
| 0x8A150083 | -1978335101 | Failed to install package dependencies.
| 0x8A150084 | -1978335100 | Application is currently in use by another application.
| 0x8A150085 | -1978335099 | Invalid parameter.
| 0x8A150086 | -1978335098 | Package not supported by the system.

## Check for package installed status

| Hex | Decimal | Description |
|-------------|-------------|-------------| 
| 0x8A150086 | -1978335098 | The Add & Remove Programs Entry for the package could not be found.
| 0x8A150086 | -1978335098 | The install location is not applicable.
| 0x8A150087 | -1978335097 | The install location could not be found.
| 0x8A150086 | -1978335098 | The hash of the existing file did not match. Please uninstall and try again.
| 0x8A150087 | -1978335097 | File not found.
| 0x8A150088 | -1978335096 | The file was found but the hash was not checked.
| 0x8A150089 | -1978335095 | The file could not be accessed.

## Configuration Errors

| Hex | Decimal | Description |
|-------------|-------------|-------------| 
| 0x8A150086 | -1978335098 | configuration file is invalid.
| 0x8A150087 | -1978335097 | The YAML syntax is invalid.
| 0x8A150088 | -1978335096 | A configuration field has an invalid type.
| 0x8A150089 | -1978335095 | The configuration has an unknown version.
| 0x8A150090 | -1978335088 | An error occured while applying the configuration.
| 0x8A150091 | -1978335087 | The configuration contains a duplicate identifier.
| 0x8A150092 | -1978335086 | The configuration is missing a dependency.
| 0x8A150093 | -1978335085 | The configuration has an unsatisfied dependency.
| 0x8A150094 | -1978335084 | An assertion for the configuration unit failed.
| 0x8A150095 | -1978335083 | The configuration was manually skipped.
| 0x8A150096 | -1978335082 | A warning was thrown and the user declined to continue execution.
| 0x8A150097 | -1978335081 | The dependency graph contains a cycle which cannot be resolved.
| 0x8A150098 | -1978335080 | The configuration has an invalid field value.
| 0x8A150099 | -1978335079 | The configuration is missing a field.

## Configuration Processor Errors

| Hex | Decimal | Description |
|-------------|-------------|-------------| 
| 0x8A150099 | -1978335079 | The configuration unit was not in the module as expected.
| 0x8A150100 | -1978334976 | The configuration unit could not be found.
| 0x8A150101 | -1978334975 | Multiple matches were found for the configuration unit specify the module to select the correct one.
| 0x8A150102 | -1978334974 | The configuration unit failed while attempting to get the current system state.
| 0x8A150103 | -1978334973 | The configuration unit failed while attempting to test the current system state.
| 0x8A150104 | -1978334972 | The configuration unit failed while attempting to apply the desired state.
| 0x8A150105 | -1978334971 | The module for the configuration unit is available in multiple locations with the same version.
| 0x8A150106 | -1978334970 | Loading the module for the configuration unit failed.
| 0x8A150107 | -1978334969 | The configuration unit returned an unexpected result during execution.