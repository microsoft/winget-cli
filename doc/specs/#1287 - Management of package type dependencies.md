---
author: Florencia Zanollo fzanollo/t-fzanollo@microsoft.com
created on: 2021-07-15
last updated: 2021-07-15
issue id: 1287
---

# Management of package type dependencies

For [#1287](https://github.com/microsoft/winget-cli/issues/1287)

## Abstract
As a new step in the pursue of dependency management, the Windows Package Manager will take care of one of the four types of dependencies (Package) for most of the commands. The underlying logic of the code will prove useful when implementing the rest of the types and commands; since it will manage dependencies' graph building and validation, as well as the correct order of installation.

## Solution Design
The Windows Package Manager will build the dependencies' graph and corroborate there's not a cyclic dependency, among other validations (depending on the command).

### Install:
Install command will build the dependency graph at runtime, from installer information. It will report on the other three types of dependencies and manage package type installation/validation (for new/installed dependencies respectively).
As a best effort, in case a cyclic dependency exists, Windows Package Manager will inform the user but try to install in some order; this is because most of the dependencies are at run time so there shouldn't present an issue on install.

While building the graph, install command will verify:
* Availability: the package declared as dependency will need to be an existing one.
* Installed version: it will check for existing versions of the dependency and update if the minimum required version is bigger than the installed.
* Version: minimum required version will need to be less or equal to the latest available one.

Information will be shown about failures, existence or installation progress for each of the dependencies required.

When installing from a local manifest, a dependency source will be required (using --dependency-source parameter), but only if the target has at least one package type dependency declared.

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
Building package type dependencies' graph:
    No errors or cyclic dependencies found.
Installing package type dependencies:
    Found WindowsTerminal [Microsoft.WindowsTerminal]
    This application is licensed to you by its owner.
    Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
    Downloading (...)
    Successfully verified installer hash
    Starting package install...
Downloading https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v7.9.5/npp.7.9.5.Installer.x64.exe
Successfully verified installer hash
Starting package install...
```

### Import:
Import command will report all first level dependencies beforehand and after will work as install command for each of the packages found, iteratively. Ex. if packages *foo* and *foo1* are part of the import, dependencies for *foo* will be fetch and installed before continuing with *foo1*.

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
Building package type dependencies' graph:
    No errors or cyclic dependencies found.
Installing package type dependencies:
    Found WindowsTerminal [Microsoft.WindowsTerminal]
    This application is licensed to you by its owner.
    Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
    Downloading (...)
    Successfully verified installer hash
    Starting package install...
Found  [plex.Plex]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
(...)
```

### Update:
* Update one: will also work as install command, we need to check again for dependencies as the new installer can have new ones or bigger minimum required versions.
* Update many: will work iteratively (as import).

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
Building package type dependencies' graph:
    No errors or cyclic dependencies found.
Installing package type dependencies:
    Found WindowsTerminal [Microsoft.WindowsTerminal]
    This application is licensed to you by its owner.
    Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
    Downloading (...)
    Successfully verified installer hash
    Starting package install...
Successfully verified installer hash
Starting package install...
```

## Capabilities
Will manage package type dependencies installation for install, import and update commands.
Will not manage or try to install any of the other types of dependencies (Windows Features, Windows Libraries, External).
