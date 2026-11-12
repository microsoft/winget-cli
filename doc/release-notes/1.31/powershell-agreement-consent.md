## New Features

### PowerShell module asks for consent to source and package agreements

Microsoft.WinGet.Client now shows source and package agreements and asks for consent before
accepting them, instead of accepting them silently. Accepted source agreements are saved, so each
source asks only once until its agreements change. If you decline a source's agreements, that source
is left out of the results. If you decline a package's agreements, the package is not installed.

* `-AcceptSourceAgreements` is available on `Find-`, `Get-`, `Install-`, `Update-`, `Uninstall-`,
  `Repair-` and `Export-WinGetPackage`.
* `-AcceptPackageAgreements` is available on `Install-`, `Update-`, `Repair-` and
  `Export-WinGetPackage`. `Repair-WinGetPackage` can now repair packages that have agreements.
* When the host cannot prompt, such as with `-NonInteractive`, the agreements are still accepted but
  are shown in a warning. This behavior is deprecated: a future release will require the switches
  in non-interactive use, so update scripts and automation to pass them.
* The `WinGetPackage` resource in Microsoft.WinGet.DSC has a new `AcceptAgreements` property, which
  defaults to `true` and passes both switches.

Accepting a package's agreements also accepts the agreements of any dependencies that are installed
with it.
