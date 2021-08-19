# Troubleshooting

## How do I get the Windows Package Manager?

### Prerequisites

The first thing to check is which version of Windows 10 you have.

[Check your version of Windows](https://support.microsoft.com/windows/see-which-version-of-windows-10-you-have-12d35019-4da9-0cb1-ba47-f8b031b712ad).

The Windows Package Manager requires at least Version 1809 (October 2018 Update).

The next requirement is ensuring you have the [App Installer](https://www.microsoft.com/p/app-installer/9nblggh4nns1) from the Microsoft Store. The Windows Package Manager is delivered as an MSIX package. The App Installer is required to install MSIX packages on Windows 10.

>Note: The Windows Package Manager is shipped with later versions of the App Installer.

### Stable Releases

The first stable release of the Windows Package Manager was v1.0.11451. This release was **not** published as an automatic update to all Windows 10 users on a supported version of Windows 10. This was an intentional decision made to provide enterprise customers (IT Professionals) sufficient time to configure and deploy Group Policy for the Windows Package Manager.

Customers may install the [latest stable release](https://github.com/microsoft/winget-cli/releases/latest/) directly from the GitHub repository. These packages are signed, and customers will receive automatic updates if IT Policy does not block the Microsoft Store.

### Developer Releases (Pre-Release)

>Note: There is a known problem restoring the client to the latest stable App Installer release. We will be distributing the latest stable builds and providing instructions once they are made available. 

During the initial Windows Package Manager Preview period, releases were distributed to all Windows Insider channels. Customers who [sign up](http://aka.ms/winget-InsiderProgram) to become members of the Windows Package Manager Insider program also receive pre-release builds. The final process for inclusion into the program requires manual steps, so the App Installer update may not be available for a few days **after** receiving their e-mail notification.

Customers may install any [release](https://github.com/microsoft/winget-cli/releases/) including pre-release builds directly from the GitHub repository. These packages are signed, and customers will receive automatic updates if IT Policy does not block the Microsoft Store.

>Note: Insiders will receive updates to the latest build (stable or pre-release) if IT Policy does not block the Microsoft Store. Other customers will receive updates to the latest stable build once a newer stable version is published if IT Policy does not block the Microsoft Store.

Only the Windows Insider DEV channel will continue receiving pre-release builds of the Windows Package Manager after v1.0.11451. Other Windows Insider channels will only receive stable release candidates or updated versions of the Windows Package Manager with critical bug fixes.

## Common Issues

### Executing `winget` doesn't display help

The following error is displayed when executed in CMD. `The system cannot execute the specified program.`

The following error is displayed when executed in PowerShell.

```
Program 'winget.exe' failed to run: The file cannot be accessed by the systemAt line:1 char:1
+ winget
+ ~~~~~~.
At line:1 char:1
+ winget
+ ~~~~~~
    + CategoryInfo          : ResourceUnavailable: (:) [], ApplicationFailedException
    + FullyQualifiedErrorId : NativeCommandFailed
```

These errors most commonly occur for one of three reasons.
1. The App Installer does not contain the Windows Package Manager. You should check to ensure the version of App Installer is greater than 1.11.11451. You can check by executing the following command in PowerShell: 

    >`Get-AppxPackage microsoft.desktopappinstaller`

2. The App Execution Alias for the Windows Package Manager is disabled. You should enable the App Execution Aias for the Windows Package Manager
3. The App Installer did not automatically add the PATH environment variable. You should add the path environment variable. The value to add is "%userprofile%\AppData\Local\Microsoft\WindowsApps".

## Common Errors


#### Error 0x801901a0

This error is related to Delivery Optimization (DO). You may configure the Windows Package Manager settings to use the standard `WININET` library. Add the following network setting:

>`"network": {"downloader": "wininet"}`

#### Error 0x80d03002

This error is related to Delivery Optimization (DO). You may configure the Windows Package Manager settings to use the standard `WININET` library. Add the following network setting:

>`"network": {"downloader": "wininet"}`
