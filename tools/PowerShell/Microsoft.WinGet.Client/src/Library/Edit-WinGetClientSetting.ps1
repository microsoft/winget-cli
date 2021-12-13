Function Edit-WinGetClientSetting
{
    <#
        .SYNOPSIS
        Opens the Windows Package Manager settings.json file in the default editor. 
        
        .DESCRIPTION
        By running this cmdlet will open the Windows Package Manager settings.json in the application configured for editing JSON files.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.

        .EXAMPLE
        Edit-WinGetClientSetting

        The settings.json file for the Windows Package Manager will be opened in the default .json editor
    #>

    
    PARAM (
        [Parameter()] [switch] $VerboseLog
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Settings"
        $WinGetArgs += "--Verbose-Logs", $VerboseLog
    }
    PROCESS{
        & "WinGet" $WingetArgs
    }
    END{
        return
    }
}

Export-ModuleMember -Function Edit-WinGetClientSetting