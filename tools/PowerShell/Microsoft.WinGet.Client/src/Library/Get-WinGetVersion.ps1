Function Get-WinGetVersion {
    <#
        .SYNOPSIS
        Gets the version from the Windows Package Manager.
        
        .DESCRIPTION
        By running this cmdlet, it will retrieve the version of the Windows Package Manager installed on the local system.

        .EXAMPLE
        Get-WinGetVersion
    #>

    BEGIN
    {
        [string[]] $WinGetArgs = "--version"
    }
    PROCESS
    {
        try {
            $Result = [version](& "winget" $WinGetArgs).trimstart("v")
        }
        catch {
            $Result = [version]"0.0.0.0"
        }
    }
    END
    {
        return $Result
    }
}

Export-ModuleMember -Function Get-WinGetVersion