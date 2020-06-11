# Windows Package Manager v1.0 Roadmap

## Overview

This document outlines our aspirational roadmap to delivering Windows Package Manager v1.0 by March-May 2021. We anticipate substantial feedback from the community, and as such, this plan is subject to change.

## Milestones

The Windows Package Manager project is engineered and delivered as described below.

### Activities

#### Dev Work

* Fixes / Features for Windows Package Manager
* Fixes / Features for future Windows Releases

#### Quality & Stability

* Bug Fixes
* Performance & Stability
* Globalization, Localization, Internationalization, Accessibility
* Tests

#### Release

* Available from to Windows Insiders through the [Microsoft Store](https://www.microsoft.com/p/app-installer/9nblggh4nns1) & [GitHub Releases](https://github.com/microsoft/winget-cli/releases)
* Release Notes & Announcement Blog published
* Engineering System Maintenance
* Community Engagement
* [Docs](https://docs.microsoft.com/windows/package-manager/)
* Future Milestone Planning

### Releases

Releases will be available here on GitHub first We will release to the App Installer for Insiders as frequently as the releases have met our quality bars.

## Windows Package Manager Roadmap / Timeline

Ultimately, we're aiming for Windows Package Manager v1.0 to be released in Spring 2021:

| Release or Milestone end date | Milestone Name | Key Deliverables |
| --- | --- | --- |
| 2020-05-19 | [v0.1](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Windows Package Manager [announced](https://devblogs.microsoft.com/commandline/windows-package-manager-preview/) & open-sourced ([Build 2020 Windows Package Manager session]) |
| June 2020 | [v0.2](https://github.com/microsoft/winget-cli/milestone/4) | |
| July 2020 | [v0.3](https://github.com/microsoft/winget-cli/milestone/5) | |
| August 2020 | [v0.4](https://github.com/microsoft/winget-cli/milestone/6) | |
| September 2020 | [v0.5](https://github.com/microsoft/winget-cli/milestone/7) | |
| October 2020 | [v0.6](https://github.com/microsoft/winget-cli/milestone/8) | |
| November 2020 | [v0.7](https://github.com/microsoft/winget-cli/milestone/9) | |
| December 2020 | [v0.8](https://github.com/microsoft/winget-cli/milestone/10) | |
| January 2021 | [v0.9](https://github.com/microsoft/winget-cli/milestone/11) | |
| February 2021 | [v0.10](https://github.com/microsoft/winget-cli/milestone/12) | |
| March 2021 | [v0.11](https://github.com/microsoft/winget-cli/milestone/13) | |
| April 2021 | [v0.12](https://github.com/microsoft/winget-cli/milestone/14) | |
| May 2021 | [v1.0](https://github.com/microsoft/winget-cli/milestone/1) | Windows Package Manager v1.0 Release |

## GitHub Milestones

Each Release above is/will be reflected in our [GitHub milestones](https://github.com/microsoft/winget-cli/milestones):

| Milestone | Description |
| --- | --- |
| [v0.1](https://github.com/microsoft/winget-cli/milestone/3) | Initial Preview |
| [Backlog](https://github.com/microsoft/winget-cli/milestone/2) | Work not yet assigned to a milestone or release |

## Issue Triage & Prioritization

Incoming issues/asks/etc. are triaged several times a week, labelled appropriately, and assigned to a milestone in priority order:

* P0 (serious crashes, data loss, etc.) issues are scheduled to be dealt with ASAP
* P1/2 issues/features/asks  assigned to the current or future milestone, or to the [Windows Package Manager v1.0 milestone](https://github.com/microsoft/winget-cli/milestone/1) for future assignment, if required to deliver a v1.0 feature
* Issues/features/asks not on our list of v1.0 features is assigned to the [Windows Package Manager Backlog](https://github.com/microsoft/winget-cli/milestone/2) for subsequent triage, prioritization & scheduling.

## v1.0 Scenarios

The following are a list of the key scenarios we're aiming to deliver for Windows Package Manager v1.0.

> 👉 Note: There may be features that don't fit within v1.0, but will be re-assessed and prioritized for a future release, the plan for which will be published in early in 2021. As features become more defined, links to the associated issues will be added.

| Release | Feature | Description/Notes |
| --- | --- | --- |
| V1 | Microsoft Store | Support for installing Apps from the Microsoft Store |
| V1 | REST Repository | Support for a REST based repository |
| V1 | --list | The client should be able to tell you what Apps are installed |
| V1 | --update | The client should be able to update one or "all" installed Apps |
| V1 | --uninstall | The client should be able to uninstall Apps |
| V1 | Multiple Installers | The client should be able to choose between multiple different installers from the same manifest |
| V1 | Multiple Languages | The client will support installation for almost every language for which there is a fixed-width font including East Asian languages. Bonus points for RTL languages/scripts. |
| V1 | Multiple Architectures | The client should support multiple architectures in the same manifest |
| V1 | Non-Zero Exit Codes | The client should support applications with non-zero exit codes as success |
| V1 | Install .zip | The client should be able to install programs in a .zip file |
| V1 | Configurability & Customization | The client will have a modern, flexible settings mechanism that persists settings to/from a JSON file stored in the user's app data folders, and/or in files synchronized between machines via OneDrive, etc. |
| V1 | Accessibility (A11y) | The client will be highly accessible and inclusive. It will expose its contents via [UIA](https://docs.microsoft.com/en-us/dotnet/framework/ui-automation/ui-automation-overview) to support tools such as [Windows Narrator](https://support.microsoft.com/en-us/help/22798/windows-10-complete-guide-to-narrator), and UI automation tools including [WinAppDriver](https://github.com/Microsoft/WinAppDriver) |
| V1 | Color Theming & Styling | The client will honor the user's Windows dark/light theme settings, and/or color accent settings. |
| V1 | Autocomplete | The client will support autocomplete for all commands and packages in the local cache |
| V1 | Release Channels | Some applications have different release channels and we should support them |
| V1 | User vs. System | Applications may be installed for the local user or for the system |
| V1 | Metered Networks | The client should download responsibly when on metered networks |
| V1 | Delivery Optimization | Delivery Optimization should be leveraged for large Apps |
| V1 | Elevated Privileges | The client should support installing Apps that require elevated privileges |
| V1 | Control Panel | Support for Apps installed in the Control Panel |
| V1 | Group Policy | Support for Group Policy control |
| V1 | Package Snapshot | Support for exporting all installed packages as a package set |
| V1 | Package Set | Support for installing a set of packages |
| V1 | Manifest Wizard | --create helps a user generate a manifest |
| V1 | App Config Files | Support for silent installers that require a configuration file |
| V1 | UX Enhancements | Client Verbosity Settings |
| V1 | Dependency Support | Some Apps depend on other Apps to be present like IDEs and Programming Languages |
| V1 | Install PWA | Support installing Progressive Web Applications |

Feature Notes:

\* Feature Priorities will be influenced by community feedback on issues.
