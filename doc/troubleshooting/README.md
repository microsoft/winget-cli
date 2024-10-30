# Troubleshooting

## How do I get the Windows Package Manager?

### Prerequisites

The first thing to check is which version of Windows 10 you have.

[Check your version of Windows](https://support.microsoft.com/windows/see-which-version-of-windows-10-you-have-12d35019-4da9-0cb1-ba47-f8b031b712ad).

The Windows Package Manager requires at least Version 1809 (October 2018 Update).

The next requirement is ensuring you have the [App Installer](https://apps.microsoft.com/detail/9nblggh4nns1) from the Microsoft Store. The Windows Package Manager is delivered as an MSIX package. The App Installer is required to install MSIX packages on Windows 10.

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

### Machine-wide Provisioning

The Windows Package Manager can be provisioned machine-wide or for each new user. The following PowerShell cmdlet can be used to provision the package machine-wide. The latest Windows Package Manager release and license can be downloaded directly from the GitHub repository. It is dependent on the [Microsoft.VCLibs](https://docs.microsoft.com/troubleshoot/cpp/c-runtime-packages-desktop-bridge) desktop framework package, which needs to be downloaded and specified in the dependency path option in the cmdlet.

  >`Add-AppxProvisionedPackage -online -PackagePath <desktop AppInstaller msixbundle package path> -LicensePath <license path> -DependencyPackagePath <Microsoft.VCLibs package path>`

After the package is provisioned, the users need to log into their Windows account to get the package registered and use it.

## Common Issues

### Executing `winget` exits with no message

If no output is displayed, it is likely that the version of WinGet on your system is using a retired Content Delivery Network (CDN).
You can check which version of WinGet is on your machine using `winget --info`. If the version is lower than `1.6.3482`, take the following troubleshooting steps.

1. Install the latest version of WinGet using one of the below methods
  * a. Through the Microsoft Store by installing the latest version of [App Installer](https://apps.microsoft.com/detail/9NBLGGH4NNS1)
  * b. Through installing the MSIX package found in the [GitHub releases](https://github.com/microsoft/winget-cli/releases)
  * c. Through installing the MSIX package from https://aka.ms/getwinget
2. Force a source update using `winget source update`

If the above guidelines do not resolve the problem, please open an issue with details of the Windows version and App Installer version you are using.

### Executing `winget` doesn't display help

The following errors are displayed when executed in CMD.
 `The system cannot execute the specified program.`
 or

```
'winget' is not recognized as an internal or external command,
operable program or batch file.
```

The following errors are displayed when executed in PowerShell.

```
winget : The term 'winget' is not recognized as the name of a cmdlet, function, script file, or operable program.
Check the spelling of the name, or if a path was included, verify that the path is correct and try again.
At line:1 char:1
+ winget
+ ~~~~~~
    + CategoryInfo          : ObjectNotFound: (winget:String) [], CommandNotFoundException
    + FullyQualifiedErrorId : CommandNotFoundException
```
or

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

The following error is displayed when executed in Windows Terminal's Powershell profile.

```
winget : The term 'winget' is not recognized as the name of a cmdlet, function, script file, or operable program.
Check the spelling of the name, or if a path was included, verify that the path is correct and try again.
At line:1 char:1
+ winget
+ ~~~~~~
    + CategoryInfo          : ObjectNotFound: (winget:String) [], CommandNotFoundException
    + FullyQualifiedErrorId : CommandNotFoundException
```

These errors most commonly occur for one of following reasons. Please try out the following troubleshooting steps.
1. The App Installer does not contain the Windows Package Manager. You should check to ensure the version of App Installer is greater than 1.11.11451. You can check by executing the following command in PowerShell:

    >`Get-AppxPackage microsoft.desktopappinstaller`

2. The App Execution Alias for the Windows Package Manager is disabled. You should enable the App Execution Alias for the Windows Package Manager. Go to `App execution aliases` option in `Apps & features Settings` to enable it.
3. The App Installer did not automatically add the PATH environment variable. You should add the path environment variable. The value to add is "%userprofile%\AppData\Local\Microsoft\WindowsApps". You can verify this by running `%LOCALAPPDATA%\Microsoft\WindowsApps\winget` from a command-prompt or  `& "$env:LOCALAPPDATA\Microsoft\WindowsApps\winget"` from a powershell. If the command runs then, but not before, then you very likely have a missing PATH environment component.
4. Apps deployed on the machine are registered per user by default. If App Installer was installed on a different user account than the one you are trying to run it on, you will have to reinstall it again on this account and try again:
   1. Get the `PackageFullName` of your installed `App Installer` package (PowerShell): `Get-AppxPackage Microsoft.DesktopAppInstaller | Select Name, PackageFullName`.
   2. `Add-AppxPackage -register "C:\Program Files\WindowsApps\{PackageFullName}\appxmanifest.xml" -DisableDevelopmentMode` (where `{PackageFullName}` is the info from the previous point).
   3. Toggle the App Execution Alias for winget, again (see above).

If the above guidelines do not resolve the problem, please open an issue with details of the Windows version and App Installer version you are using.

## Common Errors

#### Error 0x80072efd

This error is related to networking and maps to "ERROR_INTERNET_CANNOT_CONNECT". It could be related to TLS (Transport Layer Security).

This issue may be resolved by enabling TLS 1.2.

It may also be resolved by flushing your DNS cache. Instructions are available at [Microsoft Learn](https://learn.microsoft.com/windows-server/administration/windows-commands/ipconfig).


#### Error 0x801901a0

This error is related to Delivery Optimization (DO). You may configure the Windows Package Manager settings to use the standard `WININET` library. Add the following network setting:

>`"network": {"downloader": "wininet"}`

#### Error 0x80d03002

This error is related to Delivery Optimization (DO). You may configure the Windows Package Manager settings to use the standard `WININET` library. Add the following network setting:

>`"network": {"downloader": "wininet"}`

#### Error 0x80070490

The following error is displayed when trying to install some appxbundles.

```
Install failed: error 0x80070490: Opening the package from location appxbundle_Name.appxbundle failed.
0x80070490 : Element not found.
```

A possible troubleshooting step is to install the [KB5005565](https://support.microsoft.com/topic/september-14-2021-kb5005565-os-builds-19041-1237-19042-1237-and-19043-1237-292cf8ed-f97b-4cd8-9883-32b71e3e6b44) update, reboot your machine and try installing the appxbundle again.

#### Updating Microsoft.DesktopAppInstaller from version 1.0.42251.0 doesn't render package name on Windows 11 Pro image in Azure

Launching the Microsoft.DesktopAppInstaller update on Windows 11 Pro image in Azure to update from version 1.0.42251.0 might not render the correct package name title. The workaround to this issue is to install the latest Microsoft.DesktopAppInstaller with Windows Package Manager.
