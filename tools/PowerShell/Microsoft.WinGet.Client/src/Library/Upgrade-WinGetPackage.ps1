Function Upgrade-WinGetPackage
{
    <#
        .SYNOPSIS
        Upgrades a package on the local system. 
        Additional options can be provided to filter the output, much like the search command.
        
        .DESCRIPTION
        By running this cmdlet with the required inputs, it will retrieve the packages installed on the local system.

        .PARAMETER Filter
        Used to search across multiple fields of the package.
        
        .PARAMETER Id
        Used to specify the Id of the package

        .PARAMETER Name
        Used to specify the Name of the package

        .PARAMETER Moniker
        Used to specify the Moniker of the package

        .PARAMETER Tag
        Used to specify the Tag of the package
        
        .PARAMETER Command
        Used to specify the Command of the package

        .PARAMETER Channel
        Used to specify the channel of the package. Note this is not yet implemented in Windows Package Manager as of version 1.1.0.

        .PARAMETER Scope
        Used to specify install scope (user or machine)
        
        .PARAMETER Exact
        Used to specify an exact match for any parameters provided. Many of the other parameters may be used for case-insensitive substring matches if Exact is not specified.

        .PARAMETER Source
        Name of the Windows Package Manager private source. Can be identified by running: "Get-WinGetSource" and using the source Name

        .PARAMETER Manifest
        Path to the manifest on the local file system. Requires local manifest setting to be enabled.

        .PARAMETER Interactive
        Used to specify the installer should be run in interactive mode.

        .PARAMETER Silent
        Used to specify the installer should be run in silent mode with no user input.

        .PARAMETER Locale
        Used to specify the locale for localized package installer.

        .PARAMETER Log
        Used to specify the location for the log location if it is supported by the package installer.

        .PARAMETER Override
        Used to override switches passed to installer.

        .PARAMETER Force
        Used to force the upgrade when the Windows Package Manager would ordinarily not upgrade the package.

        .PARAMETER Location
        Used to specify the location for the package to be upgraded.

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.

        .PARAMETER Version
        Used to specify the Version of the package

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.
        
        .PARAMETER AcceptPackageAgreement
        Used to accept any source package required for the package.

        .PARAMETER AcceptSourceAgreement

        .EXAMPLE
        Upgrade-WinGetPackage -id "Publisher.Package"

        This example expects only a single package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Upgrade-WinGetPackage -id "Publisher.Package" -source "Private"

        This example expects the source named "Private" contains a package with "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Upgrade-WinGetPackage -Name "Package"

        This example expects the source named "Private" contains a package with "Package" as a valid name.
    #>

    PARAM(
        [Parameter(Position=0)] $Filter,
        [Parameter()]           $Name,
        [Parameter()]           $Id,
        [Parameter()]           $Moniker,
        [Parameter()]           $Source,
        [Parameter()] [ValidateSet("User", "Machine")] $Scope,
        [Parameter()] [switch]  $Interactive,
        [Parameter()] [switch]  $Silent,
        [Parameter()] [string]  $Version,
        [Parameter()] [switch]  $Exact,
        [Parameter()] [switch]  $Override,
        [Parameter()] [System.IO.FileInfo]  $Location,
        [Parameter()] [switch]  $Force,
        [Parameter()] [ValidatePattern("^([a-zA-Z]{2,3}|[iI]-[a-zA-Z]+|[xX]-[a-zA-Z]{1,8})(-[a-zA-Z]{1,8})*$")] [string] $Locale,
        [Parameter()] [System.IO.FileInfo]  $Log, ## This is a path of where to create a log.
        [Parameter()] [switch]  $AcceptSourceAgreements
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Install"
        IF($PSBoundParameters.ContainsKey('Filter')){
            $WinGetArgs += $Filter
        }
        IF($PSBoundParameters.ContainsKey('Name')){
            $WinGetArgs += "--Name", $Name
        }
        IF($PSBoundParameters.ContainsKey('Id')){
            $WinGetArgs += "--Id", $Id
        }
        IF($PSBoundParameters.ContainsKey('Moniker')){
            $WinGetArgs += "--Moniker", $Moniker
        }
        IF($PSBoundParameters.ContainsKey('Source')){
            $WinGetArgs += "--Source", $Source
        }
        IF($PSBoundParameters.ContainsKey('Scope')){
            $WinGetArgs += "--Scope", $Scope
        }
        IF($Interactive){
            $WinGetArgs += "--Interactive"
        }
        IF($Silent){
            $WinGetArgs += "--Silent"
        }
        IF($PSBoundParameters.ContainsKey('Locale')){
            $WinGetArgs += "--locale", $Locale
        }
        if($PSBoundParameters.ContainsKey('Version')){
            $WinGetArgs += "--Version", $Version
        }
        if($Exact){
            $WinGetArgs += "--Exact"
        }
        if($PSBoundParameters.ContainsKey('Log')){
            $WinGetArgs += "--Log", $Log
        }
        if($PSBoundParameters.ContainsKey('Override')){
            $WinGetArgs += "--override", $Override
        }
        if($PSBoundParameters.ContainsKey('Location')){
            $WinGetArgs += "--Location", $Location
        }
        if($Force){
            $WinGetArgs += "--Force"
        }
    }
    PROCESS
    {
        ## Exact, ID and Source - Talk with Demitrius tomorrow to better understand this.
        $Result = Find-WinGetPackage -Filter $Filter -Name $Name -Id $Id -Moniker $Moniker -Tag $Tag -Command $Command -Source $Source

        if($Result.count -eq 1) {
            & "WinGet" $WingetArgs
            $Result = ""
        }
        elseif($Result.count -lt 1){
            Write-Host "Unable to locate package for installation"
            $Result = ""
        }
        else {
            Write-Host "Multiple packages found matching input criteria. Please refine the input."
        }
    }
    END
    {
        return $Result
    }
}

Export-ModuleMember -Function Install-WinGetPackage