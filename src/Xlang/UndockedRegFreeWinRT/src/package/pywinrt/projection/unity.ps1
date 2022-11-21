$pywinrt_src_path = "$PSScriptRoot/pywinrt/winrt/src"
$pywinrt_unity_path = "$PSScriptRoot/pywinrt/winrt/unity"

remove-item $pywinrt_unity_path -Recurse -Force -ErrorAction SilentlyContinue
mkdir $pywinrt_unity_path | Out-Null

$coreFiles = Get-ChildItem $pywinrt_src_path\*.cpp | 
    Where-Object {-not $_.Name.StartsWith("py.")} 

$unityGroups = Get-ChildItem $pywinrt_src_path\*.cpp | 
    Where-Object {$_.Name.StartsWith("py.")} | 
    Group-Object -Property { ($_.name.split('.') | Select-Object -first 1 -skip 2)}

foreach ($g in $unityGroups)
{
    Write-Output "Unity: $($g.Name)"
    $cppIncludes = $g.group | ForEach-Object{"#include `"$($_.fullname)`""}
    "#include `"pch.h`"`n" | Set-Content -Path "$pywinrt_unity_path/$($g.Name).cpp" 
    $cppIncludes -join "`n" | add-Content -Path "$pywinrt_unity_path/$($g.Name).cpp" 
}

foreach ($f in $coreFiles)
{
    Write-Output "Core: $($f.Name)"
    Copy-Item $f $pywinrt_unity_path
}