Function Disable-WinGetLocalManifest
{
    <#
        .SYNOPSIS
        Disables the Windows Package Manager to work with local manifest files. 
        
        .DESCRIPTION
        By running this cmdlet the Windows Package Manager will configured not to support working with manifests on the file system.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.

        .EXAMPLE
        Disable-WinGetLocalManifest

        This cmdlet must be executed in an administrative terminal.
    #>
    PARAM (
        [Parameter()] [switch] $VerboseLog

    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Settings", "--disable", "LocalManifestFiles"
        $WinGetArgs += "--Verbose-Logs", $VerboseLog
    }
    PROCESS{
        & "WinGet" $WingetArgs
    }
    END{
        return
    }
}

Export-ModuleMember -Function Disable-WinGetLocalManifest