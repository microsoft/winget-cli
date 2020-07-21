winget uses the YAML file to locate and install tools for Windows users.  This spec provides the supported tags and when introduced.  As well as best practices.

Table of Contents
----------------------------------
YAML Spec
   1) YAML Syntax
   2) Minimal YAML file example
   3) Best Practices
History


# YAML Spec

## YAML file name and folder structure
YAML files shall be added to the repository with the following folder structure:
publisher / application / application-version.yaml

* Publisher folder is the name of the company that publishes the tool.  For example: Microsoft.
* The child folder application is the name of the application or tool.  For example: VSCode.
* The filename must be a combination of the application name and the current version.  For example: VSCode-1.0.0.yaml.

The publisher and application folders MUST match the values used to define the Id.  See Id: in the YAML for more detail.
The version in the filename should match the version field value in the YAML file.  See Version: in the YAML for more detail.

## YAML Syntax
Each field in the file must be PascalCased and cannot be duplicated.

```YAML
# Id is a required field.  It MUST include the publisher name and application name separated by a period.
# The Id convention and folder convention MUST match.  Therefore all entries MUST look like this:
# ID: PublisherName.ApplicationName
# and the folder structure PublisherName\ApplicationName\ApplicationName-Version.YAML
# Restrictions: No white spaces allowed. [min: 4, max:255]
Id: microsoft.msixsdk

# Name is a required field.  It should be the friendly name of the application.
# Restrictions: [min: 1, max:128]
Name: MSIX SDK

# AppMoniker is the common name someone may use to search for the application.
# Restrictions:  No white spaces allowed. [min: 1, max:40]
AppMoniker: msixsdk

# Version is a required field.  It is the specific version of this copy of the application.
# Versions should be separated by a period, but we will support other delimiters.
# Versions should be limited to four fields: Major.Minor.Build.Update.
# Versions will be sorted as integers following the following pattern: Major.Minor.Build.Patch.
# Restrictions: 4 sections with max value of 65535.  For example:65535.65535.65535.65535
Version: 1.7.32

# Publisher is a required field.  It should be the legal company name.
# Restrictions: [min: 1, max:128]
Publisher: Microsoft

# Channel a string representing the flight ring.  For example: stable, beta, canary. 
# By default searches will not expose results with the channel specified.
# Restrictions: [min: 1, max:40]
Channel: Prerelease

# The person or company responsible for authoring the tool.
# Restrictions: [min: 1, max:40]
Author: Microsoft

# License is a required field.  License provides the type of license the application is provided under.  
# For example: BSD, MIT, Apache, Microsoft Public License, commercial
# Restrictions: [min: 1, max:40]
License: MIT License

# LicenseURL provides a link to the license for the user to read.  
# Restrictions: The LicenseUrl must be a valid secure URL, for example beginning with https and 
# followed by a hostname.  [min: 10, max:2000]
LicenseUrl: https://github.com/microsoft/msix-packaging/blob/master/LICENSE  

# MinOSVersion uses the Windows version to limit installations on unsupported platforms.  
# For example specifying 10.0.18362.0 will only allows this tool to be installed on Windows build 1903 or greater.
# Restrictions: must follow Windows versioning model. 4 sections with max value of 65535.  For example:65535.65535.65535.65535
MinOSVersion: 10.0.0.0

# Description should be friendly providing insights into the value of the tool.
# Restrictions: [min: 1, max:500]
Description:
  The MSIX SDK project is an effort to enable developers
  on a variety of platforms to pack and unpack packages
  for the purposes of distribution from either the Microsoft
  Store, or their own content distribution networks.

# Homepage is a URL where the user can find more information on the tool.
# Restrictions: The Homepage must be a valid secure URL, for example beginning with https and 
# followed by a hostname.  [min: 10, max:2000]
Homepage: https://github.com/microsoft/msix-packaging

# Tags are comma separated list.  They represent strings that the user may use to search for a given tool.
# Restrictions: [min: 1, max:40] 
Tags: "msix, appx"

# FileExtensions is a comma separated list.  FileExtensions provides the list of extensions the application could support.
# FileExtensions are not supported in this preview (5/24/2020)
# Restrictions: [min: 1, max:40] 
FileExtensions: "docx, doc"

# Protocols is a comma separated list.  Protocols provides the list of protocols the application provides a handler for.
# Protocols are not supported in this preview (5/24/2020)
# Restrictions: [min: 1, max:40] 
Protocols: "ms-winget"

# Commands are the common executable or alias that the user might type trying to run the application.
# For example "code" for VSCode.  If multiple commands are supported, the commands must be separated by a comma.
# Restrictions: [min: 1, max:40] 
Commands: "code"

# InstallerType is a required field.  Supported types are inno, wix, msi, nullsoft, zip, appx, msix and exe.
# The winget command tool uses this value to assist in installing this application.
# If the value is an exe, you will need to provide the quiet switches.
# zip is not supported in this preview (5/24/2020)  
# Restrictions: [min: 1, max:40] 
InstallerType: msix

# winget by default specifies silent or quiet mode to the installers.  The following additional
# switches can be used to change the install behavior if supported by the InstallerType.
# When scripting, custom switches may also be passed on the command line to winget.
# The following switches are supported: Custom, Silent, SilentWithProgress, Interactive, Language, Log and InstallLocation.

Switches:

# Custom switches will be passed directly to the installer by winget.
# Restrictions: [min: 1, max:128] 
  Custom: MyCustomString

# During any installation, only one of the next three switches [Silent, SilentWithProgress, Interactive] will be passed to the
# installer when provided by the user.

# Silent represents the value that should be passed to the installer when the user chooses a silent or quiet install.
# For example, some installers support "/s".
# Restrictions: [min: 1, max:40] 
  Silent: /s

# SilentWithProgress represents the value that should be passed to the installer when the user chooses to install a non-interactive install.
# For example, some installers support "passive".
  SilentWithProgress: passive

# Interactive represents the value that should be passed to the installer when the tool requires user interaction.  If the installer has a flag
# that when passed to the installer, causes it to require user input, it should be provided here.  This flag will be used when the user passes the
# --interactive switch to the installer.  
# Interactive is not supported in this preview (5/24/2020)
  Interactive: /ShowEula

# Some installers include all localized resources.  By specifying a Language switch, winget will pass the value of language to the installer,
# when the installer is called.
# Language is not supported in this preview (5/24/2020)
  Language: en-US

# Installers often write logging files.  A user may want to redirect the log to a different location.  In order to redirect the log file, the user will 
# pass in a log file path to the installer.  For example: --log "%temp%\mylog.txt".  The file will be saved as a token in the client: <LOGPATH>.  Therefore, 
# if the installer supports log redirection, then the Log switch should be the flag that the installer expects to provide the path to the log 
# file.  For example:  /LOG=<LOGPATH>.
# Log must include the <LOGPATH> token.
  Log: /LOG=<LOGPATH> 

# Some installers allow for installing to an alternate location.   A user may want to redirect the default install to a different location.  
# In order to redirect the install location, the user will pass in a installation path to the installer.  For example: --installlocation "c:\mytool". 
# The folder path will be saved as a token in the client: <INSTALLPATH>.  Therefore, if the installer supports install location redirection, 
# then the InstallLocation switch must be the flag that the installer expects to redirect the installation path.  
# For example:  /InstallLocation=<INSTALLPATH>.
# InstallLocation must include the <INSTALLPATH> token.
  InstallLocation: /DIR=<INSTALLPATH>

# ======================= Installers  ==================

# Installers represents the collection of entries that define the actual installer.  The installer provides the architecture, url and hash that 
# ensure that the installer has not been tampered with.
Installers:
    # Arch is a required field.  Arch is the architecture of the tool, and is a required field to ensure that the tool will install correctly.
    # Supported values: arm, arm64, x86, x64 and neutral

  - Arch: x86
    # Url is a required field.  This provides the path to the installer.
    # The Url must begin with https:// and followed by a hostname.  
    # Restrictions: [min: 10, max:2000]
    Url: https://contosa.net/publiccontainer/contosainstaller.exe

    # Sha256 is a required field. The value is the hash of the installer and used to verify the executable
    # Restrictions: [valid sha256 hash]
    Sha256: 69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82

    # SignatureSha256 is a recommended field for MSIX files only. The value is the signature file's hash of the MSIX file.  
    # By providing the SignatureSha256, you can improve the installation performance of the application.
    # The SignatureSha256 can be found by typing winget create hash -msix <MSIX file>.  For more details see:
    # https://github.com/microsoft/winget-cli/docs/create.md
    # Restrictions: [valid sha256 hash]
    SignatureSha256: 69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82

    # Language is the specific language of the installer.  If no language is specified, the installer will display for all users.
    # Language must follow IETF language tag guidelines.
    # Language is not supported in this preview (5/24/2020)
    Language: en-US

    # InstallerType is a required field if not defined at the root.  Unless specified, the InstallerType will be assumed to be the same InstallerType as the root. 
    # See further restrictions on InstallerType earlier in this document.
    InstallerType: msix

    # Scope indicates if the installer is per user or per machine.  
    # Supported values: user and machine
    # Unless specified, user is the default.
    # Scope is not supported in this preview (5/24/2020)
    Scope: user
  
    # SystemAppId is a required field.  The value in the field differs depending on the InstallerType.
    # For MSI it is the product code.  Typically a GUID that is typically found in the uninstall registry location and includes the brackets.
    # For example:  {5740BD44-B58D-321A-AFC0-6D3D4556DD6C}
    # [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{3740BD44-B58D-321A-AFC0-6D3D4556DD6C}]
    # [HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{3740BD44-B58D-321A-AFC0-6D3D4556DD6C}]
    # The Package Manager will use that value to locate the uninstall string to uninstall the application.
    # For inno, wix, nullsoft, and exe, the SystemAppId should be a string that is located in either of the Uninstall keys above. 
    # For MSIX the SystemAppId should be the Package Family Name.  For example: Contoso.Toolbox.Finance_7wekyb3d8bbwe  
    # Restrictions: [min: 3, max:128] 
    SystemAppId: {3740BD44-B58D-321A-AFC0-6D3D4556DD6C}

    # Switches in installers can override the root specified switches. See definition earlier in this document.
    Switches:
      Language: /en-US
      Custom: /s

    # This is an example of an additional installer.
    # See further restrictions earlier in this document.
    # Support for multiple installers are not supported in this preview (5/24/2020)
  - Arch: x64
    Url: https://contosa.net/publiccontainer/contosainstaller64.exe
    Sha256: 69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C83
    Language: en-US
    Scope: user

    # Localized values will provide links and text to match the users settings.  For example the following links and text will be displayed instead.
Localization:
  - Language: es-MX
    Description: Text to display for es-MX
    Homepage: https://github.com/microsoft/msix-packaging/es-MX
    LicenseUrl: https://github.com/microsoft/msix-packaging/blob/master/LICENSE-es-MX

# ManifestVersion: 0.1.0
# ManifestVersion is a required field. ManifestVersion will allow the client to detect updated manifests and treat it differently.   


```
## Minimal YAML file example
As specified above, only a number of fields are required.  The minimal supported YAML file would look like this and be exposed in english to all Windows 10 users.

```YAML
Id: Microsoft.VisualStudioCode
Version: 1.41.1
Name: Visual Studio Code
Publisher: Microsoft Corporation
Installers:
    - Arch: x64
      Url: https://aka.ms/win32-x64-user-stable
      Installertype: Inno
      Sha256: 65DB2F2AC2686C7F2FD69D4A4C6683B888DC55BFA20A0E32CA9F838B51689A3B
```
## Best Practices
The Id must be unique.  You cannot have multiple submissions with the same Id.

Avoid creating multiple publisher folders.  For example, do not create "Contoso Ltd." if there is already a "Contoso" folder.

All tools must support a silent install.  If you have an executable that does not support a silent install, then we cannot provide that tool at this time.  Provide feedback to the repo.  If this is a restriction that you would like to see removed add an issue or up vote it.
My installer supports a silent install, but is not one of the supported InstallerTypes.  How can I get it added to your repo.  Ping us on the repo.

Provide as many fields as possible.  The more data you provide the better the user will find the tool.

Length of strings in this spec should be limited to 100 characters before a line break.


History:

| Version        | Date           | Description  |
| ------------- |-------------| -----|
| .01      | 1/6/2020 | draft |
| .02      | 1/9/2020 | Made many modifications based on constructive feedback |
| .03      | 1/31/2020 | Made many modifications based on constructive feedback |
| .04      | 3/18/2020 | Documented the msix hash support |
| .05      | 4/1/2020 | Added restrictions.  Added SystemAppId |
| .06      | 4/23/2020 | Renamed client.  Updated License to required. |
| .07      | 5/15/2020 | Add ManifestVersion. |
