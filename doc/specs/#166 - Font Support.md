---
author: Ryan Fu @ryfu-msft
created on: 2024-9-18
last updated: 2024-9-18
issue id: 166
---

# Support for installation of fonts

For [#66](https://github.com/microsoft/winget-cli/issues/166)

## Abstract
This spec outlines the design for supporting the installation of fonts.

## Context

### Font File Types

This feature will support font files with the following file types:
- .ttf (True Type File)
- .ttc (True Type Collection)
- .otf (Open Type Font)

The following font types to my knowledge are not supported:
- .woff & .woff2 (developed by Google for the modern browser)
- .eot (Embedded OpenType font file) Not used because of security issues.

### Installing Fonts
Typically, a user would download the font file and drag it to the `C:\Windows\Fonts` directory in the explorer view, but there are a couple things happening behind the scenes. 

Depending on the scope of the user, the font will be stored in two locations:
- `C:\Windows\Fonts`
- `%LOCALAPPDATA%\Microsoft\Windows\Fonts` (User Mode)

In addition, a new registry entry is created in the following registry paths:

- `Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts`
- `Computer\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts`

The name and value of the registry key is based on the title and type of the font file.





### Uninstalling Fonts


## Manifest Changes:


## CLI Behavior

