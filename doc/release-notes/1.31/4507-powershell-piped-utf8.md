## Bug Fixes

### Redirected human-readable output uses the console code page

Human-readable output that is redirected to a pipe is now written in the console's output code
page instead of UTF-8. A console host decodes the output of a native command with the code page
that was active before the command started, so `winget list --source winget | Sort-Object Name`
in PowerShell on a Russian or Ukrainian system now displays localized package names instead of
garbled text (https://github.com/microsoft/winget-cli/issues/4507).

The conversion only applies while standard output is a pipe and the code page is not already
UTF-8. Output that the operating system redirects to a file is not converted, and
machine-readable channels (such as the JSON output of the DSC commands) and completion output
always stay UTF-8.

Characters that the console code page cannot represent are written as `?`, and characters that
the code page has a closest equivalent for use that equivalent. On a system whose console uses
code page 866, for example, the `…` table truncation marker is written as `:`, and Japanese or
Ukrainian `і`/`ґ` characters are written as `?`. For lossless output in PowerShell, set
`[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()` to update both the console
code page and PowerShell's cached decoder. Use a UTF-8-capable terminal such as
[Windows Terminal](https://github.com/microsoft/terminal). For tools that already expect UTF-8,
set the new [`output.pipedEncoding`](https://github.com/microsoft/winget-cli/blob/master/doc/Settings.md#pipedencoding)
setting to `utf8`.

This works around the console host decoding behavior. The underlying PowerShell issues are
https://github.com/PowerShell/PowerShell/issues/28496 and
https://github.com/PowerShell/PowerShell/issues/28498.
