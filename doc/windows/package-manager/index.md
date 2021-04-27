---
title: Windows Package Manager
description: Windows Package Manager is a comprehensive package manager solution that consists of a command line tool and set of services for installing applications on Windows 10.
ms.date: 05/03/2020
ms.topic: overview
ms.localizationpriority: medium
---

# Windows Package Manager (preview)

[!INCLUDE [preview-note](../includes/package-manager-preview.md)]

Windows Package Manager is a comprehensive [package manager solution](#understanding-package-managers) that consists of a command line tool and set of services for installing applications on Windows 10.

## Windows Package Manager for developers

Developers use the **winget** command line tool to discover, install, upgrade, remove and configure a curated set of applications. After it is installed, developers can access **winget** via the Windows Terminal, PowerShell, or the Command Prompt.

For more information, see [Use the winget tool to install and manage applications](winget/index.md).

## Windows Package Manager for ISVs

Independent Software Vendors (ISVs) can use Windows Package Manager as a distribution channel for software packages containing their tools and applications. To submit software packages (containing .msix, .msi, or .exe installers) to Windows Package Manager, we provide the open source **Microsoft Community Package Manifest Repository** on GitHub where ISVs can upload [package manifests](package/manifest.md) to have their software packages considered for inclusion with Windows Package Manager. Manifests are automatically validated and may also be reviewed manually.

For more information, see [Submit packages to Windows Package Manager](package/repository.md).

## Understanding package managers

A package manager is a system or set of tools used to automate installing, upgrading, configuring and using software. Most package managers are designed for discovering and installing developer tools.

Ideally, developers use a package manager to specify the prerequisites for the tools they need to develop solutions for a given project. The package manager then follows the declarative instructions to install and configure the tools. The package manager reduces the time spent getting an environment ready, and it helps ensure the same versions of packages are installed on their machine.

Third party package managers can leverage the [Microsoft Community Package Manifest Repository](package/repository.md) to increase the size of their software catalog.

## Related topics

* [Use the winget tool to install and manage software packages](winget/index.md)
* [Submit packages to Windows Package Manager](package/index.md)
