## Validation process

When you create a pull request, this will start an automation process that validates the manifest and processes your pull request. GitHub labels are used to share progress and allow you to communicate with us.

## Submission expectations

All application submissions to the Windows Package Manager repository should be well-behaved and adhere to the [Windows Package Manager policies](./windows-package-manager-policies.md).
Here are some expectations for submissions:

- The manifest complies with the [schema requirements]("https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema").
- All URLs in the manifest lead to safe websites.

- The installer and application are virus free. The package may be identified as malware by mistake. If you believe it is a false  positive you can submit the installer to the defender team for
 analysis from [here](https://www.microsoft.com/wdsi/filesubmission).

- The application installs and uninstalls correctly for both administrators and non-administrators.

- The installer supports non-interactive modes.

- All manifest entries are accurate and not misleading.

- The installer comes directly from the publisher\'s website.

Please see [Windows Package Manager policies](.\windows-package-manager-policies.md) for a complete list of the policies.

## Pull request labels

During validation, we apply a series of labels to our pull request to
communicate progress. Some labels will direct the ISV to take action,
while others will be directed to the Package Manager developers.

### Status Labels

The following table describes the possible **status labels** you will
encounter:

| **Label** | **Details** |
|--------------|-------------|
| <img width=180/> | <img width=500/> |
| **Azure-Pipeline-Passed** | The manifest has completed the test pass. It is waiting for approval. If no issues are encountered during the test pass it will automatically be approved. If a test fails, it may be flagged for manual review.|
| **Blocking-Issue** | This label indicates that the **Pull Request** cannot be approved because there is a blocking issue. You can often tell what the blocking issue is by the included error label as well. |
| **Needs: Attention** | This label indicates that the **Pull Request** needs to be investigated by the Windows Package Manager development team. This is either due to a test failure that needs manual review, or a comment added to the **Pull Request** by the community. |
| **Needs: author feedback** | Indicates there is a failure with the submission. We will reassign **Pull Request** back to you. If you do not address the issue within 10 days, the bot will close the **pull request**. **Needs: author feedback** labels are typically added when there was a failure with the Pull Request that should be updated, or if the person reviewing the Pull Request has a question. |
| **Validation-Completed** | Indicates that the test pass has been completed successfully and your **Pull Request** will be merged.|

### Error Labels

The following table describes the possible **error labels** that will be
encountered. Not all of the error cases will be assigned to the ISV
immediately. Some may trigger manual validation.


| **Label** | **Details** |
|--------------|-------------|
|<img width=200/>|<img width=500/>|
| **Binary-Validation-Error** | The application included in this **Pull Request** failed to pass the **Installers Scan** test. This test is designed to ensure that the application installs on all environments without warnings.  For further details on this error, see [binary validation errors](.\binary-validation-errors.md). |
| **Error-Analysis-Timeout** | This label indicates that the **Binary-Validation-Test** test timed out. The **Pull Request** will get assigned to a Windows Package Manager developer to look at it. |
| **Error-Hash-Mismatch** | The submitted manifest could not be processed because the **InstallerSha256** hash provided for the **InstallerURL** did not match. Update the **InstallerSha256** in the **Pull Request** and try again. |
| **Error-Installer-Availability** | The validation service was unable to download the installer. This may be related to Azure IP ranges being blocked, or the installer URL may be incorrect. Check that the **InstallerURL** is correct and try again. If you feel this has failed in error, please add a comment and the **Pull Request** will get assigned to a Windows Package Manager developer to look investigate. |
| **Manifest-Path-Error** | The manifest files must be put into a specific folder structure. This label indicates a problem with the path of your submission. For example, the folder structure does not have the [required format](https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema). Update your manifest and path resubmit your **Pull Request**. |
| **Manifest-Validation-Error** | The submitted manifest contains a syntax error. Address the syntax issue with the manifest and re-submit. For details on the manifest format and schema see: [required format](https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema). |
| **PullRequest-Error** | The pull request is invalid because not all files submitted are under manifest folder or there is more than one package or version in the **Pull Request**. Update your **Pull Request** to address the issue and try again. |
| **URL-Validation-Error** | The **URLs Validation Test** could not locate the URL and responded with a [HTTP error status code](https://docs.microsoft.com/troubleshoot/iis/http-status-code) (403 or 404), or the URL reputation test failed. You can identify which URL is in question by looking at the [Pull Request check details](.\winget-validation-troubleshooter.md). To address this issue, update the URLs in question to resolve the [HTTP error status code](https://docs.microsoft.com/troubleshoot/iis/http-status-code). If the issue is not due to [HTTP error status code](https://docs.microsoft.com/troubleshoot/iis/http-status-code) then you can [submit the URL for review](https://www.microsoft.com/wdsi/filesubmission/) to avoid the reputation failure. |
| **Validation-Defender-Error** | During dynamic testing, Defender reported a problem. To reproduce this problem, install your application, then run a Defender full scan. If you can reproduce the problem, either fix the binary, or submit to this URL for false positive assistance. As stated in the following article,[Address false positives/negatives in Microsoft Defender for Endpoint Microsoft Docs](https://docs.microsoft.com/microsoft-365/security/defender-endpoint/defender-endpoint-false-positives-negatives?view=o365-worldwide), you can submit your binary for analysis to the [defender analysis web page](https://docs.microsoft.com/microsoft-365/security/defender-endpoint/defender-endpoint-false-positives-negatives?view=o365-worldwide#part-4-submit-a-file-for-analysis). If you are unable to reproduce, add a comment to get the Windows Package Manager developers to look at it. |
| **Validation-Domain** | The test has determined the domain if the **InstallerURL** does not match the domain expected. The Windows Package Manager policies requires that the [InstallerUrl](https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema)comes directly from the ISVs release location. If you believe this is a false detection, add a comment to the **Pull Request** to get the Windows Package Manager developers to look at it. |
| **Validation-Error** | Validation of the Windows Package Manager failed during manual approval. Look at the accompanying comment for next steps. |
| **Validation-Executable-Error** | During installation testing, the test was unable to locate the primary application. Make sure the application installs correctly on all platforms. If your application does not install an application, but should still be included in the repository, add a comment to the **Pull Request** to get the Windows Package Manager developers to look at it.|
| **Validation-Hash-Verification-Failed** | During installation testing, the application fails to install because the **InstallerSha256** no longer matches the **InstallerURL** hash. This can occur if the application is behind a vanity URL and the installer was updated without updating the **InstallerSha256**. To address this issue, update the **InstallerSha256** associated with the **InstallerURL** and submit again. |
| **Validation-HTTP-Error** | The URL used for the installer does not use the HTTPs protocol. Please update the **InstallerURL** to use HTTPS and resubmit the **Pull Request.** |
| **Validation-Indirect-URL** | The URL is not coming directly from the ISVs server. Testing has determined a redirector has been used. This is not allowed because the Windows Package Manager policies require that the[InstallerUrl](https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema) comes directly from the ISVs release location. Remove the redirection and resubmit.
| **Validation-Installation-Error** | During manual validation of this package, there was a general error. Look at the accompanying comment for next steps.|
| **Validation-Merge-Conflict** | This package could not be validated due to a merge conflict. Please address the merge conflict and resubmit your **Pull Request.**
| **Validation-MSIX-Dependency** | The MSIX package has a dependency on package that could not be resolved. Update the package to include the missing components or add the dependency to the manifest file and resubmit the **Pull Request.**|
| **Validation-Unapproved-URL** | The test has determined the domain if the **InstallerURL** does not match the domain expected. The Windows Package Manager policies requires that the [InstallerUrl](https://docs.microsoft.com/windows/package-manager/package/manifest?tabs=minschema%2Ccompschema) comes directly from the ISVs release location. |
| **Validation-Unattended-Failed** |  During installation, the test timed out.This most likely is due to the application not installing silently. It could also be due to some other error being encountered and stopping the test. Verify that you can install your manifest without user input. If you need assistance, add a comment to the **Pull Request** and the Windows Package Manager developers will look at it. |
| **Validation-Uninstall-Error**  | During uninstall testing, the application did not clean up completely following uninstall. Look at the accompanying comment for more details.|
| **Validation-VCRuntime-Dependency** | The package has a dependency on the C++ runtime that could not be resolved. Update the package to include the missing components or add the dependency to the manifest file and resubmit the **Pull Request**. |

### Content Policy Labels

The following table lists **content policy labels**. If one of
the following labels is added, then something in the manifest metadata
triggered additional manual content review to ensure that the metadata
is following the [Windows Package Manager policies](.\windows-package-manager-policies.md).

| **Label** | **Details** |
|--------------|-------------|
|<img width=180/>|<img width=500/>|
| **Policy-Test-2.1** |Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#21-general-content-requirements) |
| **Policy-Test-2.2** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#22-content-including-names-logos-original-and-third-party) |
| **Policy-Test-2.3** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#23-risk-of-harm) |
| **Policy-Test-2.4** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#24-defamatory-libelous-slanderous-and-threatening) |
| **Policy-Test-2.5** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#25-offensive-content) |
| **Policy-Test-2.6** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#26-alcohol-tobacco-weapons-and-drugs) |
| **Policy-Test-2.7** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#27-adult-content) |
| **Policy-Test-2.8** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#28-illegal-activity) |
| **Policy-Test-2.9** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#29-excessive-profanity-and-inappropriate-content) |
| **Policy-Test-2.10** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#210-countryregion-specific-requirements) |
| **Policy-Test-2.11** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#211-age-ratings) |
| **Policy-Test-2.12** | Manual review triggered see [Windows Package Manager Policies](.\windows-package-manager-policies.md#212-user-generated-content) |

### Internal Labels

The following table lists the **internal errors**. When internal errors are encountered the **Pull Request** will be assigned to the Windows Package
Manager developers to investigate:

| **Label** | **Details** |
|--------------|-------------|
|<img width=180/>|<img width=500/>|
|**Internal-Error-Domain**|During the domain validation of the URL, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-Dynamic-Scan**|During the validation of the installed binaries, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-Keyword-Policy**| During the validation of the manifest, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-Manifest**| During the validation of the manifest, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-NoArchitectures**| Testing encountered and issue where the test could not determine the architecture if the application. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-NoSupportedArchitectures**| Testing encountered and issue where the current architecture is not supported. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-PR**| An error occurred during the processing of the PR. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-Static-Scan**| During static analysis of the installers, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error-URL**| During reputation validation of the installers, the test encountered an issue. A Windows Package Manager developer will take a look at it.|
|**Internal-Error**| This indicates a generic failure or unknown error was encountered during the test pass. A Windows Package Manager developer will take a look at it.|
