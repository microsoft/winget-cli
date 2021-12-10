Function Uninstall-WinGetPackage{
    <#
        .SYNOPSIS
        Uninstalls a package from the local system. 
        Additional options can be provided to filter the output, much like the search command.
        
        .DESCRIPTION
        By running this cmdlet with the required inputs, it will uninstall a package installed on the local system.

        .PARAMETER Filter
        Used to search across multiple fields of the package.
        
        .PARAMETER Id
        Used to specify the Id of the package

        .PARAMETER Name
        Used to specify the Name of the package

        .PARAMETER Moniker
        Used to specify the Moniker of the package

        .PARAMETER Version
        Used to specify the Version of the package
        
        .PARAMETER Exact
        Used to specify an exact match for any parameters provided. Many of the other parameters may be used for case insensitive substring matches if Exact is not specified.

        .PARAMETER Source
        Name of the Windows Package Manager private source. Can be identified by running: "Get-WinGetSource" and using the source Name

        .PARAMETER Interactive
        Used to specify the uninstaller should be run in interactive mode.

        .PARAMETER Silent
        Used to specify the uninstaller should be run in silent mode with no user input.

        .PARAMETER Log
        Used to specify the location for the log location if it is supported by the package uninstaller.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.
        
        .PARAMETER AcceptSourceAgreement
        Used to explicitly accept any agreement required by the source.

        .PARAMETER Local
        Used to uninstall from a local manifest

        .EXAMPLE
        Uninstall-WinGetPackage -id "Publisher.Package"

        This example expects only a single configured REST source with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Uninstall-WinGetPackage -id "Publisher.Package" -source "Private"

        This example expects the REST source named "Private" with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Uninstall-WinGetPackage -Name "Package"

        This example expects a configured source contains a package with "Package" as a valid name.
    #>

    PARAM(
        [Parameter(Position=0)] $Filter,
        [Parameter()]           $Name,
        [Parameter()]           $Id,
        [Parameter()]           $Moniker,
        [Parameter()]           $Source,
        [Parameter()] [switch]  $Interactive,
        [Parameter()] [switch]  $Silent,
        [Parameter()] [string]  $Version,
        [Parameter()] [switch]  $Exact,
        [Parameter()] [switch]  $Override,
        [Parameter()] [System.IO.FileInfo]  $Location,
        [Parameter()] [switch]  $Force,
        [Parameter()] [System.IO.FileInfo]  $Log, ## This is a path of where to create a log.
        [Parameter()] [switch]  $AcceptSourceAgreements,
        [Parameter()] [switch]  $Local # This is for installing local manifests
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Uninstall"
        IF($PSBoundParameters.ContainsKey('Filter')){
            IF($Local) {
                $WinGetArgs += "--Manifest"
            }
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
        IF($Interactive){
            $WinGetArgs += "--Interactive"
        }
        IF($Silent){
            $WinGetArgs += "--Silent"
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
        if($PSBoundParameters.ContainsKey('Location')){
            $WinGetArgs += "--Location", $Location
        }
        if($Force){
            $WinGetArgs += "--Force"
        }
    }
    PROCESS
    {
        ## Exact, ID and Source - Talk with tomorrow to better understand this.
        IF(!$Local) {
            $Result = Find-WinGetPackage -Filter $Filter -Name $Name -Id $Id -Moniker $Moniker -Tag $Tag -Command $Command -Source $Source
        }

        if($Result.count -eq 1 -or $Local) {
            & "WinGet" $WingetArgs
            $Result = ""
        }
        elseif($Result.count -lt 1){
            Write-Host "Unable to locate package for uninstallation"
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

New-Alias -Name Remove-WinGetPackage -Value Uninstall-WinGetPackage

Export-ModuleMember -Function Uninstall-WinGetPackage
Export-ModuleMember -Alias Remove-WinGetPackage
