---
author: <first-name> <last-name> <github-id>/<email>
created on: <yyyy-mm-dd>
last updated: <yyyy-mm-dd>
issue id: <github issue id>
---

# Spec Title

## Abstract

For our preview of the Windows Package Manager, the goal was to enable install of apps.  In order to be a package manager, the Windows Package Manager must be able to list all installed packages, communicate if there is an update and uninstall apps.  

This spec will cover the following commands: list, update, and uninstall.
## Inspiration

See abstract.  :)

## Solution Design

**List**
**Update \ Upgrade**
**Uninstall**

### List

The **list** command is a very specific search.  List will show the applications currently installed.  **List** will also compare the installed version to the known repository version.  If there is an update available, this will be provided as well.

## Usage

`winget list [[-q] \<query>] [\<options>]`

![list command](Images\120-List.png)

**List** when executed by itself will show all apps installed by the Windows Package Manager.

## Arguments

The following arguments are available.

| Argument  | Description |
|--------------|-------------|
| **-q,--query** |  The query used to search for an application. |
| **-?, --help** |  Gets additional help on this command. |

## Options

The following options are available.

| Option  | Description |
|--------------|-------------|
| **--all**         |  Show all applications that are installed, including apps not installed through Windows Package Manager|
| **--id**         |  Filter results by ID. |
| **--name**   |      Filter results by name. |
| **--moniker**   |  Filter results by application moniker. |
| **--tag** |     Filter results by tag
| **--command**  | Filter results by command
| **-s,--source** |   Find the application using the specified [source](source.md). |
| **-e,--exact**     | Find the application using exact match. |
 

By default **List** will show those apps installed through the Windows Package Manager.  However developers should be able to view all apps currently installed via other means.  



**--all** 
All will allow the Windows Package Manager to provide **list** data on all apps whether installed through the Package Manager or not.

To obtain this data, the Windows Package Manager will query the 
**Apps and Features**  installed apps.  This appears to be a superset of 
**Add Remove Programs** (ARP).


### Installed with Windows Package Manager
If the app is installed with the Windows Package Manager, we have the ID to map the installed version to the repository version.  If the version installed on the PC is lower than the the version in the repository, the list command will show the available version in the "available update."

### Installed out of band from the Windows Package Manager but in repo
If the app is installed out of band from the Package Manager, the app will be discovered if it registers correctly with the **Apps and Features**.  The Windows Package Manager will map the installed app back to the repo.  If the version installed is less than the version available in the repo, the **list** command will show the available version in the "available update."

If the installed version is greater than or equal to the version of the app available in the repo, then the "available update" will be left blank.

### Installed out of band from the Windows Package Manager but not in repo
If the app is installed out of band from the Package Manager, and is not available in the repo, then the Windows Package Manager will treat it as if there are no updates available.




## Mapping between Windows Package Manager repo and Apps and Features
*StoreApps*  
Store apps will be mapped via the The Package Family Name (which is composed by Publisher and Name).   The Package Family Name will be consistent between Apps and Features and the Windows Package Manager repo

*MSIs and EXEs*  
Will be mapped to the Windows Package Manager repo via the SystemAppId.  The SystemAppId is a required field.  The value in the field differs depending on the InstallerType.

For MSI it is the product code.  Typically a GUID that is typically found in the uninstall registry location and includes the brackets.
For example: ```{5740BD44-B58D-321A-AFC0-6D3D4556DD6C}```

```HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{3740BD44-B58D-321A-AFC0-6D3D4556DD6C}]```
    
```HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{3740BD44-B58D-321A-AFC0-6D3D4556DD6C}]```

For inno, wix, nullsoft, and exe, the SystemAppId should be a string that is located in either of the Uninstall keys above. 

### Upgrade

The **upgrade** command is designed to update one or more applications.  The **upgrade** command when executed with no parameters or with --all will update all packages that have updates available.


## Usage

`winget upgrade [[-q] \<query>] [\<options>]`

![show command]()

**Upgrade** when executed by itself will upgrade all apps ready for updates by the Windows Package Manager.


| Argument  | Description |
|--------------|-------------|
| **-q,--query** |  The query used to search for an application. |
| **-?, --help** |  Gets additional help on this command. |

## Options

The following options are available.

| Option  | Description |
|--------------|-------------|
| **--all**         |  update all applications that are installed and have updates available.|
| **--id**         |  Filter results by ID. |
| **--name**   |      Filter results by name. |
| **--moniker**   |  Filter results by application moniker. |
| **--tag** |     Filter results by tag
| **--command**  | Filter results by command
| **-s,--source** |   Find the application using the specified [source](source.md). |
| **-e,--exact**     | Find the application using exact match. |


*Upgrade app when no update is available*  
If the user attempts to update an app that there is no known update for, the package manager will issue an error.
"There is no update available from the community repository.  If you need to reinstall or repair the app, try ```winget repair <appname>```.



*Depnedencies*
Handling dependencies will be a challenge once Windows Package Manager supports dependencies.  Dependencies are not covered as part of this spec.



### Uninstall

The **uninstall** command is designed to uninstall an application.   

## Usage

`winget uninstall [[-q] \<query>] [\<options>]`

![show command]()


| Argument  | Description |
|--------------|-------------|
| **-q,--query** |  The query used to search for an application. |
| **-?, --help** |  Gets additional help on this command. |

## Options

The following options are available.

| Option  | Description |
|--------------|-------------|
| **--id**         |  Filter results by ID. |
| **--name**   |      Filter results by name. |
| **--moniker**   |  Filter results by application moniker. |
| **--tag** |     Filter results by tag
| **--command**  | Filter results by command
| **-s,--source** |   Find the application using the specified [source](source.md). |
| **-e,--exact**     | Find the application using exact match. |













[comment]: # Outline the design of the solution. Feel free to include ASCII-art diagrams, etc.

## UI/UX Design

[comment]: # What will this fix/feature look like? How will it affect the end user?

## Capabilities

[comment]: # Discuss how the proposed fixes/features impact the following key considerations:

### Accessibility

[comment]: # How will the proposed change impact accessibility for users of screen readers, assistive input devices, etc.

### Security

[comment]: # How will the proposed change impact security?

### Reliability

[comment]: # Will the proposed change improve reliability? If not, why make the change?

### Compatibility

[comment]: # Will the proposed change break existing code/behaviors? If so, how, and is the breaking change "worth it"?

### Performance, Power, and Efficiency

## Potential Issues

[comment]: # What are some of the things that might cause problems with the fixes/features proposed? Consider how the user might be negatively impacted.

## Future considerations

[comment]: # What are some of the things that the fixes/features might unlock in the future? Does the implementation of this spec enable scenarios?

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.
