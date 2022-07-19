# Microsoft.WinGet.Client

This is the source code for the PowerShell module for the Windows Package Manager Client.

## Building
### Pre-Requisite Steps

A **PowerShell module** is merely a folder containing a **.psd1** file or a
**.psm1** file. In this scenario, we will be building a folder with a **.psd1**
file and all of the other necessary bits inside. But, before we do that, we need
to make sure of the following steps.

1. The project has been built for the **x64**, **x86**, and **ARM64**
   platforms in **Release** mode.
   - The build script also relies on the exact location and structure of the
     actual build files. That is, the build script will look for a **bin**
     directory in this folder and expects a directory structure like:
     - `bin\x64\Release\net6.0-windows10.0.22000.0\`
     - `bin\x86\Release\net461\`

### Running the Script
Once you've done all of the pre-requisite steps, you can build the PowerShell
module folder by running the PowerShell build script. For example,

- `.\Build-Module.ps1 -In bin -Out build\bin -Configuration Release`
