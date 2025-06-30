## New in v1.12
*No applicable update found*

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

Experimental initial Font Install and Uninstall via manifest for user and machine scopes has been added.

The font 'list' command has been updated with a new '--files' feature for an alternate view of the installed fonts.

```
