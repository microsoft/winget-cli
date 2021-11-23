---
author: Kaleb Luedtke - Trenly <trenlymc@gmail.com>
created on: 2021-11-18
last updated: 2021-11-20
issue id: 147
---

# Release Channels

For [#147](https://github.com/microsoft/winget-cli/issues/147)

## Abstract

Several packages have different channels for releasing different versions of their software. The Windows Package Manager should be able to support packages of the same Package Identifier where the versions come from different release channels. The Windows Package Manager should be also able to inform a user about which channels are available.

## Inspiration

Some other package managers have this capability. In the current state, a new package identifier must be created for each release channel which can cause conflicts when the versions are *not* side-by-side compatible.

## Solution Design

### Manifest Structure
The Windows Package Manager manifest v1.1.0 schema has provided a key for declaring which channel an installer belongs to. However, this has the potential to make the installer manifests extremely complex when there are multiple release channels with multiple installers each.
> The `Channel` key should be removed from the manifest v1.1.0 schema.

Instead, the use of multiple installer manifest files will be used to define and manage the various channels available for a package in a way similar to locales. This will ensure that the client remains backwards compatible with old manifest versions, and will make the creation and maintenance of the various channels easier for contributors in the community repository. This will require a key be added to the  manifest schema. Channels should not be compatible with singleton manifests.
> The `DefaultChannel` key should be added to the manifest v1.1.0 schema for version manifests

An example of the manifest file structure, with installer channel manifests:
```raw
/manifests/g/Google/Chrome/1.0
  | Google.Chrome.installer.beta.yaml
  | Google.Chrome.installer.canary.yaml
  | Google.Crome.installer.dev.yaml
  | Google.Chrome.installer.Stable.yaml
  | Google.Chrome.locale.en-US.yaml
  | Google.Chrome.locale.nb-NO.yaml
  | Google.Chrome.yaml
/manifests/g/Google/Chrome/2.0
  | Google.Chrome.installer.canary.yaml
  | Google.Crome.installer.dev.yaml
  | Google.Chrome.locale.en-US.yaml
  | Google.Chrome.locale.nb-NO.yaml
  | Google.Chrome.yaml
```

To maintain backwards compatability, `null` should be treated as a valid `DefaultChannel`. If the default channel is not specified, `<PackageIdentifier>.installer.yaml` would be treated as the default. Additional channels may still be added to the package, and all functionality would still be present.

An example of the manifest file structure when `DefaultChannel` is unset -
```raw
/manifests/7/7zip/7zip/1.0
  | 7zip.7zip.installer.yaml
  | 7zip.7zip.installer.Alpha.yaml
  | 7zip.7zip.locale.en-US.yaml
  | 7zip.7zip.yaml
/manifests/7/7zip/7zip/2.0
  | 7zip.7zip.installer.Alpha.yaml
  | 7zip.7zip.locale.en-US.yaml
  | 7zip.7zip.yaml
```

### CLI Behavior
Where not specified by the user, the default release channel should be used/shown first or exclusively, then other channels in ASCII sort order. If the package manifest has a default channel specified and no versions of that package have an installer manifest matching the default channel, then the next available channel should be used/shown. Channels should be treated as *case-insensitive*.

The channel `Default` shall be treated as a reserved token. When used in commands, default should refer to the default channel specified in the version manifest. In command output, when the default channel is named, the name should be shown. If the default channel is null, then `Default` should be shown. Manifest validation should throw an error when `DefaultChannel` is `Default`

#### winget search
In order to avoid package duplication in search results, only the default channel should be shown unless otherwise specified. Users should be able to search for packages by release channel in addition to all other search filters.

Some valid searches:
```powershell
PS> winget search --channel <channel> # Return all packages which have a channel matching <channel>
PS> winget search --channel "" # Return all packages which have a null channel
PS> winget search <query> --channel <channel> # Return packages matching <query> which have a channel matching <channel>
PS> winget search --tag <tag> --channel <channel> # Return packages with a tag matching <tag> which have a channel matching <channel>
...
```

#### winget show
When the user specifies `--versions`, the versions for all channels along with the channels should be shown. The versions should be grouped by channel, sorted by channel, then sorted by version descending
When the user specifies `--channels`, the available channels for the package should be shown. If a package contains a named default channel and unnamed (null) channels, only the named channels should be shown

#### winget install
When the user does not specify `--channel`, the default channel should be used. If the user does specify `--channel`, the specified channel should be used. The latest version for the channel should be installed, unless a user specified the version using the `--version` parameter. Additionally, the Windows Package Manager should record the package channel in some manner to ensure that upgrades to the package are found using the correct channel.

Optionally, the user should be able to specify the channel as part of the package identifier using the `@` symbol. For example, `winget install Google.Chrome@dev` would be equivalent to `winget install Google.Chrome --channel dev`

#### winget upgrade
When listing the packages available for upgrade, the Windows Package Manger should look only for packages matching the release channel of the installed package. 
When installing packages available for upgrade, the Windows Package Manager should only install new versions matching the release channel of the installed package.

If the package channel cannot be determined, there are several options. 
1) Ask the user for input.
2) Throw a warning and skip that package
3) Use the default channel for that package

The Windows Package Manager should never leave a package in a state where it cannot be upgraded simply because the package is unknown. To account for this, there must be some way for the user to specify a release channel. Therefore, when `winget upgrade --all` is used, and the release channel cannot be determined, a warning should be shown and the package should be skipped. For all other cases, the user should be asked to select a release channel on a per-package basis; the user's choice for that package should persist.

#### winget export/import
When exporting the list of installed packages, where possible, the release channel should be included with the export.
When importing the list of installed packages, where possible, the release channel should be honored. If the release channel is not able to be honored, a warning should be thrown, and the default release channel should be used

### Settings
The user should be able to specify a set of preferred release channels in their settings file. The `channel` parameter should be added under `installBehavior`. The matching parameter is `--channel`. The order of the preferences should be considered when installing packages such that the first value is the highest preference.
```json
"installBehavior": {
        "preferences": {
            "locale": [ "en-US", "fr-FR" ],
            "channel": ["alpha","beta","dev","canary","nightly"]
        }
    }
```

## UI/UX Design
### Searching for package version with release channel

```raw
PS> winget search Google.Chrome

Name                       Id                         Version        Channel
-----------------------------------------------------------------------------
Google Chrome              Google.Chrome              96.0.4664.45   Stable
Chrome Remote Desktop Host Google.ChromeRemoteDesktop 96.0.4664.39   Stable

PS> winget search Google.Chrome --channel dev

Name           Id             Version        Channel
----------------------------------------------------
Google Chrome  Google.Chrome  97.0.4692.20   Dev

PS> winget search Google --channel dev
Name           Id                   Version        Channel
-----------------------------------------------------------
Google Chrome  Google.Chrome        97.0.4692.20   Dev
Android Studio Google.AndroidStudio 2020.3.1       dev
```

### Showing available release channels
```raw
# When all channels have defined names
PS> winget show Google.Chrome --channels
Found Google Chrome [Google.Chrome]
Channel
-------
Stable
Beta
Canary
Dev

# When the default channel is unnamed
PS> winget show 7zip.7zip --channels
Found 7-zip [7zip.7zip]
Channel
-------
Default
alpha

# When the default channel is named "Stable", but unnamed channels exist
PS> winget show Badlion.BadlionClient --channels
Found Badlion Client [Badlion.BadlionClient]
Channel
-------
Stable
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

When the channel is not the default channel, the channel name should be shown after the package identifier when installing or upgrading
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

Many packages use different structures for release channels. The Windows Package Manager should accommodate as many of these as is practicable.

### Accessibility

The Windows Package Manager has been built in such a way that screen readers will still provide audible output as the command is executed keeping the user informed of progress, warnings, and errors. This should have no direct impact on accessibility.

### Security

There should be no security impact directly, although we must remember that different sources may not guarantee the safety of packages. The Windows Package Manager community repository performs static and dynamic analysis, and in some cases additional manual validation before accepting a package.

### Reliability

The Windows Package Manager community repository has many packages that may be affected by release channels. The ease of keeping packages updated must be a consideration when designing other tools for creating and updating manifests.

### Compatibility

The current implementation for different release channels is to have separate package identifiers for each package. Packages which have channel-specific identifiers currently existing in the community repository /should/ be updated to use this feature, but will not be required to.

The current implementation of automation - specifically @wingetbot - does not have support for release channels. Any package which implements release channels will likely not be able to undergo automatic upgrade unless and until the automation considers this feature.

### Performance, Power, and Efficiency

The time needed to return search results is directly related to the number of packages included in the scope of the search. This feature will likely remove extraneous packages as they are combined into manifests with release channel support

## Potential Issues

* If a user installs a package outside of the Windows Package Manager, we may not be able to determine the channel. This has potential to cause conflicts if an attempt is made to upgrade applications from one channel to another.
* A package may have a `DefaultChannel` set and there be no versions available for the specified channel. When a user runs a search or upgrade for the package, the behavior may be unpredictable unless fallback behavior is accounted for in the implementation. 
* Some packages may use multiple different names to refer to the same channel. Example - `Release`, `LTS`, `latest`, `current`, and `production` may all refer to the same channel. It may be wise to add an additional list parameter to the schema for `ChannelAliases`. This would allow the Windows Package Manager to identify when there is a matching package even though the user specified a release channel.

## Future considerations

* Implementation of the shorthand channel specifier may also allow for a shorthand version specifier such as `:`. This would enable something like `winget install Google.Chrome:96.0.4664.45@dev` for quick installation of specific versions and channels
* Applications which support side-by-side installation will remain under separate package identifiers under this schema. There may be a potential for the future to detect and allow for the side-by-side installations to be included in the release channels. This may need further investigation
* Release channels could potentially be used to mark packages as `portable` or `standalone` once [#182 - Support for installation of portable/standalone apps](https://github.com/microsoft/winget-cli/issues/182). This would be highly dependent upon the publisher and their release structure
## Resources

[Discussion from Trenly/winget-pkgs](https://github.com/Trenly/winget-pkgs/discussions/115)

[Indication of DefaultChannel Identifier](https://github.com/microsoft/winget-cli/discussions/1672#discussioncomment-1665516)

[Related Issue from Microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs/issues/16271)