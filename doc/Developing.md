# Developer guidance

## Prerequisites

* Windows 10 1809 (17763) or later
* [Developer Mode enabled](https://docs.microsoft.com/windows/uwp/get-started/enable-your-device-for-development)
* [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)
  * Or use WinGet to install it ;) (although you may need to adjust the workloads via Tools->Get Tools and Features...)
* The following workloads:
  * .NET Desktop Development
  * Desktop Development with C++
  * Universal Windows Platform Development
  * Check [.vsconfig file](.vsconfig) for full components list

* [Windows SDK for Windows 11 (10.0.22000.194)](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/)

> [!NOTE]
> You can also get it through `winget install Microsoft.WindowsSDK --version 10.0.22000.832` (use --force if you have a newer version installed) or via Visual Studio > Get Tools and Features > Individual Components > Windows 10 SDK (10.0.22000.0)

* The following extensions:

  * [Microsoft Visual Studio Installer Projects](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects)

## Building the client

1. Clone the repository
2. Configure your system using the [configuration file](../.config/configuration.winget) in the repository. To run the configuration, use `winget configure .config/configuration.winget` from the project root so relative paths resolve correctly.
3. Run `vcpkg integrate install` from the Developer Command Prompt / Developer PowerShell for VS 2022. This is a one-time setup step until the configuration file in step 2 is updated to work with vcpkg setup.

Open `winget-cli\src\AppInstallerCLI.sln` in Visual Studio and build. We currently only build using the solution; command-line methods of building a VS solution should work as well.

## Running and Debugging

After the build finishes, deploy the solution from Build > Deploy Solution. You can then run the client from the command line using `wingetdev`.

You can also directly run the client from Visual Studio by setting the `AppInstallerCLIPackage` project as the startup project and pressing <kbd>F5</kbd>. To prevent the client from exiting as soon as it finishes its execution, you can add the `--wait` command line argument in the project's debug settings.

To enable step-through debugging, right click on `AppInstallerCLIPackage` in the Solution Explorer, select Properties, and navigate to the Debug tab. In the Debugger type selection, change "Application process" and "Background task process" to "Native Only". This will allow you to add breakpoints and step through the code. The main entry point for the client is in `src/AppInstallerCLI/main.cpp`

## Running Unit Tests

The unit tests are located inside the `AppInstallerCLITests` project. When the solution is built, all tests are compiled under `src/<ARCHITECTURE>/<Debug|Release>/AppInstallerCLITests`. An executable `AppInstallerCLITests.exe` is generated in this directory to run the tests. Run `AppInstallerCLITests.exe` from the command line to execute the tests. To see all available options, run `AppInstallerCLITests.exe --help`.

> [!TIP]
> If you just want to run a particular test, you can specify the test name as an argument to the executable. For example, `AppInstallerCLITests.exe EnsureSortedErrorList`.
