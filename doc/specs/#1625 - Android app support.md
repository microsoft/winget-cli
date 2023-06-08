---
author: @upintheairsheep
created on: 2023-05-23
last updated: 2023-05-23
issue id: 1625
---

# Spec Title

For [#1625](https://github.com/microsoft/winget-cli/issues/1625)

## Abstract

This feature adds Android application support to winget via Windows Subsystem for Android. The goal is to allow users to install and maintain Android applications just like they would any other type of application on winget, and for more community-driven Android apps to be available via Windows.

## Inspiration

Android is by far the most used operating system on the planet, excluding a few certain regions, where iOS dominates. Many mobile developers make apps for Android, and only have a sometimes inferior web app for desktop, if they have one at all.

## Solution Design

### System requirements

Windows 11 or newer  
Windows Subsystem for Android
ADB (Android Debug Bridge) (if it ends up being the method of installation)

### Android Installer Type manifest

The "InstallerType" field in the YAML manifest file specifies the type of the application. A new InstallerType "APK" will be added to show that a given application is an Android app.

The "InstallerType" enumeration would include "APK" to indicate an Andoroid package. A sample installer manifest file would look like the following:

```
PackageIdentifier: FDroid.FDroid
Version: 1337.0
Name: F-Droid
Publisher: F-Droid
AppMoniker: fdroid
License: Test
InstallerType: Android
Installers:
- Arch: neutral
  Url: https://f-droid.org/F-Droid.apk
  InstallerType: Android
  MinAndroidPlat: 31
  ADBFlags: 
ManifestVersion: 0.1.0
```

### Package generation

WinGetCreate will download the package and extract metadata from AndroidManifest.xml, automatically filling out all data excluding the short desciption

### Package installation

Either ADB could be used to install the application, or the method that Microsoft Store installs Android apps could be utilized.
### Flow of the install process

1. Windows Subsystem for Android is installed
2. ADB must be installed on the machine
3. It must connect to the Subsystem, bypassing the developer mode restriction, which I think the uninstallation in control panel already has.
A nice feature in the manifest would be for adding the adb install [flags](https://adbshell.com/commands/adb-install) for installation of apps that require it.
## UI/UX Design


An example of the interactive install would look like this:

```
>winget install fdroid
Found F-Droid [FDroid.FDroid]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Starting Windows Subsystem for Android...
Connecting to Windows Subsystem for Android via adb...
Starting package install...
<Progress bar> (if ADB supports)
Successfully installed!
```

## Capabilities

# A field to determine minumum platform SDK version of Android is required for the app to run on like 27 (Oreo), 30 (Red Velvet Cake), 31 (Snow Cone), 32 (12L), or 33 (Tiramisu)
# A field for ADB flags to be put, like test apps.
### Accessibility

This should have no direct impact on accessibility.

### Security

Android has security concerns.
### Reliability

This is not expected to impact reliability.

### Compatibility


### Performance, Power, and Efficiency

Installing some or most Android apps leaves a service running in the background, for stuff such as notifications.
## Potential Issues


## Future considerations

1. Android 14 "Upside Down Cake" will restrict the installation of apps that target any version before 6.0 "Marshmallow" due to security concerns, however --bypass-low-target-sdk-block can bypass this limitation. Similar, certain new Android devices, such as the Pixel 7 and Pixel Tablet, have no support for 32-bit apps and will not be able to run 32 bit code.
2. Blocking issues may be present for the package repository, including the lack of Google play service replacements, the lack of support for a certain protocol, or the lack of optional root access.

## Resources


