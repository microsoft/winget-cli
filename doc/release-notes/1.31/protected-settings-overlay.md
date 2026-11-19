## New Features

### Protected administrator settings

WinGet now supports an administrator-managed settings file for each user.
Settings in this file take precedence over regular user settings, including
when WinGet runs without administrator privileges. Install and module folders
are the exception: you can configure separate folders for normal and
administrator use.

To edit the new `admin-settings.json` file, open a terminal using
**Run as administrator**, then run
`winget settings --admin`.
PowerShell user-setting cmdlets also support `-Admin`.

When running as administrator, WinGet now uses this file instead of your regular
settings for portable package folders, the default install folder, the
configuration module folder, the download folder, and portable-package cleanup.
If you previously customized these settings, WinGet displays a warning directing
you to configure their administrator values.

Configurations run as administrator now use a separate default module folder, so
WinGet may need to download modules again.

See [Administrator Settings](https://github.com/microsoft/winget-cli/blob/master/doc/Settings.md#administrator-settings) for details.
