Function Get-WinGetSource
{
    <#
        .SYNOPSIS
        Gets the configured sources. 
        Additional options can be provided to filter the output, much like the search command.
        
        .DESCRIPTION
        By running this cmdlet with the will retrieve the sources configured on the local system.

        .PARAMETER Filter
        Used to search for the source by name.

        .PARAMETER Name
        Used to specify the Name of the package

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.
        
        .EXAMPLE
        Get-WinGetSource "winget"

        This example returns the configured source named "winget".

        .EXAMPLE
        Get-WinGetSource "winget" -Name "msstore"

        This example returns the configured source named "msstore".

        .EXAMPLE
        Find-WinGetPackage -Name "Package"

        This example searches for a package containing "Package" as a valid name on all configured sources.
    #>
    
    PARAM(
        [Parameter()] [string] $Name,
        [Parameter()] [switch] $VerboseLog
    )
    BEGIN
    {
        [string[]]       $WinGetArgs  = "Source", "Export"
        [WinGetSource[]] $Result      = @()
        [string[]]       $IndexTitles = @("Name", "Argument")

        if($PSBoundParameters.ContainsKey('Filter')){
            ## Search for the Name
            $WinGetArgs += "--Filter", $Filter.Replace("…", "")
        }
        if($PSBoundParameters.ContainsKey('Name')){
            ## Search for the Name
            $WinGetArgs += "--Name", $Name.Replace("…", "")
        }
        if($VerboseLog){
            ## Search for the Name
            $WinGetArgs += "--Verbose-Logs"
        }
    }
    PROCESS
    {
        $List = Invoke-WinGetCommand -WinGetArgs $WinGetArgs -IndexTitles $IndexTitles -JSON

        foreach ($Obj in $List) {
            #$Result += [WinGetSource]::New($Obj.Name, $Obj.Argument) 
            $Result += @{
                Name       = $Obj.Name
                Argument   = $Obj.Arg
                Data       = $Obj.Data
                Identifier = $Obj.Identifier
                Type       = $Obj.Type
            }
        }
    }
    END
    {
        return $Result
    }
}

Export-ModuleMember -Function Get-WinGetSource