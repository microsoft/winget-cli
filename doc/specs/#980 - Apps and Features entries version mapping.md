---
author: Yao Sun @yao-msft
created on: 2022-05-09
last updated: 2022-05-15
issue id: 980
---

# Mapping of Apps and Features version and Winget package version

For [#980](https://github.com/microsoft/winget-cli/issues/980)

## Abstract

Some Winget packages may have the concept of marketing version and internal version. So they may have different version values in Winget manifest and in Apps and Features registry entry. Usually, the marketing version will be in the Winget manifest and internal version will be written in the Apps and Features registry.  

In this doc, Winget manifest version will be referred to as Winget version and Apps and Features version in registry will be referred to as ARP version.

## Inspiration

Winget should try to map or correlate the ARP version of an installed package to its Winget version. This will help Winget List/Upgrade better determine if there's an upgrade available.

## Solution Design

### Version parsing and comparison logic in Winget

Versions are parsed by:
1. Splitting the string based on the split character (`.`)
2. Parsing a leading, positive integer from each split part
3. Saving any remaining, non-digits as a supplemental value
4. If a version part's value is 0 and it does not have supplemental value(non-digits), the version part is dropped(i.e. `1.0.0` will be parsed internally as version with only one part with value 1)

Versions are compared by:  
 for each part in each version  
  if both sides have no more parts, return equal  
  else if one side has no more parts, it is less  
  else if integers not equal, return comparison of integers  
  else if only one side has a non-empty string part, it is less  
  else if string parts not equal, return comparison of strings  

For example:  
Version `1` is less than version `2`  
Version `1.0.0` is less than version `2.0.0`  
Version `0.0.1-alpha` is less than version `0.0.2-alpha`  
Version `0.0.1-beta` is less than version `0.0.2-alpha`  
Version `0.0.1-alpha` is less than version `0.0.1-beta`  
Version `0.0.1-alpha` is less than version `0.0.1` 
Version `13.9.8` is less than version `14.0`  
Version `1.0` is equal to version `1.0.0`  

Both Winget version and ARP version will use the above parsing and comparison logic.

### How Winget collects and stores ARP version

The `DisplayVersion` field under `AppsAndFeaturesEntries` will be treated as ARP versions of the package.
```YAML
Installers:
  AppsAndFeaturesEntries:
    DisplayVersion:     # Used as ARP version for version comparison if the key is present
```

Some packages may assign different build numbers to their internal versions for different installers. Winget will treat the ARP versions as a version range.  
For example, for below manifest, the ARP version range will be [10.0.0.1, 10.0.0.4], any ARP version between 10.0.0.1 and 10.0.0.4(both inclusive) will be treated as a match(i.e. mapped to Winget version 1.0.0).
```YAML
PackageVersion: 1.0.0
Installers:
- Architecture: x86
  AppsAndFeaturesEntries:
    DisplayVersion: 10.0.0.1
- Architecture: x64
  AppsAndFeaturesEntries:
    DisplayVersion: 10.0.0.2
- Architecture: arm
  AppsAndFeaturesEntries:
    DisplayVersion: 10.0.0.3
- Architecture: arm64
  AppsAndFeaturesEntries:
    DisplayVersion: 10.0.0.4
```

For manifest with only 1 ARP version, the only ARP version will be both the minimum and maximum version of the ARP version range.

The Winget index will record the ARP version range(if present in the manifest) in its internal VersionTable. During indexing, Winget will make sure ARP version range does not collide with each other within a same package.

### How Winget maps ARP version to Winget version

Winget will create a map of Winget version and ARP version range after all available packages info and installed package info are ready. The map will be sorted by Winget version. Conceptually below:
```text
Winget version:       1.0.0                   2.0.0                  3.0.0
ARP version range:    [10.0.0.1, 10.0.0.4]    [11.0.0.1, 11.0.0.4]   [12.0.0.1, 12.0.0.4]
```

#### No version mapping performed
For packages without ARP version ranges provided, or if all ARP version ranges provided are same as the Winget version, no ARP version and Winget version mapping will be performed. For example:
```text
Winget version:       1.0.0                   2.0.0                  3.0.0
ARP version range:    [1.0.0, 1.0.0]          [Not provided]         [3.0.0, 3.0.0]
```

#### Only mapped if installed ARP version fall within one of the ARP version range
For packages with ARP version unordered, or ordered in the opposite order of Winget version, mapping will be performed only when installed ARP version fall within one of the ARP version range. Otherwise the installed version will be set to Unknown. For example:
```text
Winget version:       1.0.0                   2.0.0                  3.0.0
ARP version range:    [10.0, 10.5]            [7.0, 7.5]             [13.0, 13.7]
```

ARP version `10.4` will be mapped as Winget version `1.0.0`.  
ARP version `13.4` will be mapped as Winget version `3.0.0`.  
ARP version `9.4` will be mapped as Winget version `Unknown`.

#### Full version mapping performed
For packages with ARP version ordered in the same order of Winget version, full mapping will be performed. For example:
```text
Winget version:       1.0.0                   2.0.0                  3.0.0             4.0.0
ARP version range:    [10.0, 10.5]            [Not provided]         [12.0, 12.5]      [13.0, 13.5]
```

Winget will perform following mapping:
1. Check if ARP version fall within one of the ARP version range
2. Try to find the closest ARP version range it's less than, this is higher priority than greater than mapping because this mapping will mostly be used for package upgrade applicability check.
3. Try to find the closes ARP version range it's greater than

A special "less than"("< ") and "greater than"("> ") version concept will be used. This is to indicate the version is less than or greater than the closest version Winget is known of. UI/UX change to the output is described in later UI/UX section.

For above version mapping:  
ARP version `10.0` will be mapped as Winget version `1.0.0`.  
ARP version `11.7` will be mapped as less than Winget version 3.0.0(`< 3.0.0`).  
ARP version `12.7` will be mapped as less than Winget version 4.0.0(`< 4.0.0`).  
ARP version `14.0` will be mapped as greater than Winget version 4.0.0(`> 4.0.0`).  
ARP version `2.0.0` will be mapped as less than Winget version 1.0.0(`< 1.0.0`)(Once ARP version mapping logic is applied, Winget will only look at ARP version range, though `2.0.0` exactly matches one Winget version).

For version comparison with special "less than" or "greater than", the "less than" or "greater than" only applies when compared to the specific version, for comparing to other versions, "less than" or "greater than" could be considered as ignored.

For example:  
Version `< 3.0` is less than version `3.0`  
Version `< 3.0` is greater than version `2.9`(because `3.0` is greater than `2.9`)  
Version `< 3.0` is less than version `4.0`  
Version `< 3.0` is less than version `> 3.0`  
Version `> 3.0` is greater than version `3.0`  
Version `> 3.0` is less than version `3.1`  (because `3.0` is less than `3.1`)  
Version `> 3.0` is greater than version `2.9`  

**Note:** It is recommended for package authors to update ARP version info for all package versions of a package for better version mapping if this feature is to be used for a package.

**Note:** Given the limited support for multiple component packages by Winget as of the writing, if a package has multiple components to be installed, it is recommended for package authors to only list the `DisplayName`s of the primary component for better version mapping results.

## UI/UX Design

For packages matched to a particular Winget version, there'll be no UI/UX change.

For packages not matched to a particular Winget version, "> " will be used to indicate the version is greater than a closest version known to Winget.

```text
Name           Id                   Version   Available Source
------------------------------------------------------------
Git            Git.Git              > 2.36.0            winget
GitHub Desktop GitHub.GitHubDesktop 2.9.15    3.0.0     winget
```

"< " will be used to indicate the version is less than a closest version known to Winget.

```text
Name           Id                   Version   Available Source
------------------------------------------------------------
Git            Git.Git              < 2.36.0  2.36.0    winget
GitHub Desktop GitHub.GitHubDesktop 2.9.15    3.0.0     winget
```

## Capabilities

This should improve Winget upgrade applicability check and lead to a better result for Winget list/upgrade.

### Accessibility

This is not expected to impact accessibility.

### Security

This should not introduce any _new_ security concerns.

### Reliability

This is not expected to impact reliability.

### Compatibility

This changes existing behavior with Winget installed packages version detection, but is expected to be an improvement.

### Performance, Power, and Efficiency

All the new ARP versions info will be indexed so this should have little impact to performance.

## Potential Issues

## Future considerations

## Resources
