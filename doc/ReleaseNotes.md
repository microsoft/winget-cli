## New in v1.11
* Dropped support for running on 32-bit ARM
* Support for [Microsoft Desired State Configuration (DSC) v3](https://learn.microsoft.com/powershell/dsc/overview?view=dsc-3.0).
* Support for exporting the configuration of the current device. This includes Windows Settings, packages from configured WinGet sources, and package settings from DSC v3 enabled packages.

## Experimental Features
* Experimental support for Fonts

---
### Experimental support for Fonts
The following snippet enables experimental support for fonts via `winget settings`. The `winget font list` command will list installed font families and the number of installed font faces.
```JSON
{
  "$schema" "https://aka.ms/winget-settings.schema.json",
  "experimentalFeatures": {
    "fonts": true
  }
}

```
