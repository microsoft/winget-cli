## New in v1.30


### Manifest `<PACKAGEVERSION>` token expansion

Added `<PACKAGEVERSION>` token which will use the `PackageVersion` from the manifest in place of the token. This applies to the following manifest fields:

* `NestedInstallerFiles.RelativeFilePath`
* `ProductCode`
* `AppsAndFeaturesEntries.DisplayName`
* `AppsAndFeaturesEntries.ProductCode`
* `InstallationMetadata.DefaultInstallLocation`
* `InstallationMetadata.Files.RelativeFilePath`
* `ReleaseNotesUrl`

## Bug Fixes

* Updated NUnit to v4
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
