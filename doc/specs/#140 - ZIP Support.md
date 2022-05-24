---
author: Ryan Fu @ryfu-msft
created on: 2022-05-24
last updated: 2022-05-24
issue id: 140
---

# Support ZIP/Archive installerTypes

For [#140](https://github.com/microsoft/winget-cli/issues/140)

## Abstract
This spec outlines the design for supporting the installation of packages using archive files. The initial implementation will aim to support only ZIP files. This spec will be updated if support for other archive types such as .tar.gz or .cab files are added.

## Design Flow/Implementation

Because ZIP archive files are essentially a compressed directory containing the relevant installer files, the install flow will need to add the following steps:

1. Perform checks on the downloaded ZIP archive for potential malware or threats.
2. Extract the contents of the ZIP archive to a temporary location.
3. Execute the appropriate install flow using the extracted installer based on the installer type derived from the manifest.

>NOTE: The initial implementation will first support installing ZIPs that contain only basic installers (msix, msi, or exe), then portable apps.

## Manifest Changes:
Addition of `NestedInstallerType`:
- Enumeration of supported nested installerTypes contained inside an archive file.

Addition of `NestedInstallerFile`
- Object containing metadata about a nested installer file contained inside an archive. 
- Properties:
    - `RelativeFilePath`: The relative path to a nested installer file contained inside an archive.
    - `PortableCommandAlias`: The command alias to be used for calling the package. Only applies to a nested portable package.

These changes would be added to the `Installer` object with `NestedInstallerFiles` representing an array of `NestedInstallerFile`. 

### Manifest Validation:
- Exactly one `NestedInstallerFile` entry must be specified for a non-portable nested installer type.
- Multiple `NestedInstallerFile` entries can be specified for portable nested installer type to support a suite of portable exes that would all be installed under the same package.
- If a ZIP `InstallerType` is specified, a `NestedInstallerType` is required to be specified.

## Supported Install Scenarios:
The initial implementation of this feature will only support the following installation scenarios:
- Single base installer (exe, msi, msix) contained inside an archive file. 
- Single or multiple portable exes bundled as a suite inside an archive file.

## ZIP Extraction
The extraction of ZIPs will be done using Windows Shell APIs. ZIP files can be represented as a [ShellFolder](https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishellfolder) object that can be used to manage the contents of the ZIP file. File contents are represented as [ShellItems](https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishellitem), which can be handled using the methods exposed by the [IFileOperation interface](https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifileoperation). If the `NestedInstallerType` is non-portable, the installer file will be copied to a temporary location and used during execution of the installation flow. If the `NestedInstallerType` is portable, all files will be moved to the appropriate install location and symlinks will be created for each `NestedInstallerFile` entry.

> Since we are utilizing Windows Shell APIs, we need to ensure that we are not invoking a UI during the extraction. This can be done by [setting the operation flag to not display any UI](https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileoperation-setoperationflags). 

## ZIP Threat Detection
ZIP and other archive file types are known threat vectors for malware in the form of ZIP bombs, which utilize layers of nested zip files which decompress into extremely large amounts of files. In order to protect our users and warn them before installing from any suspicious ZIP files, we will need to implement a basic archive file malware check within the client. 

The check will be as follows:
1. Recursively traverse through all files contained in an archive file.
2. If a nested ZIP file is detected, and the number of zip layers currently extracted is less than 3, extract the contents, increment the layer count and continue traversing. Otherwise, break out of the traversal and warn the user.

> ZIP headers are not always reliable when comparing compressed size vs uncompressed size as they can be modified externally. ZIP headers also differ from that of .cab or .tar.gz headers, therefore we should avoid taking a dependency on the data acquired from headers. 

## Supporting Multiple Portables
Currently, information regarding a single installed portable is stored in the ARP entry. In order to support installing a suite of portables contained inside an archive file, an additional table of information will need to be appended to the tracking catalog in order to support multiple installs under a single package. This table should replace most of the uninstall-related information that is stored in ARP for a given portable package.

The table should contain the following information for each portable-related file to be installed/copied from the archive file:

- InstallerType
- TargetPath
- SymlinkPath
- InstallDirectoryCreated (Flag)
- SHA256


