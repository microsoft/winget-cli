# Module created by Microsoft.PowerShell.Crescendo
class PowerShellCustomFunctionAttribute : System.Attribute { 
  [bool]$RequiresElevation
  [string]$Source
  PowerShellCustomFunctionAttribute() { $this.RequiresElevation = $false; $this.Source = "Microsoft.PowerShell.Crescendo" }
  PowerShellCustomFunctionAttribute([bool]$rElevation) {
      $this.RequiresElevation = $rElevation
      $this.Source = "Microsoft.PowerShell.Crescendo"
  }
}

<#
.SYNOPSIS
Enables the WinGet setting specified by the `Name` parameter.

.DESCRIPTION
Enables the WinGet setting specified by the `Name` parameter.
Supported settings:
  - LocalManifestFiles
  - BypassCertificatePinningForMicrosoftStore
  - InstallerHashOverride
  - LocalArchiveMalwareScanOverride

.PARAMETER Name
Specifies the name of the setting to be enabled.

.INPUTS
None.

.OUTPUTS
None

.EXAMPLE
PS> Enable-WinGetSetting -name LocalManifestFiles 
#>
function Enable-WinGetSetting
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(
[Parameter(Position=0,ValueFromPipeline=$true,ValueFromPipelineByPropertyName=$true,Mandatory=$true)]
[string]$Name
  )

BEGIN {
  $__PARAMETERMAP = @{
       Name = @{
             OriginalName = ''
             OriginalPosition = '0'
             Position = '0'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
  }

  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'settings'
  $__commandArgs += '--enable'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

<#
.SYNOPSIS
Disables the WinGet setting specified by the `Name` parameter.

.DESCRIPTION
Disables the WinGet setting specified by the `Name` parameter.
Supported settings:
  - LocalManifestFiles
  - BypassCertificatePinningForMicrosoftStore
  - InstallerHashOverride
  - LocalArchiveMalwareScanOverride

.PARAMETER Name
Specifies the name of the setting to be disabled.

.INPUTS
None.

.OUTPUTS
None

.EXAMPLE
PS> Disable-WinGetSetting -name LocalManifestFiles 
#>
function Disable-WinGetSetting
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(
[Parameter(Position=0,ValueFromPipeline=$true,ValueFromPipelineByPropertyName=$true,Mandatory=$true)]
[string]$Name
  )

BEGIN {
  $__PARAMETERMAP = @{
       Name = @{
             OriginalName = ''
             OriginalPosition = '0'
             Position = '0'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
  }

  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'settings'
  $__commandArgs += '--disable'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

<#
.SYNOPSIS
Get winget settings.

.DESCRIPTION
Get the administrator settings values as well as the location of the user settings as json string

.PARAMETER Name
None

.INPUTS
None.

.OUTPUTS
Prints the export settings json.

.EXAMPLE
PS> Get-WinGetSettings
#>
function Get-WinGetSettings
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(    )

BEGIN {
  $__PARAMETERMAP = @{}
  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'settings'
  $__commandArgs += 'export'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

<#
.SYNOPSIS
Add a new source.

.DESCRIPTION
Add a new source. A source provides the data for you to discover and install packages.
Only add a new source if you trust it as a secure location.

.PARAMETER Name
Name of the source.

.PARAMETER Argument
Argument to be given to the source.

.PARAMETER Type
Type of the source.

.INPUTS
None.

.OUTPUTS
None.

.EXAMPLE
PS> Add-WinGetSource -Name Contoso -Argument https://www.contoso.com/cache

#>
function Add-WinGetSource
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(
[Parameter(Position=0,ValueFromPipelineByPropertyName=$true,Mandatory=$true)]
[string]$Name,
[Parameter(Position=1,ValueFromPipelineByPropertyName=$true,Mandatory=$true)]
[string]$Argument,
[Parameter(Position=2,ValueFromPipelineByPropertyName=$true)]
[string]$Type
  )

BEGIN {
  $__PARAMETERMAP = @{
       Name = @{
             OriginalName = '--name'
             OriginalPosition = '0'
             Position = '0'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
       Argument = @{
             OriginalName = '--arg'
             OriginalPosition = '0'
             Position = '1'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
       Type = @{
             OriginalName = '--type'
             OriginalPosition = '0'
             Position = '2'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
  }

  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'source'
  $__commandArgs += 'add'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

<#
.SYNOPSIS
Remove a specific source.

.DESCRIPTION
Remove a specific source. The source must already exist to be removed.

.PARAMETER Name
Name of the source.

.INPUTS
None.

.OUTPUTS
None.

.EXAMPLE
PS> Remove-WinGetSource -Name Contoso

#>
function Remove-WinGetSource
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(
[Parameter(Position=0,ValueFromPipeline=$true,ValueFromPipelineByPropertyName=$true,Mandatory=$true)]
[string]$Name
  )

BEGIN {
  $__PARAMETERMAP = @{
       Name = @{
             OriginalName = '--name'
             OriginalPosition = '0'
             Position = '0'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
  }

  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'source'
  $__commandArgs += 'remove'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

<#
.SYNOPSIS
Drops existing sources. Without any argument, this command will drop all sources and add the defaults.

.DESCRIPTION
Drops existing sources, potentially leaving any local data behind. Without any argument, it will drop all sources and add the defaults.
If a named source is provided, only that source will be dropped.

.PARAMETER Name
Name of the source.

.INPUTS
None.

.OUTPUTS
None.

.EXAMPLE
PS> Reset-WinGetSource

.EXAMPLE
PS> Reset-WinGetSource -Name Contoso

#>
function Reset-WinGetSource
{
[PowerShellCustomFunctionAttribute(RequiresElevation=$False)]
[CmdletBinding(SupportsShouldProcess)]

param(
[Parameter(Position=0,ValueFromPipeline=$true,ValueFromPipelineByPropertyName=$true)]
[string]$Name
  )

BEGIN {
  $__PARAMETERMAP = @{
       Name = @{
             OriginalName = '--name'
             OriginalPosition = '0'
             Position = '0'
             ParameterType = 'string'
             ApplyToExecutable = $False
             NoGap = $False
             }
  }

  $__outputHandlers = @{ Default = @{ StreamOutput = $true; Handler = { $input } } }
}

PROCESS {
  $__boundParameters = $PSBoundParameters
  $__defaultValueParameters = $PSCmdlet.MyInvocation.MyCommand.Parameters.Values.Where({$_.Attributes.Where({$_.TypeId.Name -eq "PSDefaultValueAttribute"})}).Name
  $__defaultValueParameters.Where({ !$__boundParameters["$_"] }).ForEach({$__boundParameters["$_"] = get-variable -value $_})
  $__commandArgs = @()
  $MyInvocation.MyCommand.Parameters.Values.Where({$_.SwitchParameter -and $_.Name -notmatch "Debug|Whatif|Confirm|Verbose" -and ! $__boundParameters[$_.Name]}).ForEach({$__boundParameters[$_.Name] = [switch]::new($false)})
  if ($__boundParameters["Debug"]){wait-debugger}
  $__commandArgs += 'source'
  $__commandArgs += 'reset'
  $__commandArgs += '--force'
  foreach ($paramName in $__boundParameters.Keys|
          Where-Object {!$__PARAMETERMAP[$_].ApplyToExecutable}|
          Sort-Object {$__PARAMETERMAP[$_].OriginalPosition}) {
      $value = $__boundParameters[$paramName]
      $param = $__PARAMETERMAP[$paramName]
      if ($param) {
          if ($value -is [switch]) {
               if ($value.IsPresent) {
                   if ($param.OriginalName) { $__commandArgs += $param.OriginalName }
               }
               elseif ($param.DefaultMissingValue) { $__commandArgs += $param.DefaultMissingValue }
          }
          elseif ( $param.NoGap ) {
              $pFmt = "{0}{1}"
              if($value -match "\s") { $pFmt = "{0}""{1}""" }
              $__commandArgs += $pFmt -f $param.OriginalName, $value
          }
          else {
              if($param.OriginalName) { $__commandArgs += $param.OriginalName }
              $__commandArgs += $value | Foreach-Object {$_}
          }
      }
  }
  $__commandArgs = $__commandArgs | Where-Object {$_ -ne $null}
  if ($__boundParameters["Debug"]){wait-debugger}
  if ( $__boundParameters["Verbose"]) {
       Write-Verbose -Verbose -Message winget.exe
       $__commandArgs | Write-Verbose -Verbose
  }
  $__handlerInfo = $__outputHandlers[$PSCmdlet.ParameterSetName]
  if (! $__handlerInfo ) {
      $__handlerInfo = $__outputHandlers["Default"] # Guaranteed to be present
  }
  $__handler = $__handlerInfo.Handler
  if ( $PSCmdlet.ShouldProcess("winget.exe $__commandArgs")) {
  # check for the application and throw if it cannot be found
      if ( -not (Get-Command -ErrorAction Ignore "winget.exe")) {
        throw "Cannot find executable 'winget.exe'"
      }
      if ( $__handlerInfo.StreamOutput ) {
          & "winget.exe" $__commandArgs | & $__handler
      }
      else {
          $result = & "winget.exe" $__commandArgs
          & $__handler $result
      }
  }
} # end PROCESS
}

