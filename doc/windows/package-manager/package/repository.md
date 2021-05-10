---
title: Submit your manifest to the repository
description: After you create a package manifest that describes your application, you're ready to submit your manifest to the Windows Package Manager repository.
ms.date: 04/29/2020
ms.topic: article
ms.localizationpriority: medium
---

# Submit your manifest to the repository

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

After you create a [package manifest](manifest.md) that describes your application, you're ready to submit your manifest to the Windows Package Manager repository. This a public-facing repository that contains a collection of manifests that the **winget** tool can access. To submit your manifest, you'll upload it to the open source [https://github.com/microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs) repository on GitHub.

After you submit a pull request to add a new manifest to the GitHub repository, an automated process will validate your manifest file and check to make sure the package is not known to be malicious. If this validation is successful, your package will be added to the public-facing Windows Package Manager repository so it can be discovered by the **winget** client tool. Note the distinction between the manifests in the open source GitHub repository and the public-facing Windows Package Manager repository.

> [!IMPORTANT]
> Microsoft reserves the right to refuse a submission for any reason.

## Third-party repositories

There are currently no known third party repositories. Microsoft is working with multiple partners to develop protocols or an API to enable third party repositories.

## Manifest validation

When you submit a manifest to the [https://github.com/microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs) repository on GitHub, your manifest will be automatically validated and evaluated for the safety of the Windows ecosystem. Manifests may also be reviewed manually.

## How to submit your manifest

To submit a manifest to the repository, follow these steps.

### Step 1: Validate your manifest

The **winget** tool provides the [validate](..\winget\validate.md) command to confirm that you have created your manifest correctly. To validate your manifest, use this command.

```CMD
winget validate \<manifest-file>
```

If your validation fails, use the errors to locate the line number and make a correction. After your manifest is validated, you can submit it to the repository.

### Step 2: Clone the repository

Next, create a fork of the repository and clone it.

1. Go to [https://github.com/microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs) in your browser and click **Fork**.
    ![picture of fork](images\fork.png)

2. From a command line environment such as the Windows Command Prompt or PowerShell, use the following command to clone your fork.
    ```CMD
    git clone \<your-fork-name>
    ```

 3. If you are making multiple submissions, make a branch instead of a fork. We currently allow only one manifest file per submission.
    ```CMD
    git checkout -b \<branch-name>
    ```

### Step 3: Add your manifest to the local repository

You must add your manifest file to the repository in the following folder structure:

**manifests** / **publisher** / **application** / **version.yaml**

* The **manifests** folder is the root folder for all manifests in the repository.
* The **publisher** folder is the name of the company that publishes the software. For example, **Microsoft**.
* The **application** folder is the name of the application or tool. For example, **VSCode**.
* **version.yaml** is the file name of the manifest. The file name must be set to the current version of the application. For example, **1.0.0.yaml**.

>[!IMPORTANT]
> The `PackageIdentifier` value in the manifest must match the publisher and application names in the manifest folder path, and the `PackageVersion` value in the manifest must match the version in the file name. For more information, see [Create your package manifest](manifest.md#tips-and-best-practices).

### Step 4: Submit your manifest to the remote repository

You're now ready to push your new manifest to the remote repository.

1. Use the `add` command to prepare for submission.
    ```CMD
    git add manifests\Contoso\ContosoApp\1.0.0.yaml
    ```

2. Use the `commit` command to commit the change and provide information on the submission.
    ```CMD
    git commit -m "Submitting  ContosoApp version 1.0.0.yaml"
    ```

3. Use the `push` command to push the changes to the remote repository.
    ```CMD
    git push
    ```

### Step 5: Create a pull request

After you push your changes, return to [https://github.com/microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs) and create a pull request to merge your fork or branch to the main branch.

![picture of pull request tab](images\pull-request.png)

## Validation process

When you create a pull request, this will start an automation process that validates the manifest and processes your pull request. We add labels to your pull request so you can track progress.

### Submission expectations

All application submissions to the Windows Package Manager repository should be well-behaved. Here are some expectations for submissions:

* The manifest complies with the [schema requirements](manifest.md#manifest-contents).
* All URLs in the manifest lead to safe websites.
* The installer and application are virus free. The package may be identified as malware by mistake. If you believe it's a false positive you can submit the installer to the defender team for analysis from [here](https://www.microsoft.com/wdsi/filesubmission).
* The application installs and uninstalls correctly for both administrators and non-administrators.
* The installer supports non-interactive modes.
* All manifest entries are accurate and not misleading.
* The installer comes directly from the publisher's website.

### Pull request labels

During validation, we apply a series of labels to our pull request to communicate progress.

* **Needs: author feedback**: There is a failure with the submission. We will reassign pull request back to you. If you do not address the issue within 10 days, we will close the pull request.
* **Manifest-Validation-Error**: The submitted manifest contains a syntax error.
* **URL-Validation-Error**: One or more URLs in the submission failed [SmartScreen](/windows/security/threat-protection/microsoft-defender-smartscreen/microsoft-defender-smartscreen-overview) validation.
* **Binary-Validation-Error**: The submitted application installer failed virus scan testing or there is a hash mismatch.
* **Pull-Request-Error**: There is a problem with the pull request. For example, the folder structure does not have the [required format](#step-3-add-your-manifest-to-the-local-repository).
* **Validation-Error**: The submitted application failed a general validation test.
* **Validation-Installation-Error**: The submitted application failed install testing.
* **Validation-Uninstall-Error**: The submitted application failed uninstall testing.
* **Validation-Virus-Scan-Error**: The submitted application failed virus scan testing.
* **Azure-Pipeline-Passed**: The manifest has completed the first portion of validation. After this step, your pull request is assigned to our test team for final validation.
* **Validation-Completed**: The validation is complete and your pull request will be merged.