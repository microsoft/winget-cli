Function Remove-WinGetSource
{
    <#
        .SYNOPSIS
        Removes a source from the Windows Package Manager. 
        
        .DESCRIPTION
        By running this cmdlet will remove an existing source from the Windows Package Manager.

        .PARAMETER Name
        Used to specify the Name of the source

        .EXAMPLE
        Remove-WinGetSource -Name "Private"

        This will remove the source named "Private" from the Windows Package Manager
    #>

    PARAM(
        [Parameter(Mandatory=$true)] [string]   $Name
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Source", "remove"
        $WinGetArgs += "--Name", $Name
    }
    PROCESS
    {
        WinGet $WingetArgs
    }
    END
    {
        return
    }
}

Export-ModuleMember -Function Remove-WinGetSource