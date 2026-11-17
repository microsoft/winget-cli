## Bug Fixes

### Minor Bug Fixes

* Fixed `Assert-WinGetPackageManager` and `Repair-WinGetPackageManager` running a `winget.exe` from the current directory. They now only run `winget.exe` from the installed App Installer package.
