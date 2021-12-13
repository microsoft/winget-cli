Function Reset-WinGetSource
{
    <#
        .SYNOPSIS
        Resets the sources for the Windows Package Manager. 
        
        .DESCRIPTION
        By running this cmdlet will reset sources for the Windows Package Manager.

        .PARAMETER Name
        Used to specify the Name of the source

        .EXAMPLE
        Reset-WinGetSource

        This will reset the default sources for the Windows Package Manager

        .EXAMPLE
        Reset-WinGetSource -Name "Private"

        This will reset the source named "Private" for the Windows Package Manager
    #>

    PARAM(
        [Parameter()] [string]   $Name
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Source", "reset"
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

Export-ModuleMember -Function Reset-WinGetSource