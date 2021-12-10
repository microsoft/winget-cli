# Windows Package Manager Release Roadmap

## Overview

This document outlines our aspirational roadmap to deliver next release of Windows Package Manager. We anticipate substantial feedback from the community, and as such, this plan is subject to change.

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

## GitHub Milestones

We use [GitHub milestones](https://github.com/microsoft/winget-cli/milestones) to broadly organize what we intend to work on. Dates are only used for sequencing and do not represent a commitment.

| Milestone | Description |
|    ---    |     ---     |
| [v1.1-Client](https://github.com/microsoft/winget-cli/milestone/35) | Work Targeted for v1.1 |
| [v1.2-Client](https://github.com/microsoft/winget-cli/milestone/36) | Work Targeted for v1.2 |
| [v.Next-Client](https://github.com/microsoft/winget-cli/milestone/34) | Triage for the next Milestone |
| [Backlog-Client](https://github.com/microsoft/winget-cli/milestone/2) | Work not yet assigned to a milestone or release |

## Issue Triage & Prioritization

Incoming issues/asks/etc. are triaged several times a week, labelled appropriately, and assigned to a milestone in priority order:

* P0 (serious crashes, data loss, etc.) issues are scheduled to be dealt with ASAP.
* P1/2 issues/features/asks assigned to the current or future milestone.
* Issues/features/asks not on our list of the features of next release is assigned to the [Windows Package Manager Backlog](https://github.com/microsoft/winget-cli/milestone/2) for subsequent triage, prioritization & scheduling.

<!--
## v1.1 Scenarios

The following are a list of the key scenarios we're aiming to deliver for Windows Package Manager v1.1.

> Note: There may be features that don't fit within v1.1, but will be re-assessed and prioritized for a future release, the plan for which will be published later in 2021. As features become more defined, links to the associated issues will be added.

| Release or Milestone | Feature | Description/Notes |
|         ---          |   ---   |        ---        |
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
-->

#### Feature Priorities will be influenced by community feedback on issues.
