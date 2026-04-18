## New in v1.30

# New Feature: NoInstaller manifest type

Manifests can now declare `InstallerType: noinstaller` (manifest version `1.29.0`). This type is intended for software a publisher no longer offers as a direct download, allowing winget to continue correlating against the installed package without providing an installer URL.

Key behaviours:

- **`winget install`** — immediately stops with the package's `InstallerAvailabilityMessage` if one is set, or a default "The installer for this package is no longer available." message otherwise. The exit code is `0x8A150116` (`APPINSTALLER_CLI_ERROR_INSTALLER_NOT_AVAILABLE`).
- **`winget show`** — displays `Installer Availability Message:` instead of `Installer Url:`, and shows `Offline Distribution Supported: false`.
- **`winget upgrade`** — a real installer is always preferred over a `noinstaller` entry when both exist for the same package. A `noinstaller` entry is only selected when it is the only applicable option, at which point the install flow blocks as above.
- **Manifest fields** — `InstallerUrl` must not be present. `InstallerSha256`, `ProductCode`, and `AppsAndFeaturesEntries` are all supported for package correlation. `InstallerAvailabilityMessage` (optional, max 512 characters) may be set at root or per-installer level.


## Bug Fixes

* None yet
