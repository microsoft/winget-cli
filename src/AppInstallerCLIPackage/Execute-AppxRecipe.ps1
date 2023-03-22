[CmdletBinding()]
param(
    [Parameter(Mandatory=$true,Position=0)]
    [string]$AppxRecipePath,

    [string]$LayoutPath
)

[xml]$Local:recipe = Get-Content $AppxRecipePath

if ([System.String]::IsNullOrEmpty($LayoutPath))
{
    $LayoutPath = $Local:recipe.Project.PropertyGroup.LayoutDir
}

$Local:namespace = @{
    ns = "http://schemas.microsoft.com/developer/msbuild/2003"
}

$Local:manifestElement = Select-Xml -Xml $Local:recipe -Namespace $Local:namespace -XPath "//ns:AppXManifest" | Select-Object -ExpandProperty Node
$Local:packageFileElements = Select-Xml -Xml $Local:recipe -Namespace $Local:namespace -XPath "//ns:AppxPackagedFile" | Select-Object -ExpandProperty Node

$Local:allItems = $Local:packageFileElements + $Local:manifestElement

ForEach-Object -InputObject $Local:allItems -Process {
    $_.Include
}