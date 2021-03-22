# WinGet CLI Settings

You can configure WinGet by editing the `settings.json` file. Running `winget settings` will open the file in the default json editor, if no editor is configured, notepad.exe is used.

## File Location

Settings file is located in %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json

If you are using the non-packaged WinGet version by building it from source code, the file will be located under %LOCALAPPDATA%\Microsoft\WinGet\Settings\settings.json

## Source

The `source` settings involve configuration to the WinGet source.

```
    "source": {
        "autoUpdateIntervalInMinutes": 3
    },
``` 

### autoUpdateIntervalInMinutes

A positive integer represents the update interval in minutes. The check for updates only happens when a source is used. A zero will disable the check for updates to a source. Any other values are invalid.

- Disable: 0
- Default: 5

To manually update the source use `winget source update`

## Visual

The `visual` settings involve visual elements that are displayed by WinGet

```
    "visual": {
        "progressBar": "accent"
    },
```

### progressBar

Color of the progress bar that WinGet displays when not specified by arguments. 

- accent (default)
- retro
- rainbow

## Experimental Features

To allow work to be done and distributed to early adopters for feedback, settings can be used to enable "experimental" features. 

The `experimentalFeatures` settings involve the configuration of these "experimental" features. Individual features can be enabled under this node. The example below shows sample experimental features.

```
   "experimentalFeatures": {
       "experimentalCmd": true,
       "experimentalArg": false
   },
```

### experimentalMSStore

Microsoft Store App support in WinGet is currently implemented as an experimental feature. It supports a curated list of utility apps from Microsoft Store. You can enable the feature as shown below.

```
   "experimentalFeatures": {
       "experimentalMSStore": true
   },
```

### list

While work is in progress on list, the command is hidden behind a feature toggle. One can enable it as below:

```
   "experimentalFeatures": {
       "list": true
   },
```

### upgrade

While work is in progress on upgrade, the command is hidden behind a feature toggle. One can enable it as below:

```
   "experimentalFeatures": {
       "upgrade": true
   },
```

### uninstall

While work is in progress on uninstall, the command is hidden behind a feature toggle. One can enable it as below:

```
   "experimentalFeatures": {
       "uninstall": true
   },
```

### import

While work is in progress for import, the command is hidden behind a feature toggle. One can enable it as below:

```
   "experimentalFeatures": {
       "import": true
   },
```

### restSource

While work is in progress for rest source support, the feature is hidden behind a feature toggle. Enabling this will not change how client works currently and will allow testing any additional rest sources added. One can enable it as below:

```
   "experimentalFeatures": {
       "restSource": true
   },
```
