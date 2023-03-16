# WinGet and Desired State Configuration

The `winget configure` command (available as an experimental feature in WinGet 1.5-preview(3) and later) is designed to streamline the process of getting your Windows environment to a desired state. WinGet is leveraging PowerShell Core (not Windows PowerShell) and PowerShell DSC (Desired State Configuration) to achieve this goal. WinGet is introducing a new YAML formatted file to define the desired state in a declarative manner rather than an imperative manner. Our goal is to incorporate this new file and it's syntax into PowerShell DSC v3 in the future.

Many users will be new to "Desired State Configuration" as well as the concept of "Configuration as Code". If you want to understand the basics of what is happening when you run `winget configure <configuration file>` you can take a look at our "PowerShell DSC Primer" below.

## PowerShell DSC Primer

Getting Started with PowerShell DSC in the context of learning the basics of `winget configure`.

This section outlines the process of ensuring you have a PowerShell DSC v2 compatible version of PowerShell. Then it walks you through the process of installing PowerShell Desired State Configuration module. Next it walks you through installing the PowerShell Desired State Resources module. Finally you are given examples to run `Invoke-DscResource` and validate the behavior.

### Ensure you are using PowerShell 7.2 or later

First you need to have PowerShell 7.2 or later. You can run `$PSVersionTable` in PowerShell if you see the following result, you need to switch to a more recent version of PowerShell or install one.

```PowerShell
PS C:\Users\denelon> $PSVersionTable

Name                           Value
----                           -----
PSVersion                      5.1.22621.963
PSEdition                      Desktop
PSCompatibleVersions           {1.0, 2.0, 3.0, 4.0...}
BuildVersion                   10.0.22621.963
CLRVersion                     4.0.30319.42000
WSManStackVersion              3.0
PSRemotingProtocolVersion      2.3
SerializationVersion           1.1.0.1
```

To install or upgrade PowerShell to the latest version run `winget install Microsoft.PowerShell`.
```PowerShell
Found PowerShell [Microsoft.PowerShell] Version 7.3.3.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://github.com/PowerShell/PowerShell/releases/download/v7.3.3/PowerShell-7.3.3-win-x64.msi
  ██████████████████████████████   101 MB /  101 MB
Successfully verified installer hash
Starting package install...
Successfully installed
```

*Note: At the time of this writing, the latest version available is 7.3.3.0

Launch PowerShell and run `$PSVersionTable` to verify PSVersion is greater than 7.2:

```PowerShell
PS C:\Users\denelon> $PSVersionTable

Name                           Value
----                           -----
PSVersion                      7.3.3
PSEdition                      Core
GitCommitId                    7.3.3
OS                             Microsoft Windows 10.0.22621
Platform                       Win32NT
PSCompatibleVersions           {1.0, 2.0, 3.0, 4.0…}
PSRemotingProtocolVersion      2.3
SerializationVersion           1.1.0.1
WSManStackVersion              3.0
```

### Install the PSDesiredStateConfiguration module

These instructions are taken from [Get started with invoking DSC resources](
https://learn.microsoft.com/powershell/dsc/getting-started/invoking-dsc-resources?view=dsc-2.0).

Now you will need to install PSDesiredStateConfiguration by running
`Install-Module -Name PSDesiredStateConfiguration -RequiredVersion 2.0.6`

If you see:
```PowerShell
Untrusted repository
You are installing the modules from an untrusted repository. If you trust this repository, change its
 InstallationPolicy value by running the Set-PSRepository cmdlet. Are you sure you want to install
the modules from 'PSGallery'?
[Y] Yes  [A] Yes to All  [N] No  [L] No to All  [S] Suspend  [?] Help (default is "N"):
```

In this case, you can respond with "Y" if you trust the [PowerShell Gallery](https://www.powershellgallery.com/) to install this module.

To verify the command succeeded you can run `Get-Module -listavailable PSDesiredStateConfiguration` and you should see the following output.

```PowerShell
PS C:\Users\denelon> Get-Module -listavailable PSDesiredStateConfiguration

    Directory: C:\Users\denelon\Documents\PowerShell\Modules

ModuleType Version    PreRelease Name                                PSEdition ExportedCommands
---------- -------    ---------- ----                                --------- ----------------
Script     2.0.6                 PSDesiredStateConfiguration         Core      {Configuration, New-D…
```

### Install the PSDesiredStateConfiguration module

Run `Install-Module PSDscResources` and you will be prompted with the familiar "Untrusted repository" where you may decide to trust the [PowerShell Gallery](https://www.powershellgallery.com/).

```PowerShell
PS C:\Users\denelon> Get-Module -listavailable PSDscResources

    Directory: C:\Users\denelon\Documents\PowerShell\Modules

ModuleType Version    PreRelease Name                                PSEdition ExportedCommands
---------- -------    ---------- ----                                --------- ----------------
Manifest   2.12.0.0              PSDscResources                      Desk
```

### Invoking DSC Resources
The system is now in the proper state to install a PowerShell DSC Resource so you can run `Invoke-DSCResource`.

**Note:** *We can ignore the `Import-DscResource -ModuleName PSDscResources` command since we're looking at `winget configure` and not the traditional PowerShell DSC v2 implementation.*

In the example, we are setting a Process scoped environment variable. Before you run through the example you should validate the state of the environment variable outside of the context of PowerShell DSC as a form of independent verification that the desired state is being correctly modified. We will use `$ENV:DSC_EXAMPLE` to check for the presence, absence, and the value for the "DSC_EXAMPLE" environment variable.

An empty response indicates the environment variable is absent. When the variable is present, the value is returned when you run `$ENV:DSC_EXAMPLE`.

```
PS C:\Users\denelon> $ENV:DSC_EXAMPLE
Desired State Configuration
```

These examples are intentionally pedantic to ensure you have an understanding of the different behaviors exhibited when using `Invoke-DscResource`. When you are using DSC Resources to "get", "set", and "test" behaviors, you should independently verify the output. In this case you can run `$ENV:DSC_EXAMPLE` to verify the state of the Process scoped environment variable.

 We will start with the example provided at [Invoking DSC Resources](https://learn.microsoft.com/powershell/dsc/getting-started/invoking-dsc-resources?view=dsc-2.0#invoking-dsc-resources). 

 To remove the Process scoped environment variable run:
 ```PowerShell
 Invoke-DscResource -Name Environment -Module PSDscResources -Method Set -Property @{
    Name   = 'DSC_EXAMPLE'
    Ensure = 'Absent'
    Value  = 'Desired State Configuration'
    Target = 'Process'
}
```
To verify the removal of the Process scoped environment variable run:
 ```PowerShell
 Invoke-DscResource -Name Environment -Module PSDscResources -Method Test -Property @{
    Name   = 'DSC_EXAMPLE'
    Ensure = 'Absent'
    Value  = 'Desired State Configuration'
    Target = 'Process'
}
```

## WinGet Configure

In the WinGet 1.5-preview(3) the `winget configure` command is available.

You will need to enable the experimental feature using `winget settings`.
```YAML
    "experimentalFeatures": {
        "configuration": true
    }
```
**Note:** *This must also be run in PowerShell 7.2 or later (not Windows PowerShell or CMD.exe).*

Copy the following into a file named "configuration.dsc.yaml".
```YAML
# yaml-language-server: $schema=https://aka.ms/configuration-dsc-schema/0.1
properties:
  configurationVersion: '0.1.0'
  assertions:
    - resource: Environment
      attributes:
        module: PSDscResources
      settings:
        Name: 'DSC_EXAMPLE'
        Ensure: 'Absent'
        Value: 'Desired State Configuration'
        Target: 'Process'
  resources:
    - resource: Environment
      attributes:
        module: PSDscResources
      settings:
        Name: 'DSC_EXAMPLE'
        Ensure: 'Present'
        Value: 'Desired State Configuration'
        Target: 'Process'
```

You can use `$ENV:DSC_EXAMPLE` to validate the environment variable is not present for the first "assertion". 

Resources are not currently automatically loaded from the PowerShell gallery. You may need to install the two PowerShell modules referenced above (PSDesiredStateConfiguration and PSDscResources) if you did not try the `Invoke-DscResource` method described above.

### `winget configure show`
To "view" a configuration, run `winget configure show <path to configuration.dsc.yaml>`

**Note:** *If you see the error message "Failed to get detailed information about the confguration." you need to switch to PowerShell 7.2 or later.

### `winget configure`
Then you can run `winget configure <path to configuration.dsc.yaml>`.

**Note:** *If you se the error message "An unexpected error occurred while executing the command: 0x80131515 : unknown error" you need to switch to PowerShell 7.2 or later

**Note:** *The command is run in a child process so the environment variable is not present when you return to the PowerShell process. Running `$ENV:DSC_EXAMPLE` will not show the environment variable present after running the configuration*

This is a non-destructive configuration as nothing is retained in the system. 

If you want empiracal evidence that something is actually being performed, change the value in "Target" to `Machine` (in "configuration.dsc.yaml") and run the command in an Administrative session. After the configuration completes, you will se the value in the system environment variables.

To change the state of the environment variables back, you can either delete the System variable in the "Environment Variables" or edit in the configuration.dsc.yaml file and swap the "Ensure: 'Present'" and the "Ensure: 'Absent'".

### Help

With the experimental "configure" feature enabled you can query for help with either `winget configure show -?` or `winget configure show --help` for the show sub command. You can get help for the configure command using `winget configure -?` or `winget configure --help`.