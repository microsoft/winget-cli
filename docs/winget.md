# Windows Package Manager

The Windows Package Manager is a command line tool that developer will use to discover, install, upgrade, remove and configure their applications on Windows 10.

This document will serve as the specification for the client for easy consumption.

## Roadmap
The Windows Package Manager will be released in a series of previews.  

## Availability
The Windows Package Manager ships in the Windows Desktop App Installer package.  You can access and try out the Windows Package manager either 2 ways:
* Participate in the Windows Package Manager flight.
* Install Windows Desktop App Installer package located in the release folder of this repo. 

## Minimum Requirements
Windows 10, version 1709 (10.0.16299) or higher.

## Commands
The Windows Package Manager supports the following commands:
* [search](search.md) - used to search for an application
* [install](install.md) - used to install an application
* [show](show.md) - used to show details on the application
* list - [coming soon] - used to show which applications are installed and if there are updates available.
* uninstall [coming soon] - used to uninstall an application 
* update [coming soon] - used to update an application to a newer version
* repair [in discussion] - used to re-install or repair the application
* [create](create.md)  - command used to help create hash data or verify YAML files.
* [source](source.md) - command used to add, remove and update repos used by the Windows Package Manager 

## Options
In winget supports the following options
| options | description|
| --------------: | :------------- |
| **-v,--version** | this option returns the current version of winget
| **--info** |  info provides you with all detailed information on winget including the links to the license and privacy statement.
| **-?, --help** |  get additional help on this winget
| <img width=100   />|<img width=500 />  |
