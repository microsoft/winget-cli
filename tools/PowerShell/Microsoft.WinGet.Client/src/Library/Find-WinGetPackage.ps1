Function Find-WinGetPackage{
    <#
        .SYNOPSIS
        Searches for a package on configured sources. 
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
        
        .PARAMETER Exact
        Used to specify an exact match for any parameters provided. Many of the other parameters may be used for case insensitive substring matches if Exact is not specified.

        .PARAMETER Source
        Name of the Windows Package Manager private source. Can be identified by running: "Get-WinGetSource" and using the source Name

        .PARAMETER Count
        Used to specify the maximum number of packages to return

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.
        
        .PARAMETER AcceptSourceAgreement
        Used to accept any source agreement required for the source.

        .EXAMPLE
        Find-WinGetPackage -id "Publisher.Package"

        This example searches for a package containing "Publisher.Package" as a valid identifier on all configured sources.

        .EXAMPLE
        Find-WinGetPackage -id "Publisher.Package" -source "Private"

        This example searches for a package containing "Publisher.Package" as a valid identifier from the source named "Private".

        .EXAMPLE
        Find-WinGetPackage -Name "Package"

        This example searches for a package containing "Package" as a valid name on all configured sources.
    #>
    PARAM(
        [Parameter(Position=0)] $Filter,
        [Parameter()]           $Id,
        [Parameter()]           $Name,
        [Parameter()]           $Moniker,
        [Parameter()]           $Tag,
        [Parameter()]           $Command,
        [Parameter()] [switch]  $Exact,
        [Parameter()]           $Source,
        [Parameter()] [ValidateRange(1, [int]::maxvalue)][int]$Count,
        [Parameter()] [ValidateLength(1, 1024)]$Header,
        [Parameter()] [switch]  $VerboseLog,
        [Parameter()] [switch]  $AcceptSourceAgreement
    )
    BEGIN
    {
        [string[]]          $WinGetArgs  = @("Search")
        [WinGetPackage[]]   $Result      = @()
        [string[]]          $IndexTitles = @("Name", "Id", "Version", "Available", "Source")

        if($PSBoundParameters.ContainsKey('Filter')){
            ## Search across Name, ID, moniker, and tags
            $WinGetArgs += $Filter
        }
        if($PSBoundParameters.ContainsKey('Id')){
            ## Search for the ID
            $WinGetArgs += "--Id", $Id.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Name')){
            ## Search for the Name
            $WinGetArgs += "--Name", $Name.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Moniker')){
            ## Search for the Moniker
            $WinGetArgs += "--Moniker", $Moniker.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Tag')){
            ## Search for the Tag
            $WinGetArgs += "--Tag", $Tag.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Command')){
            ## Search for the Moniker
            $WinGetArgs += "--Command", $Command.Replace("…", "")
        }
        if($Exact){
            ## Search using exact values specified (case sensitive)
            $WinGetArgs += "--Exact"
        }
        if($PSBoundParameters.ContainsKey('Source')){
            ## Search for the Source
            $WinGetArgs += "--Source", $Source.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Count')){
            ## Specify the number of results to return
            $WinGetArgs += "--Count", $Count
        }
        if($PSBoundParameters.ContainsKey('Header')){
            ## Pass the value specified as the Windows-Package-Manager HTTP header
            $WinGetArgs += "--header", $Header
        }
        if($PSBoundParameters.ContainsKey('VerboseLog')){
            ## Search using exact values specified (case sensitive)
            $WinGetArgs += "--VerboseLog", $VerboseLog
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

Export-ModuleMember -Function Find-WinGetPackage