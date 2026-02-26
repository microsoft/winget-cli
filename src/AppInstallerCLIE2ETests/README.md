# How to Run End-To-End Tests for Windows Package Manager Client

Most of the tests require having the local test source added into winget. The local test source must be hosted in a localhost web server.

## Run locally
The E2E tests are built on the nunit testing framework and can be configured with a Test.runsettings file. The project has its own default parameters but typically you will want to expand it by modifying **src\AppInstallerCLIE2ETests\Test.runsettings** with the parameters that you want and set it up by opening **Test Explorer - Settings - Configure Run Settings point to file: src\AppInstallerCLIE2ETests\Test.runsettings**

If your tests uses the test source see the [LocalhostWebServer](#LocalhostWebServer) and [WinGetSourceCreator](#WinGetSourceCreator) sections.

### Run settings.
|Parameter| Description  |
|--|--|
| PackagedContext | Indicates if the test should be run under packaged context |
| VerboseLogging | Turn on/off verbose logging in the test result |
| AICLIPath | The AICLI executable under test. If using loose file registration and using Invoke-Command when         AppExecutionAlias is not available, this will be relative path to package root. |
| AICLIPackagePath | Used in packaged context. Path to the package containing executable under test. If LooseFileRegistration is true, this should be path to unpackaged root. |
| LooseFileRegistration | Bool to set if loose file registration should be used. |
| InvokeCommandInDesktopPackage | Bool to indicate using Invoke-CommandInDesktopPackage for test execution. This is used when AppExecutionAlias is not available, or disabled. |
| StaticFileRootPath | Path to the set of static test files that will be served as the source for testing purposes. This path should be identical to the one provided to the LocalHostWebServer|
| MsixTestInstallerPath | The MSIX (or APPX) Installer executable under test. |
| ExeTestInstallerPath |The Exe Installer executable under test. |
| MsiTestInstallerPath | The MSI Installer executable under test. |
| PackageCertificatePath | Signing Certificate Path used to certify test index source package |
| PowerShellModulePath | Path to the PowerShell module manifest file under test |
| PowerShellModulePath | The local server cert file |
| SkipTestSource | I solemnly swear the test won't use the local test source or the source is already set up. |

#### Example of Test.runsettings with completed parameters:
Assuming you clone winget-cli in c:\dev, the localhost web server is running in c:\dev\TestLocalIndex (without creating the test source) and you built x64 Debug, this should cover most of the tests.

    <RunSettings>
        <TestRunParameters>
            <Parameter name="PackagedContext" value="false" />
            <Parameter name="VerboseLogging" value="true" />
            <Parameter name="AICLIPath" value="c:\dev\winget-cli\src\AppInstallerCLIPackage\bin\x64\Debug\AppInstallerCLI\winget.exe" />
            <Parameter name="InvokeCommandInDesktopPackage" value="false" />
            <Parameter name="StaticFileRootPath" value="c:\dev\TestLocalIndex" />
            <Parameter name="MsixTestInstallerPath" value="C:\dev\Temp\AppInstallerTestMsixInstaller.msix" />
            <Parameter name="ExeTestInstallerPath" value="c:\dev\winget-cli\src\x64\Debug\AppInstallerTestExeInstaller\AppInstallerTestExeInstaller.exe" />
            <Parameter name="MsiTestInstallerPath" value="c:\dev\winget-cli\src\AppInstallerCLIE2ETests\TestData\AppInstallerTestMsiInstaller.msi" />
            <Parameter name="PackageCertificatePath" value="C:\dev\Temp\packageCertificate.pfx"/>
            <Parameter name="PowerShellModulePath" value="c:\dev\winget-cli\src\x64\Debug\PowerShell\Microsoft.WinGet.Client.psd1" />
        </TestRunParameters>
    </RunSettings>

The easiest way to generate AppInstallerTestMsixInstaller.msix is by running makeappx pack for c:\dev\winget-cli\src\AppInstallerTestMsixInstaller\bin\x64\Debug.

#### Log Files
After running the E2E Tests, the logs can be found in the following paths:

- **%LOCALAPPDATA%\Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\DiagOutputDir**
- **%LOCALAPPDATA%\E2ETestLogs**
- **%TEMP%\WinGet\defaultState**

## LocalhostWebServer
The src\Tool\LocalhostWebServer project generates an executable that serves static test files from a given directory path through a HTTPS local loopback server in order to maintain a closed and controlled repository for resources used for testing purposes.

### Start localhost web server

The localhost web server needs to be running for the duration of the tests. The easiest way to run it is to use src\Tool\LocalhostWebServer\Run-LocalhostWebServer.ps1 in a different PowerShell session.

|Parameter | Type | Description |
|--|--|--|
| **BuildRoot** | Mandatory | The output path of the LocalhostWebServer project. Normally something like <winget repo root>\src\<arch>\<configuration>\LocalhostWebServer
| **StaticFileRoot** | Mandatory | Path to serve static root directory. If the directory path does not exist, a new directory will be created for you. |
| **CertPath** | Mandatory | Path to HTTPS Developer Certificate. A self signed developer certificate will need to be created in order to  verify localhost https. |
| **CertPassword** | Mandatory | HTTPS Developer Certificate Password |
| **Port** | Optional | Port number [Default Port Number: 5001] |
| **OutCertFile** | Optional | The exported certificate used |
| **LocalSourceJson** | Optional | The local source definition. If set generates the source. |
| **ForcedExperimentalFeatures** | Optional | Experimental features that should be forcibly enabled always. |

### How to create and trust an ASP.NET Core HTTPS Development Certificate
Windows Package Manager Client (WinGet.exe) requires new sources added to the WinGet repositories be securely accessed through HTTPS. Therefore, in order to verify the LocalhostWebServer, you will need to create a self-signed development certificate to verify the localhost address. 

- Open command prompt in administrator mode
- Run **dotnet dev-certs https --trust** in the command line
- Open up **certmgr** (search Manage User Certificates in Windows search bar) 
- Locate the newly created localhost certificate in the Personal/Certificates folder with a friendly name of "ASP.NET Core HTTPS development certificate"
- Right click on the certificate --> All Tasks --> Export..
- Click Yes to export the private key
- Export file using Personal Information Exchange (.pfx) file format
- Create and confirm password using SHA256 encryption (any password will work, just make sure to remember it for later)
- Save HTTPS development certificate and refer its certificate path and password when launching the Localhost Webserver

## WinGetSourceCreator
The src\WinGetSourceCreator is a project that helps generate a new winget source for local development. It is consumed by IndexCreationTool, LocalhostWebServer and AppInstallerCLIE2ETests projects. It supports:
- Prepare installers by signing them and placing then in the working directory and computing their hashes. For msix, signature hash is also supported.
- Generate zip installers.
- Generate the signed source.msix and index.db.

LocalSource is the object that contains the definition of the source. A json serialized version of it is the input for IndexCreationTool and LocalhostWebServer.

Example:
```
{
  # If running E2E this is must be the StaticFileRoot used for the localhost web server
  "WorkingDirectory": "c:/dev/temp/TestLocalIndex",

  # The appx manifest to generate the source.msix file.
  "AppxManifest": "c:/dev/winget-cli/src/AppInstallerCLIE2ETests/TestData/Package/AppxManifest.xml",

  # A list of directories or files to copy. If a directory, it copies all the *.yaml files preserving subdirectories.
  "LocalManifests": [
    "c:/dev/winget-cli/src/x86/Release/AppInstallerCLIE2ETests/TestData/Manifests"
  ],

  # The signature to use.
  "Signature": {
    "CertFile": "cert.pfx",
    "Password": "1324",

    # If set it will modify the Package Identity Publisher in the AppxManifest.xml
    "Publisher": "CN:ThousandSunny"
  }

  # Installers that are already present in the machine, by default the installers will be signed using their Signature
  # property if set or the top level one.
  "LocalInstallers": [
    {
      "Type": "exe",
      "Input": "c:/dev/winget-cli/src/x64/Debug/AppInstallerTestExeInstaller\AppInstallerTestExeInstaller.exe",

      # Name of the installer to be copied and signed if needed.
      "Name": "AppInstallerTestExeInstaller/AppInstallerTestExeInstaller.exe",

      # The token in the manifests for this installer. This will be replaces at copy manifests time.
      "HashToken": "<EXEHASH>"

      # Overrides top level one.
      "Signature": {
          "CertFile": "cert2.pfx",
          "Password": "2345",
      }
    },
    {
      "Type": "msi",
      "Input": "c:/dev/winget-cli/src/AppInstallerCLIE2ETests/TestData/AppInstallerTestMsiInstaller.msi",
      "Name": "AppInstallerTestMsiInstaller/AppInstallerTestMsiInstaller.msi",
      "HashToken": "<MSIHASH>"

      # Don't sign this.
      "SkipSignature": true
    },
    {
      "Type": "msix",
      "Input": "D:/dev/temp/AppInstallerTestMsixInstaller.msix",
      "Name": "AppInstallerTestMsixInstaller/AppInstallerTestMsixInstaller.msix",
      "HashToken": "<MSIXHASH>",

      # Only supported by where type is msix. Package must be signed, either already signed or signed when copied.
      "SignatureToken": "<SIGNATUREHASH>",
    }
  ],

  # These are installers that are generated on the go. Currently only zip is supported.
  "DynamicInstallers": [
    {
      # Zip installers are never signed.
      "Type": "zip",

      # List of files to zip. Does not preserve subdirectories.
      "Input": [
        "D:/dev/temp/TestLocalIndex/AppInstallerTestExeInstaller/AppInstallerTestExeInstaller.exe",
        "D:/dev/temp/TestLocalIndex/AppInstallerTestMsiInstaller/AppInstallerTestMsiInstaller.msi",
        "D:/dev/temp/TestLocalIndex/AppInstallerTestMsixInstaller/AppInstallerTestMsixInstaller.msix"
      ],

      "Name": "AppInstallerTestZipInstaller/AppInstallerTestZipInstaller.zip",

      "HashToken": "<ZIPHASH>"
    }
  ],
}
```
