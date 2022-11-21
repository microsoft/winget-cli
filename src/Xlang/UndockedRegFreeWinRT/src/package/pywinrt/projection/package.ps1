param([switch]$clean)

$pythonTag = "cp37"

if ($env:VSCMD_ARG_TGT_ARCH -eq "x64") {
    $platformTag = "win_amd64"
}
elseif ($env:VSCMD_ARG_TGT_ARCH -eq "x86") {
    $platformTag = "win32"
}
else {
    Write-Error "VSCMD_ARG_TGT_ARCH not set to an expected value (x86 or x64)"
    exit 
}

Push-Location $PSScriptRoot/pywinrt 

if ($clean) {
    Remove-Item build -Recurse
    Remove-Item dist -Recurse
    Remove-Item *.egg-info -Recurse
}
else {
    py ..\setup.py bdist_wheel --python-tag $pythonTag --plat-name $platformTag 
}

Pop-Location