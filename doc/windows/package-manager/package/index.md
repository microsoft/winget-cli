---
title: Submit packages to Windows Package Manager
description: You can use Windows Package Manager as a distribution channel for software packages containing your applications.
ms.date: 04/29/2020
ms.topic: overview
ms.localizationpriority: medium
---

# Submit packages to Windows Package Manager

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

## Independent Software Vendor (ISV) or Publisher

If you are an ISV or Publisher, you can use Windows Package Manager as a distribution channel for software packages containing your applications. Windows Package Manager currently supports installers in the following formats: MSIX, MSI, and EXE.

To submit software packages to Windows Package Manager, follow these steps:

1. [Create a package manifest that provides information about your application](manifest.md). Manifests are YAML files that follow the Windows Package Manager schema.
2. [Submit your manifest to the Windows Package Manager repository](repository.md). This is an open source repository on GitHub that contains a collection of manifests that the **winget** tool can access.

## Community Member

If you are a GitHub community member, you may also submit packages to Windows Package Manager following the steps above.

Optionally, you may also request help to have a package added to the [community repository](https://github.com/microsoft/winget-pkgs). To do so, create a new [Package Request/Submission](https://github.com/microsoft/winget-pkgs/issues/new/choose) Issue.

## Related topics

* [Use the winget tool](../winget/index.md)
* [Create your package manifest](manifest.md)
* [Submit your manifest to the repository](repository.md)
