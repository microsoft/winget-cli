<#
.SYNOPSIS
    Updates the given version header file to match the version info parse from git.
.DESCRIPTION
    Uses "git describe --tags" to determine the current version, then updates the given file
    to match.  See the existing version.h for the format.
.PARAMETER TargetFile
    The file to update with version information.  If not given, simply outputs the version info.
.PARAMETER BuildVersion
    The build version to use.
.PARAMETER OutVar
    Output a pipeline variable with the version.
#>
param(
    [Parameter(Mandatory=$false)]
    [string]$TargetFile,
    
    [Parameter(Mandatory=$false)]
    [int]$BuildVersion = 0,

    [Parameter(Mandatory=$false)]
    [string]$MajorMinorOverride,

    [switch]$OutVar
)

$Local:Major = 0;
$Local:Minor = 0;

if ($MajorMinorOverride -match "([0-9]+)\.([0-9]+)")
{
    $Local:Major = $Matches[1]
    $Local:Minor = $Matches[2]
}
else {
    $ErrorActionPreference = 'SilentlyContinue'
    $Local:GitDescribeText = git describe --tags;
    $ErrorActionPreference = 'Continue'

    Write-Host "Git describe: $Local:GitDescribeText"
    
    if ($Local:GitDescribeText -match "v([0-9]+)\.([0-9]+)")
    {
        $Local:Major = $Matches[1]
        $Local:Minor = $Matches[2]
    }
    else
    {
        Write-Host "Describe did not match regex and major/minor weren't explicitly provided; using zeros"
    }
}

Write-Host "Using version: $Local:Major.$Local:Minor.$BuildVersion"

if ($OutVar)
{
    Write-Host "##vso[task.setvariable variable=tag;isOutput=true]$Local:Major.$Local:Minor"
}

if (![String]::IsNullOrEmpty($TargetFile))
{
    $Local:FullPath = Resolve-Path $TargetFile
    Write-Host "Updating file: $Local:FullPath"
    if (Test-Path $TargetFile)
    {
        $Local:ResultContent = ""
        foreach ($Local:line in [System.IO.File]::ReadLines($Local:FullPath))
        {
            if ($Local:line.StartsWith("#define VERSION_MAJOR"))
            {
                $Local:ResultContent += "#define VERSION_MAJOR $Local:Major";
            }
            elseif ($Local:line.StartsWith("#define VERSION_MINOR"))
            {
                $Local:ResultContent += "#define VERSION_MINOR $Local:Minor";
            }
            elseif ($Local:line.StartsWith("#define VERSION_BUILD"))
            {
                $Local:ResultContent += "#define VERSION_BUILD $BuildVersion";
            }
            else
            {
                $Local:ResultContent += $Local:line;
            }
            $Local:ResultContent += [System.Environment]::NewLine;
        }
        Set-Content -Path $Local:FullPath -Value $Local:ResultContent
    }
    else
    {
        Write-Error "Did not find target file: $TargetFile"
    }
}
