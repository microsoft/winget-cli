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
        Add-WinGetSource -Name "custom" -Argument "https://contoso.com/" -Type "Microsoft.Rest" -Header "string" -AcceptSourceAgreement:$true

        This example adds a new source to the Windows Package Manager named "custom" and passes the value "string" in the Windows-Package-Manager HTTP header to the source.
    #>

    PARAM(
        [Parameter(Mandatory=$true)] [string]   $Name,
        [Parameter(Mandatory=$true)] [string]   $Argument,
        [Parameter(Mandatory=$true)] [ValidateSet("Microsoft.Rest", "Microsoft.PreIndexed.Package")] [string]$Type,
        [Parameter()] [ValidateLength(1, 1024)] $Header,
        [Parameter()] [bool]                    $AcceptSourceAgreement
    )
    BEGIN
    {
        if ($AcceptSourceAgreement) {$acceptOption = '--accept-source-agreements'}
        [string] $WinGetArgs  = "Source Add --Name {0} --Arg {1} --Type {2} {3}" -f $Name, $Argument, $Type, $acceptOption 
    }
    PROCESS
    {
       Start-Process  -FilePath "winget.exe" -ArgumentList $WinGetArgs -Wait       
    }
    END
    {
        #Don't do this unless you are in a class 
        #return
    }
}

Export-ModuleMember -Function Add-WinGetSource
