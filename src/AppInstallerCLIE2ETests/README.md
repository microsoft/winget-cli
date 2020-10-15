# How to Run End-To-End Tests for Windows Package Manager Client


## Step 1: Launch Localhost Web Server
In order to run any of the E2E tests, you will need to first launch the LocalhostWebServer executable.  This program serves static test files from a given directory path through a HTTPS local loopback server in order to maintain a closed and controlled repository for resources used for testing purposes. 

### Parameters

The executable has 3 mandatory parameters and 1 optional parameter:
|Parameter Name	| Mandatory/Optional | Description |
|--|--|--|
| **StaticFileRoot** | Mandatory | Path to serve static root directory. If the directory path does not exist, a new directory will be created for you. |
| **CertPath** | Mandatory | Path to HTTPS Developer Certificate. A self signed developer certificate will need to be created in order to  verify localhost https. |
| **CertPassword** | Mandatory | HTTPS Developer Certificate Password |
| **Port** | Optional | Port number [Default Port Number: 5001] |

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
### How to run LocalhostWebServer.exe
The executable can most likely be found in this path: **\src\x86\Release\LocalhostWebServer**

The command line call to run the executable needs to follow the format:

	LocalhostWebServer.exe StaticFileRoot=<Path to Serve Static Root Directory> CertPath=<Path to HTTPS Developer Certificate> CertPassword=<Certificate Password> <Port=Port Number>

Therefore to run the executable in the command line, simply change into the directory that contains **LocalhostWebServer.exe** and run the executable with the corresponding parameter values. Here is an example: (Don't forget to modify the path to match your own local computer)

	cd C:\Users\MSFT\source\repos\winget-cli\src\AnyCPU\Debug\LocalhostWebServer

	LocalhostWebServer.exe StaticFileRoot=C:\Users\MSFT\AppData\Local\Temp\TestLocalIndex CertPath=C:\Users\MSFT\Temp\HTTPSDevCert.pfx CertPassword=password


## 2. Prepare Test.runsettings file
 The E2E tests are built on the nunit testing framework and rely on this test.runsettings file located here: **D:\Src\WinGet\Client\src\AppInstallerCLIE2ETests\Test.runsettings**. These parameters are used by the tests at runtime and need to be configured before running any of the E2E tests.
 After populating the parameters in the Test.runsettings file, make sure to configure the run settings to point to the file. This can be done by opening **Test Explorer - Settings - Configure Run Settings point to file: D:\Src\WinGet\Client\src\AppInstallerCLIE2ETests\Test.runsettings**


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
| PackageCertificatePath |Signing Certificate Path used to certify test index source package|

#### Example of Test.runsettings format:

	<RunSettings>
		<TestRunParameters>
			<Parameter name="PackagedContext" value="true" />
			<Parameter name="VerboseLogging" value="false" />
			<Parameter name="AICLIPath" value="AppInst.exe" />
			<Parameter name="AICLIPackagePath" value="AppInstallerCLIPackage.appxbundle" />
			<Parameter name="LooseFileRegistration" value="false" />
			<Parameter name="InvokeCommandInDesktopPackage" value="false" />
			<Parameter name="StaticFileRootPath" value="\TestLocalIndex" />
			<Parameter name="MsixTestInstallerPath" value="MsixTestInstaller.msix" />
			<Parameter name="ExeTestInstallerPath" value="ExeTestInstaller.exe" />
			<Parameter name="PackageCertificatePath" value="certificate.pfx"/>
		</TestRunParameters>
	</RunSettings>
  
#### Example of Test.runsettings with completed parameters:
Make sure to replace **MSFT** with your own user name. Modifying this example with the correct path to each test run parameter for your own local computer should be sufficient to successfully run the E2E tests  once all steps are completed.
   
	<RunSettings>
		<TestRunParameters>
			<Parameter name="PackagedContext" value="false" />
			<Parameter name="VerboseLogging" value="false" />
			<Parameter name="AICLIPath" value="C:\Users\<user>\source\repos\winget-cli\src\x64\Debug\AppInstallerCLI\AppInstallerCLI.exe" />
			<Parameter name="AICLIPackagePath" value="AppInstallerCLIPackage.appxbundle" />
			<Parameter name="LooseFileRegistration" value="false" />
			<Parameter name="InvokeCommandInDesktopPackage" value="false" />
			<Parameter name="StaticFileRootPath" value="C:\Users\MSFT\AppData\Local\Temp\TestLocalIndex" />
			<Parameter name="MsixTestInstallerPath" value="C:\Users\MSFT\Temp\MsixTestInstaller.msix" />
			<Parameter name="ExeTestInstallerPath" value="C:\Users\MSFT\source\repos\winget-cli\src\x64\Debug\AppInstallerTestExeInstaller\AppInstallerTestExeInstaller.exe" />
			<Parameter name="PackageCertificatePath" value="C:\Users\MSFT\Temp\packageCertificate.pfx"/>
		</TestRunParameters>
	</RunSettings>


#### Log Files
After running the E2E Tests, the logs can be found in either the following two paths:

- **%LOCALAPPDATA%\Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\DiagOutputDir**
- **%LOCALAPPDATA%\E2ETestLogs**
