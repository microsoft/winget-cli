---
author: Florencia Zanollo fzanollo/t-fzanollo@microsoft.com
created on: 2021-05-28
last updated: 2021-05-28
issue id: 1012
---

# Show dependencies

For [#1012](https://github.com/microsoft/winget-cli/issues/1012)

## Abstract
Several packages require other packages as dependencies. The Windows Package Manager should be able to support declared dependencies and, as a first step to manage them, inform the user about any required ones.

## Solution Design
The Windows Package Manager should be able to report package dependency information for each of the four different types of dependencies declared in the [v1.0 manifest schemas](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/).

* Windows Features
* Windows Libraries
* Package Dependencies (same source)
* External Dependencies

Only dependencies declared in the manifest/installer will be shown, not the entire dependency graph.

### install
The install command will enumerate the dependencies of the package version being installed as follows:
```
> winget install Notepad++
Found Notepad++ [Notepad++.Notepad++]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
This package requires the following dependencies:
  - Windows Feature: 
      Hyper-V
  - Package: 
      Microsoft.WindowsTerminal
Downloading https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v7.9.5/npp.7.9.5.Installer.x64.exe
Successfully verified installer hash
Starting package install...
```

### show
The show command will enumerate the dependencies of the package as follows:
```
> winget show Notepad++
Found Notepad++ [Notepad++.Notepad++]
Version: 7.9.5
Publisher: Notepad++ Team
Author: Don Ho
Moniker: notepad++
Description: Notepad++ is a free (as in “free speech” and also as in “free beer”) source code editor and Notepad replacement that supports several languages. Running in the MS Windows environment, its use is governed by GNU General Public License.
Homepage: https://notepad-plus-plus.org/
License: GPL-2.0-only
License Url: https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/v7.9.5/LICENSE
Installer:
  Type: Nullsoft
  Locale: en-US
  Download Url: https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v7.9.5/npp.7.9.5.Installer.x64.exe
  SHA256: 4881548cd86491b453520e83c19292c93b9c6ce485a1f9eb9301e3913a9baced
  Dependencies:
    - Windows Feature: 
        Hyper-V
    - Package: 
        Microsoft.WindowsTerminal
```

### upgrade
The upgrade command will enumerate the dependencies of the package as follows:
```
> winget upgrade Notepad++
Found Notepad++ [Notepad++.Notepad++]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
This package requires the following dependencies:
  - Windows Feature: 
      Hyper-V
  - Package: 
      Microsoft.WindowsTerminal
Successfully verified installer hash
Starting package install...
```
As of now, it will not try to validate nor install any of the dependencies for any package version.

### uninstall
Uninstall needs more work as we don't have the actual installer to get the dependencies from. This will not be added in this step.

### validate
Will gather and report dependencies for all of the installers found. Will not check if they are valid nor if there are duplicates.
```
Manifest has the following dependencies that were not validated; ensure that they are valid:
  - Windows Feature: 
      Hyper-V
  - Package: 
      Microsoft.WindowsTerminal
Manifest validation succeeded.
```

### import
Will gather all the dependencies from the packages included in the import and show them together before starting.
```
The packages found in this import have the following dependencies:
  - Windows Feature: 
      Hyper-V
      Containers
  - Windows Libraries: 
      Microsoft.WinJS
  - Package: 
      Microsoft.WindowsTerminal
  - External: 
      JDK-11.0.10
Found  [Notepad++.Notepad++]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v8/npp.8.0.Installer.x64.exe
Successfully verified installer hash
Starting package install...
Successfully installed
Found  [plex.Plex]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Successfully verified installer hash
Starting package install...
Successfully installed
```

## Capabilities
It's only an informational feature, will not check if the dependency is a valid one, nor if the source is available.
If a dependency is declared more than once (for example when gathering all dependencies in an import) it will only show the highest minimum version needed. 

Keep in mind dependencies can be declared on the root manifest and on each of the installers. If they happen to be declared in both, installer's dependencies will override those of the manifest. With the manifest's dependencies working as a default whenever installer's dependencies are not declared.

## Future considerations
It may be able to enable/disable this feature using extra options for the command.
