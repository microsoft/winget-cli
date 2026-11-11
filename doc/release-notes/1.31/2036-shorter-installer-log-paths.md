## Bug Fixes

### Shorter automatic installer log paths

WinGet now shortens default installer log names as needed to target a complete path of 150 characters, leaving more room for installers that append their own log names. Names retain the package version and timestamp when they fit; otherwise, WinGet omits the version and uses a compact timestamp, then a package-ID prefix with a time-only timestamp, before falling back to a short random name.

This is a best-effort mitigation, not a guarantee for installer-added suffixes. The log directory and explicit log paths are unchanged. If even a minimal automatic name cannot fit within the Windows path limit, WinGet warns and continues without requesting an installer log file.
