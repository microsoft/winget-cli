Function Get-WinGetPackage{
    <#
        .SYNOPSIS
        Gets installed packages on the local system. displays the packages installed on the system, as well as whether an update is available. 
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

        .PARAMETER Count
        Used to specify the maximum number of packages to return
        
        .PARAMETER Exact
        Used to specify an exact match for any parameters provided. Many of the other parameters may be used for case-insensitive substring matches if Exact is not specified.

        .PARAMETER Source
        Name of the Windows Package Manager private source. Can be identified by running: "Get-WinGetSource" and using the source Name

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.
        
        .PARAMETER AcceptSourceAgreement
        Used to accept any source agreements required by a REST source.

        .EXAMPLE
        Get-WinGetPackage -id "Publisher.Package"

        This example expects only a single configured REST source with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Get-WinGetPackage -id "Publisher.Package" -source "Private"

        This example expects the REST source named "Private" with a package containing "Publisher.Package" as a valid identifier.

        .EXAMPLE
        Get-WinGetPackage -Name "Package"

        This example expects the REST source named "Private" with a package containing "Package" as a valid name.
    #>

    PARAM(
        [Parameter(Position=0)] $Filter,
        [Parameter()]           $Name,
        [Parameter()]           $Id,
        [Parameter()]           $Moniker,
        [Parameter()]           $Tag,
        [Parameter()]           $Source,
        [Parameter()]           $Command,
        [Parameter()]           [ValidateRange(1, [int]::maxvalue)][int]$Count,
        [Parameter()]           [switch]$Exact,
        [Parameter()]           [ValidateLength(1, 1024)]$Header,
        [Parameter()]           [switch]$AcceptSourceAgreement
    )
    BEGIN
    {
        [string[]]       $WinGetArgs  = @("List")
        [WinGetPackage[]]$Result      = @()
        [string[]]       $IndexTitles = @("Name", "Id", "Version", "Available", "Source")

        if($Filter){
            ## Search across Name, ID, moniker, and tags
            $WinGetArgs += $Filter
        }
        if($PSBoundParameters.ContainsKey('Name')){
            ## Search for the Name
            $WinGetArgs += "--Name", $Name.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Id')){
            ## Search for the ID
            $WinGetArgs += "--Id", $Id.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Moniker')){
            ## Search for the Moniker
            $WinGetArgs += "--Moniker", $Moniker.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Tag')){
            ## Search for the Tag
            $WinGetArgs += "--Tag", $Tag.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Source')){
            ## Search for the Source
            $WinGetArgs += "--Source", $Source.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Count')){
            ## Specify the number of results to return
            $WinGetArgs += "--Count", $Count
        }
        if($Exact){
            ## Search using exact values specified (case-sensitive)
            $WinGetArgs += "--Exact"
        }
        if($PSBoundParameters.ContainsKey('Header')){
            ## Pass the value specified as the Windows-Package-Manager HTTP header
            $WinGetArgs += "--header", $Header
        }
        if($AcceptSourceAgreement){
            ## Accept source agreements
            $WinGetArgs += "--accept-source-agreements"
        }
    }
    PROCESS
    {
        $List = Invoke-WinGetCommand -WinGetArgs $WinGetArgs -IndexTitles $IndexTitles
    
        foreach ($Obj in $List) {
            $Result += [WinGetPackage]::New($Obj) 
        }
    }
    END
    {
        return $Result
    }
}

Export-ModuleMember -Function Get-WinGetPackage