---
author: Amrutha Srinivasan @amrutha95
created on: 2020-08-19
last updated: 2020-08-19
issue id: 164
---

# Spec Title

For [#164](https://github.com/microsoft/winget-cli/issues/164)

## Abstract

This feature adds PWA (Progressive Web Application) support to winget. The goal is to allow users to install and maintain PWAs just like they would any other type of application on winget. As a first step, this would be available as an "experimental" feature to make it available to the community and receive feedback.

## Inspiration

PWA adoption is extremely important in the Windows app ecosystem. It is also crucial that they are treated as first class citizens in the app ecosystem, just like any native application. Providing PWA support for winget is key to achieving this goal.

## Solution Design

The "InstallerType" field in the YAML manifest file specifies the type of the application. A new InstallerType "PWA" will be added to show that a given application is a PWA.

## Package generation

A POST call is made to a web service with the following details in the body : Id, Version and URL. The web service packages the PWA and its assets into an MSIX that will be unsigned.

## Package installation

The unsigned MSIX package and any other dependency packages are added for the current user and the application is silently installed on the user's system.

## Flow of the install process

1. User types the install command, eg. `winget install starbucks`
2. The Id, Version and URL from the manifest YAML file of the Starbucks PWA will be sent as a POST request to an internal web service that will generate an unsigned MSIX package
3. The generated package will then be silently installed as a Hosted App (Refer to resources for more on the Hostel App Model).

## UI/UX Design

Installing a PWA will be similar to installing other application types currently supported by winget, using the install command. The only difference in experience might be if the user passes a -i flag to get a more interactive experience. When this flag is passed, the user will be prompted to allow the PWA to be launched on install. This feature is provided because in order to register a PWA with Edge, it has to be launched once.

An example of the interactive install would look like this:

```
>winget install -i starbucks
Found Starbucks [PWAtest.Starbucks]
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Starting package install...
Succesfully installed! Launching app now. This is a necessary step to complete registration with Edge.
(App launches on Edge)
```

## Capabilities

### Accessibility

This should have no direct impact on accessibility.

### Security

No direct impact on security.

### Reliability

This is not expected to impact reliability. It will be available as an experimental feature so users can disable it any time.

### Compatibility

This is an additional functionality built on top of the existing implementation to allow the install of PWAs. Therefore we don't anticipate any breaking changes to the existing implementation.

### Performance, Power, and Efficiency

## Potential Issues

If the user navigates to the site before launching the installed PWA at least once (i.e. before registration with Edge), Edge might prompt them to install the PWA again.

## Future considerations

1. We are looking into where the catalog of PWAs should exist, and the pros and cons of maintaining a separate catalog vs including PWAs on the existing catalog.
2. In the future, we can extend this implementation to support browsers other than Edge. However, that depends on when other browsers enable silent installs of PWAs.
3. Sometimes there may be multiple versions of a PWA available. In such scenarios, we may allow a "force" option in the case where a user wants all of them to be installed.

## Resources

Hosted App Model : https://blogs.windows.com/windowsdeveloper/2020/03/19/hosted-app-model/
