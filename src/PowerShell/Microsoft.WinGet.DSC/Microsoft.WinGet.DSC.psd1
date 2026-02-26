#
# Module manifest for module 'Microsoft.WinGet.DSC'
#
# Created by: Microsoft Corporation
#

@{

    # Script module or binary module file associated with this manifest.
    RootModule = 'Microsoft.WinGet.DSC.psm1'
    
    # Version number of this module.
    ModuleVersion = '0.0.1'
    
    # Supported PSEditions
    CompatiblePSEditions = 'Core'
    
    # ID used to uniquely identify this module
    GUID = '8c9326eb-595a-40eb-8696-b289e8085cad'
    
    # Author of this module
    Author = 'Microsoft Corporation'
    
    # Company or vendor of this module
    CompanyName = 'Microsoft Corporation'
    
    # Copyright statement for this module
    Copyright = '(c) Microsoft Corporation. All rights reserved.'
    
    # Description of the functionality provided by this module
    Description = 'PowerShell Module with DSC resources related to WinGet configurations'
    
    # Minimum version of the PowerShell engine required by this module
    PowerShellVersion = '7.2'
    
    # Name of the PowerShell host required by this module
    # PowerShellHostName = ''
    
    # Minimum version of the PowerShell host required by this module
    # PowerShellHostVersion = ''
    
    # Minimum version of Microsoft .NET Framework required by this module. This prerequisite is valid for the PowerShell Desktop edition only.
    # DotNetFrameworkVersion = ''
    
    # Minimum version of the common language runtime (CLR) required by this module. This prerequisite is valid for the PowerShell Desktop edition only.
    # ClrVersion = ''
    
    # Processor architecture (None, X86, Amd64) required by this module
    # ProcessorArchitecture = ''
    
    # Modules that must be imported into the global environment prior to importing this module
    RequiredModules = @('Microsoft.WinGet.Client')
    
    # Assemblies that must be loaded prior to importing this module
    # RequiredAssemblies = @()
    
    # Script files (.ps1) that are run in the caller's environment prior to importing this module.
    # ScriptsToProcess = @()
    
    # Type files (.ps1xml) to be loaded when importing this module
    # TypesToProcess = @()
    
    # Format files (.ps1xml) to be loaded when importing this module
    # FormatsToProcess = @()
    
    # Modules to import as nested modules of the module specified in RootModule/ModuleToProcess
    # NestedModules = @()
    
    # Functions to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no functions to export.
    # FunctionsToExport = @()
    
    # Cmdlets to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no cmdlets to export.
    # CmdletsToExport = @()
    
    # Variables to export from this module
    # VariablesToExport = @()
    
    # Aliases to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no aliases to export.
    # AliasesToExport = @()
    
    # DSC resources to export from this module
    DscResourcesToExport = @(
        'WinGetUserSettings'
        'WinGetAdminSettings'
        'WinGetSource'
        'WinGetPackageManager'
        'WinGetPackage'
    )
    
    # List of all modules packaged with this module
    # ModuleList = @()
    
    # List of all files packaged with this module
    # FileList = @()
    
    # Private data to pass to the module specified in RootModule/ModuleToProcess. This may also contain a PSData hashtable with additional module metadata used by PowerShell.
    PrivateData = @{
    
        PSData = @{
    
            # Tags applied to this module. These help with module discovery in online galleries.
            Tags = @(
                'PSEdition_Core',
                'Windows',
                'WindowsPackageManager'
            )
    
            # A URL to the license for this module.
            # LicenseUri = ''
    
            # A URL to the main website for this project.
            ProjectUri = 'https://github.com/microsoft/winget-cli'
    
            # A URL to an icon representing this module.
            IconUri = 'https://aka.ms/winget-icon'
    
            # ReleaseNotes of this module
            # ReleaseNotes = ''
    
            # Prerelease string of this module
            Prerelease = 'alpha'
    
            # Flag to indicate whether the module requires explicit user acceptance for install/update/save
            # RequireLicenseAcceptance = $false
    
            # External dependent modules of this module
            # ExternalModuleDependencies = @()
    
        } # End of PSData hashtable
    
    } # End of PrivateData hashtable
    
    # HelpInfo URI of this module
    # HelpInfoURI = ''
    
    # Default prefix for commands exported from this module. Override the default prefix using Import-Module -Prefix.
    # DefaultCommandPrefix = ''
    
    }
    