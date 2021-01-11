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

| Release or Milestone end date | Milestone(s) | Key Features |
| --- | --- | --- |
| 2020-05-19 | [v0.1](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Windows Package Manager [announced](https://devblogs.microsoft.com/commandline/windows-package-manager-preview/) & open-sourced ([Build 2020 Windows Package Manager session]) |
| July 2020 |  | |
| August 2020 |  | |
| September 2020 | [v0.2](https://github.com/microsoft/winget-cli/milestone/4) | Support for Microsoft Store (curated list of developer tools in experimental feature)|
| October 2020 | [v0.3](https://github.com/microsoft/winget-cli/milestone/5) | List (includes Apps in Control Panel/Add Remove Programs) |
| November 2020 | [v0.4](https://github.com/microsoft/winget-cli/milestone/6) | Upgrade |
| December 2020 | [v0.5](https://github.com/microsoft/winget-cli/milestone/7)   | Uninstall |
| January 2021 | [v0.6](https://github.com/microsoft/winget-cli/milestone/8) | Import / Export |
| February 2021 | , [v0.7](https://github.com/microsoft/winget-cli/milestone/9), [v0.8](https://github.com/microsoft/winget-cli/milestone/10), [v0.9](https://github.com/microsoft/winget-cli/milestone/11), [v0.10](https://github.com/microsoft/winget-cli/milestone/12) | Dependency Support, Multiple Architectures, Multiple Languages, and User vs. System installation |
| March 2021 | [v0.11](https://github.com/microsoft/winget-cli/milestone/13),  [v0.12](https://github.com/microsoft/winget-cli/milestone/14), [v0.13](https://github.com/microsoft/winget-cli/milestone/15), [v0.14](https://github.com/microsoft/winget-cli/milestone/16), [v0.16](https://github.com/microsoft/winget-cli/milestone/18), [v0.17](https://github.com/microsoft/winget-cli/milestone/19) | Third party REST source, Group Policy, Delivery Optimization, Metered Networks, .zip, and .exe|
| April 2021 |||
| May 2021 | [v1.0](https://github.com/microsoft/winget-cli/milestone/1) | Windows Package Manager v1.0 Release |

Note: Many of the features have been implemented in experimental mode. If you execute `winget features` a list of experimental features and their status is displayed. You may modify your settings file with `winget settings` to enable or disable them. The experimental "list" feature is a prerequisite for "upgrade", "uninstall", and other features in development. Once the "list" feature has been fully implemented, the other stable features depending on it will also be migrated from experimental to default.

## GitHub Milestones

Each Release above is/will be reflected in our [GitHub milestones](https://github.com/microsoft/winget-cli/milestones):

| Milestone | Description |
| --- | --- |
| [v0.1](https://github.com/microsoft/winget-cli/milestone/3) | Initial Preview |
| [v0.2](https://github.com/microsoft/winget-cli/milestone/4) | `winget upgrade` and Pin a package |
| [v0.3](https://github.com/microsoft/winget-cli/milestone/5) | `winget list` |
| [v0.4](https://github.com/microsoft/winget-cli/milestone/6) | `winget upgrade` |
| [v0.5](https://github.com/microsoft/winget-cli/milestone/7) | `winget uninstall` |
| [v0.6](https://github.com/microsoft/winget-cli/milestone/8) | Import / Export |
| [v0.7](https://github.com/microsoft/winget-cli/milestone/9) | Dependency Support |
| [v0.8](https://github.com/microsoft/winget-cli/milestone/10) | Multiple Architectures |
| [v0.9](https://github.com/microsoft/winget-cli/milestone/11) | User vs. System |
| [v0.10](https://github.com/microsoft/winget-cli/milestone/12) | Multiple Languages |
| [v0.11](https://github.com/microsoft/winget-cli/milestone/13) | Third party REST source |
| [v0.12](https://github.com/microsoft/winget-cli/milestone/14) | Group Policy |
| [v0.13](https://github.com/microsoft/winget-cli/milestone/15) | Delivery Optimization |
| [v0.14](https://github.com/microsoft/winget-cli/milestone/16) | Metered Networks |
| ~~[v0.15](https://github.com/microsoft/winget-cli/milestone/17)~~ | App Config Files* |
| [v0.16](https://github.com/microsoft/winget-cli/milestone/18) | .zip |
| [v0.17](https://github.com/microsoft/winget-cli/milestone/19) | .exe  |
| [v0.18](https://github.com/microsoft/winget-cli/milestone/20) | Portable/Standalone Apps |
| [v0.19](https://github.com/microsoft/winget-cli/milestone/21) | Specify App Install Directory |
| [v0.20](https://github.com/microsoft/winget-cli/milestone/22) | Non zero exit codes |
| [v0.21](https://github.com/microsoft/winget-cli/milestone/23) | Telemetry Opt Out |
| [v0.22](https://github.com/microsoft/winget-cli/milestone/24) | Client Verbosity Settings |
| [v0.23](https://github.com/microsoft/winget-cli/milestone/25) | Release Channels |
| [v0.24](https://github.com/microsoft/winget-cli/milestone/26) | Native Power Shell |
| [v0.25](https://github.com/microsoft/winget-cli/milestone/27) | PWA |
| [v0.26](https://github.com/microsoft/winget-cli/milestone/28) | Install Multiple Apps (command) |
| [v0.27](https://github.com/microsoft/winget-cli/milestone/29) | Suppress reboot on default |
| [v0.28](https://github.com/microsoft/winget-cli/milestone/30) | Version specification |
| [v0.29](https://github.com/microsoft/winget-cli/milestone/31) | Parallel download |
| [v0.30](https://github.com/microsoft/winget-cli/milestone/32) | Fonts |
| [v0.31](https://github.com/microsoft/winget-cli/milestone/33) | Auto Upgrade Apps |
| [Backlog](https://github.com/microsoft/winget-cli/milestone/2) | Work not yet assigned to a milestone or release |

* Versions with strikethrough have been pushed post v1.0.

## Issue Triage & Prioritization

Incoming issues/asks/etc. are triaged several times a week, labelled appropriately, and assigned to a milestone in priority order:

* P0 (serious crashes, data loss, etc.) issues are scheduled to be dealt with ASAP.
* P1/2 issues/features/asks  assigned to the current or future milestone, or to the [Windows Package Manager v1.0 milestone](https://github.com/microsoft/winget-cli/milestone/1) for future assignment, if required to deliver a v1.0 feature.
* Issues/features/asks not on our list of v1.0 features is assigned to the [Windows Package Manager Backlog](https://github.com/microsoft/winget-cli/milestone/2) for subsequent triage, prioritization & scheduling.

## v1.0 Scenarios

The following are a list of the key scenarios we're aiming to deliver for Windows Package Manager v1.0.

> 👉 Note: There may be features that don't fit within v1.0, but will be re-assessed and prioritized for a future release, the plan for which will be published in early in 2021. As features become more defined, links to the associated issues will be added.

| Release | Feature | Description/Notes |
| --- | --- | --- |
| [v0.1.41331-preview](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Elevated Privileges | The client should support installing Apps that require elevated privileges. |
| [v0.1.41821-preview](https://github.com/microsoft/winget-cli/releases/tag/v0.1.41821-preview) | Configurability & Customization | The client will have a modern, flexible settings mechanism that persists settings to/from a JSON file stored in the user's app data folders, and/or in files synchronized between machines via OneDrive, etc. |
| [v0.1.41821-preview](https://github.com/microsoft/winget-cli/releases/tag/v0.1.41821-preview) | Color Theming & Styling | The client will honor the user's Windows dark/light theme settings, and/or color accent settings. |
| [v0.1.42241-preview](https://github.com/microsoft/winget-cli/releases/tag/v0.1.42241-preview) | Autocomplete | The client will support autocomplete for all commands and packages in the local cache. |
| V1 | [#119](https://github.com/microsoft/winget-cli/issues/119) `winget list` | The client should be able to tell you what Apps are installed including the Control Panel. |
| V1 | [#120](https://github.com/microsoft/winget-cli/issues/120) `winget upgrade` | The client should be able to update one or "all" installed Apps. |
| V1 | [#121](https://github.com/microsoft/winget-cli/issues/121) `winget uninstall` | The client should be able to uninstall Apps. |
| V1 | [#220](https://github.com/microsoft/winget-cli/issues/220) Export/Import | The client should be able to export the list of installed Apps and import the exported list. |
| V1 | [#163](https://github.com/microsoft/winget-cli/issues/163) Dependencies | The client should be able to install package dependencies. |
| V1 | [#132](https://github.com/microsoft/winget-cli/issues/132) Multiple Architectures | The client should support multiple architectures in the same manifest. |
| V1 | [#149](https://github.com/microsoft/winget-cli/issues/149) User vs. System | Applications may be installed for the local user or for the system. |
| V1 | [#124](https://github.com/microsoft/winget-cli/issues/124) Multiple Languages | The client will support installation for almost every language for which there is a fixed-width font including East Asian languages. Bonus points for RTL languages/scripts. |
| V1 | [#226](https://github.com/microsoft/winget-cli/issues/226) REST Repository | Support for a REST based repository. |
| V1 | [#154](https://github.com/microsoft/winget-cli/issues/154) Group Policy | Support for Group Policy control. |
| V1 | [#151](https://github.com/microsoft/winget-cli/issues/151) Delivery Optimization | Delivery Optimization should be leveraged for large Apps. |
| V1 | [#150](https://github.com/microsoft/winget-cli/issues/150) Metered Networks | The client should download responsibly when on metered networks. |
| V1 | [#140](https://github.com/microsoft/winget-cli/issues/140) Install .zip | The client should be able to install programs in a .zip file. |
| V1 | [#194](https://github.com/microsoft/winget-cli/issues/194) Install .exe | The client should be able to install a static .exe file. |
| V1 | [#182](https://github.com/microsoft/winget-cli/issues/182) Install portable app | The client should be able to install portable Apps. |
| V1 | [#201](https://github.com/microsoft/winget-cli/issues/201) Specify install directory | The client should be able to install to an alternate directory. |
| V1 | [#137](https://github.com/microsoft/winget-cli/issues/137) Non-Zero Exit Codes | The client should support applications with non-zero exit codes as success. |
| V1 | [#279](https://github.com/microsoft/winget-cli/issues/279) Opt-Out of Telemetry | The client should be able to Opt-Out of Telemetry. |
| V1 | [#161](https://github.com/microsoft/winget-cli/issues/161) Client Verbosity Settings | The client should support different verbosity settings. |
| V1 | [#147](https://github.com/microsoft/winget-cli/issues/147) Release Channels | Some applications have different release channels and we should support them. |
| V1 | [#221](https://github.com/microsoft/winget-cli/issues/221) Native PowerShell | Native PowerShell support for the client. |
| V1 | [#164](https://github.com/microsoft/winget-cli/issues/164) Install PWA | Support installing Progressive Web Applications. |
| V1 | [#219](https://github.com/microsoft/winget-cli/issues/219) Install Multiple Apps | The client should allow a user to specify multiple apps to install. |
| V1 | [#229](https://github.com/microsoft/winget-cli/issues/229) Suppress reboot | The client should allow a user to suppress reboot as a default setting. |
| V1 | [#227](https://github.com/microsoft/winget-cli/issues/227) Version specification | The client should allow more variation to specifying package versions for installation. |
| V1 | [#225](https://github.com/microsoft/winget-cli/issues/225) Parallel download | The client should support multiple connections per package for download. |
| V1 | [#166](https://github.com/microsoft/winget-cli/issues/166) Fonts | The client should support installing fonts. |
| V1 | [#212](https://github.com/microsoft/winget-cli/issues/212) Auto Upgrade | The client should be able to auto upgrade installed apps if configured to do so. |
| V1 | [#157](https://github.com/microsoft/winget-cli/issues/157) Manifest Wizard | Help a user generate a manifest. |
| V1 | [#161](https://github.com/microsoft/winget-cli/issues/161) Verbosity | Client Verbosity Settings. |
| V1 | [#117](https://github.com/microsoft/winget-cli/issues/117) Microsoft Store | Support for installing Apps from the Microsoft Store. |
| V1 | Accessibility (A11y) | The client will be highly accessible and inclusive. It will expose its contents via [UIA](https://docs.microsoft.com/en-us/dotnet/framework/ui-automation/ui-automation-overview) to support tools such as [Windows Narrator](https://support.microsoft.com/en-us/help/22798/windows-10-complete-guide-to-narrator), and UI automation tools including [WinAppDriver](https://github.com/Microsoft/WinAppDriver). |
| V1.x | [#158](https://github.com/microsoft/winget-cli/issues/158) App Config Files | Support for silent installers that require a configuration file. |

Feature Notes:

\* Feature Priorities will be influenced by community feedback on issues.
