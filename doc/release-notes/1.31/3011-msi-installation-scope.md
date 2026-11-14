## Bug Fixes

### Minor Bug Fixes

* Resolve MSI installation scope using Windows Installer context so that current-user installations registered in HKLM remain eligible for user-scoped upgrades instead of selecting a machine installer. MSI entries with unavailable or ambiguous context are excluded rather than guessing their scope. See https://github.com/microsoft/winget-cli/issues/3011.
