Function Add-WinGetSource
{
    <#
        .SYNOPSIS
        Adds a source to the Windows Package Manager. 
        
        .DESCRIPTION
        By running this cmdlet with the will add a new source to the Windows Package Manager.

        .PARAMETER Name
        Used to specify the Name of the source

        .PARAMETER Arg
        Used to specify the URL for the source

        .PARAMETER Type
        Used to specify the Type of source to add
        Currently the only two valid values are "Microsoft.Rest" and "Microsoft.PreIndexed.Package"

        .PARAMETER Header
        Used to specify the value to pass as the "Windows-Package-Manager" HTTP header for a REST source.

        .PARAMETER AcceptSourceAgreement
        Used to explicitly accept any agreement required by the source.

        .PARAMETER VerboseLog
        Used to provide verbose logging for the Windows Package Manager.
        
        .EXAMPLE
        Add-WinGetSource -Name "custom" -Argument "https://contoso.com/" -Type "Microsoft.Rest"

        This example adds a new source to the Windows Package Manager named "custom"

        .EXAMPLE
        Add-WinGetSource -Name "custom" -Argument "https://contoso.com/" -Type "Microsoft.Rest" -Header "string"

        This example adds a new source to the Windows Package Manager named "custom" and passes the value "string" in the Windows-Package-Manager HTTP header to the source.
    #>

    PARAM(
        [Parameter(Mandatory=$true)] [string]   $Name,
        [Parameter(Mandatory=$true)] [string]   $Argument,
        [Parameter(Mandatory=$true)] [ValidateSet("Microsoft.Rest", "Microsoft.PreIndexed.Package")] [string]   $Type,
        [Parameter()] [ValidateLength(1, 1024)] $Header,
        [Parameter()] [switch]                  $AcceptSourceAgreement
    )
    BEGIN
    {
        [string[]] $WinGetArgs  = "Source", "Add"
        $WinGetArgs += "--Name", $Name
        $WinGetArgs += "--Arg",  $Argument
        $WinGetArgs += "--Type", $Type
    }
    PROCESS
    {
        & "WinGet" $WingetArgs
    }
    END
    {
        return
    }
}

Export-ModuleMember -Function Add-WinGetSource