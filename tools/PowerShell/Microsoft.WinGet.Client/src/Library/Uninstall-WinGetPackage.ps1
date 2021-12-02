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

        .PARAMETER Channel
        Used to specify the Channel for the package. Note the Windows Package Manager as of version 1.1.0 does not support channels.

        .PARAMETER Source
        Name of the Windows Package Manager private source. Can be identified by running: "Get-WinGetSource" and using the source Name

        .PARAMETER Manifest
        Used to specify the Manifest of the package

        .PARAMETER Interactive
        Used to specify the installer should be run in interactive mode.

        .PARAMETER Silent
        Used to specify the installer should be run in silent mode with no user input.

        .PARAMETER Log
        Used to specify the location for the log location if it is supported by the package uninstaller.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.
        
        
        .PARAMETER AcceptSourceAgreement
        Used to explicitly accept any agreement required by the source.

        .EXAMPLE
        Get-WinGetManifest -id "Publisher.Package"

        This example expects only a single configured REST source with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Get-WinGetManifest -id "Publisher.Package" -source "Private"

        This example expects the REST source named "Private" with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Get-WinGetManifest -Name "Package"

        This example expects the REST source named "Private" with a package containing "Package" as a valid name.
    #>

    PARAM(
        
    )
    BEGIN
    {

    }
    PROCESS
    {

    }
    END
    {

    }
}

New-Alias -Name Remove-WinGetPackage -Value Uninstall-WinGetPackage

Export-ModuleMember -Function Uninstall-WinGetPackage
Export-ModuleMember -Alias Remove-WinGetPackage
