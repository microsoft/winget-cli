Function Enable-WinGetLocalManifest
{
    <#
        .SYNOPSIS
        Enables the Windows Package Manager to work with local manifest files. 
        
        .DESCRIPTION
        By running this cmdlet the Windows Package Manager will configured to support working with manifests on the file system.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.
    #>

    PARAM (
        [Parameter()] [switch]                  $VerboseLog
    )
    BEGIN
    {     
        [string[]] $WinGetArgs  = "Settings", "--enable", "LocalManifestFiles"
        $WinGetArgs += "--Verbose-Logs", $VerboseLog
    }
    PROCESS{
        & "WinGet" $WingetArgs
    }
    END{
        return
    }
}

Export-ModuleMember -Function Enable-WinGetLocalManifest