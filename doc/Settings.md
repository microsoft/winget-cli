# WinGet CLI Settings

You can configure WinGet by editing the `settings.json` file. The file can be open with the default json editor by running `winget settings`. If no editor is configure, notepad.exe will be used.

## File Location

Settings file is located in %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json

If you are using the non-packaged winget version by building it from source code the file will %LOCALAPPDATA%\Microsoft\WinGet\Settings\settings.json

## Source

These settings involve configuration to the WinGet source.

```
    "source": {
        "autoUpdateIntervalInMinutes": 3
    },
``` 

### autoUpdateIntervalInMinutes

Positive integer that represents the interval in minutes on how often to update the WinGet source automatically. For no updates use 0.

- Minimum: 0
- Default: 5

To manually update the source use `winget source update`

## Visual

These settings involve visual elements that are displayed by WinGet

```
    "visual": {
        "progressBar": "accent"
    }
```

### progressBar

Color of the progress bar that WinGet displays when not specified by arguments. 

- accent (default)
- retro
- rainbow