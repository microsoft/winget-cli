---
author: Ryan Fu @ryfu-msft
created on: 2022-05-24
last updated: 2022-12-13
issue id: 140
---

# Support ZIP/Archive InstallerTypes

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
The extraction of ZIPs will be done using Windows Shell APIs. ZIP files can be represented as a [ShellFolder](https://docs.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishellfolder) object that can be used to manage the contents of the ZIP file. File contents are represented as [ShellItems](https://docs.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishellitem), which can be handled using the methods exposed by the [IFileOperation interface](https://docs.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifileoperation).

In our initial implementation, we will only support extracting the top level archive to a temporary location. This means that the `NestedInstallerFile` must not be contained in any nested archives. The appropriate install flow will proceed based on the specified `NestedInstallerType`.

> If the community presents enough use cases that require decompressing additional layers, then we will consider extending this functionality and adding a separate manifest entry to override the default behavior of only unzipping the top level archive.

> Since we are utilizing Windows Shell APIs, we need to ensure that we are not invoking a UI during the extraction. This can be done by [setting the operation flag to not display any UI](https://docs.microsoft.com/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileoperation-setoperationflags).

> During implementation, we will also need to ensure that this process can work under SYSTEM context.

## ZIP Threat Detection
ZIP and other archive file types are known threat vectors for malware in the form of ZIP compression bombs. Two of the most common types of compression bombs are multi-layered (recursive) and single-layered (non-recursive). ZIP bombs rely on a repetition of identical files that have extremely large compression ratios. Some examples include the following:

1. **Multi-layered (recursive)**:
A ZIP file containing multiple layers of nested ZIP files that achieve high compression ratios when decompressed recursively. This technique is often used to bypass compression ratio checks performed by ZIP parsers for a single layer.

2. **Single-Layered (non-recursive)**
A ZIP bomb that expands fully after a single round of decompression. The extremely high compression ratio of the ZIP bomb is achieved by overlapping files within the ZIP container.

In order to protect our users from these possible threats, we will need to scan the ZIP file for malware using [Pure, a static analysis file format checker](https://github.com/ronomon/pure).

> Pure is licensed under the [MIT license](https://github.com/ronomon/pure/blob/master/LICENSE).

If the Pure library functions that are called on a given ZIP file detect malware, the process will terminate and a warning will be displayed to the user. The user can include `--ignore-local-archive-malware-scan` to bypass this check and continue installation at their own risk.


To minimize unnecessary costs on performance during installation, these checks will only be applied when any of the following conditions are met:
-  Hash mismatch was overridden
-  Untrusted source (i.e. installing from a local manifest, private source or third-party REST sources)

## Supporting Nested Portable(s) in an Archive
Currently, information regarding a single installed portable is stored in the ARP entry. In order to support installing a single or multiple portables contained inside an archive file, we will need to create a separate record that will be stored with the extracted files. This record will contain a table capturing the list of files that were created and placed down as well as various metadata to enable us to verify whether they have been modified. This table should replace most of the uninstall-related information that is stored in ARP for a given portable package.

The table will contain the following information for each item that we create or place down during installation.

| Metadata    | Description |
| ----------- | ----------- |
| Path      | Path to the item |
| Flag   | Flag to indicate what type of file was placed down (directory, symlink, exe) |
| Hash   | Hash for files with contents (exe) |
| SymlinkTarget | Target exe for the created symlink (only applies to symlink files) |

