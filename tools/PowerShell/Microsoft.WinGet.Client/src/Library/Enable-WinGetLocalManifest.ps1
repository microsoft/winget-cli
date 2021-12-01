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

    PARAM {
        [Parameter()] [switch]                  $VerboseLog
    }
    BEGIN
    {
        ## We might move this code to a utility function rather than duplicate it everywhere it's needed
        ## We also need to look for a better way to make sure the terminal is elevated
        function Test-IsAdmin
            {
                    
                $windowsIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
                $windowsPrincipal = new-object 'Security.Principal.WindowsPrincipal' $windowsIdentity
                if ($windowsPrincipal.IsInRole("Administrators") -eq 1) { $true } else { $false }
                   
            }
        
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