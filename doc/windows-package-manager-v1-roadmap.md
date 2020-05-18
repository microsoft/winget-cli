# Windows Package Manager v1.0 Roadmap

## Overview

This document outlines our aspirational roadmap to delivering Windows Package Manager v1.0 by Spring 2021. We anticpate substantial feedback from the community, and as such, this plan is subject to change.

## Milestones

The Windows Package Manager project is engineered and delivered as a set of 4-week milestones. An example of how this might be executed is listed below:

| Duration | Activity | Releases |
| --- | --- | --- |
| 2 weeks | Dev Work<br/> <ul><li>Fixes / Features for future Windows Releases</li><li>Fixes / Features for Windows Package Manager</li></ul> |  Release to Internal Selfhosters at end of week 2 |
| 1 week | Quality & Stability<br/> <ul><li>Bug Fixes</li><li>Perf & Stability</li><li>UI Polish</li><li>Tests</li><li>etc.</li></ul>| Push to Windows Insiders Microsoft Store at end of week 3 |
| 1 week | Release <br/> <ul><li>Available from to Windows Insiders through the [Microsoft Store](https://www.microsoft.com/p/app-installer/9nblggh4nns1) & [GitHub Releases](https://github.com/microsoft/winget-cli/releases) (Tues of 4th week)</li><li>Release Notes & Announcement Blog published</li><li>Engineering System Maintenance</li><li>Community Engagement</li><li>Docs</li><li>Future Milestone Planning</li></ul> | Release available from Microsoft Store & GitHub Releases |

## Windows Package Manager Roadmap / Timeline

Ultimately, we're aiming for Windows Package Manager v1.0 to be feature-complete by Dec 2020, and to declare v1.0 by April 2021:

> ⚠ Note: Windows Package Manager v1.0 will be a quality-oriented release driven in large part by the community. So, ___if you see bugs, find/file them___!

| Release or Milestone end date&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Milestone Name | Key Deliverables |
| --- | --- | --- |
| 2020-05-19 | [v0.1](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Windows Package Manager [announced](https://devblogs.microsoft.com/commandline/windows-package-manager-preview/) & open-sourced ([Build 2020 Windows Package Manager session]) |
| 2020-06-23 | [v0.2](https://github.com/microsoft/winget-cli/milestone/4) | |
| 2020-07-28 | [v0.3](https://github.com/microsoft/winget-cli/milestone/5) | |
| 2020-08-25 | [v0.4](https://github.com/microsoft/winget-cli/milestone/6) | |
| 2020-09-22 | [v0.5](https://github.com/microsoft/winget-cli/milestone/7) | |
| 2020-10-27 | [v0.6](https://github.com/microsoft/winget-cli/milestone/8) | |
| 2020-11-24 | [v0.7](https://github.com/microsoft/winget-cli/milestone/9) | |
| 2020-12-22 | [v0.8](https://github.com/microsoft/winget-cli/milestone/10) | |
| 2021-01-26 | [v0.9](https://github.com/microsoft/winget-cli/milestone/11) | |
| 2021-02-23 | [v0.10](https://github.com/microsoft/winget-cli/milestone/12) | |
| 2021-03-23 | [v0.11](https://github.com/microsoft/winget-cli/milestone/13) | |
| 2021-04-27 | [v0.12](https://github.com/microsoft/winget-cli/milestone/14) | |
| 2021-05    | [v1.0](https://github.com/microsoft/winget-cli/milestone/1) | Windows Package Manager v1.0 Release |

## GitHub Milestones

Each Release above is/will be reflected in our [GitHub milestones](https://github.com/microsoft/terminal/milestones):

| Milestone | Description |
| --- | --- |
| [v0.1](https://github.com/microsoft/winget-cli/milestone/3) | Initial Preview |
| [Backlog](https://github.com/microsoft/winget-cli/milestone/2) | Work not yet assigned to a milestone or release |

## Issue Triage & Prioritization

Incoming issues/asks/etc. are triaged several times a week, labelled appropriately, and assigned to a milestone in priority order:

* P0 (serious crashes, data loss, etc.) issues are scheduled to be dealt with ASAP
* P1/2 issues/features/asks  assigned to the current or future milestone, or to the [Package Manager v1.0 milestone](https://github.com/microsoft/winget-cli/milestone/1) for future assignment, if required to deliver a v1.0 feature
* Issues/features/asks not on our list of v1.0 features is assigned to the [Windows Package Manager Backlog](https://github.com/microsoft/winget-cli/milestone/2) for subsequent triage, prioritization & scheduling.

## v1.0 Scenarios

The following are a list of the key scenarios we're aiming to deliver for Windows Package Manager v1.0.

> 👉 Note: There are many other features that don't fit within v1.0, but will be re-assessed and prioritized for v2.0, the plan for which will be published in early in 2021.

| Release | Priority\* | Feature | Description/Notes |
| --- | --- | --- | --- |
| V1 | 0 | Microsoft Store | Support for installing Apps from the Microsoft Store |
| V1 | 0 | REST Repository | Support for a REST based repository |
| V1 | 0 | --list | The client should be able to tell you what Apps are installed |
| V1 | 0 | --update | The client should be able to update one or "all" installed Apps |
| V1 | 0 | --uninstall | The client should be able to uninstall Apps |
| V1 | 0 | Multiple Installers | The client should be able to choose between multiple different installers from the same manifest |
| V1 | 0 | Multiple Languages | The client will support installation for almost every language for which there is a fixed-width font including East Asian languages. Bonus points for RTL languages/scripts. |
| V1 | 0 | Multiple Architectures | The client should support multiple architectures in the same manifest |
| V1 | 0 | Non-Zero Exit Codes | The client should support applicatinons with non-zero exit codes as success |
| V1 | 0 | Install .zip | The client should be able to install programs in a .zip file |
| V1 | 0 | Configurability & Customization | The client will have a modern, flexible settings mechanism that persists settings to/from a JSON file stored in the user's app data folders, and/or in files synchronized between machines via OneDrive, etc. |
| V1 | 0 | Accessibility (A11y) | The client will be highly accessible and inclusive. It will expose its contents via [UIA](https://docs.microsoft.com/en-us/dotnet/framework/ui-automation/ui-automation-overview) to support tools such as [Windows Narrator](https://support.microsoft.com/en-us/help/22798/windows-10-complete-guide-to-narrator), and UI automation tools including [WinAppDriver](https://github.com/Microsoft/WinAppDriver) |
| V1 | 0 | Color Theming & Styling | The client will honor the user's Windows dark/light theme settings, and/or color accent settings. |
| V1 | 0 | Autocomplete | The client will support autocomplete for all commands and packages in the local cache |
| V1 | 0 | Release Channels | Some applications have different release channels and we should support them |
| V1 | 0 | User vs. System | Applications may be installed for the local user or for the system |
| V1 | 0 | Metered Networks | The client should download responsibly when on metered networks |
| V1 | 0 | Delivery Optimization | Delivery Optimization should be leveraged for large Apps |
| V1 | 0 | Elevated Priveleges | The client should support installing Apps that require elevated priveleges |
| V1 | 0 | Control Panel | Support for Apps installed in the Control Panel |
| V1 | 0 | Group Policy | Support for Group Policy control |
| V1 | 0 | Package Snapshot | Support for exporting all installed packages as a package set |
| V1 | 0 | Package Set | Support for installing a set of packages |
| V1 | 0 | Manifest Wizard | --create helps a user generate a manifest |
| V1 | 0 | App Config Files | Support for silent installers that require a configuration file |
| V1 | 0 | UX Enhancements | Client Verbosity Settings |
| V1 | 0 | Dependency Support | Some Apps depend on other Apps to be present like IDEs and Programming Languages |
| V1 | 0 | Install PWA | Support installing Progressive Web Applications |

Feature Notes:

\* Feature Priorities:

0. Mandatory <br/>
1. Optimal <br/>
2. Optional / Stretch-goal <br/>
