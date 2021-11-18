---
author: Kaleb Luedtke Trenly/trenlymc@gmail.com
created on: 2021-11-18
last updated: 2021-11-18
issue id: 147
---

# Release Channels

"For [#147](https://github.com/microsoft/winget-cli/issues/147)"

## Abstract

Several packages have different channels for releasing different versions of their software. The Windows Package Manager should be able to support packages of the same Package Identifier where the versions come from different release channels. The Windows Package Manager should be also able to inform a user about which channels are available.

## Inspiration

Many other package managers have this capability. In the current state, a new package identifier must be created for each release channel which can cause conflicts when the versions are not side-by-side compatible.

## Solution Design

The Windows Package Manager manifest v1.1.0 schema has provided a key for declaring which channel an installer belongs to. An additional key for identifying the Default Channel should be added to the manifest v1.1.0 schema




[comment]: # Outline the design of the solution. Feel free to include ASCII-art diagrams, etc.

## UI/UX Design
Where not specified, the default release channel should be used/shown first or exclusively, then other channels in ASCII sort order

### Searching for package version with release channel
```raw
PS> winget search Google.Chrome

Name                       Id                         Version        Channel
-----------------------------------------------------------------------------
Google Chrome              Google.Chrome              96.0.4664.45   Stable
Chrome Remote Destop Host  Google.ChromeRemoteDesktop 96.0.4664.39   Stable

PS> winget search Google.Chrome --channel dev

Name           Id             Version        Channel
----------------------------------------------------
Google Chrome  Google.Chrome  97.0.4692.20   Dev
```

### Showing available release channels
```raw
PS> winget show Google.Chrome --channels
Found Google Chrome [Google.Chrome]
Channel
-------
Stable
Beta
Canary
Dev
```

### Showing available versions
```raw
PS> winget show Google.Chrome --versions
Found Google Chrome [Google.Chrome]
Version      Channel
--------------------
96.0.4664.45 Stable
96.0.4667.49 Stable
96.0.4664.45 Beta
98.0.4708.0  Canary
97.0.4692.20 Dev
```

### Installing with release channel
```raw
PS> winget install Google.Chrome
Found Google Chrome [Google.Chrome] Version 96.0.4664.45
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://dl.google.com/dl/chrome/install/googlechromestandaloneenterprise64.msi
  ██████████████████████████████  78.8 MB / 78.8 MB
Successfully verified installer hash
Starting package install...
Successfully installed

PS> winget install Google.Chrome --channel dev
Found Google Chrome [Google.Chrome (Dev)] Version 97.0.4692.20
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://dl.google.com/tag/s/dl/chrome/install/dev/googlechromedevstandaloneenterprise64.msi
  ██████████████████████████████  78.4 MB / 78.4 MB
Successfully verified installer hash
Starting package install...
Successfully installed
```

### Listing installed packages
```raw
PS> winget list
Name                                    Id                                       Version            Available    Channel Source
-------------------------------------------------------------------------------------------------------------------------------
Microsoft Edge                          Microsoft.Edge                           92.0.902.67        95.0.1020.53         winget
Microsoft Edge Update                   Microsoft Edge Update                    1.3.147.37
App Installer                           Microsoft.DesktopAppInstaller_8wekyb3d8… 1.16.12653.0                    Stable 
Microsoft Edge                          Microsoft.MicrosoftEdge.Stable_8wekyb3d… 92.0.902.67                     Stable
Google Chrome                           Google.Chrome                            96.0.4664.45       97.0.4692.20 Dev     winget
```

## Capabilities

[comment]: # Discuss how the proposed fixes/features impact the following key considerations:

### Accessibility

The Windows Package Manager has been built in such a way that screen readers will still provide audible output as the command is executed keeping the user informed of progress, warnings, and errors. This should have no direct impact on accessibility.

### Security

There should be no security impact directly, although we must remember that different sources may not guarantee the safety of packages. The Windows Package Manager community repository performs static and dynamic analysis, and in some cases additional manual validation before accepting a package.

### Reliability

[comment]: # Will the proposed change improve reliability? If not, why make the change?

### Compatibility

The current implementation for different release channels is to have separate package identifiers for each package. Packages which have channel-specific identifiers currently existing in the community repository /should/ be updated to use this feature, but will not be required to.

The current implementation of automation - specifically @wingetbot - does not have support for release channels. Any package which implements release channels will likely not be able to undergo automatic upgrade unless and until the automation considers this feature.

### Performance, Power, and Efficiency

The time needed to return search results is directly related to the number of packages included in the scope of the search. This feature will likely remove extraneous packages as they are combined into manifests with release channel support

## Potential Issues

[comment]: # What are some of the things that might cause problems with the fixes/features proposed? Consider how the user might be negatively impacted.

## Future considerations

[comment]: # What are some of the things that the fixes/features might unlock in the future? Does the implementation of this spec enable scenarios?

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.
