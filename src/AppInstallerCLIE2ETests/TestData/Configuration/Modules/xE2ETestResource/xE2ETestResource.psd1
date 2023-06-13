﻿#
# Module manifest for module 'xE2ETestResource'
#

@{

RootModule = 'xE2ETestResource.psm1'
ModuleVersion = '0.0.0.1'
GUID = 'a0be43e8-ac22-4244-8efc-7263dfa50b8c'
CompatiblePSEditions = 'Core'
Author = 'WinGet Dev Team'
CompanyName = 'Microsoft Corporation'
Copyright = '(c) Microsoft Corporation. All rights reserved.'
Description = 'PowerShell module with DSC resources for unit tests'
PowerShellVersion = '7.2'
FunctionsToExport = @()
CmdletsToExport = @()
DscResourcesToExport = @(
    'E2EFileResource'
    'E2ETestResource'
    'E2ETestResourceThrows'
    'E2ETestResourceError'
    'E2ETestResourceTypes'
    'E2ETestResourceCrash'
)
HelpInfoURI = 'https://www.contoso.com/help'

# Private data to pass to the module specified in RootModule/ModuleToProcess. This may also contain a PSData hashtable with additional module metadata used by PowerShell.
PrivateData = @{

    PSData = @{
        ProjectUri = 'https://github.com/microsoft/winget-cli'
        IconUri = 'https://www.contoso.com/icons/icon.png'
    }

}

}
