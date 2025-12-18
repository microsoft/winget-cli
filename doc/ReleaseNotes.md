## New in v1.28

# Experimental Feature: 'sourceEdit'
New feature that adds an 'edit' subcommand to the 'source' command. This can be used to set an explicit source to be implicit and vice-versa. For example, with this feature you can make the 'winget-font' source an implicit source instead of explicit source.

To enable this feature, add the 'sourceEdit' experimental feature to your settings.
```
"experimentalFeatures": {
    "sourceEdit": true
},
```
To use the feature, try `winget source edit winget-font` to set the Explicit state to the default.

<!-- Nothing yet! -->

## Bug Fixes
* Portable Packages now use the correct directory separators regardless of which convention is used in the manifest
* `--suppress-initial-details` now works with `winget configure test`
* `--suppress-initial-details` no longer requires `--accept-configuration-agreements`
