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

Releases will be available here on GitHub first. We will release to the App Installer for Insiders as frequently as the releases have met our quality bars.

## Windows Package Manager Roadmap / Timeline

Ultimately, we're aiming for Windows Package Manager v1.0 to be released in March-May 2021:

| Release or Milestone end date | Release or Milestone | Key Features |
| --- | --- | --- |
| July 2020 | [v0.1.41821](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Windows Package Manager [announced](https://devblogs.microsoft.com/commandline/windows-package-manager-preview/) & open-sourced ([Build 2020 Windows Package Manager session]).|
| July 2020 | [v0.1.42101](https://github.com/microsoft/winget-cli/releases/tag/v0.1.42101-preview) | <ul><li>Experimental Feature Toggle</li><li>Settings Command</li></ul>|
| August 2020 | [v0.1.42241](https://github.com/microsoft/winget-cli/releases/tag/v0.1.42241-preview) | <ul><li>PowerShell [tab] autocomplete</li></ul>|
| September 2020 | [v0.2.2521](https://github.com/microsoft/winget-cli/releases/tag/v.0.2.2521-preview) | <ul><li> *Experimental Microsoft Store source* (curated list of developer tools)</li></ul> |
| October 2020 | [v0.2.2941](https://github.com/microsoft/winget-cli/releases/tag/v0.2.2941-preview)   | <ul><li>*Experimental List Command*</li><li>*Experimental Upgrade Command*</li></ul> |
| December 2020 | [v0.2.3162](https://github.com/microsoft/winget-cli/releases/tag/v0.2.3162-preview) | <ul><li>Include Apps from Add / Remove Programs for List and Upgrade</li></ul>|
| January 2021 | [v0.2.10191](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10191-preview)| <ul><li>*Experimental Uninstall Command*</li></ul>|
| March 2021 | [v0.2.10771](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10771-preview) | <ul><li>Non-Zero exit codes as success (requires v1.0 manifest schema)</li><li>Telemetry Opt-Out</li><li>v1.0 manifest schema validation</li><li>*Experimental Import Command*</li></ul>|
| April 2021 |[v0.2.10971](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10971-preview)|<ul><li>Group Policy via ADMX Ingestion</li><li>User vs. Machine installation (non MSIX)</li><li>*Experimental REST source*</li></ul>|
| April 2021|[v0.3.11102](https://github.com/microsoft/winget-cli/releases/tag/v-0.3.11102-preview)|<ul><li>Import Command</li></ul>|
| May 2021 | [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | Windows Package Manager v1.0 Release |

Note: Many of the features have been implemented in experimental mode. If you execute `winget features` a list of experimental features and their status is displayed. You may modify your settings file with `winget settings` to enable or disable them. The experimental "list" feature is a prerequisite for "upgrade", "uninstall", and other features in development. Once the "list" feature has been fully implemented, the other stable features depending on it will also be migrated from experimental to default.

## GitHub Milestones

We use [GitHub milestones](https://github.com/microsoft/winget-cli/milestones) to broadly organize what we intend to work on. Dates are only used for sequencing and do not represent a commitment.

| Milestone | Description |
| --- | --- |
| [Backlog](https://github.com/microsoft/winget-cli/milestone/2) | Work not yet assigned to a milestone or release |
| [v1.0](https://github.com/microsoft/winget-cli/milestone/1) | Work Targeted for v1.0 |
| [v1.1](https://github.com/microsoft/winget-cli/milestone/35) | Work Targeted for v1.1 |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34) | Triage for the next Milestone |

## Issue Triage & Prioritization

Incoming issues/asks/etc. are triaged several times a week, labelled appropriately, and assigned to a milestone in priority order:

* P0 (serious crashes, data loss, etc.) issues are scheduled to be dealt with ASAP.
* P1/2 issues/features/asks  assigned to the current or future milestone, or to the [Windows Package Manager v1.0 milestone](https://github.com/microsoft/winget-cli/milestone/1) for future assignment, if required to deliver a v1.0 feature.
* Issues/features/asks not on our list of v1.0 features is assigned to the [Windows Package Manager Backlog](https://github.com/microsoft/winget-cli/milestone/2) for subsequent triage, prioritization & scheduling.

## v1.0 Scenarios

The following are a list of the key scenarios we're aiming to deliver for Windows Package Manager v1.0.

> 👉 Note: There may be features that don't fit within v1.0, but will be re-assessed and prioritized for a future release, the plan for which will be published in early in 2021. As features become more defined, links to the associated issues will be added.

| Release or Milestone | Feature | Description/Notes |
| --- | --- | --- |
| [v0.1.41331](https://github.com/microsoft/winget-cli/releases/tag/v0.1.4331-preview) | Elevated Privileges | The client should support installing Apps that require elevated privileges. |
| [v0.1.41821](https://github.com/microsoft/winget-cli/releases/tag/v0.1.41821-preview) | Configurability & Customization | The client will have a modern, flexible settings mechanism that persists settings to/from a JSON file stored in the user's app data folders, and/or in files synchronized between machines via OneDrive, etc. |
| [v0.1.41821](https://github.com/microsoft/winget-cli/releases/tag/v0.1.41821-preview) | Color Theming & Styling | The client will honor the user's Windows dark/light theme settings, and/or color accent settings. |
| [v0.1.42241](https://github.com/microsoft/winget-cli/releases/tag/v0.1.42241-preview) | Autocomplete | The client will support autocomplete for all commands and packages in the local cache. |
| [v0.2.10771](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10771-preview) | [#132](https://github.com/microsoft/winget-cli/issues/132) Multiple Architectures | The client should support multiple architectures in the same manifest. |
| [v0.2.10771](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10771-preview) | [#137](https://github.com/microsoft/winget-cli/issues/137) Non-Zero Exit Codes | The client should support applications with non-zero exit codes as success. |
| [v0.2.10771](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10771-preview) | [#279](https://github.com/microsoft/winget-cli/issues/279) Opt-Out of Telemetry | The client should be able to Opt-Out of Telemetry. |
| [v0.2.10971](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10971-preview) | [#149](https://github.com/microsoft/winget-cli/issues/149) User vs. System | Applications may be installed for the local user or for the system. |
| [v0.2.10971](https://github.com/microsoft/winget-cli/releases/tag/v-0.2.10971-preview) | [#154](https://github.com/microsoft/winget-cli/issues/154) Group Policy | Support for Group Policy control. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#119](https://github.com/microsoft/winget-cli/issues/119) `winget list` | The client should be able to tell you what Apps are installed including the Control Panel. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#120](https://github.com/microsoft/winget-cli/issues/120) `winget upgrade` | The client should be able to update one or "all" installed Apps. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#121](https://github.com/microsoft/winget-cli/issues/121) `winget uninstall` | The client should be able to uninstall Apps. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#124](https://github.com/microsoft/winget-cli/issues/124) Multiple Languages | The client will support installation for almost every language for which there is a fixed-width font including East Asian languages. Bonus points for RTL languages/scripts. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#226](https://github.com/microsoft/winget-cli/issues/226) REST Repository | Support for a REST based repository. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#220](https://github.com/microsoft/winget-cli/issues/220) Export/Import | The client should be able to export the list of installed Apps and import the exported list. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#157](https://github.com/microsoft/winget-cli/issues/157) Manifest Wizard | Help a user generate a manifest. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#151](https://github.com/microsoft/winget-cli/issues/151) Delivery Optimization | Delivery Optimization should be leveraged for large Apps. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | [#150](https://github.com/microsoft/winget-cli/issues/150) Metered Networks | The client should download responsibly when on metered networks. |
| [1.0 Milestone](https://github.com/microsoft/winget-cli/milestone/1) | Accessibility (A11y) | The client will be highly accessible and inclusive. It will expose its contents via [UIA](https://docs.microsoft.com/dotnet/framework/ui-automation/ui-automation-overview) to support tools such as [Windows Narrator](https://support.microsoft.com/help/22798/windows-10-complete-guide-to-narrator), and UI automation tools including [WinAppDriver](https://github.com/Microsoft/WinAppDriver). |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34) | [#161](https://github.com/microsoft/winget-cli/issues/161) Verbosity | Client Verbosity Settings. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34) | [#117](https://github.com/microsoft/winget-cli/issues/117) Microsoft Store | Support for installing Apps from the Microsoft Store. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#158](https://github.com/microsoft/winget-cli/issues/158) App Config Files | Support for silent installers that require a configuration file. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#163](https://github.com/microsoft/winget-cli/issues/163) Dependencies | The client should be able to install package dependencies. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#140](https://github.com/microsoft/winget-cli/issues/140) Install .zip | The client should be able to install programs in a .zip file. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#182](https://github.com/microsoft/winget-cli/issues/182) Install portable app | The client should be able to install portable Apps. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#201](https://github.com/microsoft/winget-cli/issues/201) Specify install directory | The client should be able to install to an alternate directory. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#161](https://github.com/microsoft/winget-cli/issues/161) Client Verbosity Settings | The client should support different verbosity settings. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#147](https://github.com/microsoft/winget-cli/issues/147) Release Channels | Some applications have different release channels and we should support them. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#221](https://github.com/microsoft/winget-cli/issues/221) Native PowerShell | Native PowerShell support for the client. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#164](https://github.com/microsoft/winget-cli/issues/164) Install PWA | Support installing Progressive Web Applications. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#219](https://github.com/microsoft/winget-cli/issues/219) Install Multiple Apps | The client should allow a user to specify multiple apps to install. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#229](https://github.com/microsoft/winget-cli/issues/229) Suppress reboot | The client should allow a user to suppress reboot as a default setting. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#227](https://github.com/microsoft/winget-cli/issues/227) Version specification | The client should allow more variation to specifying package versions for installation. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#225](https://github.com/microsoft/winget-cli/issues/225) Parallel download | The client should support multiple connections per package for download. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#166](https://github.com/microsoft/winget-cli/issues/166) Fonts | The client should support installing fonts. |
| [v.Next](https://github.com/microsoft/winget-cli/milestone/34)  | [#212](https://github.com/microsoft/winget-cli/issues/212) Auto Upgrade | The client should be able to auto upgrade installed apps if configured to do so. |

Feature Notes:

\* Feature Priorities will be influenced by community feedback on issues.
