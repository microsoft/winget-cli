@{
    RootModule = "Microsoft.WinGet.Configuration.Cmdlets.dll"
    ModuleVersion = '0.0.1'
    CompatiblePSEditions = 'Core'
    GUID = '79b6b07b-7be5-4673-9cd1-fcbe3d79ba82'
    Author = 'Microsoft Corporation'
    CompanyName = 'Microsoft Corporation'
    Copyright = '(c) Microsoft Corporation. All rights reserved.'
    Description = 'PowerShell Module for the Windows Package Manager Configuration.'
    PowerShellVersion = '7.4.6'

    FunctionsToExport = @()
    AliasesToExport      = @('cmpwgc', 'cnwgc', 'ctwgcy', 'gwgc', 'gwgcd', 'iwgc', 'rwgch', 'sawgc', 'spwgc','twgc')

    CmdletsToExport = @(
        "Complete-WinGetConfiguration"
        "Get-WinGetConfiguration"
        "Get-WinGetConfigurationDetails"
        "Invoke-WinGetConfiguration"
        "Start-WinGetConfiguration"
        "Test-WinGetConfiguration"
        "Confirm-WinGetConfiguration"
        "Stop-WinGetConfiguration"
        "Remove-WinGetConfigurationHistory"
        "ConvertTo-WinGetConfigurationYaml"
    )

    PrivateData = @{
        PSData = @{
            Tags = @(
                'WindowsPackageManager',
                'WinGet'
            )
            ProjectUri = 'https://github.com/microsoft/winget-cli'
            IconUri = 'https://aka.ms/winget-icon'
            Prerelease = 'alpha'
        }
    }
}

